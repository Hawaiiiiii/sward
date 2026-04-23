#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path


RELEVANT_EXTS = {".yncp", ".xncp", ".dds", ".ar", ".arl", ".pac", ".lua", ".xml", ".json", ".ini"}
KEYWORDS = [
    "hud",
    "ui",
    "menu",
    "title",
    "pause",
    "result",
    "rank",
    "option",
    "options",
    "cursor",
    "button",
    "guide",
    "window",
    "font",
    "gauge",
    "boost",
    "ring",
    "score",
    "mission",
    "achievement",
    "transition",
    "fade",
    "black",
    "static",
    "loading",
    "tutorial",
    "subtitle",
    "prompt",
    "overlay",
]

TOOL_BY_EXT = {
    ".yncp": "Kunai / Shuriken",
    ".xncp": "Kunai / Shuriken",
    ".dds": "DDS-capable texture viewer",
    ".ar": "HedgeArcPack / HedgeLib",
    ".arl": "HedgeArcPack / HedgeLib",
    ".pac": "Archive viewer / Hedge tools depending on format",
    ".lua": "Text editor",
    ".xml": "Text editor",
    ".json": "Text editor",
    ".ini": "Text editor",
}

TYPE_BY_EXT = {
    ".yncp": "layout_animation",
    ".xncp": "layout_animation",
    ".dds": "texture",
    ".ar": "archive",
    ".arl": "archive_index",
    ".pac": "archive",
    ".lua": "script",
    ".xml": "config_or_markup",
    ".json": "config_or_metadata",
    ".ini": "config",
}


def classify_screen(name: str) -> str:
    lowered = name.lower()
    for keyword in KEYWORDS:
        if keyword in lowered:
            return keyword
    return "unknown"


def infer_purpose(path: Path) -> str:
    lowered = path.name.lower()
    if path.suffix.lower() in {".yncp", ".xncp"}:
        return "Likely Ninja CellSpriteDraw layout/animation data."
    if path.suffix.lower() == ".dds":
        return "Likely UI or HUD texture sheet."
    if path.suffix.lower() in {".ar", ".arl", ".pac"}:
        return "Archive or archive manifest that may contain UI resources."
    if any(token in lowered for token in ("font", "message", "subtitle")):
        return "Likely text, font, or message-related resource."
    if any(token in lowered for token in ("button", "guide", "prompt")):
        return "Likely button prompt or interaction guide asset."
    if any(token in lowered for token in ("title", "menu", "pause", "option", "achievement")):
        return "Likely front-end screen asset."
    return "UI-relevant asset candidate based on extension or name."


def include_file(path: Path) -> bool:
    lowered = path.name.lower()
    return path.suffix.lower() in RELEVANT_EXTS or any(keyword in lowered for keyword in KEYWORDS)


def scan_root(root: Path) -> dict:
    entries = []
    type_counter: Counter[str] = Counter()
    screen_counter: Counter[str] = Counter()

    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        if not include_file(path):
            continue
        rel = path.relative_to(root).as_posix()
        asset_type = TYPE_BY_EXT.get(path.suffix.lower(), "candidate")
        screen = classify_screen(path.name)
        type_counter[asset_type] += 1
        screen_counter[screen] += 1
        entries.append(
            {
                "root": root.as_posix(),
                "path": rel,
                "extension": path.suffix.lower(),
                "size": path.stat().st_size,
                "type": asset_type,
                "related_screen": screen,
                "inferred_purpose": infer_purpose(path),
                "suggested_tool": TOOL_BY_EXT.get(path.suffix.lower(), "Manual inspection"),
            }
        )

    return {
        "root": root.resolve().as_posix(),
        "entry_count": len(entries),
        "types": dict(sorted(type_counter.items())),
        "related_screens": dict(sorted(screen_counter.items())),
        "entries": entries,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Scan local asset roots for UI-relevant files.")
    parser.add_argument("--repo-root", default=".", help="Source root, used to resolve relative output paths.")
    parser.add_argument(
        "--asset-root",
        action="append",
        default=[],
        help="Asset root to scan. May be passed multiple times.",
    )
    parser.add_argument(
        "--output",
        default="research_uiux/data/asset_index.json",
        help="Output JSON path.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    output = Path(args.output)
    if not output.is_absolute():
        output = repo_root / output
    output.parent.mkdir(parents=True, exist_ok=True)

    roots = [Path(root).resolve() for root in args.asset_root]
    scans = [scan_root(root) for root in roots if root.exists()]

    payload = {
        "repo_root": repo_root.as_posix(),
        "asset_roots": [root.as_posix() for root in roots],
        "scan_count": len(scans),
        "scans": scans,
    }
    output.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
