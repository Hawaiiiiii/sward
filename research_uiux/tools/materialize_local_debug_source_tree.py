#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


GAMEPLAY_HUD_GROUPS = {
    "sonic_stage_hud": {
        "contract": "sonic_stage_hud_reference.json",
        "paths": [
            "HUD/Sonic/HudSonicStage.cpp",
            "HUD/Sonic/SonicMainDisplay.cpp",
            "HUD/Item/HudItemGet.cpp",
            "Player/Character/Sonic/Hud/SonicHudGuide.cpp",
            "Player/Character/Sonic/Hud/SonicHudHomingAttack.cpp",
        ],
        "roles": [
            ("time_count", "counter", "Top-left timer ownership."),
            ("score_count", "counter", "Top-left score stack."),
            ("exp_count", "counter", "EXP tally shell."),
            ("ring_count", "counter", "Primary ring counter."),
            ("so_speed_gauge", "gauge", "Speed/boost gauge band."),
            ("so_ringenagy_gauge", "gauge", "Ring-energy gauge band."),
            ("u_info", "sidecar", "Bonus/info sidecar cluster."),
            ("item_get_overlay", "transient_fx", "Shared item-get overlay probe for the in-stage HUD shell."),
        ],
    },
    "werehog_stage_hud": {
        "contract": "werehog_stage_hud_reference.json",
        "paths": [
            "HUD/Evil/HudEvilStage.cpp",
            "HUD/Evil/EvilMainDisplay.cpp",
            "Player/Character/EvilSonic/Hud/EvilHudGuide.cpp",
            "Player/Character/EvilSonic/Hud/EvilHudTarget.cpp",
        ],
        "roles": [
            ("gauge/life", "gauge", "Life gauge stack."),
            ("gauge/shield", "gauge", "Shield shard ownership."),
            ("hit_counter_num", "counter", "Hit-counter overlay."),
            ("chance_attack", "transient_fx", "Chance-attack burst shell."),
            ("target_overlay", "sidecar", "Guide/target helper shell."),
        ],
    },
    "extra_stage_hud": {
        "contract": "extra_stage_hud_reference.json",
        "paths": [
            "ExtraStage/Tails/Hud/HudExQte.cpp",
        ],
        "roles": [
            ("info_1", "counter", "Primary score/counter shell."),
            ("info_2", "counter", "Secondary counter shell."),
            ("so_speed_gauge", "gauge", "Speed gauge."),
            ("so_ringenagy_gauge", "gauge", "Energy gauge."),
            ("ui_qte", "prompt", "Controller-prompt stream."),
        ],
    },
    "super_sonic_hud": {
        "contract": "super_sonic_hud_reference.json",
        "paths": [
            "Boss/BossHudSuperSonic.cpp",
            "Boss/BossHudVitality.cpp",
            "Boss/BossNamePlate.cpp",
        ],
        "roles": [
            ("su_sonic_gauge", "gauge", "Super Sonic gauge rail."),
            ("gaia_gauge", "gauge", "Dark Gaia gauge rail."),
            ("boss_nameplate", "content", "Shared boss-name strip."),
            ("footer", "footer", "Bottom status/footer strip."),
        ],
    },
    "boss_hud": {
        "contract": "boss_hud_reference.json",
        "paths": [
            "Boss/FinalDarkGaia/Object/FinalDarkGaiaHud.cpp",
            "Boss/Phoenix/Hud/PhoenixHudVitality.cpp",
        ],
        "roles": [
            ("ui_boss_gauge", "gauge", "Boss vitality gauge shell."),
            ("ui_boss_name", "content", "Boss-name overlay."),
            ("phase_banner", "footer", "Phase / warning footer shell."),
        ],
    },
}

STAGE_TEST_TARGETS = {
    "System/GameMode/GameModeStageAchievementTest.cpp": [
        ("Boss HUD", "Boss/FinalDarkGaia/Object/FinalDarkGaiaHud.cpp", "boss_hud_reference.json", "Boss-facing debug probe for vitality/nameplate cadence."),
    ],
    "System/GameMode/GameModeStageEvilTest.cpp": [
        ("Werehog HUD", "HUD/Evil/HudEvilStage.cpp", "werehog_stage_hud_reference.json", "Night-stage HUD probe for unleash/shield/hit-counter ownership."),
    ],
    "System/GameMode/GameModeStageForwardTest.cpp": [
        ("Sonic HUD", "HUD/Sonic/HudSonicStage.cpp", "sonic_stage_hud_reference.json", "Day-stage HUD probe for counters, medals, and boost/ring gauges."),
    ],
    "System/GameMode/GameModeStageInstallTest.cpp": [
        ("Loading / Install", "HUD/Loading/Loading.cpp", "loading_transition_reference.json", "Install/loading handoff validation."),
    ],
    "System/GameMode/GameModeStageLoadXML.cpp": [
        ("Extra Stage HUD", "ExtraStage/Tails/Hud/HudExQte.cpp", "extra_stage_hud_reference.json", "XML-driven stage bootstrap that can host the extra-stage HUD probe."),
    ],
    "System/GameMode/GameModeStageMotionTest.cpp": [
        ("Extra Stage HUD", "ExtraStage/Tails/Hud/HudExQte.cpp", "extra_stage_hud_reference.json", "Motion/QTE-facing debug surface for EX-stage prompt timing."),
    ],
    "System/GameMode/GameModeStageSaveTest.cpp": [
        ("Save / Ending", "System/GameMode/Ending/EndingManager.cpp", "autosave_toast_reference.json", "Save/ending handoff validation."),
    ],
    "System/GameMode/GameModeStageScreenshot.cpp": [
        ("Sonic HUD", "HUD/Sonic/HudSonicStage.cpp", "sonic_stage_hud_reference.json", "Screenshot-facing HUD visibility probe."),
        ("Boss HUD", "Boss/BossHudVitality.cpp", "super_sonic_hud_reference.json", "Boss-facing overlay capture probe."),
    ],
    "System/GameMode/GameModeStageSwapDiskTest.cpp": [
        ("Loading / Boot", "HUD/Loading/Loading.cpp", "loading_transition_reference.json", "Swap-disk/loading handoff validation."),
    ],
}

