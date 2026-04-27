#include <patches/ui_lab_patches.h>
#include <app.h>
#include <gpu/imgui/imgui_common.h>
#include <kernel/memory.h>
#include <os/logger.h>
#include <user/config.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>

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

    static constexpr std::array<RuntimeTarget, 10> kRuntimeTargets =
    {{
        { ScreenId::TitleLoop, "title-loop", "Title Loop", "ui_title", "System/GameMode/Title/TitleStateIntro.cpp", false },
        { ScreenId::TitleMenu, "title-menu", "Title Menu", "ui_title", "System/GameMode/Title/TitleMenu.cpp", false },
        { ScreenId::TitleOptions, "title-options", "Title Options", "ui_title", "UnleashedRecomp/ui/options_menu.cpp", false },
        { ScreenId::Loading, "loading", "Loading / Miles Electric", "ui_loading", "System/Loading.cpp", false },
        { ScreenId::SonicHud, "sonic-hud", "Sonic Stage HUD", "ui_playscreen", "Player/Character/Sonic/Hud/SonicMainDisplay.cpp", true },
        { ScreenId::ExtraStageHud, "extra-stage-hud", "Extra Stage / Tornado HUD", "ui_prov_playscreen", "ExtraStage/Tails/Hud/HudExQte.cpp", true },
        { ScreenId::Result, "result", "Stage Result", "ui_result", "HUD/Result/Result.cpp", true },
        { ScreenId::Status, "status", "Status / Skill Upgrade", "ui_status", "HUD/Status/Status.cpp", false },
        { ScreenId::Tutorial, "tutorial", "Tutorial / Control Guide", "ui_loading", "Player/Character/Sonic/Hud/SonicHudGuide.cpp", true },
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
    static bool g_routePending = false;
    static bool g_titleIntroAcceptInjected = false;
    static bool g_titleMenuAcceptInjected = false;
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
    static uint64_t g_lastLiveStateSnapshotFrame = 0;
    static std::string g_lastStageReadyEventName;
    static std::string g_lastCsdProjectName;
    static uint64_t g_lastCsdProjectFrame = 0;
    static std::string g_lastTitleIntroContextDetail;
    static std::string g_lastStageTitleContextDetail;
    static std::string g_lastTitleMenuContextDetail;
    static std::string g_lastStageContextDetail;
    static std::unordered_set<std::string> g_loggedCsdProjects;

    static const RuntimeTarget& TargetFor(ScreenId id);
    static bool TargetNeedsStageHarness(ScreenId id);
    static bool TargetShouldRouteThroughLoading(ScreenId id);
    static bool TargetRoutesThroughTitleMenu(ScreenId id);
    static void RefreshTargetCsdProjectStatus();
    static bool IsNativeFrameCaptureReady();
    static std::string_view NativeFrameCaptureStatusLabel();
    static void EmitStageTargetReadyIfNeeded();
    static std::array<OperatorWindowEntry, 8> GetOperatorWindowEntries();

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
        out
            << "{\"time\":" << SecondsSinceStart()
            << ",\"frame\":" << g_presentedFrameCount
            << ",\"event\":\"" << JsonEscape(event) << "\""
            << ",\"target\":\"" << JsonEscape(target.token) << "\""
            << ",\"label\":\"" << JsonEscape(target.label) << "\""
            << ",\"csd\":\"" << JsonEscape(target.primaryCsdScene) << "\""
            << ",\"route\":\"" << JsonEscape(g_routeStatus) << "\""
            << ",\"stage\":\"" << JsonEscape(GetStageHarnessLabel()) << "\"";

        if (!detail.empty())
            out << ",\"detail\":\"" << JsonEscape(detail) << "\"";

        out << "}\n";
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

    void OnStageTargetReady(std::string_view eventName, std::string_view detail)
    {
        if (!g_isEnabled)
            return;

        g_lastStageReadyEventName = std::string(eventName);
        g_lastStageReadyFrame = g_presentedFrameCount;
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
            !g_targetCsdObserved)
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

        const auto& target = TargetFor(g_target);
        out
            << "{\n"
            << "  \"time\": " << SecondsSinceStart() << ",\n"
            << "  \"frame\": " << g_presentedFrameCount << ",\n"
            << "  " << kLiveStateTargetFieldName << ": \"" << JsonEscape(target.token) << "\",\n"
            << "  \"targetLabel\": \"" << JsonEscape(target.label) << "\",\n"
            << "  " << kLiveStateRouteFieldName << ": \"" << JsonEscape(g_routeStatus) << "\",\n"
            << "  \"routePolicy\": \"" << JsonEscape(RoutePolicyLabel()) << "\",\n"
            << "  \"stageHarness\": \"" << JsonEscape(GetStageHarnessLabel()) << "\",\n"
            << "  " << kLiveStateStageGameModeAddressFieldName << ": \"" << JsonEscape(HexU32(g_lastStageGameModeAddress)) << "\",\n"
            << "  \"stageContextFrame\": " << g_lastStageContextFrame << ",\n"
            << "  \"stageReadyEvent\": \"" << JsonEscape(g_lastStageReadyEventName) << "\",\n"
            << "  \"stageReadyFrame\": " << g_lastStageReadyFrame << ",\n"
            << "  \"targetCsdObserved\": " << (g_targetCsdObserved ? "true" : "false") << ",\n"
            << "  " << kLiveStateNativeCaptureStatusFieldName << ": \"" << JsonEscape(NativeFrameCaptureStatusLabel()) << "\",\n"
            << "  \"lastNativeFrameCapturePath\": \"" << JsonEscape(g_lastNativeFrameCapturePath) << "\",\n"
            << "  \"lastCsdProject\": \"" << JsonEscape(g_lastCsdProjectName) << "\",\n"
            << "  \"loadingDisplayType\": "
            << (g_lastLoadingDisplayType == UINT32_MAX ? -1 : static_cast<int32_t>(g_lastLoadingDisplayType))
            << ",\n"
            << "  \"titleMenuVisualReady\": " << (g_titleMenuVisualReady ? "true" : "false") << "\n"
            << "}\n";

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

    const std::array<RuntimeTarget, 10>& GetRuntimeTargets()
    {
        return kRuntimeTargets;
    }

    void RequestRouteToCurrentTarget()
    {
        if (g_observerMode)
        {
            g_routePending = false;
            g_routeStatus = "observer mode";
            WriteEvidenceEvent("route-disabled-observer");
            return;
        }

        g_titleIntroAcceptInjected = false;
        g_titleMenuAcceptInjected = false;
        g_stageContextObserved = false;
        g_targetCsdObserved = false;
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
        g_lastStageContextDetail.clear();
        g_lastStageReadyEventName.clear();
        g_lastStageGameModeAddress = 0;
        g_lastStageContextFrame = 0;
        g_lastStageReadyFrame = 0;

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

        if (g_autoExitSeconds > 0.0 && !g_autoExitRequested && SecondsSinceStart() >= g_autoExitSeconds)
        {
            g_autoExitRequested = true;
            WriteEvidenceEvent("auto-exit");
            App::Exit();
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

        if (displayType != 0)
        {
            g_loadingDisplayWasActive = true;
            g_routeStatus = "loading display active";
            WriteEvidenceEvent("loading-display-active", "display_type=" + std::to_string(displayType));
        }
        else if (g_loadingDisplayWasActive)
        {
            g_loadingDisplayWasActive = false;
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

        if (!g_loggedCsdProjects.insert(project).second)
            return;

        WriteEvidenceEvent("csd-project-made", project);

        MarkTargetCsdProjectLive(projectName);
    }

    bool ApplyTitleIntroStateForcing(float elapsedSeconds, bool& directState)
    {
        directState = false;

        if (!g_isEnabled || g_observerMode || !g_routePending || g_titleIntroAcceptInjected)
            return false;

        if (!TargetCanRouteFromTitleIntro(g_target))
            return false;

        if (elapsedSeconds < 0.75f)
            return false;

        g_titleIntroAcceptInjected = true;

        if (g_routePolicy == RoutePolicy::DirectContext && g_target != ScreenId::TitleMenu)
        {
            directState = true;
            g_routeStatus = "title intro direct state requested";
            LOGFN("SWARD UI Lab route: requested direct title intro state for target={}", TargetFor(g_target).token);
            WriteEvidenceEvent("title-intro-direct-state-requested", "state=1 dirty=1");
            return false;
        }

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
            (g_target == ScreenId::TitleMenu || g_target == ScreenId::TitleOptions);
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

        if (!TargetShouldRouteThroughLoading(g_target) || !g_routePending)
            return false;

        cursorIndex = 0;
        g_routeStatus = TargetNeedsStageHarness(g_target)
            ? "stage route via new game"
            : "loading route via new game";

        if (!g_titleMenuAcceptInjected)
        {
            g_titleMenuAcceptInjected = true;
            g_routePending = false;

            if (g_routePolicy == RoutePolicy::DirectContext)
            {
                directContext = true;
                g_routeStatus = "direct context requested";
                LOGFN("SWARD UI Lab route: requested direct title menu context latch for target={}", TargetFor(g_target).token);
                WriteEvidenceEvent("title-menu-direct-context-requested");
            }
            else
            {
                injectAccept = true;
                g_routeStatus = TargetNeedsStageHarness(g_target)
                    ? "stage accept injected"
                    : "loading accept injected";
                LOGFN("SWARD UI Lab route: forced title menu New Game accept for target={}", TargetFor(g_target).token);
                WriteEvidenceEvent("title-menu-new-game-accept-injected");
            }
        }

        return true;
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
