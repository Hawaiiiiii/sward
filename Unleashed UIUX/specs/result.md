# QA verdict — layout & interaction spec

Internal id: `result`. The outcome card of a run: a colour-coded verdict emblem, a
breakdown of the signals behind it, and a one-line recommendation. Built from
primitives + text only — no chrome art. Coordinates are in the **1280 × 720**
canvas, origin top-left, in pixels. Reference render: `screenshots/result.png`.

## Purpose
The headline answer for a run: what's the verdict, what drove it, and what to do
next. Sits after the per-pack metrics as the "so what" of a pass.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Logo slot | (40, 40), ≈168×56 | Host-supplied; empty = clean field, no header text. |
| Profile chip | top-right, ends x≈1130 | `PROFILE` over the run's profile id (gold). |
| Verdict emblem | (150, 150, 410, 320) | Panel with a colour bar, a `VERDICT` label, and the verdict word boxed in the verdict colour. |
| Signals | (600, 150, 530, 320) | Panel captioned `SIGNALS`; label + right-aligned value rows; the last row (`Total findings`) is ruled off and gold. |
| Recommendation | (150, 496, 980, 70) | Panel with a verdict-colour accent bar, a `RECOMMENDATION` label, and one line of guidance. |
| Footer | y≈628 | `Enter` Continue, `Esc` Back. |

## Verdict
One of **LIKELY OK** (green), **NEEDS REVIEW** (amber), **BLOCKED** (red). The colour
bar, the boxed verdict word and the recommendation accent all take the verdict colour.

## Signals
The evidence behind the verdict, as label/value rows: Errors, Warnings, Screenshot
diffs, Review items, and a ruled-off **Total findings** total. These are the
roll-ups the verdict is computed from.

## Interaction
- **A (Continue / accept)** — move on (to the hub in-flow).
- **B (Back)** — back out.

## Colours
- Panels: dark navy plates; captions gold.
- Verdict + accents: green / amber / red by verdict; values light, total gold.

## Notes for reimplementation
- The verdict and signal counts are content — representative until a live run
  supplies them (a battery/`Report` verdict + its roll-up counts).
- The recommendation is one short, human line tied to the top signal.
- The logo is a content slot; an empty slot renders nothing (no placeholder text).
