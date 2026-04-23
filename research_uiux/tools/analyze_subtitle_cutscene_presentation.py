#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any
import xml.etree.ElementTree as ET


def parse_bool(text: str | None) -> bool | None:
    if text is None:
        return None
    lowered = text.strip().lower()
    if lowered in {"true", "1", "yes"}:
        return True
    if lowered in {"false", "0", "no"}:
        return False
    return None


def scene_id_from_path(path: Path) -> str:
    return path.parent.name


def language_from_path(path: Path) -> str:
    return path.parents[1].name


def parse_subtitle_resource(path: Path) -> dict[str, Any]:
    root = ET.fromstring(path.read_text(encoding="utf-8"))
    scene_id = scene_id_from_path(path)
    language = language_from_path(path)

    project_start = int(root.findtext("./ProjectInfo/StartFrame", "0"))
    project_end = int(root.findtext("./ProjectInfo/EndFrame", "0"))

    converse_resources: dict[int, dict[str, Any]] = {}
    resource_nodes = root.findall("./ResourceInfo/Resource")
    for node in resource_nodes:
        resource_type = node.findtext("./Type", default="")
        if resource_type != "ConverseData":
            continue
        resource_id = int(node.findtext("./ID", "0"))
        converse_resources[resource_id] = {
            "file_name": node.findtext("./Param/FileName", default=""),
            "group_id": int(node.findtext("./Param/GroupID", "0")),
            "group_name": node.findtext("./Param/GroupIDName", default=""),
            "cell_id": int(node.findtext("./Param/CellID", "0")),
            "cell_name": node.findtext("./Param/CellIDName", default=""),
            "position": node.findtext("./Param/Position", default="UNKNOWN"),
        }

    cues: list[dict[str, Any]] = []
    for trigger in root.findall("./TriggerInfo/Trigger"):
        resource_id = int(trigger.findtext("./ResourceID", "0"))
        if resource_id not in converse_resources:
            continue
        start = int(trigger.findtext("./Frame/Start", "0"))
        end = int(trigger.findtext("./Frame/End", "0"))
        duration_frames = max(0, (end - start) + 1)
        cue = dict(converse_resources[resource_id])
        cue.update(
            {
                "resource_id": resource_id,
                "start_frame": start,
                "end_frame": end,
                "duration_frames": duration_frames,
                "duration_seconds": round(duration_frames / 60.0, 6),
            }
        )
        cues.append(cue)

    positions = sorted({cue["position"] for cue in cues})
    group_names = sorted({cue["group_name"] for cue in cues})
    durations = [cue["duration_frames"] for cue in cues]
    invalid_cues = [cue for cue in cues if cue["end_frame"] < cue["start_frame"]]
    active_cues = [cue for cue in cues if cue["end_frame"] >= cue["start_frame"] and cue["duration_frames"] > 4]

    return {
        "path": path.as_posix(),
        "scene_id": scene_id,
        "language": language,
        "project_start_frame": project_start,
        "project_end_frame": project_end,
        "project_duration_frames": max(0, (project_end - project_start) + 1),
        "project_duration_seconds": round(max(0, (project_end - project_start) + 1) / 60.0, 6),
        "cue_count": len(cues),
        "active_cue_count": len(active_cues),
        "micro_cue_count": len([cue for cue in cues if cue["duration_frames"] <= 4]),
        "invalid_cue_count": len(invalid_cues),
        "first_active_frame": min((cue["start_frame"] for cue in active_cues), default=None),
        "last_active_frame": max((cue["end_frame"] for cue in active_cues), default=None),
        "positions": positions,
        "group_names": group_names,
        "max_duration_frames": max(durations, default=0),
        "max_duration_seconds": round(max(durations, default=0) / 60.0, 6),
        "average_duration_frames": round(sum(durations) / len(durations), 3) if durations else 0.0,
        "average_duration_seconds": round((sum(durations) / len(durations)) / 60.0, 6) if durations else 0.0,
        "cues": cues,
    }


def parse_sequence(path: Path) -> list[dict[str, Any]]:
    root = ET.fromstring(path.read_text(encoding="utf-8", errors="ignore"))
    units = root.findall("./SequenceUnit")
    results: list[dict[str, Any]] = []
    for index, unit in enumerate(units):
        unit_type = unit.findtext("./type", default="")
        if unit_type != "PlayMovie":
            continue
        scene_name = unit.findtext("./param/SceneName", default="")
        if not scene_name:
            continue

        hide_layers = [node.text.strip() for node in unit.findall("./param/HideLayer") if node.text]
        previous_type = units[index - 1].findtext("./type", default="") if index > 0 else None
        next_type = units[index + 1].findtext("./type", default="") if index + 1 < len(units) else None
        next_two = units[index + 2].findtext("./type", default="") if index + 2 < len(units) else None

        results.append(
            {
                "sequence_path": path.as_posix(),
                "sequence_name": path.name,
                "unit_index": index,
                "scene_name": scene_name,
                "file_name": unit.findtext("./param/FileName", default=""),
                "comment": unit.findtext("./param/Comment", default=""),
                "flag_name": unit.findtext("./param/FlagName", default=""),
                "hide_layers": hide_layers,
                "need_loading_display_before": parse_bool(unit.findtext("./param/NeedLoadingDisplayBefore")),
                "need_loading_display_after": parse_bool(unit.findtext("./param/NeedLoadingDisplayAfter")),
                "keep_movie_until_stage_change": parse_bool(unit.findtext("./param/KeepMovieUntilStageChange")),
                "announce_before_prepare": parse_bool(unit.findtext("./param/AnnounceBeforePrepare")),
                "previous_unit_type": previous_type,
                "next_unit_type": next_type,
                "next_two_unit_type": next_two,
            }
        )
    return results


