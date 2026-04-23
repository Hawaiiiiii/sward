#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


LOCAL_HINTS = [
    {
        "hint_id": "install_debug_menu_from_title",
        "summary": "The supplied local TCRF-derived note set says a preview-build Install debug menu was reachable from the title screen, which strengthens the case for title/menu-adjacent debug hosts.",
    },
    {
        "hint_id": "debug_operation_tool",
        "summary": "The supplied local TCRF-derived note set says a Debug Operation Tool could stay open through gameplay/cutscene transitions, which makes the debug-host layer relevant for a future mixed gameplay/frontend sandbox.",
    },
    {
        "hint_id": "inspire_preview_hosts",
        "summary": "The supplied local TCRF-derived note set explicitly names InspirePreview and InspirePreview2nd as launchable preview tools, which strongly supports treating those files as cutscene/subtitle preview hosts.",
    },
    {
        "hint_id": "unused_trials_via_debug",
        "summary": "The supplied local TCRF-derived note set says unused trials could be reached through debug tools, which supports prioritizing menu/stage debug game modes as future UI sandbox hosts.",
    },
]


GROUPS = [
    {
        "group_id": "menu_debug_hosts",
        "display_name": "Menu / Stage Debug Hosts",
        "priority": "immediate",
        "relative_paths": [
            "System/GameMode/GameModeMainMenu_Test.cpp",
            "System/GameMode/GameModeMenuSelectDebug.cpp",
            "System/GameMode/GameModeStageSelectDebug.cpp",
        ],
        "likely_target_system_ids": ["title_menu", "loading_and_start"],
        "likely_runtime_contracts": ["title_menu_reference.json", "loading_transition_reference.json"],
        "rationale": "These are the most obvious menu-level and stage-selection debug hosts for a future standalone UI browser executable.",
        "hint_ids": ["install_debug_menu_from_title", "unused_trials_via_debug"],
    },
    {
        "group_id": "stage_test_hosts",
        "display_name": "Stage Test / Validation Hosts",
        "priority": "high",
        "relative_paths": [
            "System/GameMode/GameModeStageAchievementTest.cpp",
            "System/GameMode/GameModeStageEvilTest.cpp",
            "System/GameMode/GameModeStageForwardTest.cpp",
            "System/GameMode/GameModeStageInstallTest.cpp",
            "System/GameMode/GameModeStageLoadXML.cpp",
            "System/GameMode/GameModeStageMotionTest.cpp",
            "System/GameMode/GameModeStageSaveTest.cpp",
            "System/GameMode/GameModeStageScreenshot.cpp",
            "System/GameMode/GameModeStageSwapDiskTest.cpp",
        ],
        "likely_target_system_ids": ["loading_and_start", "save_and_ending", "sonic_stage_hud", "werehog_stage_hud"],
        "likely_runtime_contracts": ["loading_transition_reference.json", "autosave_toast_reference.json"],
        "rationale": "These hosts look like the bridge between frontend/menu debug control and in-stage validation surfaces such as loading, save flow, screenshots, and gameplay HUD probes.",
        "hint_ids": ["unused_trials_via_debug"],
    },
    {
        "group_id": "cutscene_preview_hosts",
        "display_name": "Cutscene / Preview Hosts",
        "priority": "immediate",
        "relative_paths": [
            "Tool/InspirePreview/InspirePreview.cpp",
            "Tool/InspirePreview/InspirePreviewMenu.cpp",
            "Tool/InspirePreview/InspireObject.cpp",
            "Tool/InspirePreview2nd/InspirePreview2nd.cpp",
            "Tool/InspirePreview2nd/InspirePreview2ndMenu.cpp",
            "Tool/MotionCameraTool/MotionCameraTool.cpp",
            "Tool/MotionCameraTool/MotionCameraMenu.cpp",
            "System/GameMode/GameModeStageMovie.cpp",
            "Movie/MovieManager.cpp",
            "Sequence/Unit/SequenceUnitPlayMovie.cpp",
            "Sequence/Unit/SequenceUnitMicroSequence.cpp",
            "Sequence/Utility/SequencePlayMovieWrapper.cpp",
        ],
        "likely_target_system_ids": ["subtitle_cutscene_presentation"],
        "likely_runtime_contracts": [],
        "rationale": "These preview, movie, and sequence surfaces are the strongest current path toward a cutscene/subtitle-capable debug sandbox instead of a title-only selector.",
        "hint_ids": ["debug_operation_tool", "inspire_preview_hosts"],
    },
    {
        "group_id": "town_dispatch_hosts",
        "display_name": "Town / Media-Room Dispatch Hosts",
        "priority": "high",
        "relative_paths": [
            "Camera/Controller/TownShopCamera.cpp",
            "Camera/Controller/TownTalkCamera.cpp",
            "Sequence/Unit/SequenceUnitSendMediaRoomMessage.cpp",
            "Sequence/Unit/SequenceUnitSendTownMessage.cpp",
        ],
        "likely_target_system_ids": ["town_ui"],
        "likely_runtime_contracts": [],
        "rationale": "These camera and sequence dispatch surfaces now form a tighter bridge into the town/media-room frontend family.",
        "hint_ids": [],
    },
    {
        "group_id": "pause_help_and_loading_dispatch",
        "display_name": "Pause / Help / Loading Dispatch",
        "priority": "high",
        "relative_paths": [
            "Sequence/Unit/SequenceUnitCallHelpWindow.cpp",
            "Sequence/Unit/SequenceUnitChangeStage.cpp",
            "Sequence/Unit/SequenceUnitSwapDisk.cpp",
            "Sequence/Utility/SequenceChangeStageUnit.cpp",
        ],
        "likely_target_system_ids": ["pause_stack", "loading_and_start"],
        "likely_runtime_contracts": ["pause_menu_reference.json", "loading_transition_reference.json"],
        "rationale": "These sequence-layer dispatchers are now concrete bridges into the current pause/help and loading/start runtime contracts.",
        "hint_ids": [],
    },
    {
        "group_id": "free_camera_and_replay_hosts",
        "display_name": "Free Camera / Replay Hosts",
        "priority": "medium",
        "relative_paths": [
            "Tool/FreeCameraTool/FreeCameraTool.cpp",
            "Camera/Controller/FreeCamera.cpp",
            "Camera/Controller/GoalCamera.cpp",
            "Replay/Camera/ReplayFreeCamera.cpp",
            "Replay/Camera/ReplayRelativeCamera.cpp",
        ],
        "likely_target_system_ids": [],
        "likely_runtime_contracts": [],
        "rationale": "These are better treated as inspection and traversal hosts for the future debug sandbox than as ordinary frontend camera ownership.",
        "hint_ids": ["debug_operation_tool"],
    },
    {
        "group_id": "frontend_app_shell",
        "display_name": "Application / World Shell",
        "priority": "medium",
        "relative_paths": [
            "System/Application.cpp",
            "System/ApplicationDocument.cpp",
            "System/ApplicationSetting.cpp",
            "System/Game.cpp",
            "System/GameDocument.cpp",
            "System/NextStagePreloadingManager.cpp",
            "System/StageListManager.cpp",
            "System/StageManager.cpp",
            "System/World.cpp",
        ],
        "likely_target_system_ids": [],
        "likely_runtime_contracts": [],
        "rationale": "These files look like app/world ownership shells rather than screen implementations, but they are the layer that will eventually need named translated ownership once the debug executable stops being purely menu-bound.",
        "hint_ids": [],
    },
]


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def unique_ordered(values):
    seen = set()
    result = []
    for value in values:
        if value in seen:
            continue
        seen.add(value)
        result.append(value)
    return result


