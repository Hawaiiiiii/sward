#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


DEFAULT_SYSTEMS = [
    "loading_and_start",
    "world_map_stack",
    "town_ui",
    "mission_briefing_and_gate",
    "mission_result_family",
    "save_and_ending",
    "tornado_defense",
    "subtitle_cutscene_presentation",
]

PATCH_ROLE_MAP = {
    "aspect_ratio_patches.cpp": "layout_projection_or_scaling",
    "fps_patches.cpp": "delta_time_or_timing_owner",
    "resident_patches.cpp": "resident_overlay_visibility",
    "object_patches.cpp": "feedback_or_prompt_fx",
    "misc_patches.cpp": "prompt_filter_or_tutorial_gate",
    "input_patches.cpp": "input_cursor_or_camera_owner",
    "video_patches.cpp": "movie_wrapper_owner",
    "CHudPause_patches.cpp": "pause_state_owner",
    "CTitleStateIntro_patches.cpp": "title_intro_owner",
    "CTitleStateMenu_patches.cpp": "title_menu_owner",
}

FUNCTION_START_RE = re.compile(r"PPC_FUNC_IMPL\(__imp__(sub_[0-9A-F]+)\)")
CALL_RE = re.compile(r"\b(sub_[0-9A-F]+)\s*\(")
FLOAT_RE = re.compile(r"\b\d+\.\d+f?\b")


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


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


def best_patch_role(patch_file: str | None) -> str:
    if not patch_file:
        return "state_or_layout_host"
    for suffix, role in PATCH_ROLE_MAP.items():
        if patch_file.endswith(suffix):
            return role
    return "state_or_layout_host"


def normalize_patch_files(symbol_entry: dict, source_type: str) -> list[str]:
    if source_type == "precise":
        return symbol_entry.get("patch_files", [])
    patch_file = symbol_entry.get("patch_file")
    return [patch_file] if patch_file else []


def choose_patch_file(symbol_entry: dict, source_type: str) -> str | None:
    patch_files = normalize_patch_files(symbol_entry, source_type)
    return sorted(patch_files)[0] if patch_files else None


def timing_anchor(system: dict) -> str | None:
    highlights = system.get("timing_highlights", [])
    if not highlights:
        return None
    item = highlights[0]
    return f"{item.get('scene_name', '<scene>')}::{item.get('animation_name', '<anim>')}={item.get('frame_count', 0)}f"


def host_anchor(system: dict) -> str | None:
    host_files = system.get("host_code_files", [])
    if not host_files:
        return None
    host = host_files[0]
    path = host.get("path", "<unknown>")
    line = host.get("line")
    if line:
        return f"{path}:{line}"
    return path


def derive_label(system: dict, symbol_entry: dict, source_type: str, ordinal: int) -> str:
    role = best_patch_role(choose_patch_file(symbol_entry, source_type))
    return f"{system['system_id']}__{role}__{ordinal:02d}"


def collect_function_window(repo_root: Path, generated_file: str | None, impl_line: int | None, symbol: str) -> dict:
    if not generated_file or impl_line is None:
        return {}

    path = Path(generated_file)
    if not path.is_absolute():
        path = repo_root / path
    if not path.exists():
        return {}

    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    start_index = max(0, int(impl_line) - 1)
    end_index = len(lines)
    for index in range(start_index + 1, len(lines)):
        if FUNCTION_START_RE.search(lines[index]):
            end_index = index
            break

    window = lines[start_index:end_index]
    joined = "\n".join(window)
    callees = [match for match in CALL_RE.findall(joined) if match != symbol]
    float_constants = FLOAT_RE.findall(joined)

    return {
        "line_span": len(window),
        "callee_count": len(unique_ordered(callees)),
        "callees": unique_ordered(callees)[:12],
        "float_constants": unique_ordered(float_constants)[:10],
    }


def build_symbol_record(repo_root: Path, system: dict, symbol_entry: dict, source_type: str, ordinal: int) -> dict:
    patch_file = choose_patch_file(symbol_entry, source_type)
    window = collect_function_window(
        repo_root,
        symbol_entry.get("generated_file"),
        symbol_entry.get("impl_line"),
        symbol_entry["symbol"],
    )
    role = best_patch_role(patch_file)
    confidence = "high" if source_type == "precise" else "medium_high"
    if not symbol_entry.get("generated_file"):
        confidence = "medium"

    return {
        "symbol": symbol_entry["symbol"],
        "candidate_label": derive_label(system, symbol_entry, source_type, ordinal),
        "source_type": source_type,
        "source_types": [source_type],
        "confidence": confidence,
        "patch_files": normalize_patch_files(symbol_entry, source_type),
        "primary_patch_file": patch_file,
        "role_family": role,
        "generated_file": relpath(repo_root, symbol_entry.get("generated_file")),
        "impl_line": symbol_entry.get("impl_line"),
        "readable_relationship": symbol_entry.get("readable_relationship"),
        "layout_ids": system.get("layout_ids", []),
        "state_focus": system.get("state_tags", []),
        "transition_focus": system.get("transition_tags", []),
        "timing_anchor": timing_anchor(system),
        "host_anchor": host_anchor(system),
        "host_summary": system.get("host_code_files", [{}])[0].get("summary") if system.get("host_code_files") else None,
        "function_window": window,
    }


