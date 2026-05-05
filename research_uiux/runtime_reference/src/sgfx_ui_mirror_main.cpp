// Phase 338: SGFX UI Mirror window.
//
// A standalone Windows app that opens an SDL2 window and renders
// Sonic Unleashed's native UI -- via SGFX's own CSD renderer + SGFX's
// orchestrator + SGFX's audio player -- driven either by:
//
//   * a scripted demo cycle that walks through Title -> WorldMap ->
//     Loading -> StageHud -> Pause -> Results -> Hub (default), or
//
//   * a logcat tail mode that follows ui_lab_events.jsonl as
//     UnleashedRecomp writes it. Each new event with a known target
//     (title-menu / loading / sonic-hud / etc.) flips the mirror's
//     active screen so the user can play UnleashedRecomp on one
//     monitor and watch SGFX's native render on another.
//
// CLI:
//   --silent                  disable SDL_mixer (no audio playback)
//   --jsonl=<path>            tail this ui_lab_events.jsonl
//   --asset-root=<dir>        override extracted-assets root
//                             (default: extracted_assets/full_install_archives)
//   --screen=<name>           force a specific screen for the lifetime
//                             of the run (debugging)
//   --demo-seconds=<n>        per-screen dwell in demo cycle (default 4)

#include "sward/ui_runtime/sgfx_orchestrator.hpp"
#include "sward/ui_runtime/sgfx_audio_player.hpp"
#include "sward/ui_runtime/sgfx_audio_dispatch.hpp"
#include "sward/ui_runtime/sgfx_hud_csd_project_loader.hpp"
#include "sward/ui_runtime/sgfx_hud_csd_cast_extractor.hpp"
#include "sward/ui_runtime/sgfx_hud_native_csd_renderer.hpp"
#include "sward/ui_runtime/sgfx_hud_font_renderer.hpp"

// Static-link SDL2 with no SDL_main shim (we provide our own main).
#define SDL_MAIN_HANDLED
#include <SDL.h>

// Phase 347: real BGM byte blob.
#include "res/music/installer.ogg.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ui = sward::ui_runtime::generated::sgfx_hud;
namespace fs = std::filesystem;

namespace
{
    // Phase 338: render at half-res (640x360) so the software CPU
    // compositor stays interactive. Present scaled to window size
    // via SDL_RenderCopy. Native renderer is single-threaded
    // alpha-blend; doubling each axis is 4x the pixel work which
    // pushed first-paint of ui_mainmenu past the user's patience.
    constexpr std::uint32_t kCanvasW = 640;
    constexpr std::uint32_t kCanvasH = 360;
    constexpr std::uint32_t kWindowW = 1280;
    constexpr std::uint32_t kWindowH = 720;

    struct ScreenSpec
    {
        ui::SgfxScreen      screen;
        const char*         friendlyName;
        const char*         projectRelPath;
    };

    // Map orchestrator screens to their primary CSD asset. These
    // come from research_uiux/data/yncp_native_component_map.json
    // (the parity-validated asset spec).
    //
    // Phase 345: StageHud entry binds to Day Sonic by default; the
    // mirror swaps to ui_playscreen_ev / ui_playscreen_su when
    // orchestrator.stageHud.mode is Werehog or Boss respectively.
    constexpr ScreenSpec kScreenSpecs[] = {
        { ui::SgfxScreen::TitleIntro, "TitleIntro", "game/Title/ui_title.yncp" },
        { ui::SgfxScreen::Title,      "Title",      "game/MainMenu/ui_mainmenu.yncp" },
        { ui::SgfxScreen::WorldMap,   "WorldMap",   "game/WorldMap/ui_worldmap.yncp" },
        { ui::SgfxScreen::Loading,    "Loading",    "game/Loading/ui_loading.yncp" },
        { ui::SgfxScreen::StageHud,   "StageHud",   "game/Sonic/ui_playscreen.yncp" },
        { ui::SgfxScreen::Pause,      "Pause",      "game/SystemCommonCore/ui_pause.yncp" },
        { ui::SgfxScreen::Results,    "Results",    "game/ActionCommon/ui_result.yncp" },
        { ui::SgfxScreen::Hub,        "Hub",        "game/Town_Common/ui_townscreen.yncp" },
    };

