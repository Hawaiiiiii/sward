# Setup — layout & interaction spec

Internal id: `installer`. A single dark panel titled "Setup" that lists the
dependency / onboarding steps the tool needs before a run, each with its own
colour-coded state word. Built from primitives + text only — no chrome art.
Coordinates are in the **1280 × 720** canvas, origin top-left, in pixels.
Reference render: `screenshots/installer.png`.

## Purpose
Show the operator what the tool still needs before it can run: each step is a
dependency or a piece of configuration, and the state word on the right says
whether it is in place, needs installing, or is not set yet.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Logo slot | (40, 40), ≈168×56 | Host-supplied; empty = clean field, no header text. |
| Header rule | y≈118, from x≈280 to x≈1000 | Thin divider under the header. |
| Setup panel | (280, 158, 720, 404) | Dark panel; caption strip "Setup" (height ≈52); step rows fill the rest. |
| Footer | y≈628, from x≈280 | `Enter` Continue, `Esc` Back — plain text hints over a thin rule at y≈612. |

## Step rows
Row height ≈78, **4 rows**, stacked from just under the caption strip. Each row
carries, left to right: the step name (top line), a one-line detail beneath it,
and a right-aligned **state word**. The focused row gets an eased blue highlight
bar spanning the panel width; its name brightens to white.

Default step set (name → detail → state):

| Step | Detail | State |
|---|---|---|
| RaConverter | Asset converter, required for export. | found |
| RaCoHeadless | Headless export runner for screenshots. | install |
| Blender | Used by the geometry checks. | found |
| Digital-3D-Car repo | The car-models working copy the tool reads. | set |

The state word is one of **found / install / set / not set** and is colour-coded:
found and set in green (in place), install in amber (action needed), not set in
dim grey (nothing yet).

## Interaction
- **Up / Down** — move the focused step (blue highlight); clamps at the ends of
  the list (does not wrap).
- **Enter (Continue)** / **Esc (Back)** — host-driven; the screen surfaces the
  hint, the host advances or backs out.

## Colours
- Background: dark navy vertical gradient.
- Panel: dark navy plate; caption text in gold; blue focus highlight.
- Names: light text, brighter when lit; details dimmer.
- State words: found/set green, install amber, not set dim grey.

## Notes for reimplementation
- Step names and details are the tool's real onboarding steps; each state word is
  content — representative until a live probe supplies the real result.
- States map to colour, not the other way round: anything "present" reads green,
  "needs work" reads amber, "absent" reads dim.
- The logo is a content slot; an empty slot renders nothing (no placeholder text).
