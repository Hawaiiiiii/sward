<p align="right">
    <img src="../../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Runtime Reference

This directory contains the Phase 21/24/27/37/38 reusable runtime and port-kit layer for the SWARD template-pack concepts.

It is intentionally decoupled from game assets and from the asset-backed Unleashed Recompiled runtime. The goal is to provide reusable implementation layers for original projects that need:

- explicit screen states
- input lock windows
- overlay-role visibility
- prompt-row visibility rules
- timer-banded transitions
- portable reference profiles across C++, C, and C#
- portable JSON contract files instead of hardcoded in-code builders
- a standalone contract-backed debug selector for the first reusable screen-browser pass
- a source-family alias layer so the selector can launch by recovered names such as `TitleMenu.cpp`, `HudPause.cpp`, and `InspirePreview.cpp`
- a richer host-bucket debug workbench around recovered debug/menu/cutscene source families

Contents:

- `include/sward/ui_runtime/runtime.hpp`
- `include/sward/ui_runtime/profiles.hpp`
- `include/sward/ui_runtime/contract_loader.hpp`
- `include/sward/ui_runtime/runtime_c.h`
- `contracts/`
- `src/runtime.cpp`
- `src/contract_loader.cpp`
- `src/profiles.cpp`
- `src/runtime_c.cpp`
- `examples/pause_menu_example.cpp`
- `examples/title_menu_example.cpp`
- `examples/toast_overlay_example.cpp`
- `examples/c_pause_menu_example.c`
- `CMakeLists.txt`
- `csharp_reference/`

Bundled contract files:

- `contracts/pause_menu_reference.json`
- `contracts/title_menu_reference.json`
- `contracts/autosave_toast_reference.json`
- `contracts/loading_transition_reference.json`
- `contracts/mission_result_reference.json`
- `contracts/subtitle_cutscene_reference.json`
- `contracts/world_map_reference.json`

Bundled reference profiles:

- `PauseMenu`
- `TitleMenu`
- `AutosaveToast`
- `LoadingTransition`
- `MissionResult`
- `SubtitleCutscene`
- `WorldMap`

Build the native layer locally:

```powershell
C:\Program Files\CMake\bin\cmake.exe -S research_uiux/runtime_reference -B b/rr38 -G "Visual Studio 17 2022" -A x64
C:\Program Files\CMake\bin\cmake.exe --build b/rr38 --config Release
b/rr38/Release/sward_ui_runtime_example.exe
b/rr38/Release/sward_ui_runtime_title_menu_example.exe
b/rr38/Release/sward_ui_runtime_toast_example.exe
b/rr38/Release/sward_ui_runtime_c_example.exe
b/rr38/Release/sward_ui_runtime_debug_selector.exe
b/rr38/Release/sward_ui_runtime_debug_workbench.exe
```

Run against the bundled contracts:

```powershell
b/rr38/Release/sward_ui_runtime_example.exe
b/rr38/Release/sward_ui_runtime_title_menu_example.exe
b/rr38/Release/sward_ui_runtime_toast_example.exe
b/rr38/Release/sward_ui_runtime_c_example.exe
b/rr38/Release/sward_ui_runtime_debug_selector.exe --list
b/rr38/Release/sward_ui_runtime_debug_selector.exe --list-families
b/rr38/Release/sward_ui_runtime_debug_selector.exe TitleMenu.cpp
b/rr38/Release/sward_ui_runtime_debug_selector.exe HudPause.cpp
b/rr38/Release/sward_ui_runtime_debug_selector.exe InspirePreview.cpp
b/rr38/Release/sward_ui_runtime_debug_workbench.exe --list-groups
b/rr38/Release/sward_ui_runtime_debug_workbench.exe --host GameModeMenuSelectDebug.cpp
b/rr38/Release/sward_ui_runtime_debug_workbench.exe --host InspirePreview.cpp
```

Run against an explicit portable contract path:

```powershell
b/rr38/Release/sward_ui_runtime_example.exe research_uiux/runtime_reference/contracts/world_map_reference.json
b/rr38/Release/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/mission_result_reference.json
b/rr38/Release/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/subtitle_cutscene_reference.json
```

Build the managed port locally:

```powershell
external_tools/dotnet8/dotnet.exe build research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release
external_tools/dotnet8/dotnet.exe run --project research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release
```

Selector notes:

- `--list` prints all bundled contract-backed screens
- `--list-families` prints the generated source-family launch groups
- `--index <n>` selects by 1-based slot
- `<token>` matches either a source-family alias or a bundled contract token
- `--family <token>` forces a source-family lookup
- `--path <contract.json>` loads an explicit portable contract file

Workbench notes:

- `--list-groups` prints the richer host-bucket groups derived from the frontend shell/debug recovery layer
- `--group <token>` lists the launchable hosts inside one workbench group
- `--host <token>` launches by recovered host name or source path such as `GameModeMenuSelectDebug.cpp` or `InspirePreview.cpp`
- the current workbench is intentionally centered on menu-debug, stage-debug, and cutscene-preview ownership; gameplay HUD host buckets are the next expansion target
