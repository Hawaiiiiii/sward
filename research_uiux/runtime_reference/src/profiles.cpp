#include <sward/ui_runtime/profiles.hpp>

#include <stdexcept>

namespace sward::ui_runtime
{
ScreenContract makePauseMenuContract()
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

ScreenContract makeTitleMenuContract()
{
    ScreenContract contract;
    contract.screenId = "TitleMenuReference";

    contract.timelineBands.emplace("intro_medium", TimelineBand{ "intro_medium", 0.333333 });
    contract.timelineBands.emplace("select_travel", TimelineBand{ "select_travel", 0.333333 });
    contract.timelineBands.emplace("confirm_hold", TimelineBand{ "confirm_hold", 0.45 });
    contract.timelineBands.emplace("cancel_hold", TimelineBand{ "cancel_hold", 0.25 });
    contract.timelineBands.emplace("logo_hold", TimelineBand{ "logo_hold", 1.2 });
    contract.timelineBands.emplace("outro_medium", TimelineBand{ "outro_medium", 0.333333 });

    contract.states.emplace(ScreenState::Boot, StateDefinition{ ScreenState::Boot, "Boot", "", std::nullopt, std::nullopt, false });
    contract.states.emplace(ScreenState::Intro, StateDefinition{ ScreenState::Intro, "Intro", "logo_reveal", "logo_hold", ScreenState::Idle, false });
    contract.states.emplace(ScreenState::Idle, StateDefinition{ ScreenState::Idle, "Idle", "carousel_idle", std::nullopt, std::nullopt, true });
    contract.states.emplace(ScreenState::Navigate, StateDefinition{ ScreenState::Navigate, "Navigate", "carousel_move", "select_travel", ScreenState::Idle, false });
    contract.states.emplace(ScreenState::Confirm, StateDefinition{ ScreenState::Confirm, "Confirm", "selection_commit", "confirm_hold", ScreenState::Outro, false });
    contract.states.emplace(ScreenState::Cancel, StateDefinition{ ScreenState::Cancel, "Cancel", "menu_back", "cancel_hold", ScreenState::Outro, false });
    contract.states.emplace(ScreenState::Outro, StateDefinition{ ScreenState::Outro, "Outro", "fade_out", "outro_medium", ScreenState::Closed, false });
    contract.states.emplace(ScreenState::Closed, StateDefinition{ ScreenState::Closed, "Closed", "", std::nullopt, std::nullopt, false });

    contract.overlayLayers = {
        { "backdrop", "backdrop", false },
        { "logo", "logo", false },
        { "carousel", "content", true },
        { "prompts", "prompt", false },
        { "flare", "transient_fx", false },
    };

    contract.visibleOverlayRoles = {
        { ScreenState::Intro, { "backdrop", "logo", "content", "transient_fx" } },
        { ScreenState::Idle, { "backdrop", "logo", "content", "prompt" } },
        { ScreenState::Navigate, { "backdrop", "logo", "content", "prompt", "transient_fx" } },
        { ScreenState::Confirm, { "backdrop", "logo", "content", "transient_fx" } },
        { ScreenState::Cancel, { "backdrop", "logo", "content", "transient_fx" } },
        { ScreenState::Outro, { "backdrop", "logo", "transient_fx" } },
    };

    contract.promptSlots = {
        { "confirm_primary", PromptButton::A, "Start", { ScreenState::Idle }, { "can_confirm" } },
        { "cancel_secondary", PromptButton::B, "Quit", { ScreenState::Idle }, { "can_cancel" } },
        { "page_left", PromptButton::LB, "Previous", { ScreenState::Idle }, { "has_previous_tab" } },
        { "page_right", PromptButton::RB, "Next", { ScreenState::Idle }, { "has_next_tab" } },
        { "press_start", PromptButton::Start, "Open", { ScreenState::Idle }, { "show_start_prompt" } },
    };

    return contract;
}

ScreenContract makeAutosaveToastContract()
{
    ScreenContract contract;
    contract.screenId = "AutosaveToastReference";

    contract.timelineBands.emplace("intro_short", TimelineBand{ "intro_short", 0.25 });
    contract.timelineBands.emplace("toast_hold", TimelineBand{ "toast_hold", 3.0 });
    contract.timelineBands.emplace("outro_short", TimelineBand{ "outro_short", 0.25 });

    contract.states.emplace(ScreenState::Boot, StateDefinition{ ScreenState::Boot, "Boot", "", std::nullopt, std::nullopt, false });
    contract.states.emplace(ScreenState::Intro, StateDefinition{ ScreenState::Intro, "Intro", "toast_in", "intro_short", ScreenState::Idle, false });
    contract.states.emplace(ScreenState::Idle, StateDefinition{ ScreenState::Idle, "Idle", "toast_hold", "toast_hold", ScreenState::Outro, false });
    contract.states.emplace(ScreenState::Outro, StateDefinition{ ScreenState::Outro, "Outro", "toast_out", "outro_short", ScreenState::Closed, false });
    contract.states.emplace(ScreenState::Closed, StateDefinition{ ScreenState::Closed, "Closed", "", std::nullopt, std::nullopt, false });

    contract.overlayLayers = {
        { "icon", "icon", false },
        { "panel", "content", false },
        { "shine", "transient_fx", false },
    };

    contract.visibleOverlayRoles = {
        { ScreenState::Intro, { "icon", "content", "transient_fx" } },
        { ScreenState::Idle, { "icon", "content" } },
        { ScreenState::Outro, { "icon", "content", "transient_fx" } },
    };

    return contract;
}

ScreenContract makeContract(ReferenceProfile profile)
{
    switch (profile)
    {
    case ReferenceProfile::PauseMenu:
        return makePauseMenuContract();
    case ReferenceProfile::TitleMenu:
        return makeTitleMenuContract();
    case ReferenceProfile::AutosaveToast:
        return makeAutosaveToastContract();
    }

    throw std::runtime_error("Unknown reference profile.");
}

std::string_view toString(ReferenceProfile profile)
{
    switch (profile)
    {
    case ReferenceProfile::PauseMenu:
        return "PauseMenu";
    case ReferenceProfile::TitleMenu:
        return "TitleMenu";
    case ReferenceProfile::AutosaveToast:
        return "AutosaveToast";
    }

    return "Unknown";
}
} // namespace sward::ui_runtime
