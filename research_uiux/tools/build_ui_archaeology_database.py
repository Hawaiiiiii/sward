#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from collections import Counter, defaultdict
from pathlib import Path


SYSTEM_DEFS = [
    {
        "system_id": "title_menu",
        "screen_name": "Title Menu",
        "layout_ids": ["ui_mainmenu"],
        "notes": "Title entry flow with intro handoff, menu cursor ownership, and shared fade/message support.",
        "audio_cues": [],
    },
    {
        "system_id": "pause_stack",
        "screen_name": "Pause Stack",
        "layout_ids": ["ui_pause", "ui_general", "ui_help"],
        "notes": "Pause shell plus shared framed-window language and help/prompt overlays.",
        "audio_cues": [],
    },
    {
        "system_id": "status_overlay",
        "screen_name": "Status Overlay",
        "layout_ids": ["ui_status"],
        "notes": "Status/progress presentation with title-shell framing, tags, and progress-bar choreography.",
        "audio_cues": [],
    },
    {
        "system_id": "loading_and_start",
        "screen_name": "Loading And Start/Clear",
        "layout_ids": ["ui_loading", "ui_start"],
        "notes": "Loading, transition, and start/clear handoff screens with time-of-day/cutscene adjacency.",
        "audio_cues": [],
    },
    {
        "system_id": "world_map_stack",
        "screen_name": "World Map Stack",
        "layout_ids": ["ui_worldmap", "ui_worldmap_help"],
        "notes": "World-map info panes, footer/header framing, and world-map help sidecars tied to cursor/camera seams.",
        "audio_cues": ["sys_worldmap_cursor", "sys_worldmap_finaldecide", "bgm_sys_worldmap.csb"],
    },
    {
        "system_id": "town_ui",
        "screen_name": "Town UI",
        "layout_ids": ["ui_balloon", "ui_shop", "ui_townscreen", "ui_mediaroom"],
        "notes": "Town-side balloon/dialog, shop, townscreen, and Media Room layout families extracted from town/common archives.",
        "audio_cues": ["bgm_sys_mediaroom.csb"],
    },
    {
        "system_id": "mission_briefing_and_gate",
        "screen_name": "Mission Briefing And Gate",
        "layout_ids": ["ui_gate", "ui_missionscreen", "ui_misson"],
        "notes": "Mission gate/status framing plus mission briefing and mission-stat counter families.",
        "audio_cues": ["Hint/BossGate.dds"],
    },
    {
        "system_id": "boss_hud",
        "screen_name": "Boss HUD",
        "layout_ids": ["ui_boss_gauge", "ui_boss_name"],
        "notes": "Boss-health/name presentation with authored gauge segments and variant-specific name branches.",
        "audio_cues": ["bgm_boss_day.csb", "bgm_boss_night.csb", "vs_boss_sonic_e.csb", "vs_boss_evil_e.csb"],
    },
    {
        "system_id": "item_result",
        "screen_name": "Item Result",
        "layout_ids": ["ui_itemresult"],
        "notes": "Compact item-result window with framed title/footer and reuse of the general pause-style shell language.",
        "audio_cues": ["bgm_sys_result.csb"],
    },
    {
        "system_id": "mission_result_family",
        "screen_name": "Mission Result Family",
        "layout_ids": ["ui_result", "ui_result_ex"],
        "notes": "Main result and EX/result variants with score blocks, title shells, and replay/new-record highlights.",
        "audio_cues": ["bgm_sys_result.csb", "vs_result_sonic_e.csb", "vs_result_evil_e.csb"],
    },
    {
        "system_id": "save_and_ending",
        "screen_name": "Save And Ending",
        "layout_ids": ["ui_saveicon", "ui_end"],
        "notes": "Autosave icon overlay plus ending/staff-roll text presentation seams.",
        "audio_cues": [],
    },
    {
        "system_id": "tornado_defense",
        "screen_name": "Tornado Defense / EX Stage",
        "layout_ids": ["ui_exstage", "ui_prov_playscreen", "ui_qte"],
        "notes": "EX-stage/Tornado Defense HUD, combat counters, and prompt/QTE-specific visual feedback.",
        "audio_cues": [],
    },
    {
        "system_id": "subtitle_cutscene_presentation",
        "screen_name": "Subtitle / Cutscene Presentation",
        "layout_ids": [],
        "notes": "Subtitle XML, PlayMovie sequence ownership, hide-layer policy, and loading/stage handoff state.",
        "audio_cues": [],
    },
]

