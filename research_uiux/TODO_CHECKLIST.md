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
