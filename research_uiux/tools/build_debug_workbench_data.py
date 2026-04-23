#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path

STAGE_TEST_CONTRACT_OVERRIDES = {
    "System/GameMode/GameModeStageForwardTest.cpp": "sonic_stage_hud_reference.json",
    "System/GameMode/GameModeStageEvilTest.cpp": "werehog_stage_hud_reference.json",
    "System/GameMode/GameModeStageMotionTest.cpp": "extra_stage_hud_reference.json",
    "System/GameMode/GameModeStageAchievementTest.cpp": "boss_hud_reference.json",
}

GAMEPLAY_HUD_GROUPS = {
    "sonic_stage_hud": {
        "group_id": "gameplay_hud_hosts",
        "display_name": "Gameplay HUD Hosts",
        "priority": "high",
        "primary_contract_file_name": "sonic_stage_hud_reference.json",
        "extra_alias_tokens": ["GameModeStageForwardTest.cpp", "ui_playscreen"],
        "note": "Day-stage HUD ownership with counters, medal sidecars, and boost/ring gauge shells.",
    },
    "werehog_stage_hud": {
        "group_id": "gameplay_hud_hosts",
        "display_name": "Gameplay HUD Hosts",
        "priority": "high",
        "primary_contract_file_name": "werehog_stage_hud_reference.json",
        "extra_alias_tokens": ["GameModeStageEvilTest.cpp", "ui_playscreen_ev", "ui_playscreen_ev_hit"],
        "note": "Werehog-stage HUD ownership with life/unleash/shield gauges and hit-counter overlays.",
    },
    "extra_stage_hud": {
        "group_id": "gameplay_hud_hosts",
        "display_name": "Gameplay HUD Hosts",
        "priority": "high",
        "primary_contract_file_name": "extra_stage_hud_reference.json",
        "extra_alias_tokens": ["GameModeStageMotionTest.cpp", "ui_prov_playscreen", "ui_qte"],
        "note": "Extra-stage / Tornado Defense HUD ownership with counter shells and QTE prompt layers.",
    },
    "super_sonic_hud": {
        "group_id": "boss_hud_hosts",
        "display_name": "Boss / Final HUD Hosts",
        "priority": "high",
        "primary_contract_file_name": "super_sonic_hud_reference.json",
        "extra_alias_tokens": ["ui_playscreen_su"],
        "note": "Super Sonic and final-phase HUD bridge with shared vitality and nameplate ownership.",
    },
    "boss_hud": {
        "group_id": "boss_hud_hosts",
        "display_name": "Boss / Final HUD Hosts",
        "priority": "high",
        "primary_contract_file_name": "boss_hud_reference.json",
        "extra_alias_tokens": ["ui_boss_gauge", "ui_boss_name"],
        "note": "Generic boss HUD ownership for vitality bars and nameplate overlays beyond the Super Sonic bridge.",
    },
}

SHELL_GROUPS = {
    "town_ui": {
        "group_id": "town_media_room_hosts",
        "display_name": "Town / Media Room Hosts",
        "priority": "high",
        "primary_contract_file_name": "town_ui_reference.json",
        "extra_alias_tokens": ["TownManager.cpp", "TalkWindow.cpp", "ShopWindow.cpp", "MediaRoom.cpp"],
        "note": "Town, shop, talk, and media-room ownership tied back to the town shell runtime contract.",
    },
    "camera_shell": {
        "group_id": "camera_replay_hosts",
        "display_name": "Camera / Replay Hosts",
        "priority": "medium",
        "primary_contract_file_name": "camera_shell_reference.json",
        "extra_alias_tokens": ["FreeCamera.cpp", "ReplayFreeCamera.cpp", "GoalCamera.cpp"],
        "note": "Free, replay, goal, and town-support camera hosts driven through the camera/replay shell contract.",
    },
    "frontend_sequence_shell": {
        "group_id": "frontend_sequence_hosts",
        "display_name": "Frontend Sequence Hosts",
        "priority": "high",
        "primary_contract_file_name": "frontend_sequence_shell_reference.json",
        "extra_alias_tokens": ["SequenceManagerImpl.cpp", "SequenceHandleUnit.cpp", "SequenceUnitFactory.cpp"],
        "note": "Sequence-core and unit-factory hosts driven through the recovered frontend sequence shell contract.",
    },
    "application_world_shell": {
        "group_id": "application_world_shell_hosts",
        "display_name": "Application / World Shell Hosts",
        "priority": "high",
        "primary_contract_file_name": "application_world_shell_reference.json",
        "extra_alias_tokens": ["Application.cpp", "GameModeBoot.cpp", "TitleMenu.cpp", "WorldMapSelect.cpp"],
        "note": "Application, document, gamemode, title, and world-shell ownership tied to the portable app/world contract.",
    },
}


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def cpp_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def normalize_token(value: str) -> str:
    return value.replace("\\", "/")


