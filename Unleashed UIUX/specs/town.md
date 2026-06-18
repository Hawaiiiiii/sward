# Town hub menu — layout & interaction spec

Internal id: `town`. A two-panel hub menu: a scrolling list of context actions on
the left and a live description of the focused action on the right. This is the
template for any "list of actions + detail pane" menu. Coordinates are in the
**1280 × 720** canvas, origin top-left, in pixels. Reference render:
`screenshots/town.png`.

## Purpose
The operator stands in a hub and picks an action from a list; the right pane
explains the focused action; some actions change global state (here, time of day).

## Regions

| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Title | "TOWN" at x≈150, y≈50 | Plain title text. A `lv NN` chip sits centre-top; a `DAY`/`NIGHT` label + a rotating sun/moon medallion sit top-right. |
| Title rule | y≈118 | Horizontal divider under the title. |
| Actions panel | x=150, y=158, w=600, h=404 | Dark rounded panel. A caption strip "ACTIONS" (height ≈52) tops it; the action rows fill the rest. |
| Info panel | x=780, y=158, w=350, h=404 | Same panel styling, caption "INFO". Shows the focused action's large icon + a two-line description. |
| Footer | y≈690 | `(↕)` Move, `(A)` Select, `(B)` Back — button glyphs. |

## Action rows
Row height ≈60, inner pad ≈14, **5 rows visible** (the list scrolls when there are
more). Each row: a square **icon cell** on the left, then the action label. The
focused row gets an eased blue highlight bar spanning the panel width.

Default action set (icon → label):

| Icon | Label | Detail (2 lines) |
|---|---|---|
| sun (→ moon at night) | Pass Time | wait for sun to rise/set; switch day↔night |
| camera | Take Photo | snap a picture; save to album |
| filled disc | Talk / Use | speak with townsfolk; gather hints |
| ring | Visit Shop | browse goods; spend rings |
| crescent moon | Records | review stage records; ranks/times/medals |
| ring | Depart | leave town for the world map |

## Icon-atlas note
Action icons come from one small (128×128) icon atlas; each icon is a sub-rect
**aspect-fit** into the square row cell. Measure each sub-rect tightly around its
glyph — a sub-rect that overruns its cell pulls in a neighbouring glyph (e.g. a
camera + a no-entry sign, or a moon overlapping the camera) and the row reads as a
fragment or the wrong icon. Verify every row's icon against the atlas, not just one.

## Interaction
- **Up / Down** — move the cursor between rows with an eased highlight; the list
  scrolls when the cursor passes the visible window; the info pane updates to the
  focused action.
- **A (Select)** — perform the focused action. **Pass Time** toggles the day/night
  cycle: the header medallion swaps sun↔moon and the `DAY`/`NIGHT` label changes.
  The other actions open their respective sub-flows (shop, records, etc.).
- **B (Back)** — close the menu (back to free-roam).

## Colours
- Panels: dark navy plate with a subtle lit grid; caption text in gold.
- Focus highlight: blue gradient bar.
- Labels: white with a dark outline.

## Notes for reimplementation
- Icons, medallion art and the level chip are content slots.
- The info pane mirrors the focused row — keep its icon and the row icon sourced
  from the same atlas entry so they always agree.
- Day/night is global state owned by this screen; other screens read it.
