#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


MENU_DEBUG_PATHS = [
    "System/GameMode/GameModeMainMenu_Test.cpp",
    "System/GameMode/GameModeMenuSelectDebug.cpp",
    "System/GameMode/GameModeStageSelectDebug.cpp",
]

CUTSCENE_HOST_PATHS = [
    "Tool/InspirePreview/InspirePreview.cpp",
    "Tool/InspirePreview/InspirePreviewMenu.cpp",
    "Tool/InspirePreview/InspireObject.cpp",
    "Tool/InspirePreview2nd/InspirePreview2nd.cpp",
    "Tool/InspirePreview2nd/InspirePreview2ndMenu.cpp",
    "System/GameMode/GameModeStageMovie.cpp",
    "Movie/MovieManager.cpp",
    "Sequence/Unit/SequenceUnitPlayMovie.cpp",
    "Sequence/Unit/SequenceUnitMicroSequence.cpp",
    "Sequence/Utility/SequencePlayMovieWrapper.cpp",
]

TARGET_RELATIVE_PATHS = MENU_DEBUG_PATHS + CUTSCENE_HOST_PATHS


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


def build_menu_debug_targets() -> list[tuple[str, str, str, str]]:
    return [
        ("Title Menu", "System/GameMode/Title/TitleMenu.cpp", "title_menu_reference.json", "Main frontend landing and title/menu ownership."),
        ("Pause Stack", "HUD/Pause/HudPause.cpp", "pause_menu_reference.json", "Shared pause/help window stack used during debug browsing."),
        ("Loading And Start", "System/GameMode/GameModeBoot.cpp", "loading_transition_reference.json", "Boot/logo/install/loading transitions."),
        ("Mission Result", "HUD/Common/Result/HudResult.cpp", "mission_result_reference.json", "Result and failure windows."),
        ("World Map", "System/GameMode/WorldMap/WorldMapSelect.cpp", "world_map_reference.json", "World map browser and mission pane stack."),
        ("Save And Ending", "System/GameMode/Ending/EndingManager.cpp", "autosave_toast_reference.json", "Autosave, ending image/text, and clear-flag flow."),
        ("Subtitle / Cutscene", "Tool/InspirePreview/InspirePreview.cpp", "subtitle_cutscene_reference.json", "Preview-hosted cutscene/subtitle playback."),
    ]


