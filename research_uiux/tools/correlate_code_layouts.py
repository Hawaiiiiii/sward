#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import defaultdict
from pathlib import Path


SOURCE_DIRS = [
    "UnleashedRecomp",
    "UnleashedRecompLib/config",
]

SOURCE_SUFFIXES = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".inl", ".toml"}

LAYOUT_ROLES = {
    "ui_mainmenu": "title_menu",
    "ui_balloon": "town_dialog_balloon",
    "ui_shop": "town_shop_menu",
    "ui_townscreen": "town_overlay",
    "ui_mediaroom": "mediaroom_menu",
    "ui_general": "shared_window_shell",
    "ui_pause": "pause_menu",
    "ui_status": "status_overlay",
    "ui_gate": "mission_gate_overlay",
    "ui_itemresult": "item_result_overlay",
    "ui_missionscreen": "mission_briefing_overlay",
    "ui_misson": "mission_briefing_window",
    "ui_help": "help_overlay",
    "ui_loading": "loading_transition",
    "ui_exstage": "exstage_hud",
    "ui_prov_playscreen": "tornado_defense_hud",
    "ui_qte": "tornado_defense_qte",
    "ui_result": "mission_result_overlay",
    "ui_result_ex": "exstage_result_overlay",
    "ui_start": "start_clear_prompt",
    "ui_worldmap": "world_map",
    "ui_worldmap_help": "world_map_help_overlay",
    "ui_saveicon": "save_overlay",
    "ui_end": "staff_roll_or_ending",
    "ui_boss_gauge": "boss_hud_gauge",
    "ui_boss_name": "boss_hud_name",
}

LAYOUT_PATCH_FILES = {
    "ui_mainmenu": [
        "UnleashedRecomp/patches/CTitleStateIntro_patches.cpp",
        "UnleashedRecomp/patches/CTitleStateMenu_patches.cpp",
    ],
    "ui_balloon": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_shop": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_townscreen": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_mediaroom": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_pause": [
        "UnleashedRecomp/patches/CHudPause_patches.cpp",
    ],
    "ui_gate": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_missionscreen": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_misson": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_worldmap": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
        "UnleashedRecomp/patches/input_patches.cpp",
    ],
    "ui_worldmap_help": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
        "UnleashedRecomp/patches/input_patches.cpp",
    ],
    "ui_exstage": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
        "UnleashedRecomp/patches/fps_patches.cpp",
    ],
    "ui_prov_playscreen": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
        "UnleashedRecomp/patches/object_patches.cpp",
        "UnleashedRecomp/patches/fps_patches.cpp",
    ],
    "ui_qte": [
        "UnleashedRecomp/patches/misc_patches.cpp",
        "UnleashedRecomp/patches/object_patches.cpp",
        "UnleashedRecomp/patches/fps_patches.cpp",
    ],
    "ui_result": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_result_ex": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_start": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_saveicon": [
        "UnleashedRecomp/patches/resident_patches.cpp",
    ],
    "ui_end": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_boss_gauge": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
    "ui_boss_name": [
        "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
    ],
}

