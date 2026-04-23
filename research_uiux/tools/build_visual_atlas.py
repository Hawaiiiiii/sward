#!/usr/bin/env python3
"""Build local visual atlas sheets from extracted UI layouts and DDS textures."""

from __future__ import annotations

import argparse
import json
import math
import textwrap
from collections import defaultdict
from pathlib import Path
from typing import Any

try:
    from PIL import Image, ImageDraw, ImageFont, ImageOps
except ModuleNotFoundError as exc:  # pragma: no cover - runtime guard
    raise SystemExit(
        "Pillow is required for build_visual_atlas.py. "
        "Install it locally in this workspace before running the atlas pass."
    ) from exc


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build atlas sheets from extracted UI texture references."
    )
    parser.add_argument(
        "--layout-data",
        default="research_uiux/data/layout_deep_analysis.json",
        help="Path to the deep layout analysis JSON.",
    )
    parser.add_argument(
        "--search-root",
        action="append",
        default=["extracted_assets"],
        help="Root(s) to search for referenced texture files.",
    )
    parser.add_argument(
        "--output-root",
        default="extracted_assets/visual_atlas",
        help="Output root for generated atlas sheets and manifests.",
    )
    parser.add_argument(
        "--tile-size",
        type=int,
        default=224,
        help="Maximum thumbnail size per texture tile.",
    )
    parser.add_argument(
        "--max-textures",
        type=int,
        default=16,
        help="Maximum number of textures to render per layout atlas.",
    )
    return parser.parse_args()


def slugify(value: str) -> str:
    lowered = value.lower()
    return "".join(char if char.isalnum() else "_" for char in lowered).strip("_")


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def load_font(size: int) -> ImageFont.ImageFont | ImageFont.FreeTypeFont:
    try:
        return ImageFont.truetype("arial.ttf", size)
    except OSError:
        return ImageFont.load_default()


def build_texture_index(search_roots: list[Path]) -> dict[str, list[Path]]:
    index: dict[str, list[Path]] = defaultdict(list)
    for root in search_roots:
        if not root.exists():
            continue
        for path in sorted(root.rglob("*.dds")):
            index[path.name.lower()].append(path)
    return index


def choose_texture_path(layout_path: Path, texture_name: str, index: dict[str, list[Path]]) -> Path | None:
    candidates = index.get(texture_name.lower(), [])
    if not candidates:
        return None

    layout_dir = layout_path.parent
    exact_dir = [path for path in candidates if path.parent == layout_dir]
    if exact_dir:
        return sorted(exact_dir)[0]

    def score(path: Path) -> tuple[int, int, str]:
        shared = 0
        for left, right in zip(layout_dir.parts, path.parent.parts):
            if left != right:
                break
            shared += 1
        distance = len(path.parent.parts)
        return (-shared, distance, path.as_posix())

    return sorted(candidates, key=score)[0]


def unwrap_used_textures(digest: dict[str, Any]) -> list[str]:
    ordered: list[str] = []
    seen: set[str] = set()
    scene_summaries = digest.get("scene_summaries", [])
    for scene in scene_summaries:
        for texture_name in scene.get("used_texture_names", []):
            key = texture_name.lower()
            if key in seen:
                continue
            seen.add(key)
            ordered.append(texture_name)

    for texture_name in digest.get("texture_names", []):
        key = texture_name.lower()
        if key in seen:
            continue
        seen.add(key)
        ordered.append(texture_name)

    return ordered


def fit_thumbnail(image: Image.Image, tile_size: int) -> Image.Image:
    source = image.convert("RGBA")
    source.thumbnail((tile_size, tile_size), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (tile_size, tile_size), (17, 22, 28, 255))
    x = (tile_size - source.width) // 2
    y = (tile_size - source.height) // 2
    canvas.paste(source, (x, y), source)
    return canvas


