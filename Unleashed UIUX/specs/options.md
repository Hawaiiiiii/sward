# Settings — layout & interaction spec

Internal id: `options`. A two-panel settings screen: a scrolling, selectable list
of operator settings on the left (label + current value or toggle), and a help
pane on the right that tracks the focused setting. Built from primitives + text
only — no chrome art. Coordinates are in the **1280 × 720** canvas, origin
top-left, in pixels. Reference render: `screenshots/options.png`.

## Purpose
Let the operator review and change the tool's settings. The list shows each
setting and its current value; the right pane explains the focused setting and
surfaces any contextual warning that the chosen value warrants.

## Regions
| Element | Rect (x,y,w,h) | Notes |
|---|---|---|
| Logo slot | (40, 40), ≈168×56 | Host-supplied; empty = clean field, no header text. |
| Header rule | y≈118 | Divider under the header, from x≈150 to x≈1130. |
| Settings panel | (150, 158, 600, 404) | Dark panel; caption strip "SETTINGS" (height ≈52); setting rows fill the rest. |
| Help panel | (780, 158, 350, 404) | Same styling, caption "ABOUT"; a small accent bar, the focused setting name, and a wrapped help line. |
| Footer | y≈628 | `Up/Down` Select, `Left/Right` Change, `Esc` Back — plain text hints. |

## Setting rows
Row height ≈56, **6 rows visible** (the list scrolls; a scrollbar appears on the
right when there are more). Each row is the setting label (left) and its current
value (right-aligned). The focused row gets an eased blue highlight bar; its label
brightens. A focused multi-choice value is wrapped in `< … >` to show it cycles
with the arrows. Toggle / choice values persist immediately when changed; path
values are shown read-only here.

Two value styles: a chosen value reads green when "on", muted when "Off"; a path
value reads as plain light text.

Settings shown (the tool's real settings):

| Setting | Value | Help (right pane) |
|---|---|---|
| Theme | Dark | Dark IDE styling. There is no light mode. |
| AI assist | Off / On | Heuristics run by default; AI is opt-in and off unless you turn it on. |
| Default profile | G65 / G70 / G45 | The car profile new runs target unless you pick another. |
| Open report after run | On / Off | Open the run report automatically once a preflight finishes. |
| Desktop notifications | On / Off | Notify you when a long run or capture completes. |
| Confirm before delivery | On / Off | Ask for confirmation before a delivery check goes ahead. |
| Verbose logging | Off / On | Write extra detail to the session log for troubleshooting. |
| Check for updates | On / Off | Look for a newer build of the tool on launch. |
| Source repo | (path) | Where the working copy of the car project lives. |
| BMW Git | (path) | The car-models repository the tool reads from. |

The first listed value of each toggle / choice is the **default**. **Theme has
only one choice — there is no light mode.** **AI assist defaults to Off**; when it
is turned On the help pane adds a contextual sub-line in amber noting that it
**sends data to the cloud**.

## Help pane
Tracks the focused setting. A short accent bar, then the setting name, then the
help line word-wrapped to the panel width. When AI assist is the focused setting
and it is On, the cloud-data sub-line follows the help line in amber.

## Interaction
- **Up / Down** — move the focused setting with an eased highlight; the list
  scrolls when the cursor passes the visible window; the help pane updates.
- **Left / Right (or accept)** — cycle the focused setting's value (multi-choice
  settings only); the change persists immediately. Path values do not cycle.
- **Esc (Back)** — leave settings.

## Colours
- Panels: dark navy plate; caption text in gold; blue focus highlight.
- Values: green when on, muted grey when Off; path values plain light text.
- The AI-assist cloud-data sub-line is amber.

## Notes for reimplementation
- The help pane mirrors the focused row — keep them sourced from the same setting.
- The settings list and its help lines are **real**; the current values are
  content — representative until persisted state / a live feed supplies them.
- AI assist defaults Off; only surface the cloud-data sub-line when it is On.
  There is no light theme.
- The logo is a content slot; an empty slot renders nothing (no placeholder text).
