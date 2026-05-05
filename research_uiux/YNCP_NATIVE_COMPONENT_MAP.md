# YNCP Native Component Map

Generated from the fully extracted current install. This is a local R&D map for turning authored Sonic Unleashed UI projects into reusable SGFX/native screen components.

Phase 236 wires this map into the in-game SWARD UI Lab `UI Projects` browser. It is runtime-derived-native-reconstruction metadata, not original authored SEGA source.

Phase 237 adds an independent drawable/keyframe preview lane for the selected scene row. The preview is driven by parsed project/cast/animation/track metadata and is not tied to the current gameplay/title/loading route, so it can be inspected while the running game is sitting in a HUD, hub, title, loading, or other state. The adjacent audio-bank correlation lane keeps SFX as placeholder cue intents until audio banks/XML, runtime hooks, and Ghidra xrefs prove exact cue IDs.

Phase 238 binds parsed casts to real extracted DDS texture names, subimage UV rectangles, source texture dimensions, and preview destination rectangles. The in-game UI Projects canvas now has a texture-backed cast/subimage preview layer using real DDS/subimage rectangles, while the SFX lane begins with runtime-hook + Ghidra xref SFX candidates such as `se_system_worldmap/sys_worldmap_decide` from `CTitleStateMenu::Update`.

Phase 239 adds a composed scene display list from the YNCP cast hierarchy. Scene draw commands apply cast tree parent transforms, `cast_info` translation/scale, selected material subimage slots, texture UVs, and extracted DDS paths so the in-game preview renders a selected scene/component instead of only showing a raw atlas crop.

Phase 240 adds keyframe interpolation/scrubbing from real authored animation tracks. The generated native map now carries preview-useful YNCP transform keyframes (`XPosition`, `YPosition`, `XScale`, `YScale`, `Rotation`, `HideFlag`, `SubImage`) with frame, value, tangent, and interpolation provenance so the in-game canvas can sample actual authored motion while the adjacent SFX lane keeps same-scene cue candidates beside it.

Phase 241 adds the in-game foreground invoke mode: the selected YNCP scene can be projected over gameplay as a pause-menu-style summoned UI surface, outside the inspector/browser panel, while still using the reconstructed scene draw commands, keyframe scrub/playback, and same-row SFX correlation data.

Phase 242 adds the guarded Native CSD Make Probe plan to the runtime bridge: selected .yncp/.xncp project bytes are header-checked as raw CPAF/NYIF/nCPJ CSD packages, copied into guest heap, and handed to `SWA::CCsdProject::Make`/`sub_825E4068` through a cloned PPC probe context, with `MakeCsdProjectMidAsmHook` traversal used as proof that the game parsed the project into a native CSD tree before render-host attachment is attempted.

Phase 243 hardens that native probe after the crash evidence showed `sub_825E4068` reads a required fourth argument through `r6`. The button now queues work until the next real `CCsdProject::Make` call, captures that live `r6` make context, and only then attempts the cloned native call; loading/save update ticks no longer fire the native Make probe with unrelated registers.

Phase 244 changes the default probe into a non-crashing native observe mode after the next crash showed direct Make can still fail when the internal package parser returns a null temporary resource. By default, the probe now waits for the game to create the selected project naturally and captures the resulting native tree at `MakeCsdProjectMidAsmHook`; a separate `danger: execute Make` checkbox keeps the direct call path available only as an explicit experiment.

Phase 245 adds the Native Foreground Render Probe. Once native observe has resolved a selected scene pointer, `Attach Native Scene` / `Spawn Native Foreground` arms that real CSD scene and piggybacks it on the next active `CScene::Render` pass, with motion scrub/playback writing the native `m_MotionFrame`. This is deliberately not an owner pointer hijack yet; render-host proof comes first, and foreground owner replacement stays a later, riskier experiment.

Phase 246 exposes that native foreground path through the live bridge as direct operator controls: `native-foreground-status`, `native-foreground-attach`, `native-foreground-detach`, `native-motion-play`, `native-motion-stop`, and `native-motion-scrub <frame>`. The bridge returns the native project/root/scene pointers, scene motion frame/repeat fields, render-pass sightings, render count, and the explicit `ownerHijackUsed=false` safety status so render-count proof can be gathered before any foreground owner attach experiment.

Phase 247 adds a tool-facing `native-make-observe <project> <scene> [frame]` bridge command. It queues the selected YNCP/XNCP project row by project name/path and scene name, then waits for the running game to create that CSD project naturally so `MakeCsdProjectMidAsmHook` can capture the native tree without requiring manual panel clicks or the dangerous direct Make path.

Phase 250 guards the Native Foreground Render Probe with a same-project host requirement. The bridge can resolve and arm a native scene from the observed game-created tree, but render piggyback now refuses to call `CScene::Render` unless the active host pass belongs to the same captured CSD project; cross-project foreground spawning is deferred to a real owner/host attach path.

Phase 251 correlates CSD resource `Scene*` records from the native project traversal with live renderable `CScene*` manager instances from the `CScene::Render` hook. Motion scrub/playback now require `nativeManagerScenePointer` plus `nativeResourceScenePointer` provenance, so the lab no longer writes manager fields on the non-renderable resource tree object. The foreground attach path is held as a same-project compatibility probe until a real foreground owner/host is resolved, because an extra render call without ownership proof is still crash-prone.

Phase 252 adds Foreground Owner/Host Attach Discovery. The live bridge can run `native-owner-discovery` / `native-owner-scan` to inspect bounded known UI owner ranges (title owner context, CHudSonicStage owner, CHudPause owner, CGeneralWindow owner, CSaveIcon owner) for direct or indirect references to the correlated manager CScene/resource Scene pair before any owner/host attach or pointer hijack is attempted.

Phase 253 adds Owner Layout Mapping. Once the title owner context exposes the observed `0x1E4` CSD field, the bridge can run `native-owner-layout` to map sibling owner CSD fields around that anchor and record read-only layout evidence for title owner layout first, then HUD owner layout when the CHudSonicStage owner range is live. This keeps native foreground attach on the real owner path instead of guessing render calls.

Phase 254 adds Owner Layout Semantic Naming. The owner layout map now annotates each sibling field with a semantic name/role, owner lifecycle note, attach setter candidate, and Ghidra xref oracle status. Title mapping starts with `titleContext.m_rcTitleManager` at `0x1E4` and `titleContext.m_rcTitleResource` at `0x1E8`; HUD owner layout stays `HUD owner layout pending runtime gameplay evidence` until gameplay samples prove the same attach/setter path.

Phase 255 adds Native Owner Setter Probe. Read-only wrappers around `sub_8250F2B8`, `sub_82581288/A8/E8`, and HUD helpers `sub_82E5FCD0 / sub_82E61A78` record helper arguments, owner fields, result registers, and DB/XML route evidence beside the native owner layout map. This confirms the real attach/setter path before any foreground owner write or hijack experiment.

Phase 256 adds HUD Owner Setter Field Map. The native owner setter probe now translates HUD helper traffic back to probable owning UI objects when `argR3 == owner+0x28`, falls back to `argR3 - 0x28` inferred owner for the proven HUD stage-bind callsites, and labels the generated `CHudSonicStage::sub_824D9308` callsites where `argR5=110` maps `owner+0xD8 -> owner+0xE0` and `argR5=121` maps the `owner+0xF0/+0xF4` scene update path. This still stays read-only, but it turns anonymous helper calls into attach-path field evidence.

Phase 257 adds HUD Owner Layout Field Map. The setter probe seeds an `inferred CHudSonicStage owner` global from the proven `argR3 - 0x28` HUD helper traffic and feeds it into the bounded owner-layout scanner so sibling owner CSD fields can be enumerated even when the constructor-time CHudSonicStage hook has not landed yet. The owner layout semantic resolver now names `hudOwner.attachSourceScene` at `owner+0xD8`, `hudOwner.attachTargetScene` at `owner+0xE0`, `hudOwner.activeUpdateScenePrimary` at `owner+0xF0`, and `hudOwner.activeUpdateSceneCompanion` at `owner+0xF4`, with parallel `hudOwnerInferred.*` names when the inferred owner is the source. The attach setter writes still stay disabled until the lifecycle is proven through Ghidra xref export.

