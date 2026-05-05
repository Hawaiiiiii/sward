"""Phase 287: First-pixel renderer for the human-readable port.

Consumes the YNCP native component map's `scene_draw_commands` (the
ground-truth dataset that the C++ loader was already parity-validated
against in Phase 281), pulls the referenced retail DDS textures from
the user's full-install asset extraction, applies each cast's
UV crop + scene transform, and composites the result into a PNG.

This is the first time the human-readable port draws a Sonic Unleashed
UI pixel on its own -- without going through UnleashedRecomp, the
PPC->C++ translated runtime, or any other intermediary. Pixel data
flows: retail .yncp -> Python parser -> draw command list -> retail
.dds -> Pillow composite -> output PNG.

Usage:
    python render_csd_scene.py \
        --project game/Sonic/ui_playscreen.yncp \
        --scene so_speed_gauge \
        --output out/so_speed_gauge.png

The --canvas option picks the logical HUD resolution (default 1280x720,
which matches Sonic Unleashed's native HUD canvas: scene_width*1280
recovers source_width and scene_height*720 recovers source_height for
every command in the retail dataset).
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Iterable

from PIL import Image


REPO_ROOT = Path(__file__).resolve().parents[2]
COMPONENT_MAP_PATH = REPO_ROOT / "research_uiux" / "data" / "yncp_native_component_map.json"
ASSET_ROOT = REPO_ROOT / "extracted_assets" / "full_install_archives"
DEFAULT_CANVAS_W = 1280
DEFAULT_CANVAS_H = 720


def load_component_map() -> dict[str, Any]:
    with COMPONENT_MAP_PATH.open("r", encoding="utf-8") as f:
        return json.load(f)


def find_project(component_map: dict[str, Any], project_relpath: str) -> dict[str, Any]:
    needle = project_relpath.replace("\\", "/")
    for group in component_map["screen_groups"].values():
        for entry in group:
            if entry.get("relative_path") == needle:
                return entry
    raise SystemExit(f"project not found in component map: {project_relpath}")


def list_scenes(project_entry: dict[str, Any]) -> list[str]:
    seen: list[str] = []
    seen_set: set[str] = set()
    for cmd in project_entry.get("scene_draw_commands", []):
        name = cmd.get("scene_name", "")
        if name and name not in seen_set:
            seen_set.add(name)
            seen.append(name)
    return seen


def filter_scene_commands(
    project_entry: dict[str, Any], scene_name: str
) -> list[dict[str, Any]]:
    cmds = [
        c for c in project_entry.get("scene_draw_commands", [])
        if c.get("scene_name") == scene_name
    ]
    cmds.sort(key=lambda c: (c.get("draw_order", 0), c.get("group_index", 0), c.get("cast_index", 0)))
    return cmds


def resolve_texture_path(cmd: dict[str, Any]) -> Path | None:
    rel = cmd.get("texture_relative_path", "")
    if not rel:
        return None
    return ASSET_ROOT / rel


def load_texture_cached(path: Path, cache: dict[Path, Image.Image]) -> Image.Image | None:
    if path in cache:
        return cache[path]
    try:
        img = Image.open(path)
        img.load()
    except Exception as exc:
        print(f"  ! could not open texture {path}: {exc}", file=sys.stderr)
        cache[path] = None  # type: ignore[assignment]
        return None
    if img.mode != "RGBA":
        img = img.convert("RGBA")
    cache[path] = img
    return img


def crop_uv_region(texture: Image.Image, cmd: dict[str, Any]) -> Image.Image:
    tex_w, tex_h = texture.size
    # Prefer pixel-space source_x/y/width/height when present (integer-exact),
    # falling back to UVs scaled by the recorded source_texture_width/height.
    sx = cmd.get("source_x")
    sy = cmd.get("source_y")
    sw = cmd.get("source_width")
    sh = cmd.get("source_height")
    if sw and sh:
        left = int(sx or 0)
        top = int(sy or 0)
        right = left + int(sw)
        bottom = top + int(sh)
    else:
        u0 = float(cmd.get("uv_left", 0.0))
        v0 = float(cmd.get("uv_top", 0.0))
        u1 = float(cmd.get("uv_right", 1.0))
        v1 = float(cmd.get("uv_bottom", 1.0))
        left = int(round(u0 * tex_w))
        top = int(round(v0 * tex_h))
        right = int(round(u1 * tex_w))
        bottom = int(round(v1 * tex_h))
    left = max(0, min(tex_w, left))
    right = max(left, min(tex_w, right))
    top = max(0, min(tex_h, top))
    bottom = max(top, min(tex_h, bottom))
    if right <= left or bottom <= top:
        return Image.new("RGBA", (1, 1), (0, 0, 0, 0))
    return texture.crop((left, top, right, bottom))


def scene_bounds(cmds: Iterable[dict[str, Any]]) -> tuple[float, float, float, float]:
    """Return (min_x, min_y, max_x, max_y) of the union of cast rects in
    normalized scene coordinates: (base_translation + scene_offset, +size)."""
    xs0: list[float] = []
    ys0: list[float] = []
    xs1: list[float] = []
    ys1: list[float] = []
    for c in cmds:
        if not c.get("has_texture", False) or c.get("hide_flag", 0):
            continue
        x0 = float(c.get("base_translation_x", 0.0)) + float(c.get("scene_left", 0.0))
        y0 = float(c.get("base_translation_y", 0.0)) + float(c.get("scene_top", 0.0))
        x1 = x0 + float(c.get("scene_width", 0.0)) * float(c.get("base_scale_x", 1.0))
        y1 = y0 + float(c.get("scene_height", 0.0)) * float(c.get("base_scale_y", 1.0))
        xs0.append(x0); ys0.append(y0); xs1.append(x1); ys1.append(y1)
    if not xs0:
        return (0.0, 0.0, 1.0, 1.0)
    return (min(xs0), min(ys0), max(xs1), max(ys1))


def composite_command(
    canvas: Image.Image,
    cmd: dict[str, Any],
    texture_cache: dict[Path, Image.Image],
    canvas_w: int,
    canvas_h: int,
    *,
    fit_offset: tuple[float, float] = (0.0, 0.0),
    fit_scale: tuple[float, float] = (1.0, 1.0),
) -> bool:
    if cmd.get("hide_flag", 0):
        return False
    if not cmd.get("has_texture", False):
        return False
    tex_path = resolve_texture_path(cmd)
    if tex_path is None or not tex_path.exists():
        return False
    texture = load_texture_cached(tex_path, texture_cache)
    if texture is None:
        return False

    crop = crop_uv_region(texture, cmd)

    # Apply per-cast base scale first (data shows base_rotation == 0 for
    # every retail HUD draw command observed; rotation can land in v2).
    sx = float(cmd.get("base_scale_x", 1.0))
    sy = float(cmd.get("base_scale_y", 1.0))

    # Scene rect in normalized (1280x720) space, post group scale and the
    # outer fit-to-bounds zoom.
    scene_w = float(cmd.get("scene_width", 0.0)) * sx * fit_scale[0]
    scene_h = float(cmd.get("scene_height", 0.0)) * sy * fit_scale[1]
    dst_w = max(1, int(round(scene_w * canvas_w)))
    dst_h = max(1, int(round(scene_h * canvas_h)))
    if (dst_w, dst_h) != crop.size:
        crop = crop.resize((dst_w, dst_h), Image.NEAREST)

    # World position = base_translation + scene_offset, then fit-to-bounds.
    bx = float(cmd.get("base_translation_x", 0.0))
    by = float(cmd.get("base_translation_y", 0.0))
    sl = float(cmd.get("scene_left", 0.0))
    st = float(cmd.get("scene_top", 0.0))
    norm_x = (bx + sl + fit_offset[0]) * fit_scale[0]
    norm_y = (by + st + fit_offset[1]) * fit_scale[1]
    world_x = norm_x * canvas_w
    world_y = norm_y * canvas_h
    px = int(round(world_x))
    py = int(round(world_y))

    canvas.alpha_composite(crop, dest=(px, py))
    return True


def render_scene(
    project_entry: dict[str, Any],
    scene_name: str,
    output_path: Path,
    canvas_w: int,
    canvas_h: int,
    background: tuple[int, int, int, int],
    fit_to_bounds: bool,
    margin_frac: float = 0.05,
) -> dict[str, Any]:
    cmds = filter_scene_commands(project_entry, scene_name)
    if not cmds:
        raise SystemExit(f"scene not found in project: {scene_name}")

    fit_offset = (0.0, 0.0)
    fit_scale = (1.0, 1.0)
    bounds = scene_bounds(cmds)
    if fit_to_bounds:
        bx0, by0, bx1, by1 = bounds
        bw = max(1e-9, bx1 - bx0)
        bh = max(1e-9, by1 - by0)
        # Translate so bounds.min lands at (margin, margin), then scale so
        # bounds extent fills (1 - 2*margin) of the normalized canvas.
        fit_offset = (-bx0, -by0)
        avail_x = (1.0 - 2.0 * margin_frac) / bw
        avail_y = (1.0 - 2.0 * margin_frac) / bh
        # Uniform scale to preserve aspect.
        s = min(avail_x, avail_y)
        fit_scale = (s, s)
        # Re-center: shift so the scaled bounds box is centered in the canvas.
        scaled_w = bw * s
        scaled_h = bh * s
        center_pad_x = (1.0 - scaled_w) / 2.0
        center_pad_y = (1.0 - scaled_h) / 2.0
        # fit_offset is applied BEFORE fit_scale in composite_command; to add a
        # post-scale centering pad, fold it into fit_offset by dividing by scale.
        fit_offset = (fit_offset[0] + center_pad_x / s, fit_offset[1] + center_pad_y / s)

    canvas = Image.new("RGBA", (canvas_w, canvas_h), background)
    texture_cache: dict[Path, Image.Image] = {}
    drawn = 0
    skipped = 0
    for cmd in cmds:
        if composite_command(
            canvas, cmd, texture_cache, canvas_w, canvas_h,
            fit_offset=fit_offset, fit_scale=fit_scale,
        ):
            drawn += 1
        else:
            skipped += 1

    output_path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output_path, "PNG")
    return {
        "commands_total": len(cmds),
        "commands_drawn": drawn,
        "commands_skipped": skipped,
        "textures_used": len(texture_cache),
        "output_path": str(output_path),
        "scene_bounds_normalized": {
            "min_x": bounds[0], "min_y": bounds[1],
            "max_x": bounds[2], "max_y": bounds[3],
        },
        "fit_to_bounds": fit_to_bounds,
        "fit_offset": list(fit_offset),
        "fit_scale": list(fit_scale),
    }


def render_all_projects(
    component_map: dict[str, Any],
    output_root: Path,
    canvas_w: int,
    canvas_h: int,
    background: tuple[int, int, int, int],
    fit_to_bounds: bool,
    manifest_path: Path | None,
) -> dict[str, Any]:
    """Render every scene in every project under screen_groups, writing PNGs
    to {output_root}/{screen_group}/{project_stem}/{scene}.png and an
    aggregate manifest JSON describing per-project pixel-emission stats."""
    output_root.mkdir(parents=True, exist_ok=True)
    project_reports: list[dict[str, Any]] = []
    totals = {
        "projects": 0, "scenes": 0,
        "commands_total": 0, "commands_drawn": 0, "commands_skipped": 0,
        "scenes_with_zero_drawn": 0,
    }
    for group_name, entries in component_map["screen_groups"].items():
        for entry in entries:
            rel = entry.get("relative_path", "")
            if not rel:
                continue
            stem = Path(rel).stem
            scene_names = list_scenes(entry)
            scene_reports: list[dict[str, Any]] = []
            for scene_name in scene_names:
                out_dir = output_root / group_name / stem
                out_path = out_dir / f"{scene_name}.png"
                try:
                    summary = render_scene(
                        entry, scene_name, out_path,
                        canvas_w, canvas_h, background, fit_to_bounds,
                    )
                except SystemExit as e:
                    summary = {"error": str(e), "output_path": str(out_path)}
                scene_reports.append({"scene": scene_name, **summary})
                totals["scenes"] += 1
                totals["commands_total"] += summary.get("commands_total", 0)
                totals["commands_drawn"] += summary.get("commands_drawn", 0)
                totals["commands_skipped"] += summary.get("commands_skipped", 0)
                if summary.get("commands_drawn", 0) == 0:
                    totals["scenes_with_zero_drawn"] += 1
            project_reports.append({
                "screen_group": group_name,
                "project_relative_path": rel,
                "scene_count": len(scene_names),
                "scenes": scene_reports,
            })
            totals["projects"] += 1
    manifest = {
        "phase": "287",
        "purpose": "First-pixel emission proof: parsed CSD bytes -> retail DDS -> Pillow -> PNG, one image per scene per project.",
        "canvas_width": canvas_w,
        "canvas_height": canvas_h,
        "fit_to_bounds": fit_to_bounds,
        "totals": totals,
        "projects": project_reports,
    }
    if manifest_path is not None:
        manifest_path.parent.mkdir(parents=True, exist_ok=True)
        with manifest_path.open("w", encoding="utf-8") as f:
            json.dump(manifest, f, indent=2)
    return manifest


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--project", help="Project relative path under extracted_assets/full_install_archives/, e.g. game/Sonic/ui_playscreen.yncp")
    parser.add_argument("--scene", help="Scene name to render. Omit (with --project) to list scenes.")
    parser.add_argument("--output", help="Output PNG path (single-scene mode).")
    parser.add_argument("--all", action="store_true", help="Batch mode: render every scene in every project.")
    parser.add_argument("--out-dir", help="Output directory root for --all batch mode.")
    parser.add_argument("--manifest", help="Path to write batch manifest JSON.")
    parser.add_argument("--canvas-width", type=int, default=DEFAULT_CANVAS_W)
    parser.add_argument("--canvas-height", type=int, default=DEFAULT_CANVAS_H)
    parser.add_argument("--background", choices=["transparent", "black", "magenta"], default="transparent")
    parser.add_argument(
        "--fit-to-bounds",
        action="store_true",
        help="Translate + uniformly scale all casts so the scene's bounding box fills the canvas. "
             "Use this for first-pixel verification when the world coordinate convention is unknown.",
    )
    args = parser.parse_args(argv)

    bg_map = {
        "transparent": (0, 0, 0, 0),
        "black": (0, 0, 0, 255),
        "magenta": (255, 0, 255, 255),
    }
    cmap = load_component_map()

    if args.all:
        if not args.out_dir:
            print("--out-dir is required when --all is provided", file=sys.stderr)
            return 2
        manifest = render_all_projects(
            cmap, Path(args.out_dir),
            args.canvas_width, args.canvas_height,
            bg_map[args.background], args.fit_to_bounds,
            Path(args.manifest) if args.manifest else None,
        )
        t = manifest["totals"]
        print(json.dumps({
            "projects": t["projects"], "scenes": t["scenes"],
            "commands_total": t["commands_total"], "commands_drawn": t["commands_drawn"],
            "commands_skipped": t["commands_skipped"],
            "scenes_with_zero_drawn": t["scenes_with_zero_drawn"],
            "manifest": args.manifest,
        }, indent=2))
        return 0

    if not args.project:
        parser.error("--project is required (or use --all for batch mode)")

    project_entry = find_project(cmap, args.project)

    if args.scene is None:
        scenes = list_scenes(project_entry)
        print(f"project: {args.project}")
        print(f"scenes ({len(scenes)}):")
        for s in scenes:
            print(f"  - {s}")
        return 0

    if not args.output:
        print("--output is required when --scene is provided", file=sys.stderr)
        return 2

    summary = render_scene(
        project_entry,
        args.scene,
        Path(args.output),
        args.canvas_width,
        args.canvas_height,
        bg_map[args.background],
        args.fit_to_bounds,
    )
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
