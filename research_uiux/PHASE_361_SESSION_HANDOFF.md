# Phase 361 -- Session handoff (what's done, what you finish)

## Verified working tonight

| Piece | State | Verified how |
|---|---|---|
| **Phase 359** asset override mount in UnleashedRecomp `mod_loader.cpp` | source-edited | applied in `local_build_env/.../mod_loader.cpp` (file diff) |
| **Phase 360** `UiLab::IsGameplaySkipMode()` helper + declaration | source-edited | applied in `ui_lab_patches.cpp` + `.h` (file diff) |
| **Phase 361** `sgfx_bridge.hpp` C++ host class | smoke-tested | 32 assertions in `sgfx_state_machines_smoke_test.cpp::testSgfxBridge`, all green |
| **Phase 361** `bridge_daemon.py` Python daemon | **live end-to-end tested** | dropped at `sg-preflight/sg_preflight/bridge_daemon.py`; `sg-preflight bridge-daemon --help` parses; daemon launched, real BMW G70/G65/G45/G50 profile data flowed into `state.json`; profile-selected event accepted via `events.jsonl`, dispatched, active_bundle changed |
| **Phase 361** CLI subcommand wired | working | `sg-preflight bridge-daemon` available in cli.py with `--bridge-dir`, `--tick-seconds`, `--quiet` |

## Live test transcript (excerpt)

```
=== INITIAL STATE (after 10s startup) ===
{
  "schema": "sgfx_bridge_state", "version": 1,
  "active_bundle": {"id": "G70", "name": "BMW G70 live slice", "path": ""},
  "profiles": [
    {"id": "G70", "label": "BMW G70 live slice", "status": "blocked"},
    {"id": "G65", "label": "BMW G65 live slice", "status": "blocked"},
    {"id": "G45", "label": "BMW G45 classic slice", "status": "blocked"},
    {"id": "G50", "label": "BMW G50 live slice", "status": "blocked"}
  ],
  ...
}

=== inject profile_selected event for G45 ===
=== STATE AFTER PROFILE SWITCH (3s later) ===
  "active_bundle": {"id": "G45", "name": "G45", "path": ""}
```

The Python side is **end-to-end verified against your real sg-preflight
desktop-state output**. The "blocked" status is derived from each
profile's summary string ("X errors, Y warnings"). Active bundle
swaps on `profile_selected` events. The bridge is ready to consume
events from any C++ host that emits the JSONL schema.

## What's gated on you (final 3 steps)

**Step A -- mount the W: subst** (one-time, ~3 sec):

```powershell
subst W: "C:\Users\DavidErikGarciaArena\Downloads\UI-UX Sonic World Adventure for SGFX - Project Quality Hero\local_build_env\ur103clean"
```

I couldn't run this myself (system-level subst is blocked at the
permission layer, correct guardrail). After this, the existing
CMake build cache resolves.

**Step B -- incremental rebuild** (probably 2-5 min for our small
delta of one .cpp + one .h):

```powershell
cd W:\b\ui_lab_runtime
ninja
```

Only `mod_loader.cpp` and `ui_lab_patches.cpp` changed; ninja will
rebuild those two TUs and relink. If it errors out, the most likely
cause is that `<algorithm>` / `<cstdlib>` aren't both already in
the precompiled header for the patches TU -- in that case add
`#include <cstdlib>` (for `std::getenv`) and `#include <string_view>`
(if not already there) at the top of `ui_lab_patches.cpp`.

**Step C -- copy fresh exe + start daemon + launch** (~30 sec):

```powershell
# Copy fresh exe over the older one in sg-preflight
copy W:\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe `
     "C:\Users\DavidErikGarciaArena\Downloads\sg-preflight\UnleashedRecomp-Windows\UnleashedRecomp.exe"

