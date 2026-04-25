<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Asset CSD Crop Preview

Phase 83 turns the selected Asset View CSD element into a deterministic footprint/crop preview:

```text
b/rr83/sward_ui_runtime_debug_gui.exe
```

## What Changed

- added `LayoutCsdElementCrop` descriptors derived from the selected package/scene/cast binding
- converts each selected element's normalized atlas footprint into a stable `1280x720` pixel rect
- surfaces `CSD element crop:` in the native detail pane and runtime snapshot text
- draws a selected-element crop inset in Asset View using the local atlas image when available
- includes crop metadata in the Asset View footer beside the selected CSD element
- added `--asset-csd-crop-smoke` for non-windowing verification

Current selected crop samples:

| Contract family | Scene | Pixel rect | Normalized rect |
|---|---|---|---|
| `sonic_stage_hud_reference.json` | `so_speed_gauge` | `691,418,486x86` | `0.54,0.58,0.38x0.12` |
| `loading_transition_reference.json` | `bg_1` | `77,58,1126x86` | `0.06,0.08,0.88x0.12` |
| `title_menu_reference.json` | `mm_bg_usual` | `102,86,1050x130` | `0.08,0.12,0.82x0.18` |
| `pause_menu_reference.json` | `bg` | `51,58,1178x590` | `0.04,0.08,0.92x0.82` |

## Verification

```text
sward_ui_runtime_debug_gui asset csd crop smoke ok sonic=so_speed_gauge:691,418,486x86:normalized=0.54,0.58,0.38x0.12 loading=bg_1:77,58,1126x86:normalized=0.06,0.08,0.88x0.12 title=mm_bg_usual:102,86,1050x130:normalized=0.08,0.12,0.82x0.18 pause=bg:51,58,1178x590:normalized=0.04,0.08,0.92x0.82
```

This is still a footprint/crop bridge, not final subimage reconstruction. The next useful renderer step is resolving decoded subimage rectangles and cast-local transforms so the crop panel can move from atlas-footprint inspection to exact subimage/cast draw commands.
