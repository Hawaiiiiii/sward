<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Data-Driven Runtime Contracts

## Summary

Phase 27 removes the last hardcoded-contract bottleneck from the reusable runtime layer.

The C++ runtime, the C ABI wrapper, and the C# reference port can now all consume portable JSON screen contracts instead of depending on in-code profile builders.

## What Landed

- Native JSON contract loader:
  - `runtime_reference/include/sward/ui_runtime/contract_loader.hpp`
  - `runtime_reference/src/contract_loader.cpp`
- Bundled portable contracts:
  - `runtime_reference/contracts/pause_menu_reference.json`
  - `runtime_reference/contracts/title_menu_reference.json`
  - `runtime_reference/contracts/autosave_toast_reference.json`
  - `runtime_reference/contracts/loading_transition_reference.json`
  - `runtime_reference/contracts/mission_result_reference.json`
  - `runtime_reference/contracts/world_map_reference.json`
- C ABI path-loading entry point:
  - `sward_ui_runtime_create_contract_path(...)`
- C# shared contract loader:
  - `runtime_reference/csharp_reference/ContractLoader.cs`
  - `runtime_reference/csharp_reference/ReferenceProfiles.cs`

## Why This Matters

This changes the reusable runtime layer from:

- hardcoded example contracts
- compile-time profile factories
- language-specific duplication

into:

- shared portable contract files
- one native schema loader
- one managed schema loader
- explicit path-based loading for new screen families

That makes the port-kit layer much closer to the real goal of this workspace: forwarding recovered UI/UX structures into unrelated original projects without dragging game-specific payloads into the implementation.

## Bundled Contract Coverage

The bundled JSON set now covers:

- pause menu
- title menu
- autosave toast
- loading/transition handoff
- mission result shell
- world map stack

The first three keep compatibility with the original Phase 24 reference profiles.

The latter three prove that the runtime can now ingest broader archaeology-derived screen contracts that were not previously compiled into the profile builder layer.

## Native Runtime Behavior

The old `makePauseMenuContract()`, `makeTitleMenuContract()`, and `makeAutosaveToastContract()` functions still exist for compatibility.

They no longer build the contracts inline.

They now load the bundled JSON contracts from `runtime_reference/contracts/`, which keeps the older examples and wrappers usable while making the data portable.

## C ABI Behavior

The C surface now supports two creation paths:

- `sward_ui_runtime_create_profile(...)`
- `sward_ui_runtime_create_contract_path(...)`

That means C consumers can still use the bundled profile IDs, or skip them and load any compatible contract file by path.

## C# Behavior

The managed port now copies the JSON contracts into the output folder and loads them with `System.Text.Json`.

That gives the C# reference layer the same effective contract source as the native runtime instead of maintaining a second hardcoded contract definition set.

## Verification

Verified locally in this workspace:

- native configure/build:
  - `C:\Program Files\CMake\bin\cmake.exe -S research_uiux/runtime_reference -B b/rr27 -G "Visual Studio 17 2022" -A x64`
  - `C:\Program Files\CMake\bin\cmake.exe --build b/rr27 --config Release`
- bundled native runs:
  - `b/rr27/Release/sward_ui_runtime_example.exe`
  - `b/rr27/Release/sward_ui_runtime_title_menu_example.exe`
  - `b/rr27/Release/sward_ui_runtime_toast_example.exe`
  - `b/rr27/Release/sward_ui_runtime_c_example.exe`
- explicit contract-path native runs:
  - `b/rr27/Release/sward_ui_runtime_example.exe research_uiux/runtime_reference/contracts/world_map_reference.json`
  - `b/rr27/Release/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/mission_result_reference.json`
- managed build/run:
  - `external_tools/dotnet8/dotnet.exe build research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release`
  - `external_tools/dotnet8/dotnet.exe run --project research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release`

## Boundary

> [!IMPORTANT]
> These contract files encode reusable state, timing, overlay, and prompt relationships only. They do not embed extracted layouts, textures, translated PPC output, or any other proprietary payload.
