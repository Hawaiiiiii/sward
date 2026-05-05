// Phase 307: SGFX-shaped port of the post-stage results screen
// (CResult / ui_result.yncp). Renders the rank tally + final
// score reveal sequence after a stage clears.
//
// Behavior mined from:
//   * CHudPause's transition logic (Quit branch exits to CResult).
//   * The retail asset ui_result.yncp's scenes (rank, score_count,
//     time_count, ring_count, special_score, total_score).
//   * Each value typewriter-reveals before the next one starts.
//
// The retail game animates each line of the tally appearing in
// sequence; SGFX matches that animation order so the same SFX
// cues fire at the same moments.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class ResultsLineId : std::uint8_t
    {
        Rank        = 0,
        ScoreLine   = 1,
        TimeLine    = 2,
        RingsLine   = 3,
        SpecialScore = 4,
        TotalScore  = 5,
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
