#include "sg_asset_overrides.h"

#include <os/logger.h>
#include <patches/sg_pack.h>
#include <patches/ui_lab_patches.h>
#include <user/paths.h>

#include <array>
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
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
    struct PictureOverride
    {
        std::vector<uint8_t> bytes;
        std::filesystem::path source;
    };

    // Phase 370C: immutable snapshot. The hot path obtains a
    // `shared_ptr<const PictureSnapshot>` so a concurrent reload
    // that builds a fresh snapshot and swaps the global pointer
    // CANNOT free the bytes a caller is still reading. Old
    // snapshots stay alive on the heap until every handle that
    // referenced them is destroyed.
    struct PictureSnapshot
    {
        std::unordered_map<std::string, PictureOverride> pictures;
    };

    // The active snapshot is read under a shared_mutex (read lock
    // is cheap; write lock taken only at boot + reload). Using
    // shared_ptr-by-value-copy from the read side means the hot
    // path never holds the lock past the copy -- the copied
    // shared_ptr keeps the snapshot alive on its own.
    static std::shared_mutex g_snapshotMutex;
    static std::shared_ptr<const PictureSnapshot> g_snapshot;

    static std::unordered_set<std::string> g_pictureHits;
    static std::mutex g_pictureHitsMutex;
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

    static void EmitLoadedMarkerIfRequested(bool emitLoadedMarker, std::size_t count)
    {
        if (!emitLoadedMarker) return;
        UiLab::EmitBridgeScreenEntered(
            "Asset:PixelOverridesLoaded:" + std::to_string(count));
    }

    static std::shared_ptr<const PictureSnapshot> BuildSnapshot(bool emitLoadedMarker)
    {
        const auto overrideDir = ResolveOverrideDir();
        if (overrideDir.empty()) return std::make_shared<const PictureSnapshot>();

        // Phase 370B: when sgfx_pack.json is present, read the pack-
        // pointed manifest path. A pack with no `asset_overrides`
        // key means "no asset overrides in this pack"; emit the
        // boot marker with zero count and return an empty snapshot.
        std::filesystem::path manifest;
        if (SGPack::IsActive())
        {
            manifest = SGPack::TryGetAssetOverridesPath();
            if (manifest.empty())
            {
                LOGF_IMPL(Utility, "SG-Preflight",
                          "asset overrides: pack active with no asset_overrides; "
                          "skipping flat-file fallback");
                EmitLoadedMarkerIfRequested(emitLoadedMarker, 0);
                return std::make_shared<const PictureSnapshot>();
            }
        }
        else
        {
            manifest = overrideDir / "sg_asset_overrides.json";
        }

        std::error_code ec;
        if (!std::filesystem::is_regular_file(manifest, ec))
            return std::make_shared<const PictureSnapshot>();

        std::ifstream stream(manifest, std::ios::binary);
        if (!stream.is_open())
            return std::make_shared<const PictureSnapshot>();

        nlohmann::json doc;
        try
        {
            stream >> doc;
        }
        catch (const nlohmann::json::exception& e)
        {
            LOGF_IMPL(Utility, "SG-Preflight",
                      "asset overrides: parse error in \"{}\": {}",
                      reinterpret_cast<const char*>(manifest.u8string().c_str()),
                      e.what());
            return std::make_shared<const PictureSnapshot>();
        }

        if (!doc.is_object()) return std::make_shared<const PictureSnapshot>();
        const auto pics = doc.find("pictures");
        if (pics == doc.end() || !pics->is_object())
            return std::make_shared<const PictureSnapshot>();

        auto fresh = std::make_shared<PictureSnapshot>();
        std::size_t loaded = 0;
        for (const auto& [name, value] : pics->items())
        {
            if (!value.is_string()) continue;

            std::filesystem::path relPath(std::u8string_view(
                reinterpret_cast<const char8_t*>(value.get_ref<const std::string&>().data()),
                value.get_ref<const std::string&>().size()));
            std::filesystem::path absPath = overrideDir / relPath;

            if (!std::filesystem::is_regular_file(absPath, ec))
            {
                LOGF_IMPL(Utility, "SG-Preflight",
                          "asset overrides: missing file for \"{}\" -> \"{}\"",
                          name,
                          reinterpret_cast<const char*>(absPath.u8string().c_str()));
                continue;
            }

            std::ifstream file(absPath, std::ios::binary);
            if (!file.is_open()) continue;

            file.seekg(0, std::ios::end);
            const auto endPos = file.tellg();
            if (endPos < static_cast<std::streamoff>(4))
            {
                LOGF_IMPL(Utility, "SG-Preflight",
                          "asset overrides: \"{}\" is too small to be a DDS",
                          name);
                continue;
            }
            const auto fileSize = static_cast<std::size_t>(endPos);
            file.seekg(0, std::ios::beg);

            std::array<char, 4> magic{};
            file.read(magic.data(), magic.size());
            const bool isDdsMagic = magic[0] == 'D' && magic[1] == 'D'
                                  && magic[2] == 'S' && magic[3] == ' ';
            if (!isDdsMagic)
            {
                LOGF_IMPL(Utility, "SG-Preflight",
                          "asset overrides: \"{}\" is not raw DDS magic; "
                          "Phase 369A pixel-override path requires raw DDS",
                          name);
                continue;
            }
            file.seekg(0, std::ios::beg);

            PictureOverride entry;
            entry.bytes.resize(fileSize);
            entry.source = std::move(absPath);
            file.read(reinterpret_cast<char*>(entry.bytes.data()),
                      static_cast<std::streamsize>(fileSize));
            if (!file)
            {
                LOGF_IMPL(Utility, "SG-Preflight",
                          "asset overrides: short read on \"{}\"",
                          reinterpret_cast<const char*>(entry.source.u8string().c_str()));
                continue;
            }

            fresh->pictures.emplace(name, std::move(entry));
            ++loaded;
        }

        LOGF_IMPL(Utility, "SG-Preflight",
                  "asset overrides: loaded {} picture override(s) from \"{}\"",
                  loaded,
                  reinterpret_cast<const char*>(manifest.u8string().c_str()));

        EmitLoadedMarkerIfRequested(emitLoadedMarker, loaded);

        return fresh;
    }
}

