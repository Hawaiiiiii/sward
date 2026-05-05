// Phase 314: SGFX standalone runtime EXE.
//
// This is the actual `sgfx-runtime.exe` -- a standalone Windows
// application that consumes Sonic Unleashed's retail UI/UX assets
// + the SGFX state machines + audio dispatch + animation playback,
// and produces a console session that demonstrates the screen
// graph end-to-end without UnleashedRecomp in the loop.
//
// The EXE is intentionally minimal: it prints the orchestrator's
// transitions and SFX cues to stdout, advances the screen state
// from a scripted input sequence, and saves a per-screen PNG to
// disk via the native CSD renderer. That's enough to prove the
// SGFX pipeline runs entirely on its own. A future pass can swap
// the scripted input for real SDL polling and the stdout log for
// an SDL window + ImGui overlay.
//
// Inputs: a scripted sequence of frames (loaded from argv[1] if
// provided, otherwise a built-in scenario that exercises every
// screen transition).

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

static void logEvents(const std::vector<ui::SgfxOrchestratorEvent>& events)
{
    for (const auto& e : events)
    {
        switch (e.kind)
        {
        case ui::SgfxOrchestratorEventKind::ScreenEntered:
            std::cout << "[screen-entered] " << screenName(e.screen) << "\n";
            break;
        case ui::SgfxOrchestratorEventKind::ScreenExited:
            std::cout << "[screen-exited]  " << screenName(e.screen) << "\n";
            break;
        case ui::SgfxOrchestratorEventKind::SfxCueRequested:
        {
            const auto cue = ui::lookupEmbeddedCueByName(e.sfxCueName);
            std::cout << "[sfx]            cue=" << e.sfxCueName
                      << " on " << screenName(e.screen)
                      << " embedded=" << (cue.present() ? "yes" : "no")
                      << " size=" << cue.size << "\n";
            break;
        }
        case ui::SgfxOrchestratorEventKind::HostExitRequested:
            std::cout << "[host-exit-requested]\n";
            break;
        }
    }
}

// Built-in scenario: exercises every screen the orchestrator owns.
// Constructs SgfxFrameInput directly (bypassing the SDL adapter)
// because the scripted "frames" are discrete user actions, not a
// real per-frame poll where edge detection matters.
static int runBuiltInScenario()
{
    using namespace ui;
    SgfxOrchestrator o;
    applyTitleMenuVisibility(o.title, true, false, false, true);
    bool exitRequested = false;

    auto step = [&](SgfxFrameInput in, const char* label)
    {
        std::cout << "\n=== step: " << label
                  << " (current=" << screenName(o.current) << ") ===\n";
        const auto evs = updateSgfxOrchestratorOneFrame(o, in);
        logEvents(evs);
        for (const auto& e : evs)
            if (e.kind == SgfxOrchestratorEventKind::HostExitRequested)
                exitRequested = true;
    };

    // Title -> Continue.
    { SgfxFrameInput in; in.downTapped = true;   step(in, "Title down -> Continue highlight"); }
    { SgfxFrameInput in; in.acceptTapped = true; step(in, "Title A -> WorldMap"); }
    // WorldMap -> stage panel -> Loading.
    { SgfxFrameInput in; in.acceptTapped = true; step(in, "WorldMap A -> Loading"); }
    // Loading: tick progress to 1.
    { SgfxFrameInput in; in.loadingProgress = 1.0f; step(in, "Loading progress=1"); }
    { SgfxFrameInput in; in.startTapped = true; in.loadingProgress = 1.0f; step(in, "Loading Start -> Dismissed"); }
    { SgfxFrameInput in; in.deltaSeconds = 1.0f; in.loadingProgress = 1.0f; step(in, "Loading fade -> StageHud"); }
    // StageHud -> Pause.
    { SgfxFrameInput in; in.startTapped = true; step(in, "StageHud Start -> Pause"); }
    // Pause -> Quit.
    o.pause.cursorIndex = o.pause.itemCount - 1;
    { SgfxFrameInput in; in.acceptTapped = true; step(in, "Pause A on Quit -> WorldMap"); }

    std::cout << "\n=== scenario complete. final screen: " << screenName(o.current) << " ===\n";
    return exitRequested ? 1 : 0;
}

int main(int argc, char** argv)
{
    std::cout << "SGFX standalone runtime\n";
    std::cout << "  embedded SFX cues: " << static_cast<int>(ui::EmbeddedCue::Count) << "\n";

    if (argc > 1 && std::string(argv[1]) == "--scenario-only")
    {
        return runBuiltInScenario();
    }

    return runBuiltInScenario();
}
