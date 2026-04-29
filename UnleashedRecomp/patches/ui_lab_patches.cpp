#include <patches/ui_lab_patches.h>
#include <app.h>
#include <api/SWA.h>
#include <gpu/imgui/imgui_common.h>
#include <kernel/memory.h>
#include <os/logger.h>
#include <user/config.h>
#include <algorithm>
#include <atomic>
#include <chrono>
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

    static constexpr std::array<ChudSonicStageExpectedOwnerField, 4> kChudSonicStageExpectedOwnerFields =
    {{
        { "m_rcPlayScreen", 0xE0, 0xE4 },
        { "m_rcSpeedGauge", 0xE8, 0xEC },
        { "m_rcRingEnergyGauge", 0xF0, 0xF4 },
        { "m_rcGaugeFrame", 0xF8, 0xFC },
    }};

    static constexpr std::string_view kChudSonicStageExpectedOwnerFieldSource =
        "api/SWA/HUD/Sonic/HudSonicStage.h offsets 0xE0..0xFC";

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

    static bool g_isEnabled = false;
    static bool g_observerMode = false;
    static bool g_routeTargetExplicit = false;
    static bool g_hideOverlay = false;
    static bool g_operatorShellVisible = true;
    static bool g_operatorShellToggleWasDown = false;
    static bool g_operatorWindowListVisible = true;
    static bool g_operatorInspectorVisible = true;
    static bool g_operatorCounterVisible = true;
    static bool g_operatorViewVisible = true;
    static bool g_operatorExportsVisible = true;
    static bool g_operatorDebugDrawVisible = true;
    static bool g_operatorWelcomeVisible = true;
    static bool g_operatorStageHudVisible = true;
    static bool g_operatorLiveApiVisible = true;
    static bool g_operatorDebugDrawLayerVisible = true;
    static ImVec2 g_operatorDebugIconPos = { 18.0f, 18.0f };
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
    static uint64_t g_chudSonicStageRawHookFrame = 0;
    static std::string g_chudSonicStageRawHookSource;
    static bool g_loggedChudSonicStageOwnerHook = false;
    static bool g_loggedChudSonicStageOwnerFieldSample = false;
    static bool g_loggedTutorialHudOwnerPathReady = false;
    static uint64_t g_chudSonicStageOwnerFieldSampleCount = 0;
    static std::vector<SonicHudOwnerFieldSample> g_chudSonicStageOwnerFieldSamples;
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
    static std::vector<RuntimeMaterialCorrelation> BuildRuntimeMaterialCorrelationPairs(
        const std::vector<RuntimeUiDrawCall>& drawCalls,
        const std::vector<RuntimeGpuSubmitCall>& submitCalls);
    static void AppendSonicHudOwnerFieldSamples(std::ostringstream& out, const std::vector<SonicHudOwnerFieldSample>& samples);
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
        return g_isEnabled && ShouldDrawOverlay();
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

    void UpdateOperatorShellToggle(bool f1Down)
    {
        if (!ShouldReserveF1DebugToggle())
        {
            g_operatorShellToggleWasDown = false;
            return;
        }

        if (!g_operatorShellToggleWasDown && f1Down)
        {
            g_operatorShellVisible = !g_operatorShellVisible;
            g_routeStatus = g_operatorShellVisible ? "operator shell visible" : "operator shell hidden";
            WriteEvidenceEvent("operator-shell-f1-toggle", g_operatorShellVisible ? "visible" : "hidden");
        }

        g_operatorShellToggleWasDown = f1Down;
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
        R"("uiDrawSequence")",
        R"("gpuSubmitSequence")",
        R"("correlationMethod": "same-frame-order-window")",
        R"("backendResolvedJoinMethod": "same-frame-order-window")",
        R"("blendParityPolicy": "backend-resolved-pso-blend")",
        R"("framebufferParityPolicy": "backend-resolved-framebuffer-registration")",
        R"("textureViewSamplerGap": "pending-descriptor-view-decode")",
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
        snapshot.rawOwnerKnown = g_chudSonicStageOwnerAddress != 0;
        snapshot.rawOwnerFieldsReady =
            g_chudSonicStagePlayScreenProjectAddress != 0 ||
            g_chudSonicStageSpeedGaugeSceneAddress != 0 ||
            g_chudSonicStageRingEnergyGaugeSceneAddress != 0 ||
            g_chudSonicStageGaugeFrameSceneAddress != 0;
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

            if (!snapshot.rawOwnerKnown)
                snapshot.ownerPointerStatus = "resolved CSD ownership; raw CHudSonicStage owner hook pending runtime observation";

            if (snapshot.rawOwnerKnown && !snapshot.rawOwnerFieldsReady && snapshot.rawOwnerResolvedMemoryCount == 0)
            {
                snapshot.ownerFieldMaturationStatus =
                    "fork API CHudSonicStage RCPtr slots stayed null; resolved ui_playscreen project/scene addresses came from CCsdProject::Make traversal";
            }
        }

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
            g_chudSonicStageGaugeFrameSceneAddress != 0;
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
        const auto sonicOwnerPath = BuildSonicHudOwnerPathInspectorSnapshot(csdProjectTree);
        const auto pauseGeneralSave = BuildPauseGeneralSaveLiveInspectorSnapshot();

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
            << "        \"resolvedFromCsdProjectTree\": " << (sonicOwnerPath.resolvedFromCsdProjectTree ? "true" : "false") << ",\n"
            << "        \"expectedOwnerFieldSource\": \"" << JsonEscape(sonicOwnerPath.expectedOwnerFieldSource) << "\",\n"
            << "        \"rawOwnerExpectedFieldOffsets\": \"" << JsonEscape(std::string(kChudSonicStageExpectedOwnerFieldSource)) << "\",\n"
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

    static std::string BuildRuntimeBackendResolvedJson()
    {
        const auto& target = TargetFor(g_target);

        std::vector<RuntimeBackendResolvedSubmit> submits;
        std::vector<RuntimeUiDrawCall> drawCalls;
        std::vector<RuntimeGpuSubmitCall> gpuSubmitCalls;
        uint64_t frame = g_presentedFrameCount;
        uint32_t droppedCount = 0;
        uint32_t sequence = 0;
        {
            std::lock_guard<std::mutex> lock(g_typedInspectorMutex);
            submits = g_runtimeBackendResolvedSubmits;
            drawCalls = g_runtimeUiDrawCalls;
            gpuSubmitCalls = g_runtimeGpuSubmitCalls;
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
        AppendStringArray(out, { "state", "events", "route-status", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "route <target>", "reset", "set-global <name> <0|1>", "capture", "help" });
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
        g_chudSonicStageRawHookFrame = 0;
        g_chudSonicStageRawHookSource.clear();
        g_loggedChudSonicStageOwnerHook = false;
        g_loggedChudSonicStageOwnerFieldSample = false;
        g_loggedTutorialHudOwnerPathReady = false;
        g_chudSonicStageOwnerFieldSampleCount = 0;
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

        g_runtimeUiDrawCalls.push_back(std::move(call));
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

        const bool hasOwnerFields =
            playScreenProjectAddress != 0 ||
            speedGaugeSceneAddress != 0 ||
            ringEnergyGaugeSceneAddress != 0 ||
            gaugeFrameSceneAddress != 0;

        const bool changed =
            g_chudSonicStageOwnerAddress != ownerAddress ||
            g_chudSonicStagePlayScreenProjectAddress != playScreenProjectAddress ||
            g_chudSonicStageSpeedGaugeSceneAddress != speedGaugeSceneAddress ||
            g_chudSonicStageRingEnergyGaugeSceneAddress != ringEnergyGaugeSceneAddress ||
            g_chudSonicStageGaugeFrameSceneAddress != gaugeFrameSceneAddress ||
            g_chudSonicStageRawHookSource != hookSource;

        g_chudSonicStageOwnerAddress = ownerAddress;
        g_chudSonicStagePlayScreenProjectAddress = playScreenProjectAddress;
        g_chudSonicStageSpeedGaugeSceneAddress = speedGaugeSceneAddress;
        g_chudSonicStageRingEnergyGaugeSceneAddress = ringEnergyGaugeSceneAddress;
        g_chudSonicStageGaugeFrameSceneAddress = gaugeFrameSceneAddress;
        g_chudSonicStageRawHookFrame = g_presentedFrameCount;
        g_chudSonicStageRawHookSource = hookSource;

        if (!g_loggedChudSonicStageOwnerHook || changed)
        {
            WriteEvidenceEvent(
                "sonic-hud-owner-hooked",
                "owner=" + HexU32(ownerAddress) +
                " play_screen=" + HexU32(playScreenProjectAddress) +
                " speed_gauge=" + HexU32(speedGaugeSceneAddress) +
                " ring_energy_gauge=" + HexU32(ringEnergyGaugeSceneAddress) +
                " gauge_frame=" + HexU32(gaugeFrameSceneAddress) +
                " owner_fields_ready=" + std::string(hasOwnerFields ? "1" : "0") +
                " source=" + std::string(hookSource) +
                (hasOwnerFields
                    ? " status=raw CHudSonicStage owner hook"
                    : " status=raw CHudSonicStage owner hook owner-only; CSD owner fields pending"));
            g_loggedChudSonicStageOwnerHook = true;
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

            if (sample.rcObjectKnown)
            {
                uint32_t resolvedMemoryAddress = 0;
                if (TryReadGuestU32(sample.rcObjectAddress + 0x4, resolvedMemoryAddress) &&
                    IsPlausibleGuestPointer(resolvedMemoryAddress))
                {
                    sample.resolvedMemoryAddress = resolvedMemoryAddress;
                    sample.resolvedMemoryKnown = true;
                    ++resolvedMemoryCount;
                }
            }

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

        if (!g_loggedChudSonicStageOwnerFieldSample || changed || resolvedMemoryCount != 0)
        {
            WriteEvidenceEvent(
                "sonic-hud-owner-field-sample",
                "owner=" + HexU32(ownerAddress) +
                " source=" + std::string(hookSource) +
                " expected=" + std::string(kChudSonicStageExpectedOwnerFieldSource) +
                " sample_count=" + std::to_string(kChudSonicStageExpectedOwnerFields.size()) +
                " resolved_memory_count=" + std::to_string(resolvedMemoryCount));
            g_loggedChudSonicStageOwnerFieldSample = true;
            EmitTutorialHudOwnerPathReadyIfNeeded();
            WriteLiveStateSnapshot();
        }

        EmitStageTargetReadyIfNeeded();
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
        AppendStringArray(out, { "state", "events", "route-status", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "route <target>", "reset", "set-global <name> <0|1>", "capture", "help" });
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
            ImGui::TextUnformatted("Default-open operator windows");
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
            ImGui::Text("commands: state, events, route-status, ui-oracle, ui-draw-list, ui-gpu-submit, ui-material-correlation, ui-backend-resolved, route, reset, set-global, capture, help");
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
        DrawOperatorDebugIcon();
        DrawOperatorWindowList();
        DrawOperatorWelcomeWindow();

        if (!g_operatorInspectorVisible)
        {
            DrawOperatorCounterWindow();
            DrawOperatorViewWindow();
            DrawOperatorExportsWindow();
            DrawOperatorDebugDrawWindow();
            DrawOperatorStageHudWindow();
            DrawOperatorLiveApiWindow();
            return;
        }

        ImGui::SetNextWindowPos({ 18.0f, 72.0f }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.82f);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing;

        if (ImGui::Begin("SWARD UI Lab", nullptr, flags))
        {
            if (ImGui::BeginTabBar("ui-lab-inspector-tabs"))
            {
                if (ImGui::BeginTabItem("Overview"))
                {
                    DrawRuntimeInspectorOverview();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Title/Menu"))
                {
                    DrawTitleMenuLatchInspector();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Capture"))
                {
                    DrawCaptureInspector();
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

        DrawOperatorCounterWindow();
        DrawOperatorViewWindow();
        DrawOperatorExportsWindow();
        DrawOperatorDebugDrawWindow();
        DrawOperatorStageHudWindow();
        DrawOperatorLiveApiWindow();
    }
}
