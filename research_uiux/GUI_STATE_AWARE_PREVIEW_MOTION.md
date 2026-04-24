<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI State-Aware Preview Motion

Phase 55 adds the first state-aware visual motion adapter to the native GUI workbench:

```text
b/rr55/sward_ui_runtime_debug_gui.exe
```

This is still portable contract-preview motion, not decoded original CSD keyframe playback. The point of this beat is to make the windowed preview react visibly to `Intro`, `Navigate`, `Confirm`, `Cancel`, and `Outro` timing bands instead of drawing every overlay role as a static rectangle.

## What Changed

- added `PreviewMotion` as a small visual adapter over runtime overlay roles
- added cubic eased progress from the active `ScreenRuntime` state elapsed time and contract band duration
- added state/role-specific offsets and alpha for counters, gauges, sidecars, prompts, and transient effects
- applied motion to preview overlay rectangles and prompt buttons while keeping layout projection bounded to the 16:9 preview canvas
- added a dark atlas backing brush before drawing ignored local PNG atlas sheets so transparent atlas regions no longer expose the white Win32 panel background
- added `--motion-smoke` so automation can verify the eased motion path without opening the GUI window

Verified motion smoke:

```text
sward_ui_runtime_debug_gui motion smoke ok intro_alpha=0.28 intro_offset_x=-204.8 intro_end_alpha=1 idle_alpha=1 outro_alpha=0.32
```

## Why It Matters

The GUI already had three necessary pieces: a real native window, local atlas preview binding, and timer-driven runtime playback. Phase 55 connects those pieces by letting preview layers respond to the current state band:

- `Intro` layers now slide/fade in instead of appearing fully settled.
- `Navigate` and action bands can show short lateral motion while the runtime is input-locked or transitioning.
- `Cancel` / `Outro` layers can fade and move out.
- proxy gameplay-HUD atlas sheets remain readable even when transparent regions would otherwise show a blank panel.

For the SGFX-style framework direction, this is the first reusable visual-motion hook in the workbench. It gives the future renderer a place to swap in decoded layout/CSD transforms once those tracks are promoted from archaeology into executable preview data.

## Current Limits

- The motion adapter is role/state-driven and portable; it is not a recovered Sonic Unleashed animation curve.
- Atlas sheets are still local-only under `extracted_assets/visual_atlas` and are not committed.
- Sonic/Werehog gameplay HUD previews still use the recovered `ui_prov_playscreen` sheet as marked proxy evidence.
- Exact UI parity still needs decoded `.xncp` / `.yncp` node transforms, CSD timeline playback, and more PPC-backed host behavior.

## Follow-On

Phase 56 adds exact-family preview placement for Title, Pause, and Loading on top of this motion layer, so those exact-atlas families no longer share the same generic role stack as gameplay HUD and support probes.

## Verification

Fresh verification on `2026-04-24`:

```text
cmd /c "call ... vcvars64.bat && cmake --build b/rr55 --config Release"
python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py
b\rr55\sward_ui_runtime_debug_gui.exe --motion-smoke
b\rr55\sward_ui_runtime_debug_gui.exe --playback-smoke
b\rr55\sward_ui_runtime_debug_gui.exe --preview-smoke
b\rr55\sward_ui_runtime_debug_gui.exe --smoke
python research_uiux\runtime_reference\examples\test_ui_debug_workbench_catalog.py
python research_uiux\tools\test_phase50_support_runtime_contracts.py
git diff --check
rg -- "- \[ \]" research_uiux/TODO_CHECKLIST.md
```

The GUI was also launch/capture-checked by selecting `Gameplay HUD Hosts -> SonicMainDisplay.cpp`, pressing `Run Host`, and capturing the rr55 window while the runtime was in the `Intro` band.
