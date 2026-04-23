#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


DRIVE_RE = re.compile(r"^([A-Za-z]):[\\/](.*)$")
PATHLIKE_RE = re.compile(r"^(?:[A-Za-z]:[\\/]|\.{1,2}[\\/]).+\.[A-Za-z0-9_]+$")


def normalize_line(raw: str) -> str | None:
    line = raw.strip().strip('"')
    if not line or line.startswith("#"):
        return None
    normalized = line.replace("\\", "/")
    normalized = re.sub(r"/+", "/", normalized)
    if not PATHLIKE_RE.match(normalized):
        return None
    return normalized


def classify_path(line: str) -> tuple[str, str, str]:
    lowered = line.lower()

    match = DRIVE_RE.match(line)
    if match:
        drive = match.group(1).upper()
        tail = match.group(2).replace("\\", "/")
        lowered_tail = tail.lower()
        if drive == "D" and lowered_tail.startswith("sonicworldadventure/swa/"):
            return "absolute_swa", "high", f"SonicWorldAdventure/{tail[len('SonicWorldAdventure/'):]}"
        if drive == "D" and lowered_tail.startswith("hedgehog/"):
            return "absolute_hedgehog", "high", tail
        if drive == "E" and lowered_tail.startswith("xenon/"):
            return "absolute_xenon", "high", tail
        return "absolute_other", "medium", f"drives/{drive}/{tail}"

    if lowered.startswith("./"):
        return "relative_dot", "medium", f"_inferred/relative_dot/{line[2:]}"

    if lowered.startswith("../") or lowered.startswith("..//") or lowered.startswith("..\\\\"):
        cleaned = line
        while cleaned.startswith("../"):
            cleaned = cleaned[3:]
        if cleaned.lower().startswith("src/xercesc/"):
            return "relative_xerces", "medium_high", f"thirdparty/xerces/{cleaned}"
        if cleaned.lower().startswith("source/"):
            return "relative_source", "medium", f"_inferred/relative_source/{cleaned}"
        if cleaned.lower().startswith("include/"):
            return "relative_include", "medium", f"_inferred/relative_include/{cleaned}"
        if cleaned.lower().startswith("library/"):
            return "relative_library", "medium", f"_inferred/relative_library/{cleaned}"
        return "relative_parent", "low", f"_inferred/relative_parent/{cleaned}"

    return "unclassified", "low", f"_inferred/unclassified/{line}"


def build_payload(input_txt: Path, output_root: Path) -> dict:
    entries = []
    bucket_counts: Counter[str] = Counter()
    confidence_counts: Counter[str] = Counter()
    top_level_counts: Counter[str] = Counter()
    by_bucket: dict[str, list[dict]] = defaultdict(list)

    for ordinal, raw in enumerate(input_txt.read_text(encoding="utf-8-sig").splitlines(), start=1):
        line = normalize_line(raw)
        if line is None:
            continue
        bucket, confidence, relative_path = classify_path(line)
        local_path = output_root / relative_path
        parent_dir = local_path.parent
        parent_dir.mkdir(parents=True, exist_ok=True)

        entry = {
            "ordinal": ordinal,
            "raw_path": raw.strip(),
            "normalized_path": line,
            "bucket": bucket,
            "confidence": confidence,
            "relative_path": relative_path.replace("\\", "/"),
            "local_parent": str(parent_dir),
            "top_level": relative_path.split("/", 1)[0],
        }
        entries.append(entry)
        bucket_counts[bucket] += 1
        confidence_counts[confidence] += 1
        top_level_counts[entry["top_level"]] += 1
        by_bucket[bucket].append(entry)

    summary = {
        "input_path_count": len(entries),
        "bucket_counts": dict(sorted(bucket_counts.items())),
        "confidence_counts": dict(sorted(confidence_counts.items())),
        "top_level_counts": dict(sorted(top_level_counts.items(), key=lambda item: (-item[1], item[0]))),
    }

    bucket_samples = []
    for bucket, items in sorted(by_bucket.items()):
        bucket_samples.append(
            {
                "bucket": bucket,
                "count": len(items),
                "confidence": items[0]["confidence"],
                "sample_relative_paths": [item["relative_path"] for item in items[:8]],
            }
        )

    return {
        "input": str(input_txt),
        "output_root": str(output_root),
        "summary": summary,
        "bucket_samples": bucket_samples,
        "entries": entries,
    }


def write_local_readme(payload: dict, readme_path: Path) -> None:
    summary = payload["summary"]
    lines = [
        "# SONIC UNLEASHED Local Mirrored Source Tree",
        "",
        "This directory is local-only and intentionally kept out of git.",
        "",
        "Purpose:",
        "- mirror the source-path dump into a stable local folder scaffold",
        "- give future humanized translated files a destination that resembles the original source-family layout",
        "- keep uncertain relative-path families separated under `_inferred/` instead of pretending they are fully resolved",
        "",
        "Summary:",
        f"- input paths: {summary['input_path_count']}",
        f"- bucket counts: {json.dumps(summary['bucket_counts'], ensure_ascii=True)}",
        f"- confidence counts: {json.dumps(summary['confidence_counts'], ensure_ascii=True)}",
        "",
        "Top-level roots:",
    ]
    for key, value in summary["top_level_counts"].items():
        lines.append(f"- {key}: {value}")
    lines.append("")
    lines.append("Generated by `research_uiux/tools/materialize_source_tree.py`.")
    readme_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Create a local-only mirrored source tree scaffold from the Sonic Unleashed path dump.")
    parser.add_argument("--input", required=True, help="Path dump text file.")
    parser.add_argument("--output-root", required=True, help="Local-only output root.")
    parser.add_argument("--manifest", required=True, help="Local-only JSON manifest path.")
    parser.add_argument("--readme", required=True, help="Local-only README path.")
    args = parser.parse_args()

    input_txt = Path(args.input).resolve()
    output_root = Path(args.output_root).resolve()
    manifest = Path(args.manifest).resolve()
    readme = Path(args.readme).resolve()

    output_root.mkdir(parents=True, exist_ok=True)
    manifest.parent.mkdir(parents=True, exist_ok=True)
    readme.parent.mkdir(parents=True, exist_ok=True)

    payload = build_payload(input_txt, output_root)
    manifest.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    write_local_readme(payload, readme)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
