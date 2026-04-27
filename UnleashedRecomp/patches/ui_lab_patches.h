#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace UiLab
{
    enum class ScreenId : uint8_t
    {
        TitleLoop,
        TitleMenu,
        TitleOptions,
        Loading,
        SonicHud,
        ExtraStageHud,
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
    bool IsObserverMode();
    bool IsNativeFrameCaptureEnabled();
    bool ShouldBypassStartupPromptBlockers();
    ScreenId GetTarget();
    std::string_view GetTargetToken();
    std::string_view GetTargetLabel();
    std::string_view GetRouteStatusLabel();
    std::string_view GetStageHarnessLabel();
    std::string_view GetTargetCsdStatusLabel();
    const std::array<RuntimeTarget, 10>& GetRuntimeTargets();
    void RequestRouteToCurrentTarget();
    void SelectPreviousTarget();
    void SelectNextTarget();
    bool ShouldReserveF1DebugToggle();
    void UpdateOperatorShellToggle(bool f1Down);

    void OnTitleStateIntroUpdate(float elapsedSeconds);
    void OnTitleIntroContext(
        uint32_t contextAddress,
        uint32_t stateMachineAddress,
        float elapsedSeconds,
        uint8_t requestedState,
        uint8_t dirtyFlag,
        uint8_t transitionArmed,
        uint8_t contextFlag580,
        uint32_t context472,
        uint32_t context480,
        uint32_t context488);
    void OnTitleIntroDirectStateApplied(
        uint8_t requestedState,
        uint8_t dirtyFlag,
        uint8_t transitionArmed,
        uint8_t outputArmed,
        uint8_t csdCompleteArmed);
    void OnGameModeStageTitleContext(
        uint32_t gameModeAddress,
        uint32_t contextAddress,
        uint32_t stateMachineAddress,
        float stateTime,
        bool isTitleStateMenu,
        bool isAutoSaveWarningShown,
        std::string_view ownerDetail);
    void OnTitleOwnerContext(
        bool isTitleStateMenu,
        uint32_t titleContextAddress,
        uint32_t titleCsdAddress,
        bool ownerGate568,
        bool ownerGate570,
        uint8_t titleRequest,
        uint8_t titleDirty,
        uint8_t titleTransition,
        uint8_t titleFlag580,
        uint8_t csdByte62,
        uint8_t csdByte84,
        uint8_t csdByte152,
        uint8_t csdByte160);
    void OnTitleStateMenuUpdate(int32_t cursorIndex);
    void OnTitleMenuContext(
        uint32_t context472,
        uint32_t context480,
        uint32_t context488,
        uint32_t contextPhase,
        uint8_t contextFlag580,
        uint32_t menuCursor,
        bool menuField3C,
        bool menuField54,
        bool menuField9A);
    void OnStageExitLoading(uint32_t gameModeStageAddress = 0);
    void OnPresentedFrame();
    std::string ConsumeNativeFrameCapturePath(uint32_t width, uint32_t height);
    void OnNativeFrameCaptured(std::string_view path, uint32_t width, uint32_t height);
    void OnNativeFrameCaptureFailed(std::string_view reason);
    void OnLoadingRequest(uint32_t displayType);
    void OnLoadingUpdate(uint32_t displayType);
    void OnCsdProjectMade(std::string_view projectName);
    bool ApplyTitleIntroStateForcing(float elapsedSeconds, bool& directState);
    bool ShouldArmTitleIntroOwnerOutput();
    bool ShouldArmTitleIntroCsdCompletion();
    bool ShouldHoldTitleMenuRuntime();
    bool ApplyTitleMenuStateForcing(int32_t& cursorIndex, bool& injectAccept, bool& suppressAccept, bool& directContext);
    void DrawOverlay();
}
