<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Renderer Blocker Cues

Phase 68 adds a next-renderer cue to the native GUI visual parity path:

```text
b/rr68/sward_ui_runtime_debug_gui.exe
```

The GUI already shows what evidence exists for a selected host. This beat adds the next practical rendering blocker so the workbench can tell the operator what must be recovered or implemented next for that host to move from diagnostic preview toward real visual playback.

## What Changed

- added `visualParityRendererBlocker(...)`
- added `next_renderer=...` to the `Visual parity:` detail-pane block
- added `--renderer-blocker-smoke`

## Current Blocker Classes

| Cue | Meaning |
|---|---|
| `exact loose HUD payload` | the host is on a marked proxy atlas and needs exact loose HUD payload binding |
| `decoded CSD channel sampling` | atlas/layout/primitive/channel evidence exists; the next renderer step is sampling original channel curves into draw commands |
| `primitive transform sampling` | primitives exist but channel classes are not yet strong enough for typed playback |
| `layout node transform decoding` | layout evidence exists but primitive draw commands are not yet bound |
| `visual evidence binding` | the host is contract-backed, but visual atlas/layout/primitive evidence is not yet attached |

## Verified Examples

```text
sonic_blocker=exact loose HUD payload
title_blocker=decoded CSD channel sampling
support_blocker=visual evidence binding
```

This keeps the Sonic HUD proxy boundary explicit while making Title/Pause/Loading-style exact-atlas families the obvious next candidates for decoded CSD channel sampling work.
Phase 69 starts that follow-up with exact-family `scene:channels@frame/count` sample tokens in [`GUI_LAYOUT_CHANNEL_SAMPLE_CUES.md`](./GUI_LAYOUT_CHANNEL_SAMPLE_CUES.md).

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr68 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr68 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr68\sward_ui_runtime_debug_gui.exe --renderer-blocker-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui renderer blocker smoke ok sonic_blocker=exact loose HUD payload title_blocker=decoded CSD channel sampling support_blocker=visual evidence binding
```

## Boundary

The cue is a triage classifier, not a renderer. It points to the next implementation or recovery blocker for a host; it does not claim original animation curves are already being sampled or that proxy HUD assets are exact.
