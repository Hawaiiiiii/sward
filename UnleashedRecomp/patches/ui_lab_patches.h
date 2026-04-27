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
        Pause,
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
    bool IsLiveBridgeEnabled();
    std::string_view GetLiveBridgeName();
    std::string BuildLiveStateJson();
    const std::array<RuntimeTarget, 11>& GetRuntimeTargets();
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
    void OnStageTargetReady(std::string_view eventName, std::string_view detail);
    bool ApplyPauseRouteInput(std::string_view hookSource = {});
    void OnPresentedFrame();
    void WriteLiveStateSnapshot();
    std::string ConsumeNativeFrameCapturePath(uint32_t width, uint32_t height);
    void OnNativeFrameCaptured(std::string_view path, uint32_t width, uint32_t height);
    void OnNativeFrameCaptureFailed(std::string_view reason);
    void OnLoadingRequest(uint32_t displayType);
    void OnLoadingUpdate(uint32_t displayType);
    void OnCsdProjectMade(std::string_view projectName);
    void OnCsdProjectTreeMade(std::string_view projectName, uint32_t projectAddress, uint32_t rootNodeAddress);
    void OnCsdSceneNodeTraversed(
        std::string_view projectName,
        std::string_view nodePath,
        uint32_t nodeAddress,
        uint32_t sceneCount,
        uint32_t childNodeCount);
    void OnCsdSceneTraversed(
        std::string_view projectName,
        std::string_view scenePath,
        uint32_t sceneAddress,
        uint32_t castNodeCount,
        uint32_t castCount);
    void OnCsdLayerTraversed(
        std::string_view projectName,
        std::string_view layerPath,
        uint32_t layerAddress,
        uint32_t castNodeAddress,
        uint32_t castNodeIndex,
        uint32_t castIndex);
    void OnHudPauseUpdate(
        uint32_t pauseAddress,
        uint32_t pauseProjectAddress,
        uint32_t bgSceneAddress,
        uint32_t action,
        uint32_t menu,
        uint32_t status,
        uint32_t transition,
        bool isVisible,
        bool isShown);
    void OnHudSonicStageUpdate(
        uint32_t ownerAddress,
        uint32_t playScreenProjectAddress,
        uint32_t speedGaugeSceneAddress,
        uint32_t ringEnergyGaugeSceneAddress,
        uint32_t gaugeFrameSceneAddress,
        std::string_view hookSource);
    void OnGeneralWindowUpdate(
        uint32_t generalWindowAddress,
        uint32_t generalProjectAddress,
        uint32_t bgSceneAddress,
        uint32_t status,
        uint32_t cursorIndex,
        uint32_t selectedIndex);
    void OnSaveIconUpdate(uint32_t saveIconAddress, bool isVisible);
    bool ApplyTitleIntroStateForcing(float elapsedSeconds, bool& directState);
    bool ShouldArmTitleIntroOwnerOutput();
    bool ShouldArmTitleIntroCsdCompletion();
    bool ShouldHoldTitleMenuRuntime();
    bool ApplyTitleMenuStateForcing(int32_t& cursorIndex, bool& injectAccept, bool& suppressAccept, bool& directContext);
    void DrawOverlay();
}
