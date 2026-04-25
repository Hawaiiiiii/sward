<p align="center">
    <img src="./docs/assets/branding/icon_sward.png" width="132" alt="SWARD icon"/>
</p>

# <img src="./docs/assets/branding/icon_sward.png" width="36" alt="SWARD icon"/> Changelog

Project history for **Project Sonic World Adventure R&D / SWARD**.

> [!NOTE]
> This changelog tracks the publishable repo layer. Local-only extracted assets, generated PPC output, staged tools, and private inputs stay outside git history by design.

## 2026-04-25

### Phase 67 GUI host readiness badges

- added [`research_uiux/GUI_HOST_READINESS_BADGES.md`](./research_uiux/GUI_HOST_READINESS_BADGES.md)
- added compact exact/proxy/layout/primitive/channel readiness badges to GUI host-list labels
- added `--host-readiness-smoke`, verifying Sonic HUD proxy, Title exact-layout, and support contract-only labels under `b/rr67`
- GUI-control-checked the native host listbox labels for `SonicMainDisplay.cpp` and `GameModeMainMenu_Test.cpp`

### Phase 66 GUI visual parity summary

- added [`research_uiux/GUI_VISUAL_PARITY_SUMMARY.md`](./research_uiux/GUI_VISUAL_PARITY_SUMMARY.md)
- added a `Visual parity` detail-pane summary for selected hosts
- surfaced exact/proxy/none atlas binding, layout evidence, primitive counts, keyframe totals, and primitive channel totals in one operator readout
- added `--visual-parity-smoke`, verifying exact Title vs proxy Sonic HUD readiness under `b/rr66`

### Phase 65 GUI layout primitive channel legend

- added [`research_uiux/GUI_LAYOUT_PRIMITIVE_CHANNEL_LEGEND.md`](./research_uiux/GUI_LAYOUT_PRIMITIVE_CHANNEL_LEGEND.md)
- added a compact preview legend for recovered primitive channel counts
- surfaced transform/color/visibility/sprite/static counts directly in the visual preview surface
- added `--layout-primitive-channel-legend-smoke`, verifying Sonic HUD proxy channel totals under `b/rr65`

### Phase 64 GUI layout primitive channel cues

- added [`research_uiux/GUI_LAYOUT_PRIMITIVE_CHANNEL_CUES.md`](./research_uiux/GUI_LAYOUT_PRIMITIVE_CHANNEL_CUES.md)
- added a conservative primitive channel classifier over recovered track summaries: color, sprite, transform, visibility, and static
- surfaced channel tags in the primitive overlay and detail-pane cue summary
- added `--layout-primitive-channel-smoke`, verifying Sonic HUD proxy channel counts and tags under `b/rr64`

### Phase 63 GUI layout primitive detail cues

- added [`research_uiux/GUI_LAYOUT_PRIMITIVE_DETAIL_CUES.md`](./research_uiux/GUI_LAYOUT_PRIMITIVE_DETAIL_CUES.md)
- surfaced layout primitive parity data in the native GUI detail pane, including primitive count, total keyframes, animation bank, sampled frame cursor, and track summary
- added `--layout-primitive-detail-smoke` for headless Sonic HUD primitive parity checks
- verified `b/rr63/sward_ui_runtime_debug_gui.exe`

### Phase 62 GUI layout primitive playback cues

- added [`research_uiux/GUI_LAYOUT_PRIMITIVE_PLAYBACK_CUES.md`](./research_uiux/GUI_LAYOUT_PRIMITIVE_PLAYBACK_CUES.md)
- attached recovered animation-bank names to the layout scene primitive table, including the Sonic/Werehog/Extra Stage HUD proxy primitive set
- added per-primitive sampled frame cursors to the native GUI primitive overlay so scene/keyframe boxes now expose the active diagnostic playback frame
- made the custom preview panel respond to `WM_PRINTCLIENT` / `WM_PRINT` so visual automation can capture the same paint path used on screen
- added `--layout-primitive-playback-smoke` to verify gameplay-HUD proxy animation names and sampled frame cursors without opening the window
- verified `b/rr62/sward_ui_runtime_debug_gui.exe`

## 2026-04-24

### Phase 61 gameplay HUD primitive ownership audit

- added [`research_uiux/GUI_GAMEPLAY_HUD_PRIMITIVE_OWNERSHIP_AUDIT.md`](./research_uiux/GUI_GAMEPLAY_HUD_PRIMITIVE_OWNERSHIP_AUDIT.md)
- corrected the gameplay HUD primitive table against `layout_deep_analysis.json` so `so_speed_gauge`, `so_ringenagy_gauge`, `info_1`, `info_2`, `ring_get_effect`, and `bg` carry their parsed scene ownership/keyframe weights
- extended `--layout-primitive-smoke` with Sonic HUD scene ownership metrics for the highest-risk proxy primitives
- verified `b/rr61/sward_ui_runtime_debug_gui.exe`

### Phase 60 GUI gameplay HUD primitive preview

- added [`research_uiux/GUI_GAMEPLAY_HUD_PRIMITIVE_PREVIEW.md`](./research_uiux/GUI_GAMEPLAY_HUD_PRIMITIVE_PREVIEW.md)
- bound Sonic, Werehog, and Extra Stage HUD preview contracts to the recovered `ui_prov_playscreen` scene primitive set
- extended `--layout-primitive-smoke` to verify gameplay HUD proxy primitive counts and keyframe totals
- verified `b/rr60/sward_ui_runtime_debug_gui.exe`
- capture-checked `Gameplay HUD Hosts -> SonicMainDisplay.cpp` through `PrintWindow`, with `bg`, `ring_get_effect`, `so_speed_gauge`, and `so_ringenagy_gauge` primitives visible over the proxy HUD atlas

### Phase 59 GUI layout scene primitive preview

