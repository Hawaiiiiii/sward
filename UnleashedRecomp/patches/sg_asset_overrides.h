#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace SGAssetOverrides
{
    // Phase 369A: pixel-level texture override for retail SU's
    // CTexturePicture/MakePictureData lane.
    //
    // Loaded from <SG_PREFLIGHT_OVERRIDE_DIR>/sg_asset_overrides.json
    // (or the path declared by Phase 370B's sgfx_pack.json):
    //
    //   {
    //     "version": 1,
    //     "pictures": {
    //       "logo_sonicteam": "sgfx_assets/logo_sgfx.dds"
    //     }
    //   }
    //
    // The key is the guest CTexturePicture name (the literal that
    // retail SU stores in `pictureData->name`, NOT the file path on
    // disk). The value is a path relative to the override dir. The
    // file at that path must be a raw `DDS ` magic file.

    // Phase 370C: opaque keep-alive returned by `TryGetPixelOverride`.
    // The handle holds a shared_ptr to the immutable picture
    // snapshot that owned the override bytes at lookup time, so a
    // concurrent hot-reload that swaps the global snapshot CANNOT
    // free the bytes while the caller (MakePictureData -> ddspp ->
    // LoadTexture) is still reading them. The handle is empty
    // (`data == nullptr`) when the picture name has no override.
    //
    // Lifetime contract: the caller MUST keep the handle alive for
    // the entire duration of any read from `data`. Once the handle
    // is destroyed, the picture bytes may be freed at the next
    // reload that drops the last reference to the snapshot.
    struct PixelOverrideHandle
    {
        const uint8_t* data = nullptr;
        std::size_t    size = 0;

        // Internal keep-alive. Type-erased to `void` so this header
        // does not pull the snapshot definition into every TU that
        // includes it; the destructor still correctly drops the
        // snapshot ref because `std::shared_ptr<void>`'s deleter is
        // captured at construction time.
        std::shared_ptr<const void> keepAlive;
    };

    // Idempotent boot-time loader. First call after the override dir
    // is known reads sg_asset_overrides.json (or the pack-pointed
    // path) and pre-decodes each referenced raw DDS file into a host
    // cache. Re-calls return immediately; the post-boot reload path
    // is `Reload()` instead.
    void EnsureLoaded();

    // Phase 370C: reload trigger. Re-reads the manifest and rebuilds
    // a fresh immutable snapshot, then atomically swaps it in. In-
    // flight reads that obtained a handle BEFORE the swap keep
    // their old bytes alive until they release the handle. Emits
    // `Asset:PixelOverridesReloaded:<count>` regardless of whether
    // the count changed (consumers can distinguish "reload accepted"
    // from "no change" by tracking previous count).
    void Reload();

    // Phase 369A pixel-override lookup. Returns a handle whose
    // `data` member is non-null iff the picture name has an
    // override. The caller must keep the handle alive for the
    // duration of any read from the returned bytes. The handle's
    // `data` pointer is stable for as long as the handle lives;
    // after destruction the underlying snapshot may be freed.
    //
    // The `pictureName` is the bare name retail SU stores in
    // `pictureData->name` (offset +2 of the guest pointer points at
    // the C string after the 2-byte length prefix).
    PixelOverrideHandle TryGetPixelOverride(std::string_view pictureName);

    // Phase 369A proof emit. Called by the MakePictureData hook the
    // first time a given pictureName receives a pixel override.
    // Bounded volume = at most one event per unique pictureName per
    // process boot. The dedup set is NOT reset on `Reload()` -- a
    // hot-reload that re-binds the same picture name does not emit
    // a fresh hit; consumers infer "still active" from the absence
    // of a teardown event.
    //
    // Emits one bridge event:
    //   screen_entered: "Asset:PixelOverrideHit:<pictureName>"
    void NoteHitForPicture(std::string_view pictureName);
}
