// Phase 337: SDL_mixer-backed implementation of sgfx_audio_player.hpp.
//
// Pattern mirrors UnleashedRecomp's EmbeddedPlayer (apu/embedded_player.cpp:
// Mix_LoadWAV_RW + SDL_RWFromConstMem for lazy-loaded chunks).
// The cue blobs come from sgfx_audio_dispatch's embedded byte arrays.

#include "sward/ui_runtime/sgfx_audio_player.hpp"

#include <SDL.h>
#include <SDL_mixer.h>

#include <cstring>
#include <string>
#include <unordered_map>

namespace sward::ui_runtime::generated::sgfx_hud
{
    namespace
    {
        bool g_isActive = false;
        std::size_t g_channelIndex = 0;
        std::array<Mix_Chunk*, static_cast<std::size_t>(EmbeddedCue::Count)> g_chunks{};
        float g_masterVolume = 1.0f;

        // BGM channel 1 is mapped onto SDL_mixer's music slot.
        Mix_Music* g_bgmMusic = nullptr;
        std::string g_bgmCurrentCueName;

        // Phase 347: cue name -> registered byte blob. Host owns
        // the bytes; SGFX just stores the (ptr, size) slice.
        struct BgmBlob { const unsigned char* bytes; std::size_t size; };
        std::unordered_map<std::string, BgmBlob> g_bgmRegistry;
    }

    bool sgfxAudioPlayerRegisterBgm(std::string_view cueName,
                                    const unsigned char* bytes,
                                    std::size_t size) noexcept
    {
        if (cueName.empty() || bytes == nullptr || size == 0) return false;
        auto [it, inserted] = g_bgmRegistry.insert_or_assign(
            std::string(cueName), BgmBlob{bytes, size});
        (void)it;
        return !inserted; // returns true when an existing entry was replaced
    }

    bool sgfxAudioPlayerInit() noexcept
    {
        if (g_isActive) return true;
        if (SDL_WasInit(SDL_INIT_AUDIO) == 0)
        {
            if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
                return false;
        }
        // Match UnleashedRecomp's EmbeddedPlayer settings (48kHz F32 stereo).
        if (Mix_OpenAudio(48000, AUDIO_F32SYS, 2, 4096) != 0)
            return false;
        // 8 simultaneous SFX channels (UI cues rarely overlap more than 3).
        Mix_AllocateChannels(8);
        for (auto& c : g_chunks) c = nullptr;
        g_channelIndex = 0;
        g_isActive = true;
        return true;
    }

    void sgfxAudioPlayerShutdown() noexcept
    {
        if (!g_isActive) return;
        Mix_HaltChannel(-1);
        for (auto& c : g_chunks)
        {
            if (c != nullptr) { Mix_FreeChunk(c); c = nullptr; }
        }
        if (g_bgmMusic != nullptr)
        {
            Mix_HaltMusic();
            Mix_FreeMusic(g_bgmMusic);
            g_bgmMusic = nullptr;
        }
        g_bgmCurrentCueName.clear();
        Mix_CloseAudio();
        g_isActive = false;
    }

    bool sgfxAudioPlayerIsActive() noexcept { return g_isActive; }

    bool sgfxAudioPlayerPlayCue(std::string_view cueName) noexcept
    {
        if (!g_isActive) return false;
        const auto info = lookupEmbeddedCueByName(cueName);
        if (!info.present()) return false;
        const auto idx = static_cast<std::size_t>(info.slot);
        if (idx >= g_chunks.size()) return false;

        if (g_chunks[idx] == nullptr)
        {
            SDL_RWops* rw = SDL_RWFromConstMem(info.bytes, static_cast<int>(info.size));
            if (rw == nullptr) return false;
            // Mix_LoadWAV_RW frees the rw on success/failure when 1 is passed.
            g_chunks[idx] = Mix_LoadWAV_RW(rw, 1);
            if (g_chunks[idx] == nullptr) return false;
        }
        const int volume = static_cast<int>(g_masterVolume * MIX_MAX_VOLUME);
        Mix_VolumeChunk(g_chunks[idx], volume);
        // Channel rotation: scattered playback so re-trigger doesn't cut self.
        Mix_PlayChannel(static_cast<int>(g_channelIndex % 8), g_chunks[idx], 0);
        ++g_channelIndex;
        return true;
    }

    void sgfxAudioPlayerSetMasterVolume(float v01) noexcept
    {
        g_masterVolume = (v01 < 0.0f) ? 0.0f : (v01 > 1.0f ? 1.0f : v01);
    }

    void sgfxAudioPlayerApplyBgmAdmin(const SgfxBgmAdmin& admin) noexcept
    {
        if (!g_isActive) return;
        // Map BGM channel 1 onto SDL_mixer's single music slot. SGFX
        // tracks the cue name + volume; when the cue changes AND a
        // matching blob is registered (Phase 347), reload + replay.
        const auto& cue = admin.cueAt(kBgmChannelMain);
        if (cue != g_bgmCurrentCueName)
        {
            if (g_bgmMusic != nullptr)
            {
                Mix_HaltMusic();
                Mix_FreeMusic(g_bgmMusic);
                g_bgmMusic = nullptr;
            }
            g_bgmCurrentCueName = cue;
            if (!cue.empty())
            {
                auto it = g_bgmRegistry.find(cue);
                if (it != g_bgmRegistry.end())
                {
                    SDL_RWops* rw = SDL_RWFromConstMem(
                        it->second.bytes, static_cast<int>(it->second.size));
                    if (rw != nullptr)
                    {
                        // SDL_FreeRW called by SDL_mixer when the second arg is 1.
                        g_bgmMusic = Mix_LoadMUS_RW(rw, 1);
                        if (g_bgmMusic != nullptr)
                        {
                            // Loop forever (-1 in SDL_mixer parlance).
                            Mix_PlayMusic(g_bgmMusic, -1);
                        }
                    }
                }
            }
        }
        const float vol01 = admin.getChannelVolume(kBgmChannelMain) * g_masterVolume;
        Mix_VolumeMusic(static_cast<int>(vol01 * MIX_MAX_VOLUME));
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
