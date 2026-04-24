#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


SYSTEM_DISPLAY = {
    "title_menu": "Title Menu",
    "pause_stack": "Pause Stack",
    "status_overlay": "Status Overlay",
    "loading_and_start": "Loading And Start/Clear",
    "world_map_stack": "World Map Stack",
    "town_ui": "Town UI",
    "mission_briefing_and_gate": "Mission Briefing And Gate",
    "boss_hud": "Boss HUD",
    "item_result": "Item Result",
    "mission_result_family": "Mission Result Family",
    "save_and_ending": "Save And Ending",
    "tornado_defense": "Tornado Defense / EX Stage",
    "subtitle_cutscene_presentation": "Subtitle / Cutscene Presentation",
    "sonic_stage_hud": "Sonic Stage HUD",
    "werehog_stage_hud": "Werehog Stage HUD",
    "extra_stage_hud": "Extra Stage / Tornado Defense HUD",
    "super_sonic_hud": "Super Sonic / Final HUD Bridge",
    "csd_ui_foundation": "CSD / UI Foundation",
    "frontend_sequence_shell": "Frontend Sequence Shell",
    "camera_shell": "Camera / Replay Shell",
    "application_world_shell": "Application / World Shell",
    "achievement_unlock_support": "Achievement / Unlock Support",
    "audio_cue_support": "Audio Cue / BGM Support",
    "xml_data_loading_support": "XML / Data Loading Support",
}

RUNTIME_CONTRACTS = {
    "pause_stack": "pause_menu_reference.json",
    "title_menu": "title_menu_reference.json",
    "loading_and_start": "loading_transition_reference.json",
    "mission_result_family": "mission_result_reference.json",
    "save_and_ending": "autosave_toast_reference.json",
    "world_map_stack": "world_map_reference.json",
    "subtitle_cutscene_presentation": "subtitle_cutscene_reference.json",
    "sonic_stage_hud": "sonic_stage_hud_reference.json",
    "werehog_stage_hud": "werehog_stage_hud_reference.json",
    "extra_stage_hud": "extra_stage_hud_reference.json",
    "super_sonic_hud": "super_sonic_hud_reference.json",
    "boss_hud": "boss_hud_reference.json",
    "town_ui": "town_ui_reference.json",
    "frontend_sequence_shell": "frontend_sequence_shell_reference.json",
    "camera_shell": "camera_shell_reference.json",
    "application_world_shell": "application_world_shell_reference.json",
}

STATUS_ORDER = {
    "contract_backed": 0,
    "archaeology_mapped": 1,
    "debug_tool_candidate": 2,
    "named_seed_only": 3,
}


