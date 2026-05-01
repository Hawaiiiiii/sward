#include <sward/ui_runtime/frontend_screen_controllers.hpp>

#include <sward/ui_runtime/frontend_screen_reference.hpp>
#include <sward/ui_runtime/sonic_hud_reference.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace sward::ui_runtime
{
namespace
{
constexpr std::string_view kSonicDayHudNext = "sonic-day-hud-next";

const SonicDayHudValueProvenance kSonicDayHudValueProvenance{
    "ui_playscreen/ring_count",
    "ui_playscreen/score_count",
    "ui_playscreen/time_count",
    "ui_playscreen/so_speed_gauge",
    "ui_playscreen/so_speed_gauge",
    "ui_playscreen/so_ringenagy_gauge",
    "ui_playscreen/player_count",
    "ui_playscreen/add/u_info",
    "stage-route-hook=CGameModeStage::ExitLoading",
    "host/live-bridge supplied value",
};

template <typename T>
std::string joinStrings(const std::vector<T>& values, std::string_view separator)
{
    std::ostringstream out;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index != 0)
            out << separator;
        out << values[index];
    }
    return out.str();
}

std::vector<std::string> sceneNamesForPolicy(std::string_view screenId)
{
    std::vector<std::string> scenes;
    if (const auto* policy = findFrontendScreenPolicy(screenId))
    {
        for (const auto& scene : policy->scenes)
            scenes.push_back(scene.sceneName);
    }
    return scenes;
}

std::vector<std::string> firstSceneOnly(std::string_view screenId)
{
    auto scenes = sceneNamesForPolicy(screenId);
    if (scenes.size() > 1)
        scenes.resize(1);
    return scenes;
}

std::vector<std::string> sonicHudSceneNames(bool includeTutorial)
{
    std::vector<std::string> scenes;
    for (const auto& scene : sonicHudScenePolicies())
    {
        if (!includeTutorial && scene.activationEvent == "tutorial-hud-owner-path-ready")
            continue;
        scenes.push_back(scene.sceneName);
    }
    return scenes;
}

std::vector<std::string> sonicHudSceneName(std::string_view sceneName)
{
    if (const auto* scene = findSonicHudScenePolicy(sceneName))
        return { scene->sceneName };
    return {};
}

std::string formatPaddedInt(int value, int width)
{
    std::ostringstream out;
    out << std::setw(width) << std::setfill('0') << std::max(0, value);
    return out.str();
}

std::string formatElapsedFrames(int elapsedFrames)
{
    const int clampedFrames = std::max(0, elapsedFrames);
    const int totalSeconds = clampedFrames / 60;
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;
    const int frames = clampedFrames % 60;

    std::ostringstream out;
    out << formatPaddedInt(minutes, 2)
        << ':' << formatPaddedInt(seconds, 2)
        << ':' << formatPaddedInt(frames, 2);
    return out.str();
}

std::string formatGauge(double value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << std::clamp(value, 0.0, 1.0);
    return out.str();
}

std::string formatRuntimeBindingStatus(const SonicDayHudRuntimeValueBinding& binding)
{
    if (binding.known)
        return "known:" + binding.source;
    return binding.source;
}

bool parseUnsignedRuntimeText(std::string_view text, int& value)
{
    int result = 0;
    bool sawDigit = false;
    for (const char c : text)
    {
        if (!std::isdigit(static_cast<unsigned char>(c)))
            continue;

        sawDigit = true;
        result = (result * 10) + (c - '0');
    }

    if (!sawDigit)
        return false;

    value = result;
    return true;
}

SonicDayHudRuntimeBindingSnapshot makeRuntimeScoreBindingSnapshot()
{
    SonicDayHudRuntimeBindingSnapshot snapshot;
    snapshot.source = "typedInspectors.sonicHud.gameplayValues";
    snapshot.values.score = 1250;
    snapshot.scoreBinding.known = true;
    snapshot.scoreBinding.source = "SWA::CGameDocument::m_pMember->m_ScoreInfo.EnemyScore+TrickScore";
    snapshot.scoreInfoPointMarkerRecordSpeedBinding.known = true;
    snapshot.scoreInfoPointMarkerRecordSpeedBinding.source =
        "SWA::CGameDocument::m_pMember->m_ScoreInfo.PointMarkerRecordSpeed";
    snapshot.scoreInfoPointMarkerCountBinding.known = true;
    snapshot.scoreInfoPointMarkerCountBinding.source =
        "SWA::CGameDocument::m_pMember->m_ScoreInfo.PointMarkerCount";
    snapshot.ringCountBinding.source = "pending-runtime-field";
    snapshot.elapsedFramesBinding.source = "pending-runtime-field";
    snapshot.speedKmhBinding.source = "pending-runtime-field";
    snapshot.boostGaugeBinding.source = "pending-runtime-field";
    snapshot.ringEnergyGaugeBinding.source = "pending-runtime-field";
    snapshot.lifeCountBinding.source = "pending-runtime-field";
    snapshot.tutorialPromptBinding.source = "pending-runtime-field";
    snapshot.sonicRingPickupSfxId = "audio-id-pending";
    snapshot.tutorialPromptOpenSfxId = "audio-id-pending";
    snapshot.pauseOpenSfxId = "sys_actstg_pausewinopen";
    snapshot.pauseCursorSfxId = "sys_actstg_pausecursor";
    return snapshot;
}

SonicDayHudRuntimeValueUpdatePath makeRuntimeValueUpdatePathSnapshot()
{
    SonicDayHudRuntimeValueUpdatePath paths;
    paths.ringCountWritePath.known = true;
    paths.ringCountWritePath.source =
        "CSD::CNode::SetText/sub_830BF640@ui_playscreen/ring_count/num_ring";
    paths.elapsedFramesWritePath.known = true;
    paths.elapsedFramesWritePath.source =
        "CSD::CNode::SetText/sub_830BF640@ui_playscreen/time_count/time001|time010|time100"
        ":pathResolutionSource=raw-chud-sonic-stage-owner-field";
    paths.speedReadoutWritePath.known = true;
    paths.speedReadoutWritePath.source =
        "CSD::CNode::SetText/sub_830BF640@ui_playscreen/add/speed_count/position/num_speed";
    paths.lifeCountWritePath.known = true;
    paths.lifeCountWritePath.source =
        "CSD::CNode::SetText/sub_830BF640@ui_playscreen/player_count/player"
        ":pathResolutionSource=raw-chud-sonic-stage-owner-field";
    paths.boostGaugeWritePath.source = "pending-gauge-or-prompt-write-hook";
    paths.ringEnergyGaugeWritePath.source = "pending-gauge-or-prompt-write-hook";
    paths.tutorialPromptWritePath.source = "pending-gauge-or-prompt-write-hook";
    return paths;
}

SonicDayHudRuntimeDrawListCoverage makeRuntimeDrawListCoverageSnapshot()
{
    SonicDayHudRuntimeDrawListCoverage coverage;
    coverage.source = "live-bridge/ui-draw-list manual observer";
    coverage.activeProject = "ui_playscreen";
    coverage.runtimeDrawCalls = 96;
    coverage.correlatedMaterialPairs = 96;
    coverage.speedGaugeObserved = true;
    coverage.gaugeFrameObserved = true;
    coverage.ringEnergyGaugeObserved = true;
    coverage.tutorialPromptObserved = false;
    coverage.pauseOverlayObserved = true;
    coverage.textWriteObserved = false;
    coverage.nextHook = "CSD::CNode::SetPatternIndex/SetHideFlag/SetScale";
    return coverage;
}

std::vector<SonicDayHudRuntimeTextWriteObservation> makeRuntimeOwnerFieldTextWriteObservations()
{
    return {
        {
            "elapsedFrames",
            "ui_playscreen/time_count/time100",
            "39",
            "raw-chud-sonic-stage-owner-field",
            "sonic-hud-value-text-write:CSD::CNode::SetText/sub_830BF640",
        },
        {
            "lifeCount",
            "ui_playscreen/player_count/player",
            "03",
            "raw-chud-sonic-stage-owner-field",
            "sonic-hud-value-text-write:CSD::CNode::SetText/sub_830BF640",
        },
    };
}

std::vector<SonicDayHudRuntimeGaugePromptWriteObservation> makeRuntimeGaugePromptWriteObservations()
{
    return {
        {
            "boostGauge",
            "ui_playscreen/so_speed_gauge",
            "scale",
            0.650,
            true,
            "raw-chud-sonic-stage-owner-field",
            "sonic-hud-gauge-scale-write:CSD::CNode::SetScale/sub_830BF090",
        },
        {
            "ringEnergyGauge",
            "ui_playscreen/so_ringenagy_gauge",
            "scale",
            0.720,
            true,
            "raw-chud-sonic-stage-owner-field",
            "sonic-hud-gauge-scale-write:CSD::CNode::SetScale/sub_830BF090",
        },
        {
            "tutorialPrompt",
            "ui_playscreen/add/u_info",
            "pattern-index",
            3.0,
            true,
            "csd-child-lookup-chain",
            "sonic-hud-gauge-pattern-write:CSD::CNode::SetPatternIndex/sub_830BF300",
        },
        {
            "tutorialPrompt",
            "ui_playscreen/add/u_info",
            "hide-flag",
            0.0,
            true,
            "csd-child-lookup-chain",
            "sonic-hud-gauge-hide-write:CSD::CNode::SetHideFlag/sub_830BF080",
        },
    };
}

