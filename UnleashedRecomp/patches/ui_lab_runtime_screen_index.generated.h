#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace UiLab::GeneratedRuntimeScreenIndex
{
    struct Row
    {
        std::string_view project;
        std::string_view token;
        std::string_view label;
        std::string_view systemId;
        std::string_view systemName;
        std::string_view sourceFamily;
        std::string_view dataSource;
    };

    static constexpr std::string_view kGeneratedAt = "2026-05-04T10:49:34.923063+00:00";
    static constexpr std::string_view kAssetIndexPath = "out\\runtime_bridge_asset_index_current_install.json";
    static constexpr std::string_view kDatabasePath = "research_uiux\\data\\ui_archaeology_database.json";
    static constexpr size_t kAssetEntryCount = 47517;

    static constexpr std::array<Row, 28> kRows =
    {{
        { "ui_boss_gauge", "boss-gauge", "Boss Gauge", "boss_hud", "Boss HUD", "ui_archaeology_database:boss_hud", "ui_archaeology_database:boss_hud" },
        { "ui_boss_name", "boss-name", "Boss Name", "boss_hud", "Boss HUD", "ui_archaeology_database:boss_hud", "ui_archaeology_database:boss_hud" },
        { "ui_itemresult", "item-result", "Item Result Runtime", "item_result", "Item Result", "CSD/ui_itemresult runtime project (source file pending)", "ui_archaeology_database:item_result" },
        { "ui_loading", "loading", "Loading / Miles Electric", "loading_and_start", "Loading And Start/Clear", "System/Loading.cpp", "ui_archaeology_database:loading_and_start" },
        { "ui_start", "loading-start", "Loading Start/Clear", "loading_and_start", "Loading And Start/Clear", "System/Loading.cpp", "ui_archaeology_database:loading_and_start" },
        { "ui_gate", "gate", "Gate", "mission_briefing_and_gate", "Mission Briefing And Gate", "ui_archaeology_database:mission_briefing_and_gate", "ui_archaeology_database:mission_briefing_and_gate" },
        { "ui_missionscreen", "missionscreen", "Missionscreen", "mission_briefing_and_gate", "Mission Briefing And Gate", "ui_archaeology_database:mission_briefing_and_gate", "ui_archaeology_database:mission_briefing_and_gate" },
        { "ui_misson", "misson", "Misson", "mission_briefing_and_gate", "Mission Briefing And Gate", "ui_archaeology_database:mission_briefing_and_gate", "ui_archaeology_database:mission_briefing_and_gate" },
        { "ui_result", "result", "Stage Result", "mission_result_family", "Mission Result Family", "HUD/Result/Result.cpp", "ui_archaeology_database:mission_result_family" },
        { "ui_result_ex", "result-ex", "EX Stage Result", "mission_result_family", "Mission Result Family", "HUD/Result/Result.cpp", "ui_archaeology_database:mission_result_family" },
        { "ui_general", "general", "General", "pause_stack", "Pause Stack", "ui_archaeology_database:pause_stack", "ui_archaeology_database:pause_stack" },
        { "ui_help", "help", "Help", "pause_stack", "Pause Stack", "ui_archaeology_database:pause_stack", "ui_archaeology_database:pause_stack" },
        { "ui_pause", "pause", "Pause Menu", "pause_stack", "Pause Stack", "HUD/Pause/HudPause.cpp", "ui_archaeology_database:pause_stack" },
        { "ui_end", "end", "End", "save_and_ending", "Save And Ending", "ui_archaeology_database:save_and_ending", "ui_archaeology_database:save_and_ending" },
        { "ui_saveicon", "saveicon", "Saveicon", "save_and_ending", "Save And Ending", "ui_archaeology_database:save_and_ending", "ui_archaeology_database:save_and_ending" },
        { "ui_playscreen", "sonic-hud", "Sonic Stage HUD", "sonic_stage_hud", "Sonic Stage HUD", "Player/Character/Sonic/Hud/SonicMainDisplay.cpp", "runtime-live-evidence-overlay" },
        { "ui_status", "status", "Status / Skill Upgrade", "status_overlay", "Status Overlay", "HUD/Status/Status.cpp", "ui_archaeology_database:status_overlay" },
        { "ui_mainmenu", "title-runtime", "Title Runtime", "title_menu", "Title Menu", "System/GameMode/Title/TitleStateIntro.cpp|TitleMenu.cpp", "ui_archaeology_database:title_menu" },
        { "ui_title", "title-runtime", "Title Runtime", "title_menu", "Title Menu", "System/GameMode/Title/TitleStateIntro.cpp|TitleMenu.cpp", "runtime-live-evidence-overlay" },
        { "ui_exstage", "exstage", "Exstage", "tornado_defense", "Tornado Defense / EX Stage", "ui_archaeology_database:tornado_defense", "ui_archaeology_database:tornado_defense" },
        { "ui_prov_playscreen", "extra-stage-hud", "Extra Stage / Tornado HUD", "tornado_defense", "Tornado Defense / EX Stage", "ExtraStage/Tails/Hud/HudExQte.cpp", "ui_archaeology_database:tornado_defense" },
        { "ui_qte", "qte", "Qte", "tornado_defense", "Tornado Defense / EX Stage", "ui_archaeology_database:tornado_defense", "ui_archaeology_database:tornado_defense" },
        { "ui_balloon", "balloon", "Balloon", "town_ui", "Town UI", "ui_archaeology_database:town_ui", "ui_archaeology_database:town_ui" },
        { "ui_mediaroom", "mediaroom", "Mediaroom", "town_ui", "Town UI", "ui_archaeology_database:town_ui", "ui_archaeology_database:town_ui" },
        { "ui_shop", "shop", "Shop", "town_ui", "Town UI", "ui_archaeology_database:town_ui", "ui_archaeology_database:town_ui" },
        { "ui_townscreen", "townscreen", "Townscreen", "town_ui", "Town UI", "ui_archaeology_database:town_ui", "ui_archaeology_database:town_ui" },
        { "ui_worldmap", "world-map", "World Map", "world_map_stack", "World Map Stack", "System/GameMode/WorldMap/WorldMapSelect.cpp", "ui_archaeology_database:world_map_stack" },
        { "ui_worldmap_help", "world-map-help", "World Map Help", "world_map_stack", "World Map Stack", "System/GameMode/WorldMap/WorldMapTutorial.cpp", "ui_archaeology_database:world_map_stack" },
    }};
}
