<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Debug Workbench Catalog View

Phase 48 added a compact catalog mode to the native debug workbench so the widened host map is inspectable as a menu surface, not only as raw group and host lists. Phase 50 keeps that catalog current with the new support-substrate runtime contracts.

## What Changed

- Added `--catalog` to `sward_ui_runtime_debug_workbench`.
- The catalog prints:
  - total host count
  - group count
  - each group name, priority, and host count
  - each group’s runtime contract set
  - representative sample hosts
- Added a subprocess regression test for the catalog command.

## Current Catalog Surface

- Total hosts: `176`
- Groups: `11`
- Largest groups:
  - `Application / World Shell Hosts`: `64`
  - `Camera / Replay Hosts`: `30`
  - `Town / Media Room Hosts`: `21`
  - `Support Substrate Hosts`: `17`
  - `Cutscene / Preview Hosts`: `12`

## Why It Matters

The workbench is now easier to inspect from a console launch before diving into a specific host. This matters for the next richer debug executable pass because the operator can see the current topology in one view:

- which host groups exist
- which contract each group drives
- which recovered source-family names are good launch probes
- whether the widened camera/presentation tranche is actually visible at runtime

## Verified

- `python research_uiux/runtime_reference/examples/test_ui_debug_workbench_catalog.py` failed against the pre-catalog `b/rr47` workbench with an unknown-token error.
- `b/rr48/sward_ui_runtime_debug_workbench.exe --catalog`
- `SWARD_UI_DEBUG_WORKBENCH_EXE=b\rr48\sward_ui_runtime_debug_workbench.exe python research_uiux/runtime_reference/examples/test_ui_debug_workbench_catalog.py`
- `b/rr48/sward_ui_runtime_debug_workbench.exe --host Player3DBossCamera.cpp`
- `b/rr50/sward_ui_runtime_debug_workbench.exe --catalog`
- `SWARD_UI_DEBUG_WORKBENCH_EXE=b\rr50\sward_ui_runtime_debug_workbench.exe python research_uiux/runtime_reference/examples/test_ui_debug_workbench_catalog.py`
- `b/rr50/sward_ui_runtime_debug_workbench.exe --host SoundController.cpp`

## Example

```powershell
b/rr50/sward_ui_runtime_debug_workbench.exe --catalog
```

The catalog includes the widened camera/presentation sample:

```text
Camera / Replay Hosts [medium] hosts=30
  contract: camera_shell_reference.json
  sample: Player3DBossCamera.cpp

Support Substrate Hosts [medium] hosts=17
  contract: achievement_unlock_support_reference.json
  contract: audio_cue_support_reference.json
  contract: xml_data_loading_support_reference.json
  sample: SoundController.cpp
```
