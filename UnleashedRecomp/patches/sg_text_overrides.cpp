#include "sg_text_overrides.h"

#include <kernel/heap.h>
#include <kernel/memory.h>
#include <user/paths.h>
#include <locale/locale.h>
#include <os/logger.h>
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

        const auto manifest = overrideDir / "sg_text_overrides.json";
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
        const auto stringsIt = doc.find("strings");
        if (stringsIt == doc.end() || !stringsIt->is_object()) return;

        size_t loaded = 0;
        for (const auto& [k, v] : stringsIt->items())
        {
            if (!v.is_string()) continue;
            g_overrides.emplace(k, v.get<std::string>());
            ++loaded;
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
                  "text overrides: loaded {} string(s) from \"{}\"",
                  loaded,
                  reinterpret_cast<const char*>(manifest.u8string().c_str()));

        // Phase 364: runtime-prove the loader fired by emitting one
        // bridge event with the loaded count baked into the screen
        // identifier. The daemon will see e.g. screen_entered:
        //   "Text:OverridesLoaded:6". Bounded volume: exactly one
        // event per process boot.
        UiLab::EmitBridgeScreenEntered(
            "Text:OverridesLoaded:" + std::to_string(loaded));

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
        const std::string* override = TryGetOverride(original);
        if (override == nullptr) return 0;

        std::scoped_lock lock(g_guestStringCacheMutex);
        auto cacheIt = g_guestStringCache.find(*override);
        if (cacheIt != g_guestStringCache.end()) return cacheIt->second;

        const size_t bytes = override->size() + 1;
        void* hostBuf = g_userHeap.Alloc(bytes);
        if (hostBuf == nullptr) return 0;
        std::memcpy(hostBuf, override->data(), override->size());
        static_cast<char*>(hostBuf)[override->size()] = '\0';

        const uint32_t guestAddr = g_memory.MapVirtual(hostBuf);
        g_guestStringCache.emplace(*override, guestAddr);

        // Phase 364 / 367b: emit a bridge event the first time each
        // override is resolved into guest memory. Phase 367b renames
        // the event from the original `Text:OverrideHit:<original>`
        // to `Text:CsdOverrideHit:<original>` so it is distinguishable
        // from host-side `Text:HostOverrideHit:<key>` (Localise path)
        // and from any boot-time loader probes. Bounded volume = at
        // most one event per unique original literal per process boot.
        UiLab::EmitBridgeScreenEntered(
            "Text:CsdOverrideHit:" + std::string(original));
        return guestAddr;
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
