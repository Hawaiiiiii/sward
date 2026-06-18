# Cold-boot splash — timeline & layout spec

Internal id: `boot_logos`. The first screen on a cold start: a short, self-running
title sequence that ends by handing off to the boot title screen. Coordinates are
in the **1280 × 720** canvas, origin top-left, in pixels. Reference render:
`screenshots/boot_logos.png`.

## Purpose
A brief, brand-neutral opening: a soft flare, a centred logo slot for the host's
own mark, a slowly rotating planet, then a loading beat. It carries no third-party
wordmarks, so it ships on its own.

## Timeline
| Window (s) | Content |
|---|---|
| 0.0 – 1.0 | Soft flare open over a dark navy field. |
| 1.0 – 3.5 | Centred **logo slot** (host-supplied). With no asset the slot stays empty — a clean field, no placeholder text. |
| 7.0 – 9.5 | The **rotating planet** (a 3D sphere, ~46°/s, lit from back-left) centred on screen — the same hub motif used elsewhere. |
| 9.75 – 12.6 | Black field with a neutral **"Loading"** line and a dot-ring spinner, lower-right; then it advances to the title screen. |

## Regions
| Element | Rect (x,y) | Notes |
|---|---|---|
| Logo slot | centred at (640, 360), fit ≈420 wide | Content slot; aspect from the supplied art. |
| Planet | centred at (640, 360), r≈96 | 3D pass; gentle idle spin. |
| Loading line | right edge ≈1000, baseline ≈596 | Plain text, muted green, gentle pulse. |
| Spinner | ≈(1023, 593), 3×3 dot ring | Lit head sweeps one cell per ~0.1 s. |

## Interaction
- **A / Start** — skip the splash; jump straight to the title screen.
- Otherwise the timeline runs once and auto-advances when it ends.

## Colours
- Stage: dark navy vertical gradient (logo beats); black (loading beat).
- Loading text + spinner: muted-to-bright green, pulsing.

## Notes for reimplementation
- The logo is a **content slot** — an empty slot renders nothing (no placeholder,
  no debug text); the host drops its own logo in.
- The planet is the shared hub motif; keep its spin slow and its lighting back-left
  so the near face sits in shadow.
- Nothing here depends on third-party art; the loading beat is drawn, not a sprite.
