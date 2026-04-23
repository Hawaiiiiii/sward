#pragma once

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sward::ui_runtime
{
enum class ScreenState
{
    Boot,
    Intro,
    Idle,
    Navigate,
    Confirm,
    Cancel,
    Outro,
    Closed,
};

enum class InputAction
{
    MovePrevious,
    MoveNext,
    PageLeft,
    PageRight,
    Confirm,
    Cancel,
};

enum class RuntimeEventType
{
    ResourcesReady,
    AnimationFinished,
    ActionCompletedKeepOpen,
    ActionCompletedClose,
    HostForceClose,
    Timeout,
};

enum class PromptButton
{
    A,
    B,
    X,
    Y,
    LB,
    RB,
    Start,
    Unknown,
};

struct TimelineBand
{
    std::string id;
    double seconds = 0.0;
};

struct StateDefinition
{
    ScreenState state = ScreenState::Boot;
    std::string debugName;
    std::string enterScene;
    std::optional<std::string> timelineBandId;
    std::optional<ScreenState> timeoutTarget;
    bool inputEnabled = false;
};

struct OverlayLayer
{
    std::string id;
    std::string role;
    bool interactive = false;
};

struct PromptSlot
{
    std::string slotId;
    PromptButton button = PromptButton::Unknown;
    std::string label;
    std::set<ScreenState> visibleStates;
    std::vector<std::string> requiredPredicates;
};

struct ScreenContract
{
    std::string screenId;
    std::map<ScreenState, StateDefinition> states;
    std::unordered_map<std::string, TimelineBand> timelineBands;
    std::vector<OverlayLayer> overlayLayers;
    std::unordered_map<ScreenState, std::set<std::string>> visibleOverlayRoles;
    std::vector<PromptSlot> promptSlots;
};

struct PromptSlotView
{
    std::string slotId;
    PromptButton button = PromptButton::Unknown;
    std::string label;
};

struct RuntimeCallbacks
{
    std::function<void(ScreenState)> onStateEntered;
    std::function<void(ScreenState, ScreenState)> onStateChanged;
    std::function<void(InputAction)> onInputBlocked;
    std::function<void(std::string_view)> onSceneRequested;
};

class ButtonPromptModel
{
public:
    explicit ButtonPromptModel(std::vector<PromptSlot> slots = {});

    void setPredicate(std::string predicateId, bool value);
    [[nodiscard]] bool predicate(std::string_view predicateId) const;
    [[nodiscard]] std::vector<PromptSlotView> visibleSlots(ScreenState state, bool inputLocked) const;

private:
    std::vector<PromptSlot> m_slots;
    std::unordered_map<std::string, bool> m_predicates;
};

class OverlayStackRuntime
{
public:
    explicit OverlayStackRuntime(ScreenContract contract = {});

    [[nodiscard]] std::vector<OverlayLayer> visibleLayers(ScreenState state) const;

private:
    ScreenContract m_contract;
};

class ScreenRuntime
{
public:
    explicit ScreenRuntime(ScreenContract contract, RuntimeCallbacks callbacks = {});

    [[nodiscard]] const std::string& screenId() const;
    [[nodiscard]] ScreenState state() const;
    [[nodiscard]] double stateElapsedSeconds() const;
    [[nodiscard]] bool isInputLocked() const;

    bool requestAction(InputAction action);
    bool dispatch(RuntimeEventType event);
    void tick(double deltaSeconds);

    void setPredicate(const std::string& predicateId, bool value);
    [[nodiscard]] std::vector<PromptSlotView> visiblePrompts() const;
    [[nodiscard]] std::vector<OverlayLayer> visibleLayers() const;

private:
    [[nodiscard]] const StateDefinition& currentDefinition() const;
    [[nodiscard]] const TimelineBand* currentTimelineBand() const;
    void transitionTo(ScreenState nextState);
    [[nodiscard]] std::optional<ScreenState> eventTarget(RuntimeEventType event) const;

    ScreenContract m_contract;
    RuntimeCallbacks m_callbacks;
    ButtonPromptModel m_promptModel;
    OverlayStackRuntime m_overlayRuntime;
    ScreenState m_state = ScreenState::Boot;
    double m_stateElapsedSeconds = 0.0;
};

[[nodiscard]] std::string_view toString(ScreenState state);
[[nodiscard]] std::string_view toString(InputAction action);
[[nodiscard]] std::string_view toString(PromptButton button);
} // namespace sward::ui_runtime
