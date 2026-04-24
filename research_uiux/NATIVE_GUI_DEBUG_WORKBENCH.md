<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Native GUI Debug Workbench

Phase 51 added the first native, windowed SWARD UI runtime workbench. Phase 52 added the first visual preview surface, Phase 53 added gameplay-HUD proxy atlas binding, Phase 54 added timer-driven playback controls, and Phase 55 adds state-aware preview motion:

```text
b/rr55/sward_ui_runtime_debug_gui.exe
```

This is the first proper non-CLI executable surface around the recovered runtime contracts and workbench host catalog. It now draws local atlas previews and schematic runtime overlays, but it is still not yet a 1:1 Sonic Unleashed UI renderer.

## What The EXE Does Now

- loads the bundled portable runtime contracts
- resolves the generated `176`-host workbench catalog
- groups hosts into the current `11` recovered ownership buckets
- presents native Win32 group and host listboxes
- runs a selected host through `ScreenRuntime`
- exposes `Run Host`, `Move Next`, `Confirm`, `Cancel`, and `Reset` controls
- exposes Play/Pause and Step controls for timer-driven contract playback
- shows state, input-lock, visible layer, visible prompt, contract, source-path, and callback log details
- draws a 16:9 preview panel with local atlas PNGs when available
- overlays runtime visible layers, prompt rows, and a timeline/progress strip
- applies eased state-aware preview motion and alpha to overlay layers and prompt buttons during active contract bands
- fills atlas-backed preview canvases with a dark backing brush before drawing local PNGs, keeping transparent proxy sheets readable
- supports `--smoke` so automation can verify the GUI target without opening a window
- supports `--preview-smoke` so automation can verify visual atlas bindings without opening a window
- supports `--playback-smoke` so automation can verify timer-driven contract advancement without opening a window
- supports `--motion-smoke` so automation can verify state-aware preview motion without opening a window

Verified smoke output:

```text
sward_ui_runtime_debug_gui smoke ok contracts=19 hosts=176 groups=11 support_hosts=17
sward_ui_runtime_debug_gui preview smoke ok atlas_candidates=10 proxy_candidates=2 existing_local_atlas=10 title=mainmenu__ui_mainmenu.png pause=systemcommoncore__ui_pause.png sonic_stage=exstagetails_common__ui_prov_playscreen.png
sward_ui_runtime_debug_gui playback smoke ok intro=Intro after_intro=Idle action=Navigate after_action=Idle
sward_ui_runtime_debug_gui motion smoke ok intro_alpha=0.28 intro_offset_x=-204.8 intro_end_alpha=1 idle_alpha=1 outro_alpha=0.32
```

## Why This Matters

The previous selector/workbench executables were real and useful, but they were console-first. Phase 51 makes the debug runtime behave like an operator tool: browse recovered source-family hosts, launch a contract, press actions, and inspect runtime consequences without memorizing CLI flags.

That is the correct foundation for the larger goal: a visual SWARD UI workbench where Sonic Unleashed UI/UX families can be loaded, inspected, animated, and eventually rendered in a 1:1-style reference executable.

## Current Limits

- It does not render decoded `.xncp` / `.yncp` node transforms yet.
- It draws ignored local atlas sheets when present, but those sheets are still local-only and not committed.
- Sonic/Werehog gameplay HUD previews currently use the recovered `ui_prov_playscreen` sheet as a marked proxy until exact loose `ui_playscreen*` assets are recovered.
- It does not yet play original CSD animation tracks.
- It now plays portable runtime contract timing bands, which is a step toward visual playback but not original CSD keyframe parity.
- It now applies portable role/state preview motion, which is a visual debugging adapter and not recovered original CSD motion.
- It does not yet bind every translated PPC seam into executable host-specific behavior.
- It currently exercises contract-backed behavior: states, transitions, timing bands, prompts, overlay roles, and host metadata.

## Practical Runway

The first non-CLI executable is present, Phase 52 added the first visual preview panel, Phase 53 made the gameplay-HUD host preview useful enough to inspect in the window, Phase 54 keeps intro/action bands visible through timer-driven playback, and Phase 55 gives those bands visible preview motion.

The next useful windowed milestones are:

1. Add family-specific visual playback for one high-confidence target, probably title, pause, loading, or world-map.
2. Decode more layout node/timeline data into draw commands rather than atlas-sheet previews.
3. Add CSD-style timeline playback for intro/idle/outro bands.
4. Replace more local-only scaffold ownership with translated PPC-backed host behavior.
5. Expand from diagnostic contracts into family-specific visual playback.

Full 1:1-style UI/UX parity for all relevant Sonic Unleashed UI families is still a long-range target, not a single remaining build step. The honest state is: the proper `.exe` shell exists now; visual timing/motion scaffolding exists; the full asset/layout/animation renderer is the next major productization track.
