<p align="right">
    <img src="../../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Runtime Reference

This directory contains the Phase 21/24/27/37/38/39/40/42/43/45/47/48/50/51/52/53/54/55/56/57/58/59/60/61/62/63/64/65/66/67/68/69/70/71/72/73/74 reusable runtime and port-kit layer for the SWARD template-pack concepts.

It is intentionally decoupled from game assets and from the asset-backed Unleashed Recompiled runtime. The goal is to provide reusable implementation layers for original projects that need:

- explicit screen states
- input lock windows
- overlay-role visibility
- prompt-row visibility rules
- timer-banded transitions
- portable reference profiles across C++, C, and C#
- portable JSON contract files instead of hardcoded in-code builders
- a standalone contract-backed debug selector for the first reusable screen-browser pass
- a source-family alias layer so the selector can launch by recovered names such as `TitleMenu.cpp`, `HudPause.cpp`, and `InspirePreview.cpp`
- a richer host-bucket debug workbench around recovered debug/menu/cutscene/gameplay-HUD/stage-test/town/camera/application-world source families
- a dedicated frontend-sequence shell family for sequence-core, unit-factory, and unlock/dispatch probing
- a Phase 47 wider source-path support layer with `159` generated workbench hosts, including `30` camera/replay presentation hosts
- a compact `--catalog` workbench view that summarizes groups, contracts, and sample launch hosts before running a specific probe
- Phase 50 support-substrate contracts for achievement unlock, audio/BGM cue, and XML/data-loading probes
- interactive selector/workbench loops plus a `--stay-open` mode so the native tools no longer look like GUI crashes when launched directly
- a native Win32 GUI workbench target for browsing groups/hosts and driving contract-backed runtime actions without CLI flags
- a GUI preview panel that can draw ignored local atlas PNGs plus runtime layer/prompt/timeline overlays, including explicit proxy atlas bindings for gameplay HUD families
- timer-driven GUI playback controls for inspecting contract intro/action/outro bands without snapping straight to the settled state
- state-aware GUI preview motion for layer/prompt offsets and alpha during contract Intro, Navigate, Confirm, Cancel, and Outro bands
- exact-family preview layout adapters for Title, Pause, and Loading contracts before decoded layout nodes are ready
- decoded layout-evidence overlays for Title, Pause, and Loading previews, with atlas-preserving structural layer fill policy
- frame-domain layout timeline readouts for the strongest parsed Title, Pause, and Loading timelines
- scene-primitive preview overlays for the highest keyframe-density parsed Title, Pause, and Loading scenes
- gameplay HUD primitive preview overlays for Sonic, Werehog, and Extra Stage hosts via the recovered `ui_prov_playscreen` proxy path
- smoke-guarded gameplay HUD primitive scene ownership for the current `ui_prov_playscreen` proxy set
- primitive animation-bank labels and sampled frame cursors for the current diagnostic scene primitive layer
- readable GUI detail-pane primitive parity summaries for host-by-host inspection
- primitive channel-classification tags for recovered transform/color/visibility/static track families
- compact primitive channel-count legends in the GUI preview surface
- host-level visual parity summaries that join atlas exact/proxy state, layout evidence, primitive coverage, keyframe totals, and channel totals
- host-list readiness badges for exact/proxy/layout/primitive/channel/contract triage before launching a host
- next-renderer blocker cues inside the visual parity detail section for exact loose HUD payload, decoded CSD channel sampling, primitive transform sampling, layout-node transform decoding, and visual evidence binding triage
- exact-family primitive channel sample tokens that bind recovered channel tags to sampled frame cursors in the GUI detail path
- exact-family draw command descriptors that bind recovered primitive geometry to sampled channel tokens
- exact-family authored CSD cast transform descriptors from parsed `layout_deep_analysis.json` evidence
- exact-family authored CSD keyframe curve descriptors from parsed `layout_deep_analysis.json` evidence
- exact-family authored CSD keyframe sample descriptors with deterministic sampled values
- exact-family authored sampled transform descriptors that pair cast rectangles with sampled X/Y keyframes

Contents:

- `include/sward/ui_runtime/runtime.hpp`
- `include/sward/ui_runtime/profiles.hpp`
- `include/sward/ui_runtime/contract_loader.hpp`
- `include/sward/ui_runtime/runtime_c.h`
- `contracts/`
- `src/runtime.cpp`
- `src/contract_loader.cpp`
- `src/profiles.cpp`
- `src/runtime_c.cpp`
- `examples/pause_menu_example.cpp`
- `examples/title_menu_example.cpp`
- `examples/toast_overlay_example.cpp`
- `examples/c_pause_menu_example.c`
- `examples/ui_debug_workbench_gui.cpp`
- `CMakeLists.txt`
- `csharp_reference/`

Bundled contract files:

- `contracts/pause_menu_reference.json`
- `contracts/title_menu_reference.json`
- `contracts/autosave_toast_reference.json`
- `contracts/loading_transition_reference.json`
- `contracts/mission_result_reference.json`
- `contracts/sonic_stage_hud_reference.json`
- `contracts/werehog_stage_hud_reference.json`
- `contracts/extra_stage_hud_reference.json`
- `contracts/super_sonic_hud_reference.json`
- `contracts/boss_hud_reference.json`
- `contracts/subtitle_cutscene_reference.json`
- `contracts/town_ui_reference.json`
- `contracts/camera_shell_reference.json`
- `contracts/application_world_shell_reference.json`
- `contracts/frontend_sequence_shell_reference.json`
- `contracts/achievement_unlock_support_reference.json`
- `contracts/audio_cue_support_reference.json`
- `contracts/xml_data_loading_support_reference.json`
- `contracts/world_map_reference.json`

Bundled reference profiles:

- `PauseMenu`
- `TitleMenu`
- `AutosaveToast`
- `LoadingTransition`
- `MissionResult`
- `SonicStageHud`
- `WerehogStageHud`
- `ExtraStageHud`
- `SuperSonicHud`
- `BossHud`
- `SubtitleCutscene`
- `TownUi`
- `CameraShell`
- `ApplicationWorldShell`
- `FrontendSequenceShell`
- `AchievementUnlockSupport`
- `AudioCueSupport`
- `XmlDataLoadingSupport`
- `WorldMap`

Build the native layer locally:

```powershell
cmd /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && "C:\Program Files\CMake\bin\cmake.exe" -S research_uiux/runtime_reference -B b/rr74 -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release && "C:\Program Files\CMake\bin\cmake.exe" --build b/rr74 --config Release'
b/rr74/sward_ui_runtime_example.exe
b/rr74/sward_ui_runtime_title_menu_example.exe
b/rr74/sward_ui_runtime_toast_example.exe
b/rr74/sward_ui_runtime_c_example.exe
b/rr74/sward_ui_runtime_debug_selector.exe
b/rr74/sward_ui_runtime_debug_workbench.exe
b/rr74/sward_ui_runtime_debug_gui.exe
```

Run against the bundled contracts:

```powershell
b/rr74/sward_ui_runtime_example.exe
b/rr74/sward_ui_runtime_title_menu_example.exe
b/rr74/sward_ui_runtime_toast_example.exe
b/rr74/sward_ui_runtime_c_example.exe
b/rr74/sward_ui_runtime_debug_selector.exe --list
b/rr74/sward_ui_runtime_debug_selector.exe --list-families
b/rr74/sward_ui_runtime_debug_selector.exe TitleMenu.cpp
b/rr74/sward_ui_runtime_debug_selector.exe TownManager.cpp
b/rr74/sward_ui_runtime_debug_selector.exe FreeCamera.cpp
b/rr74/sward_ui_runtime_debug_selector.exe Player3DBossCamera.cpp
b/rr74/sward_ui_runtime_debug_selector.exe Application.cpp
b/rr74/sward_ui_runtime_debug_selector.exe SequenceManagerImpl.cpp
b/rr74/sward_ui_runtime_debug_selector.exe AchievementManager.cpp
b/rr74/sward_ui_runtime_debug_selector.exe SoundController.cpp
b/rr74/sward_ui_runtime_debug_selector.exe XMLManager.cpp
b/rr74/sward_ui_runtime_debug_selector.exe --stay-open TitleManager.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --list-groups
b/rr74/sward_ui_runtime_debug_workbench.exe --catalog
b/rr74/sward_ui_runtime_debug_workbench.exe --host GameModeMenuSelectDebug.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --host InspirePreview.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --host HudSonicStage.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --host TownManager.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --host Player3DBossCamera.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --host TitleManager.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --host WorldMapSelect.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --host SequenceManagerImpl.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --host AchievementManager.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --host SoundController.cpp
b/rr74/sward_ui_runtime_debug_workbench.exe --host XMLManager.cpp
b/rr74/sward_ui_runtime_debug_gui.exe
b/rr74/sward_ui_runtime_debug_gui.exe --smoke
b/rr74/sward_ui_runtime_debug_gui.exe --preview-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --playback-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --motion-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --family-preview-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --layout-evidence-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --layout-timeline-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --layout-primitive-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --layout-primitive-playback-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --layout-primitive-detail-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --layout-primitive-channel-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --layout-primitive-channel-legend-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --visual-parity-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --host-readiness-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --renderer-blocker-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --layout-channel-sample-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --layout-draw-command-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --authored-cast-transform-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --authored-keyframe-curve-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --authored-keyframe-sample-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --authored-sampled-transform-smoke
b/rr74/sward_ui_runtime_debug_gui.exe --layer-fill-smoke
```

