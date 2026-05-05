// Phase 304-307 smoke test: drives every newly ported state machine
// (pause, loading, stage HUD, results) through scripted scenarios
// without any game/runtime dependency. Each scenario asserts the
// state shape + event list expected from the retail-mined behavior.

#include "sward/ui_runtime/sgfx_pause_menu.hpp"
#include "sward/ui_runtime/sgfx_loading_screen.hpp"
#include "sward/ui_runtime/sgfx_stage_hud.hpp"
#include "sward/ui_runtime/sgfx_results_screen.hpp"
#include "sward/ui_runtime/sgfx_world_map.hpp"
#include "sward/ui_runtime/sgfx_hub_screen.hpp"
#include "sward/ui_runtime/sgfx_title_intro.hpp"
#include "sward/ui_runtime/sgfx_general_window.hpp"

#include <iostream>
#include <string>
#include <type_traits>

namespace ui = sward::ui_runtime::generated::sgfx_hud;

static int g_failures = 0;
static void expect(bool ok, std::string_view label)
{
    std::cout << (ok ? "OK   " : "FAIL ") << label << "\n";
    if (!ok) ++g_failures;
}
template <typename T>
static void expectEq(const T& a, const T& e, std::string_view label)
{
    const bool ok = a == e;
    std::cout << (ok ? "OK   " : "FAIL ") << label;
    if constexpr (std::is_integral_v<T> || std::is_enum_v<T>)
        std::cout << " expected=" << static_cast<long long>(e)
                  << " actual=" << static_cast<long long>(a);
    else if constexpr (std::is_convertible_v<T, std::string_view>)
        std::cout << " expected=\"" << std::string_view(e) << "\""
                  << " actual=\"" << std::string_view(a) << "\"";
    std::cout << "\n";
    if (!ok) ++g_failures;
}

// ----- Pause menu tests -----

static void testPauseMenu()
{
    using namespace ui;
    std::cout << "\n== pause menu ==\n";

    // Stage context -> Quit transition on cancel. Phase 323
    // captured-trace update: cancel fires TWO cues (pausewinclose
    // then pausecansel) on the same frame.
    {
        PauseState s; s.context = PauseMenuContext::Stage;
        s.itemCount = 4; s.cursorIndex = 0;
        PauseInput in; in.cancelTapped = true;
        const auto evs = updatePauseMenuOneFrame(s, in);
        expectEq(evs[0].kind, PauseEventKind::QuitTransition, "pause.stage cancel = Quit");
        expectEq(s.lastTransition, PauseTransition::Quit, "pause.lastTransition Quit");
        expectEq(evs.size(), static_cast<std::size_t>(2),
                 "pause.cancel emits two cues (winclose + cansel)");
        expectEq(evs[0].sfxCueName, std::string("sys_actstg_pausewinclose"),
                 "pause.cancel cue1 = pausewinclose");
        expectEq(evs[1].sfxCueName, std::string("sys_actstg_pausecansel"),
                 "pause.cancel cue2 = pausecansel");
    }
    // Hub context -> Hide transition on cancel.
    {
        PauseState s; s.context = PauseMenuContext::Hub;
        s.itemCount = 4; s.cursorIndex = 0;
        PauseInput in; in.cancelTapped = true;
        const auto evs = updatePauseMenuOneFrame(s, in);
        expectEq(evs[0].kind, PauseEventKind::HideTransition, "pause.hub cancel = Hide");
    }
    // Select shortcut -> Achievements overlay.
    {
        PauseState s; s.context = PauseMenuContext::Stage; s.itemCount = 4;
        PauseInput in; in.selectTapped = true;
        const auto evs = updatePauseMenuOneFrame(s, in);
        expect(s.achievementsOverlayOpen, "pause.achievements opened on Select");
        expectEq(evs[0].kind, PauseEventKind::AchievementsOpened, "pause.event kind");
    }
    // Last-2 row = Options. Last-1 row = Quit/Hide.
    {
        PauseState s; s.context = PauseMenuContext::Stage; s.itemCount = 4;
        s.cursorIndex = 2; // count-2 = Options row
        PauseInput in; in.acceptTapped = true;
        const auto evs = updatePauseMenuOneFrame(s, in);
        expect(s.optionsOverlayOpen, "pause.last-2 = Options opened");
        expectEq(evs[0].kind, PauseEventKind::OptionsOpened, "pause.OptionsOpened");
    }
    {
        PauseState s; s.context = PauseMenuContext::Stage; s.itemCount = 4;
        s.cursorIndex = 3; // count-1 = Quit/Hide row
        PauseInput in; in.acceptTapped = true;
        const auto evs = updatePauseMenuOneFrame(s, in);
        expectEq(evs[0].kind, PauseEventKind::QuitTransition, "pause.last-1 = Quit");
    }
}

