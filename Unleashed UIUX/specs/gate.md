# Profile select — layout & interaction spec

Internal id: `gate`. A carousel over the work profiles: the focused profile id sits
large between arrows, a detail panel shows its label, family, focus and last
verdict. Built from primitives + text only — no chrome art. Coordinates are in the
**1280 × 720** canvas, origin top-left, in pixels. Reference render:
`screenshots/gate.png`.

## Purpose
Pick the car slice to work. The carousel moves through the profiles; the panel
describes the focused one; accept opens a confirm.

## Regions
| Element | Rect (x,y) | Notes |
|---|---|---|
| Background | full screen | Dark gradient + faint starfield. |
| Logo slot | (40, 40), ≈168×56 | Host-supplied; empty = clean field, no header text. |
| Index | top-right, ends x≈1130 | `n / total` (dim green). |
| Profile id | centred, baseline y≈168 | Large gold id (e.g. `G65`), x-stretched, with a soft shadow. |
| Carousel arrows | flank the id at y≈164 | Outward-pointing green triangles (◀ id ▶); hidden while the confirm is open. |
| Family chip | centred under the id, y≈198 | `IDCevo` / `Classic` in blue. |
| Detail panel | (340, 268, 600, 252) | LED panel: label (header + rule), two focus lines, `LAST VERDICT` + value. |
| Footer | y≈628 | `‹ ›` Switch profile, `Enter` Open, `Esc` Back. |
| Confirm | centred (520, 286, 240, 146) | "Open <id>" / "Cancel" on a dark dialog with a blue highlight row + screen dim. |

## Profiles
The carousel runs over the real profile set — the canonical `G70` / `G65` / `G45`
plus the wider IDCevo and classic families (19 in all). Each shows: id, label
(e.g. "BMW G65 live slice"), family, a two-line focus, and a last verdict
(`likely ok` / `needs review` / `blocked` / `not run`).

## Interaction
- **Left / Right (or LB / RB)** — move the focused profile; the id, family, panel
  and index update; the matching arrow nudges.
- **A (Open / accept)** — open the confirm ("Open <id>" / "Cancel").
- **B (Back)** — leave to the hub.
- In the confirm: **Up / Down** toggle the option, **A** confirms, **B** cancels.

## Colours
- Panel: dark LED plate with a lit-cell grid border and green inner frame.
- Id gold; family chip blue; focus light; verdict lime; arrows green.

## Notes for reimplementation
- Profile id, label, family and focus are **real**; the verdict (and any per-profile
  counts) are content — representative until a live feed supplies them.
- The logo is a content slot; an empty slot renders nothing (no placeholder text).
- Arrows point outward (left = previous, right = next) — never inward.
