<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Native GUI Debug Workbench

Phase 51 added the first native, windowed SWARD UI runtime workbench. Phase 52 added the first visual preview surface, Phase 53 added gameplay-HUD proxy atlas binding, Phase 54 added timer-driven playback controls, Phase 55 added state-aware preview motion, Phase 56 added exact-family preview layouts, Phase 57 added decoded layout-evidence overlays, Phase 58 added frame-domain layout timeline readouts, Phase 59 added the first scene-primitive draw pass, Phase 60 widened primitives into gameplay HUD proxy previews, Phase 61 audits gameplay HUD primitive scene ownership, Phase 62 adds primitive animation-bank/frame-cursor cues, Phase 63 surfaces those cues in the GUI detail pane, Phase 64 adds primitive channel-classification cues, Phase 65 adds a compact primitive channel-count legend in the preview, Phase 66 adds host-level visual parity summaries, Phase 67 adds host-list readiness badges, and Phase 68 adds next-renderer blocker cues:

```text
b/rr68/sward_ui_runtime_debug_gui.exe
```

This is the first proper non-CLI executable surface around the recovered runtime contracts and workbench host catalog. It now draws local atlas previews and schematic runtime overlays, but it is still not yet a 1:1 Sonic Unleashed UI renderer.

## What The EXE Does Now

- loads the bundled portable runtime contracts
- resolves the generated `176`-host workbench catalog
- groups hosts into the current `11` recovered ownership buckets
- presents native Win32 group and host listboxes
- tags host listbox labels with compact exact/proxy/layout/primitive/channel/contract readiness badges
- runs a selected host through `ScreenRuntime`
- exposes `Run Host`, `Move Next`, `Confirm`, `Cancel`, and `Reset` controls
- exposes Play/Pause and Step controls for timer-driven contract playback
- shows state, input-lock, visible layer, visible prompt, contract, source-path, and callback log details
- draws a 16:9 preview panel with local atlas PNGs when available
- overlays runtime visible layers, prompt rows, and a timeline/progress strip
- applies eased state-aware preview motion and alpha to overlay layers and prompt buttons during active contract bands
- uses exact-family preview layout adapters for Title, Pause, and Loading before falling back to generic role projection
- overlays decoded layout evidence for Title, Pause, and Loading previews, including layout IDs, verdicts, scene/animation counts, cue summaries, and longest parsed timelines
- maps active runtime progress into recovered longest-timeline frame domains and draws `Frame: current/total @ fps` readouts in the evidence panel
- draws recovered scene primitives for the highest keyframe-density Title, Pause, and Loading layout scenes over the atlas preview
- draws recovered `ui_prov_playscreen` scene primitives for Sonic, Werehog, and Extra Stage HUD preview hosts with smoke-guarded scene/keyframe ownership
- draws recovered primitive animation-bank names and sampled frame cursors for those diagnostic scene boxes
- reports primitive count, total keyframes, animation bank, sampled frame, and track summary in the detail pane for hosts with primitive evidence
- classifies recovered primitive tracks as color, sprite, transform, visibility, or static and surfaces those tags in the overlay/detail paths
- draws a compact transform/color/visibility/sprite/static primitive channel-count legend in the preview
- adds a `Visual parity` detail-pane summary covering atlas exact/proxy state, layout evidence, primitive/keyframe coverage, and channel totals
- adds `next_renderer=` blocker cues to the `Visual parity` detail section so the operator can see the next visual reconstruction step per host
- exposes the custom preview panel paint path to `WM_PRINTCLIENT` / `WM_PRINT` so visual automation can capture the preview surface directly
- fills atlas-backed preview canvases with a dark backing brush before drawing local PNGs, keeping transparent proxy sheets readable
- preserves atlas readability under structural backdrop and cinematic-frame roles by outline-drawing those overlays instead of tint-filling over them
- supports `--smoke` so automation can verify the GUI target without opening a window
- supports `--preview-smoke` so automation can verify visual atlas bindings without opening a window
- supports `--playback-smoke` so automation can verify timer-driven contract advancement without opening a window
- supports `--motion-smoke` so automation can verify state-aware preview motion without opening a window
- supports `--family-preview-smoke` so automation can verify exact-family layout placement without opening a window
- supports `--layout-evidence-smoke` so automation can verify decoded layout facts without opening a window
- supports `--layout-timeline-smoke` so automation can verify frame-domain timeline mapping without opening a window
- supports `--layout-primitive-smoke` so automation can verify scene-primitive counts and keyframe totals without opening a window
- supports `--layout-primitive-playback-smoke` so automation can verify primitive animation-bank labels and sampled frame cursors without opening a window
- supports `--layout-primitive-detail-smoke` so automation can verify primitive detail-pane parity text without opening a window
- supports `--layout-primitive-channel-smoke` so automation can verify primitive channel classifications without opening a window
- supports `--layout-primitive-channel-legend-smoke` so automation can verify preview channel-count legends without opening a window
- supports `--visual-parity-smoke` so automation can verify exact/proxy visual readiness without opening a window
- supports `--host-readiness-smoke` so automation can verify host-list readiness badges without opening a window
- supports `--renderer-blocker-smoke` so automation can verify next-renderer blocker classification without opening a window
- supports `--layer-fill-smoke` so automation can verify the atlas-preserving structural fill policy without opening a window

