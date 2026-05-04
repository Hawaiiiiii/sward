#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import math
import re
import struct
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from inspect_xncp_yncp import canonical_digest, cast_name_lookup, parse_fapc


STARTER_PROJECT_GROUPS: dict[str, set[str]] = {
    "title": {"ui_title", "ui_mainmenu"},
    "loading": {"ui_loading", "ui_start"},
    "sonic_hud": {"ui_playscreen", "ui_playscreen_ev", "ui_playscreen_ev_hit", "ui_playscreen_su"},
    "world_map": {"ui_worldmap", "ui_worldmap_help"},
    "pause": {"ui_pause", "ui_general", "ui_help"},
    "result": {"ui_result", "ui_result_ex", "ui_itemresult"},
    "status": {"ui_status"},
    "hub_town": {"ui_townscreen", "ui_balloon", "ui_shop", "ui_gate", "ui_missionscreen", "ui_misson"},
}

COMPONENT_TAGS: dict[str, tuple[str, ...]] = {
    "title_logo": ("title", "logo"),
    "menu_choices": ("menu", "select", "cursor", "new", "continue", "option", "exit"),
    "loading_device": ("loading", "pda", "start", "clear", "save"),
    "hud_life": ("life", "player", "sonic", "chip"),
    "hud_ring": ("ring", "rings"),
    "hud_speed": ("speed", "gauge", "boost", "energy"),
    "hud_score_time": ("score", "time", "count"),
    "world_map_marker": ("world", "map", "marker", "flag", "country", "stage"),
    "pause_shell": ("pause", "window", "footer", "help", "button"),
    "result_rank": ("result", "rank", "score", "bonus", "medal"),
    "status_skill": ("status", "skill", "level", "exp", "xp", "sun", "moon"),
    "town_dialog": ("town", "balloon", "talk", "shop", "mission", "gate"),
}

MAX_PREVIEW_DRAW_COMMANDS_PER_SCENE = 4
MAX_SCENE_DRAW_COMMANDS_PER_SCENE = 96

ANIMATION_TRACKS_SUPPORTED_FOR_PREVIEW: set[str] = {
    "XPosition",
    "YPosition",
    "XScale",
    "YScale",
    "Rotation",
    "HideFlag",
    "SubImage",
}

SFX_CUE_CANDIDATES: list[dict[str, str]] = [
    {
        "screen_group": "title",
        "project": "ui_title",
        "scene": "menu",
        "action": "open title options window",
        "bank_name": "se_system_worldmap",
        "cue_name": "sys_worldmap_window",
        "runtime_hook": "CTitleStateMenu::Update/Game_PlaySound",
        "ghidra_xref": "sub_825882B8 -> Game_PlaySound(\"sys_worldmap_window\")",
        "provenance": "runtime hook hits + Ghidra xrefs exact cue candidate",
    },
    {
        "screen_group": "title",
        "project": "ui_title",
        "scene": "menu",
        "action": "confirm title options",
        "bank_name": "se_system_worldmap",
        "cue_name": "sys_worldmap_decide",
        "runtime_hook": "CTitleStateMenu::Update/Game_PlaySound",
        "ghidra_xref": "sub_825882B8 -> Game_PlaySound(\"sys_worldmap_decide\")",
        "provenance": "runtime hook hits + Ghidra xrefs exact cue candidate",
    },
    {
        "screen_group": "title",
        "project": "ui_title",
        "scene": "menu",
        "action": "cancel title options",
        "bank_name": "se_system_worldmap",
        "cue_name": "sys_worldmap_cansel",
        "runtime_hook": "CTitleStateMenu::Update/Game_PlaySound",
        "ghidra_xref": "sub_825882B8 -> Game_PlaySound(\"sys_worldmap_cansel\")",
        "provenance": "runtime hook hits + Ghidra xrefs exact cue candidate",
    },
    {
        "screen_group": "title",
        "project": "ui_mainmenu",
        "scene": "mm_contentsitem_select",
        "action": "main menu select/open parity candidate",
        "bank_name": "se_system_worldmap",
        "cue_name": "sys_worldmap_decide",
        "runtime_hook": "CTitleStateMenu::Update/Game_PlaySound",
        "ghidra_xref": "sub_825882B8 -> Game_PlaySound(\"sys_worldmap_decide\")",
        "provenance": "runtime hook hits + Ghidra xrefs exact cue candidate",
    },
]


def slug_name(value: str) -> str:
    value = value.lower().replace("\\", "/")
    value = re.sub(r"[^a-z0-9]+", "-", value)
    return value.strip("-") or "unnamed"


def cpp_escape(value: object) -> str:
    text = "" if value is None else str(value)
    return text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", " ")


def join_limited(values: list[Any], limit: int = 16) -> str:
    text_values = [str(value) for value in values if str(value)]
    if len(text_values) > limit:
        return ", ".join(text_values[:limit]) + f", +{len(text_values) - limit} more"
    return ", ".join(text_values)


def summarize_mapping(mapping: dict[str, Any], limit: int = 12) -> str:
    items = sorted(mapping.items(), key=lambda item: item[0])
    pairs = [f"{key}={value}" for key, value in items[:limit]]
    if len(items) > limit:
        pairs.append(f"+{len(items) - limit} more")
    return ", ".join(pairs)


def build_texture_file_index(root: Path) -> dict[str, list[Path]]:
    index: dict[str, list[Path]] = defaultdict(list)
    for path in root.rglob("*.dds"):
        if path.is_file():
            index[path.name.lower()].append(path)
    return {name: sorted(paths) for name, paths in index.items()}


def choose_texture_path(layout_path: Path, texture_name: str, texture_index: dict[str, list[Path]]) -> Path | None:
    candidates = texture_index.get(texture_name.lower(), [])
    if not candidates:
        return None

    layout_dir = layout_path.parent
    exact_dir = [path for path in candidates if path.parent == layout_dir]
    if exact_dir:
        return exact_dir[0]

    def score(path: Path) -> tuple[int, int, str]:
        shared = 0
        for left, right in zip(layout_dir.parts, path.parent.parts):
            if left != right:
                break
            shared += 1
        return (-shared, len(path.parent.parts), path.as_posix())

    return sorted(candidates, key=score)[0]


def read_dds_dimensions(path: Path) -> tuple[int, int, str]:
    try:
        header = path.read_bytes()[:128]
    except OSError:
        return 0, 0, ""

    if len(header) < 128 or header[:4] != b"DDS ":
        return 0, 0, ""

    height = struct.unpack_from("<I", header, 12)[0]
    width = struct.unpack_from("<I", header, 16)[0]
    four_cc = header[84:88].decode("ascii", errors="replace").strip("\x00")
    return int(width), int(height), four_cc


def clamp(value: float, low: float, high: float) -> float:
    return max(low, min(high, value))


def normalized_cast_rect(cast: dict[str, Any]) -> tuple[float, float, float, float]:
    points = [
        cast.get("top_left", [0.0, 0.0]),
        cast.get("bottom_left", [0.0, 0.0]),
        cast.get("top_right", [0.0, 0.0]),
        cast.get("bottom_right", [0.0, 0.0]),
    ]
    xs = [float(point[0]) for point in points]
    ys = [float(point[1]) for point in points]
    left = min(xs)
    right = max(xs)
    top = min(ys)
    bottom = max(ys)

    width = clamp(abs(right - left), 0.015, 1.0)
    height = clamp(abs(bottom - top), 0.015, 1.0)
    normalized_left = clamp(left if left >= 0.0 else 0.5 + left, 0.0, 1.0 - width)
    normalized_top = clamp(top if top >= 0.0 else 0.5 + top, 0.0, 1.0 - height)
    return normalized_left, normalized_top, width, height


def cast_info_value(cast: dict[str, Any], key: str, fallback: Any) -> Any:
    return cast.get("cast_info", {}).get(key, fallback)


