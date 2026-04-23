<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Sequence And Item Source Deepening

This pass pushes the local-only mirrored debug tree further away from note stubs and closer to readable ownership by adding explicit sequence-shell and item-overlay scaffolds under `SONIC UNLEASHED/`.

> [!IMPORTANT]
> The mirrored files remain local-only and intentionally stay out of git. The tracked repo carries the generator, summary JSON, and this report.

## What Deepened

- Added local-only readable sequence scaffolds for:
  - `Sequence/Core/SequenceHandleUnit.cpp`
  - `Sequence/Core/SequenceManagerImpl.cpp`
  - `Sequence/Unit/SequenceUnitFactory.cpp`
  - `Sequence/Unit/SequenceUnitUnlockAchievement.cpp`
  - plus the adjacent change-stage, play-movie, help-window, town-message, media-room-message, and utility wrappers
- Added `HUD/Item/HudItemGet.cpp` to the local-only readable gameplay-HUD layer.
- Regenerated the local-only mirror metadata under `SONIC UNLEASHED/_meta/`.

## Measured State

- Total humanized local-only `.cpp` files under `SONIC UNLEASHED/`: `102`
- Expansion groups in the current materializer pass: `6`
- Frontend sequence local-only scaffolds in this pass: `12`
- Gameplay HUD local-only scaffolds in this pass: `13`

## Why It Matters

- The mirrored tree now contains readable ownership not just for menu, town, camera, and application shells, but also for the sequence-core layer that dispatches frontend units and cutscene/loading/town helpoffs.
- The in-stage `HudItemGet.cpp` overlay now lives beside the gameplay HUD family instead of remaining a dangling path in the source dump.
- The next step is to keep turning more of the debug/tool-facing subset into readable translated ownership until the mirror behaves like a real debug-oriented source base instead of a scaffold farm.