VERDICT_ORDER = {"unknown": 0, "contextual": 1, "strong": 2, "direct": 3}
CONFIDENCE_BY_VERDICT = {
    "unknown": "low",
    "contextual": "medium",
    "strong": "medium_high",
    "direct": "high",
}
ANIMATION_STATE_MAP = {
    "intro": "appear",
    "usual": "idle",
    "outro": "exit",
    "scroll": "scroll",
    "size": "resize_or_focus",
    "switch": "switch",
    "move": "move",
    "idle": "idle",
    "select": "selection",
    "ev": "event_variant",
    "so": "standard_or_sonic_variant",
}
BUTTON_ALIASES = {
    "a",
    "b",
    "x",
    "y",
    "lb",
    "rb",
    "lt",
    "rt",
    "back",
    "start",
    "select",
}
TEXTURE_FAMILY_RE = re.compile(r"_[0-9]+$")
WEAK_FUNC_RE = re.compile(r"PPC_WEAK_FUNC\((sub_[0-9A-F]+)\)")
IMPL_RE = re.compile(r"PPC_FUNC_IMPL\(__imp__(sub_[0-9A-F]+)\)")


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def relpath(repo_root: Path, value: str | Path | None) -> str | None:
    if value is None:
        return None
    path = Path(value)
    try:
        return path.resolve().relative_to(repo_root.resolve()).as_posix()
    except Exception:
        return str(value).replace("\\", "/")


def unique_ordered(values):
    seen = set()
    ordered = []
    for value in values:
        if value in seen:
            continue
        seen.add(value)
        ordered.append(value)
    return ordered


def sorted_counter(counter: Counter[str]) -> dict[str, int]:
    items = sorted(counter.items(), key=lambda item: (-item[1], item[0]))
    return {key: value for key, value in items}


def normalize_texture_family(name: str) -> str:
    stem = Path(name).stem.lower()
    return TEXTURE_FAMILY_RE.sub("", stem)


def detect_prompt_casts(scene_summaries: list[dict]) -> list[str]:
    prompts = set()
    for scene in scene_summaries:
        for cast in scene.get("cast_names", []):
            lowered = cast.lower()
            if lowered.startswith("btn_") or lowered in BUTTON_ALIASES:
                prompts.add(cast)
    return sorted(prompts)


def aggregate_layout_details(repo_root: Path, deep_analysis: dict) -> dict[str, dict]:
    aggregated: dict[str, dict] = {}

    for digest in deep_analysis.get("digests", []):
        layout_id = Path(digest["file_name"]).stem.lower()
        entry = aggregated.setdefault(
            layout_id,
            {
                "layout_id": layout_id,
                "layout_files": [],
                "archive_groups": [],
                "scene_names": [],
                "animation_names": [],
                "root_child_names": [],
                "font_names": [],
                "texture_names": [],
                "texture_families": [],
                "prompt_casts": [],
                "animation_family_counts": Counter(),
                "scene_family_counts": Counter(),
                "track_type_counts": Counter(),
                "keyframe_type_counts": Counter(),
                "longest_animations": [],
                "max_group_depth": 0,
                "max_scene_count": 0,
                "max_subimage_count": 0,
                "max_cast_count": 0,
            },
        )

        entry["layout_files"].append(relpath(repo_root, digest["path"]))
        entry["archive_groups"].append(Path(digest["path"]).parent.name)
        entry["root_child_names"].extend(digest.get("root_child_names", []))
        entry["font_names"].extend(digest.get("font_names", []))
        entry["texture_names"].extend(digest.get("texture_names", []))
        entry["texture_families"].extend(normalize_texture_family(name) for name in digest.get("texture_names", []))
        entry["animation_family_counts"].update(digest.get("animation_family_counts", {}))
        entry["scene_family_counts"].update(digest.get("scene_family_counts", {}))
        entry["track_type_counts"].update(digest.get("track_type_counts", {}))
        entry["keyframe_type_counts"].update(digest.get("keyframe_type_counts", {}))
        entry["longest_animations"].extend(digest.get("longest_animations", []))
        entry["max_group_depth"] = max(
            entry["max_group_depth"],
            max((scene.get("max_group_depth", 0) for scene in digest.get("deepest_scenes", [])), default=0),
        )
        totals = digest.get("totals", {})
        entry["max_scene_count"] = max(entry["max_scene_count"], int(totals.get("scene_count", 0)))
        entry["max_subimage_count"] = max(entry["max_subimage_count"], int(totals.get("subimage_count", 0)))

        scene_summaries = digest.get("scene_summaries", [])
        entry["scene_names"].extend(scene.get("scene_name") for scene in scene_summaries if scene.get("scene_name"))
        for scene in scene_summaries:
            entry["animation_names"].extend(scene.get("animation_names", []))
            entry["prompt_casts"].extend(detect_prompt_casts([scene]))
            entry["max_cast_count"] = max(entry["max_cast_count"], int(scene.get("cast_count", 0)))

    for entry in aggregated.values():
        entry["layout_files"] = unique_ordered(sorted(entry["layout_files"]))
        entry["archive_groups"] = unique_ordered(sorted(entry["archive_groups"]))
        entry["scene_names"] = unique_ordered(entry["scene_names"])
        entry["animation_names"] = unique_ordered(entry["animation_names"])
        entry["root_child_names"] = unique_ordered(entry["root_child_names"])
        entry["font_names"] = unique_ordered(entry["font_names"])
        entry["texture_names"] = unique_ordered(entry["texture_names"])
        entry["texture_families"] = unique_ordered(entry["texture_families"])
        entry["prompt_casts"] = unique_ordered(sorted(entry["prompt_casts"]))
        entry["animation_family_counts"] = sorted_counter(entry["animation_family_counts"])
        entry["scene_family_counts"] = sorted_counter(entry["scene_family_counts"])
        entry["track_type_counts"] = sorted_counter(entry["track_type_counts"])
        entry["keyframe_type_counts"] = sorted_counter(entry["keyframe_type_counts"])
        entry["longest_animations"] = sorted(
            entry["longest_animations"],
            key=lambda item: (-float(item.get("frame_count", 0.0)), item.get("animation_name", ""), item.get("scene_name", "")),
        )[:10]

    return aggregated