def choose_cast_subimage_index(cast: dict[str, Any]) -> int | None:
    material = cast.get("cast_material", {})
    subimage_indices = material.get("subimage_indices", [])
    used = material.get("used_subimage_indices", [])
    slot = int(round(float(cast_info_value(cast, "subimage", 0.0))))
    if 0 <= slot < len(subimage_indices) and subimage_indices[slot] >= 0:
        return int(subimage_indices[slot])
    if used:
        return int(used[0])
    return None


def build_cast_parent_map(cast_group: dict[str, Any]) -> dict[int, int]:
    hierarchy = cast_group.get("hierarchy", [])
    parent_by_child: dict[int, int] = {}
    for parent_index, item in enumerate(hierarchy):
        cursor = item.get("child_index", -1)
        seen: set[int] = set()
        while 0 <= cursor < len(hierarchy) and cursor not in seen:
            seen.add(cursor)
            parent_by_child[cursor] = parent_index
            cursor = hierarchy[cursor].get("next_index", -1)
    return parent_by_child


def build_cast_path(
    group_index: int,
    cast_index: int,
    cast_group: dict[str, Any],
    scene_cast_names: dict[tuple[int, int], str],
) -> str:
    parent_by_child = build_cast_parent_map(cast_group)
    parts: list[str] = []
    cursor = cast_index
    seen: set[int] = set()
    while cursor >= 0 and cursor not in seen:
        seen.add(cursor)
        parts.append(scene_cast_names.get((group_index, cursor), f"group{group_index}_cast{cursor}"))
        cursor = parent_by_child.get(cursor, -1)
    return "/".join(reversed(parts))


def build_group_global_transforms(cast_group: dict[str, Any]) -> dict[int, dict[str, float]]:
    parent_by_child = build_cast_parent_map(cast_group)
    casts = cast_group.get("casts", [])
    transforms: dict[int, dict[str, float]] = {}

    def resolve(cast_index: int) -> dict[str, float]:
        if cast_index in transforms:
            return transforms[cast_index]

        cast = casts[cast_index]
        translation = cast_info_value(cast, "translation", [0.0, 0.0])
        scale = cast_info_value(cast, "scale", [1.0, 1.0])
        local_x = float(translation[0] if len(translation) > 0 else 0.0)
        local_y = float(translation[1] if len(translation) > 1 else 0.0)
        local_scale_x = float(scale[0] if len(scale) > 0 else 1.0)
        local_scale_y = float(scale[1] if len(scale) > 1 else 1.0)
        parent_index = parent_by_child.get(cast_index)

        if parent_index is None or parent_index < 0 or parent_index >= len(casts):
            transform = {
                "x": local_x,
                "y": local_y,
                "scale_x": local_scale_x,
                "scale_y": local_scale_y,
            }
        else:
            parent = resolve(parent_index)
            transform = {
                "x": parent["x"] + local_x * parent["scale_x"],
                "y": parent["y"] + local_y * parent["scale_y"],
                "scale_x": parent["scale_x"] * local_scale_x,
                "scale_y": parent["scale_y"] * local_scale_y,
            }

        transforms[cast_index] = transform
        return transform

    for index in range(len(casts)):
        resolve(index)

    return transforms


def composed_cast_rect(cast: dict[str, Any], transform: dict[str, float]) -> tuple[float, float, float, float]:
    points = [
        cast.get("top_left", [0.0, 0.0]),
        cast.get("bottom_left", [0.0, 0.0]),
        cast.get("top_right", [0.0, 0.0]),
        cast.get("bottom_right", [0.0, 0.0]),
    ]
    xs = [transform["x"] + float(point[0]) * transform["scale_x"] for point in points]
    ys = [transform["y"] + float(point[1]) * transform["scale_y"] for point in points]
    left = min(xs)
    top = min(ys)
    width = max(0.0005, max(xs) - left)
    height = max(0.0005, max(ys) - top)
    return left, top, width, height


def walk_raw_scenes(
    node: dict[str, Any],
    node_path: str,
) -> list[tuple[dict[str, Any], str, str]]:
    rows: list[tuple[dict[str, Any], str, str]] = []
    scene_ids = sorted(node.get("scene_ids", []), key=lambda item: item["index"])
    for index, scene in enumerate(node.get("scenes", [])):
        scene_name = scene_ids[index]["name"] if index < len(scene_ids) else f"scene_{index}"
        rows.append((scene, scene_name, node_path))

    node_dictionaries = sorted(node.get("node_dictionaries", []), key=lambda item: item["index"])
    for index, child in enumerate(node.get("children", [])):
        child_name = node_dictionaries[index]["name"] if index < len(node_dictionaries) else f"node_{index}"
        rows.extend(walk_raw_scenes(child, f"{node_path}/{child_name}"))
    return rows


def build_subimage_texture_record(
    subimage: dict[str, Any],
    texture_names: list[str],
    layout_path: Path,
    root: Path,
    texture_file_index: dict[str, list[Path]],
) -> dict[str, Any] | None:
    texture_index = int(subimage.get("texture_index", -1))
    if not (0 <= texture_index < len(texture_names)):
        return None

    texture_name = texture_names[texture_index]
    texture_path = choose_texture_path(layout_path, texture_name, texture_file_index)
    texture_width = 0
    texture_height = 0
    texture_format = ""
    texture_relative_path = ""
    if texture_path is not None:
        texture_width, texture_height, texture_format = read_dds_dimensions(texture_path)
        texture_relative_path = (
            texture_path.relative_to(root).as_posix()
            if texture_path.is_relative_to(root)
            else texture_path.as_posix()
        )

    uv_left, uv_top = [float(value) for value in subimage["top_left"]]
    uv_right, uv_bottom = [float(value) for value in subimage["bottom_right"]]
    source_x = int(round(uv_left * texture_width)) if texture_width else 0
    source_y = int(round(uv_top * texture_height)) if texture_height else 0
    source_width = max(1, int(round((uv_right - uv_left) * texture_width))) if texture_width else 0
    source_height = max(1, int(round((uv_bottom - uv_top) * texture_height))) if texture_height else 0
    return {
        "texture_name": texture_name,
        "texture_relative_path": texture_relative_path,
        "texture_format": texture_format,
        "source_texture_width": texture_width,
        "source_texture_height": texture_height,
        "source_x": source_x,
        "source_y": source_y,
        "source_width": source_width,
        "source_height": source_height,
        "uv_left": uv_left,
        "uv_top": uv_top,
        "uv_right": uv_right,
        "uv_bottom": uv_bottom,
        "has_texture": texture_width > 0 and texture_height > 0,
    }


