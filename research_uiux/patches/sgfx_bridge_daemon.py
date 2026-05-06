"""
Phase 361 -- SGFX <-> sg-preflight Python bridge daemon.

Drop this file at `sg-preflight/sg_preflight/bridge_daemon.py`, then
expose it as a CLI subcommand by adding to `sg_preflight/__main__.py`:

    from sg_preflight import bridge_daemon
    if argv[1] == "bridge-daemon":
        bridge_daemon.main(argv[2:])

Run alongside UnleashedRecomp:

    python -m sg_preflight bridge-daemon \
        --bridge-dir "%APPDATA%\\UnleashedRecomp\\sgfx_bridge"

The daemon:

  1. Watches `<bridge-dir>/events.jsonl` (tail-read newline events).
  2. For each event, dispatches to the corresponding sg_preflight
     command (existing CLI: profile list, launch action, snapshot).
  3. Republishes `<bridge-dir>/state.json` whenever observed state
     changes, atomically via tmp-file + rename so the C++ host's
     mtime poll never reads a half-written file.

The C++ side is `sgfx_bridge.hpp` (in SWARD's runtime_reference);
the schema is mirrored here in `STATE_SCHEMA` / `EVENT_SCHEMA` for
versioning sanity. Bump both sides together when changing fields.
"""

from __future__ import annotations

import argparse
import datetime as _dt
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

STATE_SCHEMA = "sgfx_bridge_state"
EVENT_SCHEMA = "sgfx_bridge_event"
STATE_VERSION = 1
EVENT_VERSION = 1

# Default bridge dir mirrors what UnleashedRecomp uses. On Windows,
# %APPDATA%\UnleashedRecomp\sgfx_bridge\. Override via --bridge-dir.
def _default_bridge_dir() -> Path:
    if sys.platform.startswith("win"):
        base = os.environ.get("APPDATA")
        if base:
            return Path(base) / "UnleashedRecomp" / "sgfx_bridge"
    return Path.home() / ".local" / "share" / "UnleashedRecomp" / "sgfx_bridge"


# --- Mapping from SU UI events -> sg-preflight CLI actions ---
#
# The retail SU title menu has 5 visible rows; on the World Map the
# stage selector exposes 7+ regions; the Pause menu has Continue /
# Restart / Quit; Results acknowledges via accept. Each event from
# the C++ host is translated into a sg-preflight CLI action below.
#
# The mapping is deliberately external (not hard-coded into the C++
# host) so iteration on the menu->action wiring requires only a
# Python restart, not a UnleashedRecomp rebuild.

# Title menu row -> sg-preflight action name. Row IDs come from the
# retail menu order: continue / new_save / settings / dlc / exit.
TITLE_ROW_TO_ACTION = {
    "continue":  {"intent": "resume_last_bundle"},
    "new_save":  {"intent": "open_bundle_picker"},
    "settings":  {"intent": "open_settings"},
    "dlc":       {"intent": "open_extras"},
    "exit":      {"intent": "quit"},
}

# World-map "stage" -> sg-preflight validation action. The retail SU
# world map has 7 continent regions; we repurpose them as the four
# preflight packs + three workflow shortcuts.
WORLDMAP_STAGE_TO_ACTION = {
    "apotos":    {"intent": "run_action", "action_id": "anchors_check"},
    "spagonia":  {"intent": "run_action", "action_id": "constants_check"},
    "mazuri":    {"intent": "run_action", "action_id": "carpaints_check"},
    "holoska":   {"intent": "run_action", "action_id": "project_sanity_check"},
    "chunnan":   {"intent": "open_review_board"},
    "shamar":    {"intent": "open_evidence"},
    "empirecity":{"intent": "open_environment_doctor"},
}

# Pause menu -> action.
PAUSE_ROW_TO_ACTION = {
    "continue":  {"intent": "resume"},
    "restart":   {"intent": "rerun_last_action"},
    "options":   {"intent": "open_settings"},
    "quit":      {"intent": "quit_to_world_map"},
}


