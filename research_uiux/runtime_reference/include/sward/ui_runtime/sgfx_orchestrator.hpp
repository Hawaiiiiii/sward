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

#include "sgfx_title_menu.hpp"
#include "sgfx_pause_menu.hpp"
#include "sgfx_loading_screen.hpp"
#include "sgfx_stage_hud.hpp"
#include "sgfx_results_screen.hpp"
#include "sgfx_world_map.hpp"
#include "sgfx_hub_screen.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class SgfxScreen : std::uint8_t
    {
        Title    = 0,
        WorldMap = 1,
        Loading  = 2,
        StageHud = 3,
        Pause    = 4,
        Results  = 5,
        Hub      = 6,
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
        SgfxScreen current = SgfxScreen::Title;
        SgfxScreen previousNonOverlay = SgfxScreen::Title;

        // Each child machine's state. The orchestrator only ticks
        // the one matching `current`, but keeps all of them resident
        // so a Pause overlay can be popped without losing the
        // underlying StageHud values.
        TitleMenuState   title;
        WorldMapState    worldMap;
        LoadingState     loading;
        StageHudState    stageHud;
        PauseState       pause;
        ResultsState     results;
        HubState         hub;
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

        inline void switchScreen(SgfxOrchestrator& o,
                                 SgfxScreen next,
                                 std::vector<SgfxOrchestratorEvent>& events)
        {
            events.push_back(screenExited(o.current));
            // Track the underlying screen for Pause overlay return-paths.
            if (o.current != SgfxScreen::Pause)
                o.previousNonOverlay = o.current;
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
            for (const auto& e : childEvents)
            {
                if (!e.sfxCueName.empty())
                    events.push_back(sfx(SgfxScreen::Pause, e.sfxCueName));
                if (e.kind == PauseEventKind::HideTransition
                    || e.kind == PauseEventKind::BackedOut)
                {
                    // Return to whatever screen we paused.
                    switchScreen(o, o.previousNonOverlay, events);
                }
                else if (e.kind == PauseEventKind::QuitTransition)
                {
                    switchScreen(o, SgfxScreen::WorldMap, events);
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

} // namespace sward::ui_runtime::generated::sgfx_hud
