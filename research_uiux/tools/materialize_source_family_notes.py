#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path
from typing import Any

from map_ui_source_paths import SYSTEM_DISPLAY, load_system_index


STATUS_DISPLAY = {
    "contract_backed": "Contract-backed",
    "archaeology_mapped": "Archaeology-mapped",
    "debug_tool_candidate": "Debug-tool candidate",
    "named_seed_only": "Named seed only",
}


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def normalize_path_key(value: str) -> str:
    return value.replace("\\\\", "\\").replace("\\", "/").lower()


def normalize_host_files(system: dict[str, Any]) -> list[str]:
    result: list[str] = []
    for item in system.get("host_code_files", []):
        if isinstance(item, str):
            result.append(item.replace("\\", "/"))
        elif isinstance(item, dict):
            path = item.get("path")
            if path:
                result.append(str(path).replace("\\", "/"))
    return sorted(dict.fromkeys(result))


def normalize_layout_ids(system: dict[str, Any]) -> list[str]:
    values = system.get("layout_ids", [])
    if not isinstance(values, list):
        return []
    return [str(value) for value in values]


def normalize_state_tags(system: dict[str, Any]) -> list[str]:
    values = system.get("state_tags", [])
    if not isinstance(values, list):
        return []
    return [str(value) for value in values]


def normalize_generated_seams(system: dict[str, Any]) -> list[str]:
    seams: list[str] = []
    for item in system.get("generated_seams", []):
        if isinstance(item, str):
            seams.append(item)
            continue
        if isinstance(item, dict):
            symbol = item.get("symbol")
            if symbol:
                seams.append(str(symbol))
    return sorted(dict.fromkeys(seams))


def classification_for_entry(entry: dict[str, Any], systems: dict[str, dict[str, Any]]) -> tuple[str, list[dict[str, Any]]]:
    matched_systems = [systems[system_id] for system_id in entry.get("matched_system_ids", []) if system_id in systems]
    if not matched_systems:
        if entry.get("debug_tool_candidate"):
            return "debug_host_candidate", []
        return "named_only_placeholder", []

    relative_source_path = entry["relative_source_path"].replace("\\", "/")
    direct_hosts = []
    for system in matched_systems:
        if relative_source_path in normalize_host_files(system):
            direct_hosts.append(system)

    if direct_hosts:
        return "direct_host_anchor", direct_hosts
    return "family_member_anchor", matched_systems


def build_note(entry: dict[str, Any], manifest_entry: dict[str, Any], systems: dict[str, dict[str, Any]]) -> str:
    relative_source_path = entry["relative_source_path"].replace("\\", "/")
    classification, relevant_systems = classification_for_entry(entry, systems)

    lines = [
        f"# {Path(relative_source_path).name}",
        "",
        "Local-only SWARD placement note.",
        "",
        "Identity:",
        f"- source path: `{entry['source_path']}`",
        f"- normalized source path: `{relative_source_path}`",
        f"- scaffold path: `{manifest_entry['relative_path']}`",
        f"- family: `{entry['family_name']}`",
        f"- status: `{STATUS_DISPLAY.get(entry['status'], entry['status'])}`",
        f"- placement class: `{classification}`",
        f"- humanization priority: `{entry['humanization_priority']}`",
        "",
    ]

    if entry.get("matched_system_names"):
        lines.append("Matched archaeology systems:")
        for system_name in entry["matched_system_names"]:
            lines.append(f"- `{system_name}`")
        lines.append("")

    if entry.get("runtime_contracts"):
        lines.append("Runtime contracts:")
        for contract in entry["runtime_contracts"]:
            lines.append(f"- `{contract}`")
        lines.append("")

    if entry.get("notes"):
        lines.extend(["Path-family notes:", f"- {entry['notes']}", ""])

    if relevant_systems:
        lines.append("Recovered system evidence:")
        for system in relevant_systems:
            screen_name = system.get("screen_name", SYSTEM_DISPLAY.get(system.get("system_id", ""), system.get("system_id", "unknown")))
            layout_ids = normalize_layout_ids(system)
            state_tags = normalize_state_tags(system)
            generated_seams = normalize_generated_seams(system)
            host_paths = normalize_host_files(system)

            lines.append(f"- `{screen_name}`")
            if layout_ids:
                lines.append(f"  layout ids: {', '.join(f'`{layout_id}`' for layout_id in layout_ids[:12])}")
            if state_tags:
                lines.append(f"  state tags: {', '.join(f'`{tag}`' for tag in state_tags[:12])}")
            if generated_seams:
                lines.append(f"  generated seams: {', '.join(f'`{symbol}`' for symbol in generated_seams[:12])}")
            if host_paths:
                lines.append(f"  host paths: {', '.join(f'`{path}`' for path in host_paths[:8])}")
        lines.append("")

    lines.extend(
        [
            "Next local move:",
            "- place cleaned translated findings here under the same source-family neighborhood once the seam naming is strong enough",
            "- keep this note local-only until the wider humanized tree and debug executable are finished",
            "",
        ]
    )

    return "\n".join(lines) + "\n"


