<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Authored Sampled Channel Commands

Phase 77 adds the first non-position sampled authored channel state to the native GUI draw-command bridge:

```text
b/rr77/sward_ui_runtime_debug_gui.exe
```

Phase 76 made authored sampled marker rectangles command-driven. This beat extends that command surface to carry the first sampled `Color` channel, using the parsed Pause `bg/img` sample.

## What Changed

- added Pause `bg/img` to the authored sampled command set
- added `trackType`, `sampleFrame`, and `sampledValue` fields to `LayoutAuthoredSampledDrawCommand`
- added `layoutAuthoredSampledDrawCommandChannelAlpha(...)`
- added `layoutAuthoredSampledDrawCommandChannelStateDescriptor(...)`
- fed sampled channel alpha into the diagnostic preview marker fill/pen alpha
- added `--authored-sampled-channel-command-smoke`

## Verified Channel Command

```text
pause: ui_pause.yncp / bg / img / Color @ frame 7
```

Descriptor shape:

```text
scene/cast:x,y,widthxheight:track@frame=value:alpha=value
```

Verified descriptor:

```text
bg/img:0,0,1280x720:Color@7=0.000000:alpha=0.000000
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr77 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr77 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr77\sward_ui_runtime_debug_gui.exe --authored-sampled-channel-command-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui authored sampled channel command smoke ok pause_channel=bg/img:0,0,1280x720:Color@7=0.000000:alpha=0.000000
```

## Boundary

This is still a one-sample renderer bridge, not full CSD channel playback. It proves the command path can carry and expose sampled non-position channel state. The next renderer step is to broaden this from one Pause color sample into actual channel evaluation across more casts, frames, and families.