TOWN_BINDINGS = {
    "HUD/MediaRoom/MediaRoom.cpp": [("ui_mediaroom", "Town_Labo_Common/ui_mediaroom.yncp", "Media Room browser shell.")],
    "HUD/MediaRoom/MediaRoomDetail.cpp": [("ui_mediaroom", "Town_Labo_Common/ui_mediaroom.yncp", "Media Room detail pane.")],
    "HUD/MediaRoom/MediaRoomItemList.cpp": [("ui_mediaroom", "Town_Labo_Common/ui_mediaroom.yncp", "Media Room list column.")],
    "HUD/MediaRoom/MediaRoomSelectCountry.cpp": [("ui_mediaroom", "Town_Labo_Common/ui_mediaroom.yncp", "Country-selection step.")],
    "HUD/MediaRoom/MediaRoomSelectItem.cpp": [("ui_mediaroom", "Town_Labo_Common/ui_mediaroom.yncp", "Media Room item-selection step.")],
    "Town/ShopWindow.cpp": [("ui_shop", "Town_Common/ui_shop.yncp", "Shop buy/sell framing.")],
    "Town/ShopParamManager.cpp": [("ui_shop", "Town_Common/ui_shop.yncp", "Shop catalog data backing.")],
    "Town/HotdogParamManager.cpp": [("ui_townscreen", "Town_Common/ui_townscreen.yncp", "Town challenge/hotdog parameter backing.")],
    "Town/HotdogSaveManager.cpp": [("ui_townscreen", "Town_Common/ui_townscreen.yncp", "Town challenge save-data ownership.")],
    "Town/ItemParamManager.cpp": [("ui_townscreen", "Town_Common/ui_townscreen.yncp", "Town item parameter catalog.")],
    "Town/KyojuParamManager.cpp": [("ui_townscreen", "Town_Common/ui_townscreen.yncp", "Town NPC parameter backing.")],
    "Town/KyojuPresentManager.cpp": [("ui_townscreen", "Town_Common/ui_townscreen.yncp", "Town present/event dispatch.")],
    "Town/SunafkinParamManager.cpp": [("ui_townscreen", "Town_Common/ui_townscreen.yncp", "Town vendor parameter backing.")],
    "Town/TimeTableManager.cpp": [("ui_townscreen", "Town_Common/ui_townscreen.yncp", "Town schedule/day-night routing.")],
    "Town/TalkWindow.cpp": [("ui_balloon", "Town_Common/ui_balloon.yncp", "Conversation balloon shell."), ("ui_townscreen", "Town_Common/ui_townscreen.yncp", "Town lower-third shell.")],
    "Town/DialogScript.cpp": [("ui_balloon", "Town_Common/ui_balloon.yncp", "Conversation script ownership."), ("ui_townscreen", "Town_Common/ui_townscreen.yncp", "Town message routing.")],
    "Town/TownManager.cpp": [("ui_townscreen", "Town_Common/ui_townscreen.yncp", "Town-level screen manager."), ("ui_balloon", "Town_Common/ui_balloon.yncp", "Balloon/dialog dispatch.")],
    "Camera/Controller/TownShopCamera.cpp": [("ui_shop", "Town_Common/ui_shop.yncp", "Shop camera pairing with menu shell.")],
    "Camera/Controller/TownTalkCamera.cpp": [("ui_balloon", "Town_Common/ui_balloon.yncp", "Talk camera pairing with balloon shell.")],
}

CAMERA_BINDINGS = {
    "Camera/Controller/FreeCamera.cpp": [("Tooling / Debug UI", "Tool/InspirePreview/InspirePreview.cpp", "Free-view inspection host beside the debug sandbox.")],
    "Camera/Controller/GoalCamera.cpp": [("Mission Result Family", "HUD/Common/Result/HudResult.cpp", "Goal/result presentation camera handoff.")],
    "Camera/Controller/Player2DBossCamera.cpp": [("Boss / Final HUD Hosts", "Boss/BossHudVitality.cpp", "2D boss camera shell adjacent to boss HUD timing.")],
    "Camera/Controller/Player3DBossCamera.cpp": [("Boss / Final HUD Hosts", "Boss/BossHudVitality.cpp", "3D boss camera shell adjacent to vitality/nameplate ownership.")],
    "Camera/Controller/Player3DFinalDarkGaiaCamera.cpp": [("Boss / Final HUD Hosts", "Boss/BossHudSuperSonic.cpp", "Final Dark Gaia camera bridge into Super Sonic/final HUD.")],
    "Camera/Controller/PlayerSuperSonicCamera.cpp": [("Boss / Final HUD Hosts", "Boss/BossHudSuperSonic.cpp", "Super Sonic camera pairing with final HUD contract.")],
    "Replay/Camera/ReplayFreeCamera.cpp": [("Frontend Camera Shell", "Camera/Controller/FreeCamera.cpp", "Replay-oriented inspection camera.")],
    "Replay/Camera/ReplayRelativeCamera.cpp": [("Frontend Camera Shell", "Camera/Controller/GoalCamera.cpp", "Replay-relative presentation camera.")],
}

