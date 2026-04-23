<p align="center">
    <img src="./docs/assets/branding/icon_extra.png" width="108" alt="SWARD extra icon"/>
</p>

# <img src="./docs/assets/branding/icon_extra.png" width="30" alt="SWARD extra icon"/> Changelog

Project history for **Project Sonic World Adventure R&D / SWARD**.

> [!NOTE]
> This changelog tracks the publishable repo layer. Local-only extracted assets, generated PPC output, staged tools, and private inputs stay outside git history by design.

## 2026-04-23

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

The repo still does not contain:

- extracted proprietary assets
- private Xbox 360 inputs
- generated PPC translation output
- generated shader cache output
