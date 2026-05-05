// Phase 285: hand-written method bodies for `class CLoading`.
//
// CLoading owns Sonic Unleashed's loading-screen state machine: the
// Miles Electric panel, the night-to-day transition wipe, the "Now
// Loading" overlay, etc. These helpers expose the renderer-relevant
// decisions over the auto-generated layout + accessors.

#include "sward/ui_runtime/sgfx_hud_cloading.generated.h"

namespace sward::ui_runtime::generated::sgfx_hud
{
    // True iff the loading overlay is currently being displayed. The
    // SWA `m_IsVisible` field is a `be<uint32_t>` flag; non-zero means
    // any loading visualization is on screen (Miles Electric, plain
    // "Now Loading", arrows, blank, etc.).
    bool isLoadingScreenVisible(const CLoading& loading) noexcept
    {
        return loading.getIsVisible() != 0u;
    }

    // True iff the active loading display is the Miles Electric variant
    // (in-character laptop UI). The SWA loading owner uses several
    // `ELoadingDisplayType` values; the Miles Electric ones cover the
    // standard hub/stage transitions and the contextual hints.
    bool isLoadingScreenMilesElectric(const CLoading& loading) noexcept
    {
        const auto kind = loading.getLoadingDisplayType();
        return kind == ELoadingDisplayType::eLoadingDisplayType_MilesElectric
            || kind == ELoadingDisplayType::eLoadingDisplayType_MilesElectricContext;
    }

    // True iff the loading overlay is currently doing the night-to-day
    // (or day-to-night) wipe transition between Sonic / Werehog stage
    // segments. Distinct from the `m_IsNightToDay` direction flag,
    // which only carries meaning while the wipe is active.
    bool isLoadingScreenNightDayWipe(const CLoading& loading) noexcept
    {
        return loading.getLoadingDisplayType()
            == ELoadingDisplayType::eLoadingDisplayType_ChangeTimeOfDay;
    }

    // True iff a Werehog-themed loading movie is being played (the
    // dedicated loading FMV slot used between Sonic and Werehog
    // sections of the same act). Useful for routing audio mixing.
    bool isLoadingScreenWerehogMovie(const CLoading& loading) noexcept
    {
        return loading.getLoadingDisplayType()
            == ELoadingDisplayType::eLoadingDisplayType_WerehogMovie;
    }

    // True iff the loading overlay is suppressed (`eLoadingDisplayType_None`
    // or `eLoadingDisplayType_Blank`) — used to hide the panel during
    // certain cutscene-driven transitions where the loading UI would
    // otherwise overlap.
    bool isLoadingScreenDisplaySuppressed(const CLoading& loading) noexcept
    {
        const auto kind = loading.getLoadingDisplayType();
        return kind == ELoadingDisplayType::eLoadingDisplayType_None
            || kind == ELoadingDisplayType::eLoadingDisplayType_Blank;
    }

    // Layout sanity: SWA `CLoading` extends through `m_IsNightToDay` at
    // `+0x1A1`. Build breaks here if the layout regresses.
    static_assert(
        sizeof(CLoading) >= 0x1A2,
        "Phase 285 expects CLoading layout to extend through m_IsNightToDay "
        "at +0x1A1.");
}