def build_cutscene_scene_rows(subtitle_payload: dict[str, Any]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for scene in subtitle_payload.get("correlated_scenes", [])[:9]:
        rows.append(
            {
                "scene_id": scene["scene_id"],
                "announce_before_prepare": bool(scene.get("announces_before_prepare", False)),
                "keep_movie_until_stage_change": bool(scene.get("keeps_movie_until_stage_change", False)),
                "needs_loading_after": bool(scene.get("needs_loading_after", False)),
                "hide_layers": ", ".join(scene.get("hide_layers", [])) or "none",
            }
        )
    return rows


def render_menu_debug_source(relative_source_path: str, entry: dict[str, Any]) -> str:
    lines = make_header(
        relative_source_path,
        entry,
        [
            "research_uiux/data/frontend_shell_recovery.json",
            "research_uiux/data/ui_source_path_manifest.json",
            "research_uiux/runtime_reference/contracts/*.json",
        ],
    )
    lines.extend(
        [
            "struct RecoveredDebugScreenTarget",
            "{",
            "    std::string_view label;",
            "    std::string_view sourcePath;",
            "    std::string_view contractFile;",
            "    std::string_view notes;",
            "};",
            "",
            f"struct {Path(relative_source_path).stem}Host",
            "{",
            f'    static constexpr std::string_view kRecoveredSourcePath = "{cpp_string(relative_source_path)}";',
            '    static constexpr std::string_view kDefaultLaunchToken = "TitleMenu.cpp";',
            "",
            "    [[nodiscard]] static constexpr auto BuildTargets()",
            "    {",
            "        return std::array<RecoveredDebugScreenTarget, 7>{",
        ]
    )
    targets = build_menu_debug_targets()
    for index, (label, source_path, contract_file, notes) in enumerate(targets):
        lines.append("                RecoveredDebugScreenTarget{")
        lines.append(f'                    "{cpp_string(label)}",')
        lines.append(f'                    "{cpp_string(source_path)}",')
        lines.append(f'                    "{cpp_string(contract_file)}",')
        lines.append(f'                    "{cpp_string(notes)}",')
        lines.append("                }" + ("," if index + 1 != len(targets) else ""))
    lines.extend(
        [
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def render_inspire_preview_source(relative_source_path: str, entry: dict[str, Any], subtitle_payload: dict[str, Any]) -> str:
    lines = make_header(
        relative_source_path,
        entry,
        [
            "research_uiux/data/subtitle_cutscene_presentation.json",
            "Research SU.txt",
            "research_uiux/SUBTITLE_CUTSCENE_PRESENTATION_DEEP_DIVE.md",
        ],
    )
    stem = Path(relative_source_path).stem
    rows = build_cutscene_scene_rows(subtitle_payload)
    lines.extend(
        [
            "struct PreviewSceneTarget",
            "{",
            "    std::string_view sceneId;",
            "    bool announceBeforePrepare;",
            "    bool keepMovieUntilStageChange;",
            "    bool needsLoadingAfter;",
            "    std::string_view hiddenLayers;",
            "};",
            "",
            f"struct {stem}Host",
            "{",
            '    static constexpr std::string_view kPreferredContract = "subtitle_cutscene_reference.json";',
            "    static constexpr bool kCanStayOpenAcrossMovieTransitions = true;",
            "",
            "    [[nodiscard]] static constexpr auto BuildPreviewSceneTable()",
            "    {",
            f"        return std::array<PreviewSceneTarget, {len(rows)}>{{",
        ]
    )
    for index, row in enumerate(rows):
        lines.append("            PreviewSceneTarget{")
        lines.append(f'                "{cpp_string(row["scene_id"])}",')
        lines.append(f'                {"true" if row["announce_before_prepare"] else "false"},')
        lines.append(f'                {"true" if row["keep_movie_until_stage_change"] else "false"},')
        lines.append(f'                {"true" if row["needs_loading_after"] else "false"},')
        lines.append(f'                "{cpp_string(row["hide_layers"])}",')
        lines.append("            }" + ("," if index + 1 != len(rows) else ""))
    lines.extend(
        [
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def render_inspire_preview_menu_source(relative_source_path: str, entry: dict[str, Any]) -> str:
    lines = make_header(
        relative_source_path,
        entry,
        [
            "Research SU.txt",
            "research_uiux/data/frontend_shell_recovery.json",
        ],
    )
    stem = Path(relative_source_path).stem
    lines.extend(
        [
            "struct PreviewMenuEntry",
            "{",
            "    std::string_view label;",
            "    std::string_view launchToken;",
            "    std::string_view contractFile;",
            "};",
            "",
            f"struct {stem}Host",
            "{",
            "    [[nodiscard]] static constexpr auto BuildMenuEntries()",
            "    {",
            "        return std::array<PreviewMenuEntry, 3>{",
            '            PreviewMenuEntry{ "Legacy Inspire Preview", "InspirePreview.cpp", "subtitle_cutscene_reference.json" },',
            '            PreviewMenuEntry{ "Single-Scene Preview", "InspirePreview2nd.cpp", "subtitle_cutscene_reference.json" },',
            '            PreviewMenuEntry{ "Stage Movie Wrapper", "GameModeStageMovie.cpp", "subtitle_cutscene_reference.json" },',
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def render_inspire_object_source(relative_source_path: str, entry: dict[str, Any]) -> str:
    lines = make_header(
        relative_source_path,
        entry,
        [
            "research_uiux/data/subtitle_cutscene_presentation.json",
            "research_uiux/SUBTITLE_CUTSCENE_PRESENTATION_DEEP_DIVE.md",
        ],
    )
    stem = Path(relative_source_path).stem
    lines.extend(
        [
            "struct CutsceneOverlayRole",
            "{",
            "    std::string_view id;",
            "    std::string_view role;",
            "    std::string_view notes;",
            "};",
            "",
            f"struct {stem}Host",
            "{",
            "    [[nodiscard]] static constexpr auto BuildOverlayRoles()",
            "    {",
            "        return std::array<CutsceneOverlayRole, 5>{",
            '            CutsceneOverlayRole{ "movie_surface", "content", "Primary movie layer retained through stage-change ownership." },',
            '            CutsceneOverlayRole{ "subtitle_plate", "subtitle", "Bottom-anchored timed converse-data cells." },',
            '            CutsceneOverlayRole{ "letterbox", "letterbox", "Cutscene framing controlled by CutsceneAspectRatio." },',
            '            CutsceneOverlayRole{ "loading_mask", "loading_gate", "Optional loading display after movie handoff." },',
            '            CutsceneOverlayRole{ "hide_mask", "hide_layers", "Per-scene suppression of tails / 2D / 3D / Boss / Design layers." },',
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def render_movie_route_source(relative_source_path: str, entry: dict[str, Any], subtitle_payload: dict[str, Any]) -> str:
    lines = make_header(
        relative_source_path,
        entry,
        [
            "research_uiux/data/subtitle_cutscene_presentation.json",
            "research_uiux/SUBTITLE_CUTSCENE_PRESENTATION_DEEP_DIVE.md",
        ],
    )
    stem = Path(relative_source_path).stem
    rows = build_cutscene_scene_rows(subtitle_payload)
    lines.extend(
        [
            "struct MovieRouteDescriptor",
            "{",
            "    std::string_view sceneId;",
            "    bool announceBeforePrepare;",
            "    bool keepMovieUntilStageChange;",
            "    bool needsLoadingAfter;",
            "    std::string_view hiddenLayers;",
            "};",
            "",
            f"struct {stem}Host",
            "{",
            '    static constexpr std::string_view kPreferredContract = "subtitle_cutscene_reference.json";',
            "",
            "    [[nodiscard]] static constexpr auto BuildMovieRoutes()",
            "    {",
            f"        return std::array<MovieRouteDescriptor, {len(rows)}>{{",
        ]
    )
    for index, row in enumerate(rows):
        lines.append("            MovieRouteDescriptor{")
        lines.append(f'                "{cpp_string(row["scene_id"])}",')
        lines.append(f'                {"true" if row["announce_before_prepare"] else "false"},')
        lines.append(f'                {"true" if row["keep_movie_until_stage_change"] else "false"},')
        lines.append(f'                {"true" if row["needs_loading_after"] else "false"},')
        lines.append(f'                "{cpp_string(row["hide_layers"])}",')
        lines.append("            }" + ("," if index + 1 != len(rows) else ""))
    lines.extend(
        [
            "        };",
            "    }",
            "};",
        ]
    )
    lines.extend(make_footer())
    return "\n".join(lines)


def render_source(relative_source_path: str, entry: dict[str, Any], subtitle_payload: dict[str, Any]) -> str:
    if relative_source_path in MENU_DEBUG_PATHS:
        return render_menu_debug_source(relative_source_path, entry)
    if relative_source_path in {
        "Tool/InspirePreview/InspirePreview.cpp",
        "Tool/InspirePreview2nd/InspirePreview2nd.cpp",
    }:
        return render_inspire_preview_source(relative_source_path, entry, subtitle_payload)
    if relative_source_path in {
        "Tool/InspirePreview/InspirePreviewMenu.cpp",
        "Tool/InspirePreview2nd/InspirePreview2ndMenu.cpp",
    }:
        return render_inspire_preview_menu_source(relative_source_path, entry)
    if relative_source_path == "Tool/InspirePreview/InspireObject.cpp":
        return render_inspire_object_source(relative_source_path, entry)
    return render_movie_route_source(relative_source_path, entry, subtitle_payload)


def write_tracked_markdown(payload: dict[str, Any], output_path: Path) -> None:
    summary = payload["summary"]
    lines = [
        '<p align="right">',
        '    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>',
        "</p>",
        "",
        '# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Local Named Translated Ownership',
        "",
        "Phase 36 starts replacing local-only `*.sward.md` anchors with real readable source scaffolds for the first debug/cutscene host surfaces.",
        "",
        "> [!IMPORTANT]",
        "> These local-only files are still SWARD humanization scaffolds, not a claim of recovered original authored SEGA source. They live under `SONIC UNLEASHED/` on purpose and remain out of git.",
        "",
        "## Snapshot",
        "",
        f"- Immediate host targets considered: `{summary['immediate_host_target_count']}`",
        f"- Local-only readable source files materialized: `{summary['humanized_source_file_count']}`",
        f"- Immediate-host conversion rate: `{summary['immediate_host_conversion_pct']:.1f}%`",
        f"- Representative subtitle scene rows embedded across movie/preview hosts: `{summary['representative_subtitle_scene_count']}`",
        "",
        "## Local Output Layer",
        "",
        f"- output root: `{payload['output_root']}`",
        "- file shape: `<original source path>.cpp` beside the existing `*.sward.md` notes",
        "- local meta manifest: `SONIC UNLEASHED/_meta/humanized_debug_host_sources_manifest.json`",
        "",
        "## Materialized Groups",
        "",
        "| Group | Local-only files | Purpose |",
        "|---|---:|---|",
    ]

    for group in payload["groups"]:
        lines.append(f"| {group['display_name']} | `{group['file_count']}` | {group['purpose']} |")

    lines.extend(
        [
            "",
            "## Example Local Files",
            "",
        ]
    )
    for item in payload["sample_files"]:
        lines.append(f"- `{item}`")

    lines.extend(
        [
            "",
            "## What Changed",
            "",
            "- `GameModeMenuSelectDebug.cpp` and `GameModeStageSelectDebug.cpp` now have readable local-only host tables that point at the then-current contract-backed screen families plus the new subtitle/cutscene contract.",
            "- `InspirePreview*.cpp`, `MovieManager.cpp`, `GameModeStageMovie.cpp`, and the `SequenceUnitPlayMovie` wrapper layer now carry readable local-only scene/route descriptors derived from the subtitle/cutscene presentation evidence.",
            "- The mirror is no longer only notes for these hosts; it now has real `.cpp` scaffolds that can be tightened further as translated seam naming improves.",
        ]
    )

    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def update_local_readme(readme_path: Path, payload: dict[str, Any]) -> None:
    summary = payload["summary"]
    lines = [
        "# SONIC UNLEASHED Local Mirrored Source Tree",
        "",
        "This directory is local-only and intentionally kept out of git.",
        "",
        "Purpose:",
        "- mirror the source-path dump into a stable local folder scaffold",
        "- hold local-only `*.sward.md` source-family placement notes beside the recovered original-style paths",
        "- hold local-only readable `.cpp` humanization scaffolds for the first debug/cutscene host surfaces",
        "- give future translated cleanup a destination that resembles the original source-family layout",
        "",
        "Current local layers:",
        "- note suffix pattern: `<original source path>.sward.md`",
        f"- humanized source files: {summary['humanized_source_file_count']}",
        f"- immediate-host conversion rate: {summary['immediate_host_conversion_pct']:.1f}%",
        "",
        "Generated by:",
        "- `research_uiux/tools/materialize_source_family_notes.py`",
        "- `research_uiux/tools/materialize_humanized_debug_hosts.py`",
    ]
    readme_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Materialize local-only readable source scaffolds for the first debug/cutscene hosts.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument("--source-path-manifest", default="research_uiux/data/ui_source_path_manifest.json", help="Tracked source-path manifest.")
    parser.add_argument("--source-tree-manifest", default="SONIC UNLEASHED/_meta/source_tree_manifest.json", help="Local source-tree manifest.")
    parser.add_argument("--frontend-shell-json", default="research_uiux/data/frontend_shell_recovery.json", help="Frontend shell/debug recovery JSON.")
    parser.add_argument("--subtitle-json", default="research_uiux/data/subtitle_cutscene_presentation.json", help="Subtitle/cutscene correlation JSON.")
    parser.add_argument("--output-root", default="SONIC UNLEASHED", help="Local-only mirror root.")
    parser.add_argument("--local-manifest", default="SONIC UNLEASHED/_meta/humanized_debug_host_sources_manifest.json", help="Local-only humanized source manifest.")
    parser.add_argument("--tracked-json", default="research_uiux/data/local_named_translated_ownership.json", help="Tracked summary JSON.")
    parser.add_argument("--tracked-md", default="research_uiux/LOCAL_NAMED_TRANSLATED_OWNERSHIP.md", help="Tracked summary markdown.")
    parser.add_argument("--local-readme", default="SONIC UNLEASHED/_meta/README.txt", help="Local mirror readme to refresh.")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    source_path_manifest = read_json((repo_root / args.source_path_manifest).resolve())
    source_tree_manifest = read_json((repo_root / args.source_tree_manifest).resolve())
    frontend_shell_payload = read_json((repo_root / args.frontend_shell_json).resolve())
    subtitle_payload = read_json((repo_root / args.subtitle_json).resolve())
    output_root = (repo_root / args.output_root).resolve()
    local_manifest_path = (repo_root / args.local_manifest).resolve()
    tracked_json_path = (repo_root / args.tracked_json).resolve()
    tracked_md_path = (repo_root / args.tracked_md).resolve()
    local_readme_path = (repo_root / args.local_readme).resolve()

    source_entries = {entry["relative_source_path"]: entry for entry in source_path_manifest.get("entries", [])}
    tree_lookup: dict[str, dict[str, Any]] = {}
    for item in source_tree_manifest.get("entries", []):
        normalized_path = item.get("normalized_path")
        raw_path = item.get("raw_path")
        relative_path = item.get("relative_path")
        if normalized_path:
            tree_lookup[normalize_key(normalized_path)] = item
        if raw_path:
            tree_lookup[normalize_key(raw_path)] = item
        if relative_path:
            tree_lookup[normalize_key(relative_path)] = item

    converted_files: list[str] = []
    local_jobs: list[tuple[Path, str]] = []

    for relative_source_path in TARGET_RELATIVE_PATHS:
        source_entry = source_entries.get(relative_source_path)
        tree_entry = tree_lookup.get(normalize_key(source_entry["source_path"])) if source_entry else None
        if tree_entry is None:
            tree_entry = tree_lookup.get(normalize_key(relative_source_path))
        if source_entry is None or tree_entry is None:
            continue

        scaffold_relative = tree_entry["relative_path"]
        output_path = output_root / scaffold_relative
        local_jobs.append((output_path, render_source(relative_source_path, source_entry, subtitle_payload)))
        converted_files.append(scaffold_relative.replace("\\", "/"))

    for output_path, content in local_jobs:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(content, encoding="utf-8")

    immediate_host_target_count = 0
    for group in frontend_shell_payload.get("priority_groups", []):
        if group.get("priority") == "immediate":
            immediate_host_target_count += int(group.get("path_count", 0))

    groups = [
        {
            "group_id": "menu_debug_hosts",
            "display_name": "Menu / Stage Debug Hosts",
            "file_count": len(MENU_DEBUG_PATHS),
            "purpose": "Readable local-only launch ownership for title/menu/stage debug entry points.",
        },
        {
            "group_id": "cutscene_preview_hosts",
            "display_name": "Cutscene / Preview Hosts",
            "file_count": 5,
            "purpose": "Readable local-only preview browser and menu scaffolds around InspirePreview and InspirePreview2nd.",
        },
        {
            "group_id": "movie_route_hosts",
            "display_name": "Movie / Sequence Route Hosts",
            "file_count": len(CUTSCENE_HOST_PATHS) - 5,
            "purpose": "Readable local-only descriptors for movie ownership, stage-movie handoff, and sequence wrappers.",
        },
    ]

    tracked_payload = {
        "inputs": {
            "source_path_manifest": str((repo_root / args.source_path_manifest).resolve()),
            "frontend_shell_recovery": str((repo_root / args.frontend_shell_json).resolve()),
            "subtitle_cutscene_presentation": str((repo_root / args.subtitle_json).resolve()),
        },
        "output_root": str(output_root),
        "summary": {
            "immediate_host_target_count": immediate_host_target_count,
            "humanized_source_file_count": len(local_jobs),
            "immediate_host_conversion_pct": round((len(local_jobs) / immediate_host_target_count) * 100.0, 1) if immediate_host_target_count else 0.0,
            "representative_subtitle_scene_count": len(build_cutscene_scene_rows(subtitle_payload)),
        },
        "groups": groups,
        "sample_files": converted_files[:12],
    }

    local_manifest = {
        "summary": tracked_payload["summary"],
        "groups": groups,
        "files": converted_files,
    }

    local_manifest_path.parent.mkdir(parents=True, exist_ok=True)
    local_manifest_path.write_text(json.dumps(local_manifest, indent=2) + "\n", encoding="utf-8")
    local_readme_path.parent.mkdir(parents=True, exist_ok=True)
    update_local_readme(local_readme_path, tracked_payload)
    tracked_json_path.parent.mkdir(parents=True, exist_ok=True)
    tracked_json_path.write_text(json.dumps(tracked_payload, indent=2) + "\n", encoding="utf-8")
    tracked_md_path.parent.mkdir(parents=True, exist_ok=True)
    write_tracked_markdown(tracked_payload, tracked_md_path)

    print(
        "materialized_humanized_debug_hosts",
        f"files={len(local_jobs)}",
        f"immediate_targets={immediate_host_target_count}",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