- added [`research_uiux/GUI_LAYOUT_SCENE_PRIMITIVE_PREVIEW.md`](./research_uiux/GUI_LAYOUT_SCENE_PRIMITIVE_PREVIEW.md)
- added a compact `LayoutScenePrimitive` table for the highest keyframe-density Title, Pause, and Loading scenes
- added a scene-primitive draw pass over the native GUI atlas preview, including scene names, keyframe counts, and per-primitive frame progress bars
- added `--layout-primitive-smoke` regression coverage and verified `b/rr59/sward_ui_runtime_debug_gui.exe`
- capture-checked `GameModeMainMenu_Test.cpp` through `PrintWindow` with `mm_donut_move` and `mm_contentsitem_select` primitive overlays visible over the title atlas

### Phase 58 GUI layout timeline frame preview

- added [`research_uiux/GUI_LAYOUT_TIMELINE_FRAME_PREVIEW.md`](./research_uiux/GUI_LAYOUT_TIMELINE_FRAME_PREVIEW.md)
- extended `LayoutEvidence` with recovered longest-timeline frame counts and FPS for Title, Pause, and Loading previews
- added frame-domain mapping from active runtime progress into parsed layout timeline space
- added a visual timeline bar and `Frame: current/total @ fps` readout to the native GUI evidence overlay
- added `--layout-timeline-smoke` regression coverage and verified `b/rr58/sward_ui_runtime_debug_gui.exe`
- capture-checked `GameModeMainMenu_Test.cpp` through `PrintWindow` with the `ui_mainmenu` evidence overlay showing a live `98/220 @ 60fps` frame readout

### Phase 57 GUI layout evidence preview overlay

- added [`research_uiux/GUI_LAYOUT_EVIDENCE_PREVIEW_OVERLAY.md`](./research_uiux/GUI_LAYOUT_EVIDENCE_PREVIEW_OVERLAY.md)
- added a compact `LayoutEvidence` overlay to `sward_ui_runtime_debug_gui` for Title, Pause, and Loading preview families
- surfaced recovered layout IDs, correlation verdicts, scene/animation counts, cast/subimage counts, cue summaries, and longest parsed timelines directly inside the visual preview
- extended the preview footer with the active recovered layout ID so atlas, family, state, playback, prompt, and layout evidence can be read together
- added `--layout-evidence-smoke` regression coverage and verified `b/rr57/sward_ui_runtime_debug_gui.exe`
- added `--layer-fill-smoke` regression coverage and fixed structural backdrop/cinematic-frame overlays so they preserve the atlas image underneath
- capture-checked `GameModeMainMenu_Test.cpp` in the GUI with the title atlas, runtime layer projection, timeline footer, and `ui_mainmenu` evidence overlay visible

### Phase 56 GUI exact-family preview layouts

- added [`research_uiux/GUI_EXACT_FAMILY_PREVIEW_LAYOUTS.md`](./research_uiux/GUI_EXACT_FAMILY_PREVIEW_LAYOUTS.md)
- added `PreviewFamily` classification for exact Title, Pause, and Loading preview families
- added family-specific projection functions for title-logo/carousel, pause chrome/content, and loading PDA/tip/controller roles instead of forcing those exact-atlas screens through the generic HUD-like stack
- preserved the existing generic role layout for gameplay HUD, town, camera, application/world, support-substrate, and other still-broader families
- added `--family-preview-smoke` regression coverage and verified `b/rr56/sward_ui_runtime_debug_gui.exe`
- capture-checked `GameModeMainMenu_Test.cpp` in the GUI while it was in the title-menu Intro band

### Phase 55 GUI state-aware preview motion

- added [`research_uiux/GUI_STATE_AWARE_PREVIEW_MOTION.md`](./research_uiux/GUI_STATE_AWARE_PREVIEW_MOTION.md)
- added a state-aware `PreviewMotion` adapter to `sward_ui_runtime_debug_gui` so contract preview layers now slide/fade through Intro, Navigate, Confirm, Cancel, and Outro bands instead of staying fully static
- added eased progress from the active runtime state's elapsed time and contract timeline duration
- applied motion to overlay roles and prompt buttons while keeping preview rectangles bounded to the 16:9 workbench canvas
- filled atlas-backed preview canvases with a dark backing brush before drawing local PNG atlas sheets, fixing transparent HUD proxy regions that previously exposed the white panel background
- added `--motion-smoke` regression coverage and verified `b/rr55/sward_ui_runtime_debug_gui.exe`

### Phase 54 GUI timeline playback controls

- added [`research_uiux/GUI_TIMELINE_PLAYBACK_CONTROLS.md`](./research_uiux/GUI_TIMELINE_PLAYBACK_CONTROLS.md)
- added Play/Pause and Step controls to `sward_ui_runtime_debug_gui`
- added a Win32 timer loop around `ScreenRuntime::tick(...)` so `Run Host` now shows intro/action timing bands instead of immediately settling to Idle
- changed action buttons to start playback when they trigger Navigate, Confirm, or Cancel transitions
- added `--playback-smoke` regression coverage, verifying Intro -> Idle and Navigate -> Idle timeline advancement under `b/rr54`
- capture-checked `SonicMainDisplay.cpp` in the GUI while it was still in Intro playback state

### Phase 53 gameplay HUD proxy preview binding

- added [`research_uiux/GAMEPLAY_HUD_PROXY_PREVIEW_BINDING.md`](./research_uiux/GAMEPLAY_HUD_PROXY_PREVIEW_BINDING.md)
- bound Sonic and Werehog gameplay-HUD GUI previews to the recovered `ui_prov_playscreen` atlas as explicit proxy evidence while the exact loose `ui_playscreen*` packages remain unrecovered
- moved the Extra Stage HUD atlas candidate onto the recovered play-screen sheet used by the runtime contract vocabulary
- replaced the GUI preview's diagonal layer stack with bounded role-aware placement for counters, gauges, sidecars, transient effects, and prompt strips
- increased the preview panel height budget so atlas-backed HUD surfaces are readable in normal desktop workbench windows
- extended `--preview-smoke` to verify `10` atlas candidates and `2` proxy candidates, then verified `b/rr53/sward_ui_runtime_debug_gui.exe`