Phase 258 adds HUD Owner Slot Readout. Whenever a HUD setter helper sample confirms the constructor-side or inferred CHudSonicStage owner, the lab now directly reads `owner+0xD8/0xE0/0xF0/0xF4` and classifies each value as `live-manager-scene`, `resource-scene`, `indirect-manager-scene`, `indirect-resource-scene`, `uncorrelated-pointer`, `non-pointer`, or `null`. Each new slot kind/value emits a deduped `native-owner-setter-hud-owner-slot-readout` event and the latest readout per slot is mirrored into the live-state JSON so the operator can see whether `owner+0xD8/0xE0` already point at a known renderable scene before any guarded native foreground attach is tried.

Phase 259 adds HUD Setter Probe Hot-Path Filter. The HUD setter helpers `sub_82E5FCD0` and `sub_82E61A78` fire thousands of times per second during gameplay, so unconditional sample recording wrote ~12 events/sec of dead JSONL traffic and drove gameplay framerate below 20fps. `OnHudOwnerSetterProbe` now skips full sample recording for any helper traffic whose `argR5` is not the proven CHudSonicStage::sub_824D9308 callsite (`argR5=110` for `sub_82E5FCD0`, `argR5=121` for `sub_82E61A78`), emits a one-shot `native-owner-setter-hud-helper-first-seen` breadcrumb the first time each unexpected `(helper, argR5)` tuple appears, and exposes `skippedHudOwnerSetterProbeCallCount` / `recordedHudOwnerSetterProbeCallCount` perf counters in the live-state JSON.

Phase 260 adds HUD Owner Renderable-Slot Sweep. The four hardcoded slot readouts only cover the proven `+0xD8/0xE0/0xF0/0xF4` offsets, so this phase auto-runs a bounded sweep across the full `0x3000`-byte CHudSonicStage owner range as soon as the live owner is seeded (constructor-confirmed or inferred). Every owner field whose value (direct or indirect through one dereference) resolves to a live manager `CScene` emits a `native-hud-owner-renderable-slot` event with full project/scene/manager/resource provenance, the sweep itself emits one `native-hud-owner-layout-sweep-complete` per owner address, and the discovered slots are mirrored into the live-state JSON. The sweep is gated to one execution per unique owner address per session so steady-state HUD frames pay no recurring cost. First gameplay run found `owner+0xF4 -> ui_playscreen/so_ringenagy_gauge` as a live indirect-manager-scene slot.

Phase 261 adds HUD Owner Field Naming + Cross-Validation. The first full gameplay sweep returned 10 renderable HUD owner slots: `+0xEC` (`m_rcSpeedGauge` / `so_speed_gauge`), `+0xF4` (`m_rcRingEnergyGauge` / `so_ringenagy_gauge`), `+0xFC` (`m_rcGaugeFrame` / `gauge_frame`), three medal-get scene refs at `+0x1958/+0x1964/+0x19C4` sharing a single `medal_get_m` backing block, three speed-count scene refs at `+0x1B58/+0x1B64/+0x1BC4` sharing a single `speed_count` backing block, and one direct manager-scene cache at `+0x1E40` for `score_count`. The slot readout sampler now covers all 13 named offsets, the owner-layout semantic resolver names them with the SWA HUD field naming convention (`m_rc*`) for the constructor expected-fields entries and descriptive names for the deep-cluster runtime-discovered fields, and a new `native-hud-owner-field-cross-validated` event fires whenever a sweep find lands at the same `rcObjectOffset` the constructor `kChudSonicStageExpectedOwnerFields` table predicts so the two paths confirm each other.

Phase 262 adds HUD Owner Layout JSON Sidecar. After every successful owner sweep the lab writes a `hud_owner_layout.json` file beside `ui_lab_live_state.json` in the active evidence dir. The schema is explicitly versioned (`sward-hud-owner-layout-v1`) and combines the constructor expected-fields table, every named slot readout, every renderable slot the sweep discovered (with `crossValidatedExpectedField` populated when an offset matches the expected-fields table), and the owner address / source / frame stamp the sidecar was written at. The live-state JSON also surfaces `hudOwnerLayoutSidecarPath` so downstream SGFX HUD code generators have a single, structured artifact to consume — no more parsing JSONL events to reconstruct the layout. A `native-hud-owner-layout-sidecar-written` event fires each time the sidecar is rewritten.

Phase 263 adds HUD Owner Sweep Re-run on Correlation Growth. The first session-shipping version of Phase 260 only swept each owner once, which raced against the `CCsdProject::Make` correlation buildup — a sweep at constructor-hook time often saw only ~80 correlations even though the full session would later end with 380+, so the sweep found nothing renderable and never re-ran. The cache is now a `g_lastHudOwnerSweepStates` map of `(lastSweepFrame, lastCorrelationCount)` per owner address, and a re-sweep fires whenever `g_csdManagerSceneCorrelations.size()` has grown by at least `kHudOwnerSweepMinReSweepCorrelationGrowth` (16) AND at least `kHudOwnerSweepMinReSweepIntervalFrames` (600) frames have passed since the last sweep. Renderable-slot pushes are now gated by the same dedup-key set so re-sweeps never duplicate the in-memory vector or sidecar.

Phase 264 adds HUD Renderable-Slot Grouping + Confidence Tiers. The first full re-sweep gameplay run discovered 60 renderable slots, but 22 of them all pointed at the same `ui_playscreen/ring_get` manager scene (the HUD ring-pickup animation pool) and 34 came from the inferred-owner sweep that latched on a sub-object 0x300 bytes off from the real CHudSonicStage owner. Each renderable slot now carries a `confidenceTier` of `cross-validated` (offset matches `kChudSonicStageExpectedOwnerFields`), `constructor-confirmed` (sweep ran on the constructor-side owner pointer but offset is outside the expected-fields table — the deep-cluster discoveries like `ring_get` live here), or `inferred-owner` (sweep ran on the `argR3 - 0x28` inferred owner; treat as low-confidence). The sidecar JSON also exposes a `renderableSlotGroups[]` view that groups slots by `(ownerSource, managerSceneAddress)` so a 22-slot ring-pickup pool collapses to one group with `instanceCount: 22` and `fieldOffsets[...]`. The live-state JSON surfaces per-tier counts so SGFX HUD code generators can filter on confidence.

Phase 265 adds CHudSonicStage Constructor Decode. The `sub_824D89B0` constructor in `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.28.cpp:61909` initializes 21 sequential `RCPtr<T>` fields at every 8-byte offset from `this+0xE0` through `this+0x180`, but the existing `kChudSonicStageExpectedOwnerFields` table only named 9 of them — the SWA HUD members documented in `api/SWA/HUD/Sonic/HudSonicStage.h`. The expanded table now carries all 21 RCPtrs. `m_rcExpCount` at `0x100/0x104` is named from the Phase 260 sweep evidence (`+0x104` resolved to `ui_playscreen/exp_count`) plus the constructor decode confirming the RCPtr exists; the 11 still-unnamed RCPtrs use honest offset-based names (`m_rcPtrField108`, `m_rcPtrField110`, …) so their existence is recorded without inventing semantic claims. The owner-layout semantic resolver gains a dedicated case for `+0x104 m_rcExpCount` so future cross-validations also cover this slot.

Phase 266 adds the first human-readable C++ port artifact. `research_uiux/tools/build_sgfx_hud_layout.py` parses `kChudSonicStageExpectedOwnerFields` from the runtime patches, joins it with the latest `hud_owner_layout.json` sidecar evidence, and emits `research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_hud_chud_sonic_stage.generated.h` — a real `class CHudSonicStage` declaration in `namespace sward::ui_runtime::generated::sgfx_hud` with `RCPtr<T>` data members at every runtime-confirmed offset, `static_assert(offsetof(...))` guards on every member, and a `kSceneBindings[]` registry that names which CSD project / scene path each renderable RCPtr is populated from. The first generator run produced 21 RCPtr members and 4 cross-validated scene bindings (gauge cluster + the just-promoted `m_rcSpeedCount` at `+0x118`, named after the sweep cross-validated it against `ui_playscreen/add/speed_count`). Method bodies (constructor / destructor / Update / Render) are out of scope for this layout-only header and will be ported in subsequent phases as their recomp flow is decoded.

