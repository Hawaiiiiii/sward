<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# Debug Menu Fork Harvest And Live Bridge

Phase 116 harvests the local `UnleashedRecomp-debug-menu/` fork as a reference-only source of runtime structure and operator-shell patterns. The fork is not vendored into SWARD, and the local tree remains ignored. The useful material is converted into SWARD-owned UI Lab evidence, typed inspector labels, and a direct live bridge.

Machine-readable harvest: `research_uiux/data/debug_menu_fork_harvest.json`

## Harvested API Surfaces

### CSD Manager

Useful paths:

- `api/CSD/Manager/csdmProject.h`
- `api/CSD/Manager/csdmScene.h`
- `api/CSD/Manager/csdmNode.h`
- `api/CSD/Manager/csdmRCPtr.h`
- `api/CSD/Platform/csdTexList.h`

Key typed fields now carried into the UI Lab live-state vocabulary:

- `CSD.Manager.CScene.m_MotionFrame`
- `CSD.Manager.CScene.m_MotionRepeatType`
- `CSD.Manager.CNode.m_pMotionPattern`

These are best treated as CSD readiness and motion-state anchors. They should drive future CSD project/scene/node/layer inspection, not a standalone renderer.

### SWA CSD Wrappers

Useful paths:

- `api/SWA/CSD/CsdProject.h`
- `api/SWA/CSD/CsdDatabaseWrapper.h`
- `api/SWA/CSD/CsdTexListMirage.h`
- `api/SWA/CSD/GameObjectCSD.h`

Key typed fields:

- `SWA.CSD.CCsdProject.m_rcProject`
- `SWA.CSD.CGameObjectCSD.m_rcProject`

These map runtime CSD owner objects back to the UI Lab target CSD readiness events such as `target-csd-project-made` and `stage-target-csd-bound`.

### SWA HUD Owners

Useful paths:

- `api/SWA/HUD/Sonic/HudSonicStage.h`
- `api/SWA/HUD/Loading/Loading.h`
- `api/SWA/HUD/Pause/HudPause.h`
- `api/SWA/HUD/GeneralWindow/GeneralWindow.h`
- `api/SWA/HUD/SaveIcon/SaveIcon.h`

Key typed fields:

- `SWA.HUD.CHudSonicStage.m_rcPlayScreen`
- `SWA.HUD.CHudSonicStage.m_rcSpeedGauge`
- `SWA.HUD.CLoading.m_LoadingDisplayType`
- `SWA.HUD.CHudPause.m_Action`
- `SWA.HUD.CGeneralWindow.m_rcGeneral`
- `SWA.HUD.CSaveIcon.m_IsVisible`

These are the next typed HUD-owner/status inspector anchors for Sonic HUD, loading, pause/general windows, and save icon visibility.

### SWA System And GameMode

Useful paths:

- `api/SWA/System/ApplicationDocument.h`
- `api/SWA/System/GameDocument.h`
- `api/SWA/System/GameMode/GameModeStage.h`
- `api/SWA/System/GameMode/Title/TitleMenu.h`
- `api/SWA/System/GameMode/Title/TitleStateBase.h`

Key typed fields:

- `SWA.System.GameMode.CGameModeStage`
- `SWA.System.GameMode.Title.CTitleMenu.m_CursorIndex`
- `SWA.System.GameMode.Title.CTitleStateBase.CMember.m_pTitleMenu`
- `SWA.System.GameDocument.CMember.m_StageName`

These remain high-value for title-menu readiness, deterministic stage boot, game-mode ownership, loading transitions, and active-stage context.

### Reddog Operator Shell

Useful paths:

- `ui/reddog/reddog_manager.h`
- `ui/reddog/reddog_window.h`
- `ui/reddog/debug_draw.h`
- `ui/reddog/windows/window_list.h`
- `ui/reddog/windows/counter_window.h`
- `ui/reddog/windows/view_window.h`
- `ui/reddog/windows/exports_window.h`
- `ui/reddog/windows/welcome_window.h`

Useful concepts:

- `Reddog.Manager`
- `Reddog.DebugDraw`

SWARD already ports this as a default-open operator shell with window list, counters, exports, view/debug toggles, and a foreground debug-draw layer.

## Live Bridge

The Phase 116 bridge keeps native BMP capture as visual proof and JSONL as durable evidence, but adds a direct tool-facing live bridge so Codex/tools can ask the running UI Lab what it knows.

Transport:

- Windows named pipe: `\\.\pipe\sward_ui_lab_live`
- Enabled by default for UI Lab sessions.
- Can be disabled with `--ui-lab-live-bridge=off`.
- Pipe name can be changed with `--ui-lab-live-bridge-name <name>`.

Supported commands:

- `state`
- `events`
- `route-status`
- `ui-oracle`
- `ui-draw-list`
- `ui-gpu-submit`
- `route <target>`
- `reset`
- `set-global <name> <0|1>`
- `capture`
- `help`

