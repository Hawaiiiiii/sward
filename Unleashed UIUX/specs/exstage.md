# Extra-Stage (EX trial) select — layout & interaction spec

Internal id: `exstage`. A two-panel detail screen for picking one entry from a small
set of timed challenges: a preview/identity panel on the left and a stats/records
panel on the right, with left/right paging between entries. Template for a
"browse-and-start one of N timed challenges" flow. Coordinates are in the
**1280 × 720** canvas, origin top-left, in pixels. Reference render:
`screenshots/exstage.png`.

## Purpose
The operator pages through challenge entries; the left panel shows the entry's
banner, sub-name and objective; the right panel shows its record (high score, two
gauges, rank) and a start prompt.

## Regions

| Element | Position | Notes |
|---|---|---|
| Title | "EXTRA STAGE" at x≈60, y≈40 | Gold title text (~46px) with a shadow + ease-in slide. |
| Entry counter | x≈920–1220, y≈48 | `EX N / M` right-aligned (current / total). |
| Title rule | y≈106, x≈60–1220 | Horizontal divider. |
| Left panel "EX STAGE" | preview plate, height ≈296 | Caption strip "EX STAGE" atop a dark plate. Holds the banner wordmark, sub-name, objective and a medal count. |
| Right panel "EX TRIAL" | stats plate | Caption "EX TRIAL". Holds the sub-name, HI-SCORE, two gauges, rank letter and the start prompt. |
| Footer | y≈648 | `(A)` Start, `(B)` Back, `(LB/RB)` Switch Trial — button glyphs. |

## Left panel (preview)
- **Banner wordmark** — a large italic wordmark centred on the plate (e.g. "CHANCE
  ATTACK"), aspect-fit into a box (plate width minus padding × ~120 tall).
- **Sub-name** — e.g. "EX TRIAL 01", below the banner.
- **Medal count** — a silver medal/ring badge + "× N", lower on the plate.
- **Objective** — a one-line description under the panel (e.g. "Chain hits past the
  combo gate before the timer ends.").

## Right panel (record)
- **Sub-name** (e.g. "EX TRIAL 01") at the top.
- **HI-SCORE** — large value in the record digit font (e.g. `248 600`).
- **SHIELD** and **ENERGY** — two labelled gauges with a bar + percentage.
- **RANK** — a single letter (S/A/B/…); to its right a "PRESS ● TO START" prompt.

## Banner-atlas note
The banner wordmark is one row sampled from a stacked multi-language wordmark atlas;
the rows sit only ~38px apart. Sample the chosen row's vertical band **tightly** — a
band that reaches past its row pulls in the wordmark beneath it, and the surplus
clips against the plate as a garbled strip. Bound the band to the row's own opaque
extent and verify the result shows a single clean line.

## Interaction
- **Left / Right (or LB / RB)** — page to the previous / next entry; panels re-enter
  with a staggered ease (title → preview → record → footer).
- **A (Start)** — begin the focused challenge.
- **B (Back)** — leave the screen.

## Colours
- Title / captions: gold; values: bright cyan-white in the record font.
- Plates: dark navy with thin rule frames; rank letter in gold.

## Notes for reimplementation
- Banner wordmarks, the medal badge and gauge values are content slots.
- Keep the entry counter (`EX N / M`) in sync with the paged entry.
- Gauges are simple 0–100 bars; rank is an enum (S..E, or "no record").
