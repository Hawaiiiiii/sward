#!/usr/bin/env python3
"""Export a viewer status file from an SG preflight report.

The preflight already writes a JSON report per run. This maps that report (and an
optional daily-snapshot for the screenshot battery) to the compact status file the
desktop viewer reads (see ../../specs/data_bridge.md), then the viewer shows the
real run instead of its built-in placeholder values.

Pure standard library: it consumes the tool's output and writes a status file. It
does not run the tool, import it, or write anything back into it.

  python export_status.py --report g65-report.json --out sgfx_status.json
  python export_status.py --report g65-report.json --snapshot daily.json --out sgfx_status.json
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def _pack_rows(report: dict) -> list[dict]:
    rows = []
    for p in report.get("packs", []):
        s = p.get("summary", {})
        rows.append({
            "name": str(p.get("pack", "")),
            "err":  int(s.get("errors", 0)),
            "warn": int(s.get("warnings", 0)),
            "info": int(s.get("info", 0)),
        })
    return rows


def _verdict(errors: int) -> str:
    return "NEEDS REVIEW" if errors > 0 else "LIKELY OK"


def _recommendation(report: dict, errors: int, warnings: int) -> str:
    if errors:
        worst = max(
            report.get("packs", []),
            key=lambda p: p.get("summary", {}).get("errors", 0),
            default=None,
        )
        where = ""
        if worst and worst.get("summary", {}).get("errors", 0):
            where = f" (mostly {worst.get('pack')})"
        plural = "s" if errors != 1 else ""
        return f"Resolve the {errors} error{plural}{where}, then re-run before delivery."
    if warnings:
        plural = "s" if warnings != 1 else ""
        return f"No blocking errors; review the {warnings} warning{plural} before delivery."
    return "No findings. Ready to deliver."


def _battery_rows(snapshot: dict) -> list[dict]:
    rows = []
    for b in snapshot.get("battery_results", []):
        if not isinstance(b, dict):
            continue
        name = b.get("filter_name") or b.get("name")
        verdict = b.get("verdict")
        if not name or not verdict:
            continue
        rows.append({
            "name": str(name),
            "verdict": str(verdict),
            "diff": int(b.get("diff_count", 0) or 0),
        })
    return rows


def build_status(report: dict, snapshot: dict | None = None) -> dict:
    summary = report.get("summary", {})
    errors = int(summary.get("errors", 0))
    warnings = int(summary.get("warnings", 0))
    info = int(summary.get("info", 0))
    total = int(summary.get("total", errors + warnings + info))
    profile = str(report.get("context", {}).get("car_model", "")) or str(report.get("bundle", ""))

    signals = [
        {"label": "Errors",   "value": str(errors)},
        {"label": "Warnings", "value": str(warnings)},
        {"label": "Info",     "value": str(info)},
    ]
    run = {
        "activeProfile": profile,
        "packs": _pack_rows(report),
        "verdict": _verdict(errors),
        "recommendation": _recommendation(report, errors, warnings),
    }
    if snapshot is not None:
        battery = _battery_rows(snapshot)
        if battery:
            run["battery"] = battery
            review = sum(1 for b in battery if b["verdict"] != "likely_ok" and "missing" not in b["verdict"])
            missing = sum(1 for b in battery if "missing" in b["verdict"])
            signals.append({"label": "Screenshot diffs", "value": str(review)})
            signals.append({"label": "Review items",     "value": str(review + missing)})
    signals.append({"label": "Total findings", "value": str(total)})
    run["signals"] = signals
    return {"run": run}


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Export a viewer status file from an SG preflight report.")
    ap.add_argument("--report", required=True, help="the preflight report JSON the tool writes")
    ap.add_argument("--snapshot", help="optional daily-snapshot JSON for the screenshot battery")
    ap.add_argument("--out", default="sgfx_status.json", help="where to write the viewer status file")
    args = ap.parse_args(argv)

    report = json.loads(Path(args.report).read_text(encoding="utf-8"))
    snapshot = json.loads(Path(args.snapshot).read_text(encoding="utf-8")) if args.snapshot else None
    status = build_status(report, snapshot)
    Path(args.out).write_text(json.dumps(status, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    r = status["run"]
    print(f"wrote {args.out}: profile {r['activeProfile']}, {len(r['packs'])} packs, "
          f"verdict {r['verdict']}, {len(r.get('battery', []))} battery rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