    const ScreenSpec& specFor(ui::SgfxScreen s)
    {
        for (const auto& sp : kScreenSpecs)
            if (sp.screen == s) return sp;
        return kScreenSpecs[0];
    }

    // Phase 345: pick the actual CSD project for the active screen,
    // taking StageHudState::mode into account. Day Sonic uses
    // ui_playscreen.yncp; Werehog uses ui_playscreen_ev.yncp; Boss
    // uses ui_playscreen_su.yncp; BossHit uses ui_playscreen_ev_hit.
    const char* projectPathForScreen(ui::SgfxScreen s,
                                     const ui::StageHudState& stage) noexcept
    {
        if (s == ui::SgfxScreen::StageHud)
        {
            switch (stage.mode)
            {
                case ui::StageMode::Werehog:
                    return "game/EvilSonic/ui_playscreen_ev.yncp";
                case ui::StageMode::Boss:
                    return "game/BossDarkGaia1_1Air/ui_playscreen_su.yncp";
                case ui::StageMode::BossHit:
                    return "game/EvilActionCommon/ui_playscreen_ev_hit.yncp";
                case ui::StageMode::DaySonic:
                default:
                    return "game/Sonic/ui_playscreen.yncp";
            }
        }
        return specFor(s).projectRelPath;
    }

    // Map JSONL "target" field -> orchestrator screen.
    std::optional<ui::SgfxScreen> targetTokenToScreen(std::string_view tok)
    {
        if (tok == "title-menu" || tok == "title-runtime") return ui::SgfxScreen::Title;
        if (tok == "title-loop") return ui::SgfxScreen::TitleIntro;
        if (tok == "world-map" || tok == "world-map-help") return ui::SgfxScreen::WorldMap;
        if (tok == "loading" || tok == "loading-start") return ui::SgfxScreen::Loading;
        if (tok == "sonic-hud" || tok == "extra-stage-hud") return ui::SgfxScreen::StageHud;
        if (tok == "pause" || tok == "general" || tok == "help") return ui::SgfxScreen::Pause;
        if (tok == "result" || tok == "result-ex" || tok == "item-result") return ui::SgfxScreen::Results;
        if (tok == "balloon" || tok == "townscreen" || tok == "shop" || tok == "mediaroom"
            || tok == "gate" || tok == "missionscreen" || tok == "misson") return ui::SgfxScreen::Hub;
        return std::nullopt;
    }

    // Pull a string field from a one-line JSON object via simple
    // key-search. Good enough for our well-formed JSONL where each
    // line is `{"k1":..,"k2":..}` with no nesting and no embedded
    // braces in string values that we care about.
    std::string extractJsonString(std::string_view line, std::string_view key)
    {
        // Search for "<key>":"
        std::string needle;
        needle.reserve(key.size() + 4);
        needle += '"';
        needle.append(key.data(), key.size());
        needle += "\":\"";
        const auto pos = line.find(needle);
        if (pos == std::string_view::npos) return {};
        const auto valStart = pos + needle.size();
        // Find next unescaped quote.
        std::string out;
        for (auto i = valStart; i < line.size(); ++i)
        {
            const char c = line[i];
            if (c == '\\' && i + 1 < line.size()) { out.push_back(line[i + 1]); ++i; continue; }
            if (c == '"') break;
            out.push_back(c);
        }
        return out;
    }

    // Project draw-command cache -- loading + parsing the .yncp +
    // building the texture index is expensive, so cache per-screen.
    // Phase 338 fix: also keep the decoded-DDS texture cache resident
    // across frames -- otherwise each frame re-decodes every texture
    // and the renderer drops below 1 fps.
    using CsdTextureCache = std::vector<std::pair<std::string,
        std::pair<std::vector<std::uint8_t>,
                  std::pair<std::uint32_t, std::uint32_t>>>>;

