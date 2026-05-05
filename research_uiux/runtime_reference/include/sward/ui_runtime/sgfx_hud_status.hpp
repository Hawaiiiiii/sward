// Phase 341: SGFX-shaped port of SWA::CHudStatus.
//
// Source path mined from default_patched.xex (Phase 340):
//   D:\SonicWorldAdventure\SWA\source\HUD\Status\HudStatus.cpp
// RTTI symbol: .?AVCHudStatus@SWA@@
// CSD project:  ui_status.yncp (42 scenes, the most scene-heavy
//               UI project in the catalog)
//
// CHudStatus is the Status / Skill Upgrade overlay. It shows up when
// the player levels up a skill, opens the skill tree, or reviews
// purchased skill points. The screen ships 42 scenes covering:
//   * skill tree visualization
//   * skill name + description panel
//   * level-up animation
//   * "skill points spent" tally
//
// The retail class itself isn't exposed via api/ headers, so the
// inner state machine is inferred from the scene set. Real retail
// likely has 4-5 sub-states (overview, browse, confirm, level-up
// animation, close) but without RTTI for sub-states this header
// models a simpler 3-phase machine that hosts can drive directly.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class StatusPhase : std::uint8_t
    {
        Closed     = 0,
        Browsing   = 1, // user navigating the skill tree
        Confirming = 2, // confirm skill upgrade prompt
        LevelingUp = 3, // playing the level-up animation
    };

    enum class StatusEventKind : std::uint8_t
    {
        Opened,
        Closed,
        SkillHovered,
        SkillUpgradeConfirmed,
        SkillUpgradeApplied,
    };

    struct StatusInput
    {
        bool acceptTapped = false;
        bool cancelTapped = false;
        bool upTapped = false, downTapped = false;
        bool leftTapped = false, rightTapped = false;
        float deltaSeconds = 0.0f;
    };

    struct StatusState
    {
        StatusPhase phase = StatusPhase::Closed;
        std::int32_t hoveredSkillId = -1;
        std::int32_t selectedSkillId = -1;
        // Available skill points the player can spend.
        std::int32_t availableSkillPoints = 0;
        // Per-skill cost lookup is host-driven; SGFX only tracks
        // the currently-confirmed cost so it can deduct on apply.
        std::int32_t pendingCostPoints = 0;
        // Level-up animation timer.
        float        levelingUpSeconds = 0.0f;
        float        levelingUpDurationSeconds = 1.2f;
    };

    struct StatusEvent
    {
        StatusEventKind kind = StatusEventKind::Opened;
        std::int32_t    skillId = -1;
        std::string     sfxCueName;
    };

    constexpr std::string_view kStatusSfxOpen     = "sys_actstg_pausewinopen";
    constexpr std::string_view kStatusSfxClose    = "sys_actstg_pausewinclose";
    constexpr std::string_view kStatusSfxConfirm  = "sys_actstg_pausedecide";
    constexpr std::string_view kStatusSfxCursor   = "sys_actstg_pausecursor";

    inline std::vector<StatusEvent> openStatusOverlay(
        StatusState& s, std::int32_t availablePoints) noexcept
    {
        s.phase = StatusPhase::Browsing;
        s.availableSkillPoints = availablePoints;
        s.hoveredSkillId = -1;
        s.selectedSkillId = -1;
        s.pendingCostPoints = 0;
        return {{StatusEventKind::Opened, -1, std::string(kStatusSfxOpen)}};
    }

    inline std::vector<StatusEvent> hoverStatusSkill(
        StatusState& s, std::int32_t skillId) noexcept
    {
        std::vector<StatusEvent> events;
        if (s.phase != StatusPhase::Browsing) return events;
        if (skillId == s.hoveredSkillId) return events;
        s.hoveredSkillId = skillId;
        events.push_back({StatusEventKind::SkillHovered, skillId,
                          std::string(kStatusSfxCursor)});
        return events;
    }

    inline std::vector<StatusEvent> updateStatusOverlayOneFrame(
        StatusState& s, const StatusInput& input)
    {
        std::vector<StatusEvent> events;
        if (s.phase == StatusPhase::Closed) return events;

        if (s.phase == StatusPhase::LevelingUp)
        {
            s.levelingUpSeconds += input.deltaSeconds;
            if (s.levelingUpSeconds >= s.levelingUpDurationSeconds)
            {
                events.push_back({StatusEventKind::SkillUpgradeApplied,
                                  s.selectedSkillId, ""});
                s.phase = StatusPhase::Browsing;
                s.levelingUpSeconds = 0.0f;
                s.selectedSkillId = -1;
                s.pendingCostPoints = 0;
            }
            return events;
        }

        if (input.cancelTapped)
        {
            if (s.phase == StatusPhase::Confirming)
            {
                s.phase = StatusPhase::Browsing;
                s.selectedSkillId = -1;
                s.pendingCostPoints = 0;
            }
            else
            {
                s.phase = StatusPhase::Closed;
                events.push_back({StatusEventKind::Closed, -1,
                                  std::string(kStatusSfxClose)});
            }
            return events;
        }

        if (input.acceptTapped)
        {
            if (s.phase == StatusPhase::Browsing && s.hoveredSkillId >= 0)
            {
                s.phase = StatusPhase::Confirming;
                s.selectedSkillId = s.hoveredSkillId;
                events.push_back({StatusEventKind::SkillUpgradeConfirmed,
                                  s.selectedSkillId,
                                  std::string(kStatusSfxConfirm)});
            }
            else if (s.phase == StatusPhase::Confirming)
            {
                if (s.availableSkillPoints >= s.pendingCostPoints)
                {
                    s.availableSkillPoints -= s.pendingCostPoints;
                    s.phase = StatusPhase::LevelingUp;
                    s.levelingUpSeconds = 0.0f;
                }
            }
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
