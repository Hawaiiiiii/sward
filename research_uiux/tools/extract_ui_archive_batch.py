#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def resolve_input(asset_root: Path, archive: str) -> Path:
    path = asset_root / archive
    if path.suffix.lower() == ".arl":
        pair = path.with_suffix(".ar.00")
        if pair.exists():
            return pair
    return path


def output_dir_for(output_root: Path, asset_root: Path, source_path: Path) -> Path:
    relative = source_path.relative_to(asset_root)
    if source_path.name.lower().endswith(".ar.00"):
        stem = source_path.name[:-6]
    else:
        stem = source_path.stem
    return output_root.joinpath(*relative.parts[:-1], stem)


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract a batch of UI-heavy archives.")
    parser.add_argument("--tool", required=True, help="Path to HedgeArcPack executable or wrapper.")
    parser.add_argument("--asset-root", required=True, help="Root containing source archives.")
    parser.add_argument("--output-root", required=True, help="Root for extracted output.")
    parser.add_argument("--archive", action="append", required=True, help="Relative archive path. May be passed multiple times.")
    args = parser.parse_args()

    tool = Path(args.tool).resolve()
    asset_root = Path(args.asset_root).resolve()
    output_root = Path(args.output_root).resolve()
    output_root.mkdir(parents=True, exist_ok=True)

    for archive in args.archive:
        source_path = resolve_input(asset_root, archive)
        if not source_path.exists():
            raise FileNotFoundError(f"Archive not found: {archive}")
        output_dir = output_dir_for(output_root, asset_root, source_path)
        output_dir.mkdir(parents=True, exist_ok=True)
        command = [str(tool), str(source_path), str(output_dir), "-E"]
        print(" ".join(command))
        subprocess.run(command, check=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
