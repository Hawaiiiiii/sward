#pragma once

// DECOUPLED from UnleashedRecomp/ui/achievement_menu.h — the header is already
// self-contained (only POD + bool); the game coupling lives entirely in the .cpp.

class AchievementMenu
{
public:
    inline static bool s_isVisible = false;

    static void Init();
    static void Draw();
    static void Open();
    static void Close();
};
