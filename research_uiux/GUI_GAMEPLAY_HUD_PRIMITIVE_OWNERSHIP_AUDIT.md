<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Gameplay HUD Primitive Ownership Audit

Phase 61 audits the gameplay HUD primitive preview against the parsed `ui_prov_playscreen.yncp` deep-analysis facts:

```text
b/rr61/sward_ui_runtime_debug_gui.exe
```

The Phase 60 aggregate primitive counts were useful, but this beat tightens the scene ownership names so the GUI no longer treats the same six proxy primitives as interchangeable boxes.

## Corrected Scene Ownership

| Scene | Frames | Cast refs | Max cast | Keyframes | Notes |
|---|---:|---:|---:|---:|---|
| `Root/so_speed_gauge` | `100` | `47` | `47` | `360` | strongest recovered gauge primitive |
| `Root/so_ringenagy_gauge` | `100` | `43` | `43` | `240` | ring-energy gauge primitive |
| `Root/info_1` | `100` | `72` | `24` | `57` | three counter/effect animation banks |
| `Root/info_2` | `100` | `72` | `24` | `9` | lower-density counter/effect bank |
| `Root/ring_get_effect` | `5` | `2` | `2` | `14` | short intro/effect primitive |
| `Root/bg` | `100` | `29` | `21` | `0` | static/default background primitive |

Sonic, Werehog, and Extra Stage HUD previews keep the same six-primitive set and still total `680` represented keyframes per contract.

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr61 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr61 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr61\sward_ui_runtime_debug_gui.exe --layout-primitive-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layout primitive smoke ok title_primitives=6 keyframes=806 pause_primitives=6 keyframes=806 loading_primitives=6 keyframes=2775 sonic_stage_primitives=6 keyframes=680 werehog_stage_primitives=6 keyframes=680 extra_stage_primitives=6 keyframes=680 sonic_speed_gauge_kf=360 sonic_ring_energy_gauge_kf=240 sonic_ring_get_effect_kf=14 sonic_bg_kf=0
```

The GUI was also launch/capture-checked through `PrintWindow` by selecting `Gameplay HUD Hosts -> SonicMainDisplay.cpp`, pressing `Run Host`, pausing playback during `Intro`, and capturing the rr61 window with the corrected `so_speed_gauge`, `so_ringenagy_gauge`, `ring_get_effect`, and `bg` primitive ownership visible over the proxy atlas.

## Boundary

This is still a proxy evidence pass for Sonic/Werehog HUDs because the exact loose `ui_playscreen*` payloads remain unrecovered. The important upgrade is that the workbench now guards the parsed scene/keyframe ownership rather than only the aggregate primitive totals.
