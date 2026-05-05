// Phase 298: in-process A/B harness for the human-readable port.
//
// Runs the native C++ CSD renderer (Phase 296/297) inside the
// UnleashedRecomp process. Each time the SetPosition probe sees the
// runtime touch a CSD project for the first time, this patch fires a
// background render of that project's first scene and writes a PNG
// next to the evidence files. The result is a side-by-side
// comparison: the runtime's live render + the native port's render
// of the same retail bytes, both in the same process at the same
// frame, sharing the same asset directory.
//
// No D3D12 overlay yet (deferred to a later phase that wires the
// framebuffer through a Plume texture into ImGui). The PNG-on-disk
// path is enough to validate the in-process pipeline end-to-end.

#include <kernel/function.h>
#include <kernel/memory.h>
#include <patches/ui_lab_patches.h>

#include <sward/ui_runtime/sgfx_hud_csd_project_loader.hpp>
#include <sward/ui_runtime/sgfx_hud_csd_cast_extractor.hpp>
#include <sward/ui_runtime/sgfx_hud_native_csd_renderer.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ui = sward::ui_runtime::generated::sgfx_hud;

namespace
{
    std::mutex g_csdOverlayMutex;
    std::unordered_set<std::string> g_csdOverlayRendered;
    std::atomic<bool> g_csdOverlayThreadActive{false};

    // Asset roots scanned to find the .yncp + .dds files. Hardcoded
    // here because the UI Lab evidence dir lives outside the install
    // and we don't have a CLI flag at the per-patch granularity.
    // Mirrors the smoke-test default.
    std::filesystem::path getAssetRoot()
    {
        // Walk up from the EXE's directory to find the
        // `extracted_assets/full_install_archives` tree.
        std::error_code ec;
        std::filesystem::path cwd = std::filesystem::current_path(ec);
        for (int i = 0; i < 6 && !cwd.empty(); ++i)
        {
            std::filesystem::path candidate = cwd / "extracted_assets" / "full_install_archives";
            if (std::filesystem::exists(candidate, ec))
                return candidate;
            cwd = cwd.parent_path();
        }
        return {};
    }

    std::filesystem::path getEvidenceDir()
    {
        // The UI Lab patches expose g_evidenceDirectory but only as
        // a static; we re-derive it from the runtime args via the
        // process companion file written at launch time. Fall back
        // to a sibling 'csd_overlay' subdir of the install root.
        std::error_code ec;
        std::filesystem::path cwd = std::filesystem::current_path(ec);
        std::filesystem::path out = cwd / "out" / "csd_overlay_evidence";
        std::filesystem::create_directories(out, ec);
        return out;
    }

    void renderProjectInBackground(const std::string& projectName)
    {
        const auto assetRoot = getAssetRoot();
        if (assetRoot.empty())
            return;

        // Try matching projectName as a .yncp file under assetRoot.
        // The CSD probe doesn't ship full relative paths, so we scan
        // a handful of likely subdirectories.
        const std::vector<std::string> candidateRels{
            std::string("game/Sonic/") + projectName + ".yncp",
            std::string("game/WorldMap/") + projectName + ".yncp",
            std::string("game/SystemCommon/") + projectName + ".yncp",
            std::string("game/Title/") + projectName + ".yncp",
        };
        std::filesystem::path projectPath;
        std::string matchedRelative;
        for (const auto& rel : candidateRels)
        {
            const auto candidate = assetRoot / rel;
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec))
            {
                projectPath = candidate;
                matchedRelative = rel;
                break;
            }
        }
        if (projectPath.empty())
            return;

        const auto loaded = ui::loadCsdProjectFile(projectPath);
        if (!loaded.hasRecognizedMagic())
            return;

        auto cmds = ui::extractDrawCommandsFromProject(loaded);
        if (cmds.empty())
            return;

        // Resolve textureRelativePath: scan asset_root for each filename.
        std::unordered_map<std::string, std::string> texIndex;
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(assetRoot, ec);
             it != std::filesystem::recursive_directory_iterator(); ++it)
        {
            if (ec) break;
            if (!it->is_regular_file(ec)) continue;
            const auto& p = it->path();
            if (p.extension() != ".dds") continue;
            const std::string name = p.filename().string();
            if (texIndex.find(name) == texIndex.end())
            {
                const auto rel = std::filesystem::relative(p, assetRoot, ec);
                texIndex.emplace(name, rel.generic_string());
            }
        }
        const auto projectDirRel = std::filesystem::path(matchedRelative).parent_path();
        for (auto& c : cmds)
        {
            const auto siblingRel = projectDirRel / c.textureName;
            if (std::filesystem::exists(assetRoot / siblingRel, ec))
                c.textureRelativePath = siblingRel.generic_string();
            else if (auto t = texIndex.find(c.textureName); t != texIndex.end())
                c.textureRelativePath = t->second;
        }

        // Pick the first non-empty scene the project exposes and
        // render it. This is the simplest demo pass; a follow-up can
        // iterate every scene and emit one PNG per scene.
        std::string firstScene;
        for (const auto& c : cmds)
        {
            if (!c.sceneName.empty()) { firstScene = c.sceneName; break; }
        }
        if (firstScene.empty()) return;

        std::vector<ui::CsdNativeDrawCommand> filtered;
        filtered.reserve(cmds.size());
        for (auto& c : cmds)
            if (c.sceneName == firstScene)
                filtered.push_back(c);

        ui::CsdNativeFramebuffer fb;
        fb.resize(1280, 720, {0, 0, 0, 0});
        std::vector<std::pair<std::string, std::pair<std::vector<std::uint8_t>, std::pair<std::uint32_t, std::uint32_t>>>> textureCache;
        std::vector<ui::CsdNativeRuntimeOverride> overrides;
        for (const auto& c : filtered)
            ui::compositeCommand(fb, c, assetRoot, textureCache, overrides);

        const auto outDir = getEvidenceDir();
        const auto outPath = outDir / (projectName + "_" + firstScene + ".png");
        stbi_write_png(outPath.string().c_str(),
                       static_cast<int>(fb.width), static_cast<int>(fb.height),
                       4, fb.rgba.data(), static_cast<int>(fb.width * 4));
    }
} // namespace

namespace UiLab
{
    // Phase 298: invoked from `OnCsdProjectMade` in ui_lab_patches.cpp
    // when a new CSD project finishes loading at runtime. Spawns a
    // background thread that runs the native port's renderer on the
    // same retail .yncp + .dds files the runtime just loaded, and
    // saves a PNG to the evidence directory. The thread guards against
    // concurrent renders so the work doesn't pile up if many projects
    // load in quick succession.
    void RunInProcessNativeRenderForProject(std::string_view projectName)
    {
        if (projectName.empty()) return;
        const std::string name(projectName);
        {
            std::lock_guard<std::mutex> lock(g_csdOverlayMutex);
            if (!g_csdOverlayRendered.insert(name).second)
                return;
        }
        if (g_csdOverlayThreadActive.exchange(true))
            return;

        std::thread([name]
        {
            renderProjectInBackground(name);
            g_csdOverlayThreadActive.store(false);
        }).detach();
    }
} // namespace UiLab
