// Phase 311: input layer.
//
// Maps a host-polled SDL gamepad + keyboard snapshot to the
// SGFX orchestrator's SgfxFrameInput. Stays a pure function:
// host owns SDL_Init / SDL_PollEvent and fills the snapshot;
// the adapter only does the button-mapping + edge detection.
//
// Button map mirrors UnleashedRecomp's sdl_hid.cpp conventions
// (SDL_CONTROLLER_BUTTON_A maps to "accept", _B to "cancel",
// _START to "start", _BACK to "select"); keyboard fallback
// matches the patches' default scheme (Z=accept, X=cancel,
// Enter=start, Backspace=select, arrow keys = D-pad).

#pragma once

#include "sgfx_orchestrator.hpp"

#include <cstdint>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // What the host fills in each frame from SDL_GameControllerGetButton
    // and SDL_GetKeyboardState. The adapter then computes "tapped"
    // (this-frame edge) by comparing against the previous snapshot.
    struct SgfxRawInputSnapshot
    {
        bool padA = false;       // accept
        bool padB = false;       // cancel
        bool padX = false;       // talk / interact
        bool padY = false;       // alt-talk / interact
        bool padStart = false;   // start
        bool padBack = false;    // select / back (achievements)
        bool padDpadUp = false;
        bool padDpadDown = false;
        bool padDpadLeft = false;
        bool padDpadRight = false;
        // Keyboard fallback. Match the patches' default scheme.
        bool keyAccept = false;       // Z / Space / Return
        bool keyCancel = false;       // X / Backspace
        bool keyStart = false;        // Enter
        bool keySelect = false;       // Tab
        bool keyTalk = false;         // E / F
        bool keyOpenMap = false;      // M
        bool keyUp = false, keyDown = false, keyLeft = false, keyRight = false;
        // Time slice for state machines that need it (Loading fade
        // timer, StageHud per-frame tick, Results tally).
        float deltaSeconds = 0.0f;
        // Loading-screen progress; host fills 0..1 from its own
        // resource manager.
        float loadingProgress = 0.0f;
    };

    // Edge-detection state: keep last frame's raw snapshot so we
    // can compute "tapped" (rising-edge) from "down" (held).
    struct SgfxInputEdgeState
    {
        SgfxRawInputSnapshot lastFrame;
    };

    // Convert a current-frame raw snapshot into a SgfxFrameInput
    // ready for the orchestrator. Pure function; the only state it
    // needs is the previous-frame snapshot (to detect edges).
    inline SgfxFrameInput mapInputSnapshotToFrameInput(
        const SgfxRawInputSnapshot& cur,
        const SgfxRawInputSnapshot& prev)
    {
        // Tap = pressed this frame, not pressed last frame.
        auto tapped = [](bool a, bool b) { return a && !b; };

        SgfxFrameInput out;
        out.acceptTapped = tapped(cur.padA, prev.padA)
                         || tapped(cur.keyAccept, prev.keyAccept);
        out.cancelTapped = tapped(cur.padB, prev.padB)
                         || tapped(cur.keyCancel, prev.keyCancel);
        out.startTapped  = tapped(cur.padStart, prev.padStart)
                         || tapped(cur.keyStart, prev.keyStart);
        out.selectTapped = tapped(cur.padBack, prev.padBack)
                         || tapped(cur.keySelect, prev.keySelect);
        out.upTapped     = tapped(cur.padDpadUp, prev.padDpadUp)
                         || tapped(cur.keyUp, prev.keyUp);
        out.downTapped   = tapped(cur.padDpadDown, prev.padDpadDown)
                         || tapped(cur.keyDown, prev.keyDown);
        out.leftTapped   = tapped(cur.padDpadLeft, prev.padDpadLeft)
                         || tapped(cur.keyLeft, prev.keyLeft);
        out.rightTapped  = tapped(cur.padDpadRight, prev.padDpadRight)
                         || tapped(cur.keyRight, prev.keyRight);
        // Talk = X / Y on the pad, E / F on the keyboard.
        out.talkTapped   = tapped(cur.padX, prev.padX)
                         || tapped(cur.padY, prev.padY)
                         || tapped(cur.keyTalk, prev.keyTalk);
        // Open-town-map = Back/Select OR M key. Distinct from talk.
        out.openMapTapped = tapped(cur.padBack, prev.padBack)
                          || tapped(cur.keyOpenMap, prev.keyOpenMap);
        out.deltaSeconds = cur.deltaSeconds;
        out.loadingProgress = cur.loadingProgress;
        return out;
    }

    // Convenience wrapper: keeps the previous-frame snapshot internally.
    // Host calls update(cur) each frame.
    struct SgfxInputAdapter
    {
        SgfxInputEdgeState edges;

        SgfxFrameInput update(const SgfxRawInputSnapshot& cur)
        {
            const auto out = mapInputSnapshotToFrameInput(cur, edges.lastFrame);
            edges.lastFrame = cur;
            return out;
        }
    };

} // namespace sward::ui_runtime::generated::sgfx_hud