The bridge exposes:

- current target
- route/event latch
- title/menu/loading/stage/HUD readiness
- CSD project and stage pointers already known to the UI Lab
- SGlobals values and addresses
- debug-menu fork-derived typed fields
- recent JSONL events
- command capabilities

This does not replace evidence. It makes the real runtime easier to interrogate while native BMPs and JSONL remain the oracle.

## Phase 117 Live-Bridge Client And Typed Inspectors

Phase 117 adds a repo-safe live-bridge client at `research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1`. It can query `state`, `events`, `route`, `set-global`, and `capture` through the named pipe without screen scraping.

The capture helper can now use live bridge readiness as the capture driver with `-UseLiveBridgeReadiness`. JSONL remains the durable evidence log, and native BMPs remain the visual confirmation path.

The live state now promotes the fork harvest into a `typedInspectors` block:

- CSD scene motion frame and motion repeat type, when a live scene pointer is known.
- loading display type plus a debug-menu enum label.
- title cursor/menu owner readiness and owner CSD pointer.
- Sonic HUD owner fields carried as typed latches: stage game-mode address, `m_rcPlayScreen` project, and `m_rcSpeedGauge` scene identity once the real stage/CSD route binds.

## Phase 118 Route Stability Through The Live Bridge

Phase 118 hardens the live bridge for combined sweeps. The capture helper now uses a unique live bridge pipe per target by default when `-UseLiveBridgeReadiness` is active, keeping one capture target from talking to a stale or previous target pipe. The runtime also exposes a `route-status` command with route generation, reset count, hook-observed flags, latest title/menu/stage context strings, and readiness fields.

For `title-menu`, live bridge readiness is no longer enough by itself. The helper requires the durable JSONL `title-menu-visible` event before completing the readiness wait, so native BMP capture remains visual confirmation and JSONL remains the oracle. The goal is a full early-game live-bridge sweep where title-loop, title-menu, title-options, loading, and sonic-hud all pass in one run before deeper CSD/HUD owner traversal continues.

## Phase 119 Deeper Typed Inspectors

Phase 119 promotes the existing real `CCsdProject::Make` resource traversal into the live bridge. The runtime now records full CSD project/scene/node/layer traversal samples under `typedInspectors.csdProjectTree`, including the active project, observed projects, project/root addresses, scene/node/layer counts, sampled paths, and live `CScene` motion-frame / repeat-type fields when a scene pointer is known.

Pause, general-window, and save-icon runtime owners are now first-class live inspectors instead of harvest-only labels. `CHudPause`, `CGeneralWindow`, and `CSaveIcon` update `typedInspectors.pauseGeneralSave` with owner addresses, backing CSD object pointers, status/action/cursor labels, and visibility state.

`CHudSonicStage owner pointer paths` are also exposed more deeply. The live bridge reports resolved CSD ownership for `m_rcPlayScreen`, `m_rcSpeedGauge`, `m_rcRingEnergyGauge`, and `m_rcGaugeFrame` through the observed CSD tree. The raw owner pointer is still explicitly marked as pending a dedicated `CHudSonicStage` object hook; it is not inferred from screenshots or treated as solved until that hook exists.

## Phase 120 Raw Sonic Owner And Pause Route

Phase 120 adds the raw `CHudSonicStage` owner object hook: the raw CHudSonicStage owner object hook. The new `CHudSonicStage_patches.cpp` seam reports the owner plus `m_rcPlayScreen`, `m_rcSpeedGauge`, `m_rcRingEnergyGauge`, and `m_rcGaugeFrame` through `UiLab::OnHudSonicStageUpdate`; the live bridge emits `sonic-hud-owner-hooked` once a plausible owner is observed. The event and live state expose `owner_fields_ready` / `rawOwnerFieldsReady` separately, so an owner-only hook is not misread as proven CSD-field ownership.

Phase 120 also adds a deterministic pause route. `pause` is now a stage-harness target with `ui_pause` as its primary CSD, a scoped Start-input injector after stage load, and explicit `pause-target-ready` / `pause-ready` evidence once `CHudPause` reports a visible or shown owner. Native BMP capture remains the visual confirmation path; live bridge state drives readiness, not screenshot inference.

The capture helper now gives `pause` a target-aware effective auto-exit floor. A requested `75s` run records `requestedAutoExitSeconds=75` and `effectiveAutoExitSeconds=95`, which keeps slower stage/loading cycles alive long enough for the late `pause-target-ready` latch without changing non-pause targets. Native-capture sessions also request runtime exit on the next presented frame after the requested BMP set completes, so evidence runs stop at the proof point instead of sitting in the target state until a generic timeout.

## Phase 121 Sonic HUD Owner Maturation And Tutorial Route

