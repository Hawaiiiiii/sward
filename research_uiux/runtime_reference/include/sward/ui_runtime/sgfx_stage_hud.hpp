// Phase 306: SGFX-shaped port of the gameplay HUD state.
//
// Mirrors the data shape we carved out of CHudSonicStage across
// prior phases (sgfx_hud_chud_sonic_stage.generated.h, Phase 277-280
// method ports). The fields here track exactly what the retail HUD
// reads each frame to drive the visible scenes:
//   - speed_gauge fill (0..1)              -> ui_playscreen/so_speed_gauge
//   - ringenergy_gauge fill (0..1)         -> ui_playscreen/so_ringenagy_gauge
//   - ring count integer                   -> ui_playscreen/ring_get
//   - score integer                        -> ui_playscreen/score_count
//   - time seconds float                   -> ui_playscreen/time_count
//   - life count integer                   -> ui_playscreen/u_info
//   - pause requested bool                 -> opens CHudPause overlay
//
// Day Sonic (normal) and Werehog (Evil Sonic) share the same HUD
// shape; the asset variants (ui_playscreen.yncp vs ui_playscreen_ev.yncp)
// supply the texture differences. SGFX picks one via the host.

#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class StageMode : std::uint8_t
    {
        DaySonic    = 0, // ui_playscreen.yncp
        Werehog     = 1, // ui_playscreen_ev.yncp ("Evil Sonic")
        Boss        = 2, // ui_playscreen_su.yncp
        BossHit     = 3, // ui_playscreen_ev_hit.yncp
    };

    enum class StageHudEventKind : std::uint8_t
    {
        RingPickedUp,
        RingLost,
        BoostUsed,
        BoostRefilled,
        LifeLost,
        LifeGained,
        PauseRequested,
        TimeLimit,         // time hit a configured limit (host-supplied)
    };

    struct StageHudInput
    {
        bool startTapped = false; // pause request
        float deltaSeconds = 0.0f;
    };

    struct StageHudState
    {
        StageMode  mode = StageMode::DaySonic;
        // Gauge fills. 0..1; the asset's gauge cells render as a
        // percentage of width filled.
        float speedGaugeFill = 0.0f;
        float ringEnergyGaugeFill = 0.0f;
        // Counters.
        std::int32_t rings = 0;
        std::int64_t score = 0;
        float        timeSeconds = 0.0f;
        std::int32_t lives = 4;
        // Optional time limit (set to a positive value to enable).
        float timeLimitSeconds = 0.0f;
        bool  timeLimitHitFired = false;
        // Pause overlay state.
        bool paused = false;
    };

    struct StageHudEvent
    {
        StageHudEventKind kind = StageHudEventKind::RingPickedUp;
        std::int64_t value = 0;        // ring count delta, score delta, etc
        std::string  sfxCueName;
    };

    // Phase 311 fix-up: only cues actually grep-mined from
    // UnleashedRecomp's source are kept as constants. Ring / boost /
    // 1up / lifelost / timelimit cues exist in the retail .csb sound
    // banks but aren't referenced from UnleashedRecomp's host-side
    // C++ -- they trigger via the PPC-translated game code instead.
    // Empty cue strings here mark "host: wire to your audio system
    // when porting"; the state machine still emits the event, just
    // without a host-side string to map.
    constexpr std::string_view kStageSfxScore      = "sys_actstg_score";        // mined from source
    constexpr std::string_view kStageSfxPauseOpen  = "sys_actstg_pausewinopen"; // mined from source
    constexpr std::string_view kStageSfxRing       = "";                        // unverified
    constexpr std::string_view kStageSfxRingLost   = "";                        // unverified
    constexpr std::string_view kStageSfxBoost      = "";                        // unverified
    constexpr std::string_view kStageSfxLifeUp     = "";                        // unverified
    constexpr std::string_view kStageSfxLifeLost   = "";                        // unverified
    constexpr std::string_view kStageSfxTimeLimit  = "";                        // unverified

    // Host applies score/ring/life/gauge deltas via these helpers
    // so the state machine can emit the right SFX cue and surface
    // events. Pure functions, no globals.

    inline std::vector<StageHudEvent> applyRingDelta(StageHudState& s, std::int32_t delta)
    {
        std::vector<StageHudEvent> events;
        if (delta == 0) return events;
        s.rings = std::max(0, s.rings + delta);
        if (delta > 0)
            events.push_back({StageHudEventKind::RingPickedUp, delta, std::string(kStageSfxRing)});
        else
            events.push_back({StageHudEventKind::RingLost, -delta, std::string(kStageSfxRingLost)});
        return events;
    }

    inline std::vector<StageHudEvent> applyLifeDelta(StageHudState& s, std::int32_t delta)
    {
        std::vector<StageHudEvent> events;
        if (delta == 0) return events;
        s.lives = std::max(0, s.lives + delta);
        if (delta > 0)
            events.push_back({StageHudEventKind::LifeGained, delta, std::string(kStageSfxLifeUp)});
        else
            events.push_back({StageHudEventKind::LifeLost, -delta, std::string(kStageSfxLifeLost)});
        return events;
    }

    inline std::vector<StageHudEvent> applyScoreDelta(StageHudState& s, std::int64_t delta)
    {
        s.score = std::max<std::int64_t>(0, s.score + delta);
        return {};
    }

    inline std::vector<StageHudEvent> setSpeedGauge(StageHudState& s, float fill01)
    {
        std::vector<StageHudEvent> events;
        const float prev = s.speedGaugeFill;
        s.speedGaugeFill = std::clamp(fill01, 0.0f, 1.0f);
        if (prev <= 0.001f && s.speedGaugeFill > 0.001f)
            events.push_back({StageHudEventKind::BoostRefilled, 0, ""});
        if (prev > 0.001f && s.speedGaugeFill <= 0.001f)
            events.push_back({StageHudEventKind::BoostUsed, 0, std::string(kStageSfxBoost)});
        return events;
    }

    inline std::vector<StageHudEvent> updateStageHudOneFrame(
        StageHudState& s,
        const StageHudInput& input)
    {
        std::vector<StageHudEvent> events;
        s.timeSeconds += input.deltaSeconds;
        if (s.timeLimitSeconds > 0.0f
            && !s.timeLimitHitFired
            && s.timeSeconds >= s.timeLimitSeconds)
        {
            s.timeLimitHitFired = true;
            events.push_back({StageHudEventKind::TimeLimit, 0,
                              std::string(kStageSfxTimeLimit)});
        }
        if (input.startTapped && !s.paused)
        {
            s.paused = true;
            events.push_back({StageHudEventKind::PauseRequested, 0,
                              std::string(kStageSfxPauseOpen)});
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