def draw_wrapped(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], text: str, font: ImageFont.ImageFont) -> None:
    left, top, right, bottom = box
    max_width = max(8, right - left)
    characters = max(6, max_width // 7)
    wrapped = textwrap.wrap(text, width=characters)[:3]
    current_y = top
    for line in wrapped:
        if current_y >= bottom:
            break
        draw.text((left, current_y), line, font=font, fill=(220, 228, 236, 255))
        current_y += 14


def render_sheet(
    atlas_title: str,
    layout_relpath: str,
    scenes: list[str],
    resolved_textures: list[dict[str, Any]],
    output_path: Path,
    tile_size: int,
) -> None:
    cols = max(1, min(4, math.ceil(math.sqrt(len(resolved_textures) or 1))))
    rows = max(1, math.ceil((len(resolved_textures) or 1) / cols))
    label_height = 42
    padding = 20
    header_height = 112
    width = (cols * tile_size) + ((cols + 1) * padding)
    height = header_height + (rows * (tile_size + label_height)) + ((rows + 1) * padding)

    image = Image.new("RGBA", (width, height), (8, 12, 18, 255))
    draw = ImageDraw.Draw(image)
    title_font = load_font(26)
    meta_font = load_font(14)
    label_font = load_font(13)

    draw.rounded_rectangle((12, 12, width - 12, height - 12), radius=18, fill=(13, 19, 28, 255), outline=(55, 99, 150, 255), width=2)
    draw.text((24, 22), atlas_title, font=title_font, fill=(242, 246, 250, 255))
    draw.text((24, 56), layout_relpath, font=meta_font, fill=(163, 183, 204, 255))
    scene_text = ", ".join(scenes[:8]) if scenes else "No scenes parsed"
    if len(scenes) > 8:
        scene_text += ", ..."
    draw.text((24, 78), f"Scenes: {scene_text}", font=meta_font, fill=(163, 183, 204, 255))

    for index, entry in enumerate(resolved_textures):
        row = index // cols
        col = index % cols
        x = padding + col * (tile_size + padding)
        y = header_height + padding + row * (tile_size + label_height + padding)

        thumb = fit_thumbnail(entry["image"], tile_size)
        image.paste(thumb, (x, y), thumb)
        draw.rounded_rectangle((x, y, x + tile_size, y + tile_size), radius=12, outline=(55, 99, 150, 255), width=2)
        draw_wrapped(draw, (x, y + tile_size + 8, x + tile_size, y + tile_size + label_height), entry["texture_name"], label_font)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    image.save(output_path)


def analyze_digest(
    digest: dict[str, Any],
    texture_index: dict[str, list[Path]],
    output_root: Path,
    tile_size: int,
    max_textures: int,
) -> dict[str, Any]:
    layout_path = Path(digest["path"])
    texture_names = unwrap_used_textures(digest)[:max_textures]
    resolved: list[dict[str, Any]] = []
    missing: list[str] = []

    for texture_name in texture_names:
        texture_path = choose_texture_path(layout_path, texture_name, texture_index)
        if texture_path is None:
            missing.append(texture_name)
            continue

        try:
            with Image.open(texture_path) as opened:
                resolved.append(
                    {
                        "texture_name": texture_name,
                        "resolved_path": texture_path.as_posix(),
                        "size": list(opened.size),
                        "image": opened.copy(),
                    }
                )
        except Exception:
            missing.append(texture_name)

    slug = slugify(layout_path.stem)
    parent_slug = slugify(layout_path.parent.name)
    atlas_relpath = Path("sheets") / f"{parent_slug}__{slug}.png"
    atlas_path = output_root / atlas_relpath

    if resolved:
        render_sheet(
            atlas_title=layout_path.stem,
            layout_relpath=layout_path.as_posix(),
            scenes=[scene.get("scene_name", "unknown") for scene in digest.get("scene_summaries", [])],
            resolved_textures=resolved,
            output_path=atlas_path,
            tile_size=tile_size,
        )

    for entry in resolved:
        entry.pop("image", None)

    return {
        "layout_file": layout_path.name,
        "layout_path": layout_path.as_posix(),
        "atlas_path": atlas_path.as_posix() if resolved else None,
        "scene_names": [scene.get("scene_name", "unknown") for scene in digest.get("scene_summaries", [])],
        "scene_count": digest.get("totals", {}).get("scene_count", 0),
        "texture_reference_count": len(digest.get("texture_names", [])),
        "rendered_texture_count": len(resolved),
        "missing_texture_count": len(missing),
        "resolved_textures": resolved,
        "missing_textures": missing,
        "animation_family_counts": digest.get("animation_family_counts", {}),
        "longest_animations": digest.get("longest_animations", [])[:4],
    }


def build_index_markdown(entries: list[dict[str, Any]], output_root: Path) -> str:
    lines = [
        "# Visual Atlas Index",
        "",
        "Local-only atlas sheets generated from extracted UI layouts and DDS textures.",
        "",
        f"- output root: `{output_root.as_posix()}`",
        f"- atlases generated: `{sum(1 for entry in entries if entry['atlas_path'])}`",
        f"- layouts analyzed: `{len(entries)}`",
        "",
    ]

    for entry in entries:
        lines.append(f"## {entry['layout_file']}")
        lines.append("")
        lines.append(f"- layout: `{entry['layout_path']}`")
        if entry["atlas_path"]:
            lines.append(f"- atlas: `{entry['atlas_path']}`")
        else:
            lines.append("- atlas: unavailable (no textures resolved)")
        lines.append(f"- scenes: `{entry['scene_count']}`")
        lines.append(f"- rendered textures: `{entry['rendered_texture_count']}`")
        lines.append(f"- missing textures: `{entry['missing_texture_count']}`")
        if entry["scene_names"]:
            lines.append(f"- scene names: `{', '.join(entry['scene_names'][:10])}`")
        lines.append("")
    return "\n".join(lines) + "\n"


def main() -> None:
    args = parse_args()
    layout_data_path = Path(args.layout_data)
    output_root = Path(args.output_root)
    search_roots = [Path(root) for root in args.search_root]

    layout_data = load_json(layout_data_path)
    output_root.mkdir(parents=True, exist_ok=True)
    texture_index = build_texture_index(search_roots)

    entries = [
        analyze_digest(
            digest=digest,
            texture_index=texture_index,
            output_root=output_root,
            tile_size=args.tile_size,
            max_textures=args.max_textures,
        )
        for digest in sorted(layout_data.get("digests", []), key=lambda item: item["path"])
    ]

    manifest = {
        "layout_data": layout_data_path.as_posix(),
        "output_root": output_root.as_posix(),
        "search_roots": [root.as_posix() for root in search_roots],
        "atlas_count": sum(1 for entry in entries if entry["atlas_path"]),
        "layout_count": len(entries),
        "total_rendered_textures": sum(entry["rendered_texture_count"] for entry in entries),
        "entries": entries,
    }
    (output_root / "atlas_index.json").write_text(json.dumps(manifest, indent=2, sort_keys=True), encoding="utf-8")
    (output_root / "INDEX.md").write_text(build_index_markdown(entries, output_root), encoding="utf-8")

    print(json.dumps({"atlas_count": manifest["atlas_count"], "layout_count": manifest["layout_count"], "total_rendered_textures": manifest["total_rendered_textures"]}, indent=2))


if __name__ == "__main__":
    main()
