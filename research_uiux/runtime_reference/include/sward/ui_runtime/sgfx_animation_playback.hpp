// Phase 313: SGFX animation playback for CSD timeline keyframes.
//
// The retail Sonic Unleashed CSD format carries one animation
// dictionary per scene (e.g. "Intro_Anim", "Switch_Anim",
// "Switch_Anim_rev") and per-cast keyframe lists across these
// track types: XPosition, YPosition, XScale, YScale, Rotation,
// HideFlag, SubImage. The runtime samples those tracks at the
// current frame to produce a per-cast transform delta layered
// on top of the asset's base values.
//
// Sourced from:
//   * inspect_xncp_yncp.parse_keyframe (in_tangent/out_tangent
//     stored, but the retail asset uses Const + Linear interp
//     types per the value_raw_bits / type encoding).
//   * build_yncp_native_component_map.extract_animation_track_keyframes
//     which is exactly the data shape we feed in here.
//
// SGFX exposes a per-frame sampler: feed it a list of keyframes
// + the current frame, get back the value to layer onto the
// cast's base transform / hide / subimage. Pure functions,
// header-only.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class CsdAnimTrack : std::uint8_t
    {
        Unknown   = 0,
        XPosition = 1,
        YPosition = 2,
        XScale    = 3,
        YScale    = 4,
        Rotation  = 5,
        HideFlag  = 6,
        SubImage  = 7,
        // The retail format also exposes Color and gradient corner
        // tracks; not needed for the screens the user listed.
    };

    enum class CsdInterp : std::uint8_t
    {
        Const  = 0, // step
        Linear = 1,
        // Hermite uses in/out tangents; the retail UI assets we
        // have don't use it for any HUD-relevant track.
    };

    struct CsdKeyframe
    {
        std::string  animationName;  // "Intro_Anim", "Switch_Anim", ...
        std::int32_t groupIndex = 0;
        std::int32_t castIndex = 0;
        CsdAnimTrack trackType = CsdAnimTrack::Unknown;
        float        frame = 0.0f;
        float        value = 0.0f;
        CsdInterp    interp = CsdInterp::Const;
    };

    // Per-cast resolved transform delta, layered on top of the
    // asset's base transform + scene_left/top values.
    struct CsdAnimDelta
    {
        bool  hasXPos = false;        float xPos = 0.0f;
        bool  hasYPos = false;        float yPos = 0.0f;
        bool  hasXScale = false;      float xScale = 1.0f;
        bool  hasYScale = false;      float yScale = 1.0f;
        bool  hasRotation = false;    float rotation = 0.0f;
        bool  hasHideFlag = false;    std::int32_t hideFlag = 0;
        bool  hasSubImage = false;    std::int32_t subImage = 0;
    };

    inline CsdAnimTrack parseTrackName(std::string_view name) noexcept
    {
        if (name == "XPosition") return CsdAnimTrack::XPosition;
        if (name == "YPosition") return CsdAnimTrack::YPosition;
        if (name == "XScale")    return CsdAnimTrack::XScale;
        if (name == "YScale")    return CsdAnimTrack::YScale;
        if (name == "Rotation")  return CsdAnimTrack::Rotation;
        if (name == "HideFlag")  return CsdAnimTrack::HideFlag;
        if (name == "SubImage")  return CsdAnimTrack::SubImage;
        return CsdAnimTrack::Unknown;
    }

    inline CsdInterp parseInterpName(std::string_view name) noexcept
    {
        if (name == "Linear") return CsdInterp::Linear;
        return CsdInterp::Const;
    }

    // Sample a single track for one cast at `frame`. `keys` is
    // the (already cast-filtered + animation-filtered + track-
    // filtered) keyframe list, sorted by frame. Returns the
    // value at `frame`; for Const interp this is the most-recent
    // keyframe's value, for Linear it interpolates.
    inline float sampleSingleTrack(
        const std::vector<CsdKeyframe>& keys,
        float frame) noexcept
    {
        if (keys.empty()) return 0.0f;
        if (frame <= keys.front().frame) return keys.front().value;
        if (frame >= keys.back().frame)  return keys.back().value;
        // Find the bracket [a, b].
        for (std::size_t i = 1; i < keys.size(); ++i)
        {
            const auto& a = keys[i - 1];
            const auto& b = keys[i];
            if (frame < b.frame)
            {
                if (a.interp == CsdInterp::Const)
                    return a.value;
                if (b.frame <= a.frame) return a.value;
                const float t = (frame - a.frame) / (b.frame - a.frame);
                return a.value + (b.value - a.value) * t;
            }
        }
        return keys.back().value;
    }

    // Resolve a per-cast delta from a flat keyframe list. Caller
    // pre-filters to the desired (animation_name, group_index,
    // cast_index). The returned delta has only the tracks that
    // actually carry keyframes for this cast in this animation;
    // unset tracks should be ignored at the layering step (use
    // the cast's base values).
    inline CsdAnimDelta resolveCastDelta(
        const std::vector<CsdKeyframe>& castKeys,
        float frame)
    {
        CsdAnimDelta d;
        // Bucket per track. Stable order preserved by trackKeys[t].
        std::vector<CsdKeyframe> trackKeys[8];
        for (const auto& k : castKeys)
        {
            const auto t = static_cast<std::size_t>(k.trackType);
            if (t < 8) trackKeys[t].push_back(k);
        }
        for (std::size_t t = 0; t < 8; ++t)
        {
            auto& v = trackKeys[t];
            if (v.empty()) continue;
            std::sort(v.begin(), v.end(),
                [](const CsdKeyframe& a, const CsdKeyframe& b)
                { return a.frame < b.frame; });
            const float val = sampleSingleTrack(v, frame);
            switch (static_cast<CsdAnimTrack>(t))
            {
            case CsdAnimTrack::XPosition: d.hasXPos = true;     d.xPos = val; break;
            case CsdAnimTrack::YPosition: d.hasYPos = true;     d.yPos = val; break;
            case CsdAnimTrack::XScale:    d.hasXScale = true;   d.xScale = val; break;
            case CsdAnimTrack::YScale:    d.hasYScale = true;   d.yScale = val; break;
            case CsdAnimTrack::Rotation:  d.hasRotation = true; d.rotation = val; break;
            case CsdAnimTrack::HideFlag:  d.hasHideFlag = true; d.hideFlag = static_cast<std::int32_t>(val + 0.5f); break;
            case CsdAnimTrack::SubImage:  d.hasSubImage = true; d.subImage = static_cast<std::int32_t>(val + 0.5f); break;
            default: break;
            }
        }
        return d;
    }

    // Tracks the active animation + frame cursor. Host advances
    // it each frame with deltaSeconds. When the animation reaches
    // its end frame, behavior depends on `loop`: loop=true wraps,
    // loop=false clamps at the last frame.
    struct CsdAnimPlayhead
    {
        std::string animationName;
        float       frame = 0.0f;            // current sample position
        float       framerate = 60.0f;
        float       lastFrame = 0.0f;        // animation_frame_count
        bool        loop = false;
        bool        finished = false;
    };

    inline void advancePlayhead(CsdAnimPlayhead& p, float deltaSeconds) noexcept
    {
        if (p.framerate <= 0.0f) return;
        if (p.finished) return;
        p.frame += deltaSeconds * p.framerate;
        if (p.frame >= p.lastFrame)
        {
            if (p.loop)
                p.frame = (p.lastFrame > 0.0f) ? std::fmod(p.frame, p.lastFrame) : 0.0f;
            else
            {
                p.frame = p.lastFrame;
                p.finished = true;
            }
        }
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
