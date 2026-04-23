<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Multilingual UI And Subtitle Coverage

Machine-readable inventory: `research_uiux/data/multilingual_ui_coverage.json`

## Scope

This phase extends the earlier English-first UI extraction with two additional evidence layers:

- non-English localized UI archive coverage for menu/loading/world-map/common-shell assets
- subtitle/cutscene support coverage under `Inspire/subtitle/`

It does not blindly unpack every subtitle archive. It audits the full language matrix, then safely extracts a curated cross-language slice that is enough to inspect caption packaging and cutscene-adjacent UI payloads locally.

## Verified Coverage In The Owned Dump

Localized archive symmetry:

- each of the `6` supported languages present in the dump has `50` localized `.arl` archives under `Languages/<Language>/`
- the key UI-facing archive families audited in this phase are present for all six languages:
  - `Loading`
  - `SystemCommonCore`
  - `Title`
  - `WorldMap`
  - plus broader families such as `ActionCommon`, `BossCommon`, `Sonic`, `SonicActionCommon`, and `SuperSonic`

Subtitle symmetry:

- each of the `6` languages has `53` subtitle archive manifests under `Inspire/subtitle/<Language>/`
- shared subtitle scene IDs across all languages: `53`
- there are no language-specific gaps in the archive manifest layer for this subtitle bank

Shared subtitle family counts:

- `m0`: `2`
- `m1`: `4`
- `m2`: `3`
- `m3`: `4`
- `m4`: `1`
- `m5`: `2`
- `m6`: `3`
- `m7`: `3`
- `m8`: `18`
- `s1`: `4`
- `s2`: `3`
- `s3`: `2`
- `t0`: `4`

Practical reading:

- localized UI/text packaging is highly regular rather than ad hoc
- the subtitle archive bank is structurally complete across English, French, German, Italian, Japanese, and Spanish
- the late-game `m8` family is the heaviest subtitle cluster in this manifest layer

## Safe Extraction Batch

New extraction root:

- `extracted_assets/ui_multilingual_archives`

Curated archive slice extracted in this phase:

- `20` localized UI archives
  - `Languages/French/{Loading,SystemCommonCore,Title,WorldMap}`
  - `Languages/German/{Loading,SystemCommonCore,Title,WorldMap}`
  - `Languages/Italian/{Loading,SystemCommonCore,Title,WorldMap}`
  - `Languages/Japanese/{Loading,SystemCommonCore,Title,WorldMap}`
  - `Languages/Spanish/{Loading,SystemCommonCore,Title,WorldMap}`
- `18` subtitle/cutscene archives
  - `Inspire/subtitle/{English,French,German,Italian,Japanese,Spanish}/evrt_m1_03_2.ar`
  - `Inspire/subtitle/{English,French,German,Italian,Japanese,Spanish}/evrt_m8_15.ar`
  - `Inspire/subtitle/{English,French,German,Italian,Japanese,Spanish}/evrt_t0_04.ar`

Observed extracted totals under `ui_multilingual_archives`:

- extracted file count: `259`
- extension breakdown:
  - `.dds`: `128`
  - `.fco`: `78`
  - `.fte`: `35`
  - `.xml`: `18`

Top-level split:

- `Languages`: `151` files
- `Inspire`: `108` files

## Localized UI Findings

The localized UI slice does not add new `.yncp` layout containers. Instead, it exposes the language-facing texture, font-event, and message-list payloads that sit beside the already extracted shared layout projects.

Representative verified outputs:

- `Languages/French/SystemCommonCore/mat_pause_en_001.dds`
- `Languages/French/SystemCommonCore/mat_result_en_001.dds`
- `Languages/French/SystemCommonCore/mat_status_en_001.dds`
- `Languages/French/Title/mat_title_en_001.dds`
- `Languages/French/WorldMap/GeneralMessage_list.fco`
- `Languages/French/WorldMap/pause_menu_list.fco`
- `Languages/French/WorldMap/worldmap_hint_list.fco`

Equivalent payloads were extracted for German, Italian, Japanese, and Spanish as well.

Useful interpretation:

- the shared `.yncp` projects still appear to define the layout/state grammar
- localization work is pushed into neighboring texture sheets, conversation/font-event data, and message list assets
- `WorldMap` is especially rich in localized support payloads, including general messages, item lists, skill names, pause-menu beats, and hint lists
- `SystemCommonCore` carries localized status/pause/result text textures that directly support pause/status/result overlays already mapped elsewhere in the workspace

Naming note:

- many localized texture filenames still include `_en_` in their base names even when extracted from non-English archives
- this suggests the authoring/export naming stayed stable while the archive payload itself changed by language
- for research purposes, the archive path is therefore a more reliable locale signal than the individual texture basename

## Subtitle / Cutscene Findings

Representative verified outputs:

- `Inspire/subtitle/English/evrt_m1_03_2/evrt_m1_03_2_subtitle_English.inspire_resource.xml`
- `Inspire/subtitle/French/evrt_m8_15/evrt_m8_15_subtitle_French.inspire_resource.xml`
- `Inspire/subtitle/Japanese/evrt_t0_04/evrt_t0_04_subtitle_Japanese.inspire_resource.xml`
- paired event-side assets such as `.fco`, `.fte`, and `_event_000.dds`

Important packaging observation:

- some subtitle archives extract more than one event bundle
- for example:
  - `evrt_m1_03_2.ar` also exposes `evrt_m2_01_event.*`
  - `evrt_m8_15.ar` also exposes `evrt_m8_16_event.*`

Why this matters:

- subtitle archives are not purely one-file caption blobs
- they can package neighboring event/cutscene resources together
- that makes them useful for later cutscene-flow archaeology, especially when correlating mission progression, caption banks, and support overlays

## Effect On The Wider Asset Index

After re-running `scan_assets.py` across the installed build plus the full `extracted_assets` tree:

- installed-build matches remain `1986`
- extracted-asset matches increase from `1015` to `1184`
- combined matched entries increase from `3001` to `3170`

Most of the increase lands in:

- `config_or_markup`
- `texture`
- localized `.fco` / `.fte` / `.xml` support files under `WorldMap` and `Inspire/subtitle`

## High-Value Follow-Up Files

1. `extracted_assets/ui_multilingual_archives/Languages/French/SystemCommonCore/mat_pause_en_001.dds`
2. `extracted_assets/ui_multilingual_archives/Languages/Japanese/WorldMap/worldmap_hint_list.fco`
3. `extracted_assets/ui_multilingual_archives/Languages/Spanish/WorldMap/pause_menu_list.fco`
4. `extracted_assets/ui_multilingual_archives/Inspire/subtitle/English/evrt_m1_03_2/evrt_m1_03_2_subtitle_English.inspire_resource.xml`
5. `extracted_assets/ui_multilingual_archives/Inspire/subtitle/French/evrt_m8_15/evrt_m8_15_subtitle_French.inspire_resource.xml`
6. `extracted_assets/ui_multilingual_archives/Inspire/subtitle/Japanese/evrt_t0_04/evrt_t0_04_subtitle_Japanese.inspire_resource.xml`

## Interpretation

This phase strengthens the workspace in two concrete ways:

- it confirms that the game’s UI-facing localization layer is systematic across the six shipped language banks, not a partial or irregular add-on
- it proves that subtitle/cutscene support archives are locally accessible and structurally useful for UI/UX research, especially when studying message timing, event adjacency, and caption packaging without needing leaked source or blind full-dump extraction
