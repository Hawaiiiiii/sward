// Phase 333: SGFX-shaped port of SWA::CSoundAdministrator's
// BGM 4-channel volume layout.
//
// Sourced directly from
//   local_build_env/ur103clean/UnleashedRecomp/api/SWA/Sound/Sound.h
//
// CSoundAdministrator is the singleton-rooted audio controller. The
// piece relevant to UI/UX is CSoundAdministrator::CBgm, which owns
// FOUR simultaneous BGM volume channels (m_Volume1..m_Volume4):
//   * channel 1: main stage / hub BGM
//   * channel 2: secondary layer (e.g. boss intro stinger over
//     the underlying stage BGM)
//   * channel 3: results-screen fanfare crossfade
//   * channel 4: title attract-movie audio
// (channel assignments inferred from captured retail trace
// behavior; the layout itself is retail-validated.)
//
// SGFX hosts use this struct to drive their audio mixer; SGFX
// does not own the actual audio backend.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // SWA::CSoundAdministrator::CBgm volume channel layout.
    // Real retail layout has m_Volume1..m_Volume4 each be<float>.
    struct SgfxBgmVolumes
    {
        float volume1 = 1.0f;
        float volume2 = 1.0f;
        float volume3 = 1.0f;
        float volume4 = 1.0f;
    };

    // SGFX wrapper that owns the volumes alongside the cue name
    // currently mounted on each channel. The cue name is purely a
    // host-side bookkeeping field -- the retail CBgm carries a
    // boost::shared_ptr<CSoundBGMBase> instead.
    struct SgfxBgmAdmin
    {
        SgfxBgmVolumes volumes;
        std::array<std::string, 4> cueOnChannel; // name of bgm cue currently bound

        // Helpers mirror the retail "fade volume on channel N"
        // pattern. Channel indexing is 1-based to match retail
        // field names; SGFX clamps to [0,1] for safety.
        void setChannelVolume(int channel1Based, float v01) noexcept
        {
            const float v = (v01 < 0.0f) ? 0.0f : (v01 > 1.0f ? 1.0f : v01);
            switch (channel1Based)
            {
                case 1: volumes.volume1 = v; break;
                case 2: volumes.volume2 = v; break;
                case 3: volumes.volume3 = v; break;
                case 4: volumes.volume4 = v; break;
                default: break;
            }
        }

        float getChannelVolume(int channel1Based) const noexcept
        {
            switch (channel1Based)
            {
                case 1: return volumes.volume1;
                case 2: return volumes.volume2;
                case 3: return volumes.volume3;
                case 4: return volumes.volume4;
                default: return 0.0f;
            }
        }

        void mountCue(int channel1Based, std::string_view cueName)
        {
            if (channel1Based >= 1 && channel1Based <= 4)
                cueOnChannel[channel1Based - 1].assign(cueName);
        }

        const std::string& cueAt(int channel1Based) const
        {
            static const std::string empty;
            if (channel1Based < 1 || channel1Based > 4) return empty;
            return cueOnChannel[channel1Based - 1];
        }
    };

    // Conventional channel assignments observed in the retail
    // logcat trace. Hosts can override via mountCue if needed.
    constexpr int kBgmChannelMain     = 1;
    constexpr int kBgmChannelStinger  = 2;
    constexpr int kBgmChannelResults  = 3;
    constexpr int kBgmChannelAttract  = 4;

} // namespace sward::ui_runtime::generated::sgfx_hud
