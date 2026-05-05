// Phase 333: SGFX-shaped port of SWA::CSaveIcon.
//
// Sourced directly from
//   local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/SaveIcon/SaveIcon.h
//
// CSaveIcon is the small spinning save-disk icon that appears when
// the game writes to the player profile (after world-map nav,
// stage clear, options-screen apply, etc). The retail class derives
// from CUpdateUnit; its only visible state is m_IsVisible @ +0xD8.
// Everything else (animation playhead, fade alpha, position) lives
// in the opaque CUpdateUnit base.
//
// The SGFX port keeps it minimal: a visibility flag plus a host-
// supplied "remaining seconds" timer so the icon can auto-hide
// after the save completes. The retail save-icon CSD scene
// (cts_save_icon in ui_saveicon.yncp) drives the spinning anim.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class SaveIconEventKind : std::uint8_t
    {
        Shown,    // m_IsVisible 0->1
        Hidden,   // m_IsVisible 1->0
    };

    struct SaveIconState
    {
        bool  isVisible = false;          // m_IsVisible @ +0xD8
        // SGFX-only timer driven by the host. Retail CSaveIcon
        // tracks visibility lifetime via the save-IO system; SGFX
        // hosts that don't model the IO system can use this timer
        // to auto-hide after a fixed duration.
        float visibleSecondsRemaining = 0.0f;
    };

    struct SaveIconEvent
    {
        SaveIconEventKind kind = SaveIconEventKind::Shown;
        std::string       sfxCueName;
    };

    // Real cue is unverified; the save-disk spin is silent in the
    // retail captured trace.
    constexpr std::string_view kSaveIconSfxShow = "";
    constexpr std::string_view kSaveIconSfxHide = "";

    inline std::vector<SaveIconEvent> showSaveIcon(
        SaveIconState& s,
        float visibleSeconds = 1.0f) noexcept
    {
        if (s.isVisible) { s.visibleSecondsRemaining = visibleSeconds; return {}; }
        s.isVisible = true;
        s.visibleSecondsRemaining = visibleSeconds;
        return {{SaveIconEventKind::Shown, std::string(kSaveIconSfxShow)}};
    }

    inline std::vector<SaveIconEvent> updateSaveIconOneFrame(
        SaveIconState& s,
        float deltaSeconds) noexcept
    {
        std::vector<SaveIconEvent> events;
        if (!s.isVisible) return events;
        s.visibleSecondsRemaining -= deltaSeconds;
        if (s.visibleSecondsRemaining <= 0.0f)
        {
            s.isVisible = false;
            s.visibleSecondsRemaining = 0.0f;
            events.push_back({SaveIconEventKind::Hidden,
                              std::string(kSaveIconSfxHide)});
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