    struct CachedScreenAssets
    {
        std::vector<ui::CsdNativeDrawCommand> commands;
        std::vector<ui::CsdNativeRuntimeOverride> overrides;
        CsdTextureCache textureCache; // keeps decoded RGBA buffers alive
        bool loadedOk = false;
    };

    std::unordered_map<std::string, CachedScreenAssets> g_assetCache;

    // Texture-name index built once per asset root (recursive scan).
    std::unordered_map<std::string, std::string> g_textureIndex;
    fs::path g_assetRoot;

    // Phase 346: baked font for runtime text overlays. Loaded once
    // at startup from a system TTF (defaults to arial.ttf on Windows).
    ui::SgfxBakedFont g_bodyFont;
    ui::SgfxBakedFont g_smallFont;

    void buildTextureIndex(const fs::path& assetRoot)
    {
        g_textureIndex.clear();
        std::error_code ec;
        if (!fs::exists(assetRoot, ec)) return;
        for (auto it = fs::recursive_directory_iterator(assetRoot, ec);
             it != fs::recursive_directory_iterator(); ++it)
        {
            if (ec) break;
            if (!it->is_regular_file(ec)) continue;
            const auto& p = it->path();
            if (p.extension() != ".dds") continue;
            const std::string name = p.filename().string();
            if (g_textureIndex.find(name) == g_textureIndex.end())
            {
                const auto rel = fs::relative(p, assetRoot, ec);
                g_textureIndex.emplace(name, rel.generic_string());
            }
        }
    }

    const CachedScreenAssets& getOrLoadScreenAssets(const std::string& projectRelPath)
    {
        auto it = g_assetCache.find(projectRelPath);
        if (it != g_assetCache.end()) return it->second;

        CachedScreenAssets cached;
        const auto projectPath = g_assetRoot / projectRelPath;
        const auto loaded = ui::loadCsdProjectFile(projectPath);
        if (loaded.hasRecognizedMagic())
        {
            cached.commands = ui::extractDrawCommandsFromProject(loaded);
            const auto projectDirRel = fs::path(projectRelPath).parent_path();
            for (auto& c : cached.commands)
            {
                const fs::path siblingRel = projectDirRel / c.textureName;
                if (fs::exists(g_assetRoot / siblingRel))
                    c.textureRelativePath = siblingRel.generic_string();
                else if (auto idx = g_textureIndex.find(c.textureName); idx != g_textureIndex.end())
                    c.textureRelativePath = idx->second;
                else
                    c.textureRelativePath = siblingRel.generic_string();
            }
            std::sort(cached.commands.begin(), cached.commands.end(),
                [](const auto& a, const auto& b) {
                    if (a.drawOrder != b.drawOrder) return a.drawOrder < b.drawOrder;
                    if (a.groupIndex != b.groupIndex) return a.groupIndex < b.groupIndex;
                    return a.castIndex < b.castIndex;
                });
            cached.loadedOk = true;
        }
        const auto [insIt, _] = g_assetCache.emplace(projectRelPath, std::move(cached));
        return insIt->second;
    }

    // Render every scene of the screen's CSD project into the
    // framebuffer. For container-heavy projects (worldmap etc.)
    // this composites the entire screen.
    // Phase 346: known blank text-container scenes from the renderer
    // coverage audit (sgfx_renderer_coverage_audit.generated.json).
    // The CSD container has zero textured cells; in retail these
    // would be runtime-text-rasterized. SGFX paints placeholder
    // labels via stb_truetype so the layout doesn't read as blank.
    struct BlankTextScene
    {
        const char* sceneName;
        const char* placeholderText;
    };
    constexpr BlankTextScene kBlankTextScenes[] = {
        {"help_chara_1",   "[help line 1]"},
        {"help_chara_2",   "[help line 2]"},
        {"help_chara_3",   "[help line 3]"},
        {"help_text_area", "[world map help text]"},
        {"progress",       "Loading..."},
    };

