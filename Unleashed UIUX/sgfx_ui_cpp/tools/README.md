# Viewer status tools

Glue between the SG preflight (which already writes JSON reports per run) and the
desktop viewer (which reads a small status file). All pure standard library — they
**consume** the tool's output and write the viewer's status file; they do not run,
import, or modify the tool. Schema: `../../specs/data_bridge.md`.

- **`export_status.py`** — map a preflight report (and an optional daily-snapshot for
  the screenshot battery) to the viewer status file. Add `--reports-dir` to also scan
  a folder of per-profile reports for the hub overview (profile verdicts + totals).

      python export_status.py --report <run>/logs/run-profile-G65/g65-report.json \
                              --reports-dir <run>/logs --out sgfx_status.json

- **`launch.py`** — one command: point at an SG output run folder, it picks the active
  profile's report, writes the status file beside the viewer exe, and starts it.

      python launch.py <run-folder> --profile G65 --exe path/to/sgfx_screens.exe

- **`test_export_status.py`** — unit tests for the mapping.

      python -m unittest test_export_status

With no status file the viewer falls back to built-in representative defaults, so it
still runs standalone.
