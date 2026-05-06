// Phase 356: SGFX BGM bank loader.
//
// Retail Sonic Unleashed BGM ships in CRI ADX2 sound-bank `.csb`
// files (e.g. `bgm_sys_title.csb`, `bgm_act_apotos.csb`,
// `bgm_sys_result.csb`). Each .csb is a ROM-style container that
// carries one or more ADX-encoded streams plus the cue table that
// names each stream. SGFX does not bundle a CSB decoder; vgmstream
// is the canonical open-source library that handles it but is
// non-trivial to embed (large + GPL-LGPL split).
//
// The pragmatic SGFX path is: hosts pre-convert each retail .csb
// into a directory of OGG vorbis files, one per cue, with the file
// name matching the retail cue name verbatim. SGFX scans that
// directory at startup, reads each OGG, and registers its bytes
// with sgfxAudioPlayerRegisterBgm. The orchestrator's existing BGM
// admin (SgfxBgmAdmin::mountCue) then drives playback through the
// SDL_mixer streaming path Phase 347 wired up.
//
// File-name convention (matching retail cue names):
//
//     bank_dir/
//         bgm_sys_title.ogg          -> cue "bgm_sys_title"
//         bgm_sys_worldmap.ogg       -> cue "bgm_sys_worldmap"
//         bgm_act_apotos.ogg         -> cue "bgm_act_apotos"
//         bgm_act_hub_apotos.ogg     -> cue "bgm_act_hub_apotos"
//         bgm_sys_result.ogg         -> cue "bgm_sys_result"
//         bgm_sys_result_ng.ogg      -> cue "bgm_sys_result_ng"
//         ...
//
// The bank object owns the byte buffers (read fully into memory so
// SDL_mixer can stream them via Mix_LoadMUS_RW without re-opening
// the source file). Buffer addresses are stable for the lifetime
// of the SgfxBgmBank instance because storage is std::deque (which
// never relocates elements on push_back).
//
// To produce the OGG bank from retail .csb, vgmstream's CLI:
//
//     test.exe -o bank/<cue>.ogg <path-to>/<cue>.csb
//
// (vgmstream is not invoked by SGFX itself; it's the host's job to
// pre-convert. SGFX only consumes the OGG output.)

#pragma once

#include "sgfx_audio_player.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    struct SgfxBgmBank
    {
        // Stable-address storage. std::deque does not relocate
        // existing elements on push_back, so the byte pointer we
        // hand the audio player remains valid for the life of the
        // bank.
        std::deque<std::vector<unsigned char>> bytes;
        std::vector<std::string>               cueNames;
        std::size_t                            registeredCount = 0;
        std::size_t                            skippedNonOggCount = 0;
        std::size_t                            failedReadCount = 0;
    };

    namespace detail::bgm_bank
    {
        inline std::vector<unsigned char> readFileFully(
            const std::filesystem::path& path) noexcept
        {
            std::ifstream file(path, std::ios::binary);
            if (!file) return {};
            file.seekg(0, std::ios::end);
            const auto sz = file.tellg();
            file.seekg(0, std::ios::beg);
            if (sz <= 0) return {};
            std::vector<unsigned char> buf(static_cast<std::size_t>(sz));
            file.read(reinterpret_cast<char*>(buf.data()),
                      static_cast<std::streamsize>(sz));
            if (!file) return {};
            return buf;
        }
    }

    // Scan `dir` recursively for .ogg files; read each into bank
    // storage with the filename stem as its cue name. Returns the
    // count successfully read. Empty/missing dir returns 0 with no
    // side effects. The loader does NOT call the audio player --
    // the host calls sgfxBgmBankRegisterAllWithPlayer() afterwards
    // (or its own registrar) so this function stays SDL_mixer-free
    // and testable.
    //
    // The bank object accumulates entries across calls (you can
    // call this multiple times with different dirs to stack region
    // packs).
    inline std::size_t sgfxBgmBankLoadFromDir(
        SgfxBgmBank& bank,
        const std::filesystem::path& dir) noexcept
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return 0;

        std::size_t loadedThisCall = 0;
        for (auto it = fs::recursive_directory_iterator(dir, ec);
             it != fs::recursive_directory_iterator(); ++it)
        {
            if (ec) break;
            if (!it->is_regular_file(ec)) continue;
            const auto& p = it->path();
            if (p.extension() != ".ogg")
            {
                ++bank.skippedNonOggCount;
                continue;
            }
            auto buf = detail::bgm_bank::readFileFully(p);
            if (buf.empty())
            {
                ++bank.failedReadCount;
                continue;
            }
            const std::string cue = p.stem().string();
            bank.bytes.push_back(std::move(buf));
            bank.cueNames.push_back(cue);
            ++loadedThisCall;
        }
        return loadedThisCall;
    }

    // Register every cue currently in the bank with the SGFX audio
    // player. Pulled out as a separate function so the loader stays
    // testable without linking SDL_mixer. The bank must outlive the
    // audio player (the player keeps raw pointers into bank.bytes).
    inline void sgfxBgmBankRegisterAllWithPlayer(SgfxBgmBank& bank) noexcept
    {
        for (std::size_t i = 0; i < bank.bytes.size(); ++i)
        {
            sgfxAudioPlayerRegisterBgm(
                bank.cueNames[i],
                bank.bytes[i].data(),
                bank.bytes[i].size());
            ++bank.registeredCount;
        }
    }

    // Inspection helper: did the bank pick up a cue with this name?
    inline bool sgfxBgmBankHasCue(const SgfxBgmBank& bank,
                                  std::string_view cueName) noexcept
    {
        for (const auto& n : bank.cueNames)
            if (n == cueName) return true;
        return false;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
