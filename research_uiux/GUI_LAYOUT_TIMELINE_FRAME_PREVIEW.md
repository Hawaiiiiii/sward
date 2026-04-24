<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Layout Timeline Frame Preview

Phase 58 extends the Phase 57 layout-evidence overlay with recovered frame-domain playback facts:

```text
b/rr58/sward_ui_runtime_debug_gui.exe
```

The native GUI preview now shows the current projected frame inside the longest parsed layout timeline for Title, Pause, and Loading evidence families.

## What Changed

- added `longestTimelineFrames` and `framesPerSecond` to each compact `LayoutEvidence` entry
- added `layoutTimelineFrame(...)` to map the active runtime timeline progress into parsed layout frame space
- added `drawLayoutTimelineBar(...)` so the evidence panel carries a visual progress strip in addition to text
- changed the layout evidence overlay from static facts only to `Frame: current/total @ fps`
- added `--layout-timeline-smoke` so automation can verify the frame-domain mapping without opening a window
- updated GUI regression coverage to target `b/rr58/sward_ui_runtime_debug_gui.exe`

## Current Frame-Domain Anchors

| Contract | Layout | Longest parsed timeline | Frame domain |
|---|---|---|---:|
| `title_menu_reference.json` | `ui_mainmenu` | `mm_donut_move/DefaultAnim` | `220f @ 60fps` |
| `pause_menu_reference.json` | `ui_pause` | `btn_effect/charge_3_Outro` | `240f @ 60fps` |
| `loading_transition_reference.json` | `ui_loading` | `pda_txt/Usual_Anim_3` | `240f @ 60fps` |

## Boundary

This is not original CSD keyframe playback yet. The runtime still advances portable SWARD contract states, then projects that active state progress into the recovered frame domain for the strongest parsed layout timelines.

That is still a meaningful step: the GUI now speaks both the contract-state language and the parsed layout frame language in one surface. The next renderer work can replace schematic role rectangles with decoded node transforms and animation-channel sampling family by family.

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr58 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr58 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr58\sward_ui_runtime_debug_gui.exe --layout-timeline-smoke
b\rr58\sward_ui_runtime_debug_gui.exe --layout-evidence-smoke
b\rr58\sward_ui_runtime_debug_gui.exe --layer-fill-smoke
b\rr58\sward_ui_runtime_debug_gui.exe --family-preview-smoke
b\rr58\sward_ui_runtime_debug_gui.exe --motion-smoke
b\rr58\sward_ui_runtime_debug_gui.exe --playback-smoke
b\rr58\sward_ui_runtime_debug_gui.exe --preview-smoke
b\rr58\sward_ui_runtime_debug_gui.exe --smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layout timeline smoke ok title_frame=110/220 pause_frame=120/240 loading_frame=180/240 fps=60
sward_ui_runtime_debug_gui layout evidence smoke ok title=ui_mainmenu scenes=16 animations=6 pause=ui_pause scenes=29 animations=41 loading=ui_loading scenes=7 animations=37
sward_ui_runtime_debug_gui layer fill smoke ok backdrop_alpha=0 cinematic_alpha=0 content_alpha=0.58
sward_ui_runtime_debug_gui family preview smoke ok title=title_menu pause=pause_menu loading=loading_transition title_logo_y=64.8 title_content_y=331.2 pause_chrome_w=844.8 loading_frame_y=187.2
sward_ui_runtime_debug_gui motion smoke ok intro_alpha=0.28 intro_offset_x=-204.8 intro_end_alpha=1 idle_alpha=1 outro_alpha=0.32
sward_ui_runtime_debug_gui playback smoke ok intro=Intro after_intro=Idle action=Navigate after_action=Idle
sward_ui_runtime_debug_gui preview smoke ok atlas_candidates=10 proxy_candidates=2 existing_local_atlas=10 title=mainmenu__ui_mainmenu.png pause=systemcommoncore__ui_pause.png sonic_stage=exstagetails_common__ui_prov_playscreen.png
sward_ui_runtime_debug_gui smoke ok contracts=19 hosts=176 groups=11 support_hosts=17
```

The GUI was also launch/capture-checked through `PrintWindow` by selecting `Menu / Stage Debug Hosts -> GameModeMainMenu_Test.cpp`, pressing `Run Host`, pausing playback during `Intro`, and capturing the rr58 window with the `ui_mainmenu` evidence panel showing a live `Frame: 98/220 @ 60fps` timeline readout.

Phase 59 builds directly on this in [`GUI_LAYOUT_SCENE_PRIMITIVE_PREVIEW.md`](./GUI_LAYOUT_SCENE_PRIMITIVE_PREVIEW.md), drawing keyframe-density scene primitives over the same atlas preview.
