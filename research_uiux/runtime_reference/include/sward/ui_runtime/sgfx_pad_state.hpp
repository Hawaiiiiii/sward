// Phase 326: SGFX-shaped port of SWA::SPadState + SWA::EKeyState.
//
// Sourced directly from
//   local_build_env/ur103clean/UnleashedRecomp/api/SWA/System/PadState.h
//   local_build_env/ur103clean/UnleashedRecomp/api/SWA/System/InputState.h
//
// CInputState owns 8 SPadState slots in retail (one per pad index).
// All retail UI/UX state machines (CTitleStateMenu, CTitleStateWorldMap,
// CHudPause, CGeneralWindow, CHudSonicStage) read from
// CInputState::GetInstance()->GetPadState() and call IsTapped /
// IsDown / IsReleased / IsUp on the bitfields. The earlier SGFX
// input layer mapped SDL inputs straight to "tapped" booleans which
// loses the retail bitfield shape -- this header preserves it so
// the same bitmask checks work in SGFX.
//
// Field offsets (from SWA_INSERT_PADDING in the upstream header):
//   DownState              @ +0x00
//   UpState                @ +0x04
//   TappedState            @ +0x08
//   ReleasedState          @ +0x0C
//   LeftStickHorizontal    @ +0x10
//   LeftStickVertical      @ +0x14
//   <pad +0x04>
//   RightStickHorizontal   @ +0x1C
//   RightStickVertical     @ +0x20
//   <pad +0x04>
//   LeftTrigger            @ +0x28
//   RightTrigger           @ +0x2C

#pragma once

