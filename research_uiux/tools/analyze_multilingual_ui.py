#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path


LANGUAGES = ["English", "French", "German", "Italian", "Japanese", "Spanish"]
KEY_UI_ARCHIVES = [
    "ActionCommon",
    "BossCommon",
    "Loading",
    "Sonic",
    "SonicActionCommon",
    "SuperSonic",
    "SystemCommonCore",
    "Title",
    "WorldMap",
]


def archive_display_name(path: Path) -> str:
    if path.name.lower().endswith(".ar.00"):
        return path.name[:-6]
    return path.stem


def language_ui_archives(asset_root: Path) -> dict[str, list[dict[str, str]]]:
    results: dict[str, list[dict[str, str]]] = {}
    for language in LANGUAGES:
        lang_root = asset_root / "Languages" / language
        items: list[dict[str, str]] = []
        if lang_root.exists():
            seen: set[str] = set()
            for path in sorted(lang_root.rglob("*")):
                if not path.is_file():
                    continue
                suffix = path.suffix.lower()
                if suffix not in {".arl", ".00"}:
                    continue
                if suffix == ".00" and not path.name.lower().endswith(".ar.00"):
                    continue
                rel = path.relative_to(asset_root).as_posix()
                key = rel.lower()
                if key in seen:
                    continue
                seen.add(key)
                items.append(
                    {
                        "path": rel,
                        "display_name": archive_display_name(path),
                        "extension": path.suffix.lower(),
                    }
                )
        results[language] = items
    return results


def subtitle_archives(asset_root: Path) -> dict[str, list[str]]:
    results: dict[str, list[str]] = {}
    for language in LANGUAGES:
        sub_root = asset_root / "Inspire" / "subtitle" / language
        ids = sorted(
            archive_display_name(path)
            for path in sub_root.glob("*.arl")
        ) if sub_root.exists() else []
        results[language] = ids
    return results


def subtitle_family(scene_id: str) -> str:
    parts = scene_id.split("_")
    if len(parts) < 2:
        return "unknown"
    return parts[1]


def summarize_extracted_root(root: Path) -> dict:
    ext_counter: Counter[str] = Counter()
    language_counter: Counter[str] = Counter()
    top_level_counter: Counter[str] = Counter()
    localized_loading_paths: list[str] = []
    localized_title_paths: list[str] = []
    localized_worldmap_paths: list[str] = []
    subtitle_paths: list[str] = []

    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        rel = path.relative_to(root).as_posix()
        ext_counter[path.suffix.lower()] += 1
        top_level = rel.split("/", 1)[0]
        top_level_counter[top_level] += 1
        for language in LANGUAGES:
            token = f"/{language}/"
            if token in f"/{rel}/":
                language_counter[language] += 1
                break
        lowered = rel.lower()
        if "loading" in lowered and len(localized_loading_paths) < 15:
            localized_loading_paths.append(rel)
        if "title" in lowered and len(localized_title_paths) < 15:
            localized_title_paths.append(rel)
        if "worldmap" in lowered and len(localized_worldmap_paths) < 15:
            localized_worldmap_paths.append(rel)
        if "inspire/subtitle/" in lowered and len(subtitle_paths) < 30:
            subtitle_paths.append(rel)

    return {
        "root": root.as_posix(),
        "file_count": sum(ext_counter.values()),
        "extensions": dict(sorted(ext_counter.items())),
        "language_hits": dict(sorted(language_counter.items())),
        "top_level_paths": dict(sorted(top_level_counter.items())),
        "sample_loading_paths": localized_loading_paths,
        "sample_title_paths": localized_title_paths,
        "sample_worldmap_paths": localized_worldmap_paths,
        "sample_subtitle_paths": subtitle_paths,
    }


def build_payload(asset_root: Path, extracted_roots: list[Path]) -> dict:
    language_archives = language_ui_archives(asset_root)
    subtitle_by_language = subtitle_archives(asset_root)

    key_ui_matrix: dict[str, dict[str, bool]] = {}
    for archive_name in KEY_UI_ARCHIVES:
        key_ui_matrix[archive_name] = {}
        for language in LANGUAGES:
            key_ui_matrix[archive_name][language] = any(
                entry["display_name"] == archive_name
                for entry in language_archives[language]
            )

    shared_subtitle_ids = set(subtitle_by_language[LANGUAGES[0]])
    for language in LANGUAGES[1:]:
        shared_subtitle_ids &= set(subtitle_by_language[language])

    union_subtitle_ids = set()
    for language in LANGUAGES:
        union_subtitle_ids |= set(subtitle_by_language[language])

    per_language_missing: dict[str, list[str]] = {}
    for language in LANGUAGES:
        per_language_missing[language] = sorted(union_subtitle_ids - set(subtitle_by_language[language]))

    family_counter: Counter[str] = Counter()
    for scene_id in sorted(shared_subtitle_ids):
        family_counter[subtitle_family(scene_id)] += 1

    extracted_summaries = [
        summarize_extracted_root(root)
        for root in extracted_roots
        if root.exists()
    ]

    return {
        "asset_root": asset_root.as_posix(),
        "languages": LANGUAGES,
        "key_ui_archives": KEY_UI_ARCHIVES,
        "language_archive_counts": {
            language: len(language_archives[language])
            for language in LANGUAGES
        },
        "language_archives": language_archives,
        "key_ui_matrix": key_ui_matrix,
        "subtitle_counts": {
            language: len(subtitle_by_language[language])
            for language in LANGUAGES
        },
        "subtitle_by_language": subtitle_by_language,
        "shared_subtitle_count": len(shared_subtitle_ids),
        "shared_subtitle_ids": sorted(shared_subtitle_ids),
        "subtitle_missing_by_language": per_language_missing,
        "shared_subtitle_family_counts": dict(sorted(family_counter.items())),
        "extracted_roots": [root.as_posix() for root in extracted_roots],
        "extracted_summaries": extracted_summaries,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit multilingual UI and subtitle coverage.")
    parser.add_argument("--asset-root", required=True, help="Installed build game root.")
    parser.add_argument(
        "--extracted-root",
        action="append",
        default=[],
        help="Extracted multilingual roots to summarize. May be passed multiple times.",
    )
    parser.add_argument(
        "--output",
        default="research_uiux/data/multilingual_ui_coverage.json",
        help="Output JSON path.",
    )
    args = parser.parse_args()

    asset_root = Path(args.asset_root).resolve()
    extracted_roots = [Path(item).resolve() for item in args.extracted_root]
    payload = build_payload(asset_root, extracted_roots)

    output = Path(args.output)
    if not output.is_absolute():
        output = Path.cwd() / output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    print(output.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