Phase 121 adds explicit Sonic HUD owner maturation sampling. The raw `CHudSonicStage` hook was already live, but the fork-header `m_rcPlayScreen/m_rcSpeedGauge/m_rcRingEnergyGauge/m_rcGaugeFrame stayed zero` through the Phase 120 hook points. The live bridge now samples the expected `api/SWA/HUD/Sonic/HudSonicStage.h` RCPtr object slots at offsets `0xE4`, `0xEC`, `0xF4`, and `0xFC`, exposes those raw owner field samples under `typedInspectors.sonicHud.ownerPath.rawOwnerFieldSamples`, and reports an `ownerFieldMaturationStatus` instead of pretending the fork offsets are proven when they are still null.

This makes the current explanation testable: the raw owner is real, while the embedded fork API RCPtr slots are either not mature at `sub_824D89B0`, `sub_824D9308`, and `sub_824D95F8`, or the debug-menu fork layout does not match this translated runtime at those fields. Until raw samples resolve the RCPtr RCObject memory path, the mature CSD project/scene addresses remain the real `CCsdProject::Make` traversal evidence.

Phase 121 also tightens the tutorial/HUD guide route. `tutorial` now targets the `ui_playscreen` project from `Player/Character/Sonic/Hud/SonicHudGuide.cpp`, requires the raw Sonic HUD owner hook plus `tutorial-hud-owner-path-ready`, and emits `tutorial-target-ready` before `tutorial-ready`. The capture helper requires those events, so live bridge plus native BMP proof now has a real owner-path gate instead of treating generic stage/CSD readiness as enough.

Phase 121 also adds a stage-title owner direct-state arming seam for unattended stage targets. Failed long runs showed `CTitleStateIntro` reporting `requested_state=1` while the `CGameModeStageTitle` owner context repeatedly read back `dirty=0`, `transition_armed=0`, `owner_gate568=0`, and `csd_byte84=0`. The new `stage-title-owner-direct-state-requested` / `stage-title-owner-direct-state-applied` events are narrowly gated to direct-context stage-harness routes and write those owner-context bytes at the stage-title boundary, where the real owner is observed. This keeps the diagnostic explicit: it is route stabilization for the real runtime owner path, not a fake HUD renderer.

The stage title owner direct-state fallback waits for the title intro direct-state path to mature before it fires. Focused HUD evidence showed the stable runtime path can spend roughly twenty seconds of presented frames in title intro after `title-intro-direct-state-requested` before the real title-menu hook appears; firing the owner seam immediately can create an early `is_title_state_menu=1` state that never advances past `menu_cursor=0`. The title route now uses a one-shot title intro direct-state request: repeated refreshes were observed to kick the runtime back through loading/title-intro loops instead of letting the state machine mature. The owner seam is now a late fallback, while the title-menu-to-stage handoff still holds the direct-context menu latch once the real title menu is actually observed. The owner direct-state fallback is diagnostic opt-in through `--ui-lab-stage-title-owner-direct-fallback`; default captures wait for the natural title-menu owner path. When that diagnostic fallback is enabled, the direct-context menu handoff injects one accept pulse and records `title-menu-direct-context-accept-injected`, so the synthesized owner-state experiment has the same New Game accept edge that ENTER/A/Start provides during manual operation.

Phase 121 control automation moves unattended routes back to real mapped controls. The capture helper now knows `ENTER/W/A/S/D/Q/E`, focuses the UnleashedRecomp window, and `title-menu`, `title-options`, plus stage targets default to real keyboard input automation unless `-DisableControlAutomation` is supplied. In this mode, input automation is the route driver for pressing through title/menu flow, while the live bridge plus native BMP remain the oracle for readiness and visual proof. This keeps routing honest: the harness presses the same local controls a person would press, and JSONL/native captures still decide whether title menu, Sonic HUD, pause, or tutorial actually became runtime-visible. The sender uses scan-code `SendInput` first, keeps `keybd_event` as a fallback, and records foreground/send results in the manifest so a manual ENTER cannot be mistaken for proven automation.

## Phase 122 SGFX Reusable Templates

Phase 122 starts converting real-runtime evidence into a repo-safe SGFX template catalog. This is not a fake Sonic renderer and does not contain extracted assets; it packages the reusable UI/UX architecture that the runtime evidence has proven: state flow, timing bands, input-lock rules, layer roles, prompt policy, asset slots, and evidence provenance for `title-menu`, `loading`, `sonic-hud`, and `tutorial`.

The new `sward_sgfx_template_catalog` target emits portable screen recipes from `include/sward/ui_runtime/sgfx_templates.hpp` and `src/sgfx_templates.cpp`. Each recipe points back to the existing portable contract file plus live evidence events such as `title-menu-visible`, `loading-display-active`, `sonic-hud-ready`, and `tutorial-hud-owner-path-ready`. SGFX can render Sonic assets as local placeholders during lab work, then swap in custom SGFX assets through explicit slots while keeping the recovered timing and state-machine shape.

