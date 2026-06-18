# Launcher menu — layout & interaction spec

Internal id: `title`. The main launcher: a centred logo, a vertical list of
entries, and a readout strip that echoes the focused entry. Built from primitives
and text only — no chrome art. Coordinates are in the **1280 × 720** canvas,
origin top-left, in pixels. Reference render: `screenshots/title.png`.

## Purpose
The product's home menu. The operator picks where to go; the readout explains the
focused entry. The look is dark and neutral so it sits behind any branding.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Background | full screen | Dark vertical gradient (navy → near-black). |
| Logo slot | centred at (640, 132), ≈420×140 | Host-supplied logo; **empty = clean field, no header text**. |
| Menu list | column at x=360, top y=246, w=560 | Five rows, pitch 74, row height 64. |
| Readout strip | x=150, y=628, w=980, h=54 | Dark backing with a green top rule; shows the focused entry and its blurb. |

## Menu rows
Each row is a dark plate with a thin top rule; the focused row is a blue vertical
gradient with brighter label text. Rows slide in from the right on entrance,
staggered. The selection highlight eases between rows on a move.

Entry order (label → blurb shown in the readout):

| Label | Blurb |
|---|---|
| RUN | Start a new preflight run. |
| RESUME | Pick up the last session. |
| SETTINGS | Adjust paths and preferences. |
| EVIDENCE | Review screenshots and reports. |
| QUIT | Close the tool. |

## Readout strip
Left: `> <LABEL>` in green. Right: the focused entry's one-line blurb in a dimmer
green, right-aligned. It updates immediately as the cursor moves.

## Interaction
- **Up / Down** — move the cursor (wraps top↔bottom) with an eased highlight; the
  readout updates to the focused entry.
- **A (accept)** — confirm the focused entry (a brief white flash on the row).
- **B (cancel)** — back out (no-op in the standalone build).

## Colours
- Background: dark navy → near-black gradient.
- Idle row: dark plate; focused row: blue gradient; labels light, brighter when lit.
- Readout: green text on a dark backing.

## Notes for reimplementation
- The logo is a **content slot**; an empty slot renders nothing (no placeholder,
  no header text) — the host drops its own logo in.
- No chrome art, wordmark or third-party background is drawn — the screen is plain
  primitives + text and ships on its own.
- The entry list is the launcher's actions; keep the readout sourced from the same
  entry as the highlight so they always agree.
