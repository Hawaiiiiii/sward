"""Phase 300: build a single composite "A/B catalog" PNG that shows
every CSD project rendered by the in-process A/B harness in a grid,
with one panel per project's most-content-rich scene.

Operates on the directory the Phase 298/299 csd_overlay_patches.cpp
emits to: `<install>/out/csd_overlay_evidence/`. For each unique
project name (everything before `__`), pick the largest non-empty
PNG (by alpha-pixel count) so background-heavy "bg" panels don't
crowd out scenes with actual UI content.

Output is a single tracked PNG plus a JSON manifest listing which
files were included. Useful as a one-glance proof that the
human-readable port can render every Sonic Unleashed UI screen.
"""

from __future__ import annotations

import argparse
import json
import math
from collections import defaultdict
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


def alpha_pixel_count(path: Path) -> int:
    try:
        with Image.open(path) as im:
            im = im.convert("RGBA")
            data = im.tobytes()
            return sum(1 for i in range(3, len(data), 4) if data[i] > 0)
    except Exception:
        return 0


def best_per_project(evidence_dir: Path) -> dict[str, Path]:
    by_project: dict[str, list[tuple[Path, int]]] = defaultdict(list)
    for png in evidence_dir.glob("*.png"):
        name = png.stem
        if "__" not in name:
            continue
        proj = name.split("__", 1)[0]
        by_project[proj].append((png, alpha_pixel_count(png)))

    chosen: dict[str, Path] = {}
    for proj, candidates in by_project.items():
        candidates.sort(key=lambda x: -x[1])
        if candidates and candidates[0][1] > 0:
            chosen[proj] = candidates[0][0]
        elif candidates:
            chosen[proj] = candidates[0][0]
    return chosen


def render_catalog(
    chosen: dict[str, Path],
    output_path: Path,
    panel_w: int = 480,
    panel_h: int = 270,
    cols: int = 3,
    label_h: int = 32,
    bg=(20, 20, 20, 255),
    pad=12,
) -> dict:
    sorted_projects = sorted(chosen.keys())
    rows = math.ceil(len(sorted_projects) / cols)
    total_w = cols * panel_w + (cols + 1) * pad
    total_h = rows * (panel_h + label_h) + (rows + 1) * pad

    canvas = Image.new("RGBA", (total_w, total_h), bg)
    draw = ImageDraw.Draw(canvas)
    try:
        font = ImageFont.truetype("arial.ttf", 16)
    except Exception:
        font = ImageFont.load_default()

    manifest_entries = []
    for idx, proj in enumerate(sorted_projects):
        col = idx % cols
        row = idx // cols
        x = pad + col * (panel_w + pad)
        y = pad + row * (panel_h + label_h + pad)

        # Label strip
        draw.rectangle([x, y, x + panel_w, y + label_h], fill=(0, 0, 0, 255))
        scene = chosen[proj].stem.split("__", 1)[1] if "__" in chosen[proj].stem else ""
        draw.text((x + 8, y + 6), f"{proj}  ({scene})", font=font, fill=(255, 255, 255, 255))

        # Panel
        panel_y = y + label_h
        try:
            with Image.open(chosen[proj]) as im:
                im = im.convert("RGBA")
                im.thumbnail((panel_w, panel_h), Image.LANCZOS)
                px = x + (panel_w - im.width) // 2
                py = panel_y + (panel_h - im.height) // 2
                # White underlay so the transparent canvas pixels show
                # against the dark bg.
                draw.rectangle([x, panel_y, x + panel_w, panel_y + panel_h], fill=(245, 245, 245, 255))
                canvas.alpha_composite(im, dest=(px, py))
        except Exception as e:
            draw.text((x + 16, panel_y + 16), f"(load failed: {e})", font=font, fill=(255, 100, 100, 255))

        manifest_entries.append({
            "project": proj,
            "scene": scene,
            "source_png": str(chosen[proj]),
        })

    output_path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output_path, "PNG")
    return {
        "phase": "300",
        "purpose": "Single-image catalog of every CSD project the in-process A/B harness rendered. Each panel is the project's most-content-rich PNG.",
        "panel_width": panel_w,
        "panel_height": panel_h,
        "columns": cols,
        "rows": rows,
        "total_size": [total_w, total_h],
        "project_count": len(sorted_projects),
        "entries": manifest_entries,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--evidence-dir", required=True, help="csd_overlay_evidence/ directory")
    parser.add_argument("--output", required=True, help="output composite PNG path")
    parser.add_argument("--manifest", help="optional JSON manifest path")
    args = parser.parse_args(argv)

    evidence_dir = Path(args.evidence_dir)
    if not evidence_dir.is_dir():
        print(f"evidence dir not found: {evidence_dir}")
        return 2
    chosen = best_per_project(evidence_dir)
    if not chosen:
        print(f"no PNGs found in {evidence_dir}")
        return 1

    summary = render_catalog(chosen, Path(args.output))
    if args.manifest:
        Path(args.manifest).parent.mkdir(parents=True, exist_ok=True)
        Path(args.manifest).write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps({
        "project_count": summary["project_count"],
        "output": args.output,
        "manifest": args.manifest or "<none>",
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
