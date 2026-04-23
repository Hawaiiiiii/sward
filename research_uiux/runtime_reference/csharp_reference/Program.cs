using Sward.UiRuntime.Reference;

static void PrintPrompts(ScreenRuntime runtime)
{
    var prompts = runtime.VisiblePrompts();
    Console.Write("Visible prompts:");
    if (prompts.Count == 0)
    {
        Console.WriteLine(" none");
        return;
    }

    foreach (var prompt in prompts)
        Console.Write($" [{prompt.Button}: {prompt.Label}]");

    Console.WriteLine();
}

static void PrintLayers(ScreenRuntime runtime)
{
    var layers = runtime.VisibleLayers();
    Console.Write("Visible layers:");
    if (layers.Count == 0)
    {
        Console.WriteLine(" none");
        return;
    }

    foreach (var layer in layers)
        Console.Write($" [{layer.Id}:{layer.Role}]");

    Console.WriteLine();
}

static ScreenRuntime BuildRuntime(ScreenContract contract) =>
    new(
        contract,
        new RuntimeCallbacks(
            OnStateEntered: state => Console.WriteLine($"Enter {state}"),
            OnStateChanged: (from, to) => Console.WriteLine($"Transition {from} -> {to}"),
            OnInputBlocked: action => Console.WriteLine($"Blocked input: {action}"),
            OnSceneRequested: scene => Console.WriteLine($"Request scene: {scene}")));

Console.WriteLine("== PauseMenuReference ==");
var pauseRuntime = BuildRuntime(ReferenceProfiles.Load(ReferenceProfile.PauseMenu));
Console.WriteLine($"Contract source: {ReferenceProfiles.BundledPath(ReferenceProfile.PauseMenu)}");
pauseRuntime.SetPredicate("can_confirm", true);
pauseRuntime.SetPredicate("can_cancel", true);
pauseRuntime.SetPredicate("has_previous_tab", true);
pauseRuntime.SetPredicate("has_next_tab", true);
pauseRuntime.Dispatch(RuntimeEventType.ResourcesReady);
pauseRuntime.Tick(0.4);
PrintPrompts(pauseRuntime);
pauseRuntime.RequestAction(InputAction.MoveNext);
pauseRuntime.Tick(0.4);
PrintLayers(pauseRuntime);

Console.WriteLine();
Console.WriteLine("== AutosaveToastReference ==");
var toastRuntime = BuildRuntime(ReferenceProfiles.Load(ReferenceProfile.AutosaveToast));
Console.WriteLine($"Contract source: {ReferenceProfiles.BundledPath(ReferenceProfile.AutosaveToast)}");
toastRuntime.Dispatch(RuntimeEventType.ResourcesReady);
PrintLayers(toastRuntime);
toastRuntime.Tick(0.3);
toastRuntime.Tick(3.1);
toastRuntime.Tick(0.3);
Console.WriteLine($"Final state: {toastRuntime.State}");

Console.WriteLine();
Console.WriteLine("== SubtitleCutsceneReference ==");
var subtitleRuntime = BuildRuntime(ReferenceProfiles.Load(ReferenceProfile.SubtitleCutscene));
Console.WriteLine($"Contract source: {ReferenceProfiles.BundledPath(ReferenceProfile.SubtitleCutscene)}");
subtitleRuntime.SetPredicate("can_confirm", true);
subtitleRuntime.SetPredicate("can_cancel", true);
subtitleRuntime.SetPredicate("has_previous_scene", true);
subtitleRuntime.SetPredicate("has_next_scene", true);
subtitleRuntime.SetPredicate("subtitles_enabled", true);
subtitleRuntime.Dispatch(RuntimeEventType.ResourcesReady);
subtitleRuntime.Tick(0.55);
PrintPrompts(subtitleRuntime);
subtitleRuntime.RequestAction(InputAction.Confirm);
subtitleRuntime.Tick(2.6);
PrintLayers(subtitleRuntime);
Console.WriteLine($"Final state: {subtitleRuntime.State}");

Console.WriteLine();
Console.WriteLine("== SonicStageHudReference ==");
var sonicHudRuntime = BuildRuntime(ReferenceProfiles.Load(ReferenceProfile.SonicStageHud));
Console.WriteLine($"Contract source: {ReferenceProfiles.BundledPath(ReferenceProfile.SonicStageHud)}");
sonicHudRuntime.SetPredicate("can_cycle_left", true);
sonicHudRuntime.SetPredicate("can_cycle_right", true);
sonicHudRuntime.SetPredicate("can_burst", true);
sonicHudRuntime.SetPredicate("can_hide", true);
sonicHudRuntime.Dispatch(RuntimeEventType.ResourcesReady);
sonicHudRuntime.Tick(0.4);
PrintPrompts(sonicHudRuntime);
sonicHudRuntime.RequestAction(InputAction.Confirm);
sonicHudRuntime.Tick(0.5);
PrintLayers(sonicHudRuntime);
Console.WriteLine($"Final state: {sonicHudRuntime.State}");

Console.WriteLine();
Console.WriteLine("== WerehogStageHudReference ==");
var werehogHudRuntime = BuildRuntime(ReferenceProfiles.Load(ReferenceProfile.WerehogStageHud));
Console.WriteLine($"Contract source: {ReferenceProfiles.BundledPath(ReferenceProfile.WerehogStageHud)}");
werehogHudRuntime.SetPredicate("can_cycle_target", true);
werehogHudRuntime.SetPredicate("can_cycle_lane", true);
werehogHudRuntime.SetPredicate("can_trigger_claw", true);
werehogHudRuntime.SetPredicate("can_hide", true);
werehogHudRuntime.Dispatch(RuntimeEventType.ResourcesReady);
werehogHudRuntime.Tick(0.5);
PrintPrompts(werehogHudRuntime);
werehogHudRuntime.RequestAction(InputAction.MoveNext);
werehogHudRuntime.Tick(0.25);
PrintLayers(werehogHudRuntime);
Console.WriteLine($"Final state: {werehogHudRuntime.State}");