    // Phase 347: pick a BGM cue per active screen so the mirror's
    // demo cycle plays music continuously. Hosts streaming retail
    // BGM banks would replace this map with the real per-area cue.
    const char* bgmCueForScreen(ui::SgfxScreen s, ui::StageMode mode) noexcept
    {
        switch (s)
        {
            case ui::SgfxScreen::TitleIntro:
            case ui::SgfxScreen::Title:      return "bgm_sys_title";
            case ui::SgfxScreen::WorldMap:
            case ui::SgfxScreen::Loading:    return "bgm_sys_worldmap";
            case ui::SgfxScreen::StageHud:
                return (mode == ui::StageMode::Werehog)
                       ? "bgm_act_hub_apotos" : "bgm_act_apotos";
            case ui::SgfxScreen::Pause:      return "bgm_sys_worldmap";
            case ui::SgfxScreen::Results:    return "bgm_sys_result";
            case ui::SgfxScreen::Hub:        return "bgm_act_hub_apotos";
        }
        return "";
    }

    // Map screen -> caption shown at the top of the framebuffer.
    const char* screenCaption(ui::SgfxScreen s, ui::StageMode mode)
    {
        switch (s)
        {
            case ui::SgfxScreen::TitleIntro: return "Title Intro";
            case ui::SgfxScreen::Title:      return "Title Menu";
            case ui::SgfxScreen::WorldMap:   return "World Map";
            case ui::SgfxScreen::Loading:    return "Loading";
            case ui::SgfxScreen::StageHud:
                switch (mode)
                {
                    case ui::StageMode::Werehog: return "Stage HUD - Werehog";
                    case ui::StageMode::Boss:    return "Stage HUD - Boss";
                    case ui::StageMode::BossHit: return "Stage HUD - Boss Hit";
                    default:                     return "Stage HUD - Day Sonic";
                }
            case ui::SgfxScreen::Pause:      return "Pause";
            case ui::SgfxScreen::Results:    return "Results";
            case ui::SgfxScreen::Hub:        return "Hub";
        }
        return "?";
    }

    void renderScreenIntoFramebuffer(ui::SgfxScreen screen,
                                     const ui::StageHudState& stage,
                                     ui::CsdNativeFramebuffer& fb)
    {
        // Clear to dark gray (so blank areas read as "rendered, not
        // crashed").
        fb.resize(kCanvasW, kCanvasH, {16, 16, 24, 255});
        const std::string projectPath = projectPathForScreen(screen, stage);
        // Mutable lookup so we can grow the texture cache as the
        // renderer reads new textures on first appearance.
        auto it = g_assetCache.find(projectPath);
        if (it == g_assetCache.end())
        {
            getOrLoadScreenAssets(projectPath);
            it = g_assetCache.find(projectPath);
        }
        if (it == g_assetCache.end() || !it->second.loadedOk) return;
        auto& assets = it->second;
        for (const auto& cmd : assets.commands)
            (void)ui::compositeCommand(
                fb, cmd, g_assetRoot, assets.textureCache, assets.overrides);

        // Phase 346: paint placeholder text into known-blank text
        // scenes. We iterate the assets' commands looking for cast
        // sceneNames matching our blank-scene table; if a hit is
        // found, draw placeholder text near the cast's anchor.
        if (g_bodyFont.loaded)
        {
            for (const auto& cmd : assets.commands)
            {
                for (const auto& bts : kBlankTextScenes)
                {
                    if (cmd.sceneName != bts.sceneName) continue;
                    const float canvasW = static_cast<float>(fb.width);
                    const float canvasH = static_cast<float>(fb.height);
                    const auto x = static_cast<std::int32_t>(
                        (cmd.baseTranslationX + cmd.sceneLeft) * canvasW + 4.0f);
                    const auto y = static_cast<std::int32_t>(
                        (cmd.baseTranslationY + cmd.sceneTop)  * canvasH + 4.0f);
                    ui::compositeText(fb, g_bodyFont, bts.placeholderText,
                                      x, y, {255, 255, 255, 255});
                }
            }
        }

        // Phase 346: top-left screen caption so each variant is
        // visually labeled even when CSD assets are unfamiliar.
        if (g_smallFont.loaded)
        {
            ui::compositeText(fb, g_smallFont,
                              screenCaption(screen, stage.mode),
                              6, 4, {220, 230, 255, 255});
        }
    }
}

