<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Common-Flow Localization Extraction

Machine-readable inventory: `research_uiux/data/commonflow_localization_extraction.json`

## Summary

- Phase 25 extraction root: `extracted_assets/phase25_commonflow_archives`
- Archive directories extracted in this pass: `24`
- Files extracted in this pass: `338`
- New `.yncp` / `.xncp` layouts surfaced in this pass: `0`
- Combined asset-index matches after the re-scan: `6840`
- Extracted-asset matches after the re-scan: `5112`
- Indexed matches inside `phase25_commonflow_archives`: `89`

> [!NOTE]
> This pass broadened the loose localization/common-flow layer much more than it broadened the layout layer.
> The extracted value is mainly DDS/FCO/FTE material that names pause, status, result, loading, world-map, shop, mission, and EX-stage support families.

## Why This Batch Matters

- It closes part of the gap between extracted authored layouts and the localized/common-flow texture texturing that those layouts point at.
- It shows that many still-unextracted high-scoring UI candidates are language/common-flow mirrors rather than fresh `.yncp` packages.
- It gives the archaeology layer more loose-file evidence for prompt rows, hint lists, controller text, mission counters, and loading/start messaging.

## Extension Mix

- `.fco`: `261`
- `.dds`: `54`
- `.fte`: `15`
- `.xml`: `6`
- `.light`: `2`

## Family Coverage

- `Pause / Status / Result / Save loose-text layer`: `22`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_result_en_001.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/mat_result_en_001.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/pause_menu_list.fco`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/SystemCommonCore/mat_pause_en_001.dds`
- `EX Stage / Tornado Defense support layer`: `10`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/evex_ex00_event.fco`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/evex_ex00_event.fte`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/evex_ex00_event_000.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/evex_ex01_event.fco`
- `Mission / Gate / Action common layer`: `9`
  - sample: `extracted_assets/phase25_commonflow_archives/#ActN_Mission_Beach/Mission_MoveMission_S80_40.set.xml`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_gate_en_001.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_gate_en_002.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_misson_en_001.dds`
