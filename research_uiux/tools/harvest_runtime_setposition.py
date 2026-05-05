"""Phase 294: harvest live SetPosition values from the UI Lab probe.

Consumes the JSONL produced by the ui_lab_csd_setposition.jsonl probe
(see UnleashedRecomp/patches/CsdNodeValue_patches.cpp + ui_lab_patches.cpp
OnCsdNodeSetPosition) and produces a runtime_overrides.generated.json
config that render_csd_scene.py consumes via --runtime-override.

Each node_address sees many SetPosition calls; we keep the LAST observed
(x, y) per node as the steady-state value (intro animations finish, then
the UI parks at its final position).

Cross-references the harvested node addresses with the UI Lab's existing
CSD node lookup events (ui_lab_events.jsonl: csd-child-node-lookup,
csd-scene-traversed, etc.) to resolve raw node addresses to
(project, scene_name, cast_path) so the renderer can apply the override
at the right scene level.

Usage:
    python harvest_runtime_setposition.py \\
        --setposition-log path/to/ui_lab_csd_setposition.jsonl \\
        --events-log      path/to/ui_lab_events.jsonl \\
        --output          .../sgfx_hud_runtime_setposition_harvest.generated.json
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any


def parse_jsonl(path: Path) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    if not path.exists():
        return rows
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rows.append(json.loads(line))
            except json.JSONDecodeError:
                continue
    return rows


def harvest_setpositions(setposition_rows: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    """Per node_address, keep the last observed (x, y, hits, last_frame)."""
    latest: dict[str, dict[str, Any]] = {}
    for row in setposition_rows:
        node = row.get("node")
        if not node:
            continue
        latest[node] = {
            "node_address": node,
            "anchor_x_px": float(row.get("x", 0.0)),
            "anchor_y_px": float(row.get("y", 0.0)),
            "observed_hits": int(row.get("hits", 0)),
            "last_observed_frame": int(row.get("frame", 0)),
            "last_observed_time": float(row.get("time", 0.0)),
            "hook": row.get("hook", ""),
            "target_token": row.get("target", ""),
        }
    return latest


def harvest_node_address_to_scene_path(events_rows: list[dict[str, Any]]) -> dict[str, dict[str, str]]:
    """Walk the existing ui_lab_events.jsonl for csd-child-node-lookup
    and csd-node-pointer-resolved entries that bind a node address to a
    (project, scene_name, cast_path) triple. Build a lookup table so
    each SetPosition node can be tagged with the asset-side identity."""
    by_node: dict[str, dict[str, str]] = {}
    detail_re_node = re.compile(r"\bnode=(0x[0-9A-Fa-f]+)")
    detail_re_owner = re.compile(r"\bowner=(0x[0-9A-Fa-f]+)")
    detail_re_path = re.compile(r"\bpath=(\S+)")
    detail_re_scene = re.compile(r"\bscene=(\S+)")
    detail_re_proj = re.compile(r"\bproject=(\S+)")
    detail_re_castname = re.compile(r"\bcastName=(\S+)")
    for row in events_rows:
        ev = row.get("event", "")
        detail = row.get("detail", "")
        if not detail:
            continue
        if "csd" not in ev.lower() and "sonic-hud" not in ev.lower():
            continue
        m_node = detail_re_node.search(detail) or detail_re_owner.search(detail)
        if not m_node:
            continue
        node = m_node.group(1).lower()
        entry = by_node.setdefault(node, {})
        m_path = detail_re_path.search(detail)
        if m_path and "path" not in entry:
            entry["path"] = m_path.group(1)
        m_scene = detail_re_scene.search(detail)
        if m_scene and "scene_name" not in entry:
            entry["scene_name"] = m_scene.group(1)
        m_proj = detail_re_proj.search(detail)
        if m_proj and "project" not in entry:
            entry["project"] = m_proj.group(1)
        m_cast = detail_re_castname.search(detail)
        if m_cast and "cast_name" not in entry:
            entry["cast_name"] = m_cast.group(1)
    return by_node


def normalize_node_address(addr: str) -> str:
    return addr.lower() if addr.startswith("0x") else f"0x{int(addr, 0):08x}"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--setposition-log", required=True, help="ui_lab_csd_setposition.jsonl path")
    parser.add_argument("--events-log", help="ui_lab_events.jsonl path (optional, used to resolve scene names)")
    parser.add_argument("--output", required=True, help="Output JSON path")
    parser.add_argument("--logical-canvas-width", type=int, default=1280)
    parser.add_argument("--logical-canvas-height", type=int, default=720)
    args = parser.parse_args(argv)

    setpos_path = Path(args.setposition_log)
    setpos_rows = parse_jsonl(setpos_path)
    print(f"[harvest] read {len(setpos_rows)} SetPosition events from {setpos_path}", file=sys.stderr)

    events_path = Path(args.events_log) if args.events_log else None
    events_rows = parse_jsonl(events_path) if events_path else []
    if events_path:
        print(f"[harvest] read {len(events_rows)} UI lab events from {events_path}", file=sys.stderr)

    latest_per_node = harvest_setpositions(setpos_rows)
    node_to_scene = harvest_node_address_to_scene_path(events_rows)

    enriched: list[dict[str, Any]] = []
    by_scene: dict[str, list[dict[str, Any]]] = defaultdict(list)
    unresolved = 0
    for node, latest in latest_per_node.items():
        norm = node.lower()
        scene_info = node_to_scene.get(norm, {})
        if not scene_info:
            unresolved += 1
        record = {
            **latest,
            "scene_name": scene_info.get("scene_name", ""),
            "project": scene_info.get("project", ""),
            "csd_path": scene_info.get("path", ""),
            "cast_name": scene_info.get("cast_name", ""),
        }
        enriched.append(record)
        scene_key = scene_info.get("scene_name") or "<unresolved>"
        by_scene[scene_key].append(record)

    enriched.sort(key=lambda r: (r["scene_name"], r["node_address"]))

    summary = {
        "phase": "294",
        "purpose": "Harvested runtime SetPosition (sub_830BB3D0) values from a live UnleashedRecomp session. Each entry is a CCastNode anchor that the screen state machine writes every frame; we keep the steady-state (last observed) value as the effective runtime override for the human-readable port.",
        "logical_canvas_width": args.logical_canvas_width,
        "logical_canvas_height": args.logical_canvas_height,
        "input_setposition_log": str(setpos_path.resolve()),
        "input_events_log": str(events_path.resolve()) if events_path else "",
        "totals": {
            "unique_nodes": len(latest_per_node),
            "resolved_to_scene": len(latest_per_node) - unresolved,
            "unresolved_nodes": unresolved,
        },
        "by_scene": {
            scene: [
                {
                    "node_address": r["node_address"],
                    "anchor_x_px": r["anchor_x_px"],
                    "anchor_y_px": r["anchor_y_px"],
                    "observed_hits": r["observed_hits"],
                    "cast_name": r.get("cast_name", ""),
                    "csd_path": r.get("csd_path", ""),
                    "project": r.get("project", ""),
                }
                for r in sorted(records, key=lambda x: (-x["observed_hits"], x["node_address"]))
            ]
            for scene, records in sorted(by_scene.items())
        },
        "all_records": enriched,
    }

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2)
    print(f"[harvest] wrote {out_path}", file=sys.stderr)
    print(json.dumps({
        "unique_nodes": summary["totals"]["unique_nodes"],
        "resolved_to_scene": summary["totals"]["resolved_to_scene"],
        "unresolved_nodes": summary["totals"]["unresolved_nodes"],
        "scenes_seen": len(by_scene),
        "output": str(out_path),
    }, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
