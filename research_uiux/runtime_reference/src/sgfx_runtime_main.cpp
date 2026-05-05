// Phase 314 / 334: SGFX standalone runtime EXE.
//
// This is the actual `sgfx_runtime.exe` -- a standalone Windows
// application that consumes Sonic Unleashed's retail UI/UX assets
// + the SGFX state machines + audio dispatch + animation playback,
// and produces a console session that demonstrates the screen
// graph end-to-end without UnleashedRecomp in the loop.
//
// Phase 334 expansion: the scenario now exercises EVERY screen the
// orchestrator owns, including the TitleIntro entry state, Pause
// resume (in addition to Pause Quit), Results screen via the host
// "stage finished" helper, and the Hub branch. Cross-cutting
// overlays (SaveIcon, BgmAdmin, EvilHudGuide, GeneralWindow) are
// also poked so we know the orchestrator wiring compiles + runs
// against the full Phase 326-333 set.

#include "sward/ui_runtime/sgfx_orchestrator.hpp"
#include "sward/ui_runtime/sgfx_input_layer.hpp"
#include "sward/ui_runtime/sgfx_audio_dispatch.hpp"
#include "sward/ui_runtime/sgfx_animation_playback.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace ui = sward::ui_runtime::generated::sgfx_hud;

static const char* screenName(ui::SgfxScreen s)
{
    switch (s)
    {
    case ui::SgfxScreen::TitleIntro: return "TitleIntro";
    case ui::SgfxScreen::Title:    return "Title";
    case ui::SgfxScreen::WorldMap: return "WorldMap";
    case ui::SgfxScreen::Loading:  return "Loading";
    case ui::SgfxScreen::StageHud: return "StageHud";
    case ui::SgfxScreen::Pause:    return "Pause";
    case ui::SgfxScreen::Results:  return "Results";
    case ui::SgfxScreen::Hub:      return "Hub";
    }
    return "?";
}

static int g_sfxFiredCount = 0;
static int g_screenEnteredCount = 0;
static int g_screenExitedCount = 0;
static int g_hostExitCount = 0;

static void logEvents(const std::vector<ui::SgfxOrchestratorEvent>& events)
{
    for (const auto& e : events)
    {
        switch (e.kind)
        {
        case ui::SgfxOrchestratorEventKind::ScreenEntered:
            std::cout << "[screen-entered] " << screenName(e.screen) << "\n";
            ++g_screenEnteredCount;
            break;
        case ui::SgfxOrchestratorEventKind::ScreenExited:
            std::cout << "[screen-exited]  " << screenName(e.screen) << "\n";
            ++g_screenExitedCount;
            break;
        case ui::SgfxOrchestratorEventKind::SfxCueRequested:
        {
            const auto cue = ui::lookupEmbeddedCueByName(e.sfxCueName);
            std::cout << "[sfx]            cue=" << e.sfxCueName
                      << " on " << screenName(e.screen)
                      << " embedded=" << (cue.present() ? "yes" : "no")
                      << " size=" << cue.size << "\n";
            ++g_sfxFiredCount;
            break;
        }
        case ui::SgfxOrchestratorEventKind::HostExitRequested:
            std::cout << "[host-exit-requested]\n";
            ++g_hostExitCount;
            break;
        }
    }
}