namespace SGAssetOverrides
{
    void EnsureLoaded()
    {
        std::call_once(g_loadOnce, []
        {
            auto fresh = BuildSnapshot(/*emitLoadedMarker=*/true);
            std::unique_lock lock(g_snapshotMutex);
            g_snapshot = std::move(fresh);
            g_loaded.store(true, std::memory_order_release);
        });
    }

    void Reload()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return;

        auto fresh = BuildSnapshot(/*emitLoadedMarker=*/false);
        const std::size_t count = fresh ? fresh->pictures.size() : 0;
        {
            std::unique_lock lock(g_snapshotMutex);
            g_snapshot = std::move(fresh);
        }

        // Phase 370C: dedicated reload event so consumers can tell
        // a hot-reload from the boot-time PixelOverridesLoaded.
        // Always emitted, even when count == 0, so a "reload that
        // dropped all overrides" is still observable from the
        // bridge stream.
        UiLab::EmitBridgeScreenEntered(
            "Asset:PixelOverridesReloaded:" + std::to_string(count));
    }

    PixelOverrideHandle TryGetPixelOverride(std::string_view pictureName)
    {
        PixelOverrideHandle out;
        if (!g_loaded.load(std::memory_order_acquire)) return out;
        if (pictureName.empty()) return out;

        std::shared_ptr<const PictureSnapshot> snap;
        {
            std::shared_lock lock(g_snapshotMutex);
            snap = g_snapshot;
        }
        if (!snap) return out;

        auto it = snap->pictures.find(std::string(pictureName));
        if (it == snap->pictures.end()) return out;

        out.data      = it->second.bytes.data();
        out.size      = it->second.bytes.size();
        out.keepAlive = std::shared_ptr<const void>(
            std::move(snap), static_cast<const void*>(snap.get()));
        return out;
    }

    void NoteHitForPicture(std::string_view pictureName)
    {
        if (!g_loaded.load(std::memory_order_acquire)) return;
        if (pictureName.empty()) return;

        {
            std::scoped_lock lock(g_pictureHitsMutex);
            auto [it, inserted] = g_pictureHits.emplace(pictureName);
            if (!inserted) return;
        }

        UiLab::EmitBridgeScreenEntered(
            "Asset:PixelOverrideHit:" + std::string(pictureName));
    }
}
