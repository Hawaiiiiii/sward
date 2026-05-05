// Phase 341: SGFX-shaped port of HelpWindow (HUD/HelpWindow/HelpWindow.cpp).
//
// Phase 340 finding: HelpWindow is a SEPARATE class from CGeneralWindow,
// even though both are modal overlays. Source path mined from xex:
//   D:\SonicWorldAdventure\SWA\source\HUD\HelpWindow\HelpWindow.cpp
// CSD project: ui_help.yncp (4 scenes, 3 of which are help_chara_*
// text containers that the native CSD renderer can't draw yet
// because text/font rendering isn't implemented; tracked in
// sgfx_renderer_coverage_audit.generated.json as the FONT-1 gap).
//
// SGFX models HelpWindow as a thin overlay distinct from
// CGeneralWindow's confirmation-dialog flow. HelpWindow shows a
// fixed help body keyed by topic id; user dismisses with B/cancel.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class HelpWindowPhase : std::uint8_t
    {
        Closed   = 0,
        Opening  = 1, // entry animation
        Visible  = 2, // user reading; cancel dismisses
        Closing  = 3, // exit animation
    };

    enum class HelpWindowEventKind : std::uint8_t
    {
        Opened,
        Closed,
        TopicChanged,
    };

    struct HelpWindowInput
    {
        bool cancelTapped = false;
        bool leftTapped = false;
        bool rightTapped = false;
        float deltaSeconds = 0.0f;
    };

    struct HelpWindowState
    {
        HelpWindowPhase phase = HelpWindowPhase::Closed;
        std::int32_t    currentTopicId = -1;
        std::int32_t    topicCount = 0;       // host-driven; 0 = no nav
        // Animation timers.
        float           openingSeconds = 0.0f;
        float           openingDurationSeconds = 0.2f;
        float           closingSeconds = 0.0f;
        float           closingDurationSeconds = 0.2f;
    };

    struct HelpWindowEvent
    {
        HelpWindowEventKind kind = HelpWindowEventKind::Opened;
        std::int32_t        topicId = -1;
        std::string         sfxCueName;
    };

    // HelpWindow shares the same SFX bank as CGeneralWindow per the
    // captured trace (open + cancel cues are window-bank generic).
    constexpr std::string_view kHelpWindowSfxOpen   = "sys_worldmap_window";
    constexpr std::string_view kHelpWindowSfxClose  = "sys_worldmap_cansel";
    constexpr std::string_view kHelpWindowSfxCursor = "sys_worldmap_cursor";

    inline std::vector<HelpWindowEvent> openHelpWindow(
        HelpWindowState& s, std::int32_t topicId, std::int32_t topicCount = 1) noexcept
    {
        s.phase = HelpWindowPhase::Opening;
        s.openingSeconds = 0.0f;
        s.currentTopicId = topicId;
        s.topicCount = topicCount;
        return {{HelpWindowEventKind::Opened, topicId,
                 std::string(kHelpWindowSfxOpen)}};
    }

    inline std::vector<HelpWindowEvent> updateHelpWindowOneFrame(
        HelpWindowState& s, const HelpWindowInput& input)
    {
        std::vector<HelpWindowEvent> events;
        if (s.phase == HelpWindowPhase::Closed) return events;

        if (s.phase == HelpWindowPhase::Opening)
        {
            s.openingSeconds += input.deltaSeconds;
            if (s.openingSeconds >= s.openingDurationSeconds)
                s.phase = HelpWindowPhase::Visible;
            return events;
        }
        if (s.phase == HelpWindowPhase::Closing)
        {
            s.closingSeconds += input.deltaSeconds;
            if (s.closingSeconds >= s.closingDurationSeconds)
            {
                s.phase = HelpWindowPhase::Closed;
                events.push_back({HelpWindowEventKind::Closed,
                                  s.currentTopicId, ""});
                s.currentTopicId = -1;
            }
            return events;
        }
        // Visible: cancel dismisses; left/right cycles topics.
        if (input.cancelTapped)
        {
            s.phase = HelpWindowPhase::Closing;
            s.closingSeconds = 0.0f;
            events.push_back({HelpWindowEventKind::Closed, s.currentTopicId,
                              std::string(kHelpWindowSfxClose)});
        }
        else if (s.topicCount > 1 && (input.leftTapped || input.rightTapped))
        {
            const std::int32_t step = input.leftTapped ? -1 : +1;
            std::int32_t next = s.currentTopicId + step;
            if (next < 0)              next = s.topicCount - 1;
            if (next >= s.topicCount)  next = 0;
            if (next != s.currentTopicId)
            {
                s.currentTopicId = next;
                events.push_back({HelpWindowEventKind::TopicChanged, next,
                                  std::string(kHelpWindowSfxCursor)});
            }
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
