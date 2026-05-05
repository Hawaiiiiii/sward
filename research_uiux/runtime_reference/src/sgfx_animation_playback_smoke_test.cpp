// Phase 313 smoke test: drive the SGFX animation playback sampler
// against scripted keyframe lists for HideFlag (step), Linear
// position interp, and a multi-track combination matching the
// retail info_img_1 Switch_Anim shape (2 keyframes per track on
// XScale + YScale).

#include "sward/ui_runtime/sgfx_animation_playback.hpp"

#include <iostream>
#include <string>
#include <type_traits>

namespace ui = sward::ui_runtime::generated::sgfx_hud;

static int g_failures = 0;
static void expect(bool ok, std::string_view label)
{
    std::cout << (ok ? "OK   " : "FAIL ") << label << "\n";
    if (!ok) ++g_failures;
}
template <typename T>
static void expectNear(T a, T e, T eps, std::string_view label)
{
    const bool ok = (a - e) >= -eps && (a - e) <= eps;
    std::cout << (ok ? "OK   " : "FAIL ") << label
              << " expected~=" << e << " actual=" << a << "\n";
    if (!ok) ++g_failures;
}

int main()
{
    using namespace ui;

    // Test 1: HideFlag step (Const interp). Keyframes (0, 0), (60, 1).
    // At frame 30 the value should still be 0; at frame 60 it's 1.
    {
        std::vector<CsdKeyframe> ks = {
            {"Intro", 0, 0, CsdAnimTrack::HideFlag, 0.0f,  0.0f, CsdInterp::Const},
            {"Intro", 0, 0, CsdAnimTrack::HideFlag, 60.0f, 1.0f, CsdInterp::Const},
        };
        const auto d0  = resolveCastDelta(ks, 0.0f);
        const auto d30 = resolveCastDelta(ks, 30.0f);
        const auto d60 = resolveCastDelta(ks, 60.0f);
        expect(d0.hasHideFlag,  "hide.f0 has hide flag");
        expect(d0.hideFlag == 0, "hide.f0 = 0");
        expect(d30.hideFlag == 0, "hide.f30 = 0 (Const before next key)");
        expect(d60.hideFlag == 1, "hide.f60 = 1");
    }

    // Test 2: Linear position interp. Keyframes (0, 0.0), (10, 1.0).
    // At frame 5 the value should be 0.5.
    {
        std::vector<CsdKeyframe> ks = {
            {"Intro", 0, 0, CsdAnimTrack::XPosition, 0.0f,  0.0f, CsdInterp::Linear},
            {"Intro", 0, 0, CsdAnimTrack::XPosition, 10.0f, 1.0f, CsdInterp::Linear},
        };
        const auto d5 = resolveCastDelta(ks, 5.0f);
        expect(d5.hasXPos, "linear.has xPos");
        expectNear(d5.xPos, 0.5f, 0.001f, "linear.f5 = 0.5");
    }

    // Test 3: Multi-track resolve. Mirrors info_img_1's Switch_Anim
    // for "ev_blliriance" (3 keyframes on XScale + YScale).
    {
        std::vector<CsdKeyframe> ks = {
            {"Switch_Anim", 0, 0, CsdAnimTrack::XScale, 0.0f, 0.6f,  CsdInterp::Linear},
            {"Switch_Anim", 0, 0, CsdAnimTrack::XScale, 4.0f, 0.6f,  CsdInterp::Linear},
            {"Switch_Anim", 0, 0, CsdAnimTrack::XScale, 9.0f, 0.37f, CsdInterp::Linear},
            {"Switch_Anim", 0, 0, CsdAnimTrack::YScale, 0.0f, 0.6f,  CsdInterp::Linear},
            {"Switch_Anim", 0, 0, CsdAnimTrack::YScale, 4.0f, 0.6f,  CsdInterp::Linear},
            {"Switch_Anim", 0, 0, CsdAnimTrack::YScale, 9.0f, 0.37f, CsdInterp::Linear},
        };
        // At frame 4 -> still 0.6 (linear from 0.6 to 0.6 is 0.6).
        const auto d4 = resolveCastDelta(ks, 4.0f);
        expectNear(d4.xScale, 0.6f, 0.001f, "switch_anim.f4 xScale = 0.6");
        expectNear(d4.yScale, 0.6f, 0.001f, "switch_anim.f4 yScale = 0.6");
        // At frame 6.5 -> halfway between 4 (0.6) and 9 (0.37) = 0.485
        const auto d65 = resolveCastDelta(ks, 6.5f);
        expectNear(d65.xScale, 0.485f, 0.005f, "switch_anim.f6.5 xScale = 0.485");
    }

    // Test 4: Playhead advancement. 60 fps, 60-frame anim, deltaSeconds 0.5
    // -> frame jumps to 30. Another 0.5s -> frame=60 + finished.
    {
        CsdAnimPlayhead p;
        p.animationName = "Intro_Anim";
        p.framerate = 60.0f;
        p.lastFrame = 60.0f;
        p.loop = false;
        advancePlayhead(p, 0.5f);
        expectNear(p.frame, 30.0f, 0.001f, "playhead.t=0.5s -> f=30");
        expect(!p.finished, "playhead.t=0.5s not finished");
        advancePlayhead(p, 0.5f);
        expectNear(p.frame, 60.0f, 0.001f, "playhead.t=1.0s -> f=60");
        expect(p.finished, "playhead.t=1.0s finished");
    }

    // Test 5: Looping playhead.
    {
        CsdAnimPlayhead p;
        p.animationName = "Idle";
        p.framerate = 60.0f;
        p.lastFrame = 60.0f;
        p.loop = true;
        advancePlayhead(p, 1.5f);
        // 1.5s * 60fps = 90 frames; loops at 60 -> 30
        expectNear(p.frame, 30.0f, 0.001f, "playhead.loop t=1.5s -> f=30");
        expect(!p.finished, "playhead.loop never finishes");
    }

    std::cout << "\nfailures: " << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
