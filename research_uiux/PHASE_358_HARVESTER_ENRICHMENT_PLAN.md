# Phase 358 -- Harvester enrichment plan

Phase 357 landed the SGFX-side replay infrastructure
([`sgfx_csd_animation_replay.hpp`](runtime_reference/include/sward/ui_runtime/sgfx_csd_animation_replay.hpp) +
`--csd-replay=<path>` flag in the mirror). The consumer side is fully
wired and smoke-tested. What's missing is **producer-side enrichment**
in UnleashedRecomp's harvester so the JSONL events carry enough
information to drive per-cast retail-faithful animation in SGFX.

## What the harvester emits today

`local_build_env/ur103clean/UnleashedRecomp/patches/ui_lab_patches.cpp::EmitCsdSetterEvent`
writes lines like:

```json
{"frame":123,"time":2.05,"kind":"position","node":"0xABCDEF12","x":100.0,"y":50.0,
 "hits":5,"hook":"CSD::CCastNode::SetPosition/sub_830BB3D0","target":"title-menu"}
```

Hooks that already write events:

| Hook PPC sub | Kind label | Source |
|---|---|---|
| `sub_830BB3D0` | `position` | `CCastNode::SetPosition` per cast |
| `sub_830BB650` | `scale` | `CCastNode::SetScale` per cast |
| `sub_830BB5F8` | `uniformScaleOrAlpha` | per cast (single float at +0x52) |
| `sub_830BF300` | `pattern_index` | `CNode::SetPatternIndex` |
| `sub_830BF080` | `hide_flag` | `CNode::SetHideFlag` |
| `sub_830BF090` | `node_scale` | `CNode::SetScale` |
| `OnBackendMaterialSubmit` | -- | per draw call (`alphaBlendEnable`, descriptor indices) |

## What the SGFX replay needs

`sgfx_csd_animation_replay.hpp::buildRuntimeOverridesAt` returns a
`std::vector<CsdNativeRuntimeOverride>` that the renderer's
compositor consumes via `compositeCommand`'s `runtimeOverrides`
parameter. The compositor matches overrides to draw commands by
`cmd.sceneName == override.sceneName`. **The current harvester does
not record scene names**, so replay falls back to synthetic
`"node_<HEX>"` scene names that no cast in any `.yncp` matches --
making the replay path a no-op until enrichment lands.

## The enrichment

Three additions to UnleashedRecomp's harvester:

### 1. Resolve node -> (project, scene, cast) at hook time

At the `OnCsdNodeSetPosition` / `OnCsdCastNodeSetScale` callsites in
[`ui_lab_patches.cpp`](../local_build_env/ur103clean/UnleashedRecomp/patches/ui_lab_patches.cpp),
walk up from the cast node to its owning scene + project via the
existing `Chao::CSD::CCastNode -> CScene -> CProject` parent chain
(structures already typed in
[`api/CSD/Manager/`](../local_build_env/ur103clean/UnleashedRecomp/api/CSD/Manager/)).
Cache the resolution in a `std::unordered_map<uint32_t, NodeMeta>`
keyed by node address; populate on the first event per node.

`NodeMeta` carries:
- `std::string projectName` (e.g. `"game/Title/ui_title.yncp"`)
- `std::string sceneName`   (e.g. `"logo"`, `"mm_contentsitem_idle"`)
- `std::string castName`    (e.g. `"index_bar1"`)

Add these as JSONL fields:

```json
{...,"project":"game/Title/ui_title.yncp","scene":"logo","cast":"sega"}
```

### 2. Hook `CScene::SetMotion` (sub_830BA760)

Add a new `CsdSceneSetMotion_patches.cpp` mirroring
[`CsdNodeValue_patches.cpp`](../local_build_env/ur103clean/UnleashedRecomp/patches/CsdNodeValue_patches.cpp):

```cpp
PPC_FUNC_IMPL(__imp__sub_830BA760);
PPC_FUNC(sub_830BA760)
{
    const uint32_t sceneAddress = ctx.r3.u32;
    const uint32_t namePtr      = ctx.r4.u32;
    __imp__sub_830BA760(ctx, base);
    UiLab::OnCsdSceneSetMotion(sceneAddress, namePtr,
        "CSD::CScene::SetMotion/sub_830BA760");
}
```

The `namePtr` points to a guest-string buffer; the dispatch reads
it via `g_memory.Translate<char>(namePtr)`.  Emit:

```json
{...,"event":"scene-motion","scene":"logo","motion":"Intro_Anim"}
```

The SGFX replay uses this to know which animation segment is
active per scene at any point in time, so it can replay the
correct curve span when multiple segments exist for the same
scene (`Intro_Anim` -> `usual_Anim` -> `Outro_Anim`).

### 3. Hook `CScene::Update` per-frame motion-frame snapshot

`CScene::Update` is the virtual function that advances
`m_PrevMotionFrame` / `m_MotionFrame` (offsets +0x60/+0x64 per
[`csdmScene.h`](../local_build_env/ur103clean/UnleashedRecomp/api/CSD/Manager/csdmScene.h#L27-L28)).
A `PPC_FUNC` patch on the vtable thunk reads `m_MotionFrame`
post-Update and emits one event per scene per ~10 frames (the
existing `kStateMachineLogIntervalFrames` throttle). Format:

```json
{...,"event":"scene-motion-frame","scene":"logo","motion_frame":12.5,
 "motion_speed":1.0,"motion_repeat":1}
```

This is the dense playhead stream the SGFX replay walks to look up
`(scene, time) -> motion_frame`, which then indexes into the
`.yncp` motion-pattern keyframe table to get exact cast curves.
(`.yncp` motion patterns are not yet parsed by SGFX -- that's the
parallel Phase 358 work in
[`sgfx_hud_csd_project_loader.hpp`](runtime_reference/include/sward/ui_runtime/sgfx_hud_csd_project_loader.hpp).)

## Renderer-side change after enrichment lands

Once events carry `project` + `scene` + `cast`, replace the
synthetic `"node_<HEX>"` scene names in
`buildRuntimeOverridesAt` with the real `scene` field. The
existing renderer's compositor already filters overrides by scene
name, so per-cast curves will start applying without further
renderer changes.

For per-cast (not just per-scene) overrides, the renderer needs a
parallel `castName` match path; that's a small addition to
`compositeCommand`.

## Test plan

1. Patch UnleashedRecomp with the three additions.
2. Boot UnleashedRecomp, navigate Title -> WorldMap -> Loading ->
   StageHud Day -> Pause -> Results -> Hub. The harness writes
   ~5-50 MB of JSONL.
3. Run
   ```
   bin_mirror\sgfx_ui_mirror.exe \
     --csd-replay=<evidence_dir>\ui_lab_csd_setposition.jsonl \
     --screenshots-dir=research_uiux\runtime_reference\out\sgfx_replay_validation
   ```
4. Compare `sgfx_replay_validation/*.png` against the
   non-replay `sgfx_screen_validation/*.png`. Replay-driven
   composites should match retail's animated frame at the
   captured timestamp instead of the static frame-0 anchor.

## Honest delta vs SGFX's "frame 0" problem today

| Today (Phase 357) | Post-enrichment (Phase 358) |
|---|---|
| All scenes paint at their static `.yncp` anchor (effectively frame 0). | Each cast paints at the position retail computed at the captured frame. |
| Werehog gauge fills, Sonic title logo flourish, results-screen number tally -- all static. | All animate per the captured curve. |
| `--csd-replay=` plumbing in place, no-op without scene-name field. | `--csd-replay=` becomes the primary override source for in-scope screens. |

This unblocks the 25% -> 95% animation jump per the project plan.
