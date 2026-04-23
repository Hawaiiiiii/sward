#include <sward/ui_runtime/runtime.hpp>

#include <iostream>

using namespace sward::ui_runtime;

namespace
{
ScreenContract buildPauseMenuContract()
{
    ScreenContract contract;
    contract.screenId = "PauseMenuReference";

    contract.timelineBands.emplace("intro_medium", TimelineBand{ "intro_medium", 0.333333 });
    contract.timelineBands.emplace("select_travel", TimelineBand{ "select_travel", 0.333333 });
    contract.timelineBands.emplace("confirm_hold", TimelineBand{ "confirm_hold", 0.45 });
    contract.timelineBands.emplace("cancel_hold", TimelineBand{ "cancel_hold", 0.25 });
    contract.timelineBands.emplace("outro_medium", TimelineBand{ "outro_medium", 0.333333 });

    contract.states.emplace(ScreenState::Boot, StateDefinition{ ScreenState::Boot, "Boot", "", std::nullopt, std::nullopt, false });
    contract.states.emplace(ScreenState::Intro, StateDefinition{ ScreenState::Intro, "Intro", "reveal", "intro_medium", ScreenState::Idle, false });
    contract.states.emplace(ScreenState::Idle, StateDefinition{ ScreenState::Idle, "Idle", "idle", std::nullopt, std::nullopt, true });
    contract.states.emplace(ScreenState::Navigate, StateDefinition{ ScreenState::Navigate, "Navigate", "focus_pulse", "select_travel", ScreenState::Idle, false });
    contract.states.emplace(ScreenState::Confirm, StateDefinition{ ScreenState::Confirm, "Confirm", "confirm", "confirm_hold", ScreenState::Outro, false });
    contract.states.emplace(ScreenState::Cancel, StateDefinition{ ScreenState::Cancel, "Cancel", "cancel", "cancel_hold", ScreenState::Outro, false });
    contract.states.emplace(ScreenState::Outro, StateDefinition{ ScreenState::Outro, "Outro", "outro", "outro_medium", ScreenState::Closed, false });
    contract.states.emplace(ScreenState::Closed, StateDefinition{ ScreenState::Closed, "Closed", "", std::nullopt, std::nullopt, false });

    contract.overlayLayers = {
        { "backdrop", "backdrop", false },
        { "chrome", "chrome", false },
        { "content", "content", true },
        { "prompts", "prompt", false },
        { "fx", "transient_fx", false },
    };

    contract.visibleOverlayRoles = {
        { ScreenState::Intro, { "backdrop", "chrome", "content" } },
        { ScreenState::Idle, { "backdrop", "chrome", "content", "prompt" } },
        { ScreenState::Navigate, { "backdrop", "chrome", "content", "prompt", "transient_fx" } },
        { ScreenState::Confirm, { "backdrop", "chrome", "content", "transient_fx" } },
        { ScreenState::Cancel, { "backdrop", "chrome", "content", "transient_fx" } },
        { ScreenState::Outro, { "backdrop", "chrome", "content" } },
    };

    contract.promptSlots = {
        { "confirm_primary", PromptButton::A, "Select", { ScreenState::Idle }, { "can_confirm" } },
        { "cancel_secondary", PromptButton::B, "Back", { ScreenState::Idle }, { "can_cancel" } },
        { "page_left", PromptButton::LB, "Previous Tab", { ScreenState::Idle }, { "has_previous_tab" } },
        { "page_right", PromptButton::RB, "Next Tab", { ScreenState::Idle }, { "has_next_tab" } },
    };

    return contract;
}

void printVisiblePrompts(const ScreenRuntime& runtime)
{
    std::cout << "Visible prompts:";
    auto prompts = runtime.visiblePrompts();
    if (prompts.empty())
    {
        std::cout << " none\n";
        return;
    }

    for (const auto& prompt : prompts)
        std::cout << " [" << toString(prompt.button) << ": " << prompt.label << "]";
    std::cout << '\n';
}
} // namespace

int main()
{
    ScreenRuntime runtime(
        buildPauseMenuContract(),
        RuntimeCallbacks{
            .onStateEntered = [](ScreenState state) { std::cout << "Enter " << toString(state) << '\n'; },
            .onStateChanged = [](ScreenState from, ScreenState to) { std::cout << "Transition " << toString(from) << " -> " << toString(to) << '\n'; },
            .onInputBlocked = [](InputAction action) { std::cout << "Blocked input: " << toString(action) << '\n'; },
            .onSceneRequested = [](std::string_view scene) { std::cout << "Request scene: " << scene << '\n'; },
        });

    runtime.setPredicate("can_confirm", true);
    runtime.setPredicate("can_cancel", true);
    runtime.setPredicate("has_previous_tab", true);
    runtime.setPredicate("has_next_tab", true);

    runtime.dispatch(RuntimeEventType::ResourcesReady);
    runtime.tick(0.4);
    printVisiblePrompts(runtime);

    runtime.requestAction(InputAction::MoveNext);
    runtime.tick(0.4);
    printVisiblePrompts(runtime);

    runtime.requestAction(InputAction::Confirm);
    runtime.dispatch(RuntimeEventType::ActionCompletedKeepOpen);
    printVisiblePrompts(runtime);

    runtime.requestAction(InputAction::Cancel);
    runtime.tick(0.3);
    runtime.tick(0.4);

    std::cout << "Final state: " << toString(runtime.state()) << '\n';
    return 0;
}