def extract_preview_draw_commands(
    parsed: dict[str, Any],
    root: Path,
    texture_file_index: dict[str, list[Path]],
) -> list[dict[str, Any]]:
    project = parsed["resources"][0]["content"].get("csdm_project", {})
    texture_list = parsed["resources"][1]["content"].get("texture_list", {})
    texture_names = [item["name"] for item in texture_list.get("textures", [])]
    layout_path = Path(parsed["path"])
    project_relative_path = layout_path.relative_to(root).as_posix() if layout_path.is_relative_to(root) else layout_path.as_posix()
    raw_scenes = walk_raw_scenes(project.get("root", {}), project.get("project_name", "Root"))
    commands: list[dict[str, Any]] = []

    for scene, scene_name, node_path in raw_scenes:
        scene_commands = 0
        scene_cast_names = cast_name_lookup(scene)
        for group_index, cast_group in enumerate(scene.get("cast_groups", [])):
            for cast_index, cast in enumerate(cast_group.get("casts", [])):
                material = cast.get("cast_material", {})
                for subimage_index in material.get("used_subimage_indices", []):
                    if scene_commands >= MAX_PREVIEW_DRAW_COMMANDS_PER_SCENE:
                        break
                    if not (0 <= subimage_index < len(scene.get("subimages", []))):
                        continue

                    texture_record = build_subimage_texture_record(
                        scene["subimages"][subimage_index],
                        texture_names,
                        layout_path,
                        root,
                        texture_file_index,
                    )
                    if texture_record is None:
                        continue
                    normalized_left, normalized_top, normalized_width, normalized_height = normalized_cast_rect(cast)
                    cast_name = scene_cast_names.get((group_index, cast_index), f"group{group_index}_cast{cast_index}")

                    commands.append(
                        {
                            "project": parsed["stem"],
                            "file_name": parsed["file_name"],
                            "project_relative_path": project_relative_path,
                            "scene_name": scene_name,
                            "node_path": node_path,
                            "group_index": group_index,
                            "cast_index": cast_index,
                            "cast_name": cast_name,
                            "cast_path": build_cast_path(group_index, cast_index, cast_group, scene_cast_names),
                            "texture_name": texture_record["texture_name"],
                            "texture_relative_path": texture_record["texture_relative_path"],
                            "texture_format": texture_record["texture_format"],
                            "subimage_index": int(subimage_index),
                            "source_texture_width": texture_record["source_texture_width"],
                            "source_texture_height": texture_record["source_texture_height"],
                            "source_x": texture_record["source_x"],
                            "source_y": texture_record["source_y"],
                            "source_width": texture_record["source_width"],
                            "source_height": texture_record["source_height"],
                            "uv_left": texture_record["uv_left"],
                            "uv_top": texture_record["uv_top"],
                            "uv_right": texture_record["uv_right"],
                            "uv_bottom": texture_record["uv_bottom"],
                            "normalized_cast_left": normalized_left,
                            "normalized_cast_top": normalized_top,
                            "normalized_cast_width": normalized_width,
                            "normalized_cast_height": normalized_height,
                            "has_texture": texture_record["has_texture"],
                            "provenance": "real-yncp-subimage-dds-rect",
                        }
                    )
                    scene_commands += 1
                if scene_commands >= MAX_PREVIEW_DRAW_COMMANDS_PER_SCENE:
                    break
            if scene_commands >= MAX_PREVIEW_DRAW_COMMANDS_PER_SCENE:
                break

    return commands


def extract_scene_draw_commands(
    parsed: dict[str, Any],
    root: Path,
    texture_file_index: dict[str, list[Path]],
) -> list[dict[str, Any]]:
    project = parsed["resources"][0]["content"].get("csdm_project", {})
    texture_list = parsed["resources"][1]["content"].get("texture_list", {})
    texture_names = [item["name"] for item in texture_list.get("textures", [])]
    layout_path = Path(parsed["path"])
    project_relative_path = layout_path.relative_to(root).as_posix() if layout_path.is_relative_to(root) else layout_path.as_posix()
    raw_scenes = walk_raw_scenes(project.get("root", {}), project.get("project_name", "Root"))
    commands: list[dict[str, Any]] = []

    for scene, scene_name, node_path in raw_scenes:
        scene_commands = 0
        scene_cast_names = cast_name_lookup(scene)
        draw_order = 0
        for group_index, cast_group in enumerate(scene.get("cast_groups", [])):
            transforms = build_group_global_transforms(cast_group)
            for cast_index, cast in enumerate(cast_group.get("casts", [])):
                if scene_commands >= MAX_SCENE_DRAW_COMMANDS_PER_SCENE:
                    break
                if int(cast.get("is_enabled", 1)) == 0:
                    continue

                subimage_index = choose_cast_subimage_index(cast)
                if subimage_index is None or not (0 <= subimage_index < len(scene.get("subimages", []))):
                    continue

                texture_record = build_subimage_texture_record(
                    scene["subimages"][subimage_index],
                    texture_names,
                    layout_path,
                    root,
                    texture_file_index,
                )
                if texture_record is None:
                    continue

                transform = transforms.get(cast_index, {"x": 0.0, "y": 0.0, "scale_x": 1.0, "scale_y": 1.0})
                scene_left, scene_top, scene_width, scene_height = composed_cast_rect(cast, transform)
                if scene_width <= 0.0005 or scene_height <= 0.0005:
                    continue

                cast_name = scene_cast_names.get((group_index, cast_index), f"group{group_index}_cast{cast_index}")
                cast_info = cast.get("cast_info", {})
                commands.append(
                    {
                        "project": parsed["stem"],
                        "file_name": parsed["file_name"],
                        "project_relative_path": project_relative_path,
                        "scene_name": scene_name,
                        "node_path": node_path,
                        "group_index": group_index,
                        "cast_index": cast_index,
                        "draw_order": draw_order,
                        "cast_name": cast_name,
                        "cast_path": build_cast_path(group_index, cast_index, cast_group, scene_cast_names),
                        "texture_name": texture_record["texture_name"],
                        "texture_relative_path": texture_record["texture_relative_path"],
                        "texture_format": texture_record["texture_format"],
                        "subimage_index": int(subimage_index),
                        "source_texture_width": texture_record["source_texture_width"],
                        "source_texture_height": texture_record["source_texture_height"],
                        "source_x": texture_record["source_x"],
                        "source_y": texture_record["source_y"],
                        "source_width": texture_record["source_width"],
                        "source_height": texture_record["source_height"],
                        "uv_left": texture_record["uv_left"],
                        "uv_top": texture_record["uv_top"],
                        "uv_right": texture_record["uv_right"],
                        "uv_bottom": texture_record["uv_bottom"],
                        "scene_left": scene_left,
                        "scene_top": scene_top,
                        "scene_width": scene_width,
                        "scene_height": scene_height,
                        "base_translation_x": float(cast_info.get("translation", [0.0, 0.0])[0]),
                        "base_translation_y": float(cast_info.get("translation", [0.0, 0.0])[1]),
                        "base_scale_x": float(cast_info.get("scale", [1.0, 1.0])[0]),
                        "base_scale_y": float(cast_info.get("scale", [1.0, 1.0])[1]),
                        "base_rotation": float(cast_info.get("rotation", 0.0)),
                        "hide_flag": int(cast_info.get("hide_flag", 0)),
                        "has_texture": texture_record["has_texture"],
                        "provenance": "real-yncp-cast-tree-subimage-scene-rect",
                    }
                )
                draw_order += 1
                scene_commands += 1
            if scene_commands >= MAX_SCENE_DRAW_COMMANDS_PER_SCENE:
                break

    return commands


def finite_float(value: Any, fallback: float = 0.0) -> float:
    try:
        number = float(value)
    except (TypeError, ValueError):
        return fallback
    return number if math.isfinite(number) else fallback


def is_animation_track_supported_for_preview(track_type: str) -> bool:
    return track_type in ANIMATION_TRACKS_SUPPORTED_FOR_PREVIEW


