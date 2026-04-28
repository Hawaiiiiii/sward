#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import struct
from collections import Counter
from pathlib import Path
from typing import Any


SHIFT_JIS = "shift_jis"
ANIMATION_FLAG_BITS = [
    (1, "HideFlag"),
    (2, "XPosition"),
    (4, "YPosition"),
    (8, "Rotation"),
    (16, "XScale"),
    (32, "YScale"),
    (64, "SubImage"),
    (128, "Color"),
    (256, "GradientTL"),
    (512, "GradientBL"),
    (1024, "GradientTR"),
    (2048, "GradientBR"),
]
KEYFRAME_TYPE_NAMES = {
    0: "Const",
    1: "Linear",
    2: "Hermite",
}
NAME_FAMILY_TAGS = [
    "intro",
    "usual",
    "outro",
    "idle",
    "select",
    "move",
    "scroll",
    "size",
    "switch",
    "loop",
    "rev",
    "window",
    "footer",
    "gauge",
    "name",
    "event",
    "so",
    "ev",
    "chip",
    "tails",
    "sonic",
    "360",
    "ps3",
]


class ParseError(RuntimeError):
    pass


class BinaryView:
    def __init__(self, data: bytes) -> None:
        self.data = data

    def _unpack(self, fmt: str, offset: int) -> Any:
        return struct.unpack_from(fmt, self.data, offset)[0]

    def u32(self, offset: int, endian: str) -> int:
        return self._unpack(f"{endian}I", offset)

    def i32(self, offset: int, endian: str) -> int:
        return self._unpack(f"{endian}i", offset)

    def f32(self, offset: int, endian: str) -> float:
        return self._unpack(f"{endian}f", offset)

    def bytes4(self, offset: int) -> bytes:
        return self.data[offset : offset + 4]

    def ascii4(self, offset: int) -> str:
        return self.bytes4(offset).decode("ascii", errors="replace")

    def read_c_string(self, offset: int) -> str:
        if offset <= 0 or offset >= len(self.data):
            return ""
        end = self.data.find(b"\x00", offset)
        if end == -1:
            end = len(self.data)
        return self.data[offset:end].decode(SHIFT_JIS, errors="replace")


def round_float(value: float) -> float:
    return round(value, 6)


def parse_string(view: BinaryView, origin: int, rel_offset: int) -> str:
    if rel_offset == 0:
        return ""
    return view.read_c_string(origin + rel_offset)


