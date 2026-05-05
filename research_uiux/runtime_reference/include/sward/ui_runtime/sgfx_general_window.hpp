// Phase 324: SGFX-shaped port of SWA::CGeneralWindow.
//
// Sourced directly from
// local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/GeneralWindow/
// GeneralWindow.h -- the retail SWA C++ API header. CGeneralWindow is
// the modal-overlay class shared across the entire game UI for
// confirmation prompts: "Delete this save?", "Install DLC?", message
// boxes, controls help, etc. The earlier SGFX title-menu port modeled
// these as plain bools (deleteSavePromptOpen / dlcInstallPromptOpen)
// which lost the retail status-machine shape.
//
// Real retail layout:
//   m_rcGeneral   (CProject)   @ +0xD0
//   m_rcBg, m_rcWindow, m_rcWindow_2, m_rcWindowSelect, m_rcFooter
//   m_Status       (be<EWindowStatus>) @ +0x158
//   m_CursorIndex  (be<u32>)            @ +0x15C
//   m_SelectedIndex (be<u32>)           @ +0x164
//
// EWindowStatus has a real GAP at value 1 (Closed=0, OpeningMessage=2)
// -- runtime relies on these exact integer values. SGFX preserves them.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // SWA::EWindowStatus, retail enum values exactly as on disk.
    enum class WindowStatus : std::uint32_t
    {
        Closed              = 0,
        OpeningMessage      = 2, // gap on 1 in retail
        DisplayingMessage   = 3,
        OpeningControls     = 4,
        DisplayingControls  = 5,
    };

    enum class GeneralWindowEventKind : std::uint8_t
    {
        WindowOpened,
        WindowClosed,
        CursorMoved,
        Confirmed,    // user pressed accept on a row
        BackedOut,    // user pressed cancel
    };

    struct GeneralWindowState
    {
        WindowStatus status = WindowStatus::Closed;
        // m_CursorIndex: which row in the window is currently
        // highlighted. Two-row windows (Yes/No) the most common
        // shape; some windows have more rows.
        std::int32_t cursorIndex = 0;
        // m_SelectedIndex: which row was selected when the window
        // closed. Set when status transitions to Closed via Confirm.
        // Host reads this to decide what action the user picked.
        std::int32_t selectedIndex = -1;
        // Row count is set by the host when opening a specific
        // window (e.g. delete-save-confirm has 2 rows: Yes/No).
        std::int32_t rowCount = 0;
        // Whether this is a "controls help" window vs a "message"
        // window -- changes which OpeningX / DisplayingX statuses
        // the state machine cycles through.
        bool isControlsWindow = false;
    };

    struct GeneralWindowInput
    {
        bool acceptTapped = false;
        bool cancelTapped = false;
        bool upTapped = false;
        bool downTapped = false;
    };

    struct GeneralWindowEvent
    {
        GeneralWindowEventKind kind = GeneralWindowEventKind::CursorMoved;
        std::int32_t cursorAtFire = 0;
        std::string sfxCueName;
    };

    constexpr std::string_view kGeneralWindowSfxOpen    = "sys_worldmap_window";
    constexpr std::string_view kGeneralWindowSfxConfirm = "sys_worldmap_decide";
    constexpr std::string_view kGeneralWindowSfxCancel  = "sys_worldmap_cansel";

    inline std::vector<GeneralWindowEvent> openGeneralWindow(
        GeneralWindowState& s,
        std::int32_t rowCount,
        bool isControlsWindow)
    {
        s.rowCount = rowCount;
        s.cursorIndex = 0;
        s.selectedIndex = -1;
        s.isControlsWindow = isControlsWindow;
        s.status = isControlsWindow
            ? WindowStatus::OpeningControls
            : WindowStatus::OpeningMessage;
        return {{GeneralWindowEventKind::WindowOpened, 0,
                 std::string(kGeneralWindowSfxOpen)}};
    }

    // Host calls this once the open animation completes. Mirrors the
    // retail status transitions OpeningMessage -> DisplayingMessage
    // and OpeningControls -> DisplayingControls.
    inline void advanceGeneralWindowToDisplaying(GeneralWindowState& s) noexcept
    {
        if (s.status == WindowStatus::OpeningMessage)
            s.status = WindowStatus::DisplayingMessage;
        else if (s.status == WindowStatus::OpeningControls)
            s.status = WindowStatus::DisplayingControls;
    }

    inline std::vector<GeneralWindowEvent> updateGeneralWindowOneFrame(
        GeneralWindowState& s,
        const GeneralWindowInput& input)
    {
        std::vector<GeneralWindowEvent> events;
        if (s.status != WindowStatus::DisplayingMessage
            && s.status != WindowStatus::DisplayingControls)
            return events;

        if (input.upTapped || input.downTapped)
        {
            if (s.rowCount > 0)
            {
                const auto step = input.upTapped ? -1 : +1;
                std::int32_t next = s.cursorIndex + step;
                if (next < 0) next = s.rowCount - 1;
                if (next >= s.rowCount) next = 0;
                if (next != s.cursorIndex)
                {
                    s.cursorIndex = next;
                    // Captured trace shows GeneralWindow cursor
                    // in title-menu context is silent (Phase 317
                    // finding); but in pause-context it uses
                    // sys_actstg_pausecursor. Host knows which.
                    events.push_back({GeneralWindowEventKind::CursorMoved,
                                      s.cursorIndex, ""});
                }
            }
        }
        if (input.acceptTapped)
        {
            s.selectedIndex = s.cursorIndex;
            s.status = WindowStatus::Closed;
            events.push_back({GeneralWindowEventKind::Confirmed,
                              s.cursorIndex,
                              std::string(kGeneralWindowSfxConfirm)});
            events.push_back({GeneralWindowEventKind::WindowClosed,
                              s.cursorIndex, ""});
        }
        else if (input.cancelTapped)
        {
            s.status = WindowStatus::Closed;
            events.push_back({GeneralWindowEventKind::BackedOut,
                              s.cursorIndex,
                              std::string(kGeneralWindowSfxCancel)});
            events.push_back({GeneralWindowEventKind::WindowClosed,
                              s.cursorIndex, ""});
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
