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
[[nodiscard]] std::string frontendControllerInputName(FrontendControllerInput input);
} // namespace sward::ui_runtime