def extract_animation_track_keyframes(parsed: dict[str, Any], root: Path) -> list[dict[str, Any]]:
    project = parsed["resources"][0]["content"].get("csdm_project", {})
    layout_path = Path(parsed["path"])
    project_relative_path = layout_path.relative_to(root).as_posix() if layout_path.is_relative_to(root) else layout_path.as_posix()
    raw_scenes = walk_raw_scenes(project.get("root", {}), project.get("project_name", "Root"))
    rows: list[dict[str, Any]] = []

    for scene, scene_name, node_path in raw_scenes:
        scene_cast_names = cast_name_lookup(scene)
        animation_dictionaries = scene.get("animation_dictionaries", [])
        animation_frame_data_list = scene.get("animation_frame_data_list", [])
        animation_keyframe_data_list = scene.get("animation_keyframe_data_list", [])
        cast_groups = scene.get("cast_groups", [])

        for animation_slot, animation_keyframe_data in enumerate(animation_keyframe_data_list):
            animation_dictionary = (
                animation_dictionaries[animation_slot]
                if animation_slot < len(animation_dictionaries)
                else {}
            )
            animation_frame_data = (
                animation_frame_data_list[animation_slot]
                if animation_slot < len(animation_frame_data_list)
                else {}
            )
            animation_name = str(animation_dictionary.get("name", f"animation_{animation_slot}"))
            animation_index = int(animation_dictionary.get("index", animation_slot))
            animation_frame_count = finite_float(animation_frame_data.get("frame_count", 0.0))
            animation_framerate = finite_float(scene.get("animation_framerate", 0.0))

            for group_index, group_animation in enumerate(animation_keyframe_data.get("groups", [])):
                cast_group = cast_groups[group_index] if group_index < len(cast_groups) else {}
                for cast_index, cast_animation in enumerate(group_animation.get("casts", [])):
                    cast_name = scene_cast_names.get((group_index, cast_index), f"group{group_index}_cast{cast_index}")
                    cast_path = (
                        build_cast_path(group_index, cast_index, cast_group, scene_cast_names)
                        if cast_group
                        else cast_name
                    )

                    for track in cast_animation.get("sub_data", []):
                        track_type = str(track.get("track_type", ""))
                        if not is_animation_track_supported_for_preview(track_type):
                            continue

                        for keyframe_index, keyframe in enumerate(track.get("keyframes", [])):
                            value = keyframe.get("value")
                            if value is None:
                                continue

                            value = finite_float(value, math.nan)
                            if not math.isfinite(value):
                                continue
                            frame = finite_float(keyframe.get("frame", 0.0))
                            if frame < -1.0:
                                continue
                            if animation_frame_count > 0.0 and frame > animation_frame_count + 1.0:
                                continue

                            rows.append(
                                {
                                    "project": parsed["stem"],
                                    "file_name": parsed["file_name"],
                                    "project_relative_path": project_relative_path,
                                    "scene_name": scene_name,
                                    "node_path": node_path,
                                    "animation_name": animation_name,
                                    "animation_index": animation_index,
                                    "animation_slot": animation_slot,
                                    "animation_frame_count": animation_frame_count,
                                    "animation_framerate": animation_framerate,
                                    "group_index": group_index,
                                    "cast_index": cast_index,
                                    "cast_path": cast_path,
                                    "cast_name": cast_name,
                                    "track_type": track_type,
                                    "keyframe_index": keyframe_index,
                                    "frame": frame,
                                    "value": value,
                                    "in_tangent": finite_float(keyframe.get("in_tangent", 0.0)),
                                    "out_tangent": finite_float(keyframe.get("out_tangent", 0.0)),
                                    "interpolation_type": str(keyframe.get("type", "Const")),
                                    "provenance": "real-yncp-animation-keyframe",
                                }
                            )

    return rows


def discover_candidate_projects(root: Path) -> list[Path]:
    stems = set().union(*STARTER_PROJECT_GROUPS.values())
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file()
        and path.suffix.lower() in {".yncp", ".xncp"}
        and path.stem.lower() in stems
    )


def classify_screen_group(stem: str) -> str:
    lower = stem.lower()
    for group, stems in STARTER_PROJECT_GROUPS.items():
        if lower in stems:
            return group
    return "other"


def infer_component_tags(scene: dict[str, Any]) -> dict[str, int]:
    text_parts: list[str] = [scene.get("scene_name", ""), scene.get("node_path", "")]
    text_parts.extend(scene.get("cast_names", []))
    text_parts.extend(scene.get("animation_names", []))
    text_parts.extend(scene.get("used_texture_names", []))
    haystack = " ".join(text_parts).lower()

    scores: Counter[str] = Counter()
    for tag, needles in COMPONENT_TAGS.items():
        for needle in needles:
            if needle in haystack:
                scores[tag] += haystack.count(needle)
    return dict(scores.most_common())


def pick_component_role(scene: dict[str, Any]) -> str:
    scores = infer_component_tags(scene)
    if scores:
        return next(iter(scores))
    scene_name = scene.get("scene_name", "scene")
    return f"scene_{slug_name(scene_name)}"


def summarize_scene(scene: dict[str, Any]) -> dict[str, Any]:
    component_scores = infer_component_tags(scene)
    cast_names = scene.get("cast_names", [])
    animation_names = scene.get("animation_names", [])
    used_texture_names = scene.get("used_texture_names", [])

    return {
        "scene_name": scene.get("scene_name", ""),
        "node_path": scene.get("node_path", ""),
        "component_role": pick_component_role(scene),
        "component_scores": component_scores,
        "cast_count": scene.get("cast_count", 0),
        "animation_count": scene.get("animation_count", 0),
        "subimage_count": scene.get("subimage_count", 0),
        "animation_framerate": scene.get("animation_framerate", 0.0),
        "frame_count_range": scene.get("frame_count_range", [0.0, 0.0]),
        "key_cast_names": cast_names[:40],
        "key_animation_names": animation_names[:30],
        "used_texture_names": used_texture_names[:30],
        "font_casts": scene.get("font_casts", [])[:20],
        "track_type_counts": scene.get("track_type_counts", {}),
    }


def build_project_record(path: Path, root: Path, texture_file_index: dict[str, list[Path]]) -> dict[str, Any]:
    parsed = parse_fapc(path)
    digest = canonical_digest(parsed)
    scenes = [summarize_scene(scene) for scene in digest.get("scene_summaries", [])]
    role_counts = Counter(scene["component_role"] for scene in scenes)
    texture_names = digest.get("texture_names", [])
    preview_draw_commands = extract_preview_draw_commands(parsed, root, texture_file_index)
    scene_draw_commands = extract_scene_draw_commands(parsed, root, texture_file_index)
    animation_track_keyframes = extract_animation_track_keyframes(parsed, root)

    return {
        "screen_group": classify_screen_group(path.stem),
        "project": path.stem,
        "file_name": path.name,
        "path": path.as_posix(),
        "relative_path": path.relative_to(root).as_posix() if path.is_relative_to(root) else path.as_posix(),
        "sha256": parsed["sha256"],
        "endianness": parsed["endianness"],
        "project_name": digest.get("project_name", ""),
        "root_scene_names": digest.get("root_scene_names", []),
        "root_child_names": digest.get("root_child_names", []),
        "texture_count": len(texture_names),
        "texture_names": texture_names[:80],
        "font_names": digest.get("font_names", []),
        "totals": digest.get("totals", {}),
        "component_role_counts": dict(sorted(role_counts.items())),
        "scenes": scenes,
        "preview_draw_commands": preview_draw_commands,
        "scene_draw_commands": scene_draw_commands,
        "animation_track_keyframes": animation_track_keyframes,
    }