MANUAL_HINTS = {
    "ui_mainmenu": [
        {
            "path": "UnleashedRecomp/patches/CTitleStateMenu_patches.cpp",
            "evidence": "strong",
            "patterns": ["SWA::CTitleStateMenu::Update", "OptionsMenu::Open()"],
            "reason": "Wraps the original title-menu update seam and injects options/install logic over the native main-menu cursor flow.",
        },
        {
            "path": "UnleashedRecomp/patches/CTitleStateIntro_patches.cpp",
            "evidence": "contextual",
            "patterns": ["SWA::CTitleStateIntro::Update", "ProcessQuitMessage"],
            "reason": "Owns the modal and fade handoff immediately before the title menu becomes interactive.",
        },
        {
            "path": "UnleashedRecomp/ui/message_window.cpp",
            "evidence": "contextual",
            "patterns": ["MessageWindow::Open"],
            "reason": "Provides the prompt layer used by the title intro and title menu wrappers for quit/install/update flows.",
        },
        {
            "path": "UnleashedRecomp/ui/fader.cpp",
            "evidence": "contextual",
            "patterns": ["Fader::FadeOut"],
            "reason": "Provides shared fade ownership used when title flow exits, restarts, or jumps into installer/update actions.",
        },
        {
            "path": "UnleashedRecompLib/config/SWA.toml",
            "evidence": "contextual",
            "patterns": ["# Title", "UseAlternateTitleMidAsmHook", "AddPrimitive2DMidAsmHook"],
            "reason": "Declares title-specific hook sites on the original title render path even though the layout name itself is not exposed in readable C++.",
        },
    ],
    "ui_balloon": [
        {
            "path": "UnleashedRecomp/install/hashes/game.cpp",
            "evidence": "contextual",
            "patterns": ['"Item/item_balloon.dds"'],
            "reason": "The install hash table preserves the shared town-balloon texture asset by name, which lines up with the extracted `ui_balloon` talk/item window package.",
        },
    ],
    "ui_shop": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_shop/footer/shop_footer"],
            "reason": "Direct layout-path rules expose the dedicated shop footer prompt row from the extracted `ui_shop` package.",
        },
    ],
    "ui_townscreen": [
        {
            "path": "UnleashedRecomp/locale/config_locale.cpp",
            "evidence": "contextual",
            "patterns": ["CONFIG_DEFINE_LOCALE(TimeOfDayTransition)", "ETimeOfDayTransition::PlayStation"],
            "reason": "Readable config/localization evidence exposes the town time-of-day transition policy that sits adjacent to the extracted `ui_townscreen` overlay family.",
        },
    ],
    "ui_mediaroom": [
        {
            "path": "UnleashedRecomp/install/hashes/game.cpp",
            "evidence": "contextual",
            "patterns": ['"Sound/bgm_sys_mediaroom.csb"'],
            "reason": "The install hash table exposes the dedicated Media Room BGM cue, confirming this layout belongs to a distinct menu-family presentation rather than a generic town shell.",
        },
    ],
    "ui_general": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_general/bg", "ui_general/footer"],
            "reason": "Direct layout-path rules prove the generic background/footer shell is treated as a reusable CSD package rather than screen-specific one-off geometry.",
        },
        {
            "path": "UnleashedRecomp/ui/imgui_utils.cpp",
            "evidence": "strong",
            "patterns": ["DrawPauseContainer", "DrawPauseHeaderContainer"],
            "reason": "Implements a reusable pause-style shell that mirrors the asset-side `ui_general` grammar of dark background, framed window, and footer row.",
        },
        {
            "path": "UnleashedRecomp/ui/message_window.cpp",
            "evidence": "contextual",
            "patterns": ["DrawPauseContainer"],
            "reason": "Renders modal prompts inside the same shared shell language instead of hardcoding a separate message-box frame.",
        },
    ],
    "ui_pause": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_pause/bg", "ui_pause/footer/footer_A", "ui_pause/header/status_title"],
            "reason": "Direct layout-path rules cover the pause background, footer, and header-title shell.",
        },
        {
            "path": "UnleashedRecomp/patches/CHudPause_patches.cpp",
            "evidence": "strong",
            "patterns": ["SWA::CHudPause::Update", "OptionsMenu::Open(true, pHudPause->m_Menu)"],
            "reason": "Wraps the original pause HUD update seam and injects options/achievement behavior without replacing the native pause state machine.",
        },
        {
            "path": "UnleashedRecomp/ui/options_menu.cpp",
            "evidence": "strong",
            "patterns": ["OptionsMenu::Open(bool isPause, SWA::EMenuType pauseMenuType)", "s_pauseMenuType"],
            "reason": "Implements the custom submenu branch opened from pause, with separate world-map and non-world-map pause behavior.",
        },
        {
            "path": "UnleashedRecomp/ui/imgui_utils.cpp",
            "evidence": "contextual",
            "patterns": ["DrawPauseContainer", "DrawPauseHeaderContainer"],
            "reason": "Implements the shared frame language reused by pause-adjacent custom UI.",
        },
        {
            "path": "UnleashedRecompLib/config/SWA.toml",
            "evidence": "strong",
            "patterns": ["# World Map Pause Menu", "CHudPauseItemCountMidAsmHook"],
            "reason": "Declares the pause-menu injection hooks for world map, village, stage, hub, and misc pause variants.",
        },
    ],
    "ui_status": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_status/footer/status_footer", "ui_status/header/status_title", "ui_status/main/progless/prgs/prgs_bar_1"],
            "reason": "Direct layout-path rules expose footer, title shell, progress bar, tag, and arrow-effect elements from the extracted status layout.",
        },
        {
            "path": "UnleashedRecomp/ui/imgui_utils.cpp",
            "evidence": "contextual",
            "patterns": ["DrawPauseHeaderContainer"],
            "reason": "The readable UI layer reuses a title/header frame style that closely matches the extracted `ui_status` title-shell structure.",
        },
    ],
    "ui_gate": [
        {
            "path": "UnleashedRecomp/install/hashes/game.cpp",
            "evidence": "contextual",
            "patterns": ['"Hint/BossGate.dds"'],
            "reason": "The install hash table preserves a dedicated boss-gate hint texture, which fits the extracted `ui_gate` status-window package.",
        },
    ],
    "ui_itemresult": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_itemresult/footer/result_footer", "ui_itemresult/main/iresult_title"],
            "reason": "Direct layout-path rules expose the footer prompt row and title treatment from the extracted item-result layout.",
        },
    ],
    "ui_missionscreen": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_missionscreen/player_count", "ui_missionscreen/score_count", "ui_missionscreen/lap_count"],
            "reason": "Direct layout-path rules expose mission HUD counters for player, time, score, item, and lap display groups.",
        },
    ],
    "ui_misson": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_misson/header/misson_title_B", "ui_misson/window/bg_B2/position/bg"],
            "reason": "Direct layout-path rules expose the misson-title header shell and stretchable mission window background from the extracted package.",
        },
    ],
    "ui_help": [
        {
            "path": "UnleashedRecomp/ui/button_guide.cpp",
            "evidence": "contextual",
            "patterns": ["ButtonGuide::Open", "ButtonGuide::Draw"],
            "reason": "No exact `ui_help` layout-name hit exists in readable code, but the help layouts sit closest to the generic prompt/guide systems that surface control hints.",
        },
    ],
    "ui_loading": [
        {
            "path": "UnleashedRecomp/patches/resident_patches.cpp",
            "evidence": "direct",
            "patterns": ['Patch "ui_loading.yncp"', "LoadingScreenControllerMidAsmHook", "SWA::CLoading::Update"],
            "reason": "Direct readable patch evidence references `ui_loading.yncp`, patches its animation data, and swaps controller-platform string variants inside the same loading project.",
        },
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_loading/bg_1", "ui_loading/event_viewer/black/black_top", "ui_loading/pda/pda_frame/L"],
            "reason": "Direct layout-path rules expose the loading background, event-viewer black bars, and PDA frame branches from the extracted layout.",
        },
        {
            "path": "UnleashedRecomp/ui/black_bar.cpp",
            "evidence": "contextual",
            "patterns": ["Loading black-bar alpha overlays"],
            "reason": "The readable black-bar layer aligns with the loading layout's authored letterbox and black-bar scene families.",
        },
    ],
    "ui_exstage": [
        {
            "path": "UnleashedRecomp/patches/fps_patches.cpp",
            "evidence": "strong",
            "patterns": ["// Tornado Defense boss increments timers without respecting delta time.", "sub_82B00D00"],
            "reason": "Readable frame-rate fixes target the ExStage boss battle update seam that owns the extracted `ui_exstage` combat HUD timing.",
        },
    ],
    "ui_prov_playscreen": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_prov_playscreen/so_speed_gauge", "ui_prov_playscreen/ring_get_effect"],
            "reason": "Direct layout-path rules expose the Tornado Defense play-screen gauges, info blocks, and ring-get effect branches.",
        },
        {
            "path": "UnleashedRecomp/patches/object_patches.cpp",
            "evidence": "strong",
            "patterns": ["Tornado Defense bullet particles are colored by the button prompt", "sub_82B14CC0"],
            "reason": "Readable object patches tie Tornado Defense weapon feedback directly to controller-prompt coloration, matching the extracted play-screen HUD family.",
        },
        {
            "path": "UnleashedRecomp/patches/fps_patches.cpp",
            "evidence": "strong",
            "patterns": ["// Tornado Defense boss increments timers without respecting delta time.", "sub_82B00D00"],
            "reason": "Readable timing fixes confirm that Tornado Defense HUD behavior is driven by a dedicated ExStage boss battle update seam.",
        },
    ],
    "ui_qte": [
        {
            "path": "UnleashedRecomp/patches/misc_patches.cpp",
            "evidence": "strong",
            "patterns": ["Only allow enemy QTE prompts to get through.", "DisableEvilControlTutorialMidAsmHook"],
            "reason": "Readable mission/tutorial filtering proves the game distinguishes enemy QTE prompts as a discrete control-prompt stream.",
        },
        {
            "path": "UnleashedRecomp/patches/object_patches.cpp",
            "evidence": "strong",
            "patterns": ["Tornado Defense bullet particles are colored by the button prompt", "sub_82B14CC0"],
            "reason": "Readable object patches tie Tornado Defense visual feedback to button-prompt identity, matching the extracted `ui_qte` controller-prompt layout.",
        },
        {
            "path": "UnleashedRecomp/patches/fps_patches.cpp",
            "evidence": "contextual",
            "patterns": ["// Tornado Defense boss increments timers without respecting delta time.", "sub_82B00D00"],
            "reason": "Readable Tornado Defense timing fixes place the QTE layout inside the same ExStage battle loop as the extracted play-screen HUD.",
        },
    ],
    "ui_result": [
        {
            "path": "UnleashedRecomp/install/hashes/game.cpp",
            "evidence": "contextual",
            "patterns": ['"Sound/bgm_sys_result.csb"', '"Sound/vs_result_sonic_e.csb"'],
            "reason": "The install hash table exposes dedicated result-screen music cues that align with the extracted mission-result layout family.",
        },
    ],
    "ui_result_ex": [
        {
            "path": "UnleashedRecomp/install/hashes/game.cpp",
            "evidence": "contextual",
            "patterns": ['"Sound/bgm_sys_result.csb"', '"Sound/vs_result_evil_e.csb"'],
            "reason": "The install hash table exposes dedicated result-screen music cues for the extended/ex-stage result branch.",
        },
    ],
    "ui_start": [
        {
            "path": "UnleashedRecomp/locale/config_locale.cpp",
            "evidence": "contextual",
            "patterns": ["CONFIG_DEFINE_LOCALE(TimeOfDayTransition)", "spinning medal loading screen"],
            "reason": "Readable time-of-day transition policy sits adjacent to the extracted start/clear prompt package used during state handoff screens.",
        },
    ],
    "ui_worldmap": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_worldmap/contents/info/bg/cts_info_bg", "ui_worldmap/header/worldmap_header_bg", "ui_worldmap/footer/worldmap_footer_bg"],
            "reason": "Direct layout-path rules expose the extracted world-map info panes, header, and footer structures by name.",
        },
        {
            "path": "UnleashedRecomp/patches/input_patches.cpp",
            "evidence": "strong",
            "patterns": ["SWA::CWorldMapCamera::Update", "SWA::CWorldMapCursor", "WORLD_MAP_ROTATE_DEADZONE"],
            "reason": "Wraps world-map cursor/camera behavior, which is the interactive layer that drives the extracted world-map layout states.",
        },
        {
            "path": "UnleashedRecompLib/config/SWA.toml",
            "evidence": "strong",
            "patterns": ["WorldMapDeadzoneMidAsmHook", "WorldMapProjectionMidAsmHook", "WorldMapInfoMidAsmHook"],
            "reason": "Declares multiple world-map-specific hook sites that line up with the extracted info-pane and projection-sensitive layout content.",
        },
        {
            "path": "UnleashedRecomp/apu/embedded_player.cpp",
            "evidence": "contextual",
            "patterns": ["sys_worldmap_cursor", "sys_worldmap_finaldecide"],
            "reason": "Provides the world-map interaction sound hooks that likely accompany the extracted choice/info scene transitions.",
        },
    ],
    "ui_worldmap_help": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_worldmap_help/balloon/help_window/position/msg_bg_l", "ui_worldmap_help/balloon/help_window/position/msg_bg_r"],
            "reason": "Direct layout-path rules expose the left/right expandable help-window background pieces from the world-map help layout.",
        },
        {
            "path": "UnleashedRecomp/patches/input_patches.cpp",
            "evidence": "contextual",
            "patterns": ["SWA::CWorldMapCursor", "SWA::CWorldMapCamera::Update"],
            "reason": "World-map help overlays sit alongside the same world-map interaction loop that these readable input patches modify.",
        },
    ],
    "ui_saveicon": [
        {
            "path": "UnleashedRecomp/patches/resident_patches.cpp",
            "evidence": "strong",
            "patterns": ["SWA::CSaveIcon::Update", "App::s_isSaving = pSaveIcon->m_IsVisible"],
            "reason": "Directly hooks the save-icon visibility/update seam that drives the extracted save icon overlay.",
        },
    ],
    "ui_end": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "strong",
            "patterns": ["EndingTextAllocMidAsmHook", "EndingTextCtorRightMidAsmHook", "EndingTextPositionMidAsmHook"],
            "reason": "Readable ending-text hooks are the closest native seam to the extracted ending/staff-roll layout package.",
        },
        {
            "path": "UnleashedRecompLib/config/SWA.toml",
            "evidence": "strong",
            "patterns": ["EndingTextAllocMidAsmHook", "EndingTextCtorRightMidAsmHook", "EndingTextPositionMidAsmHook"],
            "reason": "Declares the ending/staff-roll hook sites that align with the extracted `ui_end` project.",
        },
    ],
    "ui_boss_gauge": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_boss_gauge/gauge_bg", "ui_boss_gauge/gauge_2", "ui_boss_gauge/gauge_breakpoint"],
            "reason": "Direct layout-path rules expose the boss gauge background, segments, and breakpoint elements by name.",
        },
    ],
    "ui_boss_name": [
        {
            "path": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
            "evidence": "direct",
            "patterns": ["ui_boss_name/name_so/bg", "ui_boss_name/name_so/pale"],
            "reason": "Direct layout-path rules expose the extracted boss-name layout's `name_so` branch and its horizontal unstretch handling.",
        },
    ],
}