### Phase 52 GUI visual preview and atlas binding

- added [`research_uiux/GUI_VISUAL_PREVIEW_AND_ATLAS_BINDING.md`](./research_uiux/GUI_VISUAL_PREVIEW_AND_ATLAS_BINDING.md)
- added a native preview panel to `sward_ui_runtime_debug_gui` so selected hosts now have a 16:9 visual surface instead of only detail/log text
- added GDI+ atlas loading hooks for `8` high-confidence contract families while keeping the extracted PNG atlas sheets local-only under `extracted_assets/`
- overlaid runtime visible layers, prompt rows, and a timeline/progress strip on top of the preview canvas
- added `--preview-smoke` regression coverage and verified `b/rr52/sward_ui_runtime_debug_gui.exe`

### Phase 51 native GUI debug workbench

- added [`research_uiux/NATIVE_GUI_DEBUG_WORKBENCH.md`](./research_uiux/NATIVE_GUI_DEBUG_WORKBENCH.md)
- added the native Win32 `sward_ui_runtime_debug_gui` target around the existing contract runtime and generated workbench host catalog
- exposed a windowed group/host browser with `Run Host`, `Move Next`, `Confirm`, `Cancel`, and `Reset` controls over `176` workbench hosts across `11` groups
- added a `--smoke` verification path for the GUI executable so automation can validate host/group resolution without opening a window
- built and launch-checked `b/rr51/sward_ui_runtime_debug_gui.exe`, confirming the first proper non-CLI operator shell while keeping the existing selector/workbench console probes intact

### Phase 50 support-substrate runtime contracts

- added [`research_uiux/SUPPORT_SUBSTRATE_RUNTIME_CONTRACTS.md`](./research_uiux/SUPPORT_SUBSTRATE_RUNTIME_CONTRACTS.md)
- added bundled runtime contracts for achievement unlock support, audio/BGM cue support, and XML/data-loading support
- exposed `AchievementUnlockSupport`, `AudioCueSupport`, and `XmlDataLoadingSupport` through the native C++ profile layer, C ABI profile IDs, and C# reference profile mapper
- promoted the current `269`-path source manifest to `203` contract-backed paths (`75.5%`) while preserving `0` `named_seed_only` entries
- regenerated the source-family selector to `19` launch families and the debug workbench to `176` hosts across `11` groups, including `17` support-substrate hosts

### Phase 49 local support-substrate humanization sweep

- added [`research_uiux/LOCAL_SUPPORT_SUBSTRATE_HUMANIZATION.md`](./research_uiux/LOCAL_SUPPORT_SUBSTRATE_HUMANIZATION.md)
- added a dedicated `support_substrate_sources` group to [`research_uiux/tools/materialize_local_debug_source_tree.py`](./research_uiux/tools/materialize_local_debug_source_tree.py)
- generated `23` local-only support-substrate scaffolds for achievement unlocks, animation event triggers, player-status feeds, audio/BGM routing, and XML/data loading without publishing `SONIC UNLEASHED/`
- refreshed [`research_uiux/LOCAL_DEBUG_SOURCE_TREE_EXPANSION.md`](./research_uiux/LOCAL_DEBUG_SOURCE_TREE_EXPANSION.md) to `7` groups, `116` materialized local-only source files, and `125` total readable `.cpp` scaffolds under the ignored mirror
- added a regression test for the new materializer group

### Phase 48 debug workbench catalog view

- added [`research_uiux/DEBUG_WORKBENCH_CATALOG_VIEW.md`](./research_uiux/DEBUG_WORKBENCH_CATALOG_VIEW.md)
- added `--catalog` to [`research_uiux/runtime_reference/examples/ui_debug_workbench.cpp`](./research_uiux/runtime_reference/examples/ui_debug_workbench.cpp)
- added a subprocess regression test for the native workbench catalog command
- verified the catalog view against the widened `159`-host workbench map under `b/rr48`

### Phase 47 broader source-path expansion beyond the current seed

- added [`research_uiux/BROADER_SOURCE_PATH_EXPANSION_PHASE47.md`](./research_uiux/BROADER_SOURCE_PATH_EXPANSION_PHASE47.md)
- widened the curated source-path seed from `220` to `269` entries while keeping the full raw path dump out of the tracked layer
- added support-substrate classification for achievement unlocks, animation event triggers, the wider camera/presentation family, player status/switch support, sound cue routing, and XML/data loading
- refreshed the source-path manifest to `24` families with `212` archaeology/support-mapped entries, `186` runtime-contract-backed entries, `57` debug-host candidates, and `0` named-only gaps
- widened the workbench host map to `159` entries across `10` groups, with `Camera / Replay Hosts` expanded to `30` launchable presentation-controller hosts

### Phase 46 sequence and item source deepening

- added [`research_uiux/SEQUENCE_AND_ITEM_SOURCE_DEEPENING.md`](./research_uiux/SEQUENCE_AND_ITEM_SOURCE_DEEPENING.md)
- deepened the local-only `SONIC UNLEASHED/` readable source layer to `102` `.cpp` scaffolds
- added `12` local-only readable sequence-shell scaffolds plus the `HUD/Item/HudItemGet.cpp` gameplay-HUD scaffold without publishing that mirror itself
- refreshed the tracked summary under [`research_uiux/data/local_debug_source_tree_expansion.json`](./research_uiux/data/local_debug_source_tree_expansion.json)

### Phase 45 frontend sequence shell runtime bridge

