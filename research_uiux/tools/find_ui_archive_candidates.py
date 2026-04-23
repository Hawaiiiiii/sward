#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


KEYWORDS = (
    "ui",
    "hud",
    "menu",
    "title",
    "pause",
    "status",
    "worldmap",
    "world_map",
    "loading",
    "help",
    "result",
    "option",
    "guide",
    "button",
    "window",
    "saveicon",
    "achievement",
    "selectstage",
    "staffroll",
    "font",
    "tutorial",
)

LAYOUT_SUFFIXES = (".yncp", ".xncp")
TEXTURE_SUFFIX = ".dds"
STRING_RE = re.compile(rb"[\x20-\x7E]{4,}")


def extract_strings(path: Path) -> list[str]:
    data = path.read_bytes()
    seen: set[str] = set()
    strings: list[str] = []
    for match in STRING_RE.finditer(data):
        value = match.group().decode("ascii", errors="ignore")
        if value in seen:
            continue
        seen.add(value)
        strings.append(value)
    return strings


def find_matches(strings: list[str]) -> tuple[list[str], list[str], list[str]]:
    keyword_hits: set[str] = set()
    layout_refs: list[str] = []
    texture_refs: list[str] = []

    for value in strings:
        lowered = value.lower()
        if lowered.endswith(LAYOUT_SUFFIXES):
            layout_refs.append(value)
        elif lowered.endswith(TEXTURE_SUFFIX):
            texture_refs.append(value)

        for keyword in KEYWORDS:
            if keyword in lowered:
                keyword_hits.add(keyword)

    return sorted(keyword_hits), sorted(layout_refs), sorted(texture_refs)


def compute_score(path: Path, keyword_hits: list[str], layout_refs: list[str], texture_refs: list[str]) -> int:
    lowered = path.name.lower()
    score = len(keyword_hits) * 3
    score += len(layout_refs) * 8
    score += min(len(texture_refs), 20)
    for keyword in KEYWORDS:
        if keyword in lowered:
            score += 4
    return score


def infer_extracted_status(path: Path, base_root: Path, extracted_roots: list[Path]) -> bool:
    stem = path.name[:-4] if path.name.lower().endswith(".arl") else path.stem
    normalized = stem.lstrip("#")
    relative_parts = path.relative_to(base_root).parts[:-1]
    candidate_names: set[str]

    if len(relative_parts) >= 2 and relative_parts[0].lower() == "languages":
        language = relative_parts[1]
        candidate_names = {
            f"{stem}_{language}",
            f"{normalized}_{language}",
        }
    else:
        candidate_names = {stem, normalized}

    for root in extracted_roots:
        if not root.exists():
            continue
        for candidate in sorted(candidate_names):
            if (root / candidate).exists():
                return True
    return False


def build_record(path: Path, base_root: Path, extracted_roots: list[Path]) -> dict:
    strings = extract_strings(path)
    keyword_hits, layout_refs, texture_refs = find_matches(strings)
    score = compute_score(path, keyword_hits, layout_refs, texture_refs)
    ar_path = path.with_suffix(".ar.00")

    return {
        "path": path.relative_to(base_root).as_posix(),
        "name": path.name,
        "score": score,
        "keyword_hits": keyword_hits,
        "layout_refs": layout_refs[:40],
        "layout_ref_count": len(layout_refs),
        "texture_ref_count": len(texture_refs),
        "has_archive_pair": ar_path.exists(),
        "archive_pair": ar_path.relative_to(base_root).as_posix() if ar_path.exists() else None,
        "already_extracted": infer_extracted_status(path, base_root, extracted_roots),
    }


def write_markdown(path: Path, asset_root: Path, records: list[dict], limit: int) -> None:
    top = records[:limit]
    lines = [
        "# Broader UI Archive Extraction",
        "",
        f"Asset root: `{asset_root.as_posix()}`",
        "",
        f"Candidate archives scored: `{len(records)}`",
        f"Top candidates shown: `{len(top)}`",
        "",
        "## Top UI-Heavy Archive Candidates",
        "",
    ]

    for record in top:
        lines.append(f"## `{record['path']}`")
        lines.append("")
        lines.append(f"- Score: `{record['score']}`")
        lines.append(f"- Already extracted: `{str(record['already_extracted']).lower()}`")
        lines.append(f"- Has `.ar.00` pair: `{str(record['has_archive_pair']).lower()}`")
        if record["keyword_hits"]:
            lines.append(f"- Keyword hits: `{', '.join(record['keyword_hits'])}`")
        if record["layout_refs"]:
            lines.append("- Referenced layouts:")
            for layout in record["layout_refs"][:10]:
                lines.append(f"  - `{layout}`")
        lines.append("")

    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Find UI-heavy archive candidates by scanning .arl references.")
    parser.add_argument("--asset-root", required=True, help="Root containing .arl/.ar.00 files.")
    parser.add_argument(
        "--extracted-root",
        action="append",
        default=[],
        help="Existing extracted root. May be passed multiple times.",
    )
    parser.add_argument("--output-json", required=True, help="JSON output path.")
    parser.add_argument("--output-md", required=True, help="Markdown output path.")
    parser.add_argument("--top", type=int, default=40, help="How many records to include in markdown.")
    args = parser.parse_args()

    asset_root = Path(args.asset_root).resolve()
    extracted_roots = [Path(root).resolve() for root in args.extracted_root]
    output_json = Path(args.output_json).resolve()
    output_md = Path(args.output_md).resolve()
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)

    records = []
    for path in sorted(asset_root.rglob("*.arl")):
        record = build_record(path, asset_root, extracted_roots)
        if record["score"] > 0 or record["layout_ref_count"] > 0:
            records.append(record)

    records.sort(
        key=lambda item: (
            -item["score"],
            item["already_extracted"],
            item["path"],
        )
    )

    payload = {
        "asset_root": asset_root.as_posix(),
        "extracted_roots": [root.as_posix() for root in extracted_roots],
        "candidate_count": len(records),
        "records": records,
    }
    output_json.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    write_markdown(output_md, asset_root, records, args.top)
    print(output_json)
    print(output_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
