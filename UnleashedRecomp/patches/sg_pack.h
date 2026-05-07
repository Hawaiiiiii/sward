#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace SGPack
{
    // Phase 370B: shared `sgfx_pack.json` index.
    //
    // When the pack file is present in `<SG_PREFLIGHT_OVERRIDE_DIR>/sgfx_pack.json`,
    // it becomes the single source of truth for the SGFX shell's
    // override lanes. The text/asset loaders consult this loader's
    // `TryGet*Path` accessors and use pack-pointed paths instead of
    // their default flat-file names. The mod loader's loose-file
    // indexer consults `GetLooseFiles()` and scopes its index to
    // exactly those entries when the pack declares them.
    //
    // When the pack is absent, the existing flat-file behavior is
    // preserved verbatim:
    //   - `sg_text_overrides.json`  loaded by SGTextOverrides
    //   - `sg_asset_overrides.json` loaded by SGAssetOverrides
    //   - all override-dir files auto-discovered as loose overrides
    //
    // Schema (Phase 370B fields; Phase 370A `branding` continues to
    // be read by SGBranding regardless of whether other lane fields
    // are present):
    //
    //   {
    //     "version": 1,
    //     "name":        "...",
    //     "description": "...",
    //     "branding":        { ... },                  // Phase 370A
    //     "text_overrides":  "text/sgfx_text.json",   // optional
    //     "asset_overrides": "pictures/sgfx_pictures.json",
    //     "loose_files":     ["loose/Loading/logo_sonicteam.dds"]
    //   }
    //
    // Path traversal guard: every relative path in the pack is
    // resolved with `std::filesystem::weakly_canonical` and rejected
    // when:
    //   - the input is absolute
    //   - the canonicalised result is not under the canonicalised
    //     override-dir base
    //   - any path component equals ".."
    // Rejected paths emit `Pack:Rejected:<reason>:<path>` bridge
    // events at parse time so the operator can debug a malformed
    // pack from the events.jsonl stream.

    // Idempotent. Reads `sgfx_pack.json` if present, parses it,
    // path-checks every relative path, and caches the resolved
    // absolute paths for the lane accessors. Always emits a single
    // `Pack:Loaded:<text|none>:<asset|none>:<looseCount>` bridge
    // event when a pack is present so consumers know which lanes
    // the pack drives. Calling a second time is a no-op (call_once
    // internally).
    void EnsureLoaded();

    // Phase 370C: post-boot hot reload. Re-reads `sgfx_pack.json`,
    // rebuilds the immutable PackSnapshot, atomically swaps it
    // under the snapshot mutex, and emits
    //   `Pack:Reloaded:<text|none>:<asset|none>:<looseCount>`
    // (always, even when nothing changed). The cascade to the
    // text/asset loaders is the watcher's job -- this entry point
    // only refreshes the pack's own state so subsequent
    // `TryGet*Path()` and `GetLooseFiles()` calls reflect the new
    // manifest.
    void Reload();

    // True when the override dir contains an `sgfx_pack.json` that
    // parsed successfully (regardless of whether any lane fields
    // were present). Text/asset/loose loaders test this BEFORE
    // falling back to flat-file defaults.
    bool IsActive();

    // Absolute path to the text-overrides JSON declared in the
    // pack. Empty when the pack is absent OR the pack does not
    // reference a text manifest OR the path failed traversal
    // guard.
    std::filesystem::path TryGetTextOverridesPath();

    // Absolute path to the asset-overrides JSON declared in the
    // pack. Empty under the same conditions as text-overrides.
    std::filesystem::path TryGetAssetOverridesPath();

    // Absolute paths to the loose files the pack explicitly lists.
    // Each entry has already been traversal-guarded. The mod
    // loader uses this list to scope its loose-file index when
    // the pack is active and `loose_files` is non-empty. When the
    // pack is active but `loose_files` is empty/absent, the mod
    // loader treats that as "no loose substitutions" (NOT
    // "auto-discover everything") -- the pack is authoritative.
    const std::vector<std::filesystem::path>& GetLooseFiles();

    // Helper used by both this loader and SGBranding to resolve a
    // pack-relative path against an override-dir base, with the
    // same traversal guard. Returns true on success and writes the
    // canonical absolute path; returns false (and leaves outPath
    // untouched) when the input is absolute, the path escapes the
    // base, or the file does not exist.
    //
    // Exposed publicly so other lane loaders can adopt the same
    // guard if they grow new optional pack-relative paths later.
    bool ResolveRelativeUnderBase(const std::filesystem::path& base,
                                  const std::filesystem::path& relative,
                                  std::filesystem::path& outPath);
}
