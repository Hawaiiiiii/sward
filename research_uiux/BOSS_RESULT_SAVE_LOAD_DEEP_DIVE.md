<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Boss HUD / Mission-Result / Save-Load Deep Dive

Support roots used in this pass:

- `extracted_assets/phase16_support_archives`
- `extracted_assets/ui_extended_archives/#Application`
- `extracted_assets/mission_probe`

## Summary

- A focused `3`-archive support probe was extracted with `HedgeArcPack`:
  - `ActionCommon.arl`
  - `ExStageTails_Common.arl`
  - `SonicActionCommonGeneral.arl`
- That probe added `1798` files and `9` additional `.yncp` projects under `extracted_assets/phase16_support_archives`.
- The key new layout discoveries were:
  - `ActionCommon/ui_missionscreen.yncp`
  - `ActionCommon/ui_misson.yncp`
  - `ActionCommon/ui_result.yncp`
  - `ExStageTails_Common/ui_result_ex.yncp`
- After re-running the local scanners, the workspace now reports:
  - `3503` combined UI-relevant asset matches
  - `23` parsed `.xncp` / `.yncp` files in the deep layout-analysis payload

> [!IMPORTANT]
> This phase closes the biggest result-screen gap from the earlier workspace: `ui_result_ex` was visible in readable path-fix code before, but now it is also present locally as an extracted layout package.

## Focused Extraction Probe

The Phase 16 probe was intentionally narrow. The goal was to answer three questions:

1. Does the broader gameplay/common archive layer contain result layouts that were missing from the earlier extraction set?
2. Are mission-facing overlays authored as UI layouts, or only as stage/setup XML?
3. Where do save/load and boss transitions actually live: asset layouts, readable patches, or `#Application` sequence graphs?

What the probe proved:

- `ActionCommon` and `ExStageTails_Common` are not just texture or effect banks; they contain UI layout packages for mission and result flows.
- The mission-specific stage archives themselves still look mostly like setup data:
  - `ActD_Mission_Africa` extracted only `StageObject.sto.xml`
  - `ActN_Mission_Africa` extracted `.set.xml` mission placement data plus `StageObject.sto.xml`
- That split matters:
  - mission logic and return routing are driven in `#Application` sequence XML
  - mission/result presentation is authored in separate layout packages under common gameplay archives

## Boss HUD

Primary local evidence:

- `extracted_assets/ui_broader_archives/BossCommon/ui_boss_gauge.yncp`
- `extracted_assets/ui_broader_archives/BossCommon/ui_boss_name.yncp`
- `local_build_env/ur103clean/UnleashedRecomp/patches/aspect_ratio_patches.cpp`
- `extracted_assets/ui_extended_archives/#Application/ArchiveTree.xml`
- `extracted_assets/ui_extended_archives/#Application/StageList.xml`

Readable-code seams:

- `aspect_ratio_patches.cpp:374-377` directly names:
  - `ui_boss_gauge/gauge_bg`
  - `ui_boss_gauge/gauge_2`
  - `ui_boss_gauge/gauge_1`
  - `ui_boss_gauge/gauge_breakpoint`
- `aspect_ratio_patches.cpp:380-381` directly names:
  - `ui_boss_name/name_so/bg`
  - `ui_boss_name/name_so/pale`

Asset-side structure:

- `ui_boss_gauge.yncp`
  - `4` scenes
  - `10` animation dictionaries
  - `33` cast dictionaries
  - longest gauge-size banks run `100` frames (`1.666667` seconds)
  - deepest gauge background hierarchy reaches group depth `7`
- `ui_boss_name.yncp`
  - `2` scenes: `name_ev`, `name_so`
  - `10` animation dictionaries
  - `200` cast dictionaries
  - numbered reveal banks `01_Anim` through `05_Anim` run `180` frames (`3.0` seconds)

Sequence-layer ownership:

- `ArchiveTree.xml` explicitly binds `BossCommon` plus boss-specific archives like `BossEggBeetle`, `BossPhoenix`, `BossPetra`, and `BossEggDragoon`.
- `StageList.xml` maps each major boss route to `GoTo*Boss` sequence IDs.
- The current extracted `StageList.xml` references each of these boss sequence IDs `3` times:
  - `GoToAfricaBoss`
  - `GoToBeachBoss`
  - `GoToChinaBoss`
  - `GoToEUBoss`
  - `GoToPetraBoss`
  - `GoToSnowBoss`
  - `GoToEggManLandBoss`

Interpretation:

- Boss UI is split cleanly between:
  - asset-authored overlay packages for name/gauge presentation
  - sequence-graph ownership for entering the boss scenario and hiding other layers during boss movies or transitions
- The boss HUD does not read like an ad-hoc overlay bolted on at runtime. It is a first-class authored CSD project whose nodes are patched by name in the readable aspect-ratio layer.

## Result And Mission-Result Overlays

Primary local evidence:

- `extracted_assets/phase16_support_archives/ActionCommon/ui_result.yncp`
- `extracted_assets/phase16_support_archives/ExStageTails_Common/ui_result_ex.yncp`
- `extracted_assets/ui_extended_archives/SystemCommon/ui_itemresult.yncp`
- `extracted_assets/phase16_support_archives/ActionCommon/ui_missionscreen.yncp`
- `extracted_assets/phase16_support_archives/ActionCommon/ui_misson.yncp`
- `local_build_env/ur103clean/UnleashedRecomp/patches/aspect_ratio_patches.cpp`

Direct readable-code seams:

- `aspect_ratio_patches.cpp:619-669` exposes `ui_result/...` nodes directly.
- `aspect_ratio_patches.cpp:671-710` exposes `ui_result_ex/...` nodes directly.
- Earlier work already tied `ui_itemresult.yncp` to `aspect_ratio_patches.cpp:408-411`.

That produces a now-clear result-family split:

- `ui_itemresult`
  - compact item-pickup or reward overlay
  - `4` scenes
  - `16` animation dictionaries
  - strongest families: `intro`, `outro`, `usual`, `ev`, `so`
- `ui_result`
  - main stage-result presentation shell
  - `17` scenes
  - `29` animation dictionaries
  - `290` cast dictionaries
  - `1649` subimages
  - longest rank reveal bank: `253` frames (`4.216667` seconds) on `result_rank_E`
- `ui_result_ex`
  - extended/specialized result variant
  - `15` scenes
  - `20` animation dictionaries
  - `204` cast dictionaries
  - `1410` subimages
  - same long-form `253`-frame rank reveal bank, plus a `200`-frame (`3.333333` seconds) `result_rank` usual bank

Mission-facing layouts discovered in the same support slice:

- `ui_missionscreen.yncp`
  - `7` scenes
  - `19` animation dictionaries
  - `134` cast dictionaries
  - explicit conditional animation naming such as `conditional_timer_ev`, `conditional_timer_so`, `conditional_meet_ev`
- `ui_misson.yncp`
  - `6` scenes
  - `7` animation dictionaries
  - `4` root children
  - animation families: `intro`, `scroll`, `usual`
  - likely a compact mission prompt / objective window project

> [!NOTE]
> The file name `ui_misson.yncp` appears to preserve the original typo in the shipped asset name. The evidence is local and exact; this report keeps the spelling as found.

Mission-system ownership:

- `SR_Main04A_SaveEmy.seq.xml` shows mission timer, finish/success conditions, stage overwrite sets, return position, and `LoadingResourceID`.
- Many `Play*Mission*.seq.xml` files in `#Application` use the same pattern:
  - configure mission parameters
  - `WaitStageEnd`
  - route back through `GoToWorldMapWithSave`
- The stage-local mission archives still appear to be data/setup payloads rather than UI-layout banks.

Interpretation:

- Mission gameplay rules live in `#Application` sequence XML and `.set.xml` stage data.
- Mission/result presentation lives in shared authored layout projects.
- Result UI is materially more elaborate than the early `ui_itemresult` overlay suggested. The full result family is now visibly composed of:
  - shared result shell
  - extended result shell
  - compact item-result shell
  - mission-condition layouts

## Save / Load / Autosave Ownership

Primary local evidence:

- `extracted_assets/ui_extended_archives/Loading/ui_loading.yncp`
- `extracted_assets/ui_extended_archives/AutoSave/ui_saveicon.yncp`
- `local_build_env/ur103clean/UnleashedRecomp/patches/resident_patches.cpp`
- `local_build_env/ur103clean/UnleashedRecomp/ui/black_bar.cpp`
- `extracted_assets/ui_extended_archives/#Application/*.seq.xml`

Readable-code seams:

- `resident_patches.cpp:68` hooks `SWA::CLoading::Update`
- `resident_patches.cpp:91` hooks `SWA::CSaveIcon::Update`
- earlier direct layout-path rules already exposed many `ui_loading/...` nodes in `aspect_ratio_patches.cpp:419-432`

Asset-side timing:

- `ui_loading.yncp`
  - `7` scenes
  - `37` animation dictionaries
  - longest `pda_txt` banks run `240` frames (`4.0` seconds)
  - combines long presentation banks with ultra-short controller/platform swap banks
- `ui_saveicon.yncp`
  - `1` scene
  - `1` animation dictionary
  - `180`-frame (`3.0` seconds) `Intro_Anim`
  - deep icon hierarchy with max group depth `5`

Sequence-layer ownership:

- `FlagList.xml` defines `iFlag_LoadingDisplayType`
- many `ChangeTime*` and mission-entry sequences set or branch on `iFlag_LoadingDisplayType`
- `SR_Main03_AfricaDayBoss.seq.xml` shows this recurring structure:
  - enter boss route
  - movie playback with `HideLayer` directives
  - `WaitStageEnd`
  - post-clear movie chain
  - `AutoSave`
  - next micro-sequence
- `SR_Main04A_SaveEmy.seq.xml` shows:
  - mission rule setup
  - `WaitStageEnd`
  - post-clear movie
  - `AutoSave`
  - final `WaitStageEnd` returning through `GoToWorldMapWithSave`
- `SR_WaitAfricaDayStageEnd.seq.xml` shows that return routing is selected by `eFlag_ReturnPlace`, not hardwired into the UI layer

Hard counts from the extracted sequence layer:

- `17` `SR_Wait*StageEnd.seq.xml` files
- `8` `SR_Main*Boss.seq.xml` files
- `7` `SR_Enter*Boss.seq.xml` files

Interpretation:

- Save/load ownership is explicitly layered:
  - `ui_loading` and `ui_saveicon` are the visual authored projects
  - readable C++ patches hook the native loading/save-icon update seams
  - `#Application` decides when loading screens, autosaves, and world-map returns occur
- The autosave icon is not the save system. It is the presentation node attached to a much larger sequence-graph pipeline.

## State-Machine Takeaways

1. Boss flow is sequence-owned, overlay-authored.
   The game enters boss routes through `GoTo*Boss` / `SR_Main*Boss` sequences, while the boss gauge/name overlays stay in dedicated common CSD packages.

2. Result flow is a family, not a single screen.
   `ui_itemresult`, `ui_result`, and `ui_result_ex` cover different result contexts and complexity levels.

3. Mission logic and mission UI are intentionally separate.
   Mission timers and conditions live in sequence/set data; mission prompts and result shells live in authored UI projects.

4. Save/load transitions are orchestrated by flags and return routes.
   `iFlag_LoadingDisplayType`, `WaitStageEnd`, `AutoSave`, and `GoToWorldMapWithSave` show that the sequence graph owns transition timing above the overlay layer.

5. Loading and save visuals are long enough to deserve lockout-aware design.
   `ui_loading` and `ui_saveicon` both contain multi-second authored timelines, so they should be treated as stateful overlays rather than instantaneous indicators.

## Confidence And Remaining Gaps

High confidence:

- boss HUD package ownership
- direct `ui_result` / `ui_result_ex` readable-code correlation
- save/load split between readable hooks and `#Application`
- mission archive cluster being mostly setup data rather than hidden UI layouts

Still partial:

- no readable C++ seam in the current publishable slice directly names `ui_missionscreen` or `ui_misson`
- result-audio and medal/rank award logic likely span additional runtime or generated seams not mapped in this beat
- the current report reconstructs mission/result ownership well, but not every mission subtype has been screen-by-screen profiled yet
