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
#include "sward/ui_runtime/sgfx_bgm_bank_loader.hpp"
#include "sward/ui_runtime/sgfx_csd_animation_replay.hpp"
#include "sward/ui_runtime/sgfx_hud_csd_project_loader.hpp"
#include "sward/ui_runtime/sgfx_hud_csd_cast_extractor.hpp"
#include "sward/ui_runtime/sgfx_hud_native_csd_renderer.hpp"
#include "sward/ui_runtime/sgfx_hud_font_renderer.hpp"

// Static-link SDL2 with no SDL_main shim (we provide our own main).
#define SDL_MAIN_HANDLED
#include <SDL.h>

// Phase 347: real BGM byte blob.
#include "res/music/installer.ogg.h"

// Phase 353: PNG screenshot writer (single header, public domain).
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <array>
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

    // Phase 346: baked font for the top-left screen caption (the
    // tiny "Title Intro" / "Pause" / etc. label that helps identify
    // each frame in the validation pack). Loaded once at startup
    // from a system TTF (defaults to arial.ttf on Windows). The
    // body-text font that previously painted Phase 346 placeholders
    // was removed in Phase 355 along with the placeholder painter.
    ui::SgfxBakedFont g_smallFont;

    // Phase 357: optional CSD animation replay. When the user
    // passes --csd-replay=<path>, the JSONL captured from
    // UnleashedRecomp's UI Lab is loaded here and queried per
    // render frame; the resulting per-target runtime overrides are
    // merged into the renderer's override list. Empty log == no-op.
    ui::CsdReplayLog g_csdReplay;
    double           g_csdReplayQueryTime = -1.0; // <0 = use frame clock

    // Map SgfxScreen -> the screen-token string the harvester writes
    // to JSONL events (matches targetTokenToScreen() above going the
    // other direction). Used by the replay path to filter events.
    inline std::string_view replayTargetTokenFor(ui::SgfxScreen s,
                                                 ui::StageMode m) noexcept
    {
        using S = ui::SgfxScreen;
        switch (s)
        {
            case S::TitleIntro: return "title-loop";
            case S::Title:      return "title-menu";
            case S::WorldMap:   return "world-map";
            case S::Loading:    return "loading";
            case S::StageHud:
                return (m == ui::StageMode::Werehog) ? "extra-stage-hud"
                                                     : "sonic-hud";
            case S::Pause:      return "pause";
            case S::Results:    return "result";
            case S::Hub:        return "townscreen";
        }
        return {};
    }

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
    //
    // Phase 355 (text-strip pass): the prior `kBlankTextScenes`
    // placeholder painter assumed the listed scenes had zero textured
    // casts and that retail rasterized their content at runtime. A
    // direct sweep of the YNCP native component map proves otherwise:
    // every `help_chara_*`, `help_text_area`, `progress` cast in
    // every in-scope project has hasTexture=true and references a
    // real DDS (`mat_title_003.dds`, `mat_help_*_en_*.dds`, etc.).
    // The "blank scenes" the renderer coverage audit reported were
    // skipped by Phase 291's sanity cap (oversized anchor-relative
    // quads), not by missing textures. Phase 354 retroactively
    // unlocked most of those via synthesizeOverrides() and the
    // active-scene whitelist. Painting placeholder text on top of
    // the real retail textures was producing the garbled overlay
    // visible at the bottom of the pre-Phase-355 Title Intro
    // composite. The painter is removed; real EN-region text strips
    // (`mat_*_en_*.dds`) come through the texture index automatically.

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

    // Phase 354: per-screen + per-state active-scene whitelist.
    //
    // Each retail .yncp project bundles many scenes (a "scene" being
    // a CSD scene-graph node, e.g. `mm_contentsitem_idle` vs
    // `mm_contentsitem_select`). The retail screen state machine
    // activates one subset at a time; rendering ALL scenes of a
    // project at once is what produced the chaotic overlap visible
    // in the pre-Phase-354 validation pack (Title Intro showing the
    // logo doubled, World Map's 32 scenes turning the canvas into
    // green stripes, etc.).
    //
    // The lists below are the steady-state "default view" of each
    // screen -- what the player sees when no transient sub-overlay
    // (popup, sub-tab, fanfare) is currently active. Sub-states the
    // orchestrator already tracks (Pause tab, Results rank) feed
    // into the appropriate variant below; transient overlays
    // (Werehog shields, ring-get pop, medal-get pop) are
    // event-triggered and intentionally omitted from the baseline.
    //
    // Empty whitelist == "no filter, render every cast" (host
    // fallback for projects we haven't catalogued).
    inline std::vector<std::string_view> activeScenesFor(
        ui::SgfxScreen screen,
        const ui::SgfxOrchestrator& orch)
    {
        using S  = ui::SgfxScreen;
        using SM = ui::StageMode;
        switch (screen)
        {
            case S::TitleIntro:
                // ui_title.yncp: bg + iconic SONIC + globe logo + the
                // EN-region title band + press-start text + loading
                // bar. `title_1` is the EN "Unleashed" word-mark
                // (txt_sonic, txt_unleashed, halos, ™); `title_2` is
                // the JP "World Adventure" word-mark BUT it also
                // owns the iconic Earth globe (img_earth) and the
                // lightning-bolt halo (img_abyss / img_abyss_halo).
                // SGFX includes BOTH scenes here and uses
                // castsToSkipFor() to drop title_2's JP text casts
                // while keeping the globe + lightning. The
                // `menu` / `menu_scroll` scenes belong to a separate
                // in-yncp menu state SGFX doesn't use (the actual
                // title menu lives in ui_mainmenu.yncp).
                return {"bg", "logo", "title_1", "title_2", "txt", "progress"};
            case S::Title:
                // ui_mainmenu.yncp default idle view. The `_intro`
                // variants are one-shot transitions; `_move` / `_select`
                // are cursor animations only on row navigation.
                return {"mm_base", "mm_bg_usual",
                        "mm_front_usual", "mm_title_usual",
                        "mm_contentsitem_idle", "mm_contentsitem_text",
                        "mm_donut_idle"};
            case S::WorldMap:
                // ui_worldmap.yncp default state: globe background +
                // scrollable stage selector + info panel for the
                // currently-highlighted stage. Skip popup overlays
                // (`cts_choices_*`, `cts_guide_*`, `cts_stage_window`,
                // `info_bg_1`, `info_img_*`) -- those only render when
                // the player opens the corresponding modal.
                return {"worldmap_background",
                        "worldmap_header_bg", "worldmap_header_img",
                        "worldmap_footer_bg", "worldmap_footer_img_A",
                        "cts_info_bg",
                        "cts_name", "cts_parts_flag", "cts_parts_sun_moon",
                        "cts_cursor", "cts_cursor_effect",
                        "cts_stage_scroll_bar", "cts_stage_scroll_bg",
                        "cts_stage_select"};
            case S::Loading:
                // ui_loading.yncp Miles-Electric PDA panel + load text.
                // `event_viewer` and `n_2_d` are in-yncp sub-modes the
                // game switches to for cutscene previews; skip them.
                return {"bg_1", "bg_2", "pda", "pda_txt", "loadinfo"};
            case S::StageHud:
                switch (orch.stageHud.mode)
                {
                    case SM::Werehog:
                    case SM::BossHit:
                        // ui_playscreen_ev[_hit].yncp Werehog HUD.
                        // The 15 `shield_NN` scenes are sub-state
                        // markers (one active at a time, indexing the
                        // Unleash gauge fill); the `unleash_gauge_effect*`
                        // scenes are the meter-full burst overlays.
                        // Both are event-triggered, omitted from
                        // baseline so the steady-state HUD is readable.
                        return {"score_count", "ring_count", "player_count",
                                "u_info", "exp_count",
                                "life", "life_bg",
                                "unleash_bar_1", "unleash_bg",
                                "unleash_body", "unleash_gauge"};
                    case SM::Boss:
                        // ui_playscreen_su.yncp: only 3 scenes total
                        // and all three render together (Sonic gauge +
                        // Gaia gauge + footer prompts).
                        return {"footer", "gaia_gauge", "su_sonic_gauge"};
                    case SM::DaySonic:
                    default:
                        // ui_playscreen.yncp Day Sonic HUD baseline.
                        // `medal_get_*` / `ring_get` are pickup pops.
                        return {"gauge_frame",
                                "score_count", "time_count",
                                "speed_count", "player_count", "exp_count",
                                "so_ringenagy_gauge", "so_speed_gauge",
                                "u_info"};
                }
            case S::Pause:
                // ui_pause.yncp -- system-common pause shell that
                // hosts Status / Skills / Settings sub-tabs. SGFX
                // captures the Skills sub-tab as the representative
                // pause snapshot since it's the visually richest
                // (yellow Sonic Unleashed branded panel + skill grid)
                // and it's what UnleashedRecomp lands on when you
                // press Start during gameplay. `bg_2` carries the
                // panel content; `skill_select` is the highlighted
                // skill cell ribbon; `skill_scroll_bar_bg` is the
                // skill list scroll track. The `btn_*`, `num`,
                // `stick`, `situation_text`, `tag_name_2/3`,
                // `scroll_bar_bg` scenes are tab/sub-state-conditional
                // and intentionally omitted from this snapshot.
                return {"bg_1", "bg_2", "bg_1_select",
                        "footer_A",
                        "icon", "tag", "status_title",
                        "skill_select", "skill_scroll_bar_bg",
                        "select", "arrow"};
            case S::Results:
            {
                // ui_result.yncp baseline: header + footer + new-record
                // banner + 6-row score block + the rank-letter scene
                // matching the cleared rank. (`result_rank_E` is in
                // the .yncp but isn't one of the 5 retail ranks
                // ResultsRank exposes -- it's an alt animation block.)
                std::vector<std::string_view> v = {
                    "result_title", "result_footer", "result_newR",
                    "result_num_1", "result_num_2", "result_num_3",
                    "result_num_4", "result_num_5", "result_num_6",
                    "result_rank"};
                switch (orch.results.rank)
                {
                    case ui::ResultsRank::S: v.push_back("result_rank_S"); break;
                    case ui::ResultsRank::A: v.push_back("result_rank_A"); break;
                    case ui::ResultsRank::B: v.push_back("result_rank_B"); break;
                    case ui::ResultsRank::C: v.push_back("result_rank_C"); break;
                    case ui::ResultsRank::D: v.push_back("result_rank_D"); break;
                }
                return v;
            }
            case S::Hub:
                // ui_townscreen.yncp: hub-world top bar (info / time /
                // camera hint) + footer prompts. `time_effect` is an
                // event burst overlay.
                return {"info", "footer", "time", "cam"};
        }
        return {}; // unknown screen -> no filter
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

    // Phase 354: per-screen cast-level skip list. The scene-name
    // whitelist gets us 95% of the way -- but a few retail .yncp
    // scenes mix region-specific text casts (e.g. JP word-mark) with
    // shared graphical casts (globe, lightning bolt) that the EN
    // build still needs. This helper supplies (sceneName, castName)
    // pairs to drop AFTER the scene whitelist passes.
    struct ScenedCastKey
    {
        std::string_view sceneName;
        std::string_view castName;
    };
    inline std::vector<ScenedCastKey> castsToSkipFor(
        ui::SgfxScreen screen,
        const ui::SgfxOrchestrator& /*orch*/)
    {
        if (screen == ui::SgfxScreen::TitleIntro)
        {
            // Drop the JP "Sonic World Adventure" text casts from
            // title_2 -- but keep its img_earth globe, halo, and
            // lightning-bolt casts since those are shared art the
            // EN title screen also displays alongside title_1's
            // "Unleashed" word-mark.
            return {
                {"title_2", "txt_KANA"},
                {"title_2", "txt_adventure"},
                {"title_2", "txt_world"},
                {"title_2", "txt_sonic"},
                {"title_2", "pale"},
                {"title_2", "txt_tm"},
            };
        }
        return {};
    }

    // Phase 354: synthesize runtime SetPosition anchors for screens
    // whose retail .yncp encodes anchor-relative coordinates. Boss
    // HUD's `su_sonic_gauge` / `gaia_gauge` / `footer` casts ship
    // with negative scene_left values (the gauge body sits to the
    // left of an anchor point retail sets at runtime via
    // CCastNode::SetPosition). The Phase 296 SetPosition harvest
    // captured anchors for Day-Sonic / Werehog HUD scenes but NOT
    // these Boss-only ones, so without an override they render
    // hundreds of pixels off-screen.
    //
    // The anchor pixel-positions below are runtime-inferred best
    // guesses computed from each cast's (sceneLeft, sceneWidth,
    // sceneTop, sceneHeight) and the desired on-canvas placement
    // (Sonic gauge top-left, Gaia gauge top-right, footer bottom-
    // center). Tag: pending-Ghidra -- a future SetPosition harvest
    // pass on the boss runtime would produce exact retail values.
    inline std::vector<ui::CsdNativeRuntimeOverride> synthesizeOverrides(
        ui::SgfxScreen screen,
        const ui::SgfxOrchestrator& orch,
        std::uint32_t canvasW,
        std::uint32_t canvasH)
    {
        std::vector<ui::CsdNativeRuntimeOverride> ov;
        if (screen != ui::SgfxScreen::StageHud) return ov;
        if (orch.stageHud.mode != ui::StageMode::Boss) return ov;
        const float w = static_cast<float>(canvasW);
        const float h = static_cast<float>(canvasH);
        // Compose: anchor that places body's geometric center at
        // (cx, cy) given a cast with sceneLeft=L, sceneTop=T,
        // sceneWidth=Ws, sceneHeight=Hs. Solving:
        //   anchorX = (cx - L - Ws/2) * w
        //   anchorY = (cy - T - Hs/2) * h
        // (Y formula: with sceneTop negative, sceneHeight positive,
        // anchor sits below the body.)
        auto pushAt = [&](const char* sceneName,
                          float cxNorm, float cyNorm,
                          float L, float Ws, float T, float Hs)
        {
            ui::CsdNativeRuntimeOverride o{};
            o.sceneName = sceneName;
            o.anchorXPx = (cxNorm - L - Ws * 0.5f) * w;
            o.anchorYPx = (cyNorm - T - Hs * 0.5f) * h;
            o.scaleX = 1.0f;
            o.scaleY = 1.0f;
            ov.push_back(std::move(o));
        };
        // Body cast metrics from yncp_native_component_map.json:
        //   su_sonic_gauge bg_gauge: L=-0.423, T=-0.179, Ws=0.365, Hs=0.019
        //   gaia_gauge     bg_gauge: L=-0.423, T=-0.130, Ws=0.365, Hs=0.019
        //   footer         txt_1   : L=-0.289, T=-0.224, Ws=0.195, Hs=0.133
        // Desired centers (normalized canvas coords):
        //   su_sonic_gauge -> top-left quarter (0.25, 0.10)
        //   gaia_gauge     -> top-right quarter (0.75, 0.10)
        //   footer         -> bottom-center (0.50, 0.90)
        pushAt("su_sonic_gauge", 0.25f, 0.10f, -0.423f, 0.365f, -0.179f, 0.019f);
        pushAt("gaia_gauge",     0.75f, 0.10f, -0.423f, 0.365f, -0.130f, 0.019f);
        pushAt("footer",         0.50f, 0.90f, -0.289f, 0.195f, -0.224f, 0.133f);
        return ov;
    }

    // Phase 365: orchestrator-state overlay. The retail digit / menu
    // row casts live as multi-cell pattern_index swaps the native
    // renderer doesn't yet drive, so until that lands we composite
    // the orchestrator-driven values + menu rows directly using the
    // Phase 346 stb_truetype font. Every value below comes from the
    // orchestrator state -- nothing is invented for the overlay.
    inline void overlayHudState(ui::CsdNativeFramebuffer& fb,
                                ui::SgfxScreen screen,
                                const ui::SgfxOrchestrator& orch)
    {
        if (!g_smallFont.loaded) return;

        constexpr std::array<std::uint8_t, 4> kWhite{255, 255, 255, 255};
        constexpr std::array<std::uint8_t, 4> kYellow{255, 220, 64, 255};
        constexpr std::array<std::uint8_t, 4> kCursor{255, 80, 32, 255};

        char buf[96];

        switch (screen)
        {
        case ui::SgfxScreen::Title:
        {
            // Right-side yellow strips run vertically down the canvas;
            // the retail layout has 5 rows centered on these baselines
            // (measured in the 02_title.png at 640x360 against the
            // ui_mainmenu.yncp output).
            constexpr int kRowX     = 410;
            constexpr int kRowYBase = 24;
            constexpr int kRowGap   = 56;
            static constexpr const char* kRowLabels[] = {
                "NEW FILE", "CONTINUE", "SETTINGS", "DLC", "EXIT",
            };
            const auto& tm = orch.title;
            const std::int32_t cur = tm.cursorIndex;
            for (int i = 0;
                 i < static_cast<int>(sizeof(kRowLabels) / sizeof(kRowLabels[0]));
                 ++i)
            {
                if (i < static_cast<int>(tm.optionVisible.size())
                    && !tm.optionVisible[static_cast<std::size_t>(i)])
                    continue;
                const auto color = (i == cur) ? kCursor : kWhite;
                ui::compositeText(fb, g_smallFont, kRowLabels[i],
                                  kRowX, kRowYBase + i * kRowGap, color);
            }
            break;
        }

        case ui::SgfxScreen::StageHud:
        {
            const auto& s = orch.stageHud;
            const int xCol = 24;
            int y = 28;
            std::snprintf(buf, sizeof(buf), "RING:  %d", s.rings);
            ui::compositeText(fb, g_smallFont, buf, xCol, y, kYellow); y += 18;
            std::snprintf(buf, sizeof(buf), "SCORE: %lld",
                          static_cast<long long>(s.score));
            ui::compositeText(fb, g_smallFont, buf, xCol, y, kWhite); y += 18;
            const int totalSec  = static_cast<int>(s.timeSeconds);
            const int minutes   = totalSec / 60;
            const int seconds   = totalSec % 60;
            const int frac100   = static_cast<int>(
                (s.timeSeconds - static_cast<float>(totalSec)) * 100.0f);
            std::snprintf(buf, sizeof(buf), "TIME:  %02d:%02d.%02d",
                          minutes, seconds, frac100);
            ui::compositeText(fb, g_smallFont, buf, xCol, y, kWhite); y += 18;
            std::snprintf(buf, sizeof(buf), "LIVES: %d", s.lives);
            ui::compositeText(fb, g_smallFont, buf, xCol, y, kWhite); y += 18;
            std::snprintf(buf, sizeof(buf), "SPEED GAUGE: %3d%%",
                          static_cast<int>(s.speedGaugeFill * 100.0f));
            ui::compositeText(fb, g_smallFont, buf, xCol, y, kWhite); y += 18;
            std::snprintf(buf, sizeof(buf), "RING ENERGY: %3d%%",
                          static_cast<int>(s.ringEnergyGaugeFill * 100.0f));
            ui::compositeText(fb, g_smallFont, buf, xCol, y, kWhite); y += 18;
            if (s.mode == ui::StageMode::Werehog
                || s.mode == ui::StageMode::BossHit)
            {
                std::snprintf(buf, sizeof(buf), "DARK GAIA: %3d%%",
                              static_cast<int>(s.darkGaiaEnergy * 100.0f));
                ui::compositeText(fb, g_smallFont, buf, xCol, y, kYellow); y += 18;
                std::snprintf(buf, sizeof(buf), "GRAPPLES: %u",
                              static_cast<unsigned>(s.outOfControlCount));
                ui::compositeText(fb, g_smallFont, buf, xCol, y, kWhite); y += 18;
            }
            if (s.paused)
                ui::compositeText(fb, g_smallFont, "[ PAUSED ]",
                                  xCol, y, kCursor);
            break;
        }

        case ui::SgfxScreen::Pause:
        {
            const auto& p = orch.pause;
            int y = 60;
            // PauseMenuContext drives which rows retail shows; SGFX
            // mirrors the retail row sets per Phase 323.
            using Ctx = ui::PauseMenuContext;
            std::vector<const char*> rows;
            switch (p.context)
            {
            case Ctx::WorldMap:
                rows = {"CONTINUE", "STATUS", "SKILLS", "SETTINGS", "QUIT"};
                break;
            case Ctx::Stage:
                rows = {"CONTINUE", "RESTART", "SETTINGS", "QUIT"};
                break;
            case Ctx::Village:
            case Ctx::Hub:
                rows = {"CONTINUE", "STATUS", "INVENTORY",
                        "SETTINGS", "RETURN"};
                break;
            case Ctx::Misc:
                rows = {"CONTINUE", "SETTINGS", "QUIT"};
                break;
            }
            const int rowsCount = static_cast<int>(rows.size());
            const int cur       = p.cursorIndex;
            for (int i = 0; i < rowsCount; ++i)
            {
                const auto color = (i == cur) ? kCursor : kWhite;
                ui::compositeText(fb, g_smallFont, rows[i],
                                  220, y + i * 28, color);
            }
            break;
        }

        case ui::SgfxScreen::Results:
        {
            const auto& r = orch.results;
            const int xLabel = 220;
            const int xValue = 360;
            int y = 60;
            const char* rankStr = "?";
            switch (r.rank)
            {
            case ui::ResultsRank::S: rankStr = "S"; break;
            case ui::ResultsRank::A: rankStr = "A"; break;
            case ui::ResultsRank::B: rankStr = "B"; break;
            case ui::ResultsRank::C: rankStr = "C"; break;
            case ui::ResultsRank::D: rankStr = "D"; break;
            }
            std::snprintf(buf, sizeof(buf), "RANK: %s", rankStr);
            ui::compositeText(fb, g_smallFont, buf, xLabel, y, kYellow);
            y += 28;

            const int totalSec = static_cast<int>(r.timeSeconds);
            const int minutes  = totalSec / 60;
            const int seconds  = totalSec % 60;
            std::snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);
            ui::compositeText(fb, g_smallFont, "TIME",  xLabel, y, kWhite);
            ui::compositeText(fb, g_smallFont, buf,     xValue, y, kWhite);
            y += 26;
            std::snprintf(buf, sizeof(buf), "%d", r.rings);
            ui::compositeText(fb, g_smallFont, "RINGS", xLabel, y, kWhite);
            ui::compositeText(fb, g_smallFont, buf,     xValue, y, kWhite);
            y += 26;
            std::snprintf(buf, sizeof(buf), "%lld",
                          static_cast<long long>(r.specialScore));
            ui::compositeText(fb, g_smallFont, "SPECIAL", xLabel, y, kWhite);
            ui::compositeText(fb, g_smallFont, buf,       xValue, y, kWhite);
            y += 26;
            std::snprintf(buf, sizeof(buf), "%lld",
                          static_cast<long long>(r.score));
            ui::compositeText(fb, g_smallFont, "SCORE", xLabel, y, kWhite);
            ui::compositeText(fb, g_smallFont, buf,     xValue, y, kWhite);
            y += 26;
            std::snprintf(buf, sizeof(buf), "%lld",
                          static_cast<long long>(r.totalScore));
            ui::compositeText(fb, g_smallFont, "TOTAL", xLabel, y, kYellow);
            ui::compositeText(fb, g_smallFont, buf,     xValue, y, kYellow);
            break;
        }

        case ui::SgfxScreen::Hub:
        {
            const auto& h = orch.hub;
            const char* modeStr = (h.mode == ui::HubMode::Werehog)
                ? "Werehog (Night)"
                : "Day Sonic";
            const char* overlayStr = "";
            switch (h.overlay)
            {
            case ui::HubOverlay::None:         overlayStr = "exploring"; break;
            case ui::HubOverlay::BalloonText:  overlayStr = "NPC dialog"; break;
            case ui::HubOverlay::ShopMenu:     overlayStr = "shop menu"; break;
            case ui::HubOverlay::StageGate:    overlayStr = "stage gate"; break;
            case ui::HubOverlay::MissionBrief: overlayStr = "mission brief"; break;
            case ui::HubOverlay::TownMap:      overlayStr = "town map"; break;
            }
            std::snprintf(buf, sizeof(buf), "HUB: %s -- %s",
                          modeStr, overlayStr);
            ui::compositeText(fb, g_smallFont, buf, 24, 28, kYellow);
            const int totalSec = static_cast<int>(h.timeOfDaySeconds);
            std::snprintf(buf, sizeof(buf), "TOD: %ds", totalSec);
            ui::compositeText(fb, g_smallFont, buf, 24, 46, kWhite);
            break;
        }

        case ui::SgfxScreen::Loading:
        {
            ui::compositeText(fb, g_smallFont,
                              "LOADING NEXT SCREEN...",
                              24, 28, kYellow);
            break;
        }

        case ui::SgfxScreen::WorldMap:
        {
            // World Map cursor / continent comes from the orchestrator
            // state once the WorldMap screen ports land; until then
            // the static label keeps the screen reading as itself.
            ui::compositeText(fb, g_smallFont,
                              "WORLD MAP -- press A to enter stage",
                              24, 28, kYellow);
            break;
        }

        case ui::SgfxScreen::TitleIntro:
            // TitleIntro is purely texture-driven; no row labels or
            // counters. Caption already covers it.
            break;
        }
    }

    void renderScreenIntoFramebuffer(ui::SgfxScreen screen,
                                     const ui::SgfxOrchestrator& orch,
                                     ui::CsdNativeFramebuffer& fb)
    {
        // Clear to dark gray (so blank areas read as "rendered, not
        // crashed").
        fb.resize(kCanvasW, kCanvasH, {16, 16, 24, 255});
        const std::string projectPath = projectPathForScreen(screen, orch.stageHud);
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

        // Phase 354: drive the composite loop through the per-state
        // active-scene whitelist so only the scenes the retail screen
        // state machine would have visible at this moment paint into
        // the framebuffer. Empty whitelist means "render all" for
        // projects we haven't catalogued.
        const auto activeScenes = activeScenesFor(screen, orch);
        const auto skipCasts   = castsToSkipFor(screen, orch);
        const bool filterEnabled = !activeScenes.empty();
        auto sceneActive = [&](const std::string& sn) noexcept
        {
            if (!filterEnabled) return true;
            for (const auto& a : activeScenes)
                if (a == sn) return true;
            return false;
        };
        auto castSkipped = [&](const std::string& sn,
                               const std::string& cn) noexcept
        {
            for (const auto& sk : skipCasts)
                if (sk.sceneName == sn && sk.castName == cn) return true;
            return false;
        };

        // Merge baked .yncp overrides (currently unused) with the
        // per-screen synthesized anchors required by anchor-relative
        // projects (Boss HUD), plus any replay-driven overrides
        // sourced from a captured ui_lab_csd_setposition.jsonl.
        std::vector<ui::CsdNativeRuntimeOverride> mergedOverrides =
            assets.overrides;
        const auto synthesized = synthesizeOverrides(screen, orch,
                                                     fb.width, fb.height);
        for (const auto& s : synthesized) mergedOverrides.push_back(s);

        // Phase 357: replay overrides. No-op when no log is loaded.
        // The current replay events use synthesized scene names of
        // the form "node_<HEX>" because the upstream harvester does
        // not yet enrich events with scene/cast names; once the
        // Phase 358 harvester pass lands, the same replay path
        // provides per-cast overrides keyed by retail scene name.
        if (!g_csdReplay.events.empty())
        {
            const double t = g_csdReplayQueryTime;
            const auto target = replayTargetTokenFor(screen, orch.stageHud.mode);
            const auto replayOvs =
                ui::buildRuntimeOverridesAt(g_csdReplay, t, target);
            for (const auto& ov : replayOvs)
                mergedOverrides.push_back(ov);
        }

        for (const auto& cmd : assets.commands)
        {
            if (!sceneActive(cmd.sceneName)) continue;
            if (castSkipped(cmd.sceneName, cmd.castName)) continue;
            (void)ui::compositeCommand(
                fb, cmd, g_assetRoot, assets.textureCache, mergedOverrides);
        }

        // Phase 346: top-left screen caption so each variant is
        // visually labeled even when CSD assets are unfamiliar.
        if (g_smallFont.loaded)
        {
            ui::compositeText(fb, g_smallFont,
                              screenCaption(screen, orch.stageHud.mode),
                              6, 4, {220, 230, 255, 255});
        }

        // Phase 365: orchestrator-state overlay. The retail digit /
        // menu-row casts use multi-cell pattern_index swaps the
        // native renderer does not yet drive (single-cell-per-cast
        // baseline only). Until that lands, we composite the
        // orchestrator-driven values + menu rows directly onto the
        // framebuffer using the Phase 346 stb_truetype font so each
        // screen reads as a real Sonic Unleashed UI surface instead
        // of an empty asset shell. Source of every value below is
        // the orchestrator state -- nothing is invented for the
        // overlay.
        if (g_smallFont.loaded)
            overlayHudState(fb, screen, orch);
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
    fs::path screenshotsDir; // Phase 353: PNG-per-screen dump mode
    fs::path bgmBankDir;     // Phase 356: pre-converted OGG bank
    fs::path csdReplayPath;  // Phase 357: ui_lab_csd_setposition.jsonl
    double   replayTimeSec = -1.0; // <0 means "follow demo cycle clock"

    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--silent") silent = true;
        else if (a.rfind("--jsonl=", 0) == 0) jsonlPath = a.substr(8);
        else if (a.rfind("--asset-root=", 0) == 0) assetRoot = a.substr(13);
        else if (a.rfind("--screen=", 0) == 0) forceScreen = a.substr(9);
        else if (a.rfind("--demo-seconds=", 0) == 0) demoSeconds = std::stof(a.substr(15));
        else if (a.rfind("--frames=", 0) == 0) maxFrames = std::stoi(a.substr(9));
        else if (a.rfind("--screenshots-dir=", 0) == 0) screenshotsDir = a.substr(18);
        else if (a.rfind("--bgm-bank=", 0) == 0) bgmBankDir = a.substr(11);
        else if (a.rfind("--csd-replay=", 0) == 0) csdReplayPath = a.substr(13);
        else if (a.rfind("--replay-time=", 0) == 0) replayTimeSec = std::stod(a.substr(14));
        else if (a == "--help" || a == "-h")
        {
            std::cout << "sgfx_ui_mirror -- SGFX native UI render in a window\n"
                      << "  --silent                  no audio\n"
                      << "  --jsonl=<path>            tail UnleashedRecomp's ui_lab_events.jsonl\n"
                      << "  --asset-root=<dir>        retail .yncp / .dds asset root\n"
                      << "  --screen=<name>           force a screen (Title/WorldMap/...)\n"
                      << "  --demo-seconds=<n>        per-screen dwell in demo cycle\n"
                      << "  --frames=<n>              exit after N frames\n"
                      << "  --screenshots-dir=<dir>   visit every screen+stage variant once,\n"
                      << "                            dump a PNG per visit, write a manifest,\n"
                      << "                            then exit (no SDL window opened)\n"
                      << "  --bgm-bank=<dir>          load pre-converted OGG cues from <dir>\n"
                      << "                            (filename stem == retail cue name)\n"
                      << "  --csd-replay=<path>       replay UnleashedRecomp's harvested\n"
                      << "                            ui_lab_csd_setposition.jsonl as the\n"
                      << "                            renderer's runtime override stream\n"
                      << "  --replay-time=<seconds>   query the replay log at this absolute\n"
                      << "                            time (default: follow demo cycle)\n";
            return 0;
        }
    }

    g_assetRoot = assetRoot;

    // Phase 353: screenshots-dir mode runs HEADLESS -- no SDL window,
    // no audio. Visits every screen + stage variant, renders to a
    // framebuffer, dumps PNG per visit, writes a JSON manifest,
    // then exits. Lets the user open the PNGs directly to compare
    // visually against UnleashedRecomp without juggling two windows.
    if (!screenshotsDir.empty())
    {
        std::error_code ec;
        fs::create_directories(screenshotsDir, ec);

        std::cout << "scanning asset textures under " << assetRoot << " ...\n";
        buildTextureIndex(assetRoot);
        std::cout << "  texture index size: " << g_textureIndex.size() << "\n";

        // Phase 357: load replay log if requested.
        if (!csdReplayPath.empty())
        {
            g_csdReplay = ui::loadCsdReplayLog(csdReplayPath);
            g_csdReplayQueryTime = (replayTimeSec >= 0.0)
                                   ? replayTimeSec
                                   : g_csdReplay.lastTimeSeconds;
            std::cout << "  csd replay: events=" << g_csdReplay.events.size()
                      << " parseErrors=" << g_csdReplay.parseErrors
                      << " lastFrame=" << g_csdReplay.lastFrame
                      << " queryTime=" << g_csdReplayQueryTime << "s\n";
        }

        // Bake the small font so the top-left screen caption renders.
        const fs::path arial = "C:/Windows/Fonts/arial.ttf";
        g_smallFont = ui::loadFontFromFile(arial, 14.0f);
        std::cout << "  fonts: small="
                  << (g_smallFont.loaded ? "ok" : "missing") << "\n";

        struct ShotSpec
        {
            ui::SgfxScreen screen;
            ui::StageMode  stageMode;
            const char*    label;
            const char*    fileName;
        };
        const ShotSpec kShots[] = {
            { ui::SgfxScreen::TitleIntro, ui::StageMode::DaySonic, "TitleIntro",       "01_title_intro.png" },
            { ui::SgfxScreen::Title,      ui::StageMode::DaySonic, "Title",            "02_title.png" },
            { ui::SgfxScreen::WorldMap,   ui::StageMode::DaySonic, "WorldMap",         "03_world_map.png" },
            { ui::SgfxScreen::Loading,    ui::StageMode::DaySonic, "Loading",          "04_loading.png" },
            { ui::SgfxScreen::StageHud,   ui::StageMode::DaySonic, "StageHud Day",     "05_stage_hud_day.png" },
            { ui::SgfxScreen::StageHud,   ui::StageMode::Werehog,  "StageHud Werehog", "06_stage_hud_werehog.png" },
            { ui::SgfxScreen::StageHud,   ui::StageMode::Boss,     "StageHud Boss",    "07_stage_hud_boss.png" },
            { ui::SgfxScreen::Pause,      ui::StageMode::DaySonic, "Pause",            "08_pause.png" },
            { ui::SgfxScreen::Results,    ui::StageMode::DaySonic, "Results",          "09_results.png" },
            { ui::SgfxScreen::Hub,        ui::StageMode::DaySonic, "Hub",              "10_hub.png" },
        };

        ui::SgfxOrchestrator orch;
        ui::CsdNativeFramebuffer fb;
        fb.resize(kCanvasW, kCanvasH);

        std::ofstream manifest(screenshotsDir / "manifest.json");
        manifest << "{\n  \"phase\": \"356\",\n"
                 << "  \"purpose\": \"per-screen composite PNGs from sgfx_ui_mirror's "
                    "native CSD renderer; Phase 354 added active-scene whitelist + "
                    "per-cast skip + synthesized SetPosition anchors; Phase 355 "
                    "removed the placeholder text painter so retail EN text "
                    "strips (mat_*_en_*.dds) come through cleanly; Phase 356 "
                    "added the OGG BGM bank loader and the in-stage HudItemGet "
                    "popup state machine (port of D:\\\\SonicWorldAdventure\\\\"
                    "SWA\\\\source\\\\HUD\\\\Item\\\\HudItemGet.cpp).\",\n"
                 << "  \"canvas\": { \"width\": " << kCanvasW
                 << ", \"height\": " << kCanvasH << " },\n"
                 << "  \"screens\": [\n";

        bool first = true;
        for (const auto& sh : kShots)
        {
            orch.current = sh.screen;
            orch.stageHud.mode = sh.stageMode;
            renderScreenIntoFramebuffer(sh.screen, orch, fb);
            const fs::path outPath = screenshotsDir / sh.fileName;
            const int rc = stbi_write_png(
                outPath.string().c_str(),
                static_cast<int>(fb.width), static_cast<int>(fb.height),
                4, fb.rgba.data(), static_cast<int>(fb.width * 4));
            const bool ok = (rc != 0);
            std::cout << "  [shot] " << sh.label
                      << " -> " << outPath.string()
                      << " (" << (ok ? "ok" : "FAILED") << ")\n";
            if (!first) manifest << ",\n";
            first = false;
            manifest << "    { \"label\": \"" << sh.label
                     << "\", \"file\": \"" << sh.fileName
                     << "\", \"screen\": \"" << specFor(sh.screen).friendlyName
                     << "\", \"stage_mode\": " << static_cast<int>(sh.stageMode)
                     << ", \"written\": " << (ok ? "true" : "false") << " }";
        }
        manifest << "\n  ],\n"
                 << "  \"comparison_workflow\": [\n"
                 << "    \"1. Open each PNG in this directory.\",\n"
                 << "    \"2. Capture the same screen from UnleashedRecomp (e.g. via Win+PrintScreen).\",\n"
                 << "    \"3. Side-by-side compare: layout, colors, text positions, asset choice.\",\n"
                 << "    \"4. Note deltas in sgfx_visual_validation_findings.md if present.\"\n"
                 << "  ]\n}\n";
        std::cout << "  manifest: " << (screenshotsDir / "manifest.json").string() << "\n";
        return 0;
    }

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

    // Phase 356: BGM bank lives outside the if-silent block so the
    // byte buffers stay alive for the whole process lifetime (the
    // audio player keeps raw pointers into bank.bytes[]).
    ui::SgfxBgmBank bgmBank;

    if (!silent)
    {
        if (!ui::sgfxAudioPlayerInit())
        {
            std::cerr << "sgfxAudioPlayerInit failed; continuing silent\n";
            silent = true;
        }
        else
        {
            // Phase 347 fallback: register the embedded installer
            // OGG blob as the bytes for every demo cue. This makes
            // the demo cycle audible even when no real BGM bank is
            // supplied. If --bgm-bank=<dir> is passed, the loader
            // below replaces these registrations with real cues.
            const char* kBgmCues[] = {
                "bgm_sys_title", "bgm_sys_worldmap", "bgm_act_apotos",
                "bgm_act_hub_apotos", "bgm_sys_result", "bgm_sys_result_ng",
            };
            for (const char* name : kBgmCues)
                ui::sgfxAudioPlayerRegisterBgm(name, g_installer_music,
                                               sizeof(g_installer_music));

            // Phase 356: load real per-cue OGGs from the host-
            // supplied bank directory. The bank's filename stems
            // are the cue names retail uses; loading them after
            // the fallbacks above lets a partial bank coexist with
            // the installer-OGG fallback for any cue the host
            // hasn't pre-converted yet.
            if (!bgmBankDir.empty())
            {
                const auto loaded =
                    ui::sgfxBgmBankLoadFromDir(bgmBank, bgmBankDir);
                ui::sgfxBgmBankRegisterAllWithPlayer(bgmBank);
                std::cout << "  bgm bank: dir=" << bgmBankDir.string()
                          << " loaded=" << loaded
                          << " registered=" << bgmBank.registeredCount
                          << " skipped_non_ogg=" << bgmBank.skippedNonOggCount
                          << " failed_read=" << bgmBank.failedReadCount << "\n";
            }
        }
    }

    std::cout << "scanning asset textures under " << assetRoot << " ...\n";
    buildTextureIndex(assetRoot);
    std::cout << "  texture index size: " << g_textureIndex.size() << "\n";

    // Phase 357: load replay log if requested.
    if (!csdReplayPath.empty())
    {
        g_csdReplay = ui::loadCsdReplayLog(csdReplayPath);
        g_csdReplayQueryTime = (replayTimeSec >= 0.0)
                               ? replayTimeSec
                               : 0.0; // live mode advances this per frame
        std::cout << "  csd replay: events=" << g_csdReplay.events.size()
                  << " parseErrors=" << g_csdReplay.parseErrors
                  << " lastFrame=" << g_csdReplay.lastFrame << "\n";
    }

    // Phase 346: bake the small font for the top-left screen caption.
    // Defaults to Windows Arial; if missing we silently leave
    // g_smallFont.loaded == false and skip the caption.
    {
        const fs::path arial = "C:/Windows/Fonts/arial.ttf";
        g_smallFont = ui::loadFontFromFile(arial, 14.0f);
        std::cout << "  fonts: small="
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
        renderScreenIntoFramebuffer(orch.current, orch, fb);

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
        // Phase 357: tick the replay clock at ~60fps unless the
        // user pinned it via --replay-time=<n>.
        if (!csdReplayPath.empty() && replayTimeSec < 0.0)
        {
            g_csdReplayQueryTime += 1.0 / 60.0;
            // Loop the replay so the demo cycle stays visible.
            if (g_csdReplay.lastTimeSeconds > 0.0
                && g_csdReplayQueryTime > g_csdReplay.lastTimeSeconds)
                g_csdReplayQueryTime = 0.0;
        }
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
