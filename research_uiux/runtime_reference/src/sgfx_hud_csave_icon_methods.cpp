// Phase 285: hand-written method bodies for `class CSaveIcon`.
//
// CSaveIcon is the smallest of the SWA HUD owners — a single byte flag
// at `+0xD8` indicating whether the autosave-in-progress icon is on
// screen. The Phase 274 generated layout already exposes a generated
// `isVisible()` accessor; the helpers below add the renderer-relevant
// derived predicates that the SWA runtime keys autosave-related sound
// effects and UI gating off of.

#include "sward/ui_runtime/sgfx_hud_csave_icon.generated.h"

namespace sward::ui_runtime::generated::sgfx_hud
{
    // True iff the save-icon overlay is currently visible. Mirrors the
    // existing `App::s_isSaving = pSaveIcon->m_IsVisible` assignment in
    // `CHudPause_patches.cpp` that drives the achievement-menu /
    // autosave gating in the recomp.
    bool isSaveInProgress(const CSaveIcon& saveIcon) noexcept
    {
        return saveIcon.isVisible();
    }

    // True iff the save-icon overlay is currently hidden — i.e. the
    // game is NOT autosaving and it is safe to show modal dialogs that
    // could otherwise pre-empt the save-in-progress visualization.
    bool isSafeToShowModalDialog(const CSaveIcon& saveIcon) noexcept
    {
        return !saveIcon.isVisible();
    }

    // Layout sanity: SWA `CSaveIcon` extends through `m_IsVisible` at
    // `+0xD8`. The recomp's `lwz r3, 216(r31)` in `sub_824E5170` reads
    // this byte directly. Build breaks here if the layout regresses.
    static_assert(
        sizeof(CSaveIcon) >= 0xD9,
        "Phase 285 expects CSaveIcon layout to extend through m_IsVisible "
        "at +0xD8.");
}