DEBUG_GAMEMODE_SUFFIXES = (
    "gamemodemenuselectdebug.cpp",
    "gamemodestageselectdebug.cpp",
    "gamemodemainmenu_test.cpp",
    "gamemodestageachievementtest.cpp",
    "gamemodestageeviltest.cpp",
    "gamemodestageforwardtest.cpp",
    "gamemodestageinstalltest.cpp",
    "gamemodestageloadxml.cpp",
    "gamemodestagemotiontest.cpp",
    "gamemodestagesavetest.cpp",
    "gamemodestagescreenshot.cpp",
    "gamemodestageswapdisktest.cpp",
)


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def load_system_index(repo_root: Path, archaeology_json: Path) -> dict[str, dict]:
    archaeology = read_json(archaeology_json)
    systems = {system["system_id"]: system for system in archaeology.get("systems", [])}

    gameplay_hud_json = repo_root / "research_uiux/data/gameplay_hud_core_map.json"
    if gameplay_hud_json.exists():
        gameplay_payload = read_json(gameplay_hud_json)
        for system in gameplay_payload.get("systems", []):
            systems[system["system_id"]] = {
                "system_id": system["system_id"],
                "screen_name": system.get("screen_name", SYSTEM_DISPLAY.get(system["system_id"], system["system_id"])),
                "layout_ids": [layout["layout_id"] for layout in system.get("layout_families", [])],
                "host_code_files": system.get("host_paths", []),
                "generated_seams": system.get("readable_hooks", {}).get("constructor_seams", [])
                + system.get("readable_hooks", {}).get("update_seams", []),
                "state_tags": [],
            }

    foundation_json = repo_root / "research_uiux/data/csd_ui_foundation_map.json"
    if foundation_json.exists():
        foundation_payload = read_json(foundation_json)
        foundation_system = foundation_payload.get("foundation_system")
        if foundation_system:
            systems[foundation_system["system_id"]] = foundation_system

    synthetic_systems = {
        "camera_shell": {
            "system_id": "camera_shell",
            "screen_name": SYSTEM_DISPLAY["camera_shell"],
            "layout_ids": [],
            "host_code_files": [
                "Camera/Controller/FreeCamera.cpp",
                "Camera/Controller/GoalCamera.cpp",
                "Camera/Controller/TownShopCamera.cpp",
                "Camera/Controller/TownTalkCamera.cpp",
                "Replay/Camera/ReplayFreeCamera.cpp",
                "Replay/Camera/ReplayRelativeCamera.cpp",
            ],
            "generated_seams": [],
            "state_tags": ["camera_shell", "replay_shell", "focus_cycle", "preview_host"],
        },
        "frontend_sequence_shell": {
            "system_id": "frontend_sequence_shell",
            "screen_name": SYSTEM_DISPLAY["frontend_sequence_shell"],
            "layout_ids": [],
            "host_code_files": [
                "Sequence/Core/SequenceHandleUnit.cpp",
                "Sequence/Core/SequenceManagerImpl.cpp",
                "Sequence/Unit/SequenceUnitFactory.cpp",
                "Sequence/Unit/SequenceUnitUnlockAchievement.cpp",
                "Sequence/Unit/SequenceUnitChangeStage.cpp",
                "Sequence/Unit/SequenceUnitMicroSequence.cpp",
                "Sequence/Unit/SequenceUnitPlayMovie.cpp",
                "Sequence/Utility/SequenceChangeStageUnit.cpp",
                "Sequence/Utility/SequencePlayMovieWrapper.cpp",
            ],
            "generated_seams": [],
            "state_tags": ["sequence_shell", "route_dispatch", "frontend_handoff", "unit_factory"],
        },
        "application_world_shell": {
            "system_id": "application_world_shell",
            "screen_name": SYSTEM_DISPLAY["application_world_shell"],
            "layout_ids": [],
            "host_code_files": [
                "System/Application.cpp",
                "System/ApplicationDocument.cpp",
                "System/Game.cpp",
                "System/GameDocument.cpp",
                "System/World.cpp",
                "System/GameMode/GameModeBoot.cpp",
                "System/GameMode/GameModeMainMenu.cpp",
                "System/GameMode/GameModeStage.cpp",
                "System/GameMode/Title/TitleManager.cpp",
                "System/GameMode/Title/TitleMenu.cpp",
                "System/GameMode/WorldMap/WorldMapSelect.cpp",
            ],
            "generated_seams": [],
            "state_tags": ["application_shell", "world_shell", "gamemode_shell", "frontend_dispatch"],
        },
        "achievement_unlock_support": {
            "system_id": "achievement_unlock_support",
            "screen_name": SYSTEM_DISPLAY["achievement_unlock_support"],
            "layout_ids": [],
            "host_code_files": [
                "Achievement/AchievementManager.cpp",
                "Sequence/Unit/SequenceUnitUnlockAchievement.cpp",
            ],
            "generated_seams": [],
            "state_tags": ["achievement_unlock", "toast_dispatch", "profile_reward"],
        },
        "audio_cue_support": {
            "system_id": "audio_cue_support",
            "screen_name": SYSTEM_DISPLAY["audio_cue_support"],
            "layout_ids": [],
            "host_code_files": [
                "Sound/Sound.cpp",
                "Sound/SoundBGMActEggman.cpp",
                "Sound/SoundBGMActEvil.cpp",
                "Sound/SoundBGMActSonic.cpp",
                "Sound/SoundBGMDispel.cpp",
                "Sound/SoundBGMExtra.cpp",
                "Sound/SoundBGMStandard.cpp",
                "Sound/SoundBGMTown.cpp",
                "Sound/SoundController.cpp",
                "Sound/SoundPlayer.cpp",
            ],
            "generated_seams": [],
            "state_tags": ["audio_cue", "bgm_route", "ui_feedback"],
        },
        "xml_data_loading_support": {
            "system_id": "xml_data_loading_support",
            "screen_name": SYSTEM_DISPLAY["xml_data_loading_support"],
            "layout_ids": [],
            "host_code_files": [
                "System/GameMode/Loader/DatabaseTree.cpp",
                "System/GameMode/Loader/StageLoaderXML.cpp",
                "XML/XMLBinData.cpp",
                "XML/XMLDocument.cpp",
                "XML/XMLManager.cpp",
                "XML/XMLNode.cpp",
                "XML/XMLTypeSLBin.cpp",
                "XML/XMLTypeSLTxt.cpp",
            ],
            "generated_seams": [],
            "state_tags": ["xml_resource", "stage_loader", "data_binding"],
        },
    }
    for system_id, system in synthetic_systems.items():
        systems.setdefault(system_id, system)

    return systems


def relpath(repo_root: Path, path: Path | str | None) -> str | None:
    if path is None:
        return None
    value = Path(path)
    try:
        return value.resolve().relative_to(repo_root.resolve()).as_posix()
    except Exception:
        return str(path).replace("\\", "/")


def unique_ordered(values):
    seen = set()
    ordered = []
    for value in values:
        if value in seen:
            continue
        seen.add(value)
        ordered.append(value)
    return ordered


def percentage(numerator: int, denominator: int) -> float:
    if denominator == 0:
        return 0.0
    return round((numerator / denominator) * 100.0, 1)


