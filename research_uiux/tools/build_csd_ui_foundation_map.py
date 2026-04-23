#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


DIRECT_SEED_PATHS = [
    "CSD/CsdPlatformMirage.cpp",
    "CSD/CsdProject.cpp",
    "CSD/CsdTexListMirage.cpp",
    "CSD/GameObjectCSD.cpp",
    "Menu/MenuTextBox.cpp",
]

CONSUMER_PATHS = [
    "HUD/GeneralWindow/GeneralWindow.cpp",
    "HUD/HelpWindow/HelpWindow.cpp",
    "Sequence/Unit/SequenceUnitCallHelpWindow.cpp",
    "Town/ShopWindow.cpp",
    "Town/TalkWindow.cpp",
]

MIRRORED_SUPPORT_PATHS = [
    "_inferred/relative_source/source/Core/csdAccess.cpp",
    "_inferred/relative_source/source/Core/csdLoader.cpp",
    "_inferred/relative_source/source/Core/csdMatrix.cpp",
    "_inferred/relative_source/source/Core/csdMotionPalette.cpp",
    "_inferred/relative_source/source/Manager/csdmMotionPalette.cpp",
    "_inferred/relative_source/source/Manager/csdmMotionPattern.cpp",
    "_inferred/relative_source/source/Manager/csdmNode.cpp",
    "_inferred/relative_source/source/Manager/csdmProject.cpp",
    "_inferred/relative_source/source/Manager/csdmScene.cpp",
    "_inferred/relative_include/include/Manager/csdmProject.h",
    "_inferred/relative_include/include/Manager/csdmRCObject.h",
    "_inferred/relative_library/library/CSD/include/Manager/csdmRCObject.h",
]

LAYOUT_IDS = [
    "ui_balloon",
    "ui_general",
    "ui_help",
    "ui_mainmenu",
    "ui_mediaroom",
    "ui_pause",
    "ui_shop",
    "ui_status",
    "ui_townscreen",
    "ui_worldmap",
    "ui_worldmap_help",
]

LAYOUT_CONSUMERS = {
    "ui_balloon": ["town_ui"],
    "ui_general": ["pause_stack", "status_overlay"],
    "ui_help": ["pause_stack"],
    "ui_mainmenu": ["title_menu"],
    "ui_mediaroom": ["town_ui"],
    "ui_pause": ["pause_stack"],
    "ui_shop": ["town_ui"],
    "ui_status": ["status_overlay"],
    "ui_townscreen": ["town_ui"],
    "ui_worldmap": ["world_map_stack"],
    "ui_worldmap_help": ["world_map_stack", "pause_stack"],
}

SYSTEM_DISPLAY = {
    "title_menu": "Title Menu",
    "pause_stack": "Pause Stack",
    "status_overlay": "Status Overlay",
    "world_map_stack": "World Map Stack",
    "town_ui": "Town UI",
}

HOOK_DEFINITIONS = [
    {
        "hook_id": "make_csd_project_midasm",
        "kind": "midasm_hook",
        "file": "UnleashedRecompLib/config/SWA.toml",
        "needle": 'name = "MakeCsdProjectMidAsmHook"',
        "symbol": "MakeCsdProjectMidAsmHook",
        "address": "0x825E4120",
        "purpose": "Intercepts `CCsdProject::Make` mid-construction so layout names and project payloads can be inspected and patched safely.",
    },
    {
        "hook_id": "resident_csd_project_make",
        "kind": "patch_wrapper",
        "file": "UnleashedRecomp/patches/resident_patches.cpp",
        "needle": "// SWA::CCsdProject::Make",
        "symbol": "sub_825E4068",
        "original_symbol": "SWA::CCsdProject::Make",
        "purpose": "Resident patch point over project creation, currently used to rewrite `ui_loading.yncp` behavior without changing the authored CSD package format.",
    },
    {
        "hook_id": "csd_platform_draw",
        "kind": "patch_wrapper",
        "file": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
        "needle": "// SWA::CCsdPlatformMirage::Draw",
        "symbol": "sub_825E2E70",
        "original_symbol": "SWA::CCsdPlatformMirage::Draw",
        "purpose": "Primary render bridge for CSD layout projection/scaling. This is the recurring seam that shows up across title, pause, world map, town, mission, and save/load families.",
    },
    {
        "hook_id": "csd_platform_draw_no_tex",
        "kind": "patch_wrapper",
        "file": "UnleashedRecomp/patches/aspect_ratio_patches.cpp",
        "needle": "// SWA::CCsdPlatformMirage::DrawNoTex",
        "symbol": "sub_825E2E88",
        "original_symbol": "SWA::CCsdPlatformMirage::DrawNoTex",
        "purpose": "No-texture variant of the same CSD platform bridge, still part of the same scene/render abstraction family.",
    },
    {
        "hook_id": "help_window_request",
        "kind": "patch_wrapper",
        "file": "UnleashedRecomp/patches/misc_patches.cpp",
        "needle": "// SWA::CHelpWindow::MsgRequestHelp::Impl",
        "symbol": "sub_824C1E60",
        "original_symbol": "SWA::CHelpWindow::MsgRequestHelp::Impl",
        "purpose": "Help-window dispatch/filter seam that proves the generic help shell is a real routed UI service rather than a one-off screen decoration.",
    },
]

