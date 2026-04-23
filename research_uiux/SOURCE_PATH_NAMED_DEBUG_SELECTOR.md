<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Source-Path-Named Debug Selector

Phase 33 upgrades the standalone selector from contract-stem browsing into source-family browsing.

> [!IMPORTANT]
> This still runs on the repo-safe runtime contracts. The new part is the naming layer: the selector can now speak in recovered source-family terms such as `TitleMenu.cpp`, `HudPause.cpp`, and `WorldMapSelect.cpp` instead of only `title`, `pause`, or raw contract filenames.

## Launch Families

- Source-path launch families: `12`

| Family | Contract | Seed paths | Example anchors |
|---|---|---:|---|
| Boss HUD | `boss_hud_reference.json` | `2` | `Boss/FinalDarkGaia/Object/FinalDarkGaiaHud.cpp`, `Boss/Phoenix/Hud/PhoenixHudVitality.cpp` |
| Extra Stage / Tornado Defense HUD | `extra_stage_hud_reference.json` | `1` | `ExtraStage/Tails/Hud/HudExQte.cpp`, `System/GameMode/GameModeStageMotionTest.cpp` |
| Loading And Start/Clear | `loading_transition_reference.json` | `8` | `HUD/Install/InstallDisplay.cpp`, `HUD/Loading/Loading.cpp`, `Sequence/Unit/SequenceUnitChangeStage.cpp` |
| Mission Result Family | `mission_result_reference.json` | `4` | `HUD/Common/Result/HudResult.cpp`, `HUD/Mission/HudMissionFinish.cpp`, `HUD/Mission/HudSelectMissionFailed.cpp` |
| Pause Stack | `pause_menu_reference.json` | `7` | `HUD/GeneralWindow/GeneralWindow.cpp`, `HUD/HelpWindow/HelpWindow.cpp`, `HUD/Pause/HudPause.cpp` |
| Save And Ending | `autosave_toast_reference.json` | `7` | `Sequence/Unit/SequenceUnitAutoSave.cpp`, `Sequence/Unit/SequenceUnitRegisterClearFlag.cpp`, `System/GameMode/Ending/EndingImageList.cpp` |
| Sonic Stage HUD | `sonic_stage_hud_reference.json` | `4` | `HUD/Sonic/HudSonicStage.cpp`, `HUD/Sonic/SonicMainDisplay.cpp`, `Player/Character/Sonic/Hud/SonicHudGuide.cpp` |
| Subtitle / Cutscene Presentation | `subtitle_cutscene_reference.json` | `32` | `Inspire/InspireAnimationCurve.cpp`, `Inspire/InspireDataLoader.cpp`, `Inspire/InspireLetterbox.cpp` |
| Super Sonic / Final HUD Bridge | `super_sonic_hud_reference.json` | `3` | `Boss/BossHudSuperSonic.cpp`, `Boss/BossHudVitality.cpp`, `Boss/BossNamePlate.cpp` |
| Title Menu | `title_menu_reference.json` | `9` | `HUD/StageSelect/HudStageSelect.cpp`, `System/GameMode/GameModeMainMenu.cpp`, `System/GameMode/GameModeStageMainMenu.cpp` |
| Werehog Stage HUD | `werehog_stage_hud_reference.json` | `4` | `HUD/Evil/EvilMainDisplay.cpp`, `HUD/Evil/HudEvilStage.cpp`, `Player/Character/EvilSonic/Hud/EvilHudGuide.cpp` |
| World Map Stack | `world_map_reference.json` | `7` | `System/GameMode/Title/TitleStateWorldMap.cpp`, `System/GameMode/WorldMap/WorldMapListBox.cpp`, `System/GameMode/WorldMap/WorldMapMission.cpp` |

## Example Family Tokens

- `Boss HUD`: `FinalDarkGaiaHud.cpp`, `PhoenixHudVitality.cpp`, `Boss HUD`, `boss_hud`
- `Extra Stage / Tornado Defense HUD`: `HudExQte.cpp`, `GameModeStageMotionTest.cpp`, `Extra Stage / Tornado Defense HUD`, `extra_stage_hud`
- `Loading And Start/Clear`: `InstallDisplay.cpp`, `Loading.cpp`, `SequenceUnitChangeStage.cpp`, `Loading And Start/Clear`, `loading_and_start`
- `Mission Result Family`: `HudResult.cpp`, `HudMissionFinish.cpp`, `HudSelectMissionFailed.cpp`, `Mission Result Family`, `mission_result_family`
- `Pause Stack`: `GeneralWindow.cpp`, `HelpWindow.cpp`, `HudPause.cpp`, `Pause Stack`, `pause_stack`
- `Save And Ending`: `SequenceUnitAutoSave.cpp`, `SequenceUnitRegisterClearFlag.cpp`, `EndingImageList.cpp`, `Save And Ending`, `save_and_ending`
- `Sonic Stage HUD`: `HudSonicStage.cpp`, `SonicMainDisplay.cpp`, `SonicHudGuide.cpp`, `Sonic Stage HUD`, `sonic_stage_hud`
- `Subtitle / Cutscene Presentation`: `InspireAnimationCurve.cpp`, `InspireDataLoader.cpp`, `InspireLetterbox.cpp`, `Subtitle / Cutscene Presentation`, `subtitle_cutscene_presentation`
- `Super Sonic / Final HUD Bridge`: `BossHudSuperSonic.cpp`, `BossHudVitality.cpp`, `BossNamePlate.cpp`, `Super Sonic / Final HUD Bridge`, `super_sonic_hud`
- `Title Menu`: `HudStageSelect.cpp`, `GameModeMainMenu.cpp`, `GameModeStageMainMenu.cpp`, `Title Menu`, `title_menu`
- `Werehog Stage HUD`: `EvilMainDisplay.cpp`, `HudEvilStage.cpp`, `EvilHudGuide.cpp`, `Werehog Stage HUD`, `werehog_stage_hud`
- `World Map Stack`: `TitleStateWorldMap.cpp`, `WorldMapListBox.cpp`, `WorldMapMission.cpp`, `World Map Stack`, `world_map_stack`

## Selector Direction

- The selector can now treat source-family aliases as first-class launch tokens.
- The current launch set now spans the bundled frontend, cutscene, gameplay-HUD, and boss/final runtime families, while keeping the tokens aligned with the mirrored source-family tree and the executable path dump.
- The next value is widening readable translated ownership and adding broader town/camera/application/world host coverage on top of this named selector layer.