def build_source_summary_map(ui_code_index: dict) -> dict[str, str]:
    summary_map = {}
    for file_entry in ui_code_index.get("files", []):
        summary_map[file_entry["path"]] = file_entry.get("summary", "")
    return summary_map


def build_patch_map(patch_hooks: dict) -> dict[str, dict]:
    return {entry["path"]: entry for entry in patch_hooks.get("files", [])}


def build_visual_map(repo_root: Path, taxonomy: dict) -> dict[str, dict]:
    result = {}
    for entry in taxonomy.get("layouts", []):
        layout_id = Path(entry["layout_file"]).stem.lower()
        result[layout_id] = {
            "family": entry.get("family"),
            "atlas_path": relpath(repo_root, entry.get("atlas_path")),
            "accent_palette": entry.get("accent_palette", []),
            "texture_count": entry.get("texture_count", 0),
        }
    return result


def build_generated_symbol_index(repo_root: Path, generated_refs: dict) -> tuple[dict[str, dict], str | None]:
    symbol_index: dict[str, dict] = {}

    generation_root = generated_refs.get("generation_root")
    if generation_root:
        generation_root_path = Path(generation_root)
    else:
        generation_root_path = repo_root / "local_build_env/ur103clean/UnleashedRecompLib/ppc"

    if generation_root_path.exists():
        for path in sorted(generation_root_path.rglob("ppc_recomp.*.cpp")):
            with path.open("r", encoding="utf-8", errors="ignore") as handle:
                for line_number, line in enumerate(handle, start=1):
                    weak = WEAK_FUNC_RE.search(line)
                    if weak:
                        symbol = weak.group(1)
                        entry = symbol_index.setdefault(symbol, {"symbol": symbol})
                        entry.setdefault("generated_file", relpath(repo_root, path))
                        entry.setdefault("weak_line", line_number)
                    impl = IMPL_RE.search(line)
                    if impl:
                        symbol = impl.group(1)
                        entry = symbol_index.setdefault(symbol, {"symbol": symbol})
                        entry["generated_file"] = relpath(repo_root, path)
                        entry["impl_line"] = line_number

    for ref in generated_refs.get("references", []):
        entry = symbol_index.setdefault(ref["symbol"], {"symbol": ref["symbol"]})
        entry["address"] = ref.get("address")
        entry["generated_file"] = relpath(repo_root, ref.get("generated_file")) or entry.get("generated_file")
        if ref.get("impl_line") is not None:
            entry["impl_line"] = ref["impl_line"]
        if ref.get("end_line") is not None:
            entry["end_line"] = ref["end_line"]
        if ref.get("mapping_line") is not None:
            entry["mapping_line"] = ref["mapping_line"]
        entry["patch_files"] = unique_ordered(entry.get("patch_files", []) + ref.get("patch_files", []))
        if ref.get("readable_relationship"):
            entry["readable_relationship"] = ref["readable_relationship"]

    return symbol_index, relpath(repo_root, generation_root_path)