std::vector<SonicDayHudRuntimeSemanticPathCandidateObservation> makeRuntimeSemanticPathCandidateObservations()
{
    return {
        {
            "speedKmh",
            "ui_playscreen/add/speed_count/position/num_speed",
            "text",
            "042",
            0.0,
            false,
            2,
            "generated-PPC-callsite-semantic-candidate",
            "semantic-bound-pending-exact-child-node-resolution",
            "sonic-hud-node-write-semantic-bound:same-frame-hud-update-context:sub_824D6418",
        },
        {
            "boostGauge",
            "ui_playscreen/so_speed_gauge",
            "scale",
            "",
            0.650,
            true,
            121,
            "generated-PPC-callsite-semantic-candidate",
            "semantic-candidate-only-pending-runtime-bound",
            "sonic-hud-node-write-semantic-path-candidate:same-frame-hud-update-context:sub_824D6C18",
        },
        {
            "ringEnergyGauge",
            "ui_playscreen/so_ringenagy_gauge",
            "scale",
            "",
            0.720,
            true,
            121,
            "generated-PPC-callsite-semantic-candidate",
            "semantic-candidate-only-pending-runtime-bound",
            "sonic-hud-node-write-semantic-path-candidate:same-frame-hud-update-context:sub_824D6C18",
        },
        {
            "tutorialPrompt",
            "ui_playscreen/add/u_info",
            "pattern-index",
            "",
            3.0,
            true,
            80,
            "generated-PPC-callsite-semantic-candidate",
            "semantic-bound-pending-exact-child-node-resolution",
            "sonic-hud-node-write-semantic-bound:same-frame-hud-update-context:sub_824D7100",
        },
    };
}

std::vector<SonicDayHudRuntimeSemanticPathCandidateObservation> makeRuntimePhase193SemanticBindingObservations()
{
    return {
        {
            "elapsedFrames",
            "ui_playscreen/time_count/time001",
            "text",
            "02",
            0.0,
            false,
            10,
            "generated-PPC-callsite-semantic-candidate",
            "semantic-bound-pending-exact-child-node-resolution",
            "phase192-live:sonic-hud-node-write-semantic-bound:same-frame-hud-update-context:sub_824D6048",
        },
        {
            "speedKmh",
            "ui_playscreen/add/speed_count/position/num_speed",
            "text",
            "00",
            0.0,
            false,
            2,
            "generated-PPC-callsite-semantic-candidate",
            "semantic-bound-pending-exact-child-node-resolution",
            "phase192-live:sonic-hud-node-write-semantic-bound:same-frame-hud-update-context:sub_824D6418",
        },
        {
            "tutorialPrompt",
            "ui_playscreen/add/u_info",
            "pattern-index",
            "",
            1.0,
            true,
            5,
            "generated-PPC-callsite-semantic-candidate",
            "semantic-bound-pending-exact-child-node-resolution",
            "phase192-live:sonic-hud-node-write-semantic-bound:nearest-generated-PPC-callsite-sample:generated-PPC:sub_824D7100",
        },
        {
            "boostGauge",
            "ui_playscreen/so_speed_gauge",
            "text",
            "357",
            0.0,
            false,
            338,
            "generated-PPC-callsite-semantic-candidate",
            "semantic-candidate-only-pending-runtime-bound",
            "phase192-live:sonic-hud-node-write-semantic-path-candidate:same-frame-hud-update-context:sub_824D6C18",
        },
        {
            "ringEnergyGauge",
            "ui_playscreen/so_ringenagy_gauge",
            "text",
            "357",
            0.0,
            false,
            338,
            "generated-PPC-callsite-semantic-candidate",
            "semantic-candidate-only-pending-runtime-bound",
            "phase192-live:sonic-hud-node-write-semantic-path-candidate:same-frame-hud-update-context:sub_824D6C18",
        },
    };
}

std::vector<SonicDayHudRuntimeGaugeChildPathResolution> makeRuntimePhase194GaugeChildPathResolutions()
{
    return {
        {
            "boostGauge",
            "ui_playscreen/so_speed_gauge/position/speed_gauge_color",
            "ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506",
            20,
            "live-bridge/ui-draw-list",
            "setter-node-address-join-pending",
        },
        {
            "ringEnergyGauge",
            "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color",
            "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color/Cast_0483",
            20,
            "live-bridge/ui-draw-list",
            "setter-node-address-join-pending",
        },
    };
}

std::vector<SonicDayHudRuntimeGaugeSetterChildPathJoin> makeRuntimePhase195GaugeSetterChildPathJoins()
{
    return {
        {
            "boostGauge",
            "0xEA09708",
            "scale",
            0.650,
            true,
            "ui_playscreen/so_speed_gauge/position/speed_gauge_color",
            "ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506",
            "runtime-draw-list-cast-node-match",
            "setter-node-address-join-runtime-proven",
        },
        {
            "ringEnergyGauge",
            "0xEA0A990",
            "scale",
            0.425,
            true,
            "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color",
            "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color/Cast_0483",
            "runtime-draw-list-cast-node-match",
            "setter-node-address-join-runtime-proven",
        },
    };
}

std::vector<SonicDayHudRuntimeRollingGaugeCounterObservation> makeRuntimePhase196RollingGaugeCounterObservations()
{
    return {
        {
            "boostGauge",
            "0x82914B0",
            "ui_playscreen/so_speed_gauge",
            "text",
            "530",
            "sub_824D6C18",
            2,
            "rolling-counter-text-candidate-pending-gauge-state-normalization",
            "phase196-live:sonic-hud-node-write-semantic-path-candidate:same-frame-hud-update-context:sub_824D6C18",
        },
        {
            "ringEnergyGauge",
            "0x82914B0",
            "ui_playscreen/so_ringenagy_gauge",
            "text",
            "530",
            "sub_824D6C18",
            1,
            "rolling-counter-text-candidate-pending-gauge-state-normalization",
            "phase196-live:sonic-hud-node-write-semantic-path-candidate:same-frame-hud-update-context:sub_824D6C18",
        },
    };
}

std::vector<SonicDayHudRuntimeOwnerFieldRollingCounterObservation> makeRuntimePhase197OwnerFieldRollingCounterObservations()
{
    return {
        {
            "boostGauge",
            "0xCE2D6B0",
            460,
            "ui_playscreen/so_speed_gauge",
            "text",
            "",
            "sub_824D6C18",
            2,
            "owner-field-rolling-counter-pending-exact-offset-normalization",
            "phase197-live:runtime-owner-field-snapshot:sub_824D6C18",
        },
        {
            "ringEnergyGauge",
            "0xCE2D6B0",
            460,
            "ui_playscreen/so_ringenagy_gauge",
            "text",
            "",
            "sub_824D6C18",
            2,
            "owner-field-rolling-counter-pending-exact-offset-normalization",
            "phase197-live:runtime-owner-field-snapshot:sub_824D6C18",
        },
        {
            "boostGauge",
            "0xCE2D6B0",
            480,
            "ui_playscreen/so_speed_gauge",
            "text",
            "",
            "sub_824D6C18",
            2,
            "owner-field-rolling-counter-pending-exact-offset-normalization",
            "phase197-live:runtime-owner-field-snapshot:sub_824D6C18",
        },
        {
            "ringEnergyGauge",
            "0xCE2D6B0",
            480,
            "ui_playscreen/so_ringenagy_gauge",
            "text",
            "",
            "sub_824D6C18",
            2,
            "owner-field-rolling-counter-pending-exact-offset-normalization",
            "phase197-live:runtime-owner-field-snapshot:sub_824D6C18",
        },
    };
}

std::vector<SonicDayHudRuntimeOwnerFieldGaugeScaleCorrelation> makeRuntimePhase198OwnerFieldGaugeScaleCorrelations()
{
    return {
        {
            "boostGauge",
            "0xCE2D6B0",
            460,
            "ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506",
            0.650,
            true,
            4,
            1,
            "owner-field-gauge-scale-correlation-pending-formula-proof",
            "phase198-live:runtime-csd-node-set-scale-owner-field-join:sub_830BF090",
        },
        {
            "ringEnergyGauge",
            "0xCE2D6B0",
            480,
            "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color/Cast_0483",
            0.425,
            true,
            684,
            1,
            "owner-field-gauge-scale-correlation-pending-formula-proof",
            "phase198-live:runtime-csd-node-set-scale-owner-field-join:sub_830BF090",
        },
    };
}

bool isSemanticBoundRuntimeObservation(const SonicDayHudRuntimeSemanticPathCandidateObservation& observation)
{
    return observation.bindingStatus == "semantic-bound-pending-exact-child-node-resolution";
}

SonicDayHudRuntimeCallsiteSample makeRuntimeTimerCallsiteSample()
{
    SonicDayHudRuntimeCallsiteSample sample;
    sample.ownerAddress = "0xCE2D6B0";
    sample.hookName = "sub_824D6048";
    sample.samplePhase = "post-original";
    sample.deltaTime = 0.066667;
    sample.r4 = "0x2F3BB30";
    sample.ownerField452 = 39;
    sample.ownerField456 = 11;
    return sample;
}

SonicDayHudRuntimeCallsiteSample makeRuntimeRollingCounterCallsiteSample()
{
    SonicDayHudRuntimeCallsiteSample sample;
    sample.ownerAddress = "0xCE2D6B0";
    sample.hookName = "sub_824D6C18";
    sample.samplePhase = "post-original";
    sample.deltaTime = 0.066667;
    sample.r4 = "0x2F3BB30";
    sample.ownerField460 = 4;
    sample.ownerField464 = 1;
    sample.ownerField468 = 2;
    sample.ownerField472 = 3;
    sample.ownerField480 = 1234;
    return sample;
}

