<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Asset CSD Element Bindings

Phase 81 moves the native GUI asset viewer past sheet-only browsing by binding selected atlas candidates to recovered CSD package, scene, cast, and subimage evidence:

```text
b/rr81/sward_ui_runtime_debug_gui.exe
```

Phase 82 continues this foundation with navigable element selection in [`GUI_ASSET_CSD_ELEMENT_NAVIGATION.md`](./GUI_ASSET_CSD_ELEMENT_NAVIGATION.md); the binding smoke now counts the widened table while preserving the original Phase 81 seed samples.

## What Changed

- added a `LayoutCsdElementBinding` table in the native GUI over parsed `layout_deep_analysis.json` facts
- added asset-viewer CSD cue markers over the selected atlas image
- added `CSD element bindings:` detail-pane summaries for selected hosts
- added `--asset-csd-binding-smoke` so automation verifies the bound package/scene/cast/subimage counts without opening a window

Current bound samples:

| Contract | Package | Scene | Cast | Scene Casts | Scene Subimages | Package Subimages |
|---|---|---|---|---:|---:|---:|
| `sonic_stage_hud_reference.json` | `ui_prov_playscreen.yncp` | `so_speed_gauge` | `position_hd` | `47` | `109` | `654` |
| `werehog_stage_hud_reference.json` | `ui_prov_playscreen.yncp` | `so_speed_gauge` | `position_hd` | `47` | `109` | `654` |
| `extra_stage_hud_reference.json` | `ui_prov_playscreen.yncp` | `so_speed_gauge` | `position_hd` | `47` | `109` | `654` |
| `loading_transition_reference.json` | `ui_loading.yncp` | `bg_1` | `arrow` | `28` | `320` | `2240` |
| `pause_menu_reference.json` | `ui_pause.yncp` | `bg` | `img` | `1` | `99` | `2871` |
| `title_menu_reference.json` | `ui_mainmenu.xncp` | `mm_bg_usual` | `black3` | `47` | `46` | `736` |

## Verification

```text
sward_ui_runtime_debug_gui asset csd binding smoke ok bindings=41 sonic=ui_prov_playscreen.yncp/so_speed_gauge/position_hd:casts=47:subimages=109 loading=ui_loading.yncp/bg_1/arrow:casts=28:subimages=320 pause=ui_pause.yncp/bg/img:casts=1:subimages=99 title=ui_mainmenu.xncp/mm_bg_usual/black3:casts=47:subimages=46
```

This is still an element binding layer, not full decoded subimage rendering. The next product step is to turn these bindings into navigable subimage/cast selection: per-scene element lists, crop/rect previews where recoverable, and then family-specific CSD timeline playback.
