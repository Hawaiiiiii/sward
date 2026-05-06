# Phase 362 -- Bridge hooks wired into UnleashedRecomp source

## What landed in UnleashedRecomp source this session

Edits to `local_build_env/ur103clean/UnleashedRecomp/` (gitignored
locally; tracked patch artefacts in `research_uiux/patches/`):

| File | Change |
|---|---|
| `mod/mod_loader.cpp` | Phase 359 -- SG_PREFLIGHT_OVERRIDE_DIR mount |
| `patches/ui_lab_patches.h` | + `IsGameplaySkipMode()`, `TickBridge()`, `EmitBridge*()` declarations |
| `patches/ui_lab_patches.cpp` | + `IsGameplaySkipMode()` body, SgfxBridge singleton + `bridge_impl` namespace, four emit helpers |
| `patches/CTitleStateMenu_patches.cpp` | + `EmitBridgeMenuAccepted("Title", row)` on accept tap, with cursor->row_id table (new_save / continue / settings / dlc / exit) |
| `patches/CHudPause_patches.cpp` | + `EmitBridgeMenuAccepted("Pause", row)` on Accept status, with continue / options / quit / row_<N> mapping |
| `app.cpp` | + `UiLab::TickBridge()` inside `CApplication::Update` (sub_822C1130) so state.json gets re-read once per frame |

What this delivers: when a rebuilt UnleashedRecomp binary runs with
the bridge daemon active, every Title-menu accept and every
Pause-menu accept becomes a JSONL line in
`<userPath>/sgfx_bridge/events.jsonl`. The daemon's existing
`TITLE_ROW_TO_ACTION` and `PAUSE_ROW_TO_ACTION` mappings dispatch
those to sg-preflight commands. State updates flow back into
`state.json`, and SU sees them via `TickBridge()` once per frame.

## Build (you, ~5 min)

The rebuild is gated on the W: subst that requires admin / explicit
permission. In a PowerShell window of your choice:

```powershell
# 1. Mount the build paths.
subst W: "C:\Users\DavidErikGarciaArena\Downloads\UI-UX Sonic World Adventure for SGFX - Project Quality Hero\local_build_env\ur103clean"

# 2. Incremental build (only modified TUs recompile).
cd W:\b\ui_lab_runtime
ninja UnleashedRecomp
# ~2-5 minutes depending on PCH cache state.

# 3. Copy the fresh exe to sg-preflight's working location.
copy W:\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe `
     "C:\Users\DavidErikGarciaArena\Downloads\sg-preflight\UnleashedRecomp-Windows\UnleashedRecomp.exe"
```

If the build errors, the most likely culprits:

* **Missing `<atomic>`/`<mutex>`/`<once_flag>` header in
  ui_lab_patches.cpp** -- those are already in the existing
  include list (lines 14-31), should be fine.
* **`<sward/ui_runtime/sgfx_bridge.hpp>` not found** -- the patches
  TU's include path needs `research_uiux/runtime_reference/include`
  on it. csd_overlay_patches.cpp already pulls `<sward/...>` headers
  successfully, so this should already be configured. If it errors,
  add to UnleashedRecomp's CMakeLists.txt (or patches CMake target)
  at the SWARD include line.
* **`std::size(kRowIds)` ambiguity** -- if MSVC complains, replace
  with `sizeof(kRowIds) / sizeof(kRowIds[0])`.

## Run (you, ~30 sec)

```powershell
# Start the bridge daemon (a separate terminal works fine):
$bridgeDir = "$env:LOCALAPPDATA\UnleashedRecomp\sgfx_bridge"
& "C:\Users\DavidErikGarciaArena\Downloads\sg-preflight\.venv\Scripts\python.exe" `
   -m sg_preflight bridge-daemon --bridge-dir $bridgeDir

# In your main shell -- env vars + launch:
$env:SG_PREFLIGHT_OVERRIDE_DIR  = "$env:LOCALAPPDATA\UnleashedRecomp\sg_overrides"
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = "1"
$env:SG_PREFLIGHT_LOG_LOADS     = "1"
& "C:\Users\DavidErikGarciaArena\Downloads\sg-preflight\UnleashedRecomp-Windows\UnleashedRecomp.exe"
```