SEQUENCE_BINDINGS = {
    "Sequence/Core/SequenceHandleUnit.cpp": [
        ("Frontend Sequence Hosts", "Sequence/Unit/SequenceUnitFactory.cpp", "frontend_sequence_shell_reference.json", "Core sequence handle shell that advances per-unit ownership."),
    ],
    "Sequence/Core/SequenceManagerImpl.cpp": [
        ("Frontend Sequence Hosts", "Sequence/Core/SequenceHandleUnit.cpp", "frontend_sequence_shell_reference.json", "Sequence-manager shell that owns route dispatch across frontend units."),
    ],
    "Sequence/Unit/SequenceUnitFactory.cpp": [
        ("Frontend Sequence Hosts", "Sequence/Core/SequenceManagerImpl.cpp", "frontend_sequence_shell_reference.json", "Unit-factory shell that maps sequence ids to concrete UI/cutscene handlers."),
    ],
    "Sequence/Unit/SequenceUnitUnlockAchievement.cpp": [
        ("Frontend Sequence Hosts", "Sequence/Core/SequenceManagerImpl.cpp", "frontend_sequence_shell_reference.json", "Achievement-unlock unit routed through the shared sequence shell."),
    ],
    "Sequence/Unit/SequenceUnitCallHelpWindow.cpp": [
        ("Pause Stack", "HUD/HelpWindow/HelpWindow.cpp", "pause_menu_reference.json", "Help-window callout unit that routes into the pause/help contract."),
    ],
    "Sequence/Unit/SequenceUnitChangeStage.cpp": [
        ("Loading / Boot / Install", "HUD/Loading/Loading.cpp", "loading_transition_reference.json", "Stage-change sequence unit that routes into the loading shell."),
    ],
    "Sequence/Unit/SequenceUnitMicroSequence.cpp": [
        ("Subtitle / Cutscene Presentation", "Tool/InspirePreview/InspirePreview.cpp", "subtitle_cutscene_reference.json", "Micro-sequence shell that stays adjacent to cutscene/timeline playback."),
    ],
    "Sequence/Unit/SequenceUnitPlayMovie.cpp": [
        ("Subtitle / Cutscene Presentation", "Tool/InspirePreview/InspirePreview.cpp", "subtitle_cutscene_reference.json", "Movie sequence unit that routes into subtitle/cutscene playback."),
    ],
    "Sequence/Unit/SequenceUnitSendMediaRoomMessage.cpp": [
        ("Town / Media Room Hosts", "HUD/MediaRoom/MediaRoom.cpp", "town_ui_reference.json", "Media-room message dispatch routed through the town/media-room shell."),
    ],
    "Sequence/Unit/SequenceUnitSendTownMessage.cpp": [
        ("Town / Media Room Hosts", "Town/TownManager.cpp", "town_ui_reference.json", "Town message dispatch routed through the town shell."),
    ],
    "Sequence/Utility/SequenceChangeStageUnit.cpp": [
        ("Loading / Boot / Install", "Sequence/Unit/SequenceUnitChangeStage.cpp", "loading_transition_reference.json", "Stage-change utility wrapper above the loading shell."),
    ],
    "Sequence/Utility/SequencePlayMovieWrapper.cpp": [
        ("Subtitle / Cutscene Presentation", "Sequence/Unit/SequenceUnitPlayMovie.cpp", "subtitle_cutscene_reference.json", "Movie-wrapper helper around the cutscene sequence unit."),
    ],
}

