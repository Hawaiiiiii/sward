// Phase 305: SGFX-shaped port of SWA's loading-screen state machine.
//
// Sourced from research_uiux/runtime_reference/include/sward/ui_runtime/
// sgfx_hud_cloading.generated.h (the offsets-of decoded layout) and
// from the in-process render of ui_loading.yncp scenes (loadinfo,
// bg_1, etc.) confirmed in Phase 302's catalog.
//
// The retail loading screen has three observable states:
//   * Booting: animated progress, instructions panel visible.
//   * Ready: "Press <button> to continue" prompt, idle until input.
//   * Dismissed: fading out, transitioning to the next screen.
// SGFX's loading-screen state machine matches that progression
// exactly so the host can swap retail's progress signal for any
// other "things still loading" predicate.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Phase 324 retail-fidelity: SWA::CLoading carries an
    // ELoadingDisplayType enum + an m_IsNightToDay bool (mined from
    // local_build_env/.../api/SWA/HUD/Loading/Loading.h). The earlier
    // SGFX Booting/Ready/Dismissed phase enum was invented; the real
    // runtime selects one of 9 display variants and a Day<->Night
    // direction. SGFX now mirrors the retail enum exactly.
    enum class LoadingDisplayType : std::uint32_t
    {
        MilesElectric        = 0, // Tails' computer interface (controller-image loading screen)
        None                 = 1, // suppress the loading display entirely
        WerehogMovie         = 2, // Day -> Werehog transition cinematic
        MilesElectricContext = 3, // contextual MilesElectric variant
        Arrows               = 4, // the chevron-arrow loading panel
        NowLoading           = 5, // generic "Now Loading" text
        EventGallery         = 6, // event-gallery cutscene
        ChangeTimeOfDay      = 7, // Day <-> Night swap
        Blank                = 8, // empty / black
    };

    // Phase 324 keeps the high-level phase enum for hosts that want
    // a coarse Booting/Ready/Dismissed view, but it is now layered
    // ON TOP of the retail-fidelity LoadingDisplayType -- the real
    // loading screen state is the (LoadingDisplayType, IsNightToDay)
    // tuple, not an arbitrary three-step machine.
    enum class LoadingPhase : std::uint8_t
    {
        Booting    = 0,
        Ready      = 1,
        Dismissed  = 2,
    };

    enum class LoadingEventKind : std::uint8_t
    {
        ProgressTicked,
        ReachedReady,
        DismissAccepted,
        FadeComplete,
    };

    struct LoadingInput
    {
        bool acceptTapped = false; // A or Start dismisses the Ready prompt
        float deltaSeconds = 0.0f; // for the Dismissed-fade tick
    };

    struct LoadingState
    {
        LoadingPhase phase = LoadingPhase::Booting;
        // Phase 324: real retail fields.
        LoadingDisplayType displayType = LoadingDisplayType::NowLoading;
        bool         isNightToDay = false; // mirrors CLoading::m_IsNightToDay
        bool         isVisible = false;    // mirrors CLoading::m_IsVisible
        // SGFX-side bookkeeping for the higher-level phase machine.
        float        progress = 0.0f;       // 0..1, host-driven
        bool         showInstructions = true;
        bool         showPressStart = false;
        float        fadeOutSeconds = 0.0f; // counts up while in Dismissed
        float        fadeOutDurationSeconds = 0.5f;
    };

    struct LoadingEvent
    {
        LoadingEventKind kind = LoadingEventKind::ProgressTicked;
        std::string      sfxCueName;
    };

    // Phase 311 fix-up: sys_loading_ready was invented; not in source.
    // Empty = host wires to its own audio. sys_worldmap_decide IS
    // mined-real (see CTitleStateMenu_patches.cpp:175).
    constexpr std::string_view kLoadingSfxReady   = "";                  // unverified
    constexpr std::string_view kLoadingSfxConfirm = "sys_worldmap_decide";

    // Host calls this each frame and provides the latest progress
    // value (0..1). Returns events the host should react to (start
    // playing the ready cue, fade music out at FadeComplete, etc.).
    inline std::vector<LoadingEvent> updateLoadingScreenOneFrame(
        LoadingState& state,
        const LoadingInput& input,
        float hostProgress)
    {
        std::vector<LoadingEvent> events;
        const float prev = state.progress;
        if (hostProgress < 0.0f) hostProgress = 0.0f;
        if (hostProgress > 1.0f) hostProgress = 1.0f;
        state.progress = hostProgress;

        if (state.phase == LoadingPhase::Booting)
        {
            if (state.progress > prev + 0.0001f)
                events.push_back({LoadingEventKind::ProgressTicked, ""});
            if (state.progress >= 1.0f)
            {
                state.phase = LoadingPhase::Ready;
                state.showInstructions = false;
                state.showPressStart = true;
                events.push_back({LoadingEventKind::ReachedReady,
                                  std::string(kLoadingSfxReady)});
            }
        }
        else if (state.phase == LoadingPhase::Ready)
        {
            if (input.acceptTapped)
            {
                state.phase = LoadingPhase::Dismissed;
                state.showPressStart = false;
                state.fadeOutSeconds = 0.0f;
                events.push_back({LoadingEventKind::DismissAccepted,
                                  std::string(kLoadingSfxConfirm)});
            }
        }
        else if (state.phase == LoadingPhase::Dismissed)
        {
            state.fadeOutSeconds += input.deltaSeconds;
            if (state.fadeOutSeconds >= state.fadeOutDurationSeconds)
            {
                events.push_back({LoadingEventKind::FadeComplete, ""});
            }
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
