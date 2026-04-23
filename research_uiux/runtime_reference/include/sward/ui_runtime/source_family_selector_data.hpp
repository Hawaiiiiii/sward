#pragma once

#include <array>
#include <string_view>

namespace sward::ui_runtime
{
struct SourceFamilySelectorEntry
{
    std::string_view familyId;
    std::string_view displayName;
    std::string_view sourceSystemId;
    std::string_view contractFileName;
    std::string_view aliasBlob;
    std::string_view sourcePathBlob;
};

inline constexpr std::array<SourceFamilySelectorEntry, 6> kSourceFamilySelectorEntries{{
    {
        "loading_and_start",
        "Loading And Start/Clear",
        "loading_and_start",
        "loading_transition_reference.json",
        "Loading And Start/Clear|loading_and_start|loading_transition_reference.json|loading_transition_reference|Loading / Boot / Install|loading_and_boot|HUD/Loading/Loading.cpp|Loading.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\Loading\\Loading.cpp|System/GameMode/GameModeStageInstall.cpp|GameModeStageInstall.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\GameModeStageInstall.cpp|System/GameMode/GameModeStageLogo.cpp|GameModeStageLogo.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\GameModeStageLogo.cpp",
        "HUD/Loading/Loading.cpp|System/GameMode/GameModeStageInstall.cpp|System/GameMode/GameModeStageLogo.cpp",
    },
    {
        "mission_result_family",
        "Mission Result Family",
        "mission_result_family",
        "mission_result_reference.json",
        "Mission Result Family|mission_result_family|mission_result_reference.json|mission_result_reference|mission_result|HUD/Common/Result/HudResult.cpp|HudResult.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\Common\\Result\\HudResult.cpp|HUD/Mission/HudMissionFinish.cpp|HudMissionFinish.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\Mission\\HudMissionFinish.cpp|HUD/Mission/HudSelectMissionFailed.cpp|HudSelectMissionFailed.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\Mission\\HudSelectMissionFailed.cpp|System/Mission/ObjMissionClear.cpp|ObjMissionClear.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\Mission\\ObjMissionClear.cpp",
        "HUD/Common/Result/HudResult.cpp|HUD/Mission/HudMissionFinish.cpp|HUD/Mission/HudSelectMissionFailed.cpp|System/Mission/ObjMissionClear.cpp",
    },
    {
        "pause_stack",
        "Pause Stack",
        "pause_stack",
        "pause_menu_reference.json",
        "Pause Stack|pause_stack|pause_menu_reference.json|pause_menu_reference|HUD/GeneralWindow/GeneralWindow.cpp|GeneralWindow.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\GeneralWindow\\GeneralWindow.cpp|HUD/HelpWindow/HelpWindow.cpp|HelpWindow.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\HelpWindow\\HelpWindow.cpp|HUD/Pause/HudPause.cpp|HudPause.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\Pause\\HudPause.cpp|HUD/Pause/Item/HudItemList.cpp|HudItemList.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\Pause\\Item\\HudItemList.cpp|HUD/Pause/Skill/HudSkillList.cpp|HudSkillList.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\Pause\\Skill\\HudSkillList.cpp|HUD/Pause/Skill/SkillParamManager.cpp|SkillParamManager.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\Pause\\Skill\\SkillParamManager.cpp",
        "HUD/GeneralWindow/GeneralWindow.cpp|HUD/HelpWindow/HelpWindow.cpp|HUD/Pause/HudPause.cpp|HUD/Pause/Item/HudItemList.cpp|HUD/Pause/Skill/HudSkillList.cpp|HUD/Pause/Skill/SkillParamManager.cpp",
    },
    {
        "save_and_ending",
        "Save And Ending",
        "save_and_ending",
        "autosave_toast_reference.json",
        "Save And Ending|save_and_ending|autosave_toast_reference.json|autosave_toast_reference|Save / Ending Flow|Sequence/Unit/SequenceUnitAutoSave.cpp|SequenceUnitAutoSave.cpp|D:\\SonicWorldAdventure\\SWA\\source\\Sequence\\Unit\\SequenceUnitAutoSave.cpp|Sequence/Unit/SequenceUnitRegisterClearFlag.cpp|SequenceUnitRegisterClearFlag.cpp|D:\\SonicWorldAdventure\\SWA\\source\\Sequence\\Unit\\SequenceUnitRegisterClearFlag.cpp|System/GameMode/Ending/EndingImageList.cpp|EndingImageList.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\Ending\\EndingImageList.cpp|System/GameMode/Ending/EndingManager.cpp|EndingManager.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\Ending\\EndingManager.cpp|System/GameMode/Ending/EndingText.cpp|EndingText.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\Ending\\EndingText.cpp|System/GameMode/GameModeEnding.cpp|GameModeEnding.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\GameModeEnding.cpp|System/GameMode/GameModeStageSaveTest.cpp|GameModeStageSaveTest.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\GameModeStageSaveTest.cpp|System/SaveLoadTest.cpp|SaveLoadTest.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\SaveLoadTest.cpp",
        "Sequence/Unit/SequenceUnitAutoSave.cpp|Sequence/Unit/SequenceUnitRegisterClearFlag.cpp|System/GameMode/Ending/EndingImageList.cpp|System/GameMode/Ending/EndingManager.cpp|System/GameMode/Ending/EndingText.cpp|System/GameMode/GameModeEnding.cpp|System/GameMode/GameModeStageSaveTest.cpp|System/SaveLoadTest.cpp",
    },
    {
        "title_menu",
        "Title Menu",
        "title_menu",
        "title_menu_reference.json",
        "Title Menu|title_menu|title_menu_reference.json|title_menu_reference|Title / Main Menu|HUD/StageSelect/HudStageSelect.cpp|HudStageSelect.cpp|D:\\SonicWorldAdventure\\SWA\\source\\HUD\\StageSelect\\HudStageSelect.cpp|System/GameMode/GameModeMainMenu.cpp|GameModeMainMenu.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\GameModeMainMenu.cpp|System/GameMode/GameModeStageMainMenu.cpp|GameModeStageMainMenu.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\GameModeStageMainMenu.cpp|System/GameMode/GameModeStageTitle.cpp|GameModeStageTitle.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\GameModeStageTitle.cpp|System/GameMode/GameModeTitleSelect.cpp|GameModeTitleSelect.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\GameModeTitleSelect.cpp|System/GameMode/MainMenu/MainMenuManager.cpp|MainMenuManager.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\MainMenu\\MainMenuManager.cpp|System/GameMode/Title/TitleManager.cpp|TitleManager.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\Title\\TitleManager.cpp|System/GameMode/Title/TitleMenu.cpp|TitleMenu.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\Title\\TitleMenu.cpp|System/GameMode/Title/TitleStateIntro.cpp|TitleStateIntro.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\Title\\TitleStateIntro.cpp",
        "HUD/StageSelect/HudStageSelect.cpp|System/GameMode/GameModeMainMenu.cpp|System/GameMode/GameModeStageMainMenu.cpp|System/GameMode/GameModeStageTitle.cpp|System/GameMode/GameModeTitleSelect.cpp|System/GameMode/MainMenu/MainMenuManager.cpp|System/GameMode/Title/TitleManager.cpp|System/GameMode/Title/TitleMenu.cpp|System/GameMode/Title/TitleStateIntro.cpp",
    },
    {
        "world_map_stack",
        "World Map Stack",
        "world_map_stack",
        "world_map_reference.json",
        "World Map Stack|world_map_stack|world_map_reference.json|world_map_reference|System/GameMode/Title/TitleStateWorldMap.cpp|TitleStateWorldMap.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\Title\\TitleStateWorldMap.cpp|System/GameMode/WorldMap/WorldMapListBox.cpp|WorldMapListBox.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\WorldMap\\WorldMapListBox.cpp|System/GameMode/WorldMap/WorldMapMission.cpp|WorldMapMission.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\WorldMap\\WorldMapMission.cpp|System/GameMode/WorldMap/WorldMapObject.cpp|WorldMapObject.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\WorldMap\\WorldMapObject.cpp|System/GameMode/WorldMap/WorldMapSelect.cpp|WorldMapSelect.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\WorldMap\\WorldMapSelect.cpp|System/GameMode/WorldMap/WorldMapSimpleInfo.cpp|WorldMapSimpleInfo.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\WorldMap\\WorldMapSimpleInfo.cpp|System/GameMode/WorldMap/WorldMapTutorial.cpp|WorldMapTutorial.cpp|D:\\SonicWorldAdventure\\SWA\\source\\System\\GameMode\\WorldMap\\WorldMapTutorial.cpp",
        "System/GameMode/Title/TitleStateWorldMap.cpp|System/GameMode/WorldMap/WorldMapListBox.cpp|System/GameMode/WorldMap/WorldMapMission.cpp|System/GameMode/WorldMap/WorldMapObject.cpp|System/GameMode/WorldMap/WorldMapSelect.cpp|System/GameMode/WorldMap/WorldMapSimpleInfo.cpp|System/GameMode/WorldMap/WorldMapTutorial.cpp",
    },
}};
} // namespace sward::ui_runtime
