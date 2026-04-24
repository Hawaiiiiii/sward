<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Timeline Playback Controls

Phase 54 turns the native GUI preview from a settled-state visual probe into the first timer-driven playback surface:

```text
b/rr54/sward_ui_runtime_debug_gui.exe
```

The workbench still uses portable runtime contracts, not decoded original CSD timelines. The important change is that the executable now shows transition time passing inside the operator UI instead of immediately jumping to the final idle state.

## What Changed

- added Play/Pause and Step controls to the native GUI workbench
- added a Win32 timer loop around `ScreenRuntime::tick(...)`
- changed `Run Host` to enter the contract intro state and start playback instead of calling the old immediate settle path
- changed action buttons to start playback when they trigger Navigate, Confirm, or Cancel state bands
- added a deterministic Step path for frame-by-frame inspection
- added preview footer state for playback status
- added `--playback-smoke` so automation can verify timeline advancement without opening a window

Verified playback smoke:

```text
sward_ui_runtime_debug_gui playback smoke ok intro=Intro after_intro=Idle action=Navigate after_action=Idle
```

## Why It Matters

This is a necessary bridge toward real UI/UX reconstruction. The previous GUI could display atlas context and runtime layers, but it flattened the timing model by settling transitions immediately. Phase 54 keeps the transition band visible long enough to inspect:

- input lock windows
- visible overlay roles during intro/outro/action states
- prompt suppression while timelines are active
- timeline/progress strip movement
- callback trace order for state, scene, and action events

For SGFX-style reuse, this is the first version that behaves like an operator-facing UI playback harness rather than a static catalog browser.

## Current Limits

- Playback uses the portable contract timeline bands, not original CSD keyframes.
- Layer transforms remain role-aware projections over local atlas sheets, not decoded `.xncp` / `.yncp` draw nodes.
- Sonic/Werehog gameplay HUD previews still use the recovered `ui_prov_playscreen` atlas as marked proxy evidence.
- Exact layout/timeline parity still needs decoded node transforms, CSD track playback, and more PPC-backed host behavior.

## Next Product Step

The next useful beat is to give one family a stronger visual playback adapter. A good target is Title, Pause, or Sonic HUD:

- Title/Pause are better for exact atlas/layout confidence.
- Sonic HUD is better for the SGFX/HUD framework direction, but it still needs exact `ui_playscreen*` payload recovery or a carefully labeled proxy adapter.
