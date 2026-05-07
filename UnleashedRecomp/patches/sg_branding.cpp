#include "sg_branding.h"

#include <os/logger.h>
#include <patches/ui_lab_patches.h>
#include <user/paths.h>

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace
{
    static std::string g_windowTitle;
    static std::string g_buildLabel;
    static std::filesystem::path g_iconPath;
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

    static bool ResolveOverrideFile(const std::filesystem::path& overrideDir,
                                    const std::filesystem::path& relativePath,
                                    std::filesystem::path& outPath)
    {
        if (overrideDir.empty() || relativePath.empty() || relativePath.is_absolute())
            return false;

        std::error_code ec;
        const auto base = std::filesystem::weakly_canonical(overrideDir, ec);
        if (ec) return false;

        const auto candidate = std::filesystem::weakly_canonical(base / relativePath, ec);
        if (ec || !std::filesystem::is_regular_file(candidate, ec))
            return false;

        const auto rel = std::filesystem::relative(candidate, base, ec);
        if (ec || rel.empty())
            return false;

        for (const auto& part : rel)
        {
            if (part == "..")
                return false;
        }

        outPath = candidate;
        return true;
    }

    static std::string TrimSeparators(std::string s)
    {
        // Replace pipe characters in user-provided strings so the
        // bridge event encoding stays unambiguous: the emit packs
        // title|icon|label into one screen field with `|`-delimited
        // sub-fields, so a literal `|` inside any field would break
        // the consumer parser.
        for (char& c : s)
        {
            if (c == '|') c = '_';
        }
        return s;
    }

    static void DoLoad()
    {
        // Phase 370A: env vars take precedence over pack values.
        // The launcher / runner sets the env explicitly when the
        // operator wants a per-launch branding without editing the
        // pack on disk.
        if (const char* env = std::getenv("SGFX_SHELL_WINDOW_TITLE");
            env != nullptr && env[0] != '\0')
        {
            g_windowTitle = env;
        }
        if (const char* env = std::getenv("SGFX_SHELL_BUILD_LABEL");
            env != nullptr && env[0] != '\0')
        {
            g_buildLabel = env;
        }

        const auto overrideDir = ResolveOverrideDir();
        std::filesystem::path iconCandidate;

        if (!overrideDir.empty())
        {
            const auto pack = overrideDir / "sgfx_pack.json";
            std::error_code ec;
            if (std::filesystem::is_regular_file(pack, ec))
            {
                std::ifstream stream(pack, std::ios::binary);
                if (stream.is_open())
                {
                    nlohmann::json doc;
                    try
                    {
                        stream >> doc;
                    }
                    catch (const nlohmann::json::exception& e)
                    {
                        LOGF_IMPL(Utility, "SG-Preflight",
                                  "branding: parse error in \"{}\": {}",
                                  reinterpret_cast<const char*>(pack.u8string().c_str()),
                                  e.what());
                    }
                    if (doc.is_object())
                    {
                        auto bIt = doc.find("branding");
                        if (bIt != doc.end() && bIt->is_object())
                        {
                            // Pack values fill in only when the env
                            // var was not already set.
                            if (g_windowTitle.empty())
                            {
                                if (auto t = bIt->find("window_title");
                                    t != bIt->end() && t->is_string())
                                    g_windowTitle = t->get<std::string>();
                            }
                            if (g_buildLabel.empty())
                            {
                                if (auto b = bIt->find("build_label");
                                    b != bIt->end() && b->is_string())
                                    g_buildLabel = b->get<std::string>();
                            }
                            if (auto i = bIt->find("icon");
                                i != bIt->end() && i->is_string())
                            {
                                std::filesystem::path rel(std::u8string_view(
                                    reinterpret_cast<const char8_t*>(i->get_ref<const std::string&>().data()),
                                    i->get_ref<const std::string&>().size()));
                                if (!ResolveOverrideFile(overrideDir, rel, iconCandidate))
                                {
                                    LOGF_IMPL(Utility, "SG-Preflight",
                                              "branding: rejected missing or out-of-pack icon path \"{}\"",
                                              i->get<std::string>());
                                }
                            }
                        }
                    }
                }
            }

            if (iconCandidate.empty())
            {
                for (const char* name : { "sgfx_branding/icon.png",
                                          "sgfx_branding/icon.bmp" })
                {
                    if (ResolveOverrideFile(overrideDir, name, iconCandidate))
                        break;
                }
            }
        }

        if (!iconCandidate.empty())
        {
            std::error_code ec;
            g_iconPath = std::filesystem::weakly_canonical(iconCandidate, ec);
            if (ec)
                g_iconPath = iconCandidate;
        }

        const bool anyActive =
            !g_windowTitle.empty() || !g_buildLabel.empty() || !g_iconPath.empty();

        if (anyActive)
        {
            const std::string iconTag = g_iconPath.empty()
                ? std::string()
                : g_iconPath.filename().string();

            LOGF_IMPL(Utility, "SG-Preflight",
                      "branding active: title=\"{}\" icon=\"{}\" label=\"{}\"",
                      g_windowTitle,
                      iconTag,
                      g_buildLabel);

            UiLab::EmitBridgeScreenEntered(
                "Branding:Active:" +
                TrimSeparators(g_windowTitle) + "|" +
                TrimSeparators(iconTag)       + "|" +
                TrimSeparators(g_buildLabel));
        }
    }
}

namespace SGBranding
{
    void EnsureLoaded()
    {
        std::call_once(g_loadOnce, []
        {
            DoLoad();
            g_loaded.store(true, std::memory_order_release);
        });
    }

    const std::string* TryGetWindowTitle()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return nullptr;
        return g_windowTitle.empty() ? nullptr : &g_windowTitle;
    }

    const std::string* TryGetBuildLabel()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return nullptr;
        return g_buildLabel.empty() ? nullptr : &g_buildLabel;
    }

    std::filesystem::path TryGetIconPath()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return {};
        return g_iconPath;
    }
}