EVIDENCE_ORDER = {
    "direct": 0,
    "strong": 1,
    "contextual": 2,
}


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def iter_source_files(repo_root: Path):
    for subdir in SOURCE_DIRS:
        root = repo_root / subdir
        if not root.exists():
            continue
        for path in sorted(root.rglob("*")):
            if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES:
                yield path


def line_number(text: str, index: int) -> int:
    return text.count("\n", 0, index) + 1


def unique_ordered(items):
    seen = set()
    ordered = []
    for item in items:
        if item in seen:
            continue
        seen.add(item)
        ordered.append(item)
    return ordered


def extract_layout_summary(record: dict) -> dict:
    path = record["path"]
    file_name = record["file_name"]
    layout_id = Path(file_name).stem.lower()
    archive_group = Path(path).parent.name
    resources = record.get("resources", [])
    project = {}
    root = {}
    if resources:
        project = resources[0].get("content", {}).get("csdm_project", {})
        root = project.get("root", {})

    scene_names = [item["name"] for item in root.get("scene_ids", [])]
    animation_names = []
    for scene in root.get("scenes", []):
        for anim in scene.get("animation_dictionaries", []):
            animation_names.append(anim["name"])

    texture_names = project.get("texture_name", [])

    return {
        "layout_id": layout_id,
        "file_name": file_name,
        "path": path,
        "paths": [path],
        "archive_group": archive_group,
        "role": LAYOUT_ROLES.get(layout_id, "unknown"),
        "scene_names": unique_ordered(scene_names),
        "animation_names": unique_ordered(animation_names),
        "texture_names": unique_ordered(texture_names),
    }


