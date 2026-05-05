// Phase 307 / 329: SGFX-shaped port of the post-stage results screen
// (CResult / ui_result.yncp). Renders the rank tally + final
// score reveal sequence after a stage clears.
//
// Behavior mined from:
//   * CHudPause's transition logic (Quit branch exits to CResult).
//   * The retail asset ui_result.yncp's scenes (rank, score_count,
//     time_count, ring_count, special_score, total_score).
//   * Each value typewriter-reveals before the next one starts.
//
// Phase 329 retail-fidelity additions:
//   * The retail asset has SIX numeric counters (result_num_1 ..
//     result_num_6) per aspect_ratio_patches.cpp:664-705. SGFX's
//     six ResultsLineId values map 1:1 onto those scene IDs.
//   * Two BGM cues exist for the results screen, mined from
//     install/hashes/game.cpp:8111-8114:
//        bgm_sys_result    -- success fanfare (rank C..S)
//        bgm_sys_result_ng -- failure fanfare (rank D / time-out)
//   * "ui_result_ex" exists as a separate retail screen for EX
//     stages (Tails Tornado / Werehog QTE clears) per
//     ui_lab_runtime_screen_index.generated.h:36. Same logic, just
//     a different CSD project bound; SGFX models it as a flag on
//     ResultsState.
//
// Note: HUD/Result/Result.cpp is the retail source file (per the
// runtime screen index), but it is NOT exposed as an api/ struct
// header in UnleashedRecomp. So inner field offsets / per-frame
// state machine are still inferred from the captured trace +
// scene shape, not retail-validated SWA_ASSERT_OFFSETOF lines.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Phase 329: the retail asset's six numeric counters
    //   result_num_1 .. result_num_6 (per aspect_ratio_patches.cpp)
    // map 1:1 onto these enum values. The semantic labels (Rank,
    // Score, Time, Rings, SpecialScore, TotalScore) are inferred
    // from the standard Sonic Unleashed results-screen layout that
    // the captured retail asset renders.
    enum class ResultsLineId : std::uint8_t
    {
        Rank        = 0, // result_num_1
        ScoreLine   = 1, // result_num_2
        TimeLine    = 2, // result_num_3
        RingsLine   = 3, // result_num_4
        SpecialScore = 4, // result_num_5
        TotalScore  = 5, // result_num_6
        Count       = 6,
    };

    enum class ResultsEventKind : std::uint8_t
    {
        LineRevealed,
        TallyComplete,
        Acknowledged,   // user presses A to advance
        // Phase 339: phase transitions mined from RTTI in
        // local_build_env/.../UnleashedRecompLib/private/default_patched.xex.
        // The retail CHudResult is a TStateMachine<CHudResult> with
        // FOUR explicit sub-states; SGFX previously collapsed all of
        // them into "tally cycle". These events fire at each retail
        // sub-state boundary so hosts can drive camera + audio cues
        // accurately.
        EnteredFirstWaiting,
        EnteredChangingCamera,
        EnteredResultAnimation,
        EnteredRank,
    };

    enum class ResultsRank : std::uint8_t
    {
        D = 0, C = 1, B = 2, A = 3, S = 4,
    };

    // Phase 339: retail CHudResult sub-states. RTTI mined from
    // default_patched.xex (uncompressed retail image):
    //   .?AVCStateFirstWaiting@CHudResult@SWA@@
    //   .?AVCStateChangingCamera@CHudResult@SWA@@
    //   .?AVCStateResultAnimation@CHudResult@SWA@@
    //   .?AVCStateRank@CHudResult@SWA@@
    // Source file path also recovered:
    //   D:\SonicWorldAdventure\SWA\source\HUD\Common\Result\HudResult.cpp
    enum class ResultsPhase : std::uint8_t
    {
        FirstWaiting     = 0, // CStateFirstWaiting -- idle while host fades in
        ChangingCamera   = 1, // CStateChangingCamera -- camera lerp into pose
        ResultAnimation  = 2, // CStateResultAnimation -- 6-line typewriter reveal
        Rank             = 3, // CStateRank -- rank fanfare + accept-to-exit
    };

    struct ResultsInput
    {
        bool acceptTapped = false;
        float deltaSeconds = 0.0f;
    };

    struct ResultsState
    {
        ResultsRank  rank = ResultsRank::D;
        std::int64_t score = 0;
        float        timeSeconds = 0.0f;
        std::int32_t rings = 0;
        std::int64_t specialScore = 0;
        std::int64_t totalScore = 0;

        // Phase 339: retail TStateMachine<CHudResult> sub-state. The
        // host drives transitions either via host helpers (camera
        // settle complete) or by exhausting timers within a phase.
        ResultsPhase phase = ResultsPhase::FirstWaiting;

        // Animation cursor: which line is currently revealing.
        ResultsLineId nextLine = ResultsLineId::Rank;
        float         secondsSinceLastReveal = 0.0f;
        float         secondsBetweenLines = 0.4f;
        bool          tallyComplete = false;
        bool          acknowledged = false;
        // Phase 339: time the host says it takes for the camera lerp
        // (CStateChangingCamera) to settle. SGFX advances out of that
        // phase when this elapses; retail real value is in the asset
        // animation tracks, not directly mined.
        float         cameraLerpSeconds = 0.5f;
        float         secondsInPhase = 0.0f;
        // Phase 339: minimum dwell on the rank fanfare phase before
        // accept moves to Acknowledged. Retail uses the rank-anim
        // length; SGFX defaults to 1s, host can override.
        float         rankPhaseMinSeconds = 1.0f;

        // Phase 329: EX-stage variant flag. Retail ships ui_result_ex
        // as a separate CSD project bound for Tails Tornado / Werehog
        // QTE stage clears; the inner state machine is identical
        // (six counters, same animation order) but the host should
        // load the EX project's textures instead. Tracked here so
        // the same updateResultsScreenOneFrame routine can drive
        // both variants without branching.
        bool isExVariant = false;
        // Phase 329: which BGM the host should kick off when this
        // screen opens. Set from rank: rank D -> failure fanfare,
        // ranks C..S -> success fanfare. Value populated by the
        // host before calling update; SGFX does not write to it.
        bool useFailureBgm = false;
    };

    struct ResultsEvent
    {
        ResultsEventKind kind = ResultsEventKind::LineRevealed;
        ResultsLineId    line = ResultsLineId::Rank;
        std::string      sfxCueName;
    };

    // Phase 311 fix-up: sys_result_line / sys_result_rank were
    // invented (not in UnleashedRecomp source nor in the existing
    // SFX_CUE_CANDIDATES list). Real cues for the results-tally
    // animation live in the .csb banks but aren't referenced from
    // the host-side patches we have access to. Confirm cue is real.
    constexpr std::string_view kResultsSfxLine    = "";                     // unverified
    constexpr std::string_view kResultsSfxRank    = "";                     // unverified
    constexpr std::string_view kResultsSfxConfirm = "sys_worldmap_decide";  // mined

    // Phase 329: BGM cues mined from
    //   local_build_env/.../UnleashedRecomp/install/hashes/game.cpp:8111-8114
    // (Sound/bgm_sys_result.cpk + bgm_sys_result_ng.cpk). These are
    // BGM stems, not SFX cues -- the host triggers them on results-
    // screen entry, not via the SGFX event stream.
    constexpr std::string_view kResultsBgmSuccess = "bgm_sys_result";
    constexpr std::string_view kResultsBgmFailure = "bgm_sys_result_ng";

    // Phase 339: helper for hosts that want to skip the camera lerp
    // (e.g. UI tests that just want to drive the tally directly).
    inline void resultsScreenSkipToAnimation(ResultsState& s) noexcept
    {
        s.phase = ResultsPhase::ResultAnimation;
        s.secondsInPhase = 0.0f;
    }

    inline std::vector<ResultsEvent> updateResultsScreenOneFrame(
        ResultsState& state,
        const ResultsInput& input)
    {
        std::vector<ResultsEvent> events;
        state.secondsInPhase += input.deltaSeconds;

        // Phase 339: drive the four-state retail machine.
        // CStateFirstWaiting: trivial; advance after one tick so hosts
        //   can synchronize fade-in. The retail FirstWaiting is also
        //   trivial -- it waits for the camera-changing trigger.
        if (state.phase == ResultsPhase::FirstWaiting)
        {
            state.phase = ResultsPhase::ChangingCamera;
            state.secondsInPhase = 0.0f;
            events.push_back({ResultsEventKind::EnteredChangingCamera,
                              ResultsLineId::Rank, ""});
            return events;
        }
        // CStateChangingCamera: dwell `cameraLerpSeconds` then advance
        // to ResultAnimation.
        if (state.phase == ResultsPhase::ChangingCamera)
        {
            if (state.secondsInPhase >= state.cameraLerpSeconds)
            {
                state.phase = ResultsPhase::ResultAnimation;
                state.secondsInPhase = 0.0f;
                state.secondsSinceLastReveal = 0.0f;
                events.push_back({ResultsEventKind::EnteredResultAnimation,
                                  ResultsLineId::Rank, ""});
            }
            return events;
        }
        // CStateResultAnimation: typewriter reveal each of the 6 lines.
        if (state.phase == ResultsPhase::ResultAnimation && !state.tallyComplete)
        {
            state.secondsSinceLastReveal += input.deltaSeconds;
            if (state.secondsSinceLastReveal >= state.secondsBetweenLines)
            {
                state.secondsSinceLastReveal = 0.0f;
                const auto line = state.nextLine;
                events.push_back({ResultsEventKind::LineRevealed,
                                  line,
                                  std::string(line == ResultsLineId::Rank
                                              ? kResultsSfxRank
                                              : kResultsSfxLine)});
                const auto next = static_cast<std::uint8_t>(line) + 1;
                if (next >= static_cast<std::uint8_t>(ResultsLineId::Count))
                {
                    state.tallyComplete = true;
                    events.push_back({ResultsEventKind::TallyComplete,
                                      ResultsLineId::TotalScore, ""});
                    state.phase = ResultsPhase::Rank;
                    state.secondsInPhase = 0.0f;
                    events.push_back({ResultsEventKind::EnteredRank,
                                      ResultsLineId::TotalScore, ""});
                }
                else
                {
                    state.nextLine = static_cast<ResultsLineId>(next);
                }
            }
        }
        // Phase 339: CStateRank handles the rank fanfare. Accept can
        // only fire Acknowledged after rankPhaseMinSeconds dwell so
        // the host has time to play the rank-specific fanfare cue.
        else if (state.phase == ResultsPhase::Rank
                 && input.acceptTapped
                 && !state.acknowledged
                 && state.secondsInPhase >= state.rankPhaseMinSeconds)
        {
            state.acknowledged = true;
            events.push_back({ResultsEventKind::Acknowledged,
                              ResultsLineId::TotalScore,
                              std::string(kResultsSfxConfirm)});
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
