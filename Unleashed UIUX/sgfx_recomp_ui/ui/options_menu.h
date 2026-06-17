#pragma once

// DECOUPLED from UnleashedRecomp/ui/options_menu.h: <api/SWA.h> -> the shim.
// The only game type the public interface needs is SWA::EMenuType (the pause-menu
// context: world-map vs stage vs hub). The shim provides it under its recomp name so
// the class declaration stays byte-identical.
#include "../platform/sgfx_platform.h"

class OptionsMenu
{
public:
    static inline bool s_isVisible = false;
    static inline bool s_isPause = false;
    static inline bool s_isRestartRequired = false;

    static inline SWA::EMenuType s_pauseMenuType;

    static void Init();
    static void Draw();
    static void Open(bool isPause = false, SWA::EMenuType pauseMenuType = SWA::eMenuType_WorldMap);
    static void Close();

    static bool CanClose();
};
