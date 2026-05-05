// Phase 312: SGFX audio dispatch.
//
// Mirrors UnleashedRecomp/apu/embedded_player.cpp's cue lookup
// table -- which is itself the host-side ground truth for which
// of retail Sonic Unleashed's bank cues the runtime hosts as
// embedded OGG blobs. SGFX exposes:
//
//   * An enum of all currently-embedded cues, matched 1:1 to
//     EmbeddedPlayer's enum class EmbeddedSound.
//   * A pure-function lookup taking a cue-name string view and
//     returning a (bytes, size) pair pointing at the .ogg.h
//     extern arrays. No copying, no allocation.
//   * A status flag indicating whether the cue is wired (bytes
//     present and non-zero size).
//
// The actual SDL_mixer / Mix_PlayChannel / Mix_LoadWAV_RW
// playback wiring lives in the SGFX-runtime EXE (Phase 314),
// not here. This header is dependency-free so any host can
// consume the byte slices.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

// The embedded byte arrays (g_sys_*) are defined in their .ogg.c
// translation units and consumed by the implementation file
// sgfx_audio_dispatch.cpp via the corresponding .ogg.h headers.
// Header-side consumers don't see the symbols directly; they
// resolve everything through lookupEmbeddedCueByName / -BySlot
// which return a (bytes, size) slice.

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Embedded SFX cues currently shipped by UnleashedRecomp's
    // EmbeddedPlayer. SGFX picks these up directly so the same
    // OGG bytes feed both the existing recomp runtime and any
    // standalone SGFX EXE.
    enum class EmbeddedCue : std::uint8_t
    {
        SysWorldMapCursor      = 0,
        SysWorldMapFinalDecide = 1,
        SysActStgPauseCansel   = 2,
        SysActStgPauseCursor   = 3,
        SysActStgPauseDecide   = 4,
        SysActStgPauseWinClose = 5,
        SysActStgPauseWinOpen  = 6,
        Count                  = 7,
    };

    // (bytes, size) slice into one of the embedded OGG arrays.
    // bytes == nullptr when the cue name doesn't map to an
    // embedded blob; the host should then fall back to a CSB-
    // bank lookup or no-op the playback.
    struct EmbeddedCueBytes
    {
        const unsigned char* bytes = nullptr;
        std::size_t          size = 0;
        bool present() const noexcept { return bytes != nullptr && size > 0; }
    };

    // Map a cue-name string view (matching the strings the state
    // machines emit via PauseEvent / TitleMenuEvent / etc.) to its
    // embedded byte slice. Defined out-of-line because the byte
    // arrays themselves come from .ogg.h compilation units that
    // we don't want to drag into every header consumer.
    EmbeddedCueBytes lookupEmbeddedCueByName(std::string_view name) noexcept;

    // Map an EmbeddedCue enum value to its byte slice. Same return
    // shape; convenient for code that already has the enum.
    EmbeddedCueBytes lookupEmbeddedCueBySlot(EmbeddedCue slot) noexcept;

    // Return true iff the SGFX audio dispatch can resolve `name`
    // to an embedded blob right now. False means the host must
    // wire the cue via a non-embedded path.
    inline bool isEmbeddedCue(std::string_view name) noexcept
    {
        return lookupEmbeddedCueByName(name).present();
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
