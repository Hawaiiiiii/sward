<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Layout Channel Sample Cues

Phase 69 adds the first renderer-facing primitive channel sample cue to the native GUI:

```text
b/rr69/sward_ui_runtime_debug_gui.exe
```

Phase 68 named `decoded CSD channel sampling` as the next blocker for exact-atlas/layout/primitive/channel families. This beat starts that path conservatively: it does not synthesize original curve values yet, but it does bind each recovered primitive to a sampled frame cursor and typed channel token that a later draw-command sampler can replace with real decoded curve values.

## What Changed

- added `layoutPrimitiveChannelSampleToken(...)`
- added `layoutPrimitiveChannelSampleSummary(...)`
- added a `Layout primitive channel samples:` section to the GUI detail pane
- added `--layout-channel-sample-smoke`

## Sample Token Shape

```text
scene:channels@frame/count
```

Representative exact-family samples:

```text
mm_donut_move:color+transform@110/220
stick:color+transform+visibility@120/240
bg_2:color+transform@1/2
```

These tokens are deliberately narrow. They prove that exact-family primitive ownership, channel classification, and runtime frame sampling now meet in one renderer-facing representation.

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr69 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr69 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr69\sward_ui_runtime_debug_gui.exe --layout-channel-sample-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layout channel sample smoke ok title_sample=mm_donut_move:color+transform@110/220 pause_sample=stick:color+transform+visibility@120/240 loading_sample=bg_2:color+transform@1/2
```

## Boundary

This is not original CSD curve evaluation yet. It is the first stable sample-token surface between recovered primitive/channel evidence and future draw-command generation.