def merge_file_refs(existing: dict[str, dict], record: dict) -> None:
    current = existing.get(record["path"])
    if current is None:
        existing[record["path"]] = record
        return
    if VERDICT_ORDER.get(record.get("evidence", "unknown"), 0) > VERDICT_ORDER.get(current.get("evidence", "unknown"), 0):
        current["evidence"] = record["evidence"]
    current["reasons"] = unique_ordered(current.get("reasons", []) + record.get("reasons", []))
    current["line"] = current.get("line") or record.get("line")
    current["summary"] = current.get("summary") or record.get("summary")


def collect_patch_generated(
    patch_files: list[str],
    patch_map: dict[str, dict],
    symbol_index: dict[str, dict],
    precise_symbols: set[str],
) -> list[dict]:
    groups = []
    for patch_file in sorted(set(patch_files)):
        patch_entry = patch_map.get(patch_file)
        if not patch_entry:
            continue
        symbols = []
        for symbol in patch_entry.get("sub_refs", []):
            if symbol in precise_symbols:
                continue
            generated = symbol_index.get(symbol, {"symbol": symbol})
            symbols.append(
                {
                    "symbol": symbol,
                    "address": generated.get("address"),
                    "generated_file": generated.get("generated_file"),
                    "impl_line": generated.get("impl_line"),
                    "readable_relationship": generated.get("readable_relationship"),
                }
            )
        groups.append(
            {
                "patch_file": patch_file,
                "summary": patch_entry.get("summary", ""),
                "ui_tags": patch_entry.get("ui_tags", []),
                "symbol_count": len(symbols),
                "resolved_symbol_count": sum(1 for item in symbols if item.get("generated_file")),
                "symbols": symbols,
            }
        )
    return groups


def derive_state_tags(animation_families: dict[str, int], scene_families: dict[str, int], prompt_casts: list[str]) -> list[str]:
    states = []
    for family in animation_families:
        mapped = ANIMATION_STATE_MAP.get(family.lower())
        if mapped:
            states.append(mapped)
        else:
            states.append(family.lower())

    if "window" in scene_families or "footer" in scene_families:
        states.append("framed_overlay")
    if "select" in scene_families:
        states.append("selection")
    if prompt_casts:
        states.append("button_prompt")
    return unique_ordered(states)


def flatten_generated_symbols(layout_records: list[dict]) -> list[dict]:
    merged = {}
    for record in layout_records:
        for seam in record.get("generated_seams", []):
            merged.setdefault(
                seam["symbol"],
                {
                    "symbol": seam["symbol"],
                    "generated_file": seam.get("generated_file"),
                    "impl_line": seam.get("impl_line"),
                    "readable_relationship": seam.get("readable_relationship"),
                    "patch_files": seam.get("patch_files", []),
                },
            )
        for group in record.get("shared_patch_generated_seams", []):
            for seam in group.get("symbols", []):
                merged.setdefault(
                    seam["symbol"],
                    {
                        "symbol": seam["symbol"],
                        "generated_file": seam.get("generated_file"),
                        "impl_line": seam.get("impl_line"),
                        "readable_relationship": seam.get("readable_relationship"),
                        "patch_files": [group["patch_file"]],
                    },
                )
    return sorted(merged.values(), key=lambda item: item["symbol"])


