<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Local Named Translated Ownership

Phase 36 starts replacing local-only `*.sward.md` anchors with real readable source scaffolds for the first debug/cutscene host surfaces.

> [!IMPORTANT]
> These local-only files are still SWARD humanization scaffolds, not a claim of recovered original authored SEGA source. They live under `SONIC UNLEASHED/` on purpose and remain out of git.

## Snapshot

- Immediate host targets considered: `15`
- Local-only readable source files materialized: `13`
- Immediate-host conversion rate: `86.7%`
- Representative subtitle scene rows embedded across movie/preview hosts: `9`

## Local Output Layer

- output root: `C:\Users\DavidErikGarciaArena\Documents\UI-UX Sonic World Adventure for SGFX - Project Quality Hero\SONIC UNLEASHED`
- file shape: `<original source path>.cpp` beside the existing `*.sward.md` notes
- local meta manifest: `SONIC UNLEASHED/_meta/humanized_debug_host_sources_manifest.json`

## Materialized Groups

| Group | Local-only files | Purpose |
|---|---:|---|
| Menu / Stage Debug Hosts | `3` | Readable local-only launch ownership for title/menu/stage debug entry points. |
| Cutscene / Preview Hosts | `5` | Readable local-only preview browser and menu scaffolds around InspirePreview and InspirePreview2nd. |
| Movie / Sequence Route Hosts | `5` | Readable local-only descriptors for movie ownership, stage-movie handoff, and sequence wrappers. |

## Example Local Files

- `SonicWorldAdventure/SWA/source/System/GameMode/GameModeMainMenu_Test.cpp`
- `SonicWorldAdventure/SWA/source/System/GameMode/GameModeMenuSelectDebug.cpp`
- `SonicWorldAdventure/SWA/source/System/GameMode/GameModeStageSelectDebug.cpp`
- `SonicWorldAdventure/SWA/source/Tool/InspirePreview/InspirePreview.cpp`
- `SonicWorldAdventure/SWA/source/Tool/InspirePreview/InspirePreviewMenu.cpp`
- `SonicWorldAdventure/SWA/source/Tool/InspirePreview/InspireObject.cpp`
- `SonicWorldAdventure/SWA/source/Tool/InspirePreview2nd/InspirePreview2nd.cpp`
- `SonicWorldAdventure/SWA/source/Tool/InspirePreview2nd/InspirePreview2ndMenu.cpp`
- `SonicWorldAdventure/SWA/source/System/GameMode/GameModeStageMovie.cpp`
- `SonicWorldAdventure/SWA/source/Movie/MovieManager.cpp`
- `SonicWorldAdventure/SWA/source/Sequence/Unit/SequenceUnitPlayMovie.cpp`
- `SonicWorldAdventure/SWA/source/Sequence/Unit/SequenceUnitMicroSequence.cpp`

## What Changed

- `GameModeMenuSelectDebug.cpp` and `GameModeStageSelectDebug.cpp` now have readable local-only host tables that point at the then-current contract-backed screen families plus the new subtitle/cutscene contract.
- `InspirePreview*.cpp`, `MovieManager.cpp`, `GameModeStageMovie.cpp`, and the `SequenceUnitPlayMovie` wrapper layer now carry readable local-only scene/route descriptors derived from the subtitle/cutscene presentation evidence.
- The mirror is no longer only notes for these hosts; it now has real `.cpp` scaffolds that can be tightened further as translated seam naming improves.
