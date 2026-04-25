<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Visual Parity Summary

Phase 66 adds a compact host-level parity readout to the native GUI detail pane:

```text
b/rr66/sward_ui_runtime_debug_gui.exe
```

The workbench already exposed local atlas bindings, layout evidence, primitive overlays, animation/frame cues, channel tags, and channel legends. This beat folds the most important readiness facts into one operator section so each selected host answers: exact or proxy atlas, decoded layout evidence or none, primitive coverage, keyframe weight, and recovered primitive channel distribution.

## What Changed

- added `VisualParitySummary`
- added `visualParitySummaryForContract(...)`
- added `visualParitySummaryText(...)`
- added a `Visual parity:` block to the detail pane when a host is running
- added `--visual-parity-smoke` for exact/proxy parity checks without opening the window

## Current Readiness Snapshot

Representative smoke-guarded comparison:

| Host family | Atlas | Layout evidence | Primitives | Channel token |
|---|---|---|---:|---|
| `SonicMainDisplay.cpp` / Sonic Stage HUD | `proxy` | `none` | `6` | `T3 C4 V2 S0 static1` |
| Title menu / `ui_mainmenu` | `exact` | `ui_mainmenu` | `6` | covered by the same primitive channel aggregation path |

This keeps the Sonic HUD boundary honest: it has useful proxy primitive/channel evidence, but not exact loose HUD payload binding yet.

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr66 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr66 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr66\sward_ui_runtime_debug_gui.exe --visual-parity-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui visual parity smoke ok sonic_atlas=proxy sonic_layout=none sonic_primitives=6 sonic_channels=T3 C4 V2 S0 static1 title_atlas=exact title_layout=ui_mainmenu title_primitives=6
```

The GUI was also control-checked by selecting `Gameplay HUD Hosts -> SonicMainDisplay.cpp`, pressing `Run Host`, and reading the native detail edit controls back through `WM_GETTEXT`. The detail pane contained the expected `Visual parity:` block and proxy-boundary note.

## Boundary

This is a parity diagnostic, not a renderer. It does not make proxy HUD assets exact, and it does not play original CSD curves. Its value is that the operator can now see the next renderer blockers per host without mentally joining atlas, layout, primitive, and channel sections by hand.