def merge_layout_variants(layouts: list[dict]) -> list[dict]:
    merged = {}
    for layout in layouts:
        current = merged.get(layout["layout_id"])
        if current is None:
            merged[layout["layout_id"]] = dict(layout)
            continue

        current["paths"] = unique_ordered(current["paths"] + layout["paths"])
        current["scene_names"] = unique_ordered(current["scene_names"] + layout["scene_names"])
        current["animation_names"] = unique_ordered(current["animation_names"] + layout["animation_names"])
        current["texture_names"] = unique_ordered(current["texture_names"] + layout["texture_names"])

    values = list(merged.values())
    values.sort(key=lambda item: (item["role"], item["layout_id"], item["path"]))
    return values


def build_source_cache(repo_root: Path) -> dict[str, dict]:
    cache = {}
    for path in iter_source_files(repo_root):
        relative = path.relative_to(repo_root).as_posix()
        text = path.read_text(encoding="utf-8", errors="ignore")
        cache[relative] = {
            "path": relative,
            "text": text,
            "lines": text.splitlines(),
            "lower": text.lower(),
        }
    return cache


def resolve_line(file_entry: dict, patterns: list[str]) -> tuple[int | None, str | None]:
    for pattern in patterns:
        index = file_entry["text"].find(pattern)
        if index != -1:
            line = line_number(file_entry["text"], index)
            return line, file_entry["lines"][line - 1].strip()
    return None, None