#include <array>
#include <cstdint>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Mirror of SWA::EKeyState (retail bitfield values, do not change).
    enum SgfxKeyState : std::uint32_t
    {
        kSgfxKey_None              = 0x0,
        kSgfxKey_A                 = 0x1,
        kSgfxKey_B                 = 0x2,
        kSgfxKey_X                 = 0x8,
        kSgfxKey_Y                 = 0x10,
        kSgfxKey_DpadUp            = 0x40,
        kSgfxKey_DpadDown          = 0x80,
        kSgfxKey_DpadLeft          = 0x100,
        kSgfxKey_DpadRight         = 0x200,
        kSgfxKey_Start             = 0x400,
        kSgfxKey_Select            = 0x800,
        kSgfxKey_LeftBumper        = 0x1000,
        kSgfxKey_RightBumper       = 0x2000,
        kSgfxKey_LeftTrigger       = 0x4000,
        kSgfxKey_RightTrigger      = 0x8000,
        kSgfxKey_LeftStick         = 0x10000,
        kSgfxKey_RightStick        = 0x20000,
        kSgfxKey_LeftStickUp       = 0x40000,
        kSgfxKey_LeftStickDown     = 0x80000,
        kSgfxKey_LeftStickLeft     = 0x100000,
        kSgfxKey_LeftStickRight    = 0x200000,
        kSgfxKey_RightStickUp      = 0x400000,
        kSgfxKey_RightStickDown    = 0x800000,
        kSgfxKey_RightStickLeft    = 0x1000000,
        kSgfxKey_RightStickRight   = 0x2000000,
    };

    // Mirror of SWA::SPadState. Field order + member methods match
    // the retail header. All four state masks are populated by
    // CInputState's per-frame snapshot loop in retail; SGFX hosts
    // fill them from SDL or keyboard via padStateFromRawSnapshot().
    struct SgfxPadState
    {
        std::uint32_t downState     = 0;
        std::uint32_t upState       = 0;
        std::uint32_t tappedState   = 0;
        std::uint32_t releasedState = 0;
        float leftStickHorizontal   = 0.0f;
        float leftStickVertical     = 0.0f;
        float rightStickHorizontal  = 0.0f;
        float rightStickVertical    = 0.0f;
        float leftTrigger           = 0.0f;
        float rightTrigger          = 0.0f;

        constexpr bool isDown(SgfxKeyState mask) const noexcept
        { return (downState & mask) != 0; }
        constexpr bool isUp(SgfxKeyState mask) const noexcept
        { return (upState & mask) != 0; }
        constexpr bool isTapped(SgfxKeyState mask) const noexcept
        { return (tappedState & mask) != 0; }
        constexpr bool isReleased(SgfxKeyState mask) const noexcept
        { return (releasedState & mask) != 0; }
    };

    // Mirror of SWA::CInputState. Retail keeps 8 slots; menu code
    // reads via GetPadState() which returns slot[currentPadIndex].
    struct SgfxInputState
    {
        std::array<SgfxPadState, 8> padStates;
        std::uint32_t currentPadStateIndex = 0;

        const SgfxPadState& getPadState() const noexcept
        { return padStates[currentPadStateIndex & 7]; }
        SgfxPadState& mutablePadState() noexcept
        { return padStates[currentPadStateIndex & 7]; }
    };

    // Helper for hosts that already have raw "down this frame" /
    // "down last frame" booleans (the SDL adapter shape from
    // sgfx_input_layer.hpp). Computes the four retail bitmasks from
    // a current+previous bool pair using the same edge logic the
    // retail xenon/sdl layer uses.
    struct SgfxPadEdgeSample
    {
        bool a, b, x, y;
        bool dpadUp, dpadDown, dpadLeft, dpadRight;
        bool start, select;
        bool leftBumper, rightBumper;
        bool leftStickClick, rightStickClick;
        bool leftStickUp, leftStickDown, leftStickLeft, leftStickRight;
        bool rightStickUp, rightStickDown, rightStickLeft, rightStickRight;
        float leftStickH = 0.0f, leftStickV = 0.0f;
        float rightStickH = 0.0f, rightStickV = 0.0f;
        float leftTrigger = 0.0f, rightTrigger = 0.0f;
    };

    inline std::uint32_t sgfxComputeKeyMask(const SgfxPadEdgeSample& s) noexcept
    {
        std::uint32_t m = 0;
        if (s.a) m |= kSgfxKey_A;
        if (s.b) m |= kSgfxKey_B;
        if (s.x) m |= kSgfxKey_X;
        if (s.y) m |= kSgfxKey_Y;
        if (s.dpadUp)    m |= kSgfxKey_DpadUp;
        if (s.dpadDown)  m |= kSgfxKey_DpadDown;
        if (s.dpadLeft)  m |= kSgfxKey_DpadLeft;
        if (s.dpadRight) m |= kSgfxKey_DpadRight;
        if (s.start)  m |= kSgfxKey_Start;
        if (s.select) m |= kSgfxKey_Select;
        if (s.leftBumper)  m |= kSgfxKey_LeftBumper;
        if (s.rightBumper) m |= kSgfxKey_RightBumper;
        if (s.leftStickClick)  m |= kSgfxKey_LeftStick;
        if (s.rightStickClick) m |= kSgfxKey_RightStick;
        if (s.leftStickUp)    m |= kSgfxKey_LeftStickUp;
        if (s.leftStickDown)  m |= kSgfxKey_LeftStickDown;
        if (s.leftStickLeft)  m |= kSgfxKey_LeftStickLeft;
        if (s.leftStickRight) m |= kSgfxKey_LeftStickRight;
        if (s.rightStickUp)    m |= kSgfxKey_RightStickUp;
        if (s.rightStickDown)  m |= kSgfxKey_RightStickDown;
        if (s.rightStickLeft)  m |= kSgfxKey_RightStickLeft;
        if (s.rightStickRight) m |= kSgfxKey_RightStickRight;
        if (s.leftTrigger  > 0.5f) m |= kSgfxKey_LeftTrigger;
        if (s.rightTrigger > 0.5f) m |= kSgfxKey_RightTrigger;
        return m;
    }

    // Retail-shape edge math: tapped = mask & ~prev, released =
    // ~mask & prev, down = mask, up = ~mask. Mirrors
    // CXboxInputDevice::Update from XCE-era retail engines.
    inline void sgfxApplyPadEdges(
        SgfxPadState& s,
        const SgfxPadEdgeSample& cur,
        const SgfxPadEdgeSample& prev) noexcept
    {
        const std::uint32_t curMask  = sgfxComputeKeyMask(cur);
        const std::uint32_t prevMask = sgfxComputeKeyMask(prev);
        s.downState     = curMask;
        s.upState       = ~curMask;
        s.tappedState   = curMask & ~prevMask;
        s.releasedState = ~curMask & prevMask;
        s.leftStickHorizontal  = cur.leftStickH;
        s.leftStickVertical    = cur.leftStickV;
        s.rightStickHorizontal = cur.rightStickH;
        s.rightStickVertical   = cur.rightStickV;
        s.leftTrigger          = cur.leftTrigger;
        s.rightTrigger         = cur.rightTrigger;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
