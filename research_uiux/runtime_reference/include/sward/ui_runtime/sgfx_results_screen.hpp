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
    };

    enum class ResultsRank : std::uint8_t
    {
        D = 0, C = 1, B = 2, A = 3, S = 4,
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
        // Animation cursor: which line is currently revealing.
        ResultsLineId nextLine = ResultsLineId::Rank;
        float         secondsSinceLastReveal = 0.0f;
        float         secondsBetweenLines = 0.4f;
        bool          tallyComplete = false;
        bool          acknowledged = false;

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

    inline std::vector<ResultsEvent> updateResultsScreenOneFrame(
        ResultsState& state,
        const ResultsInput& input)
    {
        std::vector<ResultsEvent> events;

        if (!state.tallyComplete)
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
                }
                else
                {
                    state.nextLine = static_cast<ResultsLineId>(next);
                }
            }
        }
        else if (input.acceptTapped && !state.acknowledged)
        {
            state.acknowledged = true;
            events.push_back({ResultsEventKind::Acknowledged,
                              ResultsLineId::TotalScore,
                              std::string(kResultsSfxConfirm)});
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
