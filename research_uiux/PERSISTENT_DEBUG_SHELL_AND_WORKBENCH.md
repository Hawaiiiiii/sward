<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Persistent Debug Shell And Workbench

Phase 43 upgrades the native selector/workbench from one-shot console probes into usable interactive shell tools.

## What Changed

- `research_uiux/runtime_reference/examples/ui_debug_selector.cpp`
  - interactive mode now loops back to the family menu after a launch
  - `c` switches to raw bundled-contract browsing
  - `q` cleanly exits
  - `--stay-open` pauses before exit for direct one-shot launches
- `research_uiux/runtime_reference/examples/ui_debug_workbench.cpp`
  - interactive mode now loops back to the group menu after a launch
  - `q` cleanly exits from group or host selection
  - `--stay-open` pauses before exit for direct one-shot launches
  - host lookup now prioritizes direct source-path/filename matches before broader alias matches

## Why The Old Behavior Looked Broken

The earlier `.exe` tools were plain CLI probes. They would:

- run one contract/host
- print the state/timeline walk
- exit immediately

That is why launching them like GUI tools looked like a crash, even when the process was actually exiting with code `0`.

## Verified Behavior

Verified locally from `b/rr44`:

- selector:
  - `--list-families` shows `15` launch families
  - interactive selection returns to the family menu after a launch
  - `--stay-open TitleManager.cpp` waits for Enter before exit
- workbench:
  - `--list-groups` shows `9` groups
  - host map now contains `129` launchable hosts
  - interactive selection returns to the group menu after a launch
  - direct host launches verified for:
    - `TownManager.cpp`
    - `ReplayFreeCamera.cpp`
    - `TitleManager.cpp`
    - `WorldMapSelect.cpp`

## Current Group Surface

- `Application / World Shell Hosts`: `64`
- `Boss / Final HUD Hosts`: `3`
- `Camera / Replay Hosts`: `4`
- `Cutscene / Preview Hosts`: `12`
- `Gameplay HUD Hosts`: `9`
- `Menu / Stage Debug Hosts`: `3`
- `Pause / Help / Loading Dispatch`: `4`
- `Stage Test / Validation Hosts`: `9`
- `Town / Media Room Hosts`: `21`

## Practical Answer

Yes, it was normal that the earlier builds felt like they “just crash or exit” when launched directly.

No, that was not a real runtime crash in the verified paths.

After this phase, the tools behave much closer to what you expect from a local debug shell:

- direct commands can stay open
- interactive selection no longer drops you out after one run
- host lookup respects exact recovered source-path names before looser alias matching
