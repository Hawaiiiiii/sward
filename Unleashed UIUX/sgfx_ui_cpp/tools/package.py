#!/usr/bin/env python3
"""Assemble a clean runtime drop of the viewer.

Copies only what the screens actually load at runtime — the exe, the D3D12 runtime,
the fonts, and the host logo-slot folder — into a drop folder, leaving out the
unused game art atlases, the layout data, and the music. The result carries no game
art. Drop your own logo at assets/gameart/boot_logo.png and a real run status at
sgfx_status.json (see export_status.py / launch.py).

  python package.py --build <build_dir> --out <drop_dir> [--zip]
"""
from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

EXE = "sgfx_screens.exe"
KEEP_ASSET_DIRS = ["fonts", "gameart"]      # fonts = MSDF/OTF; gameart = host logo/photo slots
KEEP_ASSET_FILES = ["ui_font.otf"]


def copy(src: Path, dst: Path) -> None:
    if src.is_dir():
        shutil.copytree(src, dst, dirs_exist_ok=True)
    elif src.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Assemble a clean runtime drop of the viewer.")
    ap.add_argument("--build", required=True, help="the build dir (with the exe, assets/, D3D12/)")
    ap.add_argument("--out", required=True, help="the drop folder to create")
    ap.add_argument("--zip", action="store_true", help="also produce <out>.zip")
    args = ap.parse_args(argv)

    build, out = Path(args.build), Path(args.out)
    exe = build / EXE
    if not exe.exists():
        ap.error(f"no {EXE} in {build} (build it first)")
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)

    copy(exe, out / EXE)
    copy(build / "D3D12", out / "D3D12")
    for d in KEEP_ASSET_DIRS:
        copy(build / "assets" / d, out / "assets" / d)
    for f in KEEP_ASSET_FILES:
        copy(build / "assets" / f, out / "assets" / f)
    (out / "assets" / "gameart").mkdir(parents=True, exist_ok=True)   # host logo slot

    repo = Path(__file__).resolve().parents[1]
    copy(repo / "sgfx_status.example.json", out / "sgfx_status.example.json")
    (out / "README.txt").write_text(
        "Operator UI runtime drop.\n\n"
        "Run sgfx_screens.exe. With no sgfx_status.json present it shows representative\n"
        "defaults. To show a real run, produce sgfx_status.json with export_status.py\n"
        "(or launch.py) and place it beside the exe. Drop your own logo at\n"
        "assets/gameart/boot_logo.png. Game art atlases, layout data and music are\n"
        "intentionally excluded; only the fonts and the D3D12 runtime ship.\n",
        encoding="utf-8",
    )

    n_files = sum(1 for p in out.rglob("*") if p.is_file())
    print(f"drop -> {out}  ({n_files} files)")
    if args.zip:
        z = shutil.make_archive(str(out), "zip", str(out))
        print(f"zip  -> {z}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