FrontendControllerFrame makeFrame(
    std::string controllerName,
    std::string screenId,
    int frame,
    std::string stateName,
    std::string motionName,
    bool inputLocked,
    int cursorIndex,
    std::string cursorLabel,
    std::string sfxHook,
    std::string nextScreenId,
    std::vector<std::string> activeScenes)
{
    return {
        std::move(controllerName),
        std::move(screenId),
        frame,
        std::move(stateName),
        std::move(motionName),
        inputLocked,
        cursorIndex,
        std::move(cursorLabel),
        std::move(sfxHook),
        std::move(nextScreenId),
        std::move(activeScenes),
    };
}

std::string titleMenuRouteForCursor(std::string_view label)
{
    if (label == "NEW_FILE")
        return "loading";
    if (label == "CONTINUE")
        return "loading";
    if (label == "SETTINGS")
        return "title-options";
    if (label == "DLC")
        return "title-dlc-placeholder";
    if (label == "EXIT")
        return "exit-game";
    return "none";
}
} // namespace

TitleMenuController::TitleMenuController()
{
    reset();
}

void TitleMenuController::reset()
{
    frame_ = makeFrame(
        "TitleMenuController",
        "title-menu",
        0,
        "press-start-wait",
        "title_loop",
        true,
        1,
        "CONTINUE",
        "none",
        "none",
        firstSceneOnly("title-menu"));
}

FrontendControllerFrame TitleMenuController::handleInput(FrontendControllerInput input)
{
    static const std::vector<std::string> kItems{
        "NEW_FILE",
        "CONTINUE",
        "SETTINGS",
        "DLC",
        "EXIT",
    };

    if (input == FrontendControllerInput::PressStart)
    {
        frame_ = makeFrame(
            "TitleMenuController",
            "title-menu",
            20,
            "menu-ready",
            "select_travel",
            false,
            1,
            "CONTINUE",
            "title_press_start_accept_sfx",
            "none",
            sceneNamesForPolicy("title-menu"));
    }
    else if (input == FrontendControllerInput::MoveDown && !frame_.inputLocked)
    {
        frame_.frame += 1;
        frame_.motionName = "cursor_move";
        frame_.cursorIndex = std::min<int>(frame_.cursorIndex + 1, static_cast<int>(kItems.size()) - 1);
        frame_.cursorLabel = kItems[static_cast<std::size_t>(frame_.cursorIndex)];
        frame_.sfxHook = "title_cursor_move_sfx";
        frame_.nextScreenId = titleMenuRouteForCursor(frame_.cursorLabel);
    }
    else if (input == FrontendControllerInput::MoveUp && !frame_.inputLocked)
    {
        frame_.frame += 1;
        frame_.motionName = "cursor_move";
        frame_.cursorIndex = std::max(0, frame_.cursorIndex - 1);
        frame_.cursorLabel = kItems[static_cast<std::size_t>(frame_.cursorIndex)];
        frame_.sfxHook = "title_cursor_move_sfx";
        frame_.nextScreenId = titleMenuRouteForCursor(frame_.cursorLabel);
    }
    else if (input == FrontendControllerInput::Confirm && !frame_.inputLocked)
    {
        frame_.frame += 1;
        frame_.sfxHook = "title_press_start_accept_sfx";
        frame_.nextScreenId = titleMenuRouteForCursor(frame_.cursorLabel);
    }
    return frame_;
}

const FrontendControllerFrame& TitleMenuController::frame() const
{
    return frame_;
}

LoadingScreenController::LoadingScreenController()
{
    reset();
}

void LoadingScreenController::reset()
{
    frame_ = makeFrame(
        "LoadingScreenController",
        "loading",
        0,
        "route-pending",
        "pda_intro",
        true,
        0,
        "none",
        "none",
        "none",
        {});
}

FrontendControllerFrame LoadingScreenController::handleInput(FrontendControllerInput input)
{
    if (input == FrontendControllerInput::RouteReady)
    {
        frame_ = makeFrame(
            "LoadingScreenController",
            "loading",
            75,
            "loading-visible",
            "pda_intro",
            true,
            0,
            "none",
            "loading_display_open_sfx",
            std::string(kSonicDayHudNext),
            sceneNamesForPolicy("loading"));
    }
    else if (input == FrontendControllerInput::LoadingComplete)
    {
        frame_.frame = 255;
        frame_.stateName = "loading-complete";
        frame_.motionName = "pda_outro";
        frame_.sfxHook = "loading_display_close_sfx";
        frame_.nextScreenId = std::string(kSonicDayHudNext);
    }
    return frame_;
}

const FrontendControllerFrame& LoadingScreenController::frame() const
{
    return frame_;
}

OptionsMenuController::OptionsMenuController()
{
    reset();
}

void OptionsMenuController::reset()
{
    frame_ = makeFrame(
        "OptionsMenuController",
        "title-options",
        15,
        "options-ready",
        "select_travel",
        false,
        0,
        "SOUND",
        "none",
        "none",
        sceneNamesForPolicy("title-options"));
}

FrontendControllerFrame OptionsMenuController::handleInput(FrontendControllerInput input)
{
    if (input == FrontendControllerInput::MoveDown)
    {
        frame_.frame += 1;
        frame_.motionName = "option_cursor_move";
        frame_.cursorIndex = 1;
        frame_.cursorLabel = "CONTROLS";
        frame_.sfxHook = "title_cursor_move_sfx";
    }
    else if (input == FrontendControllerInput::Cancel)
    {
        frame_.frame = 15;
        frame_.stateName = "options-ready";
        frame_.motionName = "select_travel";
        frame_.cursorIndex = 0;
        frame_.cursorLabel = "SOUND";
        frame_.sfxHook = "title_cursor_move_sfx";
        frame_.nextScreenId = "title-menu";
    }
    return frame_;
}

const FrontendControllerFrame& OptionsMenuController::frame() const
{
    return frame_;
}

PauseMenuController::PauseMenuController()
{
    reset();
}

void PauseMenuController::reset()
{
    frame_ = makeFrame(
        "PauseMenuController",
        "pause",
        0,
        "pause-intro",
        "intro_medium",
        true,
        0,
        "RESUME",
        "none",
        "none",
        firstSceneOnly("pause"));
}

FrontendControllerFrame PauseMenuController::handleInput(FrontendControllerInput input)
{
    if (input == FrontendControllerInput::RouteReady || input == FrontendControllerInput::Pause)
    {
        frame_ = makeFrame(
            "PauseMenuController",
            "pause",
            15,
            "pause-ready",
            "intro_medium",
            false,
            0,
            "RESUME",
            "pause_display_open_sfx",
            std::string(kSonicDayHudNext),
            sceneNamesForPolicy("pause"));
    }
    else if (input == FrontendControllerInput::MoveDown && !frame_.inputLocked)
    {
        frame_.frame += 1;
        frame_.motionName = "pause_cursor_move";
        frame_.cursorIndex = 1;
        frame_.cursorLabel = "OPTIONS";
        frame_.sfxHook = "pause_cursor_move_sfx";
        frame_.nextScreenId = "pause-options";
    }
    return frame_;
}

const FrontendControllerFrame& PauseMenuController::frame() const
{
    return frame_;
}

SonicDayHudController::SonicDayHudController()
{
    reset();
}

// Sonic HUD ownership provenance: CHudSonicStage / sub_824D9308 / ui_playscreen,
// readiness events sonic-hud-ready, stage-hud-ready, and tutorial-hud-owner-path-ready.
// Key recovered scenes include so_speed_gauge, so_ringenagy_gauge, and ring_get.
void SonicDayHudController::reset()
{
    gameplayState_ = {};
    gameplayState_.provenance = kSonicDayHudValueProvenance;
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        0,
        "hud-bootstrap",
        "owner-wait",
        true,
        0,
        "none",
        "none",
        "none",
        {});
}

FrontendControllerFrame SonicDayHudController::handleInput(FrontendControllerInput input)
{
    if (input == FrontendControllerInput::StageReady)
    {
        frame_ = makeFrame(
            "SonicDayHudController",
            "sonic-day-hud",
            99,
            "hud-ready",
            "DefaultAnim",
            false,
            0,
            "none",
            "none",
            "none",
            sonicHudSceneNames(false));
        gameplayState_.routeEvent = "stage-hud-ready";
        gameplayState_.lastSfxHook = "none";
        gameplayState_.sfxCueId = "audio-id-pending";
    }
    else if (input == FrontendControllerInput::TutorialReady)
    {
        return openTutorialPrompt("boost_prompt");
    }
    else if (input == FrontendControllerInput::RingPickup)
    {
        return applyRingPickup(1, 100);
    }
    return frame_;
}

const FrontendControllerFrame& SonicDayHudController::frame() const
{
    return frame_;
}

const SonicDayHudGameplayState& SonicDayHudController::gameplayState() const
{
    return gameplayState_;
}