- added [`research_uiux/FRONTEND_SEQUENCE_SHELL_RUNTIME_BRIDGE.md`](./research_uiux/FRONTEND_SEQUENCE_SHELL_RUNTIME_BRIDGE.md)
- added the bundled contract [`research_uiux/runtime_reference/contracts/frontend_sequence_shell_reference.json`](./research_uiux/runtime_reference/contracts/frontend_sequence_shell_reference.json)
- extended the native runtime, C ABI, and C# managed reference layers with `FrontendSequenceShell`
- refreshed the broader source-path manifest to `220` paths with `163` archaeology-mapped entries, `154` contract-backed entries, `57` debug-host candidates, and `0` named-only gaps
- widened the source-family selector to `16` launch families and the workbench to `133` hosts across `10` groups, including the new frontend-sequence host bucket

### Phase 44 local debug source tree deepening

- added [`research_uiux/LOCAL_DEBUG_SOURCE_TREE_DEEPENING.md`](./research_uiux/LOCAL_DEBUG_SOURCE_TREE_DEEPENING.md)
- deepened the local-only `SONIC UNLEASHED/` readable source layer to `92` `.cpp` scaffolds
- widened the town/media-room, camera/replay, and application/world shell ownership stubs inside the local-only mirror without publishing that tree
- refreshed the tracked summary under [`research_uiux/data/local_debug_source_tree_expansion.json`](./research_uiux/data/local_debug_source_tree_expansion.json)

### Phase 43 persistent debug shell and workbench

- added [`research_uiux/PERSISTENT_DEBUG_SHELL_AND_WORKBENCH.md`](./research_uiux/PERSISTENT_DEBUG_SHELL_AND_WORKBENCH.md)
- upgraded [`research_uiux/runtime_reference/examples/ui_debug_selector.cpp`](./research_uiux/runtime_reference/examples/ui_debug_selector.cpp) so interactive runs loop back to the menu and direct runs support `--stay-open`
- upgraded [`research_uiux/runtime_reference/examples/ui_debug_workbench.cpp`](./research_uiux/runtime_reference/examples/ui_debug_workbench.cpp) so interactive runs loop back to the group browser and direct runs support `--stay-open`
- widened the generated workbench host map to `129` hosts across `9` groups, including town/media-room, camera/replay, and application/world shell families
- verified the selector/workbench locally from `b/rr44`, including direct launches for `TownManager.cpp`, `FreeCamera.cpp`, `Application.cpp`, `TitleManager.cpp`, and `WorldMapSelect.cpp`

### Phase 42 town, camera, and application shell runtime bridge

- added [`research_uiux/TOWN_CAMERA_APPLICATION_RUNTIME_CONTRACTS.md`](./research_uiux/TOWN_CAMERA_APPLICATION_RUNTIME_CONTRACTS.md)
- added bundled contract files under [`research_uiux/runtime_reference/contracts/`](./research_uiux/runtime_reference/contracts/) for:
  - `town_ui_reference.json`
  - `camera_shell_reference.json`
  - `application_world_shell_reference.json`
- extended the native runtime, C ABI, and C# managed reference layers with `TownUi`, `CameraShell`, and `ApplicationWorldShell`
- refreshed the broader source-path manifest to `220` paths with `158` archaeology-mapped entries, `149` contract-backed entries, `57` debug-host candidates, and `5` named-only gaps
- refreshed the source-family selector metadata to `15` launch families, including town, camera/replay, and application/world shell coverage

## 2026-04-23

### Phase 41 local debug-oriented source tree expansion

- added [`research_uiux/tools/materialize_local_debug_source_tree.py`](./research_uiux/tools/materialize_local_debug_source_tree.py)
- added [`research_uiux/LOCAL_DEBUG_SOURCE_TREE_EXPANSION.md`](./research_uiux/LOCAL_DEBUG_SOURCE_TREE_EXPANSION.md)
- added the tracked summary under [`research_uiux/data/local_debug_source_tree_expansion.json`](./research_uiux/data/local_debug_source_tree_expansion.json)
- widened the local-only readable source layer under `SONIC UNLEASHED/` from `13` to `68` `.cpp` scaffolds across gameplay HUD, stage-test, town, camera, and application/world shells
- refreshed the local-only mirror metadata without publishing that mirror itself

### Phase 40 in-stage HUD workbench expansion

- added [`research_uiux/IN_STAGE_HUD_WORKBENCH_EXPANSION.md`](./research_uiux/IN_STAGE_HUD_WORKBENCH_EXPANSION.md)
- widened the generated workbench-host map under [`research_uiux/data/debug_workbench_host_map.json`](./research_uiux/data/debug_workbench_host_map.json) to `40` hosts
- expanded the native `sward_ui_runtime_debug_workbench` executable so it can launch gameplay-HUD and stage-test hosts in addition to frontend/menu/cutscene buckets
- verified the widened workbench locally against `HudSonicStage.cpp`, `HudEvilStage.cpp`, `HudExQte.cpp`, `BossHudSuperSonic.cpp`, and `GameModeStageForwardTest.cpp` from `b/rr41/Release`

### Phase 39 gameplay HUD runtime contract bridge

- added [`research_uiux/GAMEPLAY_HUD_RUNTIME_CONTRACTS.md`](./research_uiux/GAMEPLAY_HUD_RUNTIME_CONTRACTS.md)
- added bundled gameplay-HUD contract files for Sonic, Werehog, Extra Stage, Super Sonic, and boss/final HUD ownership under [`research_uiux/runtime_reference/contracts/`](./research_uiux/runtime_reference/contracts/)
- extended the native runtime, C ABI, and C# managed reference port with the new gameplay-HUD profile set
- refreshed the broader source-path manifest to `220` paths with `118` archaeology-mapped entries, `88` contract-backed entries, `60` debug-host candidates, and `42` named-only gaps
- widened the selector-family metadata to `12` launch families, including Sonic-stage, Werehog-stage, Extra-stage, Super Sonic, and boss HUD bridges

### Phase 38 first richer UI debug workbench

