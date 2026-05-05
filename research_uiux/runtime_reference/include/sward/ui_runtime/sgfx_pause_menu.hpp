// Phase 304: SGFX-shaped port of SWA::CHudPause's update logic.
//
// Sourced from UnleashedRecomp/patches/CHudPause_patches.cpp:
//   * RecordHudPauseInspector exposes the state shape: m_Action /
//     m_Menu / m_Status / m_Transition + m_IsVisible / m_IsShown.
//   * InjectMenuBehaviour at lines 56-129 enumerates the calling
//     context -> transition mapping (WorldMap/Stage/Misc -> Quit;
//     Village/Hub -> Hide) and the last-2-row special handling
//     (Options at count-2, Quit/Hide at count-1).
//   * eKeyState_Select opens the Achievement menu as a sub-menu.
//
// Port shape matches sgfx_title_menu.hpp: pure update function,
// event list, no game globals.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Pause menu's calling context. Drives which transition fires
    // when the user backs out (Quit = leave the activity; Hide =
    // just dismiss the overlay).
    enum class PauseMenuContext : std::uint8_t
    {
        Undefined = 0,
        WorldMap  = 1,
        Stage     = 2,
        Misc      = 3,
        Village   = 4,
        Hub       = 5,
    };

    enum class PauseTransition : std::uint8_t
    {
        Undefined = 0,
        Quit      = 1, // WorldMap / Stage / Misc
        Hide      = 2, // Village / Hub or Options selected
        SubMenu   = 3, // Achievement menu via Select
    };

    enum class PauseAction : std::uint8_t
    {
        Undefined = 0,
        Return    = 1,
    };

    enum class PauseEventKind : std::uint8_t
    {
        CursorMoved,
        OptionAccepted,        // Generic non-special row
        OptionsOpened,         // Settings sub-menu
        AchievementsOpened,    // Select shortcut
        QuitTransition,        // Stage/WorldMap/Misc -> exit activity
        HideTransition,        // Village/Hub -> just dismiss
        BackedOut,             // B / Cancel
    };

    struct PauseInput
    {
        bool acceptTapped = false; // A or Start
        bool cancelTapped = false; // B
        bool selectTapped = false; // Back / Select -> achievements
        bool upTapped = false;
        bool downTapped = false;
    };

    struct PauseState
    {
        PauseMenuContext context = PauseMenuContext::Undefined;
        std::int32_t     cursorIndex = 0;
        std::int32_t     itemCount = 0;
        bool             isVisible = false;
        bool             isShown = false;
        PauseAction      lastAction = PauseAction::Undefined;
        PauseTransition  lastTransition = PauseTransition::Undefined;
        bool             achievementsOverlayOpen = false;
        bool             optionsOverlayOpen = false;
    };

    struct PauseEvent
    {
        PauseEventKind kind = PauseEventKind::CursorMoved;
        std::int32_t   cursorAtFire = 0;
        std::string    sfxCueName;
    };

    // Phase 311 fix-up: cue names are now the real ones grep-mined
    // from UnleashedRecomp's actual source (Game_PlaySound /
    // PlaySound call sites). Pause uses the dedicated sys_actstg_pause*
    // bank, NOT the worldmap bank that the earlier draft assumed.
    constexpr std::string_view kPauseSfxOpenWindow  = "sys_actstg_pausewinopen";
    constexpr std::string_view kPauseSfxCloseWindow = "sys_actstg_pausewinclose";
    constexpr std::string_view kPauseSfxConfirm     = "sys_actstg_pausedecide";
    constexpr std::string_view kPauseSfxBack        = "sys_actstg_pausecansel";
    constexpr std::string_view kPauseSfxCursor      = "sys_actstg_pausecursor";
    // The achievements sub-menu uses the worldmap-window cue per the
    // existing CHudPause_patches.cpp wrapper (Phase 304 mining).
    constexpr std::string_view kPauseSfxOpenSubMenu = "sys_worldmap_window";

    namespace detail::pause_menu
    {
        inline PauseTransition transitionForContext(PauseMenuContext ctx) noexcept
        {
            switch (ctx)
            {
            case PauseMenuContext::WorldMap:
            case PauseMenuContext::Stage:
            case PauseMenuContext::Misc:
                return PauseTransition::Quit;
            case PauseMenuContext::Village:
            case PauseMenuContext::Hub:
                return PauseTransition::Hide;
            default:
                return PauseTransition::Undefined;
            }
        }
    } // namespace detail::pause_menu

    inline std::vector<PauseEvent> updatePauseMenuOneFrame(
        PauseState& state,
        const PauseInput& input)
    {
        using namespace detail::pause_menu;
        std::vector<PauseEvent> events;

        // Achievement overlay short-circuits underlying input
        // (mirrors AchievementMenu::Open in the retail patch:
        // m_Transition = SubMenu so the underlying loop yields).
        if (state.achievementsOverlayOpen)
        {
            if (input.cancelTapped)
            {
                state.achievementsOverlayOpen = false;
                state.lastTransition = PauseTransition::Undefined;
                events.push_back({PauseEventKind::BackedOut,
                                  state.cursorIndex,
                                  std::string(kPauseSfxBack)});
            }
            return events;
        }

        // Options overlay similarly suspends pause-menu cursor
        // movement (the retail patch sets m_Transition = Hide).
        if (state.optionsOverlayOpen)
        {
            if (input.cancelTapped)
            {
                state.optionsOverlayOpen = false;
                state.lastTransition = PauseTransition::Undefined;
                events.push_back({PauseEventKind::BackedOut,
                                  state.cursorIndex,
                                  std::string(kPauseSfxBack)});
            }
            return events;
        }

        // Select shortcut -> achievements (CHudPause_patches.cpp:84-91).
        if (input.selectTapped)
        {
            state.achievementsOverlayOpen = true;
            state.lastAction = PauseAction::Undefined;
            state.lastTransition = PauseTransition::SubMenu;
            events.push_back({PauseEventKind::AchievementsOpened,
                              state.cursorIndex,
                              std::string(kPauseSfxOpenSubMenu)});
            return events;
        }

        // Vertical cursor movement.
        if (input.upTapped || input.downTapped)
        {
            const std::int32_t step = input.upTapped ? -1 : +1;
            std::int32_t next = state.cursorIndex + step;
            if (state.itemCount > 0)
            {
                if (next < 0) next = state.itemCount - 1;
                if (next >= state.itemCount) next = 0;
            }
            else
            {
                next = 0;
            }
            if (next != state.cursorIndex)
            {
                state.cursorIndex = next;
                events.push_back({PauseEventKind::CursorMoved,
                                  state.cursorIndex,
                                  std::string(kPauseSfxCursor)});
            }
        }

        // Cancel = back out of the pause overlay entirely (no
        // sub-menu engaged).
        if (input.cancelTapped)
        {
            state.lastAction = PauseAction::Return;
            state.lastTransition = transitionForContext(state.context);
            const auto kind = (state.lastTransition == PauseTransition::Quit)
                ? PauseEventKind::QuitTransition
                : PauseEventKind::HideTransition;
            events.push_back({kind, state.cursorIndex, std::string(kPauseSfxBack)});
            state.isVisible = false;
            state.isShown = false;
            return events;
        }

        // Accept on a row.
        if (input.acceptTapped && state.itemCount > 0)
        {
            // Last 2 rows are special (CHudPause_patches.cpp:96-122):
            //   row count-2 = Options, row count-1 = Quit/Hide.
            if (state.cursorIndex == state.itemCount - 2)
            {
                state.optionsOverlayOpen = true;
                state.lastAction = PauseAction::Undefined;
                state.lastTransition = PauseTransition::Hide;
                events.push_back({PauseEventKind::OptionsOpened,
                                  state.cursorIndex,
                                  std::string(kPauseSfxConfirm)});
            }
            else if (state.cursorIndex == state.itemCount - 1)
            {
                state.lastAction = PauseAction::Return;
                state.lastTransition = transitionForContext(state.context);
                const auto kind = (state.lastTransition == PauseTransition::Quit)
                    ? PauseEventKind::QuitTransition
                    : PauseEventKind::HideTransition;
                events.push_back({kind, state.cursorIndex,
                                  std::string(kPauseSfxConfirm)});
                state.isVisible = false;
                state.isShown = false;
            }
            else
            {
                events.push_back({PauseEventKind::OptionAccepted,
                                  state.cursorIndex,
                                  std::string(kPauseSfxConfirm)});
            }
        }

        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
