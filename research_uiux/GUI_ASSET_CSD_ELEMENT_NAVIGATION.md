<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Asset CSD Element Navigation

Phase 82 turns the first Asset View CSD binding pass into a small element inspector:

```text
b/rr82/sward_ui_runtime_debug_gui.exe
```

## What Changed

- widened `LayoutCsdElementBinding` from `6` seed bindings to `41` package/scene/cast bindings
- added `Element Prev` / `Element Next` controls in the native GUI
- keeps the selected CSD element separate from atlas-sheet gallery selection
- draws all CSD element markers faintly while highlighting and labeling the selected element
- added `CSD element selection:` detail-pane text with selected index, package, scene, cast, anchor, and texture-family summary
- added `--asset-csd-navigation-smoke` for non-windowing verification

Current navigable sets:

| Contract family | Bound elements |
|---|---:|
| `sonic_stage_hud_reference.json` | `6` |
| `werehog_stage_hud_reference.json` | `6` |
| `extra_stage_hud_reference.json` | `6` |
| `loading_transition_reference.json` | `7` |
| `pause_menu_reference.json` | `8` |
| `title_menu_reference.json` | `8` |

## Verification

```text
sward_ui_runtime_debug_gui asset csd navigation smoke ok sonic_count=6 selected=ui_prov_playscreen.yncp/so_speed_gauge/position_hd:casts=47:subimages=109 next=ui_prov_playscreen.yncp/so_ringenagy_gauge/position_hd:casts=43:subimages=109 previous=ui_prov_playscreen.yncp/ring_get_effect/position_hd:casts=2:subimages=109 loading_count=7 title_count=8 pause_count=8
```

This is still not final CSD rendering. It is the bridge between atlas browsing and a real element viewer: each highlighted item now has evidence-backed package/scene/cast ownership, and the next useful step is deriving crop/rect previews and per-scene lists from decoded subimage/cast geometry.