SYSTEM_BINDINGS = {
    "System/Application.cpp": [
        ("Title / Main Menu", "System/GameMode/Title/TitleMenu.cpp", "title_menu_reference.json", "Application boot into title/menu shell."),
        ("Loading / Boot / Install", "HUD/Loading/Loading.cpp", "loading_transition_reference.json", "Application-level loading handoff."),
    ],
    "System/ApplicationDocument.cpp": [
        ("World Map Stack", "System/GameMode/WorldMap/WorldMapSelect.cpp", "world_map_reference.json", "Document-level world map ownership."),
        ("Save / Ending", "System/GameMode/Ending/EndingManager.cpp", "autosave_toast_reference.json", "Save/ending document handoff."),
    ],
    "System/ApplicationSetting.cpp": [
        ("Pause Stack", "HUD/Pause/HudPause.cpp", "pause_menu_reference.json", "Frontend settings/pause adjacency."),
    ],
    "System/Game.cpp": [
        ("Gameplay HUD Hosts", "HUD/Sonic/HudSonicStage.cpp", "sonic_stage_hud_reference.json", "Game-level day-stage HUD ownership."),
        ("Subtitle / Cutscene Presentation", "Tool/InspirePreview/InspirePreview.cpp", "subtitle_cutscene_reference.json", "Game-level cutscene/movie adjacency."),
    ],
    "System/GameDocument.cpp": [
        ("Gameplay HUD Hosts", "HUD/Evil/HudEvilStage.cpp", "werehog_stage_hud_reference.json", "Document-level Werehog HUD ownership."),
        ("World Map Stack", "System/GameMode/WorldMap/WorldMapSelect.cpp", "world_map_reference.json", "Document-level world map adjacency."),
    ],
    "System/GameParameter.cpp": [
        ("Gameplay HUD Hosts", "ExtraStage/Tails/Hud/HudExQte.cpp", "extra_stage_hud_reference.json", "Parameter layer adjacent to EX-stage HUD selection."),
    ],
    "System/DynamicLoad.cpp": [
        ("Loading / Boot / Install", "HUD/Loading/Loading.cpp", "loading_transition_reference.json", "Dynamic loading gate."),
    ],
    "System/DynamicLoadGITexture.cpp": [
        ("Loading / Boot / Install", "HUD/Loading/Loading.cpp", "loading_transition_reference.json", "GI texture preload alongside loading shell."),
    ],
    "System/DynamicLoadTerrain.cpp": [
        ("Loading / Boot / Install", "HUD/Loading/Loading.cpp", "loading_transition_reference.json", "Terrain preload alongside loading shell."),
    ],
    "System/NextStagePreloadingManager.cpp": [
        ("Loading / Boot / Install", "HUD/Loading/Loading.cpp", "loading_transition_reference.json", "Next-stage preloading handoff."),
    ],
    "System/StageListManager.cpp": [
        ("World Map Stack", "System/GameMode/WorldMap/WorldMapMission.cpp", "world_map_reference.json", "Stage list ownership adjacent to world map selection."),
    ],
    "System/StageManager.cpp": [
        ("Gameplay HUD Hosts", "HUD/Sonic/HudSonicStage.cpp", "sonic_stage_hud_reference.json", "Stage-level day-stage HUD ownership."),
        ("Gameplay HUD Hosts", "HUD/Evil/HudEvilStage.cpp", "werehog_stage_hud_reference.json", "Stage-level Werehog HUD ownership."),
    ],
    "System/TerrainManager.cpp": [
        ("Gameplay HUD Hosts", "ExtraStage/Tails/Hud/HudExQte.cpp", "extra_stage_hud_reference.json", "Terrain shell adjacent to EX-stage HUD validation."),
    ],
    "System/TerrainManager2nd.cpp": [
        ("Gameplay HUD Hosts", "Boss/BossHudSuperSonic.cpp", "super_sonic_hud_reference.json", "Secondary terrain shell adjacent to final HUD bridge."),
    ],
    "System/World.cpp": [
        ("Gameplay HUD Hosts", "HUD/Sonic/HudSonicStage.cpp", "sonic_stage_hud_reference.json", "World-level stage HUD routing."),
        ("Boss / Final HUD Hosts", "Boss/BossHudSuperSonic.cpp", "super_sonic_hud_reference.json", "World-level boss/final HUD adjacency."),
    ],
    "System/GameMode/GameModeStage.cpp": [
        ("Gameplay HUD Hosts", "HUD/Sonic/HudSonicStage.cpp", "sonic_stage_hud_reference.json", "Stage-mode bootstrap for in-stage HUD ownership."),
        ("Subtitle / Cutscene Presentation", "System/GameMode/GameModeStageMovie.cpp", "subtitle_cutscene_reference.json", "Stage-mode movie/cutscene handoff."),
    ],
    "System/GameMode/GameModeBoot.cpp": [
        ("Application / World Shell Hosts", "System/Application.cpp", "application_world_shell_reference.json", "Boot flow shell for the wider application/world runtime contract."),
    ],
    "System/GameMode/GameModeMainMenu.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/Title/TitleMenu.cpp", "application_world_shell_reference.json", "Main menu gamemode shell above the title/menu host layer."),
    ],
    "System/GameMode/GameModeStageMovie.cpp": [
        ("Subtitle / Cutscene Presentation", "Tool/InspirePreview/InspirePreview.cpp", "subtitle_cutscene_reference.json", "Stage movie bridge into Inspire/cutscene presentation."),
    ],
    "System/GameMode/Title/TitleManager.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/Title/TitleMenu.cpp", "application_world_shell_reference.json", "Title manager ownership for the application/world shell."),
    ],
    "System/GameMode/Title/TitleMenu.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/Title/TitleMenu.cpp", "application_world_shell_reference.json", "Title menu host inside the wider app/world shell."),
        ("Title / Main Menu", "System/GameMode/Title/TitleMenu.cpp", "title_menu_reference.json", "Direct title/menu contract anchor."),
    ],
    "System/GameMode/Title/TitleStateIntro.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/Title/TitleManager.cpp", "application_world_shell_reference.json", "Title intro dispatch state."),
    ],
    "System/GameMode/Title/TitleStateWorldMap.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/WorldMap/WorldMapSelect.cpp", "application_world_shell_reference.json", "Title-to-world-map bridge state."),
        ("World Map Stack", "System/GameMode/WorldMap/WorldMapSelect.cpp", "world_map_reference.json", "World map handoff anchor."),
    ],
    "System/GameMode/MainMenu/MainMenuManager.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/Title/TitleMenu.cpp", "application_world_shell_reference.json", "Frontend main-menu manager shell."),
        ("Title / Main Menu", "System/GameMode/Title/TitleMenu.cpp", "title_menu_reference.json", "Direct main menu manager anchor."),
    ],
    "System/GameMode/WorldMap/WorldMapListBox.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/WorldMap/WorldMapSelect.cpp", "application_world_shell_reference.json", "World map listbox shell inside the application/world host bucket."),
        ("World Map Stack", "System/GameMode/WorldMap/WorldMapSelect.cpp", "world_map_reference.json", "Direct world map list ownership."),
    ],
    "System/GameMode/WorldMap/WorldMapMission.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/WorldMap/WorldMapSelect.cpp", "application_world_shell_reference.json", "World map mission selector shell."),
        ("World Map Stack", "System/GameMode/WorldMap/WorldMapSelect.cpp", "world_map_reference.json", "World map mission ownership."),
    ],
    "System/GameMode/WorldMap/WorldMapObject.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/WorldMap/WorldMapSelect.cpp", "application_world_shell_reference.json", "World map object shell."),
        ("World Map Stack", "System/GameMode/WorldMap/WorldMapSelect.cpp", "world_map_reference.json", "World map object ownership."),
    ],
    "System/GameMode/WorldMap/WorldMapSelect.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/WorldMap/WorldMapSelect.cpp", "application_world_shell_reference.json", "Primary world map selection host in the broader shell."),
        ("World Map Stack", "System/GameMode/WorldMap/WorldMapSelect.cpp", "world_map_reference.json", "Direct world map contract anchor."),
    ],
    "System/GameMode/WorldMap/WorldMapSimpleInfo.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/WorldMap/WorldMapSelect.cpp", "application_world_shell_reference.json", "World map info sidecar shell."),
        ("World Map Stack", "System/GameMode/WorldMap/WorldMapSelect.cpp", "world_map_reference.json", "World map simple-info ownership."),
    ],
    "System/GameMode/WorldMap/WorldMapTutorial.cpp": [
        ("Application / World Shell Hosts", "System/GameMode/WorldMap/WorldMapSelect.cpp", "application_world_shell_reference.json", "World map tutorial/guide shell."),
        ("World Map Stack", "System/GameMode/WorldMap/WorldMapSelect.cpp", "world_map_reference.json", "World map tutorial ownership."),
    ],
    "System/GameMode/Loader/DatabaseTree.cpp": [
        ("Loading / Boot / Install", "HUD/Loading/Loading.cpp", "loading_transition_reference.json", "Database/load-tree shell."),
    ],
    "System/GameMode/Loader/StageLoaderXML.cpp": [
        ("Gameplay HUD Hosts", "ExtraStage/Tails/Hud/HudExQte.cpp", "extra_stage_hud_reference.json", "XML stage loader adjacent to EX-stage HUD bootstrap."),
    ],
}


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def cpp_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def normalize_key(value: str) -> str:
    return value.replace("\\\\", "\\").replace("\\", "/").lower()