// Phase 334 full-playthrough scenario: hits every orchestrator
// screen at least once and exercises the cross-cutting overlays.
static int runFullPlaythroughScenario()
{
    using namespace ui;
    SgfxOrchestrator o;
    applyTitleMenuVisibility(o.title, true, false, false, true);
    // Make the title intro auto-advance quickly for the test.
    o.titleIntro.logoFadeInDurationSeconds = 0.001f;

    // Load up the BGM admin with a starting cue.
    o.bgmAdmin.mountCue(kBgmChannelMain, "bgm_sys_title");

    auto step = [&](SgfxFrameInput in, const char* label)
    {
        std::cout << "\n=== step: " << label
                  << " (current=" << screenName(o.current) << ") ===\n";
        const auto evs = updateSgfxOrchestratorOneFrame(o, in);
        logEvents(evs);
    };

    auto stepHostHelper = [&](std::vector<SgfxOrchestratorEvent> evs,
                              const char* label)
    {
        std::cout << "\n=== host-step: " << label
                  << " (current=" << screenName(o.current) << ") ===\n";
        logEvents(evs);
    };

    // ---- TitleIntro ----
    // Tick once with deltaSeconds past the threshold -> auto-advance to Title.
    {
        SgfxFrameInput in; in.deltaSeconds = 0.020f;
        step(in, "TitleIntro fade -> PressStartIdle");
    }
    // Press start while in PressStartIdle -> handoff to Title.
    {
        SgfxFrameInput in; in.startTapped = true;
        step(in, "TitleIntro PressStart -> Title");
    }

    // ---- Title menu ----
    // Move cursor down to Continue (idx 1). NewGame opens DeleteSavePrompt
    // by retail design; Continue goes straight to WorldMap.
    {
        SgfxFrameInput in; in.downTapped = true;
        step(in, "Title down -> Continue highlighted");
    }
    {
        SgfxFrameInput in; in.acceptTapped = true;
        step(in, "Title A -> WorldMap");
    }
    o.bgmAdmin.mountCue(kBgmChannelMain, "bgm_sys_worldmap");

    // ---- WorldMap ----
    {
        SgfxFrameInput in; in.acceptTapped = true;
        step(in, "WorldMap A -> Loading");
    }

    // ---- Loading ----
    {
        SgfxFrameInput in; in.loadingProgress = 0.5f;
        step(in, "Loading progress=0.5");
    }
    {
        SgfxFrameInput in; in.loadingProgress = 1.0f;
        step(in, "Loading progress=1.0 -> Ready");
    }
    {
        SgfxFrameInput in; in.startTapped = true; in.loadingProgress = 1.0f;
        step(in, "Loading Start -> Dismissed");
    }
    {
        SgfxFrameInput in; in.deltaSeconds = 1.0f; in.loadingProgress = 1.0f;
        step(in, "Loading fade complete -> StageHud");
    }
    o.bgmAdmin.mountCue(kBgmChannelMain, "bgm_act_apotos");

    // ---- StageHud ----
    // Werehog companion fields: poke darkGaiaEnergy + outOfControlCount.
    o.stageHud.mode = StageMode::DaySonic;
    o.stageHud.darkGaiaEnergy = 0.0f;
    {
        SgfxFrameInput in; in.deltaSeconds = 0.5f;
        step(in, "StageHud tick (gameplay)");
    }
    // Pause via Start.
    {
        SgfxFrameInput in; in.startTapped = true;
        step(in, "StageHud Start -> Pause");
    }

    // ---- Pause: resume gameplay via accept on row 0 (Continue Game) ----
    o.pause.cursorIndex = 0;
    {
        SgfxFrameInput in; in.acceptTapped = true;
        step(in, "Pause A on row 0 (Continue) -> resume StageHud");
    }

    // Tick a frame on StageHud just to confirm we returned cleanly.
    {
        SgfxFrameInput in; in.deltaSeconds = 0.1f;
        step(in, "StageHud resumed (after pause Continue)");
    }

    // ---- Now demonstrate the Quit path: pause again, accept on Quit row ----
    {
        SgfxFrameInput in; in.startTapped = true;
        step(in, "StageHud Start -> Pause (second time)");
    }
    o.pause.cursorIndex = o.pause.itemCount - 1; // Quit row
    {
        SgfxFrameInput in; in.acceptTapped = true;
        step(in, "Pause A on Quit row -> WorldMap");
    }

    // Game loops back to Stage from WorldMap; we instead branch to
    // Results via host helper to show that path. Re-enter StageHud
    // first so finishStage has something to work with.
    {
        SgfxFrameInput in; in.acceptTapped = true;
        step(in, "WorldMap A -> Loading (second run)");
    }
    // Loading: 3 frames -- progress reach Ready, accept dismiss, fade out.
    {
        SgfxFrameInput in; in.loadingProgress = 1.0f;
        step(in, "Loading progress=1.0 (second run)");
    }
    {
        SgfxFrameInput in; in.startTapped = true; in.loadingProgress = 1.0f;
        step(in, "Loading Start -> Dismissed (second run)");
    }
    {
        SgfxFrameInput in; in.deltaSeconds = 1.0f; in.loadingProgress = 1.0f;
        step(in, "Loading fade -> StageHud (second run)");
    }

    // ---- StageHud finishes -> Results (host helper) ----
    {
        const auto evs = sgfxOrchestratorFinishStage(
            o, ResultsRank::A, /*score*/120000,
            /*timeSeconds*/72.5f, /*rings*/87,
            /*totalScore*/180000);
        stepHostHelper(evs, "host: stage cleared -> Results");
    }
    o.bgmAdmin.mountCue(kBgmChannelResults, std::string(kResultsBgmSuccess));

    // ---- Results: tally + acknowledge ----
    o.results.secondsBetweenLines = 0.05f;
    for (int i = 0; i < 30; ++i)
    {
        SgfxFrameInput in; in.deltaSeconds = 0.06f;
        const auto evs = updateSgfxOrchestratorOneFrame(o, in);
        if (!evs.empty())
            logEvents(evs);
        if (o.results.tallyComplete) break;
    }
    {
        SgfxFrameInput in; in.acceptTapped = true;
        step(in, "Results A -> WorldMap");
    }

    // ---- Hub branch (host triggers entry, exercise hub overlays) ----
    {
        const auto evs = sgfxOrchestratorEnterHub(o, HubMode::DaySonic);
        stepHostHelper(evs, "host: enter Hub (Day Sonic / Apotos town)");
    }
    o.bgmAdmin.mountCue(kBgmChannelMain, "bgm_act_hub_apotos");

    // Hub: open balloon (talkTapped) then close it (cancel).
    {
        SgfxFrameInput in; in.talkTapped = true;
        step(in, "Hub talk -> BalloonText overlay");
    }
    {
        SgfxFrameInput in; in.cancelTapped = true;
        step(in, "Hub cancel -> close balloon");
    }

    // Hub -> back to WorldMap (host signal).
    {
        const auto evs = sgfxOrchestratorLeaveHubToWorldMap(o);
        stepHostHelper(evs, "host: leave Hub -> WorldMap");
    }

    // ---- Cross-cutting overlays exercised on top of any screen ----
    showSaveIcon(o.saveIcon, 0.4f);
    std::cout << "\n[overlay] SaveIcon shown for 0.4s\n";
    {
        const auto saveEvents = updateSaveIconOneFrame(o.saveIcon, 0.5f);
        std::cout << "[overlay] SaveIcon tick 0.5s -> visible="
                  << o.saveIcon.isVisible
                  << " events=" << saveEvents.size() << "\n";
    }

    showEvilHudGuide(o.evilHudGuide, EvilGuideType::B,
                     EvilGuideAction::Single);
    std::cout << "[overlay] EvilHudGuide shown (Werehog QTE: B)\n";
    {
        EvilHudGuideInput in; in.bTapped = true;
        const auto evs = updateEvilHudGuideOneFrame(o.evilHudGuide, in);
        std::cout << "[overlay] EvilHudGuide B pressed -> events="
                  << evs.size() << " visible=" << o.evilHudGuide.isVisible
                  << "\n";
    }

    openGeneralWindow(o.generalWindow, /*rowCount*/2, /*controls*/false);
    advanceGeneralWindowToDisplaying(o.generalWindow);
    {
        GeneralWindowInput in; in.acceptTapped = true;
        const auto evs = updateGeneralWindowOneFrame(o.generalWindow, in);
        std::cout << "[overlay] GeneralWindow opened+confirmed -> events="
                  << evs.size() << " status="
                  << static_cast<int>(o.generalWindow.status) << "\n";
    }

    std::cout << "\n=== scenario complete. final screen: "
              << screenName(o.current) << " ===\n";
    std::cout << "  total screen-enters:  " << g_screenEnteredCount << "\n";
    std::cout << "  total screen-exits:   " << g_screenExitedCount << "\n";
    std::cout << "  total SFX cues fired: " << g_sfxFiredCount << "\n";
    std::cout << "  host-exits requested: " << g_hostExitCount << "\n";
    return 0;
}

int main(int argc, char** argv)
{
    std::cout << "SGFX standalone runtime\n";
    std::cout << "  embedded SFX cues: "
              << static_cast<int>(ui::EmbeddedCue::Count) << "\n";

    if (argc > 1 && std::string(argv[1]) == "--scenario-only")
        return runFullPlaythroughScenario();

    return runFullPlaythroughScenario();
}
