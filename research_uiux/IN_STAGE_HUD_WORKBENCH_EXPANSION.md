<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> In-Stage HUD Workbench Expansion

Phase 40 widens the native debug workbench from frontend/menu/cutscene ownership into in-stage HUD and boss/final HUD hosts.

> [!IMPORTANT]
> This is still not the final whole-game debug build. It is the first defensible extension where the workbench can drive gameplay HUD families by recovered source-host names.

## Workbench State

- Generated workbench hosts: `40`
- Group count: `6`

Group counts:

- `Boss / Final HUD Hosts`: `3`
- `Cutscene / Preview Hosts`: `12`
- `Gameplay HUD Hosts`: `9`
- `Menu / Stage Debug Hosts`: `3`
- `Pause / Help / Loading Dispatch`: `4`
- `Stage Test / Validation Hosts`: `9`

## Verified Host Launches

- `b/rr41/Release/sward_ui_runtime_debug_workbench.exe --host HudSonicStage.cpp`
- `b/rr41/Release/sward_ui_runtime_debug_workbench.exe --host HudEvilStage.cpp`
- `b/rr41/Release/sward_ui_runtime_debug_workbench.exe --host HudExQte.cpp`
- `b/rr41/Release/sward_ui_runtime_debug_workbench.exe --host BossHudSuperSonic.cpp`
- `b/rr41/Release/sward_ui_runtime_debug_workbench.exe --host GameModeStageForwardTest.cpp`

## What Changed

- Gameplay HUD hosts now enter through direct host paths instead of only contract stems.
- Boss/final HUD ownership now has a dedicated workbench bucket.
- Stage-test aliases can now resolve into the recovered HUD contracts where the ownership is strong enough.

## Immediate Result

The workbench is now materially closer to the desired Sonic Unleashed-style debug surface:

- frontend flows still work
- cutscene preview still works
- in-stage Sonic HUD works
- in-stage Werehog HUD works
- extra-stage HUD/QTE works
- boss/final HUD bridge works
