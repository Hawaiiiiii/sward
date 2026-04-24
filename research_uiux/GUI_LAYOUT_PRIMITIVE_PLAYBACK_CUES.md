<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Layout Primitive Playback Cues

Phase 62 adds the first animation-bank and frame-cursor cues to the native GUI scene primitive overlay:

```text
b/rr62/sward_ui_runtime_debug_gui.exe
```

This is still diagnostic playback, not original CSD channel sampling. The important step is that the primitive layer now carries recovered animation names next to scene names and keyframe density, then samples a deterministic frame cursor from the active runtime progress.

## What Changed

- extended `LayoutScenePrimitive` with a recovered animation-bank name
- labeled primitive boxes as `scene / animation` in the visual preview
- added per-primitive frame cursors such as `f=50/100` to the overlay text
- exposed the custom preview panel paint path through `WM_PRINTCLIENT` / `WM_PRINT` for more reliable visual automation captures
- attached gameplay HUD proxy primitives to recovered bank names:
  - `so_speed_gauge` -> `Size_Anim`
  - `so_ringenagy_gauge` -> `Size_Anim`
  - `info_1` -> `Count_Anim`
  - `info_2` -> `Count_Anim`
  - `ring_get_effect` -> `Intro_Anim`
  - `bg` -> `DefaultAnim`
- added `--layout-primitive-playback-smoke` so automation can verify those cues without opening the GUI window

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr62 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr62 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr62\sward_ui_runtime_debug_gui.exe --layout-primitive-playback-smoke
b\rr62\sward_ui_runtime_debug_gui.exe --layout-primitive-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layout primitive playback smoke ok speed_anim=Size_Anim speed_frame=50/100 energy_anim=Size_Anim energy_frame=50/100 info_anim=Count_Anim info_frame=50/100 ring_fx_anim=Intro_Anim ring_fx_frame=3/5 bg_anim=DefaultAnim bg_frame=50/100
sward_ui_runtime_debug_gui layout primitive smoke ok title_primitives=6 keyframes=806 pause_primitives=6 keyframes=806 loading_primitives=6 keyframes=2775 sonic_stage_primitives=6 keyframes=680 werehog_stage_primitives=6 keyframes=680 extra_stage_primitives=6 keyframes=680 sonic_speed_gauge_kf=360 sonic_ring_energy_gauge_kf=240 sonic_ring_get_effect_kf=14 sonic_bg_kf=0
```

The GUI was also launch/capture-checked through `PrintWindow` by selecting `Gameplay HUD Hosts -> SonicMainDisplay.cpp`, pressing `Run Host`, and capturing the rr62 preview panel with `so_speed_gauge / Size_Anim`, `so_ringenagy_gauge / Size_Anim`, `ring_get_effect / Intro_Anim`, and `bg / DefaultAnim` cues visible over the proxy atlas.

## Boundary

The new frame cursors are sampled from the existing runtime progress and recovered primitive frame domains. They do not yet evaluate original animation channels, interpolation modes, or node transforms. This beat gives the workbench a sharper diagnostic bridge toward CSD/timeline playback while keeping the proxy boundary explicit.