- added [`research_uiux/tools/build_debug_workbench_data.py`](./research_uiux/tools/build_debug_workbench_data.py)
- added [`research_uiux/FIRST_UI_DEBUG_WORKBENCH.md`](./research_uiux/FIRST_UI_DEBUG_WORKBENCH.md)
- added the generated workbench-host map under [`research_uiux/data/debug_workbench_host_map.json`](./research_uiux/data/debug_workbench_host_map.json)
- added the generated host metadata header under [`research_uiux/runtime_reference/include/sward/ui_runtime/debug_workbench_data.hpp`](./research_uiux/runtime_reference/include/sward/ui_runtime/debug_workbench_data.hpp)
- added the native `sward_ui_runtime_debug_workbench` executable around the recovered menu-debug, stage-debug, and cutscene-preview host buckets
- verified the workbench locally against `GameModeMenuSelectDebug.cpp` and `InspirePreview.cpp` from `b/rr38/Release`

### Phase 37 subtitle and cutscene runtime contract bridge

- added [`research_uiux/runtime_reference/contracts/subtitle_cutscene_reference.json`](./research_uiux/runtime_reference/contracts/subtitle_cutscene_reference.json)
- added [`research_uiux/SUBTITLE_CUTSCENE_RUNTIME_CONTRACTS.md`](./research_uiux/SUBTITLE_CUTSCENE_RUNTIME_CONTRACTS.md)
- extended the bundled profile layers across native C++, the C ABI, and the C# managed reference port with `SubtitleCutscene`
- refreshed the broader source-path manifest to `220` paths with `118` archaeology-mapped entries, `74` contract-backed entries, `60` debug-host candidates, and `42` named-only gaps
- widened the source-family selector so `InspirePreview.cpp` now resolves through the seventh bundled contract-backed runtime family

### Phase 36 local named translated ownership for debug and cutscene hosts

- added [`research_uiux/tools/materialize_humanized_debug_hosts.py`](./research_uiux/tools/materialize_humanized_debug_hosts.py)
- added [`research_uiux/LOCAL_NAMED_TRANSLATED_OWNERSHIP.md`](./research_uiux/LOCAL_NAMED_TRANSLATED_OWNERSHIP.md)
- added the tracked summary under [`research_uiux/data/local_named_translated_ownership.json`](./research_uiux/data/local_named_translated_ownership.json)
- materialized `13` local-only readable `.cpp` source-family scaffolds under `SONIC UNLEASHED/` for the immediate menu-debug and cutscene-preview host set
- converted `86.7%` of the immediate host bucket from notes-only placement into readable translated ownership stubs without publishing that mirror itself

### Phase 35 frontend shell and debug host recovery

- added [`research_uiux/tools/build_frontend_shell_recovery.py`](./research_uiux/tools/build_frontend_shell_recovery.py)
- added [`research_uiux/FRONTEND_SHELL_AND_DEBUG_HOST_RECOVERY.md`](./research_uiux/FRONTEND_SHELL_AND_DEBUG_HOST_RECOVERY.md)
- added the machine-readable shell/debug host map under [`research_uiux/data/frontend_shell_recovery.json`](./research_uiux/data/frontend_shell_recovery.json)
- tightened [`research_uiux/tools/map_ui_source_paths.py`](./research_uiux/tools/map_ui_source_paths.py) so pause/help dispatch, stage-change sequencing, town dispatch, and free/replay camera paths stop living in generic shell buckets
- refreshed the broader source-path manifest to `220` paths with `118` archaeology-mapped entries, `42` contract-backed entries, `60` debug-host candidates, and `42` named-only gaps
- narrowed the immediate shell/debug recovery pass to `46` targeted host paths, of which `13` now bridge into archaeology systems and `4` already land on current runtime contracts

### Phase 34 broader UI-adjacent source-path manifest

- added [`research_uiux/tools/build_broader_ui_adjacent_source_seed.py`](./research_uiux/tools/build_broader_ui_adjacent_source_seed.py)
- added [`research_uiux/source_path_seeds/UI_ADJACENT_SOURCE_PATHS_FROM_MATCH_DUMP.txt`](./research_uiux/source_path_seeds/UI_ADJACENT_SOURCE_PATHS_FROM_MATCH_DUMP.txt)
- refreshed the local source-path manifest to a broader `220`-path UI-adjacent/debug/system subset grouped into `19` families
- raised the current source-path bridge from a narrow UI-only slice into a wider layer with `110` archaeology-mapped paths, `38` contract-backed paths, `57` debug-host candidates, and `53` named-only gaps
- refreshed the local-only `SONIC UNLEASHED/` placement note layer to `220` `*.sward.md` notes without publishing that mirror itself
- widened the selector-family metadata so source-family tokens such as `GameModeBoot.cpp`, `EndingManager.cpp`, and `MainMenuManager.cpp` now resolve through the contract-backed runtime layer where applicable

### Phase 33 source-path-named debug selector

- added [`research_uiux/tools/build_source_family_selector_data.py`](./research_uiux/tools/build_source_family_selector_data.py)
- added [`research_uiux/SOURCE_PATH_NAMED_DEBUG_SELECTOR.md`](./research_uiux/SOURCE_PATH_NAMED_DEBUG_SELECTOR.md)
- added generated runtime metadata under [`research_uiux/runtime_reference/include/sward/ui_runtime/source_family_selector_data.hpp`](./research_uiux/runtime_reference/include/sward/ui_runtime/source_family_selector_data.hpp)
- upgraded [`research_uiux/runtime_reference/examples/ui_debug_selector.cpp`](./research_uiux/runtime_reference/examples/ui_debug_selector.cpp) from contract-stem browsing into source-family browsing
- verified the selector locally against recovered source-family tokens such as `TitleMenu.cpp`, `HudPause.cpp`, and `WorldMapSelect.cpp`
- added family-aware selector commands so the runtime layer now supports `--list-families`, `--family <token>`, and interactive source-family selection

