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
| Broader UI-adjacent source-path seed bridged into the archaeology/support layer | Partial but measured | `78.8%` |
| Broader UI-adjacent source-path seed already backed by runtime contracts | Partial but strong | `75.5%` |
| Local-only readable source layer inside the mirrored `SONIC UNLEASHED/` tree | Partial but accelerating | `125` humanized `.cpp` scaffolds |
| Native non-CLI UI runtime workbench | Gameplay-HUD proxy preview plus timer playback/motion, first exact-family layouts, decoded layout-evidence overlay, and frame-domain timeline readouts present | `b/rr58/sward_ui_runtime_debug_gui.exe` |
| GUI visual atlas bindings | First curated set plus HUD proxies present | `10` contract-to-atlas candidates, including `2` proxy candidates |
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
  - a tracked broader UI-adjacent seed at `research_uiux/source_path_seeds/UI_ADJACENT_SOURCE_PATHS_FROM_MATCH_DUMP.txt`
  - a machine-readable manifest at `research_uiux/data/ui_source_path_manifest.json`
  - a human-readable bridge report at `research_uiux/UI_SOURCE_PATH_RECOVERY_AND_HUMANIZATION_PLAN.md`
  - a Phase 47 wider-source report at `research_uiux/BROADER_SOURCE_PATH_EXPANSION_PHASE47.md`
  - a Phase 49 local support-substrate humanization report at `research_uiux/LOCAL_SUPPORT_SUBSTRATE_HUMANIZATION.md`
  - a Phase 50 support-substrate runtime contract report at `research_uiux/SUPPORT_SUBSTRATE_RUNTIME_CONTRACTS.md`
  - a Phase 51 native GUI workbench report at `research_uiux/NATIVE_GUI_DEBUG_WORKBENCH.md`
  - a Phase 52 GUI visual preview report at `research_uiux/GUI_VISUAL_PREVIEW_AND_ATLAS_BINDING.md`
  - a Phase 53 gameplay-HUD proxy preview report at `research_uiux/GAMEPLAY_HUD_PROXY_PREVIEW_BINDING.md`
  - a Phase 54 GUI timeline playback report at `research_uiux/GUI_TIMELINE_PLAYBACK_CONTROLS.md`
  - a Phase 55 GUI state-aware preview motion report at `research_uiux/GUI_STATE_AWARE_PREVIEW_MOTION.md`
  - a Phase 56 GUI exact-family preview-layout report at `research_uiux/GUI_EXACT_FAMILY_PREVIEW_LAYOUTS.md`
  - a Phase 57 GUI layout-evidence preview overlay report at `research_uiux/GUI_LAYOUT_EVIDENCE_PREVIEW_OVERLAY.md`
  - a Phase 58 GUI layout timeline frame-preview report at `research_uiux/GUI_LAYOUT_TIMELINE_FRAME_PREVIEW.md`
  - a shell/debug host recovery map at `research_uiux/data/frontend_shell_recovery.json`
  - a human-readable shell/debug host report at `research_uiux/FRONTEND_SHELL_AND_DEBUG_HOST_RECOVERY.md`
  - a dedicated frontend-sequence bridge report at `research_uiux/FRONTEND_SEQUENCE_SHELL_RUNTIME_BRIDGE.md`
  - a richer native host-bucket debug executable at `b/rr58/sward_ui_runtime_debug_workbench.exe`
  - a verified native selector with persistent interactive/stay-open behavior at `b/rr58/sward_ui_runtime_debug_selector.exe`
  - a first proper native GUI operator shell with visual gameplay-HUD proxy preview, state-aware motion, exact-family Title/Pause/Loading placement, `ui_mainmenu` / `ui_pause` / `ui_loading` layout-evidence overlays, and recovered frame-domain timeline readouts at `b/rr58/sward_ui_runtime_debug_gui.exe`
  - a compact workbench catalog report at `research_uiux/DEBUG_WORKBENCH_CATALOG_VIEW.md`
  - a dedicated CSD/UI foundation map at `research_uiux/data/csd_ui_foundation_map.json`
  - a human-readable foundation report at `research_uiux/CSD_UI_FOUNDATION_HUMANIZATION.md`
  - a local-only mirrored note layer now widened to `269` `*.sward.md` placement notes under `SONIC UNLEASHED/`
  - a local-only readable source layer with `125` `.cpp` humanization scaffolds under `SONIC UNLEASHED/`
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
- enough structure to start organizing translated findings under original source-family names for a broader UI-adjacent/debug/system subset
- enough structure to drive a native selector/workbench for frontend, frontend-sequence, town, camera, application/world, cutscene, gameplay-HUD, and stage-test host families
- enough structure to drive the first native GUI operator shell over the current contract-backed host catalog
- enough structure to bind a first curated set of local atlas sheets into that GUI without committing proprietary PNGs

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
- a first debug-oriented workbench that can exercise recovered menu, town, camera, application/world, cutscene, gameplay-HUD, and stage-test host families
- a first non-CLI operator shell for browsing hosts and driving contract-backed runtime actions
- a first visual preview panel that can draw local atlas sheets plus runtime overlay/prompt/timeline projections

