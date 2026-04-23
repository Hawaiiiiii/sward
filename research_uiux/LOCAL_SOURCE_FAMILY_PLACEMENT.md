<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Local Source-Family Placement

Phase 32 starts the actual placement step that the previous source-path reports kept pointing at: the local-only `SONIC UNLEASHED/` scaffold now carries per-path SWARD note files instead of only empty folders.

> [!IMPORTANT]
> These placements are local-only on purpose. They are research-side anchors for future translated cleanup and debug-tool work, not publishable reconstructed source files.

## Snapshot

- Local-only note files created: `108`
- Contract-backed placements: `37`
- Archaeology-mapped placements without runtime contracts: `53`
- Debug-host candidates: `13`
- Named-only placeholders still waiting on stronger recovery: `5`
- Direct host anchors: `13`
- Family-member anchors: `77`
- Debug-host anchors: `13`
- Placeholder-only anchors: `5`

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

- `System/GameMode/Title/TitleMenu.cpp`
  - contract-backed
  - bridges to `Title Menu` and `Loading And Start/Clear`
  - already points at `ui_mainmenu`, `ui_loading`, `ui_start`, and the strongest title/loading seam set
- `HUD/Pause/HudPause.cpp`
  - contract-backed
  - already sits on the `Pause Stack`
- `System/GameMode/WorldMap/WorldMapSelect.cpp`
  - contract-backed
  - already sits on the `World Map Stack`
- `CSD/CsdProject.cpp`
  - archaeology-mapped
  - now lands in the `CSD / UI Foundation` layer instead of a generic named-only bucket
- `System/GameMode/GameModeMenuSelectDebug.cpp`
  - debug-host candidate
  - now has a concrete local placement note even before a richer debug sandbox is built

## Why This Matters

This is the first beat where the humanization work stops living only in tracked reports.

The mirror now behaves like a staging tree for:

- future translated-file renaming
- per-source-family seam cleanup
- source-path-backed selector/debug entry points
- gradual movement from `sub_XXXXXXXX` clusters toward stable source-family ownership

## Remaining Gap

- these notes are still notes, not cleaned translated `.cpp` replacements
- subtitle/cutscene and debug/tool surfaces still need stronger runtime/debug bridges
- the current placement set is still limited by the older `108`-path UI-centric seed

The next two beats stay the same:

1. expand the debug selector from contract names into source-path-named families
2. widen the seed beyond the original `108` UI-centric paths so the mirror can grow with a broader source-family map
