// Phase 320: SGFX-shaped port of CTitleStateIntro::Update.
//
// Real captured behavior (phase315_take2 trace, title-intro-context
// samples between frames 2458..5361):
//   * `requested_state`: 0 -> 1 over a 2-frame window (~50 ms),
//     then stays at 1 until the menu state takes over at frame 5362.
//   * `dirty`: always 0
//   * `transition_armed`: always 0 (the intro state never fires its
//     own transition; the menu state machine transitions away once
//     the user input arrives).
//   * `elapsed_ms`: a free-running timer that increments each frame.
//
// Conclusion: the intro state is trivial in retail SU. It's a brief
// "logo / press start" idle period before the user-interactive
// menu becomes available. SGFX models it as two real-captured
// sub-states (LogoFadeIn and PressStartIdle) plus the timer.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class TitleIntroState : std::uint8_t
    {
        LogoFadeIn      = 0, // requested_state == 0; ~50 ms
        PressStartIdle  = 1, // requested_state == 1; idle until input
    };

    enum class TitleIntroEventKind : std::uint8_t
    {
        StateAdvanced,
        PressStartArmed,    // user input observed; menu about to take over
    };

    struct TitleIntroInput
    {
        bool startTapped = false;
        bool acceptTapped = false;
        float deltaSeconds = 0.0f;
    };

    struct TitleIntroSlot
    {
        TitleIntroState requestedState = TitleIntroState::LogoFadeIn;
        bool            dirty = false;            // always-0 in captured trace
        bool            transitionArmed = false;  // always-0 in captured trace
        float           elapsedSeconds = 0.0f;    // mirrors elapsed_ms / 1000
        // Phase 320 captured timing: state advance happened ~50 ms
        // after first sample; SGFX uses 60 ms as a nominal threshold
        // so a host running at 30 fps still sees the advance fire.
        float           logoFadeInDurationSeconds = 0.060f;
    };

    struct TitleIntroEvent
    {
        TitleIntroEventKind kind = TitleIntroEventKind::StateAdvanced;
        TitleIntroState     newState = TitleIntroState::LogoFadeIn;
        std::string         sfxCueName;
    };

    inline std::vector<TitleIntroEvent> updateTitleIntroOneFrame(
        TitleIntroSlot& slot,
        const TitleIntroInput& input)
    {
        std::vector<TitleIntroEvent> events;
        slot.elapsedSeconds += input.deltaSeconds;

        if (slot.requestedState == TitleIntroState::LogoFadeIn
            && slot.elapsedSeconds >= slot.logoFadeInDurationSeconds)
        {
            slot.requestedState = TitleIntroState::PressStartIdle;
            events.push_back({TitleIntroEventKind::StateAdvanced,
                              TitleIntroState::PressStartIdle, ""});
        }
        // PressStartIdle: capture the start/accept tap so the host
        // can transition to the menu state machine. transition_armed
        // stays 0 in captured trace because the actual transition is
        // owned by the menu state, not the intro -- the intro just
        // signals "user has acknowledged".
        else if (slot.requestedState == TitleIntroState::PressStartIdle
                 && (input.startTapped || input.acceptTapped))
        {
            events.push_back({TitleIntroEventKind::PressStartArmed,
                              TitleIntroState::PressStartIdle, ""});
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