def write_markdown(payload: dict[str, Any], path: Path) -> None:
    lines: list[str] = [
        "# YNCP Native Component Map",
        "",
        "Generated from the fully extracted current install. This is a local R&D map for turning authored Sonic Unleashed UI projects into reusable SGFX/native screen components.",
        "",
        "Phase 236 wires this map into the in-game SWARD UI Lab `UI Projects` browser. It is runtime-derived-native-reconstruction metadata, not original authored SEGA source.",
        "",
        "Phase 237 adds an independent drawable/keyframe preview lane for the selected scene row. The preview is driven by parsed project/cast/animation/track metadata and is not tied to the current gameplay/title/loading route, so it can be inspected while the running game is sitting in a HUD, hub, title, loading, or other state. The adjacent audio-bank correlation lane keeps SFX as placeholder cue intents until audio banks/XML, runtime hooks, and Ghidra xrefs prove exact cue IDs.",
        "",
        "Phase 238 binds parsed casts to real extracted DDS texture names, subimage UV rectangles, source texture dimensions, and preview destination rectangles. The in-game UI Projects canvas now has a texture-backed cast/subimage preview layer using real DDS/subimage rectangles, while the SFX lane begins with runtime-hook + Ghidra xref SFX candidates such as `se_system_worldmap/sys_worldmap_decide` from `CTitleStateMenu::Update`.",
        "",
        "Phase 239 adds a composed scene display list from the YNCP cast hierarchy. Scene draw commands apply cast tree parent transforms, `cast_info` translation/scale, selected material subimage slots, texture UVs, and extracted DDS paths so the in-game preview renders a selected scene/component instead of only showing a raw atlas crop.",
        "",
        "Phase 240 adds keyframe interpolation/scrubbing from real authored animation tracks. The generated native map now carries preview-useful YNCP transform keyframes (`XPosition`, `YPosition`, `XScale`, `YScale`, `Rotation`, `HideFlag`, `SubImage`) with frame, value, tangent, and interpolation provenance so the in-game canvas can sample actual authored motion while the adjacent SFX lane keeps same-scene cue candidates beside it.",
        "",
        "Phase 241 adds the in-game foreground invoke mode: the selected YNCP scene can be projected over gameplay as a pause-menu-style summoned UI surface, outside the inspector/browser panel, while still using the reconstructed scene draw commands, keyframe scrub/playback, and same-row SFX correlation data.",
        "",
        "Phase 242 adds the guarded Native CSD Make Probe plan to the runtime bridge: selected .yncp/.xncp project bytes are header-checked as raw CPAF/NYIF/nCPJ CSD packages, copied into guest heap, and handed to `SWA::CCsdProject::Make`/`sub_825E4068` through a cloned PPC probe context, with `MakeCsdProjectMidAsmHook` traversal used as proof that the game parsed the project into a native CSD tree before render-host attachment is attempted.",
        "",
        "Phase 243 hardens that native probe after the crash evidence showed `sub_825E4068` reads a required fourth argument through `r6`. The button now queues work until the next real `CCsdProject::Make` call, captures that live `r6` make context, and only then attempts the cloned native call; loading/save update ticks no longer fire the native Make probe with unrelated registers.",
        "",
        "Phase 244 changes the default probe into a non-crashing native observe mode after the next crash showed direct Make can still fail when the internal package parser returns a null temporary resource. By default, the probe now waits for the game to create the selected project naturally and captures the resulting native tree at `MakeCsdProjectMidAsmHook`; a separate `danger: execute Make` checkbox keeps the direct call path available only as an explicit experiment.",
        "",
        "Phase 245 adds the Native Foreground Render Probe. Once native observe has resolved a selected scene pointer, `Attach Native Scene` / `Spawn Native Foreground` arms that real CSD scene and piggybacks it on the next active `CScene::Render` pass, with motion scrub/playback writing the native `m_MotionFrame`. This is deliberately not an owner pointer hijack yet; render-host proof comes first, and foreground owner replacement stays a later, riskier experiment.",
        "",
        "Phase 246 exposes that native foreground path through the live bridge as direct operator controls: `native-foreground-status`, `native-foreground-attach`, `native-foreground-detach`, `native-motion-play`, `native-motion-stop`, and `native-motion-scrub <frame>`. The bridge returns the native project/root/scene pointers, scene motion frame/repeat fields, render-pass sightings, render count, and the explicit `ownerHijackUsed=false` safety status so render-count proof can be gathered before any foreground owner attach experiment.",
        "",
        "Phase 247 adds a tool-facing `native-make-observe <project> <scene> [frame]` bridge command. It queues the selected YNCP/XNCP project row by project name/path and scene name, then waits for the running game to create that CSD project naturally so `MakeCsdProjectMidAsmHook` can capture the native tree without requiring manual panel clicks or the dangerous direct Make path.",
        "",
        "Phase 250 guards the Native Foreground Render Probe with a same-project host requirement. The bridge can resolve and arm a native scene from the observed game-created tree, but render piggyback now refuses to call `CScene::Render` unless the active host pass belongs to the same captured CSD project; cross-project foreground spawning is deferred to a real owner/host attach path.",
        "",
        "Phase 251 correlates CSD resource `Scene*` records from the native project traversal with live renderable `CScene*` manager instances from the `CScene::Render` hook. Motion scrub/playback now require `nativeManagerScenePointer` plus `nativeResourceScenePointer` provenance, so the lab no longer writes manager fields on the non-renderable resource tree object. The foreground attach path is held as a same-project compatibility probe until a real foreground owner/host is resolved, because an extra render call without ownership proof is still crash-prone.",
        "",
        "Phase 252 adds Foreground Owner/Host Attach Discovery. The live bridge can run `native-owner-discovery` / `native-owner-scan` to inspect bounded known UI owner ranges (title owner context, CHudSonicStage owner, CHudPause owner, CGeneralWindow owner, CSaveIcon owner) for direct or indirect references to the correlated manager CScene/resource Scene pair before any owner/host attach or pointer hijack is attempted.",
        "",
        "Phase 253 adds Owner Layout Mapping. Once the title owner context exposes the observed `0x1E4` CSD field, the bridge can run `native-owner-layout` to map sibling owner CSD fields around that anchor and record read-only layout evidence for title owner layout first, then HUD owner layout when the CHudSonicStage owner range is live. This keeps native foreground attach on the real owner path instead of guessing render calls.",
        "",
        "Phase 254 adds Owner Layout Semantic Naming. The owner layout map now annotates each sibling field with a semantic name/role, owner lifecycle note, attach setter candidate, and Ghidra xref oracle status. Title mapping starts with `titleContext.m_rcTitleManager` at `0x1E4` and `titleContext.m_rcTitleResource` at `0x1E8`; HUD owner layout stays `HUD owner layout pending runtime gameplay evidence` until gameplay samples prove the same attach/setter path.",
        "",
        "Phase 255 adds Native Owner Setter Probe. Read-only wrappers around `sub_8250F2B8`, `sub_82581288/A8/E8`, and HUD helpers `sub_82E5FCD0 / sub_82E61A78` record helper arguments, owner fields, result registers, and DB/XML route evidence beside the native owner layout map. This confirms the real attach/setter path before any foreground owner write or hijack experiment.",
        "",
        "Phase 256 adds HUD Owner Setter Field Map. The native owner setter probe now translates HUD helper traffic back to probable owning UI objects when `argR3 == owner+0x28`, falls back to `argR3 - 0x28` inferred owner for the proven HUD stage-bind callsites, and labels the generated `CHudSonicStage::sub_824D9308` callsites where `argR5=110` maps `owner+0xD8 -> owner+0xE0` and `argR5=121` maps the `owner+0xF0/+0xF4` scene update path. This still stays read-only, but it turns anonymous helper calls into attach-path field evidence.",
        "",
        "Phase 257 adds HUD Owner Layout Field Map. The setter probe seeds an `inferred CHudSonicStage owner` global from the proven `argR3 - 0x28` HUD helper traffic and feeds it into the bounded owner-layout scanner so sibling owner CSD fields can be enumerated even when the constructor-time CHudSonicStage hook has not landed yet. The owner layout semantic resolver now names `hudOwner.attachSourceScene` at `owner+0xD8`, `hudOwner.attachTargetScene` at `owner+0xE0`, `hudOwner.activeUpdateScenePrimary` at `owner+0xF0`, and `hudOwner.activeUpdateSceneCompanion` at `owner+0xF4`, with parallel `hudOwnerInferred.*` names when the inferred owner is the source. The attach setter writes still stay disabled until the lifecycle is proven through Ghidra xref export.",
        "",
        "Phase 258 adds HUD Owner Slot Readout. Whenever a HUD setter helper sample confirms the constructor-side or inferred CHudSonicStage owner, the lab now directly reads `owner+0xD8/0xE0/0xF0/0xF4` and classifies each value as `live-manager-scene`, `resource-scene`, `indirect-manager-scene`, `indirect-resource-scene`, `uncorrelated-pointer`, `non-pointer`, or `null`. Each new slot kind/value emits a deduped `native-owner-setter-hud-owner-slot-readout` event and the latest readout per slot is mirrored into the live-state JSON so the operator can see whether `owner+0xD8/0xE0` already point at a known renderable scene before any guarded native foreground attach is tried.",
        "",
        "Phase 259 adds HUD Setter Probe Hot-Path Filter. The HUD setter helpers `sub_82E5FCD0` and `sub_82E61A78` fire thousands of times per second during gameplay, so unconditional sample recording wrote ~12 events/sec of dead JSONL traffic and drove gameplay framerate below 20fps. `OnHudOwnerSetterProbe` now skips full sample recording for any helper traffic whose `argR5` is not the proven CHudSonicStage::sub_824D9308 callsite (`argR5=110` for `sub_82E5FCD0`, `argR5=121` for `sub_82E61A78`), emits a one-shot `native-owner-setter-hud-helper-first-seen` breadcrumb the first time each unexpected `(helper, argR5)` tuple appears, and exposes `skippedHudOwnerSetterProbeCallCount` / `recordedHudOwnerSetterProbeCallCount` perf counters in the live-state JSON.",
        "",
        "Phase 260 adds HUD Owner Renderable-Slot Sweep. The four hardcoded slot readouts only cover the proven `+0xD8/0xE0/0xF0/0xF4` offsets, so this phase auto-runs a bounded sweep across the full `0x3000`-byte CHudSonicStage owner range as soon as the live owner is seeded (constructor-confirmed or inferred). Every owner field whose value (direct or indirect through one dereference) resolves to a live manager `CScene` emits a `native-hud-owner-renderable-slot` event with full project/scene/manager/resource provenance, the sweep itself emits one `native-hud-owner-layout-sweep-complete` per owner address, and the discovered slots are mirrored into the live-state JSON. The sweep is gated to one execution per unique owner address per session so steady-state HUD frames pay no recurring cost. First gameplay run found `owner+0xF4 -> ui_playscreen/so_ringenagy_gauge` as a live indirect-manager-scene slot.",
        "",
        f"- Input root: `{payload['input_root']}`",
        f"- Project files parsed: `{payload['project_count']}`",
        f"- Preview draw commands: `{payload.get('preview_draw_command_count', 0)}` real-yncp-subimage-dds-rect rows.",
        f"- Scene draw commands: `{payload.get('scene_draw_command_count', 0)}` real-yncp-cast-tree-subimage-scene-rect rows.",
        f"- Animation keyframes: `{payload.get('animation_track_keyframe_count', 0)}` real-yncp-animation-keyframe rows for keyframe interpolation.",
        f"- SFX cue candidates: `{payload.get('sfx_cue_candidate_count', 0)}` runtime-hook + Ghidra xref SFX candidates.",
        "- SFX correlation: `sfx-correlation-pending` until audio banks/XML/runtime hooks/Ghidra xrefs prove every exact cue ID.",
        "",
    ]

    for group, projects in payload["screen_groups"].items():
        lines.append(f"## {group.replace('_', ' ').title()}")
        lines.append("")
        for project in projects:
            totals = project.get("totals", {})
            lines.append(f"### `{project['project']}`")
            lines.append(f"- Path: `{project['relative_path']}`")
            lines.append(f"- Scenes: `{totals.get('scene_count', 0)}`; casts: `{totals.get('cast_dictionary_count', 0)}`; animations: `{totals.get('animation_dictionary_count', 0)}`; textures: `{project['texture_count']}`")
            lines.append(f"- Root scenes: `{', '.join(project.get('root_scene_names', [])[:16])}`")
            roles = ", ".join(f"{name}={count}" for name, count in project.get("component_role_counts", {}).items())
            lines.append(f"- Component roles: `{roles or 'unclassified'}`")
            lines.append(f"- Texture-backed preview commands: `{len(project.get('preview_draw_commands', []))}`")
            lines.append(f"- Composed scene draw commands: `{len(project.get('scene_draw_commands', []))}`")
            lines.append(f"- Animation keyframes: `{len(project.get('animation_track_keyframes', []))}`")
            lines.append("- Key scenes:")
            for scene in project.get("scenes", [])[:12]:
                lines.append(
                    f"  - `{scene['scene_name']}` -> `{scene['component_role']}` "
                    f"casts={scene['cast_count']} anims={scene['animation_count']} frames={scene['frame_count_range']}"
                )
            lines.append("")

    if payload.get("sfx_cue_candidates"):
        lines.append("## Runtime-Hook + Ghidra Xref SFX Candidates")
        lines.append("")
        for candidate in payload["sfx_cue_candidates"]:
            lines.append(
                f"- `{candidate['project']}/{candidate['scene']}` `{candidate['bank_name']}/{candidate['cue_name']}` "
                f"via `{candidate['runtime_hook']}`; xref `{candidate['ghidra_xref']}`"
            )
        lines.append("")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_header(payload: dict[str, Any], path: Path) -> None:
    projects: list[dict[str, Any]] = []
    scenes: list[dict[str, Any]] = []
    preview_draw_commands: list[dict[str, Any]] = []
    scene_draw_commands: list[dict[str, Any]] = []
    animation_track_keyframes: list[dict[str, Any]] = []

    for group, group_projects in payload["screen_groups"].items():
        for project in group_projects:
            scene_start = len(scenes)
            preview_draw_commands.extend(project.get("preview_draw_commands", []))
            scene_draw_commands.extend(project.get("scene_draw_commands", []))
            animation_track_keyframes.extend(project.get("animation_track_keyframes", []))
            for scene in project.get("scenes", []):
                frame_range = scene.get("frame_count_range", [0.0, 0.0])
                scenes.append(
                    {
                        "project": project.get("project", ""),
                        "scene_name": scene.get("scene_name", ""),
                        "node_path": scene.get("node_path", ""),
                        "component_role": scene.get("component_role", ""),
                        "cast_count": int(scene.get("cast_count", 0)),
                        "animation_count": int(scene.get("animation_count", 0)),
                        "subimage_count": int(scene.get("subimage_count", 0)),
                        "animation_framerate": float(scene.get("animation_framerate", 0.0) or 0.0),
                        "frame_min": float(frame_range[0] if frame_range else 0.0),
                        "frame_max": float(frame_range[1] if len(frame_range) > 1 else 0.0),
                        "key_animations": join_limited(scene.get("key_animation_names", []), 12),
                        "key_casts": join_limited(scene.get("key_cast_names", []), 18),
                        "textures": join_limited(scene.get("used_texture_names", []), 14),
                        "track_types": summarize_mapping(scene.get("track_type_counts", {}), 12),
                    }
                )

            totals = project.get("totals", {})
            projects.append(
                {
                    "screen_group": group,
                    "project": project.get("project", ""),
                    "file_name": project.get("file_name", ""),
                    "relative_path": project.get("relative_path", ""),
                    "project_name": project.get("project_name", ""),
                    "endianness": project.get("endianness", ""),
                    "root_scenes": join_limited(project.get("root_scene_names", []), 18),
                    "roles": summarize_mapping(project.get("component_role_counts", {}), 16),
                    "textures": join_limited(project.get("texture_names", []), 18),
                    "provenance": "runtime-derived-native-reconstruction",
                    "sfx_status": "sfx-correlation-pending",
                    "scene_start": scene_start,
                    "scene_count": len(project.get("scenes", [])),
                    "total_scenes": int(totals.get("scene_count", 0)),
                    "total_casts": int(totals.get("cast_dictionary_count", 0)),
                    "total_animations": int(totals.get("animation_dictionary_count", 0)),
                    "total_subimages": int(totals.get("subimage_count", 0)),
                    "texture_count": int(project.get("texture_count", 0)),
                }
            )

    lines: list[str] = [
        "#pragma once",
        "",
        "#include <array>",
        "#include <cstddef>",
        "#include <cstdint>",
        "#include <string_view>",
        "",
        "namespace UiLab::GeneratedYncPNativeComponentMap",
        "{",
        "    struct Scene",
        "    {",
        "        std::string_view project;",
        "        std::string_view scene;",
        "        std::string_view nodePath;",
        "        std::string_view componentRole;",
        "        uint32_t castCount;",
        "        uint32_t animationCount;",
        "        uint32_t subimageCount;",
        "        float animationFramerate;",
        "        float frameMin;",
        "        float frameMax;",
        "        std::string_view keyAnimations;",
        "        std::string_view keyCasts;",
        "        std::string_view textures;",
        "        std::string_view trackTypes;",
        "    };",
        "",
        "    struct Project",
        "    {",
        "        std::string_view screenGroup;",
        "        std::string_view project;",
        "        std::string_view fileName;",
        "        std::string_view relativePath;",
        "        std::string_view projectName;",
        "        std::string_view endianness;",
        "        std::string_view rootScenes;",
        "        std::string_view componentRoles;",
        "        std::string_view textures;",
        "        std::string_view provenance;",
        "        std::string_view sfxStatus;",
        "        size_t firstScene;",
        "        size_t sceneCount;",
        "        uint32_t totalScenes;",
        "        uint32_t totalCasts;",
        "        uint32_t totalAnimations;",
        "        uint32_t totalSubimages;",
        "        uint32_t textureCount;",
        "    };",
        "",
        "    struct PreviewDrawCommand",
        "    {",
        "        std::string_view project;",
        "        std::string_view fileName;",
        "        std::string_view projectRelativePath;",
        "        std::string_view scene;",
        "        std::string_view nodePath;",
        "        std::string_view castPath;",
        "        std::string_view castName;",
        "        std::string_view textureName;",
        "        std::string_view textureRelativePath;",
        "        std::string_view textureFormat;",
        "        int32_t groupIndex;",
        "        int32_t castIndex;",
        "        int32_t subimageIndex;",
        "        int32_t sourceTextureWidth;",
        "        int32_t sourceTextureHeight;",
        "        int32_t sourceX;",
        "        int32_t sourceY;",
        "        int32_t sourceWidth;",
        "        int32_t sourceHeight;",
        "        float uvLeft;",
        "        float uvTop;",
        "        float uvRight;",
        "        float uvBottom;",
        "        float normalizedCastLeft;",
        "        float normalizedCastTop;",
        "        float normalizedCastWidth;",
        "        float normalizedCastHeight;",
        "        bool hasTexture;",
        "        std::string_view provenance;",
        "    };",
        "",
        "    struct SceneDrawCommand",
        "    {",
        "        std::string_view project;",
        "        std::string_view fileName;",
        "        std::string_view projectRelativePath;",
        "        std::string_view scene;",
        "        std::string_view nodePath;",
        "        std::string_view castPath;",
        "        std::string_view castName;",
        "        std::string_view textureName;",
        "        std::string_view textureRelativePath;",
        "        std::string_view textureFormat;",
        "        int32_t groupIndex;",
        "        int32_t castIndex;",
        "        int32_t drawOrder;",
        "        int32_t subimageIndex;",
        "        int32_t sourceTextureWidth;",
        "        int32_t sourceTextureHeight;",
        "        int32_t sourceX;",
        "        int32_t sourceY;",
        "        int32_t sourceWidth;",
        "        int32_t sourceHeight;",
        "        float uvLeft;",
        "        float uvTop;",
        "        float uvRight;",
        "        float uvBottom;",
        "        float sceneLeft;",
        "        float sceneTop;",
        "        float sceneWidth;",
        "        float sceneHeight;",
        "        float baseTranslationX;",
        "        float baseTranslationY;",
        "        float baseScaleX;",
        "        float baseScaleY;",
        "        float baseRotation;",
        "        int32_t hideFlag;",
        "        bool hasTexture;",
        "        std::string_view provenance;",
        "    };",
        "",
        "    struct AnimationTrackKeyframe",
        "    {",
        "        std::string_view project;",
        "        std::string_view fileName;",
        "        std::string_view projectRelativePath;",
        "        std::string_view scene;",
        "        std::string_view nodePath;",
        "        std::string_view animationName;",
        "        int32_t animationIndex;",
        "        int32_t animationSlot;",
        "        float animationFrameCount;",
        "        float animationFramerate;",
        "        int32_t groupIndex;",
        "        int32_t castIndex;",
        "        std::string_view castPath;",
        "        std::string_view castName;",
        "        std::string_view trackType;",
        "        int32_t keyframeIndex;",
        "        float frame;",
        "        float value;",
        "        float inTangent;",
        "        float outTangent;",
        "        std::string_view interpolationType;",
        "        std::string_view provenance;",
        "    };",
        "",
        "    struct SfxCueCandidate",
        "    {",
        "        std::string_view screenGroup;",
        "        std::string_view project;",
        "        std::string_view scene;",
        "        std::string_view action;",
        "        std::string_view bankName;",
        "        std::string_view cueName;",
        "        std::string_view runtimeHook;",
        "        std::string_view ghidraXref;",
        "        std::string_view provenance;",
        "    };",
        "",
        f'    static constexpr std::string_view kGeneratedAt = "{cpp_escape(datetime.now(timezone.utc).isoformat())}";',
        f'    static constexpr std::string_view kInputRoot = "{cpp_escape(payload["input_root"])}";',
        '    static constexpr std::string_view kReconstructionPolicy = "runtime-derived-native-reconstruction";',
        '    static constexpr std::string_view kSfxCorrelationStatus = "sfx-correlation-pending";',
        '    static constexpr std::string_view kSgfxExportPolicy = "SGFX-facing placeholder modules with source-map provenance";',
        '    static constexpr std::string_view kPreviewDrawCommandPolicy = "real-yncp-subimage-dds-rect";',
        '    static constexpr std::string_view kSceneDrawCommandPolicy = "real-yncp-cast-tree-subimage-scene-rect";',
        '    static constexpr std::string_view kAnimationTrackKeyframePolicy = "real-yncp-animation-keyframe";',
        "",
        f"    static constexpr std::array<Scene, {len(scenes)}> kScenes =",
        "    {{",
    ]

    for scene in scenes:
        lines.append(
            '        { "'
            + cpp_escape(scene["project"])
            + '", "'
            + cpp_escape(scene["scene_name"])
            + '", "'
            + cpp_escape(scene["node_path"])
            + '", "'
            + cpp_escape(scene["component_role"])
            + f'", {scene["cast_count"]}, {scene["animation_count"]}, {scene["subimage_count"]}, '
            + f'{scene["animation_framerate"]:.3f}f, {scene["frame_min"]:.3f}f, {scene["frame_max"]:.3f}f, "'
            + cpp_escape(scene["key_animations"])
            + '", "'
            + cpp_escape(scene["key_casts"])
            + '", "'
            + cpp_escape(scene["textures"])
            + '", "'
            + cpp_escape(scene["track_types"])
            + '" },'
        )

    lines.extend([
        "    }};",
        "",
        f"    static constexpr std::array<Project, {len(projects)}> kProjects =",
        "    {{",
    ])

    for project in projects:
        lines.append(
            '        { "'
            + cpp_escape(project["screen_group"])
            + '", "'
            + cpp_escape(project["project"])
            + '", "'
            + cpp_escape(project["file_name"])
            + '", "'
            + cpp_escape(project["relative_path"])
            + '", "'
            + cpp_escape(project["project_name"])
            + '", "'
            + cpp_escape(project["endianness"])
            + '", "'
            + cpp_escape(project["root_scenes"])
            + '", "'
            + cpp_escape(project["roles"])
            + '", "'
            + cpp_escape(project["textures"])
            + '", "'
            + cpp_escape(project["provenance"])
            + '", "'
            + cpp_escape(project["sfx_status"])
            + f'", {project["scene_start"]}, {project["scene_count"]}, {project["total_scenes"]}, '
            + f'{project["total_casts"]}, {project["total_animations"]}, {project["total_subimages"]}, {project["texture_count"]} }},'
        )

    lines.extend([
        "    }};",
        "",
        f"    static constexpr std::array<PreviewDrawCommand, {len(preview_draw_commands)}> kPreviewDrawCommands =",
        "    {{",
    ])

    for command in preview_draw_commands:
        lines.append(
            '        { "'
            + cpp_escape(command["project"])
            + '", "'
            + cpp_escape(command["file_name"])
            + '", "'
            + cpp_escape(command["project_relative_path"])
            + '", "'
            + cpp_escape(command["scene_name"])
            + '", "'
            + cpp_escape(command["node_path"])
            + '", "'
            + cpp_escape(command["cast_path"])
            + '", "'
            + cpp_escape(command["cast_name"])
            + '", "'
            + cpp_escape(command["texture_name"])
            + '", "'
            + cpp_escape(command["texture_relative_path"])
            + '", "'
            + cpp_escape(command["texture_format"])
            + f'", {command["group_index"]}, {command["cast_index"]}, {command["subimage_index"]}, '
            + f'{command["source_texture_width"]}, {command["source_texture_height"]}, '
            + f'{command["source_x"]}, {command["source_y"]}, {command["source_width"]}, {command["source_height"]}, '
            + f'{command["uv_left"]:.6f}f, {command["uv_top"]:.6f}f, {command["uv_right"]:.6f}f, {command["uv_bottom"]:.6f}f, '
            + f'{command["normalized_cast_left"]:.6f}f, {command["normalized_cast_top"]:.6f}f, '
            + f'{command["normalized_cast_width"]:.6f}f, {command["normalized_cast_height"]:.6f}f, '
            + ("true" if command["has_texture"] else "false")
            + ', "'
            + cpp_escape(command["provenance"])
            + '" },'
        )

    lines.extend([
        "    }};",
        "",
        f"    static constexpr std::array<SceneDrawCommand, {len(scene_draw_commands)}> kSceneDrawCommands =",
        "    {{",
    ])

    for command in scene_draw_commands:
        lines.append(
            '        { "'
            + cpp_escape(command["project"])
            + '", "'
            + cpp_escape(command["file_name"])
            + '", "'
            + cpp_escape(command["project_relative_path"])
            + '", "'
            + cpp_escape(command["scene_name"])
            + '", "'
            + cpp_escape(command["node_path"])
            + '", "'
            + cpp_escape(command["cast_path"])
            + '", "'
            + cpp_escape(command["cast_name"])
            + '", "'
            + cpp_escape(command["texture_name"])
            + '", "'
            + cpp_escape(command["texture_relative_path"])
            + '", "'
            + cpp_escape(command["texture_format"])
            + f'", {command["group_index"]}, {command["cast_index"]}, {command["draw_order"]}, {command["subimage_index"]}, '
            + f'{command["source_texture_width"]}, {command["source_texture_height"]}, '
            + f'{command["source_x"]}, {command["source_y"]}, {command["source_width"]}, {command["source_height"]}, '
            + f'{command["uv_left"]:.6f}f, {command["uv_top"]:.6f}f, {command["uv_right"]:.6f}f, {command["uv_bottom"]:.6f}f, '
            + f'{command["scene_left"]:.6f}f, {command["scene_top"]:.6f}f, '
            + f'{command["scene_width"]:.6f}f, {command["scene_height"]:.6f}f, '
            + f'{command["base_translation_x"]:.6f}f, {command["base_translation_y"]:.6f}f, '
            + f'{command["base_scale_x"]:.6f}f, {command["base_scale_y"]:.6f}f, '
            + f'{command["base_rotation"]:.6f}f, {command["hide_flag"]}, '
            + ("true" if command["has_texture"] else "false")
            + ', "'
            + cpp_escape(command["provenance"])
            + '" },'
        )

    lines.extend([
        "    }};",
        "",
        f"    static const std::array<AnimationTrackKeyframe, {len(animation_track_keyframes)}> kAnimationTrackKeyframes =",
        "    {{",
    ])

    for keyframe in animation_track_keyframes:
        lines.append(
            '        { "'
            + cpp_escape(keyframe["project"])
            + '", "'
            + cpp_escape(keyframe["file_name"])
            + '", "'
            + cpp_escape(keyframe["project_relative_path"])
            + '", "'
            + cpp_escape(keyframe["scene_name"])
            + '", "'
            + cpp_escape(keyframe["node_path"])
            + '", "'
            + cpp_escape(keyframe["animation_name"])
            + f'", {keyframe["animation_index"]}, {keyframe["animation_slot"]}, '
            + f'{keyframe["animation_frame_count"]:.6f}f, {keyframe["animation_framerate"]:.6f}f, '
            + f'{keyframe["group_index"]}, {keyframe["cast_index"]}, "'
            + cpp_escape(keyframe["cast_path"])
            + '", "'
            + cpp_escape(keyframe["cast_name"])
            + '", "'
            + cpp_escape(keyframe["track_type"])
            + f'", {keyframe["keyframe_index"]}, '
            + f'{keyframe["frame"]:.6f}f, {keyframe["value"]:.6f}f, '
            + f'{keyframe["in_tangent"]:.6f}f, {keyframe["out_tangent"]:.6f}f, "'
            + cpp_escape(keyframe["interpolation_type"])
            + '", "'
            + cpp_escape(keyframe["provenance"])
            + '" },'
        )

    lines.extend([
        "    }};",
        "",
        f"    static constexpr std::array<SfxCueCandidate, {len(payload.get('sfx_cue_candidates', []))}> kSfxCueCandidates =",
        "    {{",
    ])

    for candidate in payload.get("sfx_cue_candidates", []):
        lines.append(
            '        { "'
            + cpp_escape(candidate["screen_group"])
            + '", "'
            + cpp_escape(candidate["project"])
            + '", "'
            + cpp_escape(candidate["scene"])
            + '", "'
            + cpp_escape(candidate["action"])
            + '", "'
            + cpp_escape(candidate["bank_name"])
            + '", "'
            + cpp_escape(candidate["cue_name"])
            + '", "'
            + cpp_escape(candidate["runtime_hook"])
            + '", "'
            + cpp_escape(candidate["ghidra_xref"])
            + '", "'
            + cpp_escape(candidate["provenance"])
            + '" },'
        )

    lines.extend(["    }};", "}", ""])
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a starter native component map from extracted .yncp UI projects.")
    parser.add_argument("--root", default="extracted_assets/full_install_archives", help="Fully extracted archive root.")
    parser.add_argument("--output", default="research_uiux/data/yncp_native_component_map.json", help="Output JSON path.")
    parser.add_argument("--markdown", default="research_uiux/YNCP_NATIVE_COMPONENT_MAP.md", help="Output Markdown summary path.")
    parser.add_argument("--output-header", default="UnleashedRecomp/patches/ui_lab_yncp_native_component_map.generated.h", help="Output C++ header path.")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    project_paths = discover_candidate_projects(root)
    texture_file_index = build_texture_file_index(root)
    project_records = [build_project_record(path, root, texture_file_index) for path in project_paths]

    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for record in project_records:
        grouped[record["screen_group"]].append(record)

    payload = {
        "input_root": root.as_posix(),
        "project_count": len(project_records),
        "screen_groups": {group: sorted(records, key=lambda item: item["relative_path"]) for group, records in sorted(grouped.items())},
        "preview_draw_command_count": sum(len(record.get("preview_draw_commands", [])) for record in project_records),
        "scene_draw_command_count": sum(len(record.get("scene_draw_commands", [])) for record in project_records),
        "animation_track_keyframe_count": sum(len(record.get("animation_track_keyframes", [])) for record in project_records),
        "sfx_cue_candidate_count": len(SFX_CUE_CANDIDATES),
        "sfx_cue_candidates": SFX_CUE_CANDIDATES,
    }

    output = Path(args.output)
    if not output.is_absolute():
        output = Path.cwd() / output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")

    markdown = Path(args.markdown)
    if not markdown.is_absolute():
        markdown = Path.cwd() / markdown
    write_markdown(payload, markdown)

    header = Path(args.output_header)
    if not header.is_absolute():
        header = Path.cwd() / header
    write_header(payload, header)

    print(output.as_posix())
    print(markdown.as_posix())
    print(header.as_posix())
    print(f"projects={len(project_records)} groups={len(grouped)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
