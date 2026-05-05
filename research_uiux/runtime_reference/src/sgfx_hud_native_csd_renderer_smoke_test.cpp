// Phase 296 smoke test: renders one CSD scene to PNG using the native
// C++ compositor (sgfx_hud_native_csd_renderer.hpp). No Python in the
// loop. Output should be visually equivalent to the Python renderer's
// PNG for the same scene.
//
// Usage:
//   sgfx_hud_native_csd_renderer_smoke_test <component_map.json> \
//                                          <asset_root> \
//                                          <project_relative_path> \
//                                          <scene_name> \
//                                          <output.png> \
//                                          [<runtime_overrides.json>]
//
// Example:
//   sgfx_hud_native_csd_renderer_smoke_test \
//      research_uiux/data/yncp_native_component_map.json \
//      extracted_assets/full_install_archives \
//      game/Sonic/ui_playscreen.yncp \
//      so_speed_gauge \
//      out/native_so_speed_gauge.png

#include "sward/ui_runtime/sgfx_hud_native_csd_renderer.hpp"
#include "sward/ui_runtime/sgfx_hud_csd_project_loader.hpp"
#include "sward/ui_runtime/sgfx_hud_csd_cast_extractor.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "nlohmann/json.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui = sward::ui_runtime::generated::sgfx_hud;
using nlohmann::json;

static std::vector<ui::CsdNativeDrawCommand> loadProjectDrawCommands(
    const std::filesystem::path& componentMapPath,
    const std::string& projectRelPath,
    const std::string& sceneName)
{
    std::ifstream f(componentMapPath);
    if (!f)
    {
        std::cerr << "could not open component map: " << componentMapPath << "\n";
        return {};
    }
    json doc;
    try
    {
        f >> doc;
    }
    catch (const std::exception& e)
    {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return {};
    }

    std::vector<ui::CsdNativeDrawCommand> commands;
    if (!doc.contains("screen_groups")) return commands;
    for (auto& [groupName, groupArr] : doc["screen_groups"].items())
    {
        for (const auto& proj : groupArr)
        {
            if (proj.value("relative_path", std::string{}) != projectRelPath) continue;
            if (!proj.contains("scene_draw_commands")) continue;
            for (const auto& c : proj["scene_draw_commands"])
            {
                if (c.value("scene_name", std::string{}) != sceneName) continue;
                ui::CsdNativeDrawCommand cmd;
                cmd.projectRelativePath = c.value("project_relative_path", std::string{});
                cmd.sceneName           = c.value("scene_name", std::string{});
                cmd.castName            = c.value("cast_name", std::string{});
                cmd.nodePath            = c.value("node_path", std::string{});
                cmd.castPath            = c.value("cast_path", std::string{});
                cmd.groupIndex          = c.value("group_index", 0u);
                cmd.castIndex           = c.value("cast_index", 0u);
                cmd.drawOrder           = c.value("draw_order", 0u);
                cmd.hasTexture          = c.value("has_texture", false);
                cmd.hideFlag            = c.value("hide_flag", 0u);
                cmd.textureName         = c.value("texture_name", std::string{});
                cmd.textureRelativePath = c.value("texture_relative_path", std::string{});
                cmd.sourceTextureWidth  = c.value("source_texture_width", 0u);
                cmd.sourceTextureHeight = c.value("source_texture_height", 0u);
                cmd.sourceX             = c.value("source_x", 0u);
                cmd.sourceY             = c.value("source_y", 0u);
                cmd.sourceWidth         = c.value("source_width", 0u);
                cmd.sourceHeight        = c.value("source_height", 0u);
                cmd.uvLeft              = c.value("uv_left", 0.0f);
                cmd.uvTop               = c.value("uv_top", 0.0f);
                cmd.uvRight             = c.value("uv_right", 0.0f);
                cmd.uvBottom            = c.value("uv_bottom", 0.0f);
                cmd.sceneLeft           = c.value("scene_left", 0.0f);
                cmd.sceneTop            = c.value("scene_top", 0.0f);
                cmd.sceneWidth          = c.value("scene_width", 0.0f);
                cmd.sceneHeight         = c.value("scene_height", 0.0f);
                cmd.baseTranslationX    = c.value("base_translation_x", 0.0f);
                cmd.baseTranslationY    = c.value("base_translation_y", 0.0f);
                cmd.baseScaleX          = c.value("base_scale_x", 1.0f);
                cmd.baseScaleY          = c.value("base_scale_y", 1.0f);
                cmd.baseRotation        = c.value("base_rotation", 0.0f);
                commands.push_back(std::move(cmd));
            }
        }
    }
    std::sort(commands.begin(), commands.end(),
        [](const auto& a, const auto& b)
        {
            if (a.drawOrder != b.drawOrder) return a.drawOrder < b.drawOrder;
            if (a.groupIndex != b.groupIndex) return a.groupIndex < b.groupIndex;
            return a.castIndex < b.castIndex;
        });
    return commands;
}

