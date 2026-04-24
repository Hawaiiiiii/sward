<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Layout Scene Primitive Preview

Phase 59 adds the first scene-graph primitive draw pass to the native GUI workbench:

```text
b/rr59/sward_ui_runtime_debug_gui.exe
```

The preview can now draw compact scene primitives from parsed layout evidence, not only generic contract-role rectangles.

## What Changed

- added `LayoutScenePrimitive` entries for the six highest keyframe-density scenes in each of the current high-confidence visual families
- added `layoutScenePrimitivesForContract(...)` and `layoutScenePrimitiveKeyframeTotal(...)`
- added `drawLayoutScenePrimitives(...)` to paint evidence-backed scene boxes over the atlas preview
- each primitive carries scene path, scene name, track summary, frame count, group count, cast-reference count, max cast count, and keyframe count
- each primitive draws its own projected frame progress bar using the same runtime-to-layout timeline progress used by the Phase 58 evidence overlay
- added `--layout-primitive-smoke` so automation can verify primitive counts and keyframe totals without opening the GUI window

## Primitive Evidence

The current primitive pass is intentionally narrow and high-signal:

| Contract | Layout | Primitive count | Keyframes represented | Highest-density primitive |
|---|---|---:|---:|---|
| `title_menu_reference.json` | `ui_mainmenu` | `6` | `806` | `Root/mm_donut_move` (`462` keyframes) |
| `pause_menu_reference.json` | `ui_pause` | `6` | `806` | `Root/window_1/item/stick` (`571` keyframes) |
| `loading_transition_reference.json` | `ui_loading` | `6` | `2775` | `Root/bg_2` (`1378` keyframes) |

These values come from the parsed `.xncp` / `.yncp` scene graph and animation-keyframe summaries in [`research_uiux/data/layout_deep_analysis.json`](./data/layout_deep_analysis.json).

## Boundary

This is still a diagnostic primitive renderer, not final CSD node transform playback.

What it does now:

- draws real recovered scene names and scene-path ownership
- uses recovered keyframe density to size the diagnostic emphasis
- projects runtime progress into each primitive's parsed frame count
- keeps atlas, runtime layers, evidence text, and scene primitives visible in one operator surface

What remains:

- decode exact node transforms and authored coordinates into draw commands
- sample animation channels instead of projecting one runtime progress value across all primitives
- bind exact subimage regions rather than drawing scene boxes over the full atlas
- expand beyond Title/Pause/Loading once the next families have enough decoded scene evidence

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr59 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr59 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr59\sward_ui_runtime_debug_gui.exe --layout-primitive-smoke
b\rr59\sward_ui_runtime_debug_gui.exe --layout-timeline-smoke
b\rr59\sward_ui_runtime_debug_gui.exe --layout-evidence-smoke
b\rr59\sward_ui_runtime_debug_gui.exe --layer-fill-smoke
b\rr59\sward_ui_runtime_debug_gui.exe --family-preview-smoke
b\rr59\sward_ui_runtime_debug_gui.exe --motion-smoke
b\rr59\sward_ui_runtime_debug_gui.exe --playback-smoke
b\rr59\sward_ui_runtime_debug_gui.exe --preview-smoke
b\rr59\sward_ui_runtime_debug_gui.exe --smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layout primitive smoke ok title_primitives=6 keyframes=806 pause_primitives=6 keyframes=806 loading_primitives=6 keyframes=2775
sward_ui_runtime_debug_gui layout timeline smoke ok title_frame=110/220 pause_frame=120/240 loading_frame=180/240 fps=60
sward_ui_runtime_debug_gui layout evidence smoke ok title=ui_mainmenu scenes=16 animations=6 pause=ui_pause scenes=29 animations=41 loading=ui_loading scenes=7 animations=37
sward_ui_runtime_debug_gui layer fill smoke ok backdrop_alpha=0 cinematic_alpha=0 content_alpha=0.58
sward_ui_runtime_debug_gui family preview smoke ok title=title_menu pause=pause_menu loading=loading_transition title_logo_y=64.8 title_content_y=331.2 pause_chrome_w=844.8 loading_frame_y=187.2
sward_ui_runtime_debug_gui motion smoke ok intro_alpha=0.28 intro_offset_x=-204.8 intro_end_alpha=1 idle_alpha=1 outro_alpha=0.32
sward_ui_runtime_debug_gui playback smoke ok intro=Intro after_intro=Idle action=Navigate after_action=Idle
sward_ui_runtime_debug_gui preview smoke ok atlas_candidates=10 proxy_candidates=2 existing_local_atlas=10 title=mainmenu__ui_mainmenu.png pause=systemcommoncore__ui_pause.png sonic_stage=exstagetails_common__ui_prov_playscreen.png
sward_ui_runtime_debug_gui smoke ok contracts=19 hosts=176 groups=11 support_hosts=17
```

The GUI was also launch/capture-checked through `PrintWindow` by selecting `Menu / Stage Debug Hosts -> GameModeMainMenu_Test.cpp`, pressing `Run Host`, pausing playback during `Intro`, and capturing the rr59 window with `mm_donut_move` and `mm_contentsitem_select` primitive overlays visible over the title atlas.
