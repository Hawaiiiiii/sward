<p align="right">
    <img src="../../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Runtime Reference

This directory contains a generic C++ reference implementation of the SWARD template-pack concepts.

It is intentionally decoupled from game assets and from the asset-backed Unleashed Recompiled runtime. The goal is to provide a reusable implementation layer for original projects that need:

- explicit screen states
- input lock windows
- overlay-role visibility
- prompt-row visibility rules
- timer-banded transitions

Contents:

- `include/sward/ui_runtime/runtime.hpp`
- `src/runtime.cpp`
- `examples/pause_menu_example.cpp`
- `CMakeLists.txt`

Build locally:

```powershell
cmake -S research_uiux/runtime_reference -B b/rr
cmake --build b/rr --config Release
b/rr/Release/sward_ui_runtime_example.exe
```
