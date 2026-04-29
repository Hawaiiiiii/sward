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

[[nodiscard]] std::vector<FrontendControllerFrame> runFrontendControllerSmokeSequence();
[[nodiscard]] std::string formatFrontendControllerCatalog();
[[nodiscard]] std::string formatFrontendControllerFrame(const FrontendControllerFrame& frame);
[[nodiscard]] std::string formatFrontendControllerSmokeSequence();
[[nodiscard]] std::string frontendControllerInputName(FrontendControllerInput input);
} // namespace sward::ui_runtime
