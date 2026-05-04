#!/usr/bin/env python3
"""Build the compact runtime screen index consumed by the UI lab bridge."""

from __future__ import annotations

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable


DEFAULT_ARCHEOLOGY_DATABASE = "research_uiux/data/ui_archaeology_database.json"
DEFAULT_OUTPUT_JSON = "research_uiux/data/runtime_bridge_screen_index.json"
DEFAULT_OUTPUT_HEADER = "UnleashedRecomp/patches/ui_lab_runtime_screen_index.generated.h"


SOURCE_BY_PROJECT = {
    "ui_title": "System/GameMode/Title/TitleStateIntro.cpp|TitleMenu.cpp",
    "ui_mainmenu": "System/GameMode/Title/TitleStateIntro.cpp|TitleMenu.cpp",
    "ui_loading": "System/Loading.cpp",
    "ui_start": "System/Loading.cpp",
    "ui_playscreen": "Player/Character/Sonic/Hud/SonicMainDisplay.cpp",
    "ui_prov_playscreen": "ExtraStage/Tails/Hud/HudExQte.cpp",
    "ui_pause": "HUD/Pause/HudPause.cpp",
    "ui_status": "HUD/Status/Status.cpp",
    "ui_result": "HUD/Result/Result.cpp",
    "ui_result_ex": "HUD/Result/Result.cpp",
    "ui_itemresult": "CSD/ui_itemresult runtime project (source file pending)",
    "ui_worldmap": "System/GameMode/WorldMap/WorldMapSelect.cpp",
    "ui_worldmap_help": "System/GameMode/WorldMap/WorldMapTutorial.cpp",
}


TOKEN_BY_PROJECT = {
    "ui_title": "title-runtime",
    "ui_mainmenu": "title-runtime",
    "ui_loading": "loading",
    "ui_start": "loading-start",
    "ui_playscreen": "sonic-hud",
    "ui_prov_playscreen": "extra-stage-hud",
    "ui_pause": "pause",
    "ui_status": "status",
    "ui_result": "result",
    "ui_result_ex": "result-ex",
    "ui_itemresult": "item-result",
    "ui_worldmap": "world-map",
    "ui_worldmap_help": "world-map-help",
}


LABEL_BY_PROJECT = {
    "ui_title": "Title Runtime",
    "ui_mainmenu": "Title Runtime",
    "ui_loading": "Loading / Miles Electric",
    "ui_start": "Loading Start/Clear",
    "ui_playscreen": "Sonic Stage HUD",
    "ui_prov_playscreen": "Extra Stage / Tornado HUD",
    "ui_pause": "Pause Menu",
    "ui_status": "Status / Skill Upgrade",
    "ui_result": "Stage Result",
    "ui_result_ex": "EX Stage Result",
    "ui_itemresult": "Item Result Runtime",
    "ui_worldmap": "World Map",
    "ui_worldmap_help": "World Map Help",
}


RUNTIME_ONLY_ROWS = [
    {
        "project": "ui_title",
        "token": "title-runtime",
        "label": "Title Runtime",
        "system_id": "title_menu",
        "system_name": "Title Menu",
        "source_family": SOURCE_BY_PROJECT["ui_title"],
    },
    {
        "project": "ui_playscreen",
        "token": "sonic-hud",
        "label": "Sonic Stage HUD",
        "system_id": "sonic_stage_hud",
        "system_name": "Sonic Stage HUD",
        "source_family": SOURCE_BY_PROJECT["ui_playscreen"],
    },
]


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def cpp_escape(value: object) -> str:
    text = "" if value is None else str(value)
    return text.replace("\\", "\\\\").replace('"', '\\"')


def slug_label(value: str) -> str:
    value = value.removeprefix("ui_").replace("_", " ")
    return " ".join(part[:1].upper() + part[1:] for part in value.split())


def token_for(project: str, system_id: str) -> str:
    if project in TOKEN_BY_PROJECT:
        return TOKEN_BY_PROJECT[project]
    token = project.removeprefix("ui_").replace("_", "-")
    return token or system_id.replace("_", "-")


def label_for(project: str, system_name: str) -> str:
    if project in LABEL_BY_PROJECT:
        return LABEL_BY_PROJECT[project]
    if project.startswith("ui_"):
        return slug_label(project)
    return system_name


def source_for(project: str, system_id: str) -> str:
    return SOURCE_BY_PROJECT.get(project, f"ui_archaeology_database:{system_id}")


def iter_asset_entries(asset_index: dict) -> Iterable[dict]:
    for scan in asset_index.get("scans", []):
        yield from scan.get("entries", [])
    yield from asset_index.get("entries", [])


def summarize_asset_index(asset_index: dict) -> dict:
    entries = list(iter_asset_entries(asset_index))
    root_counts: dict[str, int] = {}
    type_counts: dict[str, int] = {}
    screen_counts: dict[str, int] = {}
    for entry in entries:
        root = str(entry.get("root", ""))
        root_counts[root] = root_counts.get(root, 0) + 1
        entry_type = str(entry.get("type", "unknown"))
        type_counts[entry_type] = type_counts.get(entry_type, 0) + 1
        related_screen = str(entry.get("related_screen", "unknown"))
        screen_counts[related_screen] = screen_counts.get(related_screen, 0) + 1

    return {
        "scan_count": asset_index.get("scan_count", len(asset_index.get("scans", []))),
        "asset_roots": asset_index.get("asset_roots", []),
        "entry_count": len(entries),
        "root_counts": dict(sorted(root_counts.items())),
        "type_counts": dict(sorted(type_counts.items())),
        "related_screen_counts": dict(sorted(screen_counts.items())),
    }


