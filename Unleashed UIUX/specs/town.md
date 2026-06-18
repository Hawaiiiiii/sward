# Operator action hub — layout & interaction spec

Internal id: `town`. A two-panel hub: a scrolling list of operator actions on the
left, a live detail of the focused action on the right. This is the template for
any "list of actions + detail pane" menu. Built from primitives + text only — no
chrome art. Coordinates are in the **1280 × 720** canvas, origin top-left, in
pixels. Reference render: `screenshots/town.png`.

## Purpose
The operator picks an action to run; the right pane explains the focused action.
The actions run against the active profile shown top-right.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Logo slot | (40, 40), ≈168×56 | Host-supplied; empty = clean field, no header text. |
| Profile chip | top-right, ends x≈1130 | `PROFILE` label (dim green) over the active profile id (gold). |
| Header rule | y≈118 | Divider under the header. |
| Actions panel | (150, 158, 600, 404) | Dark panel; caption strip "ACTIONS" (height ≈52); action rows fill the rest. |
| Info panel | (780, 158, 350, 404) | Same styling, caption "INFO"; a small accent bar, the focused action name, and a two-line description. |
| Footer | y≈628 | `Up/Down` Move, `Enter` Run, `Esc` Back — plain text hints. |

## Action rows
Row height ≈60, **5 rows visible** (the list scrolls; a scrollbar appears on the
right when there are more). Each row is the action label, left-aligned. The focused
row gets an eased blue highlight bar spanning the panel width; its label brightens.

Default action set (label → detail):

| Action | Detail (2 lines) |
|---|---|
| Run preflight | full SG-side checks: anchors, constants, carpaints |
| Capture screenshots | export via the BMW pipeline; snap the test views |
| Check delivery | readiness across the car models and their changelogs |
| Daily digest | run every live profile; summarise the morning state |
| Scan unused Lua | find Lua files that survived into the project root |
| Manual review | open the queue of items awaiting a human verdict |

## Interaction
- **Up / Down** — move the cursor between rows with an eased highlight; the list
  scrolls when the cursor passes the visible window; the info pane updates.
- **Enter (Run / accept)** — run the focused action against the active profile (a
  transient "Running <action> on <profile>" confirmation here; wires to the real
  work in-flow).
- **Esc (Back)** — leave the hub.

## Colours
- Panels: dark navy plate; caption text in gold; blue focus highlight.
- Labels: light text, brighter when lit; descriptions dimmer.

## Notes for reimplementation
- The info pane mirrors the focused row — keep them sourced from the same action.
- Actions are the tool's real verbs; the active profile and any per-action counts
  are content (representative until a live feed supplies them).
- The logo is a content slot; an empty slot renders nothing (no placeholder text).