### Phase 32 local source-family placement

- added [`research_uiux/tools/materialize_source_family_notes.py`](./research_uiux/tools/materialize_source_family_notes.py)
- added [`research_uiux/LOCAL_SOURCE_FAMILY_PLACEMENT.md`](./research_uiux/LOCAL_SOURCE_FAMILY_PLACEMENT.md)
- generated the local-only placement manifest under `SONIC UNLEASHED/_meta/source_family_note_manifest.json`
- turned the mirrored `SONIC UNLEASHED/` scaffold into a usable staging tree by materializing `108` local-only `*.sward.md` source-family notes
- split those local placements into `13` direct host anchors, `77` family-member anchors, `13` debug-host candidates, and `5` named-only placeholders
- refreshed the local-only mirror README so the suffix and intent of the note layer are explicit

### Phase 31 CSD / UI foundation humanization

- added [`research_uiux/tools/build_csd_ui_foundation_map.py`](./research_uiux/tools/build_csd_ui_foundation_map.py)
- added [`research_uiux/CSD_UI_FOUNDATION_HUMANIZATION.md`](./research_uiux/CSD_UI_FOUNDATION_HUMANIZATION.md)
- added the machine-readable foundation map under `research_uiux/data/csd_ui_foundation_map.json`
- added [`research_uiux/tools/materialize_source_tree.py`](./research_uiux/tools/materialize_source_tree.py) to keep the local-only `SONIC UNLEASHED/` mirror scaffold reproducible from the root path dump
- mapped `5` direct `CSD/*` / `Menu/*` seed paths, `5` closely related consumer/widget paths, and `12` mirrored support paths into `3` reusable abstractions
- refreshed the source-path manifest so the measured UI-seed bridge rises to `90 / 108` paths (`83.3%`) mapped into archaeology systems, leaving only `5` named-only gaps in the current UI-focused seed
- anchored the foundation layer around `MakeCsdProjectMidAsmHook`, `CCsdProject::Make`, `CCsdPlatformMirage::Draw`, `CCsdPlatformMirage::DrawNoTex`, and `CHelpWindow::MsgRequestHelp::Impl`

### Phase 30 gameplay HUD core recovery

- added [`research_uiux/tools/build_gameplay_hud_core_map.py`](./research_uiux/tools/build_gameplay_hud_core_map.py)
- added [`research_uiux/GAMEPLAY_HUD_CORE_RECOVERY.md`](./research_uiux/GAMEPLAY_HUD_CORE_RECOVERY.md)
- generated a new machine-readable HUD-core map under `research_uiux/data/gameplay_hud_core_map.json`
- grouped the in-stage HUD evidence into `4` host-owned systems across `6` layout families
- confirmed that `ui_prov_playscreen` and `ui_qte` are loose-layout-backed while `ui_playscreen`, `ui_playscreen_ev`, `ui_playscreen_ev_hit`, and `ui_playscreen_su` remain hash/source-path-backed
- tied the strongest readable/translated bridge in this family to `CEvilHudGuide` via `EvilHudGuideAllocMidAsmHook`, `EvilHudGuideUpdateMidAsmHook`, `0x82448CF0`, and `0x82449088`

### Phase 29 standalone UI debug selector

- added [`research_uiux/STANDALONE_UI_DEBUG_SELECTOR.md`](./research_uiux/STANDALONE_UI_DEBUG_SELECTOR.md)
- added [`research_uiux/runtime_reference/examples/ui_debug_selector.cpp`](./research_uiux/runtime_reference/examples/ui_debug_selector.cpp)
- expanded the bundled runtime profile surface to include loading, mission-result, and world-map alongside pause, title, and autosave
- added the native selector target `sward_ui_runtime_debug_selector` so the contract-backed screens can now be listed, selected, and stepped from one executable
- verified the selector locally against bundled `title`, `loading`, and `world map` contracts, while re-verifying the managed C# reference after the bundled-profile expansion

### Phase 28 UI source-path recovery and humanization plan

- added [`research_uiux/tools/map_ui_source_paths.py`](./research_uiux/tools/map_ui_source_paths.py)
- added [`research_uiux/source_path_seeds/UI_SOURCE_PATHS_FROM_EXECUTABLE.txt`](./research_uiux/source_path_seeds/UI_SOURCE_PATHS_FROM_EXECUTABLE.txt)
- added [`research_uiux/UI_SOURCE_PATH_RECOVERY_AND_HUMANIZATION_PLAN.md`](./research_uiux/UI_SOURCE_PATH_RECOVERY_AND_HUMANIZATION_PLAN.md)
- generated a new local manifest under `research_uiux/data/ui_source_path_manifest.json`
- organized `108` UI-centric source paths from the supplied Xbox 360 dump into `16` families
- measured that `77` paths (`71.3%`) already land in the current archaeology layer, while `37` paths (`34.3%`) are already backed by the portable runtime contracts
- isolated `13` strong debug-tool host candidates and `18` still named-only gaps, clarifying that the next bottleneck is source-family humanization and a standalone UI screen selector rather than raw PPC generation

### Phase 25 broader remaining UI and common-flow extraction

- added [`research_uiux/tools/summarize_commonflow_extraction.py`](./research_uiux/tools/summarize_commonflow_extraction.py)
- added [`research_uiux/COMMON_FLOW_LOCALIZATION_EXTRACTION.md`](./research_uiux/COMMON_FLOW_LOCALIZATION_EXTRACTION.md)
- ranked `357` remaining UI/common-flow archive candidates after the earlier archaeology passes
- safely extracted a focused `24`-directory English common-flow/localization slice into `extracted_assets/phase25_commonflow_archives`
- confirmed this batch was loose-file heavy rather than layout heavy, adding `338` files with `54` `.dds`, `261` `.fco`, and `15` `.fte` payloads but no new `.yncp` packages
- re-ran the asset scan to `6840` combined matched entries, with `5112` extracted-side matches and `89` indexed hits inside the Phase 25 extraction root
- documented the growing emphasis on correlation/state labeling over blind archive hunting for the next beats