def build_layout_records(
    repo_root: Path,
    correlation: dict,
    deep_layouts: dict[str, dict],
    patch_map: dict[str, dict],
    source_summary_map: dict[str, str],
    symbol_index: dict[str, dict],
    visual_map: dict[str, dict],
) -> list[dict]:
    layout_records = []

    for entry in correlation.get("entries", []):
        layout_id = entry["layout_id"]
        deep = deep_layouts.get(layout_id, {})

        host_files: dict[str, dict] = {}
        supporting_files: dict[str, dict] = {}
        patch_files: list[str] = []

        for match in entry.get("matches", []):
            record = {
                "path": match["path"],
                "line": match.get("line"),
                "evidence": match.get("evidence", "unknown"),
                "summary": source_summary_map.get(match["path"]),
                "reasons": match.get("reasons", []),
            }
            if "/patches/" in match["path"] or "/ui/" in match["path"] or "UnleashedRecompLib/config/" in match["path"]:
                merge_file_refs(host_files, record)
            else:
                merge_file_refs(supporting_files, record)
            if "/patches/" in match["path"]:
                patch_files.append(match["path"])

        generated_seams = []
        precise_symbols = set()
        for ref in entry.get("generated_refs", []):
            precise_symbols.add(ref["symbol"])
            generated_seams.append(
                {
                    "symbol": ref["symbol"],
                    "generated_file": relpath(repo_root, ref.get("generated_file")),
                    "impl_line": ref.get("impl_line"),
                    "patch_files": ref.get("patch_files", []),
                    "readable_relationship": ref.get("readable_relationship"),
                }
            )
            patch_files.extend(ref.get("patch_files", []))

        shared_patch_generated = collect_patch_generated(patch_files, patch_map, symbol_index, precise_symbols)
        visual = visual_map.get(layout_id, {})

        record = {
            "layout_id": layout_id,
            "role": entry.get("role", "unknown"),
            "verdict": entry.get("verdict", "unknown"),
            "confidence": CONFIDENCE_BY_VERDICT.get(entry.get("verdict", "unknown"), "low"),
            "layout_files": unique_ordered([relpath(repo_root, value) for value in entry.get("paths", [])] + deep.get("layout_files", [])),
            "archive_groups": unique_ordered([entry.get("archive_group")] + deep.get("archive_groups", [])),
            "scene_cues": unique_ordered(entry.get("scene_names", []) + deep.get("scene_names", [])),
            "animation_cues": unique_ordered(entry.get("animation_names", []) + deep.get("animation_names", [])),
            "root_children": deep.get("root_child_names", []),
            "font_names": deep.get("font_names", []),
            "texture_names": deep.get("texture_names", []),
            "texture_families": deep.get("texture_families", []),
            "prompt_casts": deep.get("prompt_casts", []),
            "animation_family_counts": deep.get("animation_family_counts", {}),
            "scene_family_counts": deep.get("scene_family_counts", {}),
            "track_type_counts": deep.get("track_type_counts", {}),
            "keyframe_type_counts": deep.get("keyframe_type_counts", {}),
            "longest_animations": deep.get("longest_animations", []),
            "max_group_depth": deep.get("max_group_depth", 0),
            "max_scene_count": deep.get("max_scene_count", 0),
            "max_subimage_count": deep.get("max_subimage_count", 0),
            "max_cast_count": deep.get("max_cast_count", 0),
            "state_tags": derive_state_tags(
                deep.get("animation_family_counts", {}),
                deep.get("scene_family_counts", {}),
                deep.get("prompt_casts", []),
            ),
            "host_code_files": sorted(host_files.values(), key=lambda item: item["path"]),
            "supporting_files": sorted(supporting_files.values(), key=lambda item: item["path"]),
            "generated_seams": sorted(generated_seams, key=lambda item: item["symbol"]),
            "shared_patch_generated_seams": shared_patch_generated,
            "visual_taxonomy": visual,
        }
        layout_records.append(record)

    return sorted(layout_records, key=lambda item: item["layout_id"])


def merge_records(records: list[dict]) -> list[dict]:
    merged: dict[str, dict] = {}
    for record in records:
        merge_file_refs(merged, record)
    return sorted(merged.values(), key=lambda item: item["path"])


def best_verdict(verdicts: list[str]) -> str:
    if not verdicts:
        return "unknown"
    return max(verdicts, key=lambda verdict: VERDICT_ORDER.get(verdict, 0))


def build_subtitle_system(repo_root: Path, subtitle_data: dict) -> dict:
    correlated = subtitle_data.get("correlated_scenes", [])
    max_duration = max((scene.get("max_duration_seconds", 0.0) for scene in correlated), default=0.0)
    max_frames = max((scene.get("project_duration_frames_range", [0, 0])[1] for scene in correlated), default=0)

    return {
        "system_id": "subtitle_cutscene_presentation",
        "screen_name": "Subtitle / Cutscene Presentation",
        "layout_ids": [],
        "layout_files": [],
        "host_code_files": [
            {"path": "UnleashedRecomp/app.cpp", "summary": "Applies the subtitles option to the application document.", "line": 79, "evidence": "strong", "reasons": ["Readable app bootstrap applies subtitle config to the runtime document."]},
            {"path": "UnleashedRecomp/locale/config_locale.cpp", "summary": "Defines subtitles and cutscene-aspect-ratio options.", "line": 486, "evidence": "strong", "reasons": ["Readable locale/config layer exposes subtitle and cutscene-aspect-ratio controls."]},
            {"path": "UnleashedRecomp/patches/video_patches.cpp", "summary": "Movie wrapper patch layer used by subtitle/cutscene presentation notes.", "line": 7, "evidence": "contextual", "reasons": ["Video patch layer references play-movie wrapper structures used in cutscene presentation."]},
        ],
        "supporting_files": [],
        "generated_seams": [],
        "patch_generated_symbols": [],
        "texture_families": [],
        "scene_cues": [scene["scene_id"] for scene in correlated[:16]],
        "state_tags": ["prepare_movie", "subtitle_window", "loading_handoff", "stage_change"],
        "transition_tags": ["playmovie", "hide_layer", "autosave_handoff"],
        "prompt_casts": [],
        "audio_event_cues": [],
        "confidence": "high" if correlated else "low",
        "verdict": "direct" if correlated else "unknown",
        "timing_highlights": [
            {
                "animation_name": "subtitle_max_duration",
                "frame_count": round(float(max_duration) * 60.0, 3),
                "timeline_seconds": round(float(max_duration), 6),
                "scene_name": "subtitle_windows",
                "node_path": "subtitle",
                "total_keyframes": len(correlated),
            },
            {
                "animation_name": "playmovie_project_duration",
                "frame_count": max_frames,
                "timeline_seconds": round(float(max_frames) / 60.0, 6) if max_frames else 0.0,
                "scene_name": "playmovie_sequences",
                "node_path": "cutscene",
                "total_keyframes": subtitle_data.get("matched_scene_count", 0),
            },
        ],
        "notes": "Subtitle XML plus PlayMovie sequence correlation lives outside the CSD layout layer but belongs in the same cross-reference database for cutscene UI archaeology.",
        "subtitle_scene_count": subtitle_data.get("matched_scene_count", 0),
        "subtitle_languages": subtitle_data.get("languages", []),
        "playmovie_count": subtitle_data.get("playmovie_count", 0),
    }


