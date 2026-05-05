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
        AttractMovie    = 2, // CGameModeStageTitle attract-movie playing
    };

    enum class TitleIntroEventKind : std::uint8_t
    {
        StateAdvanced,
        PressStartArmed,        // user input observed; menu about to take over
        AttractMovieStarted,    // m_AdvertiseMovieWaitTime elapsed -> demo plays
        AttractMovieDismissed,  // user input during attract -> back to PressStartIdle
    };

    struct TitleIntroInput
    {
        bool startTapped = false;
        bool acceptTapped = false;
        bool anyInputThisFrame = false; // any key/pad press dismisses attract
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

        // Phase 328: SWA::CGameModeStageTitle field mirror.
        // Sourced from
        //   local_build_env/.../api/SWA/System/GameMode/GameModeStageTitle.h
        //
        // CGameModeStageTitle layout:
        //   <pad +0x0E from CGameModeStage>
        //   bool m_IsPlayingAdvertiseMovie;
        //   be<float> m_AdvertiseMovieWaitTime;
        //
        // m_AdvertiseMovieWaitTime is the idle countdown (in seconds)
        // before the title attract-movie auto-plays; the retail value
        // observed in xex2 disasm is in the 30-60 s range. We mirror
        // the timer + the playing flag here so SGFX hosts can replay
        // the same attract behavior.
        bool  isPlayingAdvertiseMovie = false;     // m_IsPlayingAdvertiseMovie
        float advertiseMovieWaitTime  = 30.0f;     // m_AdvertiseMovieWaitTime (seconds)
        float advertiseMovieIdleSeconds = 0.0f;    // running idle counter; resets on input
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
        else if (slot.requestedState == TitleIntroState::PressStartIdle)
        {
            if (input.startTapped || input.acceptTapped)
            {
                slot.advertiseMovieIdleSeconds = 0.0f;
                events.push_back({TitleIntroEventKind::PressStartArmed,
                                  TitleIntroState::PressStartIdle, ""});
            }
            else
            {
                // Phase 328: track idle time toward attract-movie
                // auto-play. Any unrelated input also resets via
                // anyInputThisFrame so attract doesn't kick in mid-
                // navigation.
                if (input.anyInputThisFrame)
                    slot.advertiseMovieIdleSeconds = 0.0f;
                else
                    slot.advertiseMovieIdleSeconds += input.deltaSeconds;

                if (slot.advertiseMovieIdleSeconds >= slot.advertiseMovieWaitTime)
                {
                    slot.requestedState = TitleIntroState::AttractMovie;
                    slot.isPlayingAdvertiseMovie = true;
                    slot.advertiseMovieIdleSeconds = 0.0f;
                    events.push_back({TitleIntroEventKind::AttractMovieStarted,
                                      TitleIntroState::AttractMovie, ""});
                }
            }
        }
        else if (slot.requestedState == TitleIntroState::AttractMovie
                 && (input.startTapped || input.acceptTapped
                     || input.anyInputThisFrame))
        {
            slot.requestedState = TitleIntroState::PressStartIdle;
            slot.isPlayingAdvertiseMovie = false;
            slot.advertiseMovieIdleSeconds = 0.0f;
            events.push_back({TitleIntroEventKind::AttractMovieDismissed,
                              TitleIntroState::PressStartIdle, ""});
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