Phase 267 adds type-aware reconciliation with the existing UnleashedRecomp SWA API header. The team had already partially decoded `class CHudSonicStage` at `local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/Sonic/HudSonicStage.h` with the proper template-argument types (`RCPtr<CProject>` for the play-screen project, `RCPtr<CScene>` for the gauge cluster, `RCPtr<CNode>` for the count fields) but only named 9 of the 21 RCPtrs the constructor decode revealed. The generator now parses that SWA API header, applies the SWA-named type per member, and falls back to `Chao::CSD::CScene` for the runtime-discovered RCPtrs the SWA API header has not yet named. Each generated member declaration carries a `type from SWA API header` or `type defaulted to CScene; SWA API header has not yet named this RCPtr` annotation so a reader knows whether the type is authoritative or runtime-evidence. The forward-declaration block enumerates the union of every type used (`class CNode; class CProject; class CScene;` for CHudSonicStage), a `Chao::CSD::` namespace alias matches the SWA convention, and the SWA API header path is cited in the preamble as authoritative.

Phase 268 generalizes the generator to multiple HUD owner classes. A new `parse_swa_api_class()` parser handles arbitrary single-class SWA API headers — RCPtr members, scalar members (`bool`, `be<T>`, integer types), enum definitions, and `SWA_ASSERT_OFFSETOF` entries — and a paired `emit_swa_api_class_header()` lays out the corresponding port class flat with byte padding so each member lands at its SWA-asserted offset. The default generator run now produces both `sgfx_hud_chud_sonic_stage.generated.h` (extended-table path) and `sgfx_hud_chud_pause.generated.h` (SWA-API-driven path); the latter ships `class CHudPause` with 8 RCPtr members, 7 scalar members (`m_IsVisible`, `m_Action`, `m_Menu`, `m_Status`, `m_Transition`, `m_Submenu`, `m_IsShown`), 4 enum definitions (`EActionType`, `EMenuType`, `EStatusType`, `ETransitionType`), 15 `static_assert(offsetof(...))` guards, and a thin `be<T>` wrapper struct so the big-endian SWA storage type round-trips at the layout level. CGameObject inheritance is modeled as leading byte padding to keep the generated header self-contained.

Phase 269 extends the generator with a sweep mode that walks every SWA API HUD header under `local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/` and emits one port header per parseable class plus a `sgfx_hud_layout_manifest.generated.json` index. The parser now accepts enums without an explicit underlying type (defaulting to `int32_t`) and class declarations whose base class lives in a multi-namespace path with optional `public` access (`class CSaveIcon : Hedgehog::Universe::CUpdateUnit`). Two new layout headers ship: `sgfx_hud_cgeneral_window.generated.h` (6 RCPtrs + 3 scalar / `be<T>` enums + 1 enum) and `sgfx_hud_cloading.generated.h` (1 RCPtr + 4 scalars + 1 enum); SaveIcon is logged as skipped because its SWA API header carries no `SWA_ASSERT_OFFSETOF` entries to anchor the layout against.

Phase 270 emits inline accessor methods for every scalar / enum member of an SWA-API-driven port class. Each accessor is a `const noexcept` getter derived purely from the SWA API header: `bool m_IsVisible` becomes `bool isVisible() const noexcept { return m_IsVisible; }`; `be<EActionType> m_Action` becomes `EActionType getAction() const noexcept { return static_cast<EActionType>(m_Action.m_storage); }`; `be<uint32_t> m_Submenu` returns the wrapped value as-is. RCPtr accessors are intentionally deferred until the SWA RCObject runtime is linked. These are the first method bodies in the human-readable port — small, mechanical, fully derivable from the SWA header, and they make the generated classes immediately usable for read-only inspection.

Phase 271 closes the asset-consumption loop. `research_uiux/tools/validate_sgfx_hud_asset_bindings.py` reads every `kSceneBindings[]` row out of the generated layout headers, joins it with the YNCP native component map to resolve each `projectName` to its on-disk `.yncp` file under `extracted_assets/full_install_archives/`, and verifies the file exists with the expected `CPAF` / `YNCP` / `XNCP` magic. The output `sgfx_hud_asset_binding_validation.generated.json` reports per-binding pass/fail with the resolved asset path, file size, and a status string (`ok` / `asset-ok-scene-list-empty` / `missing-asset` / `bad-magic` / `unresolved-project` / `unresolved-scene`). The first validator run resolved all four cross-validated CHudSonicStage gauge-cluster bindings (`m_rcSpeedGauge`, `m_rcRingEnergyGauge`, `m_rcGaugeFrame`, `m_rcSpeedCount`) to live `.yncp` assets in `extracted_assets/full_install_archives/game/Sonic/ui_playscreen.yncp` — the human-readable port now demonstrably consumes the user's extracted retail assets.

Phase 272 closes the per-scene cross-check loop. The validator now composes scene paths from each YNCP project's `(node_path, scene_name)` pair: `Root` + `so_speed_gauge` becomes `ui_playscreen/so_speed_gauge`; `Root/add` + `speed_count` becomes `ui_playscreen/add/speed_count`. With the composition fix in place, all four cross-validated gauge-cluster bindings now report `ok` — the YNCP file has the named scene and the binding is fully verified.

Phase 273 / 276 ships the first real method bodies in the human-readable port. `research_uiux/runtime_reference/src/sgfx_hud_chud_pause_methods.cpp` is hand-written C++ on top of the Phase 268 / 270 generated CHudPause layout: `isPauseQuitDialogArmed()`, `isPauseInteractive()`, `isPauseShowingSubmenu()`, `isPauseMiscMenuActionAccepted()`. Each helper reads through the auto-generated inline accessors (so it never touches the raw `be<T>` storage), references the SWA `EActionType` / `EMenuType` / `EStatusType` / `ETransitionType` enum literals by name, and stays implementation-light enough to unit-test standalone without linking the SWA executable. Concludes with a `static_assert(sizeof(CHudPause) >= 0x1B9)` so a future regen that breaks the layout fails the build here too.

Phase 274 unblocks the SaveIcon class. The SWA API header for `CSaveIcon` declares only `SWA_INSERT_PADDING(0xD8); bool m_IsVisible;` with no `SWA_ASSERT_OFFSETOF` lines, so the previous sweep skipped it. The generator now falls back to walking the SWA_INSERT_PADDING directives in source order when no asserts exist (consistent with the recomp confirming `m_IsVisible` is at `+0xD8` via `lwz r3, 216(r31)` in `sub_824E5170`). The third sweep entry now ships `sgfx_hud_csave_icon.generated.h` with `class CSaveIcon`, `m_IsVisible` at `+0xD8`, and a `static_assert(offsetof(CSaveIcon, m_IsVisible) == 0xD8)` guard.

Phase 275 adds a header-only C++ loader for Sonic Unleashed CSD project files. `research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_hud_csd_project_loader.hpp` declares `struct CsdProjectFile` and `loadCsdProjectFile(path)` that reads a `.yncp` (or `.xncp`) file from disk, validates the SWA / Hedgehog Engine `CPAF` outer magic plus the inner `YNCP` / `XNCP` payload magic, and surfaces the result via a `loadStatus` string instead of exceptions. Pure C++17 standard library — `<cstdint>`, `<cstring>`, `<filesystem>`, `<fstream>`, `<optional>`, `<string>`, `<vector>` — and a paired `sgfx_hud_csd_project_loader_smoke_test.cpp` translation unit drives the loader against the real extracted `ui_playscreen.yncp` for a one-shot end-to-end check. Decoding individual scenes / casts / animations from the YNCP payload is deferred to a follow-up phase.

