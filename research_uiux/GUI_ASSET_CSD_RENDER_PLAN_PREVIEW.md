<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Asset CSD Render Plan Preview

Phase 86 projects the selected CSD source/destination draw-command descriptors into deterministic `1280x720` render-target rectangles:

```text
b/rr86/sward_ui_runtime_debug_gui.exe
```

What changed:

- added `LayoutCsdSubimageRenderPlan`
- projects selected CSD draw commands into virtual target-space destination rectangles
- surfaces `CSD render plan:` in the GUI detail pane and runtime snapshot text
- draws a mini `1280x720` render-target preview in Asset View
- added `--asset-csd-render-plan-smoke` for non-windowing verification

Verified render-plan samples:

```text
sonic=so_speed_gauge/position_hd->Cast_0506_bg:texture:src=4,64,16x20:dst=752,357,16x20
loading=bg_1/arrow->img_1:texture:src=595,121,300x240:dst=350,360,300x240
title=mm_bg_usual/black3->black3:texture:src=896,336,16x16:dst=655,435,368x464
pause=bg/img->img:fill:dst=0,0,1280x720
```

Evidence basis:

- source rectangles come from the Phase 85 UV/cast-derived draw-command layer
- target-space centers are recovered from the selected CSD cast translation values projected into a virtual `1280x720` canvas
- destination rectangles are then centered around those recovered target-space positions
- no-subimage fills remain full target-space rectangles when the CSD evidence exposes cast geometry but no drawable subimage

Current limit:

- this is a deterministic render plan preview, not final composited texture rendering.
- the next step is to bind these render plans to actual decoded subimage blits, then feed sampled CSD channels/timeline playback into the same command path.
