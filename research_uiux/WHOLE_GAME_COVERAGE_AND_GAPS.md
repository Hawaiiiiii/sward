<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Whole-Game Coverage And Gaps

> [!IMPORTANT]
> Short answer: no, this workspace does not contain the original human-authored Sonic Unleashed source code, and it does not yet contain a fully extracted/readable copy of every shipped asset.

## Status Snapshot

| Target | State | Percentage |
|---|---|---:|
| Tracked SWARD checklist scope | Complete | `100%` |
| Publishable repo layer | Complete | `100%` |
| In-scope readable Unleashed Recompiled UI/runtime/patch layer | Indexed and documented | `100%` of the files we targeted and scanned |
| Original SEGA human-authored game source | Not available locally | `0%` verified |
| Generated translated PowerPC C++ from owned `default.xex` | Present locally | exact semantic-readability percentage is not meaningful |
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
- A fully loose-file-extracted copy of every archive in the game.
- A verified `1:1` forward-port package for the entire game UI stack.

## What The Generated PPC Output Actually Means

The generated PPC C++ is useful for behavior archaeology, hook tracing, timings, and control-flow recovery. It is not the same thing as original source code.

What it gives us:

- broad executable-level coverage from the owned `default.xex`
- callable/function-level seams for readable patch correlation
- enough low-level visibility to recover timings, state branches, and host relationships

What it does not give us:

- original class design intent
- original file/module boundaries
- clean naming
- a production-quality source base you can directly treat as authored engine code

## What This Means For `1:1` Template Work

For the studied UI systems, the workspace is strong enough to extract architecture and timing patterns that can be rebuilt in original C++, C, or C# projects.

That includes:

- title/menu/pause/options state flow
- achievement/message/button-guide overlays
- fades, bars, static, and transition timing
- subtitle/cutscene presentation cues
- boss HUD/result/save families
- reusable runtime contracts and template-pack structures

What it still does not justify:

- claiming the whole game is now available as human-readable original source
- claiming every asset/state/animation has been exhaustively extracted and correlated
- treating the generated PPC output as a drop-in whole-game engine port

## Current Best Description

The current workspace is:

- `100%` complete against the tracked research plan through Phase `24`
- strong for UI/UX reverse-engineering and template extraction
- strong for local executable-backed timing/state archaeology
- strong for reusable runtime/template productization across C++, C, and C#
- partial for whole-game loose-file asset extraction
- not equivalent to the original full source code of Sonic Unleashed

## Highest-Value Remaining Work

If the goal is to move closer to a broader `1:1` UI portability basis, the next concrete work would be:

1. Expand extraction coverage beyond the current UI-heavy archive slices into more remaining common/HUD/mission/cutscene families.
2. Push the layout parser deeper into scene graphs, animation channels, and per-scene timeline naming across more extracted layouts.
3. Continue correlating generated PPC seams against extracted layouts and readable patch hosts for the remaining UI families.
4. Extend the generic runtime reference so more recovered screen contracts can be exercised in standalone examples.

> [!NOTE]
> The new Phase 25 common-flow slice improves the loose localized support layer, and Phase 26 now labels `180` translated seams across the highest-value common-flow systems. The next bottleneck is therefore even clearer: more value now comes from contract-building and runtime productization than from blindly expecting every high-scoring archive to hide another `.yncp`.