def make_header(relative_source_path: str, entry: dict[str, Any], evidence_inputs: list[str]) -> list[str]:
    source_path = entry.get("source_path", relative_source_path)
    status = entry.get("status", "named_seed_only")
    family = entry.get("family_name", "Unknown")
    lines = [
        "// Local-only SWARD humanized source scaffold.",
        "// This file is not original SEGA source and is intentionally kept out of git.",
        f"// Recovered source path: {source_path}",
        f"// Family: {family}",
        f"// Status at generation time: {status}",
        "// Evidence inputs:",
    ]
    for evidence in evidence_inputs:
        lines.append(f"// - {evidence}")
    lines.extend(
        [
            "",
            "#include <array>",
            "#include <string_view>",
            "",
            "namespace sward_local",
            "{",
        ]
    )
    return lines


def make_footer() -> list[str]:
    return [
        "} // namespace sward_local",
        "",
    ]


def lookup_entry(entries_by_path: dict[str, dict[str, Any]], relative_source_path: str) -> dict[str, Any]:
    key = normalize_key(relative_source_path)
    found = entries_by_path.get(key)
    if found:
        return found
    return {
        "source_path": relative_source_path,
        "relative_source_path": relative_source_path,
        "family_name": "Local Debug-Oriented Source Expansion",
        "status": "local_only_humanized",
    }


def mirror_cpp_path(mirror_root: Path, relative_source_path: str) -> Path:
    return mirror_root / "SonicWorldAdventure" / "SWA" / "source" / Path(relative_source_path.replace("/", "\\"))