def build_system_records(layout_records: list[dict], subtitle_data: dict) -> list[dict]:
    by_layout = {record["layout_id"]: record for record in layout_records}
    systems = []

    for definition in SYSTEM_DEFS:
        if definition["system_id"] == "subtitle_cutscene_presentation":
            systems.append(build_subtitle_system(Path("."), subtitle_data))
            continue

        selected = [by_layout[layout_id] for layout_id in definition["layout_ids"] if layout_id in by_layout]
        if not selected:
            continue

        host_files = merge_records([file for record in selected for file in record.get("host_code_files", [])])
        supporting_files = merge_records([file for record in selected for file in record.get("supporting_files", [])])
        generated_seams = flatten_generated_symbols(selected)

        patch_symbols = {}
        for record in selected:
            for group in record.get("shared_patch_generated_seams", []):
                for symbol in group.get("symbols", []):
                    patch_symbols.setdefault(
                        symbol["symbol"],
                        {
                            "symbol": symbol["symbol"],
                            "generated_file": symbol.get("generated_file"),
                            "impl_line": symbol.get("impl_line"),
                            "readable_relationship": symbol.get("readable_relationship"),
                            "patch_file": group["patch_file"],
                        },
                    )

        all_longest = []
        for record in selected:
            all_longest.extend(record.get("longest_animations", []))
        all_longest = sorted(
            all_longest,
            key=lambda item: (-float(item.get("frame_count", 0.0)), item.get("animation_name", ""), item.get("scene_name", "")),
        )[:8]

        verdict = best_verdict([record["verdict"] for record in selected])
        system_record = {
            "system_id": definition["system_id"],
            "screen_name": definition["screen_name"],
            "layout_ids": [record["layout_id"] for record in selected],
            "layout_files": unique_ordered([path for record in selected for path in record.get("layout_files", [])]),
            "host_code_files": host_files,
            "supporting_files": supporting_files,
            "generated_seams": generated_seams,
            "patch_generated_symbols": sorted(patch_symbols.values(), key=lambda item: item["symbol"]),
            "texture_families": unique_ordered([name for record in selected for name in record.get("texture_families", [])]),
            "scene_cues": unique_ordered([name for record in selected for name in record.get("scene_cues", [])])[:24],
            "state_tags": unique_ordered([tag for record in selected for tag in record.get("state_tags", [])]),
            "transition_tags": unique_ordered([name.lower() for record in selected for name in record.get("animation_family_counts", {}).keys()]),
            "prompt_casts": unique_ordered([prompt for record in selected for prompt in record.get("prompt_casts", [])]),
            "audio_event_cues": definition["audio_cues"],
            "confidence": CONFIDENCE_BY_VERDICT.get(verdict, "low"),
            "verdict": verdict,
            "timing_highlights": all_longest,
            "notes": definition["notes"],
            "visual_families": unique_ordered(
                [
                    record.get("visual_taxonomy", {}).get("family")
                    for record in selected
                    if record.get("visual_taxonomy", {}).get("family")
                ]
            ),
        }
        systems.append(system_record)

    return systems