MODERN_BRIDGES = [
    {
        "bridge_id": "general_window_texture_shell",
        "file": "UnleashedRecomp/ui/imgui_utils.cpp",
        "needle": "g_texGeneralWindow = LOAD_ZSTD_TEXTURE(g_general_window);",
        "symbol": "g_texGeneralWindow",
        "purpose": "Modern reusable nine-slice shell that mirrors the original `ui_general` / `ui_pause` framing language.",
    },
    {
        "bridge_id": "general_window_selection_state",
        "file": "UnleashedRecomp/patches/CTitleStateMenu_patches.cpp",
        "needle": "m_pGeneralWindow->m_SelectedIndex == 1",
        "symbol": "m_pGeneralWindow->m_SelectedIndex",
        "purpose": "Readable menu-state bridge showing the generic window shell participates in title-menu confirmation routing, not only pause/status flows.",
    },
]


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def normalize_source_path(raw: str) -> str | None:
    line = raw.strip().strip('"')
    if not line or line.startswith("#"):
        return None

    normalized = line.replace("\\", "/")
    lower = normalized.lower()
    anchor = "/swa/source/"
    if anchor in lower:
        relative = normalized[lower.index(anchor) + len(anchor) :]
    elif "/source/" in lower:
        relative = normalized[lower.index("/source/") + len("/source/") :]
    else:
        relative = normalized

    return relative.lstrip("/")


def relpath(repo_root: Path, path: Path | str | None) -> str | None:
    if path is None:
        return None
    value = Path(path)
    try:
        return value.resolve().relative_to(repo_root.resolve()).as_posix()
    except Exception:
        return str(path).replace("\\", "/")


def find_line(repo_root: Path, relative_file: str, needle: str) -> int | None:
    path = repo_root / relative_file
    if not path.exists():
        return None
    for number, line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), start=1):
        if needle in line:
            return number
    return None


def load_source_index(source_dump: Path) -> dict[str, list[str]]:
    index: dict[str, list[str]] = {}
    for raw in source_dump.read_text(encoding="utf-8-sig").splitlines():
        relative = normalize_source_path(raw)
        if relative is None:
            continue
        index.setdefault(relative.replace("\\", "/"), []).append(raw.strip())
    return index


def select_paths(source_index: dict[str, list[str]], relative_paths: list[str]) -> list[dict]:
    selected = []
    for relative_path in relative_paths:
        hits = source_index.get(relative_path, [])
        selected.append(
            {
                "relative_source_path": relative_path,
                "source_path_hits": hits,
                "source_path_hit_count": len(hits),
            }
        )
    return selected


def load_mirrored_support_entries(source_manifest: Path) -> list[dict]:
    manifest = read_json(source_manifest)
    entries = manifest.get("entries", [])
    selected = []
    seen = set()
    for relative_path_target in MIRRORED_SUPPORT_PATHS:
        for entry in entries:
            relative_path = entry.get("relative_path", "")
            if relative_path != relative_path_target:
                continue
            if relative_path in seen:
                continue
            seen.add(relative_path)
            selected.append(
                {
                    "relative_path": relative_path,
                    "bucket": entry.get("bucket"),
                    "confidence": entry.get("confidence"),
                    "raw_path": entry.get("raw_path"),
                    "local_parent": entry.get("local_parent"),
                }
            )
            break
    return selected