Phase 277 adds CHudSonicStage method bodies (`research_uiux/runtime_reference/src/sgfx_hud_chud_sonic_stage_methods.cpp`) following the Phase 273 pattern: hand-written predicates / queries on top of the runtime-extended generated layout. `findSceneBindingByMember()`, `hasFullGaugeClusterBindings()`, `countCrossValidatedBindings()`, `ownedCsdProjectName()`, and `isAdditiveClusterBinding()` are real method bodies that consume the `kSceneBindings[]` registry and return derived state without touching the recomp runtime.

Phase 278 adds a PowerShell smoke-test wrapper (`research_uiux/runtime_reference/tools/build_sgfx_hud_smoke_tests.ps1`) that drives `clang-cl` + the VS Build Tools developer environment + LLVM toolchain the main UI Lab build wraps. It compiles every smoke-test translation unit under `research_uiux/runtime_reference/src/`, runs them against the user's extracted `ui_playscreen.yncp`, and aggregates compile / run failures into a single non-zero exit so it can plug straight into CI. The first smoke run produced `loadStatus = ok` against the 198 KB retail asset.

Phase 279 extends the C++ loader with real CPAF chunk + NCPJ + scene-id extraction. `loadCsdProjectFile()` now walks the outer `CPAF` / `FAPC` container, follows the chunk header to the embedded NCPJ project chunk, and reads the project's root CSD node `scene_id_table` to produce `rootSceneIds[]`. The first smoke run against `extracted_assets/full_install_archives/game/Sonic/ui_playscreen.yncp` extracted the 9 root scene names: `so_speed_gauge`, `so_ringenagy_gauge`, `gauge_frame`, `player_count`, `time_count`, `score_count`, `ring_count`, `ring_get`, `exp_count`.

Phase 280 ports a heavier method body for CHudSonicStage and recursively walks the CSD node tree in the C++ loader. `releaseAllOwnedScenes()` mirrors the SWA `sub_824D8CE8` destructor cleanup order (RCPtrs released in reverse construction order); `isCHudSonicStageInPostConstructorState()` ports the post-`sub_824D89B0` zero-initialization invariant; `isPlayScreenAssetSideReadyForBinding()` proves end-to-end asset consumption by loading the extracted YNCP and verifying every cross-validated `kSceneBindings[]` entry has a matching `(nodePath, sceneName)` pair in the parsed project. Loader's recursive node walk exposes `allSceneRefs[]` covering both root scenes and nested child clusters (e.g. the `Root/add/speed_count` SWA Sonic stage HUD overlay), bounded to depth 6 to prevent runaway recursion on corrupt headers. All 9 smoke checks pass against the live retail asset.

- Input root: `C:/Users/DavidErikGarciaArena/Downloads/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/full_install_archives`
- Project files parsed: `41`
- Preview draw commands: `1888` real-yncp-subimage-dds-rect rows.
- Scene draw commands: `7530` real-yncp-cast-tree-subimage-scene-rect rows.
- Animation keyframes: `20091` real-yncp-animation-keyframe rows for keyframe interpolation.
- SFX cue candidates: `4` runtime-hook + Ghidra xref SFX candidates.
- SFX correlation: `sfx-correlation-pending` until audio banks/XML/runtime hooks/Ghidra xrefs prove every exact cue ID.

## Hub Town

### `ui_gate`
- Path: `game/ActionCommon/ui_gate.yncp`
- Scenes: `16`; casts: `163`; animations: `30`; textures: `13`
- Root scenes: ``
- Component roles: `result_rank=15, title_logo=1`
- Texture-backed preview commands: `51`
- Composed scene draw commands: `116`
- Animation keyframes: `181`
- Key scenes:
  - `area_tag` -> `result_rank` casts=6 anims=1 frames=[15.0, 15.0]
  - `bg` -> `result_rank` casts=76 anims=2 frames=[20.0, 20.0]
  - `stage_name` -> `result_rank` casts=2 anims=2 frames=[0.0, 20.0]
  - `info_1` -> `result_rank` casts=4 anims=2 frames=[0.0, 20.0]
  - `info_2` -> `result_rank` casts=8 anims=2 frames=[0.0, 20.0]
  - `info_3` -> `result_rank` casts=7 anims=2 frames=[20.0, 59.0]
  - `info_4` -> `result_rank` casts=7 anims=2 frames=[20.0, 59.0]
  - `info_5` -> `result_rank` casts=6 anims=2 frames=[20.0, 59.0]
  - `stage_ss` -> `result_rank` casts=4 anims=2 frames=[0.0, 20.0]
  - `info_6` -> `result_rank` casts=3 anims=2 frames=[0.0, 20.0]
  - `arrow` -> `result_rank` casts=2 anims=1 frames=[0.0, 0.0]
  - `window_bg` -> `result_rank` casts=1 anims=1 frames=[5.0, 5.0]

### `ui_missionscreen`
- Path: `game/ActionCommon/ui_missionscreen.yncp`
- Scenes: `7`; casts: `134`; animations: `19`; textures: `9`
- Root scenes: `player_count, time_count, laptime_count, score_count, item_count, position, lap_count`
- Component roles: `hud_life=1, hud_score_time=4, town_dialog=2`
- Texture-backed preview commands: `22`
- Composed scene draw commands: `93`
- Animation keyframes: `243`
- Key scenes:
  - `player_count` -> `hud_life` casts=4 anims=2 frames=[100.0, 100.0]
  - `time_count` -> `hud_score_time` casts=32 anims=4 frames=[100.0, 100.0]
  - `laptime_count` -> `hud_score_time` casts=32 anims=4 frames=[100.0, 100.0]
  - `score_count` -> `hud_score_time` casts=26 anims=4 frames=[5.0, 100.0]
  - `item_count` -> `hud_score_time` casts=27 anims=3 frames=[5.0, 29.0]
  - `position` -> `town_dialog` casts=5 anims=1 frames=[100.0, 100.0]
  - `lap_count` -> `town_dialog` casts=8 anims=1 frames=[100.0, 100.0]

### `ui_misson`
- Path: `game/ActionCommon/ui_misson.yncp`
- Scenes: `6`; casts: `38`; animations: `7`; textures: `7`
- Root scenes: `bg`
- Component roles: `pause_shell=1, result_rank=4, title_logo=1`
- Texture-backed preview commands: `18`
- Composed scene draw commands: `27`
- Animation keyframes: `56`
- Key scenes:
  - `bg` -> `result_rank` casts=1 anims=1 frames=[15.0, 15.0]
  - `bg_B1` -> `result_rank` casts=14 anims=1 frames=[20.0, 20.0]
  - `select` -> `result_rank` casts=4 anims=1 frames=[30.0, 30.0]
  - `bg_B2` -> `result_rank` casts=7 anims=2 frames=[15.0, 59.0]
  - `misson_title_B` -> `title_logo` casts=10 anims=1 frames=[69.0, 69.0]
  - `footer_B` -> `pause_shell` casts=2 anims=1 frames=[0.0, 0.0]

### `ui_balloon`
- Path: `game/Town_Common/ui_balloon.yncp`
- Scenes: `15`; casts: `175`; animations: `25`; textures: `12`
- Root scenes: ``
- Component roles: `menu_choices=1, town_dialog=14`
- Texture-backed preview commands: `50`
- Composed scene draw commands: `52`
- Animation keyframes: `291`
- Key scenes:
  - `balloon_nomal` -> `town_dialog` casts=7 anims=2 frames=[10.0, 10.0]
  - `balloon_shout` -> `town_dialog` casts=7 anims=2 frames=[7.0, 7.0]
  - `balloon_think` -> `town_dialog` casts=9 anims=2 frames=[20.0, 20.0]
  - `balloon_txt_area` -> `town_dialog` casts=4 anims=2 frames=[100.0, 100.0]
  - `balloon_sonic` -> `town_dialog` casts=19 anims=1 frames=[20.0, 20.0]
  - `balloon_sonic_select` -> `menu_choices` casts=10 anims=3 frames=[20.0, 60.0]
  - `balloon_nametag` -> `town_dialog` casts=15 anims=1 frames=[20.0, 20.0]
  - `balloon_nametag_sonic` -> `town_dialog` casts=29 anims=2 frames=[100.0, 110.0]
  - `bg` -> `town_dialog` casts=1 anims=1 frames=[15.0, 15.0]
  - `item_window` -> `town_dialog` casts=35 anims=1 frames=[15.0, 15.0]
  - `hint_window` -> `town_dialog` casts=15 anims=1 frames=[15.0, 15.0]
  - `scroll_bar` -> `town_dialog` casts=11 anims=2 frames=[100.0, 100.0]

