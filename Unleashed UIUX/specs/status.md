# Run metrics — layout & interaction spec

Internal id: `status`. The result of a preflight pass over a profile: an overall
verdict and totals up top, then a table with one row per check pack and its error /
warning / info counts. Built from primitives + text only — no chrome art.
Coordinates are in the **1280 × 720** canvas, origin top-left, in pixels. Reference
render: `screenshots/status.png`.

## Purpose
Show how a run came out. The summary gives the verdict at a glance; the table breaks
it down by check pack so the operator can see where the findings are.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Logo slot | (40, 40), ≈168×56 | Host-supplied; empty = clean field, no header text. |
| Profile chip | top-right, ends x≈1130 | `PROFILE` over the run's profile id (gold). |
| Summary | (150, 134, 980, 96) | Verdict (left, colour-coded, with an accent bar) + ERRORS / WARNINGS / INFO totals (right). |
| Pack table | (150, 256, 980, 300) | Header row (CHECK PACK / ERRORS / WARN / INFO) + one row per pack; the focused row gets a blue highlight. |
| Footer | y≈628 | `Up/Down` Select pack, `Esc` Back. |

## Verdict
Derived from the totals: any errors → **NEEDS REVIEW** (red); else any warnings →
**WARNINGS** (amber); else **LIKELY OK** (green). The accent bar and the verdict
text take the matching colour.

## Pack table
One row per check pack — the real set: `anchors`, `constants`, `carpaints`,
`project_sanity`. Each row shows the pack name and three right-aligned counts.
Counts are colour-coded — errors red, warnings amber, info green — and **a zero is
dimmed** so the eye lands on the non-zero cells.

## Interaction
- **Up / Down** — move the focused pack (blue highlight).
- **B / Esc (Back)** — leave to the previous screen.

## Colours
- Panels: dark navy plates; blue focus highlight.
- Counts: errors red, warnings amber, info green, zeros dimmed grey.

## Notes for reimplementation
- Pack names are **real**; the counts and verdict are content — representative until
  a live run feeds them in (a `Report` with one `PackResult` per pack).
- The logo is a content slot; an empty slot renders nothing (no placeholder text).