def load_layout_catalog(layout_correlation: Path, layout_deep_analysis: Path) -> list[dict]:
    correlation_payload = read_json(layout_correlation)
    deep_payload = read_json(layout_deep_analysis)
    deep_by_path = {
        entry.get("path", "").replace("\\", "/").lower(): entry
        for entry in deep_payload.get("digests", [])
    }

    selected = []
    for layout_id in LAYOUT_IDS:
        correlation_entry = next(
            (entry for entry in correlation_payload.get("entries", []) if entry.get("layout_id") == layout_id),
            None,
        )
        if correlation_entry is None:
            continue
        deep_entry = deep_by_path.get(correlation_entry.get("path", "").replace("\\", "/").lower())
        matches = correlation_entry.get("matches", [])
        selected.append(
            {
                "layout_id": layout_id,
                "role": correlation_entry.get("role"),
                "verdict": correlation_entry.get("verdict"),
                "path": correlation_entry.get("path"),
                "match_count": correlation_entry.get("match_count", 0),
                "first_match": matches[0] if matches else None,
                "root_scene_names": (deep_entry or {}).get("root_scene_names", []),
                "animation_family_counts": (deep_entry or {}).get("animation_family_counts", {}),
                "deepest_scene_names": [item.get("scene_name") for item in (deep_entry or {}).get("deepest_scenes", [])[:5]],
                "texture_count": len((deep_entry or {}).get("texture_names", [])),
                "consumer_system_ids": LAYOUT_CONSUMERS.get(layout_id, []),
                "consumer_system_names": [SYSTEM_DISPLAY.get(system_id, system_id) for system_id in LAYOUT_CONSUMERS.get(layout_id, [])],
            }
        )
    return selected


def load_translated_seams(ppc_labels: Path, seam_symbols: set[str]) -> list[dict]:
    payload = read_json(ppc_labels)
    collected: dict[str, dict] = {}
    for system in payload.get("systems", []):
        system_id = system.get("system_id")
        for label in system.get("labels", []):
            symbol = label.get("symbol")
            if symbol not in seam_symbols:
                continue
            entry = collected.setdefault(
                symbol,
                {
                    "symbol": symbol,
                    "role_families": set(),
                    "systems": set(),
                    "files": set(),
                },
            )
            if label.get("role_family"):
                entry["role_families"].add(label["role_family"])
            if system_id:
                entry["systems"].add(system_id)
            if label.get("file"):
                entry["files"].add(label["file"])

    output = []
    for symbol in sorted(collected):
        entry = collected[symbol]
        output.append(
            {
                "symbol": symbol,
                "role_families": sorted(entry["role_families"]),
                "systems": sorted(entry["systems"]),
                "files": sorted(entry["files"]),
            }
        )
    return output


def build_hook_entries(repo_root: Path) -> tuple[list[dict], list[dict]]:
    hook_entries = []
    for hook in HOOK_DEFINITIONS:
        hook_entries.append(
            {
                **hook,
                "line": find_line(repo_root, hook["file"], hook["needle"]),
            }
        )

    modern_entries = []
    for bridge in MODERN_BRIDGES:
        modern_entries.append(
            {
                **bridge,
                "line": find_line(repo_root, bridge["file"], bridge["needle"]),
            }
        )
    return hook_entries, modern_entries


