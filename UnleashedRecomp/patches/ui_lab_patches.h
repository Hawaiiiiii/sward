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
    bool ShouldRefreshStageTitleOwnerDirectState();
    void OnStageTitleOwnerDirectStateApplied(
        uint32_t titleContextAddress,
        uint32_t titleCsdAddress,
        uint8_t requestedState,
        uint8_t dirtyFlag,
        uint8_t transitionArmed,
        uint8_t outputArmed,
        uint8_t ownerGateArmed,
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
    bool IsUiOnlyRenderTargetCaptureRequested();
    std::string ConsumeUiOnlyRenderTargetCapturePath(
        uint32_t width,
        uint32_t height,
        std::string_view source,
        bool containsFullFramebuffer);
    void OnUiOnlyRenderTargetCaptured(
        std::string_view path,
        uint32_t width,
        uint32_t height,
        std::string_view source,
        bool containsFullFramebuffer);
    void OnUiOnlyRenderTargetCaptureFailed(std::string_view reason);
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
    void OnCsdPlatformDraw(
        uint32_t layerAddress,
        uint32_t castNodeAddress,
        uint32_t vertexBufferAddress,
        uint32_t vertexCount,
        uint32_t vertexStride,
        bool textured,
        float minX,
        float minY,
        float maxX,
        float maxY,
        uint32_t colorSample);
    void OnCsdNodeSetText(
        uint32_t nodeAddress,
        uint32_t textAddress,
        std::string_view textUtf8,
        std::string_view hookSource);
    void OnBackendMaterialSubmit(
        std::string_view source,
        uint32_t primitiveType,
        uint32_t primitiveTopology,
        bool indexed,
        bool inlineVertexStream,
        uint32_t vertexCount,
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t startVertex,
        uint32_t startIndex,
        int32_t baseVertex,
        uint32_t vertexStride,
        uint32_t texture2DDescriptorIndex,
        uint32_t samplerDescriptorIndex,
        bool alphaBlendEnable,
        uint32_t srcBlend,
        uint32_t destBlend,
        uint32_t blendOp,
        uint32_t srcBlendAlpha,
        uint32_t destBlendAlpha,
        uint32_t blendOpAlpha,
        uint32_t colorWriteEnable,
        bool alphaTestEnable,
        float alphaThreshold,
        bool scissorEnable,
        int32_t scissorLeft,
        int32_t scissorTop,
        int32_t scissorRight,
        int32_t scissorBottom,
        uint32_t samplerMinFilter,
        uint32_t samplerMagFilter,
        uint32_t samplerMipMode,
        uint32_t samplerAddressU,
        uint32_t samplerAddressV,
        uint32_t samplerAddressW,
        float halfPixelOffsetX,
        float halfPixelOffsetY);
    void OnBackendTextureDescriptorResolved(
        uint32_t descriptorIndex,
        std::string_view source,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint32_t format,
        uint32_t viewDimension,
        uint32_t layout);
    void OnBackendSamplerDescriptorResolved(
        uint32_t descriptorIndex,
        uint32_t minFilter,
        uint32_t magFilter,
        uint32_t mipMode,
        uint32_t addressU,
        uint32_t addressV,
        uint32_t addressW,
        float mipLodBias,
        uint32_t maxAnisotropy,
        bool anisotropyEnabled,
        bool comparisonEnabled,
        uint32_t borderColor,
        float minLod,
        float maxLod);
    void OnVendorTextureResourceViewResolved(
        std::string_view backend,
        uint32_t descriptorIndex,
        uint64_t nativeTextureResourceHandle,
        uint64_t nativeTextureViewHandle,
        uint32_t nativeFormat,
        uint32_t nativeViewDimension,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        std::string_view source);
    void OnVendorSamplerResourceViewResolved(
        std::string_view backend,
        uint32_t descriptorIndex,
        uint64_t nativeSamplerHandle,
        uint32_t nativeFilter,
        uint32_t nativeAddressU,
        uint32_t nativeAddressV,
        uint32_t nativeAddressW,
        std::string_view source);
    void OnRawBackendCommand(
        std::string_view backend,
        std::string_view command,
        std::string_view source,
        bool indexed,
        uint32_t vertexCount,
        uint32_t indexCount,
        uint32_t instanceCount);
    void OnResolvedBackendSubmit(
        std::string_view backend,
        std::string_view nativeCommand,
        bool indexed,
        uint32_t vertexCount,
        uint32_t indexCount,
        uint32_t instanceCount,
        uint64_t nativePipelineHandle,
        uint64_t nativePipelineLayoutHandle,
        bool resolvedPipelineKnown,
        bool activeFramebufferKnown,
        uint32_t framebufferWidth,
        uint32_t framebufferHeight,
        uint32_t renderTargetCount,
        uint32_t renderTargetFormat0,
        uint32_t depthTargetFormat,
        uint32_t sampleCount,
        uint32_t primitiveTopology,
        bool blendEnabled,
        uint32_t srcBlend,
        uint32_t destBlend,
        uint32_t blendOp,
        uint32_t srcBlendAlpha,
        uint32_t destBlendAlpha,
        uint32_t blendOpAlpha,
        uint32_t renderTargetWriteMask,
        uint32_t inputSlotCount,
        uint32_t inputElementCount,
        bool depthEnabled,
        bool depthWriteEnabled,
        bool alphaToCoverageEnabled);
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
        uint32_t scoreCountNodeAddress,
        uint32_t timeCountNodeAddress,
        uint32_t timeCount2NodeAddress,
        uint32_t timeCount3NodeAddress,
        uint32_t playerCountNodeAddress,
        std::string_view hookSource);
    void OnHudSonicStageOwnerFieldSample(uint32_t ownerAddress, std::string_view hookSource);
    void OnSonicHudGameplayValues(
        uint32_t ringCount,
        bool ringCountKnown,
        uint32_t score,
        bool scoreKnown,
        uint32_t elapsedFrames,
        bool elapsedFramesKnown,
        float speedKmh,
        bool speedKmhKnown,
        float boostGauge,
        bool boostGaugeKnown,
        float ringEnergyGauge,
        bool ringEnergyGaugeKnown,
        uint32_t lifeCount,
        bool lifeCountKnown,
        std::string_view tutorialPromptId,
        bool tutorialPromptKnown,
        bool tutorialVisible,
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