def render_gameplay_hud_source(relative_source_path: str, entry: dict[str, Any], system_id: str, system: dict[str, Any]) -> str:
    config = GAMEPLAY_HUD_GROUPS[system_id]
    layout_rows = [
        (
            layout["layout_id"],
            "true" if layout.get("loose_layout_present", False) else "false",
            ", ".join(Path(path).name for path in system.get("loose_layout_files", [])) or "none",
        )
        for layout in system.get("layout_families", [])
    ]
    lines = make_header(
        relative_source_path,
        entry,
        [
            "research_uiux/data/gameplay_hud_core_map.json",
            "research_uiux/runtime_reference/contracts/" + config["contract"],
            "Match SU OG source code folders and locations.txt",
        ],
    )
    stem = Path(relative_source_path).stem
    lines.extend(
        [
            "struct HudLayoutBinding",
            "{",
            "    std::string_view layoutId;",
            "    bool looseLayoutPresent;",
            "    std::string_view looseLayoutHints;",
            "};",
            "",
            "struct HudOverlayProbe",
            "{",
            "    std::string_view overlayId;",
            "    std::string_view role;",
            "    std::string_view notes;",
            "};",
            "",
            f"struct {stem}Host",
            "{",
            f'    static constexpr std::string_view kPreferredContract = "{cpp_string(config["contract"])}";',
            f'    static constexpr std::string_view kSystemId = "{cpp_string(system_id)}";',
            "",
            "    [[nodiscard]] static constexpr auto BuildLayoutBindings()",
            "    {",
            f"        return std::array<HudLayoutBinding, {len(layout_rows)}>{{",
        ]
    )
    for index, (layout_id, loose_layout, loose_hint) in enumerate(layout_rows):
        lines.append("            HudLayoutBinding{")
        lines.append(f'                "{cpp_string(layout_id)}",')
        lines.append(f"                {loose_layout},")
        lines.append(f'                "{cpp_string(loose_hint)}",')
        lines.append("            }" + ("," if index + 1 != len(layout_rows) else ""))
    lines.extend(
        [
            "        };",
            "    }",
            "",
            "    [[nodiscard]] static constexpr auto BuildOverlayProbes()",
            "    {",
            f"        return std::array<HudOverlayProbe, {len(config['roles'])}>{{",
        ]
    )
    for index, (overlay_id, role, notes) in enumerate(config["roles"]):
        lines.append("            HudOverlayProbe{")
        lines.append(f'                "{cpp_string(overlay_id)}",')
        lines.append(f'                "{cpp_string(role)}",')
        lines.append(f'                "{cpp_string(notes)}",')
        lines.append("            }" + ("," if index + 1 != len(config["roles"]) else ""))
    lines.extend(
        [
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def render_stage_test_source(relative_source_path: str, entry: dict[str, Any]) -> str:
    probes = STAGE_TEST_TARGETS[relative_source_path]
    lines = make_header(
        relative_source_path,
        entry,
        [
            "Research SU.txt",
            "research_uiux/data/frontend_shell_recovery.json",
            "research_uiux/runtime_reference/contracts/*.json",
        ],
    )
    stem = Path(relative_source_path).stem
    lines.extend(
        [
            "struct DebugStageProbe",
            "{",
            "    std::string_view label;",
            "    std::string_view sourcePath;",
            "    std::string_view contractFile;",
            "    std::string_view notes;",
            "};",
            "",
            f"struct {stem}Host",
            "{",
            "    [[nodiscard]] static constexpr auto BuildStageProbes()",
            "    {",
            f"        return std::array<DebugStageProbe, {len(probes)}>{{",
        ]
    )
    for index, (label, source_path, contract_file, notes) in enumerate(probes):
        lines.append("            DebugStageProbe{")
        lines.append(f'                "{cpp_string(label)}",')
        lines.append(f'                "{cpp_string(source_path)}",')
        lines.append(f'                "{cpp_string(contract_file)}",')
        lines.append(f'                "{cpp_string(notes)}",')
        lines.append("            }" + ("," if index + 1 != len(probes) else ""))
    lines.extend(
        [
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def render_town_source(relative_source_path: str, entry: dict[str, Any]) -> str:
    bindings = TOWN_BINDINGS[relative_source_path]
    lines = make_header(
        relative_source_path,
        entry,
        [
            "research_uiux/data/ui_archaeology_database.json",
            "research_uiux/data/layout_code_correlation.json",
            "Match SU OG source code folders and locations.txt",
        ],
    )
    stem = Path(relative_source_path).stem
    lines.extend(
        [
            "struct TownLayoutBinding",
            "{",
            "    std::string_view layoutId;",
            "    std::string_view looseLayoutHint;",
            "    std::string_view notes;",
            "};",
            "",
            f"struct {stem}Shell",
            "{",
            "    [[nodiscard]] static constexpr auto BuildTownLayoutBindings()",
            "    {",
            f"        return std::array<TownLayoutBinding, {len(bindings)}>{{",
        ]
    )
    for index, (layout_id, loose_hint, notes) in enumerate(bindings):
        lines.append("            TownLayoutBinding{")
        lines.append(f'                "{cpp_string(layout_id)}",')
        lines.append(f'                "{cpp_string(loose_hint)}",')
        lines.append(f'                "{cpp_string(notes)}",')
        lines.append("            }" + ("," if index + 1 != len(bindings) else ""))
    lines.extend(
        [
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def render_camera_source(relative_source_path: str, entry: dict[str, Any]) -> str:
    bindings = CAMERA_BINDINGS[relative_source_path]
    lines = make_header(
        relative_source_path,
        entry,
        [
            "research_uiux/data/ui_source_path_manifest.json",
            "research_uiux/data/frontend_shell_recovery.json",
        ],
    )
    stem = Path(relative_source_path).stem
    lines.extend(
        [
            "struct CameraUiBinding",
            "{",
            "    std::string_view ownerFamily;",
            "    std::string_view sourcePath;",
            "    std::string_view notes;",
            "};",
            "",
            f"struct {stem}Shell",
            "{",
            "    [[nodiscard]] static constexpr auto BuildCameraBindings()",
            "    {",
            f"        return std::array<CameraUiBinding, {len(bindings)}>{{",
        ]
    )
    for index, (owner_family, source_path, notes) in enumerate(bindings):
        lines.append("            CameraUiBinding{")
        lines.append(f'                "{cpp_string(owner_family)}",')
        lines.append(f'                "{cpp_string(source_path)}",')
        lines.append(f'                "{cpp_string(notes)}",')
        lines.append("            }" + ("," if index + 1 != len(bindings) else ""))
    lines.extend(
        [
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def render_sequence_source(relative_source_path: str, entry: dict[str, Any]) -> str:
    bindings = SEQUENCE_BINDINGS[relative_source_path]
    lines = make_header(
        relative_source_path,
        entry,
        [
            "Research SU.txt",
            "research_uiux/data/ui_source_path_manifest.json",
            "research_uiux/runtime_reference/contracts/*.json",
        ],
    )
    stem = Path(relative_source_path).stem
    lines.extend(
        [
            "struct SequenceOwnershipBinding",
            "{",
            "    std::string_view ownerFamily;",
            "    std::string_view sourcePath;",
            "    std::string_view contractFile;",
            "    std::string_view notes;",
            "};",
            "",
            f"struct {stem}Shell",
            "{",
            "    [[nodiscard]] static constexpr auto BuildSequenceBindings()",
            "    {",
            f"        return std::array<SequenceOwnershipBinding, {len(bindings)}>{{",
        ]
    )
    for index, (owner_family, source_path, contract_file, notes) in enumerate(bindings):
        lines.append("            SequenceOwnershipBinding{")
        lines.append(f'                "{cpp_string(owner_family)}",')
        lines.append(f'                "{cpp_string(source_path)}",')
        lines.append(f'                "{cpp_string(contract_file)}",')
        lines.append(f'                "{cpp_string(notes)}",')
        lines.append("            }" + ("," if index + 1 != len(bindings) else ""))
    lines.extend(
        [
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def render_system_source(relative_source_path: str, entry: dict[str, Any]) -> str:
    bindings = SYSTEM_BINDINGS[relative_source_path]
    lines = make_header(
        relative_source_path,
        entry,
        [
            "research_uiux/data/ui_source_path_manifest.json",
            "research_uiux/runtime_reference/contracts/*.json",
            "Match SU OG source code folders and locations.txt",
        ],
    )
    stem = Path(relative_source_path).stem
    lines.extend(
        [
            "struct ShellOwnershipBinding",
            "{",
            "    std::string_view ownerFamily;",
            "    std::string_view sourcePath;",
            "    std::string_view contractFile;",
            "    std::string_view notes;",
            "};",
            "",
            f"struct {stem}Shell",
            "{",
            "    [[nodiscard]] static constexpr auto BuildOwnershipBindings()",
            "    {",
            f"        return std::array<ShellOwnershipBinding, {len(bindings)}>{{",
        ]
    )
    for index, (owner_family, source_path, contract_file, notes) in enumerate(bindings):
        lines.append("            ShellOwnershipBinding{")
        lines.append(f'                "{cpp_string(owner_family)}",')
        lines.append(f'                "{cpp_string(source_path)}",')
        lines.append(f'                "{cpp_string(contract_file)}",')
        lines.append(f'                "{cpp_string(notes)}",')
        lines.append("            }" + ("," if index + 1 != len(bindings) else ""))
    lines.extend(
        [
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def build_target_groups(entries_by_path: dict[str, dict[str, Any]], gameplay_payload: dict[str, Any]) -> list[dict[str, Any]]:
    systems_by_id = {system["system_id"]: system for system in gameplay_payload.get("systems", [])}
    groups = []

    gameplay_targets = []
    for system_id, config in GAMEPLAY_HUD_GROUPS.items():
        system = systems_by_id.get(system_id)
        if not system:
            continue
        for relative_source_path in config["paths"]:
            gameplay_targets.append(
                {
                    "relative_source_path": relative_source_path,
                    "entry": lookup_entry(entries_by_path, relative_source_path),
                    "renderer": "gameplay_hud",
                    "system_id": system_id,
                    "system": system,
                }
            )

    groups.append(
        {
            "group_id": "gameplay_hud_sources",
            "display_name": "Gameplay HUD Sources",
            "purpose": "Readable local-only HUD ownership scaffolds for Sonic, Werehog, EX-stage, and boss/final HUD families.",
            "targets": gameplay_targets,
        }
    )

    groups.append(
        {
            "group_id": "stage_test_sources",
            "display_name": "Stage Test Sources",
            "purpose": "Readable local-only stage-test probes that point the debug game modes at current runtime contracts.",
            "targets": [
                {
                    "relative_source_path": relative_source_path,
                    "entry": lookup_entry(entries_by_path, relative_source_path),
                    "renderer": "stage_test",
                }
                for relative_source_path in STAGE_TEST_TARGETS
            ],
        }
    )

    groups.append(
        {
            "group_id": "town_ui_sources",
            "display_name": "Town / Media Room Sources",
            "purpose": "Readable local-only town, talk, shop, and media-room scaffolds tied back to extracted town layouts.",
            "targets": [
                {
                    "relative_source_path": relative_source_path,
                    "entry": lookup_entry(entries_by_path, relative_source_path),
                    "renderer": "town",
                }
                for relative_source_path in TOWN_BINDINGS
            ],
        }
    )

    groups.append(
        {
            "group_id": "camera_shell_sources",
            "display_name": "Camera Shell Sources",
            "purpose": "Readable local-only camera scaffolds for replay, goal, and free-camera ownership around the UI stack.",
            "targets": [
                {
                    "relative_source_path": relative_source_path,
                    "entry": lookup_entry(entries_by_path, relative_source_path),
                    "renderer": "camera",
                }
                for relative_source_path in CAMERA_BINDINGS
            ],
        }
    )

    groups.append(
        {
            "group_id": "frontend_sequence_sources",
            "display_name": "Frontend Sequence Sources",
            "purpose": "Readable local-only sequence-core and sequence-unit scaffolds tied back to the new frontend sequence shell and its downstream runtime families.",
            "targets": [
                {
                    "relative_source_path": relative_source_path,
                    "entry": lookup_entry(entries_by_path, relative_source_path),
                    "renderer": "sequence",
                }
                for relative_source_path in SEQUENCE_BINDINGS
            ],
        }
    )

    groups.append(
        {
            "group_id": "application_world_sources",
            "display_name": "Application / World Shell Sources",
            "purpose": "Readable local-only system-shell scaffolds linking application/world/stage hosts back to current runtime families.",
            "targets": [
                {
                    "relative_source_path": relative_source_path,
                    "entry": lookup_entry(entries_by_path, relative_source_path),
                    "renderer": "system",
                }
                for relative_source_path in SYSTEM_BINDINGS
            ],
        }
    )

    return groups


def render_target(target: dict[str, Any]) -> str:
    relative_source_path = target["relative_source_path"]
    entry = target["entry"]
    renderer = target["renderer"]
    if renderer == "gameplay_hud":
        return render_gameplay_hud_source(relative_source_path, entry, target["system_id"], target["system"])
    if renderer == "stage_test":
        return render_stage_test_source(relative_source_path, entry)
    if renderer == "town":
        return render_town_source(relative_source_path, entry)
    if renderer == "camera":
        return render_camera_source(relative_source_path, entry)
    if renderer == "sequence":
        return render_sequence_source(relative_source_path, entry)
    return render_system_source(relative_source_path, entry)


def write_tracked_markdown(summary_payload: dict[str, Any], output_path: Path) -> None:
    summary = summary_payload["summary"]
    lines = [
        '<p align="right">',
        '    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>',
        "</p>",
        "",
        '# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Local Debug-Oriented Source Tree Expansion',
        "",
        "This pass widens the local-only readable source layer from the first menu/cutscene hosts into gameplay HUD, stage-test, town, camera, sequence, and application/world shell paths.",
        "",
        "> [!IMPORTANT]",
        "> These files stay local-only under `SONIC UNLEASHED/`. The tracked repo carries the materializer plus the summary, not the mirrored files themselves.",
        "",
        "## Snapshot",
        "",
        f"- New local-only readable `.cpp` files added in this phase: `{summary['new_humanized_source_file_count']}`",
        f"- Total local-only readable `.cpp` files under `SONIC UNLEASHED/`: `{summary['total_humanized_source_file_count']}`",
        f"- Expansion groups added: `{summary['group_count']}`",
        "",
        "## Group Breakdown",
        "",
        "| Group | Files | Purpose |",
        "|---|---:|---|",
    ]
    for group in summary_payload["groups"]:
        lines.append(f"| {group['display_name']} | `{group['file_count']}` | {group['purpose']} |")

    lines.extend(
        [
            "",
            "## What Changed",
            "",
            "- Gameplay HUD hosts now have local-only readable ownership scaffolds tied to the new runtime contracts.",
            "- Stage-test game modes now read like probe hosts instead of only path-dump notes.",
            "- Town/media-room, camera, sequence, and application/world shells now carry readable ownership bindings instead of remaining note-only placeholders.",
            "- The mirrored local source tree is materially closer to a debug-oriented source layout instead of a note staging area.",
        ]
    )
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_local_readme(mirror_root: Path, total_humanized_count: int) -> None:
    readme_path = mirror_root / "_meta" / "README.txt"
    readme_path.parent.mkdir(parents=True, exist_ok=True)
    readme_path.write_text(
        "\n".join(
            [
                "# SONIC UNLEASHED Local Mirrored Source Tree",
                "",
                "This directory is local-only and intentionally kept out of git.",
                "",
                "Purpose:",
                "- mirror the source-path dump into a stable local folder scaffold",
                "- hold local-only `*.sward.md` source-family placement notes beside the recovered original-style paths",
                "- hold local-only readable `.cpp` humanization scaffolds for the expanding debug-oriented source layer",
                "- give future translated cleanup a destination that resembles the original source-family layout",
                "",
                "Current local layers:",
                "- note suffix pattern: `<original source path>.sward.md`",
                f"- total humanized source files: {total_humanized_count}",
                "",
                "Generated by:",
                "- `research_uiux/tools/materialize_source_family_notes.py`",
                "- `research_uiux/tools/materialize_local_debug_source_tree.py`",
                "",
            ]
        ),
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Expand the local-only SONIC UNLEASHED mirror with broader readable debug-oriented source scaffolds.")
    parser.add_argument("--manifest", default="research_uiux/data/ui_source_path_manifest.json", help="Tracked source-path manifest JSON.")
    parser.add_argument("--hud-map", default="research_uiux/data/gameplay_hud_core_map.json", help="Gameplay HUD core map JSON.")
    parser.add_argument("--mirror-root", default="SONIC UNLEASHED", help="Local-only mirror root.")
    parser.add_argument("--output-json", default="research_uiux/data/local_debug_source_tree_expansion.json", help="Tracked JSON summary.")
    parser.add_argument("--output-md", default="research_uiux/LOCAL_DEBUG_SOURCE_TREE_EXPANSION.md", help="Tracked markdown summary.")
    parser.add_argument("--local-manifest", default="SONIC UNLEASHED/_meta/humanized_source_tree_expansion_manifest.json", help="Local-only manifest.")
    args = parser.parse_args()

    manifest_path = Path(args.manifest).resolve()
    hud_map_path = Path(args.hud_map).resolve()
    mirror_root = Path(args.mirror_root).resolve()
    output_json = Path(args.output_json).resolve()
    output_md = Path(args.output_md).resolve()
    local_manifest = Path(args.local_manifest).resolve()

    manifest_payload = read_json(manifest_path)
    gameplay_payload = read_json(hud_map_path)
    entries_by_path = {
        normalize_key(entry["relative_source_path"]): entry
        for entry in manifest_payload.get("entries", [])
    }

    groups = build_target_groups(entries_by_path, gameplay_payload)
    written_files: list[str] = []
    for group in groups:
        for target in group["targets"]:
            relative_source_path = target["relative_source_path"]
            target_path = mirror_cpp_path(mirror_root, relative_source_path)
            target_path.parent.mkdir(parents=True, exist_ok=True)
            target_path.write_text(render_target(target), encoding="utf-8")
            written_files.append(target_path.relative_to(mirror_root).as_posix())
        group["file_count"] = len(group["targets"])

    total_humanized_count = len(list(mirror_root.rglob("*.cpp")))
    write_local_readme(mirror_root, total_humanized_count)

    summary_payload = {
        "summary": {
            "new_humanized_source_file_count": len(written_files),
            "total_humanized_source_file_count": total_humanized_count,
            "group_count": len(groups),
        },
        "groups": [
            {
                "group_id": group["group_id"],
                "display_name": group["display_name"],
                "file_count": group["file_count"],
                "purpose": group["purpose"],
            }
            for group in groups
        ],
        "files": written_files,
    }

    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)
    local_manifest.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(summary_payload, indent=2) + "\n", encoding="utf-8")
    write_tracked_markdown(summary_payload, output_md)
    local_manifest.write_text(json.dumps(summary_payload, indent=2) + "\n", encoding="utf-8")
    print("materialized_local_debug_source_tree", f"new_files={len(written_files)}", f"total_humanized={total_humanized_count}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
