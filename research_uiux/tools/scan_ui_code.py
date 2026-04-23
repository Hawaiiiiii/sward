#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from collections import Counter
from pathlib import Path


SOURCE_DIRS = [
    "UnleashedRecomp/ui",
    "UnleashedRecomp/patches",
    "UnleashedRecomp/config",
    "UnleashedRecompLib/config",
]

SOURCE_SUFFIXES = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".inl"}
UI_TERMS = [
    "ui",
    "hud",
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
    "aspect",
    "input",
    "thumbnail",
]

SUMMARY_HINTS = {
    "achievement_menu.cpp": "ImGui-driven achievements menu flow, filtering, layout, and rendering.",
    "achievement_overlay.cpp": "Transient achievement toast/overlay timing and presentation logic.",
    "black_bar.cpp": "Letterbox or cinematic black-bar presentation effect.",
    "button_guide.cpp": "Controller prompt abstraction and button-guide rendering/state updates.",
    "fader.cpp": "Shared fade-in/fade-out alpha progression and timing helpers.",
    "game_window.cpp": "Main debug/game window orchestration and screen composition hooks.",
    "imgui_utils.cpp": "Low-level ImGui helpers, styling glue, and reusable drawing utilities.",
    "installer_wizard.cpp": "Installer-screen flow and high-fidelity onboarding UI behavior.",
    "message_window.cpp": "Message/prompt window display, input gating, and modal sequencing.",
    "options_menu.cpp": "Options menu state, categories, navigation, values, and confirmation logic.",
    "options_menu_thumbnails.cpp": "Thumbnail-backed options UI visual content and selection helpers.",
    "tv_static.cpp": "TV static overlay animation/effect logic.",
    "CHudPause_patches.cpp": "Pause HUD interception and pause-menu behavior overrides.",
    "CTitleStateIntro_patches.cpp": "Title intro hook layer and intro-state flow adjustments.",
    "CTitleStateMenu_patches.cpp": "Title menu hook layer and menu-state flow adjustments.",
    "CGameModeStageTitle_patches.cpp": "Stage title integration hooks tied to original game mode.",
    "aspect_ratio_patches.cpp": "UI scaling, safe-area, and ultrawide/aspect-ratio compensation.",
    "input_patches.cpp": "Input handling patches and controller-related behavior changes.",
}

CLASS_RE = re.compile(r"^\s*(?:class|struct)\s+([A-Za-z_]\w*)", re.MULTILINE)
ENUM_RE = re.compile(r"^\s*enum(?:\s+class)?\s+([A-Za-z_]\w*)?", re.MULTILINE)
FUNCTION_RE = re.compile(
    r"^\s*(?:inline\s+)?(?:static\s+)?(?:virtual\s+)?(?:constexpr\s+)?"
    r"(?:[\w:<>~*&]+\s+)+([A-Za-z_~]\w*(?:::\w+)*)\s*\([^;{}]*\)\s*"
    r"(?:const\b)?\s*(?:override\b)?\s*(?:final\b)?\s*(?:noexcept\b)?\s*(?:\{|$)",
    re.MULTILINE,
)
STATE_WORD_RE = re.compile(
    r"\b([A-Za-z_]\w*(?:State|Status|Mode|Phase|Transition|Fade|Overlay|Window|Guide|Menu))\b"
)
CASE_RE = re.compile(r"^\s*case\s+([^:]+)\s*:", re.MULTILINE)


def iter_source_files(repo_root: Path):
    for subdir in SOURCE_DIRS:
        root = repo_root / subdir
        if not root.exists():
            continue
        for path in sorted(root.rglob("*")):
            if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES:
                yield path


def line_number(text: str, index: int) -> int:
    return text.count("\n", 0, index) + 1


def unique_ordered(items):
    seen = set()
    output = []
    for item in items:
        if not item or item in seen:
            continue
        seen.add(item)
        output.append(item)
    return output


