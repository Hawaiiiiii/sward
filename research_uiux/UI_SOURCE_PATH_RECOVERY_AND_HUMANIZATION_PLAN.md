<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> UI Source-Path Recovery And Humanization Plan

> [!IMPORTANT]
> This report is scoped to the current SWARD source-path seed extracted from the broader Xbox 360 path dump the user supplied. It is a naming and organization bridge, not a claim that the whole game has already been humanized into clean source.

## Status Snapshot

| Target | State | Percentage |
|---|---|---:|
| Current source-path seed organized into families | Complete for this phase | `100.0%` |
| Current source-path seed mapped into the current archaeology layer | Partial but strong | `74.1%` |
| Current source-path seed already backed by portable runtime contracts | Partial | `70.0%` |
| Current source-path seed still only named/debug-targeted and not semantically recovered | Open gap | `25.9%` |

## Exact Counts

- Seeded source paths: `220`
- Family groups: `19`
- Paths mapped to current archaeology systems: `163`
- Paths already backed by runtime contracts: `154`
- Paths that are strong debug-tool host candidates: `57`
- Paths that remain named-only after this phase: `0`

## Coverage Buckets

| Bucket | Count | Meaning |
|---|---:|---|
| `contract_backed` | `154` | Path family already lands on a recovered archaeology system with a portable runtime contract. |
| `archaeology_mapped` | `9` | Path family is tied to a recovered system, but not yet exercised by the runtime contract pack. |
| `debug_tool_candidate` | `57` | Debug/tool/game-mode surface that looks like a good host for the future UI capability sandbox. |
| `named_seed_only` | `0` | We now have the source-path name organized, but the family still needs extraction/correlation/humanization work. |

## Family Breakdown

| Family | Paths | Current bridge | Runtime-backed |
|---|---:|---|---|
| Tooling / Debug UI | `60` | `Camera / Replay Shell` | `camera_shell_reference.json` |
| Frontend System Shell | `33` | `Application / World Shell` | `application_world_shell_reference.json` |
| Subtitle / Cutscene Presentation | `32` | `Subtitle / Cutscene Presentation` | `subtitle_cutscene_reference.json` |
| Town / Media Room UI | `21` | `Town UI` | `town_ui_reference.json` |
| Stage HUD Core | `9` | `Werehog Stage HUD`, `Sonic Stage HUD` | `werehog_stage_hud_reference.json`, `sonic_stage_hud_reference.json` |
| Title / Main Menu | `9` | `Title Menu`, `Loading And Start/Clear`, `Application / World Shell` | `title_menu_reference.json`, `loading_transition_reference.json`, `application_world_shell_reference.json` |
| Loading / Boot / Install | `8` | `Loading And Start/Clear`, `Application / World Shell` | `loading_transition_reference.json`, `application_world_shell_reference.json` |
| Pause Stack | `7` | `Pause Stack` | `pause_menu_reference.json` |
| Save / Ending Flow | `7` | `Save And Ending`, `Application / World Shell` | `autosave_toast_reference.json`, `application_world_shell_reference.json` |
| World Map Stack | `7` | `World Map Stack`, `Application / World Shell` | `world_map_reference.json`, `application_world_shell_reference.json` |
| Boss HUD | `5` | `Super Sonic / Final HUD Bridge`, `Boss HUD` | `super_sonic_hud_reference.json`, `boss_hud_reference.json` |
| CSD / UI Foundation | `5` | `CSD / UI Foundation` | No |
| Frontend Sequence Shell | `4` | `Frontend Sequence Shell` | `frontend_sequence_shell_reference.json` |
| Mission Result Family | `4` | `Mission Result Family`, `Item Result` | `mission_result_reference.json` |
| GameMode / Frontend Shell | `3` | `Application / World Shell` | `application_world_shell_reference.json` |
| Mission Briefing / Gate | `3` | `Mission Briefing And Gate` | No |
| Frontend Camera Shell | `1` | `Camera / Replay Shell` | `camera_shell_reference.json` |
| Status Overlay | `1` | `Status Overlay` | No |
| Tornado Defense / EX HUD | `1` | `Tornado Defense / EX Stage`, `Extra Stage / Tornado Defense HUD` | `extra_stage_hud_reference.json` |

