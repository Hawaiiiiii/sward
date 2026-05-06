#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace SGAssetOverrides
{
    // Phase 369A: pixel-level texture override for retail SU's
    // CTexturePicture/MakePictureData lane.
    //
    // Loaded from <SG_PREFLIGHT_OVERRIDE_DIR>/sg_asset_overrides.json:
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
    // file at that path must be a raw `DDS ` magic file -- ddspp
    // parses it as-is at MakePictureData time, after retail SU's
    // resource manager has already decompressed the LZX-wrapped on-
    // disk file. Dimensions / format are arbitrary; ddspp tells the
    // renderer what to allocate. CSD placement / UV / aspect from
    // the calling scene still apply, so visible result depends on
    // whether the override was authored to the slot's intended shape.

    // Idempotent. Loads sg_asset_overrides.json if present and
    // pre-reads each referenced DDS file into a host-side cache so
    // the MakePictureData hot path never blocks on disk I/O. Safe to
    // call multiple times (call_once internally). No-op when the
    // manifest is missing or the override dir is unset.
    void EnsureLoaded();

    // Pixel-override lookup. Returns true and fills `data`/`dataSize`
    // with a pointer to the cached raw DDS bytes when the picture
    // name has an override. Returns false otherwise. The pointer is
    // owned by the loader's cache and remains valid for the lifetime
    // of the process; the caller MUST NOT free it. Cheap: one
    // unordered_map lookup, no allocation, no I/O.
    //
    // The `pictureName` is the bare name retail SU stores in
    // `pictureData->name` (offset +2 of the guest pointer points at
    // the C string after the 2-byte length prefix).
    bool TryGetPixelOverride(std::string_view pictureName,
                             const uint8_t** outData,
                             std::size_t* outDataSize);

    // Phase 369A proof emit. Called by the MakePictureData hook the
    // first time a given pictureName receives a pixel override.
    // Bounded volume = at most one event per unique pictureName per
    // process boot. Cheap (one unordered_set check on the hot path).
    //
    // Emits one bridge event:
    //   screen_entered: "Asset:PixelOverrideHit:<pictureName>"
    //
    // Distinct from `Asset:VisibleOverrideHit` (Phase 367b, fires at
    // the on-disk ResolvePath substitution lane) and from
    // `Asset:OverrideHit` (Phase 359, fires at the archive-entry
    // substitution lane). The pixel-override lane operates AFTER
    // retail SU's resource manager has decompressed the file, so it
    // can substitute textures whose dimensions / format differ from
    // the retail asset.
    void NoteHitForPicture(std::string_view pictureName);
}