// ----- Loading screen tests -----

static void testLoadingScreen()
{
    using namespace ui;
    std::cout << "\n== loading screen ==\n";

    LoadingState s;
    expectEq(s.phase, LoadingPhase::Booting, "loading.starts in Booting");

    // Tick progress 0.5: progress event, still Booting.
    {
        LoadingInput in;
        const auto evs = updateLoadingScreenOneFrame(s, in, 0.5f);
        expect(!evs.empty(), "loading.progress event fired");
        expectEq(evs[0].kind, LoadingEventKind::ProgressTicked, "loading.kind");
        expectEq(s.phase, LoadingPhase::Booting, "loading.still Booting at 50%");
    }
    // Tick to 1.0: ReachedReady fires.
    {
        LoadingInput in;
        const auto evs = updateLoadingScreenOneFrame(s, in, 1.0f);
        bool sawReady = false;
        for (const auto& e : evs)
            if (e.kind == LoadingEventKind::ReachedReady) sawReady = true;
        expect(sawReady, "loading.ReachedReady fired at 100%");
        expectEq(s.phase, LoadingPhase::Ready, "loading.in Ready");
        expect(s.showPressStart, "loading.press-start prompt visible");
    }
    // Press accept -> Dismissed.
    {
        LoadingInput in; in.acceptTapped = true;
        const auto evs = updateLoadingScreenOneFrame(s, in, 1.0f);
        expectEq(s.phase, LoadingPhase::Dismissed, "loading.dismissed on accept");
        expectEq(evs[0].kind, LoadingEventKind::DismissAccepted, "loading.DismissAccepted");
    }
    // Fade to complete.
    {
        LoadingInput in; in.deltaSeconds = 1.0f; // > fadeOutDuration
        const auto evs = updateLoadingScreenOneFrame(s, in, 1.0f);
        bool sawComplete = false;
        for (const auto& e : evs)
            if (e.kind == LoadingEventKind::FadeComplete) sawComplete = true;
        expect(sawComplete, "loading.FadeComplete fired");
    }
}

// ----- Stage HUD tests -----

static void testStageHud()
{
    using namespace ui;
    std::cout << "\n== stage HUD ==\n";

    StageHudState s; s.mode = StageMode::DaySonic; s.lives = 4;
    // Ring pickup.
    {
        const auto evs = applyRingDelta(s, 5);
        expectEq(evs[0].kind, StageHudEventKind::RingPickedUp, "hud.ring pickup");
        expectEq(s.rings, 5, "hud.rings = 5");
    }
    // Ring loss.
    {
        const auto evs = applyRingDelta(s, -3);
        expectEq(evs[0].kind, StageHudEventKind::RingLost, "hud.ring lost");
        expectEq(s.rings, 2, "hud.rings = 2");
    }
    // Score delta.
    {
        applyScoreDelta(s, 1500);
        expectEq(s.score, static_cast<std::int64_t>(1500), "hud.score = 1500");
    }
    // Boost gauge fill -> empty triggers BoostUsed.
    {
        setSpeedGauge(s, 0.5f);
        const auto evs = setSpeedGauge(s, 0.0f);
        expect(!evs.empty(), "hud.boost-empty event");
        expectEq(evs[0].kind, StageHudEventKind::BoostUsed, "hud.BoostUsed");
    }
    // Pause request.
    {
        StageHudInput in; in.startTapped = true;
        const auto evs = updateStageHudOneFrame(s, in);
        expect(s.paused, "hud.paused after Start");
        expectEq(evs[0].kind, StageHudEventKind::PauseRequested, "hud.PauseRequested");
    }
    // Time limit firing.
    {
        StageHudState s2; s2.timeLimitSeconds = 10.0f;
        StageHudInput in; in.deltaSeconds = 11.0f;
        const auto evs = updateStageHudOneFrame(s2, in);
        bool sawLimit = false;
        for (const auto& e : evs)
            if (e.kind == StageHudEventKind::TimeLimit) sawLimit = true;
        expect(sawLimit, "hud.TimeLimit fired past limit");
    }
}

