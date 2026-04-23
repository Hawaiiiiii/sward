<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Frontend Sequence Shell Runtime Bridge

This phase closes the last `named_seed_only` gap inside the current `220`-path UI-adjacent source-path seed by turning the generic sequence-core shell into a first-class runtime/debug family.

> [!IMPORTANT]
> This does not mean the whole game is now humanized. It means the current tracked seed no longer has “name only, no recovered bridge” entries.

## What Landed

- Added the bundled contract [runtime_reference/contracts/frontend_sequence_shell_reference.json](./runtime_reference/contracts/frontend_sequence_shell_reference.json).
- Extended the native C++ runtime, the C ABI, and the C# managed reference layer with `FrontendSequenceShell`.
- Refreshed the source-path manifest, selector metadata, and workbench metadata so:
  - `Sequence/Core/SequenceHandleUnit.cpp`
  - `Sequence/Core/SequenceManagerImpl.cpp`
  - `Sequence/Unit/SequenceUnitFactory.cpp`
  - `Sequence/Unit/SequenceUnitUnlockAchievement.cpp`
  are no longer `named_seed_only`.
- Folded `HUD/Item/HudItemGet.cpp` into the gameplay-HUD bridge so the current seed no longer leaves that in-stage overlay floating outside the contract-backed stage HUD family set.

## Measured State

- Broader UI-adjacent source-path seed mapped into archaeology: `163 / 220` (`74.1%`)
- Broader source-path seed backed by runtime contracts: `154 / 220` (`70.0%`)
- `named_seed_only` entries in the current tracked seed: `0`
- Source-family selector launch families: `16`
- Debug workbench hosts: `133`
- Debug workbench groups: `10`

## Verified

- `b/rr46/sward_ui_runtime_debug_selector.exe --list-families`
- `b/rr46/sward_ui_runtime_debug_selector.exe SequenceManagerImpl.cpp`
- `b/rr46/sward_ui_runtime_debug_workbench.exe --list-groups`
- `b/rr46/sward_ui_runtime_debug_workbench.exe --host SequenceManagerImpl.cpp`
- `b/rr46/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/frontend_sequence_shell_reference.json`
- `external_tools/dotnet8/dotnet.exe build research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release`

## What This Changes

- The selector and workbench can now speak a recovered sequence-shell family instead of treating those paths as ungrounded names.
- The current `220`-path seed has moved from “some entries are only named” to “every entry is either contract-backed, archaeology-mapped, or intentionally debug/tool-facing.”
- The next bottleneck is no longer seed hygiene inside this tracked subset. It is widening beyond the current subset and replacing more of the local-only mirror with readable translated ownership.
