<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# UnleashedRecomp UI Lab Pivot

Phase 94 pivots SWARD away from treating the clean asset renderer as the parity target. The primary 1:1 lane is now an **UnleashedRecomp UI Lab**: a debug-mode executable path that runs the real UnleashedRecomp runtime, real game files, real CSD/material/movie/render stack, and selected translated-PPC-backed UI states.

The existing `sward_su_ui_asset_renderer.exe` remains useful, but only as a diagnostic sidecar for asset inspection, CSD correlation, and readable reconstruction experiments. It must not be treated as the final Sonic Unleashed UI renderer because it bypasses too much of the actual game runtime.

## Why The Pivot Happened

The clean renderer can draw local DDS, SFD frame, and CSD-derived evidence, but it still has to approximate behavior that the game already executes. UnleashedRecomp already contains the real UI render substrate:

- `Chao::CSD::CScene::Render` and `Chao::CSD::Scene::Render` hooks
- `SWA::CCsdPlatformMirage::Draw` and `DrawNoTex` hooks
- `Hedgehog::MirageDebug::CPrimitive2D` primitive hooks
- real `ui_loading` and `ui_title` CSD modifier paths
- real title-state update wrappers around translated PPC functions
- real movie and CSD shaders in the runtime GPU layer

That means parity work should attach to that substrate instead of reimplementing it screen by screen.

## First Runtime Attachment

This phase adds `UnleashedRecomp/patches/ui_lab_patches.*` as the first SWARD-owned UI Lab module inside the UnleashedRecomp runtime layer.

The module currently provides:

- `--ui-lab`
- `--ui-lab=<target>`
- `--ui-lab-screen <target>`
- `--ui-lab-screen=<target>`
- `--ui-lab-evidence-dir <dir>`
- `--ui-lab-evidence-dir=<dir>`
- `--ui-lab-auto-exit <seconds>`
- `--ui-lab-auto-exit=<seconds>`
- `--ui-lab-observer`
- `--ui-lab-overlay off`
- `--ui-lab-overlay=off`
- `--ui-lab-hide-overlay`
- `--ui-lab-route-policy input`
- `--ui-lab-route-policy direct-context`
- a curated target table for `TitleLoop`, `TitleMenu`, `Loading`, `SonicHud`, `ExtraStageHud`, `Result`, `Status`, `Tutorial`, and `WorldMap`
- runtime config overrides that keep the lab path debug-friendly (`ShowConsole`, `SkipIntroLogos`, `DisableAutoSaveWarning`)
- attachment points in `CTitleStateIntro::Update` and `CTitleStateMenu::Update`

This is intentionally not a fake renderer. It is the entry point for driving the real runtime.

## Current Boundary

Implemented now:

- CLI/runtime flag surface for the UI Lab
- initial target taxonomy tied to CSD scene names and recovered source-family ownership
- title intro/menu update hooks that prove the lab can attach to live translated runtime states
- visible ImGui overlay drawn inside the real UnleashedRecomp frame path, after the existing runtime UI draw calls
- overlay target controls for cycling the runtime target table without changing command-line arguments
- overlay route controls and route status for forcing selected UI Lab targets through the real title/menu runtime path
- title-loop to title-menu forcing by injecting the real translated title-state accept input after the intro state is live
- title-menu to loading forcing by pinning the translated title menu cursor to New Game and injecting the real accept input once
- stage-required targets now route through the same real title/menu/loading path before relying on stage-context observation
- title-menu accept suppression so a `title-menu` capture reaches the menu without immediately carrying the injected intro accept into the next flow
- stage-context harness arming via `--ui-lab-stage <token>` / `--ui-lab-stage=<token>` and observation of the real `CGameModeStage::ExitLoading` point for Sonic HUD/tutorial/result targets
- startup update/save/achievement prompt blockers bypassed during UI Lab runs so debug-state inspection is not interrupted by frontend modal checks
- runtime evidence logging to local JSONL, including route requests, first presented frame, periodic presented frames, title/menu hook attachment, accept injection, stage-context observation, manual evidence markers, and auto-exit
- loading-route evidence from `SWA::Message::MsgRequestStartLoading::Impl` and `SWA::CLoading::Update`, including display-type transitions
- CSD project creation evidence from the real `CCsdProject::Make` hook path, so target runs now report project names such as `ui_loading`, `ui_title`, `ui_status`, `ui_result`, `ui_worldmap`, and gameplay HUD projects when they load
- corrected gameplay HUD target separation: normal Sonic HUD now targets the real runtime `ui_playscreen` project, while `ui_prov_playscreen` is tracked as an `ExtraStageHud` / Extra-Tornado-family target instead of being treated as Sonic's regular stage HUD
- target-CSD observation state in the UI Lab overlay and JSONL evidence, including `target-csd-project-made` and `stage-target-csd-bound` events when the selected CSD project is created after a real stage context is observed
- `CGameModeStage::ExitLoading` evidence now records the guest stage context address plus selected target, requested stage token, target CSD name, and whether the target CSD has already been observed
- a local capture helper at `research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1` that launches the generated runtime, normalizes the window, prefers `PrintWindow` capture before screen-copy fallback, captures screenshots, supports long observation snapshots, supports a `-KeepRunning` manual-operator mode, and writes a manifest under ignored `out/ui_lab_runtime_evidence/`
- a passive observer launch path via `--ui-lab-observer` / capture-helper `-Observer`, which records the normal runtime without route forcing or lab-only startup prompt bypasses
- optional overlay hiding via `--ui-lab-overlay off` / capture-helper `-HideOverlay`, so manual evidence captures can show the real game frame cleanly while JSONL evidence continues in the background
- direct title-intro state requests from the translated `sub_825811C8` field contract (`context+0x180` requested state and `context+0x181` dirty flag), used by the experimental direct-context policy instead of synthetic Start at the first route gate
- direct title-menu entry via the real title CSD completion byte (`CSD scene +84`) while keeping the title owner-output bridge (`title context +0x1D1`) off for menu-only routes
- owner-output arming remains enabled for loading/stage-required targets, where the intended behavior is to leave the title owner and enter the real loading path
- title-menu context evidence from the translated runtime state, including context owner/pointer fields, phase/flag fields, and title-menu cursor/transition/select booleans
- an experimental direct-context route policy that latches `CTitleMenu` cursor/selection/transition fields for title-menu-to-loading routing without synthetic A/Start input; the default remains `input` until captures prove the direct path across more screens
- generated-clone sync in `build_unleashed_recomp_ui_lab.ps1`, so tracked UI Lab files are copied into `local_build_env/ur103clean` before each build
- regression tests that guard the runtime-lab contract

Verification note:

- The repo-root `UnleashedRecompLib` does not currently contain generated PPC/shader output, so the tracked root cannot build the full runtime directly.
- The generated clone under `local_build_env/ur103clean` does contain `261` `ppc_recomp.*.cpp` files and `shader_cache.cpp`.
- The Windows build is now unblocked locally by mounting `local_build_env/ur103clean` on a short drive letter, using the Visual Studio Build Tools developer environment, adding `C:\Program Files\LLVM\bin` to `PATH`, and passing the resolved vcpkg `dxil.dll` path into CMake.
- The reproducible helper for this is `research_uiux/runtime_reference/tools/build_unleashed_recomp_ui_lab.ps1`.
- `W:\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe` builds successfully from the generated clone after the local SDL Clang 22 shim patch and tracked `runtimeobject` link fix.
- A smoke launch from the complete installation root with `--use-cwd --ui-lab-screen title-menu` stayed alive past 8 seconds and was stopped cleanly after the smoke window.
- Static regression coverage guards the UI Lab source/module/CLI/hook/state-forcing contract.
- `capture_unleashed_recomp_ui_lab.ps1` produced local-only screenshots and `ui_lab_events.jsonl` files for title/menu/loading/stage-target runs from the built generated clone.
- A longer loading capture produced local-only observation snapshots plus CSD/loading/frame events under `out/ui_lab_runtime_evidence/`, including `csd-project-made`, `target-csd-project-made`, `loading-requested`, `loading-display-active`, and `loading-display-ended`.
- The direct-context policy is now available for focused captures through `capture_unleashed_recomp_ui_lab.ps1 -RoutePolicy direct-context`.
- A `title-menu` direct-context capture under `out/ui_lab_runtime_evidence/20260427_061744/` proved the corrected route: `title-intro-direct-state-applied requested_state=1 dirty=1 transition_armed=1 output_armed=0 csd_complete_armed=1`, followed by `title-menu-attached`, `title-menu-context`, and `title-menu-reached`.
- A `loading` direct-context capture under `out/ui_lab_runtime_evidence/20260427_062209/` proved the loading route still uses the owner-output bridge intentionally: `output_armed=1`, followed by `loading-requested display_type=8`.
- A `sonic-hud` / `extra-stage-hud` direct-context capture under `out/ui_lab_runtime_evidence/20260427_074626/` corrected the gameplay-HUD split:
  - `sonic-hud` targets `ui_playscreen`, observed `CGameModeStage::ExitLoading`, then produced `target-csd-project-made detail="ui_playscreen"` and `stage-target-csd-bound detail="target_csd=ui_playscreen stage_context=1"`.
  - `extra-stage-hud` targets `ui_prov_playscreen`, observed `CGameModeStage::ExitLoading`, but the current direct-context route still produced `ui_playscreen` instead of `ui_prov_playscreen`.