def normalize_source_path(raw: str) -> str | None:
    line = raw.strip().strip('"')
    if not line or line.startswith("#"):
        return None

    normalized = re.sub(r"/+", "/", line.replace("\\", "/"))
    lower = normalized.lower()

    anchor = "/swa/source/"
    if anchor in lower:
        relative = normalized[lower.index(anchor) + len(anchor) :]
    elif "/source/" in lower:
        relative = normalized[lower.index("/source/") + len("/source/") :]
    else:
        relative = normalized

    return relative.lstrip("/")


def classify_source_path(relative_path: str) -> dict:
    lowered = relative_path.lower()

    if lowered.startswith("tool/") or lowered.startswith("profile/") or lowered.startswith("debug/"):
        return {
            "family_id": "tooling_debug_ui",
            "family_name": "Tooling / Debug UI",
            "candidate_system_ids": [],
            "debug_tool_candidate": True,
            "notes": "Editor, preview, debug, and profiling surfaces that can host a standalone UI sandbox.",
        }

    if lowered.startswith("achievement/"):
        return {
            "family_id": "achievement_unlock_support",
            "family_name": "Achievement / Unlock Support",
            "candidate_system_ids": ["achievement_unlock_support"],
            "debug_tool_candidate": False,
            "notes": "Achievement ownership that pairs with sequence unlock dispatch and future toast/profile validation.",
        }

    if lowered.startswith("animation/eventtrigger/"):
        return {
            "family_id": "timeline_event_trigger_support",
            "family_name": "Timeline Event Trigger Support",
            "candidate_system_ids": ["subtitle_cutscene_presentation"],
            "debug_tool_candidate": False,
            "notes": "Animation event trigger support for audio, sparkle, and vibration cues around cutscene/timeline presentation.",
        }

    if lowered.startswith("sound/"):
        return {
            "family_id": "audio_cue_support",
            "family_name": "Audio Cue / BGM Support",
            "candidate_system_ids": ["audio_cue_support"],
            "debug_tool_candidate": False,
            "notes": "Audio cue, BGM route, and sound-player support layer for UI feedback and presentation-state timing.",
        }

    if lowered.startswith("xml/"):
        return {
            "family_id": "xml_data_loading_support",
            "family_name": "XML / Data Loading Support",
            "candidate_system_ids": ["xml_data_loading_support"],
            "debug_tool_candidate": False,
            "notes": "XML document/bin-data manager layer that backs stage loader data and future UI/resource binding probes.",
        }

    if lowered.startswith("player/parameter/") or lowered.startswith("player/switch/"):
        return {
            "family_id": "player_status_support",
            "family_name": "Player Status / Switch Support",
            "candidate_system_ids": ["sonic_stage_hud", "werehog_stage_hud", "super_sonic_hud"],
            "debug_tool_candidate": False,
            "notes": "Player parameter and switch state support that feeds gameplay HUD status, gauges, and mode-sensitive overlays.",
        }

    if any(lowered.endswith(suffix) for suffix in DEBUG_GAMEMODE_SUFFIXES):
        return {
            "family_id": "tooling_debug_ui",
            "family_name": "Tooling / Debug UI",
            "candidate_system_ids": [],
            "debug_tool_candidate": True,
            "notes": "Debug-oriented game modes and test hosts that look like likely entry points for a future UI capability sandbox.",
        }

    if lowered in (
        "boss/bosshudsupersonic.cpp",
        "boss/bosshudvitality.cpp",
        "boss/bossnameplate.cpp",
    ):
        return {
            "family_id": "boss_ui",
            "family_name": "Boss HUD",
            "candidate_system_ids": ["super_sonic_hud"],
            "debug_tool_candidate": False,
            "notes": "Super Sonic and final-phase boss HUD bridge with shared vitality/nameplate ownership.",
        }

    if lowered.startswith("boss/") and ("hud" in lowered or "bosshud" in lowered or "nameplate" in lowered):
        return {
            "family_id": "boss_ui",
            "family_name": "Boss HUD",
            "candidate_system_ids": ["boss_hud"],
            "debug_tool_candidate": False,
            "notes": "Boss HUD/name plate classes that line up with extracted boss gauge/name layouts.",
        }

    if lowered.startswith("hud/sonic/") or lowered.startswith("player/character/sonic/hud/"):
        return {
            "family_id": "stage_hud_core",
            "family_name": "Stage HUD Core",
            "candidate_system_ids": ["sonic_stage_hud"],
            "debug_tool_candidate": False,
            "notes": "Day-stage HUD shell plus Sonic-specific guide/homing sidecars.",
        }

    if lowered.startswith("hud/evil/") or lowered.startswith("player/character/evilsonic/hud/"):
        return {
            "family_id": "stage_hud_core",
            "family_name": "Stage HUD Core",
            "candidate_system_ids": ["werehog_stage_hud"],
            "debug_tool_candidate": False,
            "notes": "Werehog stage HUD shell plus target/guide helper overlays.",
        }

    if lowered.startswith("hud/item/"):
        return {
            "family_id": "stage_hud_core",
            "family_name": "Stage HUD Core",
            "candidate_system_ids": ["sonic_stage_hud", "werehog_stage_hud"],
            "debug_tool_candidate": False,
            "notes": "In-stage item-get overlay that appears to sit alongside the shared day/night gameplay HUD shell rather than a standalone frontend family.",
        }

    if lowered.startswith("hud/pause/") or lowered.startswith("hud/generalwindow/") or lowered.startswith("hud/helpwindow/"):
        return {
            "family_id": "pause_stack",
            "family_name": "Pause Stack",
            "candidate_system_ids": ["pause_stack"],
            "debug_tool_candidate": False,
            "notes": "Pause shell, item/skill lists, and framed window/help sidecars.",
        }

    if lowered == "sequence/unit/sequenceunitcallhelpwindow.cpp":
        return {
            "family_id": "pause_stack",
            "family_name": "Pause Stack",
            "candidate_system_ids": ["pause_stack"],
            "debug_tool_candidate": False,
            "notes": "Sequence-layer dispatcher that appears to hand off into the shared help window surface.",
        }

    if lowered.startswith("hud/status/"):
        return {
            "family_id": "status_overlay",
            "family_name": "Status Overlay",
            "candidate_system_ids": ["status_overlay"],
            "debug_tool_candidate": False,
            "notes": "Status/progress overlay family.",
        }

    if lowered.startswith("hud/loading/") or lowered.startswith("hud/install/") or lowered.endswith("gamemodeboot.cpp") or lowered.endswith("gamemodestageinstall.cpp") or lowered.endswith("gamemodestagelogo.cpp"):
        return {
            "family_id": "loading_and_boot",
            "family_name": "Loading / Boot / Install",
            "candidate_system_ids": ["loading_and_start", "application_world_shell"],
            "debug_tool_candidate": False,
            "notes": "Loading, install, and stage-logo/start handoff surfaces.",
        }

    if lowered.startswith("system/gamemode/worldmap/") or lowered.endswith("titlestateworldmap.cpp"):
        return {
            "family_id": "world_map_stack",
            "family_name": "World Map Stack",
            "candidate_system_ids": ["world_map_stack", "application_world_shell"],
            "debug_tool_candidate": False,
            "notes": "World map list/select/object/tutorial flow.",
        }

    if lowered.startswith("town/") or lowered.startswith("hud/mediaroom/"):
        return {
            "family_id": "town_ui",
            "family_name": "Town / Media Room UI",
            "candidate_system_ids": ["town_ui"],
            "debug_tool_candidate": False,
            "notes": "Town dialogs, shops, item/media-room menu shells, and town managers.",
        }

    if lowered in ("camera/controller/townshopcamera.cpp", "camera/controller/towntalkcamera.cpp"):
        return {
            "family_id": "town_ui",
            "family_name": "Town / Media Room UI",
            "candidate_system_ids": ["town_ui"],
            "debug_tool_candidate": False,
            "notes": "Town conversation/shop camera controllers that sit beside the town UI shell.",
        }

    if lowered.startswith("camera/controller/") and any(
        token in lowered for token in ("boss", "finaldarkgaia", "supersonic", "eggdragoon")
    ):
        return {
            "family_id": "frontend_camera_shell",
            "family_name": "Frontend Camera Shell",
            "candidate_system_ids": ["camera_shell", "boss_hud"],
            "debug_tool_candidate": False,
            "notes": "Boss and final-phase presentation camera controllers that pair camera-shell routing with boss HUD timing.",
        }

    if lowered.startswith("camera/controller/") and any(
        token in lowered for token in ("exstage", "explayer", "tails", "temple")
    ):
        return {
            "family_id": "frontend_camera_shell",
            "family_name": "Frontend Camera Shell",
            "candidate_system_ids": ["camera_shell", "extra_stage_hud"],
            "debug_tool_candidate": False,
            "notes": "Extra-stage and special-player presentation camera controllers adjacent to EX-stage HUD validation.",
        }

    if lowered.startswith("hud/mission/") or lowered.startswith("system/mission/"):
        if "finish" in lowered or "failed" in lowered or lowered.endswith("objmissionclear.cpp"):
            return {
                "family_id": "mission_result",
                "family_name": "Mission Result Family",
                "candidate_system_ids": ["mission_result_family"],
                "debug_tool_candidate": False,
                "notes": "Mission clear/fail/result surfaces.",
            }
        return {
            "family_id": "mission_gate",
            "family_name": "Mission Briefing / Gate",
            "candidate_system_ids": ["mission_briefing_and_gate"],
            "debug_tool_candidate": False,
            "notes": "Mission entry, objective, and gate/start surfaces.",
        }

    if lowered.startswith("hud/common/result/"):
        return {
            "family_id": "mission_result",
            "family_name": "Mission Result Family",
            "candidate_system_ids": ["mission_result_family", "item_result"],
            "debug_tool_candidate": False,
            "notes": "Generic result window family.",
        }

    if lowered.startswith("sequence/") and ("autosave" in lowered or "registerclearflag" in lowered):
        return {
            "family_id": "save_and_ending",
            "family_name": "Save / Ending Flow",
            "candidate_system_ids": ["save_and_ending", "application_world_shell"],
            "debug_tool_candidate": False,
            "notes": "Autosave and clear-flag sequencing around endings/save icon behavior.",
        }

    if lowered in (
        "sequence/unit/sequenceunitchangestage.cpp",
        "sequence/unit/sequenceunitswapdisk.cpp",
        "sequence/utility/sequencechangestageunit.cpp",
    ):
        return {
            "family_id": "loading_and_boot",
            "family_name": "Loading / Boot / Install",
            "candidate_system_ids": ["loading_and_start"],
            "debug_tool_candidate": False,
            "notes": "Sequence-layer stage-change and swap-disk handoffs that still resolve into the loading/start shell.",
        }

    if lowered in (
        "sequence/unit/sequenceunitsendmediaroommessage.cpp",
        "sequence/unit/sequenceunitsendtownmessage.cpp",
    ):
        return {
            "family_id": "town_ui",
            "family_name": "Town / Media Room UI",
            "candidate_system_ids": ["town_ui"],
            "debug_tool_candidate": False,
            "notes": "Sequence-layer message dispatch into town and media-room surfaces.",
        }

    if lowered.startswith("sequence/") and ("playmovie" in lowered or "microsequence" in lowered):
        return {
            "family_id": "subtitle_cutscene",
            "family_name": "Subtitle / Cutscene Presentation",
            "candidate_system_ids": ["subtitle_cutscene_presentation"],
            "debug_tool_candidate": False,
            "notes": "Sequence-layer movie and micro-sequence wrappers tied to cutscene/subtitle presentation.",
        }

    if lowered.startswith("system/gamemode/ending/") or lowered.endswith("gamemodeending.cpp") or lowered.endswith("gamemodestagesavetest.cpp") or lowered.endswith("saveloadtest.cpp"):
        return {
            "family_id": "save_and_ending",
            "family_name": "Save / Ending Flow",
            "candidate_system_ids": ["save_and_ending", "application_world_shell"],
            "debug_tool_candidate": False,
            "notes": "Ending text/image flow and save-test surfaces.",
        }

    if lowered.startswith("inspire/") or lowered.startswith("movie/") or lowered.endswith("gamemodestagemovie.cpp"):
        return {
            "family_id": "subtitle_cutscene",
            "family_name": "Subtitle / Cutscene Presentation",
            "candidate_system_ids": ["subtitle_cutscene_presentation"],
            "debug_tool_candidate": False,
            "notes": "Movie wrapper, subtitle, texture-overlay, and cutscene sequencing surfaces.",
        }

    if lowered.startswith("system/gamemode/title/") or lowered.endswith("gamemodemainmenu.cpp") or lowered.endswith("gamemodestagemainmenu.cpp") or lowered.endswith("gamemodestagetitle.cpp") or lowered.endswith("gamemodetitleselect.cpp") or lowered.endswith("mainmenumanager.cpp") or lowered.startswith("hud/stageselect/"):
        return {
            "family_id": "title_menu",
            "family_name": "Title / Main Menu",
            "candidate_system_ids": ["title_menu", "loading_and_start", "application_world_shell"],
            "debug_tool_candidate": False,
            "notes": "Title/menu ownership, intro handoff, and stage-title/menu entry surfaces.",
        }

    if lowered.startswith("extrastage/tails/hud/"):
        return {
            "family_id": "tornado_defense",
            "family_name": "Tornado Defense / EX HUD",
            "candidate_system_ids": ["tornado_defense", "extra_stage_hud"],
            "debug_tool_candidate": False,
            "notes": "EX-stage/Tornado Defense HUD and QTE surface.",
        }

    if lowered.startswith("csd/") or lowered.startswith("menu/"):
        return {
            "family_id": "csd_ui_foundation",
            "family_name": "CSD / UI Foundation",
            "candidate_system_ids": ["csd_ui_foundation"],
            "debug_tool_candidate": False,
            "notes": "Core CellSpriteDraw project/mirage bridge layer and generic text-box foundation.",
        }

    if lowered in (
        "camera/controller/freecamera.cpp",
        "replay/camera/replayfreecamera.cpp",
        "replay/camera/replayrelativecamera.cpp",
    ):
        return {
            "family_id": "tooling_debug_ui",
            "family_name": "Tooling / Debug UI",
            "candidate_system_ids": ["camera_shell"],
            "debug_tool_candidate": True,
            "notes": "Free/replay camera controllers that look more like debug and inspection hosts than final frontend ownership.",
        }

    if lowered.startswith("camera/") or lowered.startswith("replay/camera/"):
        return {
            "family_id": "frontend_camera_shell",
            "family_name": "Frontend Camera Shell",
            "candidate_system_ids": ["camera_shell"],
            "debug_tool_candidate": False,
            "notes": "Camera-side support around menu, town, goal, replay, and debug-facing presentation flows.",
        }

    if lowered.startswith("sequence/"):
        return {
            "family_id": "frontend_sequence_shell",
            "family_name": "Frontend Sequence Shell",
            "candidate_system_ids": ["frontend_sequence_shell"],
            "debug_tool_candidate": False,
            "notes": "Sequence orchestration and unit-factory shell that now has a dedicated recovered runtime/debug family.",
        }

    if lowered.startswith("system/gamemode/"):
        return {
            "family_id": "frontend_gamemode_shell",
            "family_name": "GameMode / Frontend Shell",
            "candidate_system_ids": ["application_world_shell"],
            "debug_tool_candidate": False,
            "notes": "Higher-level game-mode orchestration around menu, stage, frontend, and handoff shells that still need finer recovery.",
        }

    if lowered.startswith("system/"):
        return {
            "family_id": "frontend_system_shell",
            "family_name": "Frontend System Shell",
            "candidate_system_ids": ["application_world_shell"],
            "debug_tool_candidate": False,
            "notes": "Application, document, world, and stage-level system hosts adjacent to the UI stack.",
        }

    return {
        "family_id": "ui_misc",
        "family_name": "UI Misc",
        "candidate_system_ids": [],
        "debug_tool_candidate": False,
        "notes": "Seed path not yet folded into a stronger UI family.",
    }


