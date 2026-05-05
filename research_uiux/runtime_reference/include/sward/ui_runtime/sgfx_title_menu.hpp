// Phase 303: SGFX-shaped port of SWA::CTitleStateMenu::Update.
//
// First state-machine port for the human-readable port. Stays
// completely standalone -- no game globals, no PPC translation
// helpers, no UnleashedRecomp dependencies. The class accepts an
// input snapshot per frame and produces a list of UI events
// (cursor moved, option accepted, option backed-out) plus a
// clean state struct the renderer can read.
//
// Sourced from:
// - UnleashedRecomp/patches/CTitleStateMenu_patches.cpp:99-202
//   for cursor indices, accept/cancel input handling, sub-menu
//   triggers, and SFX cue names.
// - PPC body sub_825882B8 (CTitleStateMenu::Update) in
//   ppc_recomp.40.cpp; the patch wraps __imp__sub_825882B8 so we
//   know the original game's behavior matches what the patch
//   describes plus the patch's added logic (DLC install option,
//   restart-required flow).
// - UI Lab probe captures (g_observedCsdProjectOrder, frame log)
//   confirming `ui_title.yncp` is the asset that backs this menu.
//
// Scope (this header): the menu's STATE TRANSITIONS only. Asset
// rendering goes through the existing native CSD renderer
// (sgfx_hud_native_csd_renderer.hpp). SFX cue dispatch is exposed
// as event-name strings; the host wires them to its audio system.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // The five title-menu options the user listed for the SGFX target
    // ("NEW FILE, CONTINUE, SETTINGS, DLC, EXIT"). The retail SU
    // CTitleStateMenu carries 4 visible options (no Exit); Exit is
    // SGFX-host-supplied so the standalone build has a clean way out
    // without quitting via the OS.
    enum class TitleMenuOption : std::uint8_t
    {
        NewGame   = 0, // "NEW FILE"
        Continue  = 1, // "CONTINUE" (suppressed when save data is corrupt)
        Options   = 2, // "SETTINGS"
        InstallDLC = 3, // "DLC" (only present when DLC is missing)
        Exit      = 4, // SGFX-only; not present in retail SU
        Count     = 5,
    };

    // One-frame snapshot of player input, normalised across keyboard
    // and gamepad. Tap = pressed THIS frame; held continues to fire.
    struct TitleMenuInput
    {
        bool acceptTapped = false;  // A or Start
        bool cancelTapped = false;  // B
        bool upTapped = false;
        bool downTapped = false;
        bool leftTapped = false;
        bool rightTapped = false;
    };

    // The reasons a one-frame Update() can transition. The host
    // consumes these to actually open the next screen / play SFX.
    enum class TitleMenuEventKind : std::uint8_t
    {
        CursorMoved,
        OptionAccepted,
        OptionBackedOut,
        DeleteSavePromptOpened,
        DeleteSavePromptClosed,
        DlcInstallPromptOpened,
        DlcInstallPromptClosed,
        OptionsSubMenuOpened,
        OptionsSubMenuClosed,
        ExitRequested,
    };

    struct TitleMenuEvent
    {
        TitleMenuEventKind kind = TitleMenuEventKind::CursorMoved;
        TitleMenuOption    optionAtFire = TitleMenuOption::NewGame;
        std::string        sfxCueName;     // e.g. "sys_worldmap_decide"; empty = no cue
    };

    // Phase 319 retail-fidelity addition: the runtime's
    // CTitleStateMenu::CTitleStateMenuContext carries a 3-stage
    // phase counter that drives the screen-transition pipeline.
    // Captured trace (phase315_take2 trace, last 5 title-menu-context
    // samples) shows context_phase advancing 0 -> 1 -> 2 on accept,
    // with context_472 going from 0 to a non-null pointer when
    // phase advances. SGFX mirrors that as a discrete enum the
    // host can read alongside the higher-level events.
    enum class TitleMenuPhase : std::uint8_t
    {
        Idle              = 0, // cursor active, menu interactive
        TransitionRequested = 1, // accept fired, target sub-state set
        TransitioningOut  = 2, // outro animation playing
    };

    // Bookkeeping the menu carries between frames.
    struct TitleMenuState
    {
        // Cursor position (raw int so we can clamp + wrap cleanly).
        // The retail asset uses 4 visible rows; SGFX adds Exit as a 5th.
        std::int32_t cursorIndex = 0;

        // Phase 319: real retail phase counter (mirrors
        // CTitleStateMenuContext::context_phase). Idle/Requested/Out.
        TitleMenuPhase phase = TitleMenuPhase::Idle;
        // Mirrors context_472: when non-zero, a sub-state pointer
        // is armed (the runtime sets this when accept fires on a
        // row that opens a sub-screen). SGFX represents it as a
        // simple bool because the actual address isn't meaningful
        // outside the runtime.
        bool transitionTargetSet = false;

        // Modal popups.
        bool deleteSavePromptOpen = false;
        bool dlcInstallPromptOpen = false;

        // Sub-menus.
        bool optionsSubMenuOpen = false;

        // Visibility filters: which options are currently selectable.
        // Driven by host: Continue is hidden when save data is missing
        // or corrupt; InstallDLC only appears when DLC is missing.
        std::array<bool, static_cast<std::size_t>(TitleMenuOption::Count)> optionVisible{
            true,  // NewGame
            true,  // Continue
            true,  // Options (Settings)
            true,  // InstallDLC
            true,  // Exit
        };

        // Mirrors CTitleStateMenu::m_IsDeleteCheckMessageOpen +
        // m_GeneralWindow->m_SelectedIndex from the retail patch:
        // when the player taps Accept on NewGame and save data
        // already exists, the game pops a "Delete existing save?"
        // confirmation. The host fills this in via OnDeleteSavePromptDecision.
        std::int32_t deleteSavePromptSelectedIndex = -1;
    };

    // Names of the actual SFX cues the retail game plays at each
    // menu transition, mined from UnleashedRecomp/patches/
    // CTitleStateMenu_patches.cpp:174-176 and 196.
    constexpr std::string_view kTitleMenuSfxOpenSubMenu = "sys_worldmap_window";
    constexpr std::string_view kTitleMenuSfxConfirm    = "sys_worldmap_decide";
    constexpr std::string_view kTitleMenuSfxBack       = "sys_worldmap_cansel";
    constexpr std::string_view kTitleMenuSfxCursor     = "sys_worldmap_cursor";

    namespace detail::title_menu
    {
        // Walk forward / backward across `optionVisible[]` skipping
        // hidden entries; wrap around at the ends. Returns the next
        // visible cursor index, or the input if no other option is
        // visible (which shouldn't happen in practice).
        inline std::int32_t advanceCursor(
            const TitleMenuState& state,
            std::int32_t fromIndex,
            std::int32_t step) noexcept
        {
            const auto count = static_cast<std::int32_t>(TitleMenuOption::Count);
            for (std::int32_t i = 0; i < count; ++i)
            {
                fromIndex = (fromIndex + step + count) % count;
                if (state.optionVisible[static_cast<std::size_t>(fromIndex)])
                    return fromIndex;
            }
            return fromIndex;
        }

        inline TitleMenuOption optionFromIndex(std::int32_t idx) noexcept
        {
            const auto count = static_cast<std::int32_t>(TitleMenuOption::Count);
            if (idx < 0 || idx >= count) return TitleMenuOption::NewGame;
            return static_cast<TitleMenuOption>(idx);
        }
    } // namespace detail::title_menu

    // One-frame state machine update. Pure function: takes the
    // previous state + this frame's input, returns the new state +
    // a list of events the host should react to (open a sub-menu,
    // play a sound, fade to a new screen). No I/O, no globals.
    inline std::vector<TitleMenuEvent> updateTitleMenuOneFrame(
        TitleMenuState& state,
        const TitleMenuInput& input)
    {
        using namespace detail::title_menu;
        std::vector<TitleMenuEvent> events;

        // Modal popups consume input first. Mirrors the retail patch's
        // ProcessInstallMessage() short-circuit at line 184-187:
        // when an install message is open, the underlying menu
        // doesn't update.
        if (state.dlcInstallPromptOpen)
        {
            if (input.cancelTapped)
            {
                state.dlcInstallPromptOpen = false;
                events.push_back({TitleMenuEventKind::DlcInstallPromptClosed,
                                  optionFromIndex(state.cursorIndex),
                                  std::string(kTitleMenuSfxBack)});
            }
            return events;
        }
        if (state.deleteSavePromptOpen)
        {
            if (input.cancelTapped)
            {
                state.deleteSavePromptOpen = false;
                state.deleteSavePromptSelectedIndex = -1;
                events.push_back({TitleMenuEventKind::DeleteSavePromptClosed,
                                  optionFromIndex(state.cursorIndex),
                                  std::string(kTitleMenuSfxBack)});
            }
            return events;
        }
        if (state.optionsSubMenuOpen)
        {
            if (input.cancelTapped)
            {
                state.optionsSubMenuOpen = false;
                events.push_back({TitleMenuEventKind::OptionsSubMenuClosed,
                                  optionFromIndex(state.cursorIndex),
                                  std::string(kTitleMenuSfxBack)});
            }
            return events;
        }

        // Vertical cursor movement.
        // Phase 317 retail correction: the captured runtime trace
        // (phase315_take2 session, frames 5362..6416) shows cursor
        // changing 0->1->2 with NO Game_PlaySound calls at those
        // frames. Title-menu cursor movement is SILENT in retail
        // SU; a CursorMoved event still fires for the host but we
        // emit an empty SFX cue string instead of sys_worldmap_cursor.
        if (input.upTapped || input.downTapped)
        {
            const std::int32_t step = input.upTapped ? -1 : +1;
            const std::int32_t next = advanceCursor(state, state.cursorIndex, step);
            if (next != state.cursorIndex)
            {
                state.cursorIndex = next;
                events.push_back({TitleMenuEventKind::CursorMoved,
                                  optionFromIndex(state.cursorIndex),
                                  std::string{}}); // silent in retail
            }
        }

        // Accept on the highlighted option. Phase 319: also
        // advances the captured retail phase counter (Idle ->
        // TransitionRequested) and sets transitionTargetSet so
        // the host can observe the same state shape the runtime
        // carries in CTitleStateMenuContext.
        if (input.acceptTapped)
        {
            const auto opt = optionFromIndex(state.cursorIndex);
            // Captured retail flow: every accept advances phase 0 -> 1
            // and arms the transition target pointer. Exact target
            // semantics depend on which option was picked.
            state.phase = TitleMenuPhase::TransitionRequested;
            state.transitionTargetSet = true;

            switch (opt)
            {
            case TitleMenuOption::NewGame:
                state.deleteSavePromptOpen = true;
                state.deleteSavePromptSelectedIndex = -1;
                events.push_back({TitleMenuEventKind::DeleteSavePromptOpened,
                                  opt, std::string(kTitleMenuSfxConfirm)});
                break;
            case TitleMenuOption::Continue:
                events.push_back({TitleMenuEventKind::OptionAccepted,
                                  opt, std::string(kTitleMenuSfxConfirm)});
                break;
            case TitleMenuOption::Options:
                state.optionsSubMenuOpen = true;
                events.push_back({TitleMenuEventKind::OptionsSubMenuOpened,
                                  opt, std::string(kTitleMenuSfxOpenSubMenu)});
                events.push_back({TitleMenuEventKind::OptionsSubMenuOpened,
                                  opt, std::string(kTitleMenuSfxConfirm)});
                break;
            case TitleMenuOption::InstallDLC:
                state.dlcInstallPromptOpen = true;
                events.push_back({TitleMenuEventKind::DlcInstallPromptOpened,
                                  opt, std::string(kTitleMenuSfxConfirm)});
                break;
            case TitleMenuOption::Exit:
                events.push_back({TitleMenuEventKind::ExitRequested,
                                  opt, std::string(kTitleMenuSfxConfirm)});
                break;
            case TitleMenuOption::Count:
                break;
            }
        }

        return events;
    }

    // Phase 319: host calls this once the outro animation is
    // committed (e.g. fade-out has begun). Mirrors the retail
    // runtime's progression context_phase=1 -> 2 captured in the
    // last 5 title-menu-context samples of the phase315_take2 trace.
    inline void advanceTitleMenuToTransitioningOut(TitleMenuState& s) noexcept
    {
        if (s.phase == TitleMenuPhase::TransitionRequested)
            s.phase = TitleMenuPhase::TransitioningOut;
    }

    // Phase 319: host calls this when the outgoing transition has
    // fully resolved (next screen has taken over). Resets back to
    // Idle so the menu is ready for re-entry.
    inline void resetTitleMenuPhaseAfterTransition(TitleMenuState& s) noexcept
    {
        s.phase = TitleMenuPhase::Idle;
        s.transitionTargetSet = false;
        s.optionsSubMenuOpen = false;
        s.deleteSavePromptOpen = false;
        s.dlcInstallPromptOpen = false;
    }

    // Convenience: hide Continue (corrupt save); hide DLC (already
    // installed); hide Exit (host doesn't want a clean-quit path
    // -- e.g. console build). Mirrors the retail patch's
    // TitleMenuRemoveContinueOnCorruptSaveMidAsmHook +
    // TitleMenuRemoveStorageDeviceOptionMidAsmHook +
    // TitleMenuAddInstallOptionMidAsmHook conditional logic without
    // pulling those midasm hooks into the standalone port.
    inline void applyTitleMenuVisibility(
        TitleMenuState& state,
        bool saveDataPresent,
        bool saveDataCorrupt,
        bool dlcMissing,
        bool exitOptionAvailable) noexcept
    {
        state.optionVisible[static_cast<std::size_t>(TitleMenuOption::NewGame)] = true;
        state.optionVisible[static_cast<std::size_t>(TitleMenuOption::Continue)] =
            saveDataPresent && !saveDataCorrupt;
        state.optionVisible[static_cast<std::size_t>(TitleMenuOption::Options)] = true;
        state.optionVisible[static_cast<std::size_t>(TitleMenuOption::InstallDLC)] = dlcMissing;
        state.optionVisible[static_cast<std::size_t>(TitleMenuOption::Exit)] = exitOptionAvailable;

        // If the cursor lands on a now-hidden option, snap forward to
        // the next visible one.
        if (state.cursorIndex >= 0
            && state.cursorIndex < static_cast<std::int32_t>(TitleMenuOption::Count)
            && !state.optionVisible[static_cast<std::size_t>(state.cursorIndex)])
        {
            state.cursorIndex = detail::title_menu::advanceCursor(state, state.cursorIndex, +1);
        }
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
