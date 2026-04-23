<p align="center">
    <img src="./docs/assets/branding/icon_extra.png" width="108" alt="SWARD extra icon"/>
</p>

# <img src="./docs/assets/branding/icon_extra.png" width="30" alt="SWARD extra icon"/> Changelog

Project history for **Project Sonic World Adventure R&D / SWARD**.

> [!NOTE]
> This changelog tracks the publishable repo layer. Local-only extracted assets, generated PPC output, staged tools, and private inputs stay outside git history by design.

## 2026-04-23

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

The repo still does not contain:

- extracted proprietary assets
- private Xbox 360 inputs
- generated PPC translation output
- generated shader cache output
