// Ported 1:1 from UnleashedRecomp/ui/black_bar.h — the pillarbox / loading letterbox
// bars. UNCHANGED except: self-contained includes (the recomp relied on the includer
// pulling in imgui + cstdint). No game-runtime coupling lives in this header.
#pragma once

#include <cstdint>
#include <imgui.h>

struct BlackBar
{
    static inline bool g_inspirePillarbox;

    static inline ImVec2 g_loadingBlackBarMin;
    static inline ImVec2 g_loadingBlackBarMax;
    static inline uint8_t g_loadingBlackBarAlpha;

    static void Draw();
};
