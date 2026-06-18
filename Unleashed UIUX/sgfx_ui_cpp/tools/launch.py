#!/usr/bin/env python3
"""Export an SG preflight run to the viewer's status file and launch the viewer.

  python launch.py <run-dir> [--profile G65] [--exe <sgfx_screens.exe>] [--no-launch]

<run-dir> is an SG preflight output run folder (it contains
logs/run-profile-<ID>/<id>-report.json). This picks the active profile's report
(or the first one found), runs the exporter to write the viewer's status file next
to the exe, and starts the viewer. A convenience over running export_status.py and
the exe by hand; it does not run, import, or modify the tool.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import export_status as ex


def find_reports(run_dir: Path) -> tuple[Path, list[Path]]:
    base = run_dir / "logs"
    if not base.is_dir():
        base = run_dir
    return base, sorted(base.rglob("*-report.json"))


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Export an SG run and launch the viewer.")
    ap.add_argument("run_dir", help="an SG preflight output run folder (with logs/run-profile-*)")
    ap.add_argument("--profile", help="the active profile id (default: the first report found)")
    ap.add_argument("--exe", default="sgfx_screens.exe", help="the viewer exe (status is written beside it)")
    ap.add_argument("--no-launch", action="store_true", help="export only; do not start the viewer")
    args = ap.parse_args(argv)

    run_dir = Path(args.run_dir)
    base, reports = find_reports(run_dir)
    if not reports:
        ap.error(f"no *-report.json under {run_dir}")

    active = None
    if args.profile:
        active = next((r for r in reports if args.profile.lower() in r.name.lower()), None)
    active = active or reports[0]

    exe = Path(args.exe)
    out = exe.parent / "sgfx_status.json"
    rc = ex.main(["--report", str(active), "--reports-dir", str(base), "--out", str(out)])
    if rc != 0:
        return rc
    print(f"status -> {out}  (active report: {active.name})")

    if args.no_launch:
        return 0
    if not exe.exists():
        print(f"(viewer not found at {exe}; status written, skipping launch)")
        return 0
    print(f"launching {exe}")
    subprocess.Popen([str(exe)], cwd=str(exe.parent))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
