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
| Native non-CLI UI runtime workbench | Gameplay-HUD proxy preview plus timer playback/motion, first exact-family layouts, decoded layout-evidence overlay, frame-domain timeline readouts, exact-family scene primitives, audited gameplay-HUD proxy primitives, primitive animation/frame cues, readable primitive detail summaries, primitive channel cues, compact channel legends, visual parity summaries, host readiness badges, next-renderer blocker cues, first exact-family channel sample tokens, first exact-family draw command descriptors, first authored CSD cast/keyframe/sample/evaluation descriptors, first unobstructed asset-viewer mode, cwd-safe asset-root discovery, atlas-gallery controls, navigable CSD element-binding cues, selected-element crop previews, selected cast/subimage descriptors, source/destination draw-command descriptors, render-plan previews, and first decoded DDS source blits present | `b/rr89/sward_ui_runtime_debug_gui.exe` |
| Clean SU UI asset renderer | Separate native diagnostic viewer path now opens on a title-loop reconstruction backed by a local SFD movie frame, decompressed title-logo evidence, title text crops, visible screen/atlas navigation, no-window catalog smoke, local visual-atlas PNG gallery browsing, and DDS-backed evidence casts | `b/rr93/sward_su_ui_asset_renderer.exe`; `8` screen samples, `20` local DDS-backed evidence casts, `4` title-loop casts, `8` Sonic HUD reconstruction casts, `22` local atlas sheets, `5` visible controls |
| Real-runtime UnleashedRecomp UI Lab parity lane | Early-game alpha is now the default focus: title loop, title menu, title options, loading, and normal Sonic HUD; title/menu/loading direct-context routing is evidence-backed; normal Sonic HUD binds the real `ui_playscreen` CSD project; capture helper validates required runtime events; native BMP capture remains opt-in but now writes real nonblack title-loop and Sonic HUD backbuffer frames without stalling, with manifest-side RGB/nonblack signal summaries and optional required-signal gating | `local_build_env\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe`; default capture set `early-game` |
| GUI visual atlas bindings | First curated set plus HUD proxies, direct atlas-sheet viewer inventory, and sorted gallery navigation present | `10` contract-to-atlas candidates, including `2` proxy candidates; `22` local `visual_atlas/sheets` PNG files |
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
  - a Phase 59 GUI layout scene-primitive preview report at `research_uiux/GUI_LAYOUT_SCENE_PRIMITIVE_PREVIEW.md`
  - a Phase 60 gameplay HUD primitive preview report at `research_uiux/GUI_GAMEPLAY_HUD_PRIMITIVE_PREVIEW.md`
  - a Phase 61 gameplay HUD primitive ownership audit at `research_uiux/GUI_GAMEPLAY_HUD_PRIMITIVE_OWNERSHIP_AUDIT.md`
  - a Phase 62 GUI layout primitive playback-cues report at `research_uiux/GUI_LAYOUT_PRIMITIVE_PLAYBACK_CUES.md`
  - a Phase 63 GUI layout primitive detail-cues report at `research_uiux/GUI_LAYOUT_PRIMITIVE_DETAIL_CUES.md`
  - a Phase 64 GUI layout primitive channel-cues report at `research_uiux/GUI_LAYOUT_PRIMITIVE_CHANNEL_CUES.md`
  - a Phase 65 GUI layout primitive channel-legend report at `research_uiux/GUI_LAYOUT_PRIMITIVE_CHANNEL_LEGEND.md`
  - a Phase 66 GUI visual parity summary report at `research_uiux/GUI_VISUAL_PARITY_SUMMARY.md`
  - a Phase 67 GUI host readiness badge report at `research_uiux/GUI_HOST_READINESS_BADGES.md`
  - a Phase 68 GUI renderer blocker cue report at `research_uiux/GUI_RENDERER_BLOCKER_CUES.md`
  - a Phase 69 GUI layout channel sample cue report at `research_uiux/GUI_LAYOUT_CHANNEL_SAMPLE_CUES.md`
  - a Phase 70 GUI layout draw command descriptor report at `research_uiux/GUI_LAYOUT_DRAW_COMMAND_DESCRIPTORS.md`
  - a Phase 71 GUI authored cast transform descriptor report at `research_uiux/GUI_AUTHORED_CAST_TRANSFORM_DESCRIPTORS.md`
  - a Phase 72 GUI authored keyframe curve descriptor report at `research_uiux/GUI_AUTHORED_KEYFRAME_CURVE_DESCRIPTORS.md`
  - a Phase 73 GUI authored keyframe sample descriptor report at `research_uiux/GUI_AUTHORED_KEYFRAME_SAMPLE_DESCRIPTORS.md`
  - a Phase 74 GUI authored sampled transform descriptor report at `research_uiux/GUI_AUTHORED_SAMPLED_TRANSFORM_DESCRIPTORS.md`
  - a Phase 75 GUI authored sampled transform preview report at `research_uiux/GUI_AUTHORED_SAMPLED_TRANSFORM_PREVIEW.md`
  - a Phase 76 GUI authored sampled draw-command report at `research_uiux/GUI_AUTHORED_SAMPLED_DRAW_COMMANDS.md`
  - a Phase 77 GUI authored sampled channel-command report at `research_uiux/GUI_AUTHORED_SAMPLED_CHANNEL_COMMANDS.md`
  - a Phase 78 GUI authored sampled channel-evaluation report at `research_uiux/GUI_AUTHORED_SAMPLED_CHANNEL_EVALUATION.md`
  - a Phase 79 GUI asset-viewer mode report at `research_uiux/GUI_ASSET_VIEWER_MODE.md`
  - a Phase 80 GUI asset-root discovery and gallery-navigation report at `research_uiux/GUI_ASSET_GALLERY_ROOT_DISCOVERY.md`
  - a Phase 81 GUI asset CSD element-binding report at `research_uiux/GUI_ASSET_CSD_ELEMENT_BINDINGS.md`
  - a Phase 82 GUI asset CSD element-navigation report at `research_uiux/GUI_ASSET_CSD_ELEMENT_NAVIGATION.md`
  - a Phase 83 GUI asset CSD crop-preview report at `research_uiux/GUI_ASSET_CSD_CROP_PREVIEW.md`
  - a Phase 84 GUI asset CSD subimage draw-descriptor report at `research_uiux/GUI_ASSET_CSD_SUBIMAGE_DRAW_DESCRIPTORS.md`
  - a Phase 85 GUI asset CSD subimage draw-command report at `research_uiux/GUI_ASSET_CSD_SUBIMAGE_DRAW_COMMANDS.md`
  - a Phase 86 GUI asset CSD render-plan preview report at `research_uiux/GUI_ASSET_CSD_RENDER_PLAN_PREVIEW.md`
  - a Phase 87 GUI asset CSD DDS blit-preview report at `research_uiux/GUI_ASSET_CSD_DDS_BLIT_PREVIEW.md`
  - a Phase 88 clean SU UI asset-renderer vertical-slice report at `research_uiux/SU_UI_ASSET_RENDERER_VERTICAL_SLICE.md`
  - a Phase 89 clean SU UI asset-renderer composite-sheet report at `research_uiux/SU_UI_ASSET_RENDERER_COMPOSITE_SHEETS.md`
  - a Phase 90 clean SU UI renderer navigation-shell report at `research_uiux/SU_UI_RENDERER_NAVIGATION_SHELL.md`
  - a Phase 91 clean SU UI renderer atlas-gallery report at `research_uiux/SU_UI_RENDERER_ATLAS_GALLERY.md`
  - a Phase 92 clean SU UI renderer reconstructed-screen report at `research_uiux/SU_UI_RENDERER_RECONSTRUCTED_SCREEN.md`
  - a Phase 93 clean SU UI renderer title-loop reconstruction report at `research_uiux/SU_UI_RENDERER_TITLE_LOOP_RECONSTRUCTION.md`
  - a Phase 94+ real-runtime UI Lab pivot report at `research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md`
  - a shell/debug host recovery map at `research_uiux/data/frontend_shell_recovery.json`
  - a human-readable shell/debug host report at `research_uiux/FRONTEND_SHELL_AND_DEBUG_HOST_RECOVERY.md`
  - a dedicated frontend-sequence bridge report at `research_uiux/FRONTEND_SEQUENCE_SHELL_RUNTIME_BRIDGE.md`
  - a richer native host-bucket debug executable at `b/rr89/sward_ui_runtime_debug_workbench.exe`
  - a verified native selector with persistent interactive/stay-open behavior at `b/rr89/sward_ui_runtime_debug_selector.exe`
  - a first proper native GUI operator shell with visual gameplay-HUD proxy preview, state-aware motion, exact-family Title/Pause/Loading placement, `ui_mainmenu` / `ui_pause` / `ui_loading` layout-evidence overlays, recovered frame-domain timeline readouts, keyframe-density scene primitives, audited `ui_prov_playscreen` gameplay HUD proxy primitives, primitive animation/frame cues, readable primitive detail summaries, primitive channel cues, compact channel legends, visual parity summaries, host readiness badges, next-renderer blocker cues, first exact-family primitive channel sample tokens, first exact-family primitive draw command descriptors, first authored CSD cast/keyframe/sample/evaluation descriptors, first unobstructed local atlas asset-viewer mode, cwd-safe asset-root discovery, sorted atlas-gallery controls, navigable CSD element-binding cues, selected-element crop previews, selected cast/subimage descriptors, source/destination draw-command descriptors, render-plan previews, and first decoded DDS source blits at `b/rr89/sward_ui_runtime_debug_gui.exe`
  - a separate clean asset-backed renderer at `b/rr93/sward_su_ui_asset_renderer.exe`, currently smoke-guarding a title-loop reconstruction backed by the local `evmo_title_loop.sfd` preview frame, decompressed `OPmovie_titlelogo_EN` evidence, `mat_title_en_001.dds` title text crops, visible navigation shell, a local `22`-sheet visual-atlas gallery, a full-screen Loading composition, and Main Menu / Sonic HUD DDS-backed samples
  - a real-runtime UI Lab path inside the generated UnleashedRecomp build at `local_build_env\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe`, with visible overlay controls, title/menu/loading route forcing, proven direct title-menu CSD-completion forcing, owner-output loading routing, stage-context observation, JSONL evidence logging, native backbuffer BMP capture instrumentation, passive capture/evidence observer safety, native frame-series count/interval controls, native BMP signal summaries, optional required native RGB-signal gating, target-aware native best-frame scoring, per-target native capture cadence plans, native-only capture-helper runs that skip Windows screenshots, and a local-only screenshot/event capture helper that now validates target evidence, gates late screenshots on real runtime events, and can write nonblack native title-loop / loading / Sonic HUD BMPs from the real backbuffer without stalling
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