def automatic_matches(layout: dict, source_cache: dict[str, dict]) -> list[dict]:
    layout_id = layout["layout_id"]
    patterns = [layout_id]
    patterns.extend(f"{layout_id}/{scene.lower()}" for scene in layout["scene_names"][:20])

    matches = []
    for file_entry in source_cache.values():
        file_hits = []
        for idx, line in enumerate(file_entry["lines"], start=1):
            lowered = line.lower()
            if any(pattern in lowered for pattern in patterns):
                file_hits.append(
                    {
                        "path": file_entry["path"],
                        "line": idx,
                        "excerpt": line.strip()[:240],
                        "evidence": "direct",
                        "reason": "Readable source references the extracted layout name or an exact layout/scene path.",
                    }
                )
            if len(file_hits) >= 4:
                break
        matches.extend(file_hits)

    return matches


def manual_matches(layout: dict, source_cache: dict[str, dict]) -> list[dict]:
    matches = []
    for hint in MANUAL_HINTS.get(layout["layout_id"], []):
        file_entry = source_cache.get(hint["path"])
        if not file_entry:
            continue
        line, excerpt = resolve_line(file_entry, hint["patterns"])
        matches.append(
            {
                "path": hint["path"],
                "line": line,
                "excerpt": excerpt,
                "evidence": hint["evidence"],
                "reason": hint["reason"],
            }
        )
    return matches


