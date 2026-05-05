// Phase 285: hand-written method bodies for `class CGeneralWindow`.
//
// Mirrors the Phase 273 / 277 pattern — derived predicates over the
// auto-generated layout that compile + run standalone without the SWA
// runtime linked. The SWA general window state machine drives the
// modal info / controls dialog; these helpers expose the
// "is the user interacting with it" / "which page is it on" decisions
// the renderer needs without reaching into raw `be<T>` storage.

#include "sward/ui_runtime/sgfx_hud_cgeneral_window.generated.h"

namespace sward::ui_runtime::generated::sgfx_hud
{
    // True iff the window is currently displaying any visible content.
    // The SWA state machine starts at `eWindowStatus_Closed` (idle, no
    // content) and ticks through `OpeningMessage`, `DisplayingMessage`,
    // `OpeningControls`, `DisplayingControls` as the open animation
    // and content-pages advance. Any non-Closed status means a panel
    // is on screen.
    bool isGeneralWindowVisible(const CGeneralWindow& window) noexcept
    {
        return window.getStatus() != EWindowStatus::eWindowStatus_Closed;
    }

    // True iff the window is currently showing the control-glyphs
    // page (Sonic Unleashed's "press A to confirm" hint panel) rather
    // than the message page. Used by the renderer to swap between the
    // two layouts.
    bool isGeneralWindowShowingControlsPage(const CGeneralWindow& window) noexcept
    {
        const auto status = window.getStatus();
        return status == EWindowStatus::eWindowStatus_OpeningControls
            || status == EWindowStatus::eWindowStatus_DisplayingControls;
    }

    // True iff the window is in the steady-state "fully open" phase —
    // open animation completed, waiting for the user to dismiss. Input
    // dispatch is gated to this state in the SWA runtime, so the port
    // exposes the same predicate.
    bool isGeneralWindowFullyDisplayed(const CGeneralWindow& window) noexcept
    {
        const auto status = window.getStatus();
        return status == EWindowStatus::eWindowStatus_DisplayingMessage
            || status == EWindowStatus::eWindowStatus_DisplayingControls;
    }

    // True iff the user has moved the cursor away from the default
    // (zero) row. Useful for animating the cursor highlight on first
    // input.
    bool isGeneralWindowCursorMoved(const CGeneralWindow& window) noexcept
    {
        return window.getCursorIndex() != 0u;
    }

    // Layout sanity: the SWA `CGeneralWindow` extends to `+0x168` per
    // the API header (last named member at `+0x164` is `m_SelectedIndex`,
    // a 4-byte `be<uint32_t>`). If a future regen breaks the layout the
    // build fails here and forces a coordinated update.
    static_assert(
        sizeof(CGeneralWindow) >= 0x168,
        "Phase 285 expects CGeneralWindow layout to extend through "
        "m_SelectedIndex at +0x164.");
}
