<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> UI/UX Research Checklist

## Phase 0 - Local Context

- [x] Print current working directory and detect OS/shell basics.
- [x] Locate usable source root and installed build root.
- [x] Verify key source directories exist.
- [x] Verify presence/absence of `default.xex`, `default.xexp`, and `shader.ar`.
- [x] Verify presence/absence of generated PPC and shader outputs.
- [x] Populate `ENVIRONMENT_REPORT.md`.

## Phase 1 - Build / Generation

- [x] Confirm whether the local Windows environment satisfies documented build requirements.
- [x] Stage private inputs into `UnleashedRecompLib/private/` non-destructively.
- [x] Attempt configure and document the exact blocker in `BUILD_AND_GENERATION_REPORT.md`.
- [x] Record commands, outputs, generated file counts, and failures.
- [x] Successful generation of `ppc_recomp.*.cpp` and `shader_cache.cpp` in the clean local clone `local_build_env\ur103clean`.

## Phase 2 - Readable UI Code Index

- [x] Scan `UnleashedRecomp/ui/`, `UnleashedRecomp/patches/`, `UnleashedRecomp/config/`, and `UnleashedRecompLib/config/`.
- [x] Extract class names, function names, enums, state names, update/draw/render logic, input, fade/transition/timer logic.
- [x] Generate `data/ui_code_index.json`.
- [x] Generate `UI_CODE_INDEX.md`.

## Phase 3 - Patch Hook Map

- [x] Scan `UnleashedRecomp/patches/` for hooks, wrappers, sub refs, and addresses.
- [x] Map each patch to touched UI state and new behavior.
- [x] Generate `data/patch_hooks.json`.
- [x] Generate `PATCH_HOOK_INDEX.md`.

## Phase 4 - Generated PPC Map

- [x] Check whether generated PPC output exists.
- [x] Generate `data/generated_function_refs.json`.
- [x] Generate `GENERATED_CODE_MAP.md`.
- [x] Map patched UI/HUD/title/input seams to generated PPC files and implementation lines.

## Phase 5 - Asset Index

- [x] Scan the installed build for UI-relevant archives, layouts, textures, and configs.
- [x] Detect viewer/tool candidates.
- [x] Generate `data/asset_index.json`.
- [x] Generate `UI_ASSET_INDEX.md`.

## Phase 6 - State Machines and Transitions

- [x] Reconstruct title intro/menu, pause, options, achievement, button guide, stage title, fade, black bar, TV static, and HUD-related behavior where readable evidence exists.
- [x] Generate `UI_STATE_MACHINES.md`.
- [x] Generate `UI_ANIMATION_AND_TRANSITION_NOTES.md`.

## Phase 7 - Transferable Design / Engineering Notes

- [x] Capture reusable UI architecture and naming patterns.
- [x] Generate `UI_UX_INSPIRATION_NOTES.md`.

## Phase 8 - Helper Scripts

- [x] Create `scan_ui_code.py`.
- [x] Create `scan_assets.py`.
- [x] Create `map_patch_hooks.py`.
- [x] Create `extract_state_keywords.py`.

## Phase 9 - Final Summary

- [x] Update `README.md` with findings, blockers, and next commands.
- [x] Verify generated outputs exist.

## Phase 10 - Extended Layout Extraction And Semantic Inspection

- [x] Audit local `HedgeArcPack`, `Kunai`, and `Shuriken` behavior for scriptable UI layout inspection.
- [x] Safely extract additional UI-heavy archives into `extracted_assets\ui_extended_archives`.
- [x] Create `tools/inspect_xncp_yncp.py`.
- [x] Generate `data/layout_semantics.json`.
- [x] Generate `XNCP_YNCP_SEMANTIC_NOTES.md`.
- [x] Re-run `scan_assets.py` across the installed build plus the full `extracted_assets` tree.
- [x] Update `README.md`, `UI_ASSET_INDEX.md`, `UI_ANIMATION_AND_TRANSITION_NOTES.md`, and `LOCATION_MANIFEST.md`.

## Phase 11 - Broader Full UI Archive Extraction

- [x] Audit additional UI-heavy `.arl` / `.ar.00` candidates beyond the current extracted set.
- [x] Safely extract a broader archive slice into a dedicated local output root.
- [x] Re-run `scan_assets.py` and update `data/asset_index.json`.
- [x] Update `UI_ASSET_INDEX.md` with the broader extraction findings.
- [x] Generate a dedicated broader-extraction notes report.

## Phase 12 - Code-to-Layout Correlation

- [x] Correlate extracted layout names/scenes with readable UI code and patch hooks.
- [x] Generate a machine-readable correlation map.
- [x] Generate a human-readable code-to-layout correlation report.
- [x] Update `UI_STATE_MACHINES.md` and `UI_ANIMATION_AND_TRANSITION_NOTES.md` where new correlations strengthen the evidence.

## Phase 13 - Pause / Status / WorldMap Deep Dive

- [x] Produce a focused pause/status/world-map UI research report.
- [x] Reconstruct state transitions and animation families specific to those screens.
- [x] Correlate those screens against readable hooks, extracted layouts, and any useful generated PPC seams.
- [x] Update `README.md` and the checklist audit after the deep dive lands.

## Phase 14 - Multilingual Archive Expansion And Subtitle / Cutscene UI Coverage

- [x] Audit multilingual UI/archive candidates and subtitle archive coverage beyond the current extracted set.
- [x] Safely extract a multilingual UI/cutscene-support archive slice into a dedicated local output root.
- [x] Generate a machine-readable multilingual coverage summary.
- [x] Generate a human-readable multilingual/subtitle coverage report.
- [x] Re-run `scan_assets.py` and update `data/asset_index.json`, `UI_ASSET_INDEX.md`, and `research_uiux/README.md`.

## Phase 15 - Deeper XNCP / YNCP Scene Graph And Timeline Analysis

- [x] Extend the local layout parser to expose scene-graph structure, animation track families, and keyframe/timeline metrics.
- [x] Generate a machine-readable deep layout-analysis payload.
- [x] Generate a human-readable scene-graph / timeline notes report.
- [x] Update `XNCP_YNCP_SEMANTIC_NOTES.md` and related reports where the deeper parser materially strengthens claims.

## Phase 16 - Boss HUD / Mission-Result / Save-Load Deep Dives

- [x] Audit remaining mission/result/save/load archive candidates and extract a focused supporting slice if needed.
- [x] Generate a dedicated boss HUD + mission/result + save/load deep-dive report.
- [x] Update code-to-layout/state-machine notes where the new deep dive tightens behavior evidence.

## Phase 17 - Visual Atlas Docs From Extracted Textures / Layouts

- [x] Verify a local image conversion/render path for extracted UI textures.
- [x] Generate a visual atlas output root from extracted UI textures/layout groups.
- [x] Generate a publishable atlas document tying atlas sheets back to layouts/screens.

## Phase 18 - Transferable Template Pack For Original Projects

- [x] Create a reusable template pack derived from recovered state, overlay, prompt, and transition patterns.
- [x] Document how to adapt the templates without copying proprietary assets/code.
- [x] Update `README.md` and the checklist audit for the new continuation phases.

## Phase 19 - Subtitle / Cutscene Presentation State Deep Dive

- [x] Audit subtitle-resource XML plus `#Application` movie sequences for presentation-state evidence.
- [x] Extract a focused subtitle/cutscene support slice if the existing multilingual sample is too narrow.
- [x] Generate a machine-readable subtitle/cutscene presentation map.
- [x] Generate a dedicated subtitle/cutscene presentation deep-dive report.

## Phase 20 - Boss HUD / Result / Save Visual Taxonomy And Style Guide

- [x] Derive a visual taxonomy from the local atlas/layout layer for boss HUD, result, and save systems.
- [x] Generate a publishable screen-by-screen style guide tying scene families, texture families, and timing bands together.
- [x] Update the atlas/style reports and checklist audit.

## Phase 21 - Code-Backed Reusable Runtime Implementation

- [x] Implement a generic runtime reference layer that matches the template pack contracts.
- [x] Document how to build and adapt the runtime without inheriting proprietary content.
- [x] Update repo-facing docs and the checklist audit for the runtime layer.

## Phase 22 - Coverage / Gap Status And Markdown Branding

- [x] Generate a direct coverage/gap report answering what is and is not locally available for whole-game source, translated code, and extracted assets.
- [x] Normalize project-owned markdown headers around the larger `icon_sward` presentation.
- [x] Add the original-size `icon_sward` footer mark to the root `README.md`.
- [x] Update `CHANGELOG.md`, `research_uiux/README.md`, and the checklist audit for the coverage/branding pass.

## Phase 23 - Full UI Archaeology Cross-Reference

- [x] Safely extract a broader remaining UI-heavy batch into `extracted_assets\phase23_crossref_archives`.
- [x] Re-run `scan_assets.py` so the asset inventory includes the Phase 23 extraction root.
- [x] Extend `correlate_code_layouts.py` with the newly extracted town/mission/Tornado Defense layout families.
- [x] Generate `research_uiux\data\ui_archaeology_database.json`.
- [x] Generate `research_uiux\FULL_UI_ARCHAEOLOGY_CROSS_REFERENCE.md`.
- [x] Identify reusable host/state/timeline contracts across title, pause, world map, town, mission, result, boss HUD, save, Tornado Defense, and subtitle/cutscene families.
- [x] Update the checklist audit, research README, asset index, changelog, and repo-facing README for the new cross-reference phase.

## Phase 24 - Reusable Port Kits

- [x] Expand the generic runtime reference with reusable profile factories for multiple recovered screen families.
- [x] Add a C ABI wrapper and a verified native C example over the runtime layer.
- [x] Add a self-contained C# managed reference port using the repo-local `.NET 8` runtime.
- [x] Generate `research_uiux\REUSABLE_PORT_KITS.md`.
- [x] Update runtime docs, repo docs, and the checklist audit for the new port-kit layer.

## Phase 25 - Broader Remaining UI / Common-Flow Extraction

- [x] Rank still-uncovered UI/common-flow archive candidates after the archaeology and multilingual passes.
- [x] Safely extract a dedicated common-flow/localization support slice into `extracted_assets\phase25_commonflow_archives`.
- [x] Re-run `scan_assets.py` so the asset inventory includes the Phase 25 extraction root.
- [x] Generate `research_uiux\data\commonflow_localization_extraction.json`.
- [x] Generate `research_uiux\COMMON_FLOW_LOCALIZATION_EXTRACTION.md`.
- [x] Update the local checklist audit, research README, UI asset index, and repo-facing docs for the new common-flow pass.

## Phase 26 - Deeper PPC-To-Layout / State Labeling

- [x] Generate a dedicated seam/state labeling pass for the still-underexplained common-flow systems.
- [x] Produce a machine-readable PPC/layout/state label inventory.
- [x] Produce a human-readable report that groups translated seams by system, role, timing, and host ownership.
- [x] Update the archaeology/correlation docs and checklist audit for the new PPC labeling layer.

## Phase 27 - Data-Driven Runtime Contract Loading

- [x] Replace hardcoded runtime profile builders with portable contract-file loading for the reusable kits.
- [x] Add contract-file loading support across C++, C, and C#.
- [x] Add tracked portable contract files for the current reference screen families plus at least one broader common-flow family.
- [x] Verify the native and managed runtime examples against contract-file loading.
- [x] Update runtime docs, repo docs, and the checklist audit for the new data-driven runtime layer.

## Phase 28 - UI Source-Path Recovery And Humanization Plan

- [x] Curate a UI/UX-focused source-path seed from the supplied Xbox 360 executable path dump.
- [x] Generate a machine-readable manifest that maps the seed onto current archaeology systems and runtime contracts.
- [x] Generate a human-readable report with exact current coverage percentages plus a debug-tool direction.
- [x] Update repo-facing docs, the research README, and the checklist audit for the new source-path bridge layer.

## Phase 29 - Standalone UI Debug Selector

- [x] Expand the bundled runtime profile surface to cover `pause`, `title`, `autosave`, `loading`, `mission result`, and `world map`.
- [x] Add a standalone selector executable that can browse bundled contracts by list, index, token, or explicit contract path.
- [x] Verify the selector locally against at least `title`, `loading`, and `world map`.
- [x] Generate `STANDALONE_UI_DEBUG_SELECTOR.md`.

## Phase 30 - Gameplay HUD Core Recovery

- [x] Anchor the stage HUD family around `ui_playscreen`, `ui_playscreen_ev`, `ui_playscreen_ev_hit`, and `ui_playscreen_su`.
- [x] Tie `HUD/Sonic`, `HUD/Evil`, player HUD helpers, and the Extra Stage HUD family back to readable hooks, source-path seeds, asset families, and translated seams.
- [x] Generate a machine-readable gameplay HUD core map.
- [x] Generate a human-readable gameplay HUD core recovery report.
- [x] Update the archaeology/state/correlation layer where the gameplay HUD evidence materially tightens host ownership or timing claims.

## Phase 31 - CSD / UI Foundation Humanization

- [x] Maintain the local-only mirrored source tree under `SONIC UNLEASHED/` from the supplied path dump without publishing that mirror itself.
- [x] Map the `CSD/*`, `Menu/*`, and closely related core/manager files into reusable scene/widget abstractions.
- [x] Generate a machine-readable CSD/UI foundation humanization map.
- [x] Generate a human-readable CSD/UI foundation report.
- [x] Update source-path coverage docs and the checklist audit for the new humanization layer.

## Phase 32 - Local Source-Family Placement

- [x] Materialize local-only `*.sward.md` placement notes beside the mirrored `SONIC UNLEASHED/` source-family paths.
- [x] Generate a local-only placement manifest under `SONIC UNLEASHED/_meta/`.
- [x] Generate a tracked human-readable report describing the placement layer and its counts.
- [x] Update repo-facing/source-path docs and the checklist audit for the new placement layer.

## Phase 33 - Source-Path-Named Debug Selector

- [x] Expand the standalone debug selector from contract-name browsing to source-path/family-name browsing.
- [x] Add a tracked selector-family index or equivalent generated runtime metadata.
- [x] Verify the selector locally against source-family tokens rather than only contract stems.
- [x] Update runtime docs and the checklist audit for the new selector layer.

## Phase 34 - Broader UI-Adjacent Source-Path Manifest

- [x] Widen the source-path seed beyond the current `108` UI-centric entries into a broader UI-adjacent/debug/system subset.
- [x] Generate a refreshed machine-readable broader manifest locally.
- [x] Generate a tracked human-readable report with the widened coverage counts and gaps.
- [x] Update the relevant coverage/source-path docs and checklist audit.

## Phase 35 - Frontend Shell And Debug Host Recovery

- [x] Tighten the widened shell/debug layer into concrete host/dispatch buckets instead of leaving it as generic system/sequence/camera spillover.
- [x] Generate `research_uiux\data\frontend_shell_recovery.json`.
- [x] Generate `research_uiux\FRONTEND_SHELL_AND_DEBUG_HOST_RECOVERY.md`.
- [x] Refresh the broader source-path manifest, local-only `SONIC UNLEASHED\**\*.sward.md` note layer, and selector metadata where the tighter ownership changes land.

## Phase 36 - Local Named Translated Ownership For Debug / Cutscene Hosts

