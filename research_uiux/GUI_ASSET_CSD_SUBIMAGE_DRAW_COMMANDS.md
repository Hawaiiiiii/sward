<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Asset CSD Subimage Draw Commands

Phase 85 turns the selected CSD cast/subimage evidence into deterministic renderer-facing draw-command descriptors:

```text
b/rr85/sward_ui_runtime_debug_gui.exe
```

What changed:

- added `LayoutCsdSubimageDrawCommand`
- derives source texture pixel rectangles from parsed UV bounds plus cast dimensions
- derives scaled destination sizes from cast dimensions and parsed scale
- surfaces `CSD subimage draw command:` in the GUI detail pane and runtime snapshot text
- draws an Asset View draw-command cue above the selected cast/subimage cue
- added `--asset-csd-draw-command-smoke` for non-windowing verification

Verified draw-command samples:

```text
sonic=so_speed_gauge/position_hd->Cast_0506_bg:tex=ui_ps1_gauge1.dds:src=4,64,16x20:dst=16x20
loading=bg_1/arrow->img_1:tex=mat_load_comon_001.dds:src=595,121,300x240:dst=300x240
title=mm_bg_usual/black3->black3:tex=ui_mm_parts1.dds:src=896,336,16x16:dst=368x464
pause=bg/img->img:fill=cast:dst=1280x720
```

Evidence basis:

- source texture dimensions are inferred from `castWidth / uvWidth` and `castHeight / uvHeight`
- source rects are then recovered as normalized UV bounds projected into the inferred texture dimensions
- destination rects are cast dimensions multiplied by parsed scale values
- the Pause `bg/img` case remains a no-subimage cast fill because the selected parsed scene exposes cast geometry but no drawable subimage

Current limit:

- these are draw-command descriptors, not yet an executed renderer command stream.
- Phase 86 now projects these descriptors into virtual `1280x720` render-target rectangles in [`GUI_ASSET_CSD_RENDER_PLAN_PREVIEW.md`](./GUI_ASSET_CSD_RENDER_PLAN_PREVIEW.md).
- the next step is to bind those render plans to actual decoded subimage blits, then feed timeline/channel sampling into those commands.