### `ui_shop`
- Path: `game/Town_Common/ui_shop.yncp`
- Scenes: `17`; casts: `136`; animations: `24`; textures: `9`
- Root scenes: ``
- Component roles: `town_dialog=17`
- Texture-backed preview commands: `57`
- Composed scene draw commands: `33`
- Animation keyframes: `147`
- Key scenes:
  - `bg_1` -> `town_dialog` casts=17 anims=2 frames=[15.0, 15.0]
  - `bg_2` -> `town_dialog` casts=14 anims=1 frames=[15.0, 15.0]
  - `bg_3` -> `town_dialog` casts=13 anims=1 frames=[15.0, 15.0]
  - `bg_4` -> `town_dialog` casts=14 anims=1 frames=[15.0, 15.0]
  - `select_1` -> `town_dialog` casts=4 anims=3 frames=[60.0, 80.0]
  - `icon_2` -> `town_dialog` casts=9 anims=2 frames=[8.0, 8.0]
  - `icon_1` -> `town_dialog` casts=2 anims=1 frames=[45.0, 45.0]
  - `icon_3` -> `town_dialog` casts=2 anims=1 frames=[45.0, 45.0]
  - `bg_1_num` -> `town_dialog` casts=2 anims=1 frames=[100.0, 100.0]
  - `scrollbar_bg` -> `town_dialog` casts=6 anims=1 frames=[100.0, 100.0]
  - `scrollbar` -> `town_dialog` casts=4 anims=1 frames=[100.0, 100.0]
  - `bg_3_num` -> `town_dialog` casts=12 anims=1 frames=[100.0, 100.0]

### `ui_townscreen`
- Path: `game/Town_Common/ui_townscreen.yncp`
- Scenes: `5`; casts: `79`; animations: `10`; textures: `10`
- Root scenes: `time, time_effect, footer, info, cam`
- Component roles: `result_rank=1, status_skill=1, town_dialog=3`
- Texture-backed preview commands: `17`
- Composed scene draw commands: `31`
- Animation keyframes: `373`
- Key scenes:
  - `time` -> `status_skill` casts=13 anims=1 frames=[100.0, 100.0]
  - `time_effect` -> `town_dialog` casts=1 anims=1 frames=[600.0, 600.0]
  - `footer` -> `town_dialog` casts=43 anims=4 frames=[60.0, 60.0]
  - `info` -> `result_rank` casts=14 anims=2 frames=[59.0, 59.0]
  - `cam` -> `town_dialog` casts=8 anims=2 frames=[59.0, 59.0]

### `ui_gate`
- Path: `game/Town_EggManBase_Common/ui_gate.yncp`
- Scenes: `16`; casts: `163`; animations: `30`; textures: `13`
- Root scenes: ``
- Component roles: `result_rank=15, title_logo=1`
- Texture-backed preview commands: `51`
- Composed scene draw commands: `116`
- Animation keyframes: `181`
- Key scenes:
  - `area_tag` -> `result_rank` casts=6 anims=1 frames=[15.0, 15.0]
  - `bg` -> `result_rank` casts=76 anims=2 frames=[20.0, 20.0]
  - `stage_name` -> `result_rank` casts=2 anims=2 frames=[0.0, 20.0]
  - `info_1` -> `result_rank` casts=4 anims=2 frames=[0.0, 20.0]
  - `info_2` -> `result_rank` casts=8 anims=2 frames=[0.0, 20.0]
  - `info_3` -> `result_rank` casts=7 anims=2 frames=[20.0, 59.0]
  - `info_4` -> `result_rank` casts=7 anims=2 frames=[20.0, 59.0]
  - `info_5` -> `result_rank` casts=6 anims=2 frames=[20.0, 59.0]
  - `stage_ss` -> `result_rank` casts=4 anims=2 frames=[0.0, 20.0]
  - `info_6` -> `result_rank` casts=3 anims=2 frames=[0.0, 20.0]
  - `arrow` -> `result_rank` casts=2 anims=1 frames=[0.0, 0.0]
  - `window_bg` -> `result_rank` casts=1 anims=1 frames=[5.0, 5.0]

## Loading

### `ui_start`
- Path: `game/ActionCommon/ui_start.yncp`
- Scenes: `4`; casts: `39`; animations: `5`; textures: `1`
- Root scenes: `Start, Clear, Failed, Game_over`
- Component roles: `loading_device=4`
- Texture-backed preview commands: `10`
- Composed scene draw commands: `26`
- Animation keyframes: `120`
- Key scenes:
  - `Start` -> `loading_device` casts=27 anims=2 frames=[270.0, 270.0]
  - `Clear` -> `loading_device` casts=6 anims=1 frames=[155.0, 155.0]
  - `Failed` -> `loading_device` casts=3 anims=1 frames=[120.0, 120.0]
  - `Game_over` -> `loading_device` casts=3 anims=1 frames=[50.0, 50.0]

### `ui_loading`
- Path: `game/Loading/ui_loading.yncp`
- Scenes: `7`; casts: `331`; animations: `37`; textures: `10`
- Root scenes: `bg_1, loadinfo, n_2_d, event_viewer, pda, pda_txt, bg_2`
- Component roles: `hud_life=5, loading_device=2`
- Texture-backed preview commands: `28`
- Composed scene draw commands: `199`
- Animation keyframes: `773`
- Key scenes:
  - `bg_1` -> `hud_life` casts=28 anims=2 frames=[40.0, 45.0]
  - `loadinfo` -> `hud_life` casts=106 anims=13 frames=[2.0, 2.0]
  - `n_2_d` -> `hud_life` casts=10 anims=3 frames=[75.0, 128.0]
  - `event_viewer` -> `hud_life` casts=83 anims=1 frames=[80.0, 80.0]
  - `pda` -> `loading_device` casts=57 anims=8 frames=[40.0, 240.0]
  - `pda_txt` -> `loading_device` casts=19 anims=8 frames=[15.0, 240.0]
  - `bg_2` -> `hud_life` casts=28 anims=2 frames=[45.0, 51.0]

## Pause

### `ui_help`
- Path: `game/SystemCommon/ui_help.yncp`
- Scenes: `6`; casts: `30`; animations: `11`; textures: `7`
- Root scenes: ``
- Component roles: `town_dialog=6`
- Texture-backed preview commands: `21`
- Composed scene draw commands: `4`
- Animation keyframes: `52`
- Key scenes:
  - `help_window` -> `town_dialog` casts=13 anims=1 frames=[10.0, 10.0]
  - `help_text_area` -> `town_dialog` casts=4 anims=1 frames=[0.0, 0.0]
  - `help_nametag` -> `town_dialog` casts=7 anims=3 frames=[15.0, 15.0]
  - `help_chara_1` -> `town_dialog` casts=2 anims=2 frames=[0.0, 15.0]
  - `help_chara_2` -> `town_dialog` casts=2 anims=2 frames=[0.0, 15.0]
  - `help_chara_3` -> `town_dialog` casts=2 anims=2 frames=[0.0, 15.0]

### `ui_general`
- Path: `game/SystemCommonCore/ui_general.yncp`
- Scenes: `4`; casts: `26`; animations: `10`; textures: `3`
- Root scenes: `bg, window, window_select, footer`
- Component roles: `menu_choices=2, pause_shell=1, result_rank=1`
- Texture-backed preview commands: `11`
- Composed scene draw commands: `19`
- Animation keyframes: `53`
- Key scenes:
  - `bg` -> `result_rank` casts=1 anims=1 frames=[10.0, 10.0]
  - `window` -> `pause_shell` casts=15 anims=2 frames=[20.0, 100.0]
  - `window_select` -> `menu_choices` casts=4 anims=3 frames=[60.0, 200.0]
  - `footer` -> `menu_choices` casts=6 anims=4 frames=[0.0, 0.0]

