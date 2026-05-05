// Phase 310 smoke test: drive the SGFX orchestrator through the
// full game flow Title -> WorldMap -> Loading -> StageHud -> Pause
// -> Results -> WorldMap. Then re-enter via Hub for the Day/Werehog
// hub path. Pure state machines + pure events; no game/runtime
// dependency.

#include "sward/ui_runtime/sgfx_orchestrator.hpp"

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
    std::cout << "\n";
    if (!ok) ++g_failures;
}

int main()
{
    using namespace ui;

    SgfxOrchestrator o;
    expectEq(o.current, SgfxScreen::Title, "starts on Title");

    // Title: with save data present, default cursor at NewGame (0).
    // Move down to Continue (1).
    applyTitleMenuVisibility(o.title, true, false, false, true);
    {
        SgfxFrameInput in; in.downTapped = true;
        updateSgfxOrchestratorOneFrame(o, in);
    }
    expectEq(o.title.cursorIndex, 1, "title cursor at Continue");

    // Accept Continue -> WorldMap.
    {
        SgfxFrameInput in; in.acceptTapped = true;
        const auto evs = updateSgfxOrchestratorOneFrame(o, in);
        bool sawEnter = false;
        for (const auto& e : evs)
            if (e.kind == SgfxOrchestratorEventKind::ScreenEntered
                && e.screen == SgfxScreen::WorldMap)
                sawEnter = true;
        expect(sawEnter, "WorldMap entered");
        expectEq(o.current, SgfxScreen::WorldMap, "current = WorldMap");
    }

    // WorldMap: accept on the default continent -> Loading.
    {
        SgfxFrameInput in; in.acceptTapped = true;
        updateSgfxOrchestratorOneFrame(o, in);
    }
    // The first accept opens the stage panel; second accept doesn't
    // re-fire StageOpened. We enter Loading via the StageOpened path
    // directly (the orchestrator schedules the screen swap).
    expectEq(o.current, SgfxScreen::Loading, "current = Loading after stage open");

    // Loading: progress to 1, accept, fade to complete -> StageHud.
    {
        SgfxFrameInput in; in.loadingProgress = 1.0f;
        updateSgfxOrchestratorOneFrame(o, in);
    }
    {
        SgfxFrameInput in; in.startTapped = true; in.loadingProgress = 1.0f;
        updateSgfxOrchestratorOneFrame(o, in);
    }
    {
        SgfxFrameInput in; in.deltaSeconds = 1.0f; in.loadingProgress = 1.0f;
        updateSgfxOrchestratorOneFrame(o, in);
    }
    expectEq(o.current, SgfxScreen::StageHud, "current = StageHud");

    // StageHud: simulate ring pickup + score; press Start -> Pause.
    applyRingDelta(o.stageHud, 50);
    applyScoreDelta(o.stageHud, 10000);
    {
        SgfxFrameInput in; in.startTapped = true;
        updateSgfxOrchestratorOneFrame(o, in);
    }
    expectEq(o.current, SgfxScreen::Pause, "current = Pause");
    expectEq(o.pause.context, PauseMenuContext::Stage, "pause context = Stage");

    // Pause: cursor to count-1 (Quit), accept -> WorldMap.
    o.pause.cursorIndex = o.pause.itemCount - 1;
    {
        SgfxFrameInput in; in.acceptTapped = true;
        updateSgfxOrchestratorOneFrame(o, in);
    }
    expectEq(o.current, SgfxScreen::WorldMap, "current = WorldMap after Pause Quit");

    // Host signal: stage finished -> Results.
    o.current = SgfxScreen::StageHud; // pretend we re-entered
    sgfxOrchestratorFinishStage(o, ResultsRank::A, 100000, 75.5f, 99, 250000);
    expectEq(o.current, SgfxScreen::Results, "current = Results");

    // Results: tick the tally, then acknowledge -> WorldMap.
    o.results.secondsBetweenLines = 0.05f;
    for (int i = 0; i < 50; ++i)
    {
        SgfxFrameInput in; in.deltaSeconds = 0.1f;
        updateSgfxOrchestratorOneFrame(o, in);
        if (o.results.tallyComplete) break;
    }
    expect(o.results.tallyComplete, "results tally completed");
    {
        SgfxFrameInput in; in.acceptTapped = true;
        updateSgfxOrchestratorOneFrame(o, in);
    }
    expectEq(o.current, SgfxScreen::WorldMap, "current = WorldMap after Results");

    // Hub flow: host enters hub, talk to NPC, confirm a stage gate.
    sgfxOrchestratorEnterHub(o, HubMode::Werehog);
    expectEq(o.current, SgfxScreen::Hub, "current = Hub");
    expectEq(o.hub.mode, HubMode::Werehog, "hub mode = Werehog");
    {
        SgfxFrameInput in; in.talkTapped = true;
        updateSgfxOrchestratorOneFrame(o, in);
    }
    expectEq(o.hub.overlay, HubOverlay::BalloonText, "hub balloon opened");

    // Open a stage gate, confirm -> Loading.
    o.hub.overlay = HubOverlay::None;
    openHubOverlay(o.hub, HubOverlay::StageGate);
    {
        SgfxFrameInput in; in.acceptTapped = true;
        updateSgfxOrchestratorOneFrame(o, in);
    }
    expectEq(o.current, SgfxScreen::Loading, "Hub gate -> Loading");

    std::cout << "\nfailures: " << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