- Therefore the normal Sonic HUD route is now real-runtime CSD-bound, while the remaining `ui_prov_playscreen` blocker is deterministic Extra/Tornado stage owner selection rather than generic stage-context observation.

Still ahead:

- deterministic direct stage boot/routing for Extra/Tornado, tutorial, and result targets after the stage harness observes or creates the correct owner context
- CSD-project host creation for non-title screens that do not require a full gameplay owner
- backbuffer/native capture so evidence screenshots do not depend on desktop/window focus or include editor/window chrome
- route cleanup for startup prompts, save-state prompts, and DLC/install confirmation flows that can appear in front of requested UI targets
- demotion of the clean renderer in docs to diagnostic/evidence-only status everywhere it is still described too strongly

## Evidence Capture Contract

The UI Lab now has a basic evidence loop:

1. Launch the real generated UnleashedRecomp runtime with `--ui-lab-screen <target>`.
2. Enable JSONL evidence with `--ui-lab-evidence-dir <ignored local dir>`.
3. Let the real runtime present frames, attach translated title/menu hooks, and route through real state transitions.
4. Capture early/late screenshots, optional periodic observation screenshots, and `ui_lab_events.jsonl` into `out/ui_lab_runtime_evidence/<timestamp>/<target>/`.
5. For manual sessions, launch with `-KeepRunning` so the process remains alive while the operator moves screen to screen and evidence continues accumulating.
6. For manual observer sessions, prefer `-Observer -HideOverlay -KeepRunning`: this runs the real installed runtime as normally as possible, keeps the lab from forcing routes, hides the side panel, and still records CSD/loading/frame evidence.
7. Use those screenshots and events as the next debugging oracle instead of guessing from the standalone renderer.

This does not mean every target is deterministic yet. The normal Sonic HUD can now reach and bind the real `ui_playscreen` project through the runtime, but the `ui_prov_playscreen` path is a different Extra/Tornado-family owner. The next hard blocker is selecting that owner deterministically instead of drawing another clean-room approximation of the HUD.

## Immediate Product Direction

The next beats should build on `UiLab` in this order:

1. Replace desktop screenshot capture with backbuffer/native frame capture so AI review sees the exact rendered frame.
2. Promote the proven title/menu/loading direct-context path into a stable default once more route captures confirm it against normal saves and prompt variants.
3. Add deterministic stage-context creation or stage boot routing for Extra/Tornado HUD, tutorial overlays, and result/status routes, starting from the now-observed `CGameModeStage::ExitLoading` boundary and the confirmed `ui_playscreen` Sonic HUD bind.
4. Add CSD-project host creation for non-title screens that can be displayed without a full gameplay owner.
5. Add result/status/world-map only after their owner contexts can be created or routed without corrupting save/progression state.

The end-state is a real runtime-backed UI debug executable: Sonic Unleashed UI screens running through the game files and game renderer, not an inspired viewer.
