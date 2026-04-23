#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path


FAMILY_KEYWORDS = {
    "loading_transition": ["load", "start", "pda"],
    "pause_status_result": ["pause", "status", "result", "save"],
    "mission_gate_common": ["gate", "mission", "misson", "playscreen"],
    "exstage_qte": ["ex_", "exstage", "qte"],
    "town_common_media": ["shop", "townscreen", "media", "balloon"],
    "world_map_staffroll": ["worldmap", "staffroll"],
    "title_frontend": ["title"],
}

FAMILY_LABELS = {
    "loading_transition": "Loading / Start / Transition texturing",
    "pause_status_result": "Pause / Status / Result / Save loose-text layer",
    "mission_gate_common": "Mission / Gate / Action common layer",
    "exstage_qte": "EX Stage / Tornado Defense support layer",
    "town_common_media": "Town / Shop / Media Room common layer",
    "world_map_staffroll": "World Map / Staff Roll support layer",
    "title_frontend": "Title-screen common layer",
}


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def relpath(repo_root: Path, path: Path | str) -> str:
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


def sorted_counter(counter: Counter[str]) -> dict[str, int]:
    return {key: value for key, value in sorted(counter.items(), key=lambda item: (-item[1], item[0]))}


def collect_phase_root_summary(repo_root: Path, phase_root: Path) -> dict:
    extension_counts: Counter[str] = Counter()
    archive_dirs: dict[str, Counter[str]] = defaultdict(Counter)
    family_counts: Counter[str] = Counter()
    family_samples: dict[str, list[str]] = defaultdict(list)
    highlighted_paths: list[str] = []
    layout_paths: list[str] = []
    file_count = 0

    for path in sorted(phase_root.rglob("*")):
        if not path.is_file():
            continue

        file_count += 1
        extension_counts[path.suffix.lower()] += 1
        rel = relpath(repo_root, path)
        parent_rel = relpath(repo_root, path.parent)
        archive_dirs[parent_rel][path.suffix.lower()] += 1

        lowered = path.stem.lower()
        matched_family = False
        for family_id, keywords in FAMILY_KEYWORDS.items():
            if any(keyword in lowered for keyword in keywords):
                family_counts[family_id] += 1
                if len(family_samples[family_id]) < 6:
                    family_samples[family_id].append(rel)
                matched_family = True

        if path.suffix.lower() in {".yncp", ".xncp"}:
            layout_paths.append(rel)

        if matched_family or path.suffix.lower() in {".dds", ".fco", ".fte"}:
            if len(highlighted_paths) < 32:
                highlighted_paths.append(rel)

    archive_summaries = []
    for archive_dir, counts in sorted(archive_dirs.items()):
        archive_summaries.append(
            {
                "archive_dir": archive_dir,
                "file_count": sum(counts.values()),
                "extension_counts": sorted_counter(counts),
            }
        )

    return {
        "phase_root": relpath(repo_root, phase_root),
        "file_count": file_count,
        "extension_counts": sorted_counter(extension_counts),
        "archive_dir_count": len(archive_summaries),
        "archive_dirs": archive_summaries,
        "family_counts": sorted_counter(family_counts),
        "family_samples": {key: unique_ordered(values) for key, values in sorted(family_samples.items())},
        "highlighted_paths": unique_ordered(highlighted_paths),
        "layout_paths": unique_ordered(layout_paths),
    }


def collect_candidate_summary(candidates_payload: dict) -> dict:
    records = candidates_payload.get("records", [])
    remaining = [record for record in records if not record.get("already_extracted")]
    extracted = [record for record in records if record.get("already_extracted")]

    def compact(record: dict) -> dict:
        return {
            "path": record.get("path"),
            "score": record.get("score", 0),
            "layout_refs": record.get("layout_refs", []),
            "keyword_hits": record.get("keyword_hits", []),
            "texture_ref_count": record.get("texture_ref_count", 0),
            "already_extracted": record.get("already_extracted", False),
        }

    return {
        "candidate_count": candidates_payload.get("candidate_count", len(records)),
        "remaining_count": len(remaining),
        "already_extracted_count": len(extracted),
        "top_remaining": [compact(record) for record in remaining[:12]],
        "top_extracted": [compact(record) for record in extracted[:8]],
    }