- `100%` complete against the tracked research plan through Phase `112`
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
- now materially stronger for non-CLI debug-tool usability, with `b/rr89/sward_ui_runtime_debug_gui.exe` providing a native Windows group/host browser, host readiness badges, asset/runtime preview mode switching, asset-sheet gallery controls, CSD element controls, selected-element crop previews, selected cast/subimage cues, source/destination draw-command cues, render-plan previews, decoded DDS source blits, and runtime action controls over the same evidence-backed catalog
- now materially stronger for visual debug-tool usability, with `b/rr89/sward_ui_runtime_debug_gui.exe` drawing local atlas previews, runtime visible layers, prompt rows, state timeline strips, marked Sonic/Werehog gameplay-HUD proxy previews, timer-driven intro/action playback, eased state-aware preview motion, exact-family Title/Pause/Loading placement adapters, compact decoded layout-evidence panels, frame-domain timeline readouts, exact-family scene-primitive overlays, audited gameplay HUD proxy primitives, primitive animation/frame cues, readable primitive detail summaries, primitive channel cues, compact channel legends, visual parity summaries, host-list readiness badges, next-renderer blocker cues, first exact-family primitive channel sample tokens, first exact-family draw command descriptors, first authored CSD cast/keyframe/sample/evaluation descriptors, first unobstructed local atlas asset-viewer path, cwd-safe asset-root discovery, sorted atlas-gallery controls, navigable CSD element-binding cues, selected-element crop previews, selected cast/subimage descriptors, source/destination draw-command descriptors, render-plan previews, and first decoded DDS source blits for exact/proxy/readiness triage
- now materially stronger for product-facing UI renderer direction, with `b/rr93/sward_su_ui_asset_renderer.exe` separating clean asset-backed screen rendering from the archaeology/debug workbench, opening on a title-loop reconstruction that binds the local SFD movie frame, decompressed title-logo evidence, title text crops, and translated/patch state seams instead of a raw atlas gallery, while retaining visible screen/atlas navigation controls, browsing local visual-atlas PNG sheets, and exposing no-window renderer catalog/title/reconstruction smoke
- now materially stronger for real-runtime UI parity work, with the generated UnleashedRecomp UI Lab able to route title/menu/options/loading targets through live translated runtime states, prove direct title-menu entry through the real CSD completion byte, open the visible options path through title-menu cursor index `2`, keep owner-output routing isolated to loading/stage paths, emit frame/state/loading/CSD JSONL evidence, capture stable local-only screenshots/events by default, keep manual operator sessions alive, expose selected routes from an in-game overlay, run a passive manual observer mode that leaves normal game flow intact while still recording evidence, keep evidence/native-capture-only launches from route-forcing normal savefile flow, bind the normal Sonic HUD route to the real `ui_playscreen` CSD project after `CGameModeStage::ExitLoading`, evidence-gate late captures so the first normal Sonic HUD alpha frame is captured after the real CSD bind instead of by a blind timer, write real nonblack native title-loop / loading / Sonic HUD BMPs from the intermediary backbuffer without stalling the renderer, and select native best frames by target route/cadence instead of brightness alone
- now materially stronger for host coverage, with a verified `176`-host workbench map across `11` groups
- partial for whole-game loose-file asset extraction
- not yet equivalent to a whole-game clean human-readable source tree

