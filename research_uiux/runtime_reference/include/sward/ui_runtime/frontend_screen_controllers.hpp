#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime
{
enum class FrontendControllerInput
{
    None,
    PressStart,
    Confirm,
    Cancel,
    MoveUp,
    MoveDown,
    RouteReady,
    LoadingComplete,
    Pause,
    StageReady,
    TutorialReady,
    RingPickup,
};

struct FrontendControllerFrame
{
    std::string controllerName;
    std::string screenId;
    int frame = 0;
    std::string stateName;
    std::string motionName;
    bool inputLocked = false;
    int cursorIndex = 0;
    std::string cursorLabel;
    std::string sfxHook;
    std::string nextScreenId;
    std::vector<std::string> activeScenes;
};

struct SonicDayHudValueProvenance
{
    std::string ringCount;
    std::string score;
    std::string elapsedFrames;
    std::string speedKmh;
    std::string boostGauge;
    std::string ringEnergyGauge;
    std::string lifeCount;
    std::string tutorialPrompt;
    std::string routeEvent;
    std::string valueSource;
};

struct SonicDayHudGameplayState
{
    int ringCount = 0;
    int score = 0;
    int elapsedFrames = 0;
    int speedKmh = 0;
    double boostGauge = 0.0;
    double ringEnergyGauge = 1.0;
    int lifeCount = 3;
    std::string tutorialPromptId = "none";
    bool tutorialVisible = false;
    std::string routeEvent = "stage-hud-ready";
    std::string lastSfxHook = "none";
    std::string sfxCueId = "audio-id-pending";
    SonicDayHudValueProvenance provenance;
};

struct SonicDayHudRuntimeValueBinding
{
    bool known = false;
    std::string source = "pending-runtime-field";
};

struct SonicDayHudRuntimeValueUpdatePath
{
    SonicDayHudRuntimeValueBinding ringCountWritePath;
    SonicDayHudRuntimeValueBinding elapsedFramesWritePath;
    SonicDayHudRuntimeValueBinding speedReadoutWritePath;
    SonicDayHudRuntimeValueBinding lifeCountWritePath;
    SonicDayHudRuntimeValueBinding boostGaugeWritePath;
    SonicDayHudRuntimeValueBinding ringEnergyGaugeWritePath;
    SonicDayHudRuntimeValueBinding tutorialPromptWritePath;
};

struct SonicDayHudRuntimeTextWriteObservation
{
    std::string valueName;
    std::string path;
    std::string textUtf8;
    std::string pathResolutionSource;
    std::string source;
};

struct SonicDayHudRuntimeGaugePromptWriteObservation
{
    std::string valueName;
    std::string path;
    std::string writeKind;
    double numericValue = 0.0;
    bool numericValueKnown = true;
    std::string pathResolutionSource;
    std::string source;
};

struct SonicDayHudRuntimeSemanticPathCandidateObservation
{
    std::string valueName;
    std::string path;
    std::string writeKind;
    std::string textUtf8;
    double numericValue = 0.0;
    bool numericValueKnown = false;
    int candidateWriteCount = 0;
    std::string pathResolutionSource = "generated-PPC-callsite-semantic-candidate";
    std::string bindingStatus = "semantic-candidate-only-pending-runtime-bound";
    std::string source;
};

struct SonicDayHudRuntimeGaugeChildPathResolution
{
    std::string valueName;
    std::string exactParentPath;
    std::string representativeChildPath;
    int runtimeDrawLayerCount = 0;
    std::string pathResolutionSource = "live-bridge/ui-draw-list";
    std::string setterNodeJoinStatus = "setter-node-address-join-pending";
};

struct SonicDayHudRuntimeGaugeSetterChildPathJoin
{
    std::string valueName;
    std::string nodeAddress;
    std::string writeKind;
    double numericValue = 0.0;
    bool numericValueKnown = true;
    std::string exactParentPath;
    std::string exactChildPath;
    std::string addressJoinSource = "runtime-draw-list-cast-node-match";
    std::string bindingStatus = "setter-node-address-join-runtime-proven";
};

struct SonicDayHudRuntimeDrawListCoverage
{
    std::string source = "live-bridge/ui-draw-list";
    std::string activeProject = "ui_playscreen";
    int runtimeDrawCalls = 0;
    int correlatedMaterialPairs = 0;
    bool speedGaugeObserved = false;
    bool gaugeFrameObserved = false;
    bool ringEnergyGaugeObserved = false;
    bool tutorialPromptObserved = false;
    bool pauseOverlayObserved = false;
    bool textWriteObserved = false;
    std::string nextHook = "CSD::CNode::SetPatternIndex/SetHideFlag/SetScale";
};

struct SonicDayHudRuntimeCallsiteSample
{
    std::string ownerAddress = "0x0";
    std::string hookName;
    std::string samplePhase;
    double deltaTime = 0.0;
    std::string r4 = "0x0";
    int ownerField432 = 0;
    int ownerField440 = 0;
    int ownerField444 = 0;
    int ownerField452 = 0;
    int ownerField456 = 0;
    int ownerField460 = 0;
    int ownerField464 = 0;
    int ownerField468 = 0;
    int ownerField472 = 0;
    int ownerField476 = 0;
    int ownerField480 = 0;
    std::string ownerField484 = "0x0";
    std::string ownerField488 = "0x0";
};

struct SonicDayHudRuntimeCallsiteClassification
{
    std::string valueName = "unknown";
    std::string status = "unclassified";
    std::string source = "pending-runtime-callsite";
    std::string path = "unresolved";
    bool normalizedValueKnown = false;
    int normalizedValue = 0;
};

struct SonicDayHudDisplayOwnerPathBinding
{
    std::string ringCount = "ui_playscreen/ring_count";
    std::string score = "CHudSonicStage.m_rcScoreCount|ui_playscreen/score_count";
    std::string elapsedFrames = "CHudSonicStage.m_rcTimeCount|ui_playscreen/time_count";
    std::string speedKmh = "CHudSonicStage.m_rcSpeedGauge|ui_playscreen/so_speed_gauge";
    std::string boostGauge = "CHudSonicStage.m_rcSpeedGauge|ui_playscreen/so_speed_gauge";
    std::string ringEnergyGauge = "CHudSonicStage.m_rcRingEnergyGauge|ui_playscreen/so_ringenagy_gauge";
    std::string lifeCount = "CHudSonicStage.m_rcPlayerCount|ui_playscreen/player_count";
    std::string tutorialPrompt = "ui_playscreen/add/u_info";
};

struct SonicDayHudRuntimeBindingSnapshot
{
    std::string source = "typedInspectors.sonicHud.gameplayValues";
    SonicDayHudGameplayState values;
    SonicDayHudRuntimeValueBinding ringCountBinding;
    SonicDayHudRuntimeValueBinding scoreBinding;
    SonicDayHudRuntimeValueBinding scoreInfoPointMarkerRecordSpeedBinding;
    SonicDayHudRuntimeValueBinding scoreInfoPointMarkerCountBinding;
    SonicDayHudRuntimeValueBinding elapsedFramesBinding;
    SonicDayHudRuntimeValueBinding speedKmhBinding;
    SonicDayHudRuntimeValueBinding boostGaugeBinding;
    SonicDayHudRuntimeValueBinding ringEnergyGaugeBinding;
    SonicDayHudRuntimeValueBinding lifeCountBinding;
    SonicDayHudRuntimeValueBinding tutorialPromptBinding;
    SonicDayHudDisplayOwnerPathBinding displayOwnerPaths;
    std::string sonicRingPickupSfxId = "audio-id-pending";
    std::string tutorialPromptOpenSfxId = "audio-id-pending";
    std::string pauseOpenSfxId = "sys_actstg_pausewinopen";
    std::string pauseCursorSfxId = "sys_actstg_pausecursor";
};

class TitleMenuController
{
public:
    TitleMenuController();

    void reset();
    [[nodiscard]] FrontendControllerFrame handleInput(FrontendControllerInput input);
    [[nodiscard]] const FrontendControllerFrame& frame() const;

private:
    FrontendControllerFrame frame_;
};

class LoadingScreenController
{
public:
    LoadingScreenController();

    void reset();
    [[nodiscard]] FrontendControllerFrame handleInput(FrontendControllerInput input);
    [[nodiscard]] const FrontendControllerFrame& frame() const;

private:
    FrontendControllerFrame frame_;
};

class OptionsMenuController
{
public:
    OptionsMenuController();

    void reset();
    [[nodiscard]] FrontendControllerFrame handleInput(FrontendControllerInput input);
    [[nodiscard]] const FrontendControllerFrame& frame() const;

private:
    FrontendControllerFrame frame_;
};

class PauseMenuController
{
public:
    PauseMenuController();

    void reset();
    [[nodiscard]] FrontendControllerFrame handleInput(FrontendControllerInput input);
    [[nodiscard]] const FrontendControllerFrame& frame() const;

private:
    FrontendControllerFrame frame_;
};

class SonicDayHudController
{
public:
    SonicDayHudController();

    // Runtime archaeology inputs: FrontendControllerInput::StageReady,
    // FrontendControllerInput::TutorialReady, and FrontendControllerInput::RingPickup.
    void reset();
    [[nodiscard]] FrontendControllerFrame handleInput(FrontendControllerInput input);
    [[nodiscard]] const FrontendControllerFrame& frame() const;
    [[nodiscard]] const SonicDayHudGameplayState& gameplayState() const;
    [[nodiscard]] FrontendControllerFrame setGameplayState(const SonicDayHudGameplayState& state);
    [[nodiscard]] FrontendControllerFrame applyRuntimeBinding(const SonicDayHudRuntimeBindingSnapshot& snapshot);
    [[nodiscard]] FrontendControllerFrame applyRuntimeTextWrite(const SonicDayHudRuntimeTextWriteObservation& observation);
    [[nodiscard]] FrontendControllerFrame applyRuntimeGaugePromptWrite(
        const SonicDayHudRuntimeGaugePromptWriteObservation& observation);
    [[nodiscard]] FrontendControllerFrame applyRuntimeSemanticPathCandidate(
        const SonicDayHudRuntimeSemanticPathCandidateObservation& observation);
    [[nodiscard]] FrontendControllerFrame applyRuntimeGaugeSetterChildPathJoin(
        const SonicDayHudRuntimeGaugeSetterChildPathJoin& observation);
    [[nodiscard]] FrontendControllerFrame applyRuntimeCallsiteSample(
        const SonicDayHudRuntimeCallsiteSample& sample);
    [[nodiscard]] FrontendControllerFrame applyRingPickup(int ringDelta, int scoreDelta);
    [[nodiscard]] FrontendControllerFrame openTutorialPrompt(std::string_view promptId);
    [[nodiscard]] FrontendControllerFrame dismissTutorialPrompt();

private:
    FrontendControllerFrame frame_;
    SonicDayHudGameplayState gameplayState_;
};

[[nodiscard]] std::vector<FrontendControllerFrame> runFrontendControllerSmokeSequence();
[[nodiscard]] std::vector<FrontendControllerFrame> runSonicDayHudControllerSmokeSequence();
[[nodiscard]] const SonicDayHudValueProvenance& sonicDayHudValueProvenance();
[[nodiscard]] std::string formatFrontendControllerCatalog();
[[nodiscard]] std::string formatFrontendControllerFrame(const FrontendControllerFrame& frame);
[[nodiscard]] std::string formatFrontendControllerSmokeSequence();
[[nodiscard]] std::string formatSonicDayHudControllerSmokeSequence();
[[nodiscard]] std::string formatSonicDayHudGameplayState(std::string_view phase, const SonicDayHudGameplayState& state);
[[nodiscard]] std::string formatSonicDayHudGameplayStateModel();
[[nodiscard]] std::string formatSonicDayHudGameplayStateSmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBinding(const SonicDayHudRuntimeBindingSnapshot& snapshot);
[[nodiscard]] std::string formatSonicDayHudRuntimeDisplayOwnerPaths(const SonicDayHudDisplayOwnerPathBinding& paths);
[[nodiscard]] std::string formatSonicDayHudRuntimeWritePaths(const SonicDayHudRuntimeValueUpdatePath& paths);
[[nodiscard]] std::string formatSonicDayHudRuntimeTextWriteObservation(const SonicDayHudRuntimeTextWriteObservation& observation);
[[nodiscard]] std::string formatSonicDayHudRuntimeGaugePromptWriteObservation(
    const SonicDayHudRuntimeGaugePromptWriteObservation& observation);
[[nodiscard]] std::string formatSonicDayHudRuntimeSemanticPathCandidateObservation(
    const SonicDayHudRuntimeSemanticPathCandidateObservation& observation);
[[nodiscard]] std::string formatSonicDayHudRuntimeGaugeChildPathResolution(
    const SonicDayHudRuntimeGaugeChildPathResolution& resolution);
[[nodiscard]] std::string formatSonicDayHudRuntimeGaugeSetterChildPathJoin(
    const SonicDayHudRuntimeGaugeSetterChildPathJoin& observation);
[[nodiscard]] std::string formatSonicDayHudRuntimeDrawListCoverage(const SonicDayHudRuntimeDrawListCoverage& coverage);
[[nodiscard]] SonicDayHudRuntimeCallsiteClassification classifySonicDayHudRuntimeCallsiteSample(
    const SonicDayHudRuntimeCallsiteSample& sample);
[[nodiscard]] std::string formatSonicDayHudRuntimeCallsiteSample(const SonicDayHudRuntimeCallsiteSample& sample);
[[nodiscard]] std::string formatSonicDayHudRuntimeCallsiteClassification(
    const SonicDayHudRuntimeCallsiteClassification& classification);
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingSmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingPhase167SmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingPhase168SmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingPhase169SmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingPhase173SmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingPhase175SmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingPhase180SmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingPhase191SmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingPhase193SmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingPhase194SmokeSequence();
[[nodiscard]] std::string formatSonicDayHudRuntimeBindingPhase195SmokeSequence();
[[nodiscard]] std::string frontendControllerInputName(FrontendControllerInput input);
} // namespace sward::ui_runtime
