#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import struct
from collections import Counter
from pathlib import Path
from typing import Any


SHIFT_JIS = "shift_jis"


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
    animation_dictionaries = [
        parse_animation_dictionary(view, origin + animation_dictionary_offset + (8 * index), origin, endian)
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
        "animation_dictionaries": animation_dictionaries,
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

    return {
        "project_name": project.get("project_name", ""),
        "root_scene_names": [item["name"] for item in sorted(root.get("scene_ids", []), key=lambda value: value["index"])],
        "root_child_names": [item["name"] for item in sorted(root.get("node_dictionaries", []), key=lambda value: value["index"])],
        "font_names": [item["name"] for item in sorted(project.get("fonts", {}).get("font_ids", []), key=lambda value: value["index"])],
        "texture_names": [item["name"] for item in texture_list.get("textures", [])],
        "scene_summaries": scene_summaries,
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