# Start the bridge daemon in a separate terminal (it logs to stderr)
$bridgeDir = "$env:LOCALAPPDATA\UnleashedRecomp\sgfx_bridge"
Start-Process -NoNewWindow `
    -FilePath "C:\Users\DavidErikGarciaArena\Downloads\sg-preflight\.venv\Scripts\python.exe" `
    -ArgumentList "-m sg_preflight bridge-daemon --bridge-dir `"$bridgeDir`""

# Set the override mount + gameplay-skip + bridge env vars
$env:SG_PREFLIGHT_OVERRIDE_DIR  = "$env:LOCALAPPDATA\UnleashedRecomp\sg_overrides"
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = "1"
$env:SG_PREFLIGHT_LOG_LOADS     = "1"

# Launch UnleashedRecomp from sg-preflight's copy
& "C:\Users\DavidErikGarciaArena\Downloads\sg-preflight\UnleashedRecomp-Windows\UnleashedRecomp.exe"
```

(The override dir doesn't need to exist yet; the mount auto-detects.
It'll just be inactive until you drop your first override file.)

## What still needs RE work (Phase 360 follow-up)

`UiLab::IsGameplaySkipMode()` exists but is wired to nothing. The
hook template at [`research_uiux/patches/sg_preflight_gameplay_skip.patch`](patches/sg_preflight_gameplay_skip.patch)
needs ONE sub_XXXXXXXX identified from your runtime test pass.
Recipe:

1. After the rebuild lands, with `SG_PREFLIGHT_GAMEPLAY_SKIP=0` and
   `SG_PREFLIGHT_LOG_LOADS=1` set, navigate Title -> WorldMap, pick
   a stage, press A.
2. Capture the console output. Look for the FIRST `sub_82XXXXXX`
   that fires AFTER the World Map "stage selected" SFX cue and
   BEFORE the first stage-asset file load (`win32/game/<StageId>/...`).
3. Drop that address into the `PPC_FUNC_IMPL` / `PPC_FUNC` pair in
   the patch's BLOCK 3, remove the `#if 0` guard, rebuild.

This is 5-10 minutes of your time once the rebuild works.

## What still needs C++ host hook wiring (Phase 361 follow-up)

The bridge C++ class is ready (`sgfx_bridge.hpp`). To actually
emit events from retail SU's UI, you (or I in a follow-up session)
need to add a few hook calls in the existing patches:

* `CTitleStateMenu_patches.cpp` -- on accept, emit `MenuAccepted`
  with the row id translated from retail's row index (5-line switch).
* `CTitleStateWorldMap_patches.cpp` -- on stage select, emit
  `ProfileSelected`.
* `CHudPause_patches.cpp` -- on pause-menu accept, emit
  `MenuAccepted` with `screen="Pause"`.

These are surgical: each is a `g_sgfxBridge.EmitEvent({...})` line
in an existing `PPC_FUNC` patch. Once they're in, the bridge daemon
sees real events and your menu rows are driven by sg-preflight's
state. I can do these in a session where the rebuild is set up.

## Files modified outside SWARD (with your permission grant)

```
C:\Users\DavidErikGarciaArena\Downloads\sg-preflight\
    sg_preflight\
        bridge_daemon.py        # NEW (drop-in)
        cli.py                  # MODIFIED (added bridge-daemon subparser + dispatch)
```

Both edits are isolated additions; existing sg-preflight commands
are untouched. The CLI subcommand uses lazy import so non-bridge
commands aren't slowed down.

## Files modified inside SWARD

```
local_build_env/ur103clean/UnleashedRecomp/
    mod/mod_loader.cpp          # Phase 359 (gitignored locally)
    patches/ui_lab_patches.cpp  # Phase 360 helper (gitignored)
    patches/ui_lab_patches.h    # Phase 360 declaration (gitignored)

research_uiux/                  # tracked
    patches/
        sg_preflight_override_mount.patch    # Phase 359 ref
        sg_preflight_gameplay_skip.patch     # Phase 360 ref
        sgfx_bridge_daemon.py                # Phase 361 Python ref (synced)
    runtime_reference/include/sward/ui_runtime/
        sgfx_bridge.hpp                      # Phase 361 C++ class
    PHASE_361_SESSION_HANDOFF.md             # this doc
```

## Smoke test status

`sgfx_smoke.exe` -- ~300 assertions across all SGFX components,
**0 failures** including the 32 `testSgfxBridge` assertions covering
state parsing, mtime polling, event emission, and graceful schema
mismatch handling.
