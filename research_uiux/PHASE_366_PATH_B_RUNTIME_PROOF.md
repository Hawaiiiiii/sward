# Phase 366 -- Path B runtime proof

Path B is the fast SGFX shell lane: run Sonic Unleashed UI through
UnleashedRecomp itself, then override assets/text and lock gameplay so the
runtime behaves like a reusable UI prototype engine.

## Current guarantees

The tracked `UnleashedRecomp/` source now contains the runtime hooks directly.
The ignored `local_build_env/ur103clean/` build tree is only the local build
copy; it is not the source of truth.

| Guarantee | Runtime proof event |
|---|---|
| Asset override mount | `Asset:OverrideHit:<guest-path>` |
| Text override manifest load | `Text:OverridesLoaded:<count>` |
| Gameplay-skip mode | `Stage:GameplaySkip` |
| UI-only input lock | `Input:UiOnlyLock` |

Bridge files live in `%APPDATA%\UnleashedRecomp\sgfx_bridge`.
Override files live in the directory selected by `SG_PREFLIGHT_OVERRIDE_DIR`;
the launch helpers default this to `%LOCALAPPDATA%\UnleashedRecomp\sg_overrides`.

## Fresh verification command

Build:

```bat
cmd /c _retry_ninja3.bat
```

Runtime proof launch:

```powershell
$env:SG_PREFLIGHT_OVERRIDE_DIR = "$env:LOCALAPPDATA\UnleashedRecomp\sg_overrides"
$env:SG_PREFLIGHT_GAMEPLAY_SKIP = "1"
$env:SG_PREFLIGHT_UI_ONLY_INPUT = "1"
$env:SG_PREFLIGHT_LOG_LOADS = "1"
.\UnleashedRecomp.exe --ui-lab --ui-lab-auto-exit=25 --ui-lab-hide-overlay
```

Expected `events.jsonl` proof set:

```json
{
  "TextOverridesLoaded": 1,
  "AssetOverrideHit": 1,
  "GameplaySkip": 1,
  "UiOnlyInputLock": 1
}
```

The asset proof can use a byte-identical safe override copied from the retail
install, for example:

```text
%LOCALAPPDATA%\UnleashedRecomp\sg_overrides\work\Application\SR_AdjustTownState.seq.xml
```

When the game requests `NY:/work/Application/SR_AdjustTownState.seq.xml`, the
bridge emits:

```text
Asset:OverrideHit:NY:/work/Application/SR_AdjustTownState.seq.xml
```
