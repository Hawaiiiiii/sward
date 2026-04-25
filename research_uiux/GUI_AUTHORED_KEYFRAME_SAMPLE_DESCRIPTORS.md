<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Authored Keyframe Sample Descriptors

Phase 73 adds the first authored CSD keyframe sample descriptor layer to the native GUI:

```text
b/rr73/sward_ui_runtime_debug_gui.exe
```

Phase 72 exposed first/last keyframe curve descriptors. This beat stores the bounded keyframe points for the same exact-family samples and adds deterministic linear sampling so the GUI can report a value at an inspected frame. Phase 74 combines those sampled values with authored cast rectangles as renderer-facing sampled transform descriptors.

## What Changed

- added `LayoutAuthoredKeyframeSample`
- added `layoutAuthoredKeyframeSamplesForContract(...)`
- added `layoutAuthoredKeyframeCurveValueAtFrame(...)`
- added `layoutAuthoredKeyframeSampleDescriptor(...)`
- added an `Authored keyframe samples:` detail-pane section
- added `--authored-keyframe-sample-smoke`

## Verified Samples

```text
title:   ui_mainmenu.xncp / mm_donut_move / intro / index_text_pos / YPosition @ frame 30
pause:   ui_pause.yncp / bg / Intro_Anim / img / Color @ frame 7
loading: ui_loading.yncp / bg_2 / 360_sonic1 / pos_text_sonic / XPosition @ frame 1
```

Descriptor shape:

```text
scene/animation/cast/track@frame=value
```

Verified descriptors:

```text
mm_donut_move/intro/index_text_pos/YPosition@30=0.225926
bg/Intro_Anim/img/Color@7=0.000000
bg_2/360_sonic1/pos_text_sonic/XPosition@1=0.500000
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr73 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr73 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr73\sward_ui_runtime_debug_gui.exe --authored-keyframe-sample-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui authored keyframe sample smoke ok title_sample=mm_donut_move/intro/index_text_pos/YPosition@30=0.225926 pause_sample=bg/Intro_Anim/img/Color@7=0.000000 loading_sample=bg_2/360_sonic1/pos_text_sonic/XPosition@1=0.500000
```

## Boundary

This is a deterministic diagnostic sampler, not the final CSD animation player. It evaluates the curated exact-family keyframe points now visible in the GUI, but the next renderer step is still to drive draw inputs from decoded transforms/channels across full family timelines.