## What you should see

1. UnleashedRecomp boots into retail SU title screen.
2. Console logs:
   * `[SG-Preflight] override mount active: ...` (mount Phase 359)
   * `[SG-Preflight] bridge mounted at ... (events.jsonl + state.json)` (Phase 361)
3. Press Start to enter the title menu.
4. Press Down to highlight CONTINUE (cursor index 1).
5. Press A. UnleashedRecomp accepts; the bridge logs:
   * `events.jsonl` gets a line:
     `{"schema":"sgfx_bridge_event","version":1,"event":"menu_accepted","time_unix_ms":...,"screen":"Title","row_id":"continue"}`
6. Bridge daemon stderr shows:
   * `[sgfx-bridge] event: {...row_id: 'continue'...}`
   * Daemon dispatches to `TITLE_ROW_TO_ACTION["continue"]` = `{"intent": "resume_last_bundle"}`
   * `state.json` updates with the active_bundle field.

If you press Start during gameplay (or on World Map), the Pause
menu opens; Accept on each row emits `screen=Pause, row_id=<continue|options|quit|row_N>`.

## Known gaps (Phase 362 follow-ups)

* **World Map ProfileSelected hook** -- not wired this session.
  The retail `worldMapSimpleInfo` struct's stage_id offset isn't
  yet mined, so we'd be emitting imprecise events. To finish:
  add a hook in `aspect_ratio_patches.cpp` at the
  `CTitleStateWorldMap::Update` point (sub_8258B558) that detects
  the accept-tap and reads stage_id from
  `worldMapSimpleInfo + 0xXX`. Once you identify XX from a runtime
  test (the events log gives you a frame timeline to cross-reference),
  add `UiLab::EmitBridgeProfileSelected(stageName)`.
* **Phase 360 gameplay-skip wiring** -- helper exists, hook
  template at `research_uiux/patches/sg_preflight_gameplay_skip.patch`
  needs one sub_XXXXXXXX from your runtime test. RE recipe in
  the patch comment.
* **Pause menu row-id refinement** -- current logic emits "continue"
  for cursor 0, "options" for count-2, "quit" for count-1, and
  "row_<N>" for middle rows. Once you observe a Stage-context pause
  (4 rows: Continue / Restart / Save Settings / Quit), the
  `row_<N>` middle cases can be remapped to "restart" / etc.

## Build artifact integrity

The session's edits all live in gitignored UnleashedRecomp source.
After rebuild, the resulting `UnleashedRecomp.exe` carries:

* Phase 359 override mount (asset/text swap)
* Phase 360 gameplay-skip env-var helper (no hook wired yet)
* Phase 361 SgfxBridge singleton + Title/Pause event emission
* Tick() polling state.json once per frame

Plain UnleashedRecomp users with no env vars set see no behavior
change -- the bridge is a no-op without `<userPath>/sgfx_bridge/`
existing, the override mount is inactive without the env var, and
the gameplay-skip predicate returns false.

## Reference (the four edits in one place)

Tracked at `research_uiux/patches/`:

* `sg_preflight_override_mount.patch` (Phase 359)
* `sg_preflight_gameplay_skip.patch` (Phase 360 scaffolding)
* `sgfx_bridge_daemon.py` (Phase 361 Python side, verified working)

C++ companions in SWARD:

* `research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_bridge.hpp` (32 smoke assertions, all green)

The Phase 362 hook insertions are direct edits into the gitignored
UnleashedRecomp source. If you rotate the source tree, re-apply by
finding the same insertion points (search for "Phase 361:" / "Phase 362:" comments to spot them all).
