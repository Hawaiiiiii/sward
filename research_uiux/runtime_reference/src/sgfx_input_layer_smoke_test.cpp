// Phase 311 smoke test: feed scripted SDL-style raw snapshots into
// the input adapter and verify edge-detection / button-mapping
// produces the expected SgfxFrameInput each frame.

#include "sward/ui_runtime/sgfx_input_layer.hpp"

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

int main()
{
    using namespace ui;
    SgfxInputAdapter adapter;

    // Frame 1: no input.
    {
        SgfxRawInputSnapshot raw{};
        const auto in = adapter.update(raw);
        expect(!in.acceptTapped, "f1.no accept");
        expect(!in.cancelTapped, "f1.no cancel");
    }
    // Frame 2: A pressed -> tapped this frame.
    {
        SgfxRawInputSnapshot raw{}; raw.padA = true;
        const auto in = adapter.update(raw);
        expect(in.acceptTapped, "f2.A tapped");
    }
    // Frame 3: A still pressed -> NOT tapped (held).
    {
        SgfxRawInputSnapshot raw{}; raw.padA = true;
        const auto in = adapter.update(raw);
        expect(!in.acceptTapped, "f3.A held, no tap");
    }
    // Frame 4: A released, no events.
    {
        SgfxRawInputSnapshot raw{};
        const auto in = adapter.update(raw);
        expect(!in.acceptTapped, "f4.A released");
    }
    // Frame 5: Keyboard accept = same effect as pad A.
    {
        SgfxRawInputSnapshot raw{}; raw.keyAccept = true;
        const auto in = adapter.update(raw);
        expect(in.acceptTapped, "f5.keyboard accept tapped");
    }
    // Frame 6: D-pad down + right both tapped.
    {
        SgfxRawInputSnapshot raw{}; raw.padDpadDown = true; raw.padDpadRight = true;
        const auto in = adapter.update(raw);
        expect(in.downTapped && in.rightTapped, "f6.dpad down+right");
    }
    // Frame 7: deltaSeconds passes through unchanged.
    {
        SgfxRawInputSnapshot raw{}; raw.deltaSeconds = 0.0167f;
        const auto in = adapter.update(raw);
        expect(in.deltaSeconds == 0.0167f, "f7.deltaSeconds passes through");
    }
    // Frame 8: Talk via X button.
    {
        SgfxRawInputSnapshot raw{}; raw.padX = true;
        const auto in = adapter.update(raw);
        expect(in.talkTapped, "f8.X = talk");
    }
    // Frame 9: openMap via Back button (also fires selectTapped).
    {
        SgfxRawInputSnapshot raw{}; raw.padBack = true;
        const auto in = adapter.update(raw);
        expect(in.selectTapped, "f9.Back = select tapped");
        expect(in.openMapTapped, "f9.Back = openMap tapped");
    }
    // Frame 10: Loading progress passes through.
    {
        SgfxRawInputSnapshot raw{}; raw.loadingProgress = 0.42f;
        const auto in = adapter.update(raw);
        expect(in.loadingProgress == 0.42f, "f10.loadingProgress passes through");
    }

    // End-to-end with orchestrator: move to Continue, accept,
    // verify WorldMap transition fires. (NewGame intentionally
    // opens the delete-save prompt instead per the Phase 303 mining.)
    {
        SgfxOrchestrator o;
        applyTitleMenuVisibility(o.title, true, false, false, true);
        SgfxInputAdapter localAdapter;
        // Frame: tap Down to land on Continue.
        {
            SgfxRawInputSnapshot raw{}; raw.padDpadDown = true;
            const auto in = localAdapter.update(raw);
            updateSgfxOrchestratorOneFrame(o, in);
        }
        // Frame: tap A.
        {
            SgfxRawInputSnapshot raw{}; raw.padA = true;
            const auto in = localAdapter.update(raw);
            const auto evs = updateSgfxOrchestratorOneFrame(o, in);
            bool sawWorldMap = false;
            for (const auto& e : evs)
                if (e.kind == SgfxOrchestratorEventKind::ScreenEntered
                    && e.screen == SgfxScreen::WorldMap)
                    sawWorldMap = true;
            expect(sawWorldMap, "e2e.A on Continue -> WorldMap");
        }
    }

    std::cout << "\nfailures: " << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