def find_latest_current_asset_index(repo_root: Path) -> Path | None:
    evidence_root = repo_root / "out/ui_lab_runtime_evidence"
    if not evidence_root.exists():
        return None
    candidates = sorted(
        evidence_root.rglob("asset_index_current_install.json"),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    return candidates[0] if candidates else None


def build_rows(database: dict) -> list[dict]:
    rows: dict[str, dict] = {}

    def add_row(row: dict, data_source: str) -> None:
        project = row["project"]
        merged = {
            "project": project,
            "token": row["token"],
            "label": row["label"],
            "system_id": row["system_id"],
            "system_name": row["system_name"],
            "source_family": row["source_family"],
            "data_source": data_source,
        }
        rows.setdefault(project, merged)

    for system in database.get("systems", []):
        system_id = system.get("system_id", "")
        system_name = system.get("screen_name", system_id)
        data_source = f"ui_archaeology_database:{system_id}"
        for project in system.get("layout_ids", []):
            add_row(
                {
                    "project": project,
                    "token": token_for(project, system_id),
                    "label": label_for(project, system_name),
                    "system_id": system_id,
                    "system_name": system_name,
                    "source_family": source_for(project, system_id),
                },
                data_source,
            )

    for row in RUNTIME_ONLY_ROWS:
        add_row(row, "runtime-live-evidence-overlay")

    return sorted(rows.values(), key=lambda row: (row["system_id"], row["project"]))


def write_header(path: Path, payload: dict) -> None:
    rows = payload["rows"]
    lines = [
        "#pragma once",
        "",
        "#include <array>",
        "#include <cstddef>",
        "#include <string_view>",
        "",
        "namespace UiLab::GeneratedRuntimeScreenIndex",
        "{",
        "    struct Row",
        "    {",
        "        std::string_view project;",
        "        std::string_view token;",
        "        std::string_view label;",
        "        std::string_view systemId;",
        "        std::string_view systemName;",
        "        std::string_view sourceFamily;",
        "        std::string_view dataSource;",
        "    };",
        "",
        f'    static constexpr std::string_view kGeneratedAt = "{cpp_escape(payload["generated_at"])}";',
        f'    static constexpr std::string_view kAssetIndexPath = "{cpp_escape(payload["inputs"].get("asset_index", ""))}";',
        f'    static constexpr std::string_view kDatabasePath = "{cpp_escape(payload["inputs"].get("ui_archaeology_database", ""))}";',
        f'    static constexpr size_t kAssetEntryCount = {payload["asset_summary"].get("entry_count", 0)};',
        "",
        f"    static constexpr std::array<Row, {len(rows)}> kRows =",
        "    {{",
    ]

    for row in rows:
        lines.append(
            '        { "'
            + cpp_escape(row["project"])
            + '", "'
            + cpp_escape(row["token"])
            + '", "'
            + cpp_escape(row["label"])
            + '", "'
            + cpp_escape(row["system_id"])
            + '", "'
            + cpp_escape(row["system_name"])
            + '", "'
            + cpp_escape(row["source_family"])
            + '", "'
            + cpp_escape(row["data_source"])
            + '" },'
        )

    lines.extend(["    }};", "}", ""])
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument("--ui-archaeology-database", default=DEFAULT_ARCHEOLOGY_DATABASE)
    parser.add_argument("--asset-index", default=None)
    parser.add_argument("--output-json", default=DEFAULT_OUTPUT_JSON)
    parser.add_argument("--output-header", default=DEFAULT_OUTPUT_HEADER)
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()

    def resolve(value: str | None) -> Path | None:
        if not value:
            return None
        path = Path(value)
        return path if path.is_absolute() else repo_root / path

    database_path = resolve(args.ui_archaeology_database)
    if database_path is None:
        raise ValueError("--ui-archaeology-database is required")
    asset_index_path = resolve(args.asset_index) or find_latest_current_asset_index(repo_root)
    if asset_index_path is None:
        asset_index_path = repo_root / "research_uiux/data/asset_index.json"

    database = read_json(database_path)
    asset_index = read_json(asset_index_path)
    payload = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "inputs": {
            "ui_archaeology_database": str(database_path.relative_to(repo_root)),
            "asset_index": str(asset_index_path.relative_to(repo_root)),
        },
        "asset_summary": summarize_asset_index(asset_index),
        "rows": build_rows(database),
    }

    output_json = resolve(args.output_json)
    output_header = resolve(args.output_header)
    if output_json is None or output_header is None:
        raise ValueError("--output-json and --output-header are required")

    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    write_header(output_header, payload)
    print(output_json)
    print(output_header)
    print(f"rows={len(payload['rows'])} asset_entries={payload['asset_summary']['entry_count']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
