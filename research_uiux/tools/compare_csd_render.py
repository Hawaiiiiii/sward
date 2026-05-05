"""Phase 288: side-by-side and per-pixel diff harness.

Compares a render produced by the human-readable port (via
`render_csd_scene.py`) against a reference image captured from the
UnleashedRecomp window (via `capture_unleashed_recomp_window.ps1` or
any other screen capture tool that emits a PNG).

Outputs three artifacts:
  * `<output_stem>_sxs.png`   - side-by-side composite with labels.
  * `<output_stem>_diff.png`  - per-pixel red-channel diff scaled by
                                magnitude (black = match, red = miss).
  * `<output_stem>_report.json` - similarity metrics + metadata.

If the two images differ in resolution, the reference is letterbox-fit
into the same canvas as the render for the diff (preserving aspect).
This is the right behavior for first-iteration parity work where the
two pipelines produce HUD pixels at different native resolutions.

Usage:
    python compare_csd_render.py \\
        --port-render path/to/our_render.png \\
        --reference   path/to/unleashed_recomp_capture.png \\
        --label-port  "Human-readable C++ port (Phase 287)" \\
        --label-ref   "UnleashedRecomp runtime capture" \\
        --output-stem out/diff/so_speed_gauge
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


def load_rgba(path: Path) -> Image.Image:
    img = Image.open(path)
    img.load()
    if img.mode != "RGBA":
        img = img.convert("RGBA")
    return img


def fit_letterbox(src: Image.Image, target_w: int, target_h: int) -> Image.Image:
    """Resize src to fit inside (target_w, target_h) preserving aspect, padding
    the rest with transparent black."""
    sw, sh = src.size
    if sw == target_w and sh == target_h:
        return src.copy()
    scale = min(target_w / sw, target_h / sh)
    new_w = max(1, int(round(sw * scale)))
    new_h = max(1, int(round(sh * scale)))
    resized = src.resize((new_w, new_h), Image.LANCZOS)
    canvas = Image.new("RGBA", (target_w, target_h), (0, 0, 0, 0))
    pad_x = (target_w - new_w) // 2
    pad_y = (target_h - new_h) // 2
    canvas.alpha_composite(resized, dest=(pad_x, pad_y))
    return canvas


def fit_logical_canvas(src: Image.Image, logical_w: int, logical_h: int) -> Image.Image:
    """Map a runtime window capture (potentially non-16:9) into the game's
    logical UI canvas. Sonic Unleashed's HUD is authored at 1280x720 (16:9);
    if the runtime window has a different aspect, the game letterboxes its
    16:9 viewport inside that window. Inverse: crop the captured window to
    the centered 16:9 region, then resize to (logical_w, logical_h)."""
    sw, sh = src.size
    target_aspect = logical_w / logical_h
    src_aspect = sw / sh
    if abs(src_aspect - target_aspect) < 1e-6:
        return src.resize((logical_w, logical_h), Image.LANCZOS)
    if src_aspect > target_aspect:
        # Window is wider than 16:9 -> game letterboxes vertically? No: game
        # always renders 16:9; with a wider window the 16:9 viewport is
        # centered with vertical full-height and horizontal pillarbox bars.
        new_w = int(round(sh * target_aspect))
        crop_x = (sw - new_w) // 2
        cropped = src.crop((crop_x, 0, crop_x + new_w, sh))
    else:
        # Window is taller than 16:9 -> 16:9 viewport is full-width with
        # letterbox bars top/bottom.
        new_h = int(round(sw / target_aspect))
        crop_y = (sh - new_h) // 2
        cropped = src.crop((0, crop_y, sw, crop_y + new_h))
    return cropped.resize((logical_w, logical_h), Image.LANCZOS)


def build_diff(a: Image.Image, b: Image.Image) -> tuple[Image.Image, dict]:
    """Per-pixel absolute diff. Returns (diff_image, metrics)."""
    assert a.size == b.size, "diff inputs must be same size"
    a_data = a.tobytes()
    b_data = b.tobytes()
    n = len(a_data)
    diff = bytearray(n)
    sum_abs = 0
    max_abs = 0
    differing = 0
    # Process in groups of 4 (RGBA channels).
    for i in range(0, n, 4):
        ar, ag, ab, aa = a_data[i], a_data[i + 1], a_data[i + 2], a_data[i + 3]
        br, bg, bb, ba = b_data[i], b_data[i + 1], b_data[i + 2], b_data[i + 3]
        dr = abs(ar - br)
        dg = abs(ag - bg)
        db = abs(ab - bb)
        da = abs(aa - ba)
        m = max(dr, dg, db, da)
        diff[i] = m  # red = magnitude
        diff[i + 1] = 0
        diff[i + 2] = 0
        diff[i + 3] = 255
        sum_abs += dr + dg + db + da
        if m > max_abs:
            max_abs = m
        if m > 0:
            differing += 1
    pixel_count = n // 4
    metrics = {
        "pixel_count": pixel_count,
        "differing_pixels": differing,
        "differing_fraction": differing / pixel_count if pixel_count else 0.0,
        "mean_abs_diff_per_channel": (sum_abs / (n)) if n else 0.0,
        "max_abs_diff": max_abs,
        "perfect_match": differing == 0,
    }
    return Image.frombytes("RGBA", a.size, bytes(diff)), metrics


def label_strip(width: int, height: int, text: str, fill_bg=(0, 0, 0, 220), fill_fg=(255, 255, 255, 255)) -> Image.Image:
    img = Image.new("RGBA", (width, height), fill_bg)
    draw = ImageDraw.Draw(img)
    try:
        font = ImageFont.truetype("arial.ttf", max(12, height - 8))
    except Exception:
        font = ImageFont.load_default()
    draw.text((8, (height - max(12, height - 8)) // 2), text, font=font, fill=fill_fg)
    return img


def build_side_by_side(
    port: Image.Image, ref: Image.Image, diff: Image.Image,
    label_port: str, label_ref: str, label_diff: str,
) -> Image.Image:
    w, h = port.size
    label_h = 28
    gap = 8
    panel_w = w
    panel_h = h + label_h
    total_w = panel_w * 3 + gap * 2
    total_h = panel_h
    canvas = Image.new("RGBA", (total_w, total_h), (24, 24, 24, 255))
    for i, (img, label) in enumerate([(port, label_port), (ref, label_ref), (diff, label_diff)]):
        x = i * (panel_w + gap)
        canvas.alpha_composite(label_strip(panel_w, label_h, label), dest=(x, 0))
        canvas.alpha_composite(img, dest=(x, label_h))
    return canvas


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port-render", required=True, help="PNG produced by render_csd_scene.py")
    parser.add_argument("--reference", required=True, help="Reference PNG (UnleashedRecomp capture or other ground truth)")
    parser.add_argument("--label-port", default="Human-readable port")
    parser.add_argument("--label-ref", default="Reference")
    parser.add_argument("--label-diff", default="Per-pixel diff (red=mismatch)")
    parser.add_argument("--output-stem", required=True, help="Output stem; produces _sxs.png, _diff.png, _report.json")
    parser.add_argument(
        "--logical-canvas-fit", action="store_true",
        help="Treat the reference as a runtime window capture: crop the centered "
             "16:9 viewport (matching the game's letterboxed render area) and "
             "resize to the port render's size. Use this when the window aspect "
             "differs from the 16:9 logical UI canvas.",
    )
    args = parser.parse_args(argv)

    port = load_rgba(Path(args.port_render))
    ref = load_rgba(Path(args.reference))
    if args.logical_canvas_fit:
        ref_fit = fit_logical_canvas(ref, port.size[0], port.size[1])
    elif ref.size != port.size:
        ref_fit = fit_letterbox(ref, port.size[0], port.size[1])
    else:
        ref_fit = ref

    diff_img, metrics = build_diff(port, ref_fit)
    sxs = build_side_by_side(port, ref_fit, diff_img, args.label_port, args.label_ref, args.label_diff)

    stem = Path(args.output_stem)
    stem.parent.mkdir(parents=True, exist_ok=True)
    sxs_path = stem.with_name(stem.name + "_sxs.png")
    diff_path = stem.with_name(stem.name + "_diff.png")
    report_path = stem.with_name(stem.name + "_report.json")
    sxs.save(sxs_path, "PNG")
    diff_img.save(diff_path, "PNG")
    report = {
        "phase": "288",
        "purpose": "Side-by-side + per-pixel diff between human-readable port render and a reference (intended for UnleashedRecomp captures).",
        "port_render": str(Path(args.port_render).resolve()),
        "reference": str(Path(args.reference).resolve()),
        "port_size": list(port.size),
        "reference_size_native": list(ref.size),
        "reference_size_fitted": list(ref_fit.size),
        "label_port": args.label_port,
        "label_ref": args.label_ref,
        "metrics": metrics,
        "outputs": {
            "side_by_side": str(sxs_path.resolve()),
            "diff": str(diff_path.resolve()),
            "report": str(report_path.resolve()),
        },
    }
    with report_path.open("w", encoding="utf-8") as f:
        json.dump(report, f, indent=2)
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