Verified smoke output:

```text
sward_ui_runtime_debug_gui smoke ok contracts=19 hosts=176 groups=11 support_hosts=17
sward_ui_runtime_debug_gui preview smoke ok atlas_candidates=10 proxy_candidates=2 existing_local_atlas=10 title=mainmenu__ui_mainmenu.png pause=systemcommoncore__ui_pause.png sonic_stage=exstagetails_common__ui_prov_playscreen.png
sward_ui_runtime_debug_gui playback smoke ok intro=Intro after_intro=Idle action=Navigate after_action=Idle
sward_ui_runtime_debug_gui motion smoke ok intro_alpha=0.28 intro_offset_x=-204.8 intro_end_alpha=1 idle_alpha=1 outro_alpha=0.32
sward_ui_runtime_debug_gui family preview smoke ok title=title_menu pause=pause_menu loading=loading_transition title_logo_y=64.8 title_content_y=331.2 pause_chrome_w=844.8 loading_frame_y=187.2
sward_ui_runtime_debug_gui layout evidence smoke ok title=ui_mainmenu scenes=16 animations=6 pause=ui_pause scenes=29 animations=41 loading=ui_loading scenes=7 animations=37
sward_ui_runtime_debug_gui layout timeline smoke ok title_frame=110/220 pause_frame=120/240 loading_frame=180/240 fps=60
sward_ui_runtime_debug_gui layout primitive smoke ok title_primitives=6 keyframes=806 pause_primitives=6 keyframes=806 loading_primitives=6 keyframes=2775 sonic_stage_primitives=6 keyframes=680 werehog_stage_primitives=6 keyframes=680 extra_stage_primitives=6 keyframes=680 sonic_speed_gauge_kf=360 sonic_ring_energy_gauge_kf=240 sonic_ring_get_effect_kf=14 sonic_bg_kf=0
sward_ui_runtime_debug_gui layout primitive playback smoke ok speed_anim=Size_Anim speed_frame=50/100 energy_anim=Size_Anim energy_frame=50/100 info_anim=Count_Anim info_frame=50/100 ring_fx_anim=Intro_Anim ring_fx_frame=3/5 bg_anim=DefaultAnim bg_frame=50/100
sward_ui_runtime_debug_gui layout primitive detail smoke ok primitives=6 keyframes=680 speed=so_speed_gauge/Size_Anim frame=50/100 ring_fx=ring_get_effect/Intro_Anim frame=3/5
sward_ui_runtime_debug_gui layout primitive channel smoke ok sonic_transform=3 sonic_color=4 sonic_visibility=2 sonic_static=1 speed_channels=color+transform info2_channels=visibility bg_channels=static
sward_ui_runtime_debug_gui layout primitive channel legend smoke ok legend=T3 C4 V2 S0 static1 label=Channels T3 C4 V2 S0 static1
sward_ui_runtime_debug_gui visual parity smoke ok sonic_atlas=proxy sonic_layout=none sonic_primitives=6 sonic_channels=T3 C4 V2 S0 static1 title_atlas=exact title_layout=ui_mainmenu title_primitives=6
sward_ui_runtime_debug_gui host readiness smoke ok sonic_label=SonicMainDisplay.cpp [proxy primitive channels] title_label=GameModeMainMenu_Test.cpp [exact layout primitive channels] support_label=AchievementManager.cpp [contract]
sward_ui_runtime_debug_gui renderer blocker smoke ok sonic_blocker=exact loose HUD payload title_blocker=decoded CSD channel sampling support_blocker=visual evidence binding
sward_ui_runtime_debug_gui layer fill smoke ok backdrop_alpha=0 cinematic_alpha=0 content_alpha=0.58
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
- It now separates Title, Pause, and Loading preview placement, but still does not decode original layout-node transforms.
- It now surfaces parsed layout/timeline evidence in the preview, but that evidence is still a diagnostic overlay rather than the renderer source of truth.
- It now projects runtime progress into recovered layout frame domains, but it still does not sample original animation channels.
- It now draws evidence-backed scene primitives, but those primitives are diagnostic scene boxes rather than exact authored node transforms.
- Gameplay HUD primitives for Sonic/Werehog remain tied to the explicit `ui_prov_playscreen` proxy boundary, now with audited scene/keyframe ownership, animation-bank/frame-cursor cues, readable detail-pane parity text, channel-classification tags, compact preview channel-count legends, host-level visual parity summaries, host-list readiness badges, and `next_renderer=exact loose HUD payload` blocker cues for the current proxy primitive set.
- It does not yet bind every translated PPC seam into executable host-specific behavior.
- It currently exercises contract-backed behavior: states, transitions, timing bands, prompts, overlay roles, and host metadata.

## Practical Runway

The first non-CLI executable is present, Phase 52 added the first visual preview panel, Phase 53 made the gameplay-HUD host preview useful enough to inspect in the window, Phase 54 keeps intro/action bands visible through timer-driven playback, Phase 55 gives those bands visible preview motion, Phase 56 splits exact Title/Pause/Loading placement away from the generic role stack, Phase 57 puts parsed layout/timeline evidence into the same visual surface, Phase 58 adds frame-domain progress over those recovered timelines, Phase 59 starts drawing recovered scene primitives, Phase 60 widens that primitive layer to gameplay HUD proxy previews, Phase 61 guards the gameplay HUD proxy primitive ownership against the parsed deep-analysis scene facts, Phase 62 attaches recovered animation-bank names plus sampled frame cursors to that primitive layer, Phase 63 makes the same evidence readable in the operator detail pane, Phase 64 classifies the recovered track summaries into typed channel cues, Phase 65 adds a compact preview legend for those channel counts, Phase 66 folds the atlas/layout/primitive/channel readiness into a single detail-pane parity summary, Phase 67 pushes that readiness signal into the host browser, and Phase 68 adds the next-renderer blocker directly to the parity readout.

The next useful windowed milestones are:

1. Add family-specific visual playback for one high-confidence target, probably title, pause, loading, or world-map.
2. Decode more layout node/timeline data into draw commands rather than atlas-sheet previews.
3. Add CSD-style timeline playback for intro/idle/outro bands.
4. Replace more local-only scaffold ownership with translated PPC-backed host behavior.
5. Expand from diagnostic contracts into family-specific visual playback.

Full 1:1-style UI/UX parity for all relevant Sonic Unleashed UI families is still a long-range target, not a single remaining build step. The honest state is: the proper `.exe` shell exists now; visual timing/motion scaffolding exists; first exact-family placement adapters exist; parsed layout evidence, frame-domain timeline readouts, diagnostic scene primitives, audited gameplay HUD proxy primitives, primitive animation/frame cues, readable primitive detail summaries, primitive channel cues, primitive channel legends, host-level visual parity summaries, host-list readiness badges, and next-renderer blocker cues are visible in the window; the full asset/layout/animation renderer is the next major productization track.
