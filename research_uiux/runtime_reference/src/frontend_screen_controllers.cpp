#include <sward/ui_runtime/frontend_screen_controllers.hpp>

#include <sward/ui_runtime/frontend_screen_reference.hpp>

#include <algorithm>
#include <sstream>

namespace sward::ui_runtime
{
namespace
{
constexpr std::string_view kSonicDayHudNext = "sonic-day-hud-next";

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
    }
    return "unknown";
}
} // namespace sward::ui_runtime