def percentage(numerator: int, denominator: int) -> float:
    if denominator == 0:
        return 0.0
    return round((numerator / denominator) * 100.0, 1)


def build_payload(manifest: dict) -> dict:
    entries_by_path = {entry["relative_source_path"]: entry for entry in manifest.get("entries", [])}

    groups = []
    targeted_paths = []
    bridged_paths = []
    contract_paths = []
    unresolved_paths = []

    for group in GROUPS:
        present_entries = [entries_by_path[path] for path in group["relative_paths"] if path in entries_by_path]
        if not present_entries:
            continue

        targeted_paths.extend(entry["relative_source_path"] for entry in present_entries)
        bridged_paths.extend(
            entry["relative_source_path"]
            for entry in present_entries
            if entry.get("matched_system_ids")
        )
        contract_paths.extend(
            entry["relative_source_path"]
            for entry in present_entries
            if entry.get("runtime_contracts")
        )
        unresolved_paths.extend(
            entry["relative_source_path"]
            for entry in present_entries
            if not entry.get("matched_system_ids") and not entry.get("debug_tool_candidate")
        )

        groups.append(
            {
                "group_id": group["group_id"],
                "display_name": group["display_name"],
                "priority": group["priority"],
                "path_count": len(present_entries),
                "likely_target_system_ids": group["likely_target_system_ids"],
                "likely_runtime_contracts": group["likely_runtime_contracts"],
                "rationale": group["rationale"],
                "hint_ids": group["hint_ids"],
                "status_counts": {
                    "contract_backed": sum(1 for entry in present_entries if entry.get("status") == "contract_backed"),
                    "archaeology_mapped": sum(1 for entry in present_entries if entry.get("status") == "archaeology_mapped"),
                    "debug_tool_candidate": sum(1 for entry in present_entries if entry.get("status") == "debug_tool_candidate"),
                    "named_seed_only": sum(1 for entry in present_entries if entry.get("status") == "named_seed_only"),
                },
                "sample_paths": [entry["relative_source_path"] for entry in present_entries[:6]],
                "entries": [
                    {
                        "relative_source_path": entry["relative_source_path"],
                        "family_id": entry["family_id"],
                        "family_name": entry["family_name"],
                        "status": entry["status"],
                        "matched_system_ids": entry.get("matched_system_ids", []),
                        "runtime_contracts": entry.get("runtime_contracts", []),
                        "humanization_priority": entry.get("humanization_priority"),
                    }
                    for entry in present_entries
                ],
            }
        )

    priority_order = {"immediate": 0, "high": 1, "medium": 2, "low": 3}
    groups.sort(key=lambda item: (priority_order.get(item["priority"], 9), -item["path_count"], item["display_name"]))

    payload = {
        "summary": {
            "group_count": len(groups),
            "targeted_path_count": len(unique_ordered(targeted_paths)),
            "bridged_path_count": len(unique_ordered(bridged_paths)),
            "bridged_path_pct": percentage(len(unique_ordered(bridged_paths)), len(unique_ordered(targeted_paths))),
            "contract_backed_path_count": len(unique_ordered(contract_paths)),
            "contract_backed_path_pct": percentage(len(unique_ordered(contract_paths)), len(unique_ordered(targeted_paths))),
            "unresolved_shell_path_count": len(unique_ordered(unresolved_paths)),
            "unresolved_shell_path_pct": percentage(len(unique_ordered(unresolved_paths)), len(unique_ordered(targeted_paths))),
        },
        "local_hints": LOCAL_HINTS,
        "priority_groups": groups,
        "top_host_candidates": [
            {
                "relative_source_path": path,
                "reason": reason,
            }
            for path, reason in [
                ("System/GameMode/GameModeMenuSelectDebug.cpp", "Best current menu-debug host for a future selector-driven frontend executable."),
                ("System/GameMode/GameModeStageSelectDebug.cpp", "Best current stage-debug host for jumping from frontend control into gameplay-facing validation."),
                ("Tool/InspirePreview/InspirePreview.cpp", "Strongest cutscene preview host named directly by the supplied local note set."),
                ("Tool/InspirePreview2nd/InspirePreview2nd.cpp", "Newer cutscene preview host for isolated prerendered sequence checks."),
                ("Tool/FreeCameraTool/FreeCameraTool.cpp", "Inspection host for a future debug sandbox where menu/UI overlays and world traversal coexist."),
                ("Tool/MotionCameraTool/MotionCameraTool.cpp", "Camera-preview host that complements cutscene/timeline validation."),
            ]
        ],
    }
    return payload


