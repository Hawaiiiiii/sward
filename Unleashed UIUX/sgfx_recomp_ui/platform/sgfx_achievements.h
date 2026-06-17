// =============================================================================
// sgfx_achievements.h — the achievement-DB shim (was kernel/xdbf.h +
// user/achievement_manager.h). The achievement list, names, icons and unlock state
// are HARD game state (the running title's XDBF/save). The lib only DECLARES this
// provider; a host binds it to the real DB (or a demo list for a preview). Symbols
// keep their recomp names so achievement_overlay/_menu stay byte-identical.
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "sgfx_config.h"   // ELanguage

enum class EXDBFLanguage : uint32_t { English = 1, Japanese, German, French, Spanish, Italian };

struct Achievement
{
    uint16_t    ID = 0;
    std::string Name;
    std::string Description;
    std::string UnlockedDescription;
};

struct XdbfWrapper
{
    Achievement              GetAchievement(EXDBFLanguage language, uint16_t id);
    std::vector<Achievement> GetAchievements(EXDBFLanguage language);
};
extern XdbfWrapper g_xdbfWrapper;

namespace xdbf { std::string FixInvalidSequences(const std::string& str); }

namespace AchievementManager
{
    bool   IsUnlocked(uint16_t id);
    int64_t GetTimestamp(uint16_t id);
    int    GetTotalRecords();
}

inline constexpr int ACH_RECORDS = 50;   // total achievements (trophy-tier thresholds)