def merge_symbol_record(existing: dict, incoming: dict) -> None:
    existing["source_types"] = unique_ordered(existing.get("source_types", []) + incoming.get("source_types", []))
    existing["patch_files"] = unique_ordered(existing.get("patch_files", []) + incoming.get("patch_files", []))
    if existing.get("primary_patch_file") is None and incoming.get("primary_patch_file") is not None:
        existing["primary_patch_file"] = incoming["primary_patch_file"]
    if existing.get("generated_file") is None and incoming.get("generated_file") is not None:
        existing["generated_file"] = incoming["generated_file"]
        existing["impl_line"] = incoming.get("impl_line")
        existing["function_window"] = incoming.get("function_window", {})
    if existing.get("readable_relationship") is None and incoming.get("readable_relationship") is not None:
        existing["readable_relationship"] = incoming["readable_relationship"]


def build_system_record(repo_root: Path, system: dict) -> dict:
    labels_by_symbol: dict[str, dict] = {}
    precise_symbols = system.get("generated_seams", [])
    patch_bank_symbols = system.get("patch_generated_symbols", [])

    for ordinal, symbol_entry in enumerate(sorted(precise_symbols, key=lambda item: item["symbol"]), start=1):
        record = build_symbol_record(repo_root, system, symbol_entry, "precise", ordinal)
        labels_by_symbol[record["symbol"]] = record

    for ordinal, symbol_entry in enumerate(sorted(patch_bank_symbols, key=lambda item: item["symbol"]), start=1):
        record = build_symbol_record(repo_root, system, symbol_entry, "patch_bank", ordinal)
        existing = labels_by_symbol.get(record["symbol"])
        if existing is None:
            labels_by_symbol[record["symbol"]] = record
        else:
            merge_symbol_record(existing, record)

    labels = sorted(labels_by_symbol.values(), key=lambda item: (item["role_family"], item["symbol"]))
    for ordinal, label in enumerate(labels, start=1):
        label["candidate_label"] = f"{system['system_id']}__{label['role_family']}__{ordinal:02d}"

    role_counts = Counter(label["role_family"] for label in labels)
    source_counts = Counter()
    for label in labels:
        if "precise" in label.get("source_types", []):
            source_counts["precise"] += 1
        elif "patch_bank" in label.get("source_types", []):
            source_counts["patch_bank_only"] += 1
    resolved_count = sum(1 for label in labels if label.get("generated_file"))
    function_window_count = sum(1 for label in labels if label.get("function_window", {}).get("line_span"))

    return {
        "system_id": system["system_id"],
        "screen_name": system["screen_name"],
        "layout_ids": system.get("layout_ids", []),
        "state_tags": system.get("state_tags", []),
        "transition_tags": system.get("transition_tags", []),
        "host_code_files": system.get("host_code_files", []),
        "timing_highlights": system.get("timing_highlights", []),
        "notes": system.get("notes"),
        "label_count": len(labels),
        "resolved_generated_count": resolved_count,
        "function_window_count": function_window_count,
        "role_counts": {key: value for key, value in sorted(role_counts.items(), key=lambda item: (-item[1], item[0]))},
        "source_counts": {key: value for key, value in sorted(source_counts.items(), key=lambda item: (-item[1], item[0]))},
        "labels": labels,
    }


def build_payload(repo_root: Path, archaeology_json: Path, systems: list[str]) -> dict:
    archaeology = read_json(archaeology_json)
    all_systems = {system["system_id"]: system for system in archaeology.get("systems", [])}
    selected = []
    for system_id in systems:
        system = all_systems.get(system_id)
        if system:
            selected.append(build_system_record(repo_root, system))

    summary = {
        "target_system_count": len(selected),
        "labeled_symbol_count": sum(system["label_count"] for system in selected),
        "resolved_generated_count": sum(system["resolved_generated_count"] for system in selected),
        "function_window_count": sum(system["function_window_count"] for system in selected),
        "systems_without_generated_labels": [system["system_id"] for system in selected if system["label_count"] == 0],
    }

    return {
        "repo_root": repo_root.as_posix(),
        "inputs": {
            "ui_archaeology_database": relpath(repo_root, archaeology_json),
        },
        "summary": summary,
        "systems": selected,
    }


