<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Authored Sampled Channel Evaluation

Phase 78 turns the Phase 77 one-channel alpha proof into a reusable sampled-channel state evaluator:

```text
b/rr78/sward_ui_runtime_debug_gui.exe
```

The native GUI draw-command bridge now keeps both the authored cast-space origin and sampled render position, then evaluates per-command channel state for preview diagnostics.

## What Changed

- added `LayoutAuthoredSampledChannelState`
- added `layoutAuthoredSampledDrawCommandChannelState(...)`
- added `layoutAuthoredSampledDrawCommandEvaluatedStateDescriptor(...)`
- carried `baseX` and `baseY` through `LayoutAuthoredSampledDrawCommand`
- derived sampled alpha, visibility, and cast-space deltas from the command
- changed the authored sampled preview label path to use evaluated state descriptors
- added `--authored-sampled-channel-eval-smoke`

## Verified Evaluated States

```text
title:   ui_mainmenu.xncp / mm_donut_move / index_text_pos / YPosition @ frame 30
pause:   ui_pause.yncp / bg / img / Color @ frame 7
loading: ui_loading.yncp / bg_2 / pos_text_sonic / XPosition @ frame 1
```

Descriptor shape:

```text
scene/cast:x,y,widthxheight:track@frame=value:alpha=value:visible=0|1:delta=x,y
```

Verified descriptors:

```text
mm_donut_move/index_text_pos:408,163,16x16:YPosition@30=0.225926:alpha=1.000000:visible=1:delta=0,-133
bg/img:0,0,1280x720:Color@7=0.000000:alpha=0.000000:visible=1:delta=0,0
bg_2/pos_text_sonic:640,360,16x16:XPosition@1=0.500000:alpha=1.000000:visible=1:delta=0,0
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr78 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr78 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr78\sward_ui_runtime_debug_gui.exe --authored-sampled-channel-eval-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui authored sampled channel eval smoke ok title_eval=mm_donut_move/index_text_pos:408,163,16x16:YPosition@30=0.225926:alpha=1.000000:visible=1:delta=0,-133 pause_eval=bg/img:0,0,1280x720:Color@7=0.000000:alpha=0.000000:visible=1:delta=0,0 loading_eval=bg_2/pos_text_sonic:640,360,16x16:XPosition@1=0.500000:alpha=1.000000:visible=1:delta=0,0
```

## Boundary

This is still a sampled diagnostic evaluator, not full CSD playback. The important move is architectural: sampled channel interpretation now has a stable state object that can grow beyond this three-sample set into broader authored curve evaluation, visibility toggles, gradients, and exact family playback.
