# World-Map / hub screen — layout & interaction spec

Internal id: `world_map`. The top-level navigation hub: a rotating 3D globe with
selectable regions, a player-totals readout, and the currently-focused region's
flag and name. This is the template for a "spatial hub you move around to pick a
destination" flow. Coordinates are in the **1280 × 720** canvas, origin top-left,
in pixels. Reference render: `screenshots/world_map.png`.

## Purpose
A hub map: the operator rotates/scans a globe, moves a cursor between region
markers, sees the focused region's identity (flag + name) and their running totals
(lives, rings, day/night medal levels), then commits to enter a region.

## Regions

| Element | Position | Notes |
|---|---|---|
| Title wordmark | x≈124, baseline band y≈62–104 | Gold bevelled "WORLD MAP", x-stretched ~1.34, bright-gold face (`#FFFF0F`-ish) with a dark halo, on the dark header background (above the green panel band). |
| Header band | full width, y≈104–107 | Thin accent rail separating the title strip from the body. |
| Totals column | x≈124–360, rows at y≈135, 178, 221, 264 (pitch 43) | Four rows: a small icon/medal + a short label + a live value. Compact — label and value must both clear the region-flag block to the right. |
| Region flag | x≈395–490, y≈125–165 | The focused region's flag (content art). |
| Region name | x≈520–1000, y≈135–165 | Large white name of the focused region (e.g. a place name). |
| 3D globe | centred ~x640, y≈360, radius ~190 | A real tessellated, slowly-spinning sphere (~6°/s) carrying region markers; a floating region label with a leader rule points to the active marker. |
| Footer | y≈690 | `(◷)` Pass Time, `(A)` Select, `(B)` Back — button glyphs. |

## Totals column rows
Each row: icon (x≈124–156) → label (x≈178, compact) → value (just right of the
label). The four rows:

| Row | Icon | Label | Value format |
|---|---|---|---|
| 1 | blue marker | LIVES | small integer (e.g. `5`) |
| 2 | gold ring | RINGS | integer (e.g. `1230`) |
| 3 | sun medal | SUN | `lv N (NNN)` — day medal level |
| 4 | moon medal | MOON | `lv N (NNN)` — night medal level |

Layout rule: keep the column compact. The LIVES and RINGS rows share the vertical
band with the region flag/name to their right, so their values must end **left of
the flag** (x≈390). Use a label + value that fit in that space; do not let the
value run under the flag or into the region name. (The earlier reconstruction drew
max-width placeholder values at a large size and they collided with the flag — this
is the corrected behaviour.)

## The title 'P' note
The "WORLD MAP" wordmark renders through a bitmap title font. If a glyph's atlas
cell is bad it will paint as a solid block; render the affected glyph from
primitives instead (a 'P' = left stem + top bar + right bar + a mid bar closing the
bowl, leaving the counter open) in the matching title gold. Always render the whole
wordmark and check each glyph — a single bad cell reads as "broken".

## Interaction
- **Directional input** — move the cursor between region markers on the globe; the
  globe orients toward the focused marker; the flag, region name, and floating label
  update to the focused region. A populated info state vs an empty-bracket hover
  state both exist (the empty state shows just the bracket frame, no populated panel).
- **Pass Time** — toggles the world between day and night (this drives the SUN/MOON
  medal context and the lighting).
- **A (Select)** — commit to the focused region (opens its sub-screen / confirms a
  "go here" popup).
- **B (Back)** — leave the hub.

## Animation
- Globe spins continuously (~6°/s idle).
- Staggered entrance: background, header, globe and totals settle first; the focused
  region's info floods in next; the gold title and the floating region label settle
  last (~1.5 s).

## Notes for reimplementation
- The globe is real 3D (a tessellated sphere drawn through the 2D quad path, no depth
  buffer); markers ride its surface and the front hemisphere is lit.
- Flag, region name, medal icons and the globe texture are content slots — fill with
  our own art/place data.
- Keep the totals column narrow; values are right-bounded so they never cross into the
  flag/name block regardless of magnitude.
