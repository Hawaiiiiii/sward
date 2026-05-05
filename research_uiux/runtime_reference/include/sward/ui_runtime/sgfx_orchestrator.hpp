// Phase 310: SGFX top-level orchestrator.
//
// The thing that actually IS the playable native UI/UX. Wires
// every screen-state machine ported in Phases 303-309 into one
// state-of-the-application graph and routes input + events
// between them.
//
// Screen graph (matches what the retail Sonic Unleashed flow
// looks like + the user-stated SGFX target):
//
//     Title menu  --(NewGame/Continue accepted)-->  World Map
//     Title menu  --(Settings accepted)-->          [SettingsOverlay]
//     Title menu  --(Exit accepted)-->              [HostExit]
//
//     World Map   --(StageOpened)-->                Loading
//     World Map   --(BackedToTitle)-->              Title menu
//
//     Loading     --(FadeComplete)-->               StageHud (or Hub)
//
//     StageHud    --(PauseRequested)-->             PauseMenu (over StageHud)
//     StageHud    --(stage finished, host signal)-->  Results
//
//     PauseMenu   --(Quit transition)-->            World Map (or Hub if started from Hub)
//     PauseMenu   --(Hide transition)-->            previous screen
//
//     Results     --(Acknowledged)-->               World Map
//
//     Hub         --(StageEntryConfirmed)-->        Loading
//     Hub         --(host signal: leave hub)-->     World Map
//
// All transitions emit an SGFX::OrchestratorEvent so the host can
// trigger fades, swap CSD scenes, dispatch SFX, etc. The
// orchestrator owns the live state of EVERY child machine and
// runs only the active one each frame. Pure header, no globals.

#pragma once

