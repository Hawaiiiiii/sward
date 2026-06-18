# Quick menu — layout & interaction spec

Internal id: `pause`. A quick-menu overlay: a dark scrim over whatever scene is
behind, plus a small centred menu panel with a short list of entries. Built from
primitives + text only — no chrome art. Coordinates are in the **1280 × 720**
canvas, origin top-left, in pixels. Reference render: `screenshots/pause.png`.

## Purpose
Give the operator a fast way to resume, jump to a related screen, or leave —
without losing the scene behind, which stays visible (dimmed) under the scrim.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Scrim | full canvas | ~55% dark fill over the scene behind (the host draws the scene; the overlay dims it). |
| Menu panel | (430, 168, 420, 384) | Centred dark panel: caption strip "Menu" (height ≈52), a thin 1px cool accent border, then the entry rows. Height follows the entry count. |
| Footer | below the panel, y≈574 | Centred hint: `Up/Down` Move, `Enter` Select, `Esc` Resume. |

(Panel x/y/h are derived: the panel is centred on the canvas and its height is the
caption strip plus five rows of ≈56 plus padding.)

## Menu entries
Row height ≈56, **5 entries**, left-aligned labels. The focused row gets an eased
blue highlight bar; its label brightens to white. Selection wraps top-to-bottom.

Entry set (label → where it routes):

| Entry | Routes to |
|---|---|
| Resume | back to the previous screen |
| Metrics | the run-metrics screen |
| Settings | the settings screen |
| Back to hub | the QA hub |
| Quit | back to the previous screen |

## Interaction
- **Up / Down** — move the cursor between entries (wraps).
- **Enter (Select)** — accept the focused entry; the screen records its route and
  the host performs the wipe + screen switch.
- **Esc (Resume / cancel)** — resume, i.e. route back to the previous screen.

The screen keeps its own navigation: it records the chosen route on accept, and
the host polls-and-clears it to drive the transition.

## Colours
- Scrim: flat dark, partial alpha (scene reads through, dimmed).
- Panel: dark navy plate; caption text in gold; thin cool accent border; blue
  focus highlight.
- Labels: light text, brighter when lit.

## Notes for reimplementation
- Entries are the tool's real menu verbs; each carries its own route, which the
  host consumes after accept (Resume and Quit both pop back).
- The scene behind is the host's; this overlay only dims it and draws the panel.