static std::vector<ui::CsdNativeRuntimeOverride> loadRuntimeOverrides(const std::filesystem::path& path)
{
    std::vector<ui::CsdNativeRuntimeOverride> overrides;
    if (path.empty()) return overrides;
    std::ifstream f(path);
    if (!f) return overrides;
    json doc;
    try { f >> doc; }
    catch (...) { return overrides; }
    if (!doc.contains("by_scene")) return overrides;
    for (auto& [sceneName, recArr] : doc["by_scene"].items())
    {
        if (!recArr.is_array() || recArr.empty()) continue;
        const auto& top = recArr[0];
        ui::CsdNativeRuntimeOverride ov;
        ov.sceneName = sceneName;
        ov.anchorXPx = top.value("anchor_x_px", 0.0f);
        ov.anchorYPx = top.value("anchor_y_px", 0.0f);
        ov.scaleX    = top.value("scale_x", 1.0f);
        ov.scaleY    = top.value("scale_y", 1.0f);
        overrides.push_back(std::move(ov));
    }
    return overrides;
}

// Phase 297: scan the asset tree once and build a name -> first-match
// path map. Localized textures (mat_*_en_*.dds) live under
// `Languages/English/<area>/` rather than next to the .yncp, so a flat
// filename index is the cheapest way to resolve them at render time.
// Mirrors `choose_texture_path` in build_yncp_native_component_map.py.
static std::unordered_map<std::string, std::string> buildTextureNameIndex(
    const std::filesystem::path& assetRoot)
{
    std::unordered_map<std::string, std::string> index;
    std::error_code ec;
    if (!std::filesystem::exists(assetRoot, ec)) return index;
    for (auto it = std::filesystem::recursive_directory_iterator(assetRoot, ec);
         it != std::filesystem::recursive_directory_iterator(); ++it)
    {
        if (ec) break;
        if (!it->is_regular_file(ec)) continue;
        const auto& p = it->path();
        if (p.extension() != ".dds") continue;
        const std::string name = p.filename().string();
        // Prefer the first occurrence; deeper duplicates (DLC variants)
        // are skipped. Same priority as Python's first-match policy.
        if (index.find(name) == index.end())
        {
            const auto rel = std::filesystem::relative(p, assetRoot, ec);
            index.emplace(name, rel.generic_string());
        }
    }
    return index;
}

