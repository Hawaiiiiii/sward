<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Authored Cast Transform Descriptors

Phase 71 adds the first authored CSD cast transform descriptor layer to the native GUI:

```text
b/rr71/sward_ui_runtime_debug_gui.exe
```

The previous draw-command descriptors used recovered primitive rectangles. This beat starts binding the renderer surface back to parsed authored cast data from `research_uiux/data/layout_deep_analysis.json`, using exact-family CSD packages for Title, Pause, and Loading.

## What Changed

- added `LayoutAuthoredCastTransform`
- added `layoutAuthoredCastTransformsForContract(...)`
- added `layoutAuthoredCastTransformDescriptor(...)`
- added an `Authored cast transforms:` detail-pane section
- added `--authored-cast-transform-smoke`

## Verified Casts

```text
title:   ui_mainmenu.xncp / mm_donut_move / index_text_pos
pause:   ui_pause.yncp / bg / img
loading: ui_loading.yncp / bg_2 / pos_text_sonic
```

Descriptor shape:

```text
scene/cast:x,y,widthxheight:rrotation:scaleX,scaleY:color
```

Verified descriptors:

```text
mm_donut_move/index_text_pos:408,296,16x16:r0.00:s1.00,1.00:0xFFFFFFFF
bg/img:0,0,1280x720:r0.00:s1.00,1.00:0x00000000
bg_2/pos_text_sonic:640,360,16x16:r0.00:s1.00,1.00:0xFFFFFFFF
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr71 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr71 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr71\sward_ui_runtime_debug_gui.exe --authored-cast-transform-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui authored cast transform smoke ok title_cast=mm_donut_move/index_text_pos:408,296,16x16:r0.00:s1.00,1.00:0xFFFFFFFF pause_cast=bg/img:0,0,1280x720:r0.00:s1.00,1.00:0x00000000 loading_cast=bg_2/pos_text_sonic:640,360,16x16:r0.00:s1.00,1.00:0xFFFFFFFF
```

## Boundary

These are authored cast transform descriptors, not full CSD curve evaluation. They are the first stable bridge from parsed CSD cast data into the renderer-facing GUI detail path.
