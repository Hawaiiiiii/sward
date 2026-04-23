<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Local Debug Source Tree Deepening

Phase 44 pushes the local-only mirrored `SONIC UNLEASHED/` tree past the first scaffold wave and deeper into a readable debug-oriented source base.

> [!IMPORTANT]
> The mirrored tree remains local-only. The tracked repo carries the materializer, summaries, and reports, not the mirrored translated source files themselves.

## Current Local-Only State

- newly materialized readable `.cpp` files in this pass: `80`
- total readable `.cpp` files under `SONIC UNLEASHED/`: `92`
- tracked expansion groups: `5`

## Deepened Groups

- `Gameplay HUD Sources`: `12`
- `Stage Test Sources`: `9`
- `Town / Media Room Sources`: `19`
- `Camera Shell Sources`: `8`
- `Application / World Shell Sources`: `32`

## Notable Additions

Town/media-room shell deepening now includes readable ownership stubs for:

- `TownManager.cpp`
- `TalkWindow.cpp`
- `ShopWindow.cpp`
- `HotdogParamManager.cpp`
- `HotdogSaveManager.cpp`
- `ItemParamManager.cpp`
- `KyojuParamManager.cpp`
- `KyojuPresentManager.cpp`
- `SunafkinParamManager.cpp`
- `TimeTableManager.cpp`

Camera shell deepening now includes:

- `FreeCamera.cpp`
- `GoalCamera.cpp`
- `ReplayFreeCamera.cpp`
- `ReplayRelativeCamera.cpp`
- `Player2DBossCamera.cpp`
- `Player3DBossCamera.cpp`
- `Player3DFinalDarkGaiaCamera.cpp`
- `PlayerSuperSonicCamera.cpp`

Application/world shell deepening now includes:

- `Application.cpp`
- `ApplicationDocument.cpp`
- `Game.cpp`
- `GameDocument.cpp`
- `GameModeBoot.cpp`
- `GameModeMainMenu.cpp`
- `TitleManager.cpp`
- `TitleMenu.cpp`
- `TitleStateIntro.cpp`
- `TitleStateWorldMap.cpp`
- `MainMenuManager.cpp`
- `WorldMapListBox.cpp`
- `WorldMapMission.cpp`
- `WorldMapObject.cpp`
- `WorldMapSelect.cpp`
- `WorldMapSimpleInfo.cpp`
- `WorldMapTutorial.cpp`

## What This Means

The mirrored tree is still not a full humanized whole-game source base, but it is now materially closer to one:

- more recovered paths have readable translated ownership instead of only `*.sward.md` notes
- town/camera/application shells now align with real portable runtime contracts
- the debug selector/workbench and the local mirrored source tree now point at the same recovered families

That is the right direction if the end goal is a broader debug-oriented, source-family-organized, human-readable translation base for `1:1` UI/UX study and forward-port templates.