def collect_asset_index_summary(asset_index: dict) -> dict:
    combined_entry_count = sum(scan.get("entry_count", 0) for scan in asset_index.get("scans", []))
    extracted_scan = next((scan for scan in asset_index.get("scans", []) if scan.get("root", "").endswith("/extracted_assets")), {})
    phase_entries = [
        entry for entry in extracted_scan.get("entries", []) if entry.get("path", "").startswith("phase25_commonflow_archives/")
    ]
    phase_type_counts = Counter(entry.get("type", "unknown") for entry in phase_entries)
    phase_screen_counts = Counter(entry.get("related_screen", "unknown") for entry in phase_entries)

    return {
        "combined_entry_count": combined_entry_count,
        "installed_entry_count": next(
            (scan.get("entry_count", 0) for scan in asset_index.get("scans", []) if scan.get("root", "").endswith("/game")),
            0,
        ),
        "extracted_entry_count": extracted_scan.get("entry_count", 0),
        "phase25_entry_count": len(phase_entries),
        "phase25_type_counts": sorted_counter(phase_type_counts),
        "phase25_related_screen_counts": sorted_counter(phase_screen_counts),
        "phase25_highlights": [entry["path"] for entry in phase_entries[:24]],
    }


def build_payload(repo_root: Path, phase_root: Path, candidates_json: Path | None, asset_index_json: Path | None) -> dict:
    payload = {
        "repo_root": repo_root.as_posix(),
        "phase25_summary": collect_phase_root_summary(repo_root, phase_root),
    }

    if candidates_json and candidates_json.exists():
        payload["candidate_summary"] = collect_candidate_summary(read_json(candidates_json))

    if asset_index_json and asset_index_json.exists():
        payload["asset_index_summary"] = collect_asset_index_summary(read_json(asset_index_json))

    return payload


