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
#include "sward/ui_runtime/sgfx_pad_state.hpp"
#include "sward/ui_runtime/sgfx_evil_hud_guide.hpp"
#include "sward/ui_runtime/sgfx_save_icon.hpp"
#include "sward/ui_runtime/sgfx_sound_admin.hpp"
#include "sward/ui_runtime/sgfx_hud_evil_stage.hpp"
#include "sward/ui_runtime/sgfx_hud_status.hpp"
#include "sward/ui_runtime/sgfx_help_window.hpp"

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

    // Phase 339: 4-phase retail TStateMachine<CHudResult> walk.
    ResultsState s; s.rank = ResultsRank::A; s.score = 100000; s.rings = 99;
    s.totalScore = 200000; s.timeSeconds = 75.5f;
    s.secondsBetweenLines = 0.05f;
    s.cameraLerpSeconds = 0.1f;
    s.rankPhaseMinSeconds = 0.1f;
    expectEq(s.phase, ResultsPhase::FirstWaiting, "results.starts FirstWaiting");

    // Tick once: FirstWaiting -> ChangingCamera (one-frame).
    {
        ResultsInput in; in.deltaSeconds = 0.0f;
        const auto evs = updateResultsScreenOneFrame(s, in);
        expectEq(s.phase, ResultsPhase::ChangingCamera, "results.advance to ChangingCamera");
        expectEq(evs[0].kind, ResultsEventKind::EnteredChangingCamera,
                 "results.EnteredChangingCamera");
    }
    // Tick the camera lerp: -> ResultAnimation.
    {
        ResultsInput in; in.deltaSeconds = 0.15f;
        const auto evs = updateResultsScreenOneFrame(s, in);
        expectEq(s.phase, ResultsPhase::ResultAnimation, "results.advance to ResultAnimation");
        expectEq(evs[0].kind, ResultsEventKind::EnteredResultAnimation,
                 "results.EnteredResultAnimation");
    }

    // Tick through all 6 lines of the tally.
    int revealCount = 0;
    bool sawComplete = false;
    bool sawRank = false;
    for (int i = 0; i < 100 && !sawComplete; ++i)
    {
        ResultsInput in; in.deltaSeconds = 0.06f;
        const auto evs = updateResultsScreenOneFrame(s, in);
        for (const auto& e : evs)
        {
            if (e.kind == ResultsEventKind::LineRevealed) ++revealCount;
            if (e.kind == ResultsEventKind::TallyComplete) sawComplete = true;
            if (e.kind == ResultsEventKind::EnteredRank) sawRank = true;
        }
    }
    expectEq(revealCount, static_cast<int>(ResultsLineId::Count),
             "results.6 lines revealed");
    expect(sawComplete, "results.TallyComplete fired");
    expect(sawRank, "results.EnteredRank fired after tally");
    expectEq(s.phase, ResultsPhase::Rank, "results.now in Rank phase");

    // Accept BEFORE rankPhaseMinSeconds elapses -> NOT acknowledged.
    {
        ResultsInput in; in.acceptTapped = true;
        const auto evs = updateResultsScreenOneFrame(s, in);
        expect(!s.acknowledged, "results.early accept ignored");
        expect(evs.empty(), "results.no Acknowledged event yet");
    }
    // Tick to past rankPhaseMinSeconds, then accept.
    {
        ResultsInput in; in.deltaSeconds = 0.2f;
        updateResultsScreenOneFrame(s, in);
    }
    {
        ResultsInput in; in.acceptTapped = true;
        const auto evs = updateResultsScreenOneFrame(s, in);
        expect(s.acknowledged, "results.acknowledged after rank dwell");
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
    expectEq(s.hover, WorldMapContinent::Apotos, "wm.starts at Apotos");

    {
        WorldMapInput in; in.rightTapped = true;
        const auto evs = updateWorldMapOneFrame(s, in);
        expectEq(s.hover, WorldMapContinent::Spagonia, "wm.right -> Spagonia");
        expectEq(evs[0].sfxCueName, std::string(kWorldMapSfxCursor), "wm.cursor SFX");
    }
    {
        WorldMapInput in; in.rightTapped = true;
        updateWorldMapOneFrame(s, in);
        expectEq(s.hover, WorldMapContinent::Eggmanland, "wm.skipped locked");
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

// ----- Phase 326: SPadState retail bitfield -----
static void testPadState()
{
    using namespace ui;
    std::cout << "\n== pad state bitfield ==\n";
    // Retail key values (from SWA::EKeyState).
    expectEq(static_cast<unsigned>(kSgfxKey_A), 0x1u, "pad.A=0x1");
    expectEq(static_cast<unsigned>(kSgfxKey_B), 0x2u, "pad.B=0x2");
    expectEq(static_cast<unsigned>(kSgfxKey_X), 0x8u, "pad.X=0x8 (gap on 0x4)");
    expectEq(static_cast<unsigned>(kSgfxKey_Y), 0x10u, "pad.Y=0x10");
    expectEq(static_cast<unsigned>(kSgfxKey_Start), 0x400u, "pad.Start=0x400");
    expectEq(static_cast<unsigned>(kSgfxKey_DpadDown), 0x80u, "pad.DpadDown=0x80");
    expectEq(static_cast<unsigned>(kSgfxKey_LeftStickRight), 0x200000u, "pad.LeftStickRight=0x200000");
    expectEq(static_cast<unsigned>(kSgfxKey_RightStickRight), 0x2000000u, "pad.RightStickRight=0x2000000");

    // Edge math: A pressed this frame, not last frame -> tapped.
    SgfxPadEdgeSample cur{}, prev{};
    cur.a = true;
    SgfxPadState s;
    sgfxApplyPadEdges(s, cur, prev);
    expect(s.isDown(kSgfxKey_A), "pad.A is down this frame");
    expect(s.isTapped(kSgfxKey_A), "pad.A is tapped (rising edge)");
    expect(!s.isReleased(kSgfxKey_A), "pad.A not released");

    // Hold A: still down, but no longer tapped.
    prev = cur;
    sgfxApplyPadEdges(s, cur, prev);
    expect(s.isDown(kSgfxKey_A), "pad.A still down");
    expect(!s.isTapped(kSgfxKey_A), "pad.A not tapped on hold");

    // Release A: released this frame.
    cur.a = false;
    sgfxApplyPadEdges(s, cur, prev);
    expect(s.isReleased(kSgfxKey_A), "pad.A released (falling edge)");
    expect(!s.isDown(kSgfxKey_A), "pad.A no longer down");

    // Multi-mask: B and DpadDown together.
    cur = {}; prev = {};
    cur.b = true; cur.dpadDown = true;
    sgfxApplyPadEdges(s, cur, prev);
    expect(s.isTapped(static_cast<SgfxKeyState>(kSgfxKey_B | kSgfxKey_DpadDown)),
           "pad.B+DpadDown both tapped");

    // CInputState::GetPadState slot indexing.
    SgfxInputState input;
    input.currentPadStateIndex = 0;
    input.padStates[0].downState = kSgfxKey_Start;
    expect(input.getPadState().isDown(kSgfxKey_Start), "input.GetPadState() reads slot 0");
    input.currentPadStateIndex = 3;
    input.padStates[3].downState = kSgfxKey_Y;
    expect(input.getPadState().isDown(kSgfxKey_Y), "input.GetPadState() reads slot 3");
}

// ----- Phase 327: World Map camera retail fields -----
static void testWorldMapCamera()
{
    using namespace ui;
    std::cout << "\n== world map camera ==\n";
    WorldMapState s;
    // Defaults: canMove starts true (player can rotate the globe at idle).
    expect(s.camera.canMove, "wmcam.canMove default true");
    expectEq(s.camera.pitch, 0.0f, "wmcam.pitch default 0");
    expectEq(s.camera.yaw, 0.0f, "wmcam.yaw default 0");
    // Host can lock the camera during stage-launch transitions.
    s.camera.canMove = false;
    s.camera.tiltToEarthTransitionSpeed = 1.5f;
    expect(!s.camera.canMove, "wmcam.canMove can be locked");
    expectEq(s.camera.tiltToEarthTransitionSpeed, 1.5f, "wmcam.tilt speed = 1.5");
}

// ----- Phase 328: title-intro attract movie -----
static void testTitleIntroAttractMovie()
{
    using namespace ui;
    std::cout << "\n== title intro attract movie ==\n";
    TitleIntroSlot s;
    // Walk past LogoFadeIn first.
    {
        TitleIntroInput in; in.deltaSeconds = 0.080f;
        updateTitleIntroOneFrame(s, in);
    }
    expectEq(s.requestedState, TitleIntroState::PressStartIdle,
             "intro.attract.starts at PressStartIdle");

    // Idle past advertiseMovieWaitTime (default 30s) -> attract starts.
    s.advertiseMovieWaitTime = 1.0f; // shrink for the test
    {
        TitleIntroInput in; in.deltaSeconds = 1.5f;
        const auto evs = updateTitleIntroOneFrame(s, in);
        expectEq(s.requestedState, TitleIntroState::AttractMovie,
                 "intro.attract.advances on idle timeout");
        expect(s.isPlayingAdvertiseMovie, "intro.attract.isPlayingAdvertiseMovie=true");
        expectEq(evs[0].kind, TitleIntroEventKind::AttractMovieStarted,
                 "intro.attract.AttractMovieStarted event");
    }
    // Any input dismisses the attract.
    {
        TitleIntroInput in; in.anyInputThisFrame = true;
        const auto evs = updateTitleIntroOneFrame(s, in);
        expectEq(s.requestedState, TitleIntroState::PressStartIdle,
                 "intro.attract.dismissed back to PressStartIdle");
        expect(!s.isPlayingAdvertiseMovie, "intro.attract.movie stopped");
        expectEq(evs[0].kind, TitleIntroEventKind::AttractMovieDismissed,
                 "intro.attract.AttractMovieDismissed event");
    }
    // Input during PressStartIdle resets the idle counter (attract
    // doesn't kick in mid-navigation).
    {
        TitleIntroSlot s2;
        // Walk past logo.
        TitleIntroInput in; in.deltaSeconds = 0.080f;
        updateTitleIntroOneFrame(s2, in);
        s2.advertiseMovieWaitTime = 1.0f;
        TitleIntroInput in2; in2.deltaSeconds = 0.5f;
        updateTitleIntroOneFrame(s2, in2);
        // 0.5s in; now any input should reset the counter.
        TitleIntroInput in3; in3.anyInputThisFrame = true; in3.deltaSeconds = 0.0f;
        updateTitleIntroOneFrame(s2, in3);
        expectEq(s2.advertiseMovieIdleSeconds, 0.0f,
                 "intro.attract.idle counter reset on input");
    }
}

// ----- Phase 329: results-screen EX variant + BGM cues -----
static void testResultsScreenEx()
{
    using namespace ui;
    std::cout << "\n== results EX variant + BGM ==\n";
    expectEq(kResultsBgmSuccess, std::string_view("bgm_sys_result"),
             "results.success BGM cue");
    expectEq(kResultsBgmFailure, std::string_view("bgm_sys_result_ng"),
             "results.failure BGM cue");

    // EX variant flag and BGM selection are host-driven.
    ResultsState s;
    s.isExVariant = true;
    s.useFailureBgm = false;
    s.rank = ResultsRank::S;
    s.secondsBetweenLines = 0.05f;
    s.cameraLerpSeconds = 0.05f;

    // Tally completes after walking through the 4-state machine.
    bool sawComplete = false;
    int reveals = 0;
    for (int i = 0; i < 100 && !sawComplete; ++i)
    {
        ResultsInput in; in.deltaSeconds = 0.06f;
        const auto evs = updateResultsScreenOneFrame(s, in);
        for (const auto& e : evs)
        {
            if (e.kind == ResultsEventKind::LineRevealed) ++reveals;
            if (e.kind == ResultsEventKind::TallyComplete) sawComplete = true;
        }
    }
    expectEq(reveals, static_cast<int>(ResultsLineId::Count),
             "results.EX 6 lines revealed");
    expect(sawComplete, "results.EX TallyComplete fired");
    expect(s.isExVariant, "results.EX flag preserved");
    expectEq(s.phase, ResultsPhase::Rank, "results.EX in Rank after tally");
}

// ----- Phase 331: Werehog HUD QTE prompt machine -----
static void testEvilHudGuide()
{
    using namespace ui;
    std::cout << "\n== werehog HUD guide (QTE) ==\n";

    // Retail enum values (from EvilHudGuide.h).
    expectEq(static_cast<unsigned>(EvilGuideAction::Single), 0u, "evil.action.Single=0");
    expectEq(static_cast<unsigned>(EvilGuideAction::Chain), 1u, "evil.action.Chain=1");
    expectEq(static_cast<unsigned>(EvilGuideType::A), 0u, "evil.type.A=0");
    expectEq(static_cast<unsigned>(EvilGuideType::Y), 3u, "evil.type.Y=3");

    // Single A-prompt: shows, gets pressed correctly, completes.
    {
        EvilHudGuideState s;
        const auto opened = showEvilHudGuide(s, EvilGuideType::A,
                                             EvilGuideAction::Single);
        expect(s.isShown, "evil.shown=true after showEvilHudGuide");
        expect(s.isVisible, "evil.visible=true");
        expectEq(s.guideType, EvilGuideType::A, "evil.guideType=A");
        expectEq(opened[0].kind, EvilHudGuideEventKind::GuideShown,
                 "evil.GuideShown");

        EvilHudGuideInput in; in.aTapped = true;
        const auto evs = updateEvilHudGuideOneFrame(s, in);
        expectEq(evs[0].kind, EvilHudGuideEventKind::ChainCompleted,
                 "evil.single A press = ChainCompleted");
        expect(!s.isShown, "evil.hidden after press");
    }

    // Wrong button -> ButtonMissed and prompt hides.
    {
        EvilHudGuideState s;
        showEvilHudGuide(s, EvilGuideType::B, EvilGuideAction::Single);
        EvilHudGuideInput in; in.aTapped = true; // wrong button
        const auto evs = updateEvilHudGuideOneFrame(s, in);
        expectEq(evs[0].kind, EvilHudGuideEventKind::ButtonMissed,
                 "evil.wrong button = ButtonMissed");
        expect(!s.isShown, "evil.hidden after miss");
    }

    // Chain QTE: 3-press X chain.
    {
        EvilHudGuideState s;
        showEvilHudGuide(s, EvilGuideType::X, EvilGuideAction::Chain, 3);
        expectEq(s.chainPressesRemaining, 3, "evil.chain.3 remaining at start");

        // First press.
        {
            EvilHudGuideInput in; in.xTapped = true;
            const auto evs = updateEvilHudGuideOneFrame(s, in);
            expectEq(evs[0].kind, EvilHudGuideEventKind::ChainAdvanced,
                     "evil.chain.first = ChainAdvanced");
            expectEq(s.chainPressesRemaining, 2, "evil.chain.2 remaining");
        }
        // Second press.
        {
            EvilHudGuideInput in; in.xTapped = true;
            const auto evs = updateEvilHudGuideOneFrame(s, in);
            expectEq(evs[0].kind, EvilHudGuideEventKind::ChainAdvanced,
                     "evil.chain.second = ChainAdvanced");
            expectEq(s.chainPressesRemaining, 1, "evil.chain.1 remaining");
        }
        // Third press completes the chain.
        {
            EvilHudGuideInput in; in.xTapped = true;
            const auto evs = updateEvilHudGuideOneFrame(s, in);
            expectEq(evs[0].kind, EvilHudGuideEventKind::ChainCompleted,
                     "evil.chain.third = ChainCompleted");
            expect(!s.isShown, "evil.chain hidden after completion");
        }
    }

    // QTE timeout in middle of chain -> ButtonMissed.
    {
        EvilHudGuideState s;
        showEvilHudGuide(s, EvilGuideType::Y, EvilGuideAction::Chain, 2);
        EvilHudGuideInput in; in.qteTimedOut = true;
        const auto evs = updateEvilHudGuideOneFrame(s, in);
        expectEq(evs[0].kind, EvilHudGuideEventKind::ButtonMissed,
                 "evil.timeout = ButtonMissed");
        expect(!s.isShown, "evil.hidden on timeout");
    }

    // CEvilSonicContext companion fields propagate through state.
    {
        EvilHudGuideState s;
        s.darkGaiaEnergy = 0.75f;
        s.outOfControlCount = 3;
        s.animationId = 0x100;
        expectEq(s.darkGaiaEnergy, 0.75f, "evil.context.darkGaiaEnergy");
        expectEq(s.outOfControlCount, 3u, "evil.context.outOfControlCount");
        expectEq(s.animationId, 0x100u, "evil.context.animationId");
    }

    // StageHudState now carries the Werehog companion fields too.
    {
        StageHudState s; s.mode = StageMode::Werehog;
        s.darkGaiaEnergy = 0.5f;
        s.outOfControlCount = 2;
        expectEq(s.darkGaiaEnergy, 0.5f, "stage.werehog.darkGaiaEnergy carried");
        expectEq(s.outOfControlCount, 2u, "stage.werehog.outOfControlCount carried");
    }
}

// ----- Phase 333: SaveIcon retail port -----
static void testSaveIcon()
{
    using namespace ui;
    std::cout << "\n== save icon ==\n";
    SaveIconState s;
    expect(!s.isVisible, "save.starts hidden");

    {
        const auto evs = showSaveIcon(s, 0.5f);
        expect(s.isVisible, "save.shown");
        expectEq(evs[0].kind, SaveIconEventKind::Shown, "save.Shown event");
        expectEq(s.visibleSecondsRemaining, 0.5f, "save.timer set");
    }
    // Tick 0.3s -> still visible.
    {
        const auto evs = updateSaveIconOneFrame(s, 0.3f);
        expect(s.isVisible, "save.still visible after 0.3s");
        expect(evs.empty(), "save.no event yet");
    }
    // Tick 0.3s more -> auto-hide fires.
    {
        const auto evs = updateSaveIconOneFrame(s, 0.3f);
        expect(!s.isVisible, "save.auto-hidden after timer");
        expectEq(evs[0].kind, SaveIconEventKind::Hidden, "save.Hidden event");
    }
    // Re-show resets timer.
    {
        showSaveIcon(s, 1.0f);
        expect(s.isVisible, "save.re-shown");
        showSaveIcon(s, 2.0f); // updates timer without re-firing
        expectEq(s.visibleSecondsRemaining, 2.0f, "save.timer extended without re-fire");
    }
}

// ----- Phase 333: SoundAdministrator BGM channels -----
static void testBgmAdmin()
{
    using namespace ui;
    std::cout << "\n== sound admin BGM ==\n";
    SgfxBgmAdmin admin;

    // Default volumes are 1.0.
    expectEq(admin.getChannelVolume(1), 1.0f, "bgm.ch1 default 1.0");
    expectEq(admin.getChannelVolume(4), 1.0f, "bgm.ch4 default 1.0");

    // Set + clamp.
    admin.setChannelVolume(2, 0.5f);
    expectEq(admin.getChannelVolume(2), 0.5f, "bgm.ch2 = 0.5");
    admin.setChannelVolume(3, 1.5f); // over 1.0, should clamp
    expectEq(admin.getChannelVolume(3), 1.0f, "bgm.ch3 clamped to 1.0");
    admin.setChannelVolume(1, -0.2f); // under 0, should clamp
    expectEq(admin.getChannelVolume(1), 0.0f, "bgm.ch1 clamped to 0.0");

    // Mount cues per channel.
    admin.mountCue(kBgmChannelMain, "bgm_act_apotos");
    admin.mountCue(kBgmChannelResults, "bgm_sys_result");
    expectEq(admin.cueAt(1), std::string("bgm_act_apotos"), "bgm.main cue");
    expectEq(admin.cueAt(3), std::string("bgm_sys_result"), "bgm.results cue");

    // Channel constants match retail-observed convention.
    expectEq(kBgmChannelMain, 1, "bgm.main constant=1");
    expectEq(kBgmChannelResults, 3, "bgm.results constant=3");
    expectEq(kBgmChannelAttract, 4, "bgm.attract constant=4");
}

// ----- Phase 341: Werehog stage HUD overlays -----
static void testEvilStageHud()
{
    using namespace ui;
    std::cout << "\n== werehog stage HUD ==\n";
    EvilStageHudState s;
    s.base.mode = StageMode::Werehog;
    s.darkGaiaEnergy = 0.5f;
    s.outOfControlCount = 2;

    // Combo increments visible.
    {
        const auto evs = evilStageHudIncrementCombo(s);
        expect(s.combo.isVisible, "evil.combo.visible after first hit");
        expectEq(s.combo.comboCount, 1, "evil.combo.count=1");
        expectEq(evs[0].kind, EvilStageHudEventKind::ComboIncremented, "evil.ComboIncremented");
    }
    // Multi-hit then grace-window expires -> combo resets.
    evilStageHudIncrementCombo(s);
    evilStageHudIncrementCombo(s);
    expectEq(s.combo.comboCount, 3, "evil.combo.count=3");
    {
        StageHudInput in; in.deltaSeconds = 2.0f; // > graceWindow 1.5s
        const auto evs = updateEvilStageHudOneFrame(s, in, false);
        expect(!s.combo.isVisible, "evil.combo hidden after grace");
        expectEq(s.combo.comboCount, 0, "evil.combo.count=0 after grace");
        bool sawReset = false;
        for (const auto& e : evs)
            if (e.kind == EvilStageHudEventKind::ComboReset) sawReset = true;
        expect(sawReset, "evil.ComboReset fired");
    }

    // Chance Attack: prompt -> success on correct press.
    {
        evilStageHudShowChancePrompt(s, EvilGuideType::Y);
        expectEq(s.chanceAttack.phase, EvilChanceAttackPhase::Prompt, "evil.chance.Prompt");
        StageHudInput in; in.deltaSeconds = 0.05f;
        const auto evs = updateEvilStageHudOneFrame(s, in, /*chanceTapped*/true);
        expectEq(s.chanceAttack.phase, EvilChanceAttackPhase::Success, "evil.chance.Success");
        bool sawSuccess = false;
        for (const auto& e : evs)
            if (e.kind == EvilStageHudEventKind::ChanceAttackSuccess) sawSuccess = true;
        expect(sawSuccess, "evil.ChanceAttackSuccess event");
    }
    // Chance Attack: timeout -> Failed.
    {
        EvilStageHudState s2;
        evilStageHudShowChancePrompt(s2, EvilGuideType::A);
        StageHudInput in; in.deltaSeconds = 1.0f; // > 0.8s window
        const auto evs = updateEvilStageHudOneFrame(s2, in, false);
        expectEq(s2.chanceAttack.phase, EvilChanceAttackPhase::Failed, "evil.chance.Failed on timeout");
        bool sawFailed = false;
        for (const auto& e : evs)
            if (e.kind == EvilStageHudEventKind::ChanceAttackFailed) sawFailed = true;
        expect(sawFailed, "evil.ChanceAttackFailed event");
    }

    // Targeting reticle.
    {
        EvilStageHudState s2;
        const auto evs = evilStageHudAcquireTarget(s2, 640.0f, 360.0f);
        expect(s2.target.isVisible, "evil.target.visible");
        expectEq(s2.target.screenX, 640.0f, "evil.target.x=640");
        expectEq(evs[0].kind, EvilStageHudEventKind::TargetAcquired, "evil.TargetAcquired");

        StageHudInput in; in.deltaSeconds = 0.5f;
        updateEvilStageHudOneFrame(s2, in, false);
        expectEq(s2.target.lockProgress, 1.0f, "evil.target.lockProgress=1 after dwell");

        const auto evs2 = evilStageHudReleaseTarget(s2);
        expect(!s2.target.isVisible, "evil.target.released");
        expectEq(evs2[0].kind, EvilStageHudEventKind::TargetLost, "evil.TargetLost");
    }

    // Werehog companion fields propagate from EvilStageHudState to QTE.
    {
        EvilStageHudState s2;
        s2.darkGaiaEnergy = 0.75f;
        s2.outOfControlCount = 5;
        StageHudInput in; in.deltaSeconds = 0.0f;
        updateEvilStageHudOneFrame(s2, in, false);
        expectEq(s2.qtePrompt.darkGaiaEnergy, 0.75f, "evil.qte.darkGaiaEnergy mirrored");
        expectEq(s2.qtePrompt.outOfControlCount, 5u, "evil.qte.outOfControlCount mirrored");
    }
}

// ----- Phase 341: Status overlay -----
static void testStatusOverlay()
{
    using namespace ui;
    std::cout << "\n== status overlay ==\n";
    StatusState s;
    expectEq(s.phase, StatusPhase::Closed, "status.starts Closed");

    {
        const auto evs = openStatusOverlay(s, 100);
        expectEq(s.phase, StatusPhase::Browsing, "status.opens to Browsing");
        expectEq(s.availableSkillPoints, 100, "status.availablePoints=100");
        expectEq(evs[0].kind, StatusEventKind::Opened, "status.Opened");
    }
    {
        const auto evs = hoverStatusSkill(s, 7);
        expectEq(s.hoveredSkillId, 7, "status.hoveredSkillId=7");
        expectEq(evs[0].kind, StatusEventKind::SkillHovered, "status.SkillHovered");
    }
    {
        StatusInput in; in.acceptTapped = true;
        const auto evs = updateStatusOverlayOneFrame(s, in);
        expectEq(s.phase, StatusPhase::Confirming, "status.advance to Confirming");
        expectEq(s.selectedSkillId, 7, "status.selectedSkillId=7");
        expectEq(evs[0].kind, StatusEventKind::SkillUpgradeConfirmed, "status.UpgradeConfirmed");
    }
    s.pendingCostPoints = 20;
    {
        StatusInput in; in.acceptTapped = true;
        const auto evs = updateStatusOverlayOneFrame(s, in);
        expectEq(s.phase, StatusPhase::LevelingUp, "status.advance to LevelingUp");
        expectEq(s.availableSkillPoints, 80, "status.points deducted (100-20)");
    }
    {
        StatusInput in; in.deltaSeconds = 1.5f;
        const auto evs = updateStatusOverlayOneFrame(s, in);
        expectEq(s.phase, StatusPhase::Browsing, "status.returns to Browsing");
        expect(!evs.empty(), "status.SkillUpgradeApplied fired");
        expectEq(evs[0].kind, StatusEventKind::SkillUpgradeApplied, "status.UpgradeApplied");
    }
    // Cancel from Browsing closes the overlay.
    {
        StatusInput in; in.cancelTapped = true;
        const auto evs = updateStatusOverlayOneFrame(s, in);
        expectEq(s.phase, StatusPhase::Closed, "status.cancel closes");
        expectEq(evs[0].kind, StatusEventKind::Closed, "status.Closed");
    }
}

// ----- Phase 341: HelpWindow -----
static void testHelpWindow()
{
    using namespace ui;
    std::cout << "\n== help window ==\n";
    HelpWindowState s;
    expectEq(s.phase, HelpWindowPhase::Closed, "help.starts Closed");

    {
        const auto evs = openHelpWindow(s, 0, 3);
        expectEq(s.phase, HelpWindowPhase::Opening, "help.opens to Opening");
        expectEq(s.currentTopicId, 0, "help.topicId=0");
        expectEq(s.topicCount, 3, "help.topicCount=3");
        expectEq(evs[0].kind, HelpWindowEventKind::Opened, "help.Opened");
    }
    // Tick past opening duration -> Visible.
    {
        HelpWindowInput in; in.deltaSeconds = 0.3f;
        updateHelpWindowOneFrame(s, in);
        expectEq(s.phase, HelpWindowPhase::Visible, "help.advances to Visible");
    }
    // Right-tap cycles topic.
    {
        HelpWindowInput in; in.rightTapped = true;
        const auto evs = updateHelpWindowOneFrame(s, in);
        expectEq(s.currentTopicId, 1, "help.topicId=1 after right");
        expectEq(evs[0].kind, HelpWindowEventKind::TopicChanged, "help.TopicChanged");
    }
    // Left-tap wraps backwards.
    {
        HelpWindowInput in; in.leftTapped = true;
        updateHelpWindowOneFrame(s, in);
        expectEq(s.currentTopicId, 0, "help.topicId=0 after left");
    }
    {
        HelpWindowInput in; in.leftTapped = true;
        updateHelpWindowOneFrame(s, in);
        expectEq(s.currentTopicId, 2, "help.topicId=2 after left wrap");
    }
    // Cancel transitions to Closing.
    {
        HelpWindowInput in; in.cancelTapped = true;
        const auto evs = updateHelpWindowOneFrame(s, in);
        expectEq(s.phase, HelpWindowPhase::Closing, "help.advances to Closing");
        expectEq(evs[0].kind, HelpWindowEventKind::Closed, "help.Closed event on cancel");
    }
    // Tick past closing -> Closed.
    {
        HelpWindowInput in; in.deltaSeconds = 0.3f;
        updateHelpWindowOneFrame(s, in);
        expectEq(s.phase, HelpWindowPhase::Closed, "help.fully Closed");
    }
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
    testPadState();
    testWorldMapCamera();
    testTitleIntroAttractMovie();
    testResultsScreenEx();
    testEvilHudGuide();
    testSaveIcon();
    testBgmAdmin();
    testEvilStageHud();
    testStatusOverlay();
    testHelpWindow();
    std::cout << "\nfailures: " << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