FrontendControllerFrame SonicDayHudController::setGameplayState(const SonicDayHudGameplayState& state)
{
    gameplayState_ = state;
    gameplayState_.provenance = kSonicDayHudValueProvenance;
    gameplayState_.lastSfxHook = "none";
    gameplayState_.sfxCueId = "audio-id-pending";
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        100,
        "hud-value-tick",
        "DefaultAnim",
        false,
        0,
        "none",
        "none",
        "none",
        sonicHudSceneNames(gameplayState_.tutorialVisible));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::applyRuntimeBinding(const SonicDayHudRuntimeBindingSnapshot& snapshot)
{
    SonicDayHudGameplayState values = gameplayState_;

    if (snapshot.ringCountBinding.known)
        values.ringCount = snapshot.values.ringCount;
    if (snapshot.scoreBinding.known)
        values.score = snapshot.values.score;
    if (snapshot.elapsedFramesBinding.known)
        values.elapsedFrames = snapshot.values.elapsedFrames;
    if (snapshot.speedKmhBinding.known)
        values.speedKmh = snapshot.values.speedKmh;
    if (snapshot.boostGaugeBinding.known)
        values.boostGauge = snapshot.values.boostGauge;
    if (snapshot.ringEnergyGaugeBinding.known)
        values.ringEnergyGauge = snapshot.values.ringEnergyGauge;
    if (snapshot.lifeCountBinding.known)
        values.lifeCount = snapshot.values.lifeCount;
    if (snapshot.tutorialPromptBinding.known)
    {
        values.tutorialPromptId = snapshot.values.tutorialPromptId;
        values.tutorialVisible = snapshot.values.tutorialVisible;
    }

    values.provenance = kSonicDayHudValueProvenance;
    values.provenance.valueSource = snapshot.source;
    gameplayState_ = values;
    gameplayState_.lastSfxHook = "none";
    gameplayState_.sfxCueId = snapshot.sonicRingPickupSfxId;
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        101,
        "hud-runtime-bound",
        "DefaultAnim",
        false,
        0,
        "none",
        "none",
        "none",
        sonicHudSceneNames(gameplayState_.tutorialVisible));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::applyRuntimeTextWrite(const SonicDayHudRuntimeTextWriteObservation& observation)
{
    int parsedValue = 0;
    if (!parseUnsignedRuntimeText(observation.textUtf8, parsedValue))
        return frame_;

    const std::string source =
        observation.source + "@" + observation.path +
        ":pathResolutionSource=" + observation.pathResolutionSource;

    if (observation.path == "ui_playscreen/ring_count/num_ring")
    {
        gameplayState_.ringCount = parsedValue;
        gameplayState_.provenance.ringCount = observation.path;
    }
    else if (
        observation.path == "ui_playscreen/time_count/time001" ||
        observation.path == "ui_playscreen/time_count/time010" ||
        observation.path == "ui_playscreen/time_count/time100")
    {
        gameplayState_.elapsedFrames = parsedValue;
        gameplayState_.provenance.elapsedFrames = observation.path;
    }
    else if (observation.path == "ui_playscreen/add/speed_count/position/num_speed")
    {
        gameplayState_.speedKmh = parsedValue;
        gameplayState_.provenance.speedKmh = observation.path;
    }
    else if (observation.path == "ui_playscreen/player_count/player")
    {
        gameplayState_.lifeCount = parsedValue;
        gameplayState_.provenance.lifeCount = observation.path;
    }

    gameplayState_.provenance.valueSource = source;
    gameplayState_.lastSfxHook = "none";
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        frame_.frame + 1,
        "runtime-text-write",
        "sonic-hud-value-text-write",
        false,
        0,
        "none",
        "none",
        "none",
        sonicHudSceneNames(gameplayState_.tutorialVisible));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::applyRuntimeGaugePromptWrite(
    const SonicDayHudRuntimeGaugePromptWriteObservation& observation)
{
    if (!observation.numericValueKnown)
        return frame_;

    const std::string source =
        observation.source + "@" + observation.path +
        ":writeKind=" + observation.writeKind +
        ":pathResolutionSource=" + observation.pathResolutionSource +
        ":status=runtime-proven-via-csd-gauge-prompt-write";

    const bool boostPath =
        observation.valueName == "boostGauge" ||
        observation.path.find("ui_playscreen/so_speed_gauge") != std::string::npos ||
        observation.path.find("ui_playscreen/gauge_frame") != std::string::npos;
    const bool energyPath =
        observation.valueName == "ringEnergyGauge" ||
        observation.path.find("ui_playscreen/so_ringenagy_gauge") != std::string::npos;
    const bool tutorialPath =
        observation.valueName == "tutorialPrompt" ||
        observation.path.find("ui_playscreen/add/u_info") != std::string::npos;

    if (observation.writeKind == "scale" && boostPath)
    {
        gameplayState_.boostGauge = std::clamp(observation.numericValue, 0.0, 1.0);
        gameplayState_.provenance.boostGauge = observation.path;
    }
    else if (observation.writeKind == "scale" && energyPath)
    {
        gameplayState_.ringEnergyGauge = std::clamp(observation.numericValue, 0.0, 1.0);
        gameplayState_.provenance.ringEnergyGauge = observation.path;
    }
    else if (tutorialPath)
    {
        if (observation.writeKind == "pattern-index")
        {
            const int promptIndex = std::max(0, static_cast<int>(observation.numericValue));
            gameplayState_.tutorialPromptId = "pattern-" + std::to_string(promptIndex);
            gameplayState_.tutorialVisible = true;
        }
        else if (observation.writeKind == "hide-flag")
        {
            gameplayState_.tutorialVisible = observation.numericValue == 0.0;
        }
        gameplayState_.provenance.tutorialPrompt = observation.path;
    }

    gameplayState_.provenance.valueSource = source;
    gameplayState_.lastSfxHook = "none";
    gameplayState_.sfxCueId = "audio-id-pending";
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        frame_.frame + 1,
        "runtime-gauge-prompt-write",
        observation.source.empty() ? "sonic-hud-gauge-prompt-write" : observation.source,
        false,
        0,
        "none",
        "none",
        "none",
        sonicHudSceneNames(gameplayState_.tutorialVisible));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::applyRuntimeSemanticPathCandidate(
    const SonicDayHudRuntimeSemanticPathCandidateObservation& observation)
{
    const std::string source =
        observation.source + "@" + observation.path +
        ":writeKind=" + observation.writeKind +
        ":pathResolutionSource=" + observation.pathResolutionSource +
        ":status=" + observation.bindingStatus;

    bool applied = false;
    if (observation.writeKind == "text")
    {
        int parsedValue = 0;
        if (!parseUnsignedRuntimeText(observation.textUtf8, parsedValue))
            return frame_;

        if (
            observation.valueName == "speedKmh" &&
            observation.path == "ui_playscreen/add/speed_count/position/num_speed")
        {
            gameplayState_.speedKmh = parsedValue;
            gameplayState_.provenance.speedKmh = observation.path;
            applied = true;
        }
        else if (
            observation.valueName == "elapsedFrames" &&
            (observation.path == "ui_playscreen/time_count/time001" ||
                observation.path == "ui_playscreen/time_count/time010" ||
                observation.path == "ui_playscreen/time_count/time100"))
        {
            gameplayState_.elapsedFrames = parsedValue;
            gameplayState_.provenance.elapsedFrames = observation.path;
            applied = true;
        }
    }
    else if (observation.numericValueKnown)
    {
        if (
            observation.writeKind == "scale" &&
            observation.valueName == "boostGauge" &&
            observation.path == "ui_playscreen/so_speed_gauge")
        {
            gameplayState_.boostGauge = std::clamp(observation.numericValue, 0.0, 1.0);
            gameplayState_.provenance.boostGauge = observation.path;
            applied = true;
        }
        else if (
            observation.writeKind == "scale" &&
            observation.valueName == "ringEnergyGauge" &&
            observation.path == "ui_playscreen/so_ringenagy_gauge")
        {
            gameplayState_.ringEnergyGauge = std::clamp(observation.numericValue, 0.0, 1.0);
            gameplayState_.provenance.ringEnergyGauge = observation.path;
            applied = true;
        }
        else if (
            observation.valueName == "tutorialPrompt" &&
            observation.path == "ui_playscreen/add/u_info")
        {
            if (observation.writeKind == "pattern-index")
            {
                const int promptIndex = std::max(0, static_cast<int>(observation.numericValue));
                gameplayState_.tutorialPromptId = "pattern-" + std::to_string(promptIndex);
                gameplayState_.tutorialVisible = true;
                applied = true;
            }
            else if (observation.writeKind == "hide-flag")
            {
                gameplayState_.tutorialVisible = observation.numericValue == 0.0;
                applied = true;
            }

            if (applied)
                gameplayState_.provenance.tutorialPrompt = observation.path;
        }
    }

    if (!applied)
        return frame_;

    gameplayState_.provenance.valueSource = source;
    gameplayState_.lastSfxHook = "none";
    gameplayState_.sfxCueId = "audio-id-pending";
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        frame_.frame + 1,
        "runtime-semantic-candidate",
        observation.source.empty() ? "sonic-hud-node-write-semantic-bound" : observation.source,
        false,
        0,
        "none",
        "none",
        "none",
        sonicHudSceneNames(gameplayState_.tutorialVisible));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::applyRuntimeGaugeSetterChildPathJoin(
    const SonicDayHudRuntimeGaugeSetterChildPathJoin& observation)
{
    if (!observation.numericValueKnown || observation.bindingStatus != "setter-node-address-join-runtime-proven")
        return frame_;

    bool applied = false;
    if (
        observation.writeKind == "scale" &&
        observation.valueName == "boostGauge" &&
        observation.exactParentPath == "ui_playscreen/so_speed_gauge/position/speed_gauge_color")
    {
        gameplayState_.boostGauge = std::clamp(observation.numericValue, 0.0, 1.0);
        gameplayState_.provenance.boostGauge = observation.exactChildPath;
        applied = true;
    }
    else if (
        observation.writeKind == "scale" &&
        observation.valueName == "ringEnergyGauge" &&
        observation.exactParentPath == "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color")
    {
        gameplayState_.ringEnergyGauge = std::clamp(observation.numericValue, 0.0, 1.0);
        gameplayState_.provenance.ringEnergyGauge = observation.exactChildPath;
        applied = true;
    }

    if (!applied)
        return frame_;

    gameplayState_.provenance.valueSource =
        observation.addressJoinSource + ":" + observation.nodeAddress +
        "@" + observation.exactChildPath +
        ":status=" + observation.bindingStatus;
    gameplayState_.lastSfxHook = "none";
    gameplayState_.sfxCueId = "audio-id-pending";
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        frame_.frame + 1,
        "runtime-gauge-setter-child-join",
        observation.addressJoinSource,
        false,
        0,
        "none",
        "none",
        "none",
        sonicHudSceneNames(gameplayState_.tutorialVisible));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::applyRuntimeRollingGaugeCounterObservation(
    const SonicDayHudRuntimeRollingGaugeCounterObservation& observation)
{
    if (observation.callsite != "sub_824D6C18" || observation.writeKind != "text")
        return frame_;

    if (observation.valueName != "boostGauge" && observation.valueName != "ringEnergyGauge")
        return frame_;

    gameplayState_.provenance.valueSource =
        observation.source + ":" + observation.nodeAddress +
        "@" + observation.path +
        ":status=" + observation.bindingStatus;
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        frame_.frame + 1,
        "runtime-rolling-counter-candidate",
        observation.source.empty() ? "sonic-hud-node-write-semantic-path-candidate" : observation.source,
        false,
        0,
        "none",
        "none",
        "none",
        sonicHudSceneNames(gameplayState_.tutorialVisible));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::applyRuntimeOwnerFieldRollingCounterObservation(
    const SonicDayHudRuntimeOwnerFieldRollingCounterObservation& observation)
{
    if (observation.callsite != "sub_824D6C18" || observation.writeKind != "text")
        return frame_;

    if (observation.valueName != "boostGauge" && observation.valueName != "ringEnergyGauge")
        return frame_;

    if (observation.fieldOffset <= 0)
        return frame_;

    gameplayState_.provenance.valueSource =
        observation.source + ":owner=" + observation.ownerAddress +
        ":field+" + std::to_string(observation.fieldOffset) +
        "@" + observation.path +
        ":status=" + observation.bindingStatus;
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        frame_.frame + 1,
        "runtime-owner-field-rolling-counter-candidate",
        observation.source.empty() ? "runtime-owner-field-snapshot:sub_824D6C18" : observation.source,
        false,
        0,
        "none",
        "none",
        "none",
        sonicHudSceneNames(gameplayState_.tutorialVisible));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::applyRuntimeOwnerFieldGaugeScaleCorrelation(
    const SonicDayHudRuntimeOwnerFieldGaugeScaleCorrelation& correlation)
{
    if (correlation.valueName != "boostGauge" && correlation.valueName != "ringEnergyGauge")
        return frame_;

    if (correlation.fieldOffset <= 0)
        return frame_;

    if (correlation.exactChildPath.empty())
        return frame_;

    std::ostringstream provenance;
    provenance << correlation.source
               << ":owner=" << correlation.ownerAddress
               << ":field+" << correlation.fieldOffset
               << "@" << correlation.exactChildPath
               << ":scale=" << std::fixed << std::setprecision(3)
               << std::clamp(correlation.scaleValue, 0.0, 1.0)
               << ":owner_field_value=" << correlation.ownerFieldValue
               << ":status=" << correlation.bindingStatus;
    gameplayState_.provenance.valueSource = provenance.str();

    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        frame_.frame + 1,
        "runtime-owner-field-gauge-scale-correlation-candidate",
        correlation.source.empty()
            ? "runtime-csd-node-set-scale-owner-field-join:sub_830BF090"
            : correlation.source,
        false,
        0,
        "none",
        "none",
        "none",
        sonicHudSceneNames(gameplayState_.tutorialVisible));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::applyRuntimeCallsiteSample(
    const SonicDayHudRuntimeCallsiteSample& sample)
{
    const SonicDayHudRuntimeCallsiteClassification classification =
        classifySonicDayHudRuntimeCallsiteSample(sample);

    if (classification.normalizedValueKnown && classification.valueName == "elapsedFrames")
    {
        gameplayState_.elapsedFrames = classification.normalizedValue;
        gameplayState_.provenance.elapsedFrames = classification.path;
        gameplayState_.provenance.valueSource = classification.source;
    }

    gameplayState_.lastSfxHook = "none";
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        frame_.frame + 1,
        "runtime-callsite-sample",
        sample.hookName.empty() ? "sonic-hud-update-callsite-sample" : sample.hookName,
        false,
        0,
        "none",
        "none",
        "none",
        sonicHudSceneNames(gameplayState_.tutorialVisible));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::applyRingPickup(int ringDelta, int scoreDelta)
{
    gameplayState_.ringCount = std::max(0, gameplayState_.ringCount + ringDelta);
    gameplayState_.score = std::max(0, gameplayState_.score + scoreDelta);
    gameplayState_.lastSfxHook = "sonic_ring_pickup_sfx";
    gameplayState_.sfxCueId = "audio-id-pending";
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        60,
        "ring-feedback",
        "Egg_Shackle",
        false,
        0,
        "none",
        gameplayState_.lastSfxHook,
        "none",
        sonicHudSceneName("ring_get"));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::openTutorialPrompt(std::string_view promptId)
{
    gameplayState_.tutorialPromptId = promptId.empty() ? "unknown_prompt" : std::string(promptId);
    gameplayState_.tutorialVisible = true;
    gameplayState_.routeEvent = "tutorial-hud-owner-path-ready";
    gameplayState_.lastSfxHook = "tutorial_prompt_open_sfx";
    gameplayState_.sfxCueId = "audio-id-pending";
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        20,
        "tutorial-ready",
        "Intro_Anim",
        false,
        0,
        "none",
        gameplayState_.lastSfxHook,
        "none",
        sonicHudSceneNames(true));
    return frame_;
}

