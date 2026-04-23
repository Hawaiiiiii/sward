#include <sward/ui_runtime/runtime.hpp>

#include <algorithm>
#include <stdexcept>

namespace sward::ui_runtime
{
namespace
{
bool contains(const std::set<ScreenState>& haystack, ScreenState needle)
{
    return haystack.find(needle) != haystack.end();
}
} // namespace

ButtonPromptModel::ButtonPromptModel(std::vector<PromptSlot> slots)
    : m_slots(std::move(slots))
{
}

void ButtonPromptModel::setPredicate(std::string predicateId, bool value)
{
    m_predicates[std::move(predicateId)] = value;
}

bool ButtonPromptModel::predicate(std::string_view predicateId) const
{
    auto found = m_predicates.find(std::string(predicateId));
    return found != m_predicates.end() && found->second;
}

std::vector<PromptSlotView> ButtonPromptModel::visibleSlots(ScreenState state, bool inputLocked) const
{
    if (inputLocked)
        return {};

    std::vector<PromptSlotView> visible;

    for (const auto& slot : m_slots)
    {
        if (!slot.visibleStates.empty() && !contains(slot.visibleStates, state))
            continue;

        bool allowed = true;
        for (const auto& predicateId : slot.requiredPredicates)
        {
            auto found = m_predicates.find(predicateId);
            if (found == m_predicates.end() || !found->second)
            {
                allowed = false;
                break;
            }
        }

        if (!allowed)
            continue;

        visible.push_back({ slot.slotId, slot.button, slot.label });
    }

    return visible;
}

OverlayStackRuntime::OverlayStackRuntime(ScreenContract contract)
    : m_contract(std::move(contract))
{
}

std::vector<OverlayLayer> OverlayStackRuntime::visibleLayers(ScreenState state) const
{
    std::vector<OverlayLayer> visible;

    auto found = m_contract.visibleOverlayRoles.find(state);
    if (found == m_contract.visibleOverlayRoles.end())
        return visible;

    for (const auto& layer : m_contract.overlayLayers)
    {
        if (found->second.find(layer.role) != found->second.end())
            visible.push_back(layer);
    }

    return visible;
}

ScreenRuntime::ScreenRuntime(ScreenContract contract, RuntimeCallbacks callbacks)
    : m_contract(std::move(contract))
    , m_callbacks(std::move(callbacks))
    , m_promptModel(m_contract.promptSlots)
    , m_overlayRuntime(m_contract)
{
    if (m_contract.states.find(ScreenState::Boot) == m_contract.states.end())
        throw std::runtime_error("ScreenContract must define Boot state.");

    if (m_callbacks.onStateEntered)
        m_callbacks.onStateEntered(m_state);

    if (m_callbacks.onSceneRequested && !currentDefinition().enterScene.empty())
        m_callbacks.onSceneRequested(currentDefinition().enterScene);
}

const std::string& ScreenRuntime::screenId() const
{
    return m_contract.screenId;
}

ScreenState ScreenRuntime::state() const
{
    return m_state;
}

double ScreenRuntime::stateElapsedSeconds() const
{
    return m_stateElapsedSeconds;
}

bool ScreenRuntime::isInputLocked() const
{
    return !currentDefinition().inputEnabled;
}

bool ScreenRuntime::requestAction(InputAction action)
{
    if (isInputLocked())
    {
        if (m_callbacks.onInputBlocked)
            m_callbacks.onInputBlocked(action);
        return false;
    }

    switch (action)
    {
    case InputAction::MovePrevious:
    case InputAction::MoveNext:
    case InputAction::PageLeft:
    case InputAction::PageRight:
        transitionTo(ScreenState::Navigate);
        return true;
    case InputAction::Confirm:
        transitionTo(ScreenState::Confirm);
        return true;
    case InputAction::Cancel:
        transitionTo(ScreenState::Cancel);
        return true;
    default:
        return false;
    }
}

bool ScreenRuntime::dispatch(RuntimeEventType event)
{
    auto target = eventTarget(event);
    if (!target.has_value())
        return false;

    transitionTo(*target);
    return true;
}

void ScreenRuntime::tick(double deltaSeconds)
{
    if (deltaSeconds <= 0.0)
        return;

    m_stateElapsedSeconds += deltaSeconds;

    const auto* band = currentTimelineBand();
    if (!band || !currentDefinition().timeoutTarget.has_value())
        return;

    if (m_stateElapsedSeconds >= band->seconds)
        transitionTo(*currentDefinition().timeoutTarget);
}

void ScreenRuntime::setPredicate(const std::string& predicateId, bool value)
{
    m_promptModel.setPredicate(predicateId, value);
}

std::vector<PromptSlotView> ScreenRuntime::visiblePrompts() const
{
    return m_promptModel.visibleSlots(m_state, isInputLocked());
}

std::vector<OverlayLayer> ScreenRuntime::visibleLayers() const
{
    return m_overlayRuntime.visibleLayers(m_state);
}

const StateDefinition& ScreenRuntime::currentDefinition() const
{
    auto found = m_contract.states.find(m_state);
    if (found == m_contract.states.end())
        throw std::runtime_error("Current state is missing from contract.");
    return found->second;
}

const TimelineBand* ScreenRuntime::currentTimelineBand() const
{
    if (!currentDefinition().timelineBandId.has_value())
        return nullptr;

    auto found = m_contract.timelineBands.find(*currentDefinition().timelineBandId);
    if (found == m_contract.timelineBands.end())
        return nullptr;

    return &found->second;
}

void ScreenRuntime::transitionTo(ScreenState nextState)
{
    if (m_state == nextState)
        return;

    ScreenState previous = m_state;
    m_state = nextState;
    m_stateElapsedSeconds = 0.0;

    if (m_callbacks.onStateChanged)
        m_callbacks.onStateChanged(previous, nextState);

    if (m_callbacks.onStateEntered)
        m_callbacks.onStateEntered(nextState);

    if (m_callbacks.onSceneRequested && !currentDefinition().enterScene.empty())
        m_callbacks.onSceneRequested(currentDefinition().enterScene);
}

std::optional<ScreenState> ScreenRuntime::eventTarget(RuntimeEventType event) const
{
    switch (m_state)
    {
    case ScreenState::Boot:
        if (event == RuntimeEventType::ResourcesReady)
            return ScreenState::Intro;
        break;
    case ScreenState::Intro:
        if (event == RuntimeEventType::AnimationFinished || event == RuntimeEventType::Timeout)
            return ScreenState::Idle;
        if (event == RuntimeEventType::HostForceClose)
            return ScreenState::Outro;
        break;
    case ScreenState::Idle:
        if (event == RuntimeEventType::HostForceClose)
            return ScreenState::Outro;
        break;
    case ScreenState::Navigate:
        if (event == RuntimeEventType::AnimationFinished || event == RuntimeEventType::Timeout)
            return ScreenState::Idle;
        if (event == RuntimeEventType::HostForceClose)
            return ScreenState::Outro;
        break;
    case ScreenState::Confirm:
        if (event == RuntimeEventType::ActionCompletedKeepOpen)
            return ScreenState::Idle;
        if (event == RuntimeEventType::ActionCompletedClose || event == RuntimeEventType::HostForceClose || event == RuntimeEventType::Timeout)
            return ScreenState::Outro;
        break;
    case ScreenState::Cancel:
        if (event == RuntimeEventType::AnimationFinished || event == RuntimeEventType::Timeout || event == RuntimeEventType::HostForceClose)
            return ScreenState::Outro;
        break;
    case ScreenState::Outro:
        if (event == RuntimeEventType::AnimationFinished || event == RuntimeEventType::Timeout)
            return ScreenState::Closed;
        break;
    case ScreenState::Closed:
        break;
    }

    return std::nullopt;
}

std::string_view toString(ScreenState state)
{
    switch (state)
    {
    case ScreenState::Boot:
        return "Boot";
    case ScreenState::Intro:
        return "Intro";
    case ScreenState::Idle:
        return "Idle";
    case ScreenState::Navigate:
        return "Navigate";
    case ScreenState::Confirm:
        return "Confirm";
    case ScreenState::Cancel:
        return "Cancel";
    case ScreenState::Outro:
        return "Outro";
    case ScreenState::Closed:
        return "Closed";
    }
    return "Unknown";
}

std::string_view toString(InputAction action)
{
    switch (action)
    {
    case InputAction::MovePrevious:
        return "MovePrevious";
    case InputAction::MoveNext:
        return "MoveNext";
    case InputAction::PageLeft:
        return "PageLeft";
    case InputAction::PageRight:
        return "PageRight";
    case InputAction::Confirm:
        return "Confirm";
    case InputAction::Cancel:
        return "Cancel";
    }
    return "Unknown";
}

std::string_view toString(PromptButton button)
{
    switch (button)
    {
    case PromptButton::A:
        return "A";
    case PromptButton::B:
        return "B";
    case PromptButton::X:
        return "X";
    case PromptButton::Y:
        return "Y";
    case PromptButton::LB:
        return "LB";
    case PromptButton::RB:
        return "RB";
    case PromptButton::Start:
        return "Start";
    case PromptButton::Unknown:
        return "Unknown";
    }
    return "Unknown";
}
} // namespace sward::ui_runtime
