#!/usr/bin/env python3
"""Build a focused inventory for real in-game UI/UX screen reconstruction."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


FOCUS_SCREENS = [
    {
        "system_id": "title_menu",
        "label": "Title / Main Menu",
        "goal": "Recover the interactive title/menu flow, not pre-rendered SFDs.",
        "extra_projects": ["ui_title"],
        "native_focus": [
            "UnleashedRecomp/patches/CTitleStateIntro_patches.cpp",
            "UnleashedRecomp/patches/CTitleStateMenu_patches.cpp",
        ],
    },
    {
        "system_id": "loading_and_start",
        "label": "Loading / Start / Clear",
        "goal": "Recover the authored transition screens and stage start/clear overlays.",
        "extra_projects": [],
        "native_focus": ["UnleashedRecomp/patches/resident_patches.cpp"],
    },
    {
        "system_id": "town_ui",
        "label": "Hub / Town UI",
        "goal": "Recover hub-world overlays, dialog balloons, shop, and town status surfaces.",
        "extra_projects": [],
        "native_focus": ["UnleashedRecomp/patches/aspect_ratio_patches.cpp"],
    },
    {
        "system_id": "sonic_stage_hud",
        "label": "Day / Night Gameplay HUD",
        "goal": "Recover player-control HUD behavior for Sonic and Werehog/Evil Sonic stage play.",
        "extra_projects": ["ui_playscreen", "ui_lcursor", "ui_itembox"],
        "native_focus": [
            "UnleashedRecomp/patches/CHudSonicStage_patches.cpp",
            "UnleashedRecomp/patches/CGameModeStage_patches.cpp",
            "UnleashedRecomp/app.h",
        ],
    },
    {
        "system_id": "pause_stack",
        "label": "Pause / Help",
        "goal": "Recover pause-stack presentation, footer prompts, help pages, and menu state.",
        "extra_projects": [],
        "native_focus": ["UnleashedRecomp/patches/CHudPause_patches.cpp"],
    },
    {
        "system_id": "status_overlay",
        "label": "Status / Skill Upgrade",
        "goal": "Recover status and level-up overlay behavior for Sonic/Werehog progression.",
        "extra_projects": [],
        "native_focus": ["UnleashedRecomp/patches/aspect_ratio_patches.cpp"],
    },
    {
        "system_id": "world_map_stack",
        "label": "World Map",
        "goal": "Recover map select/help screens and cursor/info-panel state.",
        "extra_projects": [],
        "native_focus": ["UnleashedRecomp/patches/input_patches.cpp"],
    },
    {
        "system_id": "mission_result_family",
        "label": "Results",
        "goal": "Recover mission result screens, rankings, numbers, records, and footer flow.",
        "extra_projects": [],
        "native_focus": ["UnleashedRecomp/patches/aspect_ratio_patches.cpp"],
    },
    {
        "system_id": "item_result",
        "label": "Item Result",
        "goal": "Recover item/acquisition result overlays that differ from full mission results.",
        "extra_projects": [],
        "native_focus": ["UnleashedRecomp/patches/aspect_ratio_patches.cpp"],
    },
    {
        "system_id": "tornado_defense",
        "label": "Tornado / EX Stage HUD",
        "goal": "Recover EX/Tails/Tornado HUD and QTE-adjacent authored overlays.",
        "extra_projects": [],
        "native_focus": ["UnleashedRecomp/patches/aspect_ratio_patches.cpp"],
    },
]


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def normalize_path(path: str, repo_root: Path) -> str:
    if not path:
        return path
    text = path.replace("\\", "/")
    repo = repo_root.as_posix()
    if text.startswith(repo + "/"):
        return text[len(repo) + 1 :]
    legacy_marker = "UI-UX Sonic World Adventure for SGFX - Project Quality Hero/"
    if legacy_marker in text:
        return text.split(legacy_marker, 1)[1]
    return text


def unique(values: list[str]) -> list[str]:
    seen: set[str] = set()
    result: list[str] = []
    for value in values:
        if value and value not in seen:
            seen.add(value)
            result.append(value)
    return result


def by_key(items: list[dict[str, Any]], key: str) -> dict[str, dict[str, Any]]:
    return {str(item.get(key, "")): item for item in items if item.get(key)}


def load_deep_layout_paths(deep_analysis: dict[str, Any], repo_root: Path) -> dict[str, list[str]]:
    paths: dict[str, list[str]] = {}
    for item in deep_analysis.get("parsed_files", []):
        stem = str(item.get("stem", ""))
        path = str(item.get("path", ""))
        if stem and path:
            paths.setdefault(stem, []).append(normalize_path(path, repo_root))
    return {key: unique(value) for key, value in paths.items()}


def build_inventory(repo_root: Path) -> dict[str, Any]:
    data_dir = repo_root / "research_uiux/data"
    database = read_json(data_dir / "ui_archaeology_database.json")
    correlation = read_json(data_dir / "layout_code_correlation.json")
    runtime_index = read_json(data_dir / "runtime_bridge_screen_index.json")
    deep_analysis = read_json(data_dir / "layout_deep_analysis.json")

    systems = by_key(database.get("systems", []), "system_id")
    layouts = by_key(database.get("layouts", []), "layout_id")
    correlations = by_key(correlation.get("entries", []), "layout_id")
    runtime_rows = by_key(runtime_index.get("rows", []), "project")
    deep_paths = load_deep_layout_paths(deep_analysis, repo_root)

    screen_entries: list[dict[str, Any]] = []
    for focus in FOCUS_SCREENS:
        system_id = focus["system_id"]
        system = systems.get(system_id, {})
        projects = list(system.get("layout_ids", []))
        projects.extend(focus.get("extra_projects", []))
        projects = unique(projects)

        layout_entries: list[dict[str, Any]] = []
        source_files = list(focus.get("native_focus", []))
        generated_refs: list[dict[str, Any]] = []

        for project in projects:
            layout = layouts.get(project, {})
            corr = correlations.get(project, {})
            row = runtime_rows.get(project, {})

            paths = [
                normalize_path(path, repo_root)
                for path in layout.get("layout_files", [])
            ]
            paths.extend(normalize_path(path, repo_root) for path in corr.get("paths", []))
            paths.extend(deep_paths.get(project, []))
            paths = unique(paths)

            matches = []
            for match in corr.get("matches", [])[:6]:
                match_path = normalize_path(str(match.get("path", "")), repo_root)
                if match_path:
                    source_files.append(match_path)
                matches.append(
                    {
                        "evidence": match.get("evidence"),
                        "path": match_path,
                        "line": match.get("line"),
                        "excerpt": match.get("excerpt"),
                        "reason": match.get("reason"),
                    }
                )

            for host in layout.get("host_code_files", [])[:6]:
                host_path = normalize_path(str(host.get("path", "")), repo_root)
                if host_path:
                    source_files.append(host_path)

            for ref in corr.get("generated_refs", [])[:6]:
                normalized_ref = {
                    "symbol": ref.get("symbol"),
                    "generated_file": normalize_path(str(ref.get("generated_file", "")), repo_root),
                    "impl_line": ref.get("impl_line"),
                    "patch_files": [
                        normalize_path(str(path), repo_root)
                        for path in ref.get("patch_files", [])
                    ],
                    "readable_relationship": ref.get("readable_relationship"),
                }
                generated_refs.append(normalized_ref)
                source_files.extend(normalized_ref["patch_files"])

            layout_entries.append(
                {
                    "layout_id": project,
                    "runtime_token": row.get("token"),
                    "runtime_project_source": row.get("source_family"),
                    "runtime_data_source": row.get("data_source"),
                    "role": layout.get("role") or corr.get("role"),
                    "confidence": layout.get("confidence"),
                    "archive_groups": unique(
                        [str(value) for value in layout.get("archive_groups", [])]
                        + ([str(corr.get("archive_group"))] if corr.get("archive_group") else [])
                    ),
                    "layout_files": paths,
                    "scene_cues": layout.get("scene_cues", [])[:16],
                    "animation_cues": layout.get("animation_cues", [])[:16],
                    "root_children": layout.get("root_children", []),
                    "code_matches": matches,
                }
            )

        screen_entries.append(
            {
                "system_id": system_id,
                "label": focus["label"],
                "screen_name": system.get("screen_name", focus["label"]),
                "goal": focus["goal"],
                "runtime_layout_ids": projects,
                "source_files": unique(source_files),
                "generated_refs": generated_refs[:10],
                "layouts": layout_entries,
            }
        )

    return {
        "inputs": {
            "ui_archaeology_database": "research_uiux/data/ui_archaeology_database.json",
            "layout_code_correlation": "research_uiux/data/layout_code_correlation.json",
            "runtime_bridge_screen_index": "research_uiux/data/runtime_bridge_screen_index.json",
            "layout_deep_analysis": "research_uiux/data/layout_deep_analysis.json",
        },
        "scope_note": (
            "Focused on real interactive UI/UX surfaces. Pre-rendered SFD/cutscene-only "
            "presentation is intentionally out of scope for this inventory."
        ),
        "screens": screen_entries,
    }


def write_markdown(path: Path, inventory: dict[str, Any]) -> None:
    lines = [
        "# Real Screen Source Inventory",
        "",
        inventory["scope_note"],
        "",
        "## Summary",
        "",
        "| Screen family | Layout projects | Primary native/source focus |",
        "| --- | --- | --- |",
    ]

    for screen in inventory["screens"]:
        projects = ", ".join(f"`{project}`" for project in screen["runtime_layout_ids"])
        sources = ", ".join(f"`{source}`" for source in screen["source_files"][:4])
        lines.append(f"| {screen['label']} | {projects} | {sources} |")

    for screen in inventory["screens"]:
        lines.extend(["", f"## {screen['label']}", "", screen["goal"], ""])
        lines.append(f"- System id: `{screen['system_id']}`")
        if screen["source_files"]:
            lines.append("- Source focus:")
            for source in screen["source_files"][:12]:
                lines.append(f"  - `{source}`")
        if screen["generated_refs"]:
            lines.append("- Generated PPC/native-function references:")
            for ref in screen["generated_refs"][:8]:
                symbol = ref.get("symbol") or "unknown"
                generated_file = ref.get("generated_file") or "unknown"
                impl_line = ref.get("impl_line")
                relation = ref.get("readable_relationship") or "relationship pending"
                suffix = f":{impl_line}" if impl_line else ""
                lines.append(f"  - `{symbol}` -> `{generated_file}{suffix}` ({relation})")
        lines.append("- Layouts:")
        for layout in screen["layouts"]:
            lines.append(f"  - `{layout['layout_id']}`: {layout.get('role') or 'role pending'}")
            for layout_file in layout.get("layout_files", [])[:4]:
                lines.append(f"    - file: `{layout_file}`")
            if layout.get("scene_cues"):
                cues = ", ".join(f"`{cue}`" for cue in layout["scene_cues"][:8])
                lines.append(f"    - scene cues: {cues}")
            if layout.get("animation_cues"):
                cues = ", ".join(f"`{cue}`" for cue in layout["animation_cues"][:8])
                lines.append(f"    - animation cues: {cues}")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".")
    parser.add_argument(
        "--output-json",
        default="research_uiux/data/real_screen_source_inventory.json",
    )
    parser.add_argument(
        "--output-md",
        default="research_uiux/REAL_SCREEN_SOURCE_INVENTORY.md",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    inventory = build_inventory(repo_root)

    output_json = Path(args.output_json)
    if not output_json.is_absolute():
        output_json = repo_root / output_json
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(inventory, indent=2, sort_keys=True), encoding="utf-8")

    output_md = Path(args.output_md)
    if not output_md.is_absolute():
        output_md = repo_root / output_md
    write_markdown(output_md, inventory)

    print(output_json)
    print(output_md)
    print(f"screens={len(inventory['screens'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
