#pragma once

#include <cstdint>
#include <string_view>

// =============================================================================
// Optional runtime CSD (SurfRide) UI capture.
//
// When the environment variable SWA_CSD_CAPTURE is set to a non-zero value, every
// UI cast the game actually DRAWS is appended (one JSON object per line) to
// "csd_capture.jsonl" in the working directory:
//     {"path":"ui_pause/header/status_title", "rect":[x,y,w,h], "tex":"mat_pause_en_002", "textured":true}
// rect is in the game's 1280x720 reference space (the vertices as the engine built
// them, before the aspect-ratio transform). This recovers what static .yncp
// extraction cannot place: runtime-anchored elements (pause title bar, boss gauge
// frame) and runtime-positioned text/number sprites.
//
// Feed the file to:  Unleashed UIUX/tools/runtime_capture_to_manifest.py --stream
//
// Entirely INERT unless SWA_CSD_CAPTURE is set — every entry point early-outs on the
// cached Enabled() check, so normal play is unaffected. Capture is read-only with
// respect to the game's vertex buffers.
// =============================================================================
namespace CsdCapture
{
    // Cached check of the SWA_CSD_CAPTURE env var (evaluated once).
    bool Enabled();

    // Mirror of EmplacePath / the g_paths erase in aspect_ratio_patches.cpp: keep the
    // human-readable cast path string (g_paths itself only stores an XXH64 hash).
    void SetPath(const void* key, std::string_view path);
    void ErasePathRange(const void* lo, const void* hi);

    // The live cast / cast-node host pointer for the element about to be drawn, set from
    // the same mid-asm hooks that drive FindModifier (RenderCsdCast/CastNode).
    void NoteCurrentCast(const void* hostPtr);
    void NoteCurrentNode(const void* hostPtr);

    // Map a CSD texture (host GuestTexture*) to its name, built once at load.
    void RegisterTexture(const void* hostTexture, const char* name);

    // Record one drawn cast. `verts` = host pointer to the cast's vertex array
    // (base + ctx.r4), `count` vertices of `stride` bytes (0x14 textured / 0xC not),
    // big-endian float x@0x00 / y@0x04. `textured` selects the slot-0 texture name.
    void RecordDraw(const uint8_t* verts, uint32_t count, uint32_t stride, bool textured);
}
