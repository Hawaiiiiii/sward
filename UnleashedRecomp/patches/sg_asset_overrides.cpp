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
#include <mutex>
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

    static std::unordered_map<std::string, PictureOverride> g_pictureOverrides;
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

    static void DoLoad()
    {
        const auto overrideDir = ResolveOverrideDir();
        if (overrideDir.empty()) return;

        // Phase 370B: when sgfx_pack.json is present, the pack is
        // authoritative -- read the pack-pointed manifest path
        // instead of the legacy flat file. A pack with no
        // `asset_overrides` key means "no asset overrides in this
        // pack"; emit the boot marker with zero count and exit.
        std::filesystem::path manifest;
        if (SGPack::IsActive())
        {
            manifest = SGPack::TryGetAssetOverridesPath();
            if (manifest.empty())
            {
                LOGF_IMPL(Utility, "SG-Preflight",
                          "asset overrides: pack active with no asset_overrides; "
                          "skipping flat-file fallback");
                UiLab::EmitBridgeScreenEntered("Asset:PixelOverridesLoaded:0");
                return;
            }
        }
        else
        {
            manifest = overrideDir / "sg_asset_overrides.json";
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
                      "asset overrides: parse error in \"{}\": {}",
                      reinterpret_cast<const char*>(manifest.u8string().c_str()),
                      e.what());
            return;
        }

        if (!doc.is_object()) return;
        const auto pics = doc.find("pictures");
        if (pics == doc.end() || !pics->is_object()) return;

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
                          "asset overrides: \"{}\" is too small to be a DDS "
                          "(file at \"{}\")",
                          name,
                          reinterpret_cast<const char*>(absPath.u8string().c_str()));
                continue;
            }
            const auto fileSize = static_cast<std::size_t>(endPos);
            file.seekg(0, std::ios::beg);

            // Reject files that are not raw DDS magic. Phase 369A
            // routes pixel overrides through ddspp post-LZX-decompress;
            // an LZX-magic file at this lane would be re-decompressed
            // as garbage. Loud-fail at load time so the operator
            // knows to either swap to a raw DDS or wrap-encode.
            std::array<char, 4> magic{};
            file.read(magic.data(), magic.size());
            const bool isDdsMagic = magic[0] == 'D' && magic[1] == 'D'
                                  && magic[2] == 'S' && magic[3] == ' ';
            if (!isDdsMagic)
            {
                LOGF_IMPL(Utility, "SG-Preflight",
                          "asset overrides: \"{}\" is not raw DDS magic; "
                          "Phase 369A pixel-override path requires raw DDS "
                          "(file at \"{}\")",
                          name,
                          reinterpret_cast<const char*>(absPath.u8string().c_str()));
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

            g_pictureOverrides.emplace(name, std::move(entry));
            ++loaded;
        }

        LOGF_IMPL(Utility, "SG-Preflight",
                  "asset overrides: loaded {} picture override(s) from \"{}\"",
                  loaded,
                  reinterpret_cast<const char*>(manifest.u8string().c_str()));

        // Phase 369A boot marker. Distinct event name so consumers
        // can tell pixel-lane loads from the older
        // `Text:OverridesLoaded` text-lane marker.
        UiLab::EmitBridgeScreenEntered(
            "Asset:PixelOverridesLoaded:" + std::to_string(loaded));
    }
}

namespace SGAssetOverrides
{
    void EnsureLoaded()
    {
        std::call_once(g_loadOnce, []
        {
            DoLoad();
            g_loaded.store(true, std::memory_order_release);
        });
    }

    bool TryGetPixelOverride(std::string_view pictureName,
                             const uint8_t** outData,
                             std::size_t* outDataSize)
    {
        if (!g_loaded.load(std::memory_order_acquire)) return false;
        if (pictureName.empty() || outData == nullptr || outDataSize == nullptr)
            return false;

        auto it = g_pictureOverrides.find(std::string(pictureName));
        if (it == g_pictureOverrides.end()) return false;

        *outData = it->second.bytes.data();
        *outDataSize = it->second.bytes.size();
        return true;
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
