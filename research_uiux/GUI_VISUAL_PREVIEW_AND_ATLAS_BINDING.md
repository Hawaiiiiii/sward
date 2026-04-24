<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Visual Preview And Atlas Binding

Phase 52 turned the native GUI workbench from a metadata/action shell into the first visual preview surface. Phase 53 added the first gameplay-HUD proxy atlas bindings, Phase 54 added timer-driven playback controls over the same preview, and Phase 55 adds state-aware preview motion.

```text
b/rr55/sward_ui_runtime_debug_gui.exe
```

The preview remains publishable because the repo only commits renderer code and atlas lookup logic. The extracted atlas PNGs still live under ignored local-only `extracted_assets/`.

## What Changed

- added a dedicated `SwardUiRuntimePreviewPanel` Win32 child window
- added GDI+ rendering for local atlas PNGs when the ignored visual atlas exists
- added a contract-to-atlas candidate table for `10` screen families, including `2` explicit proxy bindings for gameplay HUD previews
- draws a 16:9 preview surface inside the GUI workbench
- overlays current `ScreenRuntime` visible layers as translucent rectangles
- draws visible prompt rows on the preview canvas
- draws a small timeline/progress strip for the active runtime state
- adds `--preview-smoke` so automation can validate atlas bindings without opening a window
- adds Play/Pause and Step controls so the timeline/progress strip can be inspected during intro/action bands

Verified preview smoke:

```text
sward_ui_runtime_debug_gui preview smoke ok atlas_candidates=10 proxy_candidates=2 existing_local_atlas=10 title=mainmenu__ui_mainmenu.png pause=systemcommoncore__ui_pause.png sonic_stage=exstagetails_common__ui_prov_playscreen.png
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
| `extra_stage_hud_reference.json` | `exstagetails_common__ui_prov_playscreen.png` |
| `sonic_stage_hud_reference.json` | `exstagetails_common__ui_prov_playscreen.png` (`proxy`) |
| `werehog_stage_hud_reference.json` | `exstagetails_common__ui_prov_playscreen.png` (`proxy`) |

## Current Limits

- The atlas image is a local visual reference sheet, not an interactive layout renderer.
- The overlay rectangles are schematic runtime-layer projections, not decoded CSD node transforms.
- The timeline strip reflects the portable contract state band, not original CSD keyframe playback.
- Phase 54 can play those portable bands over time, but decoded original CSD timeline playback is still future work.
- Phase 55 can move/fade those projected layers during active state bands, but the motion is still a portable adapter rather than recovered original CSD curves.
- Sonic/Werehog gameplay HUD atlas binding is intentionally marked as a proxy until exact loose `ui_playscreen*` payloads are recovered.
- Proprietary atlas PNGs remain local-only and ignored.

## Next Product Step

The next high-value GUI beat is a family-specific visual playback path for one small, strong target. Title, pause, loading, or world map are the cleanest candidates because they already have contract coverage plus local atlas/layout evidence.
