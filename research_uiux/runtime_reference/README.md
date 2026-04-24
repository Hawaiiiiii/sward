<p align="right">
    <img src="../../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Runtime Reference

This directory contains the Phase 21/24/27/37/38/39/40/42/43/45/47 reusable runtime and port-kit layer for the SWARD template-pack concepts.

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
- a richer host-bucket debug workbench around recovered debug/menu/cutscene/gameplay-HUD/stage-test/town/camera/application-world source families
- a dedicated frontend-sequence shell family for sequence-core, unit-factory, and unlock/dispatch probing
- a Phase 47 wider source-path support layer with `159` generated workbench hosts, including `30` camera/replay presentation hosts
- interactive selector/workbench loops plus a `--stay-open` mode so the native tools no longer look like GUI crashes when launched directly

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
- `contracts/sonic_stage_hud_reference.json`
- `contracts/werehog_stage_hud_reference.json`
- `contracts/extra_stage_hud_reference.json`
- `contracts/super_sonic_hud_reference.json`
- `contracts/boss_hud_reference.json`
- `contracts/subtitle_cutscene_reference.json`
- `contracts/town_ui_reference.json`
- `contracts/camera_shell_reference.json`
- `contracts/application_world_shell_reference.json`
- `contracts/frontend_sequence_shell_reference.json`
- `contracts/world_map_reference.json`

Bundled reference profiles:

- `PauseMenu`
- `TitleMenu`
- `AutosaveToast`
- `LoadingTransition`
- `MissionResult`
- `SonicStageHud`
- `WerehogStageHud`
- `ExtraStageHud`
- `SuperSonicHud`
- `BossHud`
- `SubtitleCutscene`
- `TownUi`
- `CameraShell`
- `ApplicationWorldShell`
- `FrontendSequenceShell`
- `WorldMap`

Build the native layer locally:

```powershell
cmd /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && "C:\Program Files\CMake\bin\cmake.exe" -S research_uiux/runtime_reference -B b/rr47 -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release && "C:\Program Files\CMake\bin\cmake.exe" --build b/rr47 --config Release'
b/rr47/sward_ui_runtime_example.exe
b/rr47/sward_ui_runtime_title_menu_example.exe
b/rr47/sward_ui_runtime_toast_example.exe
b/rr47/sward_ui_runtime_c_example.exe
b/rr47/sward_ui_runtime_debug_selector.exe
b/rr47/sward_ui_runtime_debug_workbench.exe
```

Run against the bundled contracts:

```powershell
b/rr47/sward_ui_runtime_example.exe
b/rr47/sward_ui_runtime_title_menu_example.exe
b/rr47/sward_ui_runtime_toast_example.exe
b/rr47/sward_ui_runtime_c_example.exe
b/rr47/sward_ui_runtime_debug_selector.exe --list
b/rr47/sward_ui_runtime_debug_selector.exe --list-families
b/rr47/sward_ui_runtime_debug_selector.exe TitleMenu.cpp
b/rr47/sward_ui_runtime_debug_selector.exe TownManager.cpp
b/rr47/sward_ui_runtime_debug_selector.exe FreeCamera.cpp
b/rr47/sward_ui_runtime_debug_selector.exe Player3DBossCamera.cpp
b/rr47/sward_ui_runtime_debug_selector.exe Application.cpp
b/rr47/sward_ui_runtime_debug_selector.exe SequenceManagerImpl.cpp
b/rr47/sward_ui_runtime_debug_selector.exe --stay-open TitleManager.cpp
b/rr47/sward_ui_runtime_debug_workbench.exe --list-groups
b/rr47/sward_ui_runtime_debug_workbench.exe --host GameModeMenuSelectDebug.cpp
b/rr47/sward_ui_runtime_debug_workbench.exe --host InspirePreview.cpp
b/rr47/sward_ui_runtime_debug_workbench.exe --host HudSonicStage.cpp
b/rr47/sward_ui_runtime_debug_workbench.exe --host TownManager.cpp
b/rr47/sward_ui_runtime_debug_workbench.exe --host Player3DBossCamera.cpp
b/rr47/sward_ui_runtime_debug_workbench.exe --host TitleManager.cpp
b/rr47/sward_ui_runtime_debug_workbench.exe --host WorldMapSelect.cpp
b/rr47/sward_ui_runtime_debug_workbench.exe --host SequenceManagerImpl.cpp
```

Run against an explicit portable contract path:

```powershell
b/rr47/sward_ui_runtime_example.exe research_uiux/runtime_reference/contracts/world_map_reference.json
b/rr47/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/mission_result_reference.json
b/rr47/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/subtitle_cutscene_reference.json
b/rr47/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/sonic_stage_hud_reference.json
b/rr47/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/application_world_shell_reference.json
b/rr47/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/frontend_sequence_shell_reference.json
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
- `--stay-open` waits for Enter before exit when you launch a direct one-shot command
- no-argument interactive mode now returns to the menu after a selection instead of exiting immediately

Workbench notes:

- `--list-groups` prints the richer host-bucket groups derived from the frontend shell/debug and gameplay-HUD recovery layers
- `--group <token>` lists the launchable hosts inside one workbench group
- `--host <token>` launches by recovered host name or source path such as `GameModeMenuSelectDebug.cpp` or `InspirePreview.cpp`
- `--stay-open` waits for Enter before exit when you launch a direct host/command
- the current workbench now covers menu-debug, stage-debug, cutscene-preview, gameplay-HUD, boss/final HUD, town/media-room, camera/replay, frontend-sequence, application/world shell, and stage-test ownership
- the Phase 47 workbench map contains `159` host entries across `10` groups
- no-argument interactive mode now returns to the group menu after each launch instead of exiting immediately
