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

SWARD ports this as a compact-on-demand operator shell with window list, counters, exports, view/debug toggles, and a foreground debug-draw layer. The windows are no longer default-open during gameplay; the bridge and typed inspectors stay enabled, while panes open from the small `UI` button so observer sessions do not bury the game under ImGui or hammer frame time.

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
  - Phase 152 adds a read-only `ui-backend-resolved` live-bridge command for backend-resolved D3D12/Vulkan submit details captured at command-list draw hooks. The JSON emits `backendResolvedSubmitOracle` with native draw command names, native pipeline/pipeline-layout handles, resolved PSO/blend/framebuffer state, render-target/depth formats, framebuffer size, topology, input-layout counts, depth state, and alpha-to-coverage state.
  - Phase 152 joins this backend-resolved draw stream with the existing material-correlation lane by `same-frame-order-window`. It is stronger than the Phase 151 RHI-boundary seed and gives shader/material parity work backend-owned PSO/blend/framebuffer facts, but sampler descriptor internals, texture-view decode, true vendor command-buffer dumps, text/movie timing, and SFX timing remain separate recovery work.
  - The first live Phase 152 proof was a bounded title-loop run: `ui-backend-resolved` reported `14` backend-resolved submits, `14` resolved-pipeline submits, `13` blend-enabled submits, `14` known framebuffer submits, and the native visual gate produced `3 / 3` RGB-nonblack BMPs. A same-build title-menu route attempt produced backend-resolved samples too but missed the post-Press-Start visual latch, so it is recorded as a routing issue rather than a material-oracle failure.
  - Phase 153 turns the Phase 152 numeric backend stream into backend-resolved PSO/blend/framebuffer material parity hints. `ui-backend-resolved` now emits `backendMaterialParityHints` plus per-submit `materialParityHint` labels such as `source-over-alpha`, `additive-alpha`, and `opaque-no-blend`, and the sidecar `--renderer-material-parity-hints-smoke` reports those counts per title/loading/options/pause lane.
  - The first live Phase 153 proof was a bounded title-loop run from `out/ui_lab_runtime_evidence/20260429_044413`: `ui-backend-resolved` reported `130` backend-resolved submits, `130` resolved-pipeline submits, `59` blend-enabled submits, `130` known framebuffer submits, `41` source-over submits, `18` additive submits, `71` opaque submits, and `130` framebuffer-registered submits, while the native visual gate produced `3 / 3` RGB-nonblack BMPs.
  - Phase 153 tightens the material lane by using backend-owned PSO/blend/framebuffer state instead of only local CSD flags, but texture-view/sampler descriptor internals remain pending and title/loading text/movie/SFX timing remains pending. Those are still required before the viewer can feel exactly like the running game.
  - Phase 154 adds runtime texture-view/sampler descriptor semantics to the `ui-backend-resolved` oracle. The runtime now records texture descriptor metadata from `SetTextureInRenderThread` / `SetSurface` and sampler descriptor state beside material submits, emits `backendDescriptorSemantics` with texture/sampler policy counts and semantic labels, and the sidecar `--renderer-descriptor-semantics-smoke` consumes those fields per title/loading/options/pause lane.
  - The first live Phase 154 proof was a bounded title-loop run from `out/ui_lab_runtime_evidence/20260429_051210`: `ui-backend-resolved` reported `14` backend-resolved submits, `11` material pairs, `11` known texture descriptors, `11` known sampler descriptors, `11` linear sampler descriptors, `11` wrap-address descriptors, `11` clamp-address descriptors, and the native visual gate produced `3 / 3` RGB-nonblack BMPs.
  - Phase 154 moves sampler/filter/address parity off local CSD guesses and onto runtime descriptor state. A raw native descriptor dump remains pending (`pending-native-descriptor-dump`), and title/loading movie/text/SFX timing remains pending before the viewer can feel exactly like the running game.
  - Phase 155 adds native RHI resource-view/sampler handle capture to the `ui-backend-resolved` oracle. The runtime now records D3D12/Vulkan texture resource handles, texture view handles, sampler handles, native formats, view dimensions, filter/address state, and correlates them back to UI material pairs through `backendVendorResourceCapture`.
  - The first live Phase 155 proof was a bounded title-loop run from `out/ui_lab_runtime_evidence/20260429_061651`: `ui-backend-resolved` reported `11` material pairs, `11` known texture resource views, `11` known sampler resource views, `11` paired resource captures, `423` observed texture resource views, `3` observed sampler resource views, and the native visual gate produced `3 / 3` RGB-nonblack BMPs.
  - Phase 155 is deliberately not advertised as a solved UI-only layer. The JSON says `uiOnlyLayerCaptureStatus=pending-runtime-ui-render-target-copy` and `nativeCommandCaptureGap=pending-full-vendor-command-buffer-dump`, so true UI-only rendered layer remains pending while shader/material parity gets a stronger vendor-resource oracle than screenshots.
  - Phase 156 uses the Phase 155 vendor-resource oracle for premultiplied alpha/gamma/sRGB resource-view parity. `ui-backend-resolved` now emits `backendMaterialResourceViewParity`, `resourceViewExactPairCount`, `srgbTextureResourceViewCount`, `premultipliedAlphaStatus`, `gammaSrgbStatus`, and `resourceViewExactnessStatus` so the sidecar can stop guessing these material details from CSD flags alone.
  - The first live Phase 156 proof was a bounded title-loop run from `out/ui_lab_runtime_evidence/20260429_090457`: `ui-backend-resolved` reported `14` backend-resolved submits, `11` material pairs, `11` exact resource-view pairs, `11` known texture resource views, `11` known sampler resource views, straight-alpha resource-view status, no sRGB-classified native resource views, and the native visual gate produced `5 / 5` RGB-nonblack BMPs.
  - Phase 156 also seeds the next oracle upgrade through `uiOnlyRenderTargetCaptureProbe`. UI-only render-target capture remains pending (`uiOnlyLayerCaptureStatus=pending-runtime-ui-render-target-copy`), but the policy is now explicit: copy the UI render target before present, then compare CSD/UI renderer output against that layer instead of the full 3D backbuffer.
  - Phase 157 adds a read-only `ui-vendor-command-capture` live-bridge command for a vendor command/resource dump. The runtime joins raw backend command samples, backend-resolved submits, and native texture/sampler resource-view pairs into `vendorCommandResourceDump` with `vendorCommandResourceDumpPolicy=raw-backend-command-plus-resource-view-dump`, while the sidecar `--renderer-vendor-command-resource-dump-smoke` reports the same fields per title/loading/options/pause lane.
  - The first live Phase 157 proof was a bounded title-loop run from `out/ui_lab_runtime_evidence/20260429_100709`: `ui-vendor-command-capture` reported `103` raw backend commands, `128` backend-resolved submits, `25` exact resource pairs, `555` texture resource-view samples, and `7` sampler resource-view samples, while the native visual gate produced `25 / 25` RGB-nonblack BMPs.
  - Phase 157 is deliberately still not a solved UI-only layer. UI-only render-target copy remains pending (`uiOnlyRenderedLayerStatus=pending-runtime-ui-render-target-copy`), and full vendor command-buffer replay remains pending (`vendorCommandReplayGap=pending-full-vendor-command-buffer-replay`). This gives material/shader parity a stronger raw command/resource oracle without pretending the runtime UI layer has already been copied or replayed.
  - Phase 158 starts the UI render-target capture lane. The live bridge exposes read-only `ui-layer-status` plus mutating `ui-layer-capture`; the render thread queues an active render-target readback after pending stretch rects and before ImGui/present, and the sidecar `--renderer-ui-layer-capture-smoke` reports the capture path/status beside local CSD command counts.
  - The first live Phase 158 proof was a bounded title-loop run from `out/ui_lab_runtime_evidence/20260429_105319`: `ui-layer-capture` produced `ui_layer_render_target_title-loop_1_1582x853.bmp`, `ui-layer-status` reported `active-render-target-copy-before-present`, `uiOnlyLayerIsolationStatus=not-isolated-active-color-target`, `27` runtime UI draw calls, `105` runtime GPU submit samples, and the native visual gate produced `3 / 3` RGB-nonblack BMPs.
  - This is still intentionally conservative: the capture source is `active-render-target-before-imgui-present`, and the metadata can report `uiOnlyLayerIsolationStatus=not-isolated-active-color-target` because the active render target may still include scene/background pixels. True isolated UI-only layer capture or full vendor command-buffer replay remains the next oracle upgrade if the runtime has no dedicated UI target to copy.
  - Phase 159 adds UI-layer pixel comparison in the sidecar. `--renderer-ui-layer-pixel-compare-smoke` renders the tracked title/loading/options/pause policies, searches local evidence for `ui_layer_render_target_<target>_*.bmp`, writes `out/ui_layer_pixel_compare/phase159/ui_layer_pixel_compare_manifest.json`, and reports per-lane pixel deltas or an explicit missing-capture state.
  - The first Phase 159 local proof refreshed a loading run in `out/ui_lab_runtime_evidence/20260429_113357`: `ui-layer-capture` produced `ui_layer_render_target_loading_1_1582x853.bmp`, and the sidecar compared the tracked loading policy against it with `mean_abs_rgb=29.486`, `max_abs_rgb=255`, `72` local CSD commands, and `ui_layer_capture_isolation=not-isolated-active-color-target`.
  - Phase 159 is the compare harness for the next oracle upgrade, not the final oracle itself. If a lane reports `missing`, the next proof must either route/capture that screen through `ui-layer-capture` or move to a dedicated UI target or vendor replay; title/loading text/movie/glyph/fade/SFX timing remains `pending-title-loading-media-timing`.
  - Phase 160 promotes UI-layer capture into the capture helper itself. `capture_unleashed_recomp_ui_lab.ps1` now accepts `-UiLayerCapture` and optional `-RequireUiLayerCapture`, arms `ui-layer-capture` immediately after live/JSONL readiness, polls `ui-layer-status`, and records `uiLayerCaptureAttempted`, `uiLayerCapturePassed`, `uiLayerCaptureRequest`, and `uiLayerCaptureStatus` in the local manifest.
  - The first Phase 160 sweep filled the missing frontend UI-layer compare lanes: `title-menu` in `out/ui_lab_runtime_evidence/20260429_120418`, `title-options` in `out/ui_lab_runtime_evidence/20260429_120324`, and `pause` in `out/ui_lab_runtime_evidence/20260429_120533` all reported `ui-layer-capture-observed`, `active-render-target-copy-before-present`, and `not-isolated-active-color-target`.
  - After that sweep, `--renderer-ui-layer-pixel-compare-smoke` reported found UI-layer captures for all four tracked frontend policies: title-menu `mean_abs_rgb=36.833`, loading `29.486`, title-options `34.177`, and pause `63.304`, all with `max_abs_rgb=255`. This proves the compare harness is operating across the full title/loading/options/pause set, while still explicitly requiring a dedicated UI target or vendor replay before claiming isolated UI-only pixels.
  - Phase 161 promotes the title/loading media timing gap into tracked frontend reference data instead of leaving it as a blank pending box. `frontend_screen_reference` now carries `FrontendScreenMediaCue` entries for title movie/fade/text/glyph/SFX timing and loading fade/text/glyph/SFX timing, with visual cues sourced from runtime UI-layer capture plus recovered CSD timeline policy.
  - `sward_frontend_screen_reference_catalog --phase161-media-smoke` is the bounded proof path. It reports title-menu media counts (`movie=1`, `text=1`, `glyph=1`, `fade=1`, `sfx=2`) and loading media counts (`movie=0`, `text=1`, `glyph=1`, `fade=2`, `sfx=2`), including cue-level SGFX slots such as `title_backdrop_movie`, `prompt_glyphs`, `loading_copy`, and `controller_variant`.
  - This is not yet full runtime audio/movie playback. SFX cues are timed from route/readiness latches but marked `audio-id-pending`; exact audio bank IDs, mixer routing, localized text/glyph selection, movie decode/playback, and final premultiplied/gamma/sRGB tuning still need the stronger isolated UI target or vendor replay oracle.
  - The sidecar UI-layer pixel compare now reports `text_movie_sfx_status=title-loading-media-timing-reference-ready-audio-id-pending`, which is the intended middle state: visual title/loading media timing is reference-ready for SGFX template work, while exact audio identity remains unresolved.
  - Phase 162 adds a local media asset readiness probe for those cues. `formatFrontendScreenMediaAssetProbeCatalog` resolves repo-local Sonic placeholder evidence without publishing assets: `game/movie/evmo_title_loop.sfd` through the installed game folder, title movie preview PNGs through `extracted_assets/runtime_previews/title/`, title menu DDS through `extracted_assets/ui_frontend_archives/MainMenu/`, loading DDS through `extracted_assets/ui_frontend_archives/Loading_English/` and `ui_extended_archives/Loading/`, and prompt glyph DDS through the recovered ActionCommon paths.
  - The bounded checks are `sward_frontend_screen_reference_catalog --phase162-media-asset-smoke` and `sward_su_ui_asset_renderer --renderer-media-asset-readiness-smoke`. Current local readiness is title-menu `resolved=4`, `preview=1`, `playback_ready=3`, `decode_pending=1`, `audio_pending=2`; loading `resolved=4`, `preview=0`, `playback_ready=4`, `decode_pending=0`, `audio_pending=2`.
  - The interpretation is intentionally strict: DDS-backed text/glyph/fade placeholders are ready for sidecar playback, the title SFD is found and has preview frames but still needs a real movie decoder/playback path, and all SFX cues remain timing-only until exact audio IDs/banks/mixer behavior are recovered.
  - Phase 163 starts the native readable source reconstruction lane directly. `frontend_screen_controllers.hpp/.cpp` introduces `TitleMenuController`, `LoadingScreenController`, `OptionsMenuController`, and `PauseMenuController` as hand-written portable C++ controller classes over the tracked `frontend_screen_reference` policy, rather than more viewer-only diagnostics.
  - `sward_frontend_screen_controller_catalog --phase163-smoke` emits frame-by-frame controller snapshots: screen id, frame, state, motion, input lock, active scenes, cursor label, SFX hook, and route target. The initial sequence covers title press-start/menu selection (`NEW_FILE`, `CONTINUE`, `SETTINGS`, `DLC`, `EXIT`), loading visibility, options return, and pause resume routing.
  - This is the source shape intended for reuse outside Sonic: controller classes own state, transitions, input locks, scene activation, and SFX hook points; Sonic CSD/DDS/SFX data remain placeholder/proof inputs. The next natural promotion is a `SonicDayHudController` over the existing `sonic_hud_reference` / `ui_playscreen` compositor policy.
  - Phase 164 promotes that next controller family. `SonicDayHudController` now consumes the tracked `sonic_hud_reference` scene policy and exposes native-style controller frames for `hud-bootstrap`, `hud-ready`, `tutorial-ready`, and `ring-feedback`.
  - `sward_frontend_screen_controller_catalog --phase164-sonic-hud-smoke` reports the real recovered owner provenance (`CHudSonicStage`, `sub_824D9308`, `ui_playscreen`), scene/layer counts (`13` scenes, `209` runtime layers, `167` drawable layers), and active-scene stacks for normal stage HUD, tutorial prompt, and ring pickup feedback. This is not yet full gameplay HUD source, but it is the first hand-written normal Sonic HUD controller skeleton driven by the exact `ui_playscreen` compositor policy.
  - Phase 165 adds the gameplay value model that the controller lane was missing. `SonicDayHudGameplayState` carries rings, score, timer frames, speed, boost, ring-energy, life count, tutorial prompt id/visibility, route event, and SFX hook state; `SonicDayHudValueProvenance` ties those visible fields to `ui_playscreen/ring_count`, `score_count`, `time_count`, `so_speed_gauge`, `so_ringenagy_gauge`, `player_count`, and `add/u_info`.
  - `sward_frontend_screen_controller_catalog --phase165-sonic-hud-state-smoke` intentionally labels the numeric gameplay values as `host/live-bridge supplied value` with `live-bridge-value-port-pending`. The controller now has the native source shape for HUD data flow, but exact ring/score/timer/speed/boost/energy/life memory bindings are still a live bridge/runtime hook task, not a screenshot inference.
  - Phase 166 adds the first runtime value binding contract for that model. The live bridge now emits `typedInspectors.sonicHud.gameplayValues`, with per-field `known` flags and source labels for ring count, score, timer frames, speed, boost, ring-energy, life count, tutorial prompt id/visibility, and audio ids.
  - Score is the first runtime-bound gameplay value: it is sourced from `SWA::CGameDocument::m_pMember->m_ScoreInfo.EnemyScore+TrickScore`, matching the existing UnleashedRecomp score preservation hook. The other Sonic Day HUD value ports are intentionally not guessed: ring/speed/boost/energy/life/tutorial IDs remain pending-runtime-field until exact owner/player offsets are proven.
  - `sward_frontend_screen_controller_catalog --phase166-sonic-hud-runtime-binding-smoke` exercises the controller-side binding path. Known score values update `SonicDayHudController`, while pending values preserve the prior state and keep `value_source=typedInspectors.sonicHud.gameplayValues`; Sonic HUD SFX IDs remain audio-id-pending unless a runtime callsite proves the exact cue.
  - Phase 167 promotes the exact Sonic HUD display-owner paths that the debug-menu fork and generated PPC constructor agree on. The raw `CHudSonicStage` hook now reports `m_rcScoreCount/m_rcTimeCount/m_rcPlayerCount` beside the existing play-screen, speed-gauge, ring-energy, and gauge-frame owners, while the live bridge also resolves `ui_playscreen/ring_count`, `ui_playscreen/score_count`, `ui_playscreen/time_count`, `ui_playscreen/so_speed_gauge`, `ui_playscreen/so_ringenagy_gauge`, `ui_playscreen/player_count`, and `ui_playscreen/add/u_info` from the runtime CSD tree.
  - Phase 167 also adds the remaining exact `SScoreInfo` fields that are available today: `ScoreInfo.PointMarkerRecordSpeed` and `ScoreInfo.PointMarkerCount`. These are exported as score-info evidence, not mislabeled as the live HUD speed/ring counter, because ring/timer/speed/boost/energy/lives/tutorial gameplay numerics remain pending until a real player/HUD value callsite or offset is proven.
  - Phase 168 hooks the real CSD text write sink `CNode::SetText/sub_830BF640` and maps the guest node address back through the runtime `ui_playscreen` CSD tree. This promotes `ring_count/num_ring`, `time_count/time001`, `time_count/time010`, `time_count/time100`, `add/speed_count/position/num_speed`, and `player_count/player` from display-owner paths into live display-write observations.
  - Phase 168 still keeps the boundary honest: ring, timer, speed, and lives are now `ring/timer/speed/lives:known-via-csd-text-write`, while boost/energy/tutorial remain pending until gauge/prompt setter callsites are proven. The bridge emits `sonic-hud-value-text-write` and `sonic-hud-value-write-update` evidence rather than pretending these are original gameplay storage offsets.
  - Phase 169 folds the manual-control observer session back into tracked source. The live bridge `ui-draw-list manual observer` path observed real `ui_playscreen` material-correlation coverage for speed gauge and gauge-frame CSD paths, plus later `ui_pause` overlay draw calls, while `sonic-hud-value-text-write` did not fire in that window. The controller source now exposes `SonicDayHudRuntimeDrawListCoverage` so this fallback is explicit instead of screenshot-driven.
  - Phase 169 also installs the next runtime hook set for gauge/prompt writes: `CNode::SetPatternIndex/sub_830BF300`, `CNode::SetHideFlag/sub_830BF080`, and `CNode::SetScale/sub_830BF090`. These emit `sonic-hud-gauge-pattern-write`, `sonic-hud-gauge-hide-write`, and `sonic-hud-gauge-scale-write` against `ui_playscreen/so_speed_gauge`, `ui_playscreen/gauge_frame`, `ui_playscreen/so_ringenagy_gauge`, and `ui_playscreen/add/u_info`, keeping boost/energy/tutorial status at `csd-node-pattern-hide-scale-hooks-installed-with-unresolved-write-probe-pending-runtime-normalization` until a rebuilt runtime proves exact value mapping.
  - Phase 170 closes the silent-drop gap found by the manual Phase 169 observer run. The runtime now records `sonic-hud-node-write-unresolved` whenever HUD-like numeric `CNode::SetText`, `SetPatternIndex`, `SetHideFlag`, or `SetScale` fires while `ui_playscreen` is actively drawing but the guest node address cannot yet be resolved to a `ui_playscreen/...` CSD tree path. Live-state observations now carry `pathResolved`, so the next manual session can distinguish "hook fired but path unresolved" from "this setter family is not the active gameplay write path".
  - Phase 171 starts late-resolving those anonymous writes instead of leaving them as dead-end addresses. The bridge now attempts to late-resolve unresolved CSD node writes against recent `ui_playscreen` draw-list entries by matching the raw node address to layer/cast-node addresses, updates observations with `pathResolutionSource=ui-draw-list-late-resolve`, and emits `sonic-hud-node-write-late-resolved` when a real path such as `so_speed_gauge`, `so_ringenagy_gauge`, `gauge_frame`, or `add/u_info` is recovered. Once named, scale/pattern/hide writes can feed boost, ring-energy, and tutorial prompt gameplay ports without pretending the earlier player-storage offsets are solved.
  - Phase 172 follows the live manual evidence instead of forcing another screenshot pass. The unresolved Phase 171 writes proved that `CNode::SetText` often targets child text nodes while `ui-draw-list` exposes parent cast/layer nodes. The runtime now hooks `sub_830BCCA8` and `sub_830BA228` to recover the CSD child lookup chain (`parent path + child name -> node pointer`) and wraps `sub_824D6048`, `sub_824D6418`, `sub_824D69B0`, `sub_824D6C18`, and `sub_824D7100` with a `CHudSonicStage update context`.
  - The first Phase 172 manual run also proved the raw `CHudSonicStage` value update hooks mature `m_rcPlayScreen`, `m_rcSpeedGauge`, `m_rcRingEnergyGauge`, `m_rcGaugeFrame`, `m_rcScoreCount`, `m_rcTimeCount`, `m_rcTimeCount2`, `m_rcTimeCount3`, and `m_rcPlayerCount` to non-null real addresses. The bridge now resolves timer/life text writes directly through those owner fields with `pathResolutionSource=raw-chud-sonic-stage-owner-field`, while child lookup fallback can emit `sonic-hud-node-source-owner-resolved` with `pathResolutionSource=csd-child-lookup-chain`, `sourceOwnerAddress`, and `sourceOwnerOffsetFromUpdateOwner`.
  - Phase 173 starts carrying those live `sonic-hud-value-text-write` observations into reusable controller source. `SonicDayHudController::applyRuntimeTextWrite` consumes the bridge payload shape (`value`, `path`, `text`, `pathResolutionSource`, `source`) and updates native HUD gameplay state from the same CSD text sinks, while `sward_frontend_screen_controller_catalog --phase173-sonic-hud-live-text-write-smoke` proves timer/life writes resolved by `raw-chud-sonic-stage-owner-field` can drive the controller without inventing player-storage offsets.
  - Phase 174 adds a higher-level `sonic-hud-update-callsite-sample` oracle for manual observer windows where CNode::SetText does not fire. The `CHudSonicStage` wrappers now sample pre/post original owner fields at the update-callsite boundary, including owner +452/+456 candidates from `sub_824D6048`, owner +460/+480 gauge/counter candidates from `sub_824D6C18`, and the shared delta/register context for `sub_824D6418`, `sub_824D69B0`, and `sub_824D7100`. This keeps the next value-normalization step rooted in generated PPC/runtime owner fields instead of screenshots or guessed player offsets.
  - Phase 175 promotes those callsite samples into reusable native controller source. `SonicDayHudRuntimeCallsiteSample` and `classifySonicDayHudRuntimeCallsiteSample` classify `sub_824D6048` as a generated-PPC timer text-write source (`owner+456/+452 -> CSD::CNode::SetText`) and classify `sub_824D6418` / `sub_824D6C18` as speed and rolling counter/gauge candidates until further runtime normalization proves exact boost/energy semantics. `SonicDayHudController::applyRuntimeCallsiteSample` can now drive timer state from that runtime-proven callsite while keeping ring/speed/lives on the CSD text-write path and boost/energy/tutorial explicitly pending normalization.
  - Phase 176 mirrors that classification back into the live bridge. The runtime now emits `sonic-hud-callsite-value-classified`, promotes `sub_824D6048` post-original samples into `typedInspectors.sonicHud.gameplayValues.elapsedFrames` with `timer:runtime-proven-via-chud-update-callsite-sample`, and keeps `sub_824D6418` / `sub_824D6C18` / `sub_824D7100` as classified candidates. That means Codex/operator tools can read the timer source live from the running runtime; boost/energy/tutorial remain classified candidates pending normalization.
  - Phase 177 keeps the latest classified callsite result readable after transient HUD update windows pass. The live state now exposes `typedInspectors.sonicHud.gameplayValues.lastClassifiedCallsiteValue`, so durable JSONL evidence and the latest live-state snapshot both carry the last value name, hook, status, source, normalized value, and frame. This prevents operator tools from missing a real callsite classification just because the current frame has moved back to a non-HUD/null-owner snapshot.
  - Phase 178 fixes the manual gameplay observer cost found in the 7 FPS in-game session. The visible layer is now a compact-on-demand operator overlay by default, retaining the live bridge and typed inspectors while hiding window-list/inspector/counter/view/export/debug/stage/live panes until requested. Sonic HUD update-callsite evidence now uses a stable HUD callsite signature plus a minimum JSONL interval, so volatile delta/floating owner timers update in memory for direct pipe reads without forcing per-frame durable writes or live-state snapshots. Phase 178 also hooks the generated-PPC sub_8251A568 return inside the scoped `sub_824D6418` speed update path, promoting `speed:runtime-proven-via-sub_8251A568-return` into `typedInspectors.sonicHud.gameplayValues`; boost, ring-energy, tutorial prompt, and exact SFX/audio IDs remain honest pending lanes until their setter/audio callsites prove exact identities.
  - Phase 179 makes the default operator view match the native `DrawProfiler()` workflow instead of filling the screen with separate floating debug panes. The runtime now draws a single profiler-style SWARD operator panel with a frame-time graph plus Runtime/HUD/Capture/Panels tabs, while the older counter/view/export/debug-draw/stage/live windows remain available as focused drill-downs from the Panels tab.
  - Phase 179 also keeps F1 clean: F1 remains the native Recomp Profiler toggle, and the SWARD operator panel no longer reserves or toggles on F1. To protect gameplay FPS, the raw `CHudSonicStage` owner hook no longer calls transient `RCPtr::Get()` during value-update hooks, and broad HUD update-callsite archaeology uses low-overhead Sonic HUD callsite sampling instead of durable JSONL/live-state churn every frame.
  - Phase 180 promotes the live Sonic HUD gauge/prompt setter payload shape into reusable native controller source. `SonicDayHudRuntimeGaugePromptWriteObservation` and `SonicDayHudController::applyRuntimeGaugePromptWrite` consume `sonic-hud-gauge-scale-write`, `sonic-hud-gauge-pattern-write`, and `sonic-hud-gauge-hide-write` observations for `ui_playscreen/so_speed_gauge`, `ui_playscreen/so_ringenagy_gauge`, and `ui_playscreen/add/u_info`, turning boost, ring-energy, and tutorial prompt state into `runtime-proven-via-csd-gauge-prompt-write` controller inputs while keeping exact SFX/audio IDs pending.
  - Phase 181 arrow-key automation widens the capture helper's bounded input automation to the full mapped control set the user provided: `ENTER/W/A/S/D/Q/E/UP/DOWN/LEFT/RIGHT`. The helper now records those supported keys in the automation manifest, reports the full key set when automation is enabled, and exposes a `gameplay-sweep` plan (`ENTER`, arrow movement, `Q`, `E`) for unattended observer sessions where light movement/menu navigation is needed while live bridge plus native BMP remain the oracle. Unlike route-only plans, `gameplay-sweep` continues through the post-evidence settle window after a stage/HUD readiness latch so the runtime can exercise HUD setter/audio paths after the screen is already proven visible.
  - Phase 182 adds the manual Sonic HUD value observer summarizer at `research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1`. It reads a specific `ui_lab_events.jsonl` or the newest one under an evidence directory and reports `sonic-hud-value-text-write`, `sonic-hud-gauge-scale-write`, `sonic-hud-gauge-pattern-write`, `sonic-hud-gauge-hide-write`, `sonic-hud-node-write-late-resolved`, `sonic-hud-value-write-update`, `sonic-hud-gameplay-values`, and `sonic-hud-callsite-value-classified` counts plus proven `ui_playscreen` paths/values/sources. This is meant for manual gameplay observer runs: the user drives into real HUD gameplay, then the tool tells us immediately which native HUD value paths are proven and which remain pending.
  - Phase 183 extends that tool into a manual unresolved node resolver for the current Sonic HUD gap. It now groups unresolved Sonic HUD node candidates from `sonic-hud-node-write-unresolved` evidence by guest CSD node address, reports write counts, write kinds, unique raw values, frame ranges, source callsites, and candidate labels such as `numeric-text-counter-candidate` or `gauge-or-prompt-candidate`. This keeps manual observer sessions useful even when the live runtime proves setter activity before the child lookup / draw-list resolver can name the exact `ui_playscreen/...` path.
  - Phase 184 embeds SWARD operator readouts into the native Recomp Profiler instead of spawning a second profiler-like window by default. The new `DrawProfilerAddon()` section exposes target/route/live-bridge/UI-layer status, Sonic HUD binding summaries, resolved-vs-unresolved HUD node-write counts, focused Capture/HUD tabs, the `SGlobals HUD/render switches` (`ms_IsRenderHud`, `ms_IsRenderGameMainHud`, `ms_IsRenderHudPause`), and a `Legacy floating panes` opt-in for the older windows. Phase 184 also promotes exact `ui_playscreen/score_count/score` and fallback `ui_playscreen/score_count/num_score` CSD text sinks so anonymous Sonic HUD text writes can become a named score value when those paths are resolved at runtime.
  - Phase 185 detaches SWARD from the native profiler for practical gameplay observation. F1 remains the native Recomp Profiler, F2 toggles SWARD UI Lab as a separate profiler-style panel, and the panel now carries its own `HUD Switches` tab for `SGlobals HUD/render switches`. ms_IsRenderHud is the whole UI/UX render gate, while `ms_IsRenderGameMainHud` and `ms_IsRenderHudPause` isolate the in-game HUD and pause lanes; this makes them useful parent/child render-gate evidence for later readable controller/source recovery, not a source-code replacement by themselves.
  - Phase 186 restores the old embedded-profiler SWARD UI Lab content inside the detached F2 panel instead of putting it back into the native F1 Profiler. The same old `SWARD UI Lab` tab now reports target/route/live-bridge/UI-layer state with the familiar profiler-addon tab bar, while `hudRenderGateCorrelation` exports the `ms_IsRenderHud / ms_IsRenderGameMainHud / ms_IsRenderHudPause` render-gate state, known caller groups (`frontend_listener.cpp`, `options_menu.cpp::SetOptionsMenuVisible`, `CHudPause_patches.cpp`), and unresolved ui_playscreen node writes. The runtime emits `sonic-hud-render-gate-correlated` whenever those gates or unresolved write counts change, so the switches become a live isolation oracle for the remaining anonymous Sonic HUD node writes.
  - Phase 187 moves F2 closer to the OG Profiler surface with the native Profiler font and ImPlot frame-time style while keeping the SWARD UI Lab as its own detached tab/panel. It also emits `sonic-hud-node-write-callsite-correlated` so unresolved Sonic HUD node writes can be labeled against nearby generated-PPC HUD callsite samples as `timer/speed/boost-ring-energy/tutorial` candidates before final `ui_playscreen/...` path resolution.
  - Phase 188 promotes those correlated writes into explicit semantic path candidates without pretending they are exact child-node resolutions. Unresolved `timer/speed/boost-ring-energy/tutorial` writes can now emit `sonic-hud-node-write-semantic-path-candidate` with stable `ui_playscreen/...` candidates such as `ui_playscreen/add/speed_count/position/num_speed`, `ui_playscreen/so_speed_gauge`, `ui_playscreen/so_ringenagy_gauge`, and `ui_playscreen/add/u_info`, while the summarizer reports those semantic path candidates separately from true resolved paths.
  - Phase 189 hardens manual runtime launching so SWARD UI Lab cannot accidentally boot the installer/config flow. `launch_unleashed_recomp_ui_lab_manual.ps1` defaults to the real `Unleashed Recomp - Windows (Complete Installation) 1.0.3` folder, copies the patched build plus matching DXC/D3D12 runtime DLLs into an ignored sidecar runtime, launches from the Complete Installation root, and always passes `--use-cwd` so UnleashedRecomp keeps the real game folder as its current directory.
  - `sward_frontend_screen_controller_catalog --phase167-sonic-hud-runtime-field-path-smoke` reports the split cleanly: `score:known`, `scoreinfo:known`, and `ring/timer/speed/boost/energy/lives/tutorial:pending-runtime-player-offsets`, with display-owner paths ready for the next exact numeric hook pass.

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