def status_for_entry(matched_system_ids: list[str], runtime_contracts: list[str], debug_tool_candidate: bool) -> str:
    if matched_system_ids and runtime_contracts:
        return "contract_backed"
    if matched_system_ids:
        return "archaeology_mapped"
    if debug_tool_candidate:
        return "debug_tool_candidate"
    return "named_seed_only"


def humanization_priority(status: str, family_id: str) -> str:
    if status == "contract_backed":
        return "lift current contract/runtime examples into source-path-named debug screens"
    if status == "archaeology_mapped":
        return "add cleaner translated seam labels and source-path-backed notes"
    if family_id == "stage_hud_core":
        return "create a new gameplay HUD archaeology family before deep code cleanup"
    if family_id == "csd_ui_foundation":
        return "document the CSD foundation and recover reusable scene/widget abstractions"
    if family_id == "achievement_unlock_support":
        return "tie achievement manager ownership to sequence unlock dispatch and toast/profile behavior"
    if family_id == "audio_cue_support":
        return "correlate sound cue ownership with menu, cutscene, and HUD transition events"
    if family_id == "xml_data_loading_support":
        return "recover XML/resource loader boundaries before treating data-backed screens as understood"
    if family_id == "timeline_event_trigger_support":
        return "map animation event triggers onto cutscene, feedback, and transition timing seams"
    if family_id == "player_status_support":
        return "connect player parameter/switch state to HUD gauge and overlay ownership"
    if status == "debug_tool_candidate":
        return "use this as a host surface for the future UI capability sandbox"
    return "expand extraction and correlation before pretending this path is understood"