def build_payload(
    repo_root: Path,
    source_dump: Path,
    source_manifest: Path,
    layout_correlation: Path,
    layout_deep_analysis: Path,
    ppc_labels: Path,
) -> dict:
    source_index = load_source_index(source_dump)
    direct_seed_entries = select_paths(source_index, DIRECT_SEED_PATHS)
    consumer_entries = select_paths(source_index, CONSUMER_PATHS)
    mirrored_support_entries = load_mirrored_support_entries(source_manifest)
    layout_catalog = load_layout_catalog(layout_correlation, layout_deep_analysis)
    hook_entries, modern_entries = build_hook_entries(repo_root)
    translated_seams = load_translated_seams(
        ppc_labels,
        {hook["symbol"] for hook in HOOK_DEFINITIONS},
    )

    abstractions = [
        {
            "abstraction_id": "csd_project_pipeline",
            "name": "CSD Project Pipeline",
            "responsibility": "Project creation, scene/node ownership, mirage render bridging, and low-level motion/palette graph support for authored CSD layouts.",
            "direct_source_paths": [entry for entry in direct_seed_entries if entry["relative_source_path"] != "Menu/MenuTextBox.cpp"],
            "mirrored_support_paths": mirrored_support_entries,
            "consumer_source_paths": [],
            "layout_ids": [layout["layout_id"] for layout in layout_catalog if layout["layout_id"] != "ui_help"],
            "consumer_system_ids": ["title_menu", "pause_stack", "status_overlay", "world_map_stack", "town_ui"],
            "consumer_system_names": [SYSTEM_DISPLAY[system_id] for system_id in ["title_menu", "pause_stack", "status_overlay", "world_map_stack", "town_ui"]],
            "bridge_hook_ids": ["make_csd_project_midasm", "resident_csd_project_make", "csd_platform_draw", "csd_platform_draw_no_tex"],
        },
        {
            "abstraction_id": "framed_window_widget_stack",
            "name": "Framed Window / Help Widget Stack",
            "responsibility": "Reusable darkened background, framed content window, header/footer prompt rails, and routed help-request handling.",
            "direct_source_paths": [entry for entry in direct_seed_entries if entry["relative_source_path"] == "Menu/MenuTextBox.cpp"],
            "mirrored_support_paths": [],
            "consumer_source_paths": [entry for entry in consumer_entries if entry["relative_source_path"] in {
                "HUD/GeneralWindow/GeneralWindow.cpp",
                "HUD/HelpWindow/HelpWindow.cpp",
                "Sequence/Unit/SequenceUnitCallHelpWindow.cpp",
            }],
            "layout_ids": ["ui_general", "ui_help", "ui_pause", "ui_status", "ui_worldmap_help"],
            "consumer_system_ids": ["pause_stack", "status_overlay", "world_map_stack"],
            "consumer_system_names": [SYSTEM_DISPLAY[system_id] for system_id in ["pause_stack", "status_overlay", "world_map_stack"]],
            "bridge_hook_ids": ["help_window_request"],
            "modern_bridge_ids": ["general_window_texture_shell", "general_window_selection_state"],
        },
        {
            "abstraction_id": "town_dialog_widgets",
            "name": "Town Dialog / Shop Widgets",
            "responsibility": "Balloon, talk, shop, and Media Room shell widgets layered on top of the same CSD project/render foundation.",
            "direct_source_paths": [],
            "mirrored_support_paths": [],
            "consumer_source_paths": [entry for entry in consumer_entries if entry["relative_source_path"] in {
                "Town/ShopWindow.cpp",
                "Town/TalkWindow.cpp",
            }],
            "layout_ids": ["ui_balloon", "ui_shop", "ui_townscreen", "ui_mediaroom"],
            "consumer_system_ids": ["town_ui"],
            "consumer_system_names": [SYSTEM_DISPLAY["town_ui"]],
            "bridge_hook_ids": ["csd_platform_draw", "csd_platform_draw_no_tex"],
        },
    ]

    direct_or_strong_layout_count = sum(1 for layout in layout_catalog if layout["verdict"] in {"direct", "strong"})
    foundation_system = {
        "system_id": "csd_ui_foundation",
        "screen_name": "CSD / UI Foundation",
        "layout_ids": [layout["layout_id"] for layout in layout_catalog],
        "host_code_files": [entry["relative_source_path"] for entry in direct_seed_entries + consumer_entries],
        "generated_seams": translated_seams,
        "state_tags": [
            "project_make",
            "scene_render",
            "texture_binding",
            "window_shell",
            "help_request",
            "text_box",
            "dialog_widget",
            "footer_prompt_row",
        ],
        "notes": "Humanized foundation layer for authored CSD projects, generic framed window shells, and town/dialog widgets that sit underneath the title, pause, status, world-map, and town menu families.",
    }

    summary = {
        "direct_seed_path_count": len(direct_seed_entries),
        "consumer_path_count": len(consumer_entries),
        "mirrored_support_path_count": len(mirrored_support_entries),
        "layout_family_count": len(layout_catalog),
        "direct_or_strong_layout_family_count": direct_or_strong_layout_count,
        "contextual_layout_family_count": len(layout_catalog) - direct_or_strong_layout_count,
        "foundation_hook_count": len(hook_entries),
        "modern_bridge_count": len(modern_entries),
        "translated_seam_count": len(translated_seams),
        "abstraction_count": len(abstractions),
    }

    return {
        "generated_at_repo_root": repo_root.as_posix(),
        "source_inputs": {
            "source_path_dump": relpath(repo_root, source_dump),
            "source_tree_manifest": relpath(repo_root, source_manifest),
            "layout_code_correlation": relpath(repo_root, layout_correlation),
            "layout_deep_analysis": relpath(repo_root, layout_deep_analysis),
            "ppc_ui_state_labels": relpath(repo_root, ppc_labels),
        },
        "summary": summary,
        "foundation_system": foundation_system,
        "direct_seed_paths": direct_seed_entries,
        "consumer_paths": consumer_entries,
        "mirrored_support_paths": mirrored_support_entries,
        "layout_catalog": layout_catalog,
        "bridge_hooks": hook_entries,
        "modern_bridges": modern_entries,
        "translated_seams": translated_seams,
        "abstractions": abstractions,
    }