def build_payload(
    repo_root: Path,
    correlation_path: Path,
    deep_analysis_path: Path,
    patch_hooks_path: Path,
    ui_code_index_path: Path,
    generated_refs_path: Path,
    subtitle_path: Path,
    visual_path: Path,
    asset_index_path: Path,
) -> dict:
    correlation = read_json(correlation_path)
    deep_analysis = read_json(deep_analysis_path)
    patch_hooks = read_json(patch_hooks_path)
    ui_code_index = read_json(ui_code_index_path)
    generated_refs = read_json(generated_refs_path)
    subtitle_data = read_json(subtitle_path)
    visual_data = read_json(visual_path)
    asset_index = read_json(asset_index_path)

    deep_layouts = aggregate_layout_details(repo_root, deep_analysis)
    source_summary_map = build_source_summary_map(ui_code_index)
    patch_map = build_patch_map(patch_hooks)
    visual_map = build_visual_map(repo_root, visual_data)
    symbol_index, generation_root = build_generated_symbol_index(repo_root, generated_refs)
    layout_records = build_layout_records(repo_root, correlation, deep_layouts, patch_map, source_summary_map, symbol_index, visual_map)
    systems = build_system_records(layout_records, subtitle_data)

    used_generated_symbols = set()
    for system in systems:
        for seam in system.get("generated_seams", []):
            if seam.get("generated_file"):
                used_generated_symbols.add(seam["symbol"])
        for seam in system.get("patch_generated_symbols", []):
            if seam.get("generated_file"):
                used_generated_symbols.add(seam["symbol"])

    phase23_root = repo_root / "extracted_assets/phase23_crossref_archives"
    phase23_layout_files = []
    phase23_file_count = 0
    phase23_archive_count = 0
    if phase23_root.exists():
        phase23_archive_count = len([path for path in phase23_root.iterdir() if path.is_dir()])
        phase23_file_count = sum(1 for path in phase23_root.rglob("*") if path.is_file())
        phase23_layout_files = sorted(relpath(repo_root, path) for path in phase23_root.rglob("*.yncp"))

    asset_entry_count = sum(scan.get("entry_count", 0) for scan in asset_index.get("scans", []))
    extracted_entry_count = 0
    installed_entry_count = 0
    for scan in asset_index.get("scans", []):
        root = scan.get("root", "")
        if root.endswith("/extracted_assets"):
            extracted_entry_count = scan.get("entry_count", 0)
        if root.endswith("/game"):
            installed_entry_count = scan.get("entry_count", 0)

    summary = {
        "merged_layout_count": len(layout_records),
        "parsed_layout_file_count": len(deep_analysis.get("parsed_files", [])),
        "system_count": len(systems),
        "generated_symbol_pool_count": sum(1 for value in symbol_index.values() if value.get("generated_file")),
        "resolved_generated_symbol_count": len(used_generated_symbols),
        "phase23_archive_count": phase23_archive_count,
        "phase23_file_count": phase23_file_count,
        "phase23_layout_count": len(phase23_layout_files),
        "phase23_layout_files": phase23_layout_files,
        "asset_entry_count": asset_entry_count,
        "installed_asset_entry_count": installed_entry_count,
        "extracted_asset_entry_count": extracted_entry_count,
    }

    return {
        "repo_root": repo_root.as_posix(),
        "inputs": {
            "layout_code_correlation": relpath(repo_root, correlation_path),
            "layout_deep_analysis": relpath(repo_root, deep_analysis_path),
            "patch_hooks": relpath(repo_root, patch_hooks_path),
            "ui_code_index": relpath(repo_root, ui_code_index_path),
            "generated_function_refs": relpath(repo_root, generated_refs_path),
            "subtitle_cutscene_presentation": relpath(repo_root, subtitle_path),
            "visual_taxonomy": relpath(repo_root, visual_path),
            "asset_index": relpath(repo_root, asset_index_path),
            "generated_ppc_root": generation_root,
        },
        "summary": summary,
        "layouts": layout_records,
        "systems": systems,
    }