def build_payload(repo_root: Path, input_txt: Path, archaeology_json: Path, contracts_dir: Path) -> dict:
    systems = load_system_index(repo_root, archaeology_json)

    entries = []
    family_groups: dict[str, list[dict]] = defaultdict(list)
    system_hits: dict[str, list[dict]] = defaultdict(list)

    for index, raw in enumerate(input_txt.read_text(encoding="utf-8-sig").splitlines(), start=1):
        relative_path = normalize_source_path(raw)
        if relative_path is None:
            continue

        classification = classify_source_path(relative_path)
        candidate_system_ids = classification["candidate_system_ids"]
        matched_system_ids = [system_id for system_id in candidate_system_ids if system_id in systems]
        runtime_contracts = []
        for system_id in matched_system_ids:
            contract_name = RUNTIME_CONTRACTS.get(system_id)
            if contract_name and (contracts_dir / contract_name).exists():
                runtime_contracts.append(contract_name)

        status = status_for_entry(matched_system_ids, runtime_contracts, classification["debug_tool_candidate"])

        entry = {
            "ordinal": index,
            "source_path": raw.strip(),
            "relative_source_path": relative_path.replace("\\", "/"),
            "family_id": classification["family_id"],
            "family_name": classification["family_name"],
            "candidate_system_ids": candidate_system_ids,
            "matched_system_ids": matched_system_ids,
            "matched_system_names": [SYSTEM_DISPLAY.get(system_id, system_id) for system_id in matched_system_ids],
            "runtime_contracts": runtime_contracts,
            "status": status,
            "debug_tool_candidate": classification["debug_tool_candidate"],
            "humanization_priority": humanization_priority(status, classification["family_id"]),
            "notes": classification["notes"],
        }
        entries.append(entry)
        family_groups[classification["family_id"]].append(entry)
        for system_id in matched_system_ids:
            system_hits[system_id].append(entry)

    bucket_counts = Counter(entry["status"] for entry in entries)
    family_summaries = []
    for family_id, family_entries in family_groups.items():
        sample = family_entries[0]
        matched_system_ids = unique_ordered(
            system_id for entry in family_entries for system_id in entry["matched_system_ids"]
        )
        runtime_contracts = unique_ordered(
            contract for entry in family_entries for contract in entry["runtime_contracts"]
        )
        family_summaries.append(
            {
                "family_id": family_id,
                "family_name": sample["family_name"],
                "path_count": len(family_entries),
                "matched_system_ids": matched_system_ids,
                "matched_system_names": [SYSTEM_DISPLAY.get(system_id, system_id) for system_id in matched_system_ids],
                "runtime_contracts": runtime_contracts,
                "status_counts": dict(
                    sorted(Counter(entry["status"] for entry in family_entries).items(), key=lambda item: item[0])
                ),
                "sample_relative_paths": [entry["relative_source_path"] for entry in family_entries[:5]],
            }
        )

    family_summaries.sort(key=lambda item: (-item["path_count"], item["family_name"]))

    system_summaries = []
    for system_id, hits in system_hits.items():
        runtime_contract = RUNTIME_CONTRACTS.get(system_id)
        system = systems[system_id]
        system_summaries.append(
            {
                "system_id": system_id,
                "screen_name": system.get("screen_name", SYSTEM_DISPLAY.get(system_id, system_id)),
                "seed_path_count": len(hits),
                "runtime_contract": runtime_contract if runtime_contract and (contracts_dir / runtime_contract).exists() else None,
                "layout_ids": system.get("layout_ids", []),
                "host_code_count": len(system.get("host_code_files", [])),
                "generated_seam_count": len(system.get("generated_seams", [])),
                "state_tag_count": len(system.get("state_tags", [])),
                "sample_relative_paths": [entry["relative_source_path"] for entry in hits[:5]],
            }
        )
    system_summaries.sort(key=lambda item: (-item["seed_path_count"], item["screen_name"]))

    total = len(entries)
    mapped_count = sum(1 for entry in entries if entry["matched_system_ids"])
    contract_count = sum(1 for entry in entries if entry["status"] == "contract_backed")
    debug_count = sum(1 for entry in entries if entry["status"] == "debug_tool_candidate")
    named_only_count = sum(1 for entry in entries if entry["status"] == "named_seed_only")

    summary = {
        "input_path_count": total,
        "family_count": len(family_summaries),
        "mapped_to_archaeology_count": mapped_count,
        "mapped_to_archaeology_pct": percentage(mapped_count, total),
        "runtime_contract_backed_count": contract_count,
        "runtime_contract_backed_pct": percentage(contract_count, total),
        "debug_tool_candidate_count": debug_count,
        "debug_tool_candidate_pct": percentage(debug_count, total),
        "named_seed_only_count": named_only_count,
        "named_seed_only_pct": percentage(named_only_count, total),
        "status_counts": {key: bucket_counts.get(key, 0) for key in sorted(STATUS_ORDER, key=lambda item: STATUS_ORDER[item])},
    }

    return {
        "repo_root": repo_root.as_posix(),
        "inputs": {
            "source_paths": relpath(repo_root, input_txt),
            "ui_archaeology_database": relpath(repo_root, archaeology_json),
            "contracts_dir": relpath(repo_root, contracts_dir),
        },
        "summary": summary,
        "families": family_summaries,
        "systems": system_summaries,
        "entries": entries,
    }


