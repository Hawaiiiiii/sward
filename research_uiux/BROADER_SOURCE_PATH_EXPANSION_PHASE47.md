<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Broader Source-Path Expansion Phase 47

Phase 47 widens the curated source-path bridge beyond the previous `220`-path seed without flooding the repo with the full executable path dump.

> [!IMPORTANT]
> This is still a curated UI/UX, presentation, debug, and support-substrate tranche. It is not a claim that every gameplay, object, enemy, physics, or middleware path is now humanized.

## What Expanded

The broader seed now includes the next defensible support layer around:

- `Achievement/AchievementManager.cpp`
- `Animation/EventTrigger/*`
- the wider `Camera/*` and `Replay/Camera/*` presentation family
- `Player/Parameter/*` and `Player/Switch/*`
- `Sound/*`
- `XML/*`

These paths matter because they sit under UI-facing behavior even when they are not screens themselves: achievement unlock dispatch, animation-triggered feedback, presentation cameras, player status feeding HUD overlays, audio cue routing, and XML/data loading.

## Measured State

- Source-path seed entries: `269`
- Family groups: `24`
- Paths mapped into archaeology/support systems: `212 / 269` (`78.8%`)
- Paths backed by existing runtime contracts: `186 / 269` (`69.1%`)
- Debug-tool candidates: `57`
- Named-only entries: `0`
- Local-only placement notes under `SONIC UNLEASHED/`: `269`
- Source-family selector launch families: `16`
- Debug workbench hosts: `159`
- Debug workbench groups: `10`

## New Support Families

| Family | Bridge | Runtime-backed |
|---|---|---|
| Achievement / Unlock Support | synthetic support system tied to achievement manager and sequence unlock ownership | No |
| Audio Cue / BGM Support | synthetic support system for sound controller/player/BGM routing | No |
| XML / Data Loading Support | synthetic support system for XML document/bin-data and stage-loader data binding | No |
| Timeline Event Trigger Support | cutscene/presentation timing support via animation event triggers | `subtitle_cutscene_reference.json` |
| Player Status / Switch Support | HUD-facing player parameter and switch state support | Sonic, Werehog, and Super Sonic HUD contracts |

## Workbench Impact

The workbench host map expands from `133` to `159` entries. The increase is intentionally concentrated in `Camera / Replay Hosts`, which now carries the wider camera/presentation family instead of only the first free/replay/town camera subset.

Current workbench groups:

- `Application / World Shell Hosts`: `64`
- `Camera / Replay Hosts`: `30`
- `Town / Media Room Hosts`: `21`
- `Cutscene / Preview Hosts`: `12`
- `Gameplay HUD Hosts`: `9`
- `Stage Test / Validation Hosts`: `9`
- `Frontend Sequence Hosts`: `4`
- `Pause / Help / Loading Dispatch`: `4`
- `Boss / Final HUD Hosts`: `3`
- `Menu / Stage Debug Hosts`: `3`

## Verified

- `python research_uiux/tools/test_phase47_source_expansion.py`
- `python -m py_compile research_uiux/tools/build_broader_ui_adjacent_source_seed.py research_uiux/tools/map_ui_source_paths.py research_uiux/tools/build_source_family_selector_data.py research_uiux/tools/build_debug_workbench_data.py research_uiux/tools/test_phase47_source_expansion.py`
- `b/rr47/sward_ui_runtime_debug_selector.exe --list-families`
- `b/rr47/sward_ui_runtime_debug_selector.exe Player3DBossCamera.cpp`
- `b/rr47/sward_ui_runtime_debug_workbench.exe --list-groups`
- `b/rr47/sward_ui_runtime_debug_workbench.exe --host Player3DBossCamera.cpp`
- `b/rr47/sward_ui_runtime_c_example.exe research_uiux/runtime_reference/contracts/camera_shell_reference.json`
- `external_tools/dotnet8/dotnet.exe build research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release`

## Boundary

The full path dump contains much larger gameplay/object/enemy/physics/middleware families. Those remain out of this phase on purpose. The next expansion should only pull another tranche when it can be tied to UI timing, debug-host value, source-family humanization, or runtime workbench coverage.