def collect_subtitles(roots: list[Path]) -> list[dict[str, Any]]:
    items: list[dict[str, Any]] = []
    for root in roots:
        if not root.exists():
            continue
        for path in sorted(root.rglob("*.inspire_resource.xml")):
            items.append(parse_subtitle_resource(path))
    return items


def collect_sequences(application_root: Path) -> list[dict[str, Any]]:
    items: list[dict[str, Any]] = []
    for path in sorted(application_root.glob("*.seq.xml")):
        items.extend(parse_sequence(path))
    return items


def summarize(subtitles: list[dict[str, Any]], playmovies: list[dict[str, Any]]) -> dict[str, Any]:
    playmovies_by_scene: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for item in playmovies:
        playmovies_by_scene[item["scene_name"]].append(item)

    positions = Counter()
    languages = Counter()
    family_counts = Counter()
    correlated_scenes: list[dict[str, Any]] = []

    grouped_by_scene: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for entry in subtitles:
        grouped_by_scene[entry["scene_id"]].append(entry)
        languages[entry["language"]] += 1
        for position in entry["positions"]:
            positions[position] += 1
        prefix = entry["scene_id"].split("_", 2)[1] if "_" in entry["scene_id"] else "unknown"
        family_counts[prefix] += 1

    for scene_id, entries in sorted(grouped_by_scene.items()):
        playmovie_matches = playmovies_by_scene.get(scene_id, [])
        representative = sorted(entries, key=lambda item: (item["language"], item["path"]))[0]
        correlated_scenes.append(
            {
                "scene_id": scene_id,
                "languages": sorted({entry["language"] for entry in entries}),
                "subtitle_file_count": len(entries),
                "positions": representative["positions"],
                "cue_count_range": [
                    min(entry["cue_count"] for entry in entries),
                    max(entry["cue_count"] for entry in entries),
                ],
                "project_duration_frames_range": [
                    min(entry["project_duration_frames"] for entry in entries),
                    max(entry["project_duration_frames"] for entry in entries),
                ],
                "max_duration_seconds": max(entry["max_duration_seconds"] for entry in entries),
                "playmovie_count": len(playmovie_matches),
                "playmovie_sequences": sorted({match["sequence_name"] for match in playmovie_matches}),
                "hide_layers": sorted({layer for match in playmovie_matches for layer in match["hide_layers"]}),
                "needs_loading_before": any(match["need_loading_display_before"] for match in playmovie_matches),
                "needs_loading_after": any(match["need_loading_display_after"] for match in playmovie_matches),
                "keeps_movie_until_stage_change": any(match["keep_movie_until_stage_change"] for match in playmovie_matches),
                "announces_before_prepare": any(match["announce_before_prepare"] for match in playmovie_matches),
                "previous_unit_types": sorted({match["previous_unit_type"] for match in playmovie_matches if match["previous_unit_type"]}),
                "next_unit_types": sorted({match["next_unit_type"] for match in playmovie_matches if match["next_unit_type"]}),
            }
        )

    return {
        "subtitle_file_count": len(subtitles),
        "unique_scene_count": len(grouped_by_scene),
        "playmovie_count": len(playmovies),
        "matched_scene_count": len([scene for scene in correlated_scenes if scene["playmovie_count"] > 0]),
        "languages": dict(sorted(languages.items())),
        "position_counts": dict(sorted(positions.items())),
        "scene_family_counts": dict(sorted(family_counts.items())),
        "correlated_scenes": correlated_scenes,
        "subtitle_files": subtitles,
        "playmovie_sequences": playmovies,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Analyze subtitle/cutscene presentation state.")
    parser.add_argument("--application-root", required=True, help="Path to #Application sequence XML root.")
    parser.add_argument("--subtitle-root", action="append", required=True, help="One or more subtitle extraction roots.")
    parser.add_argument("--output", required=True, help="Output JSON path.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    application_root = Path(args.application_root)
    subtitle_roots = [Path(item) for item in args.subtitle_root]
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    subtitles = collect_subtitles(subtitle_roots)
    playmovies = collect_sequences(application_root)
    summary = summarize(subtitles, playmovies)
    output.write_text(json.dumps(summary, indent=2, sort_keys=True), encoding="utf-8")
    print(
        json.dumps(
            {
                "subtitle_file_count": summary["subtitle_file_count"],
                "unique_scene_count": summary["unique_scene_count"],
                "matched_scene_count": summary["matched_scene_count"],
                "playmovie_count": summary["playmovie_count"],
            },
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