- `Town / Shop / Media Room common layer`: `8`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/Town_Common/mat_shop_en_001.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/Town_Common/mat_townscreen_en_001.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/Town_EULabo_Common/MediaRoom_list.fco`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/Town_EULabo_Common/MediaRoomJP_list.fco`
- `World Map / Staff Roll support layer`: `8`
  - sample: `extracted_assets/phase25_commonflow_archives/#Title/WorldMap.light`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/WorldMap/mat_worldmap_en_001.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/WorldMap/mat_worldmap_en_002.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/WorldMap/mat_worldmap_en_003.dds`
- `Loading / Start / Transition texturing`: `5`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_start_en_001.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/Loading/mat_load_en_001.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/Loading/mat_load_en_002.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/Loading/mat_loadinfo_text_evil_e.dds`
- `Title-screen common layer`: `2`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/Title/mat_title_en_001.dds`
  - sample: `extracted_assets/phase25_commonflow_archives/Languages/English/Title/mat_title_en_002.dds`

## Extracted Archive Directories

- `extracted_assets/phase25_commonflow_archives/#ActD_Mission_Africa`: `1` files (.xml=1)
- `extracted_assets/phase25_commonflow_archives/#ActN_Mission_Beach`: `2` files (.xml=2)
- `extracted_assets/phase25_commonflow_archives/#Title`: `4` files (.light=2, .xml=2)
- `extracted_assets/phase25_commonflow_archives/#WorldMap`: `1` files (.xml=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon`: `7` files (.dds=7)
- `extracted_assets/phase25_commonflow_archives/Languages/English/BossCommon`: `1` files (.dds=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common`: `14` files (.dds=7, .fco=4, .fte=3)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Loading`: `4` files (.dds=4)
- `extracted_assets/phase25_commonflow_archives/Languages/English/SystemCommonCore`: `7` files (.dds=7)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Title`: `2` files (.dds=2)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_Africa_Common`: `28` files (.fco=26, .dds=1, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_China_Common`: `29` files (.fco=26, .dds=2, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_Common`: `3` files (.dds=3)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_EULabo_Common`: `13` files (.fco=11, .dds=1, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_EggManBase_Common`: `17` files (.fco=12, .dds=4, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_EuropeanCity_Common`: `42` files (.fco=40, .dds=1, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_Labo_Common`: `2` files (.dds=2)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_Mykonos_Common`: `28` files (.fco=26, .dds=1, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_NYCity_Common`: `25` files (.fco=23, .dds=1, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_PetraCapital_Common`: `37` files (.fco=35, .dds=1, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_PetraLabo_Common`: `13` files (.fco=11, .dds=1, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_Snow_Common`: `20` files (.fco=18, .dds=1, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/Town_SouthEastAsia_Common`: `21` files (.fco=19, .dds=1, .fte=1)
- `extracted_assets/phase25_commonflow_archives/Languages/English/WorldMap`: `17` files (.fco=10, .dds=6, .fte=1)

## Ranked Remaining Candidates Before This Pass

- Ranked candidate archives in the pre-pass sweep: `357`
- Still-unextracted candidates at that point: `333`

Top still-unextracted examples at ranking time:
- `Languages/English/SystemCommonCore.arl` score `16` | layout refs `<none>` | keyword hits `pause, result, status`
- `Languages/French/SystemCommonCore.arl` score `16` | layout refs `<none>` | keyword hits `pause, result, status`
- `Languages/German/SystemCommonCore.arl` score `16` | layout refs `<none>` | keyword hits `pause, result, status`
- `Languages/Italian/SystemCommonCore.arl` score `16` | layout refs `<none>` | keyword hits `pause, result, status`
- `Languages/Japanese/SystemCommonCore.arl` score `16` | layout refs `<none>` | keyword hits `pause, result, status`
- `Languages/Spanish/SystemCommonCore.arl` score `16` | layout refs `<none>` | keyword hits `pause, result, status`
- `Languages/English/ActionCommon.arl` score `10` | layout refs `<none>` | keyword hits `result`
- `Languages/French/ActionCommon.arl` score `10` | layout refs `<none>` | keyword hits `result`

## Indexed Phase 25 Hits

- `texture`: `54`
- `candidate`: `29`
- `config_or_markup`: `6`

Related-screen hints inside the indexed Phase 25 slice:
- `unknown`: `49`
- `menu`: `27`
- `result`: `4`
- `pause`: `3`
- `title`: `2`
- `ui`: `2`
- `mission`: `1`
- `rank`: `1`

## Verified Loose-File Highlights

- `extracted_assets/phase25_commonflow_archives/#ActN_Mission_Beach/Mission_MoveMission_S80_40.set.xml`
- `extracted_assets/phase25_commonflow_archives/#Title/WorldMap.light`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_gate_en_001.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_gate_en_002.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_misson_en_001.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_playscreen_en_001.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_playscreen_en_002.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_result_en_001.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ActionCommon/mat_start_en_001.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/BossCommon/mat_boss_en_003.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/evex_ex00_event.fco`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/evex_ex00_event.fte`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/evex_ex00_event_000.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/evex_ex01_event.fco`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/evex_ex01_event.fte`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/evex_ex01_event_000.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/ExStage_hint_list.fco`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/fte_ConverseMain.fte`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/fte_ConverseMain_000.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/mat_ex_en_001.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/mat_playscreen_en_001.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/mat_qte_en_001.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/mat_result_en_001.dds`
- `extracted_assets/phase25_commonflow_archives/Languages/English/ExStageTails_Common/pause_menu_list.fco`

## Interpretation

- The remaining high-yield work is now less about blindly hunting for another large `.yncp` batch and more about wiring these common-flow loose files back to the already-extracted layout families.
- `SystemCommonCore`, `ActionCommon`, `Loading`, `Town_Common`, `Town_Labo_Common`, and `WorldMap` now have a stronger localized-texture/support layer even where the authored layout package had already been recovered earlier.
- The Phase 25 batch is especially useful for direct-port template work because it surfaces the naming and packaging around prompt text, hint lists, mission counters, shop/town overlays, EX-stage controller prompts, and loading/start copy.