- [x] Materialize local-only readable `.cpp` humanization scaffolds under `SONIC UNLEASHED\` for the first menu-debug and cutscene-preview host surfaces.
- [x] Generate `research_uiux\data\local_named_translated_ownership.json`.
- [x] Generate `research_uiux\LOCAL_NAMED_TRANSLATED_OWNERSHIP.md`.
- [x] Refresh the local-only mirror metadata so the tree no longer reads as notes-only for the newly humanized hosts.

## Phase 37 - Subtitle / Cutscene Runtime Contract Bridge

- [x] Add a bundled subtitle/cutscene runtime contract and wire it through the native, C ABI, and C# reference profile layers.
- [x] Refresh the broader source-path manifest and selector metadata so subtitle/cutscene becomes a seventh contract-backed family.
- [x] Generate `research_uiux\SUBTITLE_CUTSCENE_RUNTIME_CONTRACTS.md`.
- [x] Verify subtitle/cutscene launches through the native selector and explicit C/C# contract loading paths.

## Phase 38 - First Richer UI Debug Workbench

- [x] Add generated workbench-host metadata from the frontend shell/debug recovery layer.
- [x] Add a richer native debug workbench executable around `GameModeMenuSelectDebug.cpp`, `GameModeStageSelectDebug.cpp`, and `InspirePreview*.cpp`.
- [x] Generate `research_uiux\data\debug_workbench_host_map.json`.
- [x] Generate `research_uiux\FIRST_UI_DEBUG_WORKBENCH.md`.
- [x] Verify the workbench locally against menu-debug and cutscene-preview host tokens.

## Phase 39 - Gameplay HUD Runtime Contract Bridge

- [x] Add bundled runtime contracts for the in-stage HUD families covering Sonic, Werehog, Extra Stage, and boss/final HUD ownership.
- [x] Wire the new gameplay-HUD contracts through the native runtime, C ABI, and C# reference profile layers.
- [x] Refresh the source-path manifest and selector metadata so gameplay-HUD host paths stop being archaeology-only where the contracts now exist.
- [x] Generate a dedicated gameplay-HUD runtime-contract report.

## Phase 40 - In-Stage HUD Workbench Expansion

- [x] Extend the debug workbench metadata layer so it can launch gameplay-HUD and stage-test hosts in addition to the existing frontend/menu/cutscene buckets.
- [x] Verify the richer workbench locally against Sonic-stage, Werehog-stage, Extra-Stage, and boss/final HUD host tokens.
- [x] Generate a dedicated report for the in-stage HUD workbench expansion.

## Phase 41 - Local Debug-Oriented Source Tree Expansion

- [x] Expand the local-only readable `.cpp` source layer beyond the first debug/cutscene hosts into gameplay HUD, stage-test, town, camera, and application/world shell paths.
- [x] Refresh the local-only `SONIC UNLEASHED\_meta\` manifests and counts for the widened readable source layer.
- [x] Generate a tracked JSON summary plus a human-readable report for the widened local-only source-tree expansion.

## Phase 42 - Town / Camera / Application Shell Runtime Bridge

- [x] Add bundled runtime contracts for town/media-room, camera/replay, and application/world shell families.
- [x] Wire the new shell contracts through the native runtime, C ABI, and C# managed reference layers.
- [x] Refresh the broader source-path manifest and selector metadata so those shell families stop being archaeology-only or shell-only where the contracts now exist.
- [x] Generate a dedicated report for the new shell-contract bridge.

## Phase 43 - Persistent Debug Shell UX And Workbench Expansion

- [x] Upgrade the native selector/workbench interactive flows so they loop back to the menu/group browser instead of exiting after one launch.
- [x] Add a direct `--stay-open` behavior for one-shot selector/workbench launches.
- [x] Expand the generated workbench host map so town, camera, title, world-map, and application/world shell hosts become first-class direct launch targets.
- [x] Generate a dedicated report for the persistent debug-shell/workbench pass.

## Phase 44 - Local Debug Source Tree Deepening

- [x] Deepen the local-only readable source mirror with broader town, camera, and application/world shell ownership scaffolds.
- [x] Refresh the local-only `SONIC UNLEASHED\_meta\` manifests and tracked source-tree summary counts after the deeper pass.
- [x] Generate a dedicated report for the deeper local debug-oriented source-tree layer.

## Phase 45 - Frontend Sequence Shell Runtime Bridge

- [x] Add a bundled runtime contract for the generic frontend sequence-core / unit-factory shell.
- [x] Wire the new sequence-shell contract through the native runtime, C ABI, and C# managed reference layers.
- [x] Refresh the broader source-path manifest plus selector/workbench metadata so the remaining `Sequence/*` core/factory shell paths stop being `named_seed_only`.
- [x] Generate a dedicated report for the new sequence-shell bridge.

## Phase 46 - Sequence And Item Source Deepening

- [x] Deepen the local-only readable source mirror with sequence-core, sequence-unit, and item-overlay ownership scaffolds.
- [x] Refresh the local-only `SONIC UNLEASHED\_meta\` manifests and tracked source-tree summary counts after the sequence/item pass.
- [x] Generate a dedicated report for the widened sequence/item debug-oriented source layer.

## Phase 47 - Broader Source-Path Expansion Beyond The Current Seed

- [x] Widen the curated source-path seed beyond `220` entries without admitting the full raw path dump.
- [x] Add the next support-substrate tranche for achievement, animation-event, camera, player-status, sound, and XML/data-loading paths.
- [x] Refresh the source-path manifest, selector metadata, workbench metadata, and local-only source-family notes.
- [x] Generate a dedicated report for the widened Phase 47 source-path layer.

## Phase 48 - Debug Workbench Catalog View

- [x] Add a compact `--catalog` mode to the native debug workbench.
- [x] Cover the new catalog command with a subprocess regression test.
- [x] Verify the catalog against the widened `159`-host workbench map.
- [x] Generate a dedicated report for the catalog-view beat.

## Phase 49 - Local Support-Substrate Humanization Sweep

- [x] Add a dedicated local-only source-tree materializer group for Phase 47 support-substrate paths.
- [x] Generate `23` support-substrate `.cpp` scaffolds under the ignored `SONIC UNLEASHED\` mirror.
- [x] Refresh the tracked local debug source-tree expansion summary to `7` groups and `125` total local-only readable `.cpp` files.
- [x] Cover the new group with a regression test.
- [x] Generate a dedicated report for the support-substrate humanization beat.

## Phase 50 - Support-Substrate Runtime Contracts

- [x] Add portable runtime contracts for achievement unlock, audio/BGM cue, and XML/data-loading support systems.
- [x] Expose the new support contracts through native C++ profiles, the C ABI profile enum, and the C# reference profile mapper.
- [x] Promote achievement, sound/BGM, and XML/data-loading source paths from archaeology-only support into contract-backed manifest entries.
- [x] Regenerate source-family selector and debug workbench metadata so `AchievementManager.cpp`, `SoundController.cpp`, and `XMLManager.cpp` are launchable support probes.
- [x] Cover the support contracts, manifest promotion, selector families, and workbench host group with a regression test.
- [x] Generate a dedicated report for the support-substrate runtime-contract beat.

## Phase 51 - Native GUI Debug Workbench

- [x] Add a native Win32 GUI debug workbench target around the existing runtime contracts and generated host catalog.
- [x] Expose group and host browsing for the current `176` workbench hosts across `11` groups.
- [x] Add windowed controls for `Run Host`, `Move Next`, `Confirm`, `Cancel`, and `Reset` over `ScreenRuntime`.
- [x] Add a `--smoke` regression path for validating the GUI executable without opening a window.
- [x] Build and launch-check `b\rr51\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for the first non-CLI workbench beat.

## Phase 52 - GUI Visual Preview And Atlas Binding

- [x] Add a native preview panel to the GUI workbench.
- [x] Bind high-confidence runtime contracts to local visual atlas sheet candidates without committing extracted PNGs.
- [x] Draw runtime visible layers, prompt rows, and timeline/progress state on the preview canvas.
- [x] Add a `--preview-smoke` regression path for validating atlas bindings without opening a window.
- [x] Build and launch-check `b\rr52\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for the first GUI visual-preview beat.

## Phase 53 - Gameplay HUD Proxy Preview Binding

- [x] Bind Sonic and Werehog gameplay-HUD previews to the recovered `ui_prov_playscreen` atlas as explicit proxy evidence.
- [x] Keep exact `ui_playscreen*` parity marked as unrecovered rather than overclaiming atlas/layout coverage.
- [x] Add bounded role-aware preview placement for counters, gauges, sidecars, transient effects, and prompt strips.
- [x] Increase the GUI preview height budget so atlas-backed HUD previews stay readable in normal desktop windows.
- [x] Extend GUI preview smoke coverage to `10` atlas candidates and `2` proxy candidates.
- [x] Build, smoke, and capture-check `b\rr53\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for the gameplay-HUD proxy-preview beat.

## Phase 54 - GUI Timeline Playback Controls

- [x] Add Play/Pause and Step controls to the native GUI workbench.
- [x] Replace immediate GUI transition settling with timer-driven `ScreenRuntime::tick(...)` playback.
- [x] Keep input-locked intro/action bands visible in the preview instead of snapping straight to Idle.
- [x] Add a `--playback-smoke` regression path for timeline advancement without opening a window.
- [x] Build, smoke, and capture-check `b\rr54\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for the first GUI timeline-playback beat.

## Phase 55 - GUI State-Aware Preview Motion

- [x] Add a state-aware preview-motion adapter over runtime overlay roles.
- [x] Drive preview motion from eased contract state progress instead of static settled rectangles.
- [x] Apply motion and alpha to overlay layers and prompt buttons while keeping canvas bounds stable.
- [x] Add a dark atlas backing fill so transparent local PNG regions do not read as blank Win32 panel background.
- [x] Add a `--motion-smoke` regression path for motion semantics without opening a window.
- [x] Build, smoke, and capture-check `b\rr55\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for the first state-aware GUI preview-motion beat.

## Phase 56 - GUI Exact-Family Preview Layouts

- [x] Add exact-family preview classification for Title, Pause, and Loading contracts.
- [x] Add Title-specific visual placement for logo/content/prompt/transient roles.
- [x] Add Pause-specific visual placement for chrome/content/prompt/transient roles.
- [x] Add Loading-specific visual placement for cinematic-frame/content/tip/controller roles.
- [x] Keep generic role projection available for gameplay HUD, support, town, camera, and other broader families.
- [x] Add a `--family-preview-smoke` regression path for exact-family placement semantics without opening a window.
- [x] Build, smoke, and capture-check `b\rr56\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for the first exact-family GUI preview-layout beat.

## Phase 57 - GUI Layout Evidence Preview Overlay

- [x] Add a compact layout-evidence table for Title, Pause, and Loading contracts.
- [x] Draw recovered layout IDs, verdicts, scene/animation counts, cue summaries, and longest parsed timelines inside the visual preview.
- [x] Extend the GUI preview footer with the active recovered layout ID.
- [x] Preserve atlas readability under structural backdrop and cinematic-frame roles.
- [x] Add `--layout-evidence-smoke` regression coverage for decoded layout facts without opening a window.
- [x] Add `--layer-fill-smoke` regression coverage for backdrop/cinematic-frame fill policy.
- [x] Build, smoke, and capture-check `b\rr57\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for the first GUI layout-evidence overlay beat.

## Phase 58 - GUI Layout Timeline Frame Preview

- [x] Add recovered longest-timeline frame counts and FPS to the compact layout evidence table.
- [x] Map active runtime progress into parsed layout timeline frame space for Title, Pause, and Loading.
- [x] Draw a frame-domain timeline bar and `Frame: current/total @ fps` readout in the GUI evidence overlay.
- [x] Add `--layout-timeline-smoke` regression coverage for frame-domain mapping without opening a window.
- [x] Build, smoke, and capture-check `b\rr58\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for the first GUI layout timeline frame-preview beat.

## Phase 59 - GUI Layout Scene Primitive Preview

- [x] Add a compact scene-primitive table for the highest keyframe-density Title, Pause, and Loading scenes.
- [x] Draw recovered scene names, keyframe counts, and per-scene frame progress over the GUI atlas preview.
- [x] Add `--layout-primitive-smoke` regression coverage for primitive counts and keyframe totals.
- [x] Build, smoke, and capture-check `b\rr59\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for the first GUI scene-primitive preview beat.

## Phase 60 - GUI Gameplay HUD Primitive Preview

- [x] Bind Sonic, Werehog, and Extra Stage HUD previews to the recovered `ui_prov_playscreen` scene primitive set.
- [x] Extend `--layout-primitive-smoke` with gameplay HUD proxy primitive counts and keyframe totals.
- [x] Build, smoke, and capture-check `b\rr60\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for the first gameplay HUD primitive preview beat.

## Phase 61 - Gameplay HUD Primitive Ownership Audit

- [x] Audit the gameplay HUD primitive scene ownership against parsed `ui_prov_playscreen` deep-analysis facts.
- [x] Correct the GUI primitive table for `so_speed_gauge`, `so_ringenagy_gauge`, `info_1`, `info_2`, `ring_get_effect`, and `bg`.
- [x] Extend `--layout-primitive-smoke` with gameplay HUD scene/keyframe ownership metrics.
- [x] Build and smoke-check `b\rr61\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated ownership-audit report.

## Phase 62 - GUI Layout Primitive Playback Cues

- [x] Attach recovered animation-bank names to layout scene primitives.
- [x] Draw scene/animation labels and per-primitive sampled frame cursors in the native GUI.
- [x] Add `--layout-primitive-playback-smoke` for gameplay HUD proxy animation/frame cursor checks.
- [x] Build, smoke, and capture-check `b\rr62\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for primitive playback cues.

## Phase 63 - GUI Layout Primitive Detail Cues

- [x] Add a readable layout primitive cue summary to the GUI detail pane.
- [x] Include primitive count, total keyframes, animation-bank names, sampled frames, and track summaries.
- [x] Add `--layout-primitive-detail-smoke` for headless Sonic HUD proxy parity checks.
- [x] Build, smoke, and GUI-control-check `b\rr63\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for primitive detail cues.

## Phase 64 - GUI Layout Primitive Channel Cues

- [x] Add a conservative primitive channel classifier for recovered track summaries.
- [x] Surface color, sprite, transform, visibility, and static tags in the overlay/detail cue paths.
- [x] Add `--layout-primitive-channel-smoke` for Sonic HUD proxy channel checks.
- [x] Build, smoke, and GUI-control-check `b\rr64\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for primitive channel cues.

## Phase 65 - GUI Layout Primitive Channel Legend

- [x] Add a compact preview legend for recovered primitive channel counts.
- [x] Show transform/color/visibility/sprite/static counts for selected host primitives.
- [x] Add `--layout-primitive-channel-legend-smoke`.
- [x] Build, smoke, and capture-check `b\rr65\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for primitive channel legend.

## Phase 66 - GUI Visual Parity Summary

- [x] Add a compact detail-pane parity summary for selected visual hosts.
- [x] Surface exact/proxy/none atlas binding, layout evidence, primitive counts, keyframes, and channel totals together.
- [x] Add `--visual-parity-smoke` for exact Title vs proxy Sonic HUD readiness checks.
- [x] Build, smoke, and GUI-control-check `b\rr66\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for visual parity summaries.

## Phase 67 - GUI Host Readiness Badges

- [x] Add compact visual readiness badges to GUI host-list labels.
- [x] Surface exact/proxy/layout/primitive/channel/contract readiness before a host is launched.
- [x] Add `--host-readiness-smoke` for Sonic proxy, Title exact-layout, and support contract-only labels.
- [x] Build, smoke, and GUI-listbox-check `b\rr67\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for host readiness badges.

## Phase 68 - GUI Renderer Blocker Cues

- [x] Add next-renderer blocker cues to the GUI visual parity summary.
- [x] Classify proxy HUD, exact layout/channel, primitive-only, layout-only, and contract-only blocker classes.
- [x] Add `--renderer-blocker-smoke` for Sonic HUD, Title, and support-substrate readiness blockers.
- [x] Build and smoke-check `b\rr68\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for renderer blocker cues.

## Phase 69 - GUI Layout Channel Sample Cues

- [x] Add renderer-facing primitive channel sample tokens for exact-family layout primitives.
- [x] Surface `scene:channels@frame/count` samples in the GUI detail pane.
- [x] Add `--layout-channel-sample-smoke` for Title, Pause, and Loading exact-family samples.
- [x] Build and smoke-check `b\rr69\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for layout channel sample cues.

## Phase 70 - GUI Layout Draw Command Descriptors

- [x] Add renderer-facing draw command descriptors for exact-family layout primitives.
- [x] Convert recovered normalized primitive rectangles into deterministic 1280x720 geometry plus sampled channel tokens.
- [x] Surface `scene:x,y,widthxheight:channels@frame/count` descriptors in the GUI detail pane.
- [x] Add `--layout-draw-command-smoke` for Title, Pause, and Loading descriptor geometry.
- [x] Build and smoke-check `b\rr70\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for layout draw command descriptors.

## Phase 71 - GUI Authored Cast Transform Descriptors

- [x] Add exact-family authored CSD cast transform descriptors to the GUI detail path.
- [x] Bind parsed `layout_deep_analysis.json` cast evidence for Title, Pause, and Loading into renderer-facing descriptors.
- [x] Surface `scene/cast:x,y,widthxheight:rrotation:scaleX,scaleY:color` descriptors in the GUI detail pane.
- [x] Add `--authored-cast-transform-smoke` for Title, Pause, and Loading authored cast transforms.
- [x] Build and smoke-check `b\rr71\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for authored cast transform descriptors.

## Phase 72 - GUI Authored Keyframe Curve Descriptors

- [x] Add exact-family authored CSD keyframe curve descriptors to the GUI detail path.
- [x] Bind parsed `layout_deep_analysis.json` keyframe evidence for Title, Pause, and Loading into first/last frame/value descriptors.
- [x] Surface `scene/animation/cast/track:kfN:firstFrame=firstValue->lastFrame=lastValue:interpolation` descriptors in the GUI detail pane.
- [x] Add `--authored-keyframe-curve-smoke` for Title, Pause, and Loading authored keyframe curves.
- [x] Build and smoke-check `b\rr72\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for authored keyframe curve descriptors.

## Phase 73 - GUI Authored Keyframe Sample Descriptors

- [x] Add exact-family authored CSD keyframe sample descriptors to the GUI detail path.
- [x] Store bounded keyframe points for Title, Pause, and Loading sample curves.
- [x] Add deterministic linear sampling for authored keyframe curves.
- [x] Surface `scene/animation/cast/track@frame=value` descriptors in the GUI detail pane.
- [x] Add `--authored-keyframe-sample-smoke` for Title, Pause, and Loading sampled keyframe values.
- [x] Build and smoke-check `b\rr73\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for authored keyframe sample descriptors.

## Phase 74 - GUI Authored Sampled Transform Descriptors

- [x] Add renderer-facing sampled transform descriptors to the GUI detail path.
- [x] Pair authored cast rectangles with sampled authored X/Y keyframe values.
- [x] Surface `scene/cast@frame:x,y,widthxheight:track=value` descriptors for Title and Loading exact-family samples.
- [x] Add `--authored-sampled-transform-smoke` for sampled Title and Loading transform descriptors.
- [x] Build and smoke-check `b\rr74\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for authored sampled transform descriptors.

## Phase 81 - GUI Asset CSD Element Bindings

- [x] Add a CSD element-binding table for selected Asset View atlas candidates.
- [x] Bind Title, Pause, Loading, and gameplay-HUD proxy hosts to package/scene/cast/subimage evidence from `layout_deep_analysis.json`.
- [x] Draw first-pass CSD element cues over Asset View atlas images.
- [x] Surface `CSD element bindings:` summaries in the GUI detail pane.
- [x] Add `--asset-csd-binding-smoke` for package/scene/cast/subimage counts.
- [x] Build and smoke-check `b\rr81\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for Asset View CSD element bindings.

## Phase 82 - GUI Asset CSD Element Navigation

- [x] Widen CSD element bindings beyond one seed cue per family.
- [x] Add `Element Prev` / `Element Next` controls.
- [x] Keep CSD element selection independent from atlas gallery selection.
- [x] Highlight the selected CSD element while keeping other markers visible.
- [x] Add `--asset-csd-navigation-smoke`.
- [x] Build and smoke-check `b\rr82\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for Asset View CSD element navigation.

## Phase 83 - GUI Asset CSD Crop Preview

- [x] Add selected-element crop descriptors derived from CSD binding footprints.
- [x] Convert selected CSD bindings into stable `1280x720` pixel rects.
- [x] Surface `CSD element crop:` in the detail pane and runtime snapshot text.
- [x] Draw a selected-element crop inset in Asset View when a local atlas image is available.
- [x] Add `--asset-csd-crop-smoke`.
- [x] Build and smoke-check `b\rr83\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for Asset View CSD crop previews.

## Phase 84 - GUI Asset CSD Subimage Draw Descriptors

- [x] Add selected CSD cast/subimage descriptors from parsed layout evidence.
- [x] Bind Sonic HUD, Extra Stage HUD, Werehog HUD, Loading, Title, and Pause selected elements to drawable cast/subimage or no-subimage evidence.
- [x] Surface `CSD cast/subimage:` in the detail pane and runtime snapshot text.
- [x] Draw a selected cast/subimage cue in Asset View.
- [x] Add `--asset-csd-subimage-smoke`.
- [x] Build and smoke-check `b\rr84\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for Asset View CSD subimage draw descriptors.

## Phase 85 - GUI Asset CSD Subimage Draw Commands

- [x] Add selected CSD source/destination draw-command descriptors.
- [x] Derive source texture pixel rects from parsed UV bounds and cast dimensions.
- [x] Derive scaled destination sizes from cast dimensions and scale values.
- [x] Surface `CSD subimage draw command:` in the detail pane and runtime snapshot text.
- [x] Draw a selected draw-command cue in Asset View.
- [x] Add `--asset-csd-draw-command-smoke`.
- [x] Build and smoke-check `b\rr85\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for Asset View CSD subimage draw commands.

## Phase 86 - GUI Asset CSD Render Plan Preview

- [x] Add selected CSD render-plan descriptors in virtual `1280x720` target space.
- [x] Project source/destination draw commands into render-target rectangles.
- [x] Surface `CSD render plan:` in the detail pane and runtime snapshot text.
- [x] Draw a mini render-target preview in Asset View.
- [x] Add `--asset-csd-render-plan-smoke`.
- [x] Build and smoke-check `b\rr86\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for Asset View CSD render plan previews.

## Phase 87 - GUI Asset CSD DDS Blit Preview

- [x] Add curated DDS source binding for the current selected CSD samples.
- [x] Add local DXT5 decode for DDS-backed source textures.
- [x] Surface `CSD DDS blit:` in the detail pane and runtime snapshot text.
- [x] Draw decoded selected source rects into the mini render-target preview when the DDS source is available.
- [x] Add `--asset-csd-dds-blit-smoke`.
- [x] Build and smoke-check `b\rr87\sward_ui_runtime_debug_gui.exe`.
- [x] Generate a dedicated report for Asset View CSD DDS blit previews.

## Phase 88 - SU UI Asset Renderer Vertical Slice

- [x] Add a separate clean native `sward_su_ui_asset_renderer.exe` product target beside the reconstruction workbench.
- [x] Add a renderer-owned screen/cast catalog for Loading, Title, and Sonic HUD seed samples.
- [x] Reuse local DDS source binding and DXT5 decode for real asset-backed blits.
- [x] Add `--renderer-smoke` for screen/cast/texture inventory verification.
- [x] Build and smoke-check `b\rr88\sward_su_ui_asset_renderer.exe`.
- [x] Keep `sward_ui_runtime_debug_gui.exe` intact as the evidence/debug workbench.
- [x] Generate a dedicated report for the clean asset renderer vertical slice.

## Phase 89 - SU UI Asset Renderer Composite Sheets

- [x] Replace the clean renderer's first screen from an isolated arrow crop to a full-screen Loading composition.
- [x] Add Main Menu and Title sheet surfaces to the renderer catalog.
- [x] Keep the old crop samples only as regression contrast, not as the first product-facing screen.
- [x] Extend `--renderer-smoke` with destination rects and full-screen cast counts.
- [x] Build and smoke-check `b\rr89\sward_su_ui_asset_renderer.exe`.
- [x] Capture-check the renderer window and verify it opens on the full Loading composition.
- [x] Generate a dedicated report for renderer composite sheets.

## Phase 90 - SU UI Renderer Navigation Shell

- [x] Add visible `Prev` / `Next` controls to the clean asset renderer.
- [x] Add a native screen-index label so the current renderer page is discoverable.
- [x] Keep keyboard cycling intact for fast inspection.
- [x] Add `--renderer-navigation-smoke` for no-window catalog/control verification.
- [x] Move the render canvas below the control chrome so resized/fullscreen windows keep the interaction model visible.
- [x] Build, smoke-check, and launch-check `b\rr90\sward_su_ui_asset_renderer.exe`.
- [x] Generate a dedicated report for the renderer navigation shell.

## Phase 91 - SU UI Renderer Atlas Gallery

- [x] Add a `VisualAtlasGallery` page to the clean renderer.
- [x] Discover local ignored `extracted_assets\visual_atlas\sheets\*.png` files at runtime.
- [x] Add visible `Atlas Prev` / `Atlas Next` controls.
- [x] Render selected atlas PNG sheets inside the clean renderer canvas.
- [x] Add `--renderer-atlas-gallery-smoke` for local sheet inventory verification.
- [x] Build, smoke-check, launch-check, and screenshot-check `b\rr91\sward_su_ui_asset_renderer.exe`.
- [x] Generate a dedicated report for the renderer atlas gallery.

## Phase 93 - SU UI Renderer Title Loop Reconstruction

- [x] Bind the clean renderer's first screen to a title-loop composition instead of a symbolic or atlas-only view.
- [x] Use the local `evmo_title_loop.sfd` preview frame as the title background substrate.
- [x] Expand `Loading/OPmovie_titlelogo_EN.dds` with the local `tools\x_decompress` path and bind its decompressed title-logo evidence.
- [x] Use the decoded local title-logo PNG preview for the visual path while keeping the decompressed DDS as source-cast evidence.
- [x] Keep `CTitleStateIntro::Update`, `UseAlternateTitleMidAsmHook`, `ui_title/bg/bg`, and `mm_title_intro` in the renderer evidence chain.
- [x] Add `--renderer-title-screen-smoke` checks for movie frame, title-logo preview, bitmap load, and in-bounds title casts.
- [x] Build, smoke-check, launch-check, and screenshot-check `b\rr93\sward_su_ui_asset_renderer.exe`.
- [x] Generate a dedicated report for renderer title-loop reconstruction.

## Phase 94 - UnleashedRecomp UI Lab Runtime Pivot

- [x] Demote the clean asset renderer to a diagnostic sidecar rather than the parity target.
- [x] Add a real UnleashedRecomp `UiLab` patch module with CLI target selection.
- [x] Register the UI Lab patch module in the UnleashedRecomp CMake source list.
- [x] Parse `--ui-lab` / `--ui-lab-screen` before runtime boot.
- [x] Attach the UI Lab to live `CTitleStateIntro::Update` and `CTitleStateMenu::Update` translated runtime states.
- [x] Document the real-runtime UI Lab architecture and the immediate next implementation order.

## Phase 95 - Runtime UI Lab Overlay Attachment

- [x] Add a visible `UiLab::DrawOverlay()` entry point to the UnleashedRecomp patch module.
- [x] Draw the UI Lab overlay inside the real ImGui runtime frame in `gpu/video.cpp`.
- [x] Surface the selected runtime target, CSD scene, source family, stage-context requirement, and hook attachment status.
- [x] Guard the overlay attachment with the UI Lab regression contract.

## Phase 96 - Runtime UI Lab Startup Prompt Bypass

- [x] Add `UiLab::ShouldBypassStartupPromptBlockers()` for lab-only frontend modal suppression.
- [x] Bypass update/save/achievement prompt checks in `PressStartSaveLoadThreadMidAsmHook()` during UI Lab runs.
- [x] Surface startup-prompt bypass status in the UI Lab overlay.
- [x] Guard the bypass attachment with the UI Lab regression contract.

## Phase 97 - Runtime UI Lab Target Controls

- [x] Add `UiLab::SelectPreviousTarget()` and `UiLab::SelectNextTarget()`.
- [x] Add visible Previous/Next target controls to the runtime overlay.
- [x] Keep target selection tied to the curated runtime target table, CSD scene, and source-family metadata.
- [x] Guard the target-control surface with the UI Lab regression contract.

## Phase 98 - Runtime UI Lab Evidence Capture

- [x] Add `--ui-lab-evidence-dir` and `--ui-lab-auto-exit` for local automated captures.
- [x] Write JSONL route/frame/state events from the real runtime UI Lab path.
- [x] Add a local-only screenshot/event capture helper for curated UI Lab targets.
- [x] Route stage-required targets through the real title/menu/loading path instead of only arming the harness.
- [x] Suppress title-menu accept carry-through when the requested real screen is the title menu itself.
- [x] Build the generated UnleashedRecomp clone and capture-check title/menu/stage-target evidence.
- [x] Guard the evidence-capture and state-forcing contract with regression tests.

## Phase 99 - Runtime UI Lab Context Observation

- [x] Add runtime evidence hooks for loading requests and loading display-type transitions.
- [x] Add CSD project creation evidence from the real `CCsdProject::Make` hook path.
- [x] Add long-observation capture snapshots for runs that need more than early/late screenshots.
- [x] Add a `-KeepRunning` capture mode so manual screen-to-screen operator sessions can keep the game alive.
- [x] Sync tracked UI Lab runtime files into the generated clone before building.
- [x] Build the generated UnleashedRecomp UI Lab after the context-observation hooks.
- [x] Capture-check a longer loading route and verify CSD/loading/frame evidence.
- [x] Guard the context-observation and long-observation contract with regression tests.

## Phase 100 - Runtime UI Lab Manual Observer

- [x] Add a passive `--ui-lab-observer` mode for real-runtime evidence capture without forcing a target route.
- [x] Keep observer mode from applying lab-only startup prompt/intro/autosave bypasses so manual navigation follows the installed runtime's normal flow.
- [x] Add overlay hiding for clean real-screen screenshots during operator-guided sessions.
- [x] Add `-Observer` / `-HideOverlay` capture helper arguments and a `manual-observer` evidence target.
- [x] Build the generated UnleashedRecomp UI Lab after the observer-mode changes.
- [x] Launch-check an observer capture and verify it writes screenshots/events without the harness killing the process.
- [x] Guard observer mode and helper routing with regression tests.

## Phase 101 - Runtime UI Lab Direct-Context Forcing Prep

- [x] Add `--ui-lab-route-policy direct-context` beside the default input-injection route policy.
- [x] Add direct title-intro state requests through the translated `sub_825811C8` contract.
- [x] Record title intro, title owner, and title-menu context fields into the JSONL evidence stream.
- [x] Add direct title-menu latch scaffolding for cursor/selection/transition fields.
- [x] Extend the capture helper with `-RoutePolicy`.
- [x] Build the generated UnleashedRecomp UI Lab after the direct-context prep.
- [x] Capture-check direct-context title/menu behavior and preserve the owner-output bridge finding.
- [x] Guard direct-context route policy and context evidence with regression tests.

## Phase 102 - Runtime UI Lab Title/Menu Direct-Context Evidence

- [x] Split direct title-intro forcing so `title-menu` arms the CSD completion byte while loading/stage routes arm owner output.
- [x] Prove `title_ctx465` is the owner-output bridge, not the title-menu switch.
- [x] Capture-check `title-menu` direct-context routing and verify `title-menu-attached`, `title-menu-context`, and `title-menu-reached`.
- [x] Capture-check `loading` direct-context routing and verify owner-output still requests real loading.
- [x] Capture-check `sonic-hud` direct-context routing and verify `CGameModeStage::ExitLoading` is observable.
- [x] Update the UI Lab pivot and whole-game gap reports with the new route boundary.
- [x] Guard CSD completion arming, owner-output gating, and direct-context evidence fields with regression tests.

## Phase 103 - Runtime UI Lab Stage HUD Target Binding

- [x] Split normal Sonic HUD from the Extra/Tornado-family `ui_prov_playscreen` target in the UI Lab target table.
- [x] Route `sonic-hud` to the real runtime `ui_playscreen` CSD project and add `extra-stage-hud` / `prov-hud` / `tornado-hud` aliases for `ui_prov_playscreen`.
- [x] Record target-CSD observation state from the real `CCsdProject::Make` path and expose it through JSONL evidence plus the overlay.
- [x] Carry the `CGameModeStage::ExitLoading` guest stage context address into UI Lab evidence for deterministic owner selection.
- [x] Capture-check `sonic-hud` direct-context routing and verify `target-csd-project-made detail="ui_playscreen"` plus `stage-target-csd-bound`.
- [x] Capture-check `extra-stage-hud` direct-context routing and verify the current route still lands on `ui_playscreen`, proving `ui_prov_playscreen` needs the Extra/Tornado owner path.
- [x] Update the UI Lab pivot and whole-game gap reports with the corrected HUD-family split.
- [x] Guard the target-table split, helper target list, stage-address hook, and target-CSD binding evidence with regression tests.

## Phase 104 - Runtime UI Lab Early-Game Alpha Scope

- [x] Add a first-class `title-options` UI Lab target for the visible options path reachable from the title menu.
- [x] Force `title-options` through the real title menu cursor index `2` and existing `OptionsMenu::Open` path instead of creating a fake options surface.
- [x] Make the capture helper default to an `early-game` target set: `title-loop`, `title-menu`, `title-options`, `loading`, and normal `sonic-hud`.
- [x] Keep `extra-stage-hud` available but move it out of the default capture path until the early-game alpha is useful.
- [x] Update the UI Lab pivot and whole-game gap reports with the one-week alpha scope boundary.
- [x] Guard the options target and capture target-set behavior with regression tests.

## Phase 105 - Runtime UI Lab Evidence-Gated Capture

- [x] Make direct-context routing the helper default for early-game alpha captures.
- [x] Add per-target required-event validation before declaring evidence success.
- [x] Refuse desktop screenshot fallback when the foreground window does not belong to the UI Lab process.
- [x] Gate late captures on required runtime evidence instead of blind timer-only capture.
- [x] Capture-check normal Sonic HUD after the real `ui_playscreen` bind.
- [x] Guard evidence-gated capture behavior with regression tests.

## Phase 106 - Runtime UI Lab Native Backbuffer Capture

- [x] Add `--ui-lab-native-capture` and `--ui-lab-native-capture-dir`.
- [x] Write local-only top-down 32-bit BMP frames from the runtime GPU readback path.
- [x] Fix D3D12/Vulkan placed-footprint readback handling for native capture.
- [x] Keep capture/evidence-only launches passive unless a target screen is explicitly requested.
- [x] Re-verify explicit routed `sonic-hud` still reaches real runtime evidence.
- [x] Guard native capture and passive observer safety with regression tests.

## Phase 107 - Runtime UI Lab Native Frame-Series Controls

- [x] Add native capture count and interval controls.
- [x] Report every native BMP in the capture-helper manifest.
- [x] Add native-only `-SkipWindowScreenshots` capture-helper mode.
- [x] Prove helper-owned native-only captures complete instead of hanging indefinitely.
- [x] Isolate the remaining blocker to native readback source/synchronization after `ui_title`.
- [x] Guard frame-series and screenshot-skip behavior with regression tests.

## Phase 108 - Runtime UI Lab Capture-Helper Stable Defaults

- [x] Split UI Lab window preparation from screenshot capture.
- [x] Make helper native BMP capture opt-in by default.
- [x] Verify default title-loop capture passes required evidence with no native BMPs emitted.
- [x] Comparison-check title-loop with and without native capture to isolate the readback blocker.
- [x] Update the UI Lab pivot and whole-game reports with the stable-default boundary.
- [x] Guard helper defaults with regression tests.

## Phase 109 - Runtime UI Lab Native Readback Source/Fence Fix

- [x] Force UI Lab native capture through the intermediary backbuffer instead of the swapchain present image.
- [x] Add a native-capture enable gate that forces the intermediary render path when capture is active.
- [x] Prevent an already-waited native-capture command fence from being marked pending for the next frame.
- [x] Rebuild the generated UnleashedRecomp UI Lab after the readback fix.
- [x] Native-capture-check `title-loop` and verify real nonblack title-screen BMPs plus clean `auto-exit`.
- [x] Native-capture-check `sonic-hud` and verify real Miles Electric / Sonic tutorial loading BMPs plus `ui_playscreen` stage binding.
- [x] Guard source selection and fence-state behavior with regression tests.

## Phase 110 - Runtime UI Lab Native Capture Signal Manifest

- [x] Add manifest-side BMP signal analysis for native UI Lab captures.
- [x] Report RGB sum, alpha sum, nonzero byte counts, dimensions, and `rgbNonBlack` for every native BMP.
- [x] Add a `nativeFrameSignalSummary` block with capture count, valid BMP count, nonblack count, all-black status, and best RGB frame metadata.
- [x] Verify a no-build native `title-loop` run records nonblack frame evidence in the manifest.
- [x] Guard native signal-manifest fields with regression tests.

## Phase 111 - Runtime UI Lab Native RGB Evidence Gate

- [x] Add `-RequireNativeRgbSignal` to the capture helper.
- [x] Fail required native-signal runs when captured BMPs are all black or missing.
- [x] Record `nativeSignalRequired` and `nativeSignalPassed` in the manifest.
- [x] Verify required-signal native `title-loop` capture passes with a nonblack best frame.
- [x] Guard the required native-signal gate with regression tests.

## Completion Audit

- [x] Re-checked `MASTER.txt` and `research_uiux/TODO_CHECKLIST.md`.
- [x] Verified there were no remaining unchecked checklist items in the then-current tracked plan before continuation phases were added.
- [x] Relocated the completed workspace and owned installed build into the project root.
- [x] Staged local external tools and runtimes under `external_tools\`.
- [x] Re-audited `MASTER.txt` and `research_uiux/TODO_CHECKLIST.md` on `2026-04-23 09:40:33 +02:00`.
- [x] Re-verified current generated outputs: `261` `ppc_recomp.*.cpp` files, `ppc_func_mapping.cpp`, `ppc_context.h`, `ppc_config.h`, `shader_cache.h`, and `shader_cache.cpp`.
- [x] Safely extracted the front-end archive cluster into `extracted_assets\ui_frontend_archives`.
- [x] Re-scanned assets with both the installed build root and extracted front-end root, confirming `MainMenu\ui_mainmenu.xncp` and `MainMenu\ui_mainmenu.yncp`.
- [x] Follow-on phase audit on `2026-04-23 10:06:44 +02:00`: safely extracted `12` additional UI-heavy archives into `extracted_assets\ui_extended_archives`.
- [x] Follow-on phase audit on `2026-04-23 10:06:44 +02:00`: confirmed `10` new `.yncp` files plus the existing `ui_mainmenu.xncp` / `ui_mainmenu.yncp`, for `12` parsed layout files total.
- [x] Follow-on phase audit on `2026-04-23 10:06:44 +02:00`: generated `research_uiux\data\layout_semantics.json` and the new semantic notes report from local extracted assets.
- [x] Follow-on phase audit on `2026-04-23 12:14:00 +02:00`: ranked `357` additional `.arl` candidates with `find_ui_archive_candidates.py`.
- [x] Follow-on phase audit on `2026-04-23 12:14:00 +02:00`: safely extracted a broader `21`-archive UI/HUD slice into `extracted_assets\ui_broader_archives`.
- [x] Follow-on phase audit on `2026-04-23 12:14:00 +02:00`: expanded extracted coverage to `1435` files, asset-index coverage to `3001` matched entries, and semantic layout coverage to `14` parsed layout files.
- [x] Follow-on phase audit on `2026-04-23 12:53:00 +02:00`: generated `research_uiux\data\layout_code_correlation.json` and `research_uiux\CODE_TO_LAYOUT_CORRELATION.md`.
- [x] Follow-on phase audit on `2026-04-23 12:53:00 +02:00`: merged the endian-variant main-menu pair into `13` semantic layout IDs for readable-code correlation work.
- [x] Follow-on phase audit on `2026-04-23 12:53:00 +02:00`: strengthened pause, loading, world-map, title-menu, status, boss-HUD, save-icon, and ending evidence by tying extracted layouts back to readable hook files and generated seams.
- [x] Follow-on phase audit on `2026-04-23 13:16:00 +02:00`: produced `research_uiux\PAUSE_STATUS_WORLDMAP_DEEP_DIVE.md`.
- [x] Follow-on phase audit on `2026-04-23 13:16:00 +02:00`: completed the focused pause/status/world-map state-machine reconstruction using extracted layout evidence, readable hooks, embedded audio cues, and generated pause/world-map seams.
- [x] Follow-on phase audit on `2026-04-23 12:50:00 +02:00`: verified that all six languages expose `50` localized archive manifests and `53` subtitle archive manifests each.
- [x] Follow-on phase audit on `2026-04-23 12:50:00 +02:00`: safely extracted a `38`-archive multilingual slice into `extracted_assets\ui_multilingual_archives`.
- [x] Follow-on phase audit on `2026-04-23 12:50:00 +02:00`: generated `research_uiux\data\multilingual_ui_coverage.json` and `research_uiux\MULTILINGUAL_UI_AND_SUBTITLE_COVERAGE.md`.
- [x] Follow-on phase audit on `2026-04-23 12:50:00 +02:00`: expanded extracted-asset scan coverage from `1015` to `1184` matched entries, for `3170` combined asset matches.
- [x] Follow-on phase audit on `2026-04-23`: extended `research_uiux\tools\inspect_xncp_yncp.py` to decode animation frame counts, channel flags, and keyframe payload summaries.
- [x] Follow-on phase audit on `2026-04-23`: generated `research_uiux\data\layout_deep_analysis.json` and `research_uiux\XNCP_YNCP_GRAPH_AND_TIMELINE_NOTES.md`.
- [x] Follow-on phase audit on `2026-04-23`: strengthened semantic/timing notes with frame-count, keyframe, and hierarchy-depth evidence from the extracted layout layer.
- [x] Follow-on phase audit on `2026-04-23`: extracted a focused `3`-archive support slice into `extracted_assets\phase16_support_archives`, yielding `1798` additional files and `9` new `.yncp` layouts.
- [x] Follow-on phase audit on `2026-04-23`: confirmed real extracted counterparts for `ui_result.yncp`, `ui_result_ex.yncp`, `ui_missionscreen.yncp`, and `ui_misson.yncp`.
- [x] Follow-on phase audit on `2026-04-23`: re-ran local asset/layout indexing to `3503` combined asset hits and `23` parsed layout files.
- [x] Follow-on phase audit on `2026-04-23`: produced `research_uiux\BOSS_RESULT_SAVE_LOAD_DEEP_DIVE.md` and marked Phase 16 complete.
- [x] Follow-on phase audit on `2026-04-23`: verified a repo-local DDS render path with `Pillow 12.2.0`; no extra atlas tool download was required.
- [x] Follow-on phase audit on `2026-04-23`: generated `extracted_assets\visual_atlas` with `23` local atlas sheets and `144` rendered DDS references.
- [x] Follow-on phase audit on `2026-04-23`: added `research_uiux\tools\build_visual_atlas.py` and `research_uiux\VISUAL_ATLAS_DOCS.md`, then marked Phase 17 complete.
- [x] Follow-on phase audit on `2026-04-23`: added `research_uiux\TEMPLATE_PACK_FOR_ORIGINAL_PROJECTS.md` and the reusable pack under `research_uiux\templates\`.
- [x] Follow-on phase audit on `2026-04-23`: documented adaptation boundaries so the template pack preserves structure and timing without copying proprietary assets/code.
- [x] Follow-on phase audit on `2026-04-23`: marked Phase 18 complete.
- [x] Follow-on phase audit on `2026-04-23`: extracted a focused `8`-archive English subtitle slice into `extracted_assets\phase19_subtitle_archives`, with `6` payload-bearing bundles and `2` stub archives.
- [x] Follow-on phase audit on `2026-04-23`: added `research_uiux\tools\analyze_subtitle_cutscene_presentation.py` and generated `research_uiux\data\subtitle_cutscene_presentation.json`.
- [x] Follow-on phase audit on `2026-04-23`: produced `research_uiux\SUBTITLE_CUTSCENE_PRESENTATION_DEEP_DIVE.md` and marked Phase 19 complete.
- [x] Follow-on phase audit on `2026-04-23`: added `research_uiux\tools\derive_visual_taxonomy.py` and generated `research_uiux\data\visual_taxonomy.json`.
- [x] Follow-on phase audit on `2026-04-23`: produced `research_uiux\BOSS_RESULT_SAVE_VISUAL_TAXONOMY.md` from the atlas/layout layer and marked Phase 20 complete.
- [x] Follow-on phase audit on `2026-04-23`: added the generic C++ runtime reference under `research_uiux\runtime_reference\`.
- [x] Follow-on phase audit on `2026-04-23`: produced `research_uiux\CODE_BACKED_RUNTIME_IMPLEMENTATION.md` and verified the standalone example build under `b\rr`.
- [x] Follow-on phase audit on `2026-04-23`: marked Phase 21 complete.
- [x] Follow-on phase audit on `2026-04-23 14:48:07 +02:00`: produced `research_uiux\WHOLE_GAME_COVERAGE_AND_GAPS.md` to answer the whole-game-source vs translated-output vs asset-extraction question with verified local counts.
- [x] Follow-on phase audit on `2026-04-23 14:48:07 +02:00`: normalized project-owned markdown headers around the larger `icon_sward` presentation and enlarged the root README icon treatment.
- [x] Follow-on phase audit on `2026-04-23 14:48:07 +02:00`: added the original-size `icon_sward` footer mark to the root `README.md` and updated `CHANGELOG.md` plus `research_uiux\README.md`.
- [x] Follow-on phase audit on `2026-04-23 14:48:07 +02:00`: no extra tool download was needed for Phase 22.
- [x] Follow-on phase audit on `2026-04-23 15:49:28 +02:00`: safely extracted `15` additional UI-heavy archive folders into `extracted_assets\phase23_crossref_archives`, yielding `13898` local files and `5` new `.yncp` layouts.
- [x] Follow-on phase audit on `2026-04-23 15:49:28 +02:00`: re-ran `scan_assets.py`, expanding the combined asset inventory to `6751` matched entries with `5023` extracted-asset entries.
- [x] Follow-on phase audit on `2026-04-23 15:49:28 +02:00`: extended `research_uiux\data\layout_code_correlation.json` to `26` merged layout IDs with `21` direct, `4` strong, and `1` contextual correlations; `ui_qte` is no longer unresolved.
- [x] Follow-on phase audit on `2026-04-23 15:49:28 +02:00`: generated `research_uiux\data\ui_archaeology_database.json` plus `research_uiux\FULL_UI_ARCHAEOLOGY_CROSS_REFERENCE.md`, grouping the current evidence into `13` screen/system families.
- [x] Follow-on phase audit on `2026-04-23 15:49:28 +02:00`: resolved `56` generated PPC symbols into the active archaeology layer, backed by a larger indexed symbol pool of `66392` translated-function entries.
- [x] Follow-on phase audit on `2026-04-23 18:08:41 +02:00`: expanded `research_uiux\runtime_reference\` with reusable `PauseMenu`, `TitleMenu`, and `AutosaveToast` reference profiles plus additional native examples.
- [x] Follow-on phase audit on `2026-04-23 18:08:41 +02:00`: added the C ABI wrapper (`runtime_c.h` / `runtime_c.cpp`) and verified `b\rr24\Release\sward_ui_runtime_c_example.exe`.
- [x] Follow-on phase audit on `2026-04-23 18:08:41 +02:00`: added the managed C# reference port under `research_uiux\runtime_reference\csharp_reference\` and verified it with `external_tools\dotnet8\dotnet.exe`.
- [x] Follow-on phase audit on `2026-04-23 18:08:41 +02:00`: produced `research_uiux\REUSABLE_PORT_KITS.md` and marked Phase 24 complete.
- [x] Follow-on phase audit on `2026-04-23`: ranked `357` remaining UI/common-flow archive candidates after the earlier archaeology passes and confirmed that the top remaining slice skewed toward localized/common-flow mirrors rather than fresh layout packages.
- [x] Follow-on phase audit on `2026-04-23`: safely extracted a focused `24`-directory common-flow/localization support slice into `extracted_assets\phase25_commonflow_archives`, yielding `338` files with `54` `.dds`, `261` `.fco`, and `15` `.fte` payloads.
- [x] Follow-on phase audit on `2026-04-23`: re-ran local asset indexing to `6840` combined matches, with `5112` extracted-asset entries and `89` indexed matches under the Phase 25 extraction root.
- [x] Follow-on phase audit on `2026-04-23`: produced `research_uiux\COMMON_FLOW_LOCALIZATION_EXTRACTION.md` and `research_uiux\data\commonflow_localization_extraction.json`, then marked Phase 25 complete.
- [x] Follow-on phase audit on `2026-04-23`: added `research_uiux\tools\label_ppc_ui_states.py` and generated `research_uiux\data\ppc_ui_state_labels.json`.
- [x] Follow-on phase audit on `2026-04-23`: produced `research_uiux\PPC_LAYOUT_STATE_LABELS.md`, covering `180` translated seams across `8` systems with function-window sampling and role-family labeling.
- [x] Follow-on phase audit on `2026-04-23`: confirmed that `subtitle_cutscene_presentation` remains the only targeted system in this pass without a resolved translated PPC seam.
- [x] Follow-on phase audit on `2026-04-23`: added the shared JSON runtime contract bundle under `research_uiux\runtime_reference\contracts\` plus the native loader under `contract_loader.hpp` / `contract_loader.cpp`.
- [x] Follow-on phase audit on `2026-04-23`: converted the native profile wrappers into compatibility shims over bundled JSON contracts and added `sward_ui_runtime_create_contract_path(...)` to the C ABI.
- [x] Follow-on phase audit on `2026-04-23`: updated the C# port to load the same copied contract JSON files with `System.Text.Json`.
- [x] Follow-on phase audit on `2026-04-23`: verified native bundled runs, native explicit contract-path runs, and the managed build/run, then marked Phase 27 complete.
- [x] Follow-on phase audit on `2026-04-23`: added the UI-focused source-path seed under `research_uiux\source_path_seeds\UI_SOURCE_PATHS_FROM_EXECUTABLE.txt` and the new mapper `research_uiux\tools\map_ui_source_paths.py`.
- [x] Follow-on phase audit on `2026-04-23`: generated `research_uiux\data\ui_source_path_manifest.json` and `research_uiux\UI_SOURCE_PATH_RECOVERY_AND_HUMANIZATION_PLAN.md`.
- [x] Follow-on phase audit on `2026-04-23`: organized `108` seeded UI-centric source paths into `16` families, with `77` paths (`71.3%`) now bridged into the archaeology layer and `37` paths (`34.3%`) already backed by runtime contracts.
- [x] Follow-on phase audit on `2026-04-23`: isolated `13` strong debug-tool host candidates and `18` still named-only UI-family paths as the clearest next humanization/debug-sandbox gaps.
- [x] Follow-on phase audit on `2026-04-23 18:35:59 +02:00`: refreshed the local-only `SONIC UNLEASHED/` source-tree scaffold directly from `Match SU OG source code folders and locations.txt`.
- [x] Follow-on phase audit on `2026-04-23 18:35:59 +02:00`: added `research_uiux\tools\build_csd_ui_foundation_map.py`, `research_uiux\CSD_UI_FOUNDATION_HUMANIZATION.md`, and `research_uiux\data\csd_ui_foundation_map.json`.
- [x] Follow-on phase audit on `2026-04-23 18:35:59 +02:00`: mapped `5` direct `CSD/*` / `Menu/*` seed paths, `5` closely related consumer/widget paths, and `12` mirrored support paths into `3` reusable abstractions.
- [x] Follow-on phase audit on `2026-04-23 18:35:59 +02:00`: refreshed `research_uiux\data\ui_source_path_manifest.json` and raised the measured UI-seed bridge to `90` paths (`83.3%`) mapped, leaving `5` named-only gaps.
- [x] Follow-on phase audit on `2026-04-23 19:35:09 +02:00`: added `research_uiux\tools\materialize_source_family_notes.py` and generated the local-only `SONIC UNLEASHED\**\*.sward.md` note layer.
- [x] Follow-on phase audit on `2026-04-23 19:35:09 +02:00`: materialized `108` local-only placement notes, split into `13` direct host anchors, `77` family-member anchors, `13` debug-host candidates, and `5` named-only placeholders.
- [x] Follow-on phase audit on `2026-04-23 19:35:09 +02:00`: added `research_uiux\LOCAL_SOURCE_FAMILY_PLACEMENT.md` and refreshed the local-only mirror metadata under `SONIC UNLEASHED\_meta\`.
- [x] Follow-on phase audit on `2026-04-23 20:01:00 +02:00`: added `research_uiux\tools\build_source_family_selector_data.py`, `research_uiux\SOURCE_PATH_NAMED_DEBUG_SELECTOR.md`, and the generated selector metadata header under `research_uiux\runtime_reference\include\sward\ui_runtime\source_family_selector_data.hpp`.
- [x] Follow-on phase audit on `2026-04-23 20:01:00 +02:00`: upgraded `sward_ui_runtime_debug_selector` so it supports `--list-families`, `--family <token>`, and direct source-family inputs such as `TitleMenu.cpp`, `HudPause.cpp`, and `WorldMapSelect.cpp`.
- [x] Follow-on phase audit on `2026-04-23 20:01:00 +02:00`: re-verified the selector locally from `b\rr33\Release\`, confirming source-family launches for title, pause, and world-map flows.
- [x] Follow-on phase audit on `2026-04-23 21:05:00 +02:00`: added `research_uiux\tools\build_broader_ui_adjacent_source_seed.py` plus the tracked broader seed `research_uiux\source_path_seeds\UI_ADJACENT_SOURCE_PATHS_FROM_MATCH_DUMP.txt`.
- [x] Follow-on phase audit on `2026-04-23 21:05:00 +02:00`: refreshed the local `ui_source_path_manifest.json` to `220` source paths across `19` families, with `110` archaeology-mapped entries, `38` contract-backed entries, `57` debug-host candidates, and `53` named-only gaps.
- [x] Follow-on phase audit on `2026-04-23 21:05:00 +02:00`: widened the local-only `SONIC UNLEASHED\**\*.sward.md` layer to `220` placement notes and refreshed the selector metadata so aliases such as `GameModeBoot.cpp`, `EndingManager.cpp`, and `MainMenuManager.cpp` now resolve where contracts already exist.
- [x] Follow-on phase audit on `2026-04-23`: tightened `research_uiux\tools\map_ui_source_paths.py` so pause/help dispatch, stage-change sequencing, town dispatch, and free/replay camera paths no longer sit in the same generic shell buckets.
- [x] Follow-on phase audit on `2026-04-23`: added `research_uiux\tools\build_frontend_shell_recovery.py`, `research_uiux\data\frontend_shell_recovery.json`, and `research_uiux\FRONTEND_SHELL_AND_DEBUG_HOST_RECOVERY.md`.
- [x] Follow-on phase audit on `2026-04-23`: refreshed the broader `ui_source_path_manifest.json` to `220` source paths with `118` archaeology-mapped entries, `42` contract-backed entries, `60` debug-host candidates, and `42` named-only gaps.
- [x] Follow-on phase audit on `2026-04-23`: narrowed the new shell/debug pass to `46` targeted host paths with `13` archaeology bridges, `4` contract-backed dispatch anchors, and `10` still shell-only paths after the tighter recovery sweep.
- [x] Follow-on phase audit on `2026-04-23`: added `research_uiux\tools\materialize_humanized_debug_hosts.py`, generated `13` local-only readable `.cpp` host scaffolds under `SONIC UNLEASHED\`, and produced `research_uiux\data\local_named_translated_ownership.json` plus `research_uiux\LOCAL_NAMED_TRANSLATED_OWNERSHIP.md`.
- [x] Follow-on phase audit on `2026-04-23`: added `research_uiux\runtime_reference\contracts\subtitle_cutscene_reference.json` and extended the bundled profile layers across native C++, the C ABI, and the C# managed reference port.
- [x] Follow-on phase audit on `2026-04-23`: refreshed the broader source-path bridge to `74 / 220` contract-backed paths (`33.6%`) and widened selector-family coverage so `InspirePreview.cpp` now resolves through the bundled runtime layer.
- [x] Follow-on phase audit on `2026-04-23`: added `research_uiux\tools\build_debug_workbench_data.py`, generated `research_uiux\data\debug_workbench_host_map.json`, and added the native `sward_ui_runtime_debug_workbench` executable.
- [x] Follow-on phase audit on `2026-04-23`: verified the native workbench and selector under `b\rr38`, including `GameModeMenuSelectDebug.cpp` and `InspirePreview.cpp`, then re-verified the C# managed reference with the new subtitle/cutscene profile.
- [x] Follow-on phase audit on `2026-04-23`: added bundled gameplay-HUD contracts for Sonic, Werehog, Extra Stage, Super Sonic, and boss/final HUD ownership across the native runtime, C ABI, and C# managed reference layers.
- [x] Follow-on phase audit on `2026-04-23`: refreshed the broader source-path bridge to `88 / 220` contract-backed paths (`40.0%`) while keeping `118` archaeology-mapped paths, `60` debug-host candidates, and `42` named-only gaps.
- [x] Follow-on phase audit on `2026-04-23`: widened the generated selector metadata to `12` launch families and the generated workbench map to `40` host entries, including gameplay-HUD and stage-test groups.
- [x] Follow-on phase audit on `2026-04-23`: verified the selector and workbench locally under `b\rr41`, including `HudSonicStage.cpp`, `HudEvilStage.cpp`, `HudExQte.cpp`, `BossHudSuperSonic.cpp`, and `GameModeStageForwardTest.cpp`.
- [x] Follow-on phase audit on `2026-04-23`: added `research_uiux\tools\materialize_local_debug_source_tree.py`, generated `research_uiux\data\local_debug_source_tree_expansion.json`, and widened the local-only readable source tree to `68` `.cpp` scaffolds under `SONIC UNLEASHED\`.
- [x] Follow-on phase audit on `2026-04-24`: added the bundled `frontend_sequence_shell_reference.json` contract and extended the native runtime, C ABI, and C# managed reference layers with `FrontendSequenceShell`.
- [x] Follow-on phase audit on `2026-04-24`: refreshed the broader source-path manifest to `220` paths with `163` archaeology-mapped entries, `154` contract-backed entries, `57` debug-host candidates, and `0` named-only gaps.
- [x] Follow-on phase audit on `2026-04-24`: widened the selector to `16` launch families and the workbench to `133` hosts across `10` groups, including the new frontend-sequence host bucket.
- [x] Follow-on phase audit on `2026-04-24`: deepened the local-only readable source mirror to `102` `.cpp` scaffolds under `SONIC UNLEASHED\`, including `12` sequence-shell scaffolds and `HUD\Item\HudItemGet.cpp`.
- [x] Follow-on phase audit on `2026-04-24`: widened the curated source-path manifest to `269` paths across `24` families, with `212` archaeology/support-mapped entries, `186` contract-backed entries, `57` debug-host candidates, and `0` named-only gaps.
- [x] Follow-on phase audit on `2026-04-24`: added the `23`-path local-only support-substrate humanization group and raised the local mirror to `125` readable `.cpp` scaffolds without publishing `SONIC UNLEASHED\`.
- [x] Follow-on phase audit on `2026-04-24`: materialized `269` local-only placement notes under `SONIC UNLEASHED\` while keeping the mirror out of git.
- [x] Follow-on phase audit on `2026-04-24`: widened the debug workbench host map to `159` hosts across `10` groups, with the camera/replay group expanded to `30` presentation-controller hosts.
- [x] Follow-on phase audit on `2026-04-24`: added `--catalog` to `sward_ui_runtime_debug_workbench.exe` so the `159`-host map can be inspected by group, contract, and sample host before launching a specific source-family probe.
- [x] Follow-on phase audit on `2026-04-24`: added achievement, audio/BGM, and XML/data-loading runtime contracts, raising contract-backed source-path coverage to `203 / 269` (`75.5%`) and the debug workbench to `176` hosts across `11` groups.
- [x] Follow-on phase audit on `2026-04-24`: verified the catalog view under `b\rr48`, including the widened `Player3DBossCamera.cpp` camera/presentation host.
- [x] Follow-on phase audit on `2026-04-24`: added and verified the native Win32 `sward_ui_runtime_debug_gui.exe` under `b\rr51`, giving the current workbench its first proper non-CLI operator shell.
- [x] Follow-on phase audit on `2026-04-24`: added and verified the GUI preview panel under `b\rr52`, with `8` local atlas candidates plus runtime layer/prompt/timeline overlay drawing.
- [x] Follow-on phase audit on `2026-04-24`: added and verified the gameplay-HUD proxy atlas binding under `b\rr53`, with `10` atlas candidates, `2` marked proxy candidates, and a captured `SonicMainDisplay.cpp` preview using `exstagetails_common__ui_prov_playscreen.png`.
- [x] Follow-on phase audit on `2026-04-24`: added and verified GUI timeline playback controls under `b\rr54`, with Play/Pause, Step, timer-driven contract ticks, and a captured `SonicMainDisplay.cpp` Intro playback frame.
- [x] Follow-on phase audit on `2026-04-24`: added and verified GUI state-aware preview motion under `b\rr55`, with eased role/state offsets, prompt/layer alpha, a dark atlas backing fill, and a captured `SonicMainDisplay.cpp` Intro preview frame.
- [x] Follow-on phase audit on `2026-04-24`: added and verified exact-family GUI preview layouts under `b\rr56`, with Title, Pause, and Loading placement adapters plus a captured `GameModeMainMenu_Test.cpp` title Intro preview frame.
- [x] Follow-on phase audit on `2026-04-24`: added and verified GUI layout evidence overlays under `b\rr57`, with `ui_mainmenu`, `ui_pause`, and `ui_loading` parsed layout facts visible in the native preview and a captured `GameModeMainMenu_Test.cpp` title evidence frame.
- [x] Follow-on phase audit on `2026-04-24`: added and verified GUI layout timeline frame previews under `b\rr58`, with Title/Pause/Loading longest timeline frame domains and a captured `GameModeMainMenu_Test.cpp` title Intro frame readout.
- [x] Follow-on phase audit on `2026-04-24`: added and verified GUI layout scene primitive previews under `b\rr59`, with keyframe-density primitives for Title/Pause/Loading and a captured `GameModeMainMenu_Test.cpp` title primitive overlay.
- [x] Follow-on phase audit on `2026-04-24`: added and verified gameplay HUD primitive previews under `b\rr60`, with Sonic/Werehog/Extra Stage bindings to the recovered `ui_prov_playscreen` primitive set and a captured `SonicMainDisplay.cpp` proxy HUD primitive overlay.
- [x] Follow-on phase audit on `2026-04-24`: added and verified gameplay HUD primitive ownership auditing under `b\rr61`, with `so_speed_gauge`, `so_ringenagy_gauge`, `ring_get_effect`, and `bg` scene/keyframe metrics smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI layout primitive playback cues under `b\rr62`, with recovered gameplay HUD animation-bank labels and sampled frame cursors smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI layout primitive detail cues under `b\rr63`, with the Sonic HUD primitive parity summary visible through the native detail pane.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI layout primitive channel cues under `b\rr64`, with Sonic HUD transform/color/visibility/static classifications smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI layout primitive channel legends under `b\rr65`, with Sonic HUD transform/color/visibility/sprite/static counts visible in the preview.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI visual parity summaries under `b\rr66`, with exact Title and proxy Sonic HUD readiness smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI host readiness badges under `b\rr67`, with native host listbox labels smoke- and control-checked.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI renderer blocker cues under `b\rr68`, with Sonic HUD, Title, and support-substrate next-renderer blockers smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI layout channel sample cues under `b\rr69`, with Title, Pause, and Loading exact-family samples smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI layout draw command descriptors under `b\rr70`, with Title, Pause, and Loading exact-family geometry descriptors smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI authored cast transform descriptors under `b\rr71`, with Title, Pause, and Loading parsed CSD cast descriptors smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI authored keyframe curve descriptors under `b\rr72`, with Title, Pause, and Loading parsed CSD keyframe descriptors smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI authored keyframe sample descriptors under `b\rr73`, with Title, Pause, and Loading sampled CSD keyframe values smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI authored sampled transform descriptors under `b\rr74`, with Title and Loading sampled CSD transform values smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI authored sampled transform preview markers under `b\rr75`, with Title and Loading marker geometry smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI authored sampled draw commands under `b\rr76`, with Title and Loading renderer-facing command geometry smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI authored sampled channel commands under `b\rr77`, with Pause `Color` alpha state smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI authored sampled channel evaluation under `b\rr78`, with Title/Pause/Loading sampled alpha, visibility, and cast-space deltas smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI asset viewer mode under `b\rr79`, with `22` local atlas sheet PNGs and the first unobstructed atlas inspection path smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI asset root discovery plus atlas gallery navigation under `b\rr80`, keeping Asset View populated from both repo-root and build-dir launches.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI asset CSD element bindings under `b\rr81`, tying Title, Pause, Loading, and gameplay-HUD proxy atlas candidates to package/scene/cast/subimage evidence.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI asset CSD element navigation under `b\rr82`, widening the binding set to `41` package/scene/cast entries with selected-element highlighting and `Element Prev` / `Element Next` controls.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI asset CSD crop previews under `b\rr83`, with selected-element `1280x720` crop rectangles for Sonic HUD, Loading, Title, and Pause smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI asset CSD subimage draw descriptors under `b\rr84`, with Sonic HUD, Loading, Title, and Pause selected cast/subimage evidence smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI asset CSD subimage draw commands under `b\rr85`, with Sonic HUD, Loading, Title, and Pause source/destination draw descriptors smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI asset CSD render plans under `b\rr86`, with Sonic HUD, Loading, Title, and Pause virtual `1280x720` target-space rectangles smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified GUI asset CSD DDS blit previews under `b\rr87`, with local DXT5 source texture decode for Sonic HUD, Loading, and Title smoke-guarded.
- [x] Follow-on phase audit on `2026-04-25`: added and verified the separate clean SU UI asset renderer under `b\rr88`, with Loading, Title, and Sonic HUD local DDS blits smoke-guarded.
- [x] Follow-on phase audit on `2026-04-26`: corrected the clean renderer under `b\rr89` to open on a full-screen Loading composition and widened it to `5` screen samples / `8` local DDS blits.
- [x] Follow-on phase audit on `2026-04-26`: added visible screen navigation for the clean renderer under `b\rr90`, including `Prev` / `Next`, screen-index labeling, and no-window navigation smoke.
- [x] Follow-on phase audit on `2026-04-26`: added local visual-atlas gallery navigation under `b\rr91`, keeping `22` ignored atlas sheets inspectable without publishing proprietary PNG outputs.
- [x] Follow-on phase audit on `2026-04-26`: corrected the clean renderer product lane under `b\rr92` so it opens on `SonicHudReconstruction` instead of the atlas gallery, with `7` screen samples, `16` local DDS-backed casts, and a family-specific readable Sonic HUD reconstruction path smoke-guarded.
- [x] Follow-on phase audit on `2026-04-26`: corrected the clean renderer title lane under `b\rr93` so it opens on `TitleLoopReconstruction`, with `8` screen samples, `20` DDS-backed evidence casts, local `evmo_title_loop.sfd` frame evidence, decompressed `OPmovie_titlelogo_EN` evidence, and a screenshot-checked title composition.
- [x] Follow-on phase audit on `2026-04-27`: added UI Lab direct-context route policy prep, direct title-intro state requests, title-menu context evidence, capture-helper route-policy selection, and regression coverage so title/loading/stage routing can move from input guessing toward translated state-field forcing.
- [x] Follow-on phase audit on `2026-04-27`: split title-menu direct forcing onto the real CSD completion byte, kept owner-output for loading/stage routes, captured title-menu/loading/sonic-hud direct-context evidence, and marked stage/HUD owner selection as the next blocker.
- [x] Follow-on phase audit on `2026-04-27`: narrowed the first UI Lab alpha to early-game-visible title loop, title menu, title options, loading, and normal Sonic HUD, with `title-options` routed through the real title-menu options path.
- [x] Follow-on phase audit on `2026-04-27`: hardened the UI Lab capture helper with direct-context defaults, per-target evidence validation, foreground-owned screenshot fallback, evidence-gated late capture, and a verified real-runtime Sonic HUD late frame after `ui_playscreen` binding.
- [x] Follow-on phase audit on `2026-04-27`: added native UI Lab backbuffer capture, fixed D3D12/Vulkan readback-copy handling, and made evidence/native-capture-only launches passive observer runs unless a target screen is explicitly requested.
- [x] Follow-on phase audit on `2026-04-27`: extended native UI Lab capture with count/interval frame-series controls, manifest reporting for all native BMP captures, native-only `-SkipWindowScreenshots` runs, and local evidence proving the next blocker is black/stalled readback timing after `ui_title` is reached.
- [x] Follow-on phase audit on `2026-04-27`: split capture-helper window preparation from screenshot capture, made native BMP capture opt-in while readback is black/stall-prone, and comparison-verified title-loop screenshots/events remain stable through auto-exit without native readback.
- [x] Follow-on phase audit on `2026-04-27`: fixed native UI Lab readback to copy from the intermediary backbuffer, guarded already-waited capture fences from frame-reuse stalls, and verified real nonblack native BMPs for `title-loop` plus `sonic-hud`.
- [x] Follow-on phase audit on `2026-04-27`: added native BMP signal stats and manifest summaries so title-loop captures now self-report black/nonblack frame usefulness and strongest RGB frame metadata.
- [x] Follow-on phase audit on `2026-04-27`: added the required native RGB signal gate so future captures can fail explicitly when runtime events pass but native visual evidence is all black or missing.
- [x] Follow-on phase audit on `2026-04-27`: added target-aware native BMP scoring plus per-target capture cadence plans, verified `loading` now selects a real `NOW LOADING` frame from `loading display active`, and re-ran the RGB-gated early-game target set through the real runtime.
- [x] Phase 113 on `2026-04-27`: gated title-menu native capture on real post-Press-Start owner/CSD readiness plus settled `CTitleStateMenu` context (`context_472=0`, `context_phase=0`, `menu_cursor=1`, `stable_frames=40`), removed the disproven menu-level accept route, and verified focused plus full early-game RGB-gated captures select the readable real `CONTINUE` menu frame.
- [x] Phase 114 on `2026-04-27`: inspected the local `UnleashedRecomp-debug-menu/` fork as reference-only Reddog debug UI, ignored it from git, ported its useful operator-shell ideas into SWARD UI Lab (F1-visible shell, draggable debug icon, window list, counter/view/export/debug-draw windows), stabilized title-menu cursor/hold after the real visual latch, and verified full hidden/native early-game captures still select real runtime title/loading/HUD frames.
- [x] Phase 115 on `2026-04-27`: made the useful operator fork features default-open in SWARD UI Lab, added direct guest-memory SGlobals debug toggles, wrote cadence/key-latch `ui_lab_live_state.json` snapshots for direct operator reads, added Stage/HUD ready events (`sonic-hud-ready` plus generic `stage-target-ready`), and verified visible operator smoke plus full hidden/native early-game captures.
- [x] Phase 116 on `2026-04-27`: fully harvested the remaining useful debug-menu fork API surfaces into a tracked typed-field map (`CSD`, `SWA/CSD`, `SWA/HUD`, `SWA/System`, title/game-mode, and Reddog operator pieces), added a default UI Lab Windows named-pipe live bridge for `state` / `events` / `route` / `reset` / `set-global` / `capture` commands, widened `ui_lab_live_state.json` with fork-derived typed fields and SGlobals values, and kept native BMP plus JSONL evidence as the oracle.
- [x] Phase 117 on `2026-04-27`: added the repo-safe `query_unleashed_recomp_ui_lab_bridge.ps1` operator client, taught the capture helper `-UseLiveBridgeReadiness`, promoted debug-menu fork fields into live `typedInspectors` for CSD motion state, loading display type, title cursor/menu owner, and Sonic HUD owner-latch fields, then verified direct pipe reads plus focused live-bridge/native title-menu and title-options captures while documenting the remaining combined full-sweep title-menu route stability gap.
- [x] Phase 118 on `2026-04-27`: hardened full-sweep live-bridge route stability with per-target unique named pipes, centralized route latch resets/generation counters, a direct `route-status` command, and title-menu live readiness that requires durable JSONL `title-menu-visible`; rebuilt the real UI Lab runtime and verified a combined early-game live-bridge/native sweep where `title-loop`, `title-menu`, `title-options`, `loading`, and `sonic-hud` all passed with RGB-nonblack native BMP evidence.
- [x] Phase 119 on `2026-04-27`: promoted the real `CCsdProject::Make` CSD project/scene/node/layer traversal into the live bridge, added first-class pause/general-window/save-icon owner inspectors, exposed Sonic HUD CSD owner paths for `m_rcPlayScreen` and gauge scenes while marking the raw `CHudSonicStage` owner pointer hook as pending, and kept native BMP capture as visual confirmation rather than the state driver.
- [x] Phase 120 on `2026-04-27`: added the raw `CHudSonicStage` owner object hook, promoted owner-only `sonic-hud-owner-hooked` into live evidence with `rawOwnerFieldsReady` kept separate, added a deterministic `pause` stage-harness route with scoped Start injection, made pause readiness require real `CHudPause` owner visibility before `pause-target-ready` / `pause-ready` can pass, and bounded native-capture sessions with a clean post-BMP auto-exit event.
- [x] Phase 121 on `2026-04-28`: added Sonic HUD owner maturation sampling around `sub_824D89B0`, `sub_824D9308`, and `sub_824D95F8`, documented why fork-header `CHudSonicStage` embedded CSD fields remain null while the real CSD tree resolves `ui_playscreen`, routed `tutorial` through the real Sonic HUD owner path, made stage-title owner direct-state fallback diagnostic opt-in, and stabilized unattended title/stage routing with foreground-verified scan-code `SendInput` control automation for `ENTER/W/A/S/D/Q/E` plus live-bridge/native BMP proof.
- [x] Phase 122 on `2026-04-28`: added a repo-safe SGFX reusable template catalog for `title-menu`, `loading`, `sonic-hud`, and `tutorial`, carrying real-runtime evidence events, timing bands, state flow, input-lock policy, layer roles, and an explicit asset policy so Sonic assets can render as local placeholders now while custom SGFX assets replace them through named slots later; built `sward_sgfx_template_catalog` under `b\rr122` and smoke-verified the catalog/details without committing proprietary assets.
- [x] Phase 123 on `2026-04-28`: wired the SGFX template catalog into the clean SU UI asset renderer so `--template title-menu/loading/sonic-hud/tutorial` selects Sonic placeholder-backed screens, binds named local placeholder asset slots, overlays readiness/timing context in the preview path, and exposes `--sgfx-template-smoke` for template/slot/timeline verification under `b\rr123` without committing extracted Sonic assets.
- [x] Phase 124 on `2026-04-28`: added the CSD-driven local SU UI pipeline viewer path so `sward_su_ui_asset_renderer.exe --csd-pipeline-smoke` loads ignored local `layout_deep_analysis.json`, maps real recovered `ui_mainmenu`, `ui_loading`, and `ui_prov_playscreen` scene/timeline evidence into SGFX-replaceable slots, overlays pipeline/runtime comparison lines in the interactive template view, and keeps native/runtime evidence as the oracle while Sonic assets remain local-only placeholders.
- [x] Phase 125 on `2026-04-28`: promoted the local SU UI pipeline viewer from digest-level CSD facts into drawable traversal, so `--csd-drawable-smoke` walks scene IDs, cast groups, cast dictionaries, material subimage bindings, and subimage UVs for title-menu/loading/Sonic HUD/tutorial, resolves local DDS source rectangles, exports sampled transform/destination draw commands plus SGFX slot labels, and makes interactive `--template ...` previews render from CSD command streams before falling back to older hand-placed diagnostics.
- [x] Phase 126 on `2026-04-28`: added CSD timeline/keyframe playback smoke coverage for title-menu/loading/Sonic HUD/tutorial by resolving timeline scenes and animation banks, sampling finite numeric keyframes at explicit frame anchors, applying matching transform samples back onto CSD drawable commands, and emitting `rendered_frame_compare=` lines that tie sidecar sampled frames to available native BMP/runtime evidence without pretending the local renderer replaces UnleashedRecomp.
- [x] Phase 127 on `2026-04-28`: added offscreen CSD rendered-frame comparison so `--csd-render-compare-smoke` renders title-menu/loading/Sonic HUD/tutorial CSD draw streams into ignored local BMPs, applies base CSD color/alpha with source-over blending plus timeline transform samples where scenes match, resolves latest native BMP oracle frames, writes `out/csd_render_compare/phase127/csd_render_compare_manifest.json`, and reports hard sampled-pixel RGB deltas while keeping UnleashedRecomp native captures as the proof gate.
- [x] Phase 128 on `2026-04-28`: tightened CSD material/channel semantics in the offscreen compare path by switching packed colors to Shuriken-compatible RGBA order, promoting cast flags for alpha/additive blend, linear filtering, and mirror semantics, reporting unresolved packed Color/Gradient timeline blockers instead of hiding `NaN` channels, aligning native BMP comparison through a centered 16:9 crop, and writing the upgraded manifest/BMP set under ignored `out/csd_render_compare/phase128/`.
- [x] Phase 129 on `2026-04-28`: preserved raw CSD keyframe payloads in the local ignored layout extractor, decoded packed Color/Gradient timeline channels into RGBA samples, replaced the offscreen compare renderer's GDI+ color-average path with a software ARGB quad compositor for per-vertex gradient color plus `src-alpha/one` additive support, and wrote the upgraded manifest/BMP set under ignored `out/csd_render_compare/phase129/`.
- [x] Phase 130 on `2026-04-28`: tightened sampler/filter and native frame-registration parity by adding a named `csd-point-seam` sampler approximation with sample counters, searched centered `16:9` native BMP crop registration, refreshed real-runtime native captures for `title-menu`, `loading`, `sonic-hud`, and `tutorial`, and wrote the upgraded manifest/BMP set under ignored `out/csd_render_compare/phase130/`.
- [x] Phase 131 on `2026-04-28`: added material/shader parity triage by writing per-target diff BMPs, computing registered full-frame `1280x720` deltas, recording render/native coverage ratios, and classifying `sonic-hud` plus `tutorial` high deltas as missing stage/world backbuffer composition rather than primarily CSD sampler error, with the upgraded manifest/BMP set under ignored `out/csd_render_compare/phase131/`.
- [x] Phase 132 on `2026-04-28`: added UI-layer-aware CSD diffing by emitting rendered-command coverage masks, writing per-target `*_ui_layer_diff.bmp` outputs, recording masked pixel counts and `ui_layer_delta` metrics, and showing that `tutorial` improves under the mask while `sonic-hud` still needs deeper HUD scene/timing/material or runtime UI-only capture work, with the upgraded manifest/BMP set under ignored `out/csd_render_compare/phase132/`.
- [x] Phase 133 on `2026-04-28`: added Sonic HUD-specific parity archaeology by reading the latest live-bridge `sonic-hud` state inside the CSD render-compare lane, proving runtime `ui_playscreen` has `13` scenes / `2` nodes / `209` layers at `stageReadyFrame=721`, labeling the local drawable `ui_prov_playscreen.yncp` path as a proxy while exact `ui_playscreen.yncp` remains unrecovered, and emitting scene/cast coverage diagnostics that show only `so_speed_gauge` is locally rendered while most real normal Sonic HUD scenes are missing from the sidecar.
- [x] Phase 134 on `2026-04-28`: added an exact `ui_playscreen` runtime CSD tree export lane for Sonic HUD recovery, widened live-bridge CSD tree layer sampling to keep later scenes such as `so_speed_gauge`, refreshed focused `sonic-hud` runtime evidence with `13` scenes / `2` nodes / `209` runtime layers and `203` exported layer samples, wrote ignored local `out/csd_runtime_exports/phase134/ui_playscreen_runtime_tree.json`, and kept the status honest: exact loose `ui_playscreen.yncp` is still unrecovered, but the runtime scene/node/layer tree is now repo-safe evidence for the next compositor/export pass.
- [x] Phase 135 on `2026-04-28`: archive-probed the installed local game files with HedgeArcPack, recovered exact ignored local `Sonic/ui_playscreen.yncp` plus Sonic HUD DDS companions from `Sonic.ar.00`, regenerated ignored `layout_deep_analysis.json`, promoted `sonic-hud` / `tutorial` sidecar bindings from proxy `ui_prov_playscreen.yncp` to exact `ui_playscreen.yncp`, added BGRA8 DDS decoding for exact `ui_ps1_gauge1.dds`, and added `--export-runtime-csd-materials --template sonic-hud` to bind the live runtime tree to exact local material/subimage/timeline data (`167 / 203` exported layer samples resolved; the remaining `36` are structural/group layers).
- [x] Phase 136 on `2026-04-28`: built the full normal Sonic HUD compositor/reference export from exact `ui_playscreen` material evidence, covering all `13` live runtime scenes with `209` runtime layers, `203` stored layer samples, `167` drawable material/timeline samples, and `36` structural layers; added local-only `out/csd_runtime_exports/phase136/ui_playscreen_hud_compositor.json` plus generated readable `ui_playscreen_hud_reference.hpp` with `CHudSonicStage`, `sub_824D9308`, scene activation events, SGFX slots, and timeline names for portable source reconstruction.
- [x] Phase 137 on `2026-04-28`: promoted the generated Sonic HUD compositor reference into hand-written repo-safe reusable source under `runtime_reference`, adding `sonic_hud_reference.hpp/.cpp` plus `sward_sonic_hud_reference_catalog`; captured `CHudSonicStage` ownership, render order, scene activation policy, SGFX material slots, and timeline sampling for all `13` `ui_playscreen` scenes without depending on Sonic assets.
- [x] Phase 138 on `2026-04-28`: wired the interactive SU UI asset renderer's Sonic HUD lane to the Phase 137 `ui_playscreen` reference policy, rendering the exact scene-policy stack from local CSD draw commands instead of the old proxy/debug HUD card, suppressing the large SGFX template panels for Sonic HUD/tutorial, honoring live drawable-layer caps such as `add/u_info` `5 / 10`, and adding `--renderer-sonic-hud-reference-smoke` as the bounded proof path.
- [x] Phase 139 on `2026-04-28`: gave title-menu, loading, title-options, and pause the same no-proxy-card treatment in the interactive SU UI asset renderer by adding compact CSD reference viewer lanes over recovered `ui_mainmenu`, `ui_loading`, and `ui_pause` scene stacks; added missing local texture bindings for loading/pause materials; wrote offscreen viewer-frame BMP/diff/manifest outputs under ignored `out/viewer_render_compare/phase139/`; and smoke-verified the viewer path itself against native BMP evidence where available.
- [x] Phase 140 on `2026-04-28`: upgraded the title/loading/options/pause reference viewer lanes to sample each scene stack's recovered CSD timeline/keyframes, promoted source-free structural casts such as pause `bg` / `text_area` into drawable quads instead of zero-command placeholders, and added `--renderer-reference-policy-export-smoke` to emit ignored local clean reusable screen-policy source for scene activation, transition/input-lock bands, render order, material slots, and SGFX-replaceable slots under `out/csd_runtime_exports/phase140/`.
- [x] Phase 141 on `2026-04-28`: promoted the Phase 140 generated title/loading/options/pause screen-policy export into tracked hand-written reusable runtime source with `frontend_screen_reference.hpp/.cpp` plus `sward_frontend_screen_reference_catalog`, preserving scene activation, transition/input-lock timing, render order, material slots, SGFX-replaceable slots, timeline sampling, and source-free structural pause quads without depending on local Sonic assets.
- [x] Phase 142 on `2026-04-28`: rewired the title/loading/options/pause viewer lanes to consume the tracked `frontend_screen_reference` policy directly instead of duplicate hard-coded lane tables, added first-class frontend material semantics and runtime-alignment descriptors for active screen/scene stack/motion frame/cursor owner/transition/input-lock state, and kept the offscreen/native viewer compare path as visual evidence while shader-perfect/UI-only-oracle work remains explicitly separate.
- [x] Phase 143 on `2026-04-28`: fed the policy-driven title/loading/options/pause viewer lanes from latest UI Lab `ui_lab_live_state.json` snapshots when available, reporting live active screen, route/motion, frame, title cursor, loading display type, pause owner status, transition/input-lock readiness, live-state path, and per-field provenance while keeping tracked `frontend_screen_reference` policy as the scene-stack/material fallback.
- [x] Phase 144 on `2026-04-28`: added a bounded direct named-pipe `state` probe to the policy-driven frontend viewer lanes so running UI Lab sessions can feed active screen/motion/frame/cursor/readiness alignment directly through the live bridge, while stale/missing/target-mismatched pipes fall back explicitly to latest `ui_lab_live_state.json` snapshots; `--renderer-live-bridge-alignment-smoke` reports direct bridge vs snapshot fallback provenance per lane.
- [x] Phase 145 on `2026-04-28`: added the read-only `ui-oracle` live-bridge command as the first runtime UI-only oracle seed, exposing active screen/scenes/motion/frame/cursor/transition/input-lock fields plus the live runtime CSD tree under `uiLayerOracle`, taught the repo-safe bridge client and sidecar viewer smoke path to query it with state/snapshot fallback, and kept the true GPU/UI draw-list capture explicitly pending via `runtimeDrawListStatus`.
- [x] Phase 146 on `2026-04-28`: promoted `ui-oracle` / live-state snapshot frame evidence from reporting-only diagnostics into the frontend viewer playback clock, so title/loading/options/pause reference lanes now sample CSD timelines from runtime-derived frame numbers (`ui-oracle-runtime-frame` modulo each scene timeline) and `--renderer-ui-oracle-playback-smoke` proves the selected per-scene frames before the next true runtime draw-list/UI-only oracle beat.
- [x] Phase 147 on `2026-04-28`: added a runtime-aligned frontend drawable oracle seed for title-menu/loading/options/pause: `--renderer-ui-drawable-oracle-smoke` combines `ui-oracle` / `state` / latest live-state active screen evidence with local CSD material draw commands, reports active project, active scene count, per-scene runtime path, sampled timeline frame, command/texture coverage, and keeps `gpu_draw_list_status=pending` explicit until a true runtime GPU/UI draw-list capture exists.
- [x] Phase 148 on `2026-04-28`: added the read-only `ui-draw-list` live-bridge command backed by runtime `CCsdPlatformMirage::Draw` / `DrawNoTex` CSD platform hooks, sampling live UI draw calls with layer paths, cast-node pointers, vertex buffers, textured/no-texture classification, color samples, screen-space rectangles, and explicit `runtime CSD platform draw hook; GPU backend submit pending` status; rebuilt the real UI Lab runtime and proved a focused title-menu probe returns live draw calls such as `ui_title/bg/bg` while keeping full backend GPU submit capture as the next material/shader parity step.
- [x] Phase 149 on `2026-04-28`: wired the sidecar SU UI viewer to consume the `ui-draw-list` live bridge directly via `--renderer-ui-draw-list-triage-smoke`, added runtime CSD platform draw-call parsing and rectangle-vs-local-CSD command triage for title-menu/loading/options/pause, kept offline runs honest with `runtime_calls=0`, and proved a live title-menu probe returns direct `ui-draw-list` data (`65` runtime calls / `65` runtime rectangles) against `75` local title-menu CSD commands while backend GPU submit capture remains pending.
- [x] Phase 150 on `2026-04-29`: added the read-only `ui-gpu-submit` live-bridge command backed by render-thread backend submit hooks in `ProcDrawPrimitive`, `ProcDrawIndexedPrimitive`, and `ProcDrawPrimitiveUP`, sampling texture/sampler descriptor indices, alpha/blend/color-write/alpha-test/scissor/sampler state, and primitive submit shape; wired `--renderer-gpu-submit-triage-smoke` so the sidecar viewer joins backend material submits to runtime draw rectangles and local CSD commands, then live-proved title-menu with `143` backend submits, `142` textured submits, `96` alpha-blended submits, `65` draw rectangles, and `75` local title-menu CSD commands while keeping raw D3D12/Vulkan backend capture explicitly pending.
- [x] Phase 151 on `2026-04-29`: added the read-only `ui-material-correlation` live-bridge command that correlates runtime CSD draw-list rectangles with render-thread backend submit samples by same-frame/order window, names blend/filter/address/alpha/color-write state with Xenos/D3D-ish semantics, records half-pixel offsets, and seeds RHI command-list boundary observation; wired `--renderer-material-correlation-smoke` so the sidecar viewer consumes this oracle directly, then live-proved title-menu with `65` correlated pairs, `143` backend submits, `29` alpha-blend pairs, `1` additive pair, `56` linear-filter pairs, `9` point-filter pairs, and explicit raw D3D12/Vulkan command-buffer capture still pending.
- [x] Phase 152 on `2026-04-29`: added the read-only `ui-backend-resolved` live-bridge command that samples backend-resolved D3D12/Vulkan draw submit state at command-list hooks, including native command names, pipeline/pipeline-layout handles, resolved PSO blend state, render-target/depth formats, framebuffer size, topology, input layout, depth state, and alpha-to-coverage; wired `--renderer-backend-resolved-triage-smoke` so the sidecar joins these backend facts to material-correlation evidence; rebuilt the real UI Lab runtime and live-proved title-loop with `14` backend-resolved submits, `14` resolved-pipeline submits, `13` blend-enabled submits, `14` known framebuffer submits, and `3 / 3` RGB-nonblack native BMPs while keeping text/movie/SFX and full vendor command-buffer capture as later work.
- [x] Phase 153 on `2026-04-29`: converted backend-resolved PSO/blend/framebuffer state into named material parity hints, adding `backendMaterialParityHints` and per-submit `materialParityHint` labels (`source-over-alpha`, `additive-alpha`, `opaque-no-blend`, custom/unresolved cases) to `ui-backend-resolved`; wired `--renderer-material-parity-hints-smoke` so the sidecar reports backend-owned material policy counts per frontend lane; rebuilt the real UI Lab runtime and live-proved title-loop in `out/ui_lab_runtime_evidence/20260429_044413` with `130` backend-resolved submits, `41` source-over submits, `18` additive submits, `71` opaque submits, `130` framebuffer-registered submits, and `3 / 3` RGB-nonblack native BMPs while keeping texture-view/sampler descriptor internals and title/loading text/movie/SFX timing as the next “feels like game” blockers.
- [x] Phase 154 on `2026-04-29`: added runtime texture-view/sampler descriptor semantics to `ui-backend-resolved`, recording `GuestTexture` / `GuestSurface` descriptor metadata and sampler descriptor state beside material submits, emitting `backendDescriptorSemantics` with texture/sampler descriptor policy counts, semantic labels, and explicit `pending-native-descriptor-dump` status; wired `--renderer-descriptor-semantics-smoke` so the sidecar reports runtime descriptor-state coverage per frontend lane; rebuilt the real UI Lab runtime and live-proved title-loop in `out/ui_lab_runtime_evidence/20260429_051210` with `14` backend-resolved submits, `11` material pairs, `11` known texture descriptors, `11` known sampler descriptors, `11` linear sampler descriptors, and `3 / 3` RGB-nonblack native BMPs while title/loading text/movie/SFX timing and raw vendor descriptor dumps remain the next “feels like game” blockers.
- [x] Phase 155 on `2026-04-29`: added native RHI resource-view/sampler handle capture to `ui-backend-resolved`, recording D3D12/Vulkan texture resource handles, texture view handles, sampler handles, native formats, view dimensions, filter/address state, and `backendVendorResourceCapture` joins back to UI material pairs; wired `--renderer-vendor-resource-capture-smoke` so the sidecar reports vendor-resource coverage per frontend lane; rebuilt the real UI Lab runtime and live-proved title-loop in `out/ui_lab_runtime_evidence/20260429_061651` with `11` material pairs, `11` known texture resource views, `11` known sampler resource views, `11` paired resource captures, `3 / 3` RGB-nonblack native BMPs, and explicit `pending-runtime-ui-render-target-copy` / `pending-full-vendor-command-buffer-dump` status for the still-missing true UI-only rendered layer or vendor command dump.
- [x] Phase 156 on `2026-04-29`: added the material resource-view parity oracle on top of Phase 155 vendor-resource capture, exposing resource-view exactness, premultiplied alpha, gamma/sRGB classification, resource-view pair counts, sRGB candidate counts, and a seeded `uiOnlyRenderTargetCaptureProbe`; wired `--renderer-material-resource-view-parity-smoke` so the sidecar reports these fields per frontend lane; rebuilt the real UI Lab runtime and live-proved title-loop in `out/ui_lab_runtime_evidence/20260429_090457` with `11` material pairs, `11` exact resource-view pairs, `11` known texture resource views, `11` known sampler resource views, straight-alpha resource-view status, `0` sRGB-classified resource views, `5 / 5` RGB-nonblack native BMPs, and true UI-only render-target copy explicitly pending.
- [x] Phase 157 on `2026-04-29`: added the read-only `ui-vendor-command-capture` live-bridge command and sidecar `--renderer-vendor-command-resource-dump-smoke` lane for a raw backend command plus vendor resource-view dump; the runtime now emits `vendorCommandResourceDump`, raw/backend submit counts, texture/sampler resource-view dump counts, exact resource-pair counts, and explicit `pending-runtime-ui-render-target-copy` / `pending-full-vendor-command-buffer-replay` status; rebuilt the real UI Lab runtime and live-proved title-loop in `out/ui_lab_runtime_evidence/20260429_100709` with `103` raw backend commands, `128` backend-resolved submits, `25` exact resource pairs, `555` texture resource-view samples, `7` sampler resource-view samples, and `25 / 25` RGB-nonblack native BMPs while keeping true UI-only render-target copy pending.
- [x] Phase 158 on `2026-04-29`: added the `ui-layer-capture` / `ui-layer-status` live-bridge lane and render-thread `QueueUiLayerCapture` seam to copy the active render target after pending stretch rects and before ImGui/present; wired sidecar `--renderer-ui-layer-capture-smoke` to report capture status, isolation status, capture path, and local CSD command coverage; rebuilt the real UI Lab runtime and live-proved title-loop in `out/ui_lab_runtime_evidence/20260429_105319` with `ui_layer_render_target_title-loop_1_1582x853.bmp`, `active-render-target-copy-before-present`, `not-isolated-active-color-target`, `27` runtime UI draw calls, `105` runtime GPU submit samples, and `3 / 3` RGB-nonblack native BMPs while keeping the status honest that the active render target may still include scene/background pixels until a dedicated UI-only layer or full vendor command-buffer replay is proven.
- [x] Phase 159 on `2026-04-29`: added sidecar UI-layer pixel comparison with `--renderer-ui-layer-pixel-compare-smoke`, rendering tracked title/loading/options/pause policies and comparing them against the newest local `ui_layer_render_target_<target>_*.bmp` evidence when present; live-proved the loading lane in `out/ui_lab_runtime_evidence/20260429_113357` with `ui_layer_render_target_loading_1_1582x853.bmp`, `72` local commands, `mean_abs_rgb=29.486`, `max_abs_rgb=255`, and `not-isolated-active-color-target`; the smoke lane writes ignored `out/ui_layer_pixel_compare/phase159/ui_layer_pixel_compare_manifest.json`, emits per-lane `ui_layer_pixel_delta` metrics or explicit missing-capture status, and keeps `dedicated-ui-target-or-vendor-replay-needed` plus `pending-title-loading-media-timing` visible for the remaining oracle/media gaps.
- [x] Phase 160 on `2026-04-29`: promoted UI-layer capture into `capture_unleashed_recomp_ui_lab.ps1` with `-UiLayerCapture` plus optional `-RequireUiLayerCapture`, arming the live bridge at readiness and recording `uiLayerCaptureAttempted`, request/status JSON, and pass/fail state in the manifest; live-proved missing frontend lanes with `title-menu` (`out/ui_lab_runtime_evidence/20260429_120418`), `title-options` (`out/ui_lab_runtime_evidence/20260429_120324`), and `pause` (`out/ui_lab_runtime_evidence/20260429_120533`) all reporting `ui-layer-capture-observed`; reran `--renderer-ui-layer-pixel-compare-smoke` and got found UI-layer captures for all four tracked frontend lanes: title-menu `mean_abs_rgb=36.833`, loading `29.486`, title-options `34.177`, pause `63.304`, all still labeled `not-isolated-active-color-target` until a dedicated UI target or vendor replay is proven.
- [x] Phase 161 on `2026-04-29`: promoted title/loading media timing into tracked `frontend_screen_reference` policy with explicit movie, text, glyph, fade, and SFX cue tables; `sward_frontend_screen_reference_catalog --phase161-media-smoke` reports title-menu visual cues (`movie=1`, `text=1`, `glyph=1`, `fade=1`) and loading visual cues (`text=1`, `glyph=1`, `fade=2`) from runtime UI-layer capture plus CSD timeline evidence, while SFX cues are timed but honestly marked `audio-id-pending`; the sidecar UI-layer pixel compare status now reads `title-loading-media-timing-reference-ready-audio-id-pending` so the remaining gaps stay narrow: exact audio IDs/playback, isolated UI target or vendor replay, and final premultiplied/gamma/sRGB tuning.
- [x] Phase 162 on `2026-04-29`: added local title/loading media asset readiness probes on top of the Phase 161 cue table; `sward_frontend_screen_reference_catalog --phase162-media-asset-smoke` and `sward_su_ui_asset_renderer --renderer-media-asset-readiness-smoke` now resolve local Sonic placeholder assets for title movie preview, title text/glyph/fade DDS, and loading text/glyph/fade DDS, reporting title-menu `resolved=4`, `preview=1`, `playback_ready=3`, `decode_pending=1`, `audio_pending=2` and loading `resolved=4`, `playback_ready=4`, `audio_pending=2`; this makes the remaining media gap concrete: SFD decode/playback and exact SFX/audio IDs are still pending, but visual placeholder media files are now found and wired into tracked smoke paths.
- [x] Phase 163 on `2026-04-29`: shifted the reusable lane from viewer polish toward native UI/UX source reconstruction by adding tracked `frontend_screen_controllers.hpp/.cpp` plus `sward_frontend_screen_controller_catalog`; the first hand-written controller classes (`TitleMenuController`, `LoadingScreenController`, `OptionsMenuController`, `PauseMenuController`) now consume `frontend_screen_reference` policy, expose frame-by-frame state snapshots, scene activation, input-lock state, cursor labels, route targets, and SFX hook IDs, and smoke-report the early frontend flow while marking `sonic-day-hud-next` as the next controller family to promote.
- [x] Phase 164 on `2026-04-29`: promoted the normal Sonic day-stage HUD into the native controller lane with `SonicDayHudController`, driven by `sonic_hud_reference` / exact `ui_playscreen` policy instead of viewer-only diagnostics; `sward_frontend_screen_controller_catalog --phase164-sonic-hud-smoke` now reports `CHudSonicStage` / `sub_824D9308` / `ui_playscreen` ownership, `13` scenes, `209` runtime layers, `167` drawable layers, plus frame-by-frame HUD states for owner bootstrap, stage HUD readiness, tutorial prompt readiness, and ring pickup feedback with scene activation and SFX hook labels.
- [x] Phase 165 on `2026-04-29`: deepened `SonicDayHudController` from scene activation into explicit gameplay HUD value ports with `SonicDayHudGameplayState` and `SonicDayHudValueProvenance`; `sward_frontend_screen_controller_catalog --phase165-sonic-hud-state-smoke` now reports rings, score, timer, speed, boost, ring-energy, lives, tutorial prompt visibility, route event, and SFX hook transitions while labeling every numeric value as `host/live-bridge supplied value` and `live-bridge-value-port-pending` so controller reconstruction does not falsely claim recovered memory bindings before the runtime bridge exposes them.
- [x] Phase 166 on `2026-04-29`: bound the Sonic Day HUD value-port model to the live bridge contract with `typedInspectors.sonicHud.gameplayValues`, added per-field `known` flags/sources for ring count, score, timer, speed, boost, ring-energy, lives, tutorial prompt state, and audio IDs, runtime-bound score through `SWA::CGameDocument::m_pMember->m_ScoreInfo.EnemyScore+TrickScore`, and kept unproven Sonic HUD values/SFX as `pending-runtime-field` / `audio-id-pending` while `SonicDayHudController::applyRuntimeBinding` consumes known fields without inventing the rest.
- [x] Phase 167 on `2026-04-29`: promoted exact Sonic HUD runtime field paths without overclaiming numerics by adding `ScoreInfo.PointMarkerRecordSpeed` / `ScoreInfo.PointMarkerCount` score-info evidence, extending the raw `CHudSonicStage` owner hook to report `m_rcScoreCount`, `m_rcTimeCount`, `m_rcTimeCount2`, `m_rcTimeCount3`, and `m_rcPlayerCount`, resolving display-owner paths for ring/timer/speed/boost/energy/lives/tutorial through `ui_playscreen`, and smoke-reporting `score:known`, `scoreinfo:known`, with ring/timer/speed/boost/energy/lives/tutorial gameplay numerics still marked pending until true player/HUD value offsets are proven.
- [x] Phase 168 on `2026-04-29`: traced actual Sonic HUD CSD text write/update paths by hooking `CSD::CNode::SetText/sub_830BF640`, resolving guest node addresses back to `ui_playscreen/ring_count/num_ring`, `ui_playscreen/time_count/time001|time010|time100`, `ui_playscreen/add/speed_count/position/num_speed`, and `ui_playscreen/player_count/player`, feeding those display-write-proven values into `OnSonicHudGameplayValues`, and leaving boost/energy/tutorial as `pending-gauge-or-prompt-write-hook` until their gauge/prompt setter callsites are proven.
- [x] Phase 169 on `2026-04-29`: promoted the manual-control live observer result into the Sonic Day HUD controller lane by adding `SonicDayHudRuntimeDrawListCoverage` and `--phase169-sonic-hud-draw-list-coverage-smoke`, recording that live `ui-draw-list`/material-correlation can prove `ui_playscreen` speed gauge, gauge frame, ring-energy gauge, and pause overlay draw activity even when CSD text writes do not fire; added runtime CSD node hooks for `CNode::SetPatternIndex/sub_830BF300`, `CNode::SetHideFlag/sub_830BF080`, and `CNode::SetScale/sub_830BF090` so boost/energy/tutorial gauge and prompt writes can be proven on the next runtime build without control automation.
- [x] Phase 170 on `2026-04-29`: live-proved the Phase 169 observer gap as a clean negative result (`ui_playscreen` draw-list active, gauge/text write events absent), then added unresolved CSD node-write evidence for HUD-like numeric `SetText`, `SetPatternIndex`, `SetHideFlag`, and `SetScale` gated by active `ui_playscreen` draw calls; the bridge now emits `sonic-hud-node-write-unresolved` plus `pathResolved=false` observations so the next manual run can tell unresolved address timing apart from missing gameplay setter callsites.
- [x] Phase 171 on `2026-04-29`: added late resolution for anonymous Sonic HUD CSD writes by matching unresolved node addresses against recent `ui_playscreen` draw-list layer/cast-node addresses; observations now include `pathResolutionSource`, `sonic-hud-node-write-late-resolved` is emitted when a real HUD path is recovered after the setter call, and named scale/pattern/hide writes can feed boost, ring-energy, and tutorial prompt gameplay ports while exact player-storage offsets remain future work.
- [x] Phase 172 on `2026-04-29`: followed the live manual observer evidence into the CSD lookup chain by hooking `sub_830BCCA8` child lookup plus `sub_830BA228` RCPtr unwrap, wrapping the key `CHudSonicStage` update callsites with a live update context, proving the raw owner fields mature to real CSD addresses, and resolving timer/life text writes through `pathResolutionSource=raw-chud-sonic-stage-owner-field` before child lookup/draw-list fallback.
- [x] Phase 173 on `2026-04-29`: promoted the Phase 172 live `sonic-hud-value-text-write` payload shape into reusable `SonicDayHudController` source with `SonicDayHudRuntimeTextWriteObservation`, `applyRuntimeTextWrite`, and a catalog smoke path that drives timer/life HUD state from `raw-chud-sonic-stage-owner-field` resolved CSD text sinks while keeping ring/speed and boost/energy/tutorial at their proven-but-not-yet-fully-normalized lanes.
- [x] Phase 174 on `2026-04-29`: added `sonic-hud-update-callsite-sample` evidence at the raw `CHudSonicStage` update-callsite boundary, sampling pre/post original owner fields for `sub_824D6048`, `sub_824D6418`, `sub_824D69B0`, `sub_824D6C18`, and `sub_824D7100` so manual observer runs can expose timer/counter/speed/gauge candidate fields even when downstream `CNode::SetText` does not fire.
- [x] Phase 175 on `2026-04-29`: promoted `sonic-hud-update-callsite-sample` into reusable `SonicDayHudController` source with `SonicDayHudRuntimeCallsiteSample`, generated-PPC callsite classification, and `applyRuntimeCallsiteSample`; `sub_824D6048` now drives timer state from runtime-proven owner `+456/+452` text-write semantics, while `sub_824D6418` speed and `sub_824D6C18` rolling counter/gauge samples are classified as direct PPC candidates pending final boost/energy/tutorial normalization.
- [x] Phase 176 on `2026-04-29`: mirrored the Phase 175 callsite classification into the live bridge so `sonic-hud-callsite-value-classified` is emitted by the running runtime, `sub_824D6048` post-original samples promote timer data into `typedInspectors.sonicHud.gameplayValues.elapsedFrames`, and speed/rolling-counter/tutorial update callsites remain named generated-PPC candidates until boost/energy/tutorial normalization is proven.
- [x] Phase 177 on `2026-04-29`: made the live bridge retain the latest classified Sonic HUD callsite result in `typedInspectors.sonicHud.gameplayValues.lastClassifiedCallsiteValue`, so operator tools can read the most recent runtime-proven value name/source/status/normalized value from the live-state snapshot as well as from durable JSONL events.
- [x] Phase 178 on `2026-04-29`: compacted the visible UI Lab operator overlay for manual gameplay sessions by making all panes and the foreground debug layer opt-in from the `UI` button while keeping the live bridge/API enabled; throttled volatile Sonic HUD update-callsite JSONL/live-state writes behind a stable callsite signature and 120-frame evidence interval; added a scoped generated-PPC `sub_8251A568` return hook inside `sub_824D6418` so Sonic HUD speed can become a runtime-proven live value while boost, ring-energy, tutorial prompt, and exact SFX IDs remain explicit pending callsite/audio lanes.
- [x] Phase 179 on `2026-04-29`: replaced the noisy default floating operator cluster with a profiler-style SWARD operator panel that mirrors the native `DrawProfiler()` shape, keeps F1 reserved for the native Recomp Profiler, leaves old counter/view/export/debug-draw/stage/live panes as Panels-tab drill-downs, removes transient `RCPtr::Get()` owner-field derefs from value-update hooks, and adds low-overhead Sonic HUD callsite sampling so manual gameplay sessions stay readable and fast while direct live-bridge inspection remains enabled.
- [x] Phase 180 on `2026-04-29`: promoted runtime Sonic HUD gauge/prompt setter observations into reusable native controller source with `SonicDayHudRuntimeGaugePromptWriteObservation`, `applyRuntimeGaugePromptWrite`, and `--phase180-sonic-hud-gauge-prompt-write-smoke`; boost, ring-energy, and tutorial prompt ports now consume real `SetScale` / `SetPatternIndex` / `SetHideFlag` evidence while exact SFX/audio IDs remain pending.
- [x] Phase 181 on `2026-04-29`: expanded bounded UI Lab control automation to support the user's full mapped key set (`ENTER/W/A/S/D/Q/E/UP/DOWN/LEFT/RIGHT`) and added a `gameplay-sweep` pulse plan that continues through the post-evidence settle window so unattended observer sessions can use arrow-key movement/menu navigation after HUD readiness without pretending WASD covers every control path.