FrontendControllerFrame SonicDayHudController::dismissTutorialPrompt()
{
    gameplayState_.tutorialVisible = false;
    gameplayState_.routeEvent = "stage-hud-ready";
    gameplayState_.lastSfxHook = "tutorial_prompt_close_sfx";
    gameplayState_.sfxCueId = "audio-id-pending";
    frame_ = makeFrame(
        "SonicDayHudController",
        "sonic-day-hud",
        40,
        "tutorial-dismiss",
        "Outro_Anim",
        false,
        0,
        "none",
        gameplayState_.lastSfxHook,
        "none",
        sonicHudSceneNames(false));
    return frame_;
}

std::vector<FrontendControllerFrame> runFrontendControllerSmokeSequence()
{
    std::vector<FrontendControllerFrame> frames;

    TitleMenuController title;
    frames.push_back(title.frame());
    frames.push_back(title.handleInput(FrontendControllerInput::PressStart));
    frames.push_back(title.handleInput(FrontendControllerInput::MoveDown));

    LoadingScreenController loading;
    frames.push_back(loading.handleInput(FrontendControllerInput::RouteReady));

    OptionsMenuController options;
    frames.push_back(options.handleInput(FrontendControllerInput::Cancel));

    PauseMenuController pause;
    frames.push_back(pause.handleInput(FrontendControllerInput::RouteReady));

    return frames;
}

std::vector<FrontendControllerFrame> runSonicDayHudControllerSmokeSequence()
{
    SonicDayHudController hud;
    std::vector<FrontendControllerFrame> frames;
    frames.push_back(hud.frame());
    frames.push_back(hud.handleInput(FrontendControllerInput::StageReady));
    frames.push_back(hud.handleInput(FrontendControllerInput::TutorialReady));
    frames.push_back(hud.handleInput(FrontendControllerInput::RingPickup));
    return frames;
}

const SonicDayHudValueProvenance& sonicDayHudValueProvenance()
{
    return kSonicDayHudValueProvenance;
}

std::string formatFrontendControllerCatalog()
{
    std::ostringstream out;
    out << "controller_catalog=frontend-native-screen-controllers"
        << ":count=4"
        << ":policy_source=frontend_screen_reference"
        << ":sonic_day_hud_next=1"
        << '\n';
    out << "controller=TitleMenuController:screen=title-menu:items=NEW_FILE,CONTINUE,SETTINGS,DLC,EXIT"
        << ":state_machine=press-start-wait->menu-ready->route-selection"
        << '\n';
    out << "controller=LoadingScreenController:screen=loading"
        << ":state_machine=route-pending->loading-visible->loading-complete"
        << '\n';
    out << "controller=OptionsMenuController:screen=title-options"
        << ":state_machine=options-ready->option-cursor->title-menu"
        << '\n';
    out << "controller=PauseMenuController:screen=pause"
        << ":state_machine=pause-intro->pause-ready->sonic-day-hud-next"
        << '\n';
    return out.str();
}

std::string formatFrontendControllerFrame(const FrontendControllerFrame& frame)
{
    std::ostringstream out;
    out << "controller_frame=" << frame.controllerName
        << ":screen=" << frame.screenId
        << ":frame=" << frame.frame
        << ":state=" << frame.stateName
        << ":motion=" << frame.motionName
        << ":input_locked=" << (frame.inputLocked ? 1 : 0)
        << ":cursor=" << frame.cursorIndex << '/' << frame.cursorLabel
        << ":sfx=" << frame.sfxHook
        << ":next=" << frame.nextScreenId
        << ":scenes=" << joinStrings(frame.activeScenes, ",")
        << '\n';
    return out.str();
}

