# Phase 361 -- End-to-end alpha (BMW QA data flowing into SU UI)

> Current note: Phase 366 consolidates this alpha flow into tracked
> `UnleashedRecomp/` source and records the current runtime proof events.
> Treat this file as the operator overview, with
> `PHASE_366_PATH_B_RUNTIME_PROOF.md` as the proof checklist.

## What you have

Three tracked artifacts that together let real sg-preflight QA data
drive Sonic Unleashed's UI screens running under UnleashedRecomp:

| Artifact | Purpose | Where to put it |
|---|---|---|
| [`research_uiux/patches/sg_preflight_override_mount.patch`](patches/sg_preflight_override_mount.patch) | Phase 359 -- asset/text override directory mount in UnleashedRecomp's mod loader | apply to `UnleashedRecomp/mod/mod_loader.cpp` |
| [`research_uiux/patches/sg_preflight_gameplay_skip.patch`](patches/sg_preflight_gameplay_skip.patch) | Phase 360 -- env-var helper + RE recipe to short-circuit action-stage entries | apply 3 blocks across `ui_lab_patches.cpp` / `.h` + new `CGameModeStage_gameplay_skip_patches.cpp` |
| [`research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_bridge.hpp`](runtime_reference/include/sward/ui_runtime/sgfx_bridge.hpp) | Phase 361 -- C++ host-side bridge: reads `state.json`, emits `events.jsonl` | include from any UnleashedRecomp patch that wants to talk to sg-preflight |
| [`research_uiux/patches/sgfx_bridge_daemon.py`](patches/sgfx_bridge_daemon.py) | Phase 361 -- Python daemon: tails `events.jsonl`, dispatches actions, republishes `state.json` | drop at `sg-preflight/sg_preflight/bridge_daemon.py` and wire as `python -m sg_preflight bridge-daemon` |

## The dataflow

```
                 +--- UnleashedRecomp running retail SU ---+
                 |                                         |
   user input -> | UI screens -> SgfxBridge::EmitEvent --> | events.jsonl  -+
                 |                                         |                |
                 |               SgfxBridge::Tick <------- | state.json <-+ |
                 |                            |            |              | |
                 +----------------------------|------------+              | |
                                              v                           | |
                                  drives UI render                        | |
                                                                          | |
                                                                          | |
                 +--- python -m sg_preflight bridge-daemon ----+          | |
                 |                                             |          | |
                 |  tails events.jsonl  -------------------->  | <--------+ |
                 |  selects sg-preflight intent                |            |
                 |  fires existing CLI: launch-action,         |            |
                 |   desktop-state, snapshot, environment      |            |
                 |  collects results into bridge state shape   | --------->-+
                 |  writes state.json atomically               |
                 |                                             |
                 +---------------------------------------------+
```

Two files, two processes, no IPC libraries, no subprocess
management beyond what `sg_preflight` already does for its own
desktop_native shell.

## End-to-end alpha boot procedure

Per the operator's stated alpha posture (SEGA assets fine for now,
dev-machine-only, gathering scope feedback), the boot sequence is:

```powershell
# 1. Build the tracked UnleashedRecomp source, then deploy the fresh
#    executable into the Complete Installation directory.
#
# 2. Drop the bridge daemon next to sg-preflight's package:
#    sg-preflight/sg_preflight/bridge_daemon.py
#
# 3. Wire its CLI subcommand. In sg_preflight/__main__.py add:
#       elif argv[1] == "bridge-daemon":
#           from sg_preflight import bridge_daemon
#           sys.exit(bridge_daemon.main(argv[2:]))
#
# 4. (Once) start the bridge daemon (it republishes state.json
#    atomically so it's safe to run alongside the desktop_native
#    shell -- they don't fight over the same file):
$env:APPDATA  # confirm path
$bridgeDir = "$env:APPDATA\UnleashedRecomp\sgfx_bridge"
Start-Process -NoNewWindow python -ArgumentList "-m sg_preflight bridge-daemon"
#    Daemon prints to stderr; you should see "[sgfx-bridge] daemon up."

# 5. Set the override + skip + (optional) load-log env vars:
$env:SG_PREFLIGHT_OVERRIDE_DIR = "$env:LOCALAPPDATA\UnleashedRecomp\sg_overrides"
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = "1"
$env:SG_PREFLIGHT_LOG_LOADS     = "1"

# 6. Launch UnleashedRecomp normally.
.\UnleashedRecomp.exe
#    Console will log:
#       [SG-Preflight] override mount active: "..." (priority=top)
#       [Mod Loader] Loading file: "..." (per-load)
#       (and on each menu accept, the C++ host appends to events.jsonl)
```

You're now end-to-end. Press Start on the SU title -> daemon sees
`menu_accepted` event with `row_id=continue` -> daemon dispatches
the matching sg-preflight intent -> writes updated state.json ->
SU re-reads on next mtime change.

## What's in the bridge state today

`state.json` (Python -> SU) shape:

```json
{
  "schema": "sgfx_bridge_state",
  "version": 1,
  "updated_at": "2026-05-06T12:34:56Z",
  "active_bundle": {"id": "G05_C01_V07", "name": "BMW G05 carpaint pack", "path": "..."},
  "profiles": [
    {"id": "G05_C01_V07", "label": "G05 C01 V07", "status": "ready"},
    ...
  ],
  "actions": [
    {"id": "anchors_check", "label": "Anchors Check", "available": true},
    ...
  ],
  "last_validation": {
    "passed": false, "blockers": 3, "warnings": 12,
    "evidence_path": "/out/run_2026-05-06_12-34/evidence.html"
  },
  "environment": {"raco_ready": true, "blender_ready": false, "python_ready": true}
}
```

`events.jsonl` (SU -> Python) -- one JSON object per line:

```jsonl
{"schema":"sgfx_bridge_event","version":1,"event":"screen_entered","time_unix_ms":1715000000000,"screen":"WorldMap"}
{"schema":"sgfx_bridge_event","version":1,"event":"menu_accepted","time_unix_ms":1715000005000,"screen":"Title","row_id":"continue"}
{"schema":"sgfx_bridge_event","version":1,"event":"profile_selected","time_unix_ms":1715000010000,"profile_id":"G05_C01_V07"}
{"schema":"sgfx_bridge_event","version":1,"event":"action_requested","time_unix_ms":1715000015000,"action_id":"anchors_check"}
```

## What hooks the C++ host needs to call

Every existing patch can include `<sward/ui_runtime/sgfx_bridge.hpp>`
and use the singleton:

```cpp
// Once at startup (e.g. in app.cpp):
static sward::ui_runtime::generated::sgfx_hud::SgfxBridge g_sgfxBridge;
g_sgfxBridge.Init(GetUserPath() / "sgfx_bridge");

// Each frame in the main loop:
g_sgfxBridge.Tick();   // re-reads state.json if mtime changed

// On title-menu accept (CTitleStateMenu_patches.cpp):
sward::ui_runtime::generated::sgfx_hud::SgfxBridgeEvent ev;
ev.kind = SgfxBridgeEventKind::MenuAccepted;
ev.screen = "Title";
ev.rowId = currentMenuRowId;     // "continue", "new_save", "settings", ...
g_sgfxBridge.EmitEvent(ev);

// On world-map stage selected (CTitleStateWorldMap):
SgfxBridgeEvent ev;
ev.kind = SgfxBridgeEventKind::ProfileSelected;
ev.profileId = retailStageIdString;   // "apotos", "spagonia", etc.
g_sgfxBridge.EmitEvent(ev);
```

Render code reads `g_sgfxBridge.State()` to drive on-screen content
(profile list -> menu rows; last_validation.blockers -> a HUD
counter; environment.* -> a status icon).

## Smoke-test coverage today

The C++ side has 32 assertions in
[`sgfx_state_machines_smoke_test.cpp`](runtime_reference/src/sgfx_state_machines_smoke_test.cpp)::testSgfxBridge:

* Init + dir creation
* state.json mtime polling
* All schema fields (active bundle, profiles array, actions array,
  last_validation, environment)
* Tick idempotence (no-op if mtime unchanged)
* Event emission (3 distinct kinds round-tripped via re-read)
* Schema mismatch is rejected gracefully (state preserved, no clobber)

The Python side parses cleanly under `ast.parse`; full daemon
integration testing requires a real sg-preflight tree.

## What this DOESN'T do (yet, honestly)

* **The override pack itself.** Phase 359 wires the *mount*; the
  actual override .yncp / .dds / text files are art-direction work
  you'll iterate on once the alpha is running and you can A/B
  retail vs override frame-by-frame. The cleanest path is to start
  with one override file (e.g. a custom `ui_title.yncp`) and see
  the console confirm the swap, then expand.
* **Menu row IDs in retail SU.** The bridge expects events like
  `row_id=continue` / `row_id=new_save`. The retail title menu
  internally numbers rows 0..N. The host-side hook in
  CTitleStateMenu_patches needs to translate retail's numeric row
  index into one of the canonical IDs (`continue`, `new_save`,
  `settings`, `dlc`, `exit`). This is a 5-line `switch` you can
  drop into the existing patch file.
* **3D World Map content.** The Earth globe stays as retail since
  that's the alpha demo; a future production pass swaps it for
  a 3D BMW car or a stylized Paradox-Cat-themed shape.
* **Production override pack art.** The runtime path is wired; the
  custom SGFX/BMW replacement art is still an asset-authoring pass.

## Repository layout summary

```
SWARD repo (this tree):
  research_uiux/
    PHASE_359_SG_PREFLIGHT_OVERRIDE_HOWTO.md   # asset overrides
    PHASE_360_*  (none -- scaffolded inside the patch)
    PHASE_361_END_TO_END_ALPHA_HOWTO.md        # this doc
    patches/
      sg_preflight_override_mount.patch        # Phase 359
      sg_preflight_gameplay_skip.patch         # Phase 360 scaffolding
      sgfx_bridge_daemon.py                    # Phase 361 Python side
    runtime_reference/include/sward/ui_runtime/
      sgfx_bridge.hpp                          # Phase 361 C++ side
```

Apply patches into your sg-preflight UnleashedRecomp copy, drop the
Python file into sg_preflight, rebuild once, run.