def write_markdown(path: Path, payload: dict) -> None:
    summary = payload["summary"]
    lines = [
        '<p align="right">',
        '    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>',
        "</p>",
        "",
        '# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> PPC Layout / State Labels',
        "",
        "Machine-readable inventory: `research_uiux/data/ppc_ui_state_labels.json`",
        "",
        "## Summary",
        "",
        f"- Target systems labeled in this pass: `{summary['target_system_count']}`",
        f"- Total labeled translated seams: `{summary['labeled_symbol_count']}`",
        f"- Labeled seams with resolved generated locations: `{summary['resolved_generated_count']}`",
        f"- Function windows sampled directly from translated PPC output: `{summary['function_window_count']}`",
    ]

    if summary["systems_without_generated_labels"]:
        lines.append(
            f"- Systems still without resolved translated seams: `{', '.join(summary['systems_without_generated_labels'])}`"
        )

    lines.extend(
        [
            "",
            "> [!NOTE]",
            "> These labels are archaeology aids, not claims about original authored function names.",
            "> They translate patch ownership, layout families, timing anchors, and translated function windows into reusable screen-contract labels.",
            "",
        ]
    )

    for system in payload["systems"]:
        lines.append(f"## `{system['system_id']}` - {system['screen_name']}")
        lines.append("")
        if system["layout_ids"]:
            lines.append(f"- Layout IDs: `{', '.join(system['layout_ids'])}`")
        if system["state_tags"]:
            lines.append(f"- State focus: `{', '.join(system['state_tags'])}`")
        if system["transition_tags"]:
            lines.append(f"- Transition focus: `{', '.join(system['transition_tags'])}`")
        lines.append(f"- Labeled seams: `{system['label_count']}`")
        lines.append(f"- Resolved generated seams: `{system['resolved_generated_count']}`")
        if system["role_counts"]:
            role_bits = ", ".join(f"{role}={count}" for role, count in system["role_counts"].items())
            lines.append(f"- Role families: `{role_bits}`")
        if system["notes"]:
            lines.append(f"- Why this group matters: {system['notes']}")
        lines.append("")

        if not system["labels"]:
            lines.append("- No translated PPC seam is currently resolved for this system; it remains readable-code and asset-driven.")
            lines.append("")
            continue

        lines.append("### Representative Labels")
        lines.append("")
        samples = []
        precise = [label for label in system["labels"] if label["source_type"] == "precise"]
        patch_bank = [label for label in system["labels"] if label["source_type"] == "patch_bank"]
        samples.extend(precise[:6])
        samples.extend(patch_bank[:6])

        for label in samples:
            location = label["generated_file"] or "<missing>"
            if label.get("impl_line"):
                location = f"{location}:{label['impl_line']}"
            lines.append(f"- `{label['symbol']}` -> `{label['candidate_label']}`")
            lines.append(
                f"  - source: `{'+'.join(label.get('source_types', [label['source_type']]))}` via `{label['primary_patch_file'] or '<none>'}`"
            )
            lines.append(f"  - role family: `{label['role_family']}`")
            lines.append(f"  - generated location: `{location}`")
            if label.get("host_anchor"):
                lines.append(f"  - host anchor: `{label['host_anchor']}`")
            if label.get("timing_anchor"):
                lines.append(f"  - timing anchor: `{label['timing_anchor']}`")
            window = label.get("function_window", {})
            if window.get("line_span"):
                lines.append(
                    f"  - function window: `{window['line_span']}` lines, `{window['callee_count']}` unique sub-calls"
                )
            if window.get("callees"):
                lines.append(f"  - sampled callees: `{', '.join(window['callees'][:6])}`")
            if window.get("float_constants"):
                lines.append(f"  - sampled float constants: `{', '.join(window['float_constants'][:6])}`")
        lines.append("")

    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Label translated PPC seams against layout/state families for the still-underexplained UI systems.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument("--archaeology-json", default="research_uiux/data/ui_archaeology_database.json")
    parser.add_argument("--system", action="append", default=[], help="System id to label. Can be passed multiple times.")
    parser.add_argument("--output-json", default="research_uiux/data/ppc_ui_state_labels.json")
    parser.add_argument("--output-md", default="research_uiux/PPC_LAYOUT_STATE_LABELS.md")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()

    def resolve(value: str) -> Path:
        path = Path(value)
        return path if path.is_absolute() else (repo_root / path)

    systems = args.system or DEFAULT_SYSTEMS
    archaeology_json = resolve(args.archaeology_json)
    output_json = resolve(args.output_json)
    output_md = resolve(args.output_md)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)

    payload = build_payload(repo_root, archaeology_json, systems)
    output_json.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    write_markdown(output_md, payload)
    print(output_json)
    print(output_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