def build_payload(
    repo_root: Path,
    source_path_manifest_path: Path,
    source_tree_manifest_path: Path,
    archaeology_json_path: Path,
    output_root: Path,
) -> tuple[dict[str, Any], list[tuple[Path, str]]]:
    source_path_manifest = read_json(source_path_manifest_path)
    source_tree_manifest = read_json(source_tree_manifest_path)
    systems = load_system_index(repo_root, archaeology_json_path)

    manifest_by_raw: dict[str, dict[str, Any]] = {}
    for entry in source_tree_manifest.get("entries", []):
        raw_path = entry.get("raw_path", "")
        normalized_path = entry.get("normalized_path", "")
        if raw_path:
            manifest_by_raw[normalize_path_key(raw_path)] = entry
        if normalized_path:
            manifest_by_raw[normalize_path_key(normalized_path)] = entry

    note_jobs: list[tuple[Path, str]] = []
    status_counter: Counter[str] = Counter()
    classification_counter: Counter[str] = Counter()
    sample_notes: list[str] = []

    for entry in source_path_manifest.get("entries", []):
        raw_path = entry["source_path"]
        manifest_entry = manifest_by_raw.get(normalize_path_key(raw_path))
        if manifest_entry is None:
            continue

        note_relative = manifest_entry["relative_path"] + ".sward.md"
        note_path = output_root / note_relative
        note_text = build_note(entry, manifest_entry, systems)
        note_jobs.append((note_path, note_text))

        status_counter[entry["status"]] += 1
        classification, _ = classification_for_entry(entry, systems)
        classification_counter[classification] += 1
        if len(sample_notes) < 12:
            sample_notes.append(note_relative.replace("\\", "/"))

    payload = {
        "inputs": {
            "source_path_manifest": str(source_path_manifest_path),
            "source_tree_manifest": str(source_tree_manifest_path),
            "ui_archaeology_database": str(archaeology_json_path),
        },
        "output_root": str(output_root),
        "summary": {
            "note_file_count": len(note_jobs),
            "status_counts": dict(sorted(status_counter.items())),
            "classification_counts": dict(sorted(classification_counter.items())),
        },
        "sample_note_paths": sample_notes,
    }
    return payload, note_jobs


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
        "- give future humanized translated files a destination that resembles the original source-family layout",
        "- keep uncertain relative-path families separated under `_inferred/` instead of pretending they are fully resolved",
        "",
        "Current local note layer:",
        f"- note files: {summary['note_file_count']}",
        f"- status counts: {json.dumps(summary['status_counts'], ensure_ascii=True)}",
        f"- placement classes: {json.dumps(summary['classification_counts'], ensure_ascii=True)}",
        "",
        "Note suffix pattern:",
        "- `<original source path>.sward.md`",
        "",
        "Generated by `research_uiux/tools/materialize_source_family_notes.py`.",
    ]
    readme_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Materialize local-only source-family placement notes beside the mirrored Sonic Unleashed scaffold.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument("--source-path-manifest", default="research_uiux/data/ui_source_path_manifest.json", help="Current ignored source-path manifest JSON.")
    parser.add_argument("--source-tree-manifest", default="SONIC UNLEASHED/_meta/source_tree_manifest.json", help="Local-only mirrored source-tree manifest.")
    parser.add_argument("--archaeology-json", default="research_uiux/data/ui_archaeology_database.json", help="Tracked archaeology JSON.")
    parser.add_argument("--output-root", default="SONIC UNLEASHED", help="Local-only mirror root.")
    parser.add_argument("--output-manifest", default="SONIC UNLEASHED/_meta/source_family_note_manifest.json", help="Local-only placement manifest output.")
    parser.add_argument("--local-readme", default="SONIC UNLEASHED/_meta/README.txt", help="Local-only mirror readme to refresh.")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    source_path_manifest = (repo_root / args.source_path_manifest).resolve()
    source_tree_manifest = (repo_root / args.source_tree_manifest).resolve()
    archaeology_json = (repo_root / args.archaeology_json).resolve()
    output_root = (repo_root / args.output_root).resolve()
    output_manifest = (repo_root / args.output_manifest).resolve()
    local_readme = (repo_root / args.local_readme).resolve()

    payload, note_jobs = build_payload(
        repo_root=repo_root,
        source_path_manifest_path=source_path_manifest,
        source_tree_manifest_path=source_tree_manifest,
        archaeology_json_path=archaeology_json,
        output_root=output_root,
    )

    for note_path, note_text in note_jobs:
        note_path.parent.mkdir(parents=True, exist_ok=True)
        note_path.write_text(note_text, encoding="utf-8")

    output_manifest.parent.mkdir(parents=True, exist_ok=True)
    output_manifest.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    local_readme.parent.mkdir(parents=True, exist_ok=True)
    update_local_readme(local_readme, payload)
    print(
        "materialized_source_family_notes",
        f"notes={payload['summary']['note_file_count']}",
        f"statuses={payload['summary']['status_counts']}",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