About "mostly harvested": the remaining debug-menu fork deltas are useful, but they are not all immediate UI-target-routing wins. The high-value UI inspector surfaces are already harvested or mapped: CSD Manager, SWA CSD wrappers, SWA HUD owners, System/GameMode/title/input fields, Reddog operator windows, debug draw, live state, and bridge commands. Remaining debug-menu fork deltas worth future passes are `Inspire/Movie` metadata for texture/movie overlays, renderer/shader metadata for how CSD/movie passes are filtered/composited, audio/camera/system support headers, and non-Sonic HUD families. Those should feed later typed inspectors or SGFX template refinements, not be blindly enabled as mutating runtime toggles.

## Phase 123 SGFX Template-Driven Placeholder Renderer

Phase 123 wires the SGFX template catalog into the existing clean SU UI asset renderer. The renderer now accepts `--template title-menu`, `--template loading`, `--template sonic-hud`, and `--template tutorial`, selects the matching Sonic placeholder-backed screen, and overlays the active template, readiness event, and timing hook while keeping the underlying Sonic assets local-only.

The new `--sgfx-template-smoke` command proves the bridge without opening a window. It reports every template-to-renderer binding, the portable contract file, the durable readiness event, the local Sonic placeholder slots, and the first animation/timing hook from the recovered runtime template data. This is the first point where the SGFX recipes are not only described: they drive a renderer path that can show Sonic placeholders today and swap to custom SGFX art later through the same named slots.

## Phase 124 CSD-Driven Local SU UI Pipeline Viewer

Phase 124 moves the clean SU UI asset renderer from hand-placed placeholder-only recipes toward a local CSD-driven pipeline viewer. The renderer now reads the ignored local `research_uiux/data/layout_deep_analysis.json` evidence at runtime, resolves CSD package/scene/timeline facts for `title-menu`, `loading`, `sonic-hud`, and `tutorial`, and exposes those facts beside the SGFX slot mapping. Sonic assets and CSD evidence remain local-only placeholders; the tracked code only contains the loader, mappings, tests, and docs.

The new `--csd-pipeline-smoke` command reports the real recovered layout package, primary scene, cast count, subimage count, texture list, recovered animation timing, SGFX element slots, and available runtime evidence manifest comparison. Current mappings are `ui_mainmenu.yncp/mm_bg_usual`, `ui_loading.yncp/pda`, and, after Phase 135, exact normal Sonic HUD `ui_playscreen.yncp/so_speed_gauge` plus child scene `u_info` for tutorial. Runtime comparison remains the oracle; the older `ui_prov_playscreen.yncp` path is now only a legacy/proxy reconstruction screen, not the primary HUD compositor lane.

The interactive renderer also overlays `csd_pipeline=`, `sgfx_element_map=`, and `runtime_evidence_compare=` lines when launched with `--template ...`, so the operator can see which visible placeholder elements come from recovered CSD evidence and which runtime event/native BMP set is being used as proof.

## Phase 125 CSD Drawable Traversal

Phase 125 moves the local SU UI pipeline viewer from digest-level CSD facts into actual drawable traversal. The renderer now reads ignored local `layout_deep_analysis.json`, resolves the selected scene through `scene_ids`, walks `cast_groups`, `cast_dictionaries`, `cast_material.used_subimage_indices`, and `subimages`, then emits concrete draw commands with cast name, source texture, subimage index, source rectangle, destination rectangle, sampled translation/scale/rotation, and SGFX slot mapping.

The new `--csd-drawable-smoke` command exports the four current template scenes as drawable command streams: `ui_mainmenu.yncp/mm_bg_usual`, `ui_loading.yncp/pda`, exact `ui_playscreen.yncp/so_speed_gauge`, and exact `ui_playscreen.yncp/u_info`. It verifies real CSD-derived commands such as title `black3`, loading `bg`, Sonic HUD `Cast_0506_bg`, and tutorial `bar`; resolves their local DDS texture dimensions without publishing those assets; and still prints `native_bmp_compare=` lines so runtime/native evidence remains the visual oracle.

Interactive `--template ...` preview now attempts to render from the recovered CSD drawable command stream first. The older hand-placed placeholder casts remain as fallback and diagnostics, but the template lane is now CSD draw-command driven wherever local evidence and local Sonic placeholder textures exist. This is still a sidecar pipeline viewer, not a replacement for the real UnleashedRecomp runtime lane.

## Phase 126 CSD Timeline Playback

Phase 126 adds deterministic CSD keyframe sampling to the local SU UI pipeline viewer. The renderer now resolves a template's timeline scene and animation bank, reads `animation_dictionaries`, `animation_frame_data_list`, `animation_keyframe_data_list`, groups, casts, tracks, and finite numeric keyframes, then evaluates linear/const samples for the first reusable title/loading/HUD/tutorial timing anchors.

