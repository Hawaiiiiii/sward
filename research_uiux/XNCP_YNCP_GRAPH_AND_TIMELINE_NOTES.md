# <img src="../docs/assets/branding/icon_debug.png" width="30" alt="SWARD debug icon"/> XNCP / YNCP Graph And Timeline Notes

Machine-readable inventory: `research_uiux/data/layout_deep_analysis.json`

## Method

This pass extends `research_uiux/tools/inspect_xncp_yncp.py` beyond scene/cast/animation names.

The parser now reads and summarizes:

- scene graph structure from cast hierarchies
- `AnimationFrameData` frame counts
- `AnimationKeyframeData` group/cast animation payloads
- animation-track flags mapped to Shuriken's channel model:
  - `HideFlag`
  - `XPosition`
  - `YPosition`
  - `Rotation`
  - `XScale`
  - `YScale`
  - `SubImage`
  - `Color`
  - `GradientTL`
  - `GradientBL`
  - `GradientTR`
  - `GradientBR`
- keyframe interpolation types:
  - `Const`
  - `Linear`
  - `Hermite`

The goal is not perfect DCC-style reconstruction. The goal is enough structural truth to reason about scene graphs, authored durations, and transition-channel families without relying on the GUI editor.

## Cross-File Complexity Ranking

Layout projects with the broadest authored motion surface:

- `ui_worldmap.yncp`
  - `35` scenes
  - `65` animations
  - `1173` cast dictionaries
  - `14875` subimages
- `ui_status.yncp`
  - `42` scenes
  - `109` animations
  - `227` cast dictionaries
  - `7770` subimages
- `ui_pause.yncp`
  - `29` scenes
  - `41` animations
  - `260` cast dictionaries
  - `2871` subimages
- `ui_loading.yncp`
  - `7` scenes
  - `37` animations
  - `331` cast dictionaries
  - `2240` subimages

Compact overlay projects:

- `ui_saveicon.yncp`
  - `1` scene
  - `1` animation
- `ui_end.yncp`
  - `1` scene
  - `2` animations
- `ui_boss_gauge.yncp`
  - `4` scenes
  - `10` animations

Practical reading:

- `world map` and `status` are the heaviest authored screen systems in the current extracted set
- `pause` is still substantial, but its complexity is more compartmentalized
- `save` and `ending` are small, self-contained overlays with long authored durations and low structural breadth

## Longest Authored Timelines

The deepest authored durations currently visible in the extracted layout layer:

1. `ui_end.yncp`
   - `Intro_Anim_1` and `Intro_Anim_2`
   - `420` frames
   - `7.0` seconds at `60 FPS`
2. `ui_loading.yncp`
   - `pda_txt/Usual_Anim_3`
   - `240` frames
   - `4.0` seconds
3. `ui_loading.yncp`
   - `pda_txt/Outro_Anim`
   - `240` frames
   - `4.0` seconds
4. `ui_pause.yncp`
   - `btn_effect/charge_3_Outro`
   - `240` frames
   - `4.0` seconds
5. `ui_worldmap.yncp`
   - `cts_cursor/Intro_Anim`
   - `221` frames
   - `3.683333` seconds
6. `ui_mainmenu.{xncp,yncp}`
   - `mm_donut_move/DefaultAnim`
   - `220` frames
   - `3.666667` seconds
7. `ui_boss_name.yncp`
   - numbered name reveal banks such as `01_Anim`
   - `180` frames
   - `3.0` seconds
8. `ui_saveicon.yncp`
   - `Intro_Anim`
   - `180` frames
   - `3.0` seconds

Takeaway:

- long authored timelines are not limited to cinematic overlays
- pause, loading, world map, and main menu all use multi-second authored motion banks for emphasis, staging, or sustained idle-state behavior

## Track-Type Patterns By Screen Family

## Main Menu

- dominant channels:
  - `Color`: `126`
  - `XPosition`: `102`
  - `YPosition`: `102`
  - `XScale`: `93`
  - `YScale`: `83`
  - `Rotation`: `37`
- very little `SubImage` work: `5`
- no gradient channels in the current main-menu project

Reading:

- motion is mostly transform-driven plus alpha, matching the mechanical layer-stack feel seen in the earlier semantic pass
- sprite swapping exists, but it is not the dominant mechanism

## Pause / General Window Shell

`ui_general.yncp`:

- mostly `HideFlag`, `XScale`, `YScale`, and some `YPosition`
- `Scroll_Anim` reaches `200` frames on `window_select`

`ui_pause.yncp`:

- dominant channels:
  - `Color`: `129`
  - `XScale`: `66`
  - `YScale`: `65`
  - `XPosition`: `38`
  - `HideFlag`: `32`
- very little sprite swapping: `SubImage = 2`
- highest pause-local authored duration seen so far:
  - `btn_effect/charge_3_Outro`
  - `240` frames

Reading:

- pause is heavily built on scalable window pieces, visibility gating, and alpha emphasis
- it behaves like a composited shell system, not a texture-flip-heavy HUD