int main(int argc, char** argv)
{
    // Unbuffered stdout so we can tail the log even if the process
    // is killed mid-run (window apps sometimes have this issue).
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::setvbuf(stderr, nullptr, _IONBF, 0);

    bool silent = false;
    std::string jsonlPath;
    fs::path assetRoot = "extracted_assets/full_install_archives";
    std::string forceScreen;
    float demoSeconds = 4.0f;
    int maxFrames = 0; // 0 = unlimited

    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--silent") silent = true;
        else if (a.rfind("--jsonl=", 0) == 0) jsonlPath = a.substr(8);
        else if (a.rfind("--asset-root=", 0) == 0) assetRoot = a.substr(13);
        else if (a.rfind("--screen=", 0) == 0) forceScreen = a.substr(9);
        else if (a.rfind("--demo-seconds=", 0) == 0) demoSeconds = std::stof(a.substr(15));
        else if (a.rfind("--frames=", 0) == 0) maxFrames = std::stoi(a.substr(9));
        else if (a == "--help" || a == "-h")
        {
            std::cout << "sgfx_ui_mirror -- SGFX native UI render in a window\n"
                      << "  --silent                  no audio\n"
                      << "  --jsonl=<path>            tail UnleashedRecomp's ui_lab_events.jsonl\n"
                      << "  --asset-root=<dir>        retail .yncp / .dds asset root\n"
                      << "  --screen=<name>           force a screen (Title/WorldMap/...)\n"
                      << "  --demo-seconds=<n>        per-screen dwell in demo cycle\n";
            return 0;
        }
    }

    g_assetRoot = assetRoot;

    if (SDL_Init(SDL_INIT_VIDEO | (silent ? 0 : SDL_INIT_AUDIO)) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "SGFX UI Mirror -- Sonic Unleashed (native render)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        static_cast<int>(kWindowW), static_cast<int>(kWindowH),
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_Texture* tex = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
        static_cast<int>(kCanvasW), static_cast<int>(kCanvasH));
    if (!tex)
    {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!silent)
    {
        if (!ui::sgfxAudioPlayerInit())
        {
            std::cerr << "sgfxAudioPlayerInit failed; continuing silent\n";
            silent = true;
        }
        else
        {
            // Phase 347: register the embedded OGG blob as the BGM
            // bytes for every demo cue the orchestrator may mount.
            const char* kBgmCues[] = {
                "bgm_sys_title", "bgm_sys_worldmap", "bgm_act_apotos",
                "bgm_act_hub_apotos", "bgm_sys_result", "bgm_sys_result_ng",
            };
            for (const char* name : kBgmCues)
                ui::sgfxAudioPlayerRegisterBgm(name, g_installer_music,
                                               sizeof(g_installer_music));
        }
    }

    std::cout << "scanning asset textures under " << assetRoot << " ...\n";
    buildTextureIndex(assetRoot);
    std::cout << "  texture index size: " << g_textureIndex.size() << "\n";

    // Phase 346: bake fonts at startup so per-frame text overlays
    // are O(glyph) per character. Defaults to Windows Arial; if
    // missing we silently leave g_bodyFont.loaded == false and
    // the renderer skips the text overlay step.
    {
        const fs::path arial = "C:/Windows/Fonts/arial.ttf";
        g_bodyFont  = ui::loadFontFromFile(arial, 18.0f);
        g_smallFont = ui::loadFontFromFile(arial, 14.0f);
        std::cout << "  fonts: body="
                  << (g_bodyFont.loaded ? "ok" : "missing")
                  << " small="
                  << (g_smallFont.loaded ? "ok" : "missing")
                  << " (path=" << arial.string() << ")\n";
    }

    ui::SgfxOrchestrator orch;
    ui::applyTitleMenuVisibility(orch.title, true, false, false, true);
    orch.titleIntro.logoFadeInDurationSeconds = 0.001f;
    orch.titleIntro.advertiseMovieWaitTime = 9999.0f; // suppress attract during demo

    // Optional --screen= forcing.
    if (!forceScreen.empty())
    {
        for (const auto& sp : kScreenSpecs)
        {
            if (forceScreen == sp.friendlyName)
            {
                orch.current = sp.screen;
                std::cout << "forcing screen: " << sp.friendlyName << "\n";
                break;
            }
        }
    }

    ui::CsdNativeFramebuffer fb;
    fb.resize(kCanvasW, kCanvasH);

    // JSONL tail state.
    std::ifstream jsonl;
    std::streampos jsonlPos = 0;
    if (!jsonlPath.empty())
    {
        std::cout << "tailing JSONL: " << jsonlPath << "\n";
    }

    // Demo cycle: same screen flow as before, but the StageHud
    // appears 3 times -- once per StageMode (Day / Werehog / Boss)
    // -- so the mirror visibly swaps to ui_playscreen_ev /
    // ui_playscreen_su. (Phase 345)
    struct DemoStep
    {
        ui::SgfxScreen screen;
        ui::StageMode  stageMode;
        const char*    label;
    };
    const DemoStep kDemoSteps[] = {
        { ui::SgfxScreen::TitleIntro, ui::StageMode::DaySonic, "TitleIntro" },
        { ui::SgfxScreen::Title,      ui::StageMode::DaySonic, "Title" },
        { ui::SgfxScreen::WorldMap,   ui::StageMode::DaySonic, "WorldMap" },
        { ui::SgfxScreen::Loading,    ui::StageMode::DaySonic, "Loading" },
        { ui::SgfxScreen::StageHud,   ui::StageMode::DaySonic, "StageHud[Day]" },
        { ui::SgfxScreen::StageHud,   ui::StageMode::Werehog,  "StageHud[Werehog]" },
        { ui::SgfxScreen::StageHud,   ui::StageMode::Boss,     "StageHud[Boss]" },
        { ui::SgfxScreen::Pause,      ui::StageMode::DaySonic, "Pause" },
        { ui::SgfxScreen::Results,    ui::StageMode::DaySonic, "Results" },
        { ui::SgfxScreen::Hub,        ui::StageMode::DaySonic, "Hub" },
    };
    const std::size_t kDemoCount = sizeof(kDemoSteps) / sizeof(kDemoSteps[0]);
    std::size_t demoIdx = 0;
    auto demoStart = std::chrono::steady_clock::now();

    bool running = true;
    auto lastFrame = std::chrono::steady_clock::now();
    int presentedFrames = 0;
    while (running)
    {
        // Pump events.
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT) running = false;
            else if (ev.type == SDL_KEYDOWN)
            {
                if (ev.key.keysym.sym == SDLK_ESCAPE) running = false;
                // Manual screen step: SPACE = next screen in demo cycle.
                else if (ev.key.keysym.sym == SDLK_SPACE && jsonlPath.empty()
                         && forceScreen.empty())
                {
                    demoIdx = (demoIdx + 1) % kDemoCount;
                    orch.current = kDemoSteps[demoIdx].screen;
                    orch.stageHud.mode = kDemoSteps[demoIdx].stageMode;
                    demoStart = std::chrono::steady_clock::now();
                    std::cout << "[demo] step -> " << kDemoSteps[demoIdx].label << "\n";
                }
            }
        }

        // JSONL tail: read any new lines, parse target, switch screen.
        if (!jsonlPath.empty())
        {
            if (!jsonl.is_open())
            {
                jsonl.open(jsonlPath);
                if (jsonl) jsonl.seekg(0, std::ios::end), jsonlPos = jsonl.tellg();
            }
            if (jsonl.is_open())
            {
                jsonl.clear(); // clear EOF
                jsonl.seekg(jsonlPos);
                std::string line;
                while (std::getline(jsonl, line))
                {
                    const auto target = extractJsonString(line, "target");
                    if (auto s = targetTokenToScreen(target))
                    {
                        if (orch.current != *s)
                        {
                            std::cout << "[jsonl] target=" << target
                                      << " -> screen=" << specFor(*s).friendlyName << "\n";
                            orch.current = *s;
                        }
                    }
                    const auto event = extractJsonString(line, "event");
                    // Best-effort cue dispatch from event name.
                    if (event.find("decide") != std::string::npos
                        || event.find("worldmap") != std::string::npos
                        || event.find("pause") != std::string::npos)
                    {
                        ui::sgfxAudioPlayerPlayCue(event);
                    }
                }
                jsonlPos = jsonl.tellg();
                if (jsonlPos < std::streampos(0)) jsonlPos = 0;
            }
        }
        // Demo auto-cycle: advance every demoSeconds.
        else if (forceScreen.empty())
        {
            const auto now = std::chrono::steady_clock::now();
            const float dwell = std::chrono::duration<float>(now - demoStart).count();
            if (dwell >= demoSeconds)
            {
                demoIdx = (demoIdx + 1) % kDemoCount;
                orch.current = kDemoSteps[demoIdx].screen;
                orch.stageHud.mode = kDemoSteps[demoIdx].stageMode;
                demoStart = now;
                // Play a transition cue audibly.
                if (!silent) ui::sgfxAudioPlayerPlayCue("sys_actstg_pausewinopen");
                std::cout << "[demo] auto -> " << kDemoSteps[demoIdx].label << "\n";
            }
        }

        // Phase 347: keep BGM in lockstep with active screen.
        if (!silent)
        {
            const char* desiredCue = bgmCueForScreen(orch.current, orch.stageHud.mode);
            if (orch.bgmAdmin.cueAt(ui::kBgmChannelMain) != std::string(desiredCue))
                orch.bgmAdmin.mountCue(ui::kBgmChannelMain, desiredCue);
            ui::sgfxAudioPlayerApplyBgmAdmin(orch.bgmAdmin);
        }

        // Render the active screen.
        renderScreenIntoFramebuffer(orch.current, orch.stageHud, fb);

        // Blit framebuffer -> SDL_Texture -> present.
        void* pixels = nullptr;
        int pitch = 0;
        if (SDL_LockTexture(tex, nullptr, &pixels, &pitch) == 0)
        {
            const std::uint8_t* src = fb.rgba.data();
            for (std::uint32_t y = 0; y < fb.height; ++y)
            {
                std::memcpy(static_cast<std::uint8_t*>(pixels) + y * pitch,
                            src + y * fb.width * 4, fb.width * 4);
            }
            SDL_UnlockTexture(tex);
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, nullptr, nullptr);
        // Window title overlay (cheap status). Phase 345: include
        // stage mode when on StageHud so the asset swap is visible.
        std::string title = "SGFX UI Mirror -- ";
        title += specFor(orch.current).friendlyName;
        if (orch.current == ui::SgfxScreen::StageHud)
        {
            switch (orch.stageHud.mode)
            {
                case ui::StageMode::Werehog: title += " [Werehog]"; break;
                case ui::StageMode::Boss:    title += " [Boss]"; break;
                case ui::StageMode::BossHit: title += " [BossHit]"; break;
                default:                     title += " [Day]"; break;
            }
        }
        if (!jsonlPath.empty()) title += " [JSONL]";
        else if (!forceScreen.empty()) title += " [forced]";
        else title += " [demo]";
        SDL_SetWindowTitle(window, title.c_str());
        SDL_RenderPresent(renderer);

        ++presentedFrames;
        if ((presentedFrames % 30) == 0)
        {
            const auto now = std::chrono::steady_clock::now();
            const float elapsed = std::chrono::duration<float>(
                now - demoStart).count();
            std::cout << "[mirror] frame=" << presentedFrames
                      << " screen=" << specFor(orch.current).friendlyName
                      << " dwell=" << elapsed << "s\n";
        }
        if (maxFrames > 0 && presentedFrames >= maxFrames) running = false;
        // Cap to ~60fps if VSYNC isn't available.
        const auto now = std::chrono::steady_clock::now();
        const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastFrame).count();
        if (dt < 16) std::this_thread::sleep_for(std::chrono::milliseconds(16 - dt));
        lastFrame = std::chrono::steady_clock::now();
    }

    if (!silent) ui::sgfxAudioPlayerShutdown();
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "presented frames: " << presentedFrames << "\n";
    return 0;
}
