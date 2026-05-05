// Phase 337: SGFX SDL_mixer audio backend.
//
// Wraps SDL2_mixer to actually play the embedded OGG cue blobs we
// already package via sgfx_audio_dispatch. Mirrors UnleashedRecomp's
// EmbeddedPlayer (apu/embedded_player.cpp) -- same Mix_LoadWAV_RW +
// SDL_RWFromConstMem pattern, same channel-rotation approach, plus
// a 4-channel BGM stream that maps onto SgfxBgmAdmin.
//
// The cue blobs themselves live in
// local_build_env/ur103clean/UnleashedRecomp/res/sounds/*.ogg.c which
// sgfx_audio_dispatch already links into the build. This file only
// owns the SDL_mixer plumbing.

#pragma once

#include "sgfx_audio_dispatch.hpp"
#include "sgfx_sound_admin.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

// Forward-declare so callers don't need SDL2/SDL_mixer transitively.
struct Mix_Chunk;
struct Mix_Music;

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Init / shutdown owns Mix_OpenAudio. Idempotent.
    bool sgfxAudioPlayerInit() noexcept;
    void sgfxAudioPlayerShutdown() noexcept;
    bool sgfxAudioPlayerIsActive() noexcept;

    // Play a cue by name. Returns false when the cue isn't in the
    // embedded set (e.g. the .csb-bank cues that SGFX has not yet
    // bundled like sys_worldmap_decide). The host can fall back to
    // a synth or silence.
    bool sgfxAudioPlayerPlayCue(std::string_view cueName) noexcept;

    // Master volume scale (0..1) applied to all SFX channels.
    void sgfxAudioPlayerSetMasterVolume(float v01) noexcept;

    // Phase 347: register a BGM cue name -> byte blob mapping. The
    // host owns the bytes (must outlive the player); SGFX just
    // tracks the slice. Multiple cue names can share the same blob
    // (e.g. all in-stage BGMs pointing at one demo OGG). Returns
    // true if the registration replaced an existing entry.
    bool sgfxAudioPlayerRegisterBgm(std::string_view cueName,
                                    const unsigned char* bytes,
                                    std::size_t size) noexcept;

    // Apply BGM admin state to the actual SDL_mixer music channel.
    // Channel 1's volume + cue is what plays as music; channels 2-4
    // are tracked but only the loudest active one streams (SDL_mixer
    // has a single music slot). Hosts that want true 4-channel BGM
    // would need to use streamed Mix_Chunk on extra channels.
    //
    // Phase 347: when the cue on channel 1 changes AND the new cue
    // is registered via sgfxAudioPlayerRegisterBgm, the player
    // halts the current music + loads the new bytes via
    // Mix_LoadMUS_RW + plays via Mix_PlayMusic with infinite loop.
    void sgfxAudioPlayerApplyBgmAdmin(const SgfxBgmAdmin& admin) noexcept;

} // namespace sward::ui_runtime::generated::sgfx_hud