### `ui_pause`
- Path: `game/SystemCommonCore/ui_pause.yncp`
- Scenes: `29`; casts: `260`; animations: `41`; textures: `9`
- Root scenes: `bg`
- Component roles: `pause_shell=23, status_skill=5, title_logo=1`
- Texture-backed preview commands: `82`
- Composed scene draw commands: `151`
- Animation keyframes: `559`
- Key scenes:
  - `bg` -> `pause_shell` casts=1 anims=1 frames=[15.0, 15.0]
  - `bg_1` -> `pause_shell` casts=14 anims=2 frames=[20.0, 27.0]
  - `bg_1_select` -> `pause_shell` casts=4 anims=3 frames=[27.0, 120.0]
  - `bg_2` -> `pause_shell` casts=68 anims=2 frames=[15.0, 20.0]
  - `text_area` -> `pause_shell` casts=3 anims=1 frames=[200.0, 200.0]
  - `skill_select` -> `pause_shell` casts=27 anims=1 frames=[60.0, 60.0]
  - `arrow` -> `pause_shell` casts=2 anims=1 frames=[200.0, 200.0]
  - `skill_scroll_bar_bg` -> `pause_shell` casts=6 anims=1 frames=[20.0, 20.0]
  - `skill_scroll_bar` -> `pause_shell` casts=4 anims=1 frames=[100.0, 100.0]
  - `bg_3` -> `pause_shell` casts=16 anims=1 frames=[20.0, 20.0]
  - `bg_4` -> `pause_shell` casts=14 anims=1 frames=[20.0, 20.0]
  - `scroll_bar_bg` -> `pause_shell` casts=6 anims=1 frames=[20.0, 20.0]

## Result

### `ui_result`
- Path: `game/ActionCommon/ui_result.yncp`
- Scenes: `17`; casts: `290`; animations: `29`; textures: `7`
- Root scenes: ``
- Component roles: `menu_choices=1, result_rank=15, title_logo=1`
- Texture-backed preview commands: `56`
- Composed scene draw commands: `176`
- Animation keyframes: `1516`
- Key scenes:
  - `result_title` -> `title_logo` casts=10 anims=2 frames=[85.0, 85.0]
  - `result_num_1` -> `result_rank` casts=22 anims=2 frames=[40.0, 40.0]
  - `result_num_2` -> `result_rank` casts=22 anims=2 frames=[49.0, 49.0]
  - `result_num_3` -> `result_rank` casts=22 anims=2 frames=[60.0, 60.0]
  - `result_num_4` -> `result_rank` casts=22 anims=2 frames=[69.0, 69.0]
  - `result_num_5` -> `result_rank` casts=22 anims=2 frames=[76.0, 76.0]
  - `result_num_6` -> `result_rank` casts=22 anims=2 frames=[93.0, 93.0]
  - `result_newR` -> `menu_choices` casts=51 anims=2 frames=[57.0, 57.0]
  - `result_newR_position` -> `result_rank` casts=6 anims=1 frames=[0.0, 0.0]
  - `result_rank` -> `result_rank` casts=1 anims=2 frames=[10.0, 100.0]
  - `result_rank_E` -> `result_rank` casts=32 anims=1 frames=[253.0, 253.0]
  - `result_rank_D` -> `result_rank` casts=2 anims=1 frames=[45.0, 45.0]

### `ui_result_ex`
- Path: `game/ExStageTails_Common/ui_result_ex.yncp`
- Scenes: `15`; casts: `204`; animations: `20`; textures: `7`
- Root scenes: ``
- Component roles: `menu_choices=1, result_rank=13, title_logo=1`
- Texture-backed preview commands: `44`
- Composed scene draw commands: `123`
- Animation keyframes: `907`
- Key scenes:
  - `result_title` -> `title_logo` casts=10 anims=1 frames=[85.0, 85.0]
  - `result_num_1` -> `result_rank` casts=35 anims=1 frames=[92.0, 92.0]
  - `result_num` -> `result_rank` casts=10 anims=1 frames=[93.0, 93.0]
  - `result_tag_1` -> `result_rank` casts=35 anims=1 frames=[70.0, 70.0]
  - `result_tag` -> `result_rank` casts=10 anims=1 frames=[70.0, 70.0]
  - `result_newR` -> `menu_choices` casts=7 anims=2 frames=[0.0, 57.0]
  - `result_newR_position` -> `result_rank` casts=6 anims=1 frames=[0.0, 0.0]
  - `result_rank` -> `result_rank` casts=1 anims=2 frames=[100.0, 200.0]
  - `result_rank_E` -> `result_rank` casts=32 anims=1 frames=[253.0, 253.0]
  - `result_rank_D` -> `result_rank` casts=2 anims=1 frames=[45.0, 45.0]
  - `result_rank_C` -> `result_rank` casts=2 anims=1 frames=[20.0, 20.0]
  - `result_rank_B` -> `result_rank` casts=11 anims=2 frames=[75.0, 120.0]

### `ui_itemresult`
- Path: `game/SystemCommon/ui_itemresult.yncp`
- Scenes: `4`; casts: `73`; animations: `16`; textures: `8`
- Root scenes: ``
- Component roles: `result_rank=3, title_logo=1`
- Texture-backed preview commands: `15`
- Composed scene draw commands: `49`
- Animation keyframes: `181`
- Key scenes:
  - `iresult_title` -> `title_logo` casts=10 anims=6 frames=[26.0, 85.0]
  - `window` -> `result_rank` casts=36 anims=6 frames=[5.0, 5.0]
  - `contents` -> `result_rank` casts=25 anims=3 frames=[60.0, 60.0]
  - `result_footer` -> `result_rank` casts=2 anims=1 frames=[100.0, 100.0]

## Sonic Hud

### `ui_playscreen_su`
- Path: `game/BossDarkGaia1_1Air/ui_playscreen_su.yncp`
- Scenes: `3`; casts: `34`; animations: `6`; textures: `5`
- Root scenes: `su_sonic_gauge, gaia_gauge, footer`
- Component roles: `hud_speed=2, pause_shell=1`
- Texture-backed preview commands: `12`
- Composed scene draw commands: `31`
- Animation keyframes: `33`
- Key scenes:
  - `su_sonic_gauge` -> `hud_speed` casts=14 anims=2 frames=[30.0, 100.0]
  - `gaia_gauge` -> `hud_speed` casts=14 anims=3 frames=[30.0, 100.0]
  - `footer` -> `pause_shell` casts=6 anims=1 frames=[100.0, 100.0]

### `ui_playscreen_ev_hit`
- Path: `game/BossFinalDarkGaia/ui_playscreen_ev_hit.yncp`
- Scenes: `5`; casts: `63`; animations: `19`; textures: `2`
- Root scenes: `hit_counter_bg, hit_counter_num, hit_counter_txt_1, hit_counter_txt_2, chance_attack`
- Component roles: `hud_score_time=4, scene_chance-attack=1`
- Texture-backed preview commands: `19`
- Composed scene draw commands: `48`
- Animation keyframes: `279`
- Key scenes:
  - `hit_counter_bg` -> `hud_score_time` casts=7 anims=4 frames=[10.0, 12.0]
  - `hit_counter_num` -> `hud_score_time` casts=7 anims=2 frames=[10.0, 20.0]
  - `hit_counter_txt_1` -> `hud_score_time` casts=9 anims=5 frames=[12.0, 12.0]
  - `hit_counter_txt_2` -> `hud_score_time` casts=16 anims=2 frames=[30.0, 30.0]
  - `chance_attack` -> `scene_chance-attack` casts=24 anims=6 frames=[30.0, 30.0]

### `ui_playscreen_su`
- Path: `game/BossFinalDarkGaia/ui_playscreen_su.yncp`
- Scenes: `3`; casts: `34`; animations: `6`; textures: `5`
- Root scenes: `su_sonic_gauge, gaia_gauge, footer`
- Component roles: `hud_speed=2, pause_shell=1`
- Texture-backed preview commands: `12`
- Composed scene draw commands: `31`
- Animation keyframes: `33`
- Key scenes:
  - `su_sonic_gauge` -> `hud_speed` casts=14 anims=2 frames=[30.0, 100.0]
  - `gaia_gauge` -> `hud_speed` casts=14 anims=3 frames=[30.0, 100.0]
  - `footer` -> `pause_shell` casts=6 anims=1 frames=[100.0, 100.0]

