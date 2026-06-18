# Live data bridge — contract

The viewer reads a small status file the tool writes, and the run screens draw from
it. With no file present, the viewer falls back to built-in representative defaults,
so it still runs standalone and looks identical to the baked-in values.

## How it loads
At startup the viewer reads the first of these that exists:
`sgfx_status.json` (next to the exe) → `data/sgfx_status.json` → the staged data path.
On absence or a parse error, the defaults stand. Any field may be omitted; the
default for that field is kept. (Engineering note: `sgfx_data.{h,cpp}`; a worked
example is `sgfx_ui_cpp/sgfx_status.example.json`.)

## Schema
```
{ "run": {
    "activeProfile": "G65",                         // shown on the profile chips
    "packs":   [ { "name": "anchors", "err": 0, "warn": 2, "info": 5 }, ... ],
    "verdict": "NEEDS REVIEW",                       // LIKELY OK | NEEDS REVIEW | BLOCKED
    "signals": [ { "label": "Errors", "value": "3" }, ... ],   // last row = total
    "recommendation": "...",
    "battery": [ { "name": "default", "verdict": "likely_ok", "diff": 0 }, ... ]
} }
```

| Field | Drives | Notes |
|---|---|---|
| `activeProfile` | the profile chip on every run screen | a profile id string (e.g. G70). |
| `packs[]` | the run-metrics table + its totals/verdict | the real check packs (anchors / constants / carpaints / project_sanity); the metrics verdict is derived from the counts (errors → NEEDS REVIEW, else warnings → WARNINGS, else LIKELY OK). |
| `verdict` | the verdict card's emblem colour | "OK" → green, "BLOCK" → red, otherwise amber. |
| `signals[]` | the verdict card's signal rows | label/value pairs; the last row renders as the ruled-off total. |
| `recommendation` | the verdict card's recommendation line | one short human line. |
| `battery[]` | the battery-results table | one row per screenshot filter. `verdict` is an id (`likely_ok`, `needs_manual_review`, `proxy_candidate_ready`, `baseline_candidate_ready`, `baseline_missing`); `diff` is the baseline diff count. Colours: likely_ok green, baseline_missing red, the rest amber. The overall battery verdict is derived (any missing baseline → BASELINE MISSING, else any review → NEEDS REVIEW, else LIKELY OK). |

## For the writer
The preflight already writes a JSON report per run. The dev-side exporter
`sgfx_ui_cpp/tools/export_status.py` maps that report (and an optional daily-snapshot
for the battery) straight to this status file — a pure consumer that does not import
or modify the tool:

    python tools/export_status.py --report <run>/g65-report.json --out sgfx_status.json

Write `sgfx_status.json` to the path the viewer reads (next to the exe); it picks it
up at next launch. The file is host/run-specific and is not committed; the example
here documents the shape.