## Status

- strongest track families:
  - `SubImage`: `231`
  - `Color`: `148`
  - gradient corners: `140` to `144` each
  - `XPosition`: `93`
  - `YPosition`: `70`
  - `XScale` / `YScale`: `68`
- `109` authored animations
- many `0`-frame and short-frame `Usual_*` banks coexist with longer intro/outro banks

Reading:

- status uses more sprite swapping and gradient work than pause
- this aligns with richer icon/badge/gauge feedback and more elaborate badge/logo/medal presentation
- the form-specific `Intro_so_Anim` / `Intro_ev_Anim` banks are backed by materially different track payloads, not just renamed placeholders

## World Map

- strongest track families:
  - `Color`: `278`
  - `HideFlag`: `146`
  - `XScale`: `88`
  - `YScale`: `85`
  - `YPosition`: `66`
  - `XPosition`: `62`
- sprite swapping exists but is secondary: `SubImage = 13`
- standout authored bank:
  - `cts_cursor/Intro_Anim`
  - `221` frames
  - `219` keyframes

Reading:

- world map is a dense composited information surface with large numbers of visibility and transform channels
- the cursor and content-window layers are much more animated than a simple static stage-select grid
- compared with pause, world map spreads motion across more scenes and many more cast instances

## Loading

- strongest track families:
  - `Color`: `1308`
  - `XPosition`: `250`
  - `YPosition`: `255`
  - `HideFlag`: `42`
- unique frame-count spread includes:
  - `2`
  - `15`
  - `40`
  - `45`
  - `51`
  - `75`
  - `80`
  - `81`
  - `128`
  - `240`

Two especially important patterns:

- `loadinfo` contains many `2`-frame `360_*` / `ps3_*` banks
- `n_2_d` and `d_2_n` use gradients plus `SubImage` and position channels for day/night transitions

Reading:

- loading combines ultra-short platform/controller swaps with much longer authored mini-sequences
- it behaves like a small presentation engine, not one flat splash sheet

## Boss / Save / Ending Overlays

`ui_boss_gauge.yncp`:

- dominated by `XScale` and `XPosition`
- the longest boss-gauge bank is `total_size` at `100` frames

`ui_boss_name.yncp`:

- dominated by:
  - `Color`
  - `YScale`
  - `YPosition`
  - `XScale`
- all numbered reveal banks sit at `180` frames

`ui_saveicon.yncp`:

- small but complete:
  - `Color`
  - `Rotation`
  - `XScale`
  - `YScale`
- `180` frame intro bank

`ui_end.yncp`:

- compact overlay with `420` frame intro banks
- strongly transform-driven rather than graph-heavy

Reading:

- even tiny overlays keep authored channel banks instead of relying on code-only tweens
- boss name and save icon are small systems, but they still preserve a full channel grammar

## Scene-Graph Depth Signals

Highest observed cast-hierarchy depths in the current set:

- `ui_boss_gauge.yncp`
  - `gauge_bg`
  - max depth `7`
- `ui_saveicon.yncp`
  - `icon`
  - max depth `5`
- `ui_pause.yncp`
  - several scenes at depth `4`
- `ui_worldmap.yncp`
  - multiple heavy content windows at depth `4`
- `ui_status.yncp`
  - most standout scenes top out around depth `3`

Reading:

- depth alone does not equal total complexity
- `boss gauge` is locally deep in one subtree
- `world map` is structurally broader, with very large cast populations even where the maximum depth is lower

## What The Timeline Data Changes

This deeper pass strengthens several earlier claims:

- `pause` is now backed by measurable multi-second effect banks, not just named intro/scroll/size families
- `status` is now clearly a sprite-swap and gradient-heavy presentation layer rather than a mostly static stats board
- `world map` is now confirmed as a broad multi-scene motion system, with dense cursor/content choreography and very large cast populations
- `loading` is now confirmed as a hybrid of ultra-short platform swap banks and longer mini-presentation loops
- `boss HUD`, `save`, and `ending` overlays are now measurably small-but-authored systems, not hand-waved as simple texture fades

## Limitations

- this pass summarizes channel/keyframe structure rather than replaying the full animation curves visually
- a few raw animation names still contain dirty trailing control characters from the source data, and the parser currently preserves them verbatim
- the parser does not yet expose the secondary version-3 vector payloads as fully semantic transform/color objects; it only uses the primary keyframe/channel layer and frame-count tables

## Practical Follow-Up Targets

1. `ui_worldmap.yncp`
   Focus on `cts_cursor`, `cts_stage_window`, and `cts_guide_window`.
2. `ui_status.yncp`
   Focus on `logo`, medal gauges, and `select_*` effect scenes.
3. `ui_pause.yncp`
   Focus on `btn_effect`, `arrow`, and the `bg_*` window layers.
4. `ui_loading.yncp`
   Focus on `loadinfo`, `n_2_d`, and `pda_txt`.