### `ui_playscreen_ev_hit`
- Path: `game/EvilActionCommon/ui_playscreen_ev_hit.yncp`
- Scenes: `5`; casts: `64`; animations: `19`; textures: `3`
- Root scenes: `hit_counter_bg, hit_counter_num, hit_counter_txt_1, hit_counter_txt_2, chance_attack`
- Component roles: `hud_score_time=4, scene_chance-attack=1`
- Texture-backed preview commands: `19`
- Composed scene draw commands: `49`
- Animation keyframes: `281`
- Key scenes:
  - `hit_counter_bg` -> `hud_score_time` casts=7 anims=4 frames=[10.0, 12.0]
  - `hit_counter_num` -> `hud_score_time` casts=7 anims=2 frames=[10.0, 20.0]
  - `hit_counter_txt_1` -> `hud_score_time` casts=9 anims=5 frames=[12.0, 12.0]
  - `hit_counter_txt_2` -> `hud_score_time` casts=17 anims=2 frames=[30.0, 30.0]
  - `chance_attack` -> `scene_chance-attack` casts=24 anims=6 frames=[30.0, 30.0]

### `ui_playscreen_ev`
- Path: `game/EvilSonic/ui_playscreen_ev.yncp`
- Scenes: `33`; casts: `280`; animations: `60`; textures: `11`
- Root scenes: `player_count, score_count, ring_count, ring_get, exp_count`
- Component roles: `hud_life=2, hud_ring=2, hud_score_time=1, hud_speed=24, result_rank=3, status_skill=1`
- Texture-backed preview commands: `103`
- Composed scene draw commands: `159`
- Animation keyframes: `923`
- Key scenes:
  - `player_count` -> `hud_life` casts=3 anims=1 frames=[100.0, 100.0]
  - `score_count` -> `hud_score_time` casts=12 anims=1 frames=[100.0, 100.0]
  - `ring_count` -> `hud_ring` casts=12 anims=1 frames=[100.0, 100.0]
  - `ring_get` -> `hud_ring` casts=4 anims=1 frames=[29.0, 29.0]
  - `exp_count` -> `status_skill` casts=22 anims=2 frames=[20.0, 100.0]
  - `u_info` -> `result_rank` casts=16 anims=3 frames=[20.0, 30.0]
  - `medal_get_s` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]
  - `medal_get_m` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]
  - `unleash_bg` -> `hud_speed` casts=5 anims=1 frames=[100.0, 100.0]
  - `life_bg` -> `hud_life` casts=5 anims=1 frames=[100.0, 100.0]
  - `unleash_body` -> `hud_speed` casts=6 anims=2 frames=[100.0, 100.0]
  - `unleash_bar_1` -> `hud_speed` casts=4 anims=2 frames=[100.0, 100.0]

### `ui_playscreen`
- Path: `game/Sonic/ui_playscreen.yncp`
- Scenes: `13`; casts: `209`; animations: `18`; textures: `12`
- Root scenes: `so_speed_gauge, so_ringenagy_gauge, gauge_frame, player_count, time_count, score_count, ring_count, ring_get, exp_count`
- Component roles: `hud_life=1, hud_ring=3, hud_score_time=2, hud_speed=3, result_rank=3, status_skill=1`
- Texture-backed preview commands: `45`
- Composed scene draw commands: `145`
- Animation keyframes: `239`
- Key scenes:
  - `so_speed_gauge` -> `hud_speed` casts=47 anims=1 frames=[100.0, 100.0]
  - `so_ringenagy_gauge` -> `hud_ring` casts=43 anims=2 frames=[100.0, 100.0]
  - `gauge_frame` -> `hud_speed` casts=20 anims=1 frames=[100.0, 100.0]
  - `player_count` -> `hud_life` casts=3 anims=1 frames=[100.0, 100.0]
  - `time_count` -> `hud_score_time` casts=16 anims=1 frames=[100.0, 100.0]
  - `score_count` -> `hud_score_time` casts=12 anims=1 frames=[100.0, 100.0]
  - `ring_count` -> `hud_ring` casts=1 anims=1 frames=[100.0, 100.0]
  - `ring_get` -> `hud_ring` casts=3 anims=2 frames=[10.0, 60.0]
  - `exp_count` -> `status_skill` casts=22 anims=2 frames=[20.0, 100.0]
  - `u_info` -> `result_rank` casts=16 anims=3 frames=[20.0, 30.0]
  - `medal_get_s` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]
  - `medal_get_m` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]

### `ui_playscreen`
- Path: `game/SuperSonic/ui_playscreen.yncp`
- Scenes: `13`; casts: `208`; animations: `16`; textures: `12`
- Root scenes: `so_speed_gauge, so_ringenagy_gauge, gauge_frame, player_count, time_count, score_count, ring_count, ring_get, exp_count`
- Component roles: `hud_life=1, hud_ring=3, hud_score_time=2, hud_speed=3, result_rank=3, status_skill=1`
- Texture-backed preview commands: `45`
- Composed scene draw commands: `145`
- Animation keyframes: `167`
- Key scenes:
  - `so_speed_gauge` -> `hud_speed` casts=47 anims=1 frames=[100.0, 100.0]
  - `so_ringenagy_gauge` -> `hud_ring` casts=43 anims=2 frames=[100.0, 100.0]
  - `gauge_frame` -> `hud_speed` casts=20 anims=1 frames=[100.0, 100.0]
  - `player_count` -> `hud_life` casts=3 anims=1 frames=[100.0, 100.0]
  - `time_count` -> `hud_score_time` casts=16 anims=1 frames=[100.0, 100.0]
  - `score_count` -> `hud_score_time` casts=12 anims=1 frames=[100.0, 100.0]
  - `ring_count` -> `hud_ring` casts=1 anims=1 frames=[100.0, 100.0]
  - `ring_get` -> `hud_ring` casts=3 anims=2 frames=[5.0, 60.0]
  - `exp_count` -> `status_skill` casts=22 anims=2 frames=[20.0, 100.0]
  - `u_info` -> `result_rank` casts=16 anims=1 frames=[20.0, 20.0]
  - `medal_get_s` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]
  - `medal_get_m` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]

## Status

### `ui_status`
- Path: `game/SystemCommonCore/ui_status.yncp`
- Scenes: `42`; casts: `227`; animations: `109`; textures: `15`
- Root scenes: ``
- Component roles: `result_rank=2, status_skill=39, title_logo=1`
- Texture-backed preview commands: `74`
- Composed scene draw commands: `123`
- Animation keyframes: `1030`
- Key scenes:
  - `logo` -> `status_skill` casts=11 anims=3 frames=[20.0, 20.0]
  - `a_efc_1` -> `status_skill` casts=2 anims=2 frames=[60.0, 60.0]
  - `a_efc_2` -> `status_skill` casts=1 anims=2 frames=[0.0, 0.0]
  - `a_efc_3` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]
  - `a_efc_4` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]
  - `a_efc_5` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]
  - `a_efc_6` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]
  - `prgs_bg_1` -> `status_skill` casts=7 anims=4 frames=[15.0, 50.0]
  - `prgs_bg_2` -> `status_skill` casts=10 anims=6 frames=[10.0, 60.0]
  - `prgs_bg_3` -> `status_skill` casts=1 anims=2 frames=[0.0, 0.0]
  - `prgs_bg_4` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]
  - `prgs_bg_5` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]

## Title