// ----- Results screen tests -----

static void testResultsScreen()
{
    using namespace ui;
    std::cout << "\n== results screen ==\n";

    ResultsState s; s.rank = ResultsRank::A; s.score = 100000; s.rings = 99;
    s.totalScore = 200000; s.timeSeconds = 75.5f;
    s.secondsBetweenLines = 0.1f;

    // Tick through all 6 lines of the tally.
    int revealCount = 0;
    bool sawComplete = false;
    for (int i = 0; i < 100 && !sawComplete; ++i)
    {
        ResultsInput in; in.deltaSeconds = 0.15f;
        const auto evs = updateResultsScreenOneFrame(s, in);
        for (const auto& e : evs)
        {
            if (e.kind == ResultsEventKind::LineRevealed) ++revealCount;
            if (e.kind == ResultsEventKind::TallyComplete) sawComplete = true;
        }
    }
    expectEq(revealCount, static_cast<int>(ResultsLineId::Count),
             "results.6 lines revealed");
    expect(sawComplete, "results.TallyComplete fired");

    // Acknowledge.
    {
        ResultsInput in; in.acceptTapped = true;
        const auto evs = updateResultsScreenOneFrame(s, in);
        expect(s.acknowledged, "results.acknowledged");
        expectEq(evs[0].kind, ResultsEventKind::Acknowledged, "results.Acknowledged");
    }
}

// ----- World Map tests -----

static void testWorldMap()
{
    using namespace ui;
    std::cout << "\n== world map ==\n";
    WorldMapState s;
    s.unlocked[static_cast<std::size_t>(WorldMapContinent::Apotos)] = true;
    s.unlocked[static_cast<std::size_t>(WorldMapContinent::Spagonia)] = true;
    s.unlocked[static_cast<std::size_t>(WorldMapContinent::Eggmanland)] = true;
    expectEq(s.cursor, WorldMapContinent::Apotos, "wm.starts at Apotos");

    {
        WorldMapInput in; in.rightTapped = true;
        const auto evs = updateWorldMapOneFrame(s, in);
        expectEq(s.cursor, WorldMapContinent::Spagonia, "wm.right -> Spagonia");
        expectEq(evs[0].sfxCueName, std::string(kWorldMapSfxCursor), "wm.cursor SFX");
    }
    {
        WorldMapInput in; in.rightTapped = true;
        updateWorldMapOneFrame(s, in);
        expectEq(s.cursor, WorldMapContinent::Eggmanland, "wm.skipped locked");
    }
    {
        WorldMapInput in; in.acceptTapped = true;
        const auto evs = updateWorldMapOneFrame(s, in);
        expect(s.stageOpenPanelVisible, "wm.stage panel opened");
        expectEq(evs[0].kind, WorldMapEventKind::StageOpened, "wm.StageOpened");
    }
    {
        WorldMapInput in; in.cancelTapped = true;
        const auto evs = updateWorldMapOneFrame(s, in);
        expect(!s.stageOpenPanelVisible, "wm.panel closed on cancel");
    }
    {
        WorldMapInput in; in.cancelTapped = true;
        const auto evs = updateWorldMapOneFrame(s, in);
        expectEq(evs[0].kind, WorldMapEventKind::BackedToTitle, "wm.cancel from root = BackedToTitle");
    }
}