// Phase 297: build draw commands by directly parsing the .yncp /
// .xncp file (no JSON in the loop). Resolves the full draw command
// list end-to-end from retail bytes.
static std::vector<ui::CsdNativeDrawCommand> buildDrawCommandsFromBinary(
    const std::filesystem::path& assetRoot,
    const std::string& projectRelPath,
    const std::string& sceneName)
{
    const auto projectPath = assetRoot / projectRelPath;
    const auto loaded = ui::loadCsdProjectFile(projectPath);
    if (!loaded.hasRecognizedMagic())
    {
        std::cerr << "loadCsdProjectFile failed (loadStatus=" << loaded.loadStatus
                  << " parseStatus=" << loaded.parseStatus << ")\n";
        return {};
    }
    auto cmds = ui::extractDrawCommandsFromProject(loaded);
    const auto textureIndex = buildTextureNameIndex(assetRoot);
    const auto projectDirRel = std::filesystem::path(projectRelPath).parent_path();
    for (auto& c : cmds)
    {
        // Prefer texture next to the .yncp; fall back to the global
        // filename index (catches Languages/English/* localized assets).
        const std::filesystem::path siblingRel = projectDirRel / c.textureName;
        if (std::filesystem::exists(assetRoot / siblingRel))
            c.textureRelativePath = siblingRel.generic_string();
        else if (auto it = textureIndex.find(c.textureName); it != textureIndex.end())
            c.textureRelativePath = it->second;
        else
            c.textureRelativePath = siblingRel.generic_string();
    }
    // Filter to the requested scene only.
    std::vector<ui::CsdNativeDrawCommand> filtered;
    filtered.reserve(cmds.size());
    for (auto& c : cmds)
        if (c.sceneName == sceneName)
            filtered.push_back(std::move(c));
    std::sort(filtered.begin(), filtered.end(),
        [](const auto& a, const auto& b)
        {
            if (a.drawOrder != b.drawOrder) return a.drawOrder < b.drawOrder;
            if (a.groupIndex != b.groupIndex) return a.groupIndex < b.groupIndex;
            return a.castIndex < b.castIndex;
        });
    return filtered;
}

int main(int argc, char** argv)
{
    if (argc < 6)
    {
        std::cerr << "usage: " << argv[0]
                  << " <component_map.json> <asset_root> <project_relative_path> <scene_name> <output.png> [<runtime_overrides.json>]\n"
                  << "       (pass component_map.json='--binary' to parse the .yncp directly instead of using the JSON)\n";
        return 2;
    }

    const std::filesystem::path componentMapPath = argv[1];
    const std::filesystem::path assetRoot        = argv[2];
    const std::string projectRelPath             = argv[3];
    const std::string sceneName                  = argv[4];
    const std::filesystem::path outputPath       = argv[5];
    const std::filesystem::path overridesPath    = (argc >= 7) ? std::filesystem::path(argv[6]) : std::filesystem::path{};
    const bool useBinaryParser                   = (componentMapPath.string() == "--binary");

    const auto commands = useBinaryParser
        ? buildDrawCommandsFromBinary(assetRoot, projectRelPath, sceneName)
        : loadProjectDrawCommands(componentMapPath, projectRelPath, sceneName);
    if (commands.empty())
    {
        std::cerr << "no draw commands found for scene '" << sceneName << "' in project '" << projectRelPath << "'\n";
        return 1;
    }
    const auto overrides = loadRuntimeOverrides(overridesPath);

    constexpr std::uint32_t kCanvasW = 1280;
    constexpr std::uint32_t kCanvasH = 720;
    ui::CsdNativeFramebuffer fb;
    fb.resize(kCanvasW, kCanvasH, {0, 0, 0, 0});

    std::vector<std::pair<std::string, std::pair<std::vector<std::uint8_t>, std::pair<std::uint32_t, std::uint32_t>>>> textureCache;
    int drawn = 0, skipped = 0;
    for (const auto& cmd : commands)
        if (ui::compositeCommand(fb, cmd, assetRoot, textureCache, overrides))
            ++drawn;
        else
            ++skipped;

    std::filesystem::create_directories(outputPath.parent_path());
    if (!stbi_write_png(outputPath.string().c_str(),
                        static_cast<int>(fb.width), static_cast<int>(fb.height),
                        4, fb.rgba.data(), static_cast<int>(fb.width * 4)))
    {
        std::cerr << "stbi_write_png failed for " << outputPath << "\n";
        return 1;
    }

    std::cout << "scene=" << sceneName
              << " mode=" << (useBinaryParser ? "binary" : "json")
              << " commands=" << commands.size()
              << " drawn=" << drawn
              << " skipped=" << skipped
              << " textures=" << textureCache.size()
              << " output=" << outputPath.string()
              << "\n";
    return 0;
}
