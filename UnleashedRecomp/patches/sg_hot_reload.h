#pragma once

namespace SGHotReload
{
    // Phase 370C: file-watcher driver for hot-reloading SGFX shell
    // override manifests at runtime.
    //
    // Env-gated: starts only when `SG_PREFLIGHT_HOT_RELOAD=1` is in
    // the environment. Off by default so vanilla UR launches don't
    // burn a thread + per-second filesystem stats. The watcher runs
    // a single std::thread that polls `mtime()` on:
    //   - <override>/sgfx_pack.json
    //   - the text manifest path (pack-pointed or
    //     <override>/sg_text_overrides.json)
    //   - the asset manifest path (pack-pointed or
    //     <override>/sg_asset_overrides.json)
    //
    // Polling interval is 500 ms; on detected mtime change the
    // watcher waits 250 ms (debounce) and re-stats the file. Only
    // when the second stat returns the SAME mtime does the watcher
    // dispatch the reload -- this catches editor save patterns that
    // touch the file twice in quick succession (truncate then
    // write) without triggering a torn-file reload.
    //
    // On dispatch, the watcher calls (in order):
    //   - SGPack::Reload()       if sgfx_pack.json changed
    //   - SGTextOverrides::Reload()   if text manifest changed OR
    //                                  pack reload swapped the
    //                                  text path
    //   - SGAssetOverrides::Reload()  same logic for the asset path
    //
    // Pixel hot reload is "reload accepted; visible after scene
    // recreate" -- the new bytes flow through the next
    // MakePictureData call, but already-uploaded GPU textures keep
    // showing the old pixels until the calling CSD project is
    // unloaded and re-instantiated. The handle pattern in
    // SGAssetOverrides::TryGetPixelOverride keeps in-flight reads
    // safe while the swap happens.

    // Idempotent. Reads SG_PREFLIGHT_HOT_RELOAD; if the env is set
    // to "1", spawns the watcher thread. Re-calls are no-ops. Safe
    // to call before or after the override loaders' EnsureLoaded()
    // -- the watcher waits on its own first poll until the loaders
    // have a snapshot to swap.
    void Start();

    // Idempotent. Signals the watcher thread to stop and joins it.
    // Safe to call from any thread except the watcher itself.
    // Called from atexit() to keep the runtime clean on quit.
    void Stop();
}
