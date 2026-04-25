<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Authored Sampled Draw Commands

Phase 76 turns the first authored sampled transform markers into renderer-facing draw commands:

```text
b/rr76/sward_ui_runtime_debug_gui.exe
```

Phase 75 proved the preview can draw sampled authored transform evidence. This beat gives that same evidence a draw-command descriptor surface so the GUI paint path consumes a command object instead of raw sampled-transform records.

## What Changed

- added `LayoutAuthoredSampledDrawCommand`
- added `layoutAuthoredSampledDrawCommandsForContract(...)`
- added `layoutAuthoredSampledDrawCommandDescriptor(...)`
- added `layoutAuthoredSampledDrawCommandSummary(...)`
- added an `Authored sampled draw commands:` detail-pane section
- changed authored sampled preview markers to draw from sampled draw commands
- added `--authored-sampled-draw-command-smoke`

## Verified Draw Commands

```text
title:   ui_mainmenu.xncp / mm_donut_move / index_text_pos / YPosition @ frame 30
pause:   ui_pause.yncp / bg / img / Color @ frame 7
loading: ui_loading.yncp / bg_2 / pos_text_sonic / XPosition @ frame 1
```

Descriptor shape:

```text
scene/cast:x,y,widthxheight:track@frame=value
```

Verified descriptors:

```text
mm_donut_move/index_text_pos:408,163,16x16:YPosition@30=0.225926
bg/img:0,0,1280x720:Color@7=0.000000
bg_2/pos_text_sonic:640,360,16x16:XPosition@1=0.500000
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr76 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr76 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr76\sward_ui_runtime_debug_gui.exe --authored-sampled-draw-command-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui authored sampled draw command smoke ok title_draw=mm_donut_move/index_text_pos:408,163,16x16:YPosition@30=0.225926 pause_draw=bg/img:0,0,1280x720:Color@7=0.000000 loading_draw=bg_2/pos_text_sonic:640,360,16x16:XPosition@1=0.500000
```

## Boundary

These draw commands are still a first narrow bridge, not the full renderer. They make the preview path command-driven for the first authored sampled rectangles, but broad UI/UX parity still needs more decoded node transforms, CSD channel sampling, exact HUD payloads, and PPC-backed host behavior.

Phase 77 adds the first sampled channel-state descriptor on top of this command surface, using Pause `Color@7` as the first non-position channel sample.