## Highest-Value Remaining Work

If the goal is to move closer to a broader `1:1` UI portability basis, the next concrete work would be:

1. Keep replacing the widened local-only debug-oriented source tree with cleaner translated ownership so the mirrored `SONIC UNLEASHED/` paths stop reading like scaffolds and start reading like readable source.
2. Keep advancing the real-runtime UI Lab instead of spending parity time on standalone approximations: the first useful alpha should stay on title loop, title menu, options, loading, and normal Sonic HUD before returning to Werehog, Extra/Tornado, boss/final, and broader late-game UI.
3. Keep widening the current contract-backed shell layer beyond the present `269`-path seed instead of treating the current subset as the whole game.
4. Continue correlating generated PPC seams against extracted layouts, source-path seeds, and readable patch hosts for the still-unmapped or still debug/tool-only families.
5. Keep tightening the local-only `SONIC UNLEASHED/` tree until the recovered source-family paths carry readable translated ownership at a much broader whole-game shell level.

> [!NOTE]
> After Phase 112, the clean renderer is explicitly a diagnostic sidecar. The parity lane is the generated UnleashedRecomp UI Lab because it runs the real game renderer, CSD/material/movie substrate, local game files, and translated runtime states. The first alpha scope is intentionally early-game-visible: title loop, title menu, title options, loading, and normal Sonic HUD. Title-menu direct-context entry is backed by CSD completion evidence, but its current visual capture still reads as the title / Press Start screen and needs a stronger menu-ready latch. The options target opens through the real title-menu options path, loading now selects a real `NOW LOADING` native BMP from `loading display active`, normal Sonic HUD binds `ui_playscreen` through the real runtime, and the capture helper now waits for required evidence before taking late frames. Screenshot/event capture is the stable default; native backbuffer BMP capture remains opt-in but is now RGB-gated and target-aware for the early-game set. Capture/evidence-only launches are passive observer runs unless a target screen is explicitly requested. Extra/Tornado `ui_prov_playscreen` is deferred until the first alpha is useful.