Run against an explicit portable contract path:

```powershell
b/rr74/sward_ui_runtime_example.exe research_uiux/runtime_reference/contracts/world_map_reference.json
b/rr74/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/mission_result_reference.json
b/rr74/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/subtitle_cutscene_reference.json
b/rr74/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/sonic_stage_hud_reference.json
b/rr74/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/application_world_shell_reference.json
b/rr74/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/frontend_sequence_shell_reference.json
b/rr74/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/audio_cue_support_reference.json
```

Build the managed port locally:

```powershell
external_tools/dotnet8/dotnet.exe build research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release
external_tools/dotnet8/dotnet.exe run --project research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release
```

Selector notes:

- `--list` prints all bundled contract-backed screens
- `--list-families` prints the generated source-family launch groups
- `--index <n>` selects by 1-based slot
- `<token>` matches either a source-family alias or a bundled contract token
- `--family <token>` forces a source-family lookup
- `--path <contract.json>` loads an explicit portable contract file
- `--stay-open` waits for Enter before exit when you launch a direct one-shot command
- no-argument interactive mode now returns to the menu after a selection instead of exiting immediately

Workbench notes:

- `--list-groups` prints the richer host-bucket groups derived from the frontend shell/debug and gameplay-HUD recovery layers
- `--catalog` prints a compact group/contract/sample-host overview of the current workbench surface
- `--group <token>` lists the launchable hosts inside one workbench group
- `--host <token>` launches by recovered host name or source path such as `GameModeMenuSelectDebug.cpp` or `InspirePreview.cpp`
- `--stay-open` waits for Enter before exit when you launch a direct host/command
- the current workbench now covers menu-debug, stage-debug, cutscene-preview, gameplay-HUD, boss/final HUD, town/media-room, camera/replay, frontend-sequence, application/world shell, support-substrate, and stage-test ownership
- the Phase 50 workbench map contains `176` host entries across `11` groups
- no-argument interactive mode now returns to the group menu after each launch instead of exiting immediately

GUI workbench notes:

