<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Gameplay HUD Primitive Preview

Phase 60 widens the scene-primitive renderer into the gameplay HUD proxy family. Phase 61 audits the scene ownership labels against parsed `ui_prov_playscreen` deep-analysis facts:

```text
b/rr61/sward_ui_runtime_debug_gui.exe
```

The native GUI can now draw recovered `ui_prov_playscreen` scene primitives for Sonic, Werehog, and Extra Stage HUD preview hosts.

## What Changed

- bound `sonic_stage_hud_reference.json`, `werehog_stage_hud_reference.json`, and `extra_stage_hud_reference.json` to the parsed `ui_prov_playscreen` scene primitive set
- added primitive evidence for `Root/so_speed_gauge`, `Root/so_ringenagy_gauge`, `Root/info_1`, `Root/info_2`, `Root/ring_get_effect`, and `Root/bg`
- kept the proxy boundary explicit: Sonic and Werehog still use the recovered `ui_prov_playscreen` sheet as a marked proxy until exact loose `ui_playscreen*` assets are recovered
- extended `--layout-primitive-smoke` to verify gameplay HUD primitive counts and keyframe totals
- updated GUI regression coverage to target `b/rr61/sward_ui_runtime_debug_gui.exe`

## Primitive Evidence

| Contract | Layout primitive source | Primitive count | Keyframes represented |
|---|---|---:|---:|
| `sonic_stage_hud_reference.json` | `ui_prov_playscreen` proxy | `6` | `680` |
| `werehog_stage_hud_reference.json` | `ui_prov_playscreen` proxy | `6` | `680` |
| `extra_stage_hud_reference.json` | `ui_prov_playscreen` | `6` | `680` |

The highest-density gameplay HUD primitive is `Root/so_speed_gauge` with `360` keyframes. The next useful anchors are `Root/so_ringenagy_gauge` with `240` keyframes, `Root/info_1` with `57` keyframes, and `Root/ring_get_effect` with `14` keyframes.

## Boundary

This is still a proxy primitive pass for Sonic/Werehog HUDs. It is useful because the current GUI can now show gameplay HUD scene ownership, keyframe density, and frame progress in the same visual surface as the atlas and runtime contract layers.

It is not yet exact Sonic HUD reconstruction. The remaining work is to recover the exact loose play-screen layouts, bind exact subimage regions, and sample authored channels rather than drawing diagnostic scene boxes.

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr61 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr61 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr61\sward_ui_runtime_debug_gui.exe --layout-primitive-smoke
b\rr61\sward_ui_runtime_debug_gui.exe --preview-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layout primitive smoke ok title_primitives=6 keyframes=806 pause_primitives=6 keyframes=806 loading_primitives=6 keyframes=2775 sonic_stage_primitives=6 keyframes=680 werehog_stage_primitives=6 keyframes=680 extra_stage_primitives=6 keyframes=680 sonic_speed_gauge_kf=360 sonic_ring_energy_gauge_kf=240 sonic_ring_get_effect_kf=14 sonic_bg_kf=0
sward_ui_runtime_debug_gui preview smoke ok atlas_candidates=10 proxy_candidates=2 existing_local_atlas=10 title=mainmenu__ui_mainmenu.png pause=systemcommoncore__ui_pause.png sonic_stage=exstagetails_common__ui_prov_playscreen.png
```

The GUI was also launch/capture-checked through `PrintWindow` by selecting `Gameplay HUD Hosts -> SonicMainDisplay.cpp`, pressing `Run Host`, pausing playback during `Intro`, and capturing the rr60 window with gameplay HUD primitives visible over the proxy atlas. Phase 61 keeps that visual path and tightens the primitive ownership smoke.
