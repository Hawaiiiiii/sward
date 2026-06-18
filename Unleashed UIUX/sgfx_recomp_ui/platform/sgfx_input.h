// =============================================================================
// sgfx_input.h — the in-game input facade (was api/SWA.h's CInputState). The menus
// read the controller through SWA::CInputState::GetInstance()->GetPadState() when
// running in-game. The lib declares it; a host feeds it each frame from its real
// input (a preview can leave it neutral). Symbols keep their recomp names so the
// menu bodies stay byte-identical. Reached only when App::s_isInit (in-game).
// =============================================================================
#pragma once

namespace SWA
{
    enum eKeyState
    {
        eKeyState_A, eKeyState_B, eKeyState_X, eKeyState_Y,
        eKeyState_DpadUp, eKeyState_DpadDown, eKeyState_DpadLeft, eKeyState_DpadRight,
        eKeyState_LeftBumper, eKeyState_RightBumper,
        eKeyState_LeftTrigger, eKeyState_RightTrigger,
        eKeyState_Start, eKeyState_Back,
        eKeyState_LeftStickButton, eKeyState_RightStickButton,
    };

    struct SPadState
    {
        float LeftStickHorizontal = 0.0f;
        float LeftStickVertical   = 0.0f;
        float RightStickHorizontal = 0.0f;
        float RightStickVertical   = 0.0f;
        bool IsDown(eKeyState key) const;
        bool IsTapped(eKeyState key) const;
        bool IsReleased(eKeyState key) const;
    };

    class CInputState
    {
    public:
        static CInputState* GetInstance();
        SPadState& GetPadState();
    };

    // pause-menu context (was api/SWA HudPause). Decides accessible options + the stage intro.
    enum EMenuType { eMenuType_WorldMap, eMenuType_Title, eMenuType_Stage, eMenuType_Hub };

    struct SGlobals { static bool* ms_IsRenderHud; };   // host points at its HUD-visibility flag
}
