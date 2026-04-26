#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace UiLab
{
    enum class ScreenId : uint8_t
    {
        TitleLoop,
        TitleMenu,
        Loading,
        SonicHud,
        Result,
        Status,
        Tutorial,
        WorldMap
    };

    struct RuntimeTarget
    {
        ScreenId id;
        std::string_view token;
        std::string_view label;
        std::string_view primaryCsdScene;
        std::string_view sourceFamily;
        bool requiresStageContext;
    };

    void ConfigureFromCommandLine(int argc, char* argv[]);
    void ApplyConfigOverrides();

    bool IsEnabled();
    bool ShouldBypassStartupPromptBlockers();
    ScreenId GetTarget();
    std::string_view GetTargetToken();
    std::string_view GetTargetLabel();
    std::string_view GetRouteStatusLabel();
    std::string_view GetStageHarnessLabel();
    const std::array<RuntimeTarget, 8>& GetRuntimeTargets();
    void RequestRouteToCurrentTarget();
    void SelectPreviousTarget();
    void SelectNextTarget();

    void OnTitleStateIntroUpdate(float elapsedSeconds);
    void OnTitleStateMenuUpdate(int32_t cursorIndex);
    void OnStageExitLoading();
    void OnPresentedFrame();
    void OnLoadingRequest(uint32_t displayType);
    void OnLoadingUpdate(uint32_t displayType);
    void OnCsdProjectMade(std::string_view projectName);
    bool ApplyTitleIntroStateForcing(float elapsedSeconds);
    bool ApplyTitleMenuStateForcing(int32_t& cursorIndex, bool& injectAccept, bool& suppressAccept);
    void DrawOverlay();
}