def generated_matches(layout: dict, generated_refs: list[dict]) -> list[dict]:
    patch_files = set(LAYOUT_PATCH_FILES.get(layout["layout_id"], []))
    if not patch_files:
        return []

    matches = []
    for ref in generated_refs:
        ref_patch_files = set(ref.get("patch_files", []))
        if patch_files & ref_patch_files:
            matches.append(
                {
                    "symbol": ref["symbol"],
                    "patch_files": ref.get("patch_files", []),
                    "generated_file": ref.get("generated_file"),
                    "impl_line": ref.get("impl_line"),
                    "readable_relationship": ref.get("readable_relationship"),
                }
            )
    matches.sort(key=lambda item: item["symbol"])
    return matches


def merge_matches(automatic: list[dict], manual: list[dict]) -> list[dict]:
    merged = {}
    for match in automatic + manual:
        key = (match["path"], match.get("line"))
        current = merged.get(key)
        if current is None:
            merged[key] = {
                **match,
                "reasons": [match["reason"]],
            }
            continue

        current["reasons"] = unique_ordered(current["reasons"] + [match["reason"]])
        if EVIDENCE_ORDER[match["evidence"]] < EVIDENCE_ORDER[current["evidence"]]:
            current["evidence"] = match["evidence"]
        if not current.get("excerpt") and match.get("excerpt"):
            current["excerpt"] = match["excerpt"]
    values = list(merged.values())
    values.sort(key=lambda item: (EVIDENCE_ORDER[item["evidence"]], item["path"], item["line"] or 0))
    return values


def correlation_verdict(matches: list[dict]) -> str:
    if any(match["evidence"] == "direct" for match in matches):
        return "direct"
    if any(match["evidence"] == "strong" for match in matches):
        return "strong"
    if matches:
        return "contextual"
    return "unknown"


def build_entry(layout: dict, source_cache: dict[str, dict], generated_refs: list[dict]) -> dict:
    auto = automatic_matches(layout, source_cache)
    manual = manual_matches(layout, source_cache)
    matches = merge_matches(auto, manual)
    generated = generated_matches(layout, generated_refs)

    return {
        **layout,
        "verdict": correlation_verdict(matches),
        "match_count": len(matches),
        "matches": matches,
        "generated_refs": generated,
    }


def build_payload(repo_root: Path, layout_json: Path, generated_json: Path | None) -> dict:
    layout_data = read_json(layout_json)
    generated_data = read_json(generated_json) if generated_json and generated_json.exists() else {"references": []}
    layouts = merge_layout_variants([extract_layout_summary(item) for item in layout_data.get("parsed_files", [])])

    source_cache = build_source_cache(repo_root)
    entries = [build_entry(layout, source_cache, generated_data.get("references", [])) for layout in layouts]

    verdict_counts = defaultdict(int)
    for entry in entries:
        verdict_counts[entry["verdict"]] += 1

    return {
        "repo_root": repo_root.as_posix(),
        "layout_json": layout_json.as_posix(),
        "generated_json": generated_json.as_posix() if generated_json else None,
        "layout_count": len(entries),
        "verdict_counts": dict(sorted(verdict_counts.items())),
        "entries": entries,
    }