std::string formatFrontendControllerSmokeSequence()
{
    std::ostringstream out;
    out << formatFrontendControllerCatalog();
    for (const auto& frame : runFrontendControllerSmokeSequence())
        out << formatFrontendControllerFrame(frame);
    return out.str();
}

std::string formatSonicDayHudControllerSmokeSequence()
{
    const auto& owner = sonicHudOwnerReference();
    std::ostringstream out;
    out << "sonic_day_hud_controller=owner=" << owner.ownerType
        << ":hook=" << owner.ownerHook
        << ":project=" << owner.projectName
        << ":scenes=" << owner.sceneCount
        << ":runtime_layers=" << owner.runtimeLayerCount
        << ":drawable_layers=" << owner.drawableLayerCount
        << ":policy_source=sonic_hud_reference"
        << '\n';
    for (const auto& frame : runSonicDayHudControllerSmokeSequence())
        out << formatFrontendControllerFrame(frame);
    return out.str();
}

std::string formatSonicDayHudGameplayState(std::string_view phase, const SonicDayHudGameplayState& state)
{
    const auto& provenance = state.provenance;
    std::ostringstream out;
    out << "sonic_day_hud_state=phase=" << phase
        << ":rings=" << formatPaddedInt(state.ringCount, 3)
        << ":score=" << formatPaddedInt(state.score, 9)
        << ":time=" << formatElapsedFrames(state.elapsedFrames)
        << ":speed=" << formatPaddedInt(state.speedKmh, 3)
        << ":boost=" << formatGauge(state.boostGauge)
        << ":energy=" << formatGauge(state.ringEnergyGauge)
        << ":lives=" << state.lifeCount
        << ":tutorial=" << state.tutorialPromptId << ':' << (state.tutorialVisible ? "visible" : "hidden")
        << ":route=" << state.routeEvent
        << ":sfx=" << state.lastSfxHook
        << ":sfx_id=" << state.sfxCueId
        << ":provenance=ring:" << provenance.ringCount
        << ",score:" << provenance.score
        << ",time:" << provenance.elapsedFrames
        << ",speed:" << provenance.speedKmh
        << ",boost:" << provenance.boostGauge
        << ",energy:" << provenance.ringEnergyGauge
        << ",lives:" << provenance.lifeCount
        << ",tutorial:" << provenance.tutorialPrompt
        << ",route:" << provenance.routeEvent
        << ",value_source:" << provenance.valueSource
        << '\n';
    return out.str();
}