def write_markdown(path: Path, payload: dict) -> None:
    phase = payload["phase25_summary"]
    candidates = payload.get("candidate_summary", {})
    asset_summary = payload.get("asset_index_summary", {})

    lines = [
        '<p align="right">',
        '    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>',
        "</p>",
        "",
        '# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Common-Flow Localization Extraction',
        "",
        "Machine-readable inventory: `research_uiux/data/commonflow_localization_extraction.json`",
        "",
        "## Summary",
        "",
        f"- Phase 25 extraction root: `{phase['phase_root']}`",
        f"- Archive directories extracted in this pass: `{phase['archive_dir_count']}`",
        f"- Files extracted in this pass: `{phase['file_count']}`",
        f"- New `.yncp` / `.xncp` layouts surfaced in this pass: `{len(phase['layout_paths'])}`",
    ]

    if asset_summary:
        lines.extend(
            [
                f"- Combined asset-index matches after the re-scan: `{asset_summary['combined_entry_count']}`",
                f"- Extracted-asset matches after the re-scan: `{asset_summary['extracted_entry_count']}`",
                f"- Indexed matches inside `phase25_commonflow_archives`: `{asset_summary['phase25_entry_count']}`",
            ]
        )

    lines.extend(
        [
            "",
            "> [!NOTE]",
            "> This pass broadened the loose localization/common-flow layer much more than it broadened the layout layer.",
            "> The extracted value is mainly DDS/FCO/FTE material that names pause, status, result, loading, world-map, shop, mission, and EX-stage support families.",
            "",
            "## Why This Batch Matters",
            "",
            "- It closes part of the gap between extracted authored layouts and the localized/common-flow texture texturing that those layouts point at.",
            "- It shows that many still-unextracted high-scoring UI candidates are language/common-flow mirrors rather than fresh `.yncp` packages.",
            "- It gives the archaeology layer more loose-file evidence for prompt rows, hint lists, controller text, mission counters, and loading/start messaging.",
            "",
            "## Extension Mix",
            "",
        ]
    )

    for extension, count in phase["extension_counts"].items():
        lines.append(f"- `{extension}`: `{count}`")

    lines.extend(["", "## Family Coverage", ""])
    for family_id, count in phase["family_counts"].items():
        label = FAMILY_LABELS.get(family_id, family_id)
        lines.append(f"- `{label}`: `{count}`")
        for sample in phase["family_samples"].get(family_id, [])[:4]:
            lines.append(f"  - sample: `{sample}`")

    lines.extend(["", "## Extracted Archive Directories", ""])
    for archive in phase["archive_dirs"][:32]:
        ext_bits = ", ".join(f"{ext}={count}" for ext, count in archive["extension_counts"].items())
        lines.append(f"- `{archive['archive_dir']}`: `{archive['file_count']}` files ({ext_bits})")

    if candidates:
        lines.extend(["", "## Ranked Remaining Candidates Before This Pass", ""])
        lines.append(f"- Ranked candidate archives in the pre-pass sweep: `{candidates['candidate_count']}`")
        lines.append(f"- Still-unextracted candidates at that point: `{candidates['remaining_count']}`")
        lines.append("")
        lines.append("Top still-unextracted examples at ranking time:")
        for record in candidates.get("top_remaining", [])[:8]:
            layout_refs = ", ".join(record.get("layout_refs", [])) or "<none>"
            keyword_hits = ", ".join(record.get("keyword_hits", [])) or "<none>"
            lines.append(
                f"- `{record['path']}` score `{record['score']}` | layout refs `{layout_refs}` | keyword hits `{keyword_hits}`"
            )

    if asset_summary:
        lines.extend(["", "## Indexed Phase 25 Hits", ""])
        for type_name, count in asset_summary["phase25_type_counts"].items():
            lines.append(f"- `{type_name}`: `{count}`")

        if asset_summary["phase25_related_screen_counts"]:
            lines.append("")
            lines.append("Related-screen hints inside the indexed Phase 25 slice:")
            for screen_name, count in list(asset_summary["phase25_related_screen_counts"].items())[:12]:
                lines.append(f"- `{screen_name}`: `{count}`")

    lines.extend(["", "## Verified Loose-File Highlights", ""])
    for sample in phase["highlighted_paths"][:24]:
        lines.append(f"- `{sample}`")

    lines.extend(
        [
            "",
            "## Interpretation",
            "",
            "- The remaining high-yield work is now less about blindly hunting for another large `.yncp` batch and more about wiring these common-flow loose files back to the already-extracted layout families.",
            "- `SystemCommonCore`, `ActionCommon`, `Loading`, `Town_Common`, `Town_Labo_Common`, and `WorldMap` now have a stronger localized-texture/support layer even where the authored layout package had already been recovered earlier.",
            "- The Phase 25 batch is especially useful for direct-port template work because it surfaces the naming and packaging around prompt text, hint lists, mission counters, shop/town overlays, EX-stage controller prompts, and loading/start copy.",
            "",
        ]
    )

    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize the Phase 25 common-flow/localization extraction batch.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument("--phase-root", default="extracted_assets/phase25_commonflow_archives")
    parser.add_argument("--candidates-json", default="research_uiux/data/ui_archive_candidates_phase25.json")
    parser.add_argument("--asset-index-json", default="research_uiux/data/asset_index.json")
    parser.add_argument("--output-json", default="research_uiux/data/commonflow_localization_extraction.json")
    parser.add_argument("--output-md", default="research_uiux/COMMON_FLOW_LOCALIZATION_EXTRACTION.md")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()

    def resolve(value: str) -> Path:
        path = Path(value)
        return path if path.is_absolute() else (repo_root / path)

    phase_root = resolve(args.phase_root)
    output_json = resolve(args.output_json)
    output_md = resolve(args.output_md)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)

    payload = build_payload(
        repo_root,
        phase_root,
        resolve(args.candidates_json) if args.candidates_json else None,
        resolve(args.asset_index_json) if args.asset_index_json else None,
    )
    output_json.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    write_markdown(output_md, payload)
    print(output_json)
    print(output_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
