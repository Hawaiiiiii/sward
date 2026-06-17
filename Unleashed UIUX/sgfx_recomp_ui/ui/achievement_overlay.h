#pragma once

// DECOUPLED: the header relied on the game runtime to pull in <cstdint>
// (uint16_t); make it self-contained. Body UNCHANGED from the recomp.
#include <queue>
#include <cstdint>

class AchievementOverlay
{
public:
    static inline bool s_isVisible = false;

    static inline std::queue<uint16_t> s_queue{};

    static void Init();
    static void Draw();
    static void Open(int id);
    static void Close();
};