def unique_ordered(values: list[str]) -> list[str]:
    seen = set()
    result = []
    for value in values:
        if value in seen:
            continue
        seen.add(value)
        result.append(value)
    return result


def primary_contract_for_entry(group: dict, entry: dict) -> str | None:
    runtime_contracts = entry.get("runtime_contracts", [])
    if runtime_contracts:
        return runtime_contracts[0]

    relative_source_path = entry["relative_source_path"]
    lowered = relative_source_path.lower()

    if relative_source_path in STAGE_TEST_CONTRACT_OVERRIDES:
        return STAGE_TEST_CONTRACT_OVERRIDES[relative_source_path]

    if "inspirepreview" in lowered or "stagemovie" in lowered or "movie/" in lowered or "playmovie" in lowered:
        return "subtitle_cutscene_reference.json"
    if "gamemodemenuselectdebug" in lowered or "gamemodemainmenu_test" in lowered:
        return "title_menu_reference.json"
    if "gamemodestageselectdebug" in lowered:
        return "loading_transition_reference.json"

    likely_runtime_contracts = group.get("likely_runtime_contracts", [])
    if likely_runtime_contracts:
        return likely_runtime_contracts[0]
    return None


def build_frontend_host_entries(frontend_payload: dict) -> list[dict]:
    entries: list[dict] = []

    for group in frontend_payload.get("priority_groups", []):
        priority = group.get("priority", "medium")
        if priority not in {"immediate", "high"}:
            continue

        for entry in group.get("entries", []):
            primary_contract = primary_contract_for_entry(group, entry)
            if not primary_contract:
                continue

            relative_source_path = entry["relative_source_path"]
            alias_tokens = unique_ordered(
                [
                    group["display_name"],
                    group["group_id"],
                    entry["family_name"],
                    entry["family_id"],
                    relative_source_path,
                    Path(relative_source_path).name,
                    primary_contract,
                    Path(primary_contract).stem,
                ]
            )

            entries.append(
                {
                    "group_id": group["group_id"],
                    "group_display_name": group["display_name"],
                    "priority": priority,
                    "relative_source_path": relative_source_path,
                    "host_display_name": Path(relative_source_path).name,
                    "primary_contract_file_name": primary_contract,
                    "alias_tokens": alias_tokens,
                    "notes": entry.get("humanization_priority", group.get("rationale", "")),
                }
            )

    entries.sort(key=lambda item: (item["group_display_name"], item["host_display_name"]))
    return entries


def build_gameplay_hud_host_entries(hud_payload: dict) -> list[dict]:
    entries: list[dict] = []

    for system in hud_payload.get("systems", []):
        config = GAMEPLAY_HUD_GROUPS.get(system["system_id"])
        if not config:
            continue

        layout_ids = [layout["layout_id"] for layout in system.get("layout_families", [])]
        for relative_source_path in system.get("host_paths", []):
            alias_tokens = unique_ordered(
                [
                    config["display_name"],
                    config["group_id"],
                    system["screen_name"],
                    system["system_id"],
                    relative_source_path,
                    Path(relative_source_path).name,
                    config["primary_contract_file_name"],
                    Path(config["primary_contract_file_name"]).stem,
                    *layout_ids,
                    *config["extra_alias_tokens"],
                ]
            )

            entries.append(
                {
                    "group_id": config["group_id"],
                    "group_display_name": config["display_name"],
                    "priority": config["priority"],
                    "relative_source_path": relative_source_path,
                    "host_display_name": Path(relative_source_path).name,
                    "primary_contract_file_name": config["primary_contract_file_name"],
                    "alias_tokens": alias_tokens,
                    "notes": config["note"],
                }
            )

    entries.sort(key=lambda item: (item["group_display_name"], item["host_display_name"]))
    return entries