// ----- Hub tests -----

static void testHubScreen()
{
    using namespace ui;
    std::cout << "\n== hub screen ==\n";
    HubState s;
    expectEq(s.mode, HubMode::DaySonic, "hub.starts Day");

    {
        HubInput in; in.talkTapped = true;
        const auto evs = updateHubScreenOneFrame(s, in);
        expectEq(s.overlay, HubOverlay::BalloonText, "hub.talk opens balloon");
        expectEq(evs[0].kind, HubEventKind::OverlayOpened, "hub.OverlayOpened");
    }
    {
        HubInput in; in.cancelTapped = true;
        const auto evs = updateHubScreenOneFrame(s, in);
        expectEq(s.overlay, HubOverlay::None, "hub.cancel closes balloon");
    }
    {
        const auto evs = openHubOverlay(s, HubOverlay::StageGate);
        expectEq(s.overlay, HubOverlay::StageGate, "hub.gate opened by host");
    }
    {
        HubInput in; in.acceptTapped = true;
        const auto evs = updateHubScreenOneFrame(s, in);
        expectEq(evs[0].kind, HubEventKind::StageEntryConfirmed, "hub.StageEntryConfirmed");
        expectEq(s.overlay, HubOverlay::None, "hub.gate closed after confirm");
    }
    {
        const auto evs = setHubTimeOfDay(s, HubMode::Werehog);
        expectEq(s.mode, HubMode::Werehog, "hub.switched to Werehog");
        expectEq(evs[0].kind, HubEventKind::TimeOfDayChanged, "hub.TimeOfDayChanged");
    }
    // Phase 318 retail-fidelity check: hub menu open uses the
    // pause-bank cue, not the worldmap-bank cue.
    {
        HubState s2;
        const auto evs = openHubOverlay(s2, HubOverlay::ShopMenu);
        expectEq(evs[0].sfxCueName,
                 std::string(kHubSfxOpenMenu),
                 "hub.shop open uses sys_actstg_pausewinopen");
    }
    // Tutorial popup uses obj_navi_appear.
    {
        HubState s2;
        const auto evs = openHubTutorialPopup(s2);
        expectEq(evs[0].sfxCueName,
                 std::string(kHubSfxTutorialPopup),
                 "hub.tutorial popup uses obj_navi_appear");
    }
}

// ----- Title intro tests -----
//
// Captured trace (phase315_take2 frames 2458..5361) showed
// requested_state advance 0->1 over ~2 frames (~50 ms) then
// stayed at 1 for ~50 seconds until the menu state machine took
// over. Verify the same advance behavior + the press-start arm.
static void testTitleIntro()
{
    using namespace ui;
    std::cout << "\n== title intro ==\n";

    TitleIntroSlot s;
    expectEq(s.requestedState, TitleIntroState::LogoFadeIn, "intro.starts LogoFadeIn");

    // Tick 30 ms -> still in LogoFadeIn.
    {
        TitleIntroInput in; in.deltaSeconds = 0.030f;
        const auto evs = updateTitleIntroOneFrame(s, in);
        expectEq(s.requestedState, TitleIntroState::LogoFadeIn, "intro.30ms still LogoFadeIn");
        expect(evs.empty(), "intro.30ms no event");
    }
    // Tick another 50 ms -> 80ms total, past 60ms threshold -> advance.
    {
        TitleIntroInput in; in.deltaSeconds = 0.050f;
        const auto evs = updateTitleIntroOneFrame(s, in);
        expectEq(s.requestedState, TitleIntroState::PressStartIdle, "intro.advance to PressStartIdle");
        expectEq(evs[0].kind, TitleIntroEventKind::StateAdvanced, "intro.StateAdvanced");
    }
    // Press start in PressStartIdle -> PressStartArmed event.
    {
        TitleIntroInput in; in.startTapped = true;
        const auto evs = updateTitleIntroOneFrame(s, in);
        expectEq(evs[0].kind, TitleIntroEventKind::PressStartArmed, "intro.PressStartArmed");
        // dirty + transition_armed stay 0 (matches captured trace).
        expect(!s.dirty, "intro.dirty stays 0 (matches retail)");
        expect(!s.transitionArmed, "intro.transitionArmed stays 0 (matches retail)");
    }
}

