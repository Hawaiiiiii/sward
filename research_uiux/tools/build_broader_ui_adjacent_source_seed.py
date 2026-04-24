#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path


EXPLICIT_CAMERA_PATHS = {
    "Camera/Controller/FreeCamera.cpp",
    "Camera/Controller/GoalCamera.cpp",
    "Camera/Controller/TownShopCamera.cpp",
    "Camera/Controller/TownTalkCamera.cpp",
    "Replay/Camera/ReplayFreeCamera.cpp",
    "Replay/Camera/ReplayRelativeCamera.cpp",
}


def normalize_slashes(value: str) -> str:
    return re.sub(r"/+", "/", value.replace("\\", "/"))


def extract_relative_source_path(raw: str) -> str | None:
    line = raw.strip().strip('"')
    if not line or line.startswith("#"):
        return None

    normalized = normalize_slashes(line)
    lowered = normalized.lower()
    anchor = "/sonicworldadventure/swa/source/"
    if anchor not in lowered:
        return None

    return normalized[lowered.index(anchor) + len(anchor) :].lstrip("/")


def keep_relative_path(relative_path: str) -> bool:
    lowered = relative_path.lower()
    return (
        lowered.startswith("achievement/")
        or lowered.startswith("animation/eventtrigger/")
        or lowered.startswith("camera/")
        or lowered.startswith("hud/")
        or lowered.startswith("system/")
        or lowered.startswith("sequence/")
        or lowered.startswith("inspire/")
        or lowered.startswith("movie/")
        or lowered.startswith("player/parameter/")
        or lowered.startswith("player/switch/")
        or lowered.startswith("sound/")
        or lowered.startswith("town/")
        or lowered.startswith("tool/")
        or lowered.startswith("debug/")
        or lowered.startswith("profile/")
        or lowered.startswith("csd/")
        or lowered.startswith("menu/")
        or lowered.startswith("xml/")
        or lowered.startswith("extrastage/tails/hud/")
        or lowered.startswith("player/character/sonic/hud/")
        or lowered.startswith("player/character/evilsonic/hud/")
        or (lowered.startswith("boss/") and ("hud" in lowered or "nameplate" in lowered))
        or relative_path in EXPLICIT_CAMERA_PATHS
    )


def build_seed_lines(input_path: Path) -> list[str]:
    result: list[str] = []
    seen: set[str] = set()

    for raw in input_path.read_text(encoding="utf-8-sig", errors="ignore").splitlines():
        relative_path = extract_relative_source_path(raw)
        if relative_path is None or not keep_relative_path(relative_path):
            continue
        if relative_path in seen:
            continue
        seen.add(relative_path)
        result.append(raw.strip())

    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a broader UI-adjacent source-path seed from the local Sonic Unleashed path dump.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument(
        "--input",
        default="Match SU OG source code folders and locations.txt",
        help="Local path-dump text file.",
    )
    parser.add_argument(
        "--output",
        default="research_uiux/source_path_seeds/UI_ADJACENT_SOURCE_PATHS_FROM_MATCH_DUMP.txt",
        help="Tracked broader seed output.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    input_path = (repo_root / args.input).resolve()
    output_path = (repo_root / args.output).resolve()

    seed_lines = build_seed_lines(input_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(seed_lines) + "\n", encoding="utf-8")
    print(f"built_broader_ui_adjacent_seed entries={len(seed_lines)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
