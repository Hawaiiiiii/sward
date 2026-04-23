<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Source-Path-Named Debug Selector

Phase 33 upgrades the standalone selector from contract-stem browsing into source-family browsing.

> [!IMPORTANT]
> This still runs on the repo-safe runtime contracts. The new part is the naming layer: the selector can now speak in recovered source-family terms such as `TitleMenu.cpp`, `HudPause.cpp`, and `WorldMapSelect.cpp` instead of only `title`, `pause`, or raw contract filenames.

## Launch Families

- Source-path launch families: `7`

| Family | Contract | Seed paths | Example anchors |
|---|---|---:|---|
| Loading And Start/Clear | `loading_transition_reference.json` | `8` | `HUD/Install/InstallDisplay.cpp`, `HUD/Loading/Loading.cpp`, `Sequence/Unit/SequenceUnitChangeStage.cpp` |
| Mission Result Family | `mission_result_reference.json` | `4` | `HUD/Common/Result/HudResult.cpp`, `HUD/Mission/HudMissionFinish.cpp`, `HUD/Mission/HudSelectMissionFailed.cpp` |
| Pause Stack | `pause_menu_reference.json` | `7` | `HUD/GeneralWindow/GeneralWindow.cpp`, `HUD/HelpWindow/HelpWindow.cpp`, `HUD/Pause/HudPause.cpp` |
| Save And Ending | `autosave_toast_reference.json` | `7` | `Sequence/Unit/SequenceUnitAutoSave.cpp`, `Sequence/Unit/SequenceUnitRegisterClearFlag.cpp`, `System/GameMode/Ending/EndingImageList.cpp` |
| Subtitle / Cutscene Presentation | `subtitle_cutscene_reference.json` | `32` | `Inspire/InspireAnimationCurve.cpp`, `Inspire/InspireDataLoader.cpp`, `Inspire/InspireLetterbox.cpp` |
| Title Menu | `title_menu_reference.json` | `9` | `HUD/StageSelect/HudStageSelect.cpp`, `System/GameMode/GameModeMainMenu.cpp`, `System/GameMode/GameModeStageMainMenu.cpp` |
| World Map Stack | `world_map_reference.json` | `7` | `System/GameMode/Title/TitleStateWorldMap.cpp`, `System/GameMode/WorldMap/WorldMapListBox.cpp`, `System/GameMode/WorldMap/WorldMapMission.cpp` |

## Example Family Tokens

- `Loading And Start/Clear`: `InstallDisplay.cpp`, `Loading.cpp`, `SequenceUnitChangeStage.cpp`, `Loading And Start/Clear`, `loading_and_start`
- `Mission Result Family`: `HudResult.cpp`, `HudMissionFinish.cpp`, `HudSelectMissionFailed.cpp`, `Mission Result Family`, `mission_result_family`
- `Pause Stack`: `GeneralWindow.cpp`, `HelpWindow.cpp`, `HudPause.cpp`, `Pause Stack`, `pause_stack`
- `Save And Ending`: `SequenceUnitAutoSave.cpp`, `SequenceUnitRegisterClearFlag.cpp`, `EndingImageList.cpp`, `Save And Ending`, `save_and_ending`
- `Subtitle / Cutscene Presentation`: `InspireAnimationCurve.cpp`, `InspireDataLoader.cpp`, `InspireLetterbox.cpp`, `Subtitle / Cutscene Presentation`, `subtitle_cutscene_presentation`
- `Title Menu`: `HudStageSelect.cpp`, `GameModeMainMenu.cpp`, `GameModeStageMainMenu.cpp`, `Title Menu`, `title_menu`
- `World Map Stack`: `TitleStateWorldMap.cpp`, `WorldMapListBox.cpp`, `WorldMapMission.cpp`, `World Map Stack`, `world_map_stack`

## Selector Direction

- The selector can now treat source-family aliases as first-class launch tokens.
- The current launch set is still bounded by the six contract-backed systems, but the tokens now line up with the mirrored source-family tree and the executable path dump.
- This is the bridge needed before the richer debug sandbox can be widened with gameplay HUD and subtitle/cutscene families.
