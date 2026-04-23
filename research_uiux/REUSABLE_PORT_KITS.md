<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Reusable Port Kits

## Summary

Phase 24 turns the existing template pack plus generic C++ runtime into a small multi-language port kit layer.

The objective here is not to pretend the translated PPC output is clean product code. The objective is to convert the recovered UI/UX findings into reusable, publishable starter implementations that original projects can actually build on.

Port-kit root:

- `research_uiux/runtime_reference/`

> [!TIP]
> The follow-on Phase 27 runtime work is documented in [`DATA_DRIVEN_RUNTIME_CONTRACTS.md`](./DATA_DRIVEN_RUNTIME_CONTRACTS.md). The runtime kits now load shared JSON contracts instead of relying only on hardcoded profile builders.

## What Landed

- Expanded C++ runtime profile factories:
  - `PauseMenu`
  - `TitleMenu`
  - `AutosaveToast`
- Additional C++ examples:
  - `examples/pause_menu_example.cpp`
  - `examples/title_menu_example.cpp`
  - `examples/toast_overlay_example.cpp`
- New C ABI wrapper:
  - `include/sward/ui_runtime/runtime_c.h`
  - `src/runtime_c.cpp`
- New C example:
  - `examples/c_pause_menu_example.c`
- New C# reference port:
  - `runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj`
  - `runtime_reference/csharp_reference/ScreenRuntime.cs`
  - `runtime_reference/csharp_reference/ReferenceProfiles.cs`
  - `runtime_reference/csharp_reference/Program.cs`

## Why This Matters

This phase closes the gap between:

- research notes
- reusable YAML/JSON templates
- a single C++ proof-of-concept

and a more practical port surface for teams working in:

- C++
- C
- C#

That makes the workspace more useful for the stated goal: taking the recovered UI/UX state, timing, prompt, and overlay patterns and forwarding them into unrelated original projects.

## C++ Layer

The C++ layer remains the canonical implementation.

It now exposes explicit reference-profile builders instead of hardcoding a single pause-menu contract inside one example file.

Current reference profiles:

- `PauseMenu`
  - input-driven framed menu shell
  - prompt row plus tab paging
  - intro, navigate, confirm, cancel, outro timing bands
- `TitleMenu`
  - logo/card reveal
  - carousel motion
  - prompt-row visibility
  - title-entry and selection-commit choreography
- `AutosaveToast`
  - non-interactive overlay stack
  - intro/hold/outro timing
  - transient FX visibility during entry and exit

## C ABI Layer

The C wrapper provides an opaque-handle runtime for projects that want a simple ABI boundary.

Surface area:

- create a runtime by profile id
- tick and dispatch events
- request actions
- set predicates
- query visible prompts
- query visible overlay layers
- inspect current state and last requested scene

The C layer is intentionally profile-driven. That keeps the API narrow enough to be practical while still exposing the core choreography rules.

## C# Reference Port

The C# port is a pure managed reference implementation, not a P/Invoke wrapper.

That choice is deliberate:

- it preserves the contracts and state logic
- it avoids native packaging friction
- it gives C# teams a readable starting point

The managed port mirrors the same core ideas:

- explicit state enum
- event-driven transitions
- timeout-backed fallback flow
- prompt visibility by state and predicates
- overlay visibility by role

## Build Verification

Verified locally in this workspace:

- C++ / C configure:
  - `C:\Program Files\CMake\bin\cmake.exe -S research_uiux/runtime_reference -B b/rr24 -G "Visual Studio 17 2022" -A x64`
- C++ / C build:
  - `C:\Program Files\CMake\bin\cmake.exe --build b/rr24 --config Release`
- Verified executables:
  - `b/rr24/Release/sward_ui_runtime_example.exe`
  - `b/rr24/Release/sward_ui_runtime_title_menu_example.exe`
  - `b/rr24/Release/sward_ui_runtime_toast_example.exe`
  - `b/rr24/Release/sward_ui_runtime_c_example.exe`
- C# build:
  - `external_tools/dotnet8/dotnet.exe build research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release`
- C# run:
  - `external_tools/dotnet8/dotnet.exe run --project research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release`

## Recommended Use

Use the layers in this order:

1. Start from the templates under `research_uiux/templates/`.
2. Choose the closest reference profile in `runtime_reference/`.
3. Replace scene identifiers, prompt labels, strings, audio hooks, and visuals with project-local content.
4. Keep the explicit state/timing/overlay contracts even if the final presentation framework changes.

## Boundary

> [!IMPORTANT]
> These kits are structural ports only. They do not contain extracted Sonic assets, translated PPC output, or copied authored layouts. They preserve architecture and timing patterns, not proprietary payloads.
