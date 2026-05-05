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

    // Phase 299: build a global filename -> relative-path index for
    // every .yncp / .xncp under the asset root, scanned once and
    // cached. Lets the harness resolve any project the runtime loads
    // (not just the four subdirs the original list hard-coded).
    std::mutex g_projectIndexMutex;
    std::unordered_map<std::string, std::string> g_projectIndex;
    bool g_projectIndexBuilt = false;

    void ensureProjectIndex(const std::filesystem::path& assetRoot)
    {
        std::lock_guard<std::mutex> lock(g_projectIndexMutex);
        if (g_projectIndexBuilt) return;
        g_projectIndexBuilt = true;
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(assetRoot, ec);
             it != std::filesystem::recursive_directory_iterator(); ++it)
        {
            if (ec) break;
            if (!it->is_regular_file(ec)) continue;
            const auto& p = it->path();
            const auto ext = p.extension().string();
            if (ext != ".yncp" && ext != ".xncp") continue;
            const std::string stem = p.stem().string();
            if (g_projectIndex.find(stem) == g_projectIndex.end())
            {
                const auto rel = std::filesystem::relative(p, assetRoot, ec);
                g_projectIndex.emplace(stem, rel.generic_string());
            }
        }
    }

    void renderProjectInBackground(const std::string& projectName)
    {
        const auto assetRoot = getAssetRoot();
        if (assetRoot.empty())
            return;

        ensureProjectIndex(assetRoot);
        std::string matchedRelative;
        {
            std::lock_guard<std::mutex> lock(g_projectIndexMutex);
            auto it = g_projectIndex.find(projectName);
            if (it == g_projectIndex.end())
                return;
            matchedRelative = it->second;
        }
        const std::filesystem::path projectPath = assetRoot / matchedRelative;
        std::error_code ec;
        if (!std::filesystem::exists(projectPath, ec))
            return;

        const auto loaded = ui::loadCsdProjectFile(projectPath);
        if (!loaded.hasRecognizedMagic())
            return;

        auto cmds = ui::extractDrawCommandsFromProject(loaded);
        if (cmds.empty())
            return;

        // Resolve textureRelativePath: scan asset_root for each filename.
        std::unordered_map<std::string, std::string> texIndex;
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

        // Phase 299: render every distinct scene in the project. A
        // single project commonly carries 5-30 scenes (gauge, frame,
        // counters, etc.), each useful as a separate A/B anchor.
        // Group commands by scene name (stable order from the
        // extractor's draw_order sort) and emit one PNG each.
        std::vector<std::string> sceneOrder;
        std::unordered_map<std::string, std::vector<ui::CsdNativeDrawCommand>> bySceneMap;
        for (auto& c : cmds)
        {
            if (c.sceneName.empty()) continue;
            auto it = bySceneMap.find(c.sceneName);
            if (it == bySceneMap.end())
            {
                sceneOrder.push_back(c.sceneName);
                it = bySceneMap.emplace(c.sceneName, std::vector<ui::CsdNativeDrawCommand>{}).first;
            }
            it->second.push_back(c);
        }

        const auto outDir = getEvidenceDir();
        std::vector<ui::CsdNativeRuntimeOverride> overrides;
        for (const auto& scene : sceneOrder)
        {
            ui::CsdNativeFramebuffer fb;
            fb.resize(1280, 720, {0, 0, 0, 0});
            std::vector<std::pair<std::string, std::pair<std::vector<std::uint8_t>, std::pair<std::uint32_t, std::uint32_t>>>> textureCache;
            for (const auto& c : bySceneMap[scene])
                ui::compositeCommand(fb, c, assetRoot, textureCache, overrides);
            const auto outPath = outDir / (projectName + "__" + scene + ".png");
            stbi_write_png(outPath.string().c_str(),
                           static_cast<int>(fb.width), static_cast<int>(fb.height),
                           4, fb.rgba.data(), static_cast<int>(fb.width * 4));
        }
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
                return;  // Already rendered or in flight.
        }
        // Phase 299: spawn a dedicated thread per project rather than
        // a single shared worker. Each project's render is independent
        // (own loader, own framebuffer, own texture cache) and most
        // boot-time projects fire within ~100 ms of each other; the
        // shared semaphore in Phase 298 was dropping every request
        // after the first. The dedup map above keeps repeat calls cheap.
        std::thread([name]
        {
            renderProjectInBackground(name);
        }).detach();
    }
} // namespace UiLab
