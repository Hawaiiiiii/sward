<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Local Source-Family Placement

Phase 32 starts the actual placement step that the previous source-path reports kept pointing at, and the later broader-manifest pass widens it: the local-only `SONIC UNLEASHED/` scaffold now carries per-path SWARD note files instead of only empty folders.

> [!IMPORTANT]
> These placements are local-only on purpose. They are research-side anchors for future translated cleanup and debug-tool work, not publishable reconstructed source files.

## Snapshot

- Local-only note files created: `220`
- Contract-backed placements in the current wider manifest: `42`
- Archaeology-mapped placements without runtime contracts: `76`
- Debug-host candidates: `60`
- Named-only placeholders still waiting on stronger recovery: `42`
- Family-member anchors with recovered system context: `105`
- Debug-host anchors: `60`
- Placeholder-only anchors: `42`
- Direct original-source host anchors: `0` in the broader pass, because the readable host layer still points more often at patch/runtime ownership than at recovered original source-family files

## Output Shape

The local-only mirror now contains:

- `SONIC UNLEASHED/SonicWorldAdventure/SWA/source/.../*.sward.md`
- `SONIC UNLEASHED/_meta/source_family_note_manifest.json`
- a refreshed `SONIC UNLEASHED/_meta/README.txt`

The `*.sward.md` suffix is deliberate:

- it keeps the original 2008-style source-family path visible
- it avoids pretending the translated findings are already clean `.cpp` rewrites
- it gives future cleanup work a stable landing zone beside the intended source-family file

## What Each Local Note Carries

Each local note currently records:

- the original source path from the executable path dump
- the normalized source-family path
- the recovered family name and current status
- the current humanization priority
- matched archaeology systems when available
- runtime-contract links when available
- layout IDs, state tags, generated seams, and host-path bridges for recovered systems

## Strong Examples

- `System/GameMode/GameModeBoot.cpp`
  - contract-backed
  - now resolves into the `Loading And Start/Clear` selector family
  - gives the broader pass a real boot/start alias instead of only the older `Loading.cpp` seed
- `HUD/Pause/HudPause.cpp`
  - contract-backed
  - already sits on the `Pause Stack`
- `System/GameMode/Ending/EndingManager.cpp`
  - contract-backed
  - now resolves into the `Save And Ending` family alongside autosave/clear-flag sequencing
- `System/GameMode/WorldMap/WorldMapSelect.cpp`
  - contract-backed
  - already sits on the `World Map Stack`
- `Sequence/Unit/SequenceUnitCallHelpWindow.cpp`
  - contract-backed
  - now resolves into the `Pause Stack` dispatch layer through `pause_menu_reference.json`
- `Sequence/Unit/SequenceUnitChangeStage.cpp`
  - contract-backed
  - now resolves into the `Loading And Start/Clear` dispatch layer through `loading_transition_reference.json`
- `System/GameMode/GameModeMenuSelectDebug.cpp`
  - debug-host candidate
  - now has a concrete local placement note inside a much larger debug/tool host layer even before a richer debug sandbox is built

## Why This Matters

This is the first beat where the humanization work stops living only in tracked reports.

The mirror now behaves like a staging tree for:

- future translated-file renaming
- per-source-family seam cleanup
- source-path-backed selector/debug entry points
- gradual movement from `sub_XXXXXXXX` clusters toward stable source-family ownership

## Remaining Gap

- these notes are still notes, not cleaned translated `.cpp` replacements
- subtitle/cutscene, sequence-shell, camera-shell, and system-shell surfaces still need stronger recovered ownership
- the broader pass widened the mirror successfully, but much of that new surface is still shell-level rather than semantically humanized

The next two beats stay the same:

1. keep converting shell-level placements into named translated ownership under the same source-family paths
2. keep widening the debug selector and runtime contracts beyond the current six contract-backed families
