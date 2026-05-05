// Phase 312: SGFX audio dispatch implementation.
//
// Pulls the .ogg.h embedded byte arrays straight from
// UnleashedRecomp's res/sounds tree (the same source of truth
// EmbeddedPlayer reads from) and exposes them through SGFX's
// lookup API. Same arrays, same sizes; SGFX is just a
// dependency-free packaging layer.

#include "sward/ui_runtime/sgfx_audio_dispatch.hpp"

#include "res/sounds/sys_worldmap_cursor.ogg.h"
#include "res/sounds/sys_worldmap_finaldecide.ogg.h"
#include "res/sounds/sys_actstg_pausecansel.ogg.h"
#include "res/sounds/sys_actstg_pausecursor.ogg.h"
#include "res/sounds/sys_actstg_pausedecide.ogg.h"
#include "res/sounds/sys_actstg_pausewinclose.ogg.h"
#include "res/sounds/sys_actstg_pausewinopen.ogg.h"

#include <array>

namespace sward::ui_runtime::generated::sgfx_hud
{
    namespace
    {
        struct CueRow
        {
            std::string_view     name;
            EmbeddedCue          slot;
            const unsigned char* bytes;
            std::size_t          size;
        };

        const std::array<CueRow, static_cast<std::size_t>(EmbeddedCue::Count)> kCueTable =
        {{
            {"sys_worldmap_cursor",      EmbeddedCue::SysWorldMapCursor,
                g_sys_worldmap_cursor,      sizeof(g_sys_worldmap_cursor)},
            {"sys_worldmap_finaldecide", EmbeddedCue::SysWorldMapFinalDecide,
                g_sys_worldmap_finaldecide, sizeof(g_sys_worldmap_finaldecide)},
            {"sys_actstg_pausecansel",   EmbeddedCue::SysActStgPauseCansel,
                g_sys_actstg_pausecansel,   sizeof(g_sys_actstg_pausecansel)},
            {"sys_actstg_pausecursor",   EmbeddedCue::SysActStgPauseCursor,
                g_sys_actstg_pausecursor,   sizeof(g_sys_actstg_pausecursor)},
            {"sys_actstg_pausedecide",   EmbeddedCue::SysActStgPauseDecide,
                g_sys_actstg_pausedecide,   sizeof(g_sys_actstg_pausedecide)},
            {"sys_actstg_pausewinclose", EmbeddedCue::SysActStgPauseWinClose,
                g_sys_actstg_pausewinclose, sizeof(g_sys_actstg_pausewinclose)},
            {"sys_actstg_pausewinopen",  EmbeddedCue::SysActStgPauseWinOpen,
                g_sys_actstg_pausewinopen,  sizeof(g_sys_actstg_pausewinopen)},
        }};
    } // namespace

    EmbeddedCueBytes lookupEmbeddedCueByName(std::string_view name) noexcept
    {
        for (const auto& row : kCueTable)
            if (row.name == name)
                return {row.bytes, row.size};
        return {};
    }

    EmbeddedCueBytes lookupEmbeddedCueBySlot(EmbeddedCue slot) noexcept
    {
        for (const auto& row : kCueTable)
            if (row.slot == slot)
                return {row.bytes, row.size};
        return {};
    }
} // namespace sward::ui_runtime::generated::sgfx_hud