def build_shell_host_entries(manifest_payload: dict) -> list[dict]:
    entries: list[dict] = []

    for system in manifest_payload.get("systems", []):
        config = SHELL_GROUPS.get(system["system_id"])
        if not config:
            continue

        for entry in manifest_payload.get("entries", []):
            if system["system_id"] not in entry.get("matched_system_ids", []):
                continue

            relative_source_path = entry["relative_source_path"]
            alias_tokens = unique_ordered(
                [
                    config["display_name"],
                    config["group_id"],
                    system["screen_name"],
                    system["system_id"],
                    entry["family_name"],
                    entry["family_id"],
                    relative_source_path,
                    Path(relative_source_path).name,
                    config["primary_contract_file_name"],
                    Path(config["primary_contract_file_name"]).stem,
                    *config["extra_alias_tokens"],
                ]
            )

            entries.append(
                {
                    "group_id": config["group_id"],
                    "group_display_name": config["display_name"],
                    "priority": config["priority"],
                    "relative_source_path": relative_source_path,
                    "host_display_name": Path(relative_source_path).name,
                    "primary_contract_file_name": config["primary_contract_file_name"],
                    "alias_tokens": alias_tokens,
                    "notes": config["note"],
                }
            )

    entries.sort(key=lambda item: (item["group_display_name"], item["host_display_name"]))
    return entries


def build_host_entries(frontend_payload: dict, hud_payload: dict, manifest_payload: dict) -> list[dict]:
    entries = (
        build_frontend_host_entries(frontend_payload)
        + build_gameplay_hud_host_entries(hud_payload)
        + build_shell_host_entries(manifest_payload)
    )
    deduped: list[dict] = []
    seen_paths: set[str] = set()
    for entry in sorted(entries, key=lambda item: (item["group_display_name"], item["host_display_name"])):
        if entry["relative_source_path"] in seen_paths:
            continue
        seen_paths.add(entry["relative_source_path"])
        deduped.append(entry)
    return deduped


def write_header(entries: list[dict], output_path: Path) -> None:
    lines = [
        "#pragma once",
        "",
        "#include <array>",
        "#include <string_view>",
        "",
        "namespace sward::ui_runtime",
        "{",
        "struct DebugWorkbenchHostEntry",
        "{",
        "    std::string_view groupId;",
        "    std::string_view groupDisplayName;",
        "    std::string_view priority;",
        "    std::string_view relativeSourcePath;",
        "    std::string_view hostDisplayName;",
        "    std::string_view primaryContractFileName;",
        "    std::string_view aliasBlob;",
        "    std::string_view notes;",
        "};",
        "",
        f"inline constexpr std::array<DebugWorkbenchHostEntry, {len(entries)}> kDebugWorkbenchHostEntries{{{{",
    ]

    for entry in entries:
        alias_blob = "|".join(entry["alias_tokens"])
        lines.extend(
            [
                "    {",
                f'        "{cpp_string(entry["group_id"])}",',
                f'        "{cpp_string(entry["group_display_name"])}",',
                f'        "{cpp_string(entry["priority"])}",',
                f'        "{cpp_string(entry["relative_source_path"])}",',
                f'        "{cpp_string(entry["host_display_name"])}",',
                f'        "{cpp_string(entry["primary_contract_file_name"])}",',
                f'        "{cpp_string(alias_blob)}",',
                f'        "{cpp_string(entry["notes"])}",',
                "    },",
            ]
        )

    lines.extend(
        [
            "}};",
            "} // namespace sward::ui_runtime",
        ]
    )
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate richer debug workbench host metadata from the frontend shell recovery layer.")
    parser.add_argument("--input", default="research_uiux/data/frontend_shell_recovery.json", help="Frontend shell recovery JSON.")
    parser.add_argument("--hud-input", default="research_uiux/data/gameplay_hud_core_map.json", help="Gameplay HUD core map JSON.")
    parser.add_argument("--manifest-input", default="research_uiux/data/ui_source_path_manifest.json", help="Broader source-path manifest JSON.")
    parser.add_argument("--output-header", default="research_uiux/runtime_reference/include/sward/ui_runtime/debug_workbench_data.hpp", help="Generated workbench metadata header.")
    parser.add_argument("--output-json", default="research_uiux/data/debug_workbench_host_map.json", help="Tracked debug workbench host JSON.")
    args = parser.parse_args()

    input_path = Path(args.input).resolve()
    hud_input_path = Path(args.hud_input).resolve()
    manifest_input_path = Path(args.manifest_input).resolve()
    output_header = Path(args.output_header).resolve()
    output_json = Path(args.output_json).resolve()

    frontend_payload = read_json(input_path)
    hud_payload = read_json(hud_input_path)
    manifest_payload = read_json(manifest_input_path)
    entries = build_host_entries(frontend_payload, hud_payload, manifest_payload)

    output_header.parent.mkdir(parents=True, exist_ok=True)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    write_header(entries, output_header)
    output_json.write_text(json.dumps({"hosts": entries}, indent=2) + "\n", encoding="utf-8")
    print("built_debug_workbench_data", f"hosts={len(entries)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
