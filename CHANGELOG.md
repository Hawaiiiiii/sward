<p align="center">
    <img src="./docs/assets/branding/icon_sward.png" width="132" alt="SWARD icon"/>
</p>

# <img src="./docs/assets/branding/icon_sward.png" width="36" alt="SWARD icon"/> Changelog

Project history for **Project Sonic World Adventure R&D / SWARD**.

> [!NOTE]
> This changelog tracks the publishable repo layer. Local-only extracted assets, generated PPC output, staged tools, and private inputs stay outside git history by design.

## 2026-04-23

### Phase 25 broader remaining UI and common-flow extraction

- added [`research_uiux/tools/summarize_commonflow_extraction.py`](./research_uiux/tools/summarize_commonflow_extraction.py)
- added [`research_uiux/COMMON_FLOW_LOCALIZATION_EXTRACTION.md`](./research_uiux/COMMON_FLOW_LOCALIZATION_EXTRACTION.md)
- ranked `357` remaining UI/common-flow archive candidates after the earlier archaeology passes
- safely extracted a focused `24`-directory English common-flow/localization slice into `extracted_assets/phase25_commonflow_archives`
- confirmed this batch was loose-file heavy rather than layout heavy, adding `338` files with `54` `.dds`, `261` `.fco`, and `15` `.fte` payloads but no new `.yncp` packages
- re-ran the asset scan to `6840` combined matched entries, with `5112` extracted-side matches and `89` indexed hits inside the Phase 25 extraction root
- documented the growing emphasis on correlation/state labeling over blind archive hunting for the next beats

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

The repo still does not contain:

- extracted proprietary assets
- private Xbox 360 inputs
- generated PPC translation output
- generated shader cache output