def write_markdown(payload: dict, output_path: Path) -> None:
    summary = payload["summary"]
    foundation = payload["foundation_system"]
    lines = [
        '<p align="right">',
        '    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>',
        "</p>",
        "",
        '# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> CSD / UI Foundation Humanization',
        "",
        "Phase 31 closes the next naming gap after the gameplay-HUD pass: the lower-level CSD/UI foundation that sits under title, pause, status, town, and world-map presentation.",
        "",
        "> [!IMPORTANT]",
        "> This report does not claim the translated code is now clean original source. It maps the original source-family scaffold and the reusable scene/widget abstractions that are now defensible from the local evidence.",
        "",
        "## Snapshot",
        "",
        f"- Direct `CSD/*` / `Menu/*` seed paths: `{summary['direct_seed_path_count']}`",
        f"- Closely related consumer/widget paths: `{summary['consumer_path_count']}`",
        f"- Mirrored support paths under local-only `SONIC UNLEASHED/`: `{summary['mirrored_support_path_count']}`",
        f"- Layout families tied into the foundation map: `{summary['layout_family_count']}`",
        f"- Direct or strong layout families: `{summary['direct_or_strong_layout_family_count']}`",
        f"- Contextual-only layout families: `{summary['contextual_layout_family_count']}`",
        f"- Foundation hooks / seam anchors: `{summary['foundation_hook_count']}`",
        f"- Modern readable bridge points: `{summary['modern_bridge_count']}`",
        f"- Translated seam symbols grouped into the foundation layer: `{summary['translated_seam_count']}`",
        "",
        "## Local-Only Mirror Scaffold",
        "",
        "The local-only mirror under `SONIC UNLEASHED/` is now explicitly carrying the original-family scaffold for:",
        "",
    ]

    for entry in payload["direct_seed_paths"]:
        lines.append(f"- `{entry['relative_source_path']}`")

    lines.extend(["", "Mirrored support paths pulled from the root dump include:", ""])
    for entry in payload["mirrored_support_paths"]:
        lines.append(f"- `{entry['relative_path']}` (`{entry['bucket']}`, `{entry['confidence']}`)")

    lines.extend(["", "## Reusable Abstractions", ""])
    for abstraction in payload["abstractions"]:
        lines.append(f"### {abstraction['name']}")
        lines.append("")
        lines.append(abstraction["responsibility"])
        lines.append("")
        if abstraction["direct_source_paths"]:
            lines.append("Direct source paths:")
            lines.append("")
            for entry in abstraction["direct_source_paths"]:
                lines.append(f"- `{entry['relative_source_path']}`")
            lines.append("")
        if abstraction["consumer_source_paths"]:
            lines.append("Consumer/widget paths:")
            lines.append("")
            for entry in abstraction["consumer_source_paths"]:
                lines.append(f"- `{entry['relative_source_path']}`")
            lines.append("")
        if abstraction["mirrored_support_paths"]:
            lines.append("Mirrored support layer:")
            lines.append("")
            for entry in abstraction["mirrored_support_paths"]:
                lines.append(f"- `{entry['relative_path']}`")
            lines.append("")
        lines.append(f"Layout bridge: {', '.join(f'`{layout_id}`' for layout_id in abstraction['layout_ids'])}")
        lines.append("")
        lines.append(f"Current consumer systems: {', '.join(f'`{name}`' for name in abstraction['consumer_system_names'])}")
        lines.append("")

    lines.extend(["## Layout Bridge", "", "| Layout | Role | Verdict | Consumer systems |", "|---|---|---|---|"])
    for layout in payload["layout_catalog"]:
        consumers = ", ".join(f"`{name}`" for name in layout["consumer_system_names"]) if layout["consumer_system_names"] else "None"
        lines.append(f"| `{layout['layout_id']}` | `{layout['role']}` | `{layout['verdict']}` | {consumers} |")

    lines.extend(["", "## Hook And Seam Anchors", ""])
    for hook in payload["bridge_hooks"]:
        symbol = hook.get("original_symbol", hook["symbol"])
        lines.append(f"- `{symbol}` at [`{hook['file']}`](./{hook['file']}):{hook['line'] or 1}")
        lines.append(f"  Purpose: {hook['purpose']}")
    lines.append("")
    lines.append("Modern readable bridge points:")
    lines.append("")
    for bridge in payload["modern_bridges"]:
        lines.append(f"- `{bridge['symbol']}` at [`{bridge['file']}`](./{bridge['file']}):{bridge['line'] or 1}")
        lines.append(f"  Purpose: {bridge['purpose']}")

    lines.extend(["", "Translated seam reuse across the active archaeology layer:", ""])
    for seam in payload["translated_seams"]:
        systems = ", ".join(f"`{system}`" for system in seam["systems"]) if seam["systems"] else "None"
        roles = ", ".join(f"`{role}`" for role in seam["role_families"]) if seam["role_families"] else "None"
        lines.append(f"- `{seam['symbol']}` -> roles {roles}; systems {systems}")

    lines.extend(
        [
            "",
            "## What Changed In Practice",
            "",
            f"- The path family `{foundation['screen_name']}` is now a first-class mapped system instead of a named-only bucket.",
            "- The local-only source-tree mirror has a stable destination for future translated-file cleanup under the original 2008-style source-family layout.",
            "- The reusable foundation below title/pause/status/world-map/town UI is now described as project pipeline, window/help shell, and town dialog widgets instead of a loose pile of `sub_XXXXXXXX` seams.",
            "",
            "## Remaining Gap",
            "",
            "- None of this means the translated C++ is already humanized into those folders yet.",
            "- The foundation is mapped and named, but most of it is still not runtime-contract-backed like the current selector/workbench families.",
            "- The next value is to place renamed translated findings into the local-only `SONIC UNLEASHED/` scaffold and then widen host coverage around those named families.",
        ]
    )

    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a machine-readable and human-readable map for the CSD / UI foundation layer.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument("--source-dump", default="Match SU OG source code folders and locations.txt", help="Supplied source-path dump.")
    parser.add_argument("--source-manifest", default="SONIC UNLEASHED/_meta/source_tree_manifest.json", help="Local-only source tree manifest.")
    parser.add_argument("--layout-correlation", default="research_uiux/data/layout_code_correlation.json", help="Layout correlation JSON.")
    parser.add_argument("--layout-deep-analysis", default="research_uiux/data/layout_deep_analysis.json", help="Deep layout analysis JSON.")
    parser.add_argument("--ppc-labels", default="research_uiux/data/ppc_ui_state_labels.json", help="Translated seam/state label JSON.")
    parser.add_argument("--output-json", default="research_uiux/data/csd_ui_foundation_map.json", help="Destination JSON map.")
    parser.add_argument("--output-md", default="research_uiux/CSD_UI_FOUNDATION_HUMANIZATION.md", help="Destination Markdown report.")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    source_dump = (repo_root / args.source_dump).resolve()
    source_manifest = (repo_root / args.source_manifest).resolve()
    layout_correlation = (repo_root / args.layout_correlation).resolve()
    layout_deep_analysis = (repo_root / args.layout_deep_analysis).resolve()
    ppc_labels = (repo_root / args.ppc_labels).resolve()
    output_json = (repo_root / args.output_json).resolve()
    output_md = (repo_root / args.output_md).resolve()

    payload = build_payload(
        repo_root=repo_root,
        source_dump=source_dump,
        source_manifest=source_manifest,
        layout_correlation=layout_correlation,
        layout_deep_analysis=layout_deep_analysis,
        ppc_labels=ppc_labels,
    )
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    write_markdown(payload, output_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
