<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Authored Sampled Transform Descriptors

Phase 74 adds the first renderer-facing authored sampled transform descriptor layer to the native GUI:

```text
b/rr74/sward_ui_runtime_debug_gui.exe
```

Phase 73 sampled authored keyframe values. This beat pairs transform-bearing sampled curves with authored cast rectangles so the GUI can report sampled 1280x720 draw positions for exact-family casts.

## What Changed

- added `LayoutAuthoredSampledTransform`
- added `layoutAuthoredSampledTransformsForContract(...)`
- added `layoutAuthoredSampledTransformDescriptor(...)`
- added an `Authored sampled transforms:` detail-pane section
- added `--authored-sampled-transform-smoke`

## Verified Transforms

```text
title:   ui_mainmenu.xncp / mm_donut_move / index_text_pos / YPosition @ frame 30
loading: ui_loading.yncp / bg_2 / pos_text_sonic / XPosition @ frame 1
```

Descriptor shape:

```text
scene/cast@frame:x,y,widthxheight:track=value
```

Verified descriptors:

```text
mm_donut_move/index_text_pos@30:408,163,16x16:YPosition=0.225926
bg_2/pos_text_sonic@1:640,360,16x16:XPosition=0.500000
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr74 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr74 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr74\sward_ui_runtime_debug_gui.exe --authored-sampled-transform-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui authored sampled transform smoke ok title_transform=mm_donut_move/index_text_pos@30:408,163,16x16:YPosition=0.225926 loading_transform=bg_2/pos_text_sonic@1:640,360,16x16:XPosition=0.500000
```

## Boundary

These descriptors are still diagnostic. They prove that authored cast rectangles can be adjusted by sampled authored transform tracks, but the preview does not yet use these descriptors as the primary draw source for full CSD playback.
