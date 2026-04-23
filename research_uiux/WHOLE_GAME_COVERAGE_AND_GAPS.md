<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Whole-Game Coverage And Gaps

> [!IMPORTANT]
> Short answer: yes for the locally generated translated executable/shader layer from the owned build inputs; no for a clean human-readable whole-game source tree, and no for a fully extracted/readable copy of every shipped asset.

## Status Snapshot

| Target | State | Percentage |
|---|---|---:|
| Tracked SWARD checklist scope | Complete | `100%` |
| Publishable repo layer | Complete | `100%` |
| In-scope readable Unleashed Recompiled UI/runtime/patch layer | Indexed and documented | `100%` of the files we targeted and scanned |
| Original SEGA human-authored game source | Not available locally | `0%` verified |
| Generated translated PowerPC C++ and shader layer from owned inputs | Present locally | `100%` of the current generation goal |
| UI-centric source-path seed bridged into the archaeology layer | Partial but measured | `83.3%` |
| UI-centric source-path seed already backed by runtime contracts | Partial | `34.3%` |
| Whole-game asset corpus extracted into readable loose files | Not complete | no defensible exact percentage yet |
| Template-grade UI/UX recovery for the studied screens | Strong | high confidence, but not a whole-game `1:1` portability claim |

## What Exists Locally

- The open-source handwritten Unleashed Recompiled layer is present, indexed, and heavily documented.
- The local recompilation pipeline succeeded against the owned Xbox 360 executable and shader inputs.
- Local generated outputs now include:
  - `261` `ppc_recomp.*.cpp` translation units
  - `ppc_func_mapping.cpp`
  - `ppc_context.h`
  - `ppc_config.h`
  - `shader_cache.cpp`
- The reusable runtime/template productization layer now includes:
  - native C++ reference profiles
  - a C ABI wrapper plus verified C example
  - a managed C# reference port
- The source-path bridge layer now includes:
  - a tracked UI-focused source-path seed at `research_uiux/source_path_seeds/UI_SOURCE_PATHS_FROM_EXECUTABLE.txt`
  - a machine-readable manifest at `research_uiux/data/ui_source_path_manifest.json`
  - a human-readable bridge report at `research_uiux/UI_SOURCE_PATH_RECOVERY_AND_HUMANIZATION_PLAN.md`
  - a dedicated CSD/UI foundation map at `research_uiux/data/csd_ui_foundation_map.json`
  - a human-readable foundation report at `research_uiux/CSD_UI_FOUNDATION_HUMANIZATION.md`
- The UI asset workspace currently indexes:
  - `6840` UI-relevant asset entries across the installed build plus extracted roots
  - `17792` extracted files under `extracted_assets`
  - `28` parsed `.xncp` / `.yncp` layout files
  - `26` merged code-to-layout correlation entries
  - `24` subtitle resource XML files tied back to `9` matched scene IDs
  - `6` taxonomy layouts across boss HUD, result, and save families
  - `13` grouped archaeology systems in `ui_archaeology_database.json`
  - `56` generated PPC seams actively tied into the current archaeology layer
  - `89` indexed Phase 25 loose-file hits across the new common-flow/localization extraction root
  - `180` translated PPC seams labeled across `8` targeted systems in the Phase 26 seam/state pass

## What Does Not Exist Locally

- Original human-authored SEGA gameplay/UI engine source code.
- A clean, architecture-level, whole-game C++ codebase equivalent to the shipped game.
- A fully source-path-organized and humanized local tree that explains what every translated gameplay/UI unit does.
- A fully loose-file-extracted copy of every archive in the game.
- A verified `1:1` forward-port package for the entire game UI stack.

## What The Generated PPC Output Actually Means

The generated PPC C++ is useful for behavior archaeology, hook tracing, timings, and control-flow recovery. It is not the same thing as original source code.

What it gives us:

- broad executable-level coverage from the owned `default.xex`
- callable/function-level seams for readable patch correlation
- enough low-level visibility to recover timings, state branches, and host relationships
- enough structure to drive portable JSON runtime contracts across the current C++, C, and C# reference kits
- enough structure to start organizing translated findings under original source-family names for the UI/UX subset

What it does not give us:

- original class design intent
- original file/module boundaries
- clean naming
- a production-quality source base you can directly treat as authored engine code
- a finished source-path-backed debug executable that can browse every uncovered UI family

## What This Means For `1:1` Template Work

For the studied UI systems, the workspace is strong enough to extract architecture and timing patterns that can be rebuilt in original C++, C, or C# projects.

That includes:

- title/menu/pause/options state flow
- achievement/message/button-guide overlays
- fades, bars, static, and transition timing
- subtitle/cutscene presentation cues
- boss HUD/result/save families
- gameplay HUD core families across Sonic, Werehog, Extra Stage, and Super Sonic variants
- reusable runtime contracts and template-pack structures

What it still does not justify:

- claiming the whole game is now available as human-readable original source
- claiming every asset/state/animation has been exhaustively extracted and correlated
- treating the generated PPC output as a drop-in whole-game engine port

## Current Best Description

The current workspace is:

- `100%` complete against the tracked research plan through Phase `31`
- strong for UI/UX reverse-engineering and template extraction
- strong for local executable-backed timing/state archaeology
- strong for reusable runtime/template productization across C++, C, and C#
- strong for measured UI source-path organization inside the seeded UI/UX subset
- now materially stronger for in-stage gameplay HUD ownership and family separation
- now materially stronger for lower-level CSD/project/widget naming and source-family organization
- now materially stronger for local-only source-family placement inside the mirrored `SONIC UNLEASHED/` tree
- partial for whole-game loose-file asset extraction
- not yet equivalent to a whole-game clean human-readable source tree

## Highest-Value Remaining Work

If the goal is to move closer to a broader `1:1` UI portability basis, the next concrete work would be:

1. Expand the current UI-focused source-path seed into a broader manifest so the humanization pass can cover more than the current `108` UI-centric paths.
2. Grow the new local-only `SONIC UNLEASHED/` placement layer from `*.sward.md` notes into cleaner renamed translated ownership under those source-family paths.
3. Continue correlating generated PPC seams against extracted layouts and readable patch hosts for the remaining UI families, especially the last unresolved gameplay-HUD and subtitle/cutscene edges.
4. Grow the current standalone selector into a richer source-path-named debug sandbox, then widen it with gameplay-HUD and cutscene-capable contracts.

> [!NOTE]
> After the Phase 32 placement pass, the next bottleneck is even clearer: more value now comes from widening and deepening the mirrored source-family tree plus the debug sandbox than from blindly expecting every high-scoring archive to hide another `.yncp`.