The new `--csd-timeline-smoke` command reports sampled motion at explicit frame anchors: title `mm_donut_move/DefaultAnim` at frame `10`, loading `pda_txt/Usual_Anim_3` at frame `75`, Sonic HUD `so_speed_gauge/Size_Anim` at frame `99`, and tutorial `info_1/Count_Anim` at frame `50`. It exports `csd_timeline=`, `timeline_sample=`, and, when the timeline scene matches a drawable scene, `timeline_draw_command=` lines that apply sampled transform channels back onto CSD-derived draw commands.

This phase also adds `rendered_frame_compare=` lines for each template so the sampled sidecar frame stays tied to the latest available native BMP evidence. That comparison is metadata-level for this beat: it proves the local sampled frame has a native evidence target and durable event anchor, while full pixel diffing remains a follow-on step. Fresh UnleashedRecomp runs are still required whenever we need to refresh the native oracle or prove a newly routed screen, but they are not required for every local parser/sampler iteration.

## Verification

Local-only evidence, not committed:

- `out/ui_lab_runtime_evidence/phase116_bridge_20260427_213626/`
  - launched the real UI Lab runtime
  - connected to `\\.\pipe\sward_ui_lab_live`
  - sent `state`
  - saved `bridge_state_response.json`
  - response reported `target=title-loop`, bridge started, `7` bridge commands, `15` debug-menu fork-derived typed fields, and recent JSONL events

- `out/ui_lab_runtime_evidence/20260427_213715/`
  - full early-game RGB-gated native capture passed after the bridge landed
  - `title-loop`, `title-menu`, `title-options`, `loading`, and `sonic-hud` all produced evidence-ready manifests
  - all targets produced `liveBridgeName=sward_ui_lab_live`
  - all targets produced `ui_lab_live_state.json`
  - selected native routes stayed target-aware: `title menu visual ready`, `loading display active`, and `stage target ready`

- `out/ui_lab_runtime_evidence/20260427_214440/`
  - post-rebuild title-loop native capture passed after the Live API window also displayed the bridge command list and fork-derived field count
  - `3 / 3` native BMP captures were RGB-nonblack
  - manifest reported `liveBridgeName=sward_ui_lab_live` and `bestRoute=loading display ended`

- `out/ui_lab_runtime_evidence/phase117_bridge_20260427_220101/`
  - launched the real UI Lab runtime
  - queried `state`, `help`, and `events` through `query_unleashed_recomp_ui_lab_bridge.ps1`
  - `state` reported `target=title-loop`, bridge started, and a populated `typedInspectors` block

- `out/ui_lab_runtime_evidence/20260427_222005/`
  - focused title-menu capture with `-UseLiveBridgeReadiness`
  - manifest reported `readinessSource=live-bridge`, `liveRoute=title menu visual ready`, and JSONL evidence passed
  - native BMP scoring selected `bestRoute=title menu visual ready` with `14` RGB-nonblack frames

- `out/ui_lab_runtime_evidence/20260427_221031/`
  - focused title-options capture after adding route-based live readiness
  - manifest reported `readinessSource=live-bridge`, `liveRoute=title options accept injected`, typed inspectors present, and `6` RGB-nonblack native frames

- `out/ui_lab_runtime_evidence/20260427_221250/`
  - current full early-game live-bridge sweep passed `title-loop`, `title-options`, `loading`, and `sonic-hud`
  - `title-menu` missed the route in that combined run, while the focused title-menu run above passed immediately afterward
  - treat this as the next full-sweep route-stability gap, not as a failure of the bridge client or typed inspector path

- `out/ui_lab_runtime_evidence/20260427_225000/`
  - Phase 118 full early-game live-bridge/native sweep passed `title-loop`, `title-menu`, `title-options`, `loading`, and `sonic-hud` in one combined run
  - each target used a unique pipe name such as `sward_ui_lab_live_20260427_225000_title-menu`
  - title-menu readiness came from the live bridge but also required durable JSONL `title-menu-visible`; manifest reported `durableEvidenceEvent=title-menu-visible`, `durableEvidencePassed=true`, and `bestRoute=title menu visual ready`
  - title-menu produced `14` RGB-nonblack native BMPs, keeping native capture as the visual proof path

- `out/ui_lab_runtime_evidence/phase118_route_status_<timestamp>/`
  - direct client smoke queried `route-status` through `query_unleashed_recomp_ui_lab_bridge.ps1`
  - response reported route generation/reset counters, hook-observed flags, last title/stage context details, and readiness booleans from the running runtime