def _now_utc_iso() -> str:
    return _dt.datetime.now(_dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def _now_unix_ms() -> int:
    return int(time.time() * 1000)


# --- sg-preflight command shims ---
#
# In a real sg-preflight tree these would call the existing
# `python -m sg_preflight desktop-state ...` commands directly. To
# keep this drop-in self-contained, the daemon shells out to
# `python -m sg_preflight ...` for state queries and surfaces the
# parsed JSON. If the project lays out its API differently, swap
# the body of these functions; the daemon's public contract
# (returns dicts, dispatches events) does not change.

def _sg_preflight_cmd() -> list[str]:
    return [sys.executable, "-m", "sg_preflight"]


def _summary_to_counts(summary: str) -> tuple[int, int, int]:
    """Parse 'X errors, Y warnings, Z info' from a sg-preflight summary string.
       Returns (errors, warnings, info)."""
    import re
    e = w = i = 0
    if (m := re.search(r"(\d+)\s*errors?", summary)):    e = int(m.group(1))
    if (m := re.search(r"(\d+)\s*warnings?", summary)):  w = int(m.group(1))
    if (m := re.search(r"(\d+)\s*info\b", summary)):     i = int(m.group(1))
    return e, w, i


def query_profiles() -> list[dict[str, Any]]:
    """Return profiles. sg-preflight desktop-state profiles emits a list of:
       {profile_id, label, summary, recommended_action_id}.
       Status enum is derived from summary error/warning counts.
    """
    try:
        out = subprocess.check_output(
            _sg_preflight_cmd() + ["desktop-state", "profiles"],
            stderr=subprocess.DEVNULL, timeout=15)
        data = json.loads(out)
        if not isinstance(data, list):
            return []
        result: list[dict[str, Any]] = []
        for p in data:
            summary = str(p.get("summary", ""))
            errors, warns, _ = _summary_to_counts(summary)
            status = "blocked" if errors > 0 else ("warning" if warns > 0 else "ready")
            result.append({
                "id":     str(p.get("profile_id", p.get("id", ""))),
                "label":  str(p.get("label", p.get("profile_id", ""))),
                "status": status,
            })
        return result
    except Exception:
        return []


def query_actions(profile_id: str | None = None) -> list[dict[str, Any]]:
    """Return actions for one profile. desktop-state actions REQUIRES a
       profile_id positional arg. If no active profile is known, returns []
       (the daemon will refresh once the user picks one).
    """
    if not profile_id:
        return []
    try:
        out = subprocess.check_output(
            _sg_preflight_cmd() + ["desktop-state", "actions", profile_id],
            stderr=subprocess.DEVNULL, timeout=15)
        data = json.loads(out)
        if not isinstance(data, list):
            return []
        result: list[dict[str, Any]] = []
        for a in data:
            result.append({
                "id":        str(a.get("action_id", a.get("id", a.get("name", "")))),
                "label":     str(a.get("label", a.get("name", a.get("action_id", "")))),
                "available": bool(a.get("available", True)),
            })
        return result
    except Exception:
        return []


def query_environment() -> dict[str, bool]:
    """Return collapsed env booleans. desktop-state environment emits a
       list of {key, category, label, state, summary, path, next_action}.
       Collapse to {python_ready, raco_ready, blender_ready}.
    """
    try:
        out = subprocess.check_output(
            _sg_preflight_cmd() + ["desktop-state", "environment"],
            stderr=subprocess.DEVNULL, timeout=15)
        data = json.loads(out)
        if not isinstance(data, list):
            return {"raco_ready": False, "blender_ready": False, "python_ready": True}
        by_key: dict[str, str] = {}
        for entry in data:
            key = str(entry.get("key", ""))
            state = str(entry.get("state", ""))
            if key:
                by_key[key] = state
        def any_ready(needle: str) -> bool:
            return any(state == "ready"
                       for k, state in by_key.items() if needle in k.lower())
        return {
            "python_ready":  by_key.get("python_backend") == "ready"
                             or by_key.get("sg_preflight_import") == "ready",
            "raco_ready":    any_ready("raco"),
            "blender_ready": any_ready("blender"),
        }
    except Exception:
        return {"raco_ready": False, "blender_ready": False, "python_ready": True}


def query_last_validation() -> dict[str, Any]:
    """Return last validation summary by reading desktop-state recent-runs
       and parsing the most recent entry's summary string.
    """
    try:
        out = subprocess.check_output(
            _sg_preflight_cmd() + ["desktop-state", "recent-runs"],
            stderr=subprocess.DEVNULL, timeout=15)
        data = json.loads(out)
        if not isinstance(data, list) or not data:
            return {"passed": True, "blockers": 0, "warnings": 0, "evidence_path": ""}
        latest = data[0]
        summary = str(latest.get("summary", ""))
        errors, warns, _ = _summary_to_counts(summary)
        return {
            "passed":        errors == 0,
            "blockers":      errors,
            "warnings":      warns,
            "evidence_path": str(latest.get("html_report", "")),
        }
    except Exception:
        return {"passed": False, "blockers": 0, "warnings": 0, "evidence_path": ""}


def launch_action(action_id: str, profile_id: str | None = None) -> bool:
    """Fire the existing `sg_preflight launch-action` command."""
    cmd = _sg_preflight_cmd() + ["launch-action", "--action", action_id]
    if profile_id:
        cmd += ["--profile", profile_id]
    try:
        subprocess.Popen(cmd)
        return True
    except Exception as exc:  # noqa: BLE001
        sys.stderr.write(f"[sgfx-bridge] launch-action failed: {exc}\n")
        return False


# --- State authoring ---

def build_state(active_bundle_id: str | None = None) -> dict[str, Any]:
    profiles = query_profiles()
    # If no active bundle yet but we have at least one profile, default
    # to the first so the C++ host has something to render.
    if not active_bundle_id and profiles:
        active_bundle_id = profiles[0]["id"]
    actions  = query_actions(profile_id=active_bundle_id)
    last     = query_last_validation()
    env      = query_environment()
    active = None
    if active_bundle_id:
        for p in profiles:
            if p["id"] == active_bundle_id:
                active = {
                    "id": p["id"],
                    "name": p["label"],
                    "path": p.get("path", ""),
                }
                break
    return {
        "schema":     STATE_SCHEMA,
        "version":    STATE_VERSION,
        "updated_at": _now_utc_iso(),
        "active_bundle": active or {},
        "profiles": profiles,
        "actions":  actions,
        "last_validation": last,
        "environment":     env,
    }


def write_state_atomic(bridge_dir: Path, state: dict[str, Any]) -> None:
    """Write state.json via tmp + rename so readers never see a half-written file."""
    tmp = bridge_dir / "state.json.tmp"
    final = bridge_dir / "state.json"
    bridge_dir.mkdir(parents=True, exist_ok=True)
    with tmp.open("w", encoding="utf-8") as f:
        json.dump(state, f, indent=2)
    os.replace(tmp, final)


# --- Event dispatch ---

def _select_action_for_event(ev: dict[str, Any]) -> dict[str, Any] | None:
    """Translate one bridge event into a sg-preflight intent dict."""
    kind   = ev.get("event")
    screen = ev.get("screen", "")
    row    = ev.get("row_id", "")
    profile_id = ev.get("profile_id", "")
    action_id  = ev.get("action_id", "")

    if kind == "menu_accepted":
        if screen == "Title" and row in TITLE_ROW_TO_ACTION:
            return {**TITLE_ROW_TO_ACTION[row], "source": "title_menu", "row": row}
        if screen == "Pause" and row in PAUSE_ROW_TO_ACTION:
            return {**PAUSE_ROW_TO_ACTION[row], "source": "pause_menu", "row": row}
    if kind == "profile_selected" and profile_id:
        # SU's "stage" on the world map maps to a sg-preflight stage_id
        # which happens to be a continent name. Look up the action.
        if profile_id in WORLDMAP_STAGE_TO_ACTION:
            return {**WORLDMAP_STAGE_TO_ACTION[profile_id], "source": "world_map", "stage": profile_id}
        # Otherwise treat profile_id as a real bundle ID -> activate it.
        return {"intent": "activate_bundle", "bundle_id": profile_id, "source": "world_map"}
    if kind == "action_requested" and action_id:
        return {"intent": "run_action", "action_id": action_id, "source": "stage_hud"}
    if kind == "results_acknowledged":
        return {"intent": "return_to_world_map", "source": "results"}
    if kind == "quit_requested":
        return {"intent": "quit", "source": "title"}
    return None


def dispatch(intent: dict[str, Any], state: dict[str, Any]) -> tuple[dict[str, Any], bool]:
    """Apply an intent to the local state. Returns (new_state, changed)."""
    op = intent.get("intent")
    changed = False
    if op == "run_action":
        ok = launch_action(intent["action_id"],
                          state.get("active_bundle", {}).get("id"))
        if ok:
            changed = True
            state["last_validation"] = {**state.get("last_validation", {}),
                                        "passed": False, "blockers": -1, "warnings": -1,
                                        "evidence_path": "(running)"}
    elif op == "activate_bundle":
        state["active_bundle"] = {
            "id": intent["bundle_id"],
            "name": intent.get("name", intent["bundle_id"]),
            "path": intent.get("path", ""),
        }
        changed = True
    elif op == "resume_last_bundle":
        # Pick the first profile if no active bundle yet.
        if not state.get("active_bundle", {}).get("id") and state.get("profiles"):
            p = state["profiles"][0]
            state["active_bundle"] = {"id": p["id"], "name": p["label"], "path": ""}
            changed = True
    elif op == "quit":
        # Daemon doesn't terminate UnleashedRecomp; that's the host's
        # responsibility. We just record a status note.
        changed = True
    return state, changed


# --- Tail reader ---

def tail_events(events_path: Path, last_offset: int) -> tuple[list[dict[str, Any]], int]:
    """Read events.jsonl from last_offset to EOF; return parsed events + new offset."""
    out: list[dict[str, Any]] = []
    if not events_path.exists():
        return out, last_offset
    size = events_path.stat().st_size
    if size < last_offset:
        # File was truncated or rotated; re-read from start.
        last_offset = 0
    if size == last_offset:
        return out, last_offset
    with events_path.open("r", encoding="utf-8", errors="replace") as f:
        f.seek(last_offset)
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                ev = json.loads(line)
                if ev.get("schema") == EVENT_SCHEMA:
                    out.append(ev)
            except json.JSONDecodeError:
                continue
        return out, f.tell()


# --- Main loop ---

def run(bridge_dir: Path, tick_seconds: float = 0.25, log: bool = True) -> int:
    bridge_dir.mkdir(parents=True, exist_ok=True)
    events_path = bridge_dir / "events.jsonl"
    state = build_state()
    write_state_atomic(bridge_dir, state)
    if log:
        sys.stderr.write(f"[sgfx-bridge] daemon up. dir={bridge_dir}\n")
    last_offset = 0
    last_state_refresh = time.time()
    try:
        while True:
            evs, last_offset = tail_events(events_path, last_offset)
            any_changed = False
            for ev in evs:
                if log:
                    sys.stderr.write(f"[sgfx-bridge] event: {ev}\n")
                intent = _select_action_for_event(ev)
                if intent is None:
                    continue
                state, ch = dispatch(intent, state)
                any_changed = any_changed or ch

            # Periodic state refresh (re-query sg_preflight) every ~5s
            # so external changes (validation completion, env doctor
            # results) get picked up even without a UI event.
            if time.time() - last_state_refresh > 5.0:
                fresh = build_state(active_bundle_id=
                    state.get("active_bundle", {}).get("id"))
                # Preserve the active_bundle across refreshes.
                if state.get("active_bundle"):
                    fresh["active_bundle"] = state["active_bundle"]
                state = fresh
                any_changed = True
                last_state_refresh = time.time()

            if any_changed:
                state["updated_at"] = _now_utc_iso()
                write_state_atomic(bridge_dir, state)

            time.sleep(tick_seconds)
    except KeyboardInterrupt:
        if log:
            sys.stderr.write("[sgfx-bridge] daemon stopped.\n")
        return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="sg_preflight bridge-daemon")
    parser.add_argument("--bridge-dir", type=Path, default=_default_bridge_dir(),
                        help="Bridge directory (state.json + events.jsonl live here)")
    parser.add_argument("--tick-seconds", type=float, default=0.25)
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)
    return run(args.bridge_dir, args.tick_seconds, log=not args.quiet)


if __name__ == "__main__":
    sys.exit(main())
