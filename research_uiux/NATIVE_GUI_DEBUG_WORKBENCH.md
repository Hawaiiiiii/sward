<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Native GUI Debug Workbench

Phase 51 added the first native, windowed SWARD UI runtime workbench. Phase 52 adds the first visual preview surface:

```text
b/rr52/sward_ui_runtime_debug_gui.exe
```

This is the first proper non-CLI executable surface around the recovered runtime contracts and workbench host catalog. It now draws local atlas previews and schematic runtime overlays, but it is still not yet a 1:1 Sonic Unleashed UI renderer.

## What The EXE Does Now

- loads the bundled portable runtime contracts
- resolves the generated `176`-host workbench catalog
- groups hosts into the current `11` recovered ownership buckets
- presents native Win32 group and host listboxes
- runs a selected host through `ScreenRuntime`
- exposes `Run Host`, `Move Next`, `Confirm`, `Cancel`, and `Reset` controls
- shows state, input-lock, visible layer, visible prompt, contract, source-path, and callback log details
- draws a 16:9 preview panel with local atlas PNGs when available
- overlays runtime visible layers, prompt rows, and a timeline/progress strip
- supports `--smoke` so automation can verify the GUI target without opening a window
- supports `--preview-smoke` so automation can verify visual atlas bindings without opening a window

Verified smoke output:

```text
sward_ui_runtime_debug_gui smoke ok contracts=19 hosts=176 groups=11 support_hosts=17
sward_ui_runtime_debug_gui preview smoke ok atlas_candidates=8 existing_local_atlas=8 title=mainmenu__ui_mainmenu.png pause=systemcommoncore__ui_pause.png
```

## Why This Matters

The previous selector/workbench executables were real and useful, but they were console-first. Phase 51 makes the debug runtime behave like an operator tool: browse recovered source-family hosts, launch a contract, press actions, and inspect runtime consequences without memorizing CLI flags.

That is the correct foundation for the larger goal: a visual SWARD UI workbench where Sonic Unleashed UI/UX families can be loaded, inspected, animated, and eventually rendered in a 1:1-style reference executable.

## Current Limits

- It does not render decoded `.xncp` / `.yncp` node transforms yet.
- It draws ignored local atlas sheets when present, but those sheets are still local-only and not committed.
- It does not yet play original CSD animation tracks.
- It does not yet bind every translated PPC seam into executable host-specific behavior.
- It currently exercises contract-backed behavior: states, transitions, timing bands, prompts, overlay roles, and host metadata.

## Practical Runway

The first non-CLI executable is present, and Phase 52 adds the first visual preview panel.

The next useful windowed milestones are:

1. Add family-specific visual playback for one high-confidence target, probably title, pause, loading, or world-map.
2. Decode more layout node/timeline data into draw commands rather than atlas-sheet previews.
3. Add CSD-style timeline playback for intro/idle/outro bands.
4. Replace more local-only scaffold ownership with translated PPC-backed host behavior.
5. Expand from diagnostic contracts into family-specific visual playback.

Full 1:1-style UI/UX parity for all relevant Sonic Unleashed UI families is still a long-range target, not a single remaining build step. The honest state is: the proper `.exe` shell exists now; the visual renderer and full asset/layout/animation fidelity are the next major productization tracks.