- `out/ui_lab_runtime_evidence/20260427_231946/`
  - focused Phase 119 `sonic-hud` live-bridge/native capture passed
  - `typedInspectors.csdProjectTree` reported `activeProject=ui_playscreen`, `sceneCount=13`, `nodeCount=2`, and `layerCount=209`
  - `typedInspectors.sonicHud.ownerPath` resolved `m_rcPlayScreen`, `m_rcSpeedGauge`, `m_rcRingEnergyGauge`, and `m_rcGaugeFrame` to real CSD project/scene addresses while keeping the raw `CHudSonicStage` owner pointer marked pending
  - `typedInspectors.pauseGeneralSave` populated live `CGeneralWindow` and `CSaveIcon` state; pause stayed unknown because this route does not open pause

- `out/ui_lab_runtime_evidence/20260427_232129/`
  - Phase 119 full early-game live-bridge/native sweep passed `title-loop`, `title-menu`, `title-options`, `loading`, and `sonic-hud`
  - all targets used live-bridge readiness and RGB-nonblack native BMP evidence
  - the final `sonic-hud` state again reported `ui_playscreen`, `13` scenes, `209` layers, and resolved gauge scene addresses from the CSD tree

- `out/ui_lab_runtime_evidence/20260428_082902/`
  - Phase 121 focused `sonic-hud` run passed hands-off with scan-code control automation
  - manifest recorded `pulseCount=12`, `foregroundFailureCount=0`, `sendInputFailureCount=0`
  - live bridge emitted `sonic-hud-ready`; native frame summary selected a `stage target ready` RGB-nonblack BMP

- `out/ui_lab_runtime_evidence/20260428_083009/`
  - Phase 121 focused `tutorial` run passed hands-off with scan-code control automation
  - manifest recorded `pulseCount=11`, `sendInputFailureCount=0`, `tutorial-hud-owner-path-ready`, `tutorial-target-ready`, and `tutorial-ready`
  - live state kept `rawOwnerFieldsReady=false` while resolved `ui_playscreen` ownership came from the CSD project tree, matching the owner-maturation finding

- `out/ui_lab_runtime_evidence/20260428_083651/`
  - Phase 121 combined early-game live-bridge/native sweep passed `title-loop`, `title-menu`, `title-options`, `loading`, and `sonic-hud`
  - `title-menu`, `title-options`, and `sonic-hud` used foreground-verified scan-code control automation with `sendInputFailureCount=0`
  - best native routes were `title menu visual ready`, `title options accept injected`, `loading display active`, and `stage target ready`, all with RGB-nonblack BMP evidence

- `out/ui_lab_runtime_evidence/20260428_115131/`
  - Phase 133 Sonic HUD archaeology read the latest `sonic-hud/ui_lab_live_state.json` through the sidecar compare lane
  - live bridge evidence reported runtime `ui_playscreen`, `13` scenes, `2` nodes, `209` layers, and `stageReadyFrame=721`
  - owner-path evidence stayed consistent with Phase 121: raw `CHudSonicStage` owner is live, fork-header owner slots are still pending, and resolved scene ownership comes from `CCsdProject::Make`
  - local renderer diagnostics now mark `ui_prov_playscreen.yncp` as a proxy and list missing local runtime scenes instead of treating the current Sonic HUD sidecar frame as exact

- `out/ui_lab_runtime_evidence/20260428_132410/`
  - Phase 134 refreshed focused `sonic-hud` live-bridge/native evidence after widening CSD tree sample storage for exact `ui_playscreen` export
  - manifest reported `readinessSource=live-bridge`, `exitCode=0`, `6` native BMP captures, and best route `stage target ready`
  - live state reported runtime `ui_playscreen`, `13` scenes, `2` nodes, `209` runtime layers, and `203` stored layer samples
  - later Sonic HUD scene samples now survive in `typedInspectors.csdProjectTree.layers`, including `ui_playscreen/so_speed_gauge/position`
  - the sidecar export writes ignored local `out/csd_runtime_exports/phase134/ui_playscreen_runtime_tree.json`
  - note for manual operation: this capture used control automation and sent `ENTER` pulses; future manual sessions should pass `-DisableControlAutomation` so the bridge observes while the operator drives

- `extracted_assets/phase135_ui_playscreen_probe/` and `out/csd_runtime_exports/phase135/`
  - Phase 135 archive-probed the installed local game files with `HedgeArcPack`, finding exact normal Sonic HUD package `Sonic/ui_playscreen.yncp` in `Sonic.ar.00`
  - the same probe recovered local-only HUD texture companions such as `Sonic/ui_ps1_gauge1.dds`, `mat_playscreen_001.dds`, language sheets, and `SystemCommonCore` common materials
  - ignored `research_uiux/data/layout_deep_analysis.json` was regenerated so the sidecar can parse exact `ui_playscreen` scene, subimage, material, and timeline data
  - `--export-runtime-csd-materials --template sonic-hud` joins the Phase 134 live runtime tree to exact local CSD material/subimage/timeline data and writes ignored `out/csd_runtime_exports/phase135/ui_playscreen_runtime_materials.json`
  - current material export resolves `167 / 203` stored runtime layer samples; the remaining `36` are structural/group layers rather than missing Sonic HUD payload

