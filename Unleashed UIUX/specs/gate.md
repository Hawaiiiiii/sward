# Stage-Select screen — layout & interaction spec

Internal id: `gate`. One of the in-game selection screens reconstructed for the
Grafiks operator UI. This document is the hand-off spec: it describes the layout,
measurements, and interaction so the screen can be rebuilt cleanly with our own
content. All coordinates are in the fixed **1280 × 720** design canvas, origin
top-left, in pixels. Reference render: `screenshots/gate.png`.

## Purpose
A single-item selection screen: the operator picks one entry (a "stage"), reviews
its summary stats and a preview image, and confirms. It is the template for any
"choose one of N, see its details, commit" flow.

## Regions (top to bottom)

| Region | Rect (x0,y0–x1,y1) | Notes |
|---|---|---|
| Title wordmark | ~120,10 – 300,70 | Two-line italic chrome wordmark ("STAGE / SELECT"), sheared ~0.26, x-stretched ~1.35, light face with a dark 1px halo, over a left-anchored gold-to-amber sweep. A chevron arrow sits just right of it (~280,30 – 330,55). |
| Item-name plate | ~140,95 – 290,120 | Dark-grey chamfered plate (top-left 45° chamfer, slanted right end), white italic item name. |
| Detail panel | 268,176 – 1003,540 | One translucent grey panel (top ≈ rgba 112,116,122,205 grading darker downward). Holds the stat rows, medal icons, preview slot, and rank emblem. Pops in with a scale from ~0.83 to 1.0 about the panel centre. |
| Carousel arrows | left ~218–240, right ~987–1009, centred y≈385 | Left/right triangles to move between items. |
| Footer | y≈340 (below panel) | Left: `LB` Switch `RB`. Right: `(A)` Select, `(B)` Back. Button glyphs, not text keys. |

## Detail-panel contents

**Stat labels** (left side, upright outlined chrome text, x≈120–260):
`HIGH SCORE`, `BEST TIME`, `RINGS`, `MEDALS`.

**Stat values** (right side, italic chrome digits, right-edge column x≈663):

| Row | Value format | Baseline centre y | Companion icon |
|---|---|---|---|
| High score | integer (e.g. `159998`), gold | ≈344 | — |
| Best time | `MM:SS:CC` (e.g. `01:59:79`) | ≈394 | — |
| First medal count | `X / Y` single line | ≈428 | red "sun" medal disc at x≈415 |
| Second medal count | `X / Y` single line | ≈478 | blue "moon" medal disc at x≈415 |

Key layout rule for the medal rows: each count is **one clean `X / Y` line** seated
on the same baseline as its medal disc — numerator, single slash, denominator, all
on one line, right-aligned to the shared value column. Do not split numerator and
denominator onto separate baselines, and never render a slash with no numbers beside
it. (The earlier reconstruction did both; this is the corrected behaviour.)

**Preview slot**: right portion of the panel (~720,150 – 990,300) — a still image of
the selected item. Empty/host-supplied by design.

**Rank emblem**: large metallic letter (e.g. `S`) at ~900,400 – 1044,534, right of
the panel. Host/content-supplied art.

**Medal icons** (CSD/content art, x≈415): a gold ring (~y405), a red sun disc
(~y428), a blue moon disc (~y478). The two medal counts pair with the sun and moon
discs respectively.

## Colours (sampled)
- Title/banner sweep: cream `#D2D676` → amber `#D1973F`.
- Chrome wordmark face `#F6F8FC` with a near-black halo.
- Item-name plate `#60646C`.
- Panel grey `#70747A` at ~88% opacity, grading ~12 levels darker top-to-bottom.
- Stat-value digits: light steel chrome gradient with the same dark halo as the title.

## Interaction
- **LB / RB** — switch to the previous / next item. The live scene behind the panel
  stays bright; the panel re-pops its stats for the new item.
- **A (Select)** — opens a small confirm popup ("Play Stage / Cancel") rendered as a
  green scanline panel with a gradient highlight row. The cursor **defaults to
  Cancel**. Choosing the affirmative option commits and advances (loads the item);
  Cancel dismisses the popup.
- **B (Back)** — returns to the previous screen (the hub).

## Animation
- Panel scale-pop on entry (~0.83 → 1.0) about the panel centre; stat values fade/seat
  in with the panel.
- The confirm popup appears with a scene dim behind it (that dim is its transition —
  there is no separate slide-in).

## Notes for reimplementation
- The value column is right-aligned; measure each string's rendered width including the
  italic shear and halo so the right edges line up (the shear makes glyphs overshoot a
  plain width measurement by ~25–30px at this size).
- Medal counts must always read as `X / Y` on one baseline beside their disc. If a
  count is absent, draw nothing — never a bare separator.
- Preview image, rank emblem, and medal discs are content slots: position them per the
  rects above and fill with our own art.