### Phase 26 deeper PPC-to-layout and state labeling

- added [`research_uiux/tools/label_ppc_ui_states.py`](./research_uiux/tools/label_ppc_ui_states.py)
- added [`research_uiux/PPC_LAYOUT_STATE_LABELS.md`](./research_uiux/PPC_LAYOUT_STATE_LABELS.md)
- generated a new local seam/state label inventory under `research_uiux/data/ppc_ui_state_labels.json`
- labeled `180` translated PPC seams across `8` targeted systems, including loading/start, world map, town UI, mission/gate, result, save/ending, and Tornado Defense
- sampled translated function windows for each resolved seam so the label layer carries call-shape and float/timing hints rather than only symbol names
- confirmed that subtitle/cutscene presentation remains asset/readable-code driven in the current pass, with no resolved translated PPC seam yet

### Phase 27 data-driven runtime contracts

- added [`research_uiux/DATA_DRIVEN_RUNTIME_CONTRACTS.md`](./research_uiux/DATA_DRIVEN_RUNTIME_CONTRACTS.md)
- added the shared runtime contract bundle under [`research_uiux/runtime_reference/contracts/`](./research_uiux/runtime_reference/contracts/)
- added the native JSON contract loader under [`research_uiux/runtime_reference/include/sward/ui_runtime/contract_loader.hpp`](./research_uiux/runtime_reference/include/sward/ui_runtime/contract_loader.hpp) and [`research_uiux/runtime_reference/src/contract_loader.cpp`](./research_uiux/runtime_reference/src/contract_loader.cpp)
- converted the existing native profile builders into compatibility wrappers over the bundled JSON contracts
- added path-based contract loading to the C ABI and shared contract loading to the C# reference port
- verified bundled and explicit path-based runtime loading in native C++ and C, then re-verified the managed build/run in the repo-local `.NET 8` environment

### Phase 23 full UI archaeology cross-reference

- added [`research_uiux/tools/build_ui_archaeology_database.py`](./research_uiux/tools/build_ui_archaeology_database.py)
- added [`research_uiux/FULL_UI_ARCHAEOLOGY_CROSS_REFERENCE.md`](./research_uiux/FULL_UI_ARCHAEOLOGY_CROSS_REFERENCE.md)
- added [`research_uiux/data/ui_archaeology_database.json`](./research_uiux/data/ui_archaeology_database.json)
- safely extracted a broader town/common-flow archive batch into `extracted_assets/phase23_crossref_archives`
- expanded the extracted asset inventory to `6751` matched entries and the parsed layout layer to `28` files / `26` merged layout IDs
- upgraded the correlation layer so `ui_balloon`, `ui_shop`, `ui_townscreen`, `ui_mediaroom`, `ui_gate`, `ui_missionscreen`, `ui_misson`, `ui_exstage`, `ui_prov_playscreen`, `ui_result`, `ui_result_ex`, and `ui_start` have explicit roles, while `ui_qte` is now evidence-backed instead of unresolved
- grouped the current evidence into `13` reusable screen/system families and resolved `56` generated PPC seams into the active archaeology layer
- updated the research workspace README, asset index, checklist, and repo-facing README for the new cross-reference phase

### Phase 24 reusable port kits

- added [`research_uiux/REUSABLE_PORT_KITS.md`](./research_uiux/REUSABLE_PORT_KITS.md)
- expanded [`research_uiux/runtime_reference/`](./research_uiux/runtime_reference/) with reusable profile factories for pause, title-menu, and autosave-toast screen families
- added a C ABI wrapper plus a verified native C example over the generic runtime
- added a self-contained C# managed reference port under [`research_uiux/runtime_reference/csharp_reference/`](./research_uiux/runtime_reference/csharp_reference/)
- verified the native build with `C:\Program Files\CMake\bin\cmake.exe` and the managed build with `external_tools/dotnet8/dotnet.exe`

### Phase 22 coverage audit and markdown presentation pass

- added [`research_uiux/WHOLE_GAME_COVERAGE_AND_GAPS.md`](./research_uiux/WHOLE_GAME_COVERAGE_AND_GAPS.md) to answer the direct whole-game-source vs translated-output vs extracted-asset question with verified local counts
- normalized the project-owned markdown headers around the larger `icon_sward` presentation
- enlarged the root README section icons and added the original-size `icon_sward` footer mark
- updated the repo-facing README, the research workspace README, and the checklist audit for the new coverage/branding pass

### Phase 21 code-backed runtime reference implementation

- added the standalone C++ runtime reference under [`research_uiux/runtime_reference/`](./research_uiux/runtime_reference/)
- added [`research_uiux/CODE_BACKED_RUNTIME_IMPLEMENTATION.md`](./research_uiux/CODE_BACKED_RUNTIME_IMPLEMENTATION.md)
- implemented generic state, overlay, prompt, and timing-band runtime types that mirror the Phase 18 template pack
- verified the runtime locally with a standalone CMake build and the `pause_menu_example` executable

### Phase 20 boss HUD, result, and save visual taxonomy

- added [`research_uiux/tools/derive_visual_taxonomy.py`](./research_uiux/tools/derive_visual_taxonomy.py)
- added [`research_uiux/BOSS_RESULT_SAVE_VISUAL_TAXONOMY.md`](./research_uiux/BOSS_RESULT_SAVE_VISUAL_TAXONOMY.md)
- derived a machine-readable local taxonomy from atlas/layout outputs for `ui_boss_gauge`, `ui_boss_name`, `ui_result`, `ui_result_ex`, `ui_itemresult`, and `ui_saveicon`
- turned palette, texture-count, scene-family, and timing evidence into a screen-by-screen style guide for original projects

