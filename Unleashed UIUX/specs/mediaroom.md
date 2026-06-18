# Screenshot review queue — layout & interaction spec

Internal id: `mediaroom`. A side-by-side diff review of screenshot evidence: a
left panel that scrolls through screenshot pairs (filename + colour-coded
classification tag + changed-pixel %), and a right panel that shows the focused
pair in detail — two BASELINE / CANDIDATE image slots, the metrics, and a
colour-coded verdict. Built from primitives + text only — no chrome art.
Coordinates are in the **1280 × 720** canvas, origin top-left, in pixels.
Reference render: `screenshots/mediaroom.png`.

## Purpose
Let the operator step through the captured screenshot pairs and judge each one.
The list gives the classification and change size at a glance; the detail pane
opens the focused pair so the operator can compare baseline against candidate and
read the verdict.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Logo slot | (40, 40), ≈168×56 | Host-supplied; empty = clean field, no header text. |
| Profile chip | top-right, ends x≈1130 | `PROFILE` (dim green) over the captured profile id (gold). |
| Header rule | y≈118 | Divider under the header, from x≈150 to x≈1130. |
| List panel | (150, 158, 470, 404) | Dark panel; caption strip "SCREENSHOTS" (height ≈52); pair rows fill the rest. |
| Detail panel | (650, 158, 480, 404) | Same styling, caption "DIFF"; focused filename, two image slots, metrics, verdict. |
| Footer | y≈628 | `Up/Down` Select, `Enter` Open diff, `Esc` Back — plain text hints. |

## Pair rows
Row height ≈60, **5 rows visible** (the list scrolls; a scrollbar appears on the
right when there are more). Each row is two lines: the screenshot filename on top
(left-aligned), then on the lower line a colour-coded classification tag pill
(left) and the changed-pixel % (right). The focused row gets an eased blue
highlight bar spanning the panel width; its filename brightens.

The changed-pixel figure is colour-coded on its own: a missing side shows `n/a`
in red; ≥2% in amber; any smaller non-zero in green; an exact zero is dimmed.

### Classifications
Six real classes, each with a short tag and a colour:

| Classification | Tag | Colour |
|---|---|---|
| needs_review | REVIEW | amber |
| dimension_mismatch | DIM | red |
| missing_candidate | NO CAND | red |
| missing_baseline | NO BASE | red |
| near_identical | ~SAME | green |
| unchanged | SAME | dimmed |

## Detail pane
Tracks the focused pair. Top to bottom: the focused filename; two side-by-side
image slots labelled **BASELINE** and **CANDIDATE** (≈118 tall, the panel width
split in two with an 18px gutter); a metrics block; then the verdict line.

The image slots are **content slots** — host-supplied imagery drops in like the
logo. An empty slot renders as a bordered rect with nothing inside (no
placeholder text). When the focused pair's class is `missing_baseline` the
baseline slot is forced empty; `missing_candidate` forces the candidate slot
empty.

Metrics, one per line (label left, value right): `CLASSIFICATION` (in its class
colour), `CHANGED PIXELS` (same colour rule as the row), `BASELINE` dimensions,
`CANDIDATE` dimensions. A missing side shows `n/a` / `—`.

The verdict line sits at the bottom on a dark plate with a left accent bar in the
class colour, derived from the class: `NEEDS REVIEW`, `MISMATCH`,
`CANDIDATE MISSING`, `BASELINE MISSING`, `LIKELY OK`, `UNCHANGED`.

## Interaction
- **Up / Down** — move the focused pair with an eased highlight; the list scrolls
  when the cursor passes the visible window; the detail pane updates.
- **Enter (Open diff)** — open the focused pair's diff (wires to the real view
  in-flow).
- **Esc (Back)** — leave the queue.

## Colours
- Panels: dark navy plate; caption text in gold; blue focus highlight.
- Image slots: darker interior with a light-blue border.
- Classification / verdict: amber needs-review, red mismatch / missing, green
  near-identical, dimmed unchanged.

## Notes for reimplementation
- The detail pane mirrors the focused row — keep them sourced from the same pair.
- The six classifications and the battery filenames are **real**; the diff
  percentages, dimensions, and which pairs appear are content — representative
  until a live feed supplies the run.
- The BASELINE / CANDIDATE image slots are content slots; an empty slot renders a
  bordered rect with nothing inside (no placeholder text).
- The logo is a content slot; an empty slot renders nothing (no placeholder text).
