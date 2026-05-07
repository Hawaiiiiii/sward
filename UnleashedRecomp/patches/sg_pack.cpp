#include "sg_pack.h"

#include <os/logger.h>
#include <patches/ui_lab_patches.h>
#include <user/paths.h>

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{
    static std::atomic<bool> g_loaded{false};
    static std::once_flag g_loadOnce;

    static bool g_active = false;
    static std::filesystem::path g_textOverridesPath;
    static std::filesystem::path g_assetOverridesPath;
    static std::vector<std::filesystem::path> g_looseFiles;

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

    static void EmitRejected(std::string_view reason, std::string_view raw)
    {
        UiLab::EmitBridgeScreenEntered(
            "Pack:Rejected:" + std::string(reason) + ":" + std::string(raw));
        LOGF_IMPL(Utility, "SG-Preflight",
                  "pack: rejected path \"{}\" ({})",
                  std::string(raw), std::string(reason));
    }

    static std::filesystem::path FromJsonPath(const std::string& s)
    {
        return std::filesystem::path(std::u8string_view(
            reinterpret_cast<const char8_t*>(s.data()), s.size()));
    }
}

namespace SGPack
{
    bool ResolveRelativeUnderBase(const std::filesystem::path& base,
                                  const std::filesystem::path& relative,
                                  std::filesystem::path& outPath)
    {
        if (base.empty() || relative.empty()) return false;
        if (relative.is_absolute()) return false;

        // Guard 1: explicit `..` components in the input. Catches
        // "../escape" before path resolution even runs.
        for (const auto& part : relative)
        {
            if (part == "..") return false;
        }

        std::error_code ec;
        const auto canonicalBase = std::filesystem::weakly_canonical(base, ec);
        if (ec) return false;

        const auto candidate =
            std::filesystem::weakly_canonical(canonicalBase / relative, ec);
        if (ec) return false;
        if (!std::filesystem::is_regular_file(candidate, ec)) return false;

        // Guard 2: the canonical candidate must still live under
        // the canonical base. `relative()` returns "..." prefixed
        // segments when the result escapes; reject any such case.
        const auto rel = std::filesystem::relative(candidate, canonicalBase, ec);
        if (ec || rel.empty()) return false;
        for (const auto& part : rel)
        {
            if (part == "..") return false;
        }

        outPath = candidate;
        return true;
    }
}

namespace
{
    static void DoLoad()
    {
        const auto overrideDir = ResolveOverrideDir();
        if (overrideDir.empty()) return;

        const auto pack = overrideDir / "sgfx_pack.json";
        std::error_code ec;
        if (!std::filesystem::is_regular_file(pack, ec)) return;

        std::ifstream stream(pack, std::ios::binary);
        if (!stream.is_open()) return;

        nlohmann::json doc;
        try
        {
            stream >> doc;
        }
        catch (const nlohmann::json::exception& e)
        {
            LOGF_IMPL(Utility, "SG-Preflight",
                      "pack: parse error in \"{}\": {}",
                      reinterpret_cast<const char*>(pack.u8string().c_str()),
                      e.what());
            return;
        }
        if (!doc.is_object()) return;

        // Mark active even when the pack carries only a `branding`
        // section -- the lane loaders need this signal to know they
        // must NOT auto-load the legacy flat files when a pack is
        // present. Pack-only-branding == "no text/asset overrides"
        // is a valid configuration, not a fall-through.
        g_active = true;

        if (auto t = doc.find("text_overrides"); t != doc.end() && t->is_string())
        {
            const auto raw = t->get<std::string>();
            std::filesystem::path resolved;
            if (SGPack::ResolveRelativeUnderBase(overrideDir, FromJsonPath(raw), resolved))
                g_textOverridesPath = std::move(resolved);
            else
                EmitRejected("text_overrides", raw);
        }

        if (auto a = doc.find("asset_overrides"); a != doc.end() && a->is_string())
        {
            const auto raw = a->get<std::string>();
            std::filesystem::path resolved;
            if (SGPack::ResolveRelativeUnderBase(overrideDir, FromJsonPath(raw), resolved))
                g_assetOverridesPath = std::move(resolved);
            else
                EmitRejected("asset_overrides", raw);
        }

        if (auto l = doc.find("loose_files"); l != doc.end() && l->is_array())
        {
            for (const auto& entry : *l)
            {
                if (!entry.is_string()) continue;
                const auto raw = entry.get<std::string>();
                std::filesystem::path resolved;
                if (SGPack::ResolveRelativeUnderBase(overrideDir, FromJsonPath(raw), resolved))
                    g_looseFiles.push_back(std::move(resolved));
                else
                    EmitRejected("loose_files", raw);
            }
        }

        const std::string textTag  = g_textOverridesPath.empty()  ? "none" : g_textOverridesPath.filename().string();
        const std::string assetTag = g_assetOverridesPath.empty() ? "none" : g_assetOverridesPath.filename().string();

        LOGF_IMPL(Utility, "SG-Preflight",
                  "pack: loaded \"{}\"; text=\"{}\" asset=\"{}\" loose_files={}",
                  reinterpret_cast<const char*>(pack.u8string().c_str()),
                  textTag,
                  assetTag,
                  g_looseFiles.size());

        UiLab::EmitBridgeScreenEntered(
            "Pack:Loaded:" + textTag + ":" + assetTag +
            ":" + std::to_string(g_looseFiles.size()));
    }
}

namespace SGPack
{
    void EnsureLoaded()
    {
        std::call_once(g_loadOnce, []
        {
            DoLoad();
            g_loaded.store(true, std::memory_order_release);
        });
    }

    bool IsActive()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return false;
        return g_active;
    }

    std::filesystem::path TryGetTextOverridesPath()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return {};
        return g_textOverridesPath;
    }

    std::filesystem::path TryGetAssetOverridesPath()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return {};
        return g_assetOverridesPath;
    }

    const std::vector<std::filesystem::path>& GetLooseFiles()
    {
        return g_looseFiles;
    }
}
