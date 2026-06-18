#!/usr/bin/env python3
"""Unit tests for the status exporter's mapping. Pure stdlib; run with:

    python -m unittest test_export_status        (from this tools/ dir)
"""
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import export_status as ex


def _report(packs: list[tuple[str, int, int, int]], profile: str = "G65") -> dict:
    return {
        "bundle": "bundle",
        "context": {"car_model": profile},
        "summary": {
            "errors":   sum(p[1] for p in packs),
            "warnings": sum(p[2] for p in packs),
            "info":     sum(p[3] for p in packs),
            "total":    sum(p[1] + p[2] + p[3] for p in packs),
        },
        "packs": [
            {"pack": p[0], "summary": {"errors": p[1], "warnings": p[2], "info": p[3], "total": p[1] + p[2] + p[3]}}
            for p in packs
        ],
    }


class BuildStatus(unittest.TestCase):
    def test_packs_and_profile(self):
        run = ex.build_status(_report([("anchors", 0, 2, 5), ("constants", 3, 4, 6)]))["run"]
        self.assertEqual(run["activeProfile"], "G65")
        self.assertEqual(run["packs"], [
            {"name": "anchors",   "err": 0, "warn": 2, "info": 5},
            {"name": "constants", "err": 3, "warn": 4, "info": 6},
        ])

    def test_verdict_needs_review_names_worst_pack(self):
        run = ex.build_status(_report([("anchors", 0, 0, 0), ("constants", 3, 0, 0)]))["run"]
        self.assertEqual(run["verdict"], "NEEDS REVIEW")
        self.assertIn("Resolve the 3 errors", run["recommendation"])
        self.assertIn("constants", run["recommendation"])

    def test_verdict_likely_ok_when_no_errors(self):
        run = ex.build_status(_report([("anchors", 0, 1, 2)]))["run"]
        self.assertEqual(run["verdict"], "LIKELY OK")

    def test_signals_last_row_is_total(self):
        sig = ex.build_status(_report([("constants", 3, 2, 1)]))["run"]["signals"]
        self.assertEqual(sig[-1], {"label": "Total findings", "value": "6"})

    def test_battery_from_snapshot(self):
        snap = {"battery_results": [
            {"filter_name": "default",        "verdict": "likely_ok",           "diff_count": 0},
            {"filter_name": "lights_HighBeam", "verdict": "needs_manual_review", "diff_count": 12},
        ]}
        run = ex.build_status(_report([("anchors", 0, 0, 0)]), snap)["run"]
        self.assertEqual(run["battery"], [
            {"name": "default",        "verdict": "likely_ok",           "diff": 0},
            {"name": "lights_HighBeam", "verdict": "needs_manual_review", "diff": 12},
        ])


class ScanReports(unittest.TestCase):
    def test_scan_directory(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "run-profile-G65").mkdir()
            (root / "run-profile-G70").mkdir()
            (root / "run-profile-G65" / "g65-report.json").write_text(
                json.dumps(_report([("constants", 9, 2, 0)], "G65")), encoding="utf-8")
            (root / "run-profile-G70" / "g70-report.json").write_text(
                json.dumps(_report([("anchors", 0, 0, 3)], "G70")), encoding="utf-8")

            profiles, hub = ex.scan_reports(root)
            self.assertEqual({p["id"]: p["verdict"] for p in profiles},
                             {"G65": "needs review", "G70": "likely ok"})
            h = {x["label"]: x["value"] for x in hub}
            self.assertEqual(h["PROFILES"], "2")
            self.assertEqual(h["PASSING"], "1 / 2")
            self.assertEqual(h["OPEN FINDINGS"], "11")
            self.assertEqual(h["IN REVIEW"], "1")


if __name__ == "__main__":
    unittest.main()
