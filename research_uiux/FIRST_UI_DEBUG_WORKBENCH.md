<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> First UI Debug Workbench

Phase 38 adds the first richer native debug executable/menu on top of the recovered host buckets.

> [!IMPORTANT]
> This is the first host-bucket workbench, not the final whole-game debug build. It focuses on recovered menu-debug, stage-debug, and cutscene-preview ownership because those buckets are already measurable and defensible.

## What Landed

- generated host map: `research_uiux/data/debug_workbench_host_map.json`
- generated metadata header: `research_uiux/runtime_reference/include/sward/ui_runtime/debug_workbench_data.hpp`
- native executable: `b/rr38/Release/sward_ui_runtime_debug_workbench.exe`
- source hosts centered around:
  - `GameModeMenuSelectDebug.cpp`
  - `GameModeStageSelectDebug.cpp`
  - `InspirePreview.cpp`
  - `InspirePreview2nd.cpp`

## Workbench Groups

- `Cutscene / Preview Hosts`
- `Menu / Stage Debug Hosts`
- `Pause / Help / Loading Dispatch`
- `Stage Test / Validation Hosts`

## Verified Commands

```powershell
b/rr38/Release/sward_ui_runtime_debug_workbench.exe --list-groups
b/rr38/Release/sward_ui_runtime_debug_workbench.exe --group cutscene_preview_hosts
b/rr38/Release/sward_ui_runtime_debug_workbench.exe --host GameModeMenuSelectDebug.cpp
b/rr38/Release/sward_ui_runtime_debug_workbench.exe --host InspirePreview.cpp
```

Verified behavior:

- host-bucket discovery works
- menu-debug host launch works
- cutscene-preview host launch works
- the workbench resolves host names through the bundled runtime contracts instead of only raw contract tokens

## Why This Matters

- the debug layer is no longer only a contract selector; it now speaks in recovered source-host terms
- the workbench gives the local-only `SONIC UNLEASHED/` tree and the tracked runtime layer a shared landing zone
- it is now practical to widen this executable toward gameplay HUD and broader application/world shells instead of restarting from a generic selector
