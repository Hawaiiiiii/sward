<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Gameplay HUD Runtime Contracts

Phase 39 extends the bundled runtime contract layer into the in-stage HUD families instead of stopping at frontend/menu/cutscene flows.

> [!IMPORTANT]
> These contracts are reusable behavioral references derived from the recovered HUD ownership layer. They are not original authored SEGA source or proprietary layout payloads.

## What Landed

- `research_uiux/runtime_reference/contracts/sonic_stage_hud_reference.json`
- `research_uiux/runtime_reference/contracts/werehog_stage_hud_reference.json`
- `research_uiux/runtime_reference/contracts/extra_stage_hud_reference.json`
- `research_uiux/runtime_reference/contracts/super_sonic_hud_reference.json`
- `research_uiux/runtime_reference/contracts/boss_hud_reference.json`

Profile/runtime layers updated:

- native C++ `ReferenceProfile`
- native C ABI `sward_ui_profile_id`
- managed C# `ReferenceProfile`
- bundled contract loader path table

## Coverage Shift

- Broader source-path seed already backed by runtime contracts: `88 / 220` (`40.0%`)
- Previous contract-backed count before this phase: `74 / 220` (`33.6%`)
- Net gain from this HUD-contract pass: `14` path-level entries

New contract-backed systems:

- `Sonic Stage HUD`
- `Werehog Stage HUD`
- `Extra Stage / Tornado Defense HUD`
- `Super Sonic / Final HUD Bridge`
- `Boss HUD`

## Verified Paths

Native selector:

- `b/rr41/Release/sward_ui_runtime_debug_selector.exe HudSonicStage.cpp`
- `b/rr41/Release/sward_ui_runtime_debug_selector.exe HudEvilStage.cpp`
- `b/rr41/Release/sward_ui_runtime_debug_selector.exe HudExQte.cpp`
- `b/rr41/Release/sward_ui_runtime_debug_selector.exe BossHudSuperSonic.cpp`

Native C explicit contract path:

- `b/rr41/Release/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/sonic_stage_hud_reference.json`

Managed C# reference:

- `external_tools/dotnet8/dotnet.exe run --project research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release --no-build`

## Why This Matters

- The runtime layer is no longer frontend-only.
- The workbench and selector can now speak in terms of real in-stage HUD host paths instead of only pause/title/loading/result families.
- The local-only `SONIC UNLEASHED/` mirror now has reusable contract anchors for the gameplay HUD source families that were previously archaeology-only.
