<p align="center">
    <img src="./docs/assets/branding/icon_sward.png" width="132" alt="SWARD icon"/>
</p>

# <img src="./docs/assets/branding/icon_sward.png" width="36" alt="SWARD icon"/> Changelog

Project history for **Project Sonic World Adventure R&D / SWARD**.

> [!NOTE]
> This changelog tracks the publishable repo layer. Local-only extracted assets, generated PPC output, staged tools, and private inputs stay outside git history by design.

## 2026-04-23

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