def parse_scene_id(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    name_offset = view.u32(offset, endian)
    return {
        "name": parse_string(view, origin, name_offset),
        "index": view.u32(offset + 4, endian),
    }


def parse_node_dictionary(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    name_offset = view.u32(offset, endian)
    return {
        "name": parse_string(view, origin, name_offset),
        "index": view.u32(offset + 4, endian),
    }


def parse_subimage(view: BinaryView, offset: int, endian: str) -> dict[str, Any]:
    return {
        "texture_index": view.u32(offset, endian),
        "top_left": [
            round_float(view.f32(offset + 4, endian)),
            round_float(view.f32(offset + 8, endian)),
        ],
        "bottom_right": [
            round_float(view.f32(offset + 12, endian)),
            round_float(view.f32(offset + 16, endian)),
        ],
    }


def parse_cast_info(view: BinaryView, offset: int, endian: str) -> dict[str, Any]:
    return {
        "hide_flag": view.i32(offset, endian),
        "translation": [
            round_float(view.f32(offset + 4, endian)),
            round_float(view.f32(offset + 8, endian)),
        ],
        "rotation": round_float(view.f32(offset + 12, endian)),
        "scale": [
            round_float(view.f32(offset + 16, endian)),
            round_float(view.f32(offset + 20, endian)),
        ],
        "subimage": round_float(view.f32(offset + 24, endian)),
        "color": f"0x{view.u32(offset + 28, endian):08X}",
        "gradient_top_left": f"0x{view.u32(offset + 32, endian):08X}",
        "gradient_bottom_left": f"0x{view.u32(offset + 36, endian):08X}",
        "gradient_top_right": f"0x{view.u32(offset + 40, endian):08X}",
        "gradient_bottom_right": f"0x{view.u32(offset + 44, endian):08X}",
        "field30": view.u32(offset + 48, endian),
        "field34": view.u32(offset + 52, endian),
        "field38": view.u32(offset + 56, endian),
    }


def parse_cast_material(view: BinaryView, offset: int, endian: str) -> dict[str, Any]:
    indices = [view.i32(offset + (4 * index), endian) for index in range(32)]
    return {
        "subimage_indices": indices,
        "used_subimage_indices": [value for value in indices if value >= 0],
    }


def parse_cast(view: BinaryView, offset: int, origin: int, endian: str, version: int) -> dict[str, Any]:
    field00 = view.u32(offset, endian)
    field04 = view.u32(offset + 4, endian)
    is_enabled = view.u32(offset + 8, endian)

    top_left = [round_float(view.f32(offset + 12, endian)), round_float(view.f32(offset + 16, endian))]
    bottom_left = [round_float(view.f32(offset + 20, endian)), round_float(view.f32(offset + 24, endian))]
    top_right = [round_float(view.f32(offset + 28, endian)), round_float(view.f32(offset + 32, endian))]
    bottom_right = [round_float(view.f32(offset + 36, endian)), round_float(view.f32(offset + 40, endian))]

    field2c = view.u32(offset + 44, endian)
    cast_info_offset = view.u32(offset + 48, endian)
    field34 = view.u32(offset + 52, endian)
    field38 = view.u32(offset + 56, endian)
    subimage_count = view.u32(offset + 60, endian)
    cast_material_offset = view.u32(offset + 64, endian)
    font_characters_offset = view.u32(offset + 68, endian)
    font_name_offset = view.u32(offset + 72, endian)
    font_spacing_adjustment = round_float(view.f32(offset + 76, endian))

    width = None
    height = None
    cast_offset = None
    if version >= 3:
        width = view.u32(offset + 80, endian)
        height = view.u32(offset + 84, endian)
        cast_offset = [
            round_float(view.f32(offset + 96, endian)),
            round_float(view.f32(offset + 100, endian)),
        ]
    else:
        width = int((bottom_right[0] - bottom_left[0]) * 1280)
        height = int((bottom_left[1] - top_left[1]) * 720)

    parsed = {
        "field00": field00,
        "field04": field04,
        "is_enabled": is_enabled,
        "top_left": top_left,
        "bottom_left": bottom_left,
        "top_right": top_right,
        "bottom_right": bottom_right,
        "field2c": field2c,
        "field34": field34,
        "field38": field38,
        "subimage_count": subimage_count,
        "font_characters": parse_string(view, origin, font_characters_offset),
        "font_name": parse_string(view, origin, font_name_offset),
        "font_spacing_adjustment": font_spacing_adjustment,
        "width": width,
        "height": height,
        "offset": cast_offset,
    }
    if cast_info_offset:
        parsed["cast_info"] = parse_cast_info(view, origin + cast_info_offset, endian)
    if cast_material_offset:
        parsed["cast_material"] = parse_cast_material(view, origin + cast_material_offset, endian)
    return parsed


def parse_cast_group(view: BinaryView, offset: int, origin: int, endian: str, version: int) -> dict[str, Any]:
    cast_count = view.u32(offset, endian)
    cast_table_offset = view.u32(offset + 4, endian)
    root_cast_index = view.u32(offset + 8, endian)
    cast_hierarchy_tree_offset = view.u32(offset + 12, endian)

    cast_offsets = [
        view.u32(origin + cast_table_offset + (4 * index), endian)
        for index in range(cast_count)
    ]
    casts = [
        parse_cast(view, origin + cast_offset, origin, endian, version)
        for cast_offset in cast_offsets
    ]
    hierarchy = [
        {
            "child_index": view.i32(origin + cast_hierarchy_tree_offset + (8 * index), endian),
            "next_index": view.i32(origin + cast_hierarchy_tree_offset + (8 * index) + 4, endian),
        }
        for index in range(cast_count)
    ]
    return {
        "root_cast_index": root_cast_index,
        "casts": casts,
        "hierarchy": hierarchy,
    }


def parse_cast_dictionary(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    name_offset = view.u32(offset, endian)
    return {
        "name": parse_string(view, origin, name_offset),
        "group_index": view.u32(offset + 4, endian),
        "cast_index": view.u32(offset + 8, endian),
    }


def parse_animation_dictionary(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    name_offset = view.u32(offset, endian)
    return {
        "name": parse_string(view, origin, name_offset),
        "index": view.u32(offset + 4, endian),
    }


def decode_animation_flags(flags: int) -> list[str]:
    return [name for bit, name in ANIMATION_FLAG_BITS if flags & bit]


def parse_keyframe(view: BinaryView, offset: int, endian: str) -> dict[str, Any]:
    keyframe_type = view.u32(offset + 8, endian)
    value_raw_bits = view.u32(offset + 4, endian)
    return {
        "frame": view.u32(offset, endian),
        "value": round_float(view.f32(offset + 4, endian)),
        "value_raw_bits": f"0x{value_raw_bits:08X}",
        "type": KEYFRAME_TYPE_NAMES.get(keyframe_type, f"Unknown_{keyframe_type}"),
        "in_tangent": round_float(view.f32(offset + 12, endian)),
        "out_tangent": round_float(view.f32(offset + 16, endian)),
        "field14": view.u32(offset + 20, endian),
    }


def parse_cast_animation_subdata(view: BinaryView, offset: int, origin: int, endian: str, track_type: str) -> dict[str, Any]:
    keyframe_count = view.u32(offset + 4, endian)
    data_offset = view.u32(offset + 8, endian)
    keyframes = [
        parse_keyframe(view, origin + data_offset + (24 * index), endian)
        for index in range(keyframe_count)
    ] if data_offset else []
    if track_type == "Color" or track_type.startswith("Gradient"):
        for keyframe in keyframes:
            keyframe["packed_rgba"] = keyframe["value_raw_bits"]
    return {
        "field00": view.u32(offset, endian),
        "track_type": track_type,
        "keyframe_count": keyframe_count,
        "keyframes": keyframes,
    }


def parse_cast_animation_data(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    flags = view.u32(offset, endian)
    data_offset = view.u32(offset + 4, endian)
    track_types = decode_animation_flags(flags)
    sub_data = [
        parse_cast_animation_subdata(view, origin + data_offset + (12 * index), origin, endian, track_type)
        for index, track_type in enumerate(track_types)
    ] if data_offset else []
    return {
        "flags": flags,
        "track_types": track_types,
        "sub_data": sub_data,
    }


def parse_group_animation_data(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    cast_count = view.u32(offset, endian)
    cast_data_offset = view.u32(offset + 4, endian)
    casts = [
        parse_cast_animation_data(view, origin + cast_data_offset + (8 * index), origin, endian)
        for index in range(cast_count)
    ] if cast_data_offset else []
    return {
        "cast_count": cast_count,
        "casts": casts,
    }


def parse_animation_keyframe_data(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    group_count = view.u32(offset, endian)
    group_data_offset = view.u32(offset + 4, endian)
    groups = [
        parse_group_animation_data(view, origin + group_data_offset + (8 * index), origin, endian)
        for index in range(group_count)
    ] if group_data_offset else []
    return {
        "group_count": group_count,
        "groups": groups,
    }


def parse_animation_frame_data(view: BinaryView, offset: int, endian: str) -> dict[str, Any]:
    return {
        "field00": view.u32(offset, endian),
        "frame_count": round_float(view.f32(offset + 4, endian)),
    }


def parse_scene(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    version = view.u32(offset, endian)
    z_index = round_float(view.f32(offset + 4, endian))
    animation_framerate = round_float(view.f32(offset + 8, endian))
    field0c = view.u32(offset + 12, endian)
    field10 = round_float(view.f32(offset + 16, endian))
    data1_count = view.u32(offset + 20, endian)
    data1_offset = view.u32(offset + 24, endian)
    subimages_count = view.u32(offset + 28, endian)
    subimages_offset = view.u32(offset + 32, endian)
    group_count = view.u32(offset + 36, endian)
    cast_group_table_offset = view.u32(offset + 40, endian)
    cast_count = view.u32(offset + 44, endian)
    cast_dictionary_offset = view.u32(offset + 48, endian)
    animation_count = view.u32(offset + 52, endian)
    animation_keyframe_data_list_offset = view.u32(offset + 56, endian)
    animation_dictionary_offset = view.u32(offset + 60, endian)
    aspect_ratio = round_float(view.f32(offset + 64, endian))
    animation_frame_data_list_offset = view.u32(offset + 68, endian)
    animation_cast_table_offset = view.u32(offset + 72, endian)

    subimages = [
        parse_subimage(view, origin + subimages_offset + (20 * index), endian)
        for index in range(subimages_count)
    ]
    cast_groups = [
        parse_cast_group(view, origin + cast_group_table_offset + (16 * index), origin, endian, version)
        for index in range(group_count)
    ]
    cast_dictionaries = [
        parse_cast_dictionary(view, origin + cast_dictionary_offset + (12 * index), origin, endian)
        for index in range(cast_count)
    ]
    animation_keyframe_data_list = [
        parse_animation_keyframe_data(view, origin + animation_keyframe_data_list_offset + (8 * index), origin, endian)
        for index in range(animation_count)
    ]
    animation_dictionaries = [
        parse_animation_dictionary(view, origin + animation_dictionary_offset + (8 * index), origin, endian)
        for index in range(animation_count)
    ]
    animation_frame_data_list = [
        parse_animation_frame_data(view, origin + animation_frame_data_list_offset + (8 * index), endian)
        for index in range(animation_count)
    ]

    return {
        "version": version,
        "z_index": z_index,
        "animation_framerate": animation_framerate,
        "field0c": field0c,
        "field10": field10,
        "data1_count": data1_count,
        "data1_offset": data1_offset,
        "subimages_count": subimages_count,
        "group_count": group_count,
        "cast_count": cast_count,
        "animation_count": animation_count,
        "animation_keyframe_data_list_offset": animation_keyframe_data_list_offset,
        "animation_frame_data_list_offset": animation_frame_data_list_offset,
        "animation_cast_table_offset": animation_cast_table_offset,
        "aspect_ratio": aspect_ratio,
        "subimages": subimages,
        "cast_groups": cast_groups,
        "cast_dictionaries": cast_dictionaries,
        "animation_keyframe_data_list": animation_keyframe_data_list,
        "animation_dictionaries": animation_dictionaries,
        "animation_frame_data_list": animation_frame_data_list,
    }


def parse_csd_node(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    scene_count = view.u32(offset, endian)
    scene_table_offset = view.u32(offset + 4, endian)
    scene_id_table_offset = view.u32(offset + 8, endian)
    node_count = view.u32(offset + 12, endian)
    node_list_offset = view.u32(offset + 16, endian)
    node_dictionary_offset = view.u32(offset + 20, endian)

    scene_offsets = [
        view.u32(origin + scene_table_offset + (4 * index), endian)
        for index in range(scene_count)
    ]
    scenes = [
        parse_scene(view, origin + scene_offset, origin, endian)
        for scene_offset in scene_offsets
    ]
    scene_ids = [
        parse_scene_id(view, origin + scene_id_table_offset + (8 * index), origin, endian)
        for index in range(scene_count)
    ]
    children = [
        parse_csd_node(view, origin + node_list_offset + (24 * index), origin, endian)
        for index in range(node_count)
    ]
    node_dictionaries = [
        parse_node_dictionary(view, origin + node_dictionary_offset + (8 * index), origin, endian)
        for index in range(node_count)
    ]
    return {
        "scene_count": scene_count,
        "node_count": node_count,
        "scene_ids": scene_ids,
        "scenes": scenes,
        "node_dictionaries": node_dictionaries,
        "children": children,
    }


def parse_font(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    character_count = view.u32(offset, endian)
    character_mapping_table_offset = view.u32(offset + 4, endian)
    mappings = []
    for index in range(character_count):
        mapping_offset = origin + character_mapping_table_offset + (8 * index)
        if endian == ">":
            source_character = chr(view._unpack(">H", mapping_offset + 2))
        else:
            source_character = chr(view._unpack("<H", mapping_offset))
        subimage_index = view.u32(mapping_offset + 4, endian)
        mappings.append(
            {
                "source_character": source_character,
                "subimage_index": subimage_index,
            }
        )
    return {
        "character_count": character_count,
        "mappings": mappings,
    }


def parse_font_id(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    name_offset = view.u32(offset, endian)
    return {
        "name": parse_string(view, origin, name_offset),
        "index": view.u32(offset + 4, endian),
    }


def parse_font_list(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    font_count = view.u32(offset, endian)
    font_table_offset = view.u32(offset + 4, endian)
    font_id_table_offset = view.u32(offset + 8, endian)
    fonts = [
        parse_font(view, origin + font_table_offset + (8 * index), origin, endian)
        for index in range(font_count)
    ]
    font_ids = [
        parse_font_id(view, origin + font_id_table_offset + (8 * index), origin, endian)
        for index in range(font_count)
    ]
    return {
        "font_count": font_count,
        "fonts": fonts,
        "font_ids": font_ids,
    }


def parse_ncpj(view: BinaryView, offset: int, endian: str) -> dict[str, Any]:
    origin = offset
    signature = view.ascii4(offset)
    size = view.u32(offset + 4, "<")
    field08 = view.u32(offset + 8, endian)
    field0c = view.u32(offset + 12, endian)
    root_node_offset = view.u32(offset + 16, endian)
    project_name_offset = view.u32(offset + 20, endian)
    dxl_signature = view.ascii4(offset + 24)
    font_list_offset = view.u32(offset + 28, endian)

    parsed = {
        "signature": signature,
        "size": size,
        "field08": field08,
        "field0c": field0c,
        "project_name": parse_string(view, origin, project_name_offset),
        "dxl_signature": dxl_signature,
        "root": parse_csd_node(view, origin + root_node_offset, origin, endian),
    }
    if font_list_offset:
        parsed["fonts"] = parse_font_list(view, origin + font_list_offset, origin, endian)
    return parsed


def parse_texture(view: BinaryView, offset: int, origin: int, endian: str) -> dict[str, Any]:
    name_offset = view.u32(offset, endian)
    return {
        "name": parse_string(view, origin, name_offset),
        "field04": view.u32(offset + 4, endian),
    }


def parse_texture_list(view: BinaryView, offset: int, endian: str) -> dict[str, Any]:
    origin = offset
    signature = view.ascii4(offset)
    size = view.u32(offset + 4, "<")
    list_offset = view.u32(offset + 8, endian)
    field0c = view.u32(offset + 12, endian)
    texture_count = view.u32(offset + 16, endian)
    textures_offset = view.u32(offset + 20, endian)
    data_offset = view.u32(offset + 24, endian) if textures_offset > 24 else 0
    textures = [
        parse_texture(view, origin + textures_offset + (8 * index), origin, endian)
        for index in range(texture_count)
    ]
    return {
        "signature": signature,
        "size": size,
        "list_offset": list_offset,
        "field0c": field0c,
        "texture_count": texture_count,
        "textures_offset": textures_offset,
        "data_offset": data_offset,
        "textures": textures,
    }


def parse_chunk_file(view: BinaryView, offset: int, endian: str) -> dict[str, Any]:
    origin = offset
    signature = view.ascii4(offset)
    header_size = view.u32(offset + 4, "<")
    chunk_count = view.u32(offset + 8, endian)
    next_chunk_offset = view.u32(offset + 12, endian)
    chunk_list_size = view.u32(offset + 16, endian)
    offset_chunk_offset = view.u32(offset + 20, endian)
    offset_chunk_size = view.u32(offset + 24, endian)
    field1c = view.u32(offset + 28, endian)

    next_signature = view.ascii4(origin + next_chunk_offset)
    parsed = {
        "signature": signature,
        "header_size": header_size,
        "chunk_count": chunk_count,
        "next_chunk_offset": next_chunk_offset,
        "chunk_list_size": chunk_list_size,
        "offset_chunk_offset": offset_chunk_offset,
        "offset_chunk_size": offset_chunk_size,
        "field1c": field1c,
        "next_signature": next_signature,
    }
    if next_signature == "NXTL":
        parsed["texture_list"] = parse_texture_list(view, origin + next_chunk_offset, endian)
    else:
        parsed["csdm_project"] = parse_ncpj(view, origin + next_chunk_offset, endian)
    return parsed


def parse_fapc(path: Path) -> dict[str, Any]:
    data = path.read_bytes()
    view = BinaryView(data)
    raw_signature = view.ascii4(0)
    if raw_signature == "FAPC":
        endian = "<"
        endianness_name = "little"
    elif raw_signature == "CPAF":
        endian = ">"
        endianness_name = "big"
    else:
        raise ParseError(f"Unsupported FAPC signature {raw_signature!r} in {path}")

    offset = 4
    resources = []
    for index in range(2):
        content_size = view.u32(offset, endian)
        content_start = offset + 4
        resources.append(
            {
                "index": index,
                "content_size": content_size,
                "content": parse_chunk_file(view, content_start, endian),
            }
        )
        offset = content_start + content_size

    return {
        "path": path.as_posix(),
        "file_name": path.name,
        "stem": path.stem,
        "extension": path.suffix.lower(),
        "size": len(data),
        "raw_signature": raw_signature,
        "endianness": endianness_name,
        "sha256": __import__("hashlib").sha256(data).hexdigest(),
        "resources": resources,
    }


def extract_name_family_tags(name: str) -> list[str]:
    lowered = name.lower()
    return [tag for tag in NAME_FAMILY_TAGS if tag in lowered]


def cast_name_lookup(scene: dict[str, Any]) -> dict[tuple[int, int], str]:
    return {
        (item["group_index"], item["cast_index"]): item["name"]
        for item in scene["cast_dictionaries"]
    }


def compute_group_hierarchy_stats(cast_group: dict[str, Any], names_by_cast: dict[int, str]) -> dict[str, Any]:
    cast_count = len(cast_group["casts"])
    hierarchy = cast_group["hierarchy"]
    roots: list[int] = []
    seen_roots: set[int] = set()
    next_index = cast_group["root_cast_index"]
    while 0 <= next_index < cast_count and next_index not in seen_roots:
        seen_roots.add(next_index)
        roots.append(next_index)
        next_index = hierarchy[next_index]["next_index"]

    visited: set[int] = set()
    leaf_count = 0
    max_depth = 0

    def walk(index: int, depth: int) -> None:
        nonlocal leaf_count, max_depth
        if index in visited or not (0 <= index < cast_count):
            return
        visited.add(index)
        max_depth = max(max_depth, depth)
        child_index = hierarchy[index]["child_index"]
        if child_index < 0:
            leaf_count += 1
            return
        sibling_seen: set[int] = set()
        cursor = child_index
        while 0 <= cursor < cast_count and cursor not in sibling_seen:
            sibling_seen.add(cursor)
            walk(cursor, depth + 1)
            cursor = hierarchy[cursor]["next_index"]

    for root_index in roots:
        walk(root_index, 0)

    return {
        "root_cast_indices": roots,
        "root_cast_names": [names_by_cast.get(index, f"cast_{index}") for index in roots],
        "reachable_cast_count": len(visited),
        "leaf_count": leaf_count,
        "max_depth": max_depth,
    }


def summarize_animation_data(
    scene: dict[str, Any],
    scene_cast_names: dict[tuple[int, int], str],
) -> tuple[list[dict[str, Any]], Counter[str], Counter[str]]:
    animation_summaries: list[dict[str, Any]] = []
    scene_track_type_counts: Counter[str] = Counter()
    scene_keyframe_type_counts: Counter[str] = Counter()

    for index, animation_dictionary in enumerate(scene["animation_dictionaries"]):
        frame_data = scene["animation_frame_data_list"][index] if index < len(scene["animation_frame_data_list"]) else {"field00": 0, "frame_count": 0.0}
        keyframe_data = scene["animation_keyframe_data_list"][index] if index < len(scene["animation_keyframe_data_list"]) else {"groups": []}
        track_type_counts: Counter[str] = Counter()
        keyframe_type_counts: Counter[str] = Counter()
        cast_rollups: list[dict[str, Any]] = []
        total_keyframes = 0

        for group_index, group in enumerate(keyframe_data["groups"]):
            for cast_index, cast_animation in enumerate(group["casts"]):
                cast_name = scene_cast_names.get((group_index, cast_index), f"group{group_index}_cast{cast_index}")
                cast_track_types: list[str] = []
                cast_keyframe_count = 0
                for track in cast_animation["sub_data"]:
                    track_type = track["track_type"]
                    cast_track_types.append(track_type)
                    track_type_counts[track_type] += 1
                    scene_track_type_counts[track_type] += 1
                    cast_keyframe_count += track["keyframe_count"]
                    total_keyframes += track["keyframe_count"]
                    for keyframe in track["keyframes"]:
                        keyframe_type_counts[keyframe["type"]] += 1
                        scene_keyframe_type_counts[keyframe["type"]] += 1
                if cast_track_types:
                    cast_rollups.append(
                        {
                            "group_index": group_index,
                            "cast_index": cast_index,
                            "cast_name": cast_name,
                            "track_count": len(cast_track_types),
                            "track_types": sorted(set(cast_track_types)),
                            "keyframe_count": cast_keyframe_count,
                        }
                    )

        cast_rollups.sort(
            key=lambda item: (-item["keyframe_count"], -item["track_count"], item["cast_name"])
        )
        frame_count = frame_data["frame_count"]
        animation_summaries.append(
            {
                "animation_name": animation_dictionary["name"],
                "animation_index": animation_dictionary["index"],
                "frame_count": frame_count,
                "field00": frame_data["field00"],
                "timeline_seconds": round_float(frame_count / scene["animation_framerate"]) if scene["animation_framerate"] else 0.0,
                "track_type_counts": dict(sorted(track_type_counts.items())),
                "keyframe_type_counts": dict(sorted(keyframe_type_counts.items())),
                "active_cast_count": len(cast_rollups),
                "total_tracks": sum(track_type_counts.values()),
                "total_keyframes": total_keyframes,
                "top_cast_tracks": cast_rollups[:12],
            }
        )

    return animation_summaries, scene_track_type_counts, scene_keyframe_type_counts


def scene_summary(
    scene: dict[str, Any],
    scene_name: str,
    node_path: str,
    texture_names: list[str],
) -> dict[str, Any]:
    sorted_cast_dictionaries = sorted(
        scene["cast_dictionaries"],
        key=lambda item: (item["group_index"], item["cast_index"], item["name"]),
    )
    sorted_animations = sorted(scene["animation_dictionaries"], key=lambda item: (item["index"], item["name"]))

    used_texture_indices = sorted({subimage["texture_index"] for subimage in scene["subimages"]})
    used_texture_names = [
        texture_names[index]
        for index in used_texture_indices
        if 0 <= index < len(texture_names)
    ]
    scene_cast_names = cast_name_lookup(scene)

    font_casts = []
    for group_index, cast_group in enumerate(scene["cast_groups"]):
        for cast_index, cast in enumerate(cast_group["casts"]):
            if cast.get("font_name") or cast.get("font_characters"):
                font_casts.append(
                    {
                        "group_index": group_index,
                        "cast_index": cast_index,
                        "font_name": cast.get("font_name", ""),
                        "font_characters": cast.get("font_characters", ""),
                    }
                )
    group_hierarchy = [
        compute_group_hierarchy_stats(
            cast_group,
            {
                cast_index: scene_cast_names.get((group_index, cast_index), f"cast_{cast_index}")
                for cast_index in range(len(cast_group["casts"]))
            },
        )
        for group_index, cast_group in enumerate(scene["cast_groups"])
    ]
    animation_summaries, track_type_counts, keyframe_type_counts = summarize_animation_data(scene, scene_cast_names)
    frame_counts = [item["frame_count"] for item in animation_summaries]
    animation_family_counts: Counter[str] = Counter()
    for animation_summary in animation_summaries:
        for tag in extract_name_family_tags(animation_summary["animation_name"]):
            animation_family_counts[tag] += 1

    return {
        "scene_name": scene_name,
        "node_path": node_path,
        "version": scene["version"],
        "aspect_ratio": scene["aspect_ratio"],
        "animation_framerate": scene["animation_framerate"],
        "subimage_count": scene["subimages_count"],
        "group_count": scene["group_count"],
        "cast_count": scene["cast_count"],
        "animation_count": scene["animation_count"],
        "cast_names": [item["name"] for item in sorted_cast_dictionaries],
        "animation_names": [item["name"] for item in sorted_animations],
        "used_texture_names": used_texture_names,
        "font_casts": font_casts,
        "group_hierarchy": group_hierarchy,
        "animation_summaries": animation_summaries,
        "track_type_counts": dict(sorted(track_type_counts.items())),
        "keyframe_type_counts": dict(sorted(keyframe_type_counts.items())),
        "animation_family_counts": dict(sorted(animation_family_counts.items())),
        "frame_count_range": [
            min(frame_counts) if frame_counts else 0.0,
            max(frame_counts) if frame_counts else 0.0,
        ],
        "unique_frame_counts": sorted(set(frame_counts)),
    }


def walk_node(
    node: dict[str, Any],
    node_path: str,
    texture_names: list[str],
    scene_summaries: list[dict[str, Any]],
    counters: Counter[str],
) -> None:
    scene_ids = sorted(node["scene_ids"], key=lambda item: item["index"])
    for index, scene in enumerate(node["scenes"]):
        scene_name = scene_ids[index]["name"] if index < len(scene_ids) else f"scene_{index}"
        scene_summaries.append(scene_summary(scene, scene_name, node_path, texture_names))
        counters["scene_count"] += 1
        counters["cast_dictionary_count"] += len(scene["cast_dictionaries"])
        counters["animation_dictionary_count"] += len(scene["animation_dictionaries"])
        counters["subimage_count"] += len(scene["subimages"])

    node_dictionaries = sorted(node["node_dictionaries"], key=lambda item: item["index"])
    for index, child in enumerate(node["children"]):
        child_name = node_dictionaries[index]["name"] if index < len(node_dictionaries) else f"node_{index}"
        counters["node_count"] += 1
        walk_node(child, f"{node_path}/{child_name}", texture_names, scene_summaries, counters)


def canonical_digest(parsed: dict[str, Any]) -> dict[str, Any]:
    project = parsed["resources"][0]["content"].get("csdm_project", {})
    texture_list = parsed["resources"][1]["content"].get("texture_list", {})
    root = project.get("root", {})
    scene_summaries: list[dict[str, Any]] = []
    counters: Counter[str] = Counter()
    counters["node_count"] = 1 if root else 0
    walk_node(root, project.get("project_name", "root"), [item["name"] for item in texture_list.get("textures", [])], scene_summaries, counters)
    scene_family_counts: Counter[str] = Counter()
    animation_family_counts: Counter[str] = Counter()
    track_type_counts: Counter[str] = Counter()
    keyframe_type_counts: Counter[str] = Counter()
    longest_animations: list[dict[str, Any]] = []
    deepest_scenes: list[dict[str, Any]] = []
    unique_frame_counts: set[float] = set()

    for scene in scene_summaries:
        for tag in extract_name_family_tags(scene["scene_name"]):
            scene_family_counts[tag] += 1
        for tag, count in scene["animation_family_counts"].items():
            animation_family_counts[tag] += count
        for track_type, count in scene["track_type_counts"].items():
            track_type_counts[track_type] += count
        for keyframe_type, count in scene["keyframe_type_counts"].items():
            keyframe_type_counts[keyframe_type] += count
        for frame_count in scene["unique_frame_counts"]:
            unique_frame_counts.add(frame_count)
        for animation_summary in scene["animation_summaries"]:
            longest_animations.append(
                {
                    "scene_name": scene["scene_name"],
                    "node_path": scene["node_path"],
                    "animation_name": animation_summary["animation_name"],
                    "frame_count": animation_summary["frame_count"],
                    "timeline_seconds": animation_summary["timeline_seconds"],
                    "total_keyframes": animation_summary["total_keyframes"],
                }
            )
        deepest_scenes.append(
            {
                "scene_name": scene["scene_name"],
                "node_path": scene["node_path"],
                "max_group_depth": max((item["max_depth"] for item in scene["group_hierarchy"]), default=0),
                "animation_count": scene["animation_count"],
                "cast_count": scene["cast_count"],
            }
        )

    longest_animations.sort(
        key=lambda item: (-item["frame_count"], -item["total_keyframes"], item["scene_name"], item["animation_name"])
    )
    deepest_scenes.sort(
        key=lambda item: (-item["max_group_depth"], -item["cast_count"], item["scene_name"])
    )

    return {
        "project_name": project.get("project_name", ""),
        "root_scene_names": [item["name"] for item in sorted(root.get("scene_ids", []), key=lambda value: value["index"])],
        "root_child_names": [item["name"] for item in sorted(root.get("node_dictionaries", []), key=lambda value: value["index"])],
        "font_names": [item["name"] for item in sorted(project.get("fonts", {}).get("font_ids", []), key=lambda value: value["index"])],
        "texture_names": [item["name"] for item in texture_list.get("textures", [])],
        "scene_summaries": scene_summaries,
        "scene_family_counts": dict(sorted(scene_family_counts.items())),
        "animation_family_counts": dict(sorted(animation_family_counts.items())),
        "track_type_counts": dict(sorted(track_type_counts.items())),
        "keyframe_type_counts": dict(sorted(keyframe_type_counts.items())),
        "unique_frame_counts": sorted(unique_frame_counts),
        "longest_animations": longest_animations[:20],
        "deepest_scenes": deepest_scenes[:10],
        "totals": dict(counters),
    }


def discover_files(roots: list[Path]) -> list[Path]:
    files: list[Path] = []
    for root in roots:
        if root.is_file() and root.suffix.lower() in {".xncp", ".yncp"}:
            files.append(root.resolve())
            continue
        if not root.exists():
            continue
        for path in sorted(root.rglob("*")):
            if path.is_file() and path.suffix.lower() in {".xncp", ".yncp"}:
                files.append(path.resolve())
    return files


def main() -> int:
    parser = argparse.ArgumentParser(description="Inspect extracted .xncp/.yncp files.")
    parser.add_argument(
        "--root",
        action="append",
        default=[],
        help="Directory or file to inspect. May be passed multiple times.",
    )
    parser.add_argument(
        "--output",
        default="research_uiux/data/layout_semantics.json",
        help="Output JSON path.",
    )
    args = parser.parse_args()

    roots = [Path(item).resolve() for item in args.root] or [Path(".").resolve()]
    files = discover_files(roots)
    parsed_files = [parse_fapc(path) for path in files]

    digests = []
    grouped: dict[str, list[dict[str, Any]]] = {}
    for parsed in parsed_files:
        digest = canonical_digest(parsed)
        digest["path"] = parsed["path"]
        digest["file_name"] = parsed["file_name"]
        digest["extension"] = parsed["extension"]
        digest["endianness"] = parsed["endianness"]
        digest["sha256"] = parsed["sha256"]
        digests.append(digest)
        grouped.setdefault(parsed["stem"], []).append(digest)

    comparisons = []
    for stem, items in sorted(grouped.items()):
        canonical_items = []
        for item in items:
            comparable = dict(item)
            for key in ("path", "file_name", "extension", "endianness", "sha256"):
                comparable.pop(key, None)
            canonical_items.append(json.dumps(comparable, sort_keys=True))
        comparisons.append(
            {
                "stem": stem,
                "files": [
                    {
                        "file_name": item["file_name"],
                        "endianness": item["endianness"],
                        "extension": item["extension"],
                        "sha256": item["sha256"],
                    }
                    for item in items
                ],
                "semantic_match": len(set(canonical_items)) == 1,
            }
        )

    payload = {
        "roots": [root.as_posix() for root in roots],
        "file_count": len(parsed_files),
        "parsed_files": parsed_files,
        "digests": digests,
        "comparisons": comparisons,
    }

    output = Path(args.output)
    if not output.is_absolute():
        output = Path.cwd() / output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    print(output.as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
