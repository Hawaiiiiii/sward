#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


PATCH_DIR = "UnleashedRecomp/patches"
PATCH_SUFFIXES = {".cpp", ".h", ".hpp"}
KEYWORDS = [
    "hud",
    "ui",
    "menu",
    "title",
    "pause",
    "result",
    "rank",
    "option",
    "cursor",
    "button",
    "guide",
    "window",
    "achievement",
    "transition",
    "fade",
    "black",
    "static",
    "input",
    "aspect",
    "loading",
    "mission",
]

SUMMARY_HINTS = {
    "aspect_ratio_patches.cpp": "Patches original code paths to preserve UI composition across wider aspect ratios and safe-area layouts.",
    "CHudPause_patches.cpp": "Intercepts pause-HUD flow and inserts custom pause/options behavior.",
    "CTitleStateIntro_patches.cpp": "Hooks title-intro execution to alter intro sequencing and handoff.",
    "CTitleStateMenu_patches.cpp": "Hooks title-menu execution and custom menu behavior.",
    "CGameModeStageTitle_patches.cpp": "Touches stage-title presentation path in the original game mode.",
    "input_patches.cpp": "Hooks input-related code paths that affect UI interaction and controller behavior.",
}

QUALIFIED_RE = re.compile(r"\b(?:[A-Za-z_]\w*::){1,}[A-Za-z_~]\w*\b")
SUB_RE = re.compile(r"\bsub_[0-9A-Fa-f]+\b")
HEX_RE = re.compile(r"\b0x[0-9A-Fa-f]+\b")
HOOK_RE = re.compile(r"\b(?:HOOK|INSTALL_HOOK|PATCH|WRITE_MEMORY|READ_MEMORY|WRITE_JUMP|WRITE_CALL)\b")
CALL_RE = re.compile(r"\b([A-Za-z_]\w*)\s*\(")


def unique(items):
    seen = set()
    output = []
    for item in items:
        if not item or item in seen:
            continue
        seen.add(item)
        output.append(item)
    return output


def summarize(name: str, tags: list[str]) -> str:
    if name in SUMMARY_HINTS:
        return SUMMARY_HINTS[name]
    if tags:
        return f"Patch file touching UI-related behavior around: {', '.join(tags[:4])}."
    return "Patch file with no strong UI keyword match."


def scan_patch(repo_root: Path, path: Path) -> dict:
    text = path.read_text(encoding="utf-8", errors="ignore")
    lower = text.lower()
    tags = [term for term in KEYWORDS if term in lower or term in path.name.lower()]

    qualified_symbols = unique(QUALIFIED_RE.findall(text))
    sub_refs = unique(SUB_RE.findall(text))
    addresses = unique(HEX_RE.findall(text))
    hook_macros = unique(HOOK_RE.findall(text))

    candidate_calls = []
    for match in CALL_RE.finditer(text):
        name = match.group(1)
        if name in {"if", "for", "while", "switch", "return", "sizeof"}:
            continue
        candidate_calls.append(name)
    candidate_calls = unique(candidate_calls)

    generated_lookup_keys = unique(qualified_symbols + sub_refs + addresses)

    return {
        "path": path.relative_to(repo_root).as_posix(),
        "summary": summarize(path.name, tags),
        "ui_tags": sorted(set(tags)),
        "hook_macros": hook_macros,
        "original_symbols": qualified_symbols,
        "sub_refs": sub_refs,
        "addresses": addresses,
        "candidate_calls": candidate_calls[:150],
        "generated_lookup_keys": generated_lookup_keys,
    }


def build_index(repo_root: Path) -> dict:
    patch_root = repo_root / PATCH_DIR
    files = []
    if patch_root.exists():
        for path in sorted(patch_root.rglob("*")):
            if path.is_file() and path.suffix.lower() in PATCH_SUFFIXES:
                files.append(scan_patch(repo_root, path))

    return {
        "repo_root": repo_root.resolve().as_posix(),
        "patch_dir": PATCH_DIR,
        "file_count": len(files),
        "files": files,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Scan UnleashedRecomp patch hooks and emit deterministic JSON.")
    parser.add_argument("--repo-root", default=".", help="Path to the UnleashedRecomp source root.")
    parser.add_argument(
        "--output",
        default="research_uiux/data/patch_hooks.json",
        help="Output JSON path.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    output = Path(args.output)
    if not output.is_absolute():
        output = repo_root / output
    output.parent.mkdir(parents=True, exist_ok=True)

    index = build_index(repo_root)
    output.write_text(json.dumps(index, indent=2, sort_keys=True), encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
