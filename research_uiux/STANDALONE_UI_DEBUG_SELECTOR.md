<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Standalone UI Debug Selector

Phase 29 turns the contract-backed runtime layer into the first local debug selector executable instead of a pile of isolated single-screen examples.

> [!IMPORTANT]
> This selector stays in the publishable runtime-reference layer. It does not bundle extracted assets, translated PPC output, or local-only source-tree mirrors.

## Goal

Build a first local executable that can:

- enumerate the current bundled UI contracts
- select a screen family by index, token, or explicit contract path
- drive the chosen contract through a representative state walk
- print state, scene-request, overlay, and prompt visibility changes for inspection

That covers the current contract-backed screen set:

- `PauseMenuReference`
- `TitleMenuReference`
- `AutosaveToastReference`
- `LoadingTransitionReference`
- `MissionResultReference`
- `WorldMapReference`

## Runtime Additions

The Phase 29 beat expands the reusable runtime layer in three ways:

1. `ReferenceProfile` now exposes all six bundled contracts instead of only pause/title/autosave.
2. The native and managed helper layers can resolve those same bundled profiles consistently.
3. A new console executable, `sward_ui_runtime_debug_selector`, acts as a first debug shell for browsing contract-backed screens.

## Selector Commands

Build output:

- `b/rr29/Release/sward_ui_runtime_debug_selector.exe`

Selection modes:

- `--list`
- `--index <1-based-number>`
- `<screen token>` such as `title`, `loading`, or `world`
- `--path <contract.json>` for an explicit portable contract file

Examples:

```powershell
b/rr29/Release/sward_ui_runtime_debug_selector.exe --list
b/rr29/Release/sward_ui_runtime_debug_selector.exe title
b/rr29/Release/sward_ui_runtime_debug_selector.exe loading
b/rr29/Release/sward_ui_runtime_debug_selector.exe world
b/rr29/Release/sward_ui_runtime_debug_selector.exe --path research_uiux/runtime_reference/contracts/mission_result_reference.json
```

With no arguments, the selector prints the bundled list and waits for an interactive numeric choice.

## What The Selector Verifies

The selector is intentionally modest. It does not try to emulate the full game host. It verifies the reusable contract layer instead:

- `ResourcesReady` handoff into intro/open states
- timeout-band progression for intro/confirm/outro windows
- prompt-row gating via contract predicates
- overlay-role visibility by state
- host-driven close behavior for non-interactive shells such as loading

This makes it a practical stepping stone toward a richer debug menu while staying portable and repo-safe.

## Local Verification

Phase 29 verification was run locally with:

```powershell
& 'C:\Program Files\CMake\bin\cmake.exe' -S research_uiux/runtime_reference -B b/rr29 -G 'Visual Studio 17 2022' -A x64
& 'C:\Program Files\CMake\bin\cmake.exe' --build b/rr29 --config Release
& 'b/rr29/Release/sward_ui_runtime_debug_selector.exe' --list
& 'b/rr29/Release/sward_ui_runtime_debug_selector.exe' title
& 'b/rr29/Release/sward_ui_runtime_debug_selector.exe' loading
& 'b/rr29/Release/sward_ui_runtime_debug_selector.exe' world
external_tools/dotnet8/dotnet.exe build research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release
external_tools/dotnet8/dotnet.exe run --project research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release
```

Observed successful checks:

- bundled selector list resolved all six contract-backed screens
- title flow traversed `Intro -> Idle -> Navigate -> Confirm -> Outro -> Closed`
- loading flow traversed `Intro -> Idle -> Outro -> Closed` via host-force-close
- world-map flow traversed `Intro -> Idle -> Navigate -> Confirm -> Idle -> Outro -> Closed`
- the managed C# reference still built and ran cleanly after the bundled-profile expansion

## Current Limits

- It is still a console debug shell, not the final asset-backed debug UI executable.
- It only covers the current contract-backed families, not the gameplay HUD core.
- It does not yet browse the local-only mirrored source tree under `SONIC UNLEASHED/`.
- It does not yet load screens from extracted layouts directly.

Those gaps are the direct handoff into Phase 30 and Phase 31.