#include "sgfx_title_intro.hpp"
#include "sgfx_title_menu.hpp"
#include "sgfx_pause_menu.hpp"
#include "sgfx_loading_screen.hpp"
#include "sgfx_stage_hud.hpp"
#include "sgfx_results_screen.hpp"
#include "sgfx_world_map.hpp"
#include "sgfx_hub_screen.hpp"
#include "sgfx_general_window.hpp"
#include "sgfx_save_icon.hpp"
#include "sgfx_sound_admin.hpp"
#include "sgfx_evil_hud_guide.hpp"
#include "sgfx_hud_evil_stage.hpp"
#include "sgfx_hud_status.hpp"
#include "sgfx_help_window.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class SgfxScreen : std::uint8_t
    {
        TitleIntro = 0,
        Title      = 1,
        WorldMap   = 2,
        Loading    = 3,
        StageHud   = 4,
        Pause      = 5,
        Results    = 6,
        Hub        = 7,
    };

    enum class SgfxOrchestratorEventKind : std::uint8_t
    {
        ScreenEntered,
        ScreenExited,
        SfxCueRequested,
        HostExitRequested, // user picked Exit from Title
    };

    struct SgfxOrchestratorEvent
    {
        SgfxOrchestratorEventKind kind = SgfxOrchestratorEventKind::ScreenEntered;
        SgfxScreen screen = SgfxScreen::Title;
        std::string sfxCueName;
    };

    // Per-frame input snapshot the orchestrator routes to whichever
    // screen is currently active.
    struct SgfxFrameInput
    {
        bool acceptTapped = false;
        bool cancelTapped = false;
        bool selectTapped = false;
        bool startTapped = false;
        bool upTapped = false;
        bool downTapped = false;
        bool leftTapped = false;
        bool rightTapped = false;
        bool talkTapped = false;
        bool openMapTapped = false;
        float deltaSeconds = 0.0f;
        // Host-supplied progress for loading; ignored on other screens.
        float loadingProgress = 0.0f;
    };

    struct SgfxOrchestrator
    {
        SgfxScreen current = SgfxScreen::TitleIntro;
        SgfxScreen previousNonOverlay = SgfxScreen::TitleIntro;

        // Each child machine's state. The orchestrator only ticks
        // the one matching `current`, but keeps all of them resident
        // so a Pause overlay can be popped without losing the
        // underlying StageHud values.
        TitleIntroSlot   titleIntro;
        TitleMenuState   title;
        WorldMapState    worldMap;
        LoadingState     loading;
        StageHudState    stageHud;
        PauseState       pause;
        ResultsState     results;
        HubState         hub;

        // Phase 334 / 341: cross-cutting overlays + audio admin that
        // any active screen can poke. They live on the orchestrator
        // so the host has one root to drive the whole UI/UX.
        EvilHudGuideState  evilHudGuide;     // Werehog QTE prompts
        GeneralWindowState generalWindow;    // modal confirm dialogs
        HelpWindowState    helpWindow;       // distinct help overlay (Phase 341)
        SaveIconState      saveIcon;         // save-disk overlay
        SgfxBgmAdmin       bgmAdmin;         // BGM channel volumes + cues
        // Phase 341: Werehog stage HUD overlay set. Active when
        // current = StageHud AND stageHud.mode == Werehog.
        EvilStageHudState  evilStageHud;
        // Phase 341: Status / Skill Upgrade overlay. Reachable as a
        // sub-state of Pause -> Status (host opens it; not in the
        // screen graph as a top-level destination).
        StatusState        statusOverlay;
    };

    namespace detail::orchestrator
    {
        inline SgfxOrchestratorEvent screenEntered(SgfxScreen s)
        {
            return {SgfxOrchestratorEventKind::ScreenEntered, s, ""};
        }
        inline SgfxOrchestratorEvent screenExited(SgfxScreen s)
        {
            return {SgfxOrchestratorEventKind::ScreenExited, s, ""};
        }
        inline SgfxOrchestratorEvent sfx(SgfxScreen s, std::string cue)
        {
            return {SgfxOrchestratorEventKind::SfxCueRequested, s, std::move(cue)};
        }

        // Phase 334: clear transient UI flags on the screen we're
        // about to enter so its state machine starts in a known
        // shape. Only resets per-screen UI latches, NOT user data
        // (ring count, score, save state, etc).
        inline void resetTransientStateOnEntry(SgfxOrchestrator& o,
                                               SgfxScreen next) noexcept
        {
            switch (next)
            {
            case SgfxScreen::WorldMap:
                // The stage-open confirmation panel must be closed
                // when we re-enter the world map (e.g. after Quit,
                // Results acknowledge, Hub leave).
                o.worldMap.stageOpenPanelVisible = false;
                break;
            case SgfxScreen::StageHud:
                // The paused latch is what gates Start re-opening
                // the pause overlay.
                o.stageHud.paused = false;
                break;
            case SgfxScreen::Loading:
                // Each load is a fresh boot/ready/dismissed cycle.
                o.loading = LoadingState{};
                break;
            default:
                break;
            }
        }

        inline void switchScreen(SgfxOrchestrator& o,
                                 SgfxScreen next,
                                 std::vector<SgfxOrchestratorEvent>& events)
        {
            events.push_back(screenExited(o.current));
            // Track the underlying screen for Pause overlay return-paths.
            if (o.current != SgfxScreen::Pause)
                o.previousNonOverlay = o.current;
            resetTransientStateOnEntry(o, next);
            o.current = next;
            events.push_back(screenEntered(next));
        }
    } // namespace detail::orchestrator

    // The single per-frame entry point. Host calls this each frame
    // with the latest input snapshot. The orchestrator routes to
    // the active screen, collects child events, applies any screen
    // transitions, and returns a flat event list the host walks
    // (play SFX, swap CSD scene asset, fade transition, etc.).
    inline std::vector<SgfxOrchestratorEvent> updateSgfxOrchestratorOneFrame(
        SgfxOrchestrator& o,
        const SgfxFrameInput& input)
    {
        using namespace detail::orchestrator;
        std::vector<SgfxOrchestratorEvent> events;

        switch (o.current)
        {
        case SgfxScreen::TitleIntro:
        {
            TitleIntroInput in;
            in.startTapped = input.startTapped;
            in.acceptTapped = input.acceptTapped;
            in.deltaSeconds = input.deltaSeconds;
            in.anyInputThisFrame =
                input.acceptTapped || input.cancelTapped || input.startTapped
                || input.selectTapped || input.upTapped || input.downTapped
                || input.leftTapped || input.rightTapped;
            const auto childEvents = updateTitleIntroOneFrame(o.titleIntro, in);
            for (const auto& e : childEvents)
            {
                if (!e.sfxCueName.empty())
                    events.push_back(sfx(SgfxScreen::TitleIntro, e.sfxCueName));
                if (e.kind == TitleIntroEventKind::PressStartArmed)
                {
                    // Hand off to the title menu state machine.
                    switchScreen(o, SgfxScreen::Title, events);
                }
            }
            break;
        }
        case SgfxScreen::Title:
        {
            TitleMenuInput in;
            in.acceptTapped = input.acceptTapped || input.startTapped;
            in.cancelTapped = input.cancelTapped;
            in.upTapped = input.upTapped;
            in.downTapped = input.downTapped;
            const auto childEvents = updateTitleMenuOneFrame(o.title, in);
            for (const auto& e : childEvents)
            {
                if (!e.sfxCueName.empty())
                    events.push_back(sfx(SgfxScreen::Title, e.sfxCueName));
                switch (e.kind)
                {
                case TitleMenuEventKind::OptionAccepted:
                    if (e.optionAtFire == TitleMenuOption::Continue
                        || e.optionAtFire == TitleMenuOption::NewGame)
                        switchScreen(o, SgfxScreen::WorldMap, events);
                    break;
                case TitleMenuEventKind::ExitRequested:
                    events.push_back({SgfxOrchestratorEventKind::HostExitRequested,
                                      SgfxScreen::Title, ""});
                    break;
                default: break;
                }
            }
            break;
        }
        case SgfxScreen::WorldMap:
        {
            WorldMapInput in;
            in.acceptTapped = input.acceptTapped;
            in.cancelTapped = input.cancelTapped;
            in.leftTapped = input.leftTapped;
            in.rightTapped = input.rightTapped;
            const auto childEvents = updateWorldMapOneFrame(o.worldMap, in);
            for (const auto& e : childEvents)
            {
                if (!e.sfxCueName.empty())
                    events.push_back(sfx(SgfxScreen::WorldMap, e.sfxCueName));
                if (e.kind == WorldMapEventKind::StageOpened)
                {
                    o.loading = LoadingState{};
                    switchScreen(o, SgfxScreen::Loading, events);
                }
                else if (e.kind == WorldMapEventKind::BackedToTitle)
                {
                    switchScreen(o, SgfxScreen::Title, events);
                }
            }
            break;
        }
        case SgfxScreen::Loading:
        {
            LoadingInput in;
            in.acceptTapped = input.acceptTapped || input.startTapped;
            in.deltaSeconds = input.deltaSeconds;
            const auto childEvents = updateLoadingScreenOneFrame(
                o.loading, in, input.loadingProgress);
            for (const auto& e : childEvents)
            {
                if (!e.sfxCueName.empty())
                    events.push_back(sfx(SgfxScreen::Loading, e.sfxCueName));
                if (e.kind == LoadingEventKind::FadeComplete)
                {
                    o.stageHud = StageHudState{};
                    switchScreen(o, SgfxScreen::StageHud, events);
                }
            }
            break;
        }
        case SgfxScreen::StageHud:
        {
            StageHudInput in;
            in.startTapped = input.startTapped;
            in.deltaSeconds = input.deltaSeconds;
            const auto childEvents = updateStageHudOneFrame(o.stageHud, in);
            for (const auto& e : childEvents)
            {
                if (!e.sfxCueName.empty())
                    events.push_back(sfx(SgfxScreen::StageHud, e.sfxCueName));
                if (e.kind == StageHudEventKind::PauseRequested)
                {
                    o.pause = PauseState{};
                    o.pause.context = PauseMenuContext::Stage;
                    o.pause.itemCount = 4;
                    switchScreen(o, SgfxScreen::Pause, events);
                }
            }
            break;
        }
        case SgfxScreen::Pause:
        {
            PauseInput in;
            in.acceptTapped = input.acceptTapped;
            in.cancelTapped = input.cancelTapped;
            in.selectTapped = input.selectTapped;
            in.upTapped = input.upTapped;
            in.downTapped = input.downTapped;
            const auto childEvents = updatePauseMenuOneFrame(o.pause, in);
            // Phase 334 fix: Stage-context cancel emits TWO events
            // both kinded QuitTransition (with different cues). Hub-
            // context cancel emits two HideTransition events. Take
            // only the FIRST transition event; subsequent ones are
            // duplicate-cue notifications, not separate transitions.
            bool transitionTaken = false;
            auto leavePause = [&](SgfxScreen next)
            {
                // The underlying StageHud holds a "paused" latch the
                // gameplay tick uses to gate timer / input. Reset it
                // so a future Start press can re-open the pause.
                o.stageHud.paused = false;
                switchScreen(o, next, events);
                transitionTaken = true;
            };
            for (const auto& e : childEvents)
            {
                if (!e.sfxCueName.empty())
                    events.push_back(sfx(SgfxScreen::Pause, e.sfxCueName));
                if (transitionTaken)
                    continue;
                if (e.kind == PauseEventKind::HideTransition
                    || e.kind == PauseEventKind::BackedOut)
                {
                    // Return to whatever screen we paused.
                    leavePause(o.previousNonOverlay);
                }
                else if (e.kind == PauseEventKind::QuitTransition)
                {
                    // From Stage context, Quit goes to WorldMap. From
                    // a Hub-context Pause this branch shouldn't fire
                    // (Hub uses HideTransition).
                    leavePause(SgfxScreen::WorldMap);
                }
                else if (e.kind == PauseEventKind::OptionAccepted
                         && e.cursorAtFire == 0)
                {
                    // Row 0 of the pause menu is "Continue Game" --
                    // accepting it resumes gameplay (Hide back to
                    // the screen we paused). The SGFX pause state
                    // machine emits OptionAccepted but doesn't move
                    // screens itself; the orchestrator owns that.
                    leavePause(o.previousNonOverlay);
                }
            }
            break;
        }
        case SgfxScreen::Results:
        {
            ResultsInput in;
            in.acceptTapped = input.acceptTapped || input.startTapped;
            in.deltaSeconds = input.deltaSeconds;
            const auto childEvents = updateResultsScreenOneFrame(o.results, in);
            for (const auto& e : childEvents)
            {
                if (!e.sfxCueName.empty())
                    events.push_back(sfx(SgfxScreen::Results, e.sfxCueName));
                if (e.kind == ResultsEventKind::Acknowledged)
                    switchScreen(o, SgfxScreen::WorldMap, events);
            }
            break;
        }
        case SgfxScreen::Hub:
        {
            HubInput in;
            in.acceptTapped = input.acceptTapped;
            in.cancelTapped = input.cancelTapped;
            in.talkTapped = input.talkTapped;
            in.openMapTapped = input.openMapTapped;
            const auto childEvents = updateHubScreenOneFrame(o.hub, in);
            for (const auto& e : childEvents)
            {
                if (!e.sfxCueName.empty())
                    events.push_back(sfx(SgfxScreen::Hub, e.sfxCueName));
                if (e.kind == HubEventKind::StageEntryConfirmed)
                {
                    o.loading = LoadingState{};
                    switchScreen(o, SgfxScreen::Loading, events);
                }
            }
            break;
        }
        }
        return events;
    }

    // Host-callable helpers for explicit transitions the screen
    // state machines don't generate themselves (e.g. host signals
    // "stage finished", or "leave hub for world map").

    inline std::vector<SgfxOrchestratorEvent> sgfxOrchestratorFinishStage(
        SgfxOrchestrator& o,
        ResultsRank rank,
        std::int64_t score,
        float timeSeconds,
        std::int32_t rings,
        std::int64_t totalScore)
    {
        using namespace detail::orchestrator;
        std::vector<SgfxOrchestratorEvent> events;
        if (o.current != SgfxScreen::StageHud)
            return events;
        o.results = ResultsState{};
        o.results.rank = rank;
        o.results.score = score;
        o.results.timeSeconds = timeSeconds;
        o.results.rings = rings;
        o.results.totalScore = totalScore;
        switchScreen(o, SgfxScreen::Results, events);
        return events;
    }

    inline std::vector<SgfxOrchestratorEvent> sgfxOrchestratorEnterHub(
        SgfxOrchestrator& o, HubMode mode)
    {
        using namespace detail::orchestrator;
        std::vector<SgfxOrchestratorEvent> events;
        o.hub = HubState{};
        o.hub.mode = mode;
        switchScreen(o, SgfxScreen::Hub, events);
        return events;
    }

    // Phase 334: host signals "user left the hub" (e.g. fly to
    // world map button). Pure transition; doesn't run any per-frame
    // child update.
    inline std::vector<SgfxOrchestratorEvent> sgfxOrchestratorLeaveHubToWorldMap(
        SgfxOrchestrator& o)
    {
        using namespace detail::orchestrator;
        std::vector<SgfxOrchestratorEvent> events;
        if (o.current == SgfxScreen::Hub)
            switchScreen(o, SgfxScreen::WorldMap, events);
        return events;
    }

    // Phase 334: convenience for hosts that don't want to drive
    // the title-intro fade and just need to land on the menu.
    inline std::vector<SgfxOrchestratorEvent> sgfxOrchestratorSkipToTitleMenu(
        SgfxOrchestrator& o)
    {
        using namespace detail::orchestrator;
        std::vector<SgfxOrchestratorEvent> events;
        if (o.current == SgfxScreen::TitleIntro)
            switchScreen(o, SgfxScreen::Title, events);
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
