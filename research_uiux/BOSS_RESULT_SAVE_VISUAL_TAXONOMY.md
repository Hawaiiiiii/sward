# <img src="../docs/assets/branding/icon_extra.png" width="30" alt="SWARD extra icon"/> Boss / Result / Save Visual Taxonomy

## Summary

Phase 20 converts the extracted atlas/layout layer into a visual taxonomy and screen-by-screen style guide for:

- boss HUD
- result screens
- save feedback

Machine-readable support for this pass lives in `research_uiux/data/visual_taxonomy.json`.

> [!IMPORTANT]
> This report is about composition, palette strategy, texture economy, and motion vocabulary. It is not a license to reuse the original art. The useful transfer layer is the relationship between texture count, layout hierarchy, accent color, and timing.

## Inputs Used

- `extracted_assets/visual_atlas/atlas_index.json`
- `research_uiux/data/layout_deep_analysis.json`
- `research_uiux/data/visual_taxonomy.json`
- local atlas sheets under `extracted_assets/visual_atlas/sheets/`

Focused layouts in this pass:

- `ui_boss_gauge.yncp`
- `ui_boss_name.yncp`
- `ui_result.yncp`
- `ui_result_ex.yncp`
- `ui_itemresult.yncp`
- `ui_saveicon.yncp`

## Family Taxonomy

### Boss HUD

Layouts:

- `ui_boss_gauge.yncp`
- `ui_boss_name.yncp`

Current family characteristics:

- extremely low texture count
- no layout-authored font usage in the current sample
- dominant palette is near-monochrome chrome:
  - `#E0E0E0`
  - `#000000`
  - `#C0C0C0`
- accent palette is explicitly hostile:
  - `#E00000`
  - `#C00000`
  - darker red variants

Motion vocabulary:

- short size pulses around `1.666667s`
- long nameplate emphasis holds around `3.0s`

Interpretation:

- the boss HUD reads as a threat instrument, not a data dashboard
- texture economy stays tiny while motion and segmentation carry the emotional weight

### Result Family

Layouts:

- `ui_result.yncp`
- `ui_result_ex.yncp`
- `ui_itemresult.yncp`

Current family characteristics:

- moderate texture count, but much richer scene count and animation vocabulary than boss/save
- shared numeric emphasis through `Num_32`
- family palette stays mostly grayscale chrome:
  - `#E0E0E0`
  - `#000000`
  - `#C0C0C0`
- accent palette splits into two subfamilies:
  - warm gold-like emphasis for rank/result:
    - `#C0A060`
  - cooler teal/navy accents for item-result chrome:
    - `#204040`
    - `#202040`
    - `#002020`

Motion vocabulary:

- long result-rank flourish windows up to `4.216667s`
- sustain holds from `2.0s` to `3.333333s`
- item-result intros/outros around `1.416667s`

Interpretation:

- result screens are presentation-heavy reward moments
- the palette is restrained, but the taxonomy still distinguishes “grade ceremony” from “item inventory accounting”

### Save

Layout:

- `ui_saveicon.yncp`

Current family characteristics:

- minimal texture count: `2`
- no font layer
- hard monochrome bias:
  - `#E0E0E0`
  - `#404040`
- no meaningful accent palette in the extracted sample

Motion vocabulary:

- one authored intro bank at `3.0s`

Interpretation:

- save feedback is designed to be recognizable, low-noise, and easy to overlay on top of anything else
- the visual identity comes from persistence and clarity, not ornament

## Screen-By-Screen Style Guide

### `ui_boss_gauge.yncp`

Style read:

- segmented horizontal pressure bar
- low-asset, high-contrast composition
- red is reserved for alarm/threat emphasis rather than general decoration

Evidence:

- texture count: `2`
- scenes: `gauge_bg`, `gauge_2`, `gauge_1`, `gauge_breakpoint`
- strongest accent swatches: `#E00000`, `#C00000`
- longest timing band: `1.666667s`

Reusable rule:

- if a combat meter needs urgency, keep the art count tiny and let contrast plus one danger accent color do the work

### `ui_boss_name.yncp`

Style read:

- ceremonial title plate
- broad monochrome plate with long reveal/emphasis timing
- pairs naturally with the gauge but does not duplicate its aggressive red accent

Evidence:

- texture count: `2`
- scenes: `name_so`, `name_ev`
- reveal banks: `01_Anim` through `05_Anim`
- longest timing band: `3.0s`

Reusable rule:

- nameplates can afford longer, slower emphasis than health meters because they are identity beats, not reactive telemetry

### `ui_result.yncp`

Style read:

- formal grade-ceremony screen
- heavy reliance on numeric and rank scenes
- grayscale chrome with a warm prestige accent

Evidence:

- texture count: `7`
- scene count: `17`
- fonts: `Num_32`
- key scenes: `result_title`, `result_rank`, `result_rank_E`, `result_newR`
- accent swatch: `#C0A060`
- longest timing band: `4.216667s`

Reusable rule:

- grade/result screens benefit from one prestige accent tone layered over mostly neutral chrome so the rank moment lands as a reward, not as clutter

### `ui_result_ex.yncp`

Style read:

- same family as the main result screen, but with a more sustained central hold
- keeps the ceremony language while simplifying some scene structure

Evidence:

- texture count: `7`
- scene count: `15`
- fonts: `Num_32`
- same warm accent: `#C0A060`
- long sustain on `result_rank` at `3.333333s`

Reusable rule:

- alternate result screens can share the same texture family if their identity comes from timing and scene orchestration rather than fresh art

### `ui_itemresult.yncp`

Style read:

- more panelized and inventory-like than the main result screens
- uses cooler accents and more explicit window/content/footer partitioning

Evidence:

- texture count: `8`
- scene count: `4`
- fonts: `Num_32`, `Num_L`
- key scenes: `iresult_title`, `window`, `contents`, `result_footer`
- accent swatches: `#204040`, `#202040`, `#002020`
- intro/outro timing: `1.416667s`

Reusable rule:

- when a summary screen shifts from “ceremony” to “inventory/accounting,” cooler accents and stronger panel division read better than trying to reuse the full grade-screen drama unchanged

### `ui_saveicon.yncp`

Style read:

- almost purely symbolic
- no palette noise, no typography, no scene branching
- relies on dwell time rather than spectacle

Evidence:

- texture count: `2`
- scene count: `1`
- scene: `icon`
- no accent palette
- timing band: `3.0s`

Reusable rule:

- small confirmation overlays should stay visually sparse; persistence and placement matter more than complexity

## Cross-Screen Design Rules

- boss UI wants contrast and threat accents
- result UI wants neutral chrome plus one reward accent
- item-result variants can cool down the accent palette to shift from celebration toward inventory clarity
- save feedback should remain almost icon-only
- the more reactive the system, the fewer textures it needs
- the more ceremonial the system, the more it relies on long flourish timing and scene variety

## Transferable Guidance For Original Projects

- use grayscale or low-chroma chrome as the baseline across related screen families
- reserve accent colors for role:
  - red for danger/threat
  - warm metallic for prestige/reward
  - cool teal/navy for utility/detail/result breakdowns
- separate telemetry, identity, ceremony, and confirmation into different timing bands
- do not spend texture count where timing and scene hierarchy can do the same job

> [!TIP]
> Pair this report with `VISUAL_ATLAS_DOCS.md`, `BOSS_RESULT_SAVE_LOAD_DEEP_DIVE.md`, and `TEMPLATE_PACK_FOR_ORIGINAL_PROJECTS.md` when turning these patterns into original UI systems.
