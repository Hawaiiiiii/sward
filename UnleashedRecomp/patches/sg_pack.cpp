#include "sg_pack.h"

#include <os/logger.h>
#include <patches/ui_lab_patches.h>
#include <user/paths.h>

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{
    // Phase 370C: immutable PackSnapshot. Hot-path readers
    // (`SGPack::IsActive`, `TryGet*Path`, `GetLooseFiles`) acquire
    // `shared_ptr<const PackSnapshot>` via the snapshot mutex; a
    // concurrent `Reload()` builds a fresh snapshot and swaps the
    // global pointer, leaving in-flight readers' references valid
    // until they go out of scope.
    struct PackSnapshot
    {
        bool active = false;
        std::filesystem::path textOverridesPath;
        std::filesystem::path assetOverridesPath;
        std::vector<std::filesystem::path> looseFiles;
    };

    static std::shared_mutex g_snapshotMutex;
    static std::shared_ptr<const PackSnapshot> g_snapshot;
    static std::atomic<bool> g_loaded{false};
    static std::once_flag g_loadOnce;

    // Sentinel returned by `GetLooseFiles()` when no snapshot has
    // been built yet OR the snapshot has no entries. Avoids a
    // dangling reference when callers iterate the vector after the
    // snapshot pointer has been swapped out under them.
    static const std::vector<std::filesystem::path> kEmptyLooseFiles{};

    // Phase 371A: pack metadata read once at EnsureLoaded() time.
    // Stored as plain strings (not behind a snapshot) because it is
    // identifier-grade data: ticket / project / phase. We do NOT
    // hot-reload these because changing the ticket mid-run would
    // make any QA evidence already captured ambiguous. If the
    // operator wants to change ticket, they restart UR.
    static std::string g_metaTicket;
    static std::string g_metaProject;
    static std::string g_metaPhase;
    static std::atomic<bool> g_metaLoaded{false};

    static std::string ScrubBridgeField(std::string s)
    {
        // Mirror SGBranding's pipe scrub: the bridge event encoding
        // uses `:` and `|` as field separators; replace both with
        // `_` so consumer parsers stay unambiguous.
        for (char& c : s)
        {
            if (c == '|' || c == ':') c = '_';
        }
        return s;
    }

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
    // Build a fresh PackSnapshot from disk. Always returns a non-
    // null shared_ptr (an empty/inactive snapshot when no pack file
    // is found or the pack doesn't parse as a JSON object). Emits
    // the `Pack:Loaded:` or `Pack:Reloaded:` event from here so the
    // caller picks the prefix.
    static std::shared_ptr<const PackSnapshot> BuildSnapshot(bool reloadEvent)
    {
        auto fresh = std::make_shared<PackSnapshot>();
        const auto overrideDir = ResolveOverrideDir();
        if (overrideDir.empty()) return fresh;

        const auto pack = overrideDir / "sgfx_pack.json";
        std::error_code ec;
        if (!std::filesystem::is_regular_file(pack, ec)) return fresh;

        std::ifstream stream(pack, std::ios::binary);
        if (!stream.is_open()) return fresh;

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
            return fresh;
        }
        if (!doc.is_object()) return fresh;

        fresh->active = true;

        if (auto t = doc.find("text_overrides"); t != doc.end() && t->is_string())
        {
            const auto raw = t->get<std::string>();
            std::filesystem::path resolved;
            if (SGPack::ResolveRelativeUnderBase(overrideDir, FromJsonPath(raw), resolved))
                fresh->textOverridesPath = std::move(resolved);
            else
                EmitRejected("text_overrides", raw);
        }

        if (auto a = doc.find("asset_overrides"); a != doc.end() && a->is_string())
        {
            const auto raw = a->get<std::string>();
            std::filesystem::path resolved;
            if (SGPack::ResolveRelativeUnderBase(overrideDir, FromJsonPath(raw), resolved))
                fresh->assetOverridesPath = std::move(resolved);
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
                    fresh->looseFiles.push_back(std::move(resolved));
                else
                    EmitRejected("loose_files", raw);
            }
        }

        const std::string textTag  = fresh->textOverridesPath.empty()  ? "none" : fresh->textOverridesPath.filename().string();
        const std::string assetTag = fresh->assetOverridesPath.empty() ? "none" : fresh->assetOverridesPath.filename().string();

        LOGF_IMPL(Utility, "SG-Preflight",
                  "pack: {} \"{}\"; text=\"{}\" asset=\"{}\" loose_files={}",
                  reloadEvent ? "reloaded" : "loaded",
                  reinterpret_cast<const char*>(pack.u8string().c_str()),
                  textTag,
                  assetTag,
                  fresh->looseFiles.size());

        const std::string prefix = reloadEvent ? "Pack:Reloaded:" : "Pack:Loaded:";
        UiLab::EmitBridgeScreenEntered(
            prefix + textTag + ":" + assetTag +
            ":" + std::to_string(fresh->looseFiles.size()));

        return fresh;
    }
}

