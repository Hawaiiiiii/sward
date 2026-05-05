// Phase 348: SGFX-shaped port of the Mission HUD trio.
//
// Phase 340 catalog recovery (xex RTTI scan):
//   D:\SonicWorldAdventure\SWA\source\HUD\Mission\HudMissionStage.cpp
//   D:\SonicWorldAdventure\SWA\source\HUD\Mission\HudMissionFinish.cpp
//   D:\SonicWorldAdventure\SWA\source\HUD\Mission\HudSelectMissionFailed.cpp
//
// Mission HUD is the in-stage objective tracker shown during
// optional mission stages (collect 100 rings, beat the time, defeat
// N enemies, etc.). Three classes drive three phases:
//
//   HudMissionStage         -- in-stage tracker (target / progress)
//   HudMissionFinish        -- success panel on completion
//   HudSelectMissionFailed  -- fail screen with retry/quit choices
//
// Inner state isn't retail-validated (no sub-state RTTI mined);
// SGFX models a 4-phase machine the host drives.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class MissionType : std::uint8_t
    {
        CollectRings    = 0,  // gather N rings
        BeatTime        = 1,  // clear within T seconds
        DefeatEnemies   = 2,  // KO N enemies
        ReachGoal       = 3,  // standard "clear stage" objective
        Survive         = 4,  // last N seconds
        BattleTrial     = 5,  // unused trial type from prototype
    };

    enum class MissionPhase : std::uint8_t
    {
        Inactive = 0,
        InStage  = 1,
        Finished = 2,  // success panel
        Failed   = 3,  // failed select panel
    };

    enum class MissionFailedChoice : std::uint8_t
    {
        Retry = 0,
        Quit  = 1,
    };

    enum class MissionEventKind : std::uint8_t
    {
        ProgressUpdated,
        Completed,
        Failed,
        FailedChoiceMoved,
        FailedChoiceConfirmed,
    };

    struct MissionInput
    {
        bool acceptTapped = false;
        bool cancelTapped = false;
        bool upTapped = false, downTapped = false;
        float deltaSeconds = 0.0f;
    };

    struct MissionState
    {
        MissionPhase phase = MissionPhase::Inactive;
        MissionType  type = MissionType::CollectRings;
        std::int32_t targetValue = 0;       // e.g. 100 for "collect 100 rings"
        std::int32_t currentValue = 0;
        float        timeLimitSeconds = 0.0f;
        float        elapsedSeconds = 0.0f;
        // Failed-panel cursor.
        MissionFailedChoice failedCursor = MissionFailedChoice::Retry;
    };

    struct MissionEvent
    {
        MissionEventKind kind = MissionEventKind::ProgressUpdated;
        std::int32_t     value = 0;
        std::string      sfxCueName;
    };

    constexpr std::string_view kMissionSfxConfirm = "sys_worldmap_decide";
    constexpr std::string_view kMissionSfxCursor  = "sys_worldmap_cursor";
    constexpr std::string_view kMissionSfxFail    = ""; // unverified

    inline std::vector<MissionEvent> startMission(
        MissionState& s, MissionType type,
        std::int32_t targetValue, float timeLimitSeconds = 0.0f) noexcept
    {
        s.phase = MissionPhase::InStage;
        s.type = type;
        s.targetValue = targetValue;
        s.currentValue = 0;
        s.timeLimitSeconds = timeLimitSeconds;
        s.elapsedSeconds = 0.0f;
        s.failedCursor = MissionFailedChoice::Retry;
        return {};
    }

    inline std::vector<MissionEvent> applyMissionProgress(
        MissionState& s, std::int32_t delta) noexcept
    {
        std::vector<MissionEvent> events;
        if (s.phase != MissionPhase::InStage) return events;
        s.currentValue += delta;
        if (s.currentValue < 0) s.currentValue = 0;
        events.push_back({MissionEventKind::ProgressUpdated,
                          s.currentValue, ""});
        if (s.currentValue >= s.targetValue && s.targetValue > 0)
        {
            s.phase = MissionPhase::Finished;
            events.push_back({MissionEventKind::Completed,
                              s.currentValue,
                              std::string(kMissionSfxConfirm)});
        }
        return events;
    }

    inline std::vector<MissionEvent> updateMissionOneFrame(
        MissionState& s, const MissionInput& input)
    {
        std::vector<MissionEvent> events;
        if (s.phase == MissionPhase::InStage)
        {
            s.elapsedSeconds += input.deltaSeconds;
            if (s.timeLimitSeconds > 0.0f
                && s.elapsedSeconds >= s.timeLimitSeconds
                && s.currentValue < s.targetValue)
            {
                s.phase = MissionPhase::Failed;
                s.failedCursor = MissionFailedChoice::Retry;
                events.push_back({MissionEventKind::Failed,
                                  s.currentValue,
                                  std::string(kMissionSfxFail)});
            }
        }
        else if (s.phase == MissionPhase::Failed)
        {
            if (input.upTapped || input.downTapped)
            {
                s.failedCursor = (s.failedCursor == MissionFailedChoice::Retry)
                    ? MissionFailedChoice::Quit
                    : MissionFailedChoice::Retry;
                events.push_back({MissionEventKind::FailedChoiceMoved,
                                  static_cast<std::int32_t>(s.failedCursor),
                                  std::string(kMissionSfxCursor)});
            }
            else if (input.acceptTapped)
            {
                events.push_back({MissionEventKind::FailedChoiceConfirmed,
                                  static_cast<std::int32_t>(s.failedCursor),
                                  std::string(kMissionSfxConfirm)});
                s.phase = MissionPhase::Inactive;
            }
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