What it still does not justify:

- claiming the whole game is now available as human-readable original source
- claiming every asset/state/animation has been exhaustively extracted and correlated
- treating the generated PPC output as a drop-in whole-game engine port

## Current Best Description

The current workspace is:

- `100%` complete against the tracked research plan through Phase `58`
- strong for UI/UX reverse-engineering and template extraction
- strong for local executable-backed timing/state archaeology
- strong for reusable runtime/template productization across C++, C, and C#
- strong for measured source-path organization inside the widened `269`-path UI-adjacent/debug/system/support subset
- now materially stronger for in-stage gameplay HUD ownership and family separation
- now materially stronger for lower-level CSD/project/widget naming and source-family organization
- now materially stronger for local-only source-family placement inside the mirrored `SONIC UNLEASHED/` tree
- now materially stronger for frontend shell/debug host recovery, especially pause/help dispatch, stage-change sequencing, town dispatch, and cutscene preview host triage
- now materially stronger for local-only readable source-family ownership, with `125` debug-oriented `.cpp` scaffolds living under the mirrored `SONIC UNLEASHED/` tree instead of only `*.sward.md` notes
- now materially stronger for runtime productization, with `203 / 269` broader source-path seeds (`75.5%`) backed by bundled contracts across frontend, frontend-sequence, town, camera, application/world, support-substrate, cutscene, gameplay-HUD, boss/final, and save/loading/title/world-map shell variants
- now materially stronger for source-path cleanup inside the current tracked seed, with `0` remaining `named_seed_only` paths in the present `269`-path subset
- now materially stronger for support-substrate coverage, with achievement, animation-event, player-status, sound, XML/data-loading, and wider camera-controller paths classified instead of left outside the curated seed
- now materially stronger for local-only support-substrate humanization, with `23` new achievement, animation-event, player-status, sound, and XML/data-loading scaffolds tied into the ignored mirror
- now materially stronger for native debug-tool usability, with a verified selector/workbench that now loop in interactive mode and a workbench `--catalog` view for inspecting the widened host topology
- now materially stronger for non-CLI debug-tool usability, with `b/rr58/sward_ui_runtime_debug_gui.exe` providing a native Windows group/host browser and runtime action controls over the same evidence-backed catalog
- now materially stronger for visual debug-tool usability, with `b/rr58/sward_ui_runtime_debug_gui.exe` drawing local atlas previews, runtime visible layers, prompt rows, state timeline strips, marked Sonic/Werehog gameplay-HUD proxy previews, timer-driven intro/action playback, eased state-aware preview motion, exact-family Title/Pause/Loading placement adapters, compact decoded layout-evidence panels, and frame-domain timeline readouts
- now materially stronger for host coverage, with a verified `176`-host workbench map across `11` groups
- partial for whole-game loose-file asset extraction
- not yet equivalent to a whole-game clean human-readable source tree

## Highest-Value Remaining Work

If the goal is to move closer to a broader `1:1` UI portability basis, the next concrete work would be:

1. Keep replacing the widened local-only debug-oriented source tree with cleaner translated ownership so the mirrored `SONIC UNLEASHED/` paths stop reading like scaffolds and start reading like readable source.
2. Convert one atlas-backed family into family-specific visual playback rather than a static preview sheet.
3. Keep widening the current contract-backed shell layer beyond the present `269`-path seed instead of treating the current subset as the whole game.
4. Continue correlating generated PPC seams against extracted layouts, source-path seeds, and readable patch hosts for the still-unmapped or still debug/tool-only families.
5. Keep tightening the local-only `SONIC UNLEASHED/` tree until the recovered source-family paths carry readable translated ownership at a much broader whole-game shell level.

> [!NOTE]
> After Phase 58, the current bottleneck is no longer just "make an executable" or "show a visual panel." The first non-CLI executable, local-atlas preview path, gameplay-HUD proxy preview, timer-driven contract playback, state-aware preview-motion adapter, exact-family Title/Pause/Loading placement layer, parsed layout-evidence overlay, and frame-domain timeline readout now exist. The remaining product gap is turning those adapters and evidence facts into family-specific visual playback backed by decoded layouts, original animation channels, exact HUD payloads, and translated host behavior.
