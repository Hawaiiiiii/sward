using System.Collections.ObjectModel;

namespace Sward.UiRuntime.Reference;

public enum ScreenState
{
    Boot,
    Intro,
    Idle,
    Navigate,
    Confirm,
    Cancel,
    Outro,
    Closed,
}

public enum InputAction
{
    MovePrevious,
    MoveNext,
    PageLeft,
    PageRight,
    Confirm,
    Cancel,
}

public enum RuntimeEventType
{
    ResourcesReady,
    AnimationFinished,
    ActionCompletedKeepOpen,
    ActionCompletedClose,
    HostForceClose,
    Timeout,
}

public enum PromptButton
{
    A,
    B,
    X,
    Y,
    LB,
    RB,
    Start,
    Unknown,
}

public sealed record TimelineBand(string Id, double Seconds);
public sealed record StateDefinition(ScreenState State, string DebugName, string EnterScene, string? TimelineBandId, ScreenState? TimeoutTarget, bool InputEnabled);
public sealed record OverlayLayer(string Id, string Role, bool Interactive);
public sealed record PromptSlot(string SlotId, PromptButton Button, string Label, IReadOnlySet<ScreenState> VisibleStates, IReadOnlyList<string> RequiredPredicates);
public sealed record PromptSlotView(string SlotId, PromptButton Button, string Label);
public sealed record RuntimeCallbacks(Action<ScreenState>? OnStateEntered = null, Action<ScreenState, ScreenState>? OnStateChanged = null, Action<InputAction>? OnInputBlocked = null, Action<string>? OnSceneRequested = null);

public sealed class ScreenContract
{
    public required string ScreenId { get; init; }
    public required IReadOnlyDictionary<ScreenState, StateDefinition> States { get; init; }
    public required IReadOnlyDictionary<string, TimelineBand> TimelineBands { get; init; }
    public required IReadOnlyList<OverlayLayer> OverlayLayers { get; init; }
    public required IReadOnlyDictionary<ScreenState, IReadOnlySet<string>> VisibleOverlayRoles { get; init; }
    public required IReadOnlyList<PromptSlot> PromptSlots { get; init; }
}

public sealed class ScreenRuntime
{
    private readonly ScreenContract _contract;
    private readonly RuntimeCallbacks _callbacks;
    private readonly Dictionary<string, bool> _predicates = new();

    public ScreenRuntime(ScreenContract contract, RuntimeCallbacks? callbacks = null)
    {
        _contract = contract;
        _callbacks = callbacks ?? new RuntimeCallbacks();

        if (!_contract.States.ContainsKey(ScreenState.Boot))
            throw new InvalidOperationException("Screen contract must define the Boot state.");

        State = ScreenState.Boot;
        _callbacks.OnStateEntered?.Invoke(State);
        RequestSceneIfNeeded();
    }

    public ScreenState State { get; private set; }
    public double StateElapsedSeconds { get; private set; }
    public bool IsInputLocked => !CurrentDefinition.InputEnabled;

    public void SetPredicate(string predicateId, bool value)
    {
        _predicates[predicateId] = value;
    }

    public bool RequestAction(InputAction action)
    {
        if (IsInputLocked)
        {
            _callbacks.OnInputBlocked?.Invoke(action);
            return false;
        }

        return action switch
        {
            InputAction.MovePrevious or InputAction.MoveNext or InputAction.PageLeft or InputAction.PageRight => TransitionTo(ScreenState.Navigate),
            InputAction.Confirm => TransitionTo(ScreenState.Confirm),
            InputAction.Cancel => TransitionTo(ScreenState.Cancel),
            _ => false,
        };
    }

    public bool Dispatch(RuntimeEventType runtimeEvent)
    {
        var target = EventTarget(runtimeEvent);
        if (target is null)
            return false;

        return TransitionTo(target.Value);
    }

