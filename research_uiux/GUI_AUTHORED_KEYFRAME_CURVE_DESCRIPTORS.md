<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Authored Keyframe Curve Descriptors

Phase 72 adds the first authored CSD keyframe curve descriptor layer to the native GUI:

```text
b/rr72/sward_ui_runtime_debug_gui.exe
```

Phase 71 surfaced parsed authored cast transforms. This beat binds the next evidence layer into the same detail path: representative exact-family animation tracks from `research_uiux/data/layout_deep_analysis.json`, expressed as deterministic first/last keyframe descriptors.

## What Changed

- added `LayoutAuthoredKeyframeCurve`
- added `layoutAuthoredKeyframeCurvesForContract(...)`
- added `layoutAuthoredKeyframeCurveDescriptor(...)`
- added an `Authored keyframe curves:` detail-pane section
- added `--authored-keyframe-curve-smoke`

## Verified Curves

```text
title:   ui_mainmenu.xncp / mm_donut_move / intro / index_text_pos / YPosition
pause:   ui_pause.yncp / bg / Intro_Anim / img / Color
loading: ui_loading.yncp / bg_2 / 360_sonic1 / pos_text_sonic / XPosition
```

Descriptor shape:

```text
scene/animation/cast/track:kfN:firstFrame=firstValue->lastFrame=lastValue:interpolation
```

Verified descriptors:

```text
mm_donut_move/intro/index_text_pos/YPosition:kf5:0=0.411111->40=0.188889:Linear
bg/Intro_Anim/img/Color:kf2:0=0.000000->15=0.000000:Linear
bg_2/360_sonic1/pos_text_sonic/XPosition:kf1:0=0.500000->0=0.500000:Linear
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr72 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr72 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr72\sward_ui_runtime_debug_gui.exe --authored-keyframe-curve-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui authored keyframe curve smoke ok title_curve=mm_donut_move/intro/index_text_pos/YPosition:kf5:0=0.411111->40=0.188889:Linear pause_curve=bg/Intro_Anim/img/Color:kf2:0=0.000000->15=0.000000:Linear loading_curve=bg_2/360_sonic1/pos_text_sonic/XPosition:kf1:0=0.500000->0=0.500000:Linear
```

## Boundary

These descriptors do not yet evaluate or interpolate full CSD animation curves. They establish a stable GUI-visible bridge from parsed authored animation tracks to renderer/runtime parity work.
