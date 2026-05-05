// Phase 273 / 276: hand-written method bodies for `class CHudPause`.
//
// These are the first real method bodies in the human-readable 1:1 port
// of Sonic Unleashed's UI/UX. They are NOT generated — every line is a
// human-readable port of the SWA pause state-machine semantics, written
// against the auto-generated layout header in
// `sgfx_hud_chud_pause.generated.h` (Phase 268+) and using the inline
// accessors emitted in Phase 270.
//
// Each method is a derived predicate or a thin state-helper that:
// * Reads through the accessor (so the implementation never touches the
//   raw SWA `be<T>` storage type and stays compiler-portable).
// * Documents the SWA enum value(s) it is comparing against, with the
//   corresponding `EActionType` / `EMenuType` / `EStatusType` /
//   `ETransitionType` literal in scope.
// * Stays implementation-light: no I/O, no allocation, no calls into the
//   recomp runtime. The methods are pure functions of the visible class
//   state so they can be unit-tested standalone without linking the SWA
//   executable.
//
// Future phases will port the heavier methods (`Update`, `Render`,
// constructor / destructor) once the SWA RCObject runtime is also ported.

#include "sward/ui_runtime/sgfx_hud_chud_pause.generated.h"

namespace sward::ui_runtime::generated::sgfx_hud
{
    // True when the pause dialog currently has the "Quit"-flavored
    // transition armed. Mirrors the SWA pause state machine: when the
    // user picks `eActionType_Return` from the misc menu the transition
    // field is flipped to `eTransitionType_Quit` until the dialog is
    // either confirmed or aborted.
    bool isPauseQuitDialogArmed(const CHudPause& pause) noexcept
    {
        return pause.getTransition() == ETransitionType::eTransitionType_Quit;
    }

    // True when the pause overlay is fully on-screen and the user can
    // currently interact with the menu. The SWA pause owner sets
    // `m_IsVisible` once the bg fade-in completes and `m_IsShown` once
    // the menu items are populated; both are true together only between
    // open animation completion and close animation start, which is the
    // exact window where input dispatch is allowed.
    bool isPauseInteractive(const CHudPause& pause) noexcept
    {
        return pause.isVisible()
            && pause.isShown()
            && pause.getTransition() == ETransitionType::eTransitionType_Undefined;
    }

    // True when the pause overlay is rendering a sub-menu rather than the
    // top-level menu list. The SWA pause owner uses non-zero `m_Submenu`
    // values to index into the sub-menu render callbacks; zero means the
    // top-level pause list is showing.
    bool isPauseShowingSubmenu(const CHudPause& pause) noexcept
    {
        return pause.getSubmenu() != 0u;
    }

    // True when the pause owner is in a state where the SWA player input
    // dispatcher should treat menu accept/decline as committing the
    // currently-highlighted action. Matches the runtime gate that checks
    // for `eStatusType_Accept` (input was accepted) and `eMenuType_Misc`
    // (we are on the dialog/options sub-menu) together. Non-misc menus
    // commit through their own per-menu paths.
    bool isPauseMiscMenuActionAccepted(const CHudPause& pause) noexcept
    {
        return pause.getMenu() == EMenuType::eMenuType_Misc
            && pause.getStatus() == EStatusType::eStatusType_Accept;
    }

    // Compile-time sanity: the helpers above must compile against the
    // auto-generated layout header without modification. If a future
    // generator regen renames an accessor or removes an enum entry, the
    // build breaks here and forces a coordinated update.
    static_assert(
        sizeof(CHudPause) >= 0x1B9,
        "sgfx_hud_chud_pause_methods.cpp expects the Phase 268-generated "
        "CHudPause layout to be at least 0x1B9 bytes (m_IsShown lives at "
        "+0x1B8). If sizeof(CHudPause) is smaller, regenerate the layout "
        "header.");
}
