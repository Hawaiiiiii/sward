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
    // Phase 325 retail-fidelity correction: SWA::CWorldMapCursor (mined
    // from local_build_env/.../api/SWA/System/GameMode/WorldMap/
    // WorldMapCursor.h) does NOT use a discrete continent enum --
    // it's a free-moving analog cursor on the 3D globe with
    // continuous (m_CursorX, m_CursorY) float positions, driven by
    // (m_LeftStickHorizontal, m_LeftStickVertical) analog input.
    // Continents become highlighted when the cursor is over them;
    // SGFX picks them via WorldMapHover (host-supplied via raycast
    // against the globe's continent regions).
    //
    // Continent identifiers preserved as a HOVER target / unlock
    // state, not as the cursor itself.
    enum class WorldMapContinent : std::uint8_t
    {
        None        = 0xFF,
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

    // SWA::CWorldMapCursor field mirror.
    struct WorldMapCursor
    {
        float leftStickVertical = 0.0f;   // m_LeftStickVertical @ +0x34
        float leftStickHorizontal = 0.0f; // m_LeftStickHorizontal @ +0x38
        bool  isCursorMoving = false;     // m_IsCursorMoving @ +0x3C
        float cursorY = 0.0f;             // m_CursorY @ +0x44
        float cursorX = 0.0f;             // m_CursorX @ +0x48
    };

    // Phase 327: SWA::CWorldMapCamera field mirror. Sourced from
    //   local_build_env/.../api/SWA/System/GameMode/WorldMap/
    //   WorldMapCamera.h
    // The camera orbits the 3D globe; pitch/yaw drive the spherical
    // angles, distance is the orbit radius, rotationSpeed scales how
    // fast the cursor input rotates the globe, and canMove is set to
    // false during transitions in/out of stage launches.
    // tiltToEarthTransitionSpeed governs the camera lerp when the
    // player picks a continent (the camera tilts down to face that
    // region of the globe).
    struct WorldMapCamera
    {
        float pitch = 0.0f;                       // m_Pitch @ +0xD0
        float yaw = 0.0f;                         // m_Yaw @ +0xD4
        float distance = 0.0f;                    // m_Distance @ +0xD8
        float rotationSpeed = 0.0f;               // m_RotationSpeed @ +0xDC
        bool  canMove = true;                     // m_CanMove @ +0xE8
        float tiltToEarthTransitionSpeed = 0.0f;  // m_TiltToEarthTransitionSpeed @ +0x120
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
        // Phase 325: retail-shape free analog cursor. Apotos starts
        // unlocked; rest gated by host save state.
        WorldMapCursor cursor; // mirrors SWA::CWorldMapCursor

        // Phase 327: retail orbit camera around the 3D globe.
        WorldMapCamera camera; // mirrors SWA::CWorldMapCamera

        // Which continent the cursor is currently HOVERING (host
        // computes via raycast against the globe regions). Replaces
        // the prior discrete cursor enum.
        WorldMapContinent hover = WorldMapContinent::Apotos;

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
        // D-pad fallback: walks discrete unlocked continents in
        // hover order. Real retail uses the analog cursor; this
        // helper is for hosts that bind D-pad input to the world
        // map screen.
        inline WorldMapContinent advanceHover(const WorldMapState& s,
                                              WorldMapContinent from,
                                              std::int32_t step) noexcept
        {
            const auto count = static_cast<std::int32_t>(WorldMapContinent::Count);
            std::int32_t idx = (from == WorldMapContinent::None)
                ? 0 : static_cast<std::int32_t>(from);
            for (std::int32_t i = 0; i < count; ++i)
            {
                idx = (idx + step + count) % count;
                if (s.unlocked[static_cast<std::size_t>(idx)])
                    return static_cast<WorldMapContinent>(idx);
            }
            return from;
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
                                  state.hover,
                                  std::string(kWorldMapSfxCancel)});
            }
            return events;
        }
        if (input.leftTapped || input.rightTapped)
        {
            const auto next = detail::world_map::advanceHover(
                state, state.hover, input.leftTapped ? -1 : +1);
            if (next != state.hover)
            {
                state.hover = next;
                events.push_back({WorldMapEventKind::ContinentMoved,
                                  state.hover,
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
                              state.hover,
                              std::string(kWorldMapSfxFinalConfirm)});
        }
        else if (input.cancelTapped)
        {
            events.push_back({WorldMapEventKind::BackedToTitle,
                              state.hover,
                              std::string(kWorldMapSfxCancel)});
        }
        return events;
    }

    // Phase 325 retail-fidelity: feed analog stick input directly
    // into the cursor mirror. Host calls this every frame from the
    // gamepad poll. Mirrors what the runtime does in CTitleStateWorldMap::Update
    // when reading m_pWorldMapCursor->m_LeftStickHorizontal/Vertical.
    inline void applyAnalogToWorldMapCursor(
        WorldMapState& state,
        float leftStickX, float leftStickY,
        float deltaSeconds, float cursorSpeed = 1.0f) noexcept
    {
        state.cursor.leftStickHorizontal = leftStickX;
        state.cursor.leftStickVertical = leftStickY;
        state.cursor.cursorX += leftStickX * deltaSeconds * cursorSpeed;
        state.cursor.cursorY += leftStickY * deltaSeconds * cursorSpeed;
        const float magSq = leftStickX*leftStickX + leftStickY*leftStickY;
        state.cursor.isCursorMoving = (magSq > 0.001f);
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