## System Bridge

| Archaeology system | Seed paths | Layout IDs | Runtime contract |
|---|---:|---|---|
| Application / World Shell | `64` | None | `application_world_shell_reference.json` |
| Subtitle / Cutscene Presentation | `32` | None | `subtitle_cutscene_reference.json` |
| Town UI | `21` | `ui_balloon`, `ui_shop`, `ui_townscreen`, `ui_mediaroom` | `town_ui_reference.json` |
| Loading And Start/Clear | `17` | `ui_loading`, `ui_start` | `loading_transition_reference.json` |
| Title Menu | `9` | `ui_mainmenu` | `title_menu_reference.json` |
| Pause Stack | `7` | `ui_pause`, `ui_general`, `ui_help` | `pause_menu_reference.json` |
| Save And Ending | `7` | `ui_saveicon`, `ui_end` | `autosave_toast_reference.json` |
| World Map Stack | `7` | `ui_worldmap`, `ui_worldmap_help` | `world_map_reference.json` |
| CSD / UI Foundation | `5` | `ui_balloon`, `ui_general`, `ui_help`, `ui_mainmenu`, `ui_mediaroom`, `ui_pause`, `ui_shop`, `ui_status`, `ui_townscreen`, `ui_worldmap`, `ui_worldmap_help` | No |
| Sonic Stage HUD | `5` | `ui_playscreen` | `sonic_stage_hud_reference.json` |
| Werehog Stage HUD | `5` | `ui_playscreen_ev`, `ui_playscreen_ev_hit` | `werehog_stage_hud_reference.json` |
| Camera / Replay Shell | `4` | None | `camera_shell_reference.json` |
| Frontend Sequence Shell | `4` | None | `frontend_sequence_shell_reference.json` |
| Mission Result Family | `4` | `ui_result`, `ui_result_ex` | `mission_result_reference.json` |
| Mission Briefing And Gate | `3` | `ui_gate`, `ui_missionscreen`, `ui_misson` | No |
| Super Sonic / Final HUD Bridge | `3` | `ui_playscreen_su` | `super_sonic_hud_reference.json` |
| Boss HUD | `2` | `ui_boss_gauge`, `ui_boss_name` | `boss_hud_reference.json` |
| Extra Stage / Tornado Defense HUD | `1` | `ui_prov_playscreen`, `ui_qte` | `extra_stage_hud_reference.json` |
| Item Result | `1` | `ui_itemresult` | No |
| Status Overlay | `1` | `ui_status` | No |
| Tornado Defense / EX Stage | `1` | `ui_exstage`, `ui_prov_playscreen`, `ui_qte` | No |

## What This Means Right Now

- The generated translated PPC layer is present, but the clean human-readable organization layer is still incomplete.
- This phase gives the current source-path subset a stable naming scaffold, so future translated-code cleanup can follow original source-family names instead of raw `sub_XXXXXXXX` clusters alone.
- The strongest already-recovered path families are title/menu, pause, loading/start, world map, mission-result, save/ending, gameplay HUD core, town/media-room, and the lower-level CSD foundation layer.
- The clearest remaining UI/UX gaps are the still note-heavy translated ownership layer inside the local mirror, the incomplete gameplay/asset correlation outside the current contract families, and the broader whole-game source-path set that still needs to be humanized in waves.

## Debug Tool Direction

- The best current hosts for a local debug executable/menu build are the debug/test game modes plus the tool/preview surfaces grouped under `Tooling / Debug UI`.
- The current contract-backed runtime layer is already strong enough to drive the standalone selector and workbench across frontend, town, camera, application/world, cutscene, gameplay-HUD, boss/final, and stage-test-adjacent host families.
- The missing bridge for a richer debug tool is no longer raw translation; it is turning more of the source-path-backed families into readable translated ownership and widening host coverage beyond the current reusable subset.

## Next Local Work

1. Keep widening the current source-path seed in defensible chunks instead of flooding the repo with raw whole-dump noise.
2. Keep replacing local-only `SONIC UNLEASHED/` note/scaffold files with cleaner translated ownership under the recovered source-family paths.
3. Keep expanding selector/workbench coverage and mirrored translated ownership until the local-only tree behaves like a readable debug-oriented source base instead of a scaffold set.