- `sward_ui_runtime_debug_gui.exe` opens a native Windows operator shell over the same `176` workbench hosts and `19` bundled contracts
- the group and host panes browse recovered source-family ownership without requiring CLI tokens
- `Run Host`, `Move Next`, `Confirm`, `Cancel`, and `Reset` drive the selected contract through `ScreenRuntime`
- the detail/log panes expose state, prompts, overlay roles, source path, contract, and callback traces
- `--smoke` verifies catalog resolution without opening the GUI window
- the preview panel draws an ignored local atlas sheet when a contract has a high-confidence atlas candidate, then overlays runtime layers, prompts, and a timeline strip
- `--preview-smoke` verifies those atlas bindings without opening the GUI window; Phase 53 reports `10` atlas candidates and `2` proxy gameplay-HUD bindings
- `Run Host` now starts timer-driven contract playback, while `Play`/`Pause` and `Step` allow deterministic timeline inspection in the window
- `--playback-smoke` verifies that the same tick path advances Intro -> Idle and Navigate -> Idle without opening the GUI window
- preview layers and prompt buttons now apply state-aware motion/alpha during active contract bands
- `--motion-smoke` verifies the eased preview-motion adapter without opening the GUI window
- Title, Pause, and Loading contracts now use exact-family preview layout adapters before falling back to the generic role projection
- `--family-preview-smoke` verifies those exact-family layout adapters without opening the GUI window
- Title, Pause, and Loading previews now show compact parsed-layout evidence panels tied to `ui_mainmenu`, `ui_pause`, and `ui_loading`
- `--layout-evidence-smoke` verifies those recovered layout facts without opening the GUI window
- the same evidence panel now projects runtime progress into the recovered longest-timeline frame domains
- `--layout-timeline-smoke` verifies that frame-domain mapping without opening the GUI window
- the preview now draws scene primitives from parsed scene-path and keyframe-density evidence for Title, Pause, and Loading
- `--layout-primitive-smoke` verifies those primitive counts and keyframe totals without opening the GUI window
- Sonic, Werehog, and Extra Stage HUD previews now draw the recovered `ui_prov_playscreen` scene primitive set while keeping Sonic/Werehog marked as proxy atlas bindings
- `--layout-primitive-smoke` also verifies gameplay HUD scene/keyframe ownership for `so_speed_gauge`, `so_ringenagy_gauge`, `ring_get_effect`, and `bg`
- primitive overlays now include recovered animation-bank names and sampled frame cursors
- `--layout-primitive-playback-smoke` verifies gameplay HUD primitive animation/frame cues without opening the GUI window
- the detail pane now includes a readable `Layout primitive cues:` parity section for hosts with primitive evidence
- `--layout-primitive-detail-smoke` verifies that detail summary without opening the GUI window
- primitive cue paths now classify recovered track summaries into color, sprite, transform, visibility, and static channel tags
- `--layout-primitive-channel-smoke` verifies those Sonic HUD proxy channel tags without opening the GUI window
- the preview now includes a compact primitive channel legend, e.g. `Channels T3 C4 V2 S0 static1`
- `--layout-primitive-channel-legend-smoke` verifies that legend without opening the GUI window
- the detail pane now includes a `Visual parity:` summary for atlas exact/proxy state, layout evidence, primitive coverage, keyframes, and channel totals
- `--visual-parity-smoke` verifies exact Title vs proxy Sonic HUD readiness without opening the GUI window
- host listbox labels now include compact readiness badges such as `[proxy primitive channels]`, `[exact layout primitive channels]`, and `[contract]`
- `--host-readiness-smoke` verifies those browse-time labels without opening the GUI window
- the `Visual parity:` summary now includes `next_renderer=...` blocker cues for the next visual reconstruction step per host
- `--renderer-blocker-smoke` verifies Sonic HUD, Title, and support-substrate blocker classification without opening the GUI window
- the detail pane now includes `Layout primitive channel samples:` tokens in the form `scene:channels@frame/count`
- `--layout-channel-sample-smoke` verifies Title, Pause, and Loading exact-family primitive channel samples without opening the GUI window
- the detail pane now includes `Layout primitive draw commands:` descriptors in the form `scene:x,y,widthxheight:channels@frame/count`
- `--layout-draw-command-smoke` verifies Title, Pause, and Loading exact-family primitive draw command descriptors without opening the GUI window
- the detail pane now includes `Authored cast transforms:` descriptors from parsed CSD cast data
- `--authored-cast-transform-smoke` verifies Title, Pause, and Loading exact-family authored cast transforms without opening the GUI window
- the detail pane now includes `Authored keyframe curves:` descriptors from parsed CSD animation data
- `--authored-keyframe-curve-smoke` verifies Title, Pause, and Loading exact-family authored keyframe curves without opening the GUI window
- the detail pane now includes `Authored keyframe samples:` descriptors from deterministic sampled CSD keyframes
- `--authored-keyframe-sample-smoke` verifies Title, Pause, and Loading exact-family authored keyframe sample values without opening the GUI window
- the detail pane now includes `Authored sampled transforms:` descriptors that combine authored cast rectangles with sampled X/Y keyframes
- `--authored-sampled-transform-smoke` verifies Title and Loading exact-family sampled transform descriptors without opening the GUI window
- `--layer-fill-smoke` verifies that backdrop and cinematic-frame overlays preserve the atlas underneath
