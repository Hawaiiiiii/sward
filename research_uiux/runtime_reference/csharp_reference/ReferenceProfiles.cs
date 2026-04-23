namespace Sward.UiRuntime.Reference;

public static class ReferenceProfiles
{
    public static ScreenContract BuildPauseMenu() =>
        new()
        {
            ScreenId = "PauseMenuReference",
            TimelineBands = new Dictionary<string, TimelineBand>
            {
                ["intro_medium"] = new("intro_medium", 0.333333),
                ["select_travel"] = new("select_travel", 0.333333),
                ["confirm_hold"] = new("confirm_hold", 0.45),
                ["cancel_hold"] = new("cancel_hold", 0.25),
                ["outro_medium"] = new("outro_medium", 0.333333),
            },
            States = new Dictionary<ScreenState, StateDefinition>
            {
                [ScreenState.Boot] = new(ScreenState.Boot, "Boot", "", null, null, false),
                [ScreenState.Intro] = new(ScreenState.Intro, "Intro", "reveal", "intro_medium", ScreenState.Idle, false),
                [ScreenState.Idle] = new(ScreenState.Idle, "Idle", "idle", null, null, true),
                [ScreenState.Navigate] = new(ScreenState.Navigate, "Navigate", "focus_pulse", "select_travel", ScreenState.Idle, false),
                [ScreenState.Confirm] = new(ScreenState.Confirm, "Confirm", "confirm", "confirm_hold", ScreenState.Outro, false),
                [ScreenState.Cancel] = new(ScreenState.Cancel, "Cancel", "cancel", "cancel_hold", ScreenState.Outro, false),
                [ScreenState.Outro] = new(ScreenState.Outro, "Outro", "outro", "outro_medium", ScreenState.Closed, false),
                [ScreenState.Closed] = new(ScreenState.Closed, "Closed", "", null, null, false),
            },
            OverlayLayers = new List<OverlayLayer>
            {
                new("backdrop", "backdrop", false),
                new("chrome", "chrome", false),
                new("content", "content", true),
                new("prompts", "prompt", false),
                new("fx", "transient_fx", false),
            },
            VisibleOverlayRoles = new Dictionary<ScreenState, IReadOnlySet<string>>
            {
                [ScreenState.Intro] = new HashSet<string> { "backdrop", "chrome", "content" },
                [ScreenState.Idle] = new HashSet<string> { "backdrop", "chrome", "content", "prompt" },
                [ScreenState.Navigate] = new HashSet<string> { "backdrop", "chrome", "content", "prompt", "transient_fx" },
                [ScreenState.Confirm] = new HashSet<string> { "backdrop", "chrome", "content", "transient_fx" },
                [ScreenState.Cancel] = new HashSet<string> { "backdrop", "chrome", "content", "transient_fx" },
                [ScreenState.Outro] = new HashSet<string> { "backdrop", "chrome", "content" },
            },
            PromptSlots = new List<PromptSlot>
            {
                new("confirm_primary", PromptButton.A, "Select", new HashSet<ScreenState> { ScreenState.Idle }, new[] { "can_confirm" }),
                new("cancel_secondary", PromptButton.B, "Back", new HashSet<ScreenState> { ScreenState.Idle }, new[] { "can_cancel" }),
                new("page_left", PromptButton.LB, "Previous Tab", new HashSet<ScreenState> { ScreenState.Idle }, new[] { "has_previous_tab" }),
                new("page_right", PromptButton.RB, "Next Tab", new HashSet<ScreenState> { ScreenState.Idle }, new[] { "has_next_tab" }),
            },
        };

