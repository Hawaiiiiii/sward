#include <patches/ui_lab_patches.h>
#include <app.h>
#include <api/SWA.h>
#include <gpu/imgui/imgui_common.h>
#include <gpu/imgui/imgui_snapshot.h>
#include <kernel/memory.h>
#include <os/logger.h>
#include <user/config.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace UiLab
{
    enum class RoutePolicy : uint8_t
    {
        InputInjection,
        DirectContext
    };

    struct TitleIntroInspectorSnapshot
    {
        bool valid = false;
        uint32_t contextAddress = 0;
        uint32_t stateMachineAddress = 0;
        float elapsedSeconds = 0.0f;
        uint8_t requestedState = 0;
        uint8_t dirtyFlag = 0;
        uint8_t transitionArmed = 0;
        uint8_t contextFlag580 = 0;
        uint32_t context472 = 0;
        uint32_t context480 = 0;
        uint32_t context488 = 0;
        uint64_t frame = 0;
    };

    struct TitleOwnerInspectorSnapshot
    {
        bool valid = false;
        bool isTitleStateMenu = false;
        uint32_t titleContextAddress = 0;
        uint32_t titleCsdAddress = 0;
        bool ownerGate568 = false;
        bool ownerGate570 = false;
        uint8_t titleRequest = 0;
        uint8_t titleDirty = 0;
        uint8_t titleTransition = 0;
        uint8_t titleFlag580 = 0;
        uint8_t csdByte62 = 0;
        uint8_t csdByte84 = 0;
        uint8_t csdByte152 = 0;
        uint8_t csdByte160 = 0;
        bool ownerReady = false;
        uint64_t frame = 0;
    };

    struct TitleMenuInspectorSnapshot
    {
        bool valid = false;
        uint32_t context472 = 0;
        uint32_t context480 = 0;
        uint32_t context488 = 0;
        uint32_t contextPhase = 0;
        uint8_t contextFlag580 = 0;
        uint32_t menuCursor = 0;
        bool menuField3C = false;
        bool menuField54 = false;
        bool menuField9A = false;
        bool postPressStartMenuReady = false;
        uint64_t stableFrames = 0;
        uint64_t frame = 0;
    };

    struct CsdLiveInspectorSnapshot
    {
        bool sceneKnown = false;
        uint32_t sceneAddress = 0;
        bool sceneMotionKnown = false;
        float sceneMotionFrame = 0.0f;
        uint32_t sceneMotionRepeatType = UINT32_MAX;
        std::string projectName;
        std::string source;
    };

    struct CsdTreeEntry
    {
        std::string path;
        uint32_t address = 0;
        uint32_t relatedAddress = 0;
        uint32_t firstMetric = 0;
        uint32_t secondMetric = 0;
        uint64_t frame = 0;
    };

    struct RuntimeUiDrawCall
    {
        uint64_t frame = 0;
        uint32_t sequence = 0;
        std::string projectName;
        std::string layerPath;
        uint32_t layerAddress = 0;
        uint32_t castNodeAddress = 0;
        uint32_t vertexBufferAddress = 0;
        uint32_t vertexCount = 0;
        uint32_t vertexStride = 0;
        bool textured = false;
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        uint32_t colorSample = 0;
    };

    struct RuntimeGpuSubmitCall
    {
        uint64_t frame = 0;
        uint32_t sequence = 0;
        std::string source;
        uint32_t primitiveType = 0;
        uint32_t primitiveTopology = 0;
        bool indexed = false;
        bool inlineVertexStream = false;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t instanceCount = 0;
        uint32_t startVertex = 0;
        uint32_t startIndex = 0;
        int32_t baseVertex = 0;
        uint32_t vertexStride = 0;
        uint32_t texture2DDescriptorIndex = 0;
        uint32_t samplerDescriptorIndex = 0;
        bool alphaBlendEnable = false;
        uint32_t srcBlend = 0;
        uint32_t destBlend = 0;
        uint32_t blendOp = 0;
        uint32_t srcBlendAlpha = 0;
        uint32_t destBlendAlpha = 0;
        uint32_t blendOpAlpha = 0;
        uint32_t colorWriteEnable = 0;
        bool alphaTestEnable = false;
        float alphaThreshold = 0.0f;
        bool scissorEnable = false;
        int32_t scissorLeft = 0;
        int32_t scissorTop = 0;
        int32_t scissorRight = 0;
        int32_t scissorBottom = 0;
        uint32_t samplerMinFilter = 0;
        uint32_t samplerMagFilter = 0;
        uint32_t samplerMipMode = 0;
        uint32_t samplerAddressU = 0;
        uint32_t samplerAddressV = 0;
        uint32_t samplerAddressW = 0;
        float halfPixelOffsetX = 0.0f;
        float halfPixelOffsetY = 0.0f;
    };

    struct RuntimeRawBackendCommand
    {
        uint64_t frame = 0;
        uint32_t sequence = 0;
        std::string backend;
        std::string command;
        std::string source;
        bool indexed = false;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t instanceCount = 0;
    };

    struct RuntimeBackendResolvedSubmit
    {
        uint64_t frame = 0;
        uint32_t sequence = 0;
        std::string backend;
        std::string nativeCommand;
        bool indexed = false;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t instanceCount = 0;
        uint64_t nativePipelineHandle = 0;
        uint64_t nativePipelineLayoutHandle = 0;
        bool resolvedPipelineKnown = false;
        bool activeFramebufferKnown = false;
        uint32_t framebufferWidth = 0;
        uint32_t framebufferHeight = 0;
        uint32_t renderTargetCount = 0;
        uint32_t renderTargetFormat0 = 0;
        uint32_t depthTargetFormat = 0;
        uint32_t sampleCount = 0;
        uint32_t primitiveTopology = 0;
        bool blendEnabled = false;
        uint32_t srcBlend = 0;
        uint32_t destBlend = 0;
        uint32_t blendOp = 0;
        uint32_t srcBlendAlpha = 0;
        uint32_t destBlendAlpha = 0;
        uint32_t blendOpAlpha = 0;
        uint32_t renderTargetWriteMask = 0;
        uint32_t inputSlotCount = 0;
        uint32_t inputElementCount = 0;
        bool depthEnabled = false;
        bool depthWriteEnabled = false;
        bool alphaToCoverageEnabled = false;
    };

    struct RuntimeTextureDescriptorSemantic
    {
        uint64_t frame = 0;
        uint32_t descriptorIndex = 0;
        std::string source;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 0;
        uint32_t format = 0;
        uint32_t viewDimension = 0;
        uint32_t layout = 0;
    };

    struct RuntimeSamplerDescriptorSemantic
    {
        uint64_t frame = 0;
        uint32_t descriptorIndex = 0;
        uint32_t minFilter = 0;
        uint32_t magFilter = 0;
        uint32_t mipMode = 0;
        uint32_t addressU = 0;
        uint32_t addressV = 0;
        uint32_t addressW = 0;
        float mipLodBias = 0.0f;
        uint32_t maxAnisotropy = 0;
        bool anisotropyEnabled = false;
        bool comparisonEnabled = false;
        uint32_t borderColor = 0;
        float minLod = 0.0f;
        float maxLod = 0.0f;
    };

    struct RuntimeVendorTextureResourceView
    {
        uint64_t frame = 0;
        uint32_t descriptorIndex = 0;
        std::string backend;
        std::string source;
        uint64_t nativeTextureResourceHandle = 0;
        uint64_t nativeTextureViewHandle = 0;
        uint32_t nativeFormat = 0;
        uint32_t nativeViewDimension = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipLevels = 0;
    };

    struct RuntimeVendorSamplerResourceView
    {
        uint64_t frame = 0;
        uint32_t descriptorIndex = 0;
        std::string backend;
        std::string source;
        uint64_t nativeSamplerHandle = 0;
        uint32_t nativeFilter = 0;
        uint32_t nativeAddressU = 0;
        uint32_t nativeAddressV = 0;
        uint32_t nativeAddressW = 0;
    };

    struct BackendMaterialParityHint
    {
        std::string materialParityHint = "missing";
        bool sourceOverAlpha = false;
        bool additiveAlpha = false;
        bool opaqueNoBlend = false;
        bool customBlend = false;
        bool framebufferRegistered = false;
    };

    struct RuntimeMaterialCorrelation
    {
        uint64_t frame = 0;
        uint32_t uiDrawSequence = 0;
        uint32_t gpuSubmitSequence = 0;
        std::string projectName;
        std::string layerPath;
        std::string gpuSubmitSource;
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        uint32_t texture2DDescriptorIndex = 0;
        uint32_t samplerDescriptorIndex = 0;
        bool alphaBlendEnable = false;
        bool additiveBlend = false;
        bool linearFilter = false;
        bool pointFilter = false;
        std::string blendSemantic;
        std::string blendOperationSemantic;
        std::string samplerSemantic;
        std::string addressSemantic;
        std::string alphaSemantic;
        std::string colorWriteSemantic;
        float halfPixelOffsetX = 0.0f;
        float halfPixelOffsetY = 0.0f;
    };

    struct CsdProjectTreeRecord
    {
        std::string projectName;
        uint32_t projectAddress = 0;
        uint32_t rootNodeAddress = 0;
        uint32_t sceneCount = 0;
        uint32_t nodeCount = 0;
        uint32_t layerCount = 0;
        uint64_t frame = 0;
        std::vector<CsdTreeEntry> scenes;
        std::vector<CsdTreeEntry> nodes;
        std::vector<CsdTreeEntry> layers;
    };

    struct CsdProjectTreeInspectorSnapshot
    {
        bool projectKnown = false;
        std::string activeProject;
        uint32_t projectAddress = 0;
        uint32_t rootNodeAddress = 0;
        uint32_t sceneCount = 0;
        uint32_t nodeCount = 0;
        uint32_t layerCount = 0;
        uint32_t observedProjectCount = 0;
        std::vector<std::string> observedProjects;
        std::vector<CsdTreeEntry> scenes;
        std::vector<CsdTreeEntry> nodes;
        std::vector<CsdTreeEntry> layers;
        std::string source;
    };

    struct LoadingLiveInspectorSnapshot
    {
        uint32_t requestType = UINT32_MAX;
        uint32_t displayType = UINT32_MAX;
        bool displayActive = false;
        uint64_t requestFrame = 0;
        uint64_t displayFrame = 0;
    };

    struct SonicHudLiveInspectorSnapshot
    {
        bool stageContextObserved = false;
        bool targetCsdObserved = false;
        bool stageTargetReady = false;
        bool rawOwnerKnown = false;
        bool rawOwnerFieldsReady = false;
        uint32_t hudOwnerAddress = 0;
        uint32_t stageGameModeAddress = 0;
        uint64_t rawOwnerFrame = 0;
        std::string playScreenProject;
        std::string speedGaugeScene;
        std::string readyEvent;
        std::string rawHookSource;
        std::string source;
    };

    struct SonicHudGameplayValueSnapshot
    {
        std::string source = "typedInspectors.sonicHud.gameplayValues";
        bool ringCountKnown = false;
        uint32_t ringCount = 0;
        std::string ringCountSource = "pending-runtime-field";
        bool scoreKnown = false;
        uint32_t score = 0;
        std::string scoreSource = "SWA::CGameDocument::m_pMember->m_ScoreInfo.EnemyScore+TrickScore";
        bool scoreInfoPointMarkerRecordSpeedKnown = false;
        float scoreInfoPointMarkerRecordSpeed = 0.0f;
        std::string scoreInfoPointMarkerRecordSpeedSource =
            "SWA::CGameDocument::m_pMember->m_ScoreInfo.PointMarkerRecordSpeed";
        bool scoreInfoPointMarkerCountKnown = false;
        uint32_t scoreInfoPointMarkerCount = 0;
        std::string scoreInfoPointMarkerCountSource =
            "SWA::CGameDocument::m_pMember->m_ScoreInfo.PointMarkerCount";
        bool elapsedFramesKnown = false;
        uint32_t elapsedFrames = 0;
        std::string elapsedFramesSource = "pending-runtime-field";
        bool speedKmhKnown = false;
        float speedKmh = 0.0f;
        std::string speedKmhSource = "pending-runtime-field";
        bool boostGaugeKnown = false;
        float boostGauge = 0.0f;
        std::string boostGaugeSource = "pending-runtime-field";
        bool ringEnergyGaugeKnown = false;
        float ringEnergyGauge = 1.0f;
        std::string ringEnergyGaugeSource = "pending-runtime-field";
        bool lifeCountKnown = false;
        uint32_t lifeCount = 3;
        std::string lifeCountSource = "pending-runtime-field";
        bool tutorialPromptKnown = false;
        std::string tutorialPromptId = "none";
        bool tutorialVisible = false;
        std::string tutorialPromptSource = "pending-runtime-field";
        std::string sonicRingPickupSfxId = "audio-id-pending";
        std::string tutorialPromptOpenSfxId = "audio-id-pending";
        std::string pauseOpenSfxId = "sys_actstg_pausewinopen";
        std::string pauseCursorSfxId = "sys_actstg_pausecursor";
        uint64_t frame = 0;
    };

    struct SonicHudValueWriteObservation
    {
        std::string writeKind = "text";
        std::string valueName;
        std::string path;
        bool pathResolved = true;
        std::string pathResolutionSource = "csd-project-tree";
        uint32_t nodeAddress = 0;
        uint32_t textAddress = 0;
        std::string textUtf8;
        bool numericValueKnown = false;
        double numericValue = 0.0;
        std::string hookSource;
        bool callsiteCorrelationKnown = false;
        std::string callsiteValueCandidate;
        std::string callsiteCorrelationSource;
        std::string callsiteCorrelationStatus;
        int32_t callsiteCorrelationFrameDelta = 0;
        std::string semanticPathCandidate;
        std::string semanticValueName;
        uint64_t frame = 0;
    };

    struct SonicHudNodeWriteCallsiteCorrelation
    {
        bool known = false;
        std::string valueCandidate;
        std::string source;
        std::string status;
        int32_t frameDelta = 0;
    };

    struct SonicHudSemanticPathCandidate
    {
        std::string path;
        std::string valueName;
    };

    struct HudRenderGateCorrelationSnapshot
    {
        bool renderHud = false;
        bool renderGameMainHud = false;
        bool renderHudPause = false;
        uint64_t resolvedUiPlayScreenNodeWrites = 0;
        uint64_t unresolvedUiPlayScreenNodeWrites = 0;
        std::string gateStatus = "unknown";
        std::string unresolvedWriteKinds;
        uint64_t frame = 0;
        std::vector<std::string> ms_IsRenderHudCallers;
        std::vector<std::string> ms_IsRenderGameMainHudCallers;
        std::vector<std::string> ms_IsRenderHudPauseCallers;
    };

    struct LateResolvedSonicHudNodeWrite
    {
        std::string writeKind;
        std::string valueName;
        std::string path;
        uint32_t nodeAddress = 0;
        uint32_t textAddress = 0;
        std::string valueText;
        bool numericValueKnown = false;
        double numericValue = 0.0;
        std::string hookSource;
        std::string pathResolutionSource;
    };

    struct SonicHudUpdateContextFrame
    {
        uint32_t ownerAddress = 0;
        std::string hookSource;
        uint64_t frame = 0;
    };

    struct SonicHudUpdateCallsiteSample
    {
        uint32_t ownerAddress = 0;
        std::string hookName;
        std::string samplePhase;
        double deltaTime = 0.0;
        uint32_t r4 = 0;
        uint32_t ownerField424 = 0;
        uint32_t ownerField432 = 0;
        float ownerField440 = 0.0f;
        float ownerField444 = 0.0f;
        uint32_t ownerField452 = 0;
        uint32_t ownerField456 = 0;
        uint32_t ownerField460 = 0;
        uint32_t ownerField464 = 0;
        uint32_t ownerField468 = 0;
        uint32_t ownerField472 = 0;
        float ownerField476 = 0.0f;
        uint32_t ownerField480 = 0;
        uint32_t ownerField484 = 0;
        uint32_t ownerField488 = 0;
        uint64_t frame = 0;
    };

    struct SonicHudLastClassifiedCallsiteValue
    {
        bool lastClassificationKnown = false;
        std::string valueName;
        std::string status;
        std::string lastClassifiedCallsiteValueSource;
        std::string hookName;
        std::string samplePhase;
        uint32_t ownerAddress = 0;
        bool normalizedValueKnown = false;
        uint32_t normalizedValue = 0;
        uint64_t lastClassifiedCallsiteValueFrame = 0;
    };

    struct CsdChildNodeLookupObservation
    {
        uint32_t resultOwnerAddress = 0;
        uint32_t parentNodeAddress = 0;
        std::string childName;
        std::string parentPath;
        std::string path;
        std::string hookSource;
        SonicHudUpdateContextFrame updateContext;
        uint64_t frame = 0;
    };

    struct CsdNodeSourceOwnerObservation
    {
        uint32_t sourceOwnerAddress = 0;
        uint32_t nodeAddress = 0;
        uint32_t parentNodeAddress = 0;
        std::string childName;
        std::string path;
        std::string hookSource;
        SonicHudUpdateContextFrame updateContext;
        int32_t sourceOwnerOffsetFromUpdateOwner = INT32_MIN;
        uint64_t frame = 0;
    };

    struct SonicHudOwnerFieldSample
    {
        std::string field;
        uint32_t sampleOffset = 0;
        uint32_t slotValue = 0;
        uint32_t rcObjectAddress = 0;
        uint32_t resolvedMemoryAddress = 0;
        bool rcObjectKnown = false;
        bool resolvedMemoryKnown = false;
        uint64_t frame = 0;
        std::string hookSource;
    };

    struct SonicHudOwnerPathInspectorSnapshot
    {
        uint32_t chudSonicStageOwnerAddress = 0;
        uint32_t stageGameModeAddress = 0;
        uint32_t rcPlayScreenProjectAddress = 0;
        uint32_t rcSpeedGaugeSceneAddress = 0;
        uint32_t rcRingEnergyGaugeSceneAddress = 0;
        uint32_t rcGaugeFrameSceneAddress = 0;
        uint32_t rcRingCountSceneAddress = 0;
        uint32_t rcScoreCountNodeAddress = 0;
        uint32_t rcTimeCountNodeAddress = 0;
        uint32_t rcTimeCount2NodeAddress = 0;
        uint32_t rcTimeCount3NodeAddress = 0;
        uint32_t rcPlayerCountNodeAddress = 0;
        uint32_t rcTutorialInfoSceneAddress = 0;
        bool rawOwnerKnown = false;
        bool rawOwnerFieldsReady = false;
        uint64_t rawOwnerFrame = 0;
        uint64_t rawOwnerFieldSampleCount = 0;
        uint32_t rawOwnerResolvedMemoryCount = 0;
        bool resolvedFromCsdProjectTree = false;
        std::string ownerPointerStatus;
        std::string ownerFieldMaturationStatus;
        std::string expectedOwnerFieldSource;
        std::string rawHookSource;
        std::string displayOwnerPaths;
        std::string gameplayNumericBindingStatus;
        std::vector<SonicHudOwnerFieldSample> rawOwnerFieldSamples;
    };

    struct PauseGeneralSaveLiveInspectorSnapshot
    {
        bool pauseKnown = false;
        uint32_t pauseAddress = 0;
        uint32_t pauseProjectAddress = 0;
        uint32_t pauseBgSceneAddress = 0;
        uint32_t pauseAction = UINT32_MAX;
        uint32_t pauseMenu = UINT32_MAX;
        uint32_t pauseStatus = UINT32_MAX;
        uint32_t pauseTransition = UINT32_MAX;
        bool pauseVisible = false;
        bool pauseShown = false;
        uint64_t pauseFrame = 0;
        bool generalWindowKnown = false;
        uint32_t generalWindowAddress = 0;
        uint32_t generalProjectAddress = 0;
        uint32_t generalBgSceneAddress = 0;
        uint32_t generalWindowStatus = UINT32_MAX;
        uint32_t generalCursorIndex = UINT32_MAX;
        uint32_t generalSelectedIndex = UINT32_MAX;
        uint64_t generalFrame = 0;
        bool saveIconKnown = false;
        uint32_t saveIconAddress = 0;
        bool saveIconVisible = false;
        uint64_t saveIconFrame = 0;
    };

    struct OperatorWindowEntry
    {
        const char* name;
        const char* description;
        bool* visible;
    };

    struct GuestBoolRef
    {
        const char* name;
        uint32_t guestAddress;
        bool readOnly;
    };

    struct DebugMenuForkField
    {
        const char* group;
        const char* sourcePath;
        const char* field;
        const char* usage;
        const char* status;
    };

    struct ChudSonicStageExpectedOwnerField
    {
        const char* field;
        uint32_t rcPtrOffset;
        uint32_t rcObjectOffset;
    };

    static constexpr std::array<GuestBoolRef, 7> kGuestRenderGlobals =
    {{
        { "ms_IsRenderHud", 0x8328BB26, false },
        { "ms_IsRenderGameMainHud", 0x8328BB27, false },
        { "ms_IsRenderHudPause", 0x8328BB28, false },
        { "ms_IsLoading", 0x83367A4C, true },
        { "ms_VisualizeLoadedLevel", 0x833678C1, false },
        { "ms_LightFieldDebug", 0x83367BCD, false },
        { "ms_IgnoreLightFieldData", 0x83367BCF, false },
    }};

    static constexpr std::array<GuestBoolRef, 7> kGuestDebugDrawGlobals =
    {{
        { "ms_IsRenderDebugDraw", 0x8328BB23, false },
        { "ms_IsRenderDebugPositionDraw", 0x8328BB24, false },
        { "ms_IsRenderDebugDrawText", 0x8328BB25, false },
        { "ms_IsCollisionRender", 0x833678A6, false },
        { "ms_IsObjectCollisionRender", 0x83367905, false },
        { "ms_IsTriggerRender", 0x83367904, false },
        { "ms_DrawLightFieldSamplingPoint", 0x83367BCE, false },
    }};

    static constexpr std::array<DebugMenuForkField, 15> kDebugMenuForkTypedFields =
    {{
        {
            "CSD manager",
            "api/CSD/Manager/csdmScene.h",
            "CSD.Manager.CScene.m_MotionFrame",
            "scene motion-frame readiness and animation progress",
            "harvested"
        },
        {
            "CSD manager",
            "api/CSD/Manager/csdmScene.h",
            "CSD.Manager.CScene.m_MotionRepeatType",
            "scene repeat-mode inspection for intro/usual/loop latches",
            "harvested"
        },
        {
            "CSD manager",
            "api/CSD/Manager/csdmNode.h",
            "CSD.Manager.CNode.m_pMotionPattern",
            "node/layer motion ownership inspection",
            "harvested"
        },
        {
            "SWA CSD",
            "api/SWA/CSD/CsdProject.h",
            "SWA.CSD.CCsdProject.m_rcProject",
            "database CSD project to runtime CProject ownership",
            "harvested"
        },
        {
            "SWA CSD",
            "api/SWA/CSD/GameObjectCSD.h",
            "SWA.CSD.CGameObjectCSD.m_rcProject",
            "game-object CSD project owner inspection",
            "harvested"
        },
        {
            "SWA HUD",
            "api/SWA/HUD/Sonic/HudSonicStage.h",
            "SWA.HUD.CHudSonicStage.m_rcPlayScreen",
            "normal Sonic HUD project owner",
            "harvested"
        },
        {
            "SWA HUD",
            "api/SWA/HUD/Sonic/HudSonicStage.h",
            "SWA.HUD.CHudSonicStage.m_rcSpeedGauge",
            "normal Sonic speed-gauge scene owner",
            "harvested"
        },
        {
            "SWA HUD",
            "api/SWA/HUD/Loading/Loading.h",
            "SWA.HUD.CLoading.m_LoadingDisplayType",
            "loading display state enum",
            "live"
        },
        {
            "SWA HUD",
            "api/SWA/HUD/Pause/HudPause.h",
            "SWA.HUD.CHudPause.m_Action",
            "pause action/status owner",
            "live"
        },
        {
            "SWA HUD",
            "api/SWA/HUD/GeneralWindow/GeneralWindow.h",
            "SWA.HUD.CGeneralWindow.m_rcGeneral",
            "shared general-window CSD project owner",
            "live"
        },
        {
            "SWA HUD",
            "api/SWA/HUD/SaveIcon/SaveIcon.h",
            "SWA.HUD.CSaveIcon.m_IsVisible",
            "autosave/save-icon visibility",
            "live"
        },
        {
            "SWA System/GameMode",
            "api/SWA/System/GameMode/GameModeStage.h",
            "SWA.System.GameMode.CGameModeStage",
            "stage game-mode ownership boundary",
            "live"
        },
        {
            "SWA System/GameMode",
            "api/SWA/System/GameMode/Title/TitleMenu.h",
            "SWA.System.GameMode.Title.CTitleMenu.m_CursorIndex",
            "title menu cursor readiness",
            "live"
        },
        {
            "Reddog",
            "ui/reddog/reddog_manager.h",
            "Reddog.Manager",
            "operator shell/window-list manager reference pattern",
            "ported-pattern"
        },
        {
            "Reddog",
            "ui/reddog/debug_draw.h",
            "Reddog.DebugDraw",
            "foreground operator debug-draw reference pattern",
            "ported-pattern"
        },
    }};

    static constexpr std::array<ChudSonicStageExpectedOwnerField, 9> kChudSonicStageExpectedOwnerFields =
    {{
        { "m_rcPlayScreen", 0xE0, 0xE4 },
        { "m_rcSpeedGauge", 0xE8, 0xEC },
        { "m_rcRingEnergyGauge", 0xF0, 0xF4 },
        { "m_rcGaugeFrame", 0xF8, 0xFC },
        { "m_rcScoreCount", 0x128, 0x12C },
        { "m_rcTimeCount", 0x130, 0x134 },
        { "m_rcTimeCount2", 0x138, 0x13C },
        { "m_rcTimeCount3", 0x140, 0x144 },
        { "m_rcPlayerCount", 0x148, 0x14C },
    }};

    static constexpr std::string_view kChudSonicStageExpectedOwnerFieldSource =
        "api/SWA/HUD/Sonic/HudSonicStage.h offsets 0xE0..0x14C";

    static constexpr std::array<RuntimeTarget, 11> kRuntimeTargets =
    {{
        { ScreenId::TitleLoop, "title-loop", "Title Loop", "ui_title", "System/GameMode/Title/TitleStateIntro.cpp", false },
        { ScreenId::TitleMenu, "title-menu", "Title Menu", "ui_title", "System/GameMode/Title/TitleMenu.cpp", false },
        { ScreenId::TitleOptions, "title-options", "Title Options", "ui_title", "UnleashedRecomp/ui/options_menu.cpp", false },
        { ScreenId::Loading, "loading", "Loading / Miles Electric", "ui_loading", "System/Loading.cpp", false },
        { ScreenId::SonicHud, "sonic-hud", "Sonic Stage HUD", "ui_playscreen", "Player/Character/Sonic/Hud/SonicMainDisplay.cpp", true },
        { ScreenId::Pause, "pause", "Pause Menu", "ui_pause", "HUD/Pause/HudPause.cpp", true },
        { ScreenId::ExtraStageHud, "extra-stage-hud", "Extra Stage / Tornado HUD", "ui_prov_playscreen", "ExtraStage/Tails/Hud/HudExQte.cpp", true },
        { ScreenId::Result, "result", "Stage Result", "ui_result", "HUD/Result/Result.cpp", true },
        { ScreenId::Status, "status", "Status / Skill Upgrade", "ui_status", "HUD/Status/Status.cpp", false },
        { ScreenId::Tutorial, "tutorial", "Tutorial / Control Guide", "ui_playscreen", "Player/Character/Sonic/Hud/SonicHudGuide.cpp", true },
        { ScreenId::WorldMap, "world-map", "World Map", "ui_worldmap", "System/GameMode/WorldMap/WorldMapSelect.cpp", false },
    }};

    struct StarterUiCoverageRow
    {
        std::string_view screenId;
        std::string_view controller;
        std::string_view runtimeOracle;
        std::string_view prototypeRouteStatus;
        std::string_view nextEvidenceBeat;
    };

    struct SourceRecoveryLaneRow
    {
        std::string_view laneId;
        std::string_view screenId;
        std::string_view controller;
        std::string_view primaryOracle;
        std::string_view decisionStatus;
        std::string_view nextEvidenceBeat;
        bool selected;
    };

    // Phase 204: repo-safe starter UI/UX coverage matrix derived from the prototype route taxonomy.
    // The local-only Reddog style reference guides the F2 surface; no prototype DDS payloads are loaded here.
    static constexpr std::array<StarterUiCoverageRow, 9> kStarterUiCoverageRows =
    {{
        { "title-menu", "TitleMenuController", "retail title/menu JSONL + UI-layer capture", "Title + OldMainMenu route taxonomy", "compare prototype old-main-menu affordances to retail title/options policy" },
        { "loading", "LoadingScreenController", "retail loading display/CSD/native capture", "package/runtime lane, not Select.xml route", "wire text/glyph/fade/SFX timing against retail loading evidence" },
        { "options-settings", "OptionsMenuController", "retail title-options route evidence", "OldMainMenu secondary route hint", "split options cursor/input/SFX policy into reusable source" },
        { "pause", "PauseMenuController", "retail CHudPause owner + ui_pause evidence", "runtime lane, not Select.xml route", "keep pause owner/action/render-gate capture as oracle" },
        { "sonic-day-hud", "SonicDayHudController", "retail CHudSonicStage/ui_playscreen live bridge", "Sonic day LoadXML route taxonomy", "finish boost/ring-energy formula and exact SFX IDs" },
        { "werehog-hud", "WerehogHudController", "pending retail night/Evil Sonic HUD capture", "Evil Sonic LoadXML route taxonomy", "apply SonicDayHudController pattern after day HUD stabilizes" },
        { "world-map", "WorldMapController", "pending retail world-map runtime capture", "WorldMap route taxonomy", "map icons/tutorial route/disc indicators to reusable source" },
        { "results", "ResultScreenController", "pending retail result/status capture", "Ending/result package secondary hint", "separate result source from ending/staff-roll boundaries" },
        { "audio-sfx", "AudioCueCatalog", "retail audio callsite/JSONL proof pending", "Sound Test route taxonomy", "recover exact cue IDs/banks for menus/HUD/loading/pause" },
    }};

    // Phase 205: coverage-matrix-selected source-recovery queue. Sonic Day HUD stays first because
    // boost/ring-energy and exact SFX/audio IDs are the blocking 1:1 source gaps for the starter set.
    static constexpr std::array<SourceRecoveryLaneRow, 4> kSourceRecoveryLaneRows =
    {{
        { "sonic-day-hud-retail-runtime", "sonic-day-hud", "SonicDayHudController", "retail-runtime-ui-lab", "coverage-matrix-selected-retail-sonic-day-hud", "prove boost/ring-energy formulas, SetPatternIndex/SetHideFlag joins, and exact SFX/audio IDs", true },
        { "world-map-prototype-route", "world-map", "WorldMapController", "prototype-route-taxonomy-plus-future-retail-runtime-capture", "queued-after-sonic-day-hud-value-proof", "harvest WorldMap route/icon/tutorial/disc-indicator facts after HUD value proof", false },
        { "results-prototype-route", "results", "ResultScreenController", "prototype-route-taxonomy-plus-future-retail-runtime-capture", "queued-after-sonic-day-hud-value-proof", "split result-screen source from Ending/staff-roll route boundaries", false },
        { "audio-sfx-retail-runtime", "audio-sfx", "AudioCueCatalog", "retail-audio-callsite-jsonl", "parallel-hud-followup", "recover exact cue IDs and banks for HUD/menu/loading/pause actions", false },
    }};

    static bool g_isEnabled = false;
    static bool g_observerMode = false;
    static bool g_routeTargetExplicit = false;
    static bool g_hideOverlay = false;
    static bool g_operatorShellVisible = false;
    static bool g_operatorShellToggleWasDown = false;
    // Compact-on-demand operator windows: direct live bridge/API stays enabled, panes open only when requested.
    static bool g_operatorWindowListVisible = false;
    static bool g_operatorInspectorVisible = false;
    static bool g_operatorCounterVisible = false;
    static bool g_operatorViewVisible = false;
    static bool g_operatorExportsVisible = false;
    static bool g_operatorDebugDrawVisible = false;
    static bool g_operatorWelcomeVisible = false;
    static bool g_operatorStageHudVisible = false;
    static bool g_operatorLiveApiVisible = false;
    static bool g_operatorDebugDrawLayerVisible = false;
    static ImVec2 g_operatorDebugIconPos = { 18.0f, 18.0f };
    static constexpr int kOperatorProfilerFrameHistoryCount = 256;
    static float g_operatorProfilerFrameMs[kOperatorProfilerFrameHistoryCount] = {};
    static int g_operatorProfilerFrameMsIndex = 0;
    static uint64_t g_operatorProfilerLastFrame = 0;
    static ImFont* g_swardNativeProfilerFont = nullptr;
    static float g_swardNativeProfilerFontDefaultScale = 1.0f;
    static bool g_swardNativeProfilerFontPushed = false;
    static RoutePolicy g_routePolicy = RoutePolicy::InputInjection;
    static ScreenId g_target = ScreenId::TitleLoop;
    static bool g_liveBridgeEnabled = true;
    static std::string g_liveBridgeName = "sward_ui_lab_live";
    static std::atomic<bool> g_liveBridgeStarted = false;
    static std::atomic<bool> g_liveBridgeStopRequested = false;
    static std::mutex g_liveBridgeMutex;
    static std::mutex g_typedInspectorMutex;
    static std::deque<std::string> g_recentEvidenceEvents;
    static std::string g_lastLiveBridgeCommand;
    static uint64_t g_liveBridgeCommandCount = 0;
    static bool g_routePending = false;
    static uint64_t g_routeGeneration = 0;
    static uint64_t g_routeResetCount = 0;
    static bool g_titleIntroAcceptInjected = false;
    static bool g_titleIntroDirectStateApplied = false;
    static uint64_t g_titleIntroDirectStateLastRequestFrame = 0;
    static uint64_t g_stageTitleOwnerDirectStateLastRequestFrame = 0;
    static bool g_stageTitleOwnerDirectStateFallbackEnabled = false;
    static bool g_titleMenuAcceptInjected = false;
    static bool g_titleMenuDirectContextAcceptInjected = false;
    static bool g_stageContextObserved = false;
    static bool g_targetCsdObserved = false;
    static std::string_view g_routeStatus = "idle";
    static std::string g_requestedStageHarness = "auto";
    static std::filesystem::path g_evidenceDirectory;
    static std::filesystem::path g_nativeFrameCaptureDirectory;
    static double g_autoExitSeconds = 0.0;
    static uint64_t g_presentedFrameCount = 0;
    static bool g_autoExitRequested = false;
    static bool g_nativeFrameCaptureEnabled = false;
    static bool g_nativeFrameCaptureReserved = false;
    static uint32_t g_nativeFrameCaptureMaxCount = 1;
    static uint32_t g_nativeFrameCaptureWrittenCount = 0;
    static uint32_t g_nativeFrameCaptureIntervalFrames = 120;
    static uint32_t g_nativeFrameCaptureIndex = 0;
    static uint64_t g_lastNativeFrameCaptureFrame = 0;
    static std::string g_lastNativeFrameCapturePath;
    static std::string g_lastNativeFrameCaptureFailure;
    static bool g_nativeFrameCaptureCompleteExitPending = false;
    static bool g_uiOnlyRenderTargetCaptureRequested = false;
    static bool g_uiOnlyRenderTargetCaptureReserved = false;
    static uint32_t g_uiOnlyRenderTargetCaptureIndex = 0;
    static uint64_t g_lastUiOnlyRenderTargetCaptureFrame = 0;
    static std::string g_lastUiOnlyRenderTargetCapturePath;
    static std::string g_lastUiOnlyRenderTargetCaptureFailure;
    static std::string g_lastUiOnlyRenderTargetCaptureSource;
    static uint32_t g_lastUiOnlyRenderTargetCaptureWidth = 0;
    static uint32_t g_lastUiOnlyRenderTargetCaptureHeight = 0;
    static bool g_lastUiOnlyRenderTargetCaptureContainsFullFramebuffer = true;
    static const std::chrono::steady_clock::time_point g_startedAt = std::chrono::steady_clock::now();
    static bool g_loggedIntroHook = false;
    static bool g_loggedMenuHook = false;
    static bool g_loggedStageHarness = false;
    static bool g_loggedTargetCsdProjectLive = false;
    static bool g_loggedStageTargetCsdBound = false;
    static bool g_loggedStageTargetReady = false;
    static bool g_titleMenuTransitionPulseObserved = false;
    static bool g_titleMenuVisualReady = false;
    static bool g_titleMenuPressStartAccepted = false;
    static bool g_titleMenuPostPressStartHeld = false;
    static bool g_titleMenuAcceptSuppressionLogged = false;
    static bool g_titleMenuPostPressStartReadyLogged = false;
    static uint64_t g_titleMenuStableFrameStart = 0;
    static uint64_t g_titleMenuOwnerStableFrameStart = 0;
    static TitleIntroInspectorSnapshot g_titleIntroInspector;
    static TitleOwnerInspectorSnapshot g_titleOwnerInspector;
    static TitleMenuInspectorSnapshot g_titleMenuInspector;
    static uint64_t g_titleIntroContextSampleCount = 0;
    static uint64_t g_stageTitleContextSampleCount = 0;
    static uint32_t g_lastLoadingRequestType = UINT32_MAX;
    static uint64_t g_lastLoadingRequestFrame = 0;
    static uint32_t g_lastLoadingDisplayType = UINT32_MAX;
    static uint64_t g_lastLoadingDisplayFrame = 0;
    static bool g_loadingDisplayWasActive = false;
    static uint32_t g_lastStageGameModeAddress = 0;
    static uint64_t g_lastStageContextFrame = 0;
    static uint64_t g_lastStageReadyFrame = 0;
    static bool g_pauseRouteStartInjected = false;
    static uint64_t g_pauseRouteInputHoldStartFrame = 0;
    static uint64_t g_pauseRouteInputLastFrame = 0;
    static bool g_loggedPauseOwnerObserved = false;
    static std::unordered_set<std::string> g_loggedPauseRouteInputSources;
    static uint32_t g_chudSonicStageOwnerAddress = 0;
    static uint32_t g_chudSonicStagePlayScreenProjectAddress = 0;
    static uint32_t g_chudSonicStageSpeedGaugeSceneAddress = 0;
    static uint32_t g_chudSonicStageRingEnergyGaugeSceneAddress = 0;
    static uint32_t g_chudSonicStageGaugeFrameSceneAddress = 0;
    static uint32_t g_chudSonicStageScoreCountNodeAddress = 0;
    static uint32_t g_chudSonicStageTimeCountNodeAddress = 0;
    static uint32_t g_chudSonicStageTimeCount2NodeAddress = 0;
    static uint32_t g_chudSonicStageTimeCount3NodeAddress = 0;
    static uint32_t g_chudSonicStagePlayerCountNodeAddress = 0;
    static uint64_t g_chudSonicStageRawHookFrame = 0;
    static std::string g_chudSonicStageRawHookSource;
    static bool g_loggedChudSonicStageOwnerHook = false;
    static bool g_loggedChudSonicStageOwnerFieldSample = false;
    static bool g_loggedChudSonicStageOwnerFieldsReady = false;
    static std::string g_chudSonicStageOwnerHookStableSignature;
    static uint64_t g_chudSonicStageOwnerHookLastEvidenceFrame = 0;
    static std::string g_chudSonicStageOwnerFieldSampleStableSignature;
    static uint64_t g_chudSonicStageOwnerFieldSampleLastEvidenceFrame = 0;
    // Phase 197: parallel throttle/dedup state for the sub_824D6C18 owner-field
    // gauge-state snapshot. Kept separate from the constructor-time owner-field
    // sample lane so a stable owner pointer plus a churning rolling counter can
    // both emit, but neither floods the JSONL.
    static std::string g_chudSonicStageOwnerFieldGaugeSnapshotStableSignature;
    static uint64_t g_chudSonicStageOwnerFieldGaugeSnapshotLastEvidenceFrame = 0;
    // Phase 198: cache the latest owner-field snapshot so the SetScale hook can
    // read it without re-walking the guest memory. Updated unconditionally on
    // every Phase 197 snapshot call so the SetScale join sees a fresh value
    // even when the Phase 197 emission throttle has gated the corresponding
    // sonic-hud-owner-gauge-snapshot event.
    struct OwnerFieldGaugeSnapshotCache
    {
        uint32_t ownerAddress = 0;
        uint32_t field460 = 0;
        uint32_t field464 = 0;
        uint32_t field468 = 0;
        uint32_t field472 = 0;
        uint32_t field480 = 0;
        uint64_t frame = 0;
        bool populated = false;
    };
    static OwnerFieldGaugeSnapshotCache g_lastOwnerFieldGaugeSnapshot;
    // Phase 198: dedup gate for the SetScale x owner-field join event. Keyed
    // on (path, scale, ownerSignature) so steady-state HUD frames do not
    // reproduce identical joins; one join per material change is the goal.
    static std::string g_lastOwnerFieldGaugeScaleCorrelationSignature;
    static uint64_t g_lastOwnerFieldGaugeScaleCorrelationEvidenceFrame = 0;
    // Phase 198: same-frame join window. Only emit when the cached snapshot
    // is at most kOwnerFieldGaugeScaleCorrelationFrameWindow frames old, so
    // SetScale calls long after the last sub_824D6C18 hit are not joined.
    static constexpr uint64_t kOwnerFieldGaugeScaleCorrelationFrameWindow = 60;
    static bool g_loggedTutorialHudOwnerPathReady = false;
    static uint64_t g_chudSonicStageOwnerFieldSampleCount = 0;
    static std::vector<SonicHudOwnerFieldSample> g_chudSonicStageOwnerFieldSamples;
    static SonicHudGameplayValueSnapshot g_sonicHudGameplayValues;
    static constexpr size_t kSonicHudValueWriteObservationLimit = 96;
    static std::vector<SonicHudValueWriteObservation> g_sonicHudValueWriteObservations;
    static std::unordered_set<std::string> g_loggedSonicHudValueTextWriteKeys;
    static constexpr size_t kSonicHudUpdateCallsiteSampleLimit = 96;
    static std::vector<SonicHudUpdateCallsiteSample> g_sonicHudUpdateCallsiteSamples;
    static SonicHudLastClassifiedCallsiteValue g_lastSonicHudClassifiedCallsiteValue;
    static std::unordered_map<std::string, std::string> g_lastSonicHudUpdateCallsiteSampleDetails;
    static constexpr uint64_t kSonicHudUpdateCallsiteMinEvidenceIntervalFrames = 600;
    static constexpr uint64_t kSonicHudNodeCallsiteCorrelationFrameWindow = 120;
    static constexpr std::string_view kSonicHudNodeWriteCandidateSetLabel =
        "timer/speed/boost-ring-energy/tutorial";
    static uint64_t g_lastSonicHudUpdateCallsiteSampleFrame = 0;
    static std::unordered_map<std::string, std::string> g_lastSonicHudUpdateCallsiteStableSignatures;
    static std::unordered_map<std::string, uint64_t> g_lastSonicHudUpdateCallsiteEvidenceFrames;
    static std::unordered_map<std::string, uint32_t> g_lastSonicHudSpeedReadoutValues;
    static std::unordered_map<std::string, uint64_t> g_lastSonicHudSpeedReadoutEvidenceFrames;
    static bool g_hudRenderGateCorrelationKnown = false;
    static bool g_lastHudRenderGateRenderHud = false;
    static bool g_lastHudRenderGateGameMainHud = false;
    static bool g_lastHudRenderGatePauseHud = false;
    static uint64_t g_lastHudRenderGateUnresolvedWrites = 0;
    static uint64_t g_lastHudRenderGateCorrelationEvidenceFrame = 0;
    static constexpr size_t kCsdChildNodeLookupObservationLimit = 256;
    static std::unordered_map<uint32_t, CsdChildNodeLookupObservation> g_csdChildNodeLookupObservations;
    static std::unordered_map<uint32_t, CsdNodeSourceOwnerObservation> g_csdNodeSourceOwnerObservations;
    static std::unordered_set<std::string> g_loggedSonicHudNodeSourceOwnerKeys;
    static thread_local std::vector<SonicHudUpdateContextFrame> g_sonicHudUpdateContextStack;
    static uint64_t g_lastLiveStateSnapshotFrame = 0;
    static std::string g_lastStageReadyEventName;
    static std::string g_lastCsdProjectName;
    static uint64_t g_lastCsdProjectFrame = 0;
    static std::string g_lastTitleIntroContextDetail;
    static std::string g_lastStageTitleContextDetail;
    static std::string g_lastTitleMenuContextDetail;
    static std::string g_lastStageContextDetail;
    static std::unordered_set<std::string> g_loggedCsdProjects;
    static std::vector<std::string> g_observedCsdProjectOrder;
    static std::vector<CsdProjectTreeRecord> g_csdProjectTrees;
    static constexpr size_t kRuntimeUiDrawCallSampleLimit = 96;
    static std::vector<RuntimeUiDrawCall> g_runtimeUiDrawCalls;
    static uint64_t g_runtimeUiDrawListFrame = UINT64_MAX;
    static uint32_t g_runtimeUiDrawCallSequence = 0;
    static uint32_t g_runtimeUiDrawCallDroppedCount = 0;
    static constexpr size_t kRuntimeGpuSubmitCallSampleLimit = 160;
    static std::vector<RuntimeGpuSubmitCall> g_runtimeGpuSubmitCalls;
    static uint64_t g_runtimeGpuSubmitFrame = UINT64_MAX;
    static uint32_t g_runtimeGpuSubmitSequence = 0;
    static uint32_t g_runtimeGpuSubmitDroppedCount = 0;
    static constexpr size_t kRuntimeRawBackendCommandSampleLimit = 160;
    static std::vector<RuntimeRawBackendCommand> g_runtimeRawBackendCommands;
    static uint64_t g_runtimeRawBackendCommandFrame = UINT64_MAX;
    static uint32_t g_runtimeRawBackendCommandSequence = 0;
    static uint32_t g_runtimeRawBackendCommandDroppedCount = 0;
    static constexpr size_t kRuntimeBackendResolvedSubmitSampleLimit = 192;
    static std::vector<RuntimeBackendResolvedSubmit> g_runtimeBackendResolvedSubmits;
    static uint64_t g_runtimeBackendResolvedFrame = UINT64_MAX;
    static uint32_t g_runtimeBackendResolvedSequence = 0;
    static uint32_t g_runtimeBackendResolvedDroppedCount = 0;
    static std::unordered_map<uint32_t, RuntimeTextureDescriptorSemantic> g_runtimeTextureDescriptorSemantics;
    static std::unordered_map<uint32_t, RuntimeSamplerDescriptorSemantic> g_runtimeSamplerDescriptorSemantics;
    static std::unordered_map<uint32_t, RuntimeVendorTextureResourceView> g_runtimeVendorTextureResourceViews;
    static std::unordered_map<uint32_t, RuntimeVendorSamplerResourceView> g_runtimeVendorSamplerResourceViews;
    static PauseGeneralSaveLiveInspectorSnapshot g_pauseGeneralSaveInspector;

    static const RuntimeTarget& TargetFor(ScreenId id);
    static bool TargetNeedsStageHarness(ScreenId id);
    static bool TargetShouldRouteThroughLoading(ScreenId id);
    static bool TargetRoutesThroughTitleMenu(ScreenId id);
    static void RefreshTargetCsdProjectStatus();
    static void ResetRouteLatchState();
    static bool IsPauseTargetRuntimeReady();
    static bool IsTutorialTargetRuntimeReady();
    static bool IsStageTargetRuntimeReady();
    static bool IsNativeFrameCaptureReady();
    static std::string_view NativeFrameCaptureStatusLabel();
    static std::string_view UiOnlyRenderTargetCaptureStatusLabel();
    static std::string_view UiOnlyLayerIsolationStatusLabel();
    static std::string BuildRuntimeUiOnlyRenderTargetCaptureJson();
    static void EmitTutorialHudOwnerPathReadyIfNeeded();
    static void EmitStageTargetReadyIfNeeded();
    static std::array<OperatorWindowEntry, 8> GetOperatorWindowEntries();
    static void StartLiveBridge();
    static void UiLabLiveBridgeThread();
    static std::string HandleLiveBridgeCommand(std::string_view command);
    static bool ReadGuestBool(uint32_t guestAddress);
    static void WriteGuestBool(uint32_t guestAddress, bool value);
    static uint64_t TitleMenuStableFrames();
    static bool TryReadGuestU32(uint32_t guestAddress, uint32_t& value);
    static bool TryReadGuestFloat(uint32_t guestAddress, float& value);
    static std::string_view MotionRepeatTypeLabel(uint32_t repeatType);
    static std::string_view LoadingDisplayTypeLabel(uint32_t displayType);
    static std::string_view PauseActionTypeLabel(uint32_t action);
    static std::string_view PauseMenuTypeLabel(uint32_t menu);
    static std::string_view PauseStatusTypeLabel(uint32_t status);
    static std::string_view PauseTransitionTypeLabel(uint32_t transition);
    static std::string_view GeneralWindowStatusLabel(uint32_t status);
    static CsdLiveInspectorSnapshot BuildCsdLiveInspectorSnapshot();
    static CsdProjectTreeInspectorSnapshot BuildCsdProjectTreeInspectorSnapshot();
    static LoadingLiveInspectorSnapshot BuildLoadingLiveInspectorSnapshot();
    static SonicHudLiveInspectorSnapshot BuildSonicHudLiveInspectorSnapshot();
    static SonicHudGameplayValueSnapshot BuildSonicHudGameplayValueSnapshot();
    static std::string SonicHudValueWriteBindingStatus();
    static std::vector<SonicHudValueWriteObservation> BuildSonicHudValueWriteObservations();
    static std::string BaseSonicHudWriteKind(std::string_view writeKind);
    static HudRenderGateCorrelationSnapshot BuildHudRenderGateCorrelationSnapshot();
    static void UpdateHudRenderGateCorrelation();
    static std::vector<SonicHudUpdateCallsiteSample> BuildSonicHudUpdateCallsiteSamples();
    static SonicHudLastClassifiedCallsiteValue BuildSonicHudLastClassifiedCallsiteValue();
    static bool ClassifySonicHudUpdateCallsiteSample(
        const SonicHudUpdateCallsiteSample& sample,
        std::string& valueName,
        std::string& status,
        std::string& source,
        uint32_t& normalizedValue,
        bool& normalizedValueKnown);
    static std::string SonicHudCallsiteCandidateFromValueName(std::string_view valueName);
    static std::string SonicHudHookNameFromUpdateContext(std::string_view hookSource);
    static SonicHudNodeWriteCallsiteCorrelation CorrelateUnresolvedSonicHudNodeWriteWithCallsite(
        std::string_view writeKind,
        uint32_t nodeAddress);
    static std::string ResolveSonicHudValuePathFromCsdNode(uint32_t nodeAddress);
    static bool ApplySonicHudTextWriteToGameplayValues(
        std::string_view path,
        std::string_view textUtf8,
        std::string_view hookSource);
    static std::string BuildSonicHudUpdateCallsiteStableSignature(
        const SonicHudUpdateCallsiteSample& sample);
    static bool ApplySonicHudUpdateCallsiteSampleToGameplayValues(
        const SonicHudUpdateCallsiteSample& sample,
        bool writeEvidence);
    static bool ApplySonicHudSpeedReadoutValueToGameplayValues(
        uint32_t ownerAddress,
        uint32_t speedKmh,
        std::string_view hookSource);
    static SonicHudOwnerPathInspectorSnapshot BuildSonicHudOwnerPathInspectorSnapshot(
        const CsdProjectTreeInspectorSnapshot& csdProjectTree);
    static PauseGeneralSaveLiveInspectorSnapshot BuildPauseGeneralSaveLiveInspectorSnapshot();
    static void AppendCsdTreeEntries(
        std::ostringstream& out,
        const std::vector<CsdTreeEntry>& entries,
        std::string_view addressFieldName,
        std::string_view relatedAddressFieldName,
        std::string_view firstMetricName,
        std::string_view secondMetricName);
    static std::string BuildRuntimeUiDrawListJson();
    static std::string BuildRuntimeGpuSubmitJson();
    static std::string BuildRuntimeMaterialCorrelationJson();
    static std::string BuildRuntimeBackendResolvedJson();
    static std::string BuildRuntimeVendorCommandResourceDumpJson();
    static std::vector<RuntimeMaterialCorrelation> BuildRuntimeMaterialCorrelationPairs(
        const std::vector<RuntimeUiDrawCall>& drawCalls,
        const std::vector<RuntimeGpuSubmitCall>& submitCalls);
    static void AppendSonicHudOwnerFieldSamples(std::ostringstream& out, const std::vector<SonicHudOwnerFieldSample>& samples);
    static void AppendSonicHudValueWriteObservations(
        std::ostringstream& out,
        const std::vector<SonicHudValueWriteObservation>& observations);
    static void AppendSonicHudUpdateCallsiteSamples(
        std::ostringstream& out,
        const std::vector<SonicHudUpdateCallsiteSample>& samples);
    static void AppendTypedInspectors(std::ostringstream& out);

    static std::string_view RoutePolicyLabel()
    {
        switch (g_routePolicy)
        {
            case RoutePolicy::DirectContext:
                return "direct-context";

            case RoutePolicy::InputInjection:
            default:
                return "input";
        }
    }

    static bool TrySetRoutePolicy(std::string_view value)
    {
        if (value == "input" || value == "input-injection")
        {
            g_routePolicy = RoutePolicy::InputInjection;
            return true;
        }

        if (value == "direct" || value == "direct-context" || value == "context")
        {
            g_routePolicy = RoutePolicy::DirectContext;
            return true;
        }

        return false;
    }

    static bool IsTruthy(std::string_view value)
    {
        return value == "1" || value == "true" || value == "on" || value == "yes" || value == "capture";
    }

    static bool IsFalsy(std::string_view value)
    {
        return value == "0" || value == "false" || value == "off" || value == "no" || value == "none";
    }

    static uint32_t ParsePositiveU32(std::string_view value, uint32_t fallback)
    {
        std::string text(value);
        char* end = nullptr;
        const auto parsed = std::strtoul(text.c_str(), &end, 10);

        if (end == text.c_str())
            return fallback;

        if (parsed == 0)
            return 1;

        if (parsed > UINT32_MAX)
            return UINT32_MAX;

        return static_cast<uint32_t>(parsed);
    }

    static bool ShouldDrawOverlay()
    {
        if (!g_hideOverlay)
            return true;

        return false;
    }

    static std::array<OperatorWindowEntry, 8> GetOperatorWindowEntries()
    {
        return
        {{
            { "Welcome", "Default-open operator shell summary and direct-read stance", &g_operatorWelcomeVisible },
            { "Inspector", "Runtime state, route latches, native capture, targets", &g_operatorInspectorVisible },
            { "Counter", "Frame, timing, hook, and capture counters", &g_operatorCounterVisible },
            { "View", "Runtime view and debug visibility toggles", &g_operatorViewVisible },
            { "Exports", "Patch/config toggles useful during UI archaeology", &g_operatorExportsVisible },
            { "Debug Draw", "Foreground operator annotations and debug-view switches", &g_operatorDebugDrawVisible },
            { "Stage / HUD", "Stage harness, HUD CSD binding, and target-ready latches", &g_operatorStageHudVisible },
            { "Live API", "Machine-readable live-state-json snapshot path and current values", &g_operatorLiveApiVisible },
        }};
    }

    bool ShouldReserveF1DebugToggle()
    {
        return false; // Leave F1 to DrawProfiler().
    }

    static std::string JsonEscape(std::string_view value)
    {
        std::string escaped;
        escaped.reserve(value.size());

        for (char c : value)
        {
            switch (c)
            {
                case '\\':
                    escaped += "\\\\";
                    break;

                case '"':
                    escaped += "\\\"";
                    break;

                case '\n':
                    escaped += "\\n";
                    break;

                case '\r':
                    escaped += "\\r";
                    break;

                case '\t':
                    escaped += "\\t";
                    break;

                default:
                    escaped += c;
                    break;
            }
        }

        return escaped;
    }

    static std::string Trim(std::string_view value)
    {
        size_t begin = 0;
        while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])))
            ++begin;

        size_t end = value.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])))
            --end;

        return std::string(value.substr(begin, end - begin));
    }

    static std::string ToLower(std::string_view value)
    {
        std::string lowered(value);
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        return lowered;
    }

    static double SecondsSinceStart()
    {
        const auto elapsed = std::chrono::steady_clock::now() - g_startedAt;
        return std::chrono::duration<double>(elapsed).count();
    }

    static std::string HexU32(uint32_t value)
    {
        std::ostringstream out;
        out << "0x" << std::hex << std::uppercase << value;
        return out.str();
    }

    static std::string HexU64(uint64_t value)
    {
        std::ostringstream out;
        out << "0x" << std::hex << std::uppercase << value;
        return out.str();
    }

    static bool IsPlausibleGuestPointer(uint32_t value)
    {
        return value >= 0x10000;
    }

    static bool TryReadGuestU32(uint32_t guestAddress, uint32_t& value)
    {
        if (g_memory.base == nullptr || guestAddress == 0)
            return false;

        const auto* bytes = reinterpret_cast<const uint8_t*>(g_memory.Translate(guestAddress));
        value =
            (static_cast<uint32_t>(bytes[0]) << 24) |
            (static_cast<uint32_t>(bytes[1]) << 16) |
            (static_cast<uint32_t>(bytes[2]) << 8) |
            static_cast<uint32_t>(bytes[3]);
        return true;
    }

    static bool TryReadGuestFloat(uint32_t guestAddress, float& value)
    {
        uint32_t bits = 0;
        if (!TryReadGuestU32(guestAddress, bits))
            return false;

        std::memcpy(&value, &bits, sizeof(value));
        return true;
    }

    static std::string_view MotionRepeatTypeLabel(uint32_t repeatType)
    {
        switch (repeatType)
        {
            case 0:
                return "eMotionRepeatType_PlayOnce";

            case 1:
                return "eMotionRepeatType_Loop";

            case 2:
                return "eMotionRepeatType_PingPong";

            case 3:
                return "eMotionRepeatType_PlayThenDestroy";

            default:
                return "unknown";
        }
    }

    static std::string_view LoadingDisplayTypeLabel(uint32_t displayType)
    {
        switch (displayType)
        {
            case 0:
                return "eLoadingDisplayType_MilesElectric";

            case 1:
                return "eLoadingDisplayType_None";

            case 2:
                return "eLoadingDisplayType_WerehogMovie";

            case 3:
                return "eLoadingDisplayType_MilesElectricContext";

            case 4:
                return "eLoadingDisplayType_Arrows";

            case 5:
                return "eLoadingDisplayType_NowLoading";

            case 6:
                return "eLoadingDisplayType_EventGallery";

            case 7:
                return "eLoadingDisplayType_ChangeTimeOfDay";

            case 8:
                return "eLoadingDisplayType_Blank";

            default:
                return "unknown";
        }
    }

    static std::string_view PauseActionTypeLabel(uint32_t action)
    {
        switch (action)
        {
            case 0:
                return "eActionType_Undefined";

            case 1:
                return "eActionType_Status";

            case 2:
                return "eActionType_Return";

            case 3:
                return "eActionType_Inventory";

            case 4:
                return "eActionType_Skills";

            case 5:
                return "eActionType_Lab";

            case 6:
                return "eActionType_Wait";

            case 8:
                return "eActionType_Restart";

            case 9:
                return "eActionType_Continue";

            default:
                return "unknown";
        }
    }

    static std::string_view PauseMenuTypeLabel(uint32_t menu)
    {
        switch (menu)
        {
            case 0:
                return "eMenuType_WorldMap";

            case 1:
                return "eMenuType_Village";

            case 2:
                return "eMenuType_Stage";

            case 3:
                return "eMenuType_Hub";

            case 4:
                return "eMenuType_Misc";

            default:
                return "unknown";
        }
    }

    static std::string_view PauseStatusTypeLabel(uint32_t status)
    {
        switch (status)
        {
            case 0:
                return "eStatusType_Idle";

            case 1:
                return "eStatusType_Accept";

            case 2:
                return "eStatusType_Decline";

            default:
                return "unknown";
        }
    }

    static std::string_view PauseTransitionTypeLabel(uint32_t transition)
    {
        switch (transition)
        {
            case 0:
                return "eTransitionType_Undefined";

            case 2:
                return "eTransitionType_Quit";

            case 5:
                return "eTransitionType_Dialog";

            case 6:
                return "eTransitionType_Hide";

            case 7:
                return "eTransitionType_Abort";

            case 8:
                return "eTransitionType_SubMenu";

            default:
                return "unknown";
        }
    }

    static std::string_view GeneralWindowStatusLabel(uint32_t status)
    {
        switch (status)
        {
            case 0:
                return "eWindowStatus_Closed";

            case 2:
                return "eWindowStatus_OpeningMessage";

            case 3:
                return "eWindowStatus_DisplayingMessage";

            case 4:
                return "eWindowStatus_OpeningControls";

            case 5:
                return "eWindowStatus_DisplayingControls";

            default:
                return "unknown";
        }
    }

    static void WriteEvidenceEvent(std::string_view event, std::string_view detail = {})
    {
        if (!g_isEnabled || g_evidenceDirectory.empty())
            return;

        std::error_code ec;
        std::filesystem::create_directories(g_evidenceDirectory, ec);

        if (ec)
        {
            LOGFN_WARNING("SWARD UI Lab: failed to create evidence directory '{}': {}",
                g_evidenceDirectory.string(),
                ec.message());
            return;
        }

        const auto eventPath = g_evidenceDirectory / "ui_lab_events.jsonl";
        std::ofstream out(eventPath, std::ios::app);

        if (!out)
        {
            LOGFN_WARNING("SWARD UI Lab: failed to open evidence log '{}'.", eventPath.string());
            return;
        }

        const auto& target = TargetFor(g_target);
        std::ostringstream eventJson;
        eventJson
            << "{\"time\":" << SecondsSinceStart()
            << ",\"frame\":" << g_presentedFrameCount
            << ",\"event\":\"" << JsonEscape(event) << "\""
            << ",\"target\":\"" << JsonEscape(target.token) << "\""
            << ",\"label\":\"" << JsonEscape(target.label) << "\""
            << ",\"csd\":\"" << JsonEscape(target.primaryCsdScene) << "\""
            << ",\"route\":\"" << JsonEscape(g_routeStatus) << "\""
            << ",\"stage\":\"" << JsonEscape(GetStageHarnessLabel()) << "\"";

        if (!detail.empty())
            eventJson << ",\"detail\":\"" << JsonEscape(detail) << "\"";

        eventJson << "}";
        const auto eventLine = eventJson.str();
        out << eventLine << "\n";

        {
            std::lock_guard<std::mutex> lock(g_liveBridgeMutex);
            g_recentEvidenceEvents.push_back(eventLine);

            while (g_recentEvidenceEvents.size() > 80)
                g_recentEvidenceEvents.pop_front();
        }
    }

    void UpdateOperatorShellToggle(bool toggleDown)
    {
        if (!g_operatorShellToggleWasDown && toggleDown)
            g_operatorShellVisible = !g_operatorShellVisible;

        g_operatorShellToggleWasDown = toggleDown;
    }

    static const RuntimeTarget& TargetFor(ScreenId id)
    {
        for (const auto& target : kRuntimeTargets)
        {
            if (target.id == id)
                return target;
        }

        return kRuntimeTargets.front();
    }

    static size_t TargetIndexFor(ScreenId id)
    {
        for (size_t i = 0; i < kRuntimeTargets.size(); ++i)
        {
            if (kRuntimeTargets[i].id == id)
                return i;
        }

        return 0;
    }

    static void SelectTargetIndex(size_t index)
    {
        if (index >= kRuntimeTargets.size())
            index = 0;

        g_target = kRuntimeTargets[index].id;
        g_routeTargetExplicit = true;
        g_observerMode = false;
        RequestRouteToCurrentTarget();
        const auto& target = TargetFor(g_target);
        LOGFN("SWARD UI Lab target selected: {} ({})", target.token, target.label);
    }

    static bool TargetCanRouteFromTitleIntro(ScreenId id)
    {
        return TargetRoutesThroughTitleMenu(id);
    }

    static bool TargetNeedsStageHarness(ScreenId id)
    {
        return TargetFor(id).requiresStageContext;
    }

    static bool TargetShouldRouteThroughLoading(ScreenId id)
    {
        return id == ScreenId::Loading || TargetNeedsStageHarness(id);
    }

    static bool TargetRoutesThroughTitleMenu(ScreenId id)
    {
        return id == ScreenId::TitleMenu ||
            id == ScreenId::TitleOptions ||
            TargetShouldRouteThroughLoading(id);
    }

    static bool HasObservedCsdProject(std::string_view projectName)
    {
        return g_loggedCsdProjects.find(std::string(projectName)) != g_loggedCsdProjects.end();
    }

    static std::string_view StageReadyEventName()
    {
        switch (g_target)
        {
            case ScreenId::SonicHud:
                return "sonic-hud-ready";

            case ScreenId::Pause:
                return "pause-ready";

            case ScreenId::Tutorial:
                return "tutorial-ready";

            case ScreenId::Result:
                return "result-ready";

            case ScreenId::ExtraStageHud:
                return "extra-stage-hud-ready";

            default:
                return "stage-target-ready";
        }
    }

    static bool IsPauseTargetRuntimeReady()
    {
        if (g_target != ScreenId::Pause)
            return true;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        return g_pauseGeneralSaveInspector.pauseKnown &&
            g_pauseGeneralSaveInspector.pauseAddress != 0 &&
            g_pauseGeneralSaveInspector.pauseProjectAddress != 0 &&
            (g_pauseGeneralSaveInspector.pauseVisible || g_pauseGeneralSaveInspector.pauseShown);
    }

    static bool IsTutorialTargetRuntimeReady()
    {
        if (g_target != ScreenId::Tutorial)
            return true;

        return
            g_chudSonicStageOwnerAddress != 0 &&
            g_stageContextObserved &&
            g_targetCsdObserved &&
            HasObservedCsdProject("ui_playscreen");
    }

    static bool IsStageTargetRuntimeReady()
    {
        if (g_target == ScreenId::Pause)
            return IsPauseTargetRuntimeReady();

        if (g_target == ScreenId::Tutorial)
            return IsTutorialTargetRuntimeReady();

        return true;
    }

    static bool IsPauseRouteOwnerObserved()
    {
        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        return g_pauseGeneralSaveInspector.pauseKnown &&
            g_pauseGeneralSaveInspector.pauseAddress != 0 &&
            g_pauseGeneralSaveInspector.pauseProjectAddress != 0;
    }

    static void EmitTutorialHudOwnerPathReadyIfNeeded()
    {
        if (g_target != ScreenId::Tutorial ||
            g_loggedTutorialHudOwnerPathReady ||
            !IsTutorialTargetRuntimeReady())
        {
            return;
        }

        g_loggedTutorialHudOwnerPathReady = true;
        WriteEvidenceEvent(
            "tutorial-hud-owner-path-ready",
            "tutorial ready from SonicHudGuide owner path"
            " owner=" + HexU32(g_chudSonicStageOwnerAddress) +
            " stage_address=" + HexU32(g_lastStageGameModeAddress) +
            " target_csd=ui_playscreen"
            " source=Player/Character/Sonic/Hud/SonicHudGuide.cpp");
    }

    void OnStageTargetReady(std::string_view eventName, std::string_view detail)
    {
        if (!g_isEnabled)
            return;

        g_lastStageReadyEventName = std::string(eventName);
        g_lastStageReadyFrame = g_presentedFrameCount;
        g_routePending = false;
        if (g_target == ScreenId::Pause)
            g_routeStatus = "pause target ready";
        else if (g_target == ScreenId::Tutorial)
            g_routeStatus = "tutorial target ready";
        else
            g_routeStatus = "stage target ready";
        WriteEvidenceEvent(eventName, detail);
        WriteLiveStateSnapshot();
    }

    static void EmitStageTargetReadyIfNeeded()
    {
        if (!g_isEnabled ||
            !TargetNeedsStageHarness(g_target) ||
            g_loggedStageTargetReady ||
            !g_stageContextObserved ||
            !g_targetCsdObserved ||
            !IsStageTargetRuntimeReady())
        {
            return;
        }

        const auto& target = TargetFor(g_target);
        const auto detail =
            "stage_address=" + HexU32(g_lastStageGameModeAddress) +
            " target=" + std::string(target.token) +
            " requested_stage=" + g_requestedStageHarness +
            " target_csd=" + std::string(target.primaryCsdScene);

        g_loggedStageTargetReady = true;
        if (g_target == ScreenId::Tutorial)
        {
            EmitTutorialHudOwnerPathReadyIfNeeded();
            WriteEvidenceEvent("tutorial-target-ready", detail);
        }
        const auto readyEvent = StageReadyEventName();
        OnStageTargetReady(readyEvent, detail);

        if (readyEvent != "stage-target-ready")
            WriteEvidenceEvent("stage-target-ready", detail);
    }

    static void MarkTargetCsdProjectLive(std::string_view projectName)
    {
        if (projectName != TargetFor(g_target).primaryCsdScene)
            return;

        g_targetCsdObserved = true;

        if (TargetNeedsStageHarness(g_target) && g_stageContextObserved)
            g_routeStatus = "stage target csd bound";
        else
            g_routeStatus = "target csd project live";

        if (!g_loggedTargetCsdProjectLive)
        {
            WriteEvidenceEvent("target-csd-project-made", projectName);
            g_loggedTargetCsdProjectLive = true;
        }

        if (TargetNeedsStageHarness(g_target) && g_stageContextObserved && !g_loggedStageTargetCsdBound)
        {
            WriteEvidenceEvent(
                "stage-target-csd-bound",
                "target_csd=" + std::string(projectName) +
                " stage_context=1");
            g_loggedStageTargetCsdBound = true;
        }

        EmitStageTargetReadyIfNeeded();
    }

    static void RefreshTargetCsdProjectStatus()
    {
        const auto& target = TargetFor(g_target);

        if (HasObservedCsdProject(target.primaryCsdScene))
            MarkTargetCsdProjectLive(target.primaryCsdScene);
    }

    static bool IsNativeFrameCaptureReady()
    {
        if (!g_isEnabled || !g_nativeFrameCaptureEnabled || g_nativeFrameCaptureReserved)
            return false;

        if (g_nativeFrameCaptureWrittenCount >= g_nativeFrameCaptureMaxCount)
            return false;

        if (g_presentedFrameCount < 2)
            return false;

        if (g_nativeFrameCaptureWrittenCount > 0 &&
            (g_presentedFrameCount <= g_lastNativeFrameCaptureFrame ||
                (g_presentedFrameCount - g_lastNativeFrameCaptureFrame) < g_nativeFrameCaptureIntervalFrames))
        {
            return false;
        }

        if (g_observerMode && !g_routeTargetExplicit)
            return true;

        if (g_target == ScreenId::Pause)
            return IsPauseTargetRuntimeReady();

        if (TargetNeedsStageHarness(g_target))
            return g_stageContextObserved && g_targetCsdObserved;

        switch (g_target)
        {
            case ScreenId::TitleLoop:
            case ScreenId::Loading:
            case ScreenId::Status:
            case ScreenId::WorldMap:
                return g_targetCsdObserved;

            case ScreenId::TitleMenu:
                return g_titleMenuVisualReady;

            case ScreenId::TitleOptions:
                return g_titleMenuAcceptInjected;

            default:
                return g_targetCsdObserved;
        }
    }

    static std::string_view NativeFrameCaptureStatusLabel()
    {
        if (!g_nativeFrameCaptureEnabled)
            return "off";

        if (g_nativeFrameCaptureReserved)
            return "reserved";

        if (g_nativeFrameCaptureWrittenCount >= g_nativeFrameCaptureMaxCount)
            return "complete";

        return IsNativeFrameCaptureReady() ? "ready" : "waiting";
    }

    static std::string_view UiOnlyRenderTargetCaptureStatusLabel()
    {
        if (g_uiOnlyRenderTargetCaptureReserved)
            return "reserved";

        if (!g_lastUiOnlyRenderTargetCapturePath.empty())
            return "active-render-target-copy-before-present";

        if (!g_lastUiOnlyRenderTargetCaptureFailure.empty())
            return "failed";

        if (g_uiOnlyRenderTargetCaptureRequested)
            return "armed-waiting-for-pre-present-copy";

        return "not-requested";
    }

    static std::string_view UiOnlyLayerIsolationStatusLabel()
    {
        if (g_lastUiOnlyRenderTargetCapturePath.empty())
            return "pending-active-render-target-copy";

        return g_lastUiOnlyRenderTargetCaptureContainsFullFramebuffer
            ? "not-isolated-active-color-target"
            : "candidate-dedicated-ui-render-target";
    }

    static std::filesystem::path LiveStateSnapshotPath()
    {
        if (g_evidenceDirectory.empty())
            return {};

        return g_evidenceDirectory / "ui_lab_live_state.json";
    }

    static constexpr std::string_view kLiveStateTargetFieldName = R"("target")";
    static constexpr std::string_view kLiveStateRouteFieldName = R"("route")";
    static constexpr std::string_view kLiveStateStageGameModeAddressFieldName = R"("stageGameModeAddress")";
    static constexpr std::string_view kLiveStateNativeCaptureStatusFieldName = R"("nativeCaptureStatus")";
    static constexpr std::string_view kLiveStateTypedInspectorsFieldName = R"("typedInspectors")";
    static constexpr std::string_view kUiOracleJsonFields[] =
    {
        R"("uiLayerOracle")",
        R"("uiDrawListOracle")",
        R"("gpuSubmitOracle")",
        R"("materialCorrelationOracle")",
        R"("backendResolvedSubmitOracle")",
        R"("backendMaterialParityHints")",
        R"("backendDescriptorSemantics")",
        R"("backendVendorResourceCapture")",
        R"("uiDrawSequence")",
        R"("gpuSubmitSequence")",
        R"("correlationMethod": "same-frame-order-window")",
        R"("backendResolvedJoinMethod": "same-frame-order-window")",
        R"("blendParityPolicy": "backend-resolved-pso-blend")",
        R"("framebufferParityPolicy": "backend-resolved-framebuffer-registration")",
        R"("textureViewSamplerGap": "pending-descriptor-view-decode")",
        R"("textureViewSamplerStatus")",
        R"("textureDescriptorSemantic")",
        R"("samplerDescriptorSemantic")",
        R"("textureDescriptorPolicy": "runtime-texture-view-descriptor-state")",
        R"("samplerDescriptorPolicy": "runtime-sampler-descriptor-state")",
        R"("vendorDescriptorCaptureGap": "pending-native-descriptor-dump")",
        R"("vendorResourceCapturePolicy": "native-rhi-resource-view-and-sampler-handles")",
        R"("vendorResourceCaptureStatus")",
        R"("uiOnlyLayerCaptureStatus": "pending-runtime-ui-render-target-copy")",
        R"("nativeCommandCaptureGap": "pending-full-vendor-command-buffer-dump")",
        R"("nativeTextureResourceHandle")",
        R"("nativeSamplerHandle")",
        R"("backendMaterialResourceViewParity")",
        R"("materialResourceViewParityPolicy": "vendor-resource-view-alpha-gamma-srgb")",
        R"("premultipliedAlphaPolicy": "runtime-blend-state-plus-vendor-resource-view")",
        R"("gammaSrgbPolicy": "native-resource-view-format-classification")",
        R"("resourceViewExactnessStatus")",
        R"("resourceViewExactPairCount")",
        R"("srgbTextureResourceViewCount")",
        R"("premultipliedAlphaStatus")",
        R"("gammaSrgbStatus")",
        R"("uiOnlyRenderTargetCaptureProbe")",
        R"("uiOnlyRenderTargetCapturePolicy": "copy-ui-render-target-before-present")",
        R"("vendorCommandResourceDump")",
        R"("vendorCommandResourceDumpPolicy": "raw-backend-command-plus-resource-view-dump")",
        R"("vendorCommandResourceDumpStatus")",
        R"("rawBackendCommandCount")",
        R"("backendResolvedSubmitCount")",
        R"("textureResourceViewDumpCount")",
        R"("samplerResourceViewDumpCount")",
        R"("resourcePairDumpCount")",
        R"("uiOnlyRenderedLayerStatus": "pending-runtime-ui-render-target-copy")",
        R"("vendorCommandReplayGap": "pending-full-vendor-command-buffer-replay")",
        R"("textMovieSfxGap": "pending-title-loading-media-timing")",
        R"("materialParityHint")",
        R"("materialParityStatus")",
        R"("blendSemantic")",
        R"("blendOperationSemantic")",
        R"("samplerSemantic")",
        R"("addressSemantic")",
        R"("halfPixelOffset")",
        R"("resolvedBackendStatus")",
        R"("nativeCommand")",
        R"("nativePipelineHandle")",
        R"("nativePipelineLayoutHandle")",
        R"("pipelineBlendState")",
        R"("renderTargetFormat0")",
        R"("depthTargetFormat")",
        R"("framebufferSize")",
        R"("resolvedPipelineKnown")",
        R"("rawBackendCommandStatus")",
        R"("runtimeDrawListStatus")",
        R"("runtime CSD platform draw hook; GPU backend submit pending")",
        R"("render-thread material submit hook")",
        R"("drawCalls")",
        R"("gpuDrawListStatus")",
        R"("backendSubmitStatus")",
        R"("pipelineState")",
        R"("alphaBlendEnable")",
        R"("texture2DDescriptorIndex")",
        R"("samplerDescriptorIndex")",
        R"("samplerState")",
        R"("primitive": "quad")",
        R"("screenRect")",
        R"("layerPath")",
        R"("activeScreen")",
        R"("activeScenes")",
        R"("activeMotionName")",
        R"("cursorOwner")",
        R"("transitionBand")",
        R"("inputLockState")",
        R"("runtime CSD tree; GPU draw-list pending")",
    };
    static constexpr std::string_view kLiveStateTypedInspectorJsonFields[] =
    {
        R"("csd")",
        R"("csdProjectTree")",
        R"("titleMenu")",
        R"("loading")",
        R"("pauseGeneralSave")",
        R"("sonicHud")",
        R"("ownerPath")",
        R"("observedProjects")",
        R"("projectAddress")",
        R"("rootNodeAddress")",
        R"("sceneCount")",
        R"("nodeCount")",
        R"("layerCount")",
        R"("scenes")",
        R"("nodes")",
        R"("layers")",
        R"("runtimeSceneMotionFrame")",
        R"("runtimeSceneMotionRepeatTypeLabel")",
        R"("sceneMotionFrame")",
        R"("sceneMotionRepeatType")",
        R"("loadingDisplayTypeLabel")",
        R"("titleMenuOwnerContextAddress")",
        R"("titleMenuCursor")",
        R"("hudOwnerAddress")",
        R"("playScreenProject")",
        R"("speedGaugeScene")",
        R"("rawOwnerKnown")",
        R"("rawOwnerFieldsReady")",
        R"("rawOwnerFrame")",
        R"("rawHookSource")",
        R"("pause")",
        R"("generalWindow")",
        R"("saveIcon")",
        R"("pauseAddress")",
        R"("pauseProjectAddress")",
        R"("pauseAction")",
        R"("pauseActionLabel")",
        R"("generalWindowAddress")",
        R"("generalProjectAddress")",
        R"("generalWindowStatusLabel")",
        R"("saveIconAddress")",
        R"("saveIconVisible")",
        R"("chudSonicStageOwnerAddress")",
        R"("ownerPointerStatus")",
        R"("rcPlayScreenProjectAddress")",
        R"("rcSpeedGaugeSceneAddress")",
        R"("rcRingEnergyGaugeSceneAddress")",
        R"("rcGaugeFrameSceneAddress")",
        R"("resolvedFromCsdProjectTree")",
        R"("expectedOwnerFieldSource")",
        R"("stageTargetReady")",
        R"("stageGameModeAddress")",
        R"("targetCsdObserved")",
        R"("readyEvent")",
    };

    static void AppendStringArray(std::ostringstream& out, const std::vector<std::string_view>& values)
    {
        out << "[";
        for (size_t i = 0; i < values.size(); ++i)
        {
            if (i != 0)
                out << ",";

            out << "\"" << JsonEscape(values[i]) << "\"";
        }
        out << "]";
    }

    static void AppendGuestBoolArray(std::ostringstream& out, const std::array<GuestBoolRef, 7>& values)
    {
        out << "[";
        for (size_t i = 0; i < values.size(); ++i)
        {
            const auto& guestBool = values[i];
            if (i != 0)
                out << ",";

            out
                << "{"
                << "\"name\":\"" << JsonEscape(guestBool.name) << "\","
                << "\"address\":\"" << JsonEscape(HexU32(guestBool.guestAddress)) << "\","
                << "\"readOnly\":" << (guestBool.readOnly ? "true" : "false") << ","
                << "\"value\":" << (ReadGuestBool(guestBool.guestAddress) ? "true" : "false")
                << "}";
        }
        out << "]";
    }

    static void AppendDebugForkTypedFields(std::ostringstream& out)
    {
        out << "[";
        for (size_t i = 0; i < kDebugMenuForkTypedFields.size(); ++i)
        {
            const auto& field = kDebugMenuForkTypedFields[i];
            if (i != 0)
                out << ",";

            out
                << "{"
                << "\"group\":\"" << JsonEscape(field.group) << "\","
                << "\"sourcePath\":\"" << JsonEscape(field.sourcePath) << "\","
                << "\"field\":\"" << JsonEscape(field.field) << "\","
                << "\"usage\":\"" << JsonEscape(field.usage) << "\","
                << "\"status\":\"" << JsonEscape(field.status) << "\""
                << "}";
        }
        out << "]";
    }

    static CsdProjectTreeRecord& EnsureCsdProjectTreeRecordLocked(std::string_view projectName)
    {
        const std::string project(projectName);

        for (auto& record : g_csdProjectTrees)
        {
            if (record.projectName == project)
                return record;
        }

        CsdProjectTreeRecord record;
        record.projectName = project;
        record.frame = g_presentedFrameCount;
        g_csdProjectTrees.push_back(record);
        return g_csdProjectTrees.back();
    }

    static void StoreCsdTreeEntry(
        std::vector<CsdTreeEntry>& entries,
        std::string_view path,
        uint32_t address,
        uint32_t relatedAddress,
        uint32_t firstMetric,
        uint32_t secondMetric)
    {
        // Phase 134: ui_playscreen runtime tree export keeps so_speed_gauge layer samples
        // and other later Sonic HUD scenes instead of truncating after the first entries.
        static constexpr size_t kMaxCsdTreeEntrySamples = 512;

        for (auto& entry : entries)
        {
            if (entry.path == path)
            {
                entry.address = address;
                entry.relatedAddress = relatedAddress;
                entry.firstMetric = firstMetric;
                entry.secondMetric = secondMetric;
                entry.frame = g_presentedFrameCount;
                return;
            }
        }

        if (entries.size() >= kMaxCsdTreeEntrySamples)
            return;

        CsdTreeEntry entry;
        entry.path = std::string(path);
        entry.address = address;
        entry.relatedAddress = relatedAddress;
        entry.firstMetric = firstMetric;
        entry.secondMetric = secondMetric;
        entry.frame = g_presentedFrameCount;
        entries.push_back(std::move(entry));
    }

    static const CsdProjectTreeRecord* FindCsdProjectTreeRecord(
        const std::vector<CsdProjectTreeRecord>& records,
        std::string_view projectName)
    {
        for (const auto& record : records)
        {
            if (record.projectName == projectName)
                return &record;
        }

        return nullptr;
    }

    static CsdLiveInspectorSnapshot BuildCsdLiveInspectorSnapshot()
    {
        static constexpr uint32_t kCsdSceneMotionFrameOffset = 0x64;
        static constexpr uint32_t kCsdSceneMotionRepeatTypeOffset = 0x94;

        CsdLiveInspectorSnapshot snapshot;
        snapshot.projectName = g_targetCsdObserved
            ? std::string(TargetFor(g_target).primaryCsdScene)
            : g_lastCsdProjectName;
        snapshot.source = "CSD project event stream";

        if (g_titleOwnerInspector.titleCsdAddress != 0)
        {
            snapshot.sceneKnown = true;
            snapshot.sceneAddress = g_titleOwnerInspector.titleCsdAddress;
            snapshot.source = "title owner title_csd488";

            float motionFrame = 0.0f;
            uint32_t repeatType = UINT32_MAX;
            const bool motionFrameKnown = TryReadGuestFloat(
                snapshot.sceneAddress + kCsdSceneMotionFrameOffset,
                motionFrame);
            const bool repeatTypeKnown = TryReadGuestU32(
                snapshot.sceneAddress + kCsdSceneMotionRepeatTypeOffset,
                repeatType);

            snapshot.sceneMotionKnown = motionFrameKnown || repeatTypeKnown;
            if (motionFrameKnown)
                snapshot.sceneMotionFrame = motionFrame;

            if (repeatTypeKnown)
                snapshot.sceneMotionRepeatType = repeatType;
        }

        return snapshot;
    }

    static CsdProjectTreeInspectorSnapshot BuildCsdProjectTreeInspectorSnapshot()
    {
        CsdProjectTreeInspectorSnapshot snapshot;
        snapshot.activeProject = g_targetCsdObserved
            ? std::string(TargetFor(g_target).primaryCsdScene)
            : g_lastCsdProjectName;
        snapshot.source = "CCsdProject::Make resource traversal";

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        snapshot.observedProjects = g_observedCsdProjectOrder;
        snapshot.observedProjectCount = static_cast<uint32_t>(g_observedCsdProjectOrder.size());

        const CsdProjectTreeRecord* record = FindCsdProjectTreeRecord(g_csdProjectTrees, snapshot.activeProject);

        if (record == nullptr && !g_csdProjectTrees.empty())
            record = &g_csdProjectTrees.back();

        if (record == nullptr)
            return snapshot;

        snapshot.projectKnown = true;
        snapshot.activeProject = record->projectName;
        snapshot.projectAddress = record->projectAddress;
        snapshot.rootNodeAddress = record->rootNodeAddress;
        snapshot.sceneCount = record->sceneCount;
        snapshot.nodeCount = record->nodeCount;
        snapshot.layerCount = record->layerCount;
        snapshot.scenes = record->scenes;
        snapshot.nodes = record->nodes;
        snapshot.layers = record->layers;
        return snapshot;
    }

    static LoadingLiveInspectorSnapshot BuildLoadingLiveInspectorSnapshot()
    {
        LoadingLiveInspectorSnapshot snapshot;
        snapshot.requestType = g_lastLoadingRequestType;
        snapshot.displayType = g_lastLoadingDisplayType;
        snapshot.displayActive = g_loadingDisplayWasActive;
        snapshot.requestFrame = g_lastLoadingRequestFrame;
        snapshot.displayFrame = g_lastLoadingDisplayFrame;
        return snapshot;
    }

    static uint32_t FindCsdTreeAddressBySuffix(
        const CsdProjectTreeInspectorSnapshot& snapshot,
        std::string_view suffix)
    {
        for (const auto& entry : snapshot.scenes)
        {
            if (entry.path.size() >= suffix.size() &&
                entry.path.compare(entry.path.size() - suffix.size(), suffix.size(), suffix) == 0)
            {
                return entry.address;
            }
        }

        return 0;
    }

    static SonicHudOwnerPathInspectorSnapshot BuildSonicHudOwnerPathInspectorSnapshot(
        const CsdProjectTreeInspectorSnapshot& csdProjectTree)
    {
        SonicHudOwnerPathInspectorSnapshot snapshot;
        snapshot.chudSonicStageOwnerAddress = g_chudSonicStageOwnerAddress;
        snapshot.stageGameModeAddress = g_lastStageGameModeAddress;
        snapshot.rcPlayScreenProjectAddress = g_chudSonicStagePlayScreenProjectAddress;
        snapshot.rcSpeedGaugeSceneAddress = g_chudSonicStageSpeedGaugeSceneAddress;
        snapshot.rcRingEnergyGaugeSceneAddress = g_chudSonicStageRingEnergyGaugeSceneAddress;
        snapshot.rcGaugeFrameSceneAddress = g_chudSonicStageGaugeFrameSceneAddress;
        snapshot.rcScoreCountNodeAddress = g_chudSonicStageScoreCountNodeAddress;
        snapshot.rcTimeCountNodeAddress = g_chudSonicStageTimeCountNodeAddress;
        snapshot.rcTimeCount2NodeAddress = g_chudSonicStageTimeCount2NodeAddress;
        snapshot.rcTimeCount3NodeAddress = g_chudSonicStageTimeCount3NodeAddress;
        snapshot.rcPlayerCountNodeAddress = g_chudSonicStagePlayerCountNodeAddress;
        snapshot.rawOwnerKnown = g_chudSonicStageOwnerAddress != 0;
        snapshot.rawOwnerFieldsReady =
            g_chudSonicStagePlayScreenProjectAddress != 0 ||
            g_chudSonicStageSpeedGaugeSceneAddress != 0 ||
            g_chudSonicStageRingEnergyGaugeSceneAddress != 0 ||
            g_chudSonicStageGaugeFrameSceneAddress != 0 ||
            g_chudSonicStageScoreCountNodeAddress != 0 ||
            g_chudSonicStageTimeCountNodeAddress != 0 ||
            g_chudSonicStagePlayerCountNodeAddress != 0;
        snapshot.rawOwnerFrame = g_chudSonicStageRawHookFrame;
        snapshot.rawHookSource = g_chudSonicStageRawHookSource;
        snapshot.expectedOwnerFieldSource = std::string(kChudSonicStageExpectedOwnerFieldSource);

        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            snapshot.rawOwnerFieldSamples = g_chudSonicStageOwnerFieldSamples;
            snapshot.rawOwnerFieldSampleCount = g_chudSonicStageOwnerFieldSampleCount;
        }

        for (const auto& sample : snapshot.rawOwnerFieldSamples)
        {
            if (sample.resolvedMemoryKnown)
                ++snapshot.rawOwnerResolvedMemoryCount;
        }

        if (snapshot.rawOwnerKnown)
        {
            snapshot.ownerPointerStatus = snapshot.rawOwnerFieldsReady
                ? "raw CHudSonicStage owner hook live"
                : "raw CHudSonicStage owner hook live; CSD owner fields pending";

            if (snapshot.rawOwnerFieldsReady)
            {
                snapshot.ownerFieldMaturationStatus = "fork API CHudSonicStage RCPtr .Get() resolved embedded CSD fields";
            }
            else if (snapshot.rawOwnerResolvedMemoryCount != 0)
            {
                snapshot.ownerFieldMaturationStatus =
                    "raw owner field samples found RCPtr RCObject memory candidates; typed API .Get() fields still pending";
            }
            else if (!snapshot.rawOwnerFieldSamples.empty())
            {
                snapshot.ownerFieldMaturationStatus =
                    "fork API CHudSonicStage RCPtr slots stayed null at sampled hooks; CSD tree owner path is the mature source";
            }
            else
            {
                snapshot.ownerFieldMaturationStatus = "raw owner known; owner field samples pending";
            }
        }
        else
        {
            snapshot.ownerPointerStatus = "raw CHudSonicStage owner hook pending runtime observation";
            snapshot.ownerFieldMaturationStatus = "raw owner pending; field maturation not sampled";
        }

        if (csdProjectTree.projectKnown && csdProjectTree.activeProject == "ui_playscreen")
        {
            snapshot.resolvedFromCsdProjectTree = true;
            if (snapshot.rcPlayScreenProjectAddress == 0)
                snapshot.rcPlayScreenProjectAddress = csdProjectTree.projectAddress;
            if (snapshot.rcSpeedGaugeSceneAddress == 0)
                snapshot.rcSpeedGaugeSceneAddress = FindCsdTreeAddressBySuffix(csdProjectTree, "/so_speed_gauge");
            if (snapshot.rcRingEnergyGaugeSceneAddress == 0)
                snapshot.rcRingEnergyGaugeSceneAddress = FindCsdTreeAddressBySuffix(csdProjectTree, "/so_ringenagy_gauge");
            if (snapshot.rcGaugeFrameSceneAddress == 0)
                snapshot.rcGaugeFrameSceneAddress = FindCsdTreeAddressBySuffix(csdProjectTree, "/gauge_frame");
            snapshot.rcRingCountSceneAddress = FindCsdTreeAddressBySuffix(csdProjectTree, "/ring_count");
            if (snapshot.rcScoreCountNodeAddress == 0)
                snapshot.rcScoreCountNodeAddress = FindCsdTreeAddressBySuffix(csdProjectTree, "/score_count");
            if (snapshot.rcTimeCountNodeAddress == 0)
                snapshot.rcTimeCountNodeAddress = FindCsdTreeAddressBySuffix(csdProjectTree, "/time_count");
            if (snapshot.rcPlayerCountNodeAddress == 0)
                snapshot.rcPlayerCountNodeAddress = FindCsdTreeAddressBySuffix(csdProjectTree, "/player_count");
            snapshot.rcTutorialInfoSceneAddress = FindCsdTreeAddressBySuffix(csdProjectTree, "/add/u_info");

            if (!snapshot.rawOwnerKnown)
                snapshot.ownerPointerStatus = "resolved CSD ownership; raw CHudSonicStage owner hook pending runtime observation";

            if (snapshot.rawOwnerKnown && !snapshot.rawOwnerFieldsReady && snapshot.rawOwnerResolvedMemoryCount == 0)
            {
                snapshot.ownerFieldMaturationStatus =
                    "fork API CHudSonicStage RCPtr slots stayed null; resolved ui_playscreen project/scene addresses came from CCsdProject::Make traversal";
            }
        }

        snapshot.displayOwnerPaths =
            "ring=ui_playscreen/ring_count;"
            "score=CHudSonicStage.m_rcScoreCount|ui_playscreen/score_count/score|ui_playscreen/score_count/num_score;"
            "timer=CHudSonicStage.m_rcTimeCount|ui_playscreen/time_count;"
            "speed=CHudSonicStage.m_rcSpeedGauge|ui_playscreen/so_speed_gauge;"
            "boost=CHudSonicStage.m_rcSpeedGauge|ui_playscreen/so_speed_gauge;"
            "energy=CHudSonicStage.m_rcRingEnergyGauge|ui_playscreen/so_ringenagy_gauge;"
            "lives=CHudSonicStage.m_rcPlayerCount|ui_playscreen/player_count;"
            "tutorial=ui_playscreen/add/u_info";
        snapshot.gameplayNumericBindingStatus =
            SonicHudValueWriteBindingStatus();

        return snapshot;
    }

    static PauseGeneralSaveLiveInspectorSnapshot BuildPauseGeneralSaveLiveInspectorSnapshot()
    {
        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        return g_pauseGeneralSaveInspector;
    }

    static SonicHudLiveInspectorSnapshot BuildSonicHudLiveInspectorSnapshot()
    {
        SonicHudLiveInspectorSnapshot snapshot;
        const auto& target = TargetFor(g_target);
        snapshot.stageContextObserved = g_stageContextObserved;
        snapshot.targetCsdObserved = g_targetCsdObserved;
        snapshot.stageTargetReady = g_loggedStageTargetReady;
        snapshot.rawOwnerKnown = g_chudSonicStageOwnerAddress != 0;
        snapshot.rawOwnerFieldsReady =
            g_chudSonicStagePlayScreenProjectAddress != 0 ||
            g_chudSonicStageSpeedGaugeSceneAddress != 0 ||
            g_chudSonicStageRingEnergyGaugeSceneAddress != 0 ||
            g_chudSonicStageGaugeFrameSceneAddress != 0 ||
            g_chudSonicStageScoreCountNodeAddress != 0 ||
            g_chudSonicStageTimeCountNodeAddress != 0 ||
            g_chudSonicStagePlayerCountNodeAddress != 0;
        snapshot.hudOwnerAddress = g_chudSonicStageOwnerAddress;
        snapshot.stageGameModeAddress = g_lastStageGameModeAddress;
        snapshot.rawOwnerFrame = g_chudSonicStageRawHookFrame;
        snapshot.readyEvent = g_lastStageReadyEventName;
        snapshot.rawHookSource = g_chudSonicStageRawHookSource;
        snapshot.source = snapshot.rawOwnerKnown ? "raw CHudSonicStage owner hook" : "stage/CSD latches";

        if ((target.id == ScreenId::SonicHud || target.id == ScreenId::Tutorial) && g_targetCsdObserved)
        {
            snapshot.playScreenProject = std::string(target.primaryCsdScene);
            snapshot.speedGaugeScene = "ui_playscreen/so_speed_gauge";
            if (!snapshot.rawOwnerKnown)
                snapshot.source = "stage-target-csd-bound";
        }

        return snapshot;
    }

    static SonicHudGameplayValueSnapshot BuildSonicHudGameplayValueSnapshot()
    {
        SonicHudGameplayValueSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            snapshot = g_sonicHudGameplayValues;
        }

        snapshot.source = "typedInspectors.sonicHud.gameplayValues";
        if (snapshot.scoreSource.empty() || snapshot.scoreSource == "pending-runtime-field")
            snapshot.scoreSource = "SWA::CGameDocument::m_pMember->m_ScoreInfo.EnemyScore+TrickScore";

        if (auto pGameDocument = SWA::CGameDocument::GetInstance())
        {
            snapshot.scoreKnown = true;
            snapshot.score =
                pGameDocument->m_pMember->m_ScoreInfo.EnemyScore +
                pGameDocument->m_pMember->m_ScoreInfo.TrickScore;
            snapshot.scoreSource = "SWA::CGameDocument::m_pMember->m_ScoreInfo.EnemyScore+TrickScore";
            snapshot.scoreInfoPointMarkerRecordSpeedKnown = true;
            snapshot.scoreInfoPointMarkerRecordSpeed =
                pGameDocument->m_pMember->m_ScoreInfo.PointMarkerRecordSpeed;
            snapshot.scoreInfoPointMarkerRecordSpeedSource =
                "SWA::CGameDocument::m_pMember->m_ScoreInfo.PointMarkerRecordSpeed";
            snapshot.scoreInfoPointMarkerCountKnown = true;
            snapshot.scoreInfoPointMarkerCount =
                pGameDocument->m_pMember->m_ScoreInfo.PointMarkerCount;
            snapshot.scoreInfoPointMarkerCountSource =
                "SWA::CGameDocument::m_pMember->m_ScoreInfo.PointMarkerCount";
            snapshot.frame = g_presentedFrameCount;
        }

        return snapshot;
    }

    static std::vector<SonicHudValueWriteObservation> BuildSonicHudValueWriteObservations()
    {
        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        return g_sonicHudValueWriteObservations;
    }

    static HudRenderGateCorrelationSnapshot BuildHudRenderGateCorrelationSnapshot()
    {
        HudRenderGateCorrelationSnapshot snapshot;
        snapshot.renderHud = ReadGuestBool(0x8328BB26);
        snapshot.renderGameMainHud = ReadGuestBool(0x8328BB27);
        snapshot.renderHudPause = ReadGuestBool(0x8328BB28);
        snapshot.frame = g_presentedFrameCount;
        snapshot.ms_IsRenderHudCallers = {
            "frontend_listener.cpp toggles ms_IsRenderHud",
            "options_menu.cpp::SetOptionsMenuVisible writes ms_IsRenderHud",
            "CHudPause_patches.cpp reads ms_IsRenderHud for pause/button-guide visibility"
        };
        snapshot.ms_IsRenderGameMainHudCallers = {
            "SGlobals render lane at 0x8328BB27",
            "debug-menu fork SGlobals HUD/render switch",
            "UI Lab sampled child lane for game-main HUD isolation"
        };
        snapshot.ms_IsRenderHudPauseCallers = {
            "SGlobals render lane at 0x8328BB28",
            "debug-menu fork SGlobals HUD/render switch",
            "UI Lab sampled child lane for pause HUD isolation"
        };

        std::unordered_set<std::string> unresolvedKinds;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            for (const auto& observation : g_sonicHudValueWriteObservations)
            {
                if (observation.pathResolved)
                {
                    if (observation.path.starts_with("ui_playscreen/"))
                        ++snapshot.resolvedUiPlayScreenNodeWrites;
                }
                else
                {
                    ++snapshot.unresolvedUiPlayScreenNodeWrites;
                    unresolvedKinds.insert(BaseSonicHudWriteKind(observation.writeKind));
                }
            }
        }

        std::vector<std::string> sortedKinds(unresolvedKinds.begin(), unresolvedKinds.end());
        std::sort(sortedKinds.begin(), sortedKinds.end());
        for (size_t i = 0; i < sortedKinds.size(); ++i)
        {
            if (i != 0)
                snapshot.unresolvedWriteKinds += ",";

            snapshot.unresolvedWriteKinds += sortedKinds[i];
        }

        if (!snapshot.renderHud)
            snapshot.gateStatus = "whole-ui-render-disabled";
        else if (!snapshot.renderGameMainHud && snapshot.renderHudPause)
            snapshot.gateStatus = "pause-ui-only-or-game-main-hud-muted";
        else if (snapshot.renderGameMainHud && !snapshot.renderHudPause)
            snapshot.gateStatus = "game-main-hud-only-or-pause-muted";
        else if (snapshot.renderGameMainHud && snapshot.renderHudPause)
            snapshot.gateStatus = "whole-ui-render-enabled";
        else
            snapshot.gateStatus = "whole-ui-render-enabled-child-lanes-muted";

        if (snapshot.unresolvedUiPlayScreenNodeWrites > 0)
            snapshot.gateStatus += ":unresolved-ui_playscreen-node-writes-present";

        return snapshot;
    }

    static void UpdateHudRenderGateCorrelation()
    {
        const auto snapshot = BuildHudRenderGateCorrelationSnapshot();
        const bool changed =
            !g_hudRenderGateCorrelationKnown ||
            g_lastHudRenderGateRenderHud != snapshot.renderHud ||
            g_lastHudRenderGateGameMainHud != snapshot.renderGameMainHud ||
            g_lastHudRenderGatePauseHud != snapshot.renderHudPause ||
            g_lastHudRenderGateUnresolvedWrites != snapshot.unresolvedUiPlayScreenNodeWrites;

        const bool periodicUnresolved =
            snapshot.unresolvedUiPlayScreenNodeWrites > 0 &&
            g_presentedFrameCount >= g_lastHudRenderGateCorrelationEvidenceFrame + 600;

        if (!changed && !periodicUnresolved)
            return;

        g_hudRenderGateCorrelationKnown = true;
        g_lastHudRenderGateRenderHud = snapshot.renderHud;
        g_lastHudRenderGateGameMainHud = snapshot.renderGameMainHud;
        g_lastHudRenderGatePauseHud = snapshot.renderHudPause;
        g_lastHudRenderGateUnresolvedWrites = snapshot.unresolvedUiPlayScreenNodeWrites;
        g_lastHudRenderGateCorrelationEvidenceFrame = g_presentedFrameCount;

        WriteEvidenceEvent(
            "sonic-hud-render-gate-correlated",
            "ms_IsRenderHud=" + std::to_string(snapshot.renderHud ? 1 : 0) +
            " ms_IsRenderGameMainHud=" + std::to_string(snapshot.renderGameMainHud ? 1 : 0) +
            " ms_IsRenderHudPause=" + std::to_string(snapshot.renderHudPause ? 1 : 0) +
            " unresolved_ui_playscreen_node_writes=" +
            std::to_string(snapshot.unresolvedUiPlayScreenNodeWrites) +
            " gate_status=" + snapshot.gateStatus);
    }

    static std::vector<SonicHudUpdateCallsiteSample> BuildSonicHudUpdateCallsiteSamples()
    {
        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        return g_sonicHudUpdateCallsiteSamples;
    }

    static SonicHudLastClassifiedCallsiteValue BuildSonicHudLastClassifiedCallsiteValue()
    {
        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        return g_lastSonicHudClassifiedCallsiteValue;
    }

    static std::string SonicHudValueWriteBindingStatus()
    {
        // Legacy phase markers retained for contract archaeology:
        // ring/timer/speed/lives:known-via-csd-text-write
        // timer/counter/speed/gauge candidates:sampled-via-chud-update-callsites
        // boost/energy/tutorial:csd-node-pattern-hide-scale-hooks-installed-with-unresolved-write-probe-pending-runtime-normalization
        return "score:known-via-csd-text-or-game-document,scoreinfo:known,"
            "ring/speed/lives:known-via-csd-text-write,"
            "timer:runtime-proven-via-chud-update-callsite-sample,"
            "counter/speed/gauge candidates:sampled-via-chud-update-callsites,"
            "boost/energy/tutorial:classified-callsite-candidates-pending-normalization";
    }

    static bool IsSonicHudValueTextPath(std::string_view path)
    {
        return
            path == "ui_playscreen/ring_count/num_ring" ||
            path == "ui_playscreen/score_count/score" ||
            path == "ui_playscreen/score_count/num_score" ||
            path == "ui_playscreen/time_count/time001" ||
            path == "ui_playscreen/time_count/time010" ||
            path == "ui_playscreen/time_count/time100" ||
            path == "ui_playscreen/add/speed_count/position/num_speed" ||
            path == "ui_playscreen/player_count/player";
    }

    static std::string SonicHudValueNameFromTextPath(std::string_view path)
    {
        if (path == "ui_playscreen/ring_count/num_ring")
            return "ringCount";
        if (
            path == "ui_playscreen/score_count/score" ||
            path == "ui_playscreen/score_count/num_score")
        {
            return "score";
        }
        if (
            path == "ui_playscreen/time_count/time001" ||
            path == "ui_playscreen/time_count/time010" ||
            path == "ui_playscreen/time_count/time100")
        {
            return "elapsedFrames";
        }
        if (path == "ui_playscreen/add/speed_count/position/num_speed")
            return "speedKmh";
        if (path == "ui_playscreen/player_count/player")
            return "lifeCount";
        return "unknown";
    }

    static bool IsSonicHudGaugeOrPromptPath(std::string_view path)
    {
        return
            path.starts_with("ui_playscreen/so_speed_gauge") ||
            path.starts_with("ui_playscreen/gauge_frame") ||
            path.starts_with("ui_playscreen/so_ringenagy_gauge") ||
            path.starts_with("ui_playscreen/add/u_info");
    }

    static std::string SonicHudValueNameFromGaugeOrPromptPath(std::string_view path)
    {
        if (path.starts_with("ui_playscreen/so_speed_gauge") || path.starts_with("ui_playscreen/gauge_frame"))
            return "boostGauge";
        if (path.starts_with("ui_playscreen/so_ringenagy_gauge"))
            return "ringEnergyGauge";
        if (path.starts_with("ui_playscreen/add/u_info"))
            return "tutorialPrompt";
        return "unknown";
    }

    static bool TryParseUnsignedText(std::string_view text, uint32_t& value)
    {
        uint64_t result = 0;
        bool sawDigit = false;

        for (char c : text)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
                continue;

            sawDigit = true;
            result = (result * 10) + static_cast<uint32_t>(c - '0');
            if (result > UINT32_MAX)
                result = UINT32_MAX;
        }

        if (!sawDigit)
            return false;

        value = static_cast<uint32_t>(result);
        return true;
    }

    static bool HasRecentUiPlayScreenDrawActivity()
    {
        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);

        if (g_runtimeUiDrawCalls.empty())
            return false;

        if (g_runtimeUiDrawListFrame > g_presentedFrameCount)
            return false;

        if ((g_presentedFrameCount - g_runtimeUiDrawListFrame) > 180)
            return false;

        return std::any_of(
            g_runtimeUiDrawCalls.begin(),
            g_runtimeUiDrawCalls.end(),
            [](const RuntimeUiDrawCall& call)
            {
                return
                    call.projectName == "ui_playscreen" ||
                    call.layerPath.starts_with("ui_playscreen/");
            });
    }

    static bool IsLikelySonicHudUnresolvedValue(
        std::string_view writeKind,
        std::string_view valueText)
    {
        if (writeKind != "text")
            return true;

        if (valueText.empty() || valueText.size() > 8)
            return false;

        return std::all_of(
            valueText.begin(),
            valueText.end(),
            [](char c)
            {
                return std::isdigit(static_cast<unsigned char>(c));
            });
    }

    static SonicHudUpdateContextFrame CurrentSonicHudUpdateContext()
    {
        if (g_sonicHudUpdateContextStack.empty())
            return {};

        return g_sonicHudUpdateContextStack.back();
    }

    static int32_t SourceOwnerOffsetFromUpdateOwner(
        uint32_t sourceOwnerAddress,
        const SonicHudUpdateContextFrame& updateContext)
    {
        if (
            updateContext.ownerAddress == 0 ||
            sourceOwnerAddress < updateContext.ownerAddress)
        {
            return INT32_MIN;
        }

        const uint32_t offset = sourceOwnerAddress - updateContext.ownerAddress;
        if (offset > 0x2000)
            return INT32_MIN;

        return static_cast<int32_t>(offset);
    }

    static std::string OptionalOffsetText(int32_t offset)
    {
        if (offset == INT32_MIN)
            return "unknown";

        return HexU32(static_cast<uint32_t>(offset));
    }

    static void TrimCsdLookupObservationMapsLocked()
    {
        while (g_csdChildNodeLookupObservations.size() > kCsdChildNodeLookupObservationLimit)
            g_csdChildNodeLookupObservations.erase(g_csdChildNodeLookupObservations.begin());

        while (g_csdNodeSourceOwnerObservations.size() > kCsdChildNodeLookupObservationLimit)
            g_csdNodeSourceOwnerObservations.erase(g_csdNodeSourceOwnerObservations.begin());
    }

    static std::string ResolveAnyCsdNodePathLocked(uint32_t nodeAddress)
    {
        if (!IsPlausibleGuestPointer(nodeAddress))
            return {};

        for (const auto& record : g_csdProjectTrees)
        {
            for (const auto& entry : record.layers)
            {
                if (entry.address == nodeAddress || entry.relatedAddress == nodeAddress)
                    return entry.path;
            }

            for (const auto& entry : record.nodes)
            {
                if (entry.address == nodeAddress || entry.relatedAddress == nodeAddress)
                    return entry.path;
            }

            for (const auto& entry : record.scenes)
            {
                if (entry.address == nodeAddress || entry.relatedAddress == nodeAddress)
                    return entry.path;
            }
        }

        for (auto it = g_runtimeUiDrawCalls.rbegin(); it != g_runtimeUiDrawCalls.rend(); ++it)
        {
            if (g_presentedFrameCount >= it->frame && (g_presentedFrameCount - it->frame) > 180)
                continue;

            if (
                (it->layerAddress == nodeAddress || it->castNodeAddress == nodeAddress) &&
                !it->layerPath.empty() &&
                it->layerPath != "unresolved")
            {
                return it->layerPath;
            }
        }

        const auto sourceOwner = g_csdNodeSourceOwnerObservations.find(nodeAddress);
        if (sourceOwner != g_csdNodeSourceOwnerObservations.end() && !sourceOwner->second.path.empty())
            return sourceOwner->second.path;

        return {};
    }

    static std::string ResolveCsdNodePathFromLookupChainLocked(
        uint32_t nodeAddress,
        bool (*pathPredicate)(std::string_view))
    {
        const auto sourceOwner = g_csdNodeSourceOwnerObservations.find(nodeAddress);
        if (sourceOwner == g_csdNodeSourceOwnerObservations.end())
            return {};

        const auto& source = sourceOwner->second;
        if (!source.path.empty() && pathPredicate(source.path))
            return source.path;

        if (source.childName.empty())
            return {};

        std::string parentPath;
        const auto lookup = g_csdChildNodeLookupObservations.find(source.sourceOwnerAddress);
        if (lookup != g_csdChildNodeLookupObservations.end())
        {
            parentPath = lookup->second.parentPath;
            if (parentPath.empty())
                parentPath = ResolveAnyCsdNodePathLocked(lookup->second.parentNodeAddress);
        }

        if (parentPath.empty())
            parentPath = ResolveAnyCsdNodePathLocked(source.parentNodeAddress);

        if (parentPath.empty())
            return {};

        const std::string path = parentPath + "/" + source.childName;
        if (!pathPredicate(path))
            return {};

        return path;
    }

    static std::string ResolveSonicHudPathFromNodeSourceOwnerLocked(
        uint32_t nodeAddress,
        bool (*pathPredicate)(std::string_view))
    {
        return ResolveCsdNodePathFromLookupChainLocked(nodeAddress, pathPredicate);
    }

    static std::string ResolveSonicHudPathFromRawOwnerFieldsLocked(
        uint32_t nodeAddress,
        bool (*pathPredicate)(std::string_view),
        std::string* pathResolutionSource = nullptr)
    {
        static constexpr std::string_view kRawOwnerFieldResolutionSource =
            "raw-chud-sonic-stage-owner-field"; // pathResolutionSource=raw-chud-sonic-stage-owner-field

        struct FieldPath
        {
            uint32_t address;
            std::string_view path;
        };

        const FieldPath fields[] =
        {
            { g_chudSonicStageTimeCountNodeAddress, "ui_playscreen/time_count/time001" },
            { g_chudSonicStageTimeCount2NodeAddress, "ui_playscreen/time_count/time010" },
            { g_chudSonicStageTimeCount3NodeAddress, "ui_playscreen/time_count/time100" },
            { g_chudSonicStagePlayerCountNodeAddress, "ui_playscreen/player_count/player" },
        };

        for (const auto& field : fields)
        {
            if (field.address == 0 || field.address != nodeAddress || !pathPredicate(field.path))
                continue;

            if (pathResolutionSource != nullptr)
                *pathResolutionSource = kRawOwnerFieldResolutionSource;
            return std::string(field.path);
        }

        return {};
    }

    static bool RuntimeUiDrawCallSonicHudPathMatchesNode(
        const RuntimeUiDrawCall& call,
        uint32_t nodeAddress,
        bool (*pathPredicate)(std::string_view))
    {
        if (!IsPlausibleGuestPointer(nodeAddress))
            return false;

        if (
            call.projectName != "ui_playscreen" &&
            !call.layerPath.starts_with("ui_playscreen/"))
        {
            return false;
        }

        if (call.layerPath.empty() || call.layerPath == "unresolved")
            return false;

        if (call.layerAddress != nodeAddress && call.castNodeAddress != nodeAddress)
            return false;

        return pathPredicate(call.layerPath);
    }

    static std::string ResolveSonicHudPathFromRecentDrawCallsLocked(
        uint32_t nodeAddress,
        bool (*pathPredicate)(std::string_view))
    {
        for (auto it = g_runtimeUiDrawCalls.rbegin(); it != g_runtimeUiDrawCalls.rend(); ++it)
        {
            if (g_presentedFrameCount >= it->frame && (g_presentedFrameCount - it->frame) > 180)
                continue;

            if (RuntimeUiDrawCallSonicHudPathMatchesNode(*it, nodeAddress, pathPredicate))
                return it->layerPath;
        }

        return {};
    }

    static std::string ResolveSonicHudPathFromRecentDrawCalls(
        uint32_t nodeAddress,
        bool (*pathPredicate)(std::string_view),
        std::string* pathResolutionSource = nullptr)
    {
        if (!IsPlausibleGuestPointer(nodeAddress))
            return {};

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        const std::string path = ResolveSonicHudPathFromRecentDrawCallsLocked(nodeAddress, pathPredicate);
        if (!path.empty() && pathResolutionSource != nullptr)
            *pathResolutionSource = "recent-ui-draw-list";
        return path;
    }

    static std::string BaseSonicHudWriteKind(std::string_view writeKind)
    {
        constexpr std::string_view suffix = "-unresolved";
        if (writeKind.size() > suffix.size() &&
            writeKind.substr(writeKind.size() - suffix.size()) == suffix)
        {
            return std::string(writeKind.substr(0, writeKind.size() - suffix.size()));
        }

        return std::string(writeKind);
    }

    static bool IsSonicHudGaugeOrPromptWriteKind(std::string_view writeKind)
    {
        const std::string baseKind = BaseSonicHudWriteKind(writeKind);
        return
            baseKind == "pattern-index" ||
            baseKind == "hide-flag" ||
            baseKind == "scale";
    }

    static std::string ResolveSonicHudValuePathFromCsdNode(
        uint32_t nodeAddress,
        std::string* pathResolutionSource = nullptr)
    {
        if (!IsPlausibleGuestPointer(nodeAddress))
            return {};

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        for (const auto& record : g_csdProjectTrees)
        {
            if (record.projectName != "ui_playscreen")
                continue;

            for (const auto& entry : record.layers)
            {
                if ((entry.relatedAddress == nodeAddress || entry.address == nodeAddress) &&
                    IsSonicHudValueTextPath(entry.path))
                {
                    if (pathResolutionSource != nullptr)
                        *pathResolutionSource = "csd-project-tree";
                    return entry.path;
                }
            }

            for (const auto& entry : record.nodes)
            {
                if (entry.address == nodeAddress && IsSonicHudValueTextPath(entry.path))
                {
                    if (pathResolutionSource != nullptr)
                        *pathResolutionSource = "csd-project-tree";
                    return entry.path;
                }
            }
        }

        const std::string ownerFieldPath = ResolveSonicHudPathFromRawOwnerFieldsLocked(
            nodeAddress,
            IsSonicHudValueTextPath,
            pathResolutionSource);
        if (!ownerFieldPath.empty())
            return ownerFieldPath;

        const std::string lookupPath = ResolveSonicHudPathFromNodeSourceOwnerLocked(
            nodeAddress,
            IsSonicHudValueTextPath);
        if (!lookupPath.empty())
        {
            if (pathResolutionSource != nullptr)
                *pathResolutionSource = "csd-child-lookup-chain";
            return lookupPath;
        }

        const std::string path = ResolveSonicHudPathFromRecentDrawCallsLocked(nodeAddress, IsSonicHudValueTextPath);
        if (!path.empty() && pathResolutionSource != nullptr)
            *pathResolutionSource = "recent-ui-draw-list";
        return path;
    }

    static std::string ResolveSonicHudGaugeOrPromptPathFromCsdNode(
        uint32_t nodeAddress,
        std::string* pathResolutionSource = nullptr)
    {
        if (!IsPlausibleGuestPointer(nodeAddress))
            return {};

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        for (const auto& record : g_csdProjectTrees)
        {
            if (record.projectName != "ui_playscreen")
                continue;

            for (const auto& entry : record.layers)
            {
                if ((entry.relatedAddress == nodeAddress || entry.address == nodeAddress) &&
                    IsSonicHudGaugeOrPromptPath(entry.path))
                {
                    if (pathResolutionSource != nullptr)
                        *pathResolutionSource = "csd-project-tree";
                    return entry.path;
                }
            }

            for (const auto& entry : record.nodes)
            {
                if (entry.address == nodeAddress && IsSonicHudGaugeOrPromptPath(entry.path))
                {
                    if (pathResolutionSource != nullptr)
                        *pathResolutionSource = "csd-project-tree";
                    return entry.path;
                }
            }
        }

        const std::string ownerFieldPath = ResolveSonicHudPathFromRawOwnerFieldsLocked(
            nodeAddress,
            IsSonicHudGaugeOrPromptPath,
            pathResolutionSource);
        if (!ownerFieldPath.empty())
            return ownerFieldPath;

        const std::string lookupPath = ResolveSonicHudPathFromNodeSourceOwnerLocked(
            nodeAddress,
            IsSonicHudGaugeOrPromptPath);
        if (!lookupPath.empty())
        {
            if (pathResolutionSource != nullptr)
                *pathResolutionSource = "csd-child-lookup-chain";
            return lookupPath;
        }

        const std::string path = ResolveSonicHudPathFromRecentDrawCallsLocked(nodeAddress, IsSonicHudGaugeOrPromptPath);
        if (!path.empty() && pathResolutionSource != nullptr)
            *pathResolutionSource = "recent-ui-draw-list";
        return path;
    }

    static bool ApplySonicHudTextWriteToGameplayValues(
        std::string_view path,
        std::string_view textUtf8,
        std::string_view hookSource)
    {
        uint32_t parsedValue = 0;
        if (!TryParseUnsignedText(textUtf8, parsedValue))
            return false;

        const std::string source =
            std::string(hookSource) + "@" + std::string(path);

        bool changed = false;
        SonicHudGameplayValueSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            snapshot = g_sonicHudGameplayValues;

            if (path == "ui_playscreen/ring_count/num_ring")
            {
                changed =
                    !snapshot.ringCountKnown ||
                    snapshot.ringCount != parsedValue ||
                    snapshot.ringCountSource != source;
                snapshot.ringCountKnown = true;
                snapshot.ringCount = parsedValue;
                snapshot.ringCountSource = source;
            }
            else if (
                path == "ui_playscreen/score_count/score" ||
                path == "ui_playscreen/score_count/num_score")
            {
                changed =
                    !snapshot.scoreKnown ||
                    snapshot.score != parsedValue ||
                    snapshot.scoreSource != source;
                snapshot.scoreKnown = true;
                snapshot.score = parsedValue;
                snapshot.scoreSource = source;
            }
            else if (
                path == "ui_playscreen/time_count/time001" ||
                path == "ui_playscreen/time_count/time010" ||
                path == "ui_playscreen/time_count/time100")
            {
                changed =
                    !snapshot.elapsedFramesKnown ||
                    snapshot.elapsedFrames != parsedValue ||
                    snapshot.elapsedFramesSource != source;
                // This is the exact CSD-displayed timer write sink. It is promoted
                // as elapsedFrames until the earlier game-clock storage path is proven.
                snapshot.elapsedFramesKnown = true;
                snapshot.elapsedFrames = parsedValue;
                snapshot.elapsedFramesSource = source;
            }
            else if (path == "ui_playscreen/add/speed_count/position/num_speed")
            {
                const float speed = static_cast<float>(parsedValue);
                changed =
                    !snapshot.speedKmhKnown ||
                    snapshot.speedKmh != speed ||
                    snapshot.speedKmhSource != source;
                snapshot.speedKmhKnown = true;
                snapshot.speedKmh = speed;
                snapshot.speedKmhSource = source;
            }
            else if (path == "ui_playscreen/player_count/player")
            {
                changed =
                    !snapshot.lifeCountKnown ||
                    snapshot.lifeCount != parsedValue ||
                    snapshot.lifeCountSource != source;
                snapshot.lifeCountKnown = true;
                snapshot.lifeCount = parsedValue;
                snapshot.lifeCountSource = source;
            }

            if (changed)
            {
                snapshot.frame = g_presentedFrameCount;
                g_sonicHudGameplayValues = std::move(snapshot);
            }
        }

        if (changed)
        {
            WriteEvidenceEvent(
                "sonic-hud-value-write-update",
                "path=" + std::string(path) +
                " value=" + std::to_string(parsedValue) +
                " source=" + source);
        }

        return changed;
    }

    static bool ApplySonicHudGaugeOrPromptWriteToGameplayValues(
        std::string_view path,
        std::string_view writeKind,
        bool numericValueKnown,
        double numericValue,
        std::string_view hookSource)
    {
        if (!numericValueKnown)
            return false;

        const std::string baseKind = BaseSonicHudWriteKind(writeKind);
        const std::string source =
            std::string(hookSource) + "@" + std::string(path);

        bool changed = false;
        SonicHudGameplayValueSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            snapshot = g_sonicHudGameplayValues;

            if (
                baseKind == "scale" &&
                (path.starts_with("ui_playscreen/so_speed_gauge") ||
                    path.starts_with("ui_playscreen/gauge_frame")))
            {
                const float boostGauge = static_cast<float>(numericValue);
                changed =
                    !snapshot.boostGaugeKnown ||
                    snapshot.boostGauge != boostGauge ||
                    snapshot.boostGaugeSource != source;
                snapshot.boostGaugeKnown = true;
                snapshot.boostGauge = boostGauge;
                snapshot.boostGaugeSource = source;
            }
            else if (baseKind == "scale" && path.starts_with("ui_playscreen/so_ringenagy_gauge"))
            {
                const float ringEnergyGauge = static_cast<float>(numericValue);
                changed =
                    !snapshot.ringEnergyGaugeKnown ||
                    snapshot.ringEnergyGauge != ringEnergyGauge ||
                    snapshot.ringEnergyGaugeSource != source;
                snapshot.ringEnergyGaugeKnown = true;
                snapshot.ringEnergyGauge = ringEnergyGauge;
                snapshot.ringEnergyGaugeSource = source;
            }
            else if (path.starts_with("ui_playscreen/add/u_info"))
            {
                const bool visible = baseKind == "hide-flag"
                    ? numericValue == 0.0
                    : true;
                const std::string promptId = baseKind == "pattern-index"
                    ? ("pattern-" + std::to_string(static_cast<uint32_t>(numericValue)))
                    : "u_info";
                changed =
                    !snapshot.tutorialPromptKnown ||
                    snapshot.tutorialVisible != visible ||
                    snapshot.tutorialPromptId != promptId ||
                    snapshot.tutorialPromptSource != source;
                snapshot.tutorialPromptKnown = true;
                snapshot.tutorialVisible = visible;
                snapshot.tutorialPromptId = promptId;
                snapshot.tutorialPromptSource = source;
            }

            if (changed)
            {
                snapshot.frame = g_presentedFrameCount;
                g_sonicHudGameplayValues = std::move(snapshot);
            }
        }

        if (changed)
        {
            WriteEvidenceEvent(
                "sonic-hud-value-write-update",
                "path=" + std::string(path) +
                " kind=" + baseKind +
                " value=" + std::to_string(numericValue) +
                " source=" + source);
        }

        return changed;
    }

    static bool ApplySonicHudSemanticPathCandidateToGameplayValues(
        const SonicHudSemanticPathCandidate& candidate,
        std::string_view writeKind,
        std::string_view valueText,
        bool numericValueKnown,
        double numericValue,
        std::string_view hookSource,
        const SonicHudNodeWriteCallsiteCorrelation& callsiteCorrelation)
    {
        const std::string baseKind = BaseSonicHudWriteKind(writeKind);
        const std::string source =
            "generated-PPC-callsite-semantic-candidate:" +
            callsiteCorrelation.source +
            ":" + std::string(hookSource) +
            ":semanticValueName=" + candidate.valueName +
            ":semanticBindingStatus=stable-candidate-bound-pending-exact-child-node-resolution";

        if (
            baseKind == "text" &&
            (candidate.valueName == "elapsedFrames" || candidate.valueName == "speedKmh"))
        {
            return ApplySonicHudTextWriteToGameplayValues(
                candidate.path,
                valueText,
                source);
        }

        if (
            baseKind == "scale" &&
            (candidate.valueName == "boostGauge" || candidate.valueName == "ringEnergyGauge"))
        {
            return ApplySonicHudGaugeOrPromptWriteToGameplayValues(
                candidate.path,
                baseKind,
                numericValueKnown,
                numericValue,
                source);
        }

        if (
            candidate.valueName == "tutorialPrompt" &&
            (baseKind == "pattern-index" || baseKind == "hide-flag"))
        {
            return ApplySonicHudGaugeOrPromptWriteToGameplayValues(
                candidate.path,
                baseKind,
                numericValueKnown,
                numericValue,
                source);
        }

        return false;
    }

    static bool ClassifySonicHudUpdateCallsiteSample(
        const SonicHudUpdateCallsiteSample& sample,
        std::string& valueName,
        std::string& status,
        std::string& source,
        uint32_t& normalizedValue,
        bool& normalizedValueKnown)
    {
        valueName.clear();
        status.clear();
        source.clear();
        normalizedValue = 0;
        normalizedValueKnown = false;

        if (sample.hookName == "sub_824D6048" && sample.samplePhase == "post-original")
        {
            valueName = "elapsedFrames";
            status = "runtime-proven-via-chud-update-callsite-sample";
            source = "generated-PPC:sub_824D6048 owner+456/+452 -> CSD::CNode::SetText";
            normalizedValue = sample.ownerField456 * 60 + std::min<uint32_t>(sample.ownerField452, 59);
            normalizedValueKnown = true;
            return true;
        }

        if (sample.hookName == "sub_824D6418")
        {
            valueName = "speedKmh";
            status = "classified-via-generated-PPC-callsite-candidate";
            source = "generated-PPC:sub_824D6418 speed readout via sub_8251A568";
            return true;
        }

        if (sample.hookName == "sub_824D6C18")
        {
            valueName = "rollingCounterGaugeState";
            status = "classified-via-generated-PPC-callsite-candidate";
            source = "generated-PPC:sub_824D6C18 owner+460/+480 rolling counter/gauge state";
            return true;
        }

        if (sample.hookName == "sub_824D7100")
        {
            valueName = "tutorialPrompt";
            status = "classified-via-generated-PPC-callsite-candidate";
            source = "generated-PPC:sub_824D7100 tutorial/overlay update context";
            return true;
        }

        return false;
    }

    static std::string SonicHudCallsiteCandidateFromValueName(std::string_view valueName)
    {
        if (valueName == "elapsedFrames")
            return "timer";

        if (valueName == "speedKmh")
            return "speed";

        if (valueName == "rollingCounterGaugeState")
            return "boost-ring-energy";

        if (valueName == "tutorialPrompt")
            return "tutorial";

        return {};
    }

    static std::vector<SonicHudSemanticPathCandidate> ResolveSonicHudSemanticPathCandidateFromCallsiteCorrelation(
        std::string_view writeKind,
        std::string_view valueCandidate)
    {
        std::string baseKind(writeKind);
        constexpr std::string_view unresolvedSuffix = "-unresolved";
        if (
            baseKind.size() > unresolvedSuffix.size() &&
            std::string_view(baseKind).substr(baseKind.size() - unresolvedSuffix.size()) == unresolvedSuffix)
        {
            baseKind.resize(baseKind.size() - unresolvedSuffix.size());
        }
        std::vector<SonicHudSemanticPathCandidate> candidates;

        if (valueCandidate == "timer")
        {
            candidates.push_back({ "ui_playscreen/time_count/time001", "elapsedFrames" });
            return candidates;
        }

        if (valueCandidate == "speed")
        {
            candidates.push_back({ "ui_playscreen/add/speed_count/position/num_speed", "speedKmh" });
            return candidates;
        }

        if (valueCandidate == "boost-ring-energy")
        {
            // sub_824D6C18 touches the shared rolling counter/gauge lane. Keep
            // both stable scene candidates until an exact child-node path proves
            // whether this sample is the boost meter or the ring-energy meter.
            if (baseKind == "scale" || baseKind == "text" || baseKind == "pattern-index")
            {
                candidates.push_back({ "ui_playscreen/so_speed_gauge", "boostGauge" });
                candidates.push_back({ "ui_playscreen/so_ringenagy_gauge", "ringEnergyGauge" });
            }
            return candidates;
        }

        if (valueCandidate == "tutorial")
        {
            candidates.push_back({ "ui_playscreen/add/u_info", "tutorialPrompt" });
            return candidates;
        }

        return candidates;
    }

    static std::string JoinSonicHudSemanticPathCandidates(
        const std::vector<SonicHudSemanticPathCandidate>& candidates)
    {
        std::string joined;
        for (const auto& candidate : candidates)
        {
            if (candidate.path.empty())
                continue;

            if (!joined.empty())
                joined += "|";
            joined += candidate.path;
        }
        return joined;
    }

    static std::string JoinSonicHudSemanticValueNames(
        const std::vector<SonicHudSemanticPathCandidate>& candidates)
    {
        std::string joined;
        for (const auto& candidate : candidates)
        {
            if (candidate.valueName.empty())
                continue;

            if (!joined.empty())
                joined += "|";
            joined += candidate.valueName;
        }
        return joined;
    }

    static std::string SonicHudHookNameFromUpdateContext(std::string_view hookSource)
    {
        if (hookSource.find("sub_824D6048") != std::string_view::npos)
            return "sub_824D6048";

        if (hookSource.find("sub_824D6418") != std::string_view::npos)
            return "sub_824D6418";

        if (hookSource.find("sub_824D69B0") != std::string_view::npos)
            return "sub_824D69B0";

        if (hookSource.find("sub_824D6C18") != std::string_view::npos)
            return "sub_824D6C18";

        if (hookSource.find("sub_824D7100") != std::string_view::npos)
            return "sub_824D7100";

        return {};
    }

    static SonicHudNodeWriteCallsiteCorrelation CorrelateUnresolvedSonicHudNodeWriteWithCallsite(
        std::string_view writeKind,
        uint32_t nodeAddress)
    {
        (void)writeKind;
        (void)nodeAddress;

        SonicHudNodeWriteCallsiteCorrelation correlation;
        const auto updateContext = CurrentSonicHudUpdateContext();
        const std::string contextHook =
            SonicHudHookNameFromUpdateContext(updateContext.hookSource);

        if (!contextHook.empty())
        {
            if (contextHook == "sub_824D6048")
                correlation.valueCandidate = "timer";
            else if (contextHook == "sub_824D6418")
                correlation.valueCandidate = "speed";
            else if (contextHook == "sub_824D69B0" || contextHook == "sub_824D6C18")
                correlation.valueCandidate = "boost-ring-energy";
            else if (contextHook == "sub_824D7100")
                correlation.valueCandidate = "tutorial";

            if (!correlation.valueCandidate.empty())
            {
                correlation.known = true;
                correlation.source = "same-frame-hud-update-context:" + contextHook;
                correlation.status = std::string(kSonicHudNodeWriteCandidateSetLabel);
                correlation.frameDelta =
                    static_cast<int32_t>(g_presentedFrameCount - updateContext.frame);
                return correlation;
            }
        }

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        for (auto it = g_sonicHudUpdateCallsiteSamples.rbegin();
            it != g_sonicHudUpdateCallsiteSamples.rend();
            ++it)
        {
            const SonicHudUpdateCallsiteSample& sample = *it;
            const uint64_t frameDelta =
                sample.frame > g_presentedFrameCount
                    ? sample.frame - g_presentedFrameCount
                    : g_presentedFrameCount - sample.frame;

            if (frameDelta > kSonicHudNodeCallsiteCorrelationFrameWindow)
                continue;

            std::string valueName;
            std::string status;
            std::string source;
            uint32_t normalizedValue = 0;
            bool normalizedValueKnown = false;

            if (!ClassifySonicHudUpdateCallsiteSample(
                    sample,
                    valueName,
                    status,
                    source,
                    normalizedValue,
                    normalizedValueKnown))
            {
                continue;
            }

            const std::string candidate =
                SonicHudCallsiteCandidateFromValueName(valueName);
            if (candidate.empty())
                continue;

            correlation.known = true;
            correlation.valueCandidate = candidate;
            correlation.source = "nearest-generated-PPC-callsite-sample:" + source;
            correlation.status = status + ":" + std::string(kSonicHudNodeWriteCandidateSetLabel);
            correlation.frameDelta =
                static_cast<int32_t>(g_presentedFrameCount) -
                static_cast<int32_t>(sample.frame);
            return correlation;
        }

        return correlation;
    }

    static std::string BuildSonicHudUpdateCallsiteStableSignature(
        const SonicHudUpdateCallsiteSample& sample)
    {
        std::ostringstream out;
        out
            << sample.hookName
            << "|" << sample.samplePhase
            << "|owner=" << HexU32(sample.ownerAddress)
            << "|f424=" << HexU32(sample.ownerField424)
            << "|f432=" << HexU32(sample.ownerField432)
            << "|f452=" << sample.ownerField452
            << "|f456=" << sample.ownerField456
            << "|f460=" << sample.ownerField460
            << "|f464=" << sample.ownerField464
            << "|f468=" << sample.ownerField468
            << "|f472=" << sample.ownerField472
            << "|f480=" << sample.ownerField480
            << "|f484=" << HexU32(sample.ownerField484)
            << "|f488=" << HexU32(sample.ownerField488);
        return out.str();
    }

    static bool ApplySonicHudUpdateCallsiteSampleToGameplayValues(
        const SonicHudUpdateCallsiteSample& sample,
        bool writeEvidence)
    {
        std::string valueName;
        std::string status;
        std::string source;
        uint32_t normalizedValue = 0;
        bool normalizedValueKnown = false;

        if (!ClassifySonicHudUpdateCallsiteSample(
                sample,
                valueName,
                status,
                source,
                normalizedValue,
                normalizedValueKnown))
        {
            return false;
        }

        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            g_lastSonicHudClassifiedCallsiteValue.lastClassificationKnown = true;
            g_lastSonicHudClassifiedCallsiteValue.valueName = valueName;
            g_lastSonicHudClassifiedCallsiteValue.status = status;
            g_lastSonicHudClassifiedCallsiteValue.lastClassifiedCallsiteValueSource = source;
            g_lastSonicHudClassifiedCallsiteValue.hookName = sample.hookName;
            g_lastSonicHudClassifiedCallsiteValue.samplePhase = sample.samplePhase;
            g_lastSonicHudClassifiedCallsiteValue.ownerAddress = sample.ownerAddress;
            g_lastSonicHudClassifiedCallsiteValue.normalizedValueKnown = normalizedValueKnown;
            g_lastSonicHudClassifiedCallsiteValue.normalizedValue = normalizedValue;
            g_lastSonicHudClassifiedCallsiteValue.lastClassifiedCallsiteValueFrame = g_presentedFrameCount;
        }

        if (valueName == "elapsedFrames" && normalizedValueKnown)
        {
            SonicHudGameplayValueSnapshot snapshot;
            {
                std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
                snapshot = g_sonicHudGameplayValues;
                changed =
                    !snapshot.elapsedFramesKnown ||
                    snapshot.elapsedFrames != normalizedValue ||
                    snapshot.elapsedFramesSource != source;
                snapshot.elapsedFramesKnown = true;
                snapshot.elapsedFrames = normalizedValue;
                snapshot.elapsedFramesSource = source;
                snapshot.frame = g_presentedFrameCount;

                if (changed)
                    g_sonicHudGameplayValues = std::move(snapshot);
            }
        }

        std::ostringstream detail;
        detail
            << "value=" << valueName
            << " status=" << status
            << " source=" << source
            << " hook=" << sample.hookName
            << " samplePhase=" << sample.samplePhase
            << " owner=" << HexU32(sample.ownerAddress);
        if (normalizedValueKnown)
            detail << " normalizedValue=" << normalizedValue;

        if (writeEvidence)
            WriteEvidenceEvent("sonic-hud-callsite-value-classified", detail.str());
        return changed;
    }

    static bool ApplySonicHudSpeedReadoutValueToGameplayValues(
        uint32_t ownerAddress,
        uint32_t speedKmh,
        std::string_view hookSource)
    {
        const std::string source = hookSource.empty()
            ? std::string("generated-PPC:sub_824D6418 -> sub_8251A568 return")
            : std::string(hookSource);
        bool changed = false;

        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            SonicHudGameplayValueSnapshot snapshot = g_sonicHudGameplayValues;
            changed =
                !snapshot.speedKmhKnown ||
                snapshot.speedKmh != static_cast<float>(speedKmh) ||
                snapshot.speedKmhSource != source;
            snapshot.speedKmhKnown = true;
            snapshot.speedKmh = static_cast<float>(speedKmh);
            snapshot.speedKmhSource = source;
            snapshot.frame = g_presentedFrameCount;
            g_sonicHudGameplayValues = std::move(snapshot);

            g_lastSonicHudClassifiedCallsiteValue.lastClassificationKnown = true;
            g_lastSonicHudClassifiedCallsiteValue.valueName = "speedKmh";
            g_lastSonicHudClassifiedCallsiteValue.status =
                "runtime-proven-via-sub_8251A568-return";
            g_lastSonicHudClassifiedCallsiteValue.lastClassifiedCallsiteValueSource = source;
            g_lastSonicHudClassifiedCallsiteValue.hookName = "sub_8251A568";
            g_lastSonicHudClassifiedCallsiteValue.samplePhase = "return";
            g_lastSonicHudClassifiedCallsiteValue.ownerAddress = ownerAddress;
            g_lastSonicHudClassifiedCallsiteValue.normalizedValueKnown = true;
            g_lastSonicHudClassifiedCallsiteValue.normalizedValue = speedKmh;
            g_lastSonicHudClassifiedCallsiteValue.lastClassifiedCallsiteValueFrame =
                g_presentedFrameCount;
        }

        return changed;
    }

    static bool RecordSonicHudNodeWriteObservation(
        std::string_view writeKind,
        std::string_view valueName,
        std::string_view path,
        uint32_t nodeAddress,
        uint32_t textAddress,
        std::string_view textUtf8,
        bool numericValueKnown,
        double numericValue,
        std::string_view hookSource,
        std::string_view logValue,
        bool pathResolved,
        std::string_view pathResolutionSource,
        const SonicHudNodeWriteCallsiteCorrelation* callsiteCorrelation = nullptr,
        std::string_view semanticPathCandidate = {},
        std::string_view semanticValueName = {})
    {
        std::string logKey =
            std::string(writeKind) + "|" + std::string(path) + "|" + std::string(logValue) +
            "|" + (pathResolved ? std::string("resolved") : HexU32(nodeAddress)) +
            "|" + std::to_string(g_routeGeneration);
        if (callsiteCorrelation != nullptr && callsiteCorrelation->known)
            logKey += "|callsite=" + callsiteCorrelation->valueCandidate;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);

        SonicHudValueWriteObservation observation;
        observation.writeKind = std::string(writeKind);
        observation.valueName = std::string(valueName);
        observation.path = std::string(path);
        observation.pathResolved = pathResolved;
        observation.pathResolutionSource = std::string(pathResolutionSource);
        observation.nodeAddress = nodeAddress;
        observation.textAddress = textAddress;
        observation.textUtf8 = std::string(textUtf8);
        observation.numericValueKnown = numericValueKnown;
        observation.numericValue = numericValue;
        observation.hookSource = std::string(hookSource);
        if (callsiteCorrelation != nullptr)
        {
            observation.callsiteCorrelationKnown = callsiteCorrelation->known;
            observation.callsiteValueCandidate = callsiteCorrelation->valueCandidate;
            observation.callsiteCorrelationSource = callsiteCorrelation->source;
            observation.callsiteCorrelationStatus = callsiteCorrelation->status;
            observation.callsiteCorrelationFrameDelta = callsiteCorrelation->frameDelta;
        }
        observation.semanticPathCandidate = std::string(semanticPathCandidate);
        observation.semanticValueName = std::string(semanticValueName);
        observation.frame = g_presentedFrameCount;

        auto existing = std::find_if(
            g_sonicHudValueWriteObservations.begin(),
            g_sonicHudValueWriteObservations.end(),
            [&](const SonicHudValueWriteObservation& current)
            {
                if (current.path != observation.path || current.writeKind != observation.writeKind)
                    return false;

                if (observation.pathResolved)
                    return true;

                return current.nodeAddress == observation.nodeAddress;
            });

        if (existing != g_sonicHudValueWriteObservations.end())
        {
            *existing = std::move(observation);
        }
        else if (g_sonicHudValueWriteObservations.size() < kSonicHudValueWriteObservationLimit)
        {
            g_sonicHudValueWriteObservations.push_back(std::move(observation));
        }

        return g_loggedSonicHudValueTextWriteKeys.insert(logKey).second;
    }

    static bool RecordUnresolvedSonicHudNodeWrite(
        std::string_view writeKind,
        uint32_t nodeAddress,
        uint32_t textAddress,
        std::string_view valueText,
        bool numericValueKnown,
        double numericValue,
        std::string_view hookSource)
    {
        if (
            !IsPlausibleGuestPointer(nodeAddress) ||
            !IsLikelySonicHudUnresolvedValue(writeKind, valueText) ||
            !HasRecentUiPlayScreenDrawActivity())
        {
            return false;
        }

        const std::string unresolvedKind = std::string(writeKind) + "-unresolved";
        const SonicHudNodeWriteCallsiteCorrelation callsiteCorrelation =
            CorrelateUnresolvedSonicHudNodeWriteWithCallsite(writeKind, nodeAddress);
        const auto semanticCandidates = callsiteCorrelation.known
            ? ResolveSonicHudSemanticPathCandidateFromCallsiteCorrelation(
                writeKind,
                callsiteCorrelation.valueCandidate)
            : std::vector<SonicHudSemanticPathCandidate>{};
        const std::string semanticPathCandidate =
            JoinSonicHudSemanticPathCandidates(semanticCandidates);
        const std::string semanticValueName =
            JoinSonicHudSemanticValueNames(semanticCandidates);
        const bool shouldLog = RecordSonicHudNodeWriteObservation(
            unresolvedKind,
            "unknown",
            "unresolved",
            nodeAddress,
            textAddress,
            valueText,
            numericValueKnown,
            numericValue,
            hookSource,
            valueText,
            false,
            "pending-ui-draw-list-late-resolve",
            &callsiteCorrelation,
            semanticPathCandidate,
            semanticValueName);

        if (shouldLog)
        {
            const std::string candidateDetail = callsiteCorrelation.known
                ? " callsiteCandidate=" + callsiteCorrelation.valueCandidate +
                    " callsiteSource=" + callsiteCorrelation.source +
                    " callsiteStatus=" + callsiteCorrelation.status +
                    " callsiteFrameDelta=" + std::to_string(callsiteCorrelation.frameDelta) +
                    (semanticPathCandidate.empty()
                        ? std::string()
                        : " semanticPathCandidate=" + semanticPathCandidate +
                            " semanticValueName=" + semanticValueName +
                            " pathResolutionSource=generated-PPC-callsite-semantic-candidate")
                : std::string();

            WriteEvidenceEvent(
                "sonic-hud-node-write-unresolved",
                "kind=" + std::string(writeKind) +
                " node=" + HexU32(nodeAddress) +
                " value=\"" + std::string(valueText) + "\"" +
                " source=" + std::string(hookSource) +
                " reason=ui_playscreen-active-path-unresolved" +
                candidateDetail);
            if (callsiteCorrelation.known)
            {
                WriteEvidenceEvent(
                    "sonic-hud-node-write-callsite-correlated",
                    "kind=" + std::string(writeKind) +
                    " node=" + HexU32(nodeAddress) +
                    " valueCandidate=" + callsiteCorrelation.valueCandidate +
                    " source=" + callsiteCorrelation.source +
                    " status=" + callsiteCorrelation.status +
                    " frameDelta=" + std::to_string(callsiteCorrelation.frameDelta));

                for (const auto& candidate : semanticCandidates)
                {
                    const bool semanticBound = ApplySonicHudSemanticPathCandidateToGameplayValues(
                        candidate,
                        writeKind,
                        valueText,
                        numericValueKnown,
                        numericValue,
                        hookSource,
                        callsiteCorrelation);

                    WriteEvidenceEvent(
                        "sonic-hud-node-write-semantic-path-candidate",
                        "kind=" + std::string(writeKind) +
                        " node=" + HexU32(nodeAddress) +
                        " value=\"" + std::string(valueText) + "\"" +
                        " valueCandidate=" + callsiteCorrelation.valueCandidate +
                        " semanticValueName=" + candidate.valueName +
                        " semanticPathCandidate=" + candidate.path +
                        " source=" + callsiteCorrelation.source +
                        " status=" + callsiteCorrelation.status +
                        " frameDelta=" + std::to_string(callsiteCorrelation.frameDelta) +
                        " pathResolutionSource=generated-PPC-callsite-semantic-candidate" +
                        " pathResolved=false");

                    if (semanticBound)
                    {
                        WriteEvidenceEvent(
                            "sonic-hud-node-write-semantic-bound",
                            "kind=" + std::string(writeKind) +
                            " node=" + HexU32(nodeAddress) +
                            " value=\"" + std::string(valueText) + "\"" +
                            " semanticValueName=" + candidate.valueName +
                            " semanticPathCandidate=" + candidate.path +
                            " source=" + callsiteCorrelation.source +
                            " status=" + callsiteCorrelation.status +
                            " pathResolutionSource=generated-PPC-callsite-semantic-candidate" +
                            " pathResolved=false" +
                            " semanticBindingStatus=stable-candidate-bound-pending-exact-child-node-resolution");
                    }
                }
            }
            WriteLiveStateSnapshot();
        }

        return shouldLog;
    }

    static std::vector<LateResolvedSonicHudNodeWrite> TryLateResolveSonicHudNodeWriteObservations(
        const RuntimeUiDrawCall& call)
    {
        std::vector<LateResolvedSonicHudNodeWrite> resolved;

        if (
            call.layerPath.empty() ||
            call.layerPath == "unresolved" ||
            (call.projectName != "ui_playscreen" && !call.layerPath.starts_with("ui_playscreen/")))
        {
            return resolved;
        }

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);

        for (auto& observation : g_sonicHudValueWriteObservations)
        {
            if (observation.pathResolved)
                continue;

            const std::string baseKind = BaseSonicHudWriteKind(observation.writeKind);
            const bool wantsTextPath = baseKind == "text";
            bool (*pathPredicate)(std::string_view) = wantsTextPath
                ? IsSonicHudValueTextPath
                : IsSonicHudGaugeOrPromptPath;

            std::string resolvedPath = ResolveSonicHudPathFromNodeSourceOwnerLocked(
                observation.nodeAddress,
                pathPredicate);
            std::string resolutionSource = "csd-child-lookup-chain";

            if (resolvedPath.empty())
            {
                if (
                    observation.nodeAddress != call.layerAddress &&
                    observation.nodeAddress != call.castNodeAddress)
                {
                    continue;
                }

                if (!pathPredicate(call.layerPath))
                    continue;

                resolvedPath = call.layerPath;
                resolutionSource = "ui-draw-list-late-resolve";
            }

            observation.writeKind = baseKind;
            observation.path = resolvedPath;
            observation.pathResolved = true;
            observation.pathResolutionSource = resolutionSource;
            observation.valueName = wantsTextPath
                ? SonicHudValueNameFromTextPath(resolvedPath)
                : SonicHudValueNameFromGaugeOrPromptPath(resolvedPath);
            observation.frame = g_presentedFrameCount;

            LateResolvedSonicHudNodeWrite event;
            event.writeKind = observation.writeKind;
            event.valueName = observation.valueName;
            event.path = observation.path;
            event.nodeAddress = observation.nodeAddress;
            event.textAddress = observation.textAddress;
            event.valueText = observation.textUtf8;
            event.numericValueKnown = observation.numericValueKnown;
            event.numericValue = observation.numericValue;
            event.hookSource = observation.hookSource;
            event.pathResolutionSource = observation.pathResolutionSource;
            resolved.push_back(std::move(event));
        }

        return resolved;
    }

    static void EmitLateResolvedSonicHudNodeWriteEvents(
        const std::vector<LateResolvedSonicHudNodeWrite>& resolvedWrites)
    {
        bool wroteSnapshot = false;

        for (const auto& resolved : resolvedWrites)
        {
            WriteEvidenceEvent(
                "sonic-hud-node-write-late-resolved",
                "kind=" + resolved.writeKind +
                " value=" + resolved.valueName +
                " path=" + resolved.path +
                " node=" + HexU32(resolved.nodeAddress) +
                " rawValue=\"" + resolved.valueText + "\"" +
                " source=" + resolved.hookSource +
                " pathResolutionSource=" + resolved.pathResolutionSource);

            if (resolved.writeKind == "text")
            {
                ApplySonicHudTextWriteToGameplayValues(
                    resolved.path,
                    resolved.valueText,
                    resolved.hookSource);
            }
            else
            {
                ApplySonicHudGaugeOrPromptWriteToGameplayValues(
                    resolved.path,
                    resolved.writeKind,
                    resolved.numericValueKnown,
                    resolved.numericValue,
                    resolved.hookSource);
            }

            wroteSnapshot = true;
        }

        if (wroteSnapshot)
            WriteLiveStateSnapshot();
    }

    static void AppendCsdTreeEntries(
        std::ostringstream& out,
        const std::vector<CsdTreeEntry>& entries,
        std::string_view addressFieldName,
        std::string_view relatedAddressFieldName,
        std::string_view firstMetricName,
        std::string_view secondMetricName)
    {
        out << "[";
        for (size_t i = 0; i < entries.size(); ++i)
        {
            const auto& entry = entries[i];
            if (i != 0)
                out << ",";

            out
                << "{"
                << "\"path\":\"" << JsonEscape(entry.path) << "\","
                << "\"" << addressFieldName << "\":\"" << JsonEscape(HexU32(entry.address)) << "\","
                << "\"" << relatedAddressFieldName << "\":\"" << JsonEscape(HexU32(entry.relatedAddress)) << "\","
                << "\"" << firstMetricName << "\":" << entry.firstMetric << ","
                << "\"" << secondMetricName << "\":" << entry.secondMetric << ","
                << "\"frame\":" << entry.frame
                << "}";
        }
        out << "]";
    }

    static void AppendSonicHudOwnerFieldSamples(std::ostringstream& out, const std::vector<SonicHudOwnerFieldSample>& samples)
    {
        out << "[";
        for (size_t i = 0; i < samples.size(); ++i)
        {
            if (i != 0)
                out << ",";

            const auto& sample = samples[i];
            out
                << "{"
                << "\"field\":\"" << JsonEscape(sample.field) << "\","
                << "\"sampleOffset\":\"" << JsonEscape(HexU32(sample.sampleOffset)) << "\","
                << "\"slotValue\":\"" << JsonEscape(HexU32(sample.slotValue)) << "\","
                << "\"rcObjectAddress\":\"" << JsonEscape(HexU32(sample.rcObjectAddress)) << "\","
                << "\"resolvedMemoryAddress\":\"" << JsonEscape(HexU32(sample.resolvedMemoryAddress)) << "\","
                << "\"rcObjectKnown\":" << (sample.rcObjectKnown ? "true" : "false") << ","
                << "\"resolvedMemoryKnown\":" << (sample.resolvedMemoryKnown ? "true" : "false") << ","
                << "\"frame\":" << sample.frame << ","
                << "\"hookSource\":\"" << JsonEscape(sample.hookSource) << "\""
                << "}";
        }
        out << "]";
    }

    static void AppendSonicHudValueWriteObservations(
        std::ostringstream& out,
        const std::vector<SonicHudValueWriteObservation>& observations)
    {
        out << "[";
        for (size_t i = 0; i < observations.size(); ++i)
        {
            if (i != 0)
                out << ",";

            const auto& observation = observations[i];
            out
                << "{"
                << "\"writeKind\":\"" << JsonEscape(observation.writeKind) << "\","
                << "\"valueName\":\"" << JsonEscape(observation.valueName) << "\","
                << "\"path\":\"" << JsonEscape(observation.path) << "\","
                << "\"pathResolved\":" << (observation.pathResolved ? "true" : "false") << ","
                << "\"pathResolutionSource\":\"" << JsonEscape(observation.pathResolutionSource) << "\","
                << "\"nodeAddress\":\"" << JsonEscape(HexU32(observation.nodeAddress)) << "\","
                << "\"textAddress\":\"" << JsonEscape(HexU32(observation.textAddress)) << "\","
                << "\"textUtf8\":\"" << JsonEscape(observation.textUtf8) << "\","
                << "\"numericValueKnown\":" << (observation.numericValueKnown ? "true" : "false") << ","
                << "\"numericValue\":" << observation.numericValue << ","
                << "\"hookSource\":\"" << JsonEscape(observation.hookSource) << "\","
                << "\"callsiteCorrelationKnown\":"
                    << (observation.callsiteCorrelationKnown ? "true" : "false") << ","
                << "\"callsiteValueCandidate\":\""
                    << JsonEscape(observation.callsiteValueCandidate) << "\","
                << "\"callsiteCorrelationSource\":\""
                    << JsonEscape(observation.callsiteCorrelationSource) << "\","
                << "\"callsiteCorrelationStatus\":\""
                    << JsonEscape(observation.callsiteCorrelationStatus) << "\","
                << "\"callsiteCorrelationFrameDelta\":"
                    << observation.callsiteCorrelationFrameDelta << ","
                << "\"semanticPathCandidate\":\""
                    << JsonEscape(observation.semanticPathCandidate) << "\","
                << "\"semanticValueName\":\""
                    << JsonEscape(observation.semanticValueName) << "\","
                << "\"frame\":" << observation.frame
                << "}";
        }
        out << "]";
    }

    static void AppendSonicHudUpdateCallsiteSamples(
        std::ostringstream& out,
        const std::vector<SonicHudUpdateCallsiteSample>& samples)
    {
        out << "[";
        for (size_t i = 0; i < samples.size(); ++i)
        {
            if (i != 0)
                out << ",";

            const auto& sample = samples[i];
            out
                << "{"
                << "\"ownerAddress\":\"" << JsonEscape(HexU32(sample.ownerAddress)) << "\","
                << "\"hookName\":\"" << JsonEscape(sample.hookName) << "\","
                << "\"samplePhase\":\"" << JsonEscape(sample.samplePhase) << "\","
                << "\"deltaTime\":" << sample.deltaTime << ","
                << "\"r4\":\"" << JsonEscape(HexU32(sample.r4)) << "\","
                << "\"ownerField424\":\"" << JsonEscape(HexU32(sample.ownerField424)) << "\","
                << "\"ownerField432\":\"" << JsonEscape(HexU32(sample.ownerField432)) << "\","
                << "\"ownerField440\":" << sample.ownerField440 << ","
                << "\"ownerField444\":" << sample.ownerField444 << ","
                << "\"ownerField452\":" << sample.ownerField452 << ","
                << "\"ownerField456\":" << sample.ownerField456 << ","
                << "\"ownerField460\":" << sample.ownerField460 << ","
                << "\"ownerField464\":" << sample.ownerField464 << ","
                << "\"ownerField468\":" << sample.ownerField468 << ","
                << "\"ownerField472\":" << sample.ownerField472 << ","
                << "\"ownerField476\":" << sample.ownerField476 << ","
                << "\"ownerField480\":" << sample.ownerField480 << ","
                << "\"ownerField484\":\"" << JsonEscape(HexU32(sample.ownerField484)) << "\","
                << "\"ownerField488\":\"" << JsonEscape(HexU32(sample.ownerField488)) << "\","
                << "\"frame\":" << sample.frame
                << "}";
        }
        out << "]";
    }

    static void AppendStringVector(std::ostringstream& out, const std::vector<std::string>& values)
    {
        out << "[";
        for (size_t i = 0; i < values.size(); ++i)
        {
            if (i != 0)
                out << ",";

            out << "\"" << JsonEscape(values[i]) << "\"";
        }
        out << "]";
    }

    static void AppendTypedInspectors(std::ostringstream& out)
    {
        const auto& target = TargetFor(g_target);
        const auto csd = BuildCsdLiveInspectorSnapshot();
        const auto csdProjectTree = BuildCsdProjectTreeInspectorSnapshot();
        const auto loading = BuildLoadingLiveInspectorSnapshot();
        const auto sonicHud = BuildSonicHudLiveInspectorSnapshot();
        const auto sonicGameplay = BuildSonicHudGameplayValueSnapshot();
        const auto sonicValueWriteObservations = BuildSonicHudValueWriteObservations();
        const auto sonicUpdateCallsiteSamples = BuildSonicHudUpdateCallsiteSamples();
        const auto lastClassifiedCallsiteValue = BuildSonicHudLastClassifiedCallsiteValue();
        const auto sonicOwnerPath = BuildSonicHudOwnerPathInspectorSnapshot(csdProjectTree);
        const auto pauseGeneralSave = BuildPauseGeneralSaveLiveInspectorSnapshot();
        const auto hudRenderGateCorrelation = BuildHudRenderGateCorrelationSnapshot();

        out
            << "{\n"
            << "    \"csd\": {\n"
            << "      \"source\": \"" << JsonEscape(csd.source) << "\",\n"
            << "      \"projectName\": \"" << JsonEscape(csd.projectName) << "\",\n"
            << "      \"targetProject\": \"" << JsonEscape(target.primaryCsdScene) << "\",\n"
            << "      \"targetObserved\": " << (g_targetCsdObserved ? "true" : "false") << ",\n"
            << "      \"sceneKnown\": " << (csd.sceneKnown ? "true" : "false") << ",\n"
            << "      \"sceneAddress\": \"" << JsonEscape(HexU32(csd.sceneAddress)) << "\",\n"
            << "      \"sceneMotionKnown\": " << (csd.sceneMotionKnown ? "true" : "false") << ",\n"
            << "      \"sceneMotionFrame\": ";

        if (csd.sceneMotionKnown)
            out << csd.sceneMotionFrame;
        else
            out << "null";

        out
            << ",\n"
            << "      \"sceneMotionRepeatType\": "
            << (csd.sceneMotionRepeatType == UINT32_MAX ? -1 : static_cast<int32_t>(csd.sceneMotionRepeatType))
            << ",\n"
            << "      \"sceneMotionRepeatTypeLabel\": \""
            << JsonEscape(MotionRepeatTypeLabel(csd.sceneMotionRepeatType)) << "\"\n"
            << "    },\n"
            << "    \"csdProjectTree\": {\n"
            << "      \"source\": \"" << JsonEscape(csdProjectTree.source) << "\",\n"
            << "      \"activeProject\": \"" << JsonEscape(csdProjectTree.activeProject) << "\",\n"
            << "      \"projectKnown\": " << (csdProjectTree.projectKnown ? "true" : "false") << ",\n"
            << "      \"projectAddress\": \"" << JsonEscape(HexU32(csdProjectTree.projectAddress)) << "\",\n"
            << "      \"rootNodeAddress\": \"" << JsonEscape(HexU32(csdProjectTree.rootNodeAddress)) << "\",\n"
            << "      \"observedProjectCount\": " << csdProjectTree.observedProjectCount << ",\n"
            << "      \"observedProjects\": ";
        AppendStringVector(out, csdProjectTree.observedProjects);
        out
            << ",\n"
            << "      \"sceneCount\": " << csdProjectTree.sceneCount << ",\n"
            << "      \"nodeCount\": " << csdProjectTree.nodeCount << ",\n"
            << "      \"layerCount\": " << csdProjectTree.layerCount << ",\n"
            << "      \"runtimeSceneMotionFrame\": ";

        if (csd.sceneMotionKnown)
            out << csd.sceneMotionFrame;
        else
            out << "null";

        out
            << ",\n"
            << "      \"runtimeSceneMotionRepeatTypeLabel\": \""
            << JsonEscape(MotionRepeatTypeLabel(csd.sceneMotionRepeatType)) << "\",\n"
            << "      \"scenes\": ";
        AppendCsdTreeEntries(out, csdProjectTree.scenes, "sceneAddress", "projectAddress", "castNodeCount", "castCount");
        out
            << ",\n"
            << "      \"nodes\": ";
        AppendCsdTreeEntries(out, csdProjectTree.nodes, "nodeAddress", "projectAddress", "sceneCount", "childNodeCount");
        out
            << ",\n"
            << "      \"layers\": ";
        AppendCsdTreeEntries(out, csdProjectTree.layers, "layerAddress", "castNodeAddress", "castNodeIndex", "castIndex");
        out
            << "\n"
            << "    },\n"
            << "    \"titleMenu\": {\n"
            << "      \"titleMenuOwnerContextAddress\": \""
            << JsonEscape(HexU32(g_titleOwnerInspector.titleContextAddress)) << "\",\n"
            << "      \"titleMenuOwnerCsdAddress\": \""
            << JsonEscape(HexU32(g_titleOwnerInspector.titleCsdAddress)) << "\",\n"
            << "      \"titleMenuOwnerReady\": " << (g_titleOwnerInspector.ownerReady ? "true" : "false") << ",\n"
            << "      \"titleMenuCursor\": " << g_titleMenuInspector.menuCursor << ",\n"
            << "      \"titleMenuContextPhase\": " << g_titleMenuInspector.contextPhase << ",\n"
            << "      \"titleMenuVisualReady\": " << (g_titleMenuVisualReady ? "true" : "false") << ",\n"
            << "      \"titleMenuPostPressStartReady\": "
            << (g_titleMenuInspector.postPressStartMenuReady ? "true" : "false") << "\n"
            << "    },\n"
            << "    \"loading\": {\n"
            << "      \"loadingRequestType\": "
            << (loading.requestType == UINT32_MAX ? -1 : static_cast<int32_t>(loading.requestType)) << ",\n"
            << "      \"loadingRequestTypeLabel\": \""
            << JsonEscape(LoadingDisplayTypeLabel(loading.requestType)) << "\",\n"
            << "      \"loadingDisplayType\": "
            << (loading.displayType == UINT32_MAX ? -1 : static_cast<int32_t>(loading.displayType)) << ",\n"
            << "      \"loadingDisplayTypeLabel\": \""
            << JsonEscape(LoadingDisplayTypeLabel(loading.displayType)) << "\",\n"
            << "      \"loadingDisplayActive\": " << (loading.displayActive ? "true" : "false") << ",\n"
            << "      \"requestFrame\": " << loading.requestFrame << ",\n"
            << "      \"displayFrame\": " << loading.displayFrame << "\n"
            << "    },\n"
            << "    \"pauseGeneralSave\": {\n"
            << "      \"pause\": {\n"
            << "        \"known\": " << (pauseGeneralSave.pauseKnown ? "true" : "false") << ",\n"
            << "        \"pauseAddress\": \"" << JsonEscape(HexU32(pauseGeneralSave.pauseAddress)) << "\",\n"
            << "        \"pauseProjectAddress\": \"" << JsonEscape(HexU32(pauseGeneralSave.pauseProjectAddress)) << "\",\n"
            << "        \"pauseBgSceneAddress\": \"" << JsonEscape(HexU32(pauseGeneralSave.pauseBgSceneAddress)) << "\",\n"
            << "        \"pauseAction\": " << (pauseGeneralSave.pauseAction == UINT32_MAX ? -1 : static_cast<int32_t>(pauseGeneralSave.pauseAction)) << ",\n"
            << "        \"pauseActionLabel\": \"" << JsonEscape(PauseActionTypeLabel(pauseGeneralSave.pauseAction)) << "\",\n"
            << "        \"pauseMenu\": " << (pauseGeneralSave.pauseMenu == UINT32_MAX ? -1 : static_cast<int32_t>(pauseGeneralSave.pauseMenu)) << ",\n"
            << "        \"pauseMenuLabel\": \"" << JsonEscape(PauseMenuTypeLabel(pauseGeneralSave.pauseMenu)) << "\",\n"
            << "        \"pauseStatus\": " << (pauseGeneralSave.pauseStatus == UINT32_MAX ? -1 : static_cast<int32_t>(pauseGeneralSave.pauseStatus)) << ",\n"
            << "        \"pauseStatusLabel\": \"" << JsonEscape(PauseStatusTypeLabel(pauseGeneralSave.pauseStatus)) << "\",\n"
            << "        \"pauseTransition\": " << (pauseGeneralSave.pauseTransition == UINT32_MAX ? -1 : static_cast<int32_t>(pauseGeneralSave.pauseTransition)) << ",\n"
            << "        \"pauseTransitionLabel\": \"" << JsonEscape(PauseTransitionTypeLabel(pauseGeneralSave.pauseTransition)) << "\",\n"
            << "        \"pauseVisible\": " << (pauseGeneralSave.pauseVisible ? "true" : "false") << ",\n"
            << "        \"pauseShown\": " << (pauseGeneralSave.pauseShown ? "true" : "false") << ",\n"
            << "        \"frame\": " << pauseGeneralSave.pauseFrame << "\n"
            << "      },\n"
            << "      \"generalWindow\": {\n"
            << "        \"known\": " << (pauseGeneralSave.generalWindowKnown ? "true" : "false") << ",\n"
            << "        \"generalWindowAddress\": \"" << JsonEscape(HexU32(pauseGeneralSave.generalWindowAddress)) << "\",\n"
            << "        \"generalProjectAddress\": \"" << JsonEscape(HexU32(pauseGeneralSave.generalProjectAddress)) << "\",\n"
            << "        \"generalBgSceneAddress\": \"" << JsonEscape(HexU32(pauseGeneralSave.generalBgSceneAddress)) << "\",\n"
            << "        \"generalWindowStatus\": " << (pauseGeneralSave.generalWindowStatus == UINT32_MAX ? -1 : static_cast<int32_t>(pauseGeneralSave.generalWindowStatus)) << ",\n"
            << "        \"generalWindowStatusLabel\": \"" << JsonEscape(GeneralWindowStatusLabel(pauseGeneralSave.generalWindowStatus)) << "\",\n"
            << "        \"cursorIndex\": " << (pauseGeneralSave.generalCursorIndex == UINT32_MAX ? -1 : static_cast<int32_t>(pauseGeneralSave.generalCursorIndex)) << ",\n"
            << "        \"selectedIndex\": " << (pauseGeneralSave.generalSelectedIndex == UINT32_MAX ? -1 : static_cast<int32_t>(pauseGeneralSave.generalSelectedIndex)) << ",\n"
            << "        \"frame\": " << pauseGeneralSave.generalFrame << "\n"
            << "      },\n"
            << "      \"saveIcon\": {\n"
            << "        \"known\": " << (pauseGeneralSave.saveIconKnown ? "true" : "false") << ",\n"
            << "        \"saveIconAddress\": \"" << JsonEscape(HexU32(pauseGeneralSave.saveIconAddress)) << "\",\n"
            << "        \"saveIconVisible\": " << (pauseGeneralSave.saveIconVisible ? "true" : "false") << ",\n"
            << "        \"frame\": " << pauseGeneralSave.saveIconFrame << "\n"
            << "      }\n"
            << "    },\n"
            << "    \"hudRenderGateCorrelation\": {\n"
            << "      \"renderHud\": " << (hudRenderGateCorrelation.renderHud ? "true" : "false") << ",\n"
            << "      \"renderGameMainHud\": " << (hudRenderGateCorrelation.renderGameMainHud ? "true" : "false") << ",\n"
            << "      \"renderHudPause\": " << (hudRenderGateCorrelation.renderHudPause ? "true" : "false") << ",\n"
            << "      \"gateStatus\": \"" << JsonEscape(hudRenderGateCorrelation.gateStatus) << "\",\n"
            << "      \"resolvedUiPlayScreenNodeWrites\": "
            << hudRenderGateCorrelation.resolvedUiPlayScreenNodeWrites << ",\n"
            << "      \"unresolvedUiPlayScreenNodeWrites\": "
            << hudRenderGateCorrelation.unresolvedUiPlayScreenNodeWrites << ",\n"
            << "      \"unresolvedWriteKinds\": \""
            << JsonEscape(hudRenderGateCorrelation.unresolvedWriteKinds) << "\",\n"
            << "      \"ms_IsRenderHudCallers\": ";
        AppendStringVector(out, hudRenderGateCorrelation.ms_IsRenderHudCallers);
        out
            << ",\n"
            << "      \"ms_IsRenderGameMainHudCallers\": ";
        AppendStringVector(out, hudRenderGateCorrelation.ms_IsRenderGameMainHudCallers);
        out
            << ",\n"
            << "      \"ms_IsRenderHudPauseCallers\": ";
        AppendStringVector(out, hudRenderGateCorrelation.ms_IsRenderHudPauseCallers);
        out
            << ",\n"
            << "      \"frame\": " << hudRenderGateCorrelation.frame << "\n"
            << "    },\n"
            << "    \"sonicHud\": {\n"
            << "      \"source\": \"" << JsonEscape(sonicHud.source) << "\",\n"
            << "      \"rawOwnerKnown\": " << (sonicHud.rawOwnerKnown ? "true" : "false") << ",\n"
            << "      \"rawOwnerFieldsReady\": " << (sonicHud.rawOwnerFieldsReady ? "true" : "false") << ",\n"
            << "      \"hudOwnerAddress\": \"" << JsonEscape(HexU32(sonicHud.hudOwnerAddress)) << "\",\n"
            << "      \"stageGameModeAddress\": \"" << JsonEscape(HexU32(sonicHud.stageGameModeAddress)) << "\",\n"
            << "      \"rawOwnerFrame\": " << sonicHud.rawOwnerFrame << ",\n"
            << "      \"rawHookSource\": \"" << JsonEscape(sonicHud.rawHookSource) << "\",\n"
            << "      \"playScreenProject\": \"" << JsonEscape(sonicHud.playScreenProject) << "\",\n"
            << "      \"speedGaugeScene\": \"" << JsonEscape(sonicHud.speedGaugeScene) << "\",\n"
            << "      \"stageContextObserved\": " << (sonicHud.stageContextObserved ? "true" : "false") << ",\n"
            << "      \"targetCsdObserved\": " << (sonicHud.targetCsdObserved ? "true" : "false") << ",\n"
            << "      \"stageTargetReady\": " << (sonicHud.stageTargetReady ? "true" : "false") << ",\n"
            << "      \"readyEvent\": \"" << JsonEscape(sonicHud.readyEvent) << "\",\n"
            << "      \"gameplayValues\": {\n"
            << "        \"source\": \"" << JsonEscape(sonicGameplay.source) << "\",\n"
            << "        \"ringCountKnown\": " << (sonicGameplay.ringCountKnown ? "true" : "false") << ",\n"
            << "        \"ringCount\": " << sonicGameplay.ringCount << ",\n"
            << "        \"ringCountSource\": \"" << JsonEscape(sonicGameplay.ringCountSource) << "\",\n"
            << "        \"scoreKnown\": " << (sonicGameplay.scoreKnown ? "true" : "false") << ",\n"
            << "        \"score\": " << sonicGameplay.score << ",\n"
            << "        \"scoreSource\": \"" << JsonEscape(sonicGameplay.scoreSource) << "\",\n"
            << "        \"scoreInfoPointMarkerRecordSpeedKnown\": " << (sonicGameplay.scoreInfoPointMarkerRecordSpeedKnown ? "true" : "false") << ",\n"
            << "        \"scoreInfoPointMarkerRecordSpeed\": " << sonicGameplay.scoreInfoPointMarkerRecordSpeed << ",\n"
            << "        \"scoreInfoPointMarkerRecordSpeedSource\": \"" << JsonEscape(sonicGameplay.scoreInfoPointMarkerRecordSpeedSource) << "\",\n"
            << "        \"scoreInfoPointMarkerCountKnown\": " << (sonicGameplay.scoreInfoPointMarkerCountKnown ? "true" : "false") << ",\n"
            << "        \"scoreInfoPointMarkerCount\": " << sonicGameplay.scoreInfoPointMarkerCount << ",\n"
            << "        \"scoreInfoPointMarkerCountSource\": \"" << JsonEscape(sonicGameplay.scoreInfoPointMarkerCountSource) << "\",\n"
            << "        \"elapsedFramesKnown\": " << (sonicGameplay.elapsedFramesKnown ? "true" : "false") << ",\n"
            << "        \"elapsedFrames\": " << sonicGameplay.elapsedFrames << ",\n"
            << "        \"elapsedFramesSource\": \"" << JsonEscape(sonicGameplay.elapsedFramesSource) << "\",\n"
            << "        \"speedKmhKnown\": " << (sonicGameplay.speedKmhKnown ? "true" : "false") << ",\n"
            << "        \"speedKmh\": " << sonicGameplay.speedKmh << ",\n"
            << "        \"speedKmhSource\": \"" << JsonEscape(sonicGameplay.speedKmhSource) << "\",\n"
            << "        \"boostGaugeKnown\": " << (sonicGameplay.boostGaugeKnown ? "true" : "false") << ",\n"
            << "        \"boostGauge\": " << sonicGameplay.boostGauge << ",\n"
            << "        \"boostGaugeSource\": \"" << JsonEscape(sonicGameplay.boostGaugeSource) << "\",\n"
            << "        \"ringEnergyGaugeKnown\": " << (sonicGameplay.ringEnergyGaugeKnown ? "true" : "false") << ",\n"
            << "        \"ringEnergyGauge\": " << sonicGameplay.ringEnergyGauge << ",\n"
            << "        \"ringEnergyGaugeSource\": \"" << JsonEscape(sonicGameplay.ringEnergyGaugeSource) << "\",\n"
            << "        \"lifeCountKnown\": " << (sonicGameplay.lifeCountKnown ? "true" : "false") << ",\n"
            << "        \"lifeCount\": " << sonicGameplay.lifeCount << ",\n"
            << "        \"lifeCountSource\": \"" << JsonEscape(sonicGameplay.lifeCountSource) << "\",\n"
            << "        \"tutorialPromptKnown\": " << (sonicGameplay.tutorialPromptKnown ? "true" : "false") << ",\n"
            << "        \"tutorialPromptId\": \"" << JsonEscape(sonicGameplay.tutorialPromptId) << "\",\n"
            << "        \"tutorialVisible\": " << (sonicGameplay.tutorialVisible ? "true" : "false") << ",\n"
            << "        \"tutorialPromptSource\": \"" << JsonEscape(sonicGameplay.tutorialPromptSource) << "\",\n"
            << "        \"audioIds\": {\n"
            << "          \"sonicRingPickup\": \"" << JsonEscape(sonicGameplay.sonicRingPickupSfxId) << "\",\n"
            << "          \"tutorialPromptOpen\": \"" << JsonEscape(sonicGameplay.tutorialPromptOpenSfxId) << "\",\n"
            << "          \"pauseOpen\": \"" << JsonEscape(sonicGameplay.pauseOpenSfxId) << "\",\n"
            << "          \"pauseCursor\": \"" << JsonEscape(sonicGameplay.pauseCursorSfxId) << "\"\n"
            << "        },\n"
            << "        \"valueWriteBindingStatus\": \"" << JsonEscape(SonicHudValueWriteBindingStatus()) << "\",\n"
            << "        \"valueWriteObservationCount\": " << sonicValueWriteObservations.size() << ",\n"
            << "        \"valueWriteObservations\": ";
        AppendSonicHudValueWriteObservations(out, sonicValueWriteObservations);
        out
            << ",\n"
            << "        \"updateCallsiteSampleCount\": " << sonicUpdateCallsiteSamples.size() << ",\n"
            << "        \"updateCallsiteSamples\": ";
        AppendSonicHudUpdateCallsiteSamples(out, sonicUpdateCallsiteSamples);
        out
            << ",\n"
            << "        \"lastClassifiedCallsiteValue\": {\n"
            << "          \"lastClassificationKnown\": " << (lastClassifiedCallsiteValue.lastClassificationKnown ? "true" : "false") << ",\n"
            << "          \"valueName\": \"" << JsonEscape(lastClassifiedCallsiteValue.valueName) << "\",\n"
            << "          \"status\": \"" << JsonEscape(lastClassifiedCallsiteValue.status) << "\",\n"
            << "          \"lastClassifiedCallsiteValueSource\": \""
            << JsonEscape(lastClassifiedCallsiteValue.lastClassifiedCallsiteValueSource) << "\",\n"
            << "          \"hookName\": \"" << JsonEscape(lastClassifiedCallsiteValue.hookName) << "\",\n"
            << "          \"samplePhase\": \"" << JsonEscape(lastClassifiedCallsiteValue.samplePhase) << "\",\n"
            << "          \"ownerAddress\": \"" << JsonEscape(HexU32(lastClassifiedCallsiteValue.ownerAddress)) << "\",\n"
            << "          \"normalizedValueKnown\": " << (lastClassifiedCallsiteValue.normalizedValueKnown ? "true" : "false") << ",\n"
            << "          \"normalizedValue\": " << lastClassifiedCallsiteValue.normalizedValue << ",\n"
            << "          \"lastClassifiedCallsiteValueFrame\": "
            << lastClassifiedCallsiteValue.lastClassifiedCallsiteValueFrame << "\n"
            << "        },\n"
            << "        \"frame\": " << sonicGameplay.frame << "\n"
            << "      },\n"
            << "      \"ownerPath\": {\n"
            << "        \"chudSonicStageOwnerAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.chudSonicStageOwnerAddress)) << "\",\n"
            << "        \"ownerPointerStatus\": \"" << JsonEscape(sonicOwnerPath.ownerPointerStatus) << "\",\n"
            << "        \"ownerFieldMaturationStatus\": \"" << JsonEscape(sonicOwnerPath.ownerFieldMaturationStatus) << "\",\n"
            << "        \"rawOwnerKnown\": " << (sonicOwnerPath.rawOwnerKnown ? "true" : "false") << ",\n"
            << "        \"rawOwnerFieldsReady\": " << (sonicOwnerPath.rawOwnerFieldsReady ? "true" : "false") << ",\n"
            << "        \"rawOwnerFrame\": " << sonicOwnerPath.rawOwnerFrame << ",\n"
            << "        \"rawOwnerFieldSampleCount\": " << sonicOwnerPath.rawOwnerFieldSampleCount << ",\n"
            << "        \"rawOwnerResolvedMemoryCount\": " << sonicOwnerPath.rawOwnerResolvedMemoryCount << ",\n"
            << "        \"rawHookSource\": \"" << JsonEscape(sonicOwnerPath.rawHookSource) << "\",\n"
            << "        \"stageGameModeAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.stageGameModeAddress)) << "\",\n"
            << "        \"rcPlayScreenProjectAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcPlayScreenProjectAddress)) << "\",\n"
            << "        \"rcSpeedGaugeSceneAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcSpeedGaugeSceneAddress)) << "\",\n"
            << "        \"rcRingEnergyGaugeSceneAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcRingEnergyGaugeSceneAddress)) << "\",\n"
            << "        \"rcGaugeFrameSceneAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcGaugeFrameSceneAddress)) << "\",\n"
            << "        \"rcRingCountSceneAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcRingCountSceneAddress)) << "\",\n"
            << "        \"rcScoreCountNodeAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcScoreCountNodeAddress)) << "\",\n"
            << "        \"rcTimeCountNodeAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcTimeCountNodeAddress)) << "\",\n"
            << "        \"rcTimeCount2NodeAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcTimeCount2NodeAddress)) << "\",\n"
            << "        \"rcTimeCount3NodeAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcTimeCount3NodeAddress)) << "\",\n"
            << "        \"rcPlayerCountNodeAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcPlayerCountNodeAddress)) << "\",\n"
            << "        \"rcTutorialInfoSceneAddress\": \"" << JsonEscape(HexU32(sonicOwnerPath.rcTutorialInfoSceneAddress)) << "\",\n"
            << "        \"resolvedFromCsdProjectTree\": " << (sonicOwnerPath.resolvedFromCsdProjectTree ? "true" : "false") << ",\n"
            << "        \"expectedOwnerFieldSource\": \"" << JsonEscape(sonicOwnerPath.expectedOwnerFieldSource) << "\",\n"
            << "        \"rawOwnerExpectedFieldOffsets\": \"" << JsonEscape(std::string(kChudSonicStageExpectedOwnerFieldSource)) << "\",\n"
            << "        \"displayOwnerPaths\": \"" << JsonEscape(sonicOwnerPath.displayOwnerPaths) << "\",\n"
            << "        \"gameplayNumericBindingStatus\": \"" << JsonEscape(sonicOwnerPath.gameplayNumericBindingStatus) << "\",\n"
            << "        \"rawOwnerFieldSamples\": ";
        AppendSonicHudOwnerFieldSamples(out, sonicOwnerPath.rawOwnerFieldSamples);
        out
            << "\n"
            << "      }\n"
            << "    }\n"
            << "  }";
    }

    static std::string UiOracleActivationEventName(ScreenId id)
    {
        switch (id)
        {
        case ScreenId::TitleMenu:
            return "title-menu-visible";
        case ScreenId::TitleOptions:
            return "title-options-ready";
        case ScreenId::Loading:
            return "loading-display-active";
        case ScreenId::SonicHud:
            return "sonic-hud-ready";
        case ScreenId::Pause:
            return "pause-ready";
        case ScreenId::Tutorial:
            return "tutorial-ready";
        case ScreenId::Result:
            return "result-ready";
        default:
            return "target-csd-project-made";
        }
    }

    static std::string UiOracleTransitionBand(ScreenId id)
    {
        switch (id)
        {
        case ScreenId::TitleMenu:
            return "select_travel->title menu visual ready";
        case ScreenId::TitleOptions:
            return "select_travel->title options visual ready";
        case ScreenId::Loading:
            return "pda_intro->loading display active";
        case ScreenId::Pause:
            return "intro_medium->pause menu visual ready";
        case ScreenId::SonicHud:
            return "hud_in->sonic-hud-ready";
        case ScreenId::Tutorial:
            return "guide_in->tutorial-ready";
        default:
            return "runtime route->target ready";
        }
    }

    static bool UiOracleTargetReady(ScreenId id)
    {
        switch (id)
        {
        case ScreenId::TitleMenu:
            return g_titleMenuVisualReady;
        case ScreenId::TitleOptions:
            return g_targetCsdObserved;
        case ScreenId::Loading:
            return g_loadingDisplayWasActive;
        case ScreenId::SonicHud:
        case ScreenId::Tutorial:
        case ScreenId::Pause:
        case ScreenId::Result:
            return !g_lastStageReadyEventName.empty();
        default:
            return g_targetCsdObserved;
        }
    }

    static std::string UiOracleCursorOwnerLabel(
        ScreenId id,
        const LoadingLiveInspectorSnapshot& loading,
        const SonicHudLiveInspectorSnapshot& sonicHud,
        const PauseGeneralSaveLiveInspectorSnapshot& pauseGeneralSave)
    {
        std::ostringstream out;
        switch (id)
        {
        case ScreenId::TitleMenu:
            out << "CTitleStateMenu/menu_cursor=" << g_titleMenuInspector.menuCursor;
            return out.str();
        case ScreenId::TitleOptions:
            out << "CTitleStateMenu/options/menu_cursor=" << g_titleMenuInspector.menuCursor;
            return out.str();
        case ScreenId::Loading:
            out << "LoadingDisplay/display_type="
                << (loading.displayType == UINT32_MAX ? -1 : static_cast<int32_t>(loading.displayType))
                << "/label=" << LoadingDisplayTypeLabel(loading.displayType);
            return out.str();
        case ScreenId::Pause:
            out << "CHudPause/menu=" << PauseMenuTypeLabel(pauseGeneralSave.pauseMenu)
                << "/status=" << PauseStatusTypeLabel(pauseGeneralSave.pauseStatus)
                << "/transition=" << PauseTransitionTypeLabel(pauseGeneralSave.pauseTransition)
                << "/visible=" << (pauseGeneralSave.pauseVisible ? 1 : 0);
            return out.str();
        case ScreenId::SonicHud:
        case ScreenId::Tutorial:
            out << "CHudSonicStage/hud_owner=" << HexU32(sonicHud.hudOwnerAddress)
                << "/ready=" << sonicHud.readyEvent;
            return out.str();
        default:
            return "runtime-target";
        }
    }

    static void AppendUiOracleActiveScenePaths(
        std::ostringstream& out,
        const CsdProjectTreeInspectorSnapshot& csdProjectTree)
    {
        out << "[";
        for (size_t i = 0; i < csdProjectTree.scenes.size(); ++i)
        {
            if (i != 0)
                out << ",";
            out << "\"" << JsonEscape(csdProjectTree.scenes[i].path) << "\"";
        }
        out << "]";
    }

    static std::string RuntimeUiDrawListStatus(uint32_t capturedDrawCalls)
    {
        return capturedDrawCalls > 0
            ? "runtime CSD platform draw hook; GPU backend submit pending"
            : "runtime CSD platform draw hook armed; waiting for draw calls";
    }

    static std::string RuntimeGpuSubmitStatus(uint32_t capturedSubmitCalls)
    {
        return capturedSubmitCalls > 0
            ? "render-thread material submit hook active; raw D3D12/Vulkan backend capture pending"
            : "render-thread material submit hook armed; waiting for submits";
    }

    static std::string RuntimeRawBackendCommandStatus(uint32_t capturedCommands)
    {
        return capturedCommands > 0
            ? "RHI command-list boundary captured; raw D3D12/Vulkan command capture pending"
            : "RHI command-list boundary hook armed; raw D3D12/Vulkan command capture pending";
    }

    static std::string RuntimeBackendResolvedStatus(uint32_t capturedSubmits)
    {
        return capturedSubmits > 0
            ? "backend-resolved command-list submit hook active"
            : "backend-resolved command-list submit hook armed; waiting for D3D12/Vulkan draws";
    }

    static std::string RenderBlendName(uint32_t value)
    {
        switch (value)
        {
            case 1: return "D3DBLEND_ZERO";
            case 2: return "D3DBLEND_ONE";
            case 3: return "D3DBLEND_SRCCOLOR";
            case 4: return "D3DBLEND_INVSRCCOLOR";
            case 5: return "D3DBLEND_SRCALPHA";
            case 6: return "D3DBLEND_INVSRCALPHA";
            case 7: return "D3DBLEND_DESTALPHA";
            case 8: return "D3DBLEND_INVDESTALPHA";
            case 9: return "D3DBLEND_DESTCOLOR";
            case 10: return "D3DBLEND_INVDESTCOLOR";
            case 11: return "D3DBLEND_SRCALPHASAT";
            case 12: return "D3DBLEND_BLENDFACTOR";
            case 13: return "D3DBLEND_INVBLENDFACTOR";
            case 14: return "D3DBLEND_SRC1COLOR";
            case 15: return "D3DBLEND_INVSRC1COLOR";
            case 16: return "D3DBLEND_SRC1ALPHA";
            case 17: return "D3DBLEND_INVSRC1ALPHA";
            default: return "D3DBLEND_UNKNOWN";
        }
    }

    static std::string RenderBlendOperationName(uint32_t value)
    {
        switch (value)
        {
            case 1: return "D3DBLENDOP_ADD";
            case 2: return "D3DBLENDOP_SUBTRACT";
            case 3: return "D3DBLENDOP_REVSUBTRACT";
            case 4: return "D3DBLENDOP_MIN";
            case 5: return "D3DBLENDOP_MAX";
            default: return "D3DBLENDOP_UNKNOWN";
        }
    }

    static std::string RenderTextureFilterName(uint32_t value)
    {
        switch (value)
        {
            case 1: return "D3DTEXF_POINT";
            case 2: return "D3DTEXF_LINEAR";
            default: return "D3DTEXF_UNKNOWN";
        }
    }

    static std::string RenderMipmapModeName(uint32_t value)
    {
        switch (value)
        {
            case 1: return "D3DTEXF_POINT";
            case 2: return "D3DTEXF_LINEAR";
            default: return "D3DTEXF_UNKNOWN";
        }
    }

    static std::string RenderTextureAddressName(uint32_t value)
    {
        switch (value)
        {
            case 1: return "D3DTADDRESS_WRAP";
            case 2: return "D3DTADDRESS_MIRROR";
            case 3: return "D3DTADDRESS_CLAMP";
            case 4: return "D3DTADDRESS_BORDER";
            case 5: return "D3DTADDRESS_MIRRORONCE";
            default: return "D3DTADDRESS_UNKNOWN";
        }
    }

    static std::string RuntimeTextureViewDimensionName(uint32_t value)
    {
        switch (value)
        {
            case 1: return "texture-1d";
            case 2: return "texture-2d";
            case 3: return "texture-3d";
            case 4: return "texture-cube";
            default: return "texture-unknown";
        }
    }

    static std::string RuntimeRenderFormatSemantic(uint32_t value)
    {
        std::ostringstream out;
        out << "RenderFormat#" << value;
        return out.str();
    }

    static std::string RuntimeTextureDescriptorSemanticName(const RuntimeTextureDescriptorSemantic& texture)
    {
        if (texture.descriptorIndex == 0 && texture.width == 0 && texture.height == 0)
            return "texture-descriptor-unknown";

        std::ostringstream out;
        out << RuntimeTextureViewDimensionName(texture.viewDimension)
            << "/" << RuntimeRenderFormatSemantic(texture.format)
            << "/" << texture.width << "x" << texture.height;
        return out.str();
    }

    static std::string RuntimeSamplerDescriptorSemanticName(const RuntimeSamplerDescriptorSemantic& sampler)
    {
        std::string family = "mixed";
        if (sampler.minFilter == 2 && sampler.magFilter == 2)
            family = "linear";
        else if (sampler.minFilter == 1 && sampler.magFilter == 1)
            family = "point";

        return family + "(" +
            RenderTextureFilterName(sampler.minFilter) + "," +
            RenderTextureFilterName(sampler.magFilter) + "," +
            RenderMipmapModeName(sampler.mipMode) + ")/" +
            RenderTextureAddressName(sampler.addressU) + "/" +
            RenderTextureAddressName(sampler.addressV) + "/" +
            RenderTextureAddressName(sampler.addressW);
    }

    static std::string RuntimeBlendSemantic(const RuntimeGpuSubmitCall& call)
    {
        if (!call.alphaBlendEnable)
            return "opaque/no-alpha-blend";

        if (call.srcBlend == 5 && call.destBlend == 6 && call.blendOp == 1)
            return "src-alpha/inv-src-alpha";

        if (call.srcBlend == 5 && call.destBlend == 2 && call.blendOp == 1)
            return "src-alpha/one additive";

        if (call.srcBlend == 2 && call.destBlend == 2 && call.blendOp == 1)
            return "one/one additive";

        return RenderBlendName(call.srcBlend) + "/" + RenderBlendName(call.destBlend);
    }

    static bool RuntimeBlendIsAdditive(const RuntimeGpuSubmitCall& call)
    {
        return call.alphaBlendEnable &&
            call.blendOp == 1 &&
            (call.destBlend == 2 || (call.srcBlend == 2 && call.destBlend == 2));
    }

    static std::string RuntimeBlendOperationSemantic(const RuntimeGpuSubmitCall& call)
    {
        return "rgb=" + RenderBlendOperationName(call.blendOp) +
            ";alpha=" + RenderBlendOperationName(call.blendOpAlpha);
    }

    static std::string RuntimeSamplerSemantic(const RuntimeGpuSubmitCall& call)
    {
        const std::string minFilter = RenderTextureFilterName(call.samplerMinFilter);
        const std::string magFilter = RenderTextureFilterName(call.samplerMagFilter);
        const std::string mipMode = RenderMipmapModeName(call.samplerMipMode);

        std::string family = "mixed";
        if (call.samplerMinFilter == 2 && call.samplerMagFilter == 2)
            family = "linear";
        else if (call.samplerMinFilter == 1 && call.samplerMagFilter == 1)
            family = "point";

        return family + "(" + minFilter + "," + magFilter + "," + mipMode + ")";
    }

    static std::string RuntimeAddressSemantic(const RuntimeGpuSubmitCall& call)
    {
        return RenderTextureAddressName(call.samplerAddressU) + "/" +
            RenderTextureAddressName(call.samplerAddressV) + "/" +
            RenderTextureAddressName(call.samplerAddressW);
    }

    static std::string RuntimeAlphaSemantic(const RuntimeGpuSubmitCall& call)
    {
        if (call.alphaTestEnable)
        {
            std::ostringstream out;
            out << "alpha-test-threshold=" << call.alphaThreshold;
            return out.str();
        }

        return call.alphaBlendEnable ? "straight-alpha-blend" : "opaque";
    }

    static std::string RuntimeColorWriteSemantic(const RuntimeGpuSubmitCall& call)
    {
        if (call.colorWriteEnable == 15)
            return "rgba";

        std::string channels;
        if ((call.colorWriteEnable & 1) != 0)
            channels += "r";
        if ((call.colorWriteEnable & 2) != 0)
            channels += "g";
        if ((call.colorWriteEnable & 4) != 0)
            channels += "b";
        if ((call.colorWriteEnable & 8) != 0)
            channels += "a";

        return channels.empty() ? "none" : channels;
    }

    static std::vector<RuntimeMaterialCorrelation> BuildRuntimeMaterialCorrelationPairs(
        const std::vector<RuntimeUiDrawCall>& drawCalls,
        const std::vector<RuntimeGpuSubmitCall>& submitCalls)
    {
        std::vector<RuntimeMaterialCorrelation> pairs;
        const size_t pairCount = std::min(drawCalls.size(), submitCalls.size());
        pairs.reserve(pairCount);

        for (size_t i = 0; i < pairCount; ++i)
        {
            const auto& draw = drawCalls[i];
            const auto& submit = submitCalls[i];

            RuntimeMaterialCorrelation pair;
            pair.frame = submit.frame != 0 ? submit.frame : draw.frame;
            pair.uiDrawSequence = draw.sequence;
            pair.gpuSubmitSequence = submit.sequence;
            pair.projectName = draw.projectName;
            pair.layerPath = draw.layerPath;
            pair.gpuSubmitSource = submit.source;
            pair.minX = draw.minX;
            pair.minY = draw.minY;
            pair.maxX = draw.maxX;
            pair.maxY = draw.maxY;
            pair.texture2DDescriptorIndex = submit.texture2DDescriptorIndex;
            pair.samplerDescriptorIndex = submit.samplerDescriptorIndex;
            pair.alphaBlendEnable = submit.alphaBlendEnable;
            pair.additiveBlend = RuntimeBlendIsAdditive(submit);
            pair.linearFilter = submit.samplerMinFilter == 2 || submit.samplerMagFilter == 2;
            pair.pointFilter = submit.samplerMinFilter == 1 && submit.samplerMagFilter == 1;
            pair.blendSemantic = RuntimeBlendSemantic(submit);
            pair.blendOperationSemantic = RuntimeBlendOperationSemantic(submit);
            pair.samplerSemantic = RuntimeSamplerSemantic(submit);
            pair.addressSemantic = RuntimeAddressSemantic(submit);
            pair.alphaSemantic = RuntimeAlphaSemantic(submit);
            pair.colorWriteSemantic = RuntimeColorWriteSemantic(submit);
            pair.halfPixelOffsetX = submit.halfPixelOffsetX;
            pair.halfPixelOffsetY = submit.halfPixelOffsetY;
            pairs.push_back(std::move(pair));
        }

        return pairs;
    }

    static void AppendRuntimeUiDrawCalls(std::ostringstream& out, const std::vector<RuntimeUiDrawCall>& calls)
    {
        out << "[";
        for (size_t i = 0; i < calls.size(); ++i)
        {
            if (i != 0)
                out << ",";

            const auto& call = calls[i];
            out
                << "{"
                << "\"sequence\":" << call.sequence
                << ",\"frame\":" << call.frame
                << ",\"project\":\"" << JsonEscape(call.projectName) << "\""
                << ",\"layerPath\":\"" << JsonEscape(call.layerPath) << "\""
                << ",\"layerAddress\":\"" << JsonEscape(HexU32(call.layerAddress)) << "\""
                << ",\"castNodeAddress\":\"" << JsonEscape(HexU32(call.castNodeAddress)) << "\""
                << ",\"vertexBufferAddress\":\"" << JsonEscape(HexU32(call.vertexBufferAddress)) << "\""
                << ",\"primitive\": \"quad\""
                << ",\"vertexCount\":" << call.vertexCount
                << ",\"vertexStride\":" << call.vertexStride
                << ",\"textured\":" << (call.textured ? "true" : "false")
                << ",\"colorSample\":\"" << JsonEscape(HexU32(call.colorSample)) << "\""
                << ",\"screenRect\":{"
                << "\"minX\":" << call.minX
                << ",\"minY\":" << call.minY
                << ",\"maxX\":" << call.maxX
                << ",\"maxY\":" << call.maxY
                << "}"
                << "}";
        }
        out << "]";
    }

    static void AppendRuntimeGpuSubmitCalls(std::ostringstream& out, const std::vector<RuntimeGpuSubmitCall>& calls)
    {
        out << "[";
        for (size_t i = 0; i < calls.size(); ++i)
        {
            if (i != 0)
                out << ",";

            const auto& call = calls[i];
            out
                << "{"
                << "\"sequence\":" << call.sequence
                << ",\"frame\":" << call.frame
                << ",\"source\":\"" << JsonEscape(call.source) << "\""
                << ",\"primitiveType\":" << call.primitiveType
                << ",\"primitiveTopology\":" << call.primitiveTopology
                << ",\"indexed\":" << (call.indexed ? "true" : "false")
                << ",\"inlineVertexStream\":" << (call.inlineVertexStream ? "true" : "false")
                << ",\"vertexCount\":" << call.vertexCount
                << ",\"indexCount\":" << call.indexCount
                << ",\"instanceCount\":" << call.instanceCount
                << ",\"startVertex\":" << call.startVertex
                << ",\"startIndex\":" << call.startIndex
                << ",\"baseVertex\":" << call.baseVertex
                << ",\"vertexStride\":" << call.vertexStride
                << ",\"texture2DDescriptorIndex\":" << call.texture2DDescriptorIndex
                << ",\"samplerDescriptorIndex\":" << call.samplerDescriptorIndex
                << ",\"pipelineState\":{"
                << "\"alphaBlendEnable\":" << (call.alphaBlendEnable ? "true" : "false")
                << ",\"srcBlend\":" << call.srcBlend
                << ",\"destBlend\":" << call.destBlend
                << ",\"blendOp\":" << call.blendOp
                << ",\"srcBlendAlpha\":" << call.srcBlendAlpha
                << ",\"destBlendAlpha\":" << call.destBlendAlpha
                << ",\"blendOpAlpha\":" << call.blendOpAlpha
                << ",\"colorWriteEnable\":" << call.colorWriteEnable
                << ",\"alphaTestEnable\":" << (call.alphaTestEnable ? "true" : "false")
                << ",\"alphaThreshold\":" << call.alphaThreshold
                << ",\"scissorEnable\":" << (call.scissorEnable ? "true" : "false")
                << "}"
                << ",\"scissorRect\":{"
                << "\"left\":" << call.scissorLeft
                << ",\"top\":" << call.scissorTop
                << ",\"right\":" << call.scissorRight
                << ",\"bottom\":" << call.scissorBottom
                << "}"
                << ",\"samplerState\":{"
                << "\"minFilter\":" << call.samplerMinFilter
                << ",\"magFilter\":" << call.samplerMagFilter
                << ",\"mipMode\":" << call.samplerMipMode
                << ",\"addressU\":" << call.samplerAddressU
                << ",\"addressV\":" << call.samplerAddressV
                << ",\"addressW\":" << call.samplerAddressW
                << "}"
                << ",\"halfPixelOffset\":{"
                << "\"x\":" << call.halfPixelOffsetX
                << ",\"y\":" << call.halfPixelOffsetY
                << "}"
                << "}";
        }
        out << "]";
    }

    static RuntimeGpuSubmitCall BackendResolvedAsGpuSubmitSemantic(const RuntimeBackendResolvedSubmit& submit)
    {
        RuntimeGpuSubmitCall call;
        call.alphaBlendEnable = submit.blendEnabled;
        call.srcBlend = submit.srcBlend;
        call.destBlend = submit.destBlend;
        call.blendOp = submit.blendOp;
        call.srcBlendAlpha = submit.srcBlendAlpha;
        call.destBlendAlpha = submit.destBlendAlpha;
        call.blendOpAlpha = submit.blendOpAlpha;
        call.colorWriteEnable = submit.renderTargetWriteMask;
        return call;
    }

    static BackendMaterialParityHint RuntimeBackendMaterialParityHint(const RuntimeBackendResolvedSubmit& submit)
    {
        BackendMaterialParityHint hint;
        const RuntimeGpuSubmitCall semantic = BackendResolvedAsGpuSubmitSemantic(submit);

        hint.framebufferRegistered =
            submit.activeFramebufferKnown &&
            submit.framebufferWidth != 0 &&
            submit.framebufferHeight != 0 &&
            submit.renderTargetFormat0 != 0;

        if (!submit.resolvedPipelineKnown)
        {
            hint.materialParityHint = "unresolved-pso";
            return hint;
        }

        if (!submit.blendEnabled)
        {
            hint.materialParityHint = "opaque-no-blend";
            hint.opaqueNoBlend = true;
            return hint;
        }

        if (RuntimeBlendIsAdditive(semantic))
        {
            hint.materialParityHint = "additive-alpha";
            hint.additiveAlpha = true;
            return hint;
        }

        if (submit.srcBlend == 5 && submit.destBlend == 6 && submit.blendOp == 1)
        {
            hint.materialParityHint = "source-over-alpha";
            hint.sourceOverAlpha = true;
            return hint;
        }

        hint.materialParityHint = "custom-blend";
        hint.customBlend = true;
        return hint;
    }

    static std::string RuntimeBackendMaterialParityStatus(uint32_t capturedSubmits)
    {
        return capturedSubmits > 0
            ? "backend-resolved PSO/blend/framebuffer material parity hints active"
            : "backend-resolved material parity hints armed; waiting for D3D12/Vulkan draws";
    }

    static void BuildBackendMaterialParityHintsJson(
        std::ostringstream& out,
        const std::vector<RuntimeBackendResolvedSubmit>& submits,
        uint32_t sourceOverCount,
        uint32_t additiveCount,
        uint32_t opaqueCount,
        uint32_t customBlendCount,
        uint32_t framebufferRegisteredCount)
    {
        const std::string status = RuntimeBackendMaterialParityStatus(static_cast<uint32_t>(submits.size()));
        out
            << "  \"materialParityStatus\": \"" << JsonEscape(status) << "\",\n"
            << "  \"backendMaterialParityHints\": {\n"
            << "    \"source\": \"backend-resolved PSO/blend/framebuffer material parity hints\",\n"
            << "    \"materialParityStatus\": \"" << JsonEscape(status) << "\",\n"
            << "    \"blendParityPolicy\": \"backend-resolved-pso-blend\",\n"
            << "    \"framebufferParityPolicy\": \"backend-resolved-framebuffer-registration\",\n"
            << "    \"alphaParityPolicy\": \"runtime-pso-blend-factors\",\n"
            << "    \"premultipliedAlphaPolicy\": \"defer-to-runtime-blend-factors\",\n"
            << "    \"gammaSrgbPolicy\": \"pending-texture-view-decode\",\n"
            << "    \"textureViewSamplerGap\": \"pending-descriptor-view-decode\",\n"
            << "    \"textMovieSfxGap\": \"pending-title-loading-media-timing\",\n"
            << "    \"sourceOverSubmitCount\": " << sourceOverCount << ",\n"
            << "    \"additiveSubmitCount\": " << additiveCount << ",\n"
            << "    \"opaqueSubmitCount\": " << opaqueCount << ",\n"
            << "    \"customBlendSubmitCount\": " << customBlendCount << ",\n"
            << "    \"framebufferRegisteredSubmitCount\": " << framebufferRegisteredCount << "\n"
            << "  },\n";
    }

    static std::string RuntimeTextureViewSamplerStatus(
        size_t materialPairCount,
        uint32_t knownTextureCount,
        uint32_t knownSamplerCount)
    {
        if (knownTextureCount > 0 || knownSamplerCount > 0)
            return "runtime texture-view/sampler descriptor semantics active";
        if (materialPairCount > 0)
            return "runtime descriptor indices active; waiting for descriptor metadata";
        return "runtime texture-view/sampler descriptor semantics armed; waiting for correlated submits";
    }

    static void BuildBackendDescriptorSemanticsJson(
        std::ostringstream& out,
        const std::vector<RuntimeMaterialCorrelation>& materialPairs,
        const std::unordered_map<uint32_t, RuntimeTextureDescriptorSemantic>& textureDescriptors,
        const std::unordered_map<uint32_t, RuntimeSamplerDescriptorSemantic>& samplerDescriptors)
    {
        uint32_t knownTextureCount = 0;
        uint32_t knownSamplerCount = 0;
        uint32_t linearSamplerCount = 0;
        uint32_t pointSamplerCount = 0;
        uint32_t wrapSamplerCount = 0;
        uint32_t clampSamplerCount = 0;

        for (const auto& pair : materialPairs)
        {
            if (textureDescriptors.find(pair.texture2DDescriptorIndex) != textureDescriptors.end())
                ++knownTextureCount;

            const auto samplerFound = samplerDescriptors.find(pair.samplerDescriptorIndex);
            if (samplerFound == samplerDescriptors.end())
                continue;

            ++knownSamplerCount;
            const auto& sampler = samplerFound->second;
            if (sampler.minFilter == 2 || sampler.magFilter == 2)
                ++linearSamplerCount;
            if (sampler.minFilter == 1 && sampler.magFilter == 1)
                ++pointSamplerCount;
            if (sampler.addressU == 1 || sampler.addressV == 1 || sampler.addressW == 1)
                ++wrapSamplerCount;
            if (sampler.addressU == 3 || sampler.addressV == 3 || sampler.addressW == 3)
                ++clampSamplerCount;
        }

        const std::string status = RuntimeTextureViewSamplerStatus(
            materialPairs.size(),
            knownTextureCount,
            knownSamplerCount);

        out
            << "  \"textureViewSamplerStatus\": \"" << JsonEscape(status) << "\",\n"
            << "  \"backendDescriptorSemantics\": {\n"
            << "    \"source\": \"runtime texture-view/sampler descriptor semantics\",\n"
            << "    \"textureViewSamplerStatus\": \"" << JsonEscape(status) << "\",\n"
            << "    \"textureDescriptorPolicy\": \"runtime-texture-view-descriptor-state\",\n"
            << "    \"samplerDescriptorPolicy\": \"runtime-sampler-descriptor-state\",\n"
            << "    \"vendorDescriptorCaptureGap\": \"pending-native-descriptor-dump\",\n"
            << "    \"materialPairCount\": " << materialPairs.size() << ",\n"
            << "    \"textureDescriptorKnownCount\": " << knownTextureCount << ",\n"
            << "    \"samplerDescriptorKnownCount\": " << knownSamplerCount << ",\n"
            << "    \"linearSamplerDescriptorCount\": " << linearSamplerCount << ",\n"
            << "    \"pointSamplerDescriptorCount\": " << pointSamplerCount << ",\n"
            << "    \"wrapSamplerDescriptorCount\": " << wrapSamplerCount << ",\n"
            << "    \"clampSamplerDescriptorCount\": " << clampSamplerCount << ",\n"
            << "    \"descriptorPairs\": [";

        const size_t pairLimit = std::min<size_t>(materialPairs.size(), 48);
        for (size_t i = 0; i < pairLimit; ++i)
        {
            if (i != 0)
                out << ",";

            const auto& pair = materialPairs[i];
            const auto textureFound = textureDescriptors.find(pair.texture2DDescriptorIndex);
            const auto samplerFound = samplerDescriptors.find(pair.samplerDescriptorIndex);
            const bool textureKnown = textureFound != textureDescriptors.end();
            const bool samplerKnown = samplerFound != samplerDescriptors.end();

            out
                << "{"
                << "\"uiDrawSequence\":" << pair.uiDrawSequence
                << ",\"gpuSubmitSequence\":" << pair.gpuSubmitSequence
                << ",\"texture2DDescriptorIndex\":" << pair.texture2DDescriptorIndex
                << ",\"samplerDescriptorIndex\":" << pair.samplerDescriptorIndex
                << ",\"textureDescriptorKnown\":" << (textureKnown ? "true" : "false")
                << ",\"samplerDescriptorKnown\":" << (samplerKnown ? "true" : "false")
                << ",\"textureDescriptorSemantic\":\""
                << JsonEscape(textureKnown ? RuntimeTextureDescriptorSemanticName(textureFound->second) : "missing")
                << "\""
                << ",\"samplerDescriptorSemantic\":\""
                << JsonEscape(samplerKnown ? RuntimeSamplerDescriptorSemanticName(samplerFound->second) : pair.samplerSemantic)
                << "\"";

            if (textureKnown)
            {
                const auto& texture = textureFound->second;
                out
                    << ",\"textureDescriptor\":{"
                    << "\"source\":\"" << JsonEscape(texture.source) << "\""
                    << ",\"width\":" << texture.width
                    << ",\"height\":" << texture.height
                    << ",\"depth\":" << texture.depth
                    << ",\"format\":" << texture.format
                    << ",\"viewDimension\":" << texture.viewDimension
                    << ",\"layout\":" << texture.layout
                    << "}";
            }

            if (samplerKnown)
            {
                const auto& sampler = samplerFound->second;
                out
                    << ",\"samplerDescriptor\":{"
                    << "\"minFilter\":" << sampler.minFilter
                    << ",\"magFilter\":" << sampler.magFilter
                    << ",\"mipMode\":" << sampler.mipMode
                    << ",\"addressU\":" << sampler.addressU
                    << ",\"addressV\":" << sampler.addressV
                    << ",\"addressW\":" << sampler.addressW
                    << ",\"anisotropyEnabled\":" << (sampler.anisotropyEnabled ? "true" : "false")
                    << ",\"maxAnisotropy\":" << sampler.maxAnisotropy
                    << ",\"comparisonEnabled\":" << (sampler.comparisonEnabled ? "true" : "false")
                    << ",\"borderColor\":" << sampler.borderColor
                    << "}";
            }

            out << "}";
        }

        out
            << "]\n"
            << "  },\n";
    }

    static std::string RuntimeVendorResourceCaptureStatus(
        size_t materialPairCount,
        uint32_t textureResourceViewKnownCount,
        uint32_t samplerResourceViewKnownCount)
    {
        if (textureResourceViewKnownCount > 0 || samplerResourceViewKnownCount > 0)
            return "native RHI resource-view/sampler handles active";
        if (materialPairCount > 0)
            return "runtime material pairs active; waiting for native RHI resource-view/sampler handles";
        return "native RHI resource-view/sampler capture armed; waiting for correlated submits";
    }

    static void BuildBackendVendorResourceCaptureJson(
        std::ostringstream& out,
        const std::vector<RuntimeMaterialCorrelation>& materialPairs,
        const std::unordered_map<uint32_t, RuntimeVendorTextureResourceView>& textureResourceViews,
        const std::unordered_map<uint32_t, RuntimeVendorSamplerResourceView>& samplerResourceViews)
    {
        uint32_t textureResourceViewKnownCount = 0;
        uint32_t samplerResourceViewKnownCount = 0;
        uint32_t resourceViewPairCount = 0;

        for (const auto& pair : materialPairs)
        {
            const bool textureKnown = textureResourceViews.find(pair.texture2DDescriptorIndex) != textureResourceViews.end();
            const bool samplerKnown = samplerResourceViews.find(pair.samplerDescriptorIndex) != samplerResourceViews.end();
            if (textureKnown)
                ++textureResourceViewKnownCount;
            if (samplerKnown)
                ++samplerResourceViewKnownCount;
            if (textureKnown && samplerKnown)
                ++resourceViewPairCount;
        }

        const std::string status = RuntimeVendorResourceCaptureStatus(
            materialPairs.size(),
            textureResourceViewKnownCount,
            samplerResourceViewKnownCount);

        out
            << "  \"vendorResourceCaptureStatus\": \"" << JsonEscape(status) << "\",\n"
            << "  \"uiOnlyLayerCaptureStatus\": \"pending-runtime-ui-render-target-copy\",\n"
            << "  \"nativeCommandCaptureGap\": \"pending-full-vendor-command-buffer-dump\",\n"
            << "  \"backendVendorResourceCapture\": {\n"
            << "    \"source\": \"native RHI resource-view/sampler handle capture\",\n"
            << "    \"vendorResourceCapturePolicy\": \"native-rhi-resource-view-and-sampler-handles\",\n"
            << "    \"vendorResourceCaptureStatus\": \"" << JsonEscape(status) << "\",\n"
            << "    \"uiOnlyLayerCaptureStatus\": \"pending-runtime-ui-render-target-copy\",\n"
            << "    \"nativeCommandCaptureGap\": \"pending-full-vendor-command-buffer-dump\",\n"
            << "    \"materialPairCount\": " << materialPairs.size() << ",\n"
            << "    \"textureResourceViewKnownCount\": " << textureResourceViewKnownCount << ",\n"
            << "    \"samplerResourceViewKnownCount\": " << samplerResourceViewKnownCount << ",\n"
            << "    \"resourceViewPairCount\": " << resourceViewPairCount << ",\n"
            << "    \"observedTextureResourceViewCount\": " << textureResourceViews.size() << ",\n"
            << "    \"observedSamplerResourceViewCount\": " << samplerResourceViews.size() << ",\n"
            << "    \"resourcePairs\": [";

        const size_t pairLimit = std::min<size_t>(materialPairs.size(), 48);
        for (size_t i = 0; i < pairLimit; ++i)
        {
            if (i != 0)
                out << ",";

            const auto& pair = materialPairs[i];
            const auto textureFound = textureResourceViews.find(pair.texture2DDescriptorIndex);
            const auto samplerFound = samplerResourceViews.find(pair.samplerDescriptorIndex);
            const bool textureKnown = textureFound != textureResourceViews.end();
            const bool samplerKnown = samplerFound != samplerResourceViews.end();

            out
                << "{"
                << "\"uiDrawSequence\":" << pair.uiDrawSequence
                << ",\"gpuSubmitSequence\":" << pair.gpuSubmitSequence
                << ",\"texture2DDescriptorIndex\":" << pair.texture2DDescriptorIndex
                << ",\"samplerDescriptorIndex\":" << pair.samplerDescriptorIndex
                << ",\"textureResourceViewKnown\":" << (textureKnown ? "true" : "false")
                << ",\"samplerResourceViewKnown\":" << (samplerKnown ? "true" : "false")
                << ",\"nativeTextureResourceHandle\":\""
                << JsonEscape(textureKnown ? HexU64(textureFound->second.nativeTextureResourceHandle) : "0x0000000000000000")
                << "\""
                << ",\"nativeTextureViewHandle\":\""
                << JsonEscape(textureKnown ? HexU64(textureFound->second.nativeTextureViewHandle) : "0x0000000000000000")
                << "\""
                << ",\"nativeSamplerHandle\":\""
                << JsonEscape(samplerKnown ? HexU64(samplerFound->second.nativeSamplerHandle) : "0x0000000000000000")
                << "\"";

            if (textureKnown)
            {
                const auto& texture = textureFound->second;
                out
                    << ",\"textureResourceView\":{"
                    << "\"backend\":\"" << JsonEscape(texture.backend) << "\""
                    << ",\"source\":\"" << JsonEscape(texture.source) << "\""
                    << ",\"nativeFormat\":" << texture.nativeFormat
                    << ",\"nativeViewDimension\":" << texture.nativeViewDimension
                    << ",\"width\":" << texture.width
                    << ",\"height\":" << texture.height
                    << ",\"mipLevels\":" << texture.mipLevels
                    << "}";
            }

            if (samplerKnown)
            {
                const auto& sampler = samplerFound->second;
                out
                    << ",\"samplerResourceView\":{"
                    << "\"backend\":\"" << JsonEscape(sampler.backend) << "\""
                    << ",\"source\":\"" << JsonEscape(sampler.source) << "\""
                    << ",\"nativeFilter\":" << sampler.nativeFilter
                    << ",\"nativeAddressU\":" << sampler.nativeAddressU
                    << ",\"nativeAddressV\":" << sampler.nativeAddressV
                    << ",\"nativeAddressW\":" << sampler.nativeAddressW
                    << "}";
            }

            out << "}";
        }

        out
            << "]\n"
            << "  },\n";
    }

    static bool RuntimeNativeFormatLooksSrgb(const RuntimeVendorTextureResourceView& texture)
    {
        switch (texture.nativeFormat)
        {
            // DXGI_FORMAT_*_SRGB values observed by D3D12 resource views.
            case 29:  // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
            case 72:  // DXGI_FORMAT_BC1_UNORM_SRGB
            case 75:  // DXGI_FORMAT_BC2_UNORM_SRGB
            case 78:  // DXGI_FORMAT_BC3_UNORM_SRGB
            case 91:  // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
            case 93:  // DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
            case 99:  // DXGI_FORMAT_BC7_UNORM_SRGB
            // VkFormat *_SRGB values observed by Vulkan image views.
            case 43:  // VK_FORMAT_R8G8B8A8_SRGB
            case 50:  // VK_FORMAT_B8G8R8A8_SRGB
            case 132: // VK_FORMAT_BC1_RGB_SRGB_BLOCK
            case 134: // VK_FORMAT_BC1_RGBA_SRGB_BLOCK
            case 136: // VK_FORMAT_BC2_SRGB_BLOCK
            case 138: // VK_FORMAT_BC3_SRGB_BLOCK
            case 146: // VK_FORMAT_BC7_SRGB_BLOCK
                return true;
            default:
                return false;
        }
    }

    static std::string RuntimeMaterialResourceViewParityStatus(
        uint32_t resourceViewExactPairCount,
        uint32_t srgbTextureResourceViewCount,
        uint32_t alphaBlendResourcePairCount)
    {
        if (resourceViewExactPairCount == 0)
            return "waiting-for-native-resource-view-pairs";
        if (srgbTextureResourceViewCount > 0)
            return "native-resource-view-pairs-active-with-srgb-format-candidates";
        if (alphaBlendResourcePairCount > 0)
            return "native-resource-view-pairs-active-with-alpha-blend-materials";
        return "native-resource-view-pairs-active";
    }

    static std::string RuntimeResourceViewExactnessStatus(uint32_t resourceViewExactPairCount, size_t materialPairCount)
    {
        if (resourceViewExactPairCount > 0)
            return "native-texture-view-and-sampler-handles-paired";
        if (materialPairCount > 0)
            return "material-pairs-active-waiting-for-native-resource-handles";
        return "waiting-for-material-pairs";
    }

    static std::string RuntimePremultipliedAlphaStatus(uint32_t alphaBlendResourcePairCount, uint32_t additiveResourcePairCount)
    {
        if (alphaBlendResourcePairCount == 0)
            return "no-alpha-blend-resource-pairs-yet";
        if (additiveResourcePairCount > 0)
            return "runtime-blend-state-and-resource-views-show-additive-alpha-paths";
        return "runtime-blend-state-and-resource-views-show-straight-alpha-paths";
    }

    static std::string RuntimeGammaSrgbStatus(uint32_t srgbTextureResourceViewCount, uint32_t textureResourceViewKnownCount)
    {
        if (srgbTextureResourceViewCount > 0)
            return "native-srgb-resource-view-formats-observed";
        if (textureResourceViewKnownCount > 0)
            return "native-resource-view-formats-captured-no-srgb-classification";
        return "waiting-for-native-resource-view-formats";
    }

    static void BuildBackendMaterialResourceViewParityJson(
        std::ostringstream& out,
        const std::vector<RuntimeMaterialCorrelation>& materialPairs,
        const std::unordered_map<uint32_t, RuntimeVendorTextureResourceView>& textureResourceViews,
        const std::unordered_map<uint32_t, RuntimeVendorSamplerResourceView>& samplerResourceViews)
    {
        uint32_t textureResourceViewKnownCount = 0;
        uint32_t samplerResourceViewKnownCount = 0;
        uint32_t resourceViewExactPairCount = 0;
        uint32_t alphaBlendResourcePairCount = 0;
        uint32_t additiveResourcePairCount = 0;
        uint32_t srgbTextureResourceViewCount = 0;

        for (const auto& pair : materialPairs)
        {
            const auto textureFound = textureResourceViews.find(pair.texture2DDescriptorIndex);
            const auto samplerFound = samplerResourceViews.find(pair.samplerDescriptorIndex);
            const bool textureKnown = textureFound != textureResourceViews.end();
            const bool samplerKnown = samplerFound != samplerResourceViews.end();
            if (textureKnown)
            {
                ++textureResourceViewKnownCount;
                if (RuntimeNativeFormatLooksSrgb(textureFound->second))
                    ++srgbTextureResourceViewCount;
            }
            if (samplerKnown)
                ++samplerResourceViewKnownCount;
            if (!textureKnown || !samplerKnown)
                continue;

            ++resourceViewExactPairCount;
            if (pair.alphaBlendEnable)
                ++alphaBlendResourcePairCount;
            if (pair.additiveBlend)
                ++additiveResourcePairCount;
        }

        const std::string status = RuntimeMaterialResourceViewParityStatus(
            resourceViewExactPairCount,
            srgbTextureResourceViewCount,
            alphaBlendResourcePairCount);
        const std::string exactnessStatus = RuntimeResourceViewExactnessStatus(resourceViewExactPairCount, materialPairs.size());
        const std::string premultipliedStatus = RuntimePremultipliedAlphaStatus(alphaBlendResourcePairCount, additiveResourcePairCount);
        const std::string gammaStatus = RuntimeGammaSrgbStatus(srgbTextureResourceViewCount, textureResourceViewKnownCount);

        out
            << "  \"materialResourceViewParityStatus\": \"" << JsonEscape(status) << "\",\n"
            << "  \"resourceViewExactnessStatus\": \"" << JsonEscape(exactnessStatus) << "\",\n"
            << "  \"premultipliedAlphaStatus\": \"" << JsonEscape(premultipliedStatus) << "\",\n"
            << "  \"gammaSrgbStatus\": \"" << JsonEscape(gammaStatus) << "\",\n"
            << "  \"backendMaterialResourceViewParity\": {\n"
            << "    \"source\": \"vendor resource-view material parity tightening\",\n"
            << "    \"materialResourceViewParityPolicy\": \"vendor-resource-view-alpha-gamma-srgb\",\n"
            << "    \"materialResourceViewParityStatus\": \"" << JsonEscape(status) << "\",\n"
            << "    \"premultipliedAlphaPolicy\": \"runtime-blend-state-plus-vendor-resource-view\",\n"
            << "    \"gammaSrgbPolicy\": \"native-resource-view-format-classification\",\n"
            << "    \"resourceViewExactnessPolicy\": \"native-resource-view-and-sampler-handle-pairing\",\n"
            << "    \"resourceViewExactnessStatus\": \"" << JsonEscape(exactnessStatus) << "\",\n"
            << "    \"premultipliedAlphaStatus\": \"" << JsonEscape(premultipliedStatus) << "\",\n"
            << "    \"gammaSrgbStatus\": \"" << JsonEscape(gammaStatus) << "\",\n"
            << "    \"materialPairCount\": " << materialPairs.size() << ",\n"
            << "    \"textureResourceViewKnownCount\": " << textureResourceViewKnownCount << ",\n"
            << "    \"samplerResourceViewKnownCount\": " << samplerResourceViewKnownCount << ",\n"
            << "    \"resourceViewExactPairCount\": " << resourceViewExactPairCount << ",\n"
            << "    \"alphaBlendResourcePairCount\": " << alphaBlendResourcePairCount << ",\n"
            << "    \"additiveResourcePairCount\": " << additiveResourcePairCount << ",\n"
            << "    \"srgbTextureResourceViewCount\": " << srgbTextureResourceViewCount << ",\n"
            << "    \"uiOnlyRenderTargetCaptureProbe\": {\n"
            << "      \"uiOnlyRenderTargetCapturePolicy\": \"copy-ui-render-target-before-present\",\n"
            << "      \"uiOnlyLayerCaptureStatus\": \"pending-runtime-ui-render-target-copy\",\n"
            << "      \"captureHookStatus\": \"pending-ui-pass-render-target-copy\",\n"
            << "      \"nativeCommandCaptureGap\": \"pending-full-vendor-command-buffer-dump\"\n"
            << "    }\n"
            << "  },\n";
    }

    static void AppendRuntimeBackendResolvedSubmits(
        std::ostringstream& out,
        const std::vector<RuntimeBackendResolvedSubmit>& submits)
    {
        out << "[";
        for (size_t i = 0; i < submits.size(); ++i)
        {
            if (i != 0)
                out << ",";

            const auto& submit = submits[i];
            const RuntimeGpuSubmitCall semantic = BackendResolvedAsGpuSubmitSemantic(submit);
            const BackendMaterialParityHint parityHint = RuntimeBackendMaterialParityHint(submit);
            out
                << "{"
                << "\"sequence\":" << submit.sequence
                << ",\"frame\":" << submit.frame
                << ",\"backend\":\"" << JsonEscape(submit.backend) << "\""
                << ",\"nativeCommand\":\"" << JsonEscape(submit.nativeCommand) << "\""
                << ",\"indexed\":" << (submit.indexed ? "true" : "false")
                << ",\"vertexCount\":" << submit.vertexCount
                << ",\"indexCount\":" << submit.indexCount
                << ",\"instanceCount\":" << submit.instanceCount
                << ",\"nativePipelineHandle\":\"" << JsonEscape(HexU64(submit.nativePipelineHandle)) << "\""
                << ",\"nativePipelineLayoutHandle\":\"" << JsonEscape(HexU64(submit.nativePipelineLayoutHandle)) << "\""
                << ",\"resolvedPipelineKnown\":" << (submit.resolvedPipelineKnown ? "true" : "false")
                << ",\"activeFramebufferKnown\":" << (submit.activeFramebufferKnown ? "true" : "false")
                << ",\"framebufferSize\":{"
                << "\"width\":" << submit.framebufferWidth
                << ",\"height\":" << submit.framebufferHeight
                << "}"
                << ",\"renderTargetCount\":" << submit.renderTargetCount
                << ",\"renderTargetFormat0\":" << submit.renderTargetFormat0
                << ",\"depthTargetFormat\":" << submit.depthTargetFormat
                << ",\"sampleCount\":" << submit.sampleCount
                << ",\"primitiveTopology\":" << submit.primitiveTopology
                << ",\"pipelineBlendState\":{"
                << "\"blendEnabled\":" << (submit.blendEnabled ? "true" : "false")
                << ",\"srcBlend\":" << submit.srcBlend
                << ",\"destBlend\":" << submit.destBlend
                << ",\"blendOp\":" << submit.blendOp
                << ",\"srcBlendAlpha\":" << submit.srcBlendAlpha
                << ",\"destBlendAlpha\":" << submit.destBlendAlpha
                << ",\"blendOpAlpha\":" << submit.blendOpAlpha
                << ",\"renderTargetWriteMask\":" << submit.renderTargetWriteMask
                << ",\"blendSemantic\":\"" << JsonEscape(RuntimeBlendSemantic(semantic)) << "\""
                << ",\"blendOperationSemantic\":\"" << JsonEscape(RuntimeBlendOperationSemantic(semantic)) << "\""
                << ",\"additiveBlend\":" << (RuntimeBlendIsAdditive(semantic) ? "true" : "false")
                << ",\"colorWriteSemantic\":\"" << JsonEscape(RuntimeColorWriteSemantic(semantic)) << "\""
                << "}"
                << ",\"materialParityHint\":\"" << JsonEscape(parityHint.materialParityHint) << "\""
                << ",\"framebufferRegistered\":" << (parityHint.framebufferRegistered ? "true" : "false")
                << ",\"depthState\":{"
                << "\"depthEnabled\":" << (submit.depthEnabled ? "true" : "false")
                << ",\"depthWriteEnabled\":" << (submit.depthWriteEnabled ? "true" : "false")
                << "}"
                << ",\"inputLayout\":{"
                << "\"inputSlotCount\":" << submit.inputSlotCount
                << ",\"inputElementCount\":" << submit.inputElementCount
                << "}"
                << ",\"alphaToCoverageEnabled\":" << (submit.alphaToCoverageEnabled ? "true" : "false")
                << "}";
        }
        out << "]";
    }

    static void AppendRuntimeMaterialCorrelationPairs(
        std::ostringstream& out,
        const std::vector<RuntimeMaterialCorrelation>& pairs)
    {
        out << "[";
        for (size_t i = 0; i < pairs.size(); ++i)
        {
            if (i != 0)
                out << ",";

            const auto& pair = pairs[i];
            out
                << "{"
                << "\"frame\":" << pair.frame
                << ",\"uiDrawSequence\":" << pair.uiDrawSequence
                << ",\"gpuSubmitSequence\":" << pair.gpuSubmitSequence
                << ",\"project\":\"" << JsonEscape(pair.projectName) << "\""
                << ",\"layerPath\":\"" << JsonEscape(pair.layerPath) << "\""
                << ",\"gpuSubmitSource\":\"" << JsonEscape(pair.gpuSubmitSource) << "\""
                << ",\"screenRect\":{"
                << "\"minX\":" << pair.minX
                << ",\"minY\":" << pair.minY
                << ",\"maxX\":" << pair.maxX
                << ",\"maxY\":" << pair.maxY
                << "}"
                << ",\"texture2DDescriptorIndex\":" << pair.texture2DDescriptorIndex
                << ",\"samplerDescriptorIndex\":" << pair.samplerDescriptorIndex
                << ",\"alphaBlendEnable\":" << (pair.alphaBlendEnable ? "true" : "false")
                << ",\"additiveBlend\":" << (pair.additiveBlend ? "true" : "false")
                << ",\"linearFilter\":" << (pair.linearFilter ? "true" : "false")
                << ",\"pointFilter\":" << (pair.pointFilter ? "true" : "false")
                << ",\"blendSemantic\":\"" << JsonEscape(pair.blendSemantic) << "\""
                << ",\"blendOperationSemantic\":\"" << JsonEscape(pair.blendOperationSemantic) << "\""
                << ",\"samplerSemantic\":\"" << JsonEscape(pair.samplerSemantic) << "\""
                << ",\"addressSemantic\":\"" << JsonEscape(pair.addressSemantic) << "\""
                << ",\"alphaSemantic\":\"" << JsonEscape(pair.alphaSemantic) << "\""
                << ",\"colorWriteSemantic\":\"" << JsonEscape(pair.colorWriteSemantic) << "\""
                << ",\"halfPixelOffset\":{"
                << "\"x\":" << pair.halfPixelOffsetX
                << ",\"y\":" << pair.halfPixelOffsetY
                << "}"
                << "}";
        }
        out << "]";
    }

    static void AppendRuntimeRawBackendCommands(
        std::ostringstream& out,
        const std::vector<RuntimeRawBackendCommand>& commands)
    {
        out << "[";
        for (size_t i = 0; i < commands.size(); ++i)
        {
            if (i != 0)
                out << ",";

            const auto& command = commands[i];
            out
                << "{"
                << "\"sequence\":" << command.sequence
                << ",\"frame\":" << command.frame
                << ",\"backend\":\"" << JsonEscape(command.backend) << "\""
                << ",\"command\":\"" << JsonEscape(command.command) << "\""
                << ",\"source\":\"" << JsonEscape(command.source) << "\""
                << ",\"indexed\":" << (command.indexed ? "true" : "false")
                << ",\"vertexCount\":" << command.vertexCount
                << ",\"indexCount\":" << command.indexCount
                << ",\"instanceCount\":" << command.instanceCount
                << "}";
        }
        out << "]";
    }

    static void AppendRuntimeVendorResourceDumpPairs(
        std::ostringstream& out,
        const std::vector<RuntimeMaterialCorrelation>& materialPairs,
        const std::unordered_map<uint32_t, RuntimeVendorTextureResourceView>& textureResourceViews,
        const std::unordered_map<uint32_t, RuntimeVendorSamplerResourceView>& samplerResourceViews)
    {
        out << "[";
        const size_t pairLimit = std::min<size_t>(materialPairs.size(), 96);
        bool wrote = false;
        for (size_t i = 0; i < pairLimit; ++i)
        {
            const auto& pair = materialPairs[i];
            const auto textureFound = textureResourceViews.find(pair.texture2DDescriptorIndex);
            const auto samplerFound = samplerResourceViews.find(pair.samplerDescriptorIndex);
            if (textureFound == textureResourceViews.end() && samplerFound == samplerResourceViews.end())
                continue;

            if (wrote)
                out << ",";
            wrote = true;

            out
                << "{"
                << "\"uiDrawSequence\":" << pair.uiDrawSequence
                << ",\"gpuSubmitSequence\":" << pair.gpuSubmitSequence
                << ",\"texture2DDescriptorIndex\":" << pair.texture2DDescriptorIndex
                << ",\"samplerDescriptorIndex\":" << pair.samplerDescriptorIndex
                << ",\"textureResourceViewKnown\":" << (textureFound != textureResourceViews.end() ? "true" : "false")
                << ",\"samplerResourceViewKnown\":" << (samplerFound != samplerResourceViews.end() ? "true" : "false");

            if (textureFound != textureResourceViews.end())
            {
                const auto& texture = textureFound->second;
                out
                    << ",\"nativeTextureResourceHandle\":\"" << JsonEscape(HexU64(texture.nativeTextureResourceHandle)) << "\""
                    << ",\"nativeTextureViewHandle\":\"" << JsonEscape(HexU64(texture.nativeTextureViewHandle)) << "\""
                    << ",\"nativeFormat\":" << texture.nativeFormat
                    << ",\"nativeViewDimension\":" << texture.nativeViewDimension
                    << ",\"width\":" << texture.width
                    << ",\"height\":" << texture.height
                    << ",\"mipLevels\":" << texture.mipLevels;
            }

            if (samplerFound != samplerResourceViews.end())
            {
                const auto& sampler = samplerFound->second;
                out
                    << ",\"nativeSamplerHandle\":\"" << JsonEscape(HexU64(sampler.nativeSamplerHandle)) << "\""
                    << ",\"nativeFilter\":" << sampler.nativeFilter
                    << ",\"nativeAddressU\":" << sampler.nativeAddressU
                    << ",\"nativeAddressV\":" << sampler.nativeAddressV
                    << ",\"nativeAddressW\":" << sampler.nativeAddressW;
            }

            out << "}";
        }
        out << "]";
    }

    static std::string RuntimeVendorCommandResourceDumpStatus(
        size_t rawBackendCommandCount,
        size_t backendResolvedSubmitCount,
        uint32_t resourcePairDumpCount)
    {
        if (rawBackendCommandCount > 0 && resourcePairDumpCount > 0)
            return "vendor command/resource dump active";
        if (backendResolvedSubmitCount > 0 && resourcePairDumpCount > 0)
            return "vendor resource dump active; raw command boundary sparse";
        if (backendResolvedSubmitCount > 0)
            return "backend command dump active; waiting for vendor resource pairs";
        return "vendor command/resource dump armed; waiting for backend commands";
    }

    [[maybe_unused]] static constexpr std::string_view kUiOnlyRenderTargetCaptureSchemaMarkers[] =
    {
        R"("uiOnlyRenderTargetCapture")",
        R"("uiOnlyRenderTargetCapturePolicy": "copy-active-ui-render-target-before-imgui-present")",
        R"("uiOnlyRenderTargetCaptureStatus")",
        R"("uiOnlyLayerCaptureStatus")",
        R"("uiOnlyLayerIsolationStatus")",
        R"("uiOnlyRenderTargetCapturePath")",
        R"("uiOnlyRenderTargetCaptureSource")"
    };

    static std::string BuildRuntimeVendorCommandResourceDumpJson()
    {
        const auto& target = TargetFor(g_target);

        std::vector<RuntimeRawBackendCommand> rawBackendCommands;
        std::vector<RuntimeBackendResolvedSubmit> backendResolvedSubmits;
        std::vector<RuntimeUiDrawCall> drawCalls;
        std::vector<RuntimeGpuSubmitCall> gpuSubmitCalls;
        std::unordered_map<uint32_t, RuntimeVendorTextureResourceView> textureResourceViews;
        std::unordered_map<uint32_t, RuntimeVendorSamplerResourceView> samplerResourceViews;
        uint64_t frame = g_presentedFrameCount;
        uint32_t rawDroppedCount = 0;
        uint32_t backendDroppedCount = 0;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            rawBackendCommands = g_runtimeRawBackendCommands;
            backendResolvedSubmits = g_runtimeBackendResolvedSubmits;
            drawCalls = g_runtimeUiDrawCalls;
            gpuSubmitCalls = g_runtimeGpuSubmitCalls;
            textureResourceViews = g_runtimeVendorTextureResourceViews;
            samplerResourceViews = g_runtimeVendorSamplerResourceViews;
            if (g_runtimeBackendResolvedFrame != UINT64_MAX)
                frame = g_runtimeBackendResolvedFrame;
            else if (g_runtimeRawBackendCommandFrame != UINT64_MAX)
                frame = g_runtimeRawBackendCommandFrame;
            rawDroppedCount = g_runtimeRawBackendCommandDroppedCount;
            backendDroppedCount = g_runtimeBackendResolvedDroppedCount;
        }

        const auto materialPairs = BuildRuntimeMaterialCorrelationPairs(drawCalls, gpuSubmitCalls);
        uint32_t resourcePairDumpCount = 0;
        for (const auto& pair : materialPairs)
        {
            if (textureResourceViews.find(pair.texture2DDescriptorIndex) != textureResourceViews.end() &&
                samplerResourceViews.find(pair.samplerDescriptorIndex) != samplerResourceViews.end())
            {
                ++resourcePairDumpCount;
            }
        }

        const std::string status = RuntimeVendorCommandResourceDumpStatus(
            rawBackendCommands.size(),
            backendResolvedSubmits.size(),
            resourcePairDumpCount);

        std::ostringstream out;
        out
            << "{\n"
            << "  \"ok\": true,\n"
            << "  \"source\": \"runtime vendor command/resource dump\",\n"
            << "  \"version\": 1,\n"
            << "  \"frame\": " << frame << ",\n"
            << "  \"target\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"activeScreen\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"targetProject\": \"" << JsonEscape(target.primaryCsdScene) << "\",\n"
            << "  \"vendorCommandResourceDumpPolicy\": \"raw-backend-command-plus-resource-view-dump\",\n"
            << "  \"vendorCommandResourceDumpStatus\": \"" << JsonEscape(status) << "\",\n"
            << "  \"rawBackendCommandCount\": " << rawBackendCommands.size() << ",\n"
            << "  \"backendResolvedSubmitCount\": " << backendResolvedSubmits.size() << ",\n"
            << "  \"materialPairCount\": " << materialPairs.size() << ",\n"
            << "  \"textureResourceViewDumpCount\": " << textureResourceViews.size() << ",\n"
            << "  \"samplerResourceViewDumpCount\": " << samplerResourceViews.size() << ",\n"
            << "  \"resourcePairDumpCount\": " << resourcePairDumpCount << ",\n"
            << "  \"droppedRawBackendCommandCount\": " << rawDroppedCount << ",\n"
            << "  \"droppedBackendResolvedSubmitCount\": " << backendDroppedCount << ",\n"
            << "  \"uiOnlyRenderedLayerStatus\": \"pending-runtime-ui-render-target-copy\",\n"
            << "  \"uiOnlyLayerCaptureStatus\": \"pending-runtime-ui-render-target-copy\",\n"
            << "  \"vendorCommandReplayGap\": \"pending-full-vendor-command-buffer-replay\",\n"
            << "  \"nativeCommandCaptureGap\": \"vendor command/resource dump active; full command-buffer replay pending\",\n"
            << "  \"vendorCommandResourceDump\": {\n"
            << "    \"source\": \"raw backend command samples plus native resource views\",\n"
            << "    \"vendorCommandResourceDumpPolicy\": \"raw-backend-command-plus-resource-view-dump\",\n"
            << "    \"vendorCommandResourceDumpStatus\": \"" << JsonEscape(status) << "\",\n"
            << "    \"rawBackendCommandCount\": " << rawBackendCommands.size() << ",\n"
            << "    \"backendResolvedSubmitCount\": " << backendResolvedSubmits.size() << ",\n"
            << "    \"textureResourceViewDumpCount\": " << textureResourceViews.size() << ",\n"
            << "    \"samplerResourceViewDumpCount\": " << samplerResourceViews.size() << ",\n"
            << "    \"resourcePairDumpCount\": " << resourcePairDumpCount << ",\n"
            << "    \"uiOnlyRenderedLayerStatus\": \"pending-runtime-ui-render-target-copy\",\n"
            << "    \"vendorCommandReplayGap\": \"pending-full-vendor-command-buffer-replay\",\n"
            << "    \"rawCommands\": ";
        AppendRuntimeRawBackendCommands(out, rawBackendCommands);
        out
            << ",\n"
            << "    \"backendSubmits\": ";
        AppendRuntimeBackendResolvedSubmits(out, backendResolvedSubmits);
        out
            << ",\n"
            << "    \"resourcePairs\": ";
        AppendRuntimeVendorResourceDumpPairs(out, materialPairs, textureResourceViews, samplerResourceViews);
        out
            << "\n"
            << "  }\n"
            << "}\n";

        return out.str();
    }

    static std::string BuildRuntimeUiOnlyRenderTargetCaptureJson()
    {
        const auto& target = TargetFor(g_target);

        std::vector<RuntimeUiDrawCall> drawCalls;
        std::vector<RuntimeGpuSubmitCall> gpuSubmitCalls;
        uint64_t drawListFrame = UINT64_MAX;
        uint64_t submitFrame = UINT64_MAX;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            drawCalls = g_runtimeUiDrawCalls;
            gpuSubmitCalls = g_runtimeGpuSubmitCalls;
            drawListFrame = g_runtimeUiDrawListFrame;
            submitFrame = g_runtimeGpuSubmitFrame;
        }

        const uint64_t frame = g_lastUiOnlyRenderTargetCaptureFrame != 0
            ? g_lastUiOnlyRenderTargetCaptureFrame
            : g_presentedFrameCount;
        const std::string_view status = UiOnlyRenderTargetCaptureStatusLabel();
        const std::string_view isolation = UiOnlyLayerIsolationStatusLabel();
        const bool captured = !g_lastUiOnlyRenderTargetCapturePath.empty();
        const std::string source = !g_lastUiOnlyRenderTargetCaptureSource.empty()
            ? g_lastUiOnlyRenderTargetCaptureSource
            : "active-render-target-before-imgui-present";

        std::ostringstream out;
        out
            << "{\n"
            << "  \"ok\": true,\n"
            << "  \"source\": \"runtime UI render-target capture status\",\n"
            << "  \"version\": 1,\n"
            << "  \"frame\": " << frame << ",\n"
            << "  \"target\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"activeScreen\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"targetProject\": \"" << JsonEscape(target.primaryCsdScene) << "\",\n"
            << "  \"uiOnlyRenderTargetCapturePolicy\": \"copy-active-ui-render-target-before-imgui-present\",\n"
            << "  \"uiOnlyRenderTargetCaptureStatus\": \"" << JsonEscape(status) << "\",\n"
            << "  \"uiOnlyLayerCaptureStatus\": \"" << JsonEscape(status) << "\",\n"
            << "  \"uiOnlyLayerIsolationStatus\": \"" << JsonEscape(isolation) << "\",\n"
            << "  \"uiOnlyRenderTargetCapturePath\": \"" << JsonEscape(g_lastUiOnlyRenderTargetCapturePath) << "\",\n"
            << "  \"uiOnlyRenderTargetCaptureFailure\": \"" << JsonEscape(g_lastUiOnlyRenderTargetCaptureFailure) << "\",\n"
            << "  \"uiOnlyRenderTargetCaptureSource\": \"" << JsonEscape(source) << "\",\n"
            << "  \"uiOnlyRenderTargetCaptureWidth\": " << g_lastUiOnlyRenderTargetCaptureWidth << ",\n"
            << "  \"uiOnlyRenderTargetCaptureHeight\": " << g_lastUiOnlyRenderTargetCaptureHeight << ",\n"
            << "  \"uiOnlyRenderTargetCaptureFrame\": " << g_lastUiOnlyRenderTargetCaptureFrame << ",\n"
            << "  \"uiOnlyRenderTargetContainsFullFramebuffer\": "
            << (g_lastUiOnlyRenderTargetCaptureContainsFullFramebuffer ? "true" : "false") << ",\n"
            << "  \"runtimeUiDrawCallCount\": " << drawCalls.size() << ",\n"
            << "  \"runtimeGpuSubmitCallCount\": " << gpuSubmitCalls.size() << ",\n"
            << "  \"runtimeUiDrawListFrame\": "
            << (drawListFrame == UINT64_MAX ? -1 : static_cast<int64_t>(drawListFrame)) << ",\n"
            << "  \"runtimeGpuSubmitFrame\": "
            << (submitFrame == UINT64_MAX ? -1 : static_cast<int64_t>(submitFrame)) << ",\n"
            << "  \"uiOnlyRenderTargetCapture\": {\n"
            << "    \"captured\": " << (captured ? "true" : "false") << ",\n"
            << "    \"policy\": \"copy-active-ui-render-target-before-imgui-present\",\n"
            << "    \"status\": \"" << JsonEscape(status) << "\",\n"
            << "    \"layerStatus\": \"" << JsonEscape(status) << "\",\n"
            << "    \"isolationStatus\": \"" << JsonEscape(isolation) << "\",\n"
            << "    \"path\": \"" << JsonEscape(g_lastUiOnlyRenderTargetCapturePath) << "\",\n"
            << "    \"failure\": \"" << JsonEscape(g_lastUiOnlyRenderTargetCaptureFailure) << "\",\n"
            << "    \"source\": \"" << JsonEscape(source) << "\",\n"
            << "    \"width\": " << g_lastUiOnlyRenderTargetCaptureWidth << ",\n"
            << "    \"height\": " << g_lastUiOnlyRenderTargetCaptureHeight << ",\n"
            << "    \"frame\": " << g_lastUiOnlyRenderTargetCaptureFrame << ",\n"
            << "    \"containsFullFramebuffer\": "
            << (g_lastUiOnlyRenderTargetCaptureContainsFullFramebuffer ? "true" : "false") << ",\n"
            << "    \"runtimeUiDrawCallCount\": " << drawCalls.size() << ",\n"
            << "    \"runtimeGpuSubmitCallCount\": " << gpuSubmitCalls.size() << "\n"
            << "  }\n"
            << "}\n";

        return out.str();
    }

    static std::string BuildRuntimeBackendResolvedJson()
    {
        const auto& target = TargetFor(g_target);

        std::vector<RuntimeBackendResolvedSubmit> submits;
        std::vector<RuntimeUiDrawCall> drawCalls;
        std::vector<RuntimeGpuSubmitCall> gpuSubmitCalls;
        std::unordered_map<uint32_t, RuntimeTextureDescriptorSemantic> textureDescriptors;
        std::unordered_map<uint32_t, RuntimeSamplerDescriptorSemantic> samplerDescriptors;
        std::unordered_map<uint32_t, RuntimeVendorTextureResourceView> textureResourceViews;
        std::unordered_map<uint32_t, RuntimeVendorSamplerResourceView> samplerResourceViews;
        uint64_t frame = g_presentedFrameCount;
        uint32_t droppedCount = 0;
        uint32_t sequence = 0;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            submits = g_runtimeBackendResolvedSubmits;
            drawCalls = g_runtimeUiDrawCalls;
            gpuSubmitCalls = g_runtimeGpuSubmitCalls;
            textureDescriptors = g_runtimeTextureDescriptorSemantics;
            samplerDescriptors = g_runtimeSamplerDescriptorSemantics;
            textureResourceViews = g_runtimeVendorTextureResourceViews;
            samplerResourceViews = g_runtimeVendorSamplerResourceViews;
            if (g_runtimeBackendResolvedFrame != UINT64_MAX)
                frame = g_runtimeBackendResolvedFrame;
            else if (g_runtimeGpuSubmitFrame != UINT64_MAX)
                frame = g_runtimeGpuSubmitFrame;
            droppedCount = g_runtimeBackendResolvedDroppedCount;
            sequence = g_runtimeBackendResolvedSequence;
        }

        uint32_t resolvedPipelineCount = 0;
        uint32_t blendEnabledCount = 0;
        uint32_t rt0KnownCount = 0;
        uint32_t framebufferKnownCount = 0;
        uint32_t indexedCount = 0;
        uint32_t sourceOverCount = 0;
        uint32_t additiveCount = 0;
        uint32_t opaqueCount = 0;
        uint32_t customBlendCount = 0;
        uint32_t framebufferRegisteredCount = 0;
        for (const auto& submit : submits)
        {
            if (submit.resolvedPipelineKnown)
                ++resolvedPipelineCount;
            if (submit.blendEnabled)
                ++blendEnabledCount;
            if (submit.renderTargetFormat0 != 0)
                ++rt0KnownCount;
            if (submit.activeFramebufferKnown)
                ++framebufferKnownCount;
            if (submit.indexed)
                ++indexedCount;

            const BackendMaterialParityHint hint = RuntimeBackendMaterialParityHint(submit);
            if (hint.sourceOverAlpha)
                ++sourceOverCount;
            if (hint.additiveAlpha)
                ++additiveCount;
            if (hint.opaqueNoBlend)
                ++opaqueCount;
            if (hint.customBlend)
                ++customBlendCount;
            if (hint.framebufferRegistered)
                ++framebufferRegisteredCount;
        }

        const auto materialPairs = BuildRuntimeMaterialCorrelationPairs(drawCalls, gpuSubmitCalls);
        const std::string status = RuntimeBackendResolvedStatus(static_cast<uint32_t>(submits.size()));

        std::ostringstream out;
        out
            << "{\n"
            << "  \"ok\": true,\n"
            << "  \"source\": \"backend-resolved D3D12/Vulkan command-list submit hook\",\n"
            << "  \"version\": 1,\n"
            << "  \"frame\": " << frame << ",\n"
            << "  \"target\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"activeScreen\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"targetProject\": \"" << JsonEscape(target.primaryCsdScene) << "\",\n"
            << "  \"resolvedBackendStatus\": \"" << JsonEscape(status) << "\",\n"
            << "  \"backendResolvedSubmitCount\": " << submits.size() << ",\n"
            << "  \"capturedBackendResolvedSubmitCount\": " << submits.size() << ",\n"
            << "  \"droppedBackendResolvedSubmitCount\": " << droppedCount << ",\n"
            << "  \"backendResolvedSubmitSequence\": " << sequence << ",\n"
            << "  \"resolvedPipelineSubmitCount\": " << resolvedPipelineCount << ",\n"
            << "  \"blendEnabledSubmitCount\": " << blendEnabledCount << ",\n"
            << "  \"renderTargetFormat0KnownCount\": " << rt0KnownCount << ",\n"
            << "  \"framebufferKnownSubmitCount\": " << framebufferKnownCount << ",\n"
            << "  \"indexedSubmitCount\": " << indexedCount << ",\n"
            << "  \"materialPairCount\": " << materialPairs.size() << ",\n";
        BuildBackendMaterialParityHintsJson(
            out,
            submits,
            sourceOverCount,
            additiveCount,
            opaqueCount,
            customBlendCount,
            framebufferRegisteredCount);
        BuildBackendDescriptorSemanticsJson(
            out,
            materialPairs,
            textureDescriptors,
            samplerDescriptors);
        BuildBackendVendorResourceCaptureJson(
            out,
            materialPairs,
            textureResourceViews,
            samplerResourceViews);
        BuildBackendMaterialResourceViewParityJson(
            out,
            materialPairs,
            textureResourceViews,
            samplerResourceViews);
        out
            << "  \"backendResolvedJoinMethod\": \"same-frame-order-window\",\n"
            << "  \"sampleLimit\": " << kRuntimeBackendResolvedSubmitSampleLimit << ",\n"
            << "  \"backendResolvedSubmitOracle\": {\n"
            << "    \"source\": \"D3D12/Vulkan command-list draw hooks\",\n"
            << "    \"resolvedBackendStatus\": \"" << JsonEscape(status) << "\",\n"
            << "    \"backendResolvedJoinMethod\": \"same-frame-order-window\",\n"
            << "    \"backendResolvedSubmitCount\": " << submits.size() << ",\n"
            << "    \"resolvedPipelineSubmitCount\": " << resolvedPipelineCount << ",\n"
            << "    \"blendEnabledSubmitCount\": " << blendEnabledCount << ",\n"
            << "    \"renderTargetFormat0KnownCount\": " << rt0KnownCount << ",\n"
            << "    \"framebufferKnownSubmitCount\": " << framebufferKnownCount << ",\n"
            << "    \"sourceOverSubmitCount\": " << sourceOverCount << ",\n"
            << "    \"additiveSubmitCount\": " << additiveCount << ",\n"
            << "    \"opaqueSubmitCount\": " << opaqueCount << ",\n"
            << "    \"customBlendSubmitCount\": " << customBlendCount << ",\n"
            << "    \"framebufferRegisteredSubmitCount\": " << framebufferRegisteredCount << ",\n"
            << "    \"materialPairCount\": " << materialPairs.size() << ",\n"
            << "    \"submits\": ";
        AppendRuntimeBackendResolvedSubmits(out, submits);
        out
            << "\n"
            << "  }\n"
            << "}\n";

        return out.str();
    }

    static std::string BuildRuntimeGpuSubmitJson()
    {
        const auto& target = TargetFor(g_target);

        std::vector<RuntimeGpuSubmitCall> calls;
        uint64_t frame = 0;
        uint32_t droppedCount = 0;
        uint32_t sequence = 0;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            calls = g_runtimeGpuSubmitCalls;
            frame = g_runtimeGpuSubmitFrame == UINT64_MAX ? g_presentedFrameCount : g_runtimeGpuSubmitFrame;
            droppedCount = g_runtimeGpuSubmitDroppedCount;
            sequence = g_runtimeGpuSubmitSequence;
        }

        uint32_t texturedCount = 0;
        uint32_t alphaBlendCount = 0;
        uint32_t indexedCount = 0;
        for (const auto& call : calls)
        {
            if (call.texture2DDescriptorIndex != 0)
                ++texturedCount;
            if (call.alphaBlendEnable)
                ++alphaBlendCount;
            if (call.indexed)
                ++indexedCount;
        }

        const std::string status = RuntimeGpuSubmitStatus(static_cast<uint32_t>(calls.size()));

        std::ostringstream out;
        out
            << "{\n"
            << "  \"ok\": true,\n"
            << "  \"source\": \"render-thread material submit hook\",\n"
            << "  \"version\": 1,\n"
            << "  \"frame\": " << frame << ",\n"
            << "  \"target\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"activeScreen\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"targetProject\": \"" << JsonEscape(target.primaryCsdScene) << "\",\n"
            << "  \"backendSubmitStatus\": \"" << JsonEscape(status) << "\",\n"
            << "  \"backendSubmitCallCount\": " << calls.size() << ",\n"
            << "  \"capturedSubmitCallCount\": " << calls.size() << ",\n"
            << "  \"droppedSubmitCallCount\": " << droppedCount << ",\n"
            << "  \"submitCallSequence\": " << sequence << ",\n"
            << "  \"texturedSubmitCallCount\": " << texturedCount << ",\n"
            << "  \"alphaBlendSubmitCallCount\": " << alphaBlendCount << ",\n"
            << "  \"indexedSubmitCallCount\": " << indexedCount << ",\n"
            << "  \"sampleLimit\": " << kRuntimeGpuSubmitCallSampleLimit << ",\n"
            << "  \"gpuSubmitOracle\": {\n"
            << "    \"source\": \"render-thread material submit hook\",\n"
            << "    \"backendSubmitStatus\": \"" << JsonEscape(status) << "\",\n"
            << "    \"rawBackendStatus\": \"raw D3D12/Vulkan backend capture pending\",\n"
            << "    \"backendSubmitCallCount\": " << calls.size() << ",\n"
            << "    \"droppedSubmitCallCount\": " << droppedCount << ",\n"
            << "    \"texturedSubmitCallCount\": " << texturedCount << ",\n"
            << "    \"alphaBlendSubmitCallCount\": " << alphaBlendCount << ",\n"
            << "    \"submitCalls\": ";
        AppendRuntimeGpuSubmitCalls(out, calls);
        out
            << "\n"
            << "  }\n"
            << "}\n";

        return out.str();
    }

    static std::string BuildRuntimeMaterialCorrelationJson()
    {
        const auto& target = TargetFor(g_target);

        std::vector<RuntimeUiDrawCall> drawCalls;
        std::vector<RuntimeGpuSubmitCall> submitCalls;
        std::vector<RuntimeRawBackendCommand> rawBackendCommands;
        std::vector<RuntimeBackendResolvedSubmit> backendResolvedSubmits;
        uint64_t frame = g_presentedFrameCount;
        uint32_t runtimeUiDrawDroppedCount = 0;
        uint32_t runtimeGpuSubmitDroppedCount = 0;
        uint32_t runtimeRawBackendDroppedCount = 0;
        uint32_t runtimeBackendResolvedDroppedCount = 0;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            drawCalls = g_runtimeUiDrawCalls;
            submitCalls = g_runtimeGpuSubmitCalls;
            rawBackendCommands = g_runtimeRawBackendCommands;
            backendResolvedSubmits = g_runtimeBackendResolvedSubmits;
            if (g_runtimeGpuSubmitFrame != UINT64_MAX)
                frame = g_runtimeGpuSubmitFrame;
            else if (g_runtimeUiDrawListFrame != UINT64_MAX)
                frame = g_runtimeUiDrawListFrame;
            runtimeUiDrawDroppedCount = g_runtimeUiDrawCallDroppedCount;
            runtimeGpuSubmitDroppedCount = g_runtimeGpuSubmitDroppedCount;
            runtimeRawBackendDroppedCount = g_runtimeRawBackendCommandDroppedCount;
            runtimeBackendResolvedDroppedCount = g_runtimeBackendResolvedDroppedCount;
        }

        const auto pairs = BuildRuntimeMaterialCorrelationPairs(drawCalls, submitCalls);
        uint32_t alphaBlendPairCount = 0;
        uint32_t additivePairCount = 0;
        uint32_t linearFilterPairCount = 0;
        uint32_t pointFilterPairCount = 0;
        for (const auto& pair : pairs)
        {
            if (pair.alphaBlendEnable)
                ++alphaBlendPairCount;
            if (pair.additiveBlend)
                ++additivePairCount;
            if (pair.linearFilter)
                ++linearFilterPairCount;
            if (pair.pointFilter)
                ++pointFilterPairCount;
        }

        const std::string backendSubmitStatus = RuntimeGpuSubmitStatus(static_cast<uint32_t>(submitCalls.size()));
        const std::string rawBackendCommandStatus = RuntimeRawBackendCommandStatus(static_cast<uint32_t>(rawBackendCommands.size()));
        const std::string resolvedBackendStatus = RuntimeBackendResolvedStatus(static_cast<uint32_t>(backendResolvedSubmits.size()));

        std::ostringstream out;
        out
            << "{\n"
            << "  \"ok\": true,\n"
            << "  \"source\": \"runtime CSD draw-list + render-thread material submit correlation\",\n"
            << "  \"version\": 1,\n"
            << "  \"frame\": " << frame << ",\n"
            << "  \"target\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"activeScreen\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"targetProject\": \"" << JsonEscape(target.primaryCsdScene) << "\",\n"
            << "  \"correlationStatus\": \"same-frame-order-window active; raw D3D12/Vulkan command capture pending\",\n"
            << "  \"backendSubmitStatus\": \"" << JsonEscape(backendSubmitStatus) << "\",\n"
            << "  \"rawBackendCommandStatus\": \"" << JsonEscape(rawBackendCommandStatus) << "\",\n"
            << "  \"resolvedBackendStatus\": \"" << JsonEscape(resolvedBackendStatus) << "\",\n"
            << "  \"uiDrawCallCount\": " << drawCalls.size() << ",\n"
            << "  \"backendSubmitCallCount\": " << submitCalls.size() << ",\n"
            << "  \"rawBackendCommandCount\": " << rawBackendCommands.size() << ",\n"
            << "  \"backendResolvedSubmitCount\": " << backendResolvedSubmits.size() << ",\n"
            << "  \"correlatedPairCount\": " << pairs.size() << ",\n"
            << "  \"alphaBlendPairCount\": " << alphaBlendPairCount << ",\n"
            << "  \"additivePairCount\": " << additivePairCount << ",\n"
            << "  \"linearFilterPairCount\": " << linearFilterPairCount << ",\n"
            << "  \"pointFilterPairCount\": " << pointFilterPairCount << ",\n"
            << "  \"droppedDrawCallCount\": " << runtimeUiDrawDroppedCount << ",\n"
            << "  \"droppedSubmitCallCount\": " << runtimeGpuSubmitDroppedCount << ",\n"
            << "  \"droppedRawBackendCommandCount\": " << runtimeRawBackendDroppedCount << ",\n"
            << "  \"droppedBackendResolvedSubmitCount\": " << runtimeBackendResolvedDroppedCount << ",\n"
            << "  \"materialCorrelationOracle\": {\n"
            << "    \"source\": \"ui-draw-list + ui-gpu-submit\",\n"
            << "    \"correlationMethod\": \"same-frame-order-window\",\n"
            << "    \"backendResolvedJoinMethod\": \"same-frame-order-window\",\n"
            << "    \"blendSemantics\": \"named Xenos/D3D-ish material semantics\",\n"
            << "    \"samplerSemantics\": \"named Xenos/D3D-ish material semantics\",\n"
            << "    \"rawBackendCommandStatus\": \"" << JsonEscape(rawBackendCommandStatus) << "\",\n"
            << "    \"resolvedBackendStatus\": \"" << JsonEscape(resolvedBackendStatus) << "\",\n"
            << "    \"uiDrawCallCount\": " << drawCalls.size() << ",\n"
            << "    \"backendSubmitCallCount\": " << submitCalls.size() << ",\n"
            << "    \"backendResolvedSubmitCount\": " << backendResolvedSubmits.size() << ",\n"
            << "    \"correlatedPairCount\": " << pairs.size() << ",\n"
            << "    \"pairs\": ";
        AppendRuntimeMaterialCorrelationPairs(out, pairs);
        out
            << "\n"
            << "  }\n"
            << "}\n";

        return out.str();
    }

    static std::string BuildRuntimeUiDrawListJson()
    {
        const auto& target = TargetFor(g_target);

        std::vector<RuntimeUiDrawCall> calls;
        uint64_t frame = 0;
        uint32_t droppedCount = 0;
        uint32_t sequence = 0;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            calls = g_runtimeUiDrawCalls;
            frame = g_runtimeUiDrawListFrame == UINT64_MAX ? g_presentedFrameCount : g_runtimeUiDrawListFrame;
            droppedCount = g_runtimeUiDrawCallDroppedCount;
            sequence = g_runtimeUiDrawCallSequence;
        }

        uint32_t texturedCount = 0;
        uint32_t noTextureCount = 0;
        for (const auto& call : calls)
        {
            if (call.textured)
                ++texturedCount;
            else
                ++noTextureCount;
        }

        const std::string status = RuntimeUiDrawListStatus(static_cast<uint32_t>(calls.size()));

        std::ostringstream out;
        out
            << "{\n"
            << "  \"ok\": true,\n"
            << "  \"source\": \"runtime CSD platform draw hook\",\n"
            << "  \"version\": 1,\n"
            << "  \"frame\": " << frame << ",\n"
            << "  \"target\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"activeScreen\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"targetProject\": \"" << JsonEscape(target.primaryCsdScene) << "\",\n"
            << "  \"runtimeDrawListStatus\": \"" << JsonEscape(status) << "\",\n"
            << "  \"gpuDrawListStatus\": \"GPU backend submit pending\",\n"
            << "  \"drawCallCount\": " << calls.size() << ",\n"
            << "  \"capturedDrawCallCount\": " << calls.size() << ",\n"
            << "  \"droppedDrawCallCount\": " << droppedCount << ",\n"
            << "  \"drawCallSequence\": " << sequence << ",\n"
            << "  \"texturedDrawCallCount\": " << texturedCount << ",\n"
            << "  \"noTextureDrawCallCount\": " << noTextureCount << ",\n"
            << "  \"sampleLimit\": " << kRuntimeUiDrawCallSampleLimit << ",\n"
            << "  \"uiDrawListOracle\": {\n"
            << "    \"source\": \"SWA::CCsdPlatformMirage::Draw/DrawNoTex hook\",\n"
            << "    \"runtimeDrawListStatus\": \"" << JsonEscape(status) << "\",\n"
            << "    \"gpuDrawListStatus\": \"GPU backend submit pending\",\n"
            << "    \"drawCallCount\": " << calls.size() << ",\n"
            << "    \"droppedDrawCallCount\": " << droppedCount << ",\n"
            << "    \"texturedDrawCallCount\": " << texturedCount << ",\n"
            << "    \"noTextureDrawCallCount\": " << noTextureCount << ",\n"
            << "    \"drawCalls\": ";
        AppendRuntimeUiDrawCalls(out, calls);
        out
            << "\n"
            << "  }\n"
            << "}\n";

        return out.str();
    }

    static std::string BuildUiOracleJson()
    {
        const auto& target = TargetFor(g_target);
        const auto csd = BuildCsdLiveInspectorSnapshot();
        const auto csdProjectTree = BuildCsdProjectTreeInspectorSnapshot();
        const auto loading = BuildLoadingLiveInspectorSnapshot();
        const auto sonicHud = BuildSonicHudLiveInspectorSnapshot();
        const auto pauseGeneralSave = BuildPauseGeneralSaveLiveInspectorSnapshot();
        const std::string activationEvent = UiOracleActivationEventName(target.id);
        const bool ready = UiOracleTargetReady(target.id);
        const std::string inputLockState = std::string(ready ? "released:" : "until:") + activationEvent;
        std::vector<RuntimeUiDrawCall> drawCalls;
        std::vector<RuntimeGpuSubmitCall> gpuSubmitCalls;
        uint32_t runtimeUiDrawDroppedCount = 0;
        uint32_t runtimeGpuSubmitDroppedCount = 0;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            drawCalls = g_runtimeUiDrawCalls;
            gpuSubmitCalls = g_runtimeGpuSubmitCalls;
            runtimeUiDrawDroppedCount = g_runtimeUiDrawCallDroppedCount;
            runtimeGpuSubmitDroppedCount = g_runtimeGpuSubmitDroppedCount;
        }
        const std::string runtimeDrawListStatus = RuntimeUiDrawListStatus(static_cast<uint32_t>(drawCalls.size()));
        const std::string backendSubmitStatus = RuntimeGpuSubmitStatus(static_cast<uint32_t>(gpuSubmitCalls.size()));

        std::ostringstream out;
        out
            << "{\n"
            << "  \"ok\": true,\n"
            << "  \"source\": \"live-bridge ui-only oracle\",\n"
            << "  \"version\": 1,\n"
            << "  \"frame\": " << g_presentedFrameCount << ",\n"
            << "  \"target\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"activeScreen\": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"activeProject\": \"" << JsonEscape(csdProjectTree.activeProject) << "\",\n"
            << "  \"targetProject\": \"" << JsonEscape(target.primaryCsdScene) << "\",\n"
            << "  \"route\": \"" << JsonEscape(g_routeStatus) << "\",\n"
            << "  \"activeMotionName\": \"" << JsonEscape(g_routeStatus) << "\",\n"
            << "  \"activeScenes\": ";
        AppendUiOracleActiveScenePaths(out, csdProjectTree);
        out
            << ",\n"
            << "  \"cursorOwner\": \"" << JsonEscape(UiOracleCursorOwnerLabel(target.id, loading, sonicHud, pauseGeneralSave)) << "\",\n"
            << "  \"transitionBand\": \"" << JsonEscape(UiOracleTransitionBand(target.id)) << "\",\n"
            << "  \"inputLockState\": \"" << JsonEscape(inputLockState) << "\",\n"
            << "  \"activationEvent\": \"" << JsonEscape(activationEvent) << "\",\n"
            << "  \"runtimeDrawListStatus\": \"" << JsonEscape(runtimeDrawListStatus) << "\",\n"
            << "  \"uiLayerOracle\": {\n"
            << "    \"source\": \"" << JsonEscape(csdProjectTree.source) << "\",\n"
            << "    \"activeProject\": \"" << JsonEscape(csdProjectTree.activeProject) << "\",\n"
            << "    \"targetProject\": \"" << JsonEscape(target.primaryCsdScene) << "\",\n"
            << "    \"projectKnown\": " << (csdProjectTree.projectKnown ? "true" : "false") << ",\n"
            << "    \"projectAddress\": \"" << JsonEscape(HexU32(csdProjectTree.projectAddress)) << "\",\n"
            << "    \"rootNodeAddress\": \"" << JsonEscape(HexU32(csdProjectTree.rootNodeAddress)) << "\",\n"
            << "    \"sceneCount\": " << csdProjectTree.sceneCount << ",\n"
            << "    \"nodeCount\": " << csdProjectTree.nodeCount << ",\n"
            << "    \"layerCount\": " << csdProjectTree.layerCount << ",\n"
            << "    \"runtimeSceneMotionFrame\": ";
        if (csd.sceneMotionKnown)
            out << csd.sceneMotionFrame;
        else
            out << "null";
        out
            << ",\n"
            << "    \"runtimeSceneMotionRepeatTypeLabel\": \"" << JsonEscape(MotionRepeatTypeLabel(csd.sceneMotionRepeatType)) << "\",\n"
            << "    \"runtimeDrawListStatus\": \"" << JsonEscape(runtimeDrawListStatus) << "\",\n"
            << "    \"scenes\": ";
        AppendCsdTreeEntries(out, csdProjectTree.scenes, "sceneAddress", "projectAddress", "castNodeCount", "castCount");
        out
            << ",\n"
            << "    \"layers\": ";
        AppendCsdTreeEntries(out, csdProjectTree.layers, "layerAddress", "castNodeAddress", "castNodeIndex", "castIndex");
        out
            << "\n"
            << "  },\n"
            << "  \"uiDrawListOracle\": {\n"
            << "    \"source\": \"SWA::CCsdPlatformMirage::Draw/DrawNoTex hook\",\n"
            << "    \"runtimeDrawListStatus\": \"" << JsonEscape(runtimeDrawListStatus) << "\",\n"
            << "    \"gpuDrawListStatus\": \"GPU backend submit pending\",\n"
            << "    \"drawCallCount\": " << drawCalls.size() << ",\n"
            << "    \"droppedDrawCallCount\": " << runtimeUiDrawDroppedCount << ",\n"
            << "    \"drawCalls\": ";
        AppendRuntimeUiDrawCalls(out, drawCalls);
        out
            << "\n"
            << "  },\n"
            << "  \"gpuSubmitOracle\": {\n"
            << "    \"source\": \"render-thread material submit hook\",\n"
            << "    \"backendSubmitStatus\": \"" << JsonEscape(backendSubmitStatus) << "\",\n"
            << "    \"rawBackendStatus\": \"raw D3D12/Vulkan backend capture pending\",\n"
            << "    \"backendSubmitCallCount\": " << gpuSubmitCalls.size() << ",\n"
            << "    \"droppedSubmitCallCount\": " << runtimeGpuSubmitDroppedCount << ",\n"
            << "    \"submitCalls\": ";
        AppendRuntimeGpuSubmitCalls(out, gpuSubmitCalls);
        out
            << "\n"
            << "  },\n"
            << "  \"readiness\": {\n"
            << "    \"ready\": " << (ready ? "true" : "false") << ",\n"
            << "    \"titleMenuVisible\": " << (g_titleMenuVisualReady ? "true" : "false") << ",\n"
            << "    \"loadingActive\": " << (g_loadingDisplayWasActive ? "true" : "false") << ",\n"
            << "    \"stageTargetReady\": " << (g_loggedStageTargetReady ? "true" : "false") << ",\n"
            << "    \"stageReadyEvent\": \"" << JsonEscape(g_lastStageReadyEventName) << "\"\n"
            << "  }\n"
            << "}\n";

        return out.str();
    }

    static void AppendRecentEvents(std::ostringstream& out)
    {
        std::lock_guard<std::mutex> lock(g_liveBridgeMutex);

        out << "[";
        for (size_t i = 0; i < g_recentEvidenceEvents.size(); ++i)
        {
            if (i != 0)
                out << ",";

            out << g_recentEvidenceEvents[i];
        }
        out << "]";
    }

    std::string BuildLiveStateJson()
    {
        const auto& target = TargetFor(g_target);
        std::ostringstream out;

        out
            << "{\n"
            << "  \"time\": " << SecondsSinceStart() << ",\n"
            << "  \"frame\": " << g_presentedFrameCount << ",\n"
            << "  " << kLiveStateTargetFieldName << ": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"targetLabel\": \"" << JsonEscape(target.label) << "\",\n"
            << "  \"targetCsd\": \"" << JsonEscape(target.primaryCsdScene) << "\",\n"
            << "  \"sourceFamily\": \"" << JsonEscape(target.sourceFamily) << "\",\n"
            << "  " << kLiveStateRouteFieldName << ": \"" << JsonEscape(g_routeStatus) << "\",\n"
            << "  \"routePending\": " << (g_routePending ? "true" : "false") << ",\n"
            << "  \"routeGeneration\": " << g_routeGeneration << ",\n"
            << "  \"routeResetCount\": " << g_routeResetCount << ",\n"
            << "  \"routePolicy\": \"" << JsonEscape(RoutePolicyLabel()) << "\",\n"
            << "  \"stageHarness\": \"" << JsonEscape(GetStageHarnessLabel()) << "\",\n"
            << "  " << kLiveStateStageGameModeAddressFieldName << ": \"" << JsonEscape(HexU32(g_lastStageGameModeAddress)) << "\",\n"
            << "  \"stageContextFrame\": " << g_lastStageContextFrame << ",\n"
            << "  \"stageReadyEvent\": \"" << JsonEscape(g_lastStageReadyEventName) << "\",\n"
            << "  \"stageReadyFrame\": " << g_lastStageReadyFrame << ",\n"
            << "  \"targetCsdObserved\": " << (g_targetCsdObserved ? "true" : "false") << ",\n"
            << "  " << kLiveStateNativeCaptureStatusFieldName << ": \"" << JsonEscape(NativeFrameCaptureStatusLabel()) << "\",\n"
            << "  \"lastNativeFrameCapturePath\": \"" << JsonEscape(g_lastNativeFrameCapturePath) << "\",\n"
            << "  \"lastNativeFrameCaptureFailure\": \"" << JsonEscape(g_lastNativeFrameCaptureFailure) << "\",\n"
            << "  \"uiOnlyRenderTargetCaptureStatus\": \"" << JsonEscape(UiOnlyRenderTargetCaptureStatusLabel()) << "\",\n"
            << "  \"uiOnlyLayerCaptureStatus\": \"" << JsonEscape(UiOnlyRenderTargetCaptureStatusLabel()) << "\",\n"
            << "  \"uiOnlyLayerIsolationStatus\": \"" << JsonEscape(UiOnlyLayerIsolationStatusLabel()) << "\",\n"
            << "  \"uiOnlyRenderTargetCapturePath\": \"" << JsonEscape(g_lastUiOnlyRenderTargetCapturePath) << "\",\n"
            << "  \"uiOnlyRenderTargetCaptureSource\": \"" << JsonEscape(g_lastUiOnlyRenderTargetCaptureSource) << "\",\n"
            << "  \"lastCsdProject\": \"" << JsonEscape(g_lastCsdProjectName) << "\",\n"
            << "  \"lastCsdProjectFrame\": " << g_lastCsdProjectFrame << ",\n"
            << "  \"loadingRequestType\": "
            << (g_lastLoadingRequestType == UINT32_MAX ? -1 : static_cast<int32_t>(g_lastLoadingRequestType))
            << ",\n"
            << "  \"loadingDisplayType\": "
            << (g_lastLoadingDisplayType == UINT32_MAX ? -1 : static_cast<int32_t>(g_lastLoadingDisplayType))
            << ",\n"
            << "  \"loadingDisplayActive\": " << (g_loadingDisplayWasActive ? "true" : "false") << ",\n"
            << "  \"titleMenuVisualReady\": " << (g_titleMenuVisualReady ? "true" : "false") << ",\n"
            << "  \"titleMenuPressStartAccepted\": " << (g_titleMenuPressStartAccepted ? "true" : "false") << ",\n"
            << "  \"titleMenuPostPressStartHeld\": " << (g_titleMenuPostPressStartHeld ? "true" : "false") << ",\n"
            << "  \"titleMenuStableFrames\": " << TitleMenuStableFrames() << ",\n"
            << "  \"titleIntroHookObserved\": " << (g_loggedIntroHook ? "true" : "false") << ",\n"
            << "  \"titleMenuHookObserved\": " << (g_loggedMenuHook ? "true" : "false") << ",\n"
            << "  \"lastTitleIntroContext\": \"" << JsonEscape(g_lastTitleIntroContextDetail) << "\",\n"
            << "  \"lastTitleMenuContext\": \"" << JsonEscape(g_lastTitleMenuContextDetail) << "\",\n"
            << "  \"lastStageTitleContext\": \"" << JsonEscape(g_lastStageTitleContextDetail) << "\",\n"
            << "  \"title\": {\n"
            << "    \"introContextAddress\": \"" << JsonEscape(HexU32(g_titleIntroInspector.contextAddress)) << "\",\n"
            << "    \"introStateMachineAddress\": \"" << JsonEscape(HexU32(g_titleIntroInspector.stateMachineAddress)) << "\",\n"
            << "    \"ownerTitleContextAddress\": \"" << JsonEscape(HexU32(g_titleOwnerInspector.titleContextAddress)) << "\",\n"
            << "    \"ownerTitleCsdAddress\": \"" << JsonEscape(HexU32(g_titleOwnerInspector.titleCsdAddress)) << "\",\n"
            << "    \"ownerReady\": " << (g_titleOwnerInspector.ownerReady ? "true" : "false") << ",\n"
            << "    \"menuCursor\": " << g_titleMenuInspector.menuCursor << ",\n"
            << "    \"contextPhase\": " << g_titleMenuInspector.contextPhase << "\n"
            << "  },\n"
            << "  \"readiness\": {\n"
            << "    \"titleMenuVisible\": " << (g_titleMenuVisualReady ? "true" : "false") << ",\n"
            << "    \"loadingActive\": " << (g_loadingDisplayWasActive ? "true" : "false") << ",\n"
            << "    \"stageContextObserved\": " << (g_stageContextObserved ? "true" : "false") << ",\n"
            << "    \"targetCsdObserved\": " << (g_targetCsdObserved ? "true" : "false") << ",\n"
            << "    \"stageTargetReady\": " << (g_loggedStageTargetReady ? "true" : "false") << "\n"
            << "  },\n"
            << "  \"liveBridge\": {\n"
            << "    \"enabled\": " << (g_liveBridgeEnabled ? "true" : "false") << ",\n"
            << "    \"transport\": \"windows-named-pipe\",\n"
            << "    \"name\": \"" << JsonEscape(g_liveBridgeName) << "\",\n"
            << "    \"started\": " << (g_liveBridgeStarted.load() ? "true" : "false") << ",\n"
            << "    \"lastCommand\": \"" << JsonEscape(g_lastLiveBridgeCommand) << "\",\n"
            << "    \"commandCount\": " << g_liveBridgeCommandCount << ",\n"
            << "    \"commands\": ";
        AppendStringArray(out, { "state", "events", "route-status", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route <target>", "reset", "set-global <name> <0|1>", "capture", "help" });
        out
            << "\n"
            << "  },\n"
            << "  \"capabilities\": ";
        AppendStringArray(out, {
            "current target",
            "route/event latch",
            "title/menu/loading/stage/HUD readiness",
            "CSD project and scene pointers",
            "runtime UI-only CSD oracle",
            "SGlobals toggles",
            "debug-menu fork-derived typed fields",
            "command channel"
        });
        out
            << ",\n"
            << "  \"sglobals\": {\n"
            << "    \"render\": ";
        AppendGuestBoolArray(out, kGuestRenderGlobals);
        out
            << ",\n"
            << "    \"debugDraw\": ";
        AppendGuestBoolArray(out, kGuestDebugDrawGlobals);
        out
            << "\n"
            << "  },\n"
            << "  \"debugForkTypedFields\": ";
        AppendDebugForkTypedFields(out);
        out
            << ",\n"
            << "  " << kLiveStateTypedInspectorsFieldName << ": ";
        AppendTypedInspectors(out);
        out
            << ",\n"
            << "  \"recentEvents\": ";
        AppendRecentEvents(out);
        out
            << "\n"
            << "}\n";

        return out.str();
    }

    void WriteLiveStateSnapshot()
    {
        if (!g_isEnabled || g_evidenceDirectory.empty())
            return;

        std::error_code ec;
        std::filesystem::create_directories(g_evidenceDirectory, ec);

        if (ec)
            return;

        const auto liveStatePath = LiveStateSnapshotPath();
        if (liveStatePath.empty())
            return;

        std::ofstream out(liveStatePath, std::ios::trunc);
        if (!out)
            return;

        out << BuildLiveStateJson();

        g_lastLiveStateSnapshotFrame = g_presentedFrameCount;
    }

    static uint64_t TitleMenuStableFrames()
    {
        if (g_titleMenuStableFrameStart == 0 || g_presentedFrameCount < g_titleMenuStableFrameStart)
            return 0;

        return g_presentedFrameCount - g_titleMenuStableFrameStart;
    }

    static bool IsTitleMenuContextReady(
        uint32_t context472,
        uint32_t context488,
        uint32_t contextPhase,
        uint32_t menuCursor,
        bool menuField54)
    {
        return g_titleMenuPressStartAccepted &&
            g_titleMenuPostPressStartHeld &&
            g_titleMenuPostPressStartReadyLogged &&
            menuField54 &&
            context488 != 0 &&
            context472 == 0 &&
            contextPhase == 0 &&
            menuCursor != 0;
    }

    static void StoreTitleMenuInspectorSnapshot(
        uint32_t context472,
        uint32_t context480,
        uint32_t context488,
        uint32_t contextPhase,
        uint8_t contextFlag580,
        uint32_t menuCursor,
        bool menuField3C,
        bool menuField54,
        bool menuField9A)
    {
        g_titleMenuInspector.valid = true;
        g_titleMenuInspector.context472 = context472;
        g_titleMenuInspector.context480 = context480;
        g_titleMenuInspector.context488 = context488;
        g_titleMenuInspector.contextPhase = contextPhase;
        g_titleMenuInspector.contextFlag580 = contextFlag580;
        g_titleMenuInspector.menuCursor = menuCursor;
        g_titleMenuInspector.menuField3C = menuField3C;
        g_titleMenuInspector.menuField54 = menuField54;
        g_titleMenuInspector.menuField9A = menuField9A;
        g_titleMenuInspector.postPressStartMenuReady = IsTitleMenuContextReady(
            context472,
            context488,
            contextPhase,
            menuCursor,
            menuField54);
        g_titleMenuInspector.stableFrames = TitleMenuStableFrames();
        g_titleMenuInspector.frame = g_presentedFrameCount;
    }

    static bool TrySetTarget(std::string_view token)
    {
        if (token == "title")
            token = "title-loop";
        else if (token == "menu")
            token = "title-menu";
        else if (token == "options" || token == "option")
            token = "title-options";
        else if (token == "hud")
            token = "sonic-hud";
        else if (token == "prov-hud" || token == "tornado-hud")
            token = "extra-stage-hud";
        else if (token == "worldmap")
            token = "world-map";

        for (const auto& target : kRuntimeTargets)
        {
            if (target.token == token)
            {
                g_target = target.id;
                return true;
            }
        }

        return false;
    }

    void ConfigureFromCommandLine(int argc, char* argv[])
    {
        for (int i = 1; i < argc; ++i)
        {
            std::string_view arg(argv[i]);

            if (arg == "--ui-lab")
            {
                g_isEnabled = true;
                continue;
            }

            if (arg == "--ui-lab-observer")
            {
                g_isEnabled = true;
                g_observerMode = true;
                g_routeStatus = "observer mode";
                continue;
            }

            if (arg == "--ui-lab-hide-overlay")
            {
                g_hideOverlay = true;
                continue;
            }

            if (arg == "--ui-lab-overlay")
            {
                if ((i + 1) < argc)
                {
                    const std::string_view value(argv[++i]);
                    g_hideOverlay = value == "off" || value == "false" || value == "0" || value == "hidden";
                }
                else
                {
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-overlay was provided without a value.");
                }

                continue;
            }

            constexpr std::string_view overlayPrefix = "--ui-lab-overlay=";
            if (arg.starts_with(overlayPrefix))
            {
                const auto value = arg.substr(overlayPrefix.size());
                g_hideOverlay = value == "off" || value == "false" || value == "0" || value == "hidden";
                continue;
            }

            if (arg == "--ui-lab-route-policy")
            {
                g_isEnabled = true;

                if ((i + 1) < argc && TrySetRoutePolicy(argv[i + 1]))
                    ++i;
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-route-policy was provided without a known policy.");

                continue;
            }

            constexpr std::string_view routePolicyPrefix = "--ui-lab-route-policy=";
            if (arg.starts_with(routePolicyPrefix))
            {
                g_isEnabled = true;
                const auto policy = arg.substr(routePolicyPrefix.size());

                if (!TrySetRoutePolicy(policy))
                    LOGFN_WARNING("SWARD UI Lab: unknown route policy '{}'.", std::string(policy));

                continue;
            }

            if (arg == "--ui-lab-stage-title-owner-direct-fallback")
            {
                g_isEnabled = true;
                g_stageTitleOwnerDirectStateFallbackEnabled = true;
                continue;
            }

            if (arg == "--ui-lab-live-bridge")
            {
                g_isEnabled = true;
                g_liveBridgeEnabled = true;
                continue;
            }

            if (arg == "--ui-lab-no-live-bridge")
            {
                g_isEnabled = true;
                g_liveBridgeEnabled = false;
                continue;
            }

            constexpr std::string_view liveBridgePrefix = "--ui-lab-live-bridge=";
            if (arg.starts_with(liveBridgePrefix))
            {
                g_isEnabled = true;
                const auto value = arg.substr(liveBridgePrefix.size());

                if (IsTruthy(value))
                    g_liveBridgeEnabled = true;
                else if (IsFalsy(value))
                    g_liveBridgeEnabled = false;
                else
                    LOGFN_WARNING("SWARD UI Lab: unknown live bridge value '{}'.", std::string(value));

                continue;
            }

            if (arg == "--ui-lab-live-bridge-name")
            {
                g_isEnabled = true;

                if ((i + 1) < argc)
                    g_liveBridgeName = argv[++i];
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-live-bridge-name was provided without a name.");

                continue;
            }

            constexpr std::string_view liveBridgeNamePrefix = "--ui-lab-live-bridge-name=";
            if (arg.starts_with(liveBridgeNamePrefix))
            {
                g_isEnabled = true;
                g_liveBridgeName = std::string(arg.substr(liveBridgeNamePrefix.size()));
                continue;
            }

            if (arg == "--ui-lab-screen")
            {
                g_isEnabled = true;

                if ((i + 1) < argc && TrySetTarget(argv[i + 1]))
                {
                    ++i;
                    g_routeTargetExplicit = true;
                }
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-screen was provided without a known target.");

                continue;
            }

            constexpr std::string_view screenPrefix = "--ui-lab-screen=";
            if (arg.starts_with(screenPrefix))
            {
                g_isEnabled = true;
                auto token = arg.substr(screenPrefix.size());

                if (!TrySetTarget(token))
                    LOGFN_WARNING("SWARD UI Lab: unknown screen target '{}'.", std::string(token));
                else
                    g_routeTargetExplicit = true;

                continue;
            }

            constexpr std::string_view labPrefix = "--ui-lab=";
            if (arg.starts_with(labPrefix))
            {
                g_isEnabled = true;
                auto token = arg.substr(labPrefix.size());

                if (!TrySetTarget(token))
                    LOGFN_WARNING("SWARD UI Lab: unknown screen target '{}'.", std::string(token));
                else
                    g_routeTargetExplicit = true;

                continue;
            }

            if (arg == "--ui-lab-stage")
            {
                g_isEnabled = true;

                if ((i + 1) < argc)
                    g_requestedStageHarness = argv[++i];
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-stage was provided without a stage token.");

                continue;
            }

            constexpr std::string_view stagePrefix = "--ui-lab-stage=";
            if (arg.starts_with(stagePrefix))
            {
                g_isEnabled = true;
                g_requestedStageHarness = std::string(arg.substr(stagePrefix.size()));
                continue;
            }

            if (arg == "--ui-lab-evidence-dir")
            {
                g_isEnabled = true;

                if ((i + 1) < argc)
                    g_evidenceDirectory = argv[++i];
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-evidence-dir was provided without a directory.");

                continue;
            }

            constexpr std::string_view evidencePrefix = "--ui-lab-evidence-dir=";
            if (arg.starts_with(evidencePrefix))
            {
                g_isEnabled = true;
                g_evidenceDirectory = std::string(arg.substr(evidencePrefix.size()));
                continue;
            }

            if (arg == "--ui-lab-native-capture")
            {
                g_isEnabled = true;
                g_nativeFrameCaptureEnabled = true;
                continue;
            }

            constexpr std::string_view nativeCapturePrefix = "--ui-lab-native-capture=";
            if (arg.starts_with(nativeCapturePrefix))
            {
                g_isEnabled = true;
                const auto value = arg.substr(nativeCapturePrefix.size());

                if (IsTruthy(value))
                    g_nativeFrameCaptureEnabled = true;
                else if (IsFalsy(value))
                    g_nativeFrameCaptureEnabled = false;
                else
                    LOGFN_WARNING("SWARD UI Lab: unknown native capture value '{}'.", std::string(value));

                continue;
            }

            if (arg == "--ui-lab-native-capture-dir")
            {
                g_isEnabled = true;
                g_nativeFrameCaptureEnabled = true;

                if ((i + 1) < argc)
                    g_nativeFrameCaptureDirectory = argv[++i];
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-native-capture-dir was provided without a directory.");

                continue;
            }

            if (arg == "--ui-lab-native-capture-count")
            {
                g_isEnabled = true;
                g_nativeFrameCaptureEnabled = true;

                if ((i + 1) < argc)
                    g_nativeFrameCaptureMaxCount = ParsePositiveU32(argv[++i], g_nativeFrameCaptureMaxCount);
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-native-capture-count was provided without a count.");

                continue;
            }

            constexpr std::string_view nativeCaptureCountPrefix = "--ui-lab-native-capture-count=";
            if (arg.starts_with(nativeCaptureCountPrefix))
            {
                g_isEnabled = true;
                g_nativeFrameCaptureEnabled = true;
                g_nativeFrameCaptureMaxCount = ParsePositiveU32(
                    arg.substr(nativeCaptureCountPrefix.size()),
                    g_nativeFrameCaptureMaxCount);
                continue;
            }

            if (arg == "--ui-lab-native-capture-interval-frames")
            {
                g_isEnabled = true;
                g_nativeFrameCaptureEnabled = true;

                if ((i + 1) < argc)
                    g_nativeFrameCaptureIntervalFrames = ParsePositiveU32(argv[++i], g_nativeFrameCaptureIntervalFrames);
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-native-capture-interval-frames was provided without a frame count.");

                continue;
            }

            constexpr std::string_view nativeCaptureIntervalPrefix = "--ui-lab-native-capture-interval-frames=";
            if (arg.starts_with(nativeCaptureIntervalPrefix))
            {
                g_isEnabled = true;
                g_nativeFrameCaptureEnabled = true;
                g_nativeFrameCaptureIntervalFrames = ParsePositiveU32(
                    arg.substr(nativeCaptureIntervalPrefix.size()),
                    g_nativeFrameCaptureIntervalFrames);
                continue;
            }

            constexpr std::string_view nativeCaptureDirPrefix = "--ui-lab-native-capture-dir=";
            if (arg.starts_with(nativeCaptureDirPrefix))
            {
                g_isEnabled = true;
                g_nativeFrameCaptureEnabled = true;
                g_nativeFrameCaptureDirectory = std::string(arg.substr(nativeCaptureDirPrefix.size()));
                continue;
            }

            if (arg == "--ui-lab-auto-exit")
            {
                if ((i + 1) < argc)
                    g_autoExitSeconds = std::strtod(argv[++i], nullptr);
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-auto-exit was provided without a seconds value.");

                continue;
            }

            constexpr std::string_view autoExitPrefix = "--ui-lab-auto-exit=";
            if (arg.starts_with(autoExitPrefix))
            {
                g_autoExitSeconds = std::strtod(std::string(arg.substr(autoExitPrefix.size())).c_str(), nullptr);
                continue;
            }
        }

        if (g_isEnabled)
        {
            if (g_nativeFrameCaptureEnabled && g_nativeFrameCaptureDirectory.empty())
                g_nativeFrameCaptureDirectory = g_evidenceDirectory;

            if (!g_observerMode && !g_routeTargetExplicit)
            {
                g_observerMode = true;
                g_routeStatus = "capture/evidence observer mode";
                WriteEvidenceEvent("capture-evidence-observer-mode");
            }

            if (!g_observerMode)
                RequestRouteToCurrentTarget();
            else
                WriteEvidenceEvent("observer-mode");

            const auto& target = TargetFor(g_target);
            LOGFN(
                "SWARD UI Lab enabled: mode={} target={} label={} csd={} family={} stage_context={}",
                g_observerMode ? "observer" : "route",
                target.token,
                target.label,
                target.primaryCsdScene,
                target.sourceFamily,
                target.requiresStageContext ? "yes" : "no");

            LOGFN("SWARD UI Lab route policy: {}", RoutePolicyLabel());

            if (target.requiresStageContext)
                LOGFN("SWARD UI Lab stage harness armed: requested_stage={}", g_requestedStageHarness);

            if (!g_evidenceDirectory.empty())
            {
                LOGFN("SWARD UI Lab evidence directory: {}", g_evidenceDirectory.string());
                WriteEvidenceEvent("configured");
            }

            if (g_nativeFrameCaptureEnabled)
            {
                LOGFN("SWARD UI Lab native frame capture directory: {}", g_nativeFrameCaptureDirectory.string());
                WriteEvidenceEvent("native-frame-capture-armed", g_nativeFrameCaptureDirectory.string());
            }

            if (g_liveBridgeEnabled)
                StartLiveBridge();

            WriteLiveStateSnapshot();
        }
    }

    void ApplyConfigOverrides()
    {
        if (!g_isEnabled)
            return;

        Config::ShowConsole = true;

        if (g_observerMode)
            return;

        Config::SkipIntroLogos = true;
        Config::DisableAutoSaveWarning = true;
    }

    bool IsEnabled()
    {
        return g_isEnabled;
    }

    bool IsObserverMode()
    {
        return g_observerMode;
    }

    bool IsNativeFrameCaptureEnabled()
    {
        return g_isEnabled && g_nativeFrameCaptureEnabled;
    }

    bool IsLiveBridgeEnabled()
    {
        return g_isEnabled && g_liveBridgeEnabled;
    }

    std::string_view GetLiveBridgeName()
    {
        return g_liveBridgeName;
    }

    bool ShouldBypassStartupPromptBlockers()
    {
        return g_isEnabled && !g_observerMode;
    }

    ScreenId GetTarget()
    {
        return g_target;
    }

    std::string_view GetTargetToken()
    {
        return TargetFor(g_target).token;
    }

    std::string_view GetTargetLabel()
    {
        return TargetFor(g_target).label;
    }

    std::string_view GetRouteStatusLabel()
    {
        return g_routeStatus;
    }

    std::string_view GetStageHarnessLabel()
    {
        if (!TargetNeedsStageHarness(g_target))
            return "not required";

        if (g_stageContextObserved)
            return "stage context observed";

        return g_requestedStageHarness;
    }

    std::string_view GetTargetCsdStatusLabel()
    {
        if (g_targetCsdObserved)
            return "observed";

        return "waiting";
    }

    const std::array<RuntimeTarget, 11>& GetRuntimeTargets()
    {
        return kRuntimeTargets;
    }

    static void ResetRouteLatchState()
    {
        ++g_routeGeneration;
        ++g_routeResetCount;
        g_titleIntroAcceptInjected = false;
        g_titleIntroDirectStateApplied = false;
        g_titleIntroDirectStateLastRequestFrame = 0;
        g_stageTitleOwnerDirectStateLastRequestFrame = 0;
        g_titleMenuAcceptInjected = false;
        g_titleMenuDirectContextAcceptInjected = false;
        g_stageContextObserved = false;
        g_targetCsdObserved = false;
        g_loggedIntroHook = false;
        g_loggedMenuHook = false;
        g_loggedStageHarness = false;
        g_loggedTargetCsdProjectLive = false;
        g_loggedStageTargetCsdBound = false;
        g_loggedStageTargetReady = false;
        g_titleMenuTransitionPulseObserved = false;
        g_titleMenuVisualReady = false;
        g_titleMenuPressStartAccepted = false;
        g_titleMenuPostPressStartHeld = false;
        g_titleMenuAcceptSuppressionLogged = false;
        g_titleMenuPostPressStartReadyLogged = false;
        g_titleMenuStableFrameStart = 0;
        g_titleMenuOwnerStableFrameStart = 0;
        g_titleIntroInspector = {};
        g_titleOwnerInspector = {};
        g_titleMenuInspector = {};
        g_nativeFrameCaptureReserved = false;
        g_nativeFrameCaptureWrittenCount = 0;
        g_lastNativeFrameCaptureFrame = 0;
        g_lastNativeFrameCapturePath.clear();
        g_lastNativeFrameCaptureFailure.clear();
        g_nativeFrameCaptureCompleteExitPending = false;
        g_uiOnlyRenderTargetCaptureRequested = false;
        g_uiOnlyRenderTargetCaptureReserved = false;
        g_lastUiOnlyRenderTargetCaptureFrame = 0;
        g_lastUiOnlyRenderTargetCapturePath.clear();
        g_lastUiOnlyRenderTargetCaptureFailure.clear();
        g_lastUiOnlyRenderTargetCaptureSource.clear();
        g_lastUiOnlyRenderTargetCaptureWidth = 0;
        g_lastUiOnlyRenderTargetCaptureHeight = 0;
        g_lastUiOnlyRenderTargetCaptureContainsFullFramebuffer = true;
        g_lastLoadingRequestType = UINT32_MAX;
        g_lastLoadingRequestFrame = 0;
        g_lastLoadingDisplayType = UINT32_MAX;
        g_lastLoadingDisplayFrame = 0;
        g_loadingDisplayWasActive = false;
        g_lastStageContextDetail.clear();
        g_lastStageReadyEventName.clear();
        g_lastStageGameModeAddress = 0;
        g_lastStageContextFrame = 0;
        g_lastStageReadyFrame = 0;
        g_pauseRouteStartInjected = false;
        g_pauseRouteInputHoldStartFrame = 0;
        g_pauseRouteInputLastFrame = 0;
        g_loggedPauseOwnerObserved = false;
        g_loggedPauseRouteInputSources.clear();
        g_chudSonicStageOwnerAddress = 0;
        g_chudSonicStagePlayScreenProjectAddress = 0;
        g_chudSonicStageSpeedGaugeSceneAddress = 0;
        g_chudSonicStageRingEnergyGaugeSceneAddress = 0;
        g_chudSonicStageGaugeFrameSceneAddress = 0;
        g_chudSonicStageScoreCountNodeAddress = 0;
        g_chudSonicStageTimeCountNodeAddress = 0;
        g_chudSonicStageTimeCount2NodeAddress = 0;
        g_chudSonicStageTimeCount3NodeAddress = 0;
        g_chudSonicStagePlayerCountNodeAddress = 0;
        g_chudSonicStageRawHookFrame = 0;
        g_chudSonicStageRawHookSource.clear();
        g_loggedChudSonicStageOwnerHook = false;
        g_loggedChudSonicStageOwnerFieldSample = false;
        g_loggedChudSonicStageOwnerFieldsReady = false;
        g_chudSonicStageOwnerHookStableSignature.clear();
        g_chudSonicStageOwnerHookLastEvidenceFrame = 0;
        g_chudSonicStageOwnerFieldSampleStableSignature.clear();
        g_chudSonicStageOwnerFieldSampleLastEvidenceFrame = 0;
        g_chudSonicStageOwnerFieldGaugeSnapshotStableSignature.clear();
        g_chudSonicStageOwnerFieldGaugeSnapshotLastEvidenceFrame = 0;
        g_lastOwnerFieldGaugeSnapshot = OwnerFieldGaugeSnapshotCache{};
        g_lastOwnerFieldGaugeScaleCorrelationSignature.clear();
        g_lastOwnerFieldGaugeScaleCorrelationEvidenceFrame = 0;
        g_loggedTutorialHudOwnerPathReady = false;
        g_chudSonicStageOwnerFieldSampleCount = 0;
        g_lastSonicHudUpdateCallsiteSampleFrame = 0;
        g_lastCsdProjectName.clear();
        g_lastCsdProjectFrame = 0;
        g_lastTitleIntroContextDetail.clear();
        g_lastStageTitleContextDetail.clear();
        g_lastTitleMenuContextDetail.clear();
        g_loggedCsdProjects.clear();

        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            g_observedCsdProjectOrder.clear();
            g_csdProjectTrees.clear();
            g_pauseGeneralSaveInspector = {};
            g_chudSonicStageOwnerFieldSamples.clear();
            g_sonicHudGameplayValues = {};
            g_sonicHudValueWriteObservations.clear();
            g_sonicHudUpdateCallsiteSamples.clear();
            g_lastSonicHudClassifiedCallsiteValue = {};
            g_lastSonicHudUpdateCallsiteSampleDetails.clear();
            g_lastSonicHudUpdateCallsiteStableSignatures.clear();
            g_lastSonicHudUpdateCallsiteEvidenceFrames.clear();
            g_lastSonicHudSpeedReadoutValues.clear();
            g_lastSonicHudSpeedReadoutEvidenceFrames.clear();
            g_loggedSonicHudValueTextWriteKeys.clear();
        }

        {
            std::lock_guard<std::mutex> lock(g_liveBridgeMutex);
            g_recentEvidenceEvents.clear();
        }
    }

    void RequestRouteToCurrentTarget()
    {
        ResetRouteLatchState();

        if (g_observerMode)
        {
            g_routePending = false;
            g_routeStatus = "observer mode";
            WriteEvidenceEvent("route-disabled-observer");
            return;
        }

        if (g_target == ScreenId::TitleLoop)
        {
            g_routePending = false;
            g_routeStatus = "holding title loop";
            WriteEvidenceEvent("route-hold-title-loop");
            return;
        }

        g_routePending = true;
        g_routeStatus = TargetNeedsStageHarness(g_target)
            ? "stage harness armed"
            : "route pending";
        WriteEvidenceEvent("route-requested");

        if (TargetNeedsStageHarness(g_target))
        {
            const auto& target = TargetFor(g_target);
            WriteEvidenceEvent(
                "stage-harness-selected",
                "target=" + std::string(target.token) +
                " requested_stage=" + g_requestedStageHarness +
                " target_csd=" + std::string(target.primaryCsdScene));
        }

        RefreshTargetCsdProjectStatus();
    }

    void SelectPreviousTarget()
    {
        auto index = TargetIndexFor(g_target);
        SelectTargetIndex(index == 0 ? kRuntimeTargets.size() - 1 : index - 1);
    }

    void SelectNextTarget()
    {
        SelectTargetIndex((TargetIndexFor(g_target) + 1) % kRuntimeTargets.size());
    }

    void OnTitleStateIntroUpdate(float elapsedSeconds)
    {
        if (!g_isEnabled || g_loggedIntroHook)
            return;

        const auto& target = TargetFor(g_target);
        LOGFN(
            "SWARD UI Lab attached to CTitleStateIntro::Update: target={} elapsed={:.3f} csd={}",
            target.token,
            elapsedSeconds,
            target.primaryCsdScene);
        WriteEvidenceEvent("title-intro-attached");
        g_loggedIntroHook = true;
    }

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
        uint32_t context488)
    {
        if (!g_isEnabled)
            return;

        ++g_titleIntroContextSampleCount;
        g_titleIntroInspector.valid = true;
        g_titleIntroInspector.contextAddress = contextAddress;
        g_titleIntroInspector.stateMachineAddress = stateMachineAddress;
        g_titleIntroInspector.elapsedSeconds = elapsedSeconds;
        g_titleIntroInspector.requestedState = requestedState;
        g_titleIntroInspector.dirtyFlag = dirtyFlag;
        g_titleIntroInspector.transitionArmed = transitionArmed;
        g_titleIntroInspector.contextFlag580 = contextFlag580;
        g_titleIntroInspector.context472 = context472;
        g_titleIntroInspector.context480 = context480;
        g_titleIntroInspector.context488 = context488;
        g_titleIntroInspector.frame = g_presentedFrameCount;

        const auto detail =
            "context=" + std::to_string(contextAddress) +
            " state_machine=" + std::to_string(stateMachineAddress) +
            " elapsed_ms=" + std::to_string(static_cast<uint32_t>(elapsedSeconds * 1000.0f)) +
            " requested_state=" + std::to_string(requestedState) +
            " dirty=" + std::to_string(dirtyFlag) +
            " transition_armed=" + std::to_string(transitionArmed) +
            " context_flag580=" + std::to_string(contextFlag580) +
            " context_472=" + std::to_string(context472) +
            " context_480=" + std::to_string(context480) +
            " context_488=" + std::to_string(context488);

        if (detail == g_lastTitleIntroContextDetail && (g_titleIntroContextSampleCount % 60) != 0)
            return;

        g_lastTitleIntroContextDetail = detail;
        WriteEvidenceEvent("title-intro-context", detail);
    }

    void OnTitleIntroDirectStateApplied(
        uint8_t requestedState,
        uint8_t dirtyFlag,
        uint8_t transitionArmed,
        uint8_t outputArmed,
        uint8_t csdCompleteArmed)
    {
        if (!g_isEnabled)
            return;

        WriteEvidenceEvent(
            "title-intro-direct-state-applied",
            "requested_state=" + std::to_string(requestedState) +
            " dirty=" + std::to_string(dirtyFlag) +
            " transition_armed=" + std::to_string(transitionArmed) +
                " output_armed=" + std::to_string(outputArmed) +
                " csd_complete_armed=" + std::to_string(csdCompleteArmed));
    }

    bool ShouldRefreshStageTitleOwnerDirectState()
    {
        if (!g_stageTitleOwnerDirectStateFallbackEnabled)
            return false;

        if (!g_isEnabled ||
            g_observerMode ||
            !g_routePending ||
            g_routePolicy != RoutePolicy::DirectContext ||
            !TargetNeedsStageHarness(g_target) ||
            g_loggedStageTargetReady ||
            g_loggedMenuHook ||
            !g_stageContextObserved ||
            g_targetCsdObserved)
        {
            return false;
        }

        constexpr uint64_t kStageTitleOwnerDirectStateFallbackFrames = 1260;
        if (g_titleIntroDirectStateLastRequestFrame == 0 ||
            g_presentedFrameCount < g_titleIntroDirectStateLastRequestFrame + kStageTitleOwnerDirectStateFallbackFrames)
        {
            return false;
        }

        constexpr uint64_t kStageTitleOwnerDirectStateRefreshFrames = 60;
        if (g_stageTitleOwnerDirectStateLastRequestFrame != 0 &&
            g_presentedFrameCount < g_stageTitleOwnerDirectStateLastRequestFrame + kStageTitleOwnerDirectStateRefreshFrames)
        {
            return false;
        }

        const bool refresh = g_stageTitleOwnerDirectStateLastRequestFrame != 0;
        g_stageTitleOwnerDirectStateLastRequestFrame = g_presentedFrameCount;
        g_routeStatus = "stage title owner direct state requested";
        WriteEvidenceEvent(
            refresh ? "stage-title-owner-direct-state-refreshed" : "stage-title-owner-direct-state-requested",
            "state=1 dirty=1 transition=1 owner_gate=1 csd_complete=1");
        return true;
    }

    void OnStageTitleOwnerDirectStateApplied(
        uint32_t titleContextAddress,
        uint32_t titleCsdAddress,
        uint8_t requestedState,
        uint8_t dirtyFlag,
        uint8_t transitionArmed,
        uint8_t outputArmed,
        uint8_t ownerGateArmed,
        uint8_t csdCompleteArmed)
    {
        if (!g_isEnabled)
            return;

        WriteEvidenceEvent(
            "stage-title-owner-direct-state-applied",
            "owner_title_context=" + HexU32(titleContextAddress) +
            " title_csd488=" + HexU32(titleCsdAddress) +
            " requested_state=" + std::to_string(requestedState) +
            " dirty=" + std::to_string(dirtyFlag) +
            " transition_armed=" + std::to_string(transitionArmed) +
            " output_armed=" + std::to_string(outputArmed) +
            " owner_gate568=" + std::to_string(ownerGateArmed) +
            " csd_complete_armed=" + std::to_string(csdCompleteArmed));
    }

    void OnGameModeStageTitleContext(
        uint32_t gameModeAddress,
        uint32_t contextAddress,
        uint32_t stateMachineAddress,
        float stateTime,
        bool isTitleStateMenu,
        bool isAutoSaveWarningShown,
        std::string_view ownerDetail)
    {
        if (!g_isEnabled)
            return;

        ++g_stageTitleContextSampleCount;

        const auto detail =
            "game_mode=" + std::to_string(gameModeAddress) +
            " context=" + std::to_string(contextAddress) +
            " state_machine=" + std::to_string(stateMachineAddress) +
            " state_time_ms=" + std::to_string(static_cast<uint32_t>(stateTime * 1000.0f)) +
            " is_title_state_menu=" + std::to_string(isTitleStateMenu ? 1 : 0) +
            " autosave_warning=" + std::to_string(isAutoSaveWarningShown ? 1 : 0) +
            (ownerDetail.empty() ? "" : " " + std::string(ownerDetail));

        if (detail == g_lastStageTitleContextDetail && (g_stageTitleContextSampleCount % 60) != 0)
            return;

        g_lastStageTitleContextDetail = detail;
        WriteEvidenceEvent("stage-title-context", detail);
    }

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
        uint8_t csdByte160)
    {
        if (!g_isEnabled)
            return;

        const bool ownerReady =
            g_titleMenuPostPressStartHeld &&
            isTitleStateMenu &&
            titleContextAddress != 0 &&
            titleCsdAddress != 0 &&
            ownerGate568 &&
            titleRequest != 0 &&
            titleTransition != 0 &&
            csdByte84 != 0;

        g_titleOwnerInspector.valid = true;
        g_titleOwnerInspector.isTitleStateMenu = isTitleStateMenu;
        g_titleOwnerInspector.titleContextAddress = titleContextAddress;
        g_titleOwnerInspector.titleCsdAddress = titleCsdAddress;
        g_titleOwnerInspector.ownerGate568 = ownerGate568;
        g_titleOwnerInspector.ownerGate570 = ownerGate570;
        g_titleOwnerInspector.titleRequest = titleRequest;
        g_titleOwnerInspector.titleDirty = titleDirty;
        g_titleOwnerInspector.titleTransition = titleTransition;
        g_titleOwnerInspector.titleFlag580 = titleFlag580;
        g_titleOwnerInspector.csdByte62 = csdByte62;
        g_titleOwnerInspector.csdByte84 = csdByte84;
        g_titleOwnerInspector.csdByte152 = csdByte152;
        g_titleOwnerInspector.csdByte160 = csdByte160;
        g_titleOwnerInspector.ownerReady = ownerReady;
        g_titleOwnerInspector.frame = g_presentedFrameCount;

        if (g_target != ScreenId::TitleMenu || g_titleMenuVisualReady)
            return;

        const auto detail =
            "is_title_state_menu=" + std::to_string(isTitleStateMenu ? 1 : 0) +
            " owner_title_context=" + std::to_string(titleContextAddress) +
            " title_csd488=" + std::to_string(titleCsdAddress) +
            " owner_gate568=" + std::to_string(ownerGate568 ? 1 : 0) +
            " owner_gate570=" + std::to_string(ownerGate570 ? 1 : 0) +
            " title_request=" + std::to_string(titleRequest) +
            " title_dirty=" + std::to_string(titleDirty) +
            " title_transition=" + std::to_string(titleTransition) +
            " title_flag580=" + std::to_string(titleFlag580) +
            " csd_byte62=" + std::to_string(csdByte62) +
            " csd_byte84=" + std::to_string(csdByte84) +
            " csd_byte152=" + std::to_string(csdByte152) +
            " csd_byte160=" + std::to_string(csdByte160);

        if (!ownerReady)
        {
            g_titleMenuOwnerStableFrameStart = 0;
            return;
        }

        if (g_titleMenuOwnerStableFrameStart == 0)
        {
            g_titleMenuOwnerStableFrameStart = g_presentedFrameCount;

            if (!g_titleMenuPostPressStartReadyLogged)
            {
                g_titleMenuPostPressStartReadyLogged = true;
                WriteEvidenceEvent("title-menu-post-press-start-ready", detail);
            }
        }

        // The owner/CSD bytes prove the post-Press-Start state exists, but they
        // can become true before the menu is a good capture target. The visual
        // latch is completed from CTitleStateMenu context below.
    }

    void OnTitleStateMenuUpdate(int32_t cursorIndex)
    {
        if (!g_isEnabled || g_loggedMenuHook)
            return;

        const auto& target = TargetFor(g_target);
        LOGFN(
            "SWARD UI Lab attached to CTitleStateMenu::Update: target={} cursor={} csd={}",
            target.token,
            cursorIndex,
            target.primaryCsdScene);
        WriteEvidenceEvent("title-menu-attached");
        g_loggedMenuHook = true;
    }

    void OnTitleMenuContext(
        uint32_t context472,
        uint32_t context480,
        uint32_t context488,
        uint32_t contextPhase,
        uint8_t contextFlag580,
        uint32_t menuCursor,
        bool menuField3C,
        bool menuField54,
        bool menuField9A)
    {
        if (!g_isEnabled)
            return;

        const auto detail =
            "context_472=" + std::to_string(context472) +
            " context_480=" + std::to_string(context480) +
            " context_488=" + std::to_string(context488) +
            " context_phase=" + std::to_string(contextPhase) +
            " context_flag580=" + std::to_string(contextFlag580) +
            " menu_cursor=" + std::to_string(menuCursor) +
            " menu_field3c=" + std::to_string(menuField3C ? 1 : 0) +
            " menu_field54=" + std::to_string(menuField54 ? 1 : 0) +
            " menu_field9a=" + std::to_string(menuField9A ? 1 : 0);

        StoreTitleMenuInspectorSnapshot(
            context472,
            context480,
            context488,
            contextPhase,
            contextFlag580,
            menuCursor,
            menuField3C,
            menuField54,
            menuField9A);

        if (g_target == ScreenId::TitleMenu && !g_titleMenuVisualReady)
        {
            const bool postPressStartMenuReady = IsTitleMenuContextReady(
                context472,
                context488,
                contextPhase,
                menuCursor,
                menuField54);

            if (postPressStartMenuReady)
            {
                if (g_titleMenuStableFrameStart == 0)
                    g_titleMenuStableFrameStart = g_presentedFrameCount;

                constexpr uint64_t kTitleMenuContextVisualSettleFrames = 40;
                const auto stableFrames = g_presentedFrameCount - g_titleMenuStableFrameStart;
                StoreTitleMenuInspectorSnapshot(
                    context472,
                    context480,
                    context488,
                    contextPhase,
                    contextFlag580,
                    menuCursor,
                    menuField3C,
                    menuField54,
                    menuField9A);

                if (stableFrames < kTitleMenuContextVisualSettleFrames)
                    return;

                g_titleMenuVisualReady = true;
                g_routeStatus = "title menu visual ready";
                WriteEvidenceEvent(
                    "title-menu-visible",
                    detail + " stable_frames=" + std::to_string(stableFrames) +
                    " source=title-menu-context");
            }
            else if (menuField3C)
            {
                g_titleMenuTransitionPulseObserved = true;
                g_titleMenuStableFrameStart = 0;
            }
            else if (
                g_titleMenuPressStartAccepted &&
                g_titleMenuPostPressStartReadyLogged &&
                g_titleMenuTransitionPulseObserved &&
                menuField54 &&
                context488 != 0 &&
                context472 == 0 &&
                contextPhase == 0)
            {
                if (g_titleMenuStableFrameStart == 0)
                    g_titleMenuStableFrameStart = g_presentedFrameCount;

                constexpr uint64_t kTitleMenuVisualSettleFrames = 75;
                const auto stableFrames = g_presentedFrameCount - g_titleMenuStableFrameStart;

                if (stableFrames >= kTitleMenuVisualSettleFrames)
                {
                    g_titleMenuVisualReady = true;
                    g_routeStatus = "title menu visual ready";
                    WriteEvidenceEvent(
                        "title-menu-visible",
                        detail + " stable_frames=" + std::to_string(stableFrames) +
                        " source=title-menu-context");
                }
            }
            else
            {
                g_titleMenuStableFrameStart = 0;
            }
        }

        if (detail == g_lastTitleMenuContextDetail)
            return;

        g_lastTitleMenuContextDetail = detail;
        WriteEvidenceEvent("title-menu-context", detail);
    }

    void OnStageExitLoading(uint32_t gameModeStageAddress)
    {
        if (!g_isEnabled || !TargetNeedsStageHarness(g_target))
            return;

        g_stageContextObserved = true;
        g_lastStageGameModeAddress = gameModeStageAddress;
        g_lastStageContextFrame = g_presentedFrameCount;
        g_routeStatus = "stage context live";

        const auto& target = TargetFor(g_target);
        const auto detail =
            "CGameModeStage::ExitLoading" +
            std::string(" stage_address=") + HexU32(gameModeStageAddress) +
            " target=" + std::string(target.token) +
            " requested_stage=" + g_requestedStageHarness +
            " target_csd=" + std::string(target.primaryCsdScene) +
            " target_csd_observed=" + std::to_string(g_targetCsdObserved ? 1 : 0);

        if (!g_loggedStageHarness)
        {
            LOGFN(
                "SWARD UI Lab stage harness observed CGameModeStage::ExitLoading: target={} requested_stage={} csd={} stage={}",
                target.token,
                g_requestedStageHarness,
                target.primaryCsdScene,
                HexU32(gameModeStageAddress));
            WriteEvidenceEvent("stage-context-observed", detail);
            g_loggedStageHarness = true;
        }
        else if (detail != g_lastStageContextDetail)
        {
            WriteEvidenceEvent("stage-context-sample", detail);
        }

        g_lastStageContextDetail = detail;
        RefreshTargetCsdProjectStatus();
        EmitStageTargetReadyIfNeeded();
        WriteLiveStateSnapshot();
    }

    bool ApplyPauseRouteInput(std::string_view hookSource)
    {
        if (!g_isEnabled ||
            g_observerMode ||
            g_target != ScreenId::Pause ||
            !g_stageContextObserved ||
            g_loggedStageTargetReady ||
            IsPauseTargetRuntimeReady())
        {
            return false;
        }

        const std::string source = hookSource.empty() ? "unknown" : std::string(hookSource);

        if (g_loggedPauseRouteInputSources.insert(source).second)
        {
            WriteEvidenceEvent(
                "pause-route-input-gate-observed",
                "frame=" + std::to_string(g_presentedFrameCount) +
                " pause-route-input-source=" + source +
                " stage_address=" + HexU32(g_lastStageGameModeAddress));
        }

        constexpr uint64_t kPauseRouteStageSettleFrames = 45;
        constexpr uint64_t kPauseRoutePostLoadingSettleFrames = 60;
        constexpr uint64_t kPauseRouteHoldFrames = 30;
        constexpr uint64_t kPauseRouteRetryFrames = 45;

        if (g_presentedFrameCount < g_lastStageContextFrame + kPauseRouteStageSettleFrames)
            return false;

        const uint64_t lastLoadingFrame = std::max(g_lastLoadingRequestFrame, g_lastLoadingDisplayFrame);
        const bool runtimeLoading = SWA::SGlobals::ms_IsLoading && *SWA::SGlobals::ms_IsLoading;
        if (runtimeLoading || g_loadingDisplayWasActive ||
            (lastLoadingFrame != 0 && g_presentedFrameCount < lastLoadingFrame + kPauseRoutePostLoadingSettleFrames))
        {
            return false;
        }

        if (!IsPauseRouteOwnerObserved())
            return false;

        if (g_pauseRouteInputHoldStartFrame == 0 ||
            g_presentedFrameCount > g_pauseRouteInputLastFrame + kPauseRouteRetryFrames)
        {
            g_pauseRouteInputHoldStartFrame = g_presentedFrameCount;
            g_pauseRouteInputLastFrame = g_presentedFrameCount;

            if (!g_pauseRouteStartInjected)
            {
                g_pauseRouteStartInjected = true;
                g_routeStatus = "pause start injected";
                WriteEvidenceEvent(
                    "pause-route-start-injected",
                    "frame=" + std::to_string(g_presentedFrameCount) +
                    " stage_address=" + HexU32(g_lastStageGameModeAddress) +
                    " pause-route-input-source=" + source);
            }
            else
            {
                WriteEvidenceEvent(
                    "pause-route-start-retried",
                    "frame=" + std::to_string(g_presentedFrameCount) +
                    " stage_address=" + HexU32(g_lastStageGameModeAddress) +
                    " pause-route-input-source=" + source);
            }
        }

        if (g_presentedFrameCount >= g_pauseRouteInputHoldStartFrame + kPauseRouteHoldFrames)
            return false;

        auto pInputState = SWA::CInputState::GetInstance();
        if (!pInputState)
            return false;

        auto& padState = pInputState->m_PadStates[(uint32_t)pInputState->m_CurrentPadStateIndex];
        constexpr uint32_t startMask = SWA::eKeyState_Start;
        padState.DownState = (uint32_t)padState.DownState | startMask;
        padState.TappedState = (uint32_t)padState.TappedState | startMask;
        g_pauseRouteInputLastFrame = g_presentedFrameCount;
        g_routeStatus = "pause start injected";
        return true;
    }

    static void RequestUiLabExit(std::string_view eventName, std::string_view detail = {})
    {
        if (g_autoExitRequested)
            return;

        g_autoExitRequested = true;
        WriteEvidenceEvent(eventName, detail);
        App::Exit();
    }

    void OnPresentedFrame()
    {
        if (!g_isEnabled)
            return;

        ++g_presentedFrameCount;

        if (g_presentedFrameCount == 1)
            WriteEvidenceEvent("first-presented-frame");
        else if ((g_presentedFrameCount % 60) == 0)
            WriteEvidenceEvent("presented-frame");

        UpdateHudRenderGateCorrelation();

        if ((g_presentedFrameCount % 30) == 0 && g_lastLiveStateSnapshotFrame != g_presentedFrameCount)
            WriteLiveStateSnapshot();

        if (g_nativeFrameCaptureCompleteExitPending &&
            g_autoExitSeconds > 0.0 &&
            !g_autoExitRequested)
        {
            RequestUiLabExit(
                "native-frame-capture-complete-auto-exit",
                "captures=" + std::to_string(g_nativeFrameCaptureWrittenCount) +
                " max=" + std::to_string(g_nativeFrameCaptureMaxCount));
        }

        if (g_autoExitSeconds > 0.0 && !g_autoExitRequested && SecondsSinceStart() >= g_autoExitSeconds)
        {
            RequestUiLabExit("auto-exit");
        }
    }

    std::string ConsumeNativeFrameCapturePath(uint32_t width, uint32_t height)
    {
        if (!IsNativeFrameCaptureReady())
            return {};

        if (g_nativeFrameCaptureDirectory.empty())
        {
            OnNativeFrameCaptureFailed("native capture directory is empty");
            return {};
        }

        g_nativeFrameCaptureReserved = true;
        ++g_nativeFrameCaptureIndex;

        std::error_code ec;
        std::filesystem::create_directories(g_nativeFrameCaptureDirectory, ec);

        if (ec)
        {
            OnNativeFrameCaptureFailed("failed to create native capture directory: " + ec.message());
            return {};
        }

        const auto& target = TargetFor(g_target);
        std::ostringstream fileName;
        fileName
            << "native_frame_"
            << target.token
            << "_"
            << g_nativeFrameCaptureIndex
            << "_"
            << width
            << "x"
            << height
            << ".bmp";

        return (g_nativeFrameCaptureDirectory / fileName.str()).string();
    }

    void OnNativeFrameCaptured(std::string_view path, uint32_t width, uint32_t height)
    {
        if (!g_isEnabled)
            return;

        g_nativeFrameCaptureReserved = false;
        ++g_nativeFrameCaptureWrittenCount;
        g_lastNativeFrameCaptureFrame = g_presentedFrameCount;
        g_lastNativeFrameCapturePath = std::string(path);
        g_lastNativeFrameCaptureFailure.clear();
        WriteEvidenceEvent(
            "native-frame-captured",
            "path=" + std::string(path) +
            " width=" + std::to_string(width) +
            " height=" + std::to_string(height) +
            " format=B8G8R8A8 bmp=1" +
            " index=" + std::to_string(g_nativeFrameCaptureWrittenCount) +
            " max=" + std::to_string(g_nativeFrameCaptureMaxCount));
        WriteLiveStateSnapshot();

        if (g_autoExitSeconds > 0.0 &&
            g_nativeFrameCaptureWrittenCount >= g_nativeFrameCaptureMaxCount)
        {
            g_nativeFrameCaptureCompleteExitPending = true;
        }
    }

    void OnNativeFrameCaptureFailed(std::string_view reason)
    {
        if (!g_isEnabled)
            return;

        g_nativeFrameCaptureReserved = false;
        g_lastNativeFrameCaptureFailure = std::string(reason);
        WriteEvidenceEvent("native-frame-capture-failed", reason);
        WriteLiveStateSnapshot();
    }

    bool IsUiOnlyRenderTargetCaptureRequested()
    {
        return g_isEnabled &&
            g_uiOnlyRenderTargetCaptureRequested &&
            !g_uiOnlyRenderTargetCaptureReserved;
    }

    std::string ConsumeUiOnlyRenderTargetCapturePath(
        uint32_t width,
        uint32_t height,
        std::string_view source,
        bool containsFullFramebuffer)
    {
        if (!IsUiOnlyRenderTargetCaptureRequested())
            return {};

        const std::filesystem::path directory = !g_nativeFrameCaptureDirectory.empty()
            ? g_nativeFrameCaptureDirectory
            : g_evidenceDirectory;

        if (directory.empty())
        {
            OnUiOnlyRenderTargetCaptureFailed("UI render-target capture directory is empty");
            return {};
        }

        g_uiOnlyRenderTargetCaptureReserved = true;
        ++g_uiOnlyRenderTargetCaptureIndex;

        std::error_code ec;
        std::filesystem::create_directories(directory, ec);

        if (ec)
        {
            OnUiOnlyRenderTargetCaptureFailed("failed to create UI render-target capture directory: " + ec.message());
            return {};
        }

        g_lastUiOnlyRenderTargetCaptureSource = std::string(source);
        g_lastUiOnlyRenderTargetCaptureWidth = width;
        g_lastUiOnlyRenderTargetCaptureHeight = height;
        g_lastUiOnlyRenderTargetCaptureContainsFullFramebuffer = containsFullFramebuffer;

        const auto& target = TargetFor(g_target);
        std::ostringstream fileName;
        fileName
            << "ui_layer_render_target_"
            << target.token
            << "_"
            << g_uiOnlyRenderTargetCaptureIndex
            << "_"
            << width
            << "x"
            << height
            << ".bmp";

        return (directory / fileName.str()).string();
    }

    void OnUiOnlyRenderTargetCaptured(
        std::string_view path,
        uint32_t width,
        uint32_t height,
        std::string_view source,
        bool containsFullFramebuffer)
    {
        if (!g_isEnabled)
            return;

        g_uiOnlyRenderTargetCaptureRequested = false;
        g_uiOnlyRenderTargetCaptureReserved = false;
        g_lastUiOnlyRenderTargetCaptureFrame = g_presentedFrameCount + 1;
        g_lastUiOnlyRenderTargetCapturePath = std::string(path);
        g_lastUiOnlyRenderTargetCaptureFailure.clear();
        g_lastUiOnlyRenderTargetCaptureSource = std::string(source);
        g_lastUiOnlyRenderTargetCaptureWidth = width;
        g_lastUiOnlyRenderTargetCaptureHeight = height;
        g_lastUiOnlyRenderTargetCaptureContainsFullFramebuffer = containsFullFramebuffer;

        WriteEvidenceEvent(
            "ui-render-target-captured",
            "path=" + std::string(path) +
            " width=" + std::to_string(width) +
            " height=" + std::to_string(height) +
            " source=" + std::string(source) +
            " isolation=" + std::string(UiOnlyLayerIsolationStatusLabel()) +
            " format=B8G8R8A8 bmp=1");
        WriteLiveStateSnapshot();
    }

    void OnUiOnlyRenderTargetCaptureFailed(std::string_view reason)
    {
        if (!g_isEnabled)
            return;

        g_uiOnlyRenderTargetCaptureRequested = false;
        g_uiOnlyRenderTargetCaptureReserved = false;
        g_lastUiOnlyRenderTargetCaptureFailure = std::string(reason);
        WriteEvidenceEvent("ui-render-target-capture-failed", reason);
        WriteLiveStateSnapshot();
    }

    void OnLoadingRequest(uint32_t displayType)
    {
        if (!g_isEnabled)
            return;

        g_lastLoadingRequestType = displayType;
        g_lastLoadingRequestFrame = g_presentedFrameCount;
        g_routeStatus = "loading request observed";
        WriteEvidenceEvent("loading-requested", "display_type=" + std::to_string(displayType));
    }

    void OnLoadingUpdate(uint32_t displayType)
    {
        if (!g_isEnabled)
            return;

        if (displayType == g_lastLoadingDisplayType)
            return;

        g_lastLoadingDisplayType = displayType;
        g_lastLoadingDisplayFrame = g_presentedFrameCount;
        const bool preservePauseReadyRoute = g_target == ScreenId::Pause && g_loggedStageTargetReady;

        if (displayType != 0)
        {
            g_loadingDisplayWasActive = true;
            if (!preservePauseReadyRoute)
                g_routeStatus = "loading display active";
            WriteEvidenceEvent("loading-display-active", "display_type=" + std::to_string(displayType));
        }
        else if (g_loadingDisplayWasActive)
        {
            g_loadingDisplayWasActive = false;
            if (!preservePauseReadyRoute)
                g_routeStatus = "loading display ended";
            WriteEvidenceEvent("loading-display-ended");
        }
    }

    void OnCsdProjectMade(std::string_view projectName)
    {
        if (!g_isEnabled || projectName.empty())
            return;

        const std::string project(projectName);
        g_lastCsdProjectName = project;
        g_lastCsdProjectFrame = g_presentedFrameCount;

        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            if (std::find(g_observedCsdProjectOrder.begin(), g_observedCsdProjectOrder.end(), project) ==
                g_observedCsdProjectOrder.end())
            {
                g_observedCsdProjectOrder.push_back(project);
            }

            auto& record = EnsureCsdProjectTreeRecordLocked(project);
            record.frame = g_presentedFrameCount;
        }

        if (!g_loggedCsdProjects.insert(project).second)
            return;

        WriteEvidenceEvent("csd-project-made", project);

        MarkTargetCsdProjectLive(projectName);
    }

    void OnCsdProjectTreeMade(std::string_view projectName, uint32_t projectAddress, uint32_t rootNodeAddress)
    {
        if (!g_isEnabled || projectName.empty())
            return;

        const std::string project(projectName);
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            if (std::find(g_observedCsdProjectOrder.begin(), g_observedCsdProjectOrder.end(), project) ==
                g_observedCsdProjectOrder.end())
            {
                g_observedCsdProjectOrder.push_back(project);
            }

            auto& record = EnsureCsdProjectTreeRecordLocked(project);
            record.projectAddress = projectAddress;
            record.rootNodeAddress = rootNodeAddress;
            record.sceneCount = 0;
            record.nodeCount = 0;
            record.layerCount = 0;
            record.scenes.clear();
            record.nodes.clear();
            record.layers.clear();
            record.frame = g_presentedFrameCount;
        }
    }

    void OnCsdSceneNodeTraversed(
        std::string_view projectName,
        std::string_view nodePath,
        uint32_t nodeAddress,
        uint32_t sceneCount,
        uint32_t childNodeCount)
    {
        if (!g_isEnabled || projectName.empty() || nodePath.empty())
            return;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        auto& record = EnsureCsdProjectTreeRecordLocked(projectName);
        ++record.nodeCount;
        StoreCsdTreeEntry(
            record.nodes,
            nodePath,
            nodeAddress,
            record.projectAddress,
            sceneCount,
            childNodeCount);
    }

    void OnCsdSceneTraversed(
        std::string_view projectName,
        std::string_view scenePath,
        uint32_t sceneAddress,
        uint32_t castNodeCount,
        uint32_t castCount)
    {
        if (!g_isEnabled || projectName.empty() || scenePath.empty())
            return;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        auto& record = EnsureCsdProjectTreeRecordLocked(projectName);
        ++record.sceneCount;
        StoreCsdTreeEntry(
            record.scenes,
            scenePath,
            sceneAddress,
            record.projectAddress,
            castNodeCount,
            castCount);
    }

    void OnCsdLayerTraversed(
        std::string_view projectName,
        std::string_view layerPath,
        uint32_t layerAddress,
        uint32_t castNodeAddress,
        uint32_t castNodeIndex,
        uint32_t castIndex)
    {
        if (!g_isEnabled || projectName.empty() || layerPath.empty())
            return;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        auto& record = EnsureCsdProjectTreeRecordLocked(projectName);
        ++record.layerCount;
        StoreCsdTreeEntry(
            record.layers,
            layerPath,
            layerAddress,
            castNodeAddress,
            castNodeIndex,
            castIndex);
    }

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
        uint32_t colorSample)
    {
        if (!g_isEnabled)
            return;

        RuntimeUiDrawCall lateResolveCall;
        bool hasLateResolveCall = false;

        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);

            if (g_runtimeUiDrawListFrame != g_presentedFrameCount)
            {
                g_runtimeUiDrawListFrame = g_presentedFrameCount;
                g_runtimeUiDrawCalls.clear();
                g_runtimeUiDrawCallSequence = 0;
                g_runtimeUiDrawCallDroppedCount = 0;
            }

            ++g_runtimeUiDrawCallSequence;
            if (g_runtimeUiDrawCalls.size() >= kRuntimeUiDrawCallSampleLimit)
            {
                ++g_runtimeUiDrawCallDroppedCount;
                return;
            }

            RuntimeUiDrawCall call;
            call.frame = g_presentedFrameCount;
            call.sequence = g_runtimeUiDrawCallSequence;
            call.layerAddress = layerAddress;
            call.castNodeAddress = castNodeAddress;
            call.vertexBufferAddress = vertexBufferAddress;
            call.vertexCount = vertexCount;
            call.vertexStride = vertexStride;
            call.textured = textured;
            call.minX = minX;
            call.minY = minY;
            call.maxX = maxX;
            call.maxY = maxY;
            call.colorSample = colorSample;
            call.layerPath = "unresolved";

            for (const auto& record : g_csdProjectTrees)
            {
                const auto found = std::find_if(
                    record.layers.begin(),
                    record.layers.end(),
                    [layerAddress](const CsdTreeEntry& entry)
                    {
                        return entry.address == layerAddress;
                    });
                if (found == record.layers.end())
                    continue;

                call.projectName = record.projectName;
                call.layerPath = found->path;
                break;
            }

            if (call.projectName.empty())
                call.projectName = g_lastCsdProjectName.empty() ? std::string("unknown") : g_lastCsdProjectName;

            lateResolveCall = call;
            hasLateResolveCall = true;
            g_runtimeUiDrawCalls.push_back(std::move(call));
        }

        if (hasLateResolveCall)
            EmitLateResolvedSonicHudNodeWriteEvents(TryLateResolveSonicHudNodeWriteObservations(lateResolveCall));
    }

    void PushSonicHudUpdateContext(
        uint32_t ownerAddress,
        std::string_view hookSource)
    {
        if (!g_isEnabled)
            return;

        SonicHudUpdateContextFrame context;
        context.ownerAddress = ownerAddress;
        context.hookSource = hookSource.empty()
            ? std::string("sonic-hud-update-context")
            : std::string(hookSource);
        context.frame = g_presentedFrameCount;
        g_sonicHudUpdateContextStack.push_back(std::move(context));
    }

    void PopSonicHudUpdateContext(std::string_view hookSource)
    {
        (void)hookSource;

        if (g_sonicHudUpdateContextStack.empty())
            return;

        g_sonicHudUpdateContextStack.pop_back();
    }

    static bool ShouldSampleSonicHudUpdateCallsiteFrame()
    {
        if (g_lastSonicHudUpdateCallsiteSampleFrame == g_presentedFrameCount)
            return true;

        if (g_lastSonicHudUpdateCallsiteSampleFrame != 0 &&
            g_presentedFrameCount <
                g_lastSonicHudUpdateCallsiteSampleFrame + kSonicHudUpdateCallsiteMinEvidenceIntervalFrames)
        {
            return false;
        }

        g_lastSonicHudUpdateCallsiteSampleFrame = g_presentedFrameCount;
        return true;
    }

    void OnSonicHudUpdateCallsiteSample(
        uint32_t ownerAddress,
        std::string_view hookName,
        std::string_view samplePhase,
        double deltaTime,
        uint32_t r4)
    {
        if (!g_isEnabled || !IsPlausibleGuestPointer(ownerAddress))
            return;

        if (!ShouldSampleSonicHudUpdateCallsiteFrame())
            return;

        SonicHudUpdateCallsiteSample sample;
        sample.ownerAddress = ownerAddress;
        sample.hookName = hookName.empty() ? "CHudSonicStage/update-callsite" : std::string(hookName);
        sample.samplePhase = samplePhase.empty() ? "unknown" : std::string(samplePhase);
        sample.deltaTime = deltaTime;
        sample.r4 = r4;
        sample.frame = g_presentedFrameCount;

        TryReadGuestU32(ownerAddress + 424, sample.ownerField424);
        TryReadGuestU32(ownerAddress + 432, sample.ownerField432);
        TryReadGuestFloat(ownerAddress + 440, sample.ownerField440);
        TryReadGuestFloat(ownerAddress + 444, sample.ownerField444);
        TryReadGuestU32(ownerAddress + 452, sample.ownerField452);
        TryReadGuestU32(ownerAddress + 456, sample.ownerField456);
        TryReadGuestU32(ownerAddress + 460, sample.ownerField460);
        TryReadGuestU32(ownerAddress + 464, sample.ownerField464);
        TryReadGuestU32(ownerAddress + 468, sample.ownerField468);
        TryReadGuestU32(ownerAddress + 472, sample.ownerField472);
        TryReadGuestFloat(ownerAddress + 476, sample.ownerField476);
        TryReadGuestU32(ownerAddress + 480, sample.ownerField480);
        TryReadGuestU32(ownerAddress + 484, sample.ownerField484);
        TryReadGuestU32(ownerAddress + 488, sample.ownerField488);

        std::ostringstream detail;
        detail
            << "hook=" << sample.hookName
            << " samplePhase=" << sample.samplePhase
            << " owner=" << HexU32(sample.ownerAddress)
            << " deltaTime=" << sample.deltaTime
            << " r4=" << HexU32(sample.r4)
            << " ownerField424=" << HexU32(sample.ownerField424)
            << " ownerField432=" << HexU32(sample.ownerField432)
            << " ownerField440=" << sample.ownerField440
            << " ownerField444=" << sample.ownerField444
            << " ownerField452=" << sample.ownerField452
            << " ownerField456=" << sample.ownerField456
            << " ownerField460=" << sample.ownerField460
            << " ownerField464=" << sample.ownerField464
            << " ownerField468=" << sample.ownerField468
            << " ownerField472=" << sample.ownerField472
            << " ownerField476=" << sample.ownerField476
            << " ownerField480=" << sample.ownerField480
            << " ownerField484=" << HexU32(sample.ownerField484)
            << " ownerField488=" << HexU32(sample.ownerField488);

        const std::string key =
            sample.hookName + "|" + sample.samplePhase + "|" + HexU32(sample.ownerAddress);
        const std::string detailText = detail.str();
        const std::string stableSignature =
            BuildSonicHudUpdateCallsiteStableSignature(sample);

        bool signatureChanged = false;
        bool intervalElapsed = false;
        bool shouldWriteEvidence = false;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            auto existing = g_lastSonicHudUpdateCallsiteStableSignatures.find(key);
            signatureChanged =
                existing == g_lastSonicHudUpdateCallsiteStableSignatures.end() ||
                existing->second != stableSignature;

            auto lastEvidenceFrame = g_lastSonicHudUpdateCallsiteEvidenceFrames.find(key);
            intervalElapsed =
                lastEvidenceFrame == g_lastSonicHudUpdateCallsiteEvidenceFrames.end() ||
                g_presentedFrameCount >=
                    lastEvidenceFrame->second + kSonicHudUpdateCallsiteMinEvidenceIntervalFrames;

            shouldWriteEvidence =
                existing == g_lastSonicHudUpdateCallsiteStableSignatures.end() ||
                intervalElapsed;

            if (signatureChanged)
            {
                g_lastSonicHudUpdateCallsiteStableSignatures[key] = stableSignature;
                g_lastSonicHudUpdateCallsiteSampleDetails[key] = detailText;
                if (g_sonicHudUpdateCallsiteSamples.size() < kSonicHudUpdateCallsiteSampleLimit)
                    g_sonicHudUpdateCallsiteSamples.push_back(sample);
                else
                    g_sonicHudUpdateCallsiteSamples[g_sonicHudUpdateCallsiteSamples.size() - 1] = sample;
            }

            if (shouldWriteEvidence)
                g_lastSonicHudUpdateCallsiteEvidenceFrames[key] = g_presentedFrameCount;
        }

        const bool gameplayChanged =
            ApplySonicHudUpdateCallsiteSampleToGameplayValues(sample, shouldWriteEvidence);

        if (shouldWriteEvidence)
        {
            WriteEvidenceEvent(
                "sonic-hud-update-callsite-sample",
                detailText + " stableSignature=" + stableSignature);
            WriteLiveStateSnapshot();
        }
        else if (gameplayChanged && intervalElapsed)
        {
            WriteLiveStateSnapshot();
        }
    }

    void OnSonicHudSpeedReadoutValue(
        uint32_t ownerAddress,
        uint32_t speedKmh,
        std::string_view hookSource)
    {
        if (!g_isEnabled || !IsPlausibleGuestPointer(ownerAddress))
            return;

        const std::string source = hookSource.empty()
            ? std::string("generated-PPC:sub_824D6418 -> sub_8251A568 return")
            : std::string(hookSource);
        const std::string key = HexU32(ownerAddress);

        bool valueChanged = false;
        bool intervalElapsed = false;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            auto existing = g_lastSonicHudSpeedReadoutValues.find(key);
            valueChanged =
                existing == g_lastSonicHudSpeedReadoutValues.end() ||
                existing->second != speedKmh;
            if (valueChanged)
                g_lastSonicHudSpeedReadoutValues[key] = speedKmh;

            auto lastEvidenceFrame = g_lastSonicHudSpeedReadoutEvidenceFrames.find(key);
            intervalElapsed =
                lastEvidenceFrame == g_lastSonicHudSpeedReadoutEvidenceFrames.end() ||
                g_presentedFrameCount >=
                    lastEvidenceFrame->second + kSonicHudUpdateCallsiteMinEvidenceIntervalFrames;
            if (intervalElapsed)
                g_lastSonicHudSpeedReadoutEvidenceFrames[key] = g_presentedFrameCount;
        }

        const bool gameplayChanged =
            ApplySonicHudSpeedReadoutValueToGameplayValues(ownerAddress, speedKmh, source);

        if (valueChanged || intervalElapsed)
        {
            WriteEvidenceEvent(
                "sonic-hud-speed-readout-value",
                "value=speedKmh status=runtime-proven-via-sub_8251A568-return source=" +
                    source +
                    " owner=" + HexU32(ownerAddress) +
                    " normalizedValue=" + std::to_string(speedKmh));
            WriteLiveStateSnapshot();
        }
        else if (gameplayChanged && intervalElapsed)
        {
            WriteLiveStateSnapshot();
        }
    }

    void OnCsdChildNodeLookupResolved(
        uint32_t resultOwnerAddress,
        uint32_t parentNodeAddress,
        std::string_view childName,
        std::string_view hookSource)
    {
        if (
            !g_isEnabled ||
            !IsPlausibleGuestPointer(resultOwnerAddress) ||
            !IsPlausibleGuestPointer(parentNodeAddress) ||
            childName.empty())
        {
            return;
        }

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);

        CsdChildNodeLookupObservation observation;
        observation.resultOwnerAddress = resultOwnerAddress;
        observation.parentNodeAddress = parentNodeAddress;
        observation.childName = std::string(childName);
        observation.parentPath = ResolveAnyCsdNodePathLocked(parentNodeAddress);
        if (!observation.parentPath.empty())
            observation.path = observation.parentPath + "/" + observation.childName;
        observation.hookSource = hookSource.empty()
            ? std::string("CSD::CNode::GetChild/sub_830BCCA8")
            : std::string(hookSource);
        observation.updateContext = CurrentSonicHudUpdateContext();
        observation.frame = g_presentedFrameCount;

        g_csdChildNodeLookupObservations[resultOwnerAddress] = std::move(observation);
        TrimCsdLookupObservationMapsLocked();
    }

    void OnCsdNodePointerResolved(
        uint32_t sourceOwnerAddress,
        uint32_t nodeAddress,
        std::string_view hookSource)
    {
        if (
            !g_isEnabled ||
            !IsPlausibleGuestPointer(sourceOwnerAddress) ||
            !IsPlausibleGuestPointer(nodeAddress))
        {
            return;
        }

        bool shouldLog = false;
        CsdNodeSourceOwnerObservation loggedObservation;

        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);

            CsdNodeSourceOwnerObservation observation;
            observation.sourceOwnerAddress = sourceOwnerAddress;
            observation.nodeAddress = nodeAddress;
            observation.hookSource = hookSource.empty()
                ? std::string("CSD::RCPtr::Get/sub_830BA228")
                : std::string(hookSource);
            observation.updateContext = CurrentSonicHudUpdateContext();
            observation.sourceOwnerOffsetFromUpdateOwner = SourceOwnerOffsetFromUpdateOwner(
                sourceOwnerAddress,
                observation.updateContext);
            observation.frame = g_presentedFrameCount;

            const auto lookup = g_csdChildNodeLookupObservations.find(sourceOwnerAddress);
            if (lookup != g_csdChildNodeLookupObservations.end())
            {
                observation.parentNodeAddress = lookup->second.parentNodeAddress;
                observation.childName = lookup->second.childName;
                observation.path = lookup->second.path;
                if (observation.path.empty() && !lookup->second.parentPath.empty())
                    observation.path = lookup->second.parentPath + "/" + observation.childName;
            }

            if (observation.path.empty())
                observation.path = ResolveAnyCsdNodePathLocked(nodeAddress);

            g_csdNodeSourceOwnerObservations[nodeAddress] = observation;
            TrimCsdLookupObservationMapsLocked();

            if (
                !observation.path.empty() &&
                observation.path.starts_with("ui_playscreen/") &&
                (IsSonicHudValueTextPath(observation.path) || IsSonicHudGaugeOrPromptPath(observation.path)))
            {
                const std::string logKey =
                    HexU32(nodeAddress) + "|" + observation.path + "|" +
                    HexU32(sourceOwnerAddress) + "|" + std::to_string(g_routeGeneration);
                shouldLog = g_loggedSonicHudNodeSourceOwnerKeys.insert(logKey).second;
                loggedObservation = observation;
            }
        }

        if (shouldLog)
        {
            WriteEvidenceEvent(
                "sonic-hud-node-source-owner-resolved",
                "path=" + loggedObservation.path +
                " node=" + HexU32(loggedObservation.nodeAddress) +
                " sourceOwnerAddress=" + HexU32(loggedObservation.sourceOwnerAddress) +
                " sourceOwnerOffsetFromUpdateOwner=" +
                    OptionalOffsetText(loggedObservation.sourceOwnerOffsetFromUpdateOwner) +
                " updateContext=\"" + loggedObservation.updateContext.hookSource + "\"" +
                " source=" + loggedObservation.hookSource +
                " pathResolutionSource=csd-child-lookup-chain");
        }
    }

    void OnCsdNodeSetText(
        uint32_t nodeAddress,
        uint32_t textAddress,
        std::string_view textUtf8,
        std::string_view hookSource)
    {
        if (!g_isEnabled || textUtf8.empty())
            return;

        const std::string source = hookSource.empty()
            ? std::string("CSD::CNode::SetText/sub_830BF640")
            : std::string(hookSource);

        std::string pathResolutionSource;
        const std::string path = ResolveSonicHudValuePathFromCsdNode(nodeAddress, &pathResolutionSource);
        if (path.empty())
        {
            RecordUnresolvedSonicHudNodeWrite(
                "text",
                nodeAddress,
                textAddress,
                textUtf8,
                false,
                0.0,
                source);
            return;
        }

        const std::string valueName = SonicHudValueNameFromTextPath(path);
        const std::string logKey =
            path + "|" + std::string(textUtf8) + "|" + std::to_string(g_routeGeneration);

        bool shouldLogTextWrite = false;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);

            SonicHudValueWriteObservation observation;
            observation.valueName = valueName;
            observation.path = path;
            observation.pathResolutionSource = pathResolutionSource;
            observation.nodeAddress = nodeAddress;
            observation.textAddress = textAddress;
            observation.textUtf8 = std::string(textUtf8);
            observation.hookSource = source;
            observation.frame = g_presentedFrameCount;

            auto existing = std::find_if(
                g_sonicHudValueWriteObservations.begin(),
                g_sonicHudValueWriteObservations.end(),
                [&](const SonicHudValueWriteObservation& current)
                {
                    return current.path == observation.path;
                });

            if (existing != g_sonicHudValueWriteObservations.end())
            {
                *existing = std::move(observation);
            }
            else if (g_sonicHudValueWriteObservations.size() < kSonicHudValueWriteObservationLimit)
            {
                g_sonicHudValueWriteObservations.push_back(std::move(observation));
            }

            shouldLogTextWrite = g_loggedSonicHudValueTextWriteKeys.insert(logKey).second;
        }

        if (shouldLogTextWrite)
        {
            WriteEvidenceEvent(
                "sonic-hud-value-text-write",
                "value=" + valueName +
                " path=" + path +
                " node=" + HexU32(nodeAddress) +
                " text=\"" + std::string(textUtf8) + "\"" +
                " pathResolutionSource=" + pathResolutionSource +
                " source=" + source);
        }

        const bool changed = ApplySonicHudTextWriteToGameplayValues(path, textUtf8, source);
        if (shouldLogTextWrite || changed)
            WriteLiveStateSnapshot();
    }

    void OnCsdNodeSetPatternIndex(
        uint32_t nodeAddress,
        uint32_t patternIndex,
        std::string_view hookSource)
    {
        if (!g_isEnabled)
            return;

        const std::string source = hookSource.empty()
            ? std::string("CSD::CNode::SetPatternIndex/sub_830BF300")
            : std::string(hookSource);
        const std::string patternText = std::to_string(patternIndex);

        std::string pathResolutionSource;
        const std::string path = ResolveSonicHudGaugeOrPromptPathFromCsdNode(nodeAddress, &pathResolutionSource);
        if (path.empty())
        {
            RecordUnresolvedSonicHudNodeWrite(
                "pattern-index",
                nodeAddress,
                0,
                patternText,
                true,
                static_cast<double>(patternIndex),
                source);
            return;
        }

        const std::string valueName = SonicHudValueNameFromGaugeOrPromptPath(path);

        const bool shouldLog = RecordSonicHudNodeWriteObservation(
            "pattern-index",
            valueName,
            path,
            nodeAddress,
            0,
            patternText,
            true,
            static_cast<double>(patternIndex),
            source,
            patternText,
            true,
            pathResolutionSource);

        const bool changed = ApplySonicHudGaugeOrPromptWriteToGameplayValues(
            path,
            "pattern-index",
            true,
            static_cast<double>(patternIndex),
            source);

        if (shouldLog)
        {
            WriteEvidenceEvent(
                "sonic-hud-gauge-pattern-write",
                "value=" + valueName +
                " path=" + path +
                " node=" + HexU32(nodeAddress) +
                " pattern=" + patternText +
                " source=" + source);
        }

        if (shouldLog || changed)
            WriteLiveStateSnapshot();
    }

    void OnCsdNodeSetHideFlag(
        uint32_t nodeAddress,
        uint32_t hideFlag,
        std::string_view hookSource)
    {
        if (!g_isEnabled)
            return;

        const std::string source = hookSource.empty()
            ? std::string("CSD::CNode::SetHideFlag/sub_830BF080")
            : std::string(hookSource);
        const std::string hideText = std::to_string(hideFlag);

        std::string pathResolutionSource;
        const std::string path = ResolveSonicHudGaugeOrPromptPathFromCsdNode(nodeAddress, &pathResolutionSource);
        if (path.empty())
        {
            RecordUnresolvedSonicHudNodeWrite(
                "hide-flag",
                nodeAddress,
                0,
                hideText,
                true,
                static_cast<double>(hideFlag),
                source);
            return;
        }

        const std::string valueName = SonicHudValueNameFromGaugeOrPromptPath(path);

        const bool shouldLog = RecordSonicHudNodeWriteObservation(
            "hide-flag",
            valueName,
            path,
            nodeAddress,
            0,
            hideText,
            true,
            static_cast<double>(hideFlag),
            source,
            hideText,
            true,
            pathResolutionSource);

        const bool changed = ApplySonicHudGaugeOrPromptWriteToGameplayValues(
            path,
            "hide-flag",
            true,
            static_cast<double>(hideFlag),
            source);

        if (shouldLog)
        {
            WriteEvidenceEvent(
                "sonic-hud-gauge-hide-write",
                "value=" + valueName +
                " path=" + path +
                " node=" + HexU32(nodeAddress) +
                " hide=" + hideText +
                " source=" + source);
        }

        if (shouldLog || changed)
            WriteLiveStateSnapshot();
    }

    void OnCsdNodeSetScale(
        uint32_t nodeAddress,
        float scaleX,
        float scaleY,
        std::string_view hookSource)
    {
        if (!g_isEnabled)
            return;

        const std::string source = hookSource.empty()
            ? std::string("CSD::CNode::SetScale/sub_830BF090")
            : std::string(hookSource);

        std::ostringstream value;
        value << scaleX << "," << scaleY;

        std::string pathResolutionSource;
        const std::string path = ResolveSonicHudGaugeOrPromptPathFromCsdNode(nodeAddress, &pathResolutionSource);
        if (path.empty())
        {
            RecordUnresolvedSonicHudNodeWrite(
                "scale",
                nodeAddress,
                0,
                value.str(),
                true,
                static_cast<double>(scaleX),
                source);
            return;
        }

        const std::string valueName = SonicHudValueNameFromGaugeOrPromptPath(path);

        const bool shouldLog = RecordSonicHudNodeWriteObservation(
            "scale",
            valueName,
            path,
            nodeAddress,
            0,
            value.str(),
            true,
            static_cast<double>(scaleX),
            source,
            value.str(),
            true,
            pathResolutionSource);

        const bool changed = ApplySonicHudGaugeOrPromptWriteToGameplayValues(
            path,
            "scale",
            true,
            static_cast<double>(scaleX),
            source);

        if (shouldLog)
        {
            WriteEvidenceEvent(
                "sonic-hud-gauge-scale-write",
                "value=" + valueName +
                " path=" + path +
                " node=" + HexU32(nodeAddress) +
                " scale=" + value.str() +
                " source=" + source);
        }

        // Phase 198: same-frame join between the Phase 197 owner-field cache
        // and the SetScale write on a visible boost / ring-energy gauge fill
        // cast node. Only emit when (a) the path is one of the two gauge
        // semantic prefixes, (b) the cache is populated and within the
        // frame window, and (c) the (path, scale, ownerSignature) signature
        // has changed since the last emission.
        const bool isGaugeFillPath =
            path.rfind("ui_playscreen/so_speed_gauge", 0) == 0 ||
            path.rfind("ui_playscreen/so_ringenagy_gauge", 0) == 0;
        if (isGaugeFillPath)
        {
            OwnerFieldGaugeSnapshotCache snapshot;
            {
                std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
                snapshot = g_lastOwnerFieldGaugeSnapshot;
            }
            if (snapshot.populated && IsPlausibleGuestPointer(snapshot.ownerAddress))
            {
                const uint64_t frameDelta = g_presentedFrameCount >= snapshot.frame
                    ? g_presentedFrameCount - snapshot.frame
                    : 0;
                if (frameDelta <= kOwnerFieldGaugeScaleCorrelationFrameWindow)
                {
                    std::ostringstream signatureStream;
                    signatureStream << path
                        << "|scale=" << scaleX
                        << "|owner=" << HexU32(snapshot.ownerAddress)
                        << "|f460=" << HexU32(snapshot.field460)
                        << "|f464=" << HexU32(snapshot.field464)
                        << "|f468=" << HexU32(snapshot.field468)
                        << "|f472=" << HexU32(snapshot.field472)
                        << "|f480=" << HexU32(snapshot.field480);
                    const std::string signature = signatureStream.str();

                    if (signature != g_lastOwnerFieldGaugeScaleCorrelationSignature)
                    {
                        std::ostringstream scaleStream;
                        scaleStream.setf(std::ios::fixed);
                        scaleStream.precision(3);
                        scaleStream << scaleX;

                        WriteEvidenceEvent(
                            "sonic-hud-gauge-scale-owner-correlated",
                            "path=" + path +
                            " node=" + HexU32(nodeAddress) +
                            " scale=" + scaleStream.str() +
                            " ownerAddress=" + HexU32(snapshot.ownerAddress) +
                            " ownerField460=" + std::to_string(snapshot.field460) +
                            " ownerField464=" + std::to_string(snapshot.field464) +
                            " ownerField468=" + std::to_string(snapshot.field468) +
                            " ownerField472=" + std::to_string(snapshot.field472) +
                            " ownerField480=" + std::to_string(snapshot.field480) +
                            " frameDelta=" + std::to_string(frameDelta) +
                            " source=runtime-csd-node-set-scale-owner-field-join:sub_830BF090");

                        g_lastOwnerFieldGaugeScaleCorrelationSignature = signature;
                        g_lastOwnerFieldGaugeScaleCorrelationEvidenceFrame = g_presentedFrameCount;
                    }
                }
            }
        }

        if (shouldLog || changed)
            WriteLiveStateSnapshot();
    }

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
        float halfPixelOffsetY)
    {
        if (!g_isEnabled)
            return;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);

        if (g_runtimeGpuSubmitFrame != g_presentedFrameCount)
        {
            g_runtimeGpuSubmitFrame = g_presentedFrameCount;
            g_runtimeGpuSubmitCalls.clear();
            g_runtimeGpuSubmitSequence = 0;
            g_runtimeGpuSubmitDroppedCount = 0;
        }

        ++g_runtimeGpuSubmitSequence;
        if (g_runtimeGpuSubmitCalls.size() >= kRuntimeGpuSubmitCallSampleLimit)
        {
            ++g_runtimeGpuSubmitDroppedCount;
            return;
        }

        RuntimeGpuSubmitCall call;
        call.frame = g_presentedFrameCount;
        call.sequence = g_runtimeGpuSubmitSequence;
        call.source = source.empty() ? std::string("unknown") : std::string(source);
        call.primitiveType = primitiveType;
        call.primitiveTopology = primitiveTopology;
        call.indexed = indexed;
        call.inlineVertexStream = inlineVertexStream;
        call.vertexCount = vertexCount;
        call.indexCount = indexCount;
        call.instanceCount = instanceCount;
        call.startVertex = startVertex;
        call.startIndex = startIndex;
        call.baseVertex = baseVertex;
        call.vertexStride = vertexStride;
        call.texture2DDescriptorIndex = texture2DDescriptorIndex;
        call.samplerDescriptorIndex = samplerDescriptorIndex;
        call.alphaBlendEnable = alphaBlendEnable;
        call.srcBlend = srcBlend;
        call.destBlend = destBlend;
        call.blendOp = blendOp;
        call.srcBlendAlpha = srcBlendAlpha;
        call.destBlendAlpha = destBlendAlpha;
        call.blendOpAlpha = blendOpAlpha;
        call.colorWriteEnable = colorWriteEnable;
        call.alphaTestEnable = alphaTestEnable;
        call.alphaThreshold = alphaThreshold;
        call.scissorEnable = scissorEnable;
        call.scissorLeft = scissorLeft;
        call.scissorTop = scissorTop;
        call.scissorRight = scissorRight;
        call.scissorBottom = scissorBottom;
        call.samplerMinFilter = samplerMinFilter;
        call.samplerMagFilter = samplerMagFilter;
        call.samplerMipMode = samplerMipMode;
        call.samplerAddressU = samplerAddressU;
        call.samplerAddressV = samplerAddressV;
        call.samplerAddressW = samplerAddressW;
        call.halfPixelOffsetX = halfPixelOffsetX;
        call.halfPixelOffsetY = halfPixelOffsetY;

        g_runtimeGpuSubmitCalls.push_back(std::move(call));
    }

    void OnBackendTextureDescriptorResolved(
        uint32_t descriptorIndex,
        std::string_view source,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint32_t format,
        uint32_t viewDimension,
        uint32_t layout)
    {
        if (!g_isEnabled)
            return;

        RuntimeTextureDescriptorSemantic descriptor;
        descriptor.frame = g_presentedFrameCount;
        descriptor.descriptorIndex = descriptorIndex;
        descriptor.source = source.empty() ? std::string("unknown") : std::string(source);
        descriptor.width = width;
        descriptor.height = height;
        descriptor.depth = depth;
        descriptor.format = format;
        descriptor.viewDimension = viewDimension;
        descriptor.layout = layout;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        g_runtimeTextureDescriptorSemantics[descriptorIndex] = std::move(descriptor);
    }

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
        float maxLod)
    {
        if (!g_isEnabled)
            return;

        RuntimeSamplerDescriptorSemantic descriptor;
        descriptor.frame = g_presentedFrameCount;
        descriptor.descriptorIndex = descriptorIndex;
        descriptor.minFilter = minFilter;
        descriptor.magFilter = magFilter;
        descriptor.mipMode = mipMode;
        descriptor.addressU = addressU;
        descriptor.addressV = addressV;
        descriptor.addressW = addressW;
        descriptor.mipLodBias = mipLodBias;
        descriptor.maxAnisotropy = maxAnisotropy;
        descriptor.anisotropyEnabled = anisotropyEnabled;
        descriptor.comparisonEnabled = comparisonEnabled;
        descriptor.borderColor = borderColor;
        descriptor.minLod = minLod;
        descriptor.maxLod = maxLod;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        g_runtimeSamplerDescriptorSemantics[descriptorIndex] = descriptor;
    }

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
        std::string_view source)
    {
        if (!g_isEnabled)
            return;

        RuntimeVendorTextureResourceView resourceView;
        resourceView.frame = g_presentedFrameCount;
        resourceView.descriptorIndex = descriptorIndex;
        resourceView.backend = backend.empty() ? std::string("unknown") : std::string(backend);
        resourceView.source = source.empty() ? std::string("unknown") : std::string(source);
        resourceView.nativeTextureResourceHandle = nativeTextureResourceHandle;
        resourceView.nativeTextureViewHandle = nativeTextureViewHandle;
        resourceView.nativeFormat = nativeFormat;
        resourceView.nativeViewDimension = nativeViewDimension;
        resourceView.width = width;
        resourceView.height = height;
        resourceView.mipLevels = mipLevels;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        g_runtimeVendorTextureResourceViews[descriptorIndex] = std::move(resourceView);
    }

    void OnVendorSamplerResourceViewResolved(
        std::string_view backend,
        uint32_t descriptorIndex,
        uint64_t nativeSamplerHandle,
        uint32_t nativeFilter,
        uint32_t nativeAddressU,
        uint32_t nativeAddressV,
        uint32_t nativeAddressW,
        std::string_view source)
    {
        if (!g_isEnabled)
            return;

        RuntimeVendorSamplerResourceView resourceView;
        resourceView.frame = g_presentedFrameCount;
        resourceView.descriptorIndex = descriptorIndex;
        resourceView.backend = backend.empty() ? std::string("unknown") : std::string(backend);
        resourceView.source = source.empty() ? std::string("unknown") : std::string(source);
        resourceView.nativeSamplerHandle = nativeSamplerHandle;
        resourceView.nativeFilter = nativeFilter;
        resourceView.nativeAddressU = nativeAddressU;
        resourceView.nativeAddressV = nativeAddressV;
        resourceView.nativeAddressW = nativeAddressW;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        g_runtimeVendorSamplerResourceViews[descriptorIndex] = std::move(resourceView);
    }

    void OnRawBackendCommand(
        std::string_view backend,
        std::string_view command,
        std::string_view source,
        bool indexed,
        uint32_t vertexCount,
        uint32_t indexCount,
        uint32_t instanceCount)
    {
        if (!g_isEnabled)
            return;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);

        if (g_runtimeRawBackendCommandFrame != g_presentedFrameCount)
        {
            g_runtimeRawBackendCommandFrame = g_presentedFrameCount;
            g_runtimeRawBackendCommands.clear();
            g_runtimeRawBackendCommandSequence = 0;
            g_runtimeRawBackendCommandDroppedCount = 0;
        }

        ++g_runtimeRawBackendCommandSequence;
        if (g_runtimeRawBackendCommands.size() >= kRuntimeRawBackendCommandSampleLimit)
        {
            ++g_runtimeRawBackendCommandDroppedCount;
            return;
        }

        RuntimeRawBackendCommand rawCommand;
        rawCommand.frame = g_presentedFrameCount;
        rawCommand.sequence = g_runtimeRawBackendCommandSequence;
        rawCommand.backend = backend.empty() ? std::string("unknown") : std::string(backend);
        rawCommand.command = command.empty() ? std::string("RHI command-list boundary") : std::string(command);
        rawCommand.source = source.empty() ? std::string("unknown") : std::string(source);
        rawCommand.indexed = indexed;
        rawCommand.vertexCount = vertexCount;
        rawCommand.indexCount = indexCount;
        rawCommand.instanceCount = instanceCount;
        g_runtimeRawBackendCommands.push_back(std::move(rawCommand));
    }

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
        bool alphaToCoverageEnabled)
    {
        if (!g_isEnabled)
            return;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);

        if (g_runtimeBackendResolvedFrame != g_presentedFrameCount)
        {
            g_runtimeBackendResolvedFrame = g_presentedFrameCount;
            g_runtimeBackendResolvedSubmits.clear();
            g_runtimeBackendResolvedSequence = 0;
            g_runtimeBackendResolvedDroppedCount = 0;
        }

        ++g_runtimeBackendResolvedSequence;
        if (g_runtimeBackendResolvedSubmits.size() >= kRuntimeBackendResolvedSubmitSampleLimit)
        {
            ++g_runtimeBackendResolvedDroppedCount;
            return;
        }

        RuntimeBackendResolvedSubmit submit;
        submit.frame = g_presentedFrameCount;
        submit.sequence = g_runtimeBackendResolvedSequence;
        submit.backend = backend.empty() ? std::string("unknown") : std::string(backend);
        submit.nativeCommand = nativeCommand.empty() ? std::string("unknown") : std::string(nativeCommand);
        submit.indexed = indexed;
        submit.vertexCount = vertexCount;
        submit.indexCount = indexCount;
        submit.instanceCount = instanceCount;
        submit.nativePipelineHandle = nativePipelineHandle;
        submit.nativePipelineLayoutHandle = nativePipelineLayoutHandle;
        submit.resolvedPipelineKnown = resolvedPipelineKnown;
        submit.activeFramebufferKnown = activeFramebufferKnown;
        submit.framebufferWidth = framebufferWidth;
        submit.framebufferHeight = framebufferHeight;
        submit.renderTargetCount = renderTargetCount;
        submit.renderTargetFormat0 = renderTargetFormat0;
        submit.depthTargetFormat = depthTargetFormat;
        submit.sampleCount = sampleCount;
        submit.primitiveTopology = primitiveTopology;
        submit.blendEnabled = blendEnabled;
        submit.srcBlend = srcBlend;
        submit.destBlend = destBlend;
        submit.blendOp = blendOp;
        submit.srcBlendAlpha = srcBlendAlpha;
        submit.destBlendAlpha = destBlendAlpha;
        submit.blendOpAlpha = blendOpAlpha;
        submit.renderTargetWriteMask = renderTargetWriteMask;
        submit.inputSlotCount = inputSlotCount;
        submit.inputElementCount = inputElementCount;
        submit.depthEnabled = depthEnabled;
        submit.depthWriteEnabled = depthWriteEnabled;
        submit.alphaToCoverageEnabled = alphaToCoverageEnabled;

        g_runtimeBackendResolvedSubmits.push_back(std::move(submit));
    }

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
        std::string_view hookSource)
    {
        if (!g_isEnabled || !IsPlausibleGuestPointer(ownerAddress))
            return;

        if (!IsPlausibleGuestPointer(playScreenProjectAddress))
            playScreenProjectAddress = 0;
        if (!IsPlausibleGuestPointer(speedGaugeSceneAddress))
            speedGaugeSceneAddress = 0;
        if (!IsPlausibleGuestPointer(ringEnergyGaugeSceneAddress))
            ringEnergyGaugeSceneAddress = 0;
        if (!IsPlausibleGuestPointer(gaugeFrameSceneAddress))
            gaugeFrameSceneAddress = 0;
        if (!IsPlausibleGuestPointer(scoreCountNodeAddress))
            scoreCountNodeAddress = 0;
        if (!IsPlausibleGuestPointer(timeCountNodeAddress))
            timeCountNodeAddress = 0;
        if (!IsPlausibleGuestPointer(timeCount2NodeAddress))
            timeCount2NodeAddress = 0;
        if (!IsPlausibleGuestPointer(timeCount3NodeAddress))
            timeCount3NodeAddress = 0;
        if (!IsPlausibleGuestPointer(playerCountNodeAddress))
            playerCountNodeAddress = 0;

        const bool hasOwnerFields =
            playScreenProjectAddress != 0 ||
            speedGaugeSceneAddress != 0 ||
            ringEnergyGaugeSceneAddress != 0 ||
            gaugeFrameSceneAddress != 0 ||
            scoreCountNodeAddress != 0 ||
            timeCountNodeAddress != 0 ||
            playerCountNodeAddress != 0;

        const bool changed =
            g_chudSonicStageOwnerAddress != ownerAddress ||
            g_chudSonicStagePlayScreenProjectAddress != playScreenProjectAddress ||
            g_chudSonicStageSpeedGaugeSceneAddress != speedGaugeSceneAddress ||
            g_chudSonicStageRingEnergyGaugeSceneAddress != ringEnergyGaugeSceneAddress ||
            g_chudSonicStageGaugeFrameSceneAddress != gaugeFrameSceneAddress ||
            g_chudSonicStageScoreCountNodeAddress != scoreCountNodeAddress ||
            g_chudSonicStageTimeCountNodeAddress != timeCountNodeAddress ||
            g_chudSonicStageTimeCount2NodeAddress != timeCount2NodeAddress ||
            g_chudSonicStageTimeCount3NodeAddress != timeCount3NodeAddress ||
            g_chudSonicStagePlayerCountNodeAddress != playerCountNodeAddress ||
            g_chudSonicStageRawHookSource != hookSource;

        g_chudSonicStageOwnerAddress = ownerAddress;
        g_chudSonicStagePlayScreenProjectAddress = playScreenProjectAddress;
        g_chudSonicStageSpeedGaugeSceneAddress = speedGaugeSceneAddress;
        g_chudSonicStageRingEnergyGaugeSceneAddress = ringEnergyGaugeSceneAddress;
        g_chudSonicStageGaugeFrameSceneAddress = gaugeFrameSceneAddress;
        g_chudSonicStageScoreCountNodeAddress = scoreCountNodeAddress;
        g_chudSonicStageTimeCountNodeAddress = timeCountNodeAddress;
        g_chudSonicStageTimeCount2NodeAddress = timeCount2NodeAddress;
        g_chudSonicStageTimeCount3NodeAddress = timeCount3NodeAddress;
        g_chudSonicStagePlayerCountNodeAddress = playerCountNodeAddress;
        g_chudSonicStageRawHookFrame = g_presentedFrameCount;
        g_chudSonicStageRawHookSource = hookSource;

        const std::string stableSignature =
            std::string(hasOwnerFields ? "fields-ready" : "owner-only") +
            "|play=" + HexU32(playScreenProjectAddress) +
            "|speed=" + HexU32(speedGaugeSceneAddress) +
            "|energy=" + HexU32(ringEnergyGaugeSceneAddress) +
            "|gauge=" + HexU32(gaugeFrameSceneAddress) +
            "|score=" + HexU32(scoreCountNodeAddress) +
            "|time=" + HexU32(timeCountNodeAddress) +
            "|player=" + HexU32(playerCountNodeAddress);
        const bool ownerFieldsBecameReady =
            hasOwnerFields && !g_loggedChudSonicStageOwnerFieldsReady;
        const bool evidenceIntervalElapsed =
            g_chudSonicStageOwnerHookLastEvidenceFrame == 0 ||
            g_presentedFrameCount >=
                g_chudSonicStageOwnerHookLastEvidenceFrame +
                    kSonicHudUpdateCallsiteMinEvidenceIntervalFrames;
        const bool stableSignatureChanged =
            g_chudSonicStageOwnerHookStableSignature != stableSignature;

        if (
            !g_loggedChudSonicStageOwnerHook ||
            ownerFieldsBecameReady ||
            (changed && stableSignatureChanged && evidenceIntervalElapsed))
        {
            WriteEvidenceEvent(
                "sonic-hud-owner-hooked",
                "owner=" + HexU32(ownerAddress) +
                " play_screen=" + HexU32(playScreenProjectAddress) +
                " speed_gauge=" + HexU32(speedGaugeSceneAddress) +
                " ring_energy_gauge=" + HexU32(ringEnergyGaugeSceneAddress) +
                " gauge_frame=" + HexU32(gaugeFrameSceneAddress) +
                " score_count=" + HexU32(scoreCountNodeAddress) +
                " time_count=" + HexU32(timeCountNodeAddress) +
                " time_count2=" + HexU32(timeCount2NodeAddress) +
                " time_count3=" + HexU32(timeCount3NodeAddress) +
                " player_count=" + HexU32(playerCountNodeAddress) +
                " owner_fields_ready=" + std::string(hasOwnerFields ? "1" : "0") +
                " source=" + std::string(hookSource) +
                (hasOwnerFields
                    ? " status=raw CHudSonicStage owner hook"
                    : " status=raw CHudSonicStage owner hook owner-only; CSD owner fields pending"));
            g_loggedChudSonicStageOwnerHook = true;
            if (hasOwnerFields)
                g_loggedChudSonicStageOwnerFieldsReady = true;
            g_chudSonicStageOwnerHookStableSignature = stableSignature;
            g_chudSonicStageOwnerHookLastEvidenceFrame = g_presentedFrameCount;
            WriteLiveStateSnapshot();
        }
    }

    void OnHudSonicStageOwnerFieldSample(uint32_t ownerAddress, std::string_view hookSource)
    {
        if (!g_isEnabled || !IsPlausibleGuestPointer(ownerAddress))
            return;

        std::vector<SonicHudOwnerFieldSample> samples;
        samples.reserve(kChudSonicStageExpectedOwnerFields.size());

        uint32_t resolvedMemoryCount = 0;
        for (const auto& field : kChudSonicStageExpectedOwnerFields)
        {
            SonicHudOwnerFieldSample sample;
            sample.field = field.field;
            sample.sampleOffset = field.rcObjectOffset;
            sample.frame = g_presentedFrameCount;
            sample.hookSource = std::string(hookSource);

            uint32_t slotValue = 0;
            if (TryReadGuestU32(ownerAddress + field.rcObjectOffset, slotValue))
                sample.slotValue = slotValue;

            sample.rcObjectAddress = sample.slotValue;
            sample.rcObjectKnown = IsPlausibleGuestPointer(sample.rcObjectAddress);
            // Do not dereference the RCObject pointer from this raw owner
            // sampler. The CHudSonicStage owner is live before every embedded
            // RCPtr slot is guaranteed to be a stable CSD RCObject, and the
            // real CSD node/project addresses are resolved by tree/draw-list
            // hooks that run at the CSD boundary.

            samples.push_back(std::move(sample));
        }

        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            changed = g_chudSonicStageOwnerFieldSamples.size() != samples.size();
            if (!changed)
            {
                for (size_t i = 0; i < samples.size(); ++i)
                {
                    if (g_chudSonicStageOwnerFieldSamples[i].slotValue != samples[i].slotValue ||
                        g_chudSonicStageOwnerFieldSamples[i].resolvedMemoryAddress != samples[i].resolvedMemoryAddress ||
                        g_chudSonicStageOwnerFieldSamples[i].hookSource != samples[i].hookSource)
                    {
                        changed = true;
                        break;
                    }
                }
            }

            g_chudSonicStageOwnerFieldSamples = std::move(samples);
            g_chudSonicStageOwnerFieldSampleCount += kChudSonicStageExpectedOwnerFields.size();
        }

        const std::string stableSignature =
            "resolved_memory_count=" + std::to_string(resolvedMemoryCount);
        const bool evidenceIntervalElapsed =
            g_chudSonicStageOwnerFieldSampleLastEvidenceFrame == 0 ||
            g_presentedFrameCount >=
                g_chudSonicStageOwnerFieldSampleLastEvidenceFrame +
                    kSonicHudUpdateCallsiteMinEvidenceIntervalFrames;
        const bool stableSignatureChanged =
            g_chudSonicStageOwnerFieldSampleStableSignature != stableSignature;

        if (
            !g_loggedChudSonicStageOwnerFieldSample ||
            (resolvedMemoryCount != 0 && stableSignatureChanged) ||
            (changed && stableSignatureChanged && evidenceIntervalElapsed))
        {
            WriteEvidenceEvent(
                "sonic-hud-owner-field-sample",
                "owner=" + HexU32(ownerAddress) +
                " source=" + std::string(hookSource) +
                " expected=" + std::string(kChudSonicStageExpectedOwnerFieldSource) +
                " sample_count=" + std::to_string(kChudSonicStageExpectedOwnerFields.size()) +
                " resolved_memory_count=" + std::to_string(resolvedMemoryCount));
            g_loggedChudSonicStageOwnerFieldSample = true;
            g_chudSonicStageOwnerFieldSampleStableSignature = stableSignature;
            g_chudSonicStageOwnerFieldSampleLastEvidenceFrame = g_presentedFrameCount;
            EmitTutorialHudOwnerPathReadyIfNeeded();
            WriteLiveStateSnapshot();
        }

        EmitStageTargetReadyIfNeeded();
    }

    void OnHudSonicStageOwnerFieldGaugeSnapshot(uint32_t ownerAddress, std::string_view callsite)
    {
        if (!g_isEnabled || !IsPlausibleGuestPointer(ownerAddress))
            return;

        // Phase 197: snapshot CHudSonicStage owner-field dwords at the staging
        // block consumed by the sub_824D6C18 rolling counter / gauge-state path.
        // Honest scope: these five offsets co-occur with same-frame boost and
        // ring-energy text writes; their per-offset semantics (integer counter
        // vs. float fill scale vs. hide/style enum vs. animation phase) are
        // intentionally NOT claimed here — that classification is left to the
        // summarizer + reusable controller until the exact semantic is proven.
        static constexpr int kOwnerFieldOffsets[] = { 460, 464, 468, 472, 480 };
        constexpr size_t kOwnerFieldCount = sizeof(kOwnerFieldOffsets) / sizeof(kOwnerFieldOffsets[0]);

        uint32_t fieldValues[kOwnerFieldCount] = {};
        bool fieldReadOk[kOwnerFieldCount] = {};
        size_t resolvedFieldCount = 0;
        for (size_t i = 0; i < kOwnerFieldCount; ++i)
        {
            uint32_t value = 0;
            if (TryReadGuestU32(ownerAddress + kOwnerFieldOffsets[i], value))
            {
                fieldValues[i] = value;
                fieldReadOk[i] = true;
                ++resolvedFieldCount;
            }
        }

        if (resolvedFieldCount == 0)
            return;

        // Phase 198: refresh the cache unconditionally so the SetScale join
        // can use the freshest owner-field state, independent of the Phase
        // 197 emission throttle below.
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            g_lastOwnerFieldGaugeSnapshot.ownerAddress = ownerAddress;
            g_lastOwnerFieldGaugeSnapshot.field460 = fieldReadOk[0] ? fieldValues[0] : 0;
            g_lastOwnerFieldGaugeSnapshot.field464 = fieldReadOk[1] ? fieldValues[1] : 0;
            g_lastOwnerFieldGaugeSnapshot.field468 = fieldReadOk[2] ? fieldValues[2] : 0;
            g_lastOwnerFieldGaugeSnapshot.field472 = fieldReadOk[3] ? fieldValues[3] : 0;
            g_lastOwnerFieldGaugeSnapshot.field480 = fieldReadOk[4] ? fieldValues[4] : 0;
            g_lastOwnerFieldGaugeSnapshot.frame = g_presentedFrameCount;
            g_lastOwnerFieldGaugeSnapshot.populated = true;
        }

        std::ostringstream signatureStream;
        signatureStream << "owner=" << HexU32(ownerAddress);
        for (size_t i = 0; i < kOwnerFieldCount; ++i)
        {
            signatureStream << ":+" << kOwnerFieldOffsets[i] << "=";
            if (fieldReadOk[i])
                signatureStream << HexU32(fieldValues[i]);
            else
                signatureStream << "?";
        }
        const std::string stableSignature = signatureStream.str();

        const bool evidenceIntervalElapsed =
            g_chudSonicStageOwnerFieldGaugeSnapshotLastEvidenceFrame == 0 ||
            g_presentedFrameCount >=
                g_chudSonicStageOwnerFieldGaugeSnapshotLastEvidenceFrame +
                    kSonicHudUpdateCallsiteMinEvidenceIntervalFrames;
        const bool stableSignatureChanged =
            g_chudSonicStageOwnerFieldGaugeSnapshotStableSignature != stableSignature;

        if (!stableSignatureChanged || !evidenceIntervalElapsed)
            return;

        std::ostringstream offsetsStream;
        std::ostringstream valuesStream;
        std::ostringstream hexesStream;
        for (size_t i = 0; i < kOwnerFieldCount; ++i)
        {
            if (i != 0)
            {
                offsetsStream << ",";
                valuesStream << ",";
                hexesStream << ",";
            }
            offsetsStream << kOwnerFieldOffsets[i];
            if (fieldReadOk[i])
            {
                valuesStream << fieldValues[i];
                hexesStream << HexU32(fieldValues[i]);
            }
            else
            {
                valuesStream << "?";
                hexesStream << "?";
            }
        }

        const std::string callsiteString =
            callsite.empty() ? std::string("sub_824D6C18") : std::string(callsite);

        WriteEvidenceEvent(
            "sonic-hud-owner-gauge-snapshot",
            "ownerAddress=" + HexU32(ownerAddress) +
            " callsite=" + callsiteString +
            " fieldOffsets=" + offsetsStream.str() +
            " fieldValues=" + valuesStream.str() +
            " fieldValueHexes=" + hexesStream.str() +
            " candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge"
            " candidateValueNames=boostGauge|ringEnergyGauge"
            " source=runtime-owner-field-snapshot:" + callsiteString);

        g_chudSonicStageOwnerFieldGaugeSnapshotStableSignature = stableSignature;
        g_chudSonicStageOwnerFieldGaugeSnapshotLastEvidenceFrame = g_presentedFrameCount;
    }

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
        std::string_view hookSource)
    {
        if (!g_isEnabled)
            return;

        const std::string source = hookSource.empty()
            ? std::string("runtime gameplay value hook")
            : std::string(hookSource);

        SonicHudGameplayValueSnapshot snapshot;
        snapshot.ringCountKnown = ringCountKnown;
        snapshot.ringCount = ringCount;
        snapshot.ringCountSource = ringCountKnown ? source : "pending-runtime-field";
        snapshot.scoreKnown = scoreKnown;
        snapshot.score = score;
        snapshot.scoreSource = scoreKnown
            ? source
            : "SWA::CGameDocument::m_pMember->m_ScoreInfo.EnemyScore+TrickScore";
        snapshot.elapsedFramesKnown = elapsedFramesKnown;
        snapshot.elapsedFrames = elapsedFrames;
        snapshot.elapsedFramesSource = elapsedFramesKnown ? source : "pending-runtime-field";
        snapshot.speedKmhKnown = speedKmhKnown;
        snapshot.speedKmh = speedKmh;
        snapshot.speedKmhSource = speedKmhKnown ? source : "pending-runtime-field";
        snapshot.boostGaugeKnown = boostGaugeKnown;
        snapshot.boostGauge = boostGauge;
        snapshot.boostGaugeSource = boostGaugeKnown ? source : "pending-runtime-field";
        snapshot.ringEnergyGaugeKnown = ringEnergyGaugeKnown;
        snapshot.ringEnergyGauge = ringEnergyGauge;
        snapshot.ringEnergyGaugeSource = ringEnergyGaugeKnown ? source : "pending-runtime-field";
        snapshot.lifeCountKnown = lifeCountKnown;
        snapshot.lifeCount = lifeCount;
        snapshot.lifeCountSource = lifeCountKnown ? source : "pending-runtime-field";
        snapshot.tutorialPromptKnown = tutorialPromptKnown;
        snapshot.tutorialPromptId = tutorialPromptKnown && !tutorialPromptId.empty()
            ? std::string(tutorialPromptId)
            : "none";
        snapshot.tutorialVisible = tutorialVisible;
        snapshot.tutorialPromptSource = tutorialPromptKnown ? source : "pending-runtime-field";
        snapshot.frame = g_presentedFrameCount;

        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            changed =
                g_sonicHudGameplayValues.ringCountKnown != snapshot.ringCountKnown ||
                g_sonicHudGameplayValues.ringCount != snapshot.ringCount ||
                g_sonicHudGameplayValues.scoreKnown != snapshot.scoreKnown ||
                g_sonicHudGameplayValues.score != snapshot.score ||
                g_sonicHudGameplayValues.elapsedFramesKnown != snapshot.elapsedFramesKnown ||
                g_sonicHudGameplayValues.elapsedFrames != snapshot.elapsedFrames ||
                g_sonicHudGameplayValues.speedKmhKnown != snapshot.speedKmhKnown ||
                g_sonicHudGameplayValues.speedKmh != snapshot.speedKmh ||
                g_sonicHudGameplayValues.boostGaugeKnown != snapshot.boostGaugeKnown ||
                g_sonicHudGameplayValues.boostGauge != snapshot.boostGauge ||
                g_sonicHudGameplayValues.ringEnergyGaugeKnown != snapshot.ringEnergyGaugeKnown ||
                g_sonicHudGameplayValues.ringEnergyGauge != snapshot.ringEnergyGauge ||
                g_sonicHudGameplayValues.lifeCountKnown != snapshot.lifeCountKnown ||
                g_sonicHudGameplayValues.lifeCount != snapshot.lifeCount ||
                g_sonicHudGameplayValues.tutorialPromptKnown != snapshot.tutorialPromptKnown ||
                g_sonicHudGameplayValues.tutorialPromptId != snapshot.tutorialPromptId ||
                g_sonicHudGameplayValues.tutorialVisible != snapshot.tutorialVisible;

            g_sonicHudGameplayValues = std::move(snapshot);
        }

        if (changed)
        {
            WriteEvidenceEvent(
                "sonic-hud-gameplay-values",
                "ring_known=" + std::string(ringCountKnown ? "1" : "0") +
                " score_known=" + std::string(scoreKnown ? "1" : "0") +
                " timer_known=" + std::string(elapsedFramesKnown ? "1" : "0") +
                " speed_known=" + std::string(speedKmhKnown ? "1" : "0") +
                " boost_known=" + std::string(boostGaugeKnown ? "1" : "0") +
                " energy_known=" + std::string(ringEnergyGaugeKnown ? "1" : "0") +
                " lives_known=" + std::string(lifeCountKnown ? "1" : "0") +
                " tutorial_known=" + std::string(tutorialPromptKnown ? "1" : "0") +
                " source=" + source);
            WriteLiveStateSnapshot();
        }
    }

    void OnHudPauseUpdate(
        uint32_t pauseAddress,
        uint32_t pauseProjectAddress,
        uint32_t bgSceneAddress,
        uint32_t action,
        uint32_t menu,
        uint32_t status,
        uint32_t transition,
        bool isVisible,
        bool isShown)
    {
        if (!g_isEnabled)
            return;

        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            g_pauseGeneralSaveInspector.pauseKnown = true;
            g_pauseGeneralSaveInspector.pauseAddress = pauseAddress;
            g_pauseGeneralSaveInspector.pauseProjectAddress = pauseProjectAddress;
            g_pauseGeneralSaveInspector.pauseBgSceneAddress = bgSceneAddress;
            g_pauseGeneralSaveInspector.pauseAction = action;
            g_pauseGeneralSaveInspector.pauseMenu = menu;
            g_pauseGeneralSaveInspector.pauseStatus = status;
            g_pauseGeneralSaveInspector.pauseTransition = transition;
            g_pauseGeneralSaveInspector.pauseVisible = isVisible;
            g_pauseGeneralSaveInspector.pauseShown = isShown;
            g_pauseGeneralSaveInspector.pauseFrame = g_presentedFrameCount;
        }

        if (g_target == ScreenId::Pause && pauseProjectAddress != 0)
        {
            g_targetCsdObserved = true;

            if (!g_loggedPauseOwnerObserved)
            {
                WriteEvidenceEvent(
                    "pause-owner-observed",
                    "pause_address=" + HexU32(pauseAddress) +
                    " pause_project=" + HexU32(pauseProjectAddress) +
                    " visible=" + std::to_string(isVisible ? 1 : 0) +
                    " shown=" + std::to_string(isShown ? 1 : 0));
                g_loggedPauseOwnerObserved = true;
            }

            if (!g_loggedTargetCsdProjectLive)
            {
                WriteEvidenceEvent("target-csd-project-made", "ui_pause source=CHudPause.m_rcPause");
                g_loggedTargetCsdProjectLive = true;
            }

            if (g_stageContextObserved && !g_loggedStageTargetCsdBound)
            {
                WriteEvidenceEvent(
                    "stage-target-csd-bound",
                    "target_csd=ui_pause stage_context=1 source=CHudPause.m_rcPause");
                g_loggedStageTargetCsdBound = true;
            }

            if ((isVisible || isShown) && !g_loggedStageTargetReady)
            {
                const auto detail =
                    "pause_address=" + HexU32(pauseAddress) +
                    " pause_project=" + HexU32(pauseProjectAddress) +
                    " action=" + std::to_string(action) +
                    " menu=" + std::to_string(menu) +
                    " status=" + std::to_string(status) +
                    " transition=" + std::to_string(transition) +
                    " visible=" + std::to_string(isVisible ? 1 : 0) +
                    " shown=" + std::to_string(isShown ? 1 : 0);
                WriteEvidenceEvent("pause-target-ready", detail);
            }
        }

        EmitStageTargetReadyIfNeeded();
        WriteLiveStateSnapshot();
    }

    void OnGeneralWindowUpdate(
        uint32_t generalWindowAddress,
        uint32_t generalProjectAddress,
        uint32_t bgSceneAddress,
        uint32_t status,
        uint32_t cursorIndex,
        uint32_t selectedIndex)
    {
        if (!g_isEnabled)
            return;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        g_pauseGeneralSaveInspector.generalWindowKnown = true;
        g_pauseGeneralSaveInspector.generalWindowAddress = generalWindowAddress;
        g_pauseGeneralSaveInspector.generalProjectAddress = generalProjectAddress;
        g_pauseGeneralSaveInspector.generalBgSceneAddress = bgSceneAddress;
        g_pauseGeneralSaveInspector.generalWindowStatus = status;
        g_pauseGeneralSaveInspector.generalCursorIndex = cursorIndex;
        g_pauseGeneralSaveInspector.generalSelectedIndex = selectedIndex;
        g_pauseGeneralSaveInspector.generalFrame = g_presentedFrameCount;
    }

    void OnSaveIconUpdate(uint32_t saveIconAddress, bool isVisible)
    {
        if (!g_isEnabled)
            return;

        std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
        g_pauseGeneralSaveInspector.saveIconKnown = true;
        g_pauseGeneralSaveInspector.saveIconAddress = saveIconAddress;
        g_pauseGeneralSaveInspector.saveIconVisible = isVisible;
        g_pauseGeneralSaveInspector.saveIconFrame = g_presentedFrameCount;
    }

    bool ApplyTitleIntroStateForcing(float elapsedSeconds, bool& directState)
    {
        directState = false;

        if (!g_isEnabled || g_observerMode || !g_routePending)
            return false;

        if (!TargetCanRouteFromTitleIntro(g_target))
            return false;

        if (elapsedSeconds < 0.75f)
            return false;

        if (g_titleIntroAcceptInjected && g_routePolicy != RoutePolicy::DirectContext)
            return false;

        if (g_routePolicy == RoutePolicy::DirectContext && g_target != ScreenId::TitleMenu)
        {
            if (g_titleIntroDirectStateApplied)
                return false;

            g_titleIntroAcceptInjected = true;
            g_titleIntroDirectStateApplied = true;
            g_titleIntroDirectStateLastRequestFrame = g_presentedFrameCount;
            directState = true;
            g_routeStatus = "title intro direct state requested";
            LOGFN("SWARD UI Lab route: requested direct title intro state for target={}", TargetFor(g_target).token);
            WriteEvidenceEvent("title-intro-direct-state-requested", "state=1 dirty=1");
            return false;
        }

        g_titleIntroAcceptInjected = true;

        if (g_target == ScreenId::TitleMenu)
        {
            g_titleMenuPressStartAccepted = true;
            g_routeStatus = "title press start accept injected";
            LOGFN("SWARD UI Lab route: injected real Press Start accept for target={}", TargetFor(g_target).token);
            WriteEvidenceEvent("title-press-start-accept-injected");
            return true;
        }

        g_routeStatus = "title accept injected";
        LOGFN("SWARD UI Lab route: injected title accept for target={}", TargetFor(g_target).token);
        WriteEvidenceEvent("title-accept-injected");
        return true;
    }

    bool ShouldArmTitleIntroOwnerOutput()
    {
        return g_isEnabled &&
            !g_observerMode &&
            g_routePolicy == RoutePolicy::DirectContext &&
            TargetShouldRouteThroughLoading(g_target);
    }

    bool ShouldArmTitleIntroCsdCompletion()
    {
        return g_isEnabled &&
            !g_observerMode &&
            g_routePolicy == RoutePolicy::DirectContext &&
            TargetRoutesThroughTitleMenu(g_target);
    }

    bool ShouldHoldTitleMenuRuntime()
    {
        return g_isEnabled &&
            !g_observerMode &&
            g_target == ScreenId::TitleMenu &&
            g_titleMenuVisualReady;
    }

    bool ApplyTitleMenuStateForcing(int32_t& cursorIndex, bool& injectAccept, bool& suppressAccept, bool& directContext)
    {
        injectAccept = false;
        suppressAccept = false;
        directContext = false;

        if (!g_isEnabled || g_observerMode)
            return false;

        if (g_target == ScreenId::TitleMenu)
        {
            cursorIndex = 1;
            suppressAccept = true;

            if (g_routePending)
            {
                g_routePending = false;
                g_routeStatus = "title menu reached";
                LOGN("SWARD UI Lab route: title menu reached.");
                WriteEvidenceEvent("title-menu-reached");
            }

            if (!g_titleMenuPostPressStartHeld)
            {
                g_titleMenuPostPressStartHeld = true;
                g_routeStatus = "title menu post press start held";
                WriteEvidenceEvent("title-menu-post-press-start-held");
            }

            if (!g_titleMenuAcceptSuppressionLogged)
            {
                g_titleMenuAcceptSuppressionLogged = true;
                WriteEvidenceEvent("title-menu-accept-suppressed");
            }

            return true;
        }

        if (g_target == ScreenId::TitleOptions)
        {
            if (!g_routePending)
                return false;

            cursorIndex = 2;
            g_routeStatus = "title options via menu";

            if (!g_titleMenuAcceptInjected)
            {
                g_titleMenuAcceptInjected = true;
                g_routePending = false;
                injectAccept = true;
                g_routeStatus = "title options accept injected";
                LOGN("SWARD UI Lab route: forced title menu Options accept.");
                WriteEvidenceEvent("title-options-accept-injected");
            }

            return true;
        }

        if (!TargetShouldRouteThroughLoading(g_target))
            return false;

        const bool shouldHoldDirectContext =
            g_routePolicy == RoutePolicy::DirectContext &&
            g_titleMenuAcceptInjected &&
            !g_targetCsdObserved &&
            !g_loggedStageTargetReady;

        if (!g_routePending && !shouldHoldDirectContext)
            return false;

        cursorIndex = 0;
        if (!shouldHoldDirectContext)
        {
            g_routeStatus = TargetNeedsStageHarness(g_target)
                ? "stage route via new game"
                : "loading route via new game";
        }

        if (g_routePolicy == RoutePolicy::DirectContext)
        {
            directContext = true;

            if (!g_titleMenuAcceptInjected)
            {
                g_titleMenuAcceptInjected = true;
                g_routeStatus = "direct context requested";
                LOGFN("SWARD UI Lab route: requested direct title menu context latch for target={}", TargetFor(g_target).token);
                WriteEvidenceEvent("title-menu-direct-context-requested");
            }

            if (g_stageTitleOwnerDirectStateFallbackEnabled &&
                g_stageTitleOwnerDirectStateLastRequestFrame != 0 &&
                !g_titleMenuDirectContextAcceptInjected)
            {
                g_titleMenuDirectContextAcceptInjected = true;
                injectAccept = true;
                LOGFN("SWARD UI Lab route: injected direct title menu accept for target={}", TargetFor(g_target).token);
                WriteEvidenceEvent("title-menu-direct-context-accept-injected");
            }

            return true;
        }

        if (!g_titleMenuAcceptInjected)
        {
            g_titleMenuAcceptInjected = true;
            g_routePending = false;
            injectAccept = true;
            g_routeStatus = TargetNeedsStageHarness(g_target)
                ? "stage accept injected"
                : "loading accept injected";
            LOGFN("SWARD UI Lab route: forced title menu New Game accept for target={}", TargetFor(g_target).token);
            WriteEvidenceEvent("title-menu-new-game-accept-injected");
        }

        return true;
    }

    static const GuestBoolRef* FindGuestBool(std::string_view name)
    {
        const auto requested = ToLower(name);

        for (const auto& guestBool : kGuestRenderGlobals)
        {
            if (ToLower(guestBool.name) == requested)
                return &guestBool;
        }

        for (const auto& guestBool : kGuestDebugDrawGlobals)
        {
            if (ToLower(guestBool.name) == requested)
                return &guestBool;
        }

        return nullptr;
    }

    static std::string BuildBridgeResult(bool ok, std::string_view message)
    {
        std::ostringstream out;
        out
            << "{\"ok\":" << (ok ? "true" : "false")
            << ",\"message\":\"" << JsonEscape(message) << "\"}\n";
        return out.str();
    }

    static std::string BuildRecentEventsJson()
    {
        std::ostringstream out;
        out << "{\"ok\":true,\"events\":";
        AppendRecentEvents(out);
        out << "}\n";
        return out.str();
    }

    static std::string BuildRouteStatusJson()
    {
        const auto& target = TargetFor(g_target);
        std::ostringstream out;

        out
            << "{"
            << "\"ok\":true"
            << ",\"target\":\"" << JsonEscape(target.token) << "\""
            << ",\"targetLabel\":\"" << JsonEscape(target.label) << "\""
            << ",\"route\":\"" << JsonEscape(g_routeStatus) << "\""
            << ",\"routePending\":" << (g_routePending ? "true" : "false")
            << ",\"routePolicy\":\"" << JsonEscape(RoutePolicyLabel()) << "\""
            << ",\"routeGeneration\":" << g_routeGeneration
            << ",\"routeResetCount\":" << g_routeResetCount
            << ",\"frame\":" << g_presentedFrameCount
            << ",\"titleIntroHookObserved\":" << (g_loggedIntroHook ? "true" : "false")
            << ",\"titleMenuHookObserved\":" << (g_loggedMenuHook ? "true" : "false")
            << ",\"titleMenuPressStartAccepted\":" << (g_titleMenuPressStartAccepted ? "true" : "false")
            << ",\"titleMenuPostPressStartHeld\":" << (g_titleMenuPostPressStartHeld ? "true" : "false")
            << ",\"titleMenuVisualReady\":" << (g_titleMenuVisualReady ? "true" : "false")
            << ",\"titleMenuStableFrames\":" << TitleMenuStableFrames()
            << ",\"titleMenuOwnerReady\":" << (g_titleOwnerInspector.ownerReady ? "true" : "false")
            << ",\"titleMenuPostPressStartReady\":" << (g_titleMenuInspector.postPressStartMenuReady ? "true" : "false")
            << ",\"titleMenuContextPhase\":" << g_titleMenuInspector.contextPhase
            << ",\"titleMenuCursor\":" << g_titleMenuInspector.menuCursor
            << ",\"lastTitleIntroContext\":\"" << JsonEscape(g_lastTitleIntroContextDetail) << "\""
            << ",\"lastTitleMenuContext\":\"" << JsonEscape(g_lastTitleMenuContextDetail) << "\""
            << ",\"lastStageTitleContext\":\"" << JsonEscape(g_lastStageTitleContextDetail) << "\""
            << ",\"loadingDisplayActive\":" << (g_loadingDisplayWasActive ? "true" : "false")
            << ",\"loadingDisplayType\":"
            << (g_lastLoadingDisplayType == UINT32_MAX ? -1 : static_cast<int32_t>(g_lastLoadingDisplayType))
            << ",\"stageContextObserved\":" << (g_stageContextObserved ? "true" : "false")
            << ",\"targetCsdObserved\":" << (g_targetCsdObserved ? "true" : "false")
            << ",\"stageReadyEvent\":\"" << JsonEscape(g_lastStageReadyEventName) << "\""
            << "}\n";

        return out.str();
    }

    static std::string BuildHelpJson()
    {
        std::ostringstream out;
        out << "{\"ok\":true,\"commands\":";
        AppendStringArray(out, { "state", "events", "route-status", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route <target>", "reset", "set-global <name> <0|1>", "capture", "help" });
        out << "}\n";
        return out.str();
    }

    static std::string HandleLiveBridgeCommand(std::string_view commandText)
    {
        std::string command = Trim(commandText);
        command.erase(std::remove(command.begin(), command.end(), '\0'), command.end());
        command = Trim(command);

        if (command.empty())
            command = "state";

        {
            std::lock_guard<std::mutex> lock(g_liveBridgeMutex);
            g_lastLiveBridgeCommand = command;
            ++g_liveBridgeCommandCount;
        }

        std::istringstream input(command);
        std::string verb;
        input >> verb;
        verb = ToLower(verb);

        if (verb == "state" || verb == "get-state")
            return BuildLiveStateJson();

        if (verb == "events" || verb == "recent-events")
            return BuildRecentEventsJson();

        if (verb == "route-status" || verb == "status")
            return BuildRouteStatusJson();

        if (verb == "ui-oracle" || verb == "ui-layer-oracle" || verb == "csd-ui-oracle")
            return BuildUiOracleJson();

        if (verb == "ui-draw-list" || verb == "ui-gpu-draw-list" || verb == "runtime-ui-draw-list")
            return BuildRuntimeUiDrawListJson();

        if (verb == "ui-gpu-submit" || verb == "gpu-submit" || verb == "backend-submit")
            return BuildRuntimeGpuSubmitJson();

        if (verb == "ui-material-correlation" || verb == "material-correlation" || verb == "ui-material-map")
            return BuildRuntimeMaterialCorrelationJson();

        if (verb == "ui-backend-resolved" || verb == "backend-resolved" || verb == "ui-backend-submit")
            return BuildRuntimeBackendResolvedJson();

        if (verb == "ui-vendor-command-capture" || verb == "vendor-command-resource-dump" || verb == "ui-vendor-resource-dump")
            return BuildRuntimeVendorCommandResourceDumpJson();

        if (verb == "ui-layer-status" || verb == "ui-render-target-status")
            return BuildRuntimeUiOnlyRenderTargetCaptureJson();

        if (verb == "ui-layer-capture" || verb == "ui-render-target-capture")
        {
            g_uiOnlyRenderTargetCaptureRequested = true;
            g_uiOnlyRenderTargetCaptureReserved = false;
            g_lastUiOnlyRenderTargetCaptureFailure.clear();
            WriteEvidenceEvent(
                "ui-render-target-capture-requested",
                "policy=copy-active-ui-render-target-before-imgui-present");
            return BuildRuntimeUiOnlyRenderTargetCaptureJson();
        }

        if (verb == "help" || verb == "commands")
            return BuildHelpJson();

        if (verb == "route")
        {
            std::string token;
            input >> token;

            if (token.empty())
                return BuildBridgeResult(false, "route command needs a target token");

            if (!TrySetTarget(token))
                return BuildBridgeResult(false, "unknown route target");

            g_routeTargetExplicit = true;
            g_observerMode = false;
            RequestRouteToCurrentTarget();
            WriteEvidenceEvent("live-bridge-command", "route target=" + token);
            return BuildBridgeResult(true, "route requested");
        }

        if (verb == "reset")
        {
            RequestRouteToCurrentTarget();
            WriteEvidenceEvent("live-bridge-command", "reset");
            return BuildBridgeResult(true, "route reset");
        }

        if (verb == "set-global")
        {
            std::string name;
            std::string valueText;
            input >> name >> valueText;

            if (name.empty() || valueText.empty())
                return BuildBridgeResult(false, "set-global <name> <0|1>");

            const auto* guestBool = FindGuestBool(name);
            if (guestBool == nullptr)
                return BuildBridgeResult(false, "unknown SGlobals toggle");

            if (guestBool->readOnly)
                return BuildBridgeResult(false, "SGlobals toggle is read-only");

            const bool value = IsTruthy(valueText) || valueText == "1";
            WriteGuestBool(guestBool->guestAddress, value);
            WriteEvidenceEvent(
                "live-bridge-command",
                "set-global " + std::string(guestBool->name) + "=" + (value ? "1" : "0"));
            return BuildBridgeResult(true, "SGlobals toggle written");
        }

        if (verb == "capture")
        {
            g_nativeFrameCaptureEnabled = true;

            if (g_nativeFrameCaptureDirectory.empty() && !g_evidenceDirectory.empty())
                g_nativeFrameCaptureDirectory = g_evidenceDirectory;

            if (g_nativeFrameCaptureMaxCount <= g_nativeFrameCaptureWrittenCount)
                g_nativeFrameCaptureMaxCount = g_nativeFrameCaptureWrittenCount + 1;

            g_lastNativeFrameCaptureFrame = 0;
            WriteEvidenceEvent("live-bridge-capture-requested", "next ready native frame");
            return BuildBridgeResult(true, "capture requested");
        }

        return BuildBridgeResult(false, "unknown command");
    }

    static constexpr std::string_view kDefaultLiveBridgePipePath = "\\\\.\\pipe\\sward_ui_lab_live";

    static std::string LiveBridgePipePath()
    {
        if (g_liveBridgeName.empty() || g_liveBridgeName == "sward_ui_lab_live")
            return std::string(kDefaultLiveBridgePipePath);

        return std::string("\\\\.\\pipe\\") + g_liveBridgeName;
    }

    static void StartLiveBridge()
    {
        if (!g_isEnabled || !g_liveBridgeEnabled)
            return;

        bool expected = false;
        if (!g_liveBridgeStarted.compare_exchange_strong(expected, true))
            return;

        std::thread(UiLabLiveBridgeThread).detach();
        LOGFN("SWARD UI Lab live bridge started: {}", LiveBridgePipePath());
        WriteEvidenceEvent("live-bridge-started", LiveBridgePipePath());
    }

    static void UiLabLiveBridgeThread()
    {
#ifdef _WIN32
        while (!g_liveBridgeStopRequested.load())
        {
            const auto pipePath = LiveBridgePipePath();
            HANDLE pipe = CreateNamedPipeA(
                pipePath.c_str(),
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1,
                65536,
                65536,
                250,
                nullptr);

            if (pipe == INVALID_HANDLE_VALUE)
            {
                Sleep(500);
                continue;
            }

            const BOOL connected = ConnectNamedPipe(pipe, nullptr)
                ? TRUE
                : (GetLastError() == ERROR_PIPE_CONNECTED);

            if (connected)
            {
                char buffer[4096];
                DWORD bytesRead = 0;

                while (ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    const auto response = HandleLiveBridgeCommand(buffer);
                    DWORD bytesWritten = 0;
                    WriteFile(
                        pipe,
                        response.data(),
                        static_cast<DWORD>(response.size()),
                        &bytesWritten,
                        nullptr);
                }
            }

            FlushFileBuffers(pipe);
            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
        }
#else
        while (!g_liveBridgeStopRequested.load())
            std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
    }

    static void DrawPredicate(const char* label, bool value)
    {
        const ImVec4 color = value
            ? ImVec4(0.35f, 0.92f, 0.42f, 1.0f)
            : ImVec4(1.0f, 0.78f, 0.28f, 1.0f);

        ImGui::TextColored(color, "%s: %s", label, value ? "yes" : "waiting");
    }

    static void DrawHexField(const char* label, uint32_t value)
    {
        ImGui::Text("%s: %s", label, HexU32(value).c_str());
    }

    static void DrawSwardReddogStyleSectionHeader(const char* label)
    {
        // Phase 204: local-only Reddog style reference, implemented as repo-safe ImGui colors/spacing only.
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.93f, 1.0f, 1.0f));
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    static void DrawSwardReddogStatusPip(const char* label, bool active)
    {
        const ImVec4 color = active
            ? ImVec4(0.28f, 0.72f, 1.0f, 1.0f)
            : ImVec4(0.48f, 0.52f, 0.54f, 1.0f);

        ImGui::TextColored(color, "%s", active ? "[x]" : "[ ]");
        ImGui::SameLine();
        ImGui::TextUnformatted(label);
    }

    static bool ReadGuestBool(uint32_t guestAddress)
    {
        if (g_memory.base == nullptr || guestAddress == 0)
            return false;

        return *reinterpret_cast<const bool*>(g_memory.Translate(guestAddress));
    }

    static void WriteGuestBool(uint32_t guestAddress, bool value)
    {
        if (g_memory.base == nullptr || guestAddress == 0)
            return;

        *reinterpret_cast<bool*>(g_memory.Translate(guestAddress)) = value;
    }

    static void DrawGuestBoolCheckbox(const GuestBoolRef& guestBool)
    {
        bool value = ReadGuestBool(guestBool.guestAddress);
        const auto label = std::string(guestBool.name) + " " + HexU32(guestBool.guestAddress);

        if (guestBool.readOnly)
        {
            ImGui::Text("%s: %s", label.c_str(), value ? "true" : "false");
            return;
        }

        if (ImGui::Checkbox(label.c_str(), &value))
            WriteGuestBool(guestBool.guestAddress, value);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("direct guest-memory debug globals");
    }

    static void DrawRuntimeInspectorOverview()
    {
        const auto& target = TargetFor(g_target);
        const std::string targetLabel(target.label);
        const std::string targetToken(target.token);

        ImGui::TextUnformatted("Runtime-backed UI Lab inspector");
        ImGui::Separator();

        if (ImGui::BeginCombo("Target", targetLabel.c_str()))
        {
            for (size_t i = 0; i < kRuntimeTargets.size(); ++i)
            {
                const auto& runtimeTarget = kRuntimeTargets[i];
                const bool selected = runtimeTarget.id == g_target;
                const std::string selectableLabel =
                    std::string(runtimeTarget.label) + " [" + std::string(runtimeTarget.token) + "]";

                if (ImGui::Selectable(selectableLabel.c_str(), selected))
                    SelectTargetIndex(i);

                if (selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();

        if (ImGui::Button("Route"))
            RequestRouteToCurrentTarget();

        ImGui::SameLine();

        if (ImGui::Button("Mark Evidence"))
            WriteEvidenceEvent("manual-evidence-marker");

        ImGui::Text("Mode: %s", g_observerMode ? "observer mode" : "route forcing");
        ImGui::Text("Target token: %s", targetToken.c_str());
        ImGui::Text("CSD scene: %s", std::string(target.primaryCsdScene).c_str());
        ImGui::Text("Source family: %s", std::string(target.sourceFamily).c_str());
        ImGui::Text("Stage harness: %s", std::string(GetStageHarnessLabel()).c_str());
        ImGui::Text("Target CSD: %s", std::string(GetTargetCsdStatusLabel()).c_str());
        ImGui::Text("Route: %s", std::string(GetRouteStatusLabel()).c_str());
        ImGui::Text("Route policy: %s", std::string(RoutePolicyLabel()).c_str());
        ImGui::Text("Presented frames: %llu", static_cast<unsigned long long>(g_presentedFrameCount));
        ImGui::Text("Startup prompt blockers: %s", ShouldBypassStartupPromptBlockers() ? "bypassed" : "normal");
        ImGui::Separator();
        ImGui::Text("Last CSD project: %s", g_lastCsdProjectName.empty() ? "none" : g_lastCsdProjectName.c_str());
        ImGui::Text("Last CSD frame: %llu", static_cast<unsigned long long>(g_lastCsdProjectFrame));
        ImGui::Text(
            "Loading request: %s",
            g_lastLoadingRequestType == UINT32_MAX ? "none" : std::to_string(g_lastLoadingRequestType).c_str());
        ImGui::Text("Loading request frame: %llu", static_cast<unsigned long long>(g_lastLoadingRequestFrame));
        ImGui::Text(
            "Loading display: %s",
            g_lastLoadingDisplayType == UINT32_MAX ? "none" : std::to_string(g_lastLoadingDisplayType).c_str());
        ImGui::Text("Loading display frame: %llu", static_cast<unsigned long long>(g_lastLoadingDisplayFrame));
        ImGui::Separator();
        DrawPredicate("Title intro hook", g_loggedIntroHook);
        DrawPredicate("Title menu hook", g_loggedMenuHook);
        DrawPredicate("Stage context", !target.requiresStageContext || g_stageContextObserved);
    }

    static void DrawTitleMenuLatchInspector()
    {
        ImGui::TextUnformatted("Title menu latch predicates");
        ImGui::Separator();
        DrawPredicate("Intro Press Start accepted", g_titleMenuPressStartAccepted);
        DrawPredicate("Menu post-Press-Start held", g_titleMenuPostPressStartHeld);
        DrawPredicate("Owner/CSD ready", g_titleMenuPostPressStartReadyLogged);
        DrawPredicate("Menu context ready", g_titleMenuInspector.postPressStartMenuReady);
        DrawPredicate("Visual ready", g_titleMenuVisualReady);
        DrawPredicate("Native capture ready", IsNativeFrameCaptureReady());
        ImGui::Text("Stable frames: %llu", static_cast<unsigned long long>(TitleMenuStableFrames()));

        if (ImGui::CollapsingHeader("Title intro context", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (!g_titleIntroInspector.valid)
            {
                ImGui::TextUnformatted("waiting");
            }
            else
            {
                DrawHexField("context", g_titleIntroInspector.contextAddress);
                DrawHexField("state machine", g_titleIntroInspector.stateMachineAddress);
                ImGui::Text("elapsed: %.3f", g_titleIntroInspector.elapsedSeconds);
                ImGui::Text("requested_state: %u", static_cast<unsigned>(g_titleIntroInspector.requestedState));
                ImGui::Text("dirty: %u", static_cast<unsigned>(g_titleIntroInspector.dirtyFlag));
                ImGui::Text("transition_armed: %u", static_cast<unsigned>(g_titleIntroInspector.transitionArmed));
                ImGui::Text("context_flag580: %u", static_cast<unsigned>(g_titleIntroInspector.contextFlag580));
                DrawHexField("context_472", g_titleIntroInspector.context472);
                DrawHexField("context_480", g_titleIntroInspector.context480);
                DrawHexField("context_488", g_titleIntroInspector.context488);
                ImGui::Text("frame: %llu", static_cast<unsigned long long>(g_titleIntroInspector.frame));
            }
        }

        if (ImGui::CollapsingHeader("Title owner / CSD", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (!g_titleOwnerInspector.valid)
            {
                ImGui::TextUnformatted("waiting");
            }
            else
            {
                DrawPredicate("owner ready", g_titleOwnerInspector.ownerReady);
                ImGui::Text("is_title_state_menu: %u", g_titleOwnerInspector.isTitleStateMenu ? 1u : 0u);
                DrawHexField("owner_title_context", g_titleOwnerInspector.titleContextAddress);
                DrawHexField("title_csd488", g_titleOwnerInspector.titleCsdAddress);
                ImGui::Text("owner_gate568: %u", g_titleOwnerInspector.ownerGate568 ? 1u : 0u);
                ImGui::Text("owner_gate570: %u", g_titleOwnerInspector.ownerGate570 ? 1u : 0u);
                ImGui::Text("title_request: %u", static_cast<unsigned>(g_titleOwnerInspector.titleRequest));
                ImGui::Text("title_dirty: %u", static_cast<unsigned>(g_titleOwnerInspector.titleDirty));
                ImGui::Text("title_transition: %u", static_cast<unsigned>(g_titleOwnerInspector.titleTransition));
                ImGui::Text("title_flag580: %u", static_cast<unsigned>(g_titleOwnerInspector.titleFlag580));
                ImGui::Text("csd bytes +62/+84/+152/+160: %u / %u / %u / %u",
                    static_cast<unsigned>(g_titleOwnerInspector.csdByte62),
                    static_cast<unsigned>(g_titleOwnerInspector.csdByte84),
                    static_cast<unsigned>(g_titleOwnerInspector.csdByte152),
                    static_cast<unsigned>(g_titleOwnerInspector.csdByte160));
                ImGui::Text("frame: %llu", static_cast<unsigned long long>(g_titleOwnerInspector.frame));
            }
        }

        if (ImGui::CollapsingHeader("CTitleStateMenu context", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (!g_titleMenuInspector.valid)
            {
                ImGui::TextUnformatted("waiting");
            }
            else
            {
                DrawPredicate("post-Press-Start menu ready", g_titleMenuInspector.postPressStartMenuReady);
                DrawHexField("context_472", g_titleMenuInspector.context472);
                DrawHexField("context_480", g_titleMenuInspector.context480);
                DrawHexField("context_488", g_titleMenuInspector.context488);
                ImGui::Text("context_phase: %u", g_titleMenuInspector.contextPhase);
                ImGui::Text("context_flag580: %u", static_cast<unsigned>(g_titleMenuInspector.contextFlag580));
                ImGui::Text("menu_cursor: %u", g_titleMenuInspector.menuCursor);
                ImGui::Text("menu_field3c: %u", g_titleMenuInspector.menuField3C ? 1u : 0u);
                ImGui::Text("menu_field54: %u", g_titleMenuInspector.menuField54 ? 1u : 0u);
                ImGui::Text("menu_field9a: %u", g_titleMenuInspector.menuField9A ? 1u : 0u);
                ImGui::Text("stable_frames: %llu", static_cast<unsigned long long>(g_titleMenuInspector.stableFrames));
                ImGui::Text("frame: %llu", static_cast<unsigned long long>(g_titleMenuInspector.frame));
            }
        }
    }

    static void DrawCaptureInspector()
    {
        ImGui::TextUnformatted("Capture and evidence");
        ImGui::Separator();
        ImGui::Text("Evidence: %s", g_evidenceDirectory.empty() ? "off" : g_evidenceDirectory.string().c_str());
        ImGui::Text(
            "Native capture: %s",
            std::string(NativeFrameCaptureStatusLabel()).c_str());
        ImGui::Text(
            "Native captures: %u/%u every %u frames",
            g_nativeFrameCaptureWrittenCount,
            g_nativeFrameCaptureMaxCount,
            g_nativeFrameCaptureIntervalFrames);
        ImGui::Text("Last native frame: %llu", static_cast<unsigned long long>(g_lastNativeFrameCaptureFrame));
        ImGui::TextWrapped(
            "Last native path: %s",
            g_lastNativeFrameCapturePath.empty() ? "none" : g_lastNativeFrameCapturePath.c_str());
        ImGui::TextWrapped(
            "Last native failure: %s",
            g_lastNativeFrameCaptureFailure.empty() ? "none" : g_lastNativeFrameCaptureFailure.c_str());
        ImGui::Text("Auto exit: %.2f", g_autoExitSeconds);
        ImGui::Text("Auto exit requested: %s", g_autoExitRequested ? "yes" : "no");
        ImGui::Separator();

        if (ImGui::Button("Write Manual Evidence Marker"))
            WriteEvidenceEvent("manual-evidence-marker");
    }

    static void DrawTargetRouterInspector()
    {
        ImGui::TextUnformatted("Real screen routes");
        ImGui::Separator();

        if (ImGui::Button("Previous Target"))
            SelectPreviousTarget();

        ImGui::SameLine();

        if (ImGui::Button("Next Target"))
            SelectNextTarget();

        ImGui::SameLine();

        if (ImGui::Button("Route Selected Target"))
            RequestRouteToCurrentTarget();

        for (size_t i = 0; i < kRuntimeTargets.size(); ++i)
        {
            const auto& runtimeTarget = kRuntimeTargets[i];
            const std::string label(runtimeTarget.label);
            const std::string token(runtimeTarget.token);
            const std::string buttonLabel = (runtimeTarget.id == g_target ? "> " : "  ") + label + "###ui-lab-target-" + std::to_string(i);

            if (ImGui::Button(buttonLabel.c_str()))
                SelectTargetIndex(i);

            ImGui::SameLine();
            ImGui::Text("(%s, %s)", token.c_str(), std::string(runtimeTarget.primaryCsdScene).c_str());
        }
    }

    static bool PushSwardNativeProfilerFont()
    {
        ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
        if (font == nullptr)
            return false;

        g_swardNativeProfilerFont = font;
        g_swardNativeProfilerFontDefaultScale = font->Scale;
        font->Scale = ImGui::GetDefaultFont()->FontSize / font->FontSize;
        ImGui::PushFont(font);
        g_swardNativeProfilerFontPushed = true;
        return true;
    }

    static void PopSwardNativeProfilerFont()
    {
        if (!g_swardNativeProfilerFontPushed)
            return;

        ImGui::PopFont();
        if (g_swardNativeProfilerFont != nullptr)
            g_swardNativeProfilerFont->Scale = g_swardNativeProfilerFontDefaultScale;

        g_swardNativeProfilerFont = nullptr;
        g_swardNativeProfilerFontDefaultScale = 1.0f;
        g_swardNativeProfilerFontPushed = false;
    }

    static void UpdateOperatorProfilerFrameHistory()
    {
        if (g_operatorProfilerLastFrame == g_presentedFrameCount)
            return;

        g_operatorProfilerLastFrame = g_presentedFrameCount;
        g_operatorProfilerFrameMs[g_operatorProfilerFrameMsIndex] =
            static_cast<float>(App::s_deltaTime * 1000.0);
        g_operatorProfilerFrameMsIndex =
            (g_operatorProfilerFrameMsIndex + 1) % kOperatorProfilerFrameHistoryCount;
    }

    static void DrawSwardNativeProfilerFrameTimePlot()
    {
        UpdateOperatorProfilerFrameHistory();

        // Phase 179 used ImGui::PlotLines("Frame Time"); Phase 187 uses the native Profiler ImPlot path.
        if (ImPlot::BeginPlot("Frame Time"))
        {
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 20.0);
            ImPlot::SetupAxis(ImAxis_Y1, "ms", ImPlotAxisFlags_None);
            ImPlot::PlotLine<float>("Application",
                g_operatorProfilerFrameMs,
                kOperatorProfilerFrameHistoryCount,
                1.0,
                0.0,
                ImPlotLineFlags_None,
                g_operatorProfilerFrameMsIndex);
            ImPlot::EndPlot();
        }
    }

    static void DrawOperatorProfilerSummary()
    {
        UpdateOperatorProfilerFrameHistory();

        const double frameMs = App::s_deltaTime * 1000.0;
        const double fps = App::s_deltaTime > 0.0 ? 1.0 / App::s_deltaTime : 0.0;
        float averageFrameMs = 0.0f;
        int averageFrameCount = 0;

        for (const float value : g_operatorProfilerFrameMs)
        {
            if (value <= 0.0f)
                continue;

            averageFrameMs += value;
            ++averageFrameCount;
        }

        if (averageFrameCount > 0)
            averageFrameMs /= static_cast<float>(averageFrameCount);

        DrawSwardNativeProfilerFrameTimePlot();
        ImGui::Text("Current Application: %.3f ms (%.2f FPS)", frameMs, fps);
        ImGui::Text(
            "Average Application: %.3f ms (%.2f FPS)",
            averageFrameMs,
            averageFrameMs > 0.0f ? 1000.0f / averageFrameMs : 0.0f);
        ImGui::Text("Frame: %llu", static_cast<unsigned long long>(g_presentedFrameCount));
        ImGui::Separator();
        ImGui::Text("Target: %s", std::string(GetTargetToken()).c_str());
        ImGui::Text("Route: %s", std::string(GetRouteStatusLabel()).c_str());
        ImGui::Text("Native: %s", std::string(NativeFrameCaptureStatusLabel()).c_str());
        ImGui::Text("UI layer: %s", std::string(UiOnlyLayerIsolationStatusLabel()).c_str());
        ImGui::Text("Live bridge: %s", IsLiveBridgeEnabled() ? "enabled" : "off");
        DrawPredicate("Stage/HUD ready", !g_lastStageReadyEventName.empty());
    }

    static void DrawOperatorProfilerHudTab()
    {
        const auto sonicHud = BuildSonicHudLiveInspectorSnapshot();
        const auto sonicGameplay = BuildSonicHudGameplayValueSnapshot();
        const auto lastClassified = BuildSonicHudLastClassifiedCallsiteValue();

        ImGui::TextUnformatted("Sonic Day HUD");
        ImGui::Separator();
        DrawPredicate("Stage context", sonicHud.stageContextObserved);
        DrawPredicate("Target CSD", sonicHud.targetCsdObserved);
        DrawPredicate("Stage target ready", sonicHud.stageTargetReady);
        DrawPredicate("Raw owner", sonicHud.rawOwnerKnown);
        DrawPredicate("Owner fields", sonicHud.rawOwnerFieldsReady);
        DrawHexField("CHudSonicStage", sonicHud.hudOwnerAddress);
        DrawHexField("Stage game mode", sonicHud.stageGameModeAddress);
        ImGui::Text("Play screen: %s", sonicHud.playScreenProject.empty() ? "waiting" : sonicHud.playScreenProject.c_str());
        ImGui::Text("Speed gauge: %s", sonicHud.speedGaugeScene.empty() ? "waiting" : sonicHud.speedGaugeScene.c_str());
        ImGui::Text("Ready event: %s", sonicHud.readyEvent.empty() ? "waiting" : sonicHud.readyEvent.c_str());
        ImGui::Separator();
        ImGui::Text("Binding: %s", SonicHudValueWriteBindingStatus().c_str());
        ImGui::Text(
            "rings: %s %u",
            sonicGameplay.ringCountKnown ? "known" : "waiting",
            sonicGameplay.ringCount);
        ImGui::Text(
            "score: %s %u",
            sonicGameplay.scoreKnown ? "known" : "waiting",
            sonicGameplay.score);
        ImGui::Text(
            "timer frames: %s %u",
            sonicGameplay.elapsedFramesKnown ? "known" : "waiting",
            sonicGameplay.elapsedFrames);
        ImGui::Text(
            "speed: %s %.1f km/h",
            sonicGameplay.speedKmhKnown ? "known" : "waiting",
            sonicGameplay.speedKmh);
        ImGui::Text(
            "boost: %s %.3f",
            sonicGameplay.boostGaugeKnown ? "known" : "waiting",
            sonicGameplay.boostGauge);
        ImGui::Text(
            "ring energy: %s %.3f",
            sonicGameplay.ringEnergyGaugeKnown ? "known" : "waiting",
            sonicGameplay.ringEnergyGauge);
        ImGui::Text(
            "lives: %s %u",
            sonicGameplay.lifeCountKnown ? "known" : "waiting",
            sonicGameplay.lifeCount);
        ImGui::Text(
            "tutorial: %s %s",
            sonicGameplay.tutorialPromptKnown ? "known" : "waiting",
            sonicGameplay.tutorialPromptId.c_str());

        if (lastClassified.lastClassificationKnown)
        {
            ImGui::Separator();
            ImGui::Text("Last classified: %s", lastClassified.valueName.c_str());
            ImGui::Text("Status: %s", lastClassified.status.c_str());
            ImGui::Text("Hook: %s / %s", lastClassified.hookName.c_str(), lastClassified.samplePhase.c_str());
            ImGui::Text("Frame: %llu", static_cast<unsigned long long>(lastClassified.lastClassifiedCallsiteValueFrame));
        }
    }

    static void DrawOperatorHudSwitchesPanel()
    {
        ImGui::TextUnformatted("SGlobals HUD/render switches");
        ImGui::TextWrapped(
            "ms_IsRenderHud is the whole UI render gate; ms_IsRenderGameMainHud and ms_IsRenderHudPause split the in-game HUD and pause HUD lanes for visual isolation.");
        ImGui::Separator();

        for (const auto& guestBool : kGuestRenderGlobals)
        {
            const std::string_view name = guestBool.name;
            if (
                name == "ms_IsRenderHud" ||
                name == "ms_IsRenderGameMainHud" ||
                name == "ms_IsRenderHudPause")
            {
                DrawGuestBoolCheckbox(guestBool);
            }
        }

        ImGui::Separator();
        const auto gateCorrelation = BuildHudRenderGateCorrelationSnapshot();
        ImGui::TextWrapped("HUD render gate correlation: %s", gateCorrelation.gateStatus.c_str());
        ImGui::Text(
            "ui_playscreen writes: resolved=%llu unresolved=%llu",
            static_cast<unsigned long long>(gateCorrelation.resolvedUiPlayScreenNodeWrites),
            static_cast<unsigned long long>(gateCorrelation.unresolvedUiPlayScreenNodeWrites));

        if (!gateCorrelation.unresolvedWriteKinds.empty())
            ImGui::TextWrapped("unresolved kinds: %s", gateCorrelation.unresolvedWriteKinds.c_str());

        ImGui::TextWrapped(
            "ms_IsRenderHud callers: frontend_listener.cpp, options_menu.cpp::SetOptionsMenuVisible, CHudPause_patches.cpp");
        ImGui::Separator();
        ImGui::TextWrapped(
            "These switches identify the global UI render gate and child HUD render lanes. They are an isolation oracle, not a substitute for the typed owner/CSD/callsite source recovery.");
    }

    static void DrawOperatorProfilerPanelsTab()
    {
        ImGui::TextUnformatted("Open focused drill-down windows");
        ImGui::Separator();

        for (const auto& entry : GetOperatorWindowEntries())
        {
            ImGui::Checkbox(entry.name, entry.visible);

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", entry.description);
        }

        ImGui::Separator();
        ImGui::Checkbox("Operator foreground layer", &g_operatorDebugDrawLayerVisible);

        if (ImGui::Button("Write Live State Snapshot"))
            WriteLiveStateSnapshot();

        ImGui::SameLine();

        if (ImGui::Button("Manual Evidence"))
            WriteEvidenceEvent("manual-evidence-marker");
    }

    static void DrawOperatorNextSourceRecoveryLane()
    {
        DrawSwardReddogStyleSectionHeader("Next source-recovery lane");
        ImGui::TextWrapped(
            "Phase 205 uses the coverage matrix to choose the next implementation lane instead of treating every prototype route as equally urgent.");

        for (const auto& row : kSourceRecoveryLaneRows)
        {
            DrawSwardReddogStatusPip(row.laneId.data(), row.selected);
            ImGui::SameLine();
            ImGui::TextUnformatted(row.selected ? "selected" : "queued");
            ImGui::Text("screen: %s", row.screenId.data());
            ImGui::Text("controller: %s", row.controller.data());
            ImGui::TextWrapped("oracle: %s", row.primaryOracle.data());
            ImGui::TextWrapped("status: %s", row.decisionStatus.data());
            ImGui::TextWrapped("next: %s", row.nextEvidenceBeat.data());
            ImGui::Separator();
        }

        ImGui::TextWrapped(
            "Selected reason: Sonic Day HUD is closest to reusable native HUD source, but boost/ring-energy normalization and exact SFX/audio IDs still block 1:1 behavior.");
    }

    static void DrawOperatorCoverageMatrixTab()
    {
        DrawOperatorNextSourceRecoveryLane();
        ImGui::Separator();
        DrawSwardReddogStyleSectionHeader("Starter UI/UX coverage matrix");
        ImGui::TextWrapped(
            "Prototype #SelectStage route taxonomy is a secondary oracle. Retail runtime evidence remains primary for 1:1 source recovery.");
        ImGui::TextDisabled("Reddog/profiler visual language is used as local-only style guidance; no extracted DDS assets are loaded or committed.");
        ImGui::Separator();

        for (const auto& row : kStarterUiCoverageRows)
        {
            const bool runtimeReady =
                row.screenId == "title-menu" ||
                row.screenId == "loading" ||
                row.screenId == "options-settings" ||
                row.screenId == "pause" ||
                row.screenId == "sonic-day-hud";

            DrawSwardReddogStatusPip(row.screenId.data(), runtimeReady);
            ImGui::Text("controller: %s", row.controller.data());
            ImGui::TextWrapped("retail oracle: %s", row.runtimeOracle.data());
            ImGui::TextWrapped("prototype route: %s", row.prototypeRouteStatus.data());
            ImGui::TextWrapped("next: %s", row.nextEvidenceBeat.data());
            ImGui::Separator();
        }
    }

    static void DrawProfilerAddonContent()
    {
        if (!ImGui::CollapsingHeader("SWARD UI Lab", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::TextUnformatted("F2 toggles detached SWARD UI Lab; F2 toggles SWARD UI Lab");
        ImGui::TextUnformatted("F1 remains native Profiler");
        ImGui::Text("Target: %s", std::string(GetTargetToken()).c_str());
        ImGui::Text("Route: %s", std::string(GetRouteStatusLabel()).c_str());
        ImGui::Text("Live bridge: %s", IsLiveBridgeEnabled() ? "enabled" : "off");
        ImGui::Text("UI layer: %s", std::string(UiOnlyLayerIsolationStatusLabel()).c_str());

        const auto observations = BuildSonicHudValueWriteObservations();
        uint64_t resolvedObservations = 0;
        uint64_t unresolvedObservations = 0;
        for (const auto& observation : observations)
        {
            if (observation.pathResolved)
                ++resolvedObservations;
            else
                ++unresolvedObservations;
        }

        ImGui::Text(
            "HUD node writes: resolved=%llu unresolved=%llu",
            static_cast<unsigned long long>(resolvedObservations),
            static_cast<unsigned long long>(unresolvedObservations));
        ImGui::TextWrapped("Sonic HUD binding: %s", SonicHudValueWriteBindingStatus().c_str());

        if (ImGui::BeginTabBar("sward-profiler-addon-tabs"))
        {
            if (ImGui::BeginTabItem("Overview"))
            {
                DrawRuntimeInspectorOverview();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("HUD"))
            {
                DrawOperatorProfilerHudTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("HUD Switches"))
            {
                DrawOperatorHudSwitchesPanel();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Capture"))
            {
                DrawCaptureInspector();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Coverage"))
            {
                DrawOperatorCoverageMatrixTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Panels"))
            {
                ImGui::TextUnformatted("Legacy floating panes");
                ImGui::Separator();
                DrawOperatorProfilerPanelsTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    static void DrawDetachedProfilerAddonTab()
    {
        ImGui::PushID("sward-detached-profiler-addon");
        DrawProfilerAddonContent();
        ImGui::PopID();
    }

    void DrawProfilerAddon()
    {
        if (!g_isEnabled)
            return;

        ImGui::Separator();
        DrawProfilerAddonContent();
    }

    static void DrawOperatorProfilerPanel()
    {
        if (!g_operatorShellVisible)
            return;

        ImGui::SetNextWindowPos(ImVec2(96.0f, 118.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(430.0f, 700.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.86f);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing;

        const bool nativeFontPushed = PushSwardNativeProfilerFont();

        if (ImGui::Begin("SWARD UI Lab###SWARD Operator Profiler", nullptr, flags))
        {
            DrawOperatorProfilerSummary();
            ImGui::TextDisabled("F2 toggles detached SWARD UI Lab; F2 toggles SWARD UI Lab");
            ImGui::TextDisabled("F1 remains native Profiler");
            ImGui::Separator();

            if (ImGui::BeginTabBar("sward-operator-profiler-tabs"))
            {
                if (ImGui::BeginTabItem("SWARD UI Lab"))
                {
                    DrawDetachedProfilerAddonTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Overview"))
                {
                    DrawRuntimeInspectorOverview();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Runtime"))
                {
                    DrawRuntimeInspectorOverview();
                    ImGui::Separator();
                    DrawTargetRouterInspector();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Title/Menu"))
                {
                    DrawTitleMenuLatchInspector();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("HUD"))
                {
                    DrawOperatorProfilerHudTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("HUD Switches"))
                {
                    DrawOperatorHudSwitchesPanel();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Capture"))
                {
                    DrawCaptureInspector();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Coverage"))
                {
                    DrawOperatorCoverageMatrixTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Panels"))
                {
                    DrawOperatorProfilerPanelsTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Targets"))
                {
                    DrawTargetRouterInspector();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }

        ImGui::End();

        if (nativeFontPushed)
            PopSwardNativeProfilerFont();
    }

    static void DrawOperatorDebugIcon()
    {
        ImGui::SetNextWindowPos(g_operatorDebugIconPos, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.78f);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing;

        if (ImGui::Begin("SWARD Operator Debug Icon", nullptr, flags))
        {
            g_operatorDebugIconPos = ImGui::GetWindowPos();

            if (ImGui::Button("UI", ImVec2(34.0f, 34.0f)))
                g_operatorWindowListVisible = !g_operatorWindowListVisible;

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("SWARD operator windows");
        }

        ImGui::End();
    }

    static void DrawOperatorWindowList()
    {
        if (!g_operatorWindowListVisible)
            return;

        ImGui::SetNextWindowPos(
            ImVec2(g_operatorDebugIconPos.x, g_operatorDebugIconPos.y + 48.0f),
            ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.88f);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("SWARD Operator Window List", &g_operatorWindowListVisible, flags))
        {
            for (const auto& entry : GetOperatorWindowEntries())
            {
                ImGui::Checkbox(entry.name, entry.visible);

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", entry.description);
            }
        }

        ImGui::End();
    }

    static void DrawOperatorWelcomeWindow()
    {
        if (!g_operatorWelcomeVisible)
            return;

        if (ImGui::Begin("SWARD Welcome", &g_operatorWelcomeVisible))
        {
            ImGui::TextUnformatted("Compact-on-demand operator windows");
            ImGui::Separator();
            ImGui::Text("Target: %s", std::string(GetTargetToken()).c_str());
            ImGui::Text("Route: %s", std::string(GetRouteStatusLabel()).c_str());
            ImGui::Text("Evidence: %s", g_evidenceDirectory.empty() ? "off" : g_evidenceDirectory.string().c_str());
            ImGui::Text("Native: %s", std::string(NativeFrameCaptureStatusLabel()).c_str());
            ImGui::Text("Stage/HUD ready: %s", g_lastStageReadyEventName.empty() ? "waiting" : g_lastStageReadyEventName.c_str());
        }

        ImGui::End();
    }

    static void DrawOperatorCounterWindow()
    {
        if (!g_operatorCounterVisible)
            return;

        if (ImGui::Begin("SWARD Counter", &g_operatorCounterVisible))
        {
            const double frameMs = App::s_deltaTime * 1000.0;
            const double fps = App::s_deltaTime > 0.0 ? 1.0 / App::s_deltaTime : 0.0;

            ImGui::Text("Frame: %llu", static_cast<unsigned long long>(g_presentedFrameCount));
            ImGui::Text("Delta: %.3f ms", frameMs);
            ImGui::Text("FPS: %.1f", fps);
            ImGui::Separator();
            ImGui::Text("Title intro samples: %llu", static_cast<unsigned long long>(g_titleIntroContextSampleCount));
            ImGui::Text("Stage title samples: %llu", static_cast<unsigned long long>(g_stageTitleContextSampleCount));
            ImGui::Text("Native captures: %u/%u", g_nativeFrameCaptureWrittenCount, g_nativeFrameCaptureMaxCount);
            ImGui::Text("Native status: %s", std::string(NativeFrameCaptureStatusLabel()).c_str());
            ImGui::Text("Route: %s", std::string(GetRouteStatusLabel()).c_str());
        }

        ImGui::End();
    }

    static void DrawOperatorViewWindow()
    {
        if (!g_operatorViewVisible)
            return;

        if (ImGui::Begin("SWARD View", &g_operatorViewVisible))
        {
            ImGui::Checkbox("Render FPS", &Config::ShowFPS.Value);
            ImGui::Separator();
            ImGui::TextUnformatted("SGlobals HUD/render switches");
            for (const auto& guestBool : kGuestRenderGlobals)
                DrawGuestBoolCheckbox(guestBool);

            ImGui::Separator();
            ImGui::Checkbox("Event collision debug view", &Config::EnableEventCollisionDebugView.Value);
            ImGui::Checkbox("Object collision debug view", &Config::EnableObjectCollisionDebugView.Value);
            ImGui::Checkbox("Stage collision debug view", &Config::EnableStageCollisionDebugView.Value);
            ImGui::Checkbox("GI mip-level debug view", &Config::EnableGIMipLevelDebugView.Value);
            ImGui::Separator();
            ImGui::TextUnformatted("Some render debug switches are sampled by game patches and may need a stage restart.");
        }

        ImGui::End();
    }

    static void DrawOperatorExportsWindow()
    {
        if (!g_operatorExportsVisible)
            return;

        if (ImGui::Begin("SWARD Exports", &g_operatorExportsVisible))
        {
            ImGui::TextUnformatted("Runtime patch/config switches for UI archaeology sessions.");
            ImGui::Separator();
            ImGui::Checkbox("Allow Cancelling Unleash", &Config::AllowCancellingUnleash.Value);
            ImGui::Checkbox("Disable Auto Save Warning", &Config::DisableAutoSaveWarning.Value);
            ImGui::Checkbox("Disable DLC Icon", &Config::DisableDLCIcon.Value);
            ImGui::Checkbox("Fix Unleash Out Of Control Drain", &Config::FixUnleashOutOfControlDrain.Value);
            ImGui::Checkbox("Homing Attack On Jump", &Config::HomingAttackOnJump.Value);
            ImGui::Checkbox("Save Score At Checkpoints", &Config::SaveScoreAtCheckpoints.Value);
            ImGui::Checkbox("Skip Intro Logos", &Config::SkipIntroLogos.Value);
            ImGui::Checkbox("Use Alternate Title", &Config::UseAlternateTitle.Value);
        }

        ImGui::End();
    }

    static void DrawOperatorDebugDrawWindow()
    {
        if (!g_operatorDebugDrawVisible)
            return;

        if (ImGui::Begin("SWARD Debug Draw", &g_operatorDebugDrawVisible))
        {
            ImGui::Checkbox("Operator foreground layer", &g_operatorDebugDrawLayerVisible);
            ImGui::Separator();
            ImGui::TextUnformatted("SGlobals debug-draw switches");
            for (const auto& guestBool : kGuestDebugDrawGlobals)
                DrawGuestBoolCheckbox(guestBool);

            ImGui::Separator();
            ImGui::Checkbox("Event collision debug view", &Config::EnableEventCollisionDebugView.Value);
            ImGui::Checkbox("Object collision debug view", &Config::EnableObjectCollisionDebugView.Value);
            ImGui::Checkbox("Stage collision debug view", &Config::EnableStageCollisionDebugView.Value);
            ImGui::Checkbox("GI mip-level debug view", &Config::EnableGIMipLevelDebugView.Value);
            ImGui::Separator();
            ImGui::Text("Target: %s", std::string(GetTargetToken()).c_str());
            ImGui::Text("Native ready: %s", IsNativeFrameCaptureReady() ? "yes" : "waiting");
            ImGui::Text("Title menu stable frames: %llu", static_cast<unsigned long long>(TitleMenuStableFrames()));
        }

        ImGui::End();
    }

    static void DrawOperatorStageHudWindow()
    {
        if (!g_operatorStageHudVisible)
            return;

        if (ImGui::Begin("SWARD Stage / HUD", &g_operatorStageHudVisible))
        {
            const auto& target = TargetFor(g_target);
            ImGui::TextUnformatted("Stage/HUD operator");
            ImGui::Separator();
            ImGui::Text("Target: %s", std::string(target.token).c_str());
            ImGui::Text("Target CSD: %s", std::string(target.primaryCsdScene).c_str());
            ImGui::Text("Requested stage: %s", g_requestedStageHarness.c_str());
            ImGui::Text("Stage harness: %s", std::string(GetStageHarnessLabel()).c_str());
            DrawPredicate("Stage context", g_stageContextObserved);
            DrawPredicate("Target CSD observed", g_targetCsdObserved);
            DrawPredicate("Stage target ready", g_loggedStageTargetReady);
            DrawHexField("stageGameModeAddress", g_lastStageGameModeAddress);
            ImGui::Text("stageContextFrame: %llu", static_cast<unsigned long long>(g_lastStageContextFrame));
            ImGui::Text("stageReadyEvent: %s", g_lastStageReadyEventName.empty() ? "waiting" : g_lastStageReadyEventName.c_str());
            ImGui::Text("stageReadyFrame: %llu", static_cast<unsigned long long>(g_lastStageReadyFrame));
            ImGui::Text("nextReadyEvent: %s", std::string(StageReadyEventName()).c_str());

            if (ImGui::Button("Emit Stage Ready If Latched"))
                EmitStageTargetReadyIfNeeded();
        }

        ImGui::End();
    }

    static void DrawOperatorLiveApiWindow()
    {
        if (!g_operatorLiveApiVisible)
            return;

        if (ImGui::Begin("SWARD Live API", &g_operatorLiveApiVisible))
        {
            const auto liveStatePath = LiveStateSnapshotPath();
            ImGui::TextUnformatted("live-state-json");
            ImGui::Separator();
            ImGui::TextWrapped("ui_lab_live_state.json: %s", liveStatePath.empty() ? "off" : liveStatePath.string().c_str());
            ImGui::Text("target: %s", std::string(GetTargetToken()).c_str());
            ImGui::Text("route: %s", std::string(GetRouteStatusLabel()).c_str());
            ImGui::Text("stageGameModeAddress: %s", HexU32(g_lastStageGameModeAddress).c_str());
            ImGui::Text("nativeCaptureStatus: %s", std::string(NativeFrameCaptureStatusLabel()).c_str());
            ImGui::Text("last snapshot frame: %llu", static_cast<unsigned long long>(g_lastLiveStateSnapshotFrame));
            ImGui::Separator();
            ImGui::Text("live bridge: %s", IsLiveBridgeEnabled() ? "enabled" : "off");
            ImGui::TextWrapped("pipe: %s", LiveBridgePipePath().c_str());
            ImGui::Text("commands: state, events, route-status, ui-oracle, ui-draw-list, ui-gpu-submit, ui-material-correlation, ui-backend-resolved, ui-vendor-command-capture, ui-layer-capture, ui-layer-status, route, reset, set-global, capture, help");
            ImGui::Text("debugForkTypedFields: %zu", kDebugMenuForkTypedFields.size());

            if (ImGui::CollapsingHeader("Typed live inspectors", ImGuiTreeNodeFlags_DefaultOpen))
            {
                const auto csd = BuildCsdLiveInspectorSnapshot();
                const auto loading = BuildLoadingLiveInspectorSnapshot();
                const auto sonicHud = BuildSonicHudLiveInspectorSnapshot();

                ImGui::Text("CSD project: %s", csd.projectName.empty() ? "waiting" : csd.projectName.c_str());
                DrawHexField("CSD scene", csd.sceneAddress);
                ImGui::Text("CSD scene motion frame: %.3f", csd.sceneMotionKnown ? csd.sceneMotionFrame : 0.0f);
                ImGui::Text("CSD scene repeat: %s", std::string(MotionRepeatTypeLabel(csd.sceneMotionRepeatType)).c_str());
                ImGui::Separator();
                ImGui::Text(
                    "Loading display type: %d (%s)",
                    loading.displayType == UINT32_MAX ? -1 : static_cast<int32_t>(loading.displayType),
                    std::string(LoadingDisplayTypeLabel(loading.displayType)).c_str());
                DrawPredicate("Loading display active", loading.displayActive);
                ImGui::Separator();
                ImGui::Text("Title cursor/menu owner: cursor=%u owner=%s",
                    g_titleMenuInspector.menuCursor,
                    HexU32(g_titleOwnerInspector.titleContextAddress).c_str());
                ImGui::Text("Sonic HUD play screen: %s", sonicHud.playScreenProject.empty() ? "waiting" : sonicHud.playScreenProject.c_str());
                ImGui::Text("Sonic HUD speed gauge: %s", sonicHud.speedGaugeScene.empty() ? "waiting" : sonicHud.speedGaugeScene.c_str());
                DrawHexField("Sonic HUD owner", sonicHud.hudOwnerAddress);
            }

            if (ImGui::CollapsingHeader("Debug-menu fork typed fields"))
            {
                for (const auto& field : kDebugMenuForkTypedFields)
                    ImGui::TextWrapped("%s | %s | %s", field.group, field.field, field.status);
            }

            if (ImGui::Button("Write Live State Snapshot"))
                WriteLiveStateSnapshot();
        }

        ImGui::End();
    }

    static void DrawOperatorDebugDrawLayer()
    {
        if (!g_operatorDebugDrawLayerVisible)
            return;

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        const ImGuiIO& io = ImGui::GetIO();
        const ImU32 lineColor = ImGui::GetColorU32(ImVec4(0.15f, 0.85f, 0.72f, 0.72f));
        const ImU32 textColor = ImGui::GetColorU32(ImVec4(0.95f, 1.0f, 0.82f, 0.92f));
        const ImU32 waitingColor = ImGui::GetColorU32(ImVec4(1.0f, 0.64f, 0.22f, 0.92f));
        const float top = 8.0f;
        const float right = io.DisplaySize.x - 12.0f;

        drawList->AddLine(ImVec2(12.0f, top), ImVec2(right, top), lineColor, 1.0f);
        drawList->AddLine(ImVec2(12.0f, top), ImVec2(12.0f, top + 24.0f), lineColor, 1.0f);

        const std::string status =
            "SWARD operator layer | target=" + std::string(GetTargetToken()) +
            " | route=" + std::string(GetRouteStatusLabel()) +
            " | native=" + std::string(NativeFrameCaptureStatusLabel());

        drawList->AddText(ImVec2(18.0f, top + 4.0f), textColor, status.c_str());

        if (g_target == ScreenId::TitleMenu && !g_titleMenuVisualReady)
        {
            drawList->AddText(
                ImVec2(18.0f, top + 24.0f),
                waitingColor,
                "title-menu visual latch waiting");
        }
    }

    void DrawOverlay()
    {
        if (!g_isEnabled)
            return;

        if (!ShouldDrawOverlay())
            return;

        if (!g_operatorShellVisible)
            return;

        DrawOperatorDebugDrawLayer();
        DrawOperatorProfilerPanel();
        DrawOperatorWindowList();
        DrawOperatorWelcomeWindow();
        DrawOperatorCounterWindow();
        DrawOperatorViewWindow();
        DrawOperatorExportsWindow();
        DrawOperatorDebugDrawWindow();
        DrawOperatorStageHudWindow();
        DrawOperatorLiveApiWindow();
    }
}
