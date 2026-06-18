# Hub help card — layout & interaction spec

Internal id: `world_map_help`. The QA-hub help overlay: a dim scrim over the live
hub, with a single centred dark panel titled "Help" listing the hub's controls in
neutral prose. Built from primitives + text only — no chrome art. Coordinates are
in the **1280 × 720** canvas, origin top-left, in pixels. Reference render:
`screenshots/world_map_help.png`.

## Purpose
Remind the operator how to drive the hub: which keys move between QA areas, open
the focused one, and back out — without leaving the hub, which stays visible
(dimmed) behind the card.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Scrim | full canvas | Dim dark fill over the live hub behind. |
| Help panel | centred, 560 wide | Dark panel: header cap (height ≈70) with a centred gold "Help" title and a thin rule beneath, then the hint rows, then a footer strip. Panel height follows the row count; vertically centred (nudged up ≈6). |
| Footer | inside the panel bottom | `Esc` Close, over a thin rule. |

## Hint rows
Row height ≈60, **3 rows**, each a left-column **control key** (warm tint) and a
right-column **action description** (light). Rows fade in staggered.

| Key | Description |
|---|---|
| Left / Right | Move between QA areas |
| Enter | Open the focused area |
| Esc | Back |

## Interaction
- **Enter or Esc** — closes the help overlay (cancel routes back via the host's
  flow wrapper). A short "Closing..." confirm flash plays in the standalone build.

## Colours
- Scrim: flat dark, partial alpha (hub reads through, dimmed).
- Panel: dark navy plate; darker header cap; gold title; thin cool rule.
- Rows: key in a warm tint, description in light text.
- Footer / confirm flash: dim footer hint; the transient "Closing..." note is
  green.

## Notes for reimplementation
- Keys and descriptions are the hub's real controls — fixed, not a live feed.
- This is an overlay over the live hub: the hub stays drawn behind the scrim.
- The "Closing..." flash is a standalone-build nicety; the host flow normally
  drives the actual close.
