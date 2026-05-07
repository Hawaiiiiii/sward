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
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{
    // Stable, host-side store of override strings keyed by the original
    // literal value. The keys also live here as std::string so they are
    // safe to refer to via std::string_view from g_locale.
    static std::unordered_map<std::string, std::string> g_overrides;

    // Lazy guest-heap copies for retail SetText hooks. The cache is keyed
    // by the override value (not the original key) so two different
    // originals that map to the same override share one allocation.
    static std::unordered_map<std::string, uint32_t> g_guestStringCache;
    static std::mutex g_guestStringCacheMutex;

    // Phase 367: host-side hit dedup set. Each override key is emitted
    // at most once per process boot via the bridge. Mirrors the existing
    // guest-side cache semantics but is keyed by the override key
    // (not value), since Localise() callers ask for keys.
    static std::unordered_set<std::string> g_hostHitSet;
    static std::mutex g_hostHitMutex;

    // Phase 369B: scoped text override rules. Each rule replaces a
    // literal only when at least one currently-active CSD project
    // name contains the rule's `csd_project_substring`. Rules are
    // evaluated in declaration order; the first matching rule wins.
    struct ScopedTextRule
    {
        std::string literal;
        std::string projectSubstring;
        std::string replacement;
    };
    static std::vector<ScopedTextRule> g_scopedRules;

    // Phase 369B: scoped match dedup set. Keyed by `<literal>@<scope>`
    // so the same literal hit through two different scope rules emits
    // two distinct events.
    static std::unordered_set<std::string> g_scopedHitSet;
    static std::mutex g_scopedHitMutex;

    // Phase 369B: active CSD project tracker. Set is appended-to by
    // `MarkCsdProjectActive` (called from ui_lab_patches at every
    // CSD project make) and never shrinks during the process's life
    // -- retail SU rarely unloads CSD projects mid-session, and the
    // dedup-set semantics of the scope match make a one-way set
    // safer than maintaining add/remove balance through every retail
    // hook callsite.
    static std::unordered_set<std::string> g_activeCsdProjects;
    static std::mutex g_activeCsdProjectsMutex;

    static std::atomic<bool> g_loaded{false};
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

    static void DoLoad()
    {
        const auto overrideDir = ResolveOverrideDir();
        if (overrideDir.empty()) return;

        // Phase 370B: when sgfx_pack.json is present, the pack is
        // authoritative -- the loader uses the pack-pointed manifest
        // path and does NOT fall back to the legacy flat file. A
        // pack with no `text_overrides` key means "no text overrides
        // in this pack"; the loader exits without loading anything.
        // Only when the pack is absent do we read the legacy flat
        // `sg_text_overrides.json`.
        std::filesystem::path manifest;
        if (SGPack::IsActive())
        {
            manifest = SGPack::TryGetTextOverridesPath();
            if (manifest.empty())
            {
                LOGF_IMPL(Utility, "SG-Preflight",
                          "text overrides: pack active with no text_overrides; "
                          "skipping flat-file fallback");
                UiLab::EmitBridgeScreenEntered("Text:OverridesLoaded:0");
                UiLab::EmitBridgeScreenEntered("Text:ScopedRulesLoaded:0");
                return;
            }
        }
        else
        {
            manifest = overrideDir / "sg_text_overrides.json";
        }

        std::error_code ec;
        if (!std::filesystem::is_regular_file(manifest, ec)) return;

        std::ifstream stream(manifest, std::ios::binary);
        if (!stream.is_open()) return;

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
            return;
        }

        if (!doc.is_object()) return;

        size_t loaded = 0;
        const auto stringsIt = doc.find("strings");
        if (stringsIt != doc.end() && stringsIt->is_object())
        {
            for (const auto& [k, v] : stringsIt->items())
            {
                if (!v.is_string()) continue;
                g_overrides.emplace(k, v.get<std::string>());
                ++loaded;
            }
        }

        // Phase 369B: parse `scoped_rules` array. v1 packs without
        // this key still work -- we just leave g_scopedRules empty.
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
                g_scopedRules.push_back(std::move(entry));
                ++scopedLoaded;
            }
        }

        // Apply overrides to g_locale so Localise() returns the override
        // for any matching key. We insert the same value for every
        // language so the override wins regardless of Config::Language.
        for (const auto& [key, value] : g_overrides)
        {
            auto& langMap = g_locale[std::string_view(key)];
            for (auto lang : { ELanguage::English, ELanguage::Japanese,
                               ELanguage::German, ELanguage::French,
                               ELanguage::Spanish, ELanguage::Italian })
            {
                langMap[lang] = value;
            }
        }

        LOGF_IMPL(Utility, "SG-Preflight",
                  "text overrides: loaded {} string(s) + {} scoped rule(s) from \"{}\"",
                  loaded,
                  scopedLoaded,
                  reinterpret_cast<const char*>(manifest.u8string().c_str()));

        // Phase 364: runtime-prove the loader fired by emitting one
        // bridge event with the loaded count baked into the screen
        // identifier. The daemon will see e.g. screen_entered:
        //   "Text:OverridesLoaded:6". Bounded volume: exactly one
        // event per process boot.
        UiLab::EmitBridgeScreenEntered(
            "Text:OverridesLoaded:" + std::to_string(loaded));

        // Phase 369B: separate marker for scoped rule count so a
        // bridge consumer can distinguish "v1 pack, no scopes" from
        // "v2 pack, N scoped rules". Always emitted (even when zero)
        // so consumers can detect whether the manifest understood
        // the v2 schema vs. silently ignored the `scoped_rules` key.
        UiLab::EmitBridgeScreenEntered(
            "Text:ScopedRulesLoaded:" + std::to_string(scopedLoaded));
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

    static const ScopedTextRule* FindMatchingScopedRule(std::string_view original)
    {
        for (const auto& rule : g_scopedRules)
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
            DoLoad();
            g_loaded.store(true, std::memory_order_release);
        });
    }

    const std::string* TryGetOverride(std::string_view original)
    {
        if (!g_loaded.load(std::memory_order_acquire)) return nullptr;
        // unordered_map<std::string, ...>::find on string_view is
        // C++20 heterogenous lookup; fall back to constructing a key
        // string for portability with this map type.
        auto it = g_overrides.find(std::string(original));
        if (it == g_overrides.end()) return nullptr;
        return &it->second;
    }

    uint32_t TryGetOverrideGuestPtr(std::string_view original)
    {
        // Phase 369B: now a thin wrapper over the scoped variant so
        // both callers (Phase 364 and any new Phase 369B caller)
        // share a single allocation cache and emit policy. The out-
        // scope string is discarded here; callers that want the
        // scope context use the scoped variant directly.
        std::string scopeDiscard;
        return TryGetOverrideGuestPtrScoped(original, &scopeDiscard);
    }

    uint32_t TryGetOverrideGuestPtrScoped(std::string_view original,
                                          std::string* outScopeSubstring)
    {
        if (outScopeSubstring) outScopeSubstring->clear();
        if (!g_loaded.load(std::memory_order_acquire)) return 0;

        // Phase 369B: scoped rules are tried first because they are
        // narrower than the global lane. The first matching rule (in
        // declaration order) wins; if no rule scope-matches, fall
        // through to the v1 global map.
        const ScopedTextRule* matched = FindMatchingScopedRule(original);
        const std::string* replacement = nullptr;
        if (matched != nullptr)
        {
            replacement = &matched->replacement;
            if (outScopeSubstring) *outScopeSubstring = matched->projectSubstring;
        }
        else
        {
            const std::string* global = TryGetOverride(original);
            if (global == nullptr) return 0;
            replacement = global;
        }

        std::scoped_lock lock(g_guestStringCacheMutex);
        auto cacheIt = g_guestStringCache.find(*replacement);
        const bool freshAlloc = (cacheIt == g_guestStringCache.end());
        uint32_t guestAddr = 0;
        if (!freshAlloc)
        {
            guestAddr = cacheIt->second;
        }
        else
        {
            const size_t bytes = replacement->size() + 1;
            void* hostBuf = g_userHeap.Alloc(bytes);
            if (hostBuf == nullptr) return 0;
            std::memcpy(hostBuf, replacement->data(), replacement->size());
            static_cast<char*>(hostBuf)[replacement->size()] = '\0';
            guestAddr = g_memory.MapVirtual(hostBuf);
            g_guestStringCache.emplace(*replacement, guestAddr);
        }

        // Phase 364 / 367b / 369B: distinct emit per lane so bridge
        // consumers can tell scoped hits from global hits. Both
        // emits are deduped (per literal, or per literal+scope) so
        // event volume stays bounded.
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
            // Phase 364 dedup is on the value-cache (one emit per
            // unique replacement). Preserve that contract.
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
        // Phase 369B diagnostic: emit one bridge event per UNIQUE
        // project name so the proof script can see exactly which
        // project names retail SU registered during the run. Lets
        // the operator validate scoped-rule substrings against the
        // real names instead of guessing. Bounded volume = number of
        // distinct CSD projects loaded per process boot.
        if (inserted)
        {
            UiLab::EmitBridgeScreenEntered(
                "Text:CsdProjectActive:" + std::string(projectName));
        }
    }

    void NoteHostHitForKey(std::string_view key)
    {
        if (!g_loaded.load(std::memory_order_acquire)) return;

        // Heterogeneous-lookup-friendly path: only enter the locked
        // section if the override map has the key, and only allocate a
        // std::string for the dedup set on the actual first hit.
        if (g_overrides.find(std::string(key)) == g_overrides.end())
            return;

        {
            std::scoped_lock lock(g_hostHitMutex);
            auto [it, inserted] = g_hostHitSet.emplace(key);
            if (!inserted) return;
        }

        // Phase 367b: explicit `Text:HostOverrideHit:<key>` so the
        // host-side Localise() path is distinguishable from the
        // guest-side CSD SetText path (`Text:CsdOverrideHit:<original>`)
        // and from boot-time markers (`Text:OverridesLoaded:<count>`).
        // The dedup set above makes this exactly-once per key per boot.
        UiLab::EmitBridgeScreenEntered(
            "Text:HostOverrideHit:" + std::string(key));
    }
}