    public void Tick(double deltaSeconds)
    {
        if (deltaSeconds <= 0.0)
            return;

        StateElapsedSeconds += deltaSeconds;

        if (CurrentDefinition.TimeoutTarget is null || CurrentTimelineBand is null)
            return;

        if (StateElapsedSeconds >= CurrentTimelineBand.Seconds)
            TransitionTo(CurrentDefinition.TimeoutTarget.Value);
    }

    public IReadOnlyList<PromptSlotView> VisiblePrompts()
    {
        if (IsInputLocked)
            return Array.Empty<PromptSlotView>();

        var visible = new List<PromptSlotView>();
        foreach (var slot in _contract.PromptSlots)
        {
            if (slot.VisibleStates.Count > 0 && !slot.VisibleStates.Contains(State))
                continue;

            if (slot.RequiredPredicates.Any(predicateId => !_predicates.TryGetValue(predicateId, out var value) || !value))
                continue;

            visible.Add(new PromptSlotView(slot.SlotId, slot.Button, slot.Label));
        }

        return new ReadOnlyCollection<PromptSlotView>(visible);
    }

    public IReadOnlyList<OverlayLayer> VisibleLayers()
    {
        if (!_contract.VisibleOverlayRoles.TryGetValue(State, out var visibleRoles))
            return Array.Empty<OverlayLayer>();

        return new ReadOnlyCollection<OverlayLayer>(_contract.OverlayLayers.Where(layer => visibleRoles.Contains(layer.Role)).ToList());
    }

    private StateDefinition CurrentDefinition => _contract.States[State];

    private TimelineBand? CurrentTimelineBand =>
        CurrentDefinition.TimelineBandId is null ? null : _contract.TimelineBands.GetValueOrDefault(CurrentDefinition.TimelineBandId);

    private bool TransitionTo(ScreenState nextState)
    {
        if (State == nextState)
            return false;

        var previous = State;
        State = nextState;
        StateElapsedSeconds = 0.0;

        _callbacks.OnStateChanged?.Invoke(previous, nextState);
        _callbacks.OnStateEntered?.Invoke(nextState);
        RequestSceneIfNeeded();
        return true;
    }

    private ScreenState? EventTarget(RuntimeEventType runtimeEvent)
    {
        return State switch
        {
            ScreenState.Boot when runtimeEvent == RuntimeEventType.ResourcesReady => ScreenState.Intro,
            ScreenState.Intro when runtimeEvent is RuntimeEventType.AnimationFinished or RuntimeEventType.Timeout => ScreenState.Idle,
            ScreenState.Intro when runtimeEvent == RuntimeEventType.HostForceClose => ScreenState.Outro,
            ScreenState.Idle when runtimeEvent == RuntimeEventType.HostForceClose => ScreenState.Outro,
            ScreenState.Navigate when runtimeEvent is RuntimeEventType.AnimationFinished or RuntimeEventType.Timeout => ScreenState.Idle,
            ScreenState.Navigate when runtimeEvent == RuntimeEventType.HostForceClose => ScreenState.Outro,
            ScreenState.Confirm when runtimeEvent == RuntimeEventType.ActionCompletedKeepOpen => ScreenState.Idle,
            ScreenState.Confirm when runtimeEvent is RuntimeEventType.ActionCompletedClose or RuntimeEventType.HostForceClose or RuntimeEventType.Timeout => ScreenState.Outro,
            ScreenState.Cancel when runtimeEvent is RuntimeEventType.AnimationFinished or RuntimeEventType.Timeout or RuntimeEventType.HostForceClose => ScreenState.Outro,
            ScreenState.Outro when runtimeEvent is RuntimeEventType.AnimationFinished or RuntimeEventType.Timeout => ScreenState.Closed,
            _ => null,
        };
    }

    private void RequestSceneIfNeeded()
    {
        if (!string.IsNullOrWhiteSpace(CurrentDefinition.EnterScene))
            _callbacks.OnSceneRequested?.Invoke(CurrentDefinition.EnterScene);
    }
}
