<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Layout Primitive Detail Cues

Phase 63 turns the primitive playback cues from tiny overlay labels into readable operator detail text:

```text
b/rr63/sward_ui_runtime_debug_gui.exe
```

The native GUI detail pane now reports the selected host's layout primitive cue summary: primitive count, total keyframes, animation bank, sampled frame cursor, per-primitive keyframes, and recovered track summary.

## What Changed

- added `layoutPrimitiveCueSummary(...)` as the shared primitive parity text formatter
- injected `Layout primitive cues:` into the GUI detail pane for selected/running hosts
- added `--layout-primitive-detail-smoke` for headless Sonic HUD proxy parity checks
- kept the summary diagnostic: it reads recovered scene/animation/frame facts but does not claim original CSD channel evaluation yet

For `Gameplay HUD Hosts -> SonicMainDisplay.cpp`, the detail pane now includes entries like:

```text
Layout primitive cues:
  primitives=6 keyframes=680
  so_speed_gauge / Size_Anim : frame 100/100, keyframes=360, tracks=Gradient, X/Y scale
  so_ringenagy_gauge / Size_Anim : frame 100/100, keyframes=240, tracks=Gradient, X scale
  info_1 / Count_Anim : frame 100/100, keyframes=57, tracks=Gradient, HideFlag
  ring_get_effect / Intro_Anim : frame 5/5, keyframes=14, tracks=Gradient, Rotation
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr63 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr63 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr63\sward_ui_runtime_debug_gui.exe --layout-primitive-detail-smoke
b\rr63\sward_ui_runtime_debug_gui.exe --layout-primitive-playback-smoke
b\rr63\sward_ui_runtime_debug_gui.exe --layout-primitive-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layout primitive detail smoke ok primitives=6 keyframes=680 speed=so_speed_gauge/Size_Anim frame=50/100 ring_fx=ring_get_effect/Intro_Anim frame=3/5
sward_ui_runtime_debug_gui layout primitive playback smoke ok speed_anim=Size_Anim speed_frame=50/100 energy_anim=Size_Anim energy_frame=50/100 info_anim=Count_Anim info_frame=50/100 ring_fx_anim=Intro_Anim ring_fx_frame=3/5 bg_anim=DefaultAnim bg_frame=50/100
sward_ui_runtime_debug_gui layout primitive smoke ok title_primitives=6 keyframes=806 pause_primitives=6 keyframes=806 loading_primitives=6 keyframes=2775 sonic_stage_primitives=6 keyframes=680 werehog_stage_primitives=6 keyframes=680 extra_stage_primitives=6 keyframes=680 sonic_speed_gauge_kf=360 sonic_ring_energy_gauge_kf=240 sonic_ring_get_effect_kf=14 sonic_bg_kf=0
```

The GUI was launch-checked by selecting `Gameplay HUD Hosts -> SonicMainDisplay.cpp`, pressing `Run Host`, and reading back the native detail edit control with `WM_GETTEXT`. The detail text contained `Layout primitive cues:`, `so_speed_gauge / Size_Anim`, and `ring_get_effect / Intro_Anim` for the running Sonic HUD host.

## Boundary

This is a parity/readability upgrade for the workbench shell. It makes the current primitive evidence easier to inspect family-by-family, but it still sits before exact node-transform rendering and original animation-channel playback.
