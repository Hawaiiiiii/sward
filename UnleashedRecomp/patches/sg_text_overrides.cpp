#include "sg_text_overrides.h"

#include <kernel/heap.h>
#include <kernel/memory.h>
#include <user/paths.h>
#include <locale/locale.h>
#include <os/logger.h>
#include <patches/sg_pack.h>
#include <patches/ui_lab_patches.h>

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{
    // Phase 369B: scoped text override rule.
    struct ScopedTextRule
    {
        std::string literal;
        std::string projectSubstring;
        std::string replacement;
    };

    // Phase 370C: immutable snapshot of the loadable text override
    // state. Hot-path readers obtain a `shared_ptr<const TextSnapshot>`
    // and look up rules from there; a concurrent `Reload()` swaps in a
    // fresh snapshot without invalidating the in-flight readers'
    // references.
    struct TextSnapshot
    {
        // Global v1 lane.
        std::unordered_map<std::string, std::string> overrides;
        // v2 scoped lane.
        std::vector<ScopedTextRule> scopedRules;
    };

    // Snapshot is read under a shared_mutex. Hot-path readers copy
    // the shared_ptr while holding the read lock, then release the
    // lock immediately -- subsequent map/vector accesses go through
    // the local shared_ptr keep-alive without any further locking.
    static std::shared_mutex g_snapshotMutex;
    static std::shared_ptr<const TextSnapshot> g_snapshot;

    // Lazy guest-heap copies for retail SetText hooks. The cache is
    // keyed by the override VALUE (not the original key) so two
    // distinct originals that map to the same replacement share one
    // allocation. Entries are NEVER evicted -- guest pointers given
    // out by previous Reload()s must stay valid for the lifetime of
    // the process. A reload that introduces a new replacement value
    // grows the cache; a reload that drops a replacement leaves the
    // old guest copy resident (acceptable: bounded by total unique
    // replacements observed across all reloads).
    static std::unordered_map<std::string, uint32_t> g_guestStringCache;
    static std::mutex g_guestStringCacheMutex;

    // Phase 367: host-side hit dedup set. Each override key emits at
    // most once per process boot. NOT reset on Reload().
    static std::unordered_set<std::string> g_hostHitSet;
    static std::mutex g_hostHitMutex;

    // Phase 369B: scoped match dedup set. Keyed by `<literal>@<scope>`.
    // NOT reset on Reload().
    static std::unordered_set<std::string> g_scopedHitSet;
    static std::mutex g_scopedHitMutex;

    // Phase 369B: active CSD project tracker. Append-only.
    static std::unordered_set<std::string> g_activeCsdProjects;
    static std::mutex g_activeCsdProjectsMutex;

    static std::atomic<bool> g_loaded{false};
    // Phase 371C: monotonic counter incremented on every successful
    // Reload(). Read by SGQAPanel for the in-game status display.
    static std::atomic<uint64_t> g_reloadCount{0};
    static std::once_flag g_loadOnce;

    static std::filesystem::path ResolveOverrideDir()
    {
        if (const char* env = std::getenv("SG_PREFLIGHT_OVERRIDE_DIR");
            env != nullptr && env[0] != '\0')
        {
            return std::filesystem::path(std::u8string_view(
                reinterpret_cast<const char8_t*>(env)));
        }

        const auto candidate = GetUserPath() / "sg_preflight_overrides";
        std::error_code ec;
        if (std::filesystem::is_directory(candidate, ec))
            return candidate;

        return {};
    }

    // Build a fresh TextSnapshot from disk. Always returns a non-
    // null shared_ptr (an empty snapshot when no manifest is found
    // or parse fails). Emits the boot/reload count markers from
    // here so the caller decides which event prefix to use.
    static std::shared_ptr<const TextSnapshot> BuildSnapshot(bool reloadEvent,
                                                             std::size_t* outOverridesCount,
                                                             std::size_t* outScopedCount)
    {
        if (outOverridesCount) *outOverridesCount = 0;
        if (outScopedCount)    *outScopedCount    = 0;

        auto fresh = std::make_shared<TextSnapshot>();
        const auto overrideDir = ResolveOverrideDir();
        if (overrideDir.empty()) return fresh;

        // Phase 370B: pack supersedes flat-file fallback.
        std::filesystem::path manifest;
        if (SGPack::IsActive())
        {
            manifest = SGPack::TryGetTextOverridesPath();
            if (manifest.empty())
            {
                LOGF_IMPL(Utility, "SG-Preflight",
                          "text overrides: pack active with no text_overrides; "
                          "skipping flat-file fallback");
                const std::string prefix = reloadEvent ? "Text:OverridesReloaded:" : "Text:OverridesLoaded:";
                const std::string scopedPrefix = reloadEvent ? "Text:ScopedRulesReloaded:" : "Text:ScopedRulesLoaded:";
                UiLab::EmitBridgeScreenEntered(prefix + "0");
                UiLab::EmitBridgeScreenEntered(scopedPrefix + "0");
                return fresh;
            }
        }
        else
        {
            manifest = overrideDir / "sg_text_overrides.json";
        }

        std::error_code ec;
        if (!std::filesystem::is_regular_file(manifest, ec)) return fresh;

        std::ifstream stream(manifest, std::ios::binary);
        if (!stream.is_open()) return fresh;

        nlohmann::json doc;
        try
        {
            stream >> doc;
        }
        catch (const nlohmann::json::exception& e)
        {
            LOGF_IMPL(Utility, "SG-Preflight",
                      "text overrides: parse error in \"{}\": {}",
                      reinterpret_cast<const char*>(manifest.u8string().c_str()),
                      e.what());
            return fresh;
        }
        if (!doc.is_object()) return fresh;

        size_t loaded = 0;
        const auto stringsIt = doc.find("strings");
        if (stringsIt != doc.end() && stringsIt->is_object())
        {
            for (const auto& [k, v] : stringsIt->items())
            {
                if (!v.is_string()) continue;
                fresh->overrides.emplace(k, v.get<std::string>());
                ++loaded;
            }
        }

        size_t scopedLoaded = 0;
        const auto scopedIt = doc.find("scoped_rules");
        if (scopedIt != doc.end() && scopedIt->is_array())
        {
            for (const auto& rule : *scopedIt)
            {
                if (!rule.is_object()) continue;
                const auto litIt   = rule.find("literal");
                const auto scopeIt = rule.find("csd_project_substring");
                const auto replIt  = rule.find("replacement");
                if (litIt   == rule.end() || !litIt->is_string())   continue;
                if (scopeIt == rule.end() || !scopeIt->is_string()) continue;
                if (replIt  == rule.end() || !replIt->is_string())  continue;

                ScopedTextRule entry;
                entry.literal          = litIt->get<std::string>();
                entry.projectSubstring = scopeIt->get<std::string>();
                entry.replacement      = replIt->get<std::string>();
                if (entry.literal.empty() || entry.projectSubstring.empty())
                    continue;
                fresh->scopedRules.push_back(std::move(entry));
                ++scopedLoaded;
            }
        }

        if (outOverridesCount) *outOverridesCount = loaded;
        if (outScopedCount)    *outScopedCount    = scopedLoaded;

        LOGF_IMPL(Utility, "SG-Preflight",
                  "text overrides: {} {} string(s) + {} scoped rule(s) from \"{}\"",
                  reloadEvent ? "reloaded" : "loaded",
                  loaded,
                  scopedLoaded,
                  reinterpret_cast<const char*>(manifest.u8string().c_str()));

        const std::string prefix       = reloadEvent ? "Text:OverridesReloaded:" : "Text:OverridesLoaded:";
        const std::string scopedPrefix = reloadEvent ? "Text:ScopedRulesReloaded:" : "Text:ScopedRulesLoaded:";
        UiLab::EmitBridgeScreenEntered(prefix + std::to_string(loaded));
        UiLab::EmitBridgeScreenEntered(scopedPrefix + std::to_string(scopedLoaded));

        return fresh;
    }

    // Apply the snapshot's overrides to the host-side g_locale map.
    // Boot-time only -- a Reload() does not roll back prior writes
    // (g_locale is not undo-able), so a removed override at reload
    // time keeps its previously-applied locale value. New overrides
    // added at reload do NOT propagate to g_locale; the
    // documentation calls this out explicitly.
    static void ApplyToLocale(const TextSnapshot& snap)
    {
        for (const auto& [key, value] : snap.overrides)
        {
            auto& langMap = g_locale[std::string_view(key)];
            for (auto lang : { ELanguage::English, ELanguage::Japanese,
                               ELanguage::German, ELanguage::French,
                               ELanguage::Spanish, ELanguage::Italian })
            {
                langMap[lang] = value;
            }
        }
    }

    static std::shared_ptr<const TextSnapshot> AcquireSnapshot()
    {
        std::shared_lock lock(g_snapshotMutex);
        return g_snapshot;
    }

    static bool ScopeMatchesAnyActiveProject(const std::string& projectSubstring)
    {
        std::scoped_lock lock(g_activeCsdProjectsMutex);
        for (const auto& projectName : g_activeCsdProjects)
        {
            if (projectName.find(projectSubstring) != std::string::npos)
                return true;
        }
        return false;
    }

    static const ScopedTextRule* FindMatchingScopedRule(
        const TextSnapshot& snap, std::string_view original)
    {
        for (const auto& rule : snap.scopedRules)
        {
            if (rule.literal != original) continue;
            if (ScopeMatchesAnyActiveProject(rule.projectSubstring))
                return &rule;
        }
        return nullptr;
    }
}

