<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> UI Source-Path Recovery And Humanization Plan

> [!IMPORTANT]
> This report is scoped to the UI/UX-focused source-path seed extracted from the broader Xbox 360 path dump the user supplied. It is a naming and organization bridge, not a claim that the whole game has already been humanized into clean source.

## Status Snapshot

| Target | State | Percentage |
|---|---|---:|
| UI-centric source-path seed organized into families | Complete for this phase | `100.0%` |
| UI-centric source-path seed mapped into the current archaeology layer | Partial but strong | `71.3%` |
| UI-centric source-path seed already backed by portable runtime contracts | Partial | `34.3%` |
| UI-centric source-path seed still only named/debug-targeted and not semantically recovered | Open gap | `28.7%` |

## Exact Counts

- Seeded source paths: `108`
- Family groups: `16`
- Paths mapped to current archaeology systems: `77`
- Paths already backed by runtime contracts: `37`
- Paths that are strong debug-tool host candidates: `13`
- Paths that remain named-only after this phase: `18`

## Coverage Buckets

| Bucket | Count | Meaning |
|---|---:|---|
| `contract_backed` | `37` | Path family already lands on a recovered archaeology system with a portable runtime contract. |
| `archaeology_mapped` | `40` | Path family is tied to a recovered system, but not yet exercised by the runtime contract pack. |
| `debug_tool_candidate` | `13` | Debug/tool/game-mode surface that looks like a good host for the future UI capability sandbox. |
| `named_seed_only` | `18` | We now have the source-path name organized, but the family still needs extraction/correlation/humanization work. |

## Family Breakdown

| Family | Paths | Current bridge | Runtime-backed |
|---|---:|---|---|
| Subtitle / Cutscene Presentation | `18` | `Subtitle / Cutscene Presentation` | No |
| Tooling / Debug UI | `13` | Not yet in archaeology | No |
| Town / Media Room UI | `12` | `Town UI` | No |
| Stage HUD Core | `9` | Not yet in archaeology | No |
| Title / Main Menu | `9` | `Title Menu`, `Loading And Start/Clear` | `title_menu_reference.json`, `loading_transition_reference.json` |
| Save / Ending Flow | `8` | `Save And Ending` | `autosave_toast_reference.json` |
| World Map Stack | `7` | `World Map Stack` | `world_map_reference.json` |
| Pause Stack | `6` | `Pause Stack` | `pause_menu_reference.json` |
| Boss HUD | `5` | `Boss HUD` | No |
| CSD / UI Foundation | `5` | Not yet in archaeology | No |
| Mission Result Family | `4` | `Mission Result Family`, `Item Result` | `mission_result_reference.json` |
| UI Misc | `4` | Not yet in archaeology | No |
| Loading / Boot / Install | `3` | `Loading And Start/Clear` | `loading_transition_reference.json` |
| Mission Briefing / Gate | `3` | `Mission Briefing And Gate` | No |
| Status Overlay | `1` | `Status Overlay` | No |
| Tornado Defense / EX HUD | `1` | `Tornado Defense / EX Stage` | No |

## System Bridge

| Archaeology system | Seed paths | Layout IDs | Runtime contract |
|---|---:|---|---|
| Subtitle / Cutscene Presentation | `18` | None | No |
| Loading And Start/Clear | `12` | `ui_loading`, `ui_start` | `loading_transition_reference.json` |
| Town UI | `12` | `ui_balloon`, `ui_shop`, `ui_townscreen`, `ui_mediaroom` | No |
| Title Menu | `9` | `ui_mainmenu` | `title_menu_reference.json` |
| Save And Ending | `8` | `ui_saveicon`, `ui_end` | `autosave_toast_reference.json` |
| World Map Stack | `7` | `ui_worldmap`, `ui_worldmap_help` | `world_map_reference.json` |
| Pause Stack | `6` | `ui_pause`, `ui_general`, `ui_help` | `pause_menu_reference.json` |
| Boss HUD | `5` | `ui_boss_gauge`, `ui_boss_name` | No |
| Mission Result Family | `4` | `ui_result`, `ui_result_ex` | `mission_result_reference.json` |
| Mission Briefing And Gate | `3` | `ui_gate`, `ui_missionscreen`, `ui_misson` | No |
| Item Result | `1` | `ui_itemresult` | No |
| Status Overlay | `1` | `ui_status` | No |
| Tornado Defense / EX Stage | `1` | `ui_exstage`, `ui_prov_playscreen`, `ui_qte` | No |

## What This Means Right Now

- The generated translated PPC layer is present, but the clean human-readable organization layer is still incomplete.
- This phase gives the UI/UX subset of the executable path dump a stable naming scaffold, so future translated-code cleanup can follow original source-family names instead of raw `sub_XXXXXXXX` clusters alone.
- The strongest already-recovered path families are title/menu, pause, loading/start, world map, mission-result, save/ending, boss HUD, town/media-room, and cutscene/subtitle presentation.
- The clearest remaining UI/UX gaps are the gameplay HUD core, the lower-level CSD foundation, and the debug/tool surfaces we can repurpose into a standalone UI capability sandbox.

## Debug Tool Direction

- The best current hosts for a local debug executable/menu build are the debug/test game modes plus the tool/preview surfaces grouped under `Tooling / Debug UI`.
- The current contract-backed runtime layer is already strong enough to prototype a standalone screen selector for title, pause, loading, result, autosave, and world-map flows.
- The missing bridge for a richer debug tool is not raw translation anymore; it is source-path-backed naming plus a broader gameplay HUD/system contract set.

## Next Local Work

1. Expand this seed from the UI/UX subset into a broader whole-dump manifest without drowning the repo in raw path noise.
2. Start a source-path-backed humanization pass over translated seams for the uncovered families, especially gameplay HUD core and CSD foundations.
3. Build a dedicated local debug screen selector over the current runtime contracts, then add new contracts as more path families become semantically recovered.