def write_markdown(payload: dict, output_path: Path) -> None:
    summary = payload["summary"]
    lines = [
        '<p align="right">',
        '    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>',
        "</p>",
        "",
        '# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> UI Source-Path Recovery And Humanization Plan',
        "",
        "> [!IMPORTANT]",
        "> This report is scoped to the current SWARD source-path seed extracted from the broader Xbox 360 path dump the user supplied. It is a naming and organization bridge, not a claim that the whole game has already been humanized into clean source.",
        "",
        "## Status Snapshot",
        "",
        "| Target | State | Percentage |",
        "|---|---|---:|",
        f"| Current source-path seed organized into families | Complete for this phase | `{percentage(summary['input_path_count'], summary['input_path_count']):.1f}%` |",
        f"| Current source-path seed mapped into the current archaeology layer | Partial but strong | `{summary['mapped_to_archaeology_pct']:.1f}%` |",
        f"| Current source-path seed already backed by portable runtime contracts | Partial | `{summary['runtime_contract_backed_pct']:.1f}%` |",
        f"| Current source-path seed still only named/debug-targeted and not semantically recovered | Open gap | `{percentage(summary['named_seed_only_count'] + summary['debug_tool_candidate_count'], summary['input_path_count']):.1f}%` |",
        "",
        "## Exact Counts",
        "",
        f"- Seeded source paths: `{summary['input_path_count']}`",
        f"- Family groups: `{summary['family_count']}`",
        f"- Paths mapped to current archaeology systems: `{summary['mapped_to_archaeology_count']}`",
        f"- Paths already backed by runtime contracts: `{summary['runtime_contract_backed_count']}`",
        f"- Paths that are strong debug-tool host candidates: `{summary['debug_tool_candidate_count']}`",
        f"- Paths that remain named-only after this phase: `{summary['named_seed_only_count']}`",
        "",
        "## Coverage Buckets",
        "",
        "| Bucket | Count | Meaning |",
        "|---|---:|---|",
        f"| `contract_backed` | `{summary['status_counts']['contract_backed']}` | Path family already lands on a recovered archaeology system with a portable runtime contract. |",
        f"| `archaeology_mapped` | `{summary['status_counts']['archaeology_mapped']}` | Path family is tied to a recovered system, but not yet exercised by the runtime contract pack. |",
        f"| `debug_tool_candidate` | `{summary['status_counts']['debug_tool_candidate']}` | Debug/tool/game-mode surface that looks like a good host for the future UI capability sandbox. |",
        f"| `named_seed_only` | `{summary['status_counts']['named_seed_only']}` | We now have the source-path name organized, but the family still needs extraction/correlation/humanization work. |",
        "",
        "## Family Breakdown",
        "",
        "| Family | Paths | Current bridge | Runtime-backed |",
        "|---|---:|---|---|",
    ]

    for family in payload["families"]:
        bridge = ", ".join(f"`{name}`" for name in family["matched_system_names"]) if family["matched_system_names"] else "Not yet in archaeology"
        runtime = ", ".join(f"`{name}`" for name in family["runtime_contracts"]) if family["runtime_contracts"] else "No"
        lines.append(f"| {family['family_name']} | `{family['path_count']}` | {bridge} | {runtime} |")

    lines.extend(
        [
            "",
            "## System Bridge",
            "",
            "| Archaeology system | Seed paths | Layout IDs | Runtime contract |",
            "|---|---:|---|---|",
        ]
    )

    for system in payload["systems"]:
        layout_ids = ", ".join(f"`{layout_id}`" for layout_id in system["layout_ids"]) if system["layout_ids"] else "None"
        runtime_contract = f"`{system['runtime_contract']}`" if system["runtime_contract"] else "No"
        lines.append(f"| {system['screen_name']} | `{system['seed_path_count']}` | {layout_ids} | {runtime_contract} |")

    lines.extend(
        [
            "",
            "## What This Means Right Now",
            "",
            "- The generated translated PPC layer is present, but the clean human-readable organization layer is still incomplete.",
            "- This phase gives the current source-path subset a stable naming scaffold, so future translated-code cleanup can follow original source-family names instead of raw `sub_XXXXXXXX` clusters alone.",
            "- The strongest already-recovered path families are title/menu, pause, loading/start, world map, mission-result, save/ending, gameplay HUD core, town/media-room, and the lower-level CSD foundation layer.",
            "- The clearest remaining UI/UX gaps are the still note-heavy translated ownership layer inside the local mirror, the incomplete gameplay/asset correlation outside the current contract families, and the broader whole-game source-path set that still needs to be humanized in waves.",
            "",
            "## Debug Tool Direction",
            "",
            "- The best current hosts for a local debug executable/menu build are the debug/test game modes plus the tool/preview surfaces grouped under `Tooling / Debug UI`.",
            "- The current contract-backed runtime layer is already strong enough to drive the standalone selector and workbench across frontend, town, camera, application/world, cutscene, gameplay-HUD, boss/final, and stage-test-adjacent host families.",
            "- The missing bridge for a richer debug tool is no longer raw translation; it is turning more of the source-path-backed families into readable translated ownership and widening host coverage beyond the current reusable subset.",
            "",
            "## Next Local Work",
            "",
            "1. Keep widening the current source-path seed in defensible chunks instead of flooding the repo with raw whole-dump noise.",
            "2. Keep replacing local-only `SONIC UNLEASHED/` note/scaffold files with cleaner translated ownership under the recovered source-family paths.",
            "3. Keep expanding selector/workbench coverage and mirrored translated ownership until the local-only tree behaves like a readable debug-oriented source base instead of a scaffold set.",
        ]
    )

    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Map a UI-focused source-path seed onto the current SWARD archaeology/runtime layer.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument("--input", required=True, help="Text file containing one source path per line.")
    parser.add_argument("--archaeology-json", required=True, help="Path to ui_archaeology_database.json.")
    parser.add_argument("--contracts-dir", required=True, help="Runtime contracts directory.")
    parser.add_argument("--output-json", required=True, help="Destination JSON manifest.")
    parser.add_argument("--output-md", required=True, help="Destination Markdown report.")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    input_txt = Path(args.input)
    if not input_txt.is_absolute():
        input_txt = (repo_root / input_txt).resolve()
    archaeology_json = Path(args.archaeology_json)
    if not archaeology_json.is_absolute():
        archaeology_json = (repo_root / args.archaeology_json).resolve()
    contracts_dir = Path(args.contracts_dir)
    if not contracts_dir.is_absolute():
        contracts_dir = (repo_root / args.contracts_dir).resolve()
    output_json = Path(args.output_json)
    if not output_json.is_absolute():
        output_json = (repo_root / args.output_json).resolve()
    output_md = Path(args.output_md)
    if not output_md.is_absolute():
        output_md = (repo_root / args.output_md).resolve()

    payload = build_payload(repo_root, input_txt, archaeology_json, contracts_dir)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    write_markdown(payload, output_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