std::string formatSonicDayHudGameplayStateModel()
{
    std::ostringstream out;
    out << "sonic_day_hud_state_model=fields=ring,score,time,speed,boost,energy,lives,tutorial,route"
        << ":layout=" << sonicHudOwnerReference().projectName
        << ":controller=SonicDayHudController"
        << ":value_source=" << sonicDayHudValueProvenance().valueSource
        << ":memory_binding=live-bridge-value-port-pending"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudGameplayStateSmokeSequence()
{
    SonicDayHudController hud;
    std::ostringstream out;
    out << formatSonicDayHudGameplayStateModel();

    (void)hud.handleInput(FrontendControllerInput::StageReady);
    out << formatSonicDayHudGameplayState("stage-ready", hud.gameplayState());

    SonicDayHudGameplayState values = hud.gameplayState();
    values.score = 1250;
    values.elapsedFrames = 320;
    values.speedKmh = 186;
    values.boostGauge = 0.65;
    values.ringEnergyGauge = 0.72;
    (void)hud.setGameplayState(values);
    out << formatSonicDayHudGameplayState("value-tick", hud.gameplayState());

    (void)hud.applyRingPickup(1, 100);
    out << formatSonicDayHudGameplayState("ring-pickup", hud.gameplayState());

    (void)hud.openTutorialPrompt("boost_prompt");
    out << formatSonicDayHudGameplayState("tutorial-open", hud.gameplayState());

    (void)hud.dismissTutorialPrompt();
    out << formatSonicDayHudGameplayState("tutorial-dismiss", hud.gameplayState());

    return out.str();
}

std::string formatSonicDayHudRuntimeBinding(const SonicDayHudRuntimeBindingSnapshot& snapshot)
{
    std::ostringstream out;
    out << "sonic_day_hud_runtime_binding=source=" << snapshot.source
        << ":score=" << formatRuntimeBindingStatus(snapshot.scoreBinding)
        << ":ring=" << formatRuntimeBindingStatus(snapshot.ringCountBinding)
        << ":timer=" << formatRuntimeBindingStatus(snapshot.elapsedFramesBinding)
        << ":speed=" << formatRuntimeBindingStatus(snapshot.speedKmhBinding)
        << ":boost=" << formatRuntimeBindingStatus(snapshot.boostGaugeBinding)
        << ":energy=" << formatRuntimeBindingStatus(snapshot.ringEnergyGaugeBinding)
        << ":lives=" << formatRuntimeBindingStatus(snapshot.lifeCountBinding)
        << ":tutorial=" << formatRuntimeBindingStatus(snapshot.tutorialPromptBinding)
        << ":sfx=sonic_ring_pickup:" << snapshot.sonicRingPickupSfxId
        << ",tutorial_prompt_open:" << snapshot.tutorialPromptOpenSfxId
        << ",pause_open:" << snapshot.pauseOpenSfxId
        << ",pause_cursor:" << snapshot.pauseCursorSfxId
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeDisplayOwnerPaths(const SonicDayHudDisplayOwnerPathBinding& paths)
{
    std::ostringstream out;
    out << "sonic_day_hud_display_owner_paths="
        << "ring=" << paths.ringCount
        << ":score=" << paths.score
        << ":timer=" << paths.elapsedFrames
        << ":speed=" << paths.speedKmh
        << ":boost=" << paths.boostGauge
        << ":energy=" << paths.ringEnergyGauge
        << ":lives=" << paths.lifeCount
        << ":tutorial=" << paths.tutorialPrompt
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeWritePaths(const SonicDayHudRuntimeValueUpdatePath& paths)
{
    std::ostringstream out;
    out << "sonic_day_hud_runtime_write_paths="
        << "ring=" << formatRuntimeBindingStatus(paths.ringCountWritePath)
        << ":timer=" << formatRuntimeBindingStatus(paths.elapsedFramesWritePath)
        << ":speed=" << formatRuntimeBindingStatus(paths.speedReadoutWritePath)
        << ":lives=" << formatRuntimeBindingStatus(paths.lifeCountWritePath)
        << ":boost=" << formatRuntimeBindingStatus(paths.boostGaugeWritePath)
        << ":energy=" << formatRuntimeBindingStatus(paths.ringEnergyGaugeWritePath)
        << ":tutorial=" << formatRuntimeBindingStatus(paths.tutorialPromptWritePath)
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeTextWriteObservation(const SonicDayHudRuntimeTextWriteObservation& observation)
{
    std::ostringstream out;
    out << "sonic_day_hud_runtime_text_write="
        << "value=" << observation.valueName
        << ":path=" << observation.path
        << ":text=" << observation.textUtf8
        << ":resolution=" << observation.pathResolutionSource
        << ":source=" << observation.source
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeGaugePromptWriteObservation(
    const SonicDayHudRuntimeGaugePromptWriteObservation& observation)
{
    std::ostringstream out;
    out << "sonic_day_hud_runtime_gauge_prompt_write="
        << "value=" << observation.valueName
        << ":path=" << observation.path
        << ":kind=" << observation.writeKind
        << ":value=" << std::fixed << std::setprecision(3) << observation.numericValue
        << ":resolution=" << observation.pathResolutionSource
        << ":source=" << observation.source
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeSemanticPathCandidateObservation(
    const SonicDayHudRuntimeSemanticPathCandidateObservation& observation)
{
    std::ostringstream out;
    out << "sonic_day_hud_runtime_semantic_path_candidate="
        << "value=" << observation.valueName
        << ":path=" << observation.path
        << ":kind=" << observation.writeKind;
    if (observation.writeKind == "text")
        out << ":text=" << observation.textUtf8;
    else
        out << ":value=" << std::fixed << std::setprecision(3) << observation.numericValue;
    out << ":candidate_writes=" << observation.candidateWriteCount
        << ":resolution=" << observation.pathResolutionSource
        << ":binding_status=" << observation.bindingStatus
        << ":source=" << observation.source
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeGaugeChildPathResolution(
    const SonicDayHudRuntimeGaugeChildPathResolution& resolution)
{
    std::ostringstream out;
    out << "sonic_day_hud_gauge_child_path="
        << "value=" << resolution.valueName
        << ":exact_parent=" << resolution.exactParentPath
        << ":representative_child=" << resolution.representativeChildPath
        << ":draw_layers=" << resolution.runtimeDrawLayerCount
        << ":resolution=" << resolution.pathResolutionSource
        << ":node_join=" << resolution.setterNodeJoinStatus
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeGaugeSetterChildPathJoin(
    const SonicDayHudRuntimeGaugeSetterChildPathJoin& observation)
{
    std::ostringstream out;
    out << "sonic_day_hud_gauge_setter_child_join="
        << "value=" << observation.valueName
        << ":node=" << observation.nodeAddress
        << ":kind=" << observation.writeKind
        << ":value=" << std::fixed << std::setprecision(3) << observation.numericValue
        << ":exact_parent=" << observation.exactParentPath
        << ":exact_child=" << observation.exactChildPath
        << ":join=" << observation.addressJoinSource
        << ":binding_status=" << observation.bindingStatus
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeRollingGaugeCounterObservation(
    const SonicDayHudRuntimeRollingGaugeCounterObservation& observation)
{
    std::ostringstream out;
    out << "sonic_day_hud_rolling_gauge_counter="
        << "value=" << observation.valueName
        << ":node=" << observation.nodeAddress
        << ":path=" << observation.path
        << ":kind=" << observation.writeKind
        << ":text=" << observation.textUtf8
        << ":callsite=" << observation.callsite
        << ":counter_writes=" << observation.counterWriteCount
        << ":status=" << observation.bindingStatus
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeOwnerFieldRollingCounterObservation(
    const SonicDayHudRuntimeOwnerFieldRollingCounterObservation& observation)
{
    std::ostringstream out;
    out << "sonic_day_hud_owner_field_rolling_counter="
        << "value=" << observation.valueName
        << ":owner=" << observation.ownerAddress
        << ":field_offset=" << observation.fieldOffset
        << ":path=" << observation.path
        << ":kind=" << observation.writeKind
        << ":text=" << observation.textUtf8
        << ":callsite=" << observation.callsite
        << ":counter_writes=" << observation.counterWriteCount
        << ":status=" << observation.bindingStatus
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeOwnerFieldGaugeScaleCorrelation(
    const SonicDayHudRuntimeOwnerFieldGaugeScaleCorrelation& correlation)
{
    std::ostringstream out;
    out << "sonic_day_hud_owner_field_gauge_scale_correlation="
        << "value=" << correlation.valueName
        << ":owner=" << correlation.ownerAddress
        << ":field_offset=" << correlation.fieldOffset
        << ":exact_child=" << correlation.exactChildPath
        << ":scale=" << std::fixed << std::setprecision(3)
        << std::clamp(correlation.scaleValue, 0.0, 1.0)
        << ":owner_field_value=" << correlation.ownerFieldValue
        << ":joins=" << correlation.joinCount
        << ":status=" << correlation.bindingStatus
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeDrawListCoverage(const SonicDayHudRuntimeDrawListCoverage& coverage)
{
    std::ostringstream out;
    out << "sonic_day_hud_runtime_draw_list_coverage="
        << "source=" << coverage.source
        << ":project=" << coverage.activeProject
        << ":runtime_calls=" << coverage.runtimeDrawCalls
        << ":correlated_pairs=" << coverage.correlatedMaterialPairs
        << ":speed_gauge=" << (coverage.speedGaugeObserved ? "observed" : "missing")
        << ":gauge_frame=" << (coverage.gaugeFrameObserved ? "observed" : "missing")
        << ":energy_gauge=" << (coverage.ringEnergyGaugeObserved ? "observed" : "missing")
        << ":tutorial_prompt=" << (coverage.tutorialPromptObserved ? "observed" : "pending")
        << ":pause_overlay=" << (coverage.pauseOverlayObserved ? "observed" : "missing")
        << ":text_write_observed=" << (coverage.textWriteObserved ? 1 : 0)
        << ":next_hook=" << coverage.nextHook
        << '\n';
    return out.str();
}

SonicDayHudRuntimeCallsiteClassification classifySonicDayHudRuntimeCallsiteSample(
    const SonicDayHudRuntimeCallsiteSample& sample)
{
    SonicDayHudRuntimeCallsiteClassification classification;

    if (sample.hookName == "sub_824D6048" && sample.samplePhase == "post-original")
    {
        classification.valueName = "elapsedFrames";
        classification.status = "runtime-proven-via-chud-update-callsite-sample";
        classification.source = "generated-PPC:sub_824D6048 owner+456/+452 -> CSD::CNode::SetText";
        classification.path = "CHudSonicStage.owner+456/+452|ui_playscreen/time_count";
        classification.normalizedValueKnown = true;
        classification.normalizedValue =
            std::max(0, sample.ownerField456) * 60 + std::clamp(sample.ownerField452, 0, 59);
    }
    else if (sample.hookName == "sub_824D6418")
    {
        classification.valueName = "speedKmh";
        classification.status = "classified-via-generated-PPC-callsite-candidate";
        classification.source = "generated-PPC:sub_824D6418 speed readout via sub_8251A568";
        classification.path = "CHudSonicStage.m_rcSpeedGauge|ui_playscreen/add/speed_count/position/num_speed";
    }
    else if (sample.hookName == "sub_824D6C18")
    {
        classification.valueName = "rollingCounterGaugeState";
        classification.status = "classified-via-generated-PPC-callsite-candidate";
        classification.source = "generated-PPC:sub_824D6C18 owner+460/+480 rolling counter/gauge state";
        classification.path = "CHudSonicStage.owner+460/+464/+468/+472/+480|ui_playscreen gauge/counter nodes";
    }
    else if (sample.hookName == "sub_824D7100")
    {
        classification.valueName = "tutorialPrompt";
        classification.status = "classified-via-generated-PPC-callsite-candidate";
        classification.source = "generated-PPC:sub_824D7100 tutorial/overlay update context";
        classification.path = "CHudSonicStage tutorial/update context|ui_playscreen/add/u_info";
    }

    return classification;
}

std::string formatSonicDayHudRuntimeCallsiteSample(const SonicDayHudRuntimeCallsiteSample& sample)
{
    std::ostringstream out;
    out << "sonic_day_hud_runtime_callsite_sample="
        << "hook=" << sample.hookName
        << ":phase=" << sample.samplePhase
        << ":owner=" << sample.ownerAddress
        << ":delta=" << std::fixed << std::setprecision(6) << sample.deltaTime
        << ":r4=" << sample.r4
        << ":field452=" << sample.ownerField452
        << ":field456=" << sample.ownerField456
        << ":field460=" << sample.ownerField460
        << ":field480=" << sample.ownerField480
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeCallsiteClassification(
    const SonicDayHudRuntimeCallsiteClassification& classification)
{
    std::ostringstream out;
    out << "sonic_day_hud_runtime_callsite_classification="
        << "value=" << classification.valueName
        << ":status=" << classification.status
        << ":source=" << classification.source
        << ":path=" << classification.path;
    if (classification.normalizedValueKnown)
        out << ":normalized=" << classification.normalizedValue;
    out << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingSmokeSequence()
{
    SonicDayHudController hud;
    const auto snapshot = makeRuntimeScoreBindingSnapshot();
    std::ostringstream out;
    out << formatSonicDayHudRuntimeBinding(snapshot);
    (void)hud.handleInput(FrontendControllerInput::StageReady);
    (void)hud.applyRuntimeBinding(snapshot);
    out << formatSonicDayHudGameplayState("runtime-bound", hud.gameplayState());
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase167SmokeSequence()
{
    const auto snapshot = makeRuntimeScoreBindingSnapshot();
    std::ostringstream out;
    out << formatSonicDayHudRuntimeBinding(snapshot);
    out << "sonic_day_hud_runtime_scoreinfo="
        << "record_speed=" << formatRuntimeBindingStatus(snapshot.scoreInfoPointMarkerRecordSpeedBinding)
        << ":point_marker_count=" << formatRuntimeBindingStatus(snapshot.scoreInfoPointMarkerCountBinding)
        << '\n';
    out << formatSonicDayHudRuntimeDisplayOwnerPaths(snapshot.displayOwnerPaths);
    out << "gameplay_numeric_binding=score:known,scoreinfo:known,"
        << "ring/timer/speed/boost/energy/lives/tutorial:pending-runtime-player-offsets"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase168SmokeSequence()
{
    const auto snapshot = makeRuntimeScoreBindingSnapshot();
    const auto writePaths = makeRuntimeValueUpdatePathSnapshot();
    std::ostringstream out;
    out << formatSonicDayHudRuntimeBinding(snapshot);
    out << "sonic_day_hud_runtime_scoreinfo="
        << "record_speed=" << formatRuntimeBindingStatus(snapshot.scoreInfoPointMarkerRecordSpeedBinding)
        << ":point_marker_count=" << formatRuntimeBindingStatus(snapshot.scoreInfoPointMarkerCountBinding)
        << '\n';
    out << formatSonicDayHudRuntimeDisplayOwnerPaths(snapshot.displayOwnerPaths);
    out << formatSonicDayHudRuntimeWritePaths(writePaths);
    out << "gameplay_numeric_binding=score:known,scoreinfo:known,"
        << "ring/timer/speed/lives:known-via-csd-text-write,"
        << "boost/energy/tutorial:pending-gauge-or-prompt-write-hook"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase169SmokeSequence()
{
    const auto snapshot = makeRuntimeScoreBindingSnapshot();
    const auto writePaths = makeRuntimeValueUpdatePathSnapshot();
    const auto drawListCoverage = makeRuntimeDrawListCoverageSnapshot();
    std::ostringstream out;
    out << formatSonicDayHudRuntimeBinding(snapshot);
    out << formatSonicDayHudRuntimeDisplayOwnerPaths(snapshot.displayOwnerPaths);
    out << formatSonicDayHudRuntimeWritePaths(writePaths);
    out << formatSonicDayHudRuntimeDrawListCoverage(drawListCoverage);
    out << "gameplay_numeric_binding=score:known,scoreinfo:known,"
        << "ring/timer/speed/lives:known-via-csd-text-write,"
        << "boost/energy/tutorial:csd-node-pattern-hide-scale-hooks-installed-pending-runtime-normalization"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase173SmokeSequence()
{
    SonicDayHudController hud;
    (void)hud.handleInput(FrontendControllerInput::StageReady);

    const auto observations = makeRuntimeOwnerFieldTextWriteObservations();
    std::ostringstream out;
    for (const auto& observation : observations)
    {
        out << formatSonicDayHudRuntimeTextWriteObservation(observation);
        (void)hud.applyRuntimeTextWrite(observation);
    }

    out << formatSonicDayHudGameplayState("raw-owner-text-write", hud.gameplayState());
    out << "gameplay_numeric_binding=score:known,scoreinfo:known,"
        << "timer/lives:runtime-proven-via-raw-owner-field-text-write,"
        << "ring/speed:csd-text-write-ready,"
        << "boost/energy/tutorial:csd-node-pattern-hide-scale-hooks-installed-pending-runtime-normalization"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase175SmokeSequence()
{
    SonicDayHudController hud;
    (void)hud.handleInput(FrontendControllerInput::StageReady);

    const auto timerSample = makeRuntimeTimerCallsiteSample();
    const auto rollingCounterSample = makeRuntimeRollingCounterCallsiteSample();

    std::ostringstream out;
    out << formatSonicDayHudRuntimeCallsiteSample(timerSample);
    out << formatSonicDayHudRuntimeCallsiteClassification(
        classifySonicDayHudRuntimeCallsiteSample(timerSample));
    (void)hud.applyRuntimeCallsiteSample(timerSample);
    out << formatSonicDayHudGameplayState("callsite-sample", hud.gameplayState());

    out << formatSonicDayHudRuntimeCallsiteSample(rollingCounterSample);
    out << formatSonicDayHudRuntimeCallsiteClassification(
        classifySonicDayHudRuntimeCallsiteSample(rollingCounterSample));

    out << "gameplay_numeric_binding=score:known,scoreinfo:known,"
        << "timer:runtime-proven-via-chud-update-callsite-sample,"
        << "ring/speed/lives:csd-text-write-ready,"
        << "boost/energy/tutorial:classified-callsite-candidates-pending-normalization"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase180SmokeSequence()
{
    SonicDayHudController hud;
    (void)hud.handleInput(FrontendControllerInput::StageReady);

    std::ostringstream out;
    for (const auto& observation : makeRuntimeGaugePromptWriteObservations())
    {
        out << formatSonicDayHudRuntimeGaugePromptWriteObservation(observation);
        (void)hud.applyRuntimeGaugePromptWrite(observation);
    }

    out << formatSonicDayHudGameplayState("gauge-prompt-write", hud.gameplayState());
    out << "gameplay_numeric_binding=score:known,scoreinfo:known,"
        << "timer:runtime-proven-via-chud-update-callsite-sample,"
        << "ring/speed/lives:csd-text-write-ready,"
        << "boost/energy/tutorial:runtime-proven-via-csd-gauge-prompt-write,"
        << "audio:pending-exact-sfx-id"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase191SmokeSequence()
{
    SonicDayHudController hud;
    (void)hud.handleInput(FrontendControllerInput::StageReady);

    std::ostringstream out;
    for (const auto& observation : makeRuntimeSemanticPathCandidateObservations())
    {
        out << formatSonicDayHudRuntimeSemanticPathCandidateObservation(observation);
        (void)hud.applyRuntimeSemanticPathCandidate(observation);
    }

    out << formatSonicDayHudGameplayState("semantic-candidate-binding", hud.gameplayState());
    out << "gameplay_numeric_binding=score:known,scoreinfo:known,"
        << "speed/boost/energy/tutorial:semantic-candidate-bound-pending-exact-child-node-resolution,"
        << "timer/ring/lives:exact-csd-text-write-or-callsite-lanes,"
        << "audio:pending-exact-sfx-id"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase193SmokeSequence()
{
    SonicDayHudController hud;
    (void)hud.handleInput(FrontendControllerInput::StageReady);

    std::ostringstream out;
    for (const auto& observation : makeRuntimePhase193SemanticBindingObservations())
    {
        out << formatSonicDayHudRuntimeSemanticPathCandidateObservation(observation);
        if (isSemanticBoundRuntimeObservation(observation))
            (void)hud.applyRuntimeSemanticPathCandidate(observation);
    }

    out << formatSonicDayHudGameplayState("phase192-strict-semantic-bound", hud.gameplayState());
    out << "gameplay_numeric_binding="
        << "elapsedFrames/speed/tutorial:semantic-bound-pending-exact-child-node-resolution,"
        << "boost/energy:semantic-candidate-only-pending-runtime-bound,"
        << "exact-child-node-resolution:pending,"
        << "audio:pending-exact-sfx-id"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase194SmokeSequence()
{
    std::ostringstream out;
    for (const auto& resolution : makeRuntimePhase194GaugeChildPathResolutions())
        out << formatSonicDayHudRuntimeGaugeChildPathResolution(resolution);

    out << "gameplay_numeric_binding="
        << "boost/energy:exact-runtime-draw-child-paths-known,"
        << "setter-node-address-join:pending,"
        << "controller-value-update:still-requires-setter-node-match,"
        << "audio:pending-exact-sfx-id"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase195SmokeSequence()
{
    SonicDayHudController hud;
    (void)hud.handleInput(FrontendControllerInput::StageReady);

    std::ostringstream out;
    for (const auto& observation : makeRuntimePhase195GaugeSetterChildPathJoins())
    {
        out << formatSonicDayHudRuntimeGaugeSetterChildPathJoin(observation);
        (void)hud.applyRuntimeGaugeSetterChildPathJoin(observation);
    }

    out << formatSonicDayHudGameplayState("phase195-gauge-setter-child-join", hud.gameplayState());
    out << "gameplay_numeric_binding="
        << "boost/energy:setter-node-address-join-runtime-proven,"
        << "controller-value-update:runtime-proven-via-exact-gauge-child-path,"
        << "audio:pending-exact-sfx-id"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase196SmokeSequence()
{
    SonicDayHudController hud;
    (void)hud.handleInput(FrontendControllerInput::StageReady);

    std::ostringstream out;
    for (const auto& observation : makeRuntimePhase196RollingGaugeCounterObservations())
    {
        out << formatSonicDayHudRuntimeRollingGaugeCounterObservation(observation);
        (void)hud.applyRuntimeRollingGaugeCounterObservation(observation);
    }

    out << formatSonicDayHudGameplayState("phase196-rolling-counter-candidate", hud.gameplayState());
    out << "gameplay_numeric_binding="
        << "boost/energy:rolling-counter-text-candidate-pending-gauge-state-normalization,"
        << "setter-node-address-join:still-required-for-final-gauge-values,"
        << "audio:pending-exact-sfx-id"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase197SmokeSequence()
{
    SonicDayHudController hud;
    (void)hud.handleInput(FrontendControllerInput::StageReady);

    std::ostringstream out;
    for (const auto& observation : makeRuntimePhase197OwnerFieldRollingCounterObservations())
    {
        out << formatSonicDayHudRuntimeOwnerFieldRollingCounterObservation(observation);
        (void)hud.applyRuntimeOwnerFieldRollingCounterObservation(observation);
    }

    out << formatSonicDayHudGameplayState("phase197-owner-field-rolling-counter-candidate", hud.gameplayState());
    out << "gameplay_numeric_binding="
        << "boost/energy:owner-field-rolling-counter-pending-exact-offset-normalization,"
        << "setter-node-address-join:still-required-for-final-gauge-values,"
        << "audio:pending-exact-sfx-id"
        << '\n';
    return out.str();
}

std::string formatSonicDayHudRuntimeBindingPhase198SmokeSequence()
{
    SonicDayHudController hud;
    (void)hud.handleInput(FrontendControllerInput::StageReady);

    std::ostringstream out;
    for (const auto& correlation : makeRuntimePhase198OwnerFieldGaugeScaleCorrelations())
    {
        out << formatSonicDayHudRuntimeOwnerFieldGaugeScaleCorrelation(correlation);
        (void)hud.applyRuntimeOwnerFieldGaugeScaleCorrelation(correlation);
    }

    out << formatSonicDayHudGameplayState("phase198-owner-field-gauge-scale-correlation-candidate", hud.gameplayState());
    out << "gameplay_numeric_binding="
        << "boost/energy:owner-field-gauge-scale-correlation-pending-formula-proof,"
        << "setter-node-address-join:still-required-for-final-gauge-values,"
        << "audio:pending-exact-sfx-id"
        << '\n';
    return out.str();
}

std::string frontendControllerInputName(FrontendControllerInput input)
{
    switch (input)
    {
    case FrontendControllerInput::None:
        return "none";
    case FrontendControllerInput::PressStart:
        return "press-start";
    case FrontendControllerInput::Confirm:
        return "confirm";
    case FrontendControllerInput::Cancel:
        return "cancel";
    case FrontendControllerInput::MoveUp:
        return "move-up";
    case FrontendControllerInput::MoveDown:
        return "move-down";
    case FrontendControllerInput::RouteReady:
        return "route-ready";
    case FrontendControllerInput::LoadingComplete:
        return "loading-complete";
    case FrontendControllerInput::Pause:
        return "pause";
    case FrontendControllerInput::StageReady:
        return "stage-ready";
    case FrontendControllerInput::TutorialReady:
        return "tutorial-ready";
    case FrontendControllerInput::RingPickup:
        return "ring-pickup";
    }
    return "unknown";
}
} // namespace sward::ui_runtime
