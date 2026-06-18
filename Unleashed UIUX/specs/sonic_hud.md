# Run HUD — layout & interaction spec

Internal id: `sonic_hud`. A live-run HUD overlay shown WHILE a check runs over a
profile: a profile chip, the running action, a "pack N / total" line, and a thin
progress bar. Built from primitives + text only — no chrome art. Unlike the menu
screens there is no full-screen panel; the HUD floats over whatever is behind it,
so the backdrop stays a faint scrim only. Coordinates are in the **1280 × 720**
canvas, origin top-left, in pixels. Reference render: `screenshots/sonic_hud.png`.

## Purpose
Show, at a glance, that a check is running and how far it has got: which profile,
which action, how many check packs are done, and the percentage complete.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Scrim | full canvas | Faint dark overlay so the HUD reads over the scene behind; no solid fill. |
| Profile chip | (92, 64, 132, 64) | Small rounded dark chip: `PROFILE` label over the active profile id (gold), thin cool border. |
| Running action | from x≈240, beside the chip | The action wordmark (light), with a small `running...` / `paused` state under it that pulses while live. |
| Progress block | bar at (360, 360, 560, 14) | A "N / total packs" line and the action name above the bar; a dim track with a cool-blue fill; a percent readout right-aligned below. |
| Footer | y≈628, from x≈360 | `Up/Down` Step, `Esc` Pause — plain text hints over a thin rule at y≈612. |

## Progress block
Above the track: a left-aligned **"N / total packs"** line, with the action name
right-aligned on the same row. The track is a dim bar with thin edge lines; the
**fill** is a cool-blue vertical gradient that eases when the value changes, with
a soft leading edge so it reads as in motion. Below the track, a right-aligned
**percent** readout. Representative state: profile G65, action PREFLIGHT, 3 of 4
packs done (75%).

## Interaction
- **Up / Down (Step)** — step the completed-pack count up/down; the bar follows
  with an eased transition.
- **Left / Right** — nudge the fill directly by a small step (for inspection).
- **Enter (accept)** — toggle the live "running" pulse (running ↔ paused).
- **Esc (Pause / cancel)** — would pause the run in-flow (handled by the host's
  flow wrapper).

## Colours
- Scrim: flat dark, low alpha.
- Chip / state: chip is a dark gradient plate with a cool border; `PROFILE` label
  dim, profile id gold; the `running...` state pulses in a light cool tone.
- Track: dim plate with thin cool edge lines; fill is a cool-blue gradient with a
  soft additive leading edge.
- Text: action wordmark and pack line light; labels and percent dimmer.

## Notes for reimplementation
- The profile, action, pack total and progress are content — representative until
  a live run feeds them in. The percent is derived from done / total.
- This is an overlay: there is no opaque background, only a faint scrim — the
  scene behind stays visible.
