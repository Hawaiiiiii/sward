<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Visual Atlas Docs

## Summary

Phase 17 turns the extracted UI texture layer into a browsable local atlas set without pushing derivative images into git history.

- Render path verified locally with `Pillow 12.2.0`
- Local atlas root generated at `extracted_assets/visual_atlas`
- `23` atlas sheets generated from the current extracted layout set
- `144` referenced DDS textures rendered into contact-sheet style atlases

> [!IMPORTANT]
> The generated atlas PNG sheets are local-only because they are derived from extracted proprietary textures. This report and the atlas-builder script are the publishable layer.

## Local Output Shape

The atlas pass writes:

- `extracted_assets/visual_atlas/sheets/*.png`
- `extracted_assets/visual_atlas/atlas_index.json`
- `extracted_assets/visual_atlas/INDEX.md`

Generation command used in this pass:

```powershell
python research_uiux/tools/build_visual_atlas.py `
  --layout-data research_uiux/data/layout_deep_analysis.json `
  --search-root extracted_assets `
  --output-root extracted_assets/visual_atlas `
  --max-textures 16
```

## What The Atlas Layer Adds

The earlier semantic/layout reports already exposed scene names, animation families, frame counts, and track types. The atlas layer adds a fast visual bridge between:

- extracted layout packages
- real DDS texture payloads
- screen families already mapped in readable code and patch hooks

That matters because several systems are easier to validate visually than textually:

- main-menu texture partitioning
- pause/status/window chrome reuse
- world-map information panel families
- loading-card language/platform variants
- result/boss/save texture minimalism versus scene-count complexity

## Screen Families Covered

### Main Menu / Front-End

- `ui_mainmenu.yncp`
  - atlas: `extracted_assets/visual_atlas/sheets/mainmenu__ui_mainmenu.png`
  - rendered textures: `3`
  - strongest texture evidence: `ui_mm_base.dds`, `ui_mm_parts1.dds`, `ui_mm_contentstext.dds`
  - strongest scene evidence: `mm_bg_*`, `mm_donut_*`

The atlas confirms that the main menu gets a relatively small texture set and pushes most of its richness into scene composition and animation families rather than a huge material spread.

### Pause / Status / System Shells

- `ui_pause.yncp`
  - atlas: `extracted_assets/visual_atlas/sheets/systemcommoncore__ui_pause.png`
  - rendered textures: `9`
  - representative scenes: `bg`, `bg_1_select`, `text_area`, `skill_select`
- `ui_status.yncp`
  - atlas: `extracted_assets/visual_atlas/sheets/systemcommoncore__ui_status.png`
  - rendered textures: `15`
  - representative scenes: `logo`, `status_title`, `status_footer`

These sheets make the reusable system-shell strategy obvious: shared common-number/common-frame materials are recombined into multiple overlay types, while readable hooks control state and placement.

### World Map / Loading / Save

- `ui_worldmap.yncp`
  - atlas: `extracted_assets/visual_atlas/sheets/worldmap__ui_worldmap.png`
  - rendered textures: `16`
  - representative scenes: `worldmap_background`, `info_bg_*`, `cts_info_bg`
- `ui_loading.yncp`
  - atlas: `extracted_assets/visual_atlas/sheets/loading__ui_loading.png`
  - rendered textures: `10`
  - representative scenes: `loadinfo`, `n_2_d`, `event_viewer`, `pda`
- `ui_saveicon.yncp`
  - atlas: `extracted_assets/visual_atlas/sheets/autosave__ui_saveicon.png`
  - rendered textures: `2`
  - representative scene: `icon`

The world-map atlas shows the widest texture vocabulary in this pass, while save remains intentionally tiny. Loading sits between them, with distinct card/info assets and platform-language branches.

### Boss / Result / Mission

- `ui_boss_gauge.yncp`
  - atlas: `extracted_assets/visual_atlas/sheets/bosscommon__ui_boss_gauge.png`
  - rendered textures: `2`
  - representative scenes: `gauge_bg`, `gauge_2`, `gauge_1`, `gauge_breakpoint`
- `ui_boss_name.yncp`
  - atlas: `extracted_assets/visual_atlas/sheets/bosscommon__ui_boss_name.png`
  - rendered textures: `2`
  - representative scenes: `name_so`, `name_ev`
- `ui_result.yncp`
  - atlas: `extracted_assets/visual_atlas/sheets/actioncommon__ui_result.png`
  - rendered textures: `7`
  - representative scenes: `result_title`, `result_num_*`
- `ui_result_ex.yncp`
  - atlas: `extracted_assets/visual_atlas/sheets/exstagetails_common__ui_result_ex.png`
  - rendered textures: `7`
  - representative scenes: `result_title`, `result_num`, `result_tag`, `result_newR`

This family is the clearest reminder that scene complexity and texture count are not the same thing. Boss-name and save overlays are texture-light but behavior-rich; result layouts use a moderate texture set but distribute it across long animation banks and many scene shells.

## Practical Readings

- Low texture count does not imply low UI complexity.
  Boss-name, save, and some title/result shells prove that authored timing and grouping carry a large part of the visual load.
- Shared texture families are doing real architectural work.
  Common-number/common-frame sheets appear across pause, status, result, and mission-oriented layouts.
- Atlas review is a fast sanity check on parser claims.
  When a layout reports many scenes but only a handful of textures, the likely complexity source is grouping, animation, and subimage switching rather than raw asset volume.

## Reuse Value For Original Projects

The atlas layer is useful as a template source in a non-copying sense:

- identify how many materials a screen type really needs
- separate “signature art” from reusable chrome
- budget texture count independently from animation complexity
- keep small overlays lean while letting authored timing do the expressive work

> [!TIP]
> For local review, open `extracted_assets/visual_atlas/INDEX.md` first, then compare the relevant atlas sheet against `XNCP_YNCP_SEMANTIC_NOTES.md`, `CODE_TO_LAYOUT_CORRELATION.md`, and the deep-dive reports for the same screen family.
