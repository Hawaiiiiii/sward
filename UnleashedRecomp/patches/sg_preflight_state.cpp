#include "sg_preflight_state.h"

#include <os/logger.h>
#include <patches/ui_lab_patches.h>
#include <user/paths.h>

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace
{
    static std::string g_selectedProfile;
    static std::string g_sourceRoot;
    static std::string g_latestRunStatus;
    static std::string g_firstWarning;
    static std::size_t g_actionCount = 0;

    static std::atomic<bool> g_loaded{false};
    static std::once_flag g_loadOnce;

    static std::string ScrubField(std::string s)
    {
        // Bridge event encoding uses `:` and `|` as field
        // separators; scrub them out of operator-controlled
        // values so the consumer parser stays unambiguous.
        for (char& c : s)
        {
            if (c == ':' || c == '|') c = '_';
        }
        return s;
    }

    static std::filesystem::path ResolveStatePath()
    {
        // Precedence: explicit env var, then convention path
        // under the override dir. Both paths are absolute.
        if (const char* env = std::getenv("SG_PREFLIGHT_STATE_JSON");
            env != nullptr && env[0] != '\0')
        {
            return std::filesystem::path(std::u8string_view(
                reinterpret_cast<const char8_t*>(env)));
        }

        // Fallback: <SG_PREFLIGHT_OVERRIDE_DIR>/sg_preflight_state.json
        if (const char* env = std::getenv("SG_PREFLIGHT_OVERRIDE_DIR");
            env != nullptr && env[0] != '\0')
        {
            std::filesystem::path overrideDir(std::u8string_view(
                reinterpret_cast<const char8_t*>(env)));
            return overrideDir / "sg_preflight_state.json";
        }

        const auto candidate =
            GetUserPath() / "sg_preflight_overrides" / "sg_preflight_state.json";
        return candidate;
    }

    static void EmitMissing(std::string_view reason)
    {
        UiLab::EmitBridgeScreenEntered(
            "SgPreflightState:Missing:" + std::string(reason));
        LOGF_IMPL(Utility, "SG-Preflight",
                  "preflight-state: missing ({})",
                  std::string(reason));
    }

    static std::string ExtractFirstWarning(const nlohmann::json& doc)
    {
        auto it = doc.find("warnings");
        if (it == doc.end() || !it->is_array() || it->empty())
            return std::string();
        const auto& first = (*it)[0];
        if (!first.is_string()) return std::string();
        return first.get<std::string>();
    }

    static std::string ExtractLatestRunStatus(const nlohmann::json& doc)
    {
        auto it = doc.find("latest_run");
        if (it == doc.end()) return std::string();
        if (it->is_null()) return std::string();
        if (!it->is_object()) return std::string();
        auto sit = it->find("status");
        if (sit == it->end() || !sit->is_string()) return std::string();
        return sit->get<std::string>();
    }

    static void LoadOnce()
    {
        const auto path = ResolveStatePath();
        std::error_code ec;
        if (path.empty() || !std::filesystem::is_regular_file(path, ec))
        {
            EmitMissing("path_absent");
            return;
        }

        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open())
        {
            EmitMissing("open_failed");
            return;
        }

        nlohmann::json doc;
        try
        {
            stream >> doc;
        }
        catch (const nlohmann::json::exception& e)
        {
            LOGF_IMPL(Utility, "SG-Preflight",
                      "preflight-state: parse error in \"{}\": {}",
                      reinterpret_cast<const char*>(path.u8string().c_str()),
                      e.what());
            EmitMissing("parse_error");
            return;
        }
        if (!doc.is_object())
        {
            EmitMissing("not_object");
            return;
        }

        // Schema gate: accept any version of `sgfx_preflight_state`
        // for now. Future versions can be conditionally upgraded
        // here when the schema actually changes.
        if (auto sit = doc.find("schema"); sit == doc.end() ||
            !sit->is_string() || sit->get<std::string>() != "sgfx_preflight_state")
        {
            EmitMissing("schema_mismatch");
            return;
        }

        const auto readStr = [&](const char* key) -> std::string
        {
            auto it = doc.find(key);
            if (it == doc.end() || !it->is_string()) return std::string();
            return it->get<std::string>();
        };

        g_selectedProfile = ScrubField(readStr("selected_profile"));
        g_sourceRoot      = readStr("source_root");
        g_latestRunStatus = ScrubField(ExtractLatestRunStatus(doc));
        g_firstWarning    = ScrubField(ExtractFirstWarning(doc));

        std::size_t actionCount = 0;
        if (auto it = doc.find("actions"); it != doc.end() && it->is_array())
        {
            actionCount = it->size();
        }
        g_actionCount = actionCount;

        g_loaded.store(true, std::memory_order_release);

        const std::string profileTag =
            g_selectedProfile.empty() ? "_" : g_selectedProfile;

        LOGF_IMPL(Utility, "SG-Preflight",
                  "preflight-state: loaded profile=\"{}\" actions={} warnings_first=\"{}\"",
                  profileTag,
                  actionCount,
                  g_firstWarning.empty() ? "_" : g_firstWarning);

        UiLab::EmitBridgeScreenEntered(
            "SgPreflightState:Loaded:" + profileTag +
            ":" + std::to_string(actionCount));
    }
}

namespace SGPreflightState
{
    void EnsureLoaded()
    {
        std::call_once(g_loadOnce, LoadOnce);
    }

    bool IsLoaded()
    {
        return g_loaded.load(std::memory_order_acquire);
    }

    const std::string* TryGetSelectedProfile()
    {
        if (!IsLoaded() || g_selectedProfile.empty()) return nullptr;
        return &g_selectedProfile;
    }

    const std::string* TryGetSourceRoot()
    {
        if (!IsLoaded() || g_sourceRoot.empty()) return nullptr;
        return &g_sourceRoot;
    }

    const std::string* TryGetLatestRunStatus()
    {
        if (!IsLoaded() || g_latestRunStatus.empty()) return nullptr;
        return &g_latestRunStatus;
    }

    const std::string* TryGetFirstWarning()
    {
        if (!IsLoaded() || g_firstWarning.empty()) return nullptr;
        return &g_firstWarning;
    }

    std::size_t GetActionCount()
    {
        if (!IsLoaded()) return 0;
        return g_actionCount;
    }
}
