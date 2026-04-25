<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Authored Sampled Transform Preview

Phase 75 moves the first authored sampled transform evidence out of the detail pane and into the native GUI preview:

```text
b/rr75/sward_ui_runtime_debug_gui.exe
```

Phase 74 proved that parsed authored cast rectangles can be paired with sampled CSD keyframe values. This beat draws those sampled rectangles as preview markers so the window starts using authored sampled transform evidence as visual data.

## What Changed

- added `LayoutAuthoredSampledTransformPixels`
- added `layoutAuthoredSampledTransformPixels(...)`
- added `layoutAuthoredSampledTransformPreviewDescriptor(...)`
- added `layoutAuthoredSampledTransformRect(...)`
- added `drawAuthoredSampledTransforms(...)`
- wired the preview paint path to draw authored sampled markers for selected contracts
- added `--authored-sampled-transform-preview-smoke`

## Verified Preview Markers

```text
title:   ui_mainmenu.xncp / mm_donut_move / index_text_pos / YPosition @ frame 30
loading: ui_loading.yncp / bg_2 / pos_text_sonic / XPosition @ frame 1
```

Preview marker shape:

```text
scene/cast:x,y,widthxheight
```

Verified marker geometry:

```text
mm_donut_move/index_text_pos:408,163,16x16
bg_2/pos_text_sonic:640,360,16x16
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr75 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr75 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr75\sward_ui_runtime_debug_gui.exe --authored-sampled-transform-preview-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui authored sampled transform preview smoke ok title_preview=mm_donut_move/index_text_pos:408,163,16x16 loading_preview=bg_2/pos_text_sonic:640,360,16x16
```

## Boundary

This is still a diagnostic preview layer, not full CSD playback. It proves that the GUI can draw authored sampled transform evidence on the same canvas as atlas sheets, runtime overlays, layout primitives, and evidence panels. The next product step is turning these sampled markers into broader family-specific draw commands driven by decoded CSD node transforms and animation channels.
