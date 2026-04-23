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
var pauseRuntime = BuildRuntime(ReferenceProfiles.BuildPauseMenu());
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
var toastRuntime = BuildRuntime(ReferenceProfiles.BuildAutosaveToast());
toastRuntime.Dispatch(RuntimeEventType.ResourcesReady);
PrintLayers(toastRuntime);
toastRuntime.Tick(0.3);
toastRuntime.Tick(3.1);
toastRuntime.Tick(0.3);
Console.WriteLine($"Final state: {toastRuntime.State}");
