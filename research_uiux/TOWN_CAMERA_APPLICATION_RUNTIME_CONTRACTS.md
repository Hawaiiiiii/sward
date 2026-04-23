<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Town, Camera, And Application Runtime Contracts

Phase 42 closes the biggest remaining shell gap in the portable runtime layer: town/media-room flows, camera/replay shells, and the broader application/world shell now have bundled contracts instead of living only as named source-path buckets.

> [!IMPORTANT]
> These contracts are still repo-safe abstractions. They model host/state/timing structure and prompt/layout roles without copying proprietary assets or claiming to be original SEGA-authored source.

## Added Contract Files

- `research_uiux/runtime_reference/contracts/town_ui_reference.json`
- `research_uiux/runtime_reference/contracts/camera_shell_reference.json`
- `research_uiux/runtime_reference/contracts/application_world_shell_reference.json`

## Runtime Surface

- Native C++ bundled profiles now include `TownUi`, `CameraShell`, and `ApplicationWorldShell`.
- The C ABI now exposes:
  - `SWARD_UI_PROFILE_TOWN_UI`
  - `SWARD_UI_PROFILE_CAMERA_SHELL`
  - `SWARD_UI_PROFILE_APPLICATION_WORLD_SHELL`
- The managed C# reference port now mirrors the same three profile IDs and bundled contract-path mapping.

## Source-Path Bridge Impact

After the Phase 42 bridge:

- broader source-path seed: `220`
- archaeology-mapped paths: `158 / 220` (`71.8%`)
- contract-backed paths: `149 / 220` (`67.7%`)
- named-only gaps: `5 / 220` (`2.3%`)

The newly strengthened bridge is most visible in these families:

- `Town / Media Room UI`: now contract-backed through `town_ui_reference.json`
- `Frontend Camera Shell` plus debug/replay camera hosts: now contract-backed through `camera_shell_reference.json`
- `Frontend System Shell`, `GameMode / Frontend Shell`, and title/world-map/save/loading shell spillover: now materially contract-backed through `application_world_shell_reference.json`

## Verified Launches

Verified locally from `b/rr44`:

- `sward_ui_runtime_debug_selector.exe TownManager.cpp`
- `sward_ui_runtime_debug_selector.exe FreeCamera.cpp`
- `sward_ui_runtime_debug_selector.exe Application.cpp`
- `sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/application_world_shell_reference.json`

## What This Unlocks

- the selector can now launch the town/camera/application shell families by recovered source-path names
- the workbench can treat those shells as first-class host buckets rather than archaeology-only metadata
- the local-only `SONIC UNLEASHED/` mirror now has a stronger contract target for readable translated ownership inside those families
