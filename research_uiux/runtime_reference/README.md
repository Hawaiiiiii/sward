<p align="right">
    <img src="../../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Runtime Reference

This directory contains the Phase 21/24 reusable runtime and port-kit layer for the SWARD template-pack concepts.

It is intentionally decoupled from game assets and from the asset-backed Unleashed Recompiled runtime. The goal is to provide reusable implementation layers for original projects that need:

- explicit screen states
- input lock windows
- overlay-role visibility
- prompt-row visibility rules
- timer-banded transitions
- portable reference profiles across C++, C, and C#

Contents:

- `include/sward/ui_runtime/runtime.hpp`
- `include/sward/ui_runtime/profiles.hpp`
- `include/sward/ui_runtime/runtime_c.h`
- `src/runtime.cpp`
- `src/profiles.cpp`
- `src/runtime_c.cpp`
- `examples/pause_menu_example.cpp`
- `examples/title_menu_example.cpp`
- `examples/toast_overlay_example.cpp`
- `examples/c_pause_menu_example.c`
- `CMakeLists.txt`
- `csharp_reference/`

Build the native layer locally:

```powershell
C:\Program Files\CMake\bin\cmake.exe -S research_uiux/runtime_reference -B b/rr24 -G "Visual Studio 17 2022" -A x64
C:\Program Files\CMake\bin\cmake.exe --build b/rr24 --config Release
b/rr24/Release/sward_ui_runtime_example.exe
b/rr24/Release/sward_ui_runtime_title_menu_example.exe
b/rr24/Release/sward_ui_runtime_toast_example.exe
b/rr24/Release/sward_ui_runtime_c_example.exe
```

Build the managed port locally:

```powershell
external_tools/dotnet8/dotnet.exe build research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release
external_tools/dotnet8/dotnet.exe run --project research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release
```
