<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Host Readiness Badges

Phase 67 makes visual parity visible before a host is launched:

```text
b/rr67/sward_ui_runtime_debug_gui.exe
```

Phase 66 added the detail-pane `Visual parity:` block after running a host. This beat moves the same readiness signal into the host browser so the operator can pick targets by exact/proxy/layout/primitive/channel state directly from the list.

## What Changed

- added `hostVisualReadinessBadge(...)`
- added `hostDisplayLabel(...)`
- changed the native host listbox to show compact readiness badges
- added `--host-readiness-smoke`

## Badge Examples

```text
SonicMainDisplay.cpp [proxy primitive channels]
GameModeMainMenu_Test.cpp [exact layout primitive channels]
AchievementManager.cpp [contract]
```

The badge tokens mean:

| Token | Meaning |
|---|---|
| `exact` | the host contract has an exact local atlas candidate |
| `proxy` | the host contract has a marked proxy atlas candidate |
| `layout` | decoded layout evidence is bound |
| `primitive` | diagnostic scene primitives are bound |
| `channels` | recovered primitive channel classes are present |
| `contract` | contract-backed behavior exists, but no visual atlas/layout/primitive evidence is bound yet |

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr67 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr67 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr67\sward_ui_runtime_debug_gui.exe --host-readiness-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui host readiness smoke ok sonic_label=SonicMainDisplay.cpp [proxy primitive channels] title_label=GameModeMainMenu_Test.cpp [exact layout primitive channels] support_label=AchievementManager.cpp [contract]
```

The GUI was also control-checked by reading the native host listbox through `LB_GETTEXT`, confirming the visible labels for `SonicMainDisplay.cpp` and `GameModeMainMenu_Test.cpp`.

## Boundary

These badges are triage metadata. They do not make a proxy atlas exact, and they do not imply full original CSD playback. They make the next renderer work easier to steer by showing which hosts already have exact/proxy/layout/primitive/channel evidence at browse time.