namespace
{
    // Phase 371A: read pack_meta.json once at EnsureLoaded() time.
    // Best-effort: missing file = silent no-op. Malformed file =
    // log + no-op (no event emit, no rejection event -- the meta
    // file is purely informational and not on the override path).
    static void LoadPackMetaOnce()
    {
        const auto overrideDir = ResolveOverrideDir();
        if (overrideDir.empty()) return;

        const auto metaPath = overrideDir / "pack_meta.json";
        std::error_code ec;
        if (!std::filesystem::is_regular_file(metaPath, ec)) return;

        std::ifstream stream(metaPath, std::ios::binary);
        if (!stream.is_open()) return;

        nlohmann::json doc;
        try
        {
            stream >> doc;
        }
        catch (const nlohmann::json::exception& e)
        {
            LOGF_IMPL(Utility, "SG-Preflight",
                      "pack_meta: parse error in \"{}\": {}",
                      reinterpret_cast<const char*>(metaPath.u8string().c_str()),
                      e.what());
            return;
        }
        if (!doc.is_object()) return;

        const auto readStr = [&](const char* key) -> std::string
        {
            auto it = doc.find(key);
            if (it == doc.end() || !it->is_string()) return {};
            return ScrubBridgeField(it->get<std::string>());
        };

        g_metaTicket  = readStr("ticket");
        g_metaProject = readStr("project");
        g_metaPhase   = readStr("phase");
        g_metaLoaded.store(true, std::memory_order_release);

        const std::string ticket  = g_metaTicket.empty()  ? "_" : g_metaTicket;
        const std::string project = g_metaProject.empty() ? "_" : g_metaProject;
        const std::string phase   = g_metaPhase.empty()   ? "_" : g_metaPhase;

        LOGF_IMPL(Utility, "SG-Preflight",
                  "pack_meta: loaded ticket=\"{}\" project=\"{}\" phase=\"{}\"",
                  ticket, project, phase);

        UiLab::EmitBridgeScreenEntered(
            "Pack:Meta:" + ticket + ":" + project + ":" + phase);
    }
}

namespace SGPack
{
    void EnsureLoaded()
    {
        std::call_once(g_loadOnce, []
        {
            auto fresh = BuildSnapshot(/*reloadEvent=*/false);
            std::unique_lock lock(g_snapshotMutex);
            g_snapshot = std::move(fresh);
            g_loaded.store(true, std::memory_order_release);
            // Sibling metadata file: read once. Independent of the
            // override-lane snapshot but loaded in the same
            // EnsureLoaded call so the bridge sees a coherent
            // boot-time pack state in events.jsonl.
            LoadPackMetaOnce();
        });
    }

    const std::string* TryGetTicket()
    {
        if (!g_metaLoaded.load(std::memory_order_acquire)) return nullptr;
        if (g_metaTicket.empty()) return nullptr;
        return &g_metaTicket;
    }

    const std::string* TryGetProject()
    {
        if (!g_metaLoaded.load(std::memory_order_acquire)) return nullptr;
        if (g_metaProject.empty()) return nullptr;
        return &g_metaProject;
    }

    const std::string* TryGetPhase()
    {
        if (!g_metaLoaded.load(std::memory_order_acquire)) return nullptr;
        if (g_metaPhase.empty()) return nullptr;
        return &g_metaPhase;
    }

    void Reload()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return;
        auto fresh = BuildSnapshot(/*reloadEvent=*/true);
        std::unique_lock lock(g_snapshotMutex);
        g_snapshot = std::move(fresh);
    }

    bool IsActive()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return false;
        std::shared_lock lock(g_snapshotMutex);
        return g_snapshot && g_snapshot->active;
    }

    std::filesystem::path TryGetTextOverridesPath()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return {};
        std::shared_lock lock(g_snapshotMutex);
        if (!g_snapshot) return {};
        return g_snapshot->textOverridesPath;
    }

    std::filesystem::path TryGetAssetOverridesPath()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return {};
        std::shared_lock lock(g_snapshotMutex);
        if (!g_snapshot) return {};
        return g_snapshot->assetOverridesPath;
    }

    const std::vector<std::filesystem::path>& GetLooseFiles()
    {
        if (!g_loaded.load(std::memory_order_acquire)) return kEmptyLooseFiles;
        std::shared_lock lock(g_snapshotMutex);
        if (!g_snapshot) return kEmptyLooseFiles;
        // Return by const ref through the snapshot. The caller
        // copies if it needs to outlive a concurrent reload; in
        // practice the only caller (mod_loader's
        // `IndexSgPreflightLooseOverrides`) iterates immediately
        // and does not retain a reference past the call, so this
        // is safe.
        return g_snapshot->looseFiles;
    }
}