- `out/csd_runtime_exports/phase136/`
  - Phase 136 adds `--export-sonic-hud-compositor --template sonic-hud`
  - the export writes ignored local `ui_playscreen_hud_compositor.json` with all `13` live normal Sonic HUD scenes mapped to activation events, SGFX slots, timelines, unique textures, drawable layer counts, and structural layer counts
  - the export also writes ignored local `ui_playscreen_hud_reference.hpp`, a readable C++-style reference skeleton for `CHudSonicStage`, hook `sub_824D9308`, project `ui_playscreen`, and the scene table needed for portable source reconstruction
  - this is now a full runtime-scene compositor model; shader/palette/render-order parity still needs native comparison before claiming pixel-perfect HUD recreation

- `research_uiux/runtime_reference/examples/su_ui_asset_renderer.cpp`
  - Phase 143 lets the tracked frontend reference viewer consume the latest UI Lab `ui_lab_live_state.json` snapshots directly for title-menu, loading, title-options, and pause alignment
  - the viewer now labels live-read fields separately from policy fallback fields: active screen, route/motion, frame, title cursor, loading display type, pause owner status, transition band, input-lock release, live-state path, and per-field provenance
  - `--renderer-runtime-alignment-smoke` is the bounded operator check for this path; native BMPs remain the visual proof, while live-state/bridge evidence becomes the state driver
  - Phase 144 adds a read-only direct live-bridge probe for the same viewer lanes: `queryUiLabLiveBridgeState` sends `state` to the Windows named pipe when a running UI Lab is present, accepts the result only when the response target matches the lane, and reports `direct-live-bridge` vs `snapshot-fallback` through `--renderer-live-bridge-alignment-smoke`
  - missing, stale, or target-mismatched pipes now fall back to latest `ui_lab_live_state.json` snapshots without mutating routes or SGlobals, so native BMPs stay visual proof while the bridge becomes the preferred state read path
  - Phase 145 adds the read-only `ui-oracle` bridge command as the first UI-only oracle seed. It returns active screen, active scenes, active motion/frame, cursor owner, transition band, input-lock state, and a `uiLayerOracle` block from the live runtime CSD tree; its `runtimeDrawListStatus` is intentionally `runtime CSD tree; GPU draw-list pending` until a true runtime GPU/UI draw-list capture exists.
  - `--renderer-ui-oracle-smoke` lets the sidecar viewer query that direct oracle when a matching runtime is live, then falls back explicitly to `state` or latest `ui_lab_live_state.json` snapshots. This moves comparison toward CSD-to-CSD/UI-only oracle checks while native BMPs remain visual proof.
  - Phase 146 makes the frontend viewer consume that oracle path as a playback clock. `--renderer-ui-oracle-playback-smoke` reports per-lane runtime frames and per-scene timeline frames selected from `ui-oracle` / `state` / snapshot evidence (`playback_clock=ui-oracle-runtime-frame`, `timeline_frame_source=ui-oracle-mod-frame`) instead of rendering every title/loading/options/pause lane at a fixed policy sample frame.
  - Phase 147 adds `--renderer-ui-drawable-oracle-smoke`, which derives a runtime-aligned drawable oracle from active `ui-oracle` / `state` / snapshot evidence plus local CSD material draw commands. The sidecar now reports per-lane active project, active scene count, per-scene runtime path, sampled frame, command count, sampled-track count, and texture count with `runtime_drawable_oracle_status=runtime-csd-tree-local-material`.
  - The Phase 147 drawable oracle is still not a GPU draw-list export. It keeps `gpu_draw_list_status=pending` on purpose so shader/material parity work has a clean next target instead of mistaking local CSD reconstruction for the runtime renderer's final submitted draw list.
  - Phase 148 adds a read-only `ui-draw-list` live-bridge command backed by the runtime CSD platform draw hook in `SWA::CCsdPlatformMirage::Draw` / `DrawNoTex`. The hook records sampled per-frame UI draw calls with layer address/path, cast-node address, vertex buffer, textured/no-texture status, color sample, and screen-space rectangle after the runtime aspect-ratio/UI transform path.
  - This is the first real runtime draw-list oracle, but it is still labeled precisely as `runtime CSD platform draw hook; GPU backend submit pending`. It proves what the game-side CSD platform asked to draw before the backend renderer submits it, while a deeper D3D/Xenos backend capture remains future shader-parity work.
  - Phase 149 makes the sidecar viewer consume `ui-draw-list` directly through `--renderer-ui-draw-list-triage-smoke`. When a matching runtime target is live it reports direct runtime CSD platform draw calls and rectangles beside local CSD drawable command counts; when no pipe is attached it reports local coverage with `runtime_calls=0` instead of guessing.
  - Phase 149 triage is explicitly `material_triage=runtime-rectangles-vs-local-csd` with `backend_submit_status=pending`. It is now good enough to identify scene/path/rectangle mismatches before shader work, but shader-perfect parity still needs the backend submit capture or a true UI-only rendered layer.
  - Phase 150 adds a read-only `ui-gpu-submit` live-bridge command backed by a render-thread material submit hook at the UnleashedRecomp `ProcDrawPrimitive`, `ProcDrawIndexedPrimitive`, and `ProcDrawPrimitiveUP` seams. The hook records sampled backend submit/material state such as primitive topology, indexed/inline-vertex path, texture and sampler descriptor indices, alpha blend state, blend factors, color-write mask, alpha-test threshold, scissor state, and sampler filter/wrap values.
  - Phase 150 deliberately labels this as a `render-thread material submit hook`: it is below the CSD platform draw hook and strong enough for shader/material triage, with raw D3D12/Vulkan backend capture pending. The sidecar `--renderer-gpu-submit-triage-smoke` consumes this oracle beside the existing runtime CSD rectangles and local CSD commands so material parity can move off full-backbuffer noise.
  - Phase 151 adds a read-only `ui-material-correlation` live-bridge command that correlates `ui-draw-list` rectangles with `ui-gpu-submit` material samples through a `same-frame-order-window`. The JSON emits `materialCorrelationOracle` pairs with draw/submit sequence ids, screen rects, descriptor indices, half-pixel offsets, and named Xenos/D3D-ish material semantics for blend, alpha, sampler filter, and address modes.
  - Phase 151 also seeds an RHI command-list boundary hook so each correlated sample can report whether the D3D12 or Vulkan draw boundary was observed. This is still explicitly below full raw D3D12/Vulkan command capture pending; it is a stronger oracle than full-backbuffer screenshots, but not yet a vendor command-buffer dump.

