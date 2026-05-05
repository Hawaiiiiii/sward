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
    // Phase 323 retail-fidelity correction: enum values + ordering
    // mined directly from local_build_env/ur103clean/UnleashedRecomp/
    // api/SWA/HUD/Pause/HudPause.h. The earlier draft (Phase 304)
    // had WRONG values for every member of every enum; the runtime
    // uses these exact integer values, including the gaps (Quit=2
    // not 1, Restart=8 not 7, Dialog=5 not consecutive).
    //
    // SWA::EMenuType (calling context).
    enum class PauseMenuContext : std::uint32_t
    {
        WorldMap = 0,
        Village  = 1,
        Stage    = 2,
        Hub      = 3,
        Misc     = 4,
    };

    // SWA::ETransitionType. The retail enum has gaps: Quit=2 (no 1),
    // Dialog=5 (no 3,4). SGFX preserves these exact values so the
    // host can mirror the runtime's expected wire format.
    enum class PauseTransition : std::uint32_t
    {
        Undefined = 0,
        Quit      = 2,  // WorldMap / Stage / Misc context
        Dialog    = 5,  // Modal dialog overlay
        Hide      = 6,  // Village / Hub context, or Options selected
        Abort     = 7,
        SubMenu   = 8,  // Achievements / nested menu
    };

    // SWA::EActionType. Nine values total; runtime uses these to
    // signal what "kind of return" the player has requested when
    // they back out of the pause overlay.
    enum class PauseAction : std::uint32_t
    {
        Undefined = 0,
        Status    = 1, // open status sub-screen
        Return    = 2, // back out
        Inventory = 3,
        Skills    = 4,
        Lab       = 5,
        Wait      = 6,
        Restart   = 8, // gap on 7 in retail
        Continue  = 9,
    };

    // SWA::EStatusType. Three states. SGFX previously didn't model
    // Decline; runtime uses it when the player cancels a sub-prompt.
    enum class PauseStatus : std::uint32_t
    {
        Idle    = 0,
        Accept  = 1,
        Decline = 2,
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
        // Phase 323: retail's EMenuType has no Undefined value;
        // WorldMap is the zero entry. Host should always set an
        // explicit context before pause becomes visible.
        PauseMenuContext context = PauseMenuContext::WorldMap;
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
                                  std::string(kPauseSfxCloseWindow)}); // pausewinclose first
                events.push_back({PauseEventKind::BackedOut,
                                  state.cursorIndex,
                                  std::string(kPauseSfxBack)});      // then pausecansel
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
                                  std::string(kPauseSfxCloseWindow)}); // pausewinclose first
                events.push_back({PauseEventKind::BackedOut,
                                  state.cursorIndex,
                                  std::string(kPauseSfxBack)});      // then pausecansel
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
        // sub-menu engaged). Phase 323 retail-fidelity update from
        // captured trace (frame 7432, t=171.60s): the runtime fires
        // sys_actstg_pausewinclose AND sys_actstg_pausecansel on
        // the same frame in this order. Earlier draft only emitted
        // pausecansel.
        if (input.cancelTapped)
        {
            state.lastAction = PauseAction::Return;
            state.lastTransition = transitionForContext(state.context);
            const auto kind = (state.lastTransition == PauseTransition::Quit)
                ? PauseEventKind::QuitTransition
                : PauseEventKind::HideTransition;
            events.push_back({kind, state.cursorIndex,
                              std::string(kPauseSfxCloseWindow)});  // pausewinclose first
            events.push_back({kind, state.cursorIndex,
                              std::string(kPauseSfxBack)});         // then pausecansel
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
