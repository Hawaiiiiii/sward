#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

from PIL import Image


DEFAULT_LAYOUTS = [
    "ui_boss_gauge.yncp",
    "ui_boss_name.yncp",
    "ui_result.yncp",
    "ui_result_ex.yncp",
    "ui_itemresult.yncp",
    "ui_saveicon.yncp",
]

FAMILY_MAP = {
    "ui_boss_gauge.yncp": "boss_hud",
    "ui_boss_name.yncp": "boss_hud",
    "ui_result.yncp": "result",
    "ui_result_ex.yncp": "result",
    "ui_itemresult.yncp": "result",
    "ui_saveicon.yncp": "save",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Derive a visual taxonomy from atlas/layout outputs.")
    parser.add_argument("--atlas-index", default="extracted_assets/visual_atlas/atlas_index.json")
    parser.add_argument("--layout-data", default="research_uiux/data/layout_deep_analysis.json")
    parser.add_argument("--output", default="research_uiux/data/visual_taxonomy.json")
    parser.add_argument("--layout", action="append", default=[], help="Specific layout file name to include.")
    return parser.parse_args()


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def to_hex(rgb: tuple[int, int, int]) -> str:
    return "#{:02X}{:02X}{:02X}".format(*rgb)


def quantize_color(rgb: tuple[int, int, int]) -> tuple[int, int, int]:
    return tuple(min(255, (channel // 32) * 32) for channel in rgb)


def summarize_texture(path: Path) -> dict[str, Any]:
    with Image.open(path) as opened:
        image = opened.convert("RGBA")
        image.thumbnail((256, 256), Image.Resampling.LANCZOS)
        pixels = [image.getpixel((x, y)) for y in range(image.height) for x in range(image.width)]

    visible = [pixel for pixel in pixels if pixel[3] >= 16]
    if not visible:
        return {
            "path": path.as_posix(),
            "size": [image.width, image.height],
            "average_color": "#000000",
            "dominant_colors": [],
            "alpha_coverage": 0.0,
            "brightness": 0.0,
        }

    color_counter: Counter[tuple[int, int, int]] = Counter()
    red = green = blue = 0
    for pixel in visible:
        rgb = pixel[:3]
        red += rgb[0]
        green += rgb[1]
        blue += rgb[2]
        color_counter[quantize_color(rgb)] += 1

    count = len(visible)
    average_rgb = (round(red / count), round(green / count), round(blue / count))
    brightness = round(sum(average_rgb) / (3 * 255), 4)
    alpha_coverage = round(count / max(1, len(pixels)), 4)

    dominant = [
        {"hex": to_hex(color), "count": color_count}
        for color, color_count in color_counter.most_common(5)
    ]

    return {
        "path": path.as_posix(),
        "size": [image.width, image.height],
        "average_color": to_hex(average_rgb),
        "dominant_colors": dominant,
        "alpha_coverage": alpha_coverage,
        "brightness": brightness,
    }


def aggregate_palette(texture_summaries: list[dict[str, Any]]) -> list[dict[str, Any]]:
    counter: Counter[str] = Counter()
    for summary in texture_summaries:
        for item in summary["dominant_colors"]:
            counter[item["hex"]] += item["count"]
    return [
        {"hex": color, "count": count}
        for color, count in counter.most_common(6)
    ]


def aggregate_accent_palette(texture_summaries: list[dict[str, Any]]) -> list[dict[str, Any]]:
    counter: Counter[str] = Counter()
    for summary in texture_summaries:
        for item in summary["dominant_colors"]:
            hex_color = item["hex"]
            rgb = tuple(int(hex_color[index:index + 2], 16) for index in (1, 3, 5))
            if max(rgb) - min(rgb) < 24:
                continue
            counter[hex_color] += item["count"]
    return [
        {"hex": color, "count": count}
        for color, count in counter.most_common(6)
    ]


def build_taxonomy(
    atlas_index: dict[str, Any],
    layout_data: dict[str, Any],
    selected_layouts: list[str],
) -> dict[str, Any]:
    atlas_lookup = {entry["layout_file"]: entry for entry in atlas_index["entries"]}
    digest_lookup = {Path(entry["path"]).name: entry for entry in layout_data["digests"]}

    layout_entries: list[dict[str, Any]] = []
    family_entries: dict[str, list[dict[str, Any]]] = defaultdict(list)

    for layout_name in selected_layouts:
        atlas_entry = atlas_lookup.get(layout_name)
        digest = digest_lookup.get(layout_name)
        if not atlas_entry or not digest:
            continue

        texture_summaries = [
            summarize_texture(Path(texture["resolved_path"]))
            for texture in atlas_entry.get("resolved_textures", [])
        ]

        layout_summary = {
            "layout_file": layout_name,
            "family": FAMILY_MAP.get(layout_name, "other"),
            "atlas_path": atlas_entry.get("atlas_path"),
            "scene_names": atlas_entry.get("scene_names", []),
            "texture_names": [texture["texture_name"] for texture in atlas_entry.get("resolved_textures", [])],
            "texture_count": atlas_entry.get("rendered_texture_count", 0),
            "scene_count": digest.get("totals", {}).get("scene_count", 0),
            "font_names": digest.get("font_names", []),
            "animation_family_counts": digest.get("animation_family_counts", {}),
            "longest_animations": digest.get("longest_animations", [])[:4],
            "palette": aggregate_palette(texture_summaries),
            "accent_palette": aggregate_accent_palette(texture_summaries),
            "texture_summaries": texture_summaries,
        }
        layout_entries.append(layout_summary)
        family_entries[layout_summary["family"]].append(layout_summary)

    families: dict[str, Any] = {}
    for family_name, entries in sorted(family_entries.items()):
        family_palette = Counter()
        font_names = sorted({font for entry in entries for font in entry["font_names"]})
        scene_names = sorted({scene for entry in entries for scene in entry["scene_names"]})
        animation_families = Counter()
        for entry in entries:
            for swatch in entry["palette"]:
                family_palette[swatch["hex"]] += swatch["count"]
            for swatch in entry["accent_palette"]:
                family_palette[swatch["hex"]] += swatch["count"]
            for name, count in entry["animation_family_counts"].items():
                animation_families[name] += count
        families[family_name] = {
            "layout_files": [entry["layout_file"] for entry in entries],
            "font_names": font_names,
            "scene_names": scene_names,
            "animation_family_counts": dict(sorted(animation_families.items())),
            "palette": [{"hex": color, "count": count} for color, count in family_palette.most_common(6)],
            "accent_palette": aggregate_accent_palette(
                [texture for entry in entries for texture in entry["texture_summaries"]]
            ),
        }

    return {
        "layout_count": len(layout_entries),
        "selected_layouts": selected_layouts,
        "layouts": layout_entries,
        "families": families,
    }


def main() -> int:
    args = parse_args()
    atlas_index = load_json(Path(args.atlas_index))
    layout_data = load_json(Path(args.layout_data))
    selected_layouts = args.layout or DEFAULT_LAYOUTS
    taxonomy = build_taxonomy(atlas_index, layout_data, selected_layouts)

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(taxonomy, indent=2, sort_keys=True), encoding="utf-8")
    print(json.dumps({"layout_count": taxonomy["layout_count"], "families": sorted(taxonomy["families"].keys())}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