- `research_uiux/runtime_reference/include/sward/ui_runtime/sonic_hud_reference.hpp` and `src/sonic_hud_reference.cpp`
  - Phase 137 promotes the generated Phase 136 reference into hand-written repo-safe source
  - the module keeps the live-bridge provenance (`CHudSonicStage`, `sub_824D9308`, `ui_playscreen`) while exposing reusable scene activation policy, render ordering, SGFX slots, material slot descriptors, and timeline sampling
  - `sward_sonic_hud_reference_catalog.exe --phase137-smoke` verifies the portable policy table without loading or publishing Sonic CSD/DDS payloads
  - this is the reusable architecture lane for SGFX; native BMP/runtime captures remain the visual oracle and the legacy asset viewer still needs a follow-on wiring beat to use this exact model

- `research_uiux/runtime_reference/examples/su_ui_asset_renderer.cpp`
  - Phase 138 wires the interactive Sonic HUD viewer to the Phase 137 policy table
  - `SonicHudReconstruction` now renders exact local `ui_playscreen.yncp` CSD scenes according to recovered scene policy/order instead of the old hand-placed proxy HUD card
  - Sonic HUD/tutorial template launches no longer show the large SGFX placeholder/debug panels over the UI; the viewer uses a compact `phase137-ui_playscreen-policy` status overlay while native BMP/runtime captures remain the oracle
  - `--renderer-sonic-hud-reference-smoke` verifies the viewer path and keeps the `add/u_info` scene capped to the `5` live drawable layers proven by the compositor export

- `out/ui_lab_runtime_evidence/20260428_011255/`
  - focused Phase 120 `sonic-hud` live-bridge/native capture passed on the final raw-owner hook build
  - JSONL emitted `sonic-hud-owner-hooked` with `owner_fields_ready=0`, proving the raw `CHudSonicStage` owner pointer while keeping embedded CSD owner fields marked pending
  - live state reported `rawOwnerKnown=true`, `rawOwnerFieldsReady=false`, `rawHookSource=raw CHudSonicStage owner hook sub_824D9308`, and `ownerPointerStatus=raw CHudSonicStage owner hook live; CSD owner fields pending`
  - native BMP signal remained visual confirmation: `6 / 6` RGB-nonblack captures, best route `stage target ready`

- `out/ui_lab_runtime_evidence/20260428_012413/`
  - focused Phase 120 `pause` live-bridge/native capture passed with the target-aware pause timeout floor and clean runtime exit
  - manifest reported `requestedAutoExitSeconds=75`, `effectiveAutoExitSeconds=95`, `readinessSource=live-bridge`, `exitCode=0`, and no missing evidence events
  - JSONL emitted `pause-owner-observed`, `pause-route-start-injected`, `pause-target-ready`, and `pause-ready`
  - native BMP signal passed with `4 / 4` RGB-nonblack captures, `bestIndex=4`, and `bestRoute=pause target ready`
  - JSONL ended with `native-frame-capture-complete-auto-exit` after the fourth native BMP, keeping the proof session bounded
