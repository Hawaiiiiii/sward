#include "sg_hot_reload.h"

#include <os/logger.h>
#include <patches/sg_asset_overrides.h>
#include <patches/sg_pack.h>
#include <patches/sg_text_overrides.h>
#include <patches/ui_lab_patches.h>
#include <user/paths.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

namespace
{
    static std::atomic<bool> g_running{false};
    static std::atomic<bool> g_stop{false};
    static std::thread       g_thread;
    static std::once_flag    g_startOnce;

    static constexpr auto kPollInterval     = std::chrono::milliseconds(500);
    static constexpr auto kDebounceInterval = std::chrono::milliseconds(250);

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

    static std::filesystem::file_time_type SafeMtime(const std::filesystem::path& p)
    {
        if (p.empty()) return std::filesystem::file_time_type::min();
        std::error_code ec;
        auto t = std::filesystem::last_write_time(p, ec);
        if (ec) return std::filesystem::file_time_type::min();
        return t;
    }

    struct WatchedFile
    {
        std::filesystem::path path;
        std::filesystem::file_time_type lastMtime = std::filesystem::file_time_type::min();
        std::filesystem::file_time_type pendingMtime = std::filesystem::file_time_type::min();
        bool pending = false;
        std::chrono::steady_clock::time_point pendingSince{};
    };

    enum class WatchKind { Pack, Text, Asset };

    static bool PollOne(WatchedFile& w,
                        std::chrono::steady_clock::time_point now)
    {
        const auto mtime = SafeMtime(w.path);
        if (w.pending)
        {
            // Debouncing in progress. Re-stat after the debounce
            // window expires; only fire if the mtime is stable
            // across the window.
            if (now - w.pendingSince < kDebounceInterval) return false;
            if (mtime == w.pendingMtime && mtime != w.lastMtime)
            {
                w.lastMtime = mtime;
                w.pending   = false;
                return true;
            }
            // mtime moved again during the debounce window. Keep
            // pending with the new mtime so the next stable
            // observation triggers.
            w.pendingMtime = mtime;
            w.pendingSince = now;
            return false;
        }
        if (mtime != w.lastMtime && mtime != std::filesystem::file_time_type::min())
        {
            w.pending = true;
            w.pendingMtime = mtime;
            w.pendingSince = now;
        }
        return false;
    }

    static void DispatchReload(WatchKind kind, const std::filesystem::path& path)
    {
        const auto pathStr = std::filesystem::path(path).filename().string();
        LOGF_IMPL(Utility, "SG-Preflight",
                  "hot-reload: triggered by \"{}\"",
                  pathStr);

        switch (kind)
        {
            case WatchKind::Pack:
                // Pack reload first; the text/asset paths it
                // declares may have changed, so we re-issue text
                // and asset reloads against the fresh pack state.
                SGPack::Reload();
                SGTextOverrides::Reload();
                SGAssetOverrides::Reload();
                break;
            case WatchKind::Text:
                SGTextOverrides::Reload();
                break;
            case WatchKind::Asset:
                SGAssetOverrides::Reload();
                break;
        }
    }

    static void RefreshWatchPaths(WatchedFile& packW,
                                  WatchedFile& textW,
                                  WatchedFile& assetW,
                                  const std::filesystem::path& overrideDir)
    {
        // Pack path is fixed at <override>/sgfx_pack.json.
        const auto packPath = overrideDir / "sgfx_pack.json";
        if (packW.path != packPath)
        {
            packW.path = packPath;
            packW.lastMtime = SafeMtime(packPath);
        }

        // Text/asset paths may be pack-pointed or flat. Re-resolve
        // both on every refresh so a pack reload that changed the
        // text path swaps which file the watcher is polling.
        std::filesystem::path textPath;
        if (SGPack::IsActive())
            textPath = SGPack::TryGetTextOverridesPath();
        else
            textPath = overrideDir / "sg_text_overrides.json";
        if (textW.path != textPath)
        {
            textW.path = textPath;
            textW.lastMtime = SafeMtime(textPath);
            textW.pending = false;
        }

        std::filesystem::path assetPath;
        if (SGPack::IsActive())
            assetPath = SGPack::TryGetAssetOverridesPath();
        else
            assetPath = overrideDir / "sg_asset_overrides.json";
        if (assetW.path != assetPath)
        {
            assetW.path = assetPath;
            assetW.lastMtime = SafeMtime(assetPath);
            assetW.pending = false;
        }
    }

    static void WatcherLoop()
    {
        const auto overrideDir = ResolveOverrideDir();
        if (overrideDir.empty())
        {
            LOGF_IMPL(Utility, "SG-Preflight",
                      "hot-reload: no override dir; watcher exiting");
            g_running.store(false, std::memory_order_release);
            return;
        }

        WatchedFile packW;
        WatchedFile textW;
        WatchedFile assetW;
        RefreshWatchPaths(packW, textW, assetW, overrideDir);

        // Phase 370C boot marker for the watcher itself. Bridge
        // consumers can prove the watcher actually started without
        // racing to detect the first reload.
        UiLab::EmitBridgeScreenEntered("HotReload:WatcherStarted");

        while (!g_stop.load(std::memory_order_acquire))
        {
            std::this_thread::sleep_for(kPollInterval);
            if (g_stop.load(std::memory_order_acquire)) break;

            const auto now = std::chrono::steady_clock::now();

            if (PollOne(packW, now))
            {
                DispatchReload(WatchKind::Pack, packW.path);
                // After a pack reload the text/asset paths may
                // have changed; refresh the watch paths so the
                // watcher follows the new manifest locations.
                RefreshWatchPaths(packW, textW, assetW, overrideDir);
                continue;
            }
            if (PollOne(textW, now))
            {
                DispatchReload(WatchKind::Text, textW.path);
                continue;
            }
            if (PollOne(assetW, now))
            {
                DispatchReload(WatchKind::Asset, assetW.path);
                continue;
            }
        }
        g_running.store(false, std::memory_order_release);
    }
}

namespace SGHotReload
{
    void Start()
    {
        std::call_once(g_startOnce, []
        {
            const char* env = std::getenv("SG_PREFLIGHT_HOT_RELOAD");
            if (env == nullptr || std::string_view(env) != "1") return;

            g_stop.store(false, std::memory_order_release);
            g_running.store(true, std::memory_order_release);
            g_thread = std::thread(WatcherLoop);
        });
    }

    void Stop()
    {
        g_stop.store(true, std::memory_order_release);
        if (g_thread.joinable() &&
            g_thread.get_id() != std::this_thread::get_id())
        {
            g_thread.join();
        }
        g_running.store(false, std::memory_order_release);
    }
}