def write_markdown(path: Path, payload: dict) -> None:
    lines = [
        "# Code-to-Layout Correlation",
        "",
        "Machine-readable inventory: `research_uiux/data/layout_code_correlation.json`",
        "",
        "## Summary",
        "",
        f"- Layout files correlated: `{payload['layout_count']}`",
        f"- Direct-correlation layouts: `{payload['verdict_counts'].get('direct', 0)}`",
        f"- Strong-correlation layouts: `{payload['verdict_counts'].get('strong', 0)}`",
        f"- Context-only layouts: `{payload['verdict_counts'].get('contextual', 0)}`",
        f"- Unresolved layouts: `{payload['verdict_counts'].get('unknown', 0)}`",
        "",
        "> [!NOTE]",
        "> `direct` means the readable code names the extracted layout or scene path explicitly.",
        "> `strong` means the readable code wraps the same game subsystem/state without exposing the raw layout string.",
        "> `contextual` means the readable code is adjacent support infrastructure rather than a direct asset-name seam.",
        "",
    ]

    for entry in payload["entries"]:
        lines.append(f"## `{entry['layout_id']}`")
        lines.append("")
        if len(entry["paths"]) == 1:
            lines.append(f"- Asset path: `{entry['path']}`")
        else:
            lines.append(f"- Asset variants: `{', '.join(entry['paths'])}`")
        lines.append(f"- Archive group: `{entry['archive_group']}`")
        lines.append(f"- Inferred role: `{entry['role']}`")
        lines.append(f"- Correlation verdict: `{entry['verdict']}`")
        if entry["scene_names"]:
            lines.append(f"- Scene cues: `{', '.join(entry['scene_names'][:10])}`")
        if entry["animation_names"]:
            lines.append(f"- Animation cues: `{', '.join(entry['animation_names'][:10])}`")
        lines.append("")
        lines.append("### Readable Correlations")
        lines.append("")

        if entry["matches"]:
            for match in entry["matches"]:
                location = f"{match['path']}:{match['line']}" if match["line"] else match["path"]
                lines.append(f"- `{match['evidence']}` `{location}`")
                if match.get("excerpt"):
                    lines.append(f"  - evidence line: `{match['excerpt']}`")
                lines.append(f"  - why it matters: {' '.join(match['reasons'])}")
        else:
            lines.append("- No readable-code correlation found yet beyond the extracted asset itself.")

        if entry["generated_refs"]:
            lines.append("")
            lines.append("### Generated Seams")
            lines.append("")
            for ref in entry["generated_refs"]:
                generated_file = ref["generated_file"] or "<missing>"
                location = generated_file
                if ref.get("impl_line"):
                    location = f"{generated_file}:{ref['impl_line']}"
                lines.append(f"- `{ref['symbol']}` via `{', '.join(ref['patch_files'])}`")
                lines.append(f"  - generated location: `{location}`")
                lines.append(f"  - readable relationship: {ref['readable_relationship']}")

        lines.append("")

    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Correlate extracted UI layouts against readable code, hooks, and generated seams.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument(
        "--layout-json",
        default="research_uiux/data/layout_semantics.json",
        help="Layout semantics JSON produced by inspect_xncp_yncp.py.",
    )
    parser.add_argument(
        "--generated-json",
        default="research_uiux/data/generated_function_refs.json",
        help="Generated seam reference JSON.",
    )
    parser.add_argument(
        "--output-json",
        default="research_uiux/data/layout_code_correlation.json",
        help="Output JSON path.",
    )
    parser.add_argument(
        "--output-md",
        default="research_uiux/CODE_TO_LAYOUT_CORRELATION.md",
        help="Output markdown path.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    layout_json = (repo_root / args.layout_json).resolve() if not Path(args.layout_json).is_absolute() else Path(args.layout_json)
    generated_json = (repo_root / args.generated_json).resolve() if not Path(args.generated_json).is_absolute() else Path(args.generated_json)
    output_json = (repo_root / args.output_json).resolve() if not Path(args.output_json).is_absolute() else Path(args.output_json)
    output_md = (repo_root / args.output_md).resolve() if not Path(args.output_md).is_absolute() else Path(args.output_md)

    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)

    payload = build_payload(repo_root, layout_json, generated_json)
    output_json.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    write_markdown(output_md, payload)

    print(output_json)
    print(output_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