namespace SGTextOverrides
{
    void EnsureLoaded()
    {
        std::call_once(g_loadOnce, []
        {
            std::size_t loaded = 0;
            std::size_t scoped = 0;
            auto fresh = BuildSnapshot(/*reloadEvent=*/false, &loaded, &scoped);
            ApplyToLocale(*fresh);
            {
                std::unique_lock lock(g_snapshotMutex);
                g_snapshot = std::move(fresh);
            }
            g_loaded.store(true, std::memory_order_release);
        });
    }

    void Reload()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return;

        std::size_t loaded = 0;
        std::size_t scoped = 0;
        auto fresh = BuildSnapshot(/*reloadEvent=*/true, &loaded, &scoped);
        // Apply the new manifest's overrides to g_locale on top of
        // any prior writes. Removed entries are NOT undone (the
        // header documents this); the next reload that re-adds the
        // entry will overwrite g_locale again.
        ApplyToLocale(*fresh);
        {
            std::unique_lock lock(g_snapshotMutex);
            g_snapshot = std::move(fresh);
        }
        g_reloadCount.fetch_add(1, std::memory_order_acq_rel);
    }

    uint64_t GetReloadCount()
    {
        return g_reloadCount.load(std::memory_order_acquire);
    }

    bool TryGetOverride(std::string_view original, std::string* outOverride)
    {
        if (outOverride == nullptr) return false;
        outOverride->clear();
        if (!g_loaded.load(std::memory_order_acquire)) return false;
        auto snap = AcquireSnapshot();
        if (!snap) return false;
        auto it = snap->overrides.find(std::string(original));
        if (it == snap->overrides.end()) return false;
        *outOverride = it->second;
        return true;
    }

    uint32_t TryGetOverrideGuestPtr(std::string_view original)
    {
        std::string scopeDiscard;
        return TryGetOverrideGuestPtrScoped(original, &scopeDiscard);
    }

    uint32_t TryGetOverrideGuestPtrScoped(std::string_view original,
                                          std::string* outScopeSubstring)
    {
        if (outScopeSubstring) outScopeSubstring->clear();
        if (!g_loaded.load(std::memory_order_acquire)) return 0;

        auto snap = AcquireSnapshot();
        if (!snap) return 0;

        const ScopedTextRule* matched = FindMatchingScopedRule(*snap, original);
        const std::string* replacement = nullptr;
        if (matched != nullptr)
        {
            replacement = &matched->replacement;
            if (outScopeSubstring) *outScopeSubstring = matched->projectSubstring;
        }
        else
        {
            auto it = snap->overrides.find(std::string(original));
            if (it == snap->overrides.end()) return 0;
            replacement = &it->second;
        }

        // Copy the replacement OUT of the snapshot before we drop
        // our shared_ptr ref. The guest-string cache key must
        // outlive the snapshot, and the guest copy itself lives
        // on g_userHeap which is process-lifetime.
        const std::string replacementCopy = *replacement;

        std::scoped_lock lock(g_guestStringCacheMutex);
        auto cacheIt = g_guestStringCache.find(replacementCopy);
        const bool freshAlloc = (cacheIt == g_guestStringCache.end());
        uint32_t guestAddr = 0;
        if (!freshAlloc)
        {
            guestAddr = cacheIt->second;
        }
        else
        {
            const size_t bytes = replacementCopy.size() + 1;
            void* hostBuf = g_userHeap.Alloc(bytes);
            if (hostBuf == nullptr) return 0;
            std::memcpy(hostBuf, replacementCopy.data(), replacementCopy.size());
            static_cast<char*>(hostBuf)[replacementCopy.size()] = '\0';
            guestAddr = g_memory.MapVirtual(hostBuf);
            g_guestStringCache.emplace(replacementCopy, guestAddr);
        }

        if (matched != nullptr)
        {
            const std::string scopedKey =
                std::string(original) + "@" + matched->projectSubstring;
            std::scoped_lock lockHits(g_scopedHitMutex);
            if (g_scopedHitSet.emplace(scopedKey).second)
            {
                UiLab::EmitBridgeScreenEntered(
                    "Text:CsdScopedOverrideHit:" + scopedKey);
            }
        }
        else
        {
            if (freshAlloc)
            {
                UiLab::EmitBridgeScreenEntered(
                    "Text:CsdOverrideHit:" + std::string(original));
            }
        }
        return guestAddr;
    }

    void MarkCsdProjectActive(std::string_view projectName)
    {
        if (projectName.empty()) return;
        bool inserted = false;
        {
            std::scoped_lock lock(g_activeCsdProjectsMutex);
            inserted = g_activeCsdProjects.emplace(projectName).second;
        }
        if (inserted)
        {
            UiLab::EmitBridgeScreenEntered(
                "Text:CsdProjectActive:" + std::string(projectName));
        }
    }

    void NoteHostHitForKey(std::string_view key)
    {
        if (!g_loaded.load(std::memory_order_acquire)) return;

        auto snap = AcquireSnapshot();
        if (!snap) return;
        if (snap->overrides.find(std::string(key)) == snap->overrides.end())
            return;

        {
            std::scoped_lock lock(g_hostHitMutex);
            auto [it, inserted] = g_hostHitSet.emplace(key);
            if (!inserted) return;
        }

        UiLab::EmitBridgeScreenEntered(
            "Text:HostOverrideHit:" + std::string(key));
    }
}
