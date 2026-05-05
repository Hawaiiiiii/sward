"""Phase 316: logcat session analyzer.

Reads a UI Lab evidence directory's ui_lab_events.jsonl + the
sister jsonl files (csd_setposition, etc.) and produces:

  * A per-screen timeline: every event grouped by the active CSD
    project at the time, in chronological order.
  * A SFX cue tally per screen: which cues fire, how often, in what
    order, with their inter-cue frame deltas.
  * A state-transition graph: for screens that have *_update events
    (title-menu, world-map, hud-pause, hud-sonic-stage), the
    sequence of distinct field-value tuples observed -- each
    transition is one observed change.

The output is a JSON report keyed by screen group + a flat
human-readable summary. Phase 317+ uses the JSON as the ground
truth diff against each sgfx_*.hpp port.

Usage:
    python analyze_logcat_session.py \\
        --evidence-dir <session-dir>/manual-observer \\
        --output-json out/sgfx_logcat_analysis.json \\
        --output-summary out/sgfx_logcat_summary.txt
"""

from __future__ import annotations

import argparse
import json
import re
from collections import defaultdict, Counter
from pathlib import Path
from typing import Any


def parse_jsonl(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    rows = []
    with path.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rows.append(json.loads(line))
            except json.JSONDecodeError:
                pass
    return rows


# Detail-string parsers. Each event has a free-form `detail` field
# we walk with regex to lift out structured key=value pairs.
_DETAIL_KV = re.compile(r"\b([a-zA-Z][a-zA-Z0-9_]*)=(\S+)")


def parse_detail(detail: str) -> dict[str, str]:
    return {m.group(1): m.group(2) for m in _DETAIL_KV.finditer(detail or "")}


def group_by_screen(events: list[dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    """Group events by the CSD project context that was live when
    the event fired. The `csd-project-made` events advance the
    rolling cursor; subsequent events are assigned to that project
    until the next csd-project-made fires."""
    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    current = "<bootstrap>"
    for evt in events:
        if evt.get("event") == "csd-project-made":
            current = evt.get("detail", current).strip().split()[0] or current
        evt_with_screen = dict(evt)
        evt_with_screen["_screen"] = current
        grouped[current].append(evt_with_screen)
    return grouped


def summarize_sfx_cues(events: list[dict[str, Any]]) -> dict[str, Any]:
    cues_per_screen: dict[str, Counter] = defaultdict(Counter)
    sequence: list[dict[str, Any]] = []
    for evt in events:
        if evt.get("event") != "game-play-sound":
            continue
        kv = parse_detail(evt.get("detail", ""))
        cue = kv.get("cue", "")
        screen = kv.get("csd", evt.get("_screen", ""))
        if cue:
            cues_per_screen[screen][cue] += 1
            sequence.append({
                "frame": evt.get("frame", 0),
                "time": evt.get("time", 0),
                "cue": cue,
                "screen": screen,
            })
    return {
        "per_screen_counts": {k: dict(v) for k, v in cues_per_screen.items()},
        "sequence": sequence,
        "total_distinct_cues": len({c for ev in cues_per_screen.values() for c in ev}),
    }


def summarize_state_transitions(events: list[dict[str, Any]]) -> dict[str, Any]:
    """Find every '*_update' / '*-update' / '*-context' event and
    track distinct field-value tuples to identify transitions."""
    state_machines: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for evt in events:
        ev_name = evt.get("event", "")
        if not (ev_name.endswith("-update") or "-context" in ev_name):
            continue
        kv = parse_detail(evt.get("detail", ""))
        # Drop fields that change every frame and would dominate
        # the diff (frame, time, this-pointer addresses).
        for noisy in ("frame", "time", "this", "ownerAddress"):
            kv.pop(noisy, None)
        state_machines[ev_name].append({
            "frame": evt.get("frame", 0),
            "time": evt.get("time", 0),
            "fields": kv,
        })
    transitions = {}
    for kind, samples in state_machines.items():
        seen_states: list[dict[str, str]] = []
        for s in samples:
            f = s["fields"]
            if not seen_states or seen_states[-1] != f:
                seen_states.append(f)
        transitions[kind] = {
            "samples_total": len(samples),
            "distinct_states": len(seen_states),
            "first_5_states": seen_states[:5],
            "last_5_states": seen_states[-5:],
        }
    return transitions


def summarize_csd_projects(events: list[dict[str, Any]]) -> list[dict[str, Any]]:
    out = []
    for evt in events:
        if evt.get("event") != "csd-project-made":
            continue
        out.append({
            "frame": evt.get("frame", 0),
            "time": evt.get("time", 0),
            "project": evt.get("detail", ""),
        })
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--evidence-dir", required=True)
    parser.add_argument("--output-json", required=True)
    parser.add_argument("--output-summary")
    args = parser.parse_args(argv)

    evdir = Path(args.evidence_dir)
    events = parse_jsonl(evdir / "ui_lab_events.jsonl")
    setpos = parse_jsonl(evdir / "ui_lab_csd_setposition.jsonl")
    print(f"events={len(events)} setpos={len(setpos)}")

    grouped = group_by_screen(events)
    csd_projects = summarize_csd_projects(events)
    sfx = summarize_sfx_cues(events)
    transitions = summarize_state_transitions(events)

    report = {
        "phase": "316",
        "evidence_dir": str(evdir.resolve()),
        "totals": {
            "events": len(events),
            "csd_projects_observed": len(csd_projects),
            "screens_with_events": len(grouped),
            "distinct_sfx_cues": sfx["total_distinct_cues"],
            "state_machine_kinds": len(transitions),
        },
        "csd_project_timeline": csd_projects,
        "sfx_summary": sfx,
        "state_transitions": transitions,
        "per_screen_event_counts": {
            screen: Counter(e.get("event", "") for e in evlist).most_common()
            for screen, evlist in grouped.items()
        },
    }

    out_json = Path(args.output_json)
    out_json.parent.mkdir(parents=True, exist_ok=True)
    with out_json.open("w", encoding="utf-8") as f:
        json.dump(report, f, indent=2, default=str)
    print(f"wrote {out_json}")

    if args.output_summary:
        lines = []
        lines.append(f"=== SGFX logcat session summary ===")
        lines.append(f"evidence_dir: {evdir}")
        lines.append(f"total events: {len(events)}")
        lines.append(f"csd_projects observed: {len(csd_projects)}")
        lines.append(f"distinct SFX cues fired: {sfx['total_distinct_cues']}")
        lines.append("")
        lines.append("CSD project timeline:")
        for p in csd_projects[:30]:
            lines.append(f"  frame={p['frame']:8d} t={p['time']:8.2f}  {p['project']}")
        lines.append("")
        lines.append("SFX cues per screen:")
        for screen, cues in sorted(sfx["per_screen_counts"].items()):
            lines.append(f"  [{screen}]")
            for cue, count in sorted(cues.items(), key=lambda x: -x[1]):
                lines.append(f"    {count:5d}  {cue}")
        lines.append("")
        lines.append("State machine transitions:")
        for kind, info in sorted(transitions.items()):
            lines.append(f"  {kind}: samples={info['samples_total']} distinct={info['distinct_states']}")
        Path(args.output_summary).parent.mkdir(parents=True, exist_ok=True)
        Path(args.output_summary).write_text("\n".join(lines), encoding="utf-8")
        print(f"wrote {args.output_summary}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