def write_markdown(payload: dict, output_path: Path) -> None:
    summary = payload["summary"]
    lines = [
        '<p align="right">',
        '    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>',
        "</p>",
        "",
        '# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Frontend Shell And Debug Host Recovery',
        "",
        "Phase 35 tightens the newly widened shell/debug families instead of leaving them as a pile of named-but-unowned paths.",
        "",
        "> [!IMPORTANT]",
        "> This report still does not claim the original frontend/debug source is fully recovered. It narrows the host layer so the future debug executable has defensible landing zones and the translated cleanup can follow clearer ownership.",
        "",
        "## Snapshot",
        "",
        f"- Targeted shell/debug paths in this pass: `{summary['targeted_path_count']}`",
        f"- Paths with current archaeology bridges: `{summary['bridged_path_count']}` (`{summary['bridged_path_pct']:.1f}%`)",
        f"- Paths already landing on current runtime contracts: `{summary['contract_backed_path_count']}` (`{summary['contract_backed_path_pct']:.1f}%`)",
        f"- Paths still shell-only after this pass: `{summary['unresolved_shell_path_count']}` (`{summary['unresolved_shell_path_pct']:.1f}%`)",
        "",
        "## Local Hint Set Used",
        "",
    ]

    for hint in payload["local_hints"]:
        lines.append(f"- `{hint['hint_id']}`: {hint['summary']}")

    lines.extend(
        [
            "",
            "## Priority Groups",
            "",
            "| Group | Priority | Paths | Bridged | Contract-backed |",
            "|---|---|---:|---:|---:|",
        ]
    )

    for group in payload["priority_groups"]:
        status_counts = group["status_counts"]
        bridged = status_counts["contract_backed"] + status_counts["archaeology_mapped"]
        lines.append(
            f"| {group['display_name']} | `{group['priority']}` | `{group['path_count']}` | `{bridged}` | `{status_counts['contract_backed']}` |"
        )

    lines.extend(
        [
            "",
            "## Group Notes",
            "",
        ]
    )

    for group in payload["priority_groups"]:
        lines.append(f"### {group['display_name']}")
        lines.append("")
        lines.append(group["rationale"])
        lines.append("")
        if group["likely_target_system_ids"]:
            lines.append("Likely target systems:")
            for system_id in group["likely_target_system_ids"]:
                lines.append(f"- `{system_id}`")
            lines.append("")
        if group["likely_runtime_contracts"]:
            lines.append("Likely runtime contracts:")
            for contract in group["likely_runtime_contracts"]:
                lines.append(f"- `{contract}`")
            lines.append("")
        lines.append("Representative paths:")
        for path in group["sample_paths"]:
            lines.append(f"- `{path}`")
        lines.append("")

    lines.extend(
        [
            "## Best Immediate Hosts For The Future Debug Executable",
            "",
        ]
    )

    for host in payload["top_host_candidates"]:
        lines.append(f"- `{host['relative_source_path']}`: {host['reason']}")

    lines.extend(
        [
            "",
            "## What This Changes",
            "",
            "- The project now has a tighter answer for where the future debug executable should live first: menu/stage debug game modes plus cutscene-preview tools, not the entire system shell at once.",
            "- The widened path seed is no longer just a count exercise; several shell-level files now have plausible ownership targets and, in some cases, current contract aliases already usable in the selector.",
            "- The unresolved frontier is clearer: application/world shell ownership and the broader debug/editor surface still need deeper translated cleanup before they become named runtime screens.",
        ]
    )

    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a tighter recovery map for the frontend shell and debug-host layer.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument(
        "--input",
        default="research_uiux/data/ui_source_path_manifest.json",
        help="Tracked source-path manifest.",
    )
    parser.add_argument(
        "--output-json",
        default="research_uiux/data/frontend_shell_recovery.json",
        help="Destination JSON output.",
    )
    parser.add_argument(
        "--output-md",
        default="research_uiux/FRONTEND_SHELL_AND_DEBUG_HOST_RECOVERY.md",
        help="Destination markdown report.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    input_path = (repo_root / args.input).resolve()
    output_json = (repo_root / args.output_json).resolve()
    output_md = (repo_root / args.output_md).resolve()

    payload = build_payload(read_json(input_path))
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    write_markdown(payload, output_md)
    print(
        "built_frontend_shell_recovery",
        f"groups={payload['summary']['group_count']}",
        f"targeted_paths={payload['summary']['targeted_path_count']}",
        f"bridged_paths={payload['summary']['bridged_path_count']}",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
