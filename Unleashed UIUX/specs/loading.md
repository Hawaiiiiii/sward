# Loading & splash — layout & interaction spec

Internal ids: `loading`, `boot_loading`, `start`. The brand-neutral
loading / splash screens. All three are passive (no interaction). Built from
primitives + text only — no chrome art. Coordinates are in the **1280 × 720**
canvas, origin top-left, in pixels. Reference renders: `screenshots/loading.png`,
`screenshots/boot_loading.png`, `screenshots/start.png`.

## Purpose
Hold the field while something loads. The two loading screens show a small
"Loading" line plus a spinner; the start splash shows the host logo over a clean
field with a soft subtitle.

## loading / boot_loading
Near-identical. A dark field with a muted-green **"Loading"** line lower-right and
a drawn **dot-ring spinner** just right of it. No header wordmark.

| Element | Position | Notes |
|---|---|---|
| Field | full canvas | Dark navy vertical gradient. |
| "Loading" line | lower-right, baseline y≈596, ends x≈1011 | Muted green; breathes (brightness pulses) on a ~1.0s cycle. |
| Dot-ring spinner | right of the word, around (1023, 593) | An 8-cell ring of small squares; a lit head sweeps around, trailing cells fading from green to dark. |
| Progress hint (`loading` only) | thin track under the word, y≈628 | A 2px track with a slow sweeping fill; present on `loading`, absent on `boot_loading`. |

`boot_loading` is the first-boot variant: the same field, "Loading" line, and
spinner, but with no progress hint under the word.

## start
A clean dark field with a centred **host logo slot** and a soft **"Loading"**
subtitle below it. No wordmark, no countdown.

| Element | Position | Notes |
|---|---|---|
| Field | full canvas | Dark navy vertical gradient. |
| Logo slot | centred, ≈420 wide | Host-supplied logo; empty = clean field, nothing drawn. |
| Subtitle | centred, below the slot (y≈510) | Soft neutral "Loading"; gently breathes. |

## Interaction
None — all three are passive splashes; flow is driven by the host.

## Colours
- Field: dark navy vertical gradient throughout.
- "Loading" line / spinner: muted green, brightness pulsing.
- start subtitle: soft neutral grey-blue, gentle breathe.

## Notes for reimplementation
- The logo on `start` is a content slot; an empty slot renders nothing (no
  placeholder text).
- The spinner and pulses are time-driven decoration, not a real progress feed.
- `loading` and `boot_loading` differ only in the progress hint under the word.
