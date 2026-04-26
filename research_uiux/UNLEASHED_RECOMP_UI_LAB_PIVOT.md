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
- a curated target table for `TitleLoop`, `TitleMenu`, `Loading`, `SonicHud`, `Result`, `Status`, `Tutorial`, and `WorldMap`
- runtime config overrides that keep the lab path debug-friendly (`ShowConsole`, `SkipIntroLogos`, `DisableAutoSaveWarning`)
- attachment points in `CTitleStateIntro::Update` and `CTitleStateMenu::Update`

This is intentionally not a fake renderer. It is the entry point for driving the real runtime.

## Current Boundary

Implemented now:

- CLI/runtime flag surface for the UI Lab
- initial target taxonomy tied to CSD scene names and recovered source-family ownership
- title intro/menu update hooks that prove the lab can attach to live translated runtime states
- regression tests that guard the runtime-lab contract

Verification note:

- The repo-root `UnleashedRecompLib` does not currently contain generated PPC/shader output, so the tracked root cannot build the full runtime directly.
- The generated clone under `local_build_env/ur103clean` does contain `261` `ppc_recomp.*.cpp` files and `shader_cache.cpp`, but its existing CMake cache was created from `U:/local_build_env/ur103clean`; a fresh configure in this workspace path is required before a compile.
- This shell currently exposes `ninja`, but not `clang-cl`, `cl`, `clang++`, or `g++`, so the runtime compile is environment-blocked in this beat.
- Static regression coverage was added so the UI Lab source/module/CLI/hook contract is guarded until the compiler path is restored.

Still ahead:

- visible in-game debug menu or overlay
- state forcing for title/menu/loading without relying on normal user navigation
- CSD-project host creation for non-title screens
- stage-context harness for Sonic HUD/tutorial/result targets
- screenshot/vision verification against live UnleashedRecomp frames
- demotion of the clean renderer in docs to diagnostic/evidence-only status everywhere it is still described too strongly

## Immediate Product Direction

The next beats should build on `UiLab` in this order:

1. Add a visible runtime overlay/menu that lists `UiLab::GetRuntimeTargets()`.
2. Add title-loop/title-menu forcing first, because those states already run without a stage world.
3. Add loading/Miles Electric forcing, using the existing `ui_loading` and movie/loading hooks.
4. Add a lightweight stage-context harness for Sonic HUD and tutorial overlays.
5. Add result/status/world-map only after their owner contexts can be created or routed without corrupting save/progression state.

The end-state is a real runtime-backed UI debug executable: Sonic Unleashed UI screens running through the game files and game renderer, not an inspired viewer.