// ----- General Window tests (Phase 324 retail-mined) -----
//
// Mirrors SWA::CGeneralWindow's status machine: Closed ->
// OpeningMessage -> DisplayingMessage -> Closed (when accepted /
// canceled), or Closed -> OpeningControls -> DisplayingControls
// -> Closed for help windows.
static void testGeneralWindow()
{
    using namespace ui;
    std::cout << "\n== general window ==\n";
    GeneralWindowState s;
    expectEq(s.status, WindowStatus::Closed, "gw.starts Closed");

    // Open a 2-row message window (delete-save-confirm shape).
    {
        const auto evs = openGeneralWindow(s, 2, false);
        expectEq(s.status, WindowStatus::OpeningMessage, "gw.opens to OpeningMessage");
        expectEq(static_cast<int>(WindowStatus::OpeningMessage), 2, "gw.OpeningMessage value=2 (retail gap)");
        expectEq(evs[0].kind, GeneralWindowEventKind::WindowOpened, "gw.WindowOpened");
        expectEq(evs[0].sfxCueName, std::string("sys_worldmap_window"), "gw.open SFX");
    }
    // Advance to displaying.
    advanceGeneralWindowToDisplaying(s);
    expectEq(s.status, WindowStatus::DisplayingMessage, "gw.advance -> DisplayingMessage");
    expectEq(static_cast<int>(WindowStatus::DisplayingMessage), 3, "gw.DisplayingMessage value=3");

    // Cursor down.
    {
        GeneralWindowInput in; in.downTapped = true;
        const auto evs = updateGeneralWindowOneFrame(s, in);
        expectEq(s.cursorIndex, 1, "gw.cursor moved to row 1");
        expectEq(evs[0].kind, GeneralWindowEventKind::CursorMoved, "gw.CursorMoved");
    }
    // Accept on row 1 -> Confirmed + Closed.
    {
        GeneralWindowInput in; in.acceptTapped = true;
        const auto evs = updateGeneralWindowOneFrame(s, in);
        expectEq(s.selectedIndex, 1, "gw.selectedIndex captured");
        expectEq(s.status, WindowStatus::Closed, "gw.closes after accept");
        expectEq(evs[0].kind, GeneralWindowEventKind::Confirmed, "gw.Confirmed");
        expectEq(evs[1].kind, GeneralWindowEventKind::WindowClosed, "gw.WindowClosed");
    }
    // Controls window flow.
    {
        GeneralWindowState c;
        openGeneralWindow(c, 0, true);
        expectEq(c.status, WindowStatus::OpeningControls, "gw.controls -> OpeningControls=4");
        expectEq(static_cast<int>(WindowStatus::OpeningControls), 4, "gw.OpeningControls value=4");
        advanceGeneralWindowToDisplaying(c);
        expectEq(c.status, WindowStatus::DisplayingControls, "gw.controls -> DisplayingControls=5");
    }
}

// ----- Loading retail-fidelity sanity -----
static void testLoadingDisplayType()
{
    using namespace ui;
    std::cout << "\n== loading retail enum ==\n";
    expectEq(static_cast<int>(LoadingDisplayType::MilesElectric), 0, "load.MilesElectric=0");
    expectEq(static_cast<int>(LoadingDisplayType::Arrows), 4, "load.Arrows=4 (chevrons)");
    expectEq(static_cast<int>(LoadingDisplayType::ChangeTimeOfDay), 7, "load.ChangeTimeOfDay=7");
    expectEq(static_cast<int>(LoadingDisplayType::Blank), 8, "load.Blank=8");
}

int main()
{
    testPauseMenu();
    testLoadingScreen();
    testStageHud();
    testResultsScreen();
    testWorldMap();
    testHubScreen();
    testTitleIntro();
    testGeneralWindow();
    testLoadingDisplayType();
    std::cout << "\nfailures: " << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
