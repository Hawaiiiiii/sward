# QA hub — layout & interaction spec

Internal id: `world_map`. The hub: a rotating planet with one node per QA area, a
left panel of global hub totals, and a right panel detailing the focused area.
Built from primitives + text only — no chrome art. Coordinates are in the
**1280 × 720** canvas, origin top-left, in pixels. Reference render:
`screenshots/world_map.png`.

## Purpose
A single overview of the work. The planet carries a node per QA area; the operator
moves between areas and the right panel explains the focused one. The left panel is
a constant at-a-glance summary of the whole workspace.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Background | full screen | Dark gradient + a faint starfield. |
| Logo slot | (40, 44), ≈168×56 | Host-supplied; empty = clean field, no header text. |
| Planet | centred (612, 384), r≈176 | A 3D sphere, slow idle spin; one node per area. |
| Hub totals | (40, 132, 260, 224) | LED panel: global counts (label + value rows, dashed underlines). |
| Area detail | (856, 120, 326, 480) | LED panel: focused area name + `n / total`, description, three metric rows. |
| Footer | y≈662 | `‹ ›` Select area, `Enter` Open; a transient "Opening <area>" confirmation. |

## Areas (planet nodes)
Each area is a node on the planet (green; the focused node lights **gold** and grows)
and a detail page on the right. The default set:

| Area | Detail summary | Metrics |
|---|---|---|
| PREFLIGHT | the deterministic SG-side checks | PACKS / ERRORS / WARNINGS |
| DELIVERY | delivered vs. open changelog work | MODELS / DELIVERED / PENDING |
| SCREENSHOTS | captures vs. approved baselines | PAIRS / REVIEW / DIFFS |
| DAILY DIGEST | the morning live-profile snapshot | PROFILES / BLOCKED / FLAGGED |
| MANUAL REVIEW | items awaiting a human verdict | QUEUE / P0 / P1 |
| PROFILES | the car slices to work | TOTAL / IDCEVO / CLASSIC |

## Hub totals (left panel)
A constant four-row summary of the workspace: PROFILES, DELIVERED (`n / total`),
OPEN FINDINGS, IN REVIEW. Label on top, value below, dashed rule between rows.

## Interaction
- **Left / Right** — move the focused area; the matching planet node lights gold and
  the right panel + index update immediately.
- **A (Open / accept)** — open the focused area (a transient "Opening <area>"
  confirmation here; wires to the area's screen in-flow).

## Colours
- Panels: dark LED plate with a lit-cell grid border and a green inner frame.
- Headers/values: gold area names, lime metric values, dim-green labels.
- Nodes: green idle, gold focused.

## Notes for reimplementation
- The planet is the shared hub motif; keep its spin slow and lighting back-left.
- Totals and per-area metrics are **content** — authentic in shape and vocabulary
  (real profiles, packs, verdict terms) but representative until a host process
  supplies them (e.g. a status file, the way the layouts feed in).
- The logo is a content slot; an empty slot renders nothing (no placeholder text).
