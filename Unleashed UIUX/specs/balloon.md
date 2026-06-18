# Notification toast — layout & interaction spec

Internal id: `balloon`. A notification toast: a small rounded dark panel lower-
centre with a gold title and a one/two-line message, shown after a long-running
action finishes. Built from primitives + text only — no chrome art. Coordinates
are in the **1280 × 720** canvas, origin top-left, in pixels. Reference render:
`screenshots/balloon.png`.

## Purpose
Tell the operator a background action finished and give the headline result in a
line or two, with a prompt for the obvious next step.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Logo slot | (40, 40), ≈168×56 | Host-supplied; empty = clean field, no header text. |
| Profile chip | top-right, ends x≈1130 | `PROFILE` (dim green) over the active profile id (gold). |
| Toast panel | (360, 452, 560, 144) | Lower-centre dark panel; top-left + bottom-right chamfered corners (rounded read), a thin cool border, a gold title, a thin gold rule, two message lines. |
| Footer | y≈640, from x≈360 | `Enter` Select, `Esc` Back — plain text hints over a thin rule at y≈624. |

## Toast content
A short **gold title** line, a thin gold rule beneath it, then **two message
lines** — the first in light text, the second dimmer. Representative content
(title → line 1 → line 2):

| Title | Line 1 | Line 2 |
|---|---|---|
| Preflight complete | G65 - 3 errors, 12 warnings. | Open the verdict? |
| Screenshots captured | G65 - 4 of 4 test views snapped. | Ready for review. |
| Delivery checked | G65 - changelog and Ramses size | look in range. |

## Interaction
- **Enter (Select)** — cycles through the sample toasts (stands in for "act on
  this notification").
- **Esc (Back)** — backs out (handled by the host's flow wrapper).

## Animation
The panel scales open from near-zero about its own centre (an inflate-pop), then
the title, rule, and message lines fade in once it has reached full size.

## Colours
- Background: dark navy vertical gradient.
- Profile chip: `PROFILE` label dim green, profile id gold.
- Panel: dark navy gradient plate; thin cool border; chamfer corners cut back to
  the backdrop tone.
- Text: title gold, first message line light, second line dimmer.

## Notes for reimplementation
- The titles and messages are representative QA notifications — content until a
  live feed supplies the real result; the profile id is likewise content.
- The two message lines are independent strings; either may be short.
- The logo is a content slot; an empty slot renders nothing (no placeholder text).
