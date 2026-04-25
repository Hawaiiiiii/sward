<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Layout Draw Command Descriptors

Phase 70 adds the first renderer-facing draw command descriptor layer to the native GUI:

```text
b/rr70/sward_ui_runtime_debug_gui.exe
```

Phase 69 gave exact-family primitives stable `scene:channels@frame/count` sample tokens. This beat adds the next renderer bridge: recovered normalized primitive rectangles are converted into deterministic 1280x720 command geometry and paired with the sampled channel token.

## What Changed

- added `LayoutPrimitiveDrawCommand`
- added `layoutPrimitiveDrawCommandsForContract(...)`
- added `layoutPrimitiveDrawCommandDescriptor(...)`
- added a `Layout primitive draw commands:` detail-pane section
- added `--layout-draw-command-smoke`

## Descriptor Shape

```text
scene:x,y,widthxheight:channels@frame/count
```

Representative exact-family descriptors:

```text
mm_donut_move:102,202,563x115:color+transform@110/220
stick:435,187,589x101:color+transform+visibility@120/240
bg_2:64,101,1152x115:color+transform@1/2
```

This is a stronger renderer boundary than a text-only cue: a later exact-family renderer can swap the diagnostic geometry/channel values for decoded authored transforms without changing the command-level inspection surface.

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr70 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr70 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr70\sward_ui_runtime_debug_gui.exe --layout-draw-command-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layout draw command smoke ok title_commands=6 title_first=mm_donut_move:102,202,563x115:color+transform@110/220 pause_first=stick:435,187,589x101:color+transform+visibility@120/240 loading_first=bg_2:64,101,1152x115:color+transform@1/2
```

## Boundary

The descriptors are still diagnostic. They bind recovered primitive ownership, normalized geometry, channel class, and sampled frame into a command shape, but they do not yet evaluate original CSD transform/color curves.
