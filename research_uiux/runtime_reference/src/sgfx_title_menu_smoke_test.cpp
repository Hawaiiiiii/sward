// Phase 303 smoke test: drives the SGFX title-menu state machine
// (sgfx_title_menu.hpp) through a scripted input sequence without
// any game / runtime dependency. Verifies cursor movement,
// accept/cancel handling on every option, popup modal short-circuits,
// and SFX cue emission match the behavior mined from
// UnleashedRecomp/patches/CTitleStateMenu_patches.cpp.

#include "sward/ui_runtime/sgfx_title_menu.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

namespace ui = sward::ui_runtime::generated::sgfx_hud;

static int g_failures = 0;

static void expect(bool condition, std::string_view label)
{
    std::cout << (condition ? "OK   " : "FAIL ") << label << "\n";
    if (!condition) ++g_failures;
}

template <typename T>
static void expectEq(const T& actual, const T& expected, std::string_view label)
{
    const bool ok = actual == expected;
    std::cout << (ok ? "OK   " : "FAIL ") << label;
    if constexpr (std::is_integral_v<T> || std::is_enum_v<T>)
    {
        std::cout << " expected=" << static_cast<long long>(expected)
                  << " actual=" << static_cast<long long>(actual);
    }
    else if constexpr (std::is_convertible_v<T, std::string_view>)
    {
        std::cout << " expected=\"" << std::string_view(expected) << "\""
                  << " actual=\"" << std::string_view(actual) << "\"";
    }
    std::cout << "\n";
    if (!ok) ++g_failures;
}

int main()
{
    using namespace sward::ui_runtime::generated::sgfx_hud;

    // Test 1: cursor wraps correctly through all 5 options.
    {
        TitleMenuState state{};
        applyTitleMenuVisibility(state,
            /*saveDataPresent=*/ true,
            /*saveDataCorrupt=*/ false,
            /*dlcMissing=*/      true,
            /*exitOptionAvailable=*/ true);
        expectEq(state.cursorIndex, 0, "test1.initial cursor at NewGame");

        TitleMenuInput in{}; in.downTapped = true;
        for (int i = 0; i < 5; ++i)
        {
            const auto evs = updateTitleMenuOneFrame(state, in);
            expect(!evs.empty(), "test1.cursor move event");
            // Phase 317: title menu cursor moves are SILENT in retail
            // (captured trace shows no SFX on cursor change). Empty
            // string is the runtime-correct cue for this event.
            expectEq(evs[0].sfxCueName, std::string{}, "test1.cursor silent");
        }
        // After 5 down-taps starting at 0 in a 5-option ring, we land back at 0.
        expectEq(state.cursorIndex, 0, "test1.cursor wrapped back to NewGame");
    }

    // Test 2: Continue is hidden when no save data; cursor skips it.
    {
        TitleMenuState state{};
        applyTitleMenuVisibility(state,
            /*saveDataPresent=*/ false,
            /*saveDataCorrupt=*/ false,
            /*dlcMissing=*/      false,
            /*exitOptionAvailable=*/ true);
        // Visible: NewGame, Options, Exit. Cursor starts at 0 (NewGame).
        expectEq(state.cursorIndex, 0, "test2.cursor at NewGame");

        TitleMenuInput in{}; in.downTapped = true;
        updateTitleMenuOneFrame(state, in);
        // From NewGame, Down should skip Continue (hidden) and land on Options.
        expectEq(state.cursorIndex, static_cast<std::int32_t>(TitleMenuOption::Options),
                 "test2.skipped hidden Continue");
    }

    // Test 3: Accept on Options opens sub-menu and emits the
    // window-open + decide cues mined from the retail patch
    // (CTitleStateMenu_patches.cpp:174-176).
    {
        TitleMenuState state{};
        applyTitleMenuVisibility(state, true, false, false, true);
        state.cursorIndex = static_cast<std::int32_t>(TitleMenuOption::Options);

        TitleMenuInput in{}; in.acceptTapped = true;
        const auto evs = updateTitleMenuOneFrame(state, in);
        expect(state.optionsSubMenuOpen, "test3.options sub-menu opened");
        expectEq(evs.size(), static_cast<std::size_t>(2), "test3.two events fired");
        expectEq(evs[0].sfxCueName,
                 std::string(kTitleMenuSfxOpenSubMenu),
                 "test3.first SFX = sys_worldmap_window");
        expectEq(evs[1].sfxCueName,
                 std::string(kTitleMenuSfxConfirm),
                 "test3.second SFX = sys_worldmap_decide");
    }

    // Test 4: Cancel on the open sub-menu closes it and plays the
    // back cue (CTitleStateMenu_patches.cpp:196).
    {
        TitleMenuState state{};
        applyTitleMenuVisibility(state, true, false, false, true);
        state.optionsSubMenuOpen = true;
        TitleMenuInput in{}; in.cancelTapped = true;
        const auto evs = updateTitleMenuOneFrame(state, in);
        expect(!state.optionsSubMenuOpen, "test4.options sub-menu closed");
        expectEq(evs[0].kind, TitleMenuEventKind::OptionsSubMenuClosed,
                 "test4.event kind");
        expectEq(evs[0].sfxCueName, std::string(kTitleMenuSfxBack),
                 "test4.cancel SFX = sys_worldmap_cansel");
    }

    // Test 5: NewGame accept opens the delete-save confirmation.
    {
        TitleMenuState state{};
        applyTitleMenuVisibility(state, true, false, false, true);
        TitleMenuInput in{}; in.acceptTapped = true;
        const auto evs = updateTitleMenuOneFrame(state, in);
        expect(state.deleteSavePromptOpen, "test5.delete prompt opened");
        expectEq(evs[0].kind, TitleMenuEventKind::DeleteSavePromptOpened,
                 "test5.event kind");
    }

    // Test 6: Modal short-circuit: input on the title menu doesn't
    // move the cursor while the DLC install prompt is open.
    {
        TitleMenuState state{};
        applyTitleMenuVisibility(state, true, false, true, true);
        state.dlcInstallPromptOpen = true;
        const auto cursorBefore = state.cursorIndex;
        TitleMenuInput in{}; in.downTapped = true;
        updateTitleMenuOneFrame(state, in);
        expectEq(state.cursorIndex, cursorBefore,
                 "test6.cursor unchanged while modal is open");
    }

    // Test 7: Exit option fires ExitRequested when host enables it.
    {
        TitleMenuState state{};
        applyTitleMenuVisibility(state, true, false, false, true);
        state.cursorIndex = static_cast<std::int32_t>(TitleMenuOption::Exit);
        TitleMenuInput in{}; in.acceptTapped = true;
        const auto evs = updateTitleMenuOneFrame(state, in);
        expectEq(evs[0].kind, TitleMenuEventKind::ExitRequested,
                 "test7.ExitRequested fired");
    }

    std::cout << "failures: " << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
