// Phase 308: SGFX-shaped port of the World Map state machine
// (CTitleStateWorldMap). Stage selection grid + cursor +
// stage-launch transitions.
//
// Sourced from:
//   * UnleashedRecomp/patches/aspect_ratio_patches.cpp:285-344
//     for the runtime layout overrides on info_bg_1 / info_img_*
//     (already encoded as runtime_overrides in the renderer).
//   * The retail asset ui_worldmap.yncp's scenes (worldmap_header_img,
//     info_img_1..4, cts_stage_select, cts_choices_*) extracted via
//     the binary parser in Phase 297.
//   * The 8 continents Sonic Unleashed ships (Apotos, Spagonia,
//     Mazuri, Holoska, Chun-nan, Empire City, Adabat, Eggmanland)
//     plus the boss-stage entry "Egg Dragoon" / "Final Dark Gaia".

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class WorldMapContinent : std::uint8_t
    {
        Apotos      = 0,
        Spagonia    = 1,
        Mazuri      = 2,
        Holoska     = 3,
        ChunNan     = 4,
        EmpireCity  = 5,
        Adabat      = 6,
        Eggmanland  = 7,
        Count       = 8,
    };

    enum class WorldMapEventKind : std::uint8_t
    {
        ContinentMoved,
        StageOpened,         // confirm pressed on highlighted stage
        ContinentInfoOpened, // hovering shows the dialogue panel
        BackedToTitle,       // cancel from the World Map root
    };

    struct WorldMapInput
    {
        bool acceptTapped = false;
        bool cancelTapped = false;
        bool leftTapped = false;
        bool rightTapped = false;
    };

    struct WorldMapState
    {
        WorldMapContinent cursor = WorldMapContinent::Apotos;
        std::array<bool, static_cast<std::size_t>(WorldMapContinent::Count)>
            unlocked{ true, false, false, false, false, false, false, false };
        bool stageOpenPanelVisible = false;
    };

    struct WorldMapEvent
    {
        WorldMapEventKind kind = WorldMapEventKind::ContinentMoved;
        WorldMapContinent continent = WorldMapContinent::Apotos;
        std::string sfxCueName;
    };

    // Phase 311 fix-up: all four cues here are real (mined from the
    // UnleashedRecomp source). sys_worldmap_finaldecide is the cue
    // played when the player commits to launching a stage from the
    // World Map (mined alongside the others).
    constexpr std::string_view kWorldMapSfxCursor       = "sys_worldmap_cursor";
    constexpr std::string_view kWorldMapSfxConfirm      = "sys_worldmap_decide";
    constexpr std::string_view kWorldMapSfxFinalConfirm = "sys_worldmap_finaldecide";
    constexpr std::string_view kWorldMapSfxCancel       = "sys_worldmap_cansel";
    constexpr std::string_view kWorldMapSfxOpen         = "sys_worldmap_window";

    namespace detail::world_map
    {
        inline WorldMapContinent advance(WorldMapState& s, std::int32_t step) noexcept
        {
            const auto count = static_cast<std::int32_t>(WorldMapContinent::Count);
            std::int32_t idx = static_cast<std::int32_t>(s.cursor);
            for (std::int32_t i = 0; i < count; ++i)
            {
                idx = (idx + step + count) % count;
                if (s.unlocked[static_cast<std::size_t>(idx)])
                    return static_cast<WorldMapContinent>(idx);
            }
            return s.cursor;
        }
    }

    inline std::vector<WorldMapEvent> updateWorldMapOneFrame(
        WorldMapState& state,
        const WorldMapInput& input)
    {
        std::vector<WorldMapEvent> events;
        if (state.stageOpenPanelVisible)
        {
            if (input.cancelTapped)
            {
                state.stageOpenPanelVisible = false;
                events.push_back({WorldMapEventKind::ContinentMoved,
                                  state.cursor,
                                  std::string(kWorldMapSfxCancel)});
            }
            return events;
        }
        if (input.leftTapped || input.rightTapped)
        {
            const auto next = detail::world_map::advance(state, input.leftTapped ? -1 : +1);
            if (next != state.cursor)
            {
                state.cursor = next;
                events.push_back({WorldMapEventKind::ContinentMoved,
                                  state.cursor,
                                  std::string(kWorldMapSfxCursor)});
            }
        }
        if (input.acceptTapped)
        {
            // Phase 317 retail correction: captured runtime trace
            // (phase315_take2, frames 7192..7764) shows
            // sys_worldmap_finaldecide fires when the player commits
            // to a stage on the World Map -- NOT plain _decide.
            state.stageOpenPanelVisible = true;
            events.push_back({WorldMapEventKind::StageOpened,
                              state.cursor,
                              std::string(kWorldMapSfxFinalConfirm)});
        }
        else if (input.cancelTapped)
        {
            events.push_back({WorldMapEventKind::BackedToTitle,
                              state.cursor,
                              std::string(kWorldMapSfxCancel)});
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
