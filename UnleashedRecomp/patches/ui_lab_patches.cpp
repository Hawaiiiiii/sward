#include <patches/ui_lab_patches.h>
#include <app.h>
#include <gpu/imgui/imgui_common.h>
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
    static bool g_nativeFrameCaptureWritten = false;
    static uint32_t g_nativeFrameCaptureIndex = 0;
    static const std::chrono::steady_clock::time_point g_startedAt = std::chrono::steady_clock::now();
    static bool g_loggedIntroHook = false;
    static bool g_loggedMenuHook = false;
    static bool g_loggedStageHarness = false;
    static bool g_loggedTargetCsdProjectLive = false;
    static bool g_loggedStageTargetCsdBound = false;
    static uint64_t g_titleIntroContextSampleCount = 0;
    static uint64_t g_stageTitleContextSampleCount = 0;
    static uint32_t g_lastLoadingDisplayType = UINT32_MAX;
    static bool g_loadingDisplayWasActive = false;
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

    static bool ShouldDrawOverlay()
    {
        if (!g_hideOverlay)
            return true;

        return false;
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
    }

    static void RefreshTargetCsdProjectStatus()
    {
        const auto& target = TargetFor(g_target);

        if (HasObservedCsdProject(target.primaryCsdScene))
            MarkTargetCsdProjectLive(target.primaryCsdScene);
    }

    static bool IsNativeFrameCaptureReady()
    {
        if (!g_isEnabled || !g_nativeFrameCaptureEnabled || g_nativeFrameCaptureReserved || g_nativeFrameCaptureWritten)
            return false;

        if (g_presentedFrameCount < 2)
            return false;

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
                return !g_routePending && g_loggedMenuHook;

            case ScreenId::TitleOptions:
                return g_titleMenuAcceptInjected;

            default:
                return g_targetCsdObserved;
        }
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
        g_nativeFrameCaptureReserved = false;
        g_nativeFrameCaptureWritten = false;
        g_lastStageContextDetail.clear();

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

        g_nativeFrameCaptureWritten = true;
        WriteEvidenceEvent(
            "native-frame-captured",
            "path=" + std::string(path) +
            " width=" + std::to_string(width) +
            " height=" + std::to_string(height) +
            " format=B8G8R8A8 bmp=1");
    }

    void OnNativeFrameCaptureFailed(std::string_view reason)
    {
        if (!g_isEnabled)
            return;

        WriteEvidenceEvent("native-frame-capture-failed", reason);
    }

    void OnLoadingRequest(uint32_t displayType)
    {
        if (!g_isEnabled)
            return;

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

        if (g_routePolicy == RoutePolicy::DirectContext)
        {
            directState = true;
            g_routeStatus = "title intro direct state requested";
            LOGFN("SWARD UI Lab route: requested direct title intro state for target={}", TargetFor(g_target).token);
            WriteEvidenceEvent("title-intro-direct-state-requested", "state=1 dirty=1");
            return false;
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

    bool ApplyTitleMenuStateForcing(int32_t& cursorIndex, bool& injectAccept, bool& suppressAccept, bool& directContext)
    {
        injectAccept = false;
        suppressAccept = false;
        directContext = false;

        if (!g_isEnabled || g_observerMode)
            return false;

        if (g_target == ScreenId::TitleMenu)
        {
            if (g_routePending)
            {
                g_routePending = false;
                g_routeStatus = "title menu reached";
                LOGN("SWARD UI Lab route: title menu reached.");
                WriteEvidenceEvent("title-menu-reached");
                suppressAccept = true;
                WriteEvidenceEvent("title-menu-accept-suppressed");
            }

            return false;
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

    void DrawOverlay()
    {
        if (!g_isEnabled)
            return;

        if (!ShouldDrawOverlay())
            return;

        const auto& target = TargetFor(g_target);
        const std::string targetLabel(target.label);
        const std::string targetToken(target.token);
        const std::string targetScene(target.primaryCsdScene);
        const std::string targetFamily(target.sourceFamily);

        ImGui::SetNextWindowPos({ 18.0f, 18.0f }, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.82f);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing;

        if (ImGui::Begin("SWARD UI Lab", nullptr, flags))
        {
            ImGui::TextUnformatted("Runtime-backed UI Lab");
            ImGui::Separator();
            ImGui::Text("Mode: %s", g_observerMode ? "observer mode" : "route forcing");
            ImGui::Text("Target: %s [%s]", targetLabel.c_str(), targetToken.c_str());
            ImGui::Text("CSD scene: %s", targetScene.c_str());
            ImGui::Text("Source family: %s", targetFamily.c_str());
            ImGui::Text("Stage context: %s", target.requiresStageContext ? "required" : "not required");
            ImGui::Text("Stage harness: %s", std::string(GetStageHarnessLabel()).c_str());
            ImGui::Text("Target CSD: %s", std::string(GetTargetCsdStatusLabel()).c_str());
            ImGui::Text("Title intro hook: %s", g_loggedIntroHook ? "attached" : "waiting");
            ImGui::Text("Title menu hook: %s", g_loggedMenuHook ? "attached" : "waiting");
            ImGui::Text("Route: %s", std::string(GetRouteStatusLabel()).c_str());
            ImGui::Text("Route policy: %s", std::string(RoutePolicyLabel()).c_str());
            ImGui::Text("Presented frames: %llu", static_cast<unsigned long long>(g_presentedFrameCount));
            ImGui::Text("Evidence: %s", g_evidenceDirectory.empty() ? "off" : g_evidenceDirectory.string().c_str());
            ImGui::Text(
                "Native capture: %s",
                !g_nativeFrameCaptureEnabled ? "off" :
                g_nativeFrameCaptureWritten ? "written" :
                g_nativeFrameCaptureReserved ? "reserved" :
                IsNativeFrameCaptureReady() ? "ready" : "waiting");
            ImGui::Text("Startup prompt blockers: %s", ShouldBypassStartupPromptBlockers() ? "bypassed" : "normal");
            ImGui::Separator();

            if (ImGui::Button("Previous Target"))
                SelectPreviousTarget();

            ImGui::SameLine();

            if (ImGui::Button("Next Target"))
                SelectNextTarget();

            ImGui::SameLine();

            if (ImGui::Button("Route Selected Target"))
                RequestRouteToCurrentTarget();

            ImGui::SameLine();

            if (ImGui::Button("Mark Evidence"))
                WriteEvidenceEvent("manual-evidence-marker");

            ImGui::Separator();
            ImGui::TextUnformatted("Real screen routes:");

            for (size_t i = 0; i < kRuntimeTargets.size(); ++i)
            {
                const auto& runtimeTarget = kRuntimeTargets[i];
                const std::string label(runtimeTarget.label);
                const std::string token(runtimeTarget.token);
                const std::string buttonLabel = (runtimeTarget.id == g_target ? "> " : "  ") + label + "###ui-lab-target-" + std::to_string(i);

                if (ImGui::Button(buttonLabel.c_str()))
                    SelectTargetIndex(i);

                ImGui::SameLine();
                ImGui::Text("(%s)", token.c_str());
            }
        }

        ImGui::End();
    }
}
