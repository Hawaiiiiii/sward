<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Layout Primitive Channel Legend

Phase 65 adds a compact visual legend for the primitive channel-classification layer:

```text
b/rr65/sward_ui_runtime_debug_gui.exe
```

Phase 64 classified recovered primitive track summaries as transform, color, sprite, visibility, or static and exposed those tags in primitive labels plus the detail pane. This beat makes the count distribution visible in the preview itself so the operator can see whether a host is mostly transform playback, color/gradient playback, visibility switching, sprite swapping, or static layout support.

## What Changed

- added `PrimitiveChannelCounts` and channel-count aggregation over `LayoutScenePrimitive`
- added `layoutPrimitiveChannelLegendLabel(...)`
- added `drawLayoutPrimitiveChannelLegend(...)` to the preview paint path
- draws a compact top-left preview badge such as `Channels T3 C4 V2 S0 static1`
- added `--layout-primitive-channel-legend-smoke` for headless verification

## Sonic HUD Proxy Legend

For the current `ui_prov_playscreen` proxy primitive set, `SonicMainDisplay.cpp` reports:

```text
Channels T3 C4 V2 S0 static1
```

Expanded:

| Channel class | Count |
|---|---:|
| transform | `3` |
| color | `4` |
| visibility | `2` |
| sprite | `0` |
| static | `1` |

The GUI preview was launch-checked by selecting `Gameplay HUD Hosts -> SonicMainDisplay.cpp`, pressing `Run Host`, and capturing the native window through `PrintWindow`. The preview surface showed the channel legend in the header band above the diagnostic primitive boxes.

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr65 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr65 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr65\sward_ui_runtime_debug_gui.exe --layout-primitive-channel-legend-smoke
b\rr65\sward_ui_runtime_debug_gui.exe --layout-primitive-channel-smoke
b\rr65\sward_ui_runtime_debug_gui.exe --layout-primitive-detail-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layout primitive channel legend smoke ok legend=T3 C4 V2 S0 static1 label=Channels T3 C4 V2 S0 static1
```

## Boundary

This legend is still diagnostic metadata over recovered primitive track summaries. It does not yet sample original CSD curves or evaluate authored node transforms. Its value is operator visibility: the preview now tells us which channel families the next renderer pass must implement for a selected host.