### Phase 19 subtitle and cutscene presentation deep dive

- added [`research_uiux/tools/analyze_subtitle_cutscene_presentation.py`](./research_uiux/tools/analyze_subtitle_cutscene_presentation.py)
- added [`research_uiux/SUBTITLE_CUTSCENE_PRESENTATION_DEEP_DIVE.md`](./research_uiux/SUBTITLE_CUTSCENE_PRESENTATION_DEEP_DIVE.md)
- extracted a focused English subtitle slice into `extracted_assets/phase19_subtitle_archives` without needing any new tool download
- correlated `*.inspire_resource.xml` subtitle cue windows against `#Application` `PlayMovie` ownership, hide-layer flags, and loading/stage-handoff behavior
- tied the extracted subtitle layer back to readable runtime controls for `Subtitles` and `CutsceneAspectRatio`

### Phase 18 transferable template pack for original projects

- added [`research_uiux/TEMPLATE_PACK_FOR_ORIGINAL_PROJECTS.md`](./research_uiux/TEMPLATE_PACK_FOR_ORIGINAL_PROJECTS.md)
- added the reusable template bundle under [`research_uiux/templates/`](./research_uiux/templates)
- distilled recovered screen-state, overlay, prompt-row, and timing patterns into generic YAML/JSON contracts
- documented how to adapt the pack for original work without copying proprietary assets, strings, audio, or scene naming

### Phase 17 visual atlas docs from extracted textures and layouts

- added [`research_uiux/tools/build_visual_atlas.py`](./research_uiux/tools/build_visual_atlas.py)
- added [`research_uiux/VISUAL_ATLAS_DOCS.md`](./research_uiux/VISUAL_ATLAS_DOCS.md)
- verified a repo-local DDS render path with `Pillow 12.2.0`, so no extra atlas tool download was needed
- generated a local-only atlas root at `extracted_assets/visual_atlas`
- produced `23` atlas sheets covering `144` rendered UI texture references across the current extracted layout set

### Phase 16 deep dive: boss HUD, result, and save-load flow

- added [`research_uiux/BOSS_RESULT_SAVE_LOAD_DEEP_DIVE.md`](./research_uiux/BOSS_RESULT_SAVE_LOAD_DEEP_DIVE.md)
- ran a focused local support extraction over `ActionCommon.arl`, `ExStageTails_Common.arl`, and `SonicActionCommonGeneral.arl`
- closed the earlier `ui_result_ex` gap by confirming real extracted `ui_result.yncp` and `ui_result_ex.yncp` packages
- tied boss HUD, mission-result overlays, autosave, and loading transitions back to the `#Application` sequence graph
- re-ran the local asset/layout indexes, expanding the local-only workspace to `3503` combined asset hits and `23` parsed layout files

### `d0a8b48` `Crack open pause, status, and world map flow`

- added [`research_uiux/PAUSE_STATUS_WORLDMAP_DEEP_DIVE.md`](./research_uiux/PAUSE_STATUS_WORLDMAP_DEEP_DIVE.md)
- consolidated the strongest pause, status, and world-map findings into a single screen-level report
- clarified confidence boundaries between asset-side evidence and readable-code host-state evidence

### `deb2da2` `Wire extracted layouts back to readable code`

- added [`research_uiux/tools/correlate_code_layouts.py`](./research_uiux/tools/correlate_code_layouts.py)
- added [`research_uiux/CODE_TO_LAYOUT_CORRELATION.md`](./research_uiux/CODE_TO_LAYOUT_CORRELATION.md)
- linked extracted layout IDs back to readable patch hooks and generated seams where available

### `7378a06` `Pull a wider UI archive slice into view`

- added [`research_uiux/tools/find_ui_archive_candidates.py`](./research_uiux/tools/find_ui_archive_candidates.py)
- added [`research_uiux/tools/extract_ui_archive_batch.py`](./research_uiux/tools/extract_ui_archive_batch.py)
- added [`research_uiux/BROADER_UI_ARCHIVE_EXTRACTION.md`](./research_uiux/BROADER_UI_ARCHIVE_EXTRACTION.md)
- expanded targeted local extraction into boss HUD and broader UI/HUD/common-flow archives

### `5312b10` `Give the repo a SWARD face`

- renamed the GitHub-facing identity to `SWARD`
- moved branding assets into [`docs/assets/branding/`](./docs/assets/branding)
- updated README branding and repo-facing links

### `24e8395` `Rename repo identity and make CI asset-aware`

- set the repo identity to **Project Sonic World Adventure R&D**
- made GitHub Actions pass cleanly without private asset secrets
- preserved a split between public-safe validation and asset-backed validation

### `0486080` `Initial publishable UI research workspace`

- established the publishable repo boundary
- checked in the safe research tooling and open-source integration layer
- published the first index-and-notes pass for the UI/UX R&D workspace

## Current Shape

The repo now contains:

- publishable research tooling
- readable UI/patch analysis
- repo-safe branding and documentation
- report-level correlation between extracted local layouts and readable code
- dedicated deep dives for pause/world-map and boss/result/save-load flow
- a publishable atlas-builder that renders local-only contact sheets from the extracted DDS/UI layer
- a reusable template pack for original screen-state, overlay, prompt, and transition work
- a subtitle/cutscene presentation report that bridges subtitle timing resources, `PlayMovie` sequences, and readable runtime controls
- a visual taxonomy/style guide for boss HUD, result, and save systems grounded in the local atlas layer
- a buildable generic runtime reference that turns the template pack into executable C++ code
- a reusable multi-language port-kit layer across C++, C, and C#
- a standalone contract-backed debug selector over the current reusable screen family set
- a dedicated gameplay-HUD-core recovery layer for the day/night/extra-stage/super variants

The repo still does not contain:

- extracted proprietary assets
- private Xbox 360 inputs
- generated PPC translation output
- generated shader cache output
