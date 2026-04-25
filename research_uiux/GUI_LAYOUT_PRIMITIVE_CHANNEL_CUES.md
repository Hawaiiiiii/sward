<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Layout Primitive Channel Cues

Phase 64 adds the first channel-classification layer to the native GUI primitive preview:

```text
b/rr64/sward_ui_runtime_debug_gui.exe
```

The previous beats exposed scene ownership, animation-bank names, frame cursors, and readable primitive detail text. This beat adds a conservative channel mask over the recovered track summaries so the workbench can tell whether a primitive is carrying transform, color, sprite, visibility, or no recognized animated channel.

## What Changed

- added `PrimitiveChannelMask` and `layoutPrimitiveChannelTags(...)`
- classifies recovered track summaries into:
  - `color` from `Color` / `Gradient`
  - `sprite` from `SubImage`
  - `transform` from position, scale, and rotation tracks
  - `visibility` from `HideFlag`
  - `static` when no recognized channel class is present
- adds `ch=...` to primitive overlay labels
- adds `channels=...` to the detail-pane primitive cue summary
- adds `--layout-primitive-channel-smoke` for headless Sonic HUD proxy checks

## Sonic HUD Proxy Channel Snapshot

For the current `ui_prov_playscreen` proxy primitive set:

| Channel class | Count | Notes |
|---|---:|---|
| `transform` | `3` | `so_speed_gauge`, `so_ringenagy_gauge`, `ring_get_effect` |
| `color` | `4` | gauge/effect gradients plus `info_1` |
| `visibility` | `2` | `info_1`, `info_2` |
| `static` | `1` | `bg` |

Representative tags:

```text
so_speed_gauge -> color+transform
info_2 -> visibility
bg -> static
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr64 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr64 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr64\sward_ui_runtime_debug_gui.exe --layout-primitive-channel-smoke
b\rr64\sward_ui_runtime_debug_gui.exe --layout-primitive-detail-smoke
b\rr64\sward_ui_runtime_debug_gui.exe --layout-primitive-playback-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layout primitive channel smoke ok sonic_transform=3 sonic_color=4 sonic_visibility=2 sonic_static=1 speed_channels=color+transform info2_channels=visibility bg_channels=static
```

The GUI was also launch-checked by selecting `Gameplay HUD Hosts -> SonicMainDisplay.cpp`, pressing `Run Host`, and reading the native detail edit control back with `WM_GETTEXT`. The detail text contained `channels=color+transform` for `so_speed_gauge`, `channels=visibility` for `info_2`, and `channels=static` for `bg`.

## Boundary

This is still not original curve sampling. It is the first typed channel semantic layer over the recovered primitive track summaries, which gives the future renderer a cleaner bridge toward transform/color/visibility playback without overclaiming exact node evaluation yet.