### `ui_mainmenu`
- Path: `game/MainMenu/ui_mainmenu.xncp`
- Scenes: `16`; casts: `399`; animations: `20`; textures: `3`
- Root scenes: `mm_bg_usual, mm_bg_intro, mm_donut_idle, mm_donut_select, mm_donut_move, mm_donut_intro, mm_contentsitem_select, mm_contentsitem_idle, mm_contentsitem_move, mm_contentsitem_intro, mm_contentsitem_text, mm_base, mm_title_usual, mm_title_intro, mm_front_usual, mm_front_intro`
- Component roles: `menu_choices=2, result_rank=2, scene_mm-base=1, scene_mm-contentsitem-idle=1, scene_mm-contentsitem-intro=1, scene_mm-contentsitem-move=1, scene_mm-contentsitem-text=1, scene_mm-donut-idle=1, scene_mm-donut-intro=1, scene_mm-donut-move=1, scene_mm-front-intro=1, scene_mm-front-usual=1, title_logo=2`
- Texture-backed preview commands: `61`
- Composed scene draw commands: `287`
- Animation keyframes: `677`
- Key scenes:
  - `mm_bg_usual` -> `result_rank` casts=47 anims=1 frames=[120.0, 120.0]
  - `mm_bg_intro` -> `result_rank` casts=99 anims=1 frames=[60.0, 60.0]
  - `mm_donut_idle` -> `scene_mm-donut-idle` casts=9 anims=1 frames=[120.0, 120.0]
  - `mm_donut_select` -> `menu_choices` casts=9 anims=1 frames=[15.0, 15.0]
  - `mm_donut_move` -> `scene_mm-donut-move` casts=9 anims=1 frames=[220.0, 220.0]
  - `mm_donut_intro` -> `scene_mm-donut-intro` casts=9 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_select` -> `menu_choices` casts=19 anims=1 frames=[15.0, 15.0]
  - `mm_contentsitem_idle` -> `scene_mm-contentsitem-idle` casts=13 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_move` -> `scene_mm-contentsitem-move` casts=46 anims=1 frames=[40.0, 40.0]
  - `mm_contentsitem_intro` -> `scene_mm-contentsitem-intro` casts=28 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_text` -> `scene_mm-contentsitem-text` casts=35 anims=5 frames=[15.0, 60.0]
  - `mm_base` -> `scene_mm-base` casts=3 anims=1 frames=[60.0, 60.0]

### `ui_mainmenu`
- Path: `game/MainMenu/ui_mainmenu.yncp`
- Scenes: `16`; casts: `399`; animations: `20`; textures: `3`
- Root scenes: `mm_bg_usual, mm_bg_intro, mm_donut_idle, mm_donut_select, mm_donut_move, mm_donut_intro, mm_contentsitem_select, mm_contentsitem_idle, mm_contentsitem_move, mm_contentsitem_intro, mm_contentsitem_text, mm_base, mm_title_usual, mm_title_intro, mm_front_usual, mm_front_intro`
- Component roles: `menu_choices=2, result_rank=2, scene_mm-base=1, scene_mm-contentsitem-idle=1, scene_mm-contentsitem-intro=1, scene_mm-contentsitem-move=1, scene_mm-contentsitem-text=1, scene_mm-donut-idle=1, scene_mm-donut-intro=1, scene_mm-donut-move=1, scene_mm-front-intro=1, scene_mm-front-usual=1, title_logo=2`
- Texture-backed preview commands: `61`
- Composed scene draw commands: `287`
- Animation keyframes: `677`
- Key scenes:
  - `mm_bg_usual` -> `result_rank` casts=47 anims=1 frames=[120.0, 120.0]
  - `mm_bg_intro` -> `result_rank` casts=99 anims=1 frames=[60.0, 60.0]
  - `mm_donut_idle` -> `scene_mm-donut-idle` casts=9 anims=1 frames=[120.0, 120.0]
  - `mm_donut_select` -> `menu_choices` casts=9 anims=1 frames=[15.0, 15.0]
  - `mm_donut_move` -> `scene_mm-donut-move` casts=9 anims=1 frames=[220.0, 220.0]
  - `mm_donut_intro` -> `scene_mm-donut-intro` casts=9 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_select` -> `menu_choices` casts=19 anims=1 frames=[15.0, 15.0]
  - `mm_contentsitem_idle` -> `scene_mm-contentsitem-idle` casts=13 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_move` -> `scene_mm-contentsitem-move` casts=46 anims=1 frames=[40.0, 40.0]
  - `mm_contentsitem_intro` -> `scene_mm-contentsitem-intro` casts=28 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_text` -> `scene_mm-contentsitem-text` casts=35 anims=5 frames=[15.0, 60.0]
  - `mm_base` -> `scene_mm-base` casts=3 anims=1 frames=[60.0, 60.0]

### `ui_title`
- Path: `game/Title/ui_title.yncp`
- Scenes: `8`; casts: `502`; animations: `49`; textures: `7`
- Root scenes: `bg, logo, title_1, title_2, menu, menu_scroll, txt, progress`
- Component roles: `title_logo=8`
- Texture-backed preview commands: `28`
- Composed scene draw commands: `265`
- Animation keyframes: `1447`
- Key scenes:
  - `bg` -> `title_logo` casts=14 anims=5 frames=[60.0, 120.0]
  - `logo` -> `title_logo` casts=2 anims=2 frames=[180.0, 180.0]
  - `title_1` -> `title_logo` casts=14 anims=3 frames=[5.0, 300.0]
  - `title_2` -> `title_logo` casts=14 anims=3 frames=[5.0, 105.0]
  - `menu` -> `title_logo` casts=203 anims=21 frames=[15.0, 180.0]
  - `menu_scroll` -> `title_logo` casts=188 anims=8 frames=[15.0, 15.0]
  - `txt` -> `title_logo` casts=64 anims=4 frames=[55.0, 55.0]
  - `progress` -> `title_logo` casts=3 anims=3 frames=[55.0, 100.0]

### `ui_title`
- Path: `game/TitleE3/ui_title.yncp`
- Scenes: `9`; casts: `418`; animations: `41`; textures: `5`
- Root scenes: `sample, bg, logo, title_1, title_2, menu, menu_scroll, txt, progress`
- Component roles: `title_logo=9`
- Texture-backed preview commands: `29`
- Composed scene draw commands: `251`
- Animation keyframes: `790`
- Key scenes:
  - `sample` -> `title_logo` casts=1 anims=1 frames=[100.0, 100.0]
  - `bg` -> `title_logo` casts=14 anims=5 frames=[60.0, 120.0]
  - `logo` -> `title_logo` casts=2 anims=2 frames=[180.0, 180.0]
  - `title_1` -> `title_logo` casts=16 anims=3 frames=[5.0, 300.0]
  - `title_2` -> `title_logo` casts=20 anims=3 frames=[5.0, 105.0]
  - `menu` -> `title_logo` casts=158 anims=16 frames=[15.0, 180.0]
  - `menu_scroll` -> `title_logo` casts=159 anims=6 frames=[15.0, 15.0]
  - `txt` -> `title_logo` casts=45 anims=3 frames=[50.0, 55.0]
  - `progress` -> `title_logo` casts=3 anims=2 frames=[55.0, 100.0]

## World Map

### `ui_worldmap`
- Path: `dlc/Apotos & Shamar Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Apotos & Shamar Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `dlc/Chun-nan Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Chun-nan Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `dlc/Empire City & Adabat Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Empire City & Adabat Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `dlc/Holoska Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Holoska Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `dlc/Mazuri Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Mazuri Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `dlc/Spagonia Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Spagonia Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `game/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `game/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

## Runtime-Hook + Ghidra Xref SFX Candidates

- `ui_title/menu` `se_system_worldmap/sys_worldmap_window` via `CTitleStateMenu::Update/Game_PlaySound`; xref `sub_825882B8 -> Game_PlaySound("sys_worldmap_window")`
- `ui_title/menu` `se_system_worldmap/sys_worldmap_decide` via `CTitleStateMenu::Update/Game_PlaySound`; xref `sub_825882B8 -> Game_PlaySound("sys_worldmap_decide")`
- `ui_title/menu` `se_system_worldmap/sys_worldmap_cansel` via `CTitleStateMenu::Update/Game_PlaySound`; xref `sub_825882B8 -> Game_PlaySound("sys_worldmap_cansel")`
- `ui_mainmenu/mm_contentsitem_select` `se_system_worldmap/sys_worldmap_decide` via `CTitleStateMenu::Update/Game_PlaySound`; xref `sub_825882B8 -> Game_PlaySound("sys_worldmap_decide")`