def summarize_file(name: str, matches: dict[str, int]) -> str:
    if name in SUMMARY_HINTS:
        return SUMMARY_HINTS[name]
    dominant = [term for term, _count in sorted(matches.items(), key=lambda item: (-item[1], item[0]))[:3]]
    if dominant:
        return f"Relevant UI/UX source with emphasis on: {', '.join(dominant)}."
    return "Relevant source file with UI/UX-adjacent behavior."


def scan_file(repo_root: Path, path: Path) -> dict:
    text = path.read_text(encoding="utf-8", errors="ignore")
    lower = text.lower()
    term_counts = {term: lower.count(term) for term in UI_TERMS if lower.count(term)}

    classes = unique_ordered(CLASS_RE.findall(text))
    enums = unique_ordered(name or "<anonymous>" for name in ENUM_RE.findall(text))
    functions = []
    for match in FUNCTION_RE.finditer(text):
        name = match.group(1)
        if name in {"if", "for", "while", "switch", "return"}:
            continue
        functions.append({"name": name, "line": line_number(text, match.start())})

    update_draw_like = []
    for function in functions:
        if any(token in function["name"] for token in ("Update", "Draw", "Render", "Init", "Enter", "Exit", "Open", "Close")):
            update_draw_like.append(function)

    input_lines = []
    timing_lines = []
    menu_item_lines = []
    for lineno, line in enumerate(text.splitlines(), start=1):
        lowered = line.lower()
        if any(token in lowered for token in ("input", "button", "pad", "controller", "key", "mouse")):
            input_lines.append({"line": lineno, "text": line.strip()[:240]})
        if any(token in lowered for token in ("timer", "time", "duration", "delay", "frame", "fade", "anim", "transition")):
            timing_lines.append({"line": lineno, "text": line.strip()[:240]})
        if any(token in lowered for token in ("menu", "item", "thumbnail")) and any(token in lowered for token in ("array", "{", "vector", "span")):
            menu_item_lines.append({"line": lineno, "text": line.strip()[:240]})

    state_words = unique_ordered(STATE_WORD_RE.findall(text))
    switch_cases = unique_ordered(case.strip() for case in CASE_RE.findall(text))
    state_candidates = unique_ordered(state_words + switch_cases)

    return {
        "path": path.relative_to(repo_root).as_posix(),
        "summary": summarize_file(path.name, term_counts),
        "term_counts": dict(sorted(term_counts.items())),
        "classes": classes,
        "enums": enums,
        "functions": sorted(functions, key=lambda item: (item["name"], item["line"])),
        "update_draw_render_methods": sorted(update_draw_like, key=lambda item: (item["name"], item["line"])),
        "state_candidates": state_candidates,
        "input_lines": input_lines[:40],
        "timing_lines": timing_lines[:40],
        "menu_item_lines": menu_item_lines[:40],
        "line_count": text.count("\n") + 1,
    }


def build_index(repo_root: Path) -> dict:
    files = [scan_file(repo_root, path) for path in iter_source_files(repo_root)]
    files.sort(key=lambda item: item["path"])

    term_counter: Counter[str] = Counter()
    symbol_counter: Counter[str] = Counter()
    for item in files:
        term_counter.update(item["term_counts"])
        symbol_counter.update(item["state_candidates"])

    return {
        "repo_root": repo_root.resolve().as_posix(),
        "scanned_dirs": SOURCE_DIRS,
        "ui_terms": UI_TERMS,
        "file_count": len(files),
        "top_terms": dict(sorted(term_counter.items(), key=lambda item: (-item[1], item[0]))[:50]),
        "top_state_candidates": dict(sorted(symbol_counter.items(), key=lambda item: (-item[1], item[0]))[:100]),
        "files": files,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Scan UI-adjacent source files and emit deterministic JSON.")
    parser.add_argument("--repo-root", default=".", help="Path to the UnleashedRecomp source root.")
    parser.add_argument(
        "--output",
        default="research_uiux/data/ui_code_index.json",
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