def write_markdown(path: Path, payload: dict) -> None:
    summary = payload["summary"]
    lines = [
        '<p align="right">',
        '    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>',
        "</p>",
        "",
        '# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Full UI Archaeology Cross-Reference',
        "",
        "Machine-readable inventory: `research_uiux/data/ui_archaeology_database.json`",
        "",
        "## Summary",
        "",
        f"- Merged layout IDs cross-referenced: `{summary['merged_layout_count']}`",
        f"- Parsed layout files in the deep-analysis layer: `{summary['parsed_layout_file_count']}`",
        f"- Screen/system groups assembled: `{summary['system_count']}`",
        f"- Asset-index entries after the current re-scan: `{summary['asset_entry_count']}`",
        f"- Resolved generated PPC symbols available to the archaeology layer: `{summary['resolved_generated_symbol_count']}`",
        f"- Total generated PPC symbol pool indexed for fallback seam resolution: `{summary['generated_symbol_pool_count']}`",
        "",
        "## Phase 23 Extraction Batch",
        "",
        f"- Dedicated extraction root: `extracted_assets/phase23_crossref_archives`",
        f"- Archive folders extracted in this batch: `{summary['phase23_archive_count']}`",
        f"- Files extracted in this batch: `{summary['phase23_file_count']}`",
        f"- New layout files surfaced in this batch: `{summary['phase23_layout_count']}`",
    ]

    if summary["phase23_layout_files"]:
        lines.append(f"- Phase 23 layout files: `{', '.join(summary['phase23_layout_files'])}`")

    lines.extend(
        [
            "",
            "> [!NOTE]",
            "> `generated_seams` are precise symbol links already established by earlier mapping work.",
            "> `patch_generated_symbols` are broader patch-file seam banks resolved against the local translated PPC output for this phase.",
            "",
        ]
    )

    for system in payload["systems"]:
        lines.append(f"## `{system['system_id']}` - {system['screen_name']}")
        lines.append("")
        lines.append(f"- Confidence: `{system['confidence']}` (`{system['verdict']}` evidence)")
        if system.get("layout_ids"):
            lines.append(f"- Layout IDs: `{', '.join(system['layout_ids'])}`")
        if system.get("layout_files"):
            lines.append(f"- Layout files: `{', '.join(system['layout_files'][:8])}`")
        if system.get("state_tags"):
            lines.append(f"- State tags: `{', '.join(system['state_tags'])}`")
        if system.get("transition_tags"):
            lines.append(f"- Transition families: `{', '.join(system['transition_tags'][:12])}`")
        if system.get("prompt_casts"):
            lines.append(f"- Prompt casts: `{', '.join(system['prompt_casts'])}`")
        if system.get("texture_families"):
            lines.append(f"- Texture families: `{', '.join(system['texture_families'][:16])}`")
        if system.get("audio_event_cues"):
            lines.append(f"- Audio/event cues: `{', '.join(system['audio_event_cues'])}`")
        if system.get("visual_families"):
            lines.append(f"- Visual taxonomy families: `{', '.join(system['visual_families'])}`")
        lines.append(f"- Host code files: `{len(system.get('host_code_files', []))}`")
        lines.append(f"- Supporting files: `{len(system.get('supporting_files', []))}`")
        lines.append(f"- Precise generated seams: `{len(system.get('generated_seams', []))}`")
        lines.append(f"- Patch-derived generated seams: `{len(system.get('patch_generated_symbols', []))}`")
        if system.get("timing_highlights"):
            timing_bits = []
            for item in system["timing_highlights"][:4]:
                timing_bits.append(f"{item.get('scene_name', '<scene>')}::{item.get('animation_name', '<anim>')}={item.get('frame_count', 0)}f")
            lines.append(f"- Timing highlights: `{', '.join(timing_bits)}`")
        lines.append(f"- Why this group matters: {system['notes']}")
        lines.append("")

    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a unified UI archaeology database from layout, code, patch, subtitle, and generated-seam artifacts.")
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument("--layout-correlation", default="research_uiux/data/layout_code_correlation.json")
    parser.add_argument("--layout-deep-analysis", default="research_uiux/data/layout_deep_analysis.json")
    parser.add_argument("--patch-hooks", default="research_uiux/data/patch_hooks.json")
    parser.add_argument("--ui-code-index", default="research_uiux/data/ui_code_index.json")
    parser.add_argument("--generated-refs", default="research_uiux/data/generated_function_refs.json")
    parser.add_argument("--subtitle-data", default="research_uiux/data/subtitle_cutscene_presentation.json")
    parser.add_argument("--visual-taxonomy", default="research_uiux/data/visual_taxonomy.json")
    parser.add_argument("--asset-index", default="research_uiux/data/asset_index.json")
    parser.add_argument("--output-json", default="research_uiux/data/ui_archaeology_database.json")
    parser.add_argument("--output-md", default="research_uiux/FULL_UI_ARCHAEOLOGY_CROSS_REFERENCE.md")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()

    def resolve(path_value: str) -> Path:
        path = Path(path_value)
        return path if path.is_absolute() else (repo_root / path)

    output_json = resolve(args.output_json)
    output_md = resolve(args.output_md)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)

    payload = build_payload(
        repo_root=repo_root,
        correlation_path=resolve(args.layout_correlation),
        deep_analysis_path=resolve(args.layout_deep_analysis),
        patch_hooks_path=resolve(args.patch_hooks),
        ui_code_index_path=resolve(args.ui_code_index),
        generated_refs_path=resolve(args.generated_refs),
        subtitle_path=resolve(args.subtitle_data),
        visual_path=resolve(args.visual_taxonomy),
        asset_index_path=resolve(args.asset_index),
    )

    output_json.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    write_markdown(output_md, payload)
    print(output_json)
    print(output_md)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
