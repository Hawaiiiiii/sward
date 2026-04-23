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
    WorldMap,
};

[[nodiscard]] ScreenContract makePauseMenuContract();
[[nodiscard]] ScreenContract makeTitleMenuContract();
[[nodiscard]] ScreenContract makeAutosaveToastContract();
[[nodiscard]] ScreenContract makeLoadingTransitionContract();
[[nodiscard]] ScreenContract makeMissionResultContract();
[[nodiscard]] ScreenContract makeWorldMapContract();
[[nodiscard]] ScreenContract makeContract(ReferenceProfile profile);
[[nodiscard]] std::string_view toString(ReferenceProfile profile);
} // namespace sward::ui_runtime
