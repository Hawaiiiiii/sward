// Phase 312 smoke test: verify SGFX audio dispatch resolves the
// 7 embedded cues to non-null OGG byte slices, mirroring
// UnleashedRecomp's EmbeddedPlayer table 1:1.

#include "sward/ui_runtime/sgfx_audio_dispatch.hpp"

#include <iostream>
#include <string>

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

    constexpr std::string_view kAllCues[] = {
        "sys_worldmap_cursor",
        "sys_worldmap_finaldecide",
        "sys_actstg_pausecansel",
        "sys_actstg_pausecursor",
        "sys_actstg_pausedecide",
        "sys_actstg_pausewinclose",
        "sys_actstg_pausewinopen",
    };

    for (const auto& name : kAllCues)
    {
        const auto bytes = lookupEmbeddedCueByName(name);
        std::string label = std::string("name->bytes ") + std::string(name);
        expect(bytes.present(), label);
        std::cout << "     size=" << bytes.size << "\n";
    }

    // Slot lookup matches name lookup.
    for (std::size_t i = 0; i < static_cast<std::size_t>(EmbeddedCue::Count); ++i)
    {
        const auto bySlot = lookupEmbeddedCueBySlot(static_cast<EmbeddedCue>(i));
        const auto byName = lookupEmbeddedCueByName(kAllCues[i]);
        expect(bySlot.bytes == byName.bytes, "slot lookup matches name lookup");
        expect(bySlot.size == byName.size, "slot size matches name size");
    }

    // isEmbeddedCue gates correctly.
    expect(isEmbeddedCue("sys_worldmap_cursor"), "isEmbeddedCue true for known cue");
    expect(!isEmbeddedCue("not_a_real_cue"), "isEmbeddedCue false for unknown");

    // Cues we KNOW the state machines reference but that aren't
    // currently embedded -- e.g. sys_worldmap_window, _decide,
    // _cansel. These should resolve to NOT present, which is the
    // signal the host should fall back to non-embedded playback.
    expect(!isEmbeddedCue("sys_worldmap_window"),
           "sys_worldmap_window NOT embedded yet (host falls back)");
    expect(!isEmbeddedCue("sys_worldmap_decide"),
           "sys_worldmap_decide NOT embedded yet");
    expect(!isEmbeddedCue("sys_worldmap_cansel"),
           "sys_worldmap_cansel NOT embedded yet");

    std::cout << "\nfailures: " << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
