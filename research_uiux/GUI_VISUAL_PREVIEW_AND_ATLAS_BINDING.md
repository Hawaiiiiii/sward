<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Visual Preview And Atlas Binding

Phase 52 turns the native GUI workbench from a metadata/action shell into the first visual preview surface.

```text
b/rr52/sward_ui_runtime_debug_gui.exe
```

The preview remains publishable because the repo only commits renderer code and atlas lookup logic. The extracted atlas PNGs still live under ignored local-only `extracted_assets/`.

## What Changed

- added a dedicated `SwardUiRuntimePreviewPanel` Win32 child window
- added GDI+ rendering for local atlas PNGs when the ignored visual atlas exists
- added a contract-to-atlas candidate table for `8` high-confidence screen families
- draws a 16:9 preview surface inside the GUI workbench
- overlays current `ScreenRuntime` visible layers as translucent rectangles
- draws visible prompt rows on the preview canvas
- draws a small timeline/progress strip for the active runtime state
- adds `--preview-smoke` so automation can validate atlas bindings without opening a window

Verified preview smoke:

```text
sward_ui_runtime_debug_gui preview smoke ok atlas_candidates=8 existing_local_atlas=8 title=mainmenu__ui_mainmenu.png pause=systemcommoncore__ui_pause.png
```

## Current Atlas Candidates

| Runtime contract | Local atlas sheet |
|---|---|
| `title_menu_reference.json` | `mainmenu__ui_mainmenu.png` |
| `pause_menu_reference.json` | `systemcommoncore__ui_pause.png` |
| `autosave_toast_reference.json` | `autosave__ui_saveicon.png` |
| `loading_transition_reference.json` | `loading__ui_loading.png` |
| `mission_result_reference.json` | `actioncommon__ui_result.png` |
| `world_map_reference.json` | `worldmap__ui_worldmap.png` |
| `boss_hud_reference.json` | `bosscommon__ui_boss_gauge.png` |
| `extra_stage_hud_reference.json` | `exstagetails_common__ui_exstage.png` |

## Current Limits

- The atlas image is a local visual reference sheet, not an interactive layout renderer.
- The overlay rectangles are schematic runtime-layer projections, not decoded CSD node transforms.
- The timeline strip reflects the portable contract state band, not original CSD keyframe playback.
- Proprietary atlas PNGs remain local-only and ignored.

## Next Product Step

The next high-value GUI beat is a family-specific visual playback path for one small, strong target. Title, pause, loading, or world map are the cleanest candidates because they already have contract coverage plus local atlas/layout evidence.