    public static ScreenContract BuildTitleMenu() =>
        new()
        {
            ScreenId = "TitleMenuReference",
            TimelineBands = new Dictionary<string, TimelineBand>
            {
                ["intro_medium"] = new("intro_medium", 0.333333),
                ["select_travel"] = new("select_travel", 0.333333),
                ["confirm_hold"] = new("confirm_hold", 0.45),
                ["cancel_hold"] = new("cancel_hold", 0.25),
                ["logo_hold"] = new("logo_hold", 1.2),
                ["outro_medium"] = new("outro_medium", 0.333333),
            },
            States = new Dictionary<ScreenState, StateDefinition>
            {
                [ScreenState.Boot] = new(ScreenState.Boot, "Boot", "", null, null, false),
                [ScreenState.Intro] = new(ScreenState.Intro, "Intro", "logo_reveal", "logo_hold", ScreenState.Idle, false),
                [ScreenState.Idle] = new(ScreenState.Idle, "Idle", "carousel_idle", null, null, true),
                [ScreenState.Navigate] = new(ScreenState.Navigate, "Navigate", "carousel_move", "select_travel", ScreenState.Idle, false),
                [ScreenState.Confirm] = new(ScreenState.Confirm, "Confirm", "selection_commit", "confirm_hold", ScreenState.Outro, false),
                [ScreenState.Cancel] = new(ScreenState.Cancel, "Cancel", "menu_back", "cancel_hold", ScreenState.Outro, false),
                [ScreenState.Outro] = new(ScreenState.Outro, "Outro", "fade_out", "outro_medium", ScreenState.Closed, false),
                [ScreenState.Closed] = new(ScreenState.Closed, "Closed", "", null, null, false),
            },
            OverlayLayers = new List<OverlayLayer>
            {
                new("backdrop", "backdrop", false),
                new("logo", "logo", false),
                new("carousel", "content", true),
                new("prompts", "prompt", false),
                new("flare", "transient_fx", false),
            },
            VisibleOverlayRoles = new Dictionary<ScreenState, IReadOnlySet<string>>
            {
                [ScreenState.Intro] = new HashSet<string> { "backdrop", "logo", "content", "transient_fx" },
                [ScreenState.Idle] = new HashSet<string> { "backdrop", "logo", "content", "prompt" },
                [ScreenState.Navigate] = new HashSet<string> { "backdrop", "logo", "content", "prompt", "transient_fx" },
                [ScreenState.Confirm] = new HashSet<string> { "backdrop", "logo", "content", "transient_fx" },
                [ScreenState.Cancel] = new HashSet<string> { "backdrop", "logo", "content", "transient_fx" },
                [ScreenState.Outro] = new HashSet<string> { "backdrop", "logo", "transient_fx" },
            },
            PromptSlots = new List<PromptSlot>
            {
                new("confirm_primary", PromptButton.A, "Start", new HashSet<ScreenState> { ScreenState.Idle }, new[] { "can_confirm" }),
                new("cancel_secondary", PromptButton.B, "Quit", new HashSet<ScreenState> { ScreenState.Idle }, new[] { "can_cancel" }),
                new("page_left", PromptButton.LB, "Previous", new HashSet<ScreenState> { ScreenState.Idle }, new[] { "has_previous_tab" }),
                new("page_right", PromptButton.RB, "Next", new HashSet<ScreenState> { ScreenState.Idle }, new[] { "has_next_tab" }),
                new("press_start", PromptButton.Start, "Open", new HashSet<ScreenState> { ScreenState.Idle }, new[] { "show_start_prompt" }),
            },
        };

    public static ScreenContract BuildAutosaveToast() =>
        new()
        {
            ScreenId = "AutosaveToastReference",
            TimelineBands = new Dictionary<string, TimelineBand>
            {
                ["intro_short"] = new("intro_short", 0.25),
                ["toast_hold"] = new("toast_hold", 3.0),
                ["outro_short"] = new("outro_short", 0.25),
            },
            States = new Dictionary<ScreenState, StateDefinition>
            {
                [ScreenState.Boot] = new(ScreenState.Boot, "Boot", "", null, null, false),
                [ScreenState.Intro] = new(ScreenState.Intro, "Intro", "toast_in", "intro_short", ScreenState.Idle, false),
                [ScreenState.Idle] = new(ScreenState.Idle, "Idle", "toast_hold", "toast_hold", ScreenState.Outro, false),
                [ScreenState.Outro] = new(ScreenState.Outro, "Outro", "toast_out", "outro_short", ScreenState.Closed, false),
                [ScreenState.Closed] = new(ScreenState.Closed, "Closed", "", null, null, false),
            },
            OverlayLayers = new List<OverlayLayer>
            {
                new("icon", "icon", false),
                new("panel", "content", false),
                new("shine", "transient_fx", false),
            },
            VisibleOverlayRoles = new Dictionary<ScreenState, IReadOnlySet<string>>
            {
                [ScreenState.Intro] = new HashSet<string> { "icon", "content", "transient_fx" },
                [ScreenState.Idle] = new HashSet<string> { "icon", "content" },
                [ScreenState.Outro] = new HashSet<string> { "icon", "content", "transient_fx" },
            },
            PromptSlots = new List<PromptSlot>(),
        };
}
