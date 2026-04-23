#pragma once

#include <sward/ui_runtime/runtime.hpp>

#include <string_view>

namespace sward::ui_runtime
{
enum class ReferenceProfile
{
    PauseMenu,
    TitleMenu,
    AutosaveToast,
    LoadingTransition,
    MissionResult,
    SubtitleCutscene,
    WorldMap,
    SonicStageHud,
    WerehogStageHud,
    ExtraStageHud,
    SuperSonicHud,
    BossHud,
    TownUi,
    CameraShell,
    ApplicationWorldShell,
    FrontendSequenceShell,
};

[[nodiscard]] ScreenContract makePauseMenuContract();
[[nodiscard]] ScreenContract makeTitleMenuContract();
[[nodiscard]] ScreenContract makeAutosaveToastContract();
[[nodiscard]] ScreenContract makeLoadingTransitionContract();
[[nodiscard]] ScreenContract makeMissionResultContract();
[[nodiscard]] ScreenContract makeSubtitleCutsceneContract();
[[nodiscard]] ScreenContract makeWorldMapContract();
[[nodiscard]] ScreenContract makeSonicStageHudContract();
[[nodiscard]] ScreenContract makeWerehogStageHudContract();
[[nodiscard]] ScreenContract makeExtraStageHudContract();
[[nodiscard]] ScreenContract makeSuperSonicHudContract();
[[nodiscard]] ScreenContract makeBossHudContract();
[[nodiscard]] ScreenContract makeTownUiContract();
[[nodiscard]] ScreenContract makeCameraShellContract();
[[nodiscard]] ScreenContract makeApplicationWorldShellContract();
[[nodiscard]] ScreenContract makeFrontendSequenceShellContract();
[[nodiscard]] ScreenContract makeContract(ReferenceProfile profile);
[[nodiscard]] std::string_view toString(ReferenceProfile profile);
} // namespace sward::ui_runtime
