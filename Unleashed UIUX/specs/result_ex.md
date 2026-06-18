# Screenshot battery results — layout & interaction spec

Internal id: `result_ex`. The per-filter breakdown of a screenshot test run over a
profile: an overall battery verdict and totals up top, then a table with one row
per battery filter and its verdict + baseline diff count. Built from primitives +
text only — no chrome art. Coordinates are in the **1280 × 720** canvas, origin
top-left, in pixels. Reference render: `screenshots/result_ex.png`.

## Purpose
Show how a screenshot battery came out. The summary gives the overall verdict at a
glance; the table breaks it down by filter so the operator can see which views
need a look.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Logo slot | (40, 40), ≈168×56 | Host-supplied; empty = clean field, no header text. |
| Profile chip | top-right, ends x≈1130 | `PROFILE` (dim green) over the run's profile id (gold). |
| Summary | (150, 134, 980, 96) | Overall verdict (left, colour-coded, with a left accent bar) + REVIEWED / NEED REVIEW / NO BASELINE totals (right). |
| Filter table | (150, 240, 980, 364) | Header row (BATTERY FILTER / VERDICT / DIFF) + one row per filter; the focused row gets a blue highlight. |
| Footer | y≈628 | `Up/Down` Select, `Esc` Back — plain text hints. |

## Verdict
Derived from the totals: any baseline-missing → **BASELINE MISSING** (red); else
any review pending → **NEEDS REVIEW** (amber); else **LIKELY OK** (green). The
accent bar and the verdict text take the matching colour.

The summary totals, right of the verdict: **REVIEWED** (green), **NEED REVIEW**
(amber), **NO BASELINE** (red) — each a count over its label; a zero count is
dimmed.

## Filter table
One row per battery filter — the real set: `default`, `lights_drl_front`,
`lights_LowBeam`, `lights_HighBeam`, `lights_OnlyCones`, `openAllDoors_`,
`automatic_Doors_`, `welcome_animation_`, `highlighting_Doors`. Row height ≈35.
Each row shows the filter name (left), a colour-coded verdict label (centre), and
a right-aligned diff count; **a zero diff is dimmed** so the eye lands on the
counts that moved. The focused row gets a blue highlight.

### Verdicts
Five real verdicts in three colour buckets — green = clean, amber = attention,
red = blocked:

| Verdict | Label | Colour |
|---|---|---|
| likely_ok | LIKELY OK | green |
| needs_manual_review | NEEDS REVIEW | amber |
| proxy_candidate_ready | PROXY READY | amber |
| baseline_candidate_ready | BASELINE READY | amber |
| baseline_missing | BASELINE MISSING | red |

## Interaction
- **Up / Down** — move the focused filter (blue highlight).
- **Esc (Back)** — leave to the previous screen.

## Colours
- Panels: dark navy plates; blue focus highlight.
- Verdicts: green likely-ok, amber needs-review / proxy / candidate, red
  baseline-missing.
- Totals + diff counts colour-coded; zeros dimmed grey.

## Notes for reimplementation
- Filter names and the five verdicts are **real**; the per-filter verdict, the
  diff counts, the totals, and the overall verdict are content — representative
  until a live run feeds them in (one filter result per row).
- The logo is a content slot; an empty slot renders nothing (no placeholder text).
