# Boot title — layout & interaction spec

Internal id: `boot_title`. The title screen reached after the cold-boot splash.
Two states: a **splash** (logo + a prompt to continue) and a **menu** (a
horizontal carousel of entries). Coordinates are in the **1280 × 720** canvas,
origin top-left, in pixels. Reference render: `screenshots/boot_title.png`.

## Purpose
Present the product's face and a small launch menu. The splash invites the
operator to continue; the menu lets them resume, start fresh, or open settings.

## Splash state
| Element | Rect (x,y) | Notes |
|---|---|---|
| Starfield | full screen | Black field with scattered faint stars. |
| Logo slot | centred, upper-middle | Host-supplied; empty = clean starfield, no placeholder text. |
| Prompt capsule | centred ≈(640, 521), ≈284×50 | A glowing gold rim capsule with a dark inner well and the caption **"ENTER"**, gently pulsing. |

## Menu state
| Element | Rect (x,y) | Notes |
|---|---|---|
| Planet | centred ≈(652, 371), r≈187 | A 3D sphere rising behind the logo, drifting slowly. |
| Letterbox bands | y 0–105 and y 616–720 | Olive scanline bands with pale-green edge lines. |
| Carousel | centred ≈(640, 501), selector ≈191×24 | One entry shown on a green scanline selector bar, flanked by green arrows and outboard fading wedge bars. |

Menu entries (carousel order): **RESUME**, **NEW RUN**, **SETTINGS**.

## Interaction
- **A (accept)** — on the splash, advance to the menu. On the menu, confirm the
  centred entry: RESUME / NEW RUN open the hub; SETTINGS opens the settings screen.
- **Left / Right** — cycle the carousel entry (menu state only).
- **B (cancel)** — return to the splash.

## Colours
- Splash: black starfield; gold prompt capsule; white caption.
- Menu: olive scanline bands, pale-green edges; lime entry text; green selector.

## Notes for reimplementation
- The logo is a **content slot**; an empty slot renders a clean starfield with no
  placeholder or debug text.
- The planet is the shared hub motif; reuse the same sphere as the splash sequence.
- No third-party wordmark, copyright line or console-storage notice is drawn — the
  screen reads as the host's own and ships without that art.
