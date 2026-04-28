<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# Sonic HUD Parity Archaeology

Phase 133 adds a Sonic-HUD-specific archaeology pass to the CSD render compare lane.

The key finding is deliberately conservative:

- the real runtime normal Sonic HUD is `ui_playscreen`;
- the live bridge reports `13` runtime scenes, `2` nodes, and `209` layers for that project;
- the local CSD sidecar still has only the recovered `ui_prov_playscreen.yncp` proxy layout for the current drawable HUD path;
- therefore the sidecar can compare the shared `so_speed_gauge` scene shape, but it cannot yet claim exact normal Sonic HUD local rendering.

## Runtime Evidence

The latest Phase 133 smoke read the current local live state:

```text
out/ui_lab_runtime_evidence/20260428_115131/sonic-hud/ui_lab_live_state.json
```

That live state proves:

- `targetCsd = ui_playscreen`
- `typedInspectors.csdProjectTree.activeProject = ui_playscreen`
- `sceneCount = 13`
- `nodeCount = 2`
- `layerCount = 209`
- `stageReadyFrame = 721`
- `typedInspectors.sonicHud.ownerPath.ownerPointerStatus = raw CHudSonicStage owner hook live; CSD owner fields pending`
- `typedInspectors.sonicHud.ownerPath.resolvedFromCsdProjectTree = true`

This matches the Phase 121 owner-maturation result: the raw `CHudSonicStage` owner pointer is live, but the fork-header embedded owner slots are not the source of the resolved CSD project/scene addresses. The useful resolved addresses currently come from `CCsdProject::Make` traversal.

## Local Renderer Gap

`--csd-render-compare-smoke` now emits:

```text
sonic_hud_runtime_scene=sonic-hud:runtime_project=ui_playscreen:local_layout=ui_prov_playscreen.yncp:local_project=ui_prov_playscreen:local_scene=so_speed_gauge:layout_status=exact-ui-playscreen-layout-unrecovered;local-proxy-layout-ui_prov_playscreen
```

The scene coverage diagnostic currently reports:

| Runtime scene | Runtime casts | Local commands | Local status |
|---|---:|---:|---|
| `so_speed_gauge` | `47` | `43` | shared-name proxy scene rendered |
| `so_ringenagy_gauge` | `43` | `0` | missing from local render |
| `gauge_frame` | `20` | `0` | missing from local render |
| `exp_count` | `22` | `0` | missing from local render |
| `time_count` | `16` | `0` | missing from local render |
| `score_count` | `12` | `0` | missing from local render |
| `add/u_info` | `16` | `0` | missing from local render |

The remaining runtime scenes are likewise listed in the Phase 133 manifest under `sonicHudSceneCoverage`.

## What This Means

The current Sonic HUD sidecar delta is not just a shader/material problem. It is also a coverage problem:

- exact loose `ui_playscreen.yncp` has not been recovered into `layout_deep_analysis.json`;
- the local sidecar is drawing a proxy Extra/Tornado-family package;
- only one shared scene name is locally drawable today;
- most real normal Sonic HUD runtime scenes have no local draw commands yet.

Native BMP capture remains the visual proof path. The Phase 133 diagnostics explain why the sidecar cannot yet be used as a 1:1 Sonic HUD renderer.

## Next Best Work

1. Recover or export exact local `ui_playscreen.yncp` CSD/layout payloads from the local game files or runtime traversal.
2. Extend the live bridge or patch layer toward a runtime UI-only capture/export path, because full backbuffer screenshots include the stage world.
3. Once exact `ui_playscreen` data is locally drawable, render all `13` runtime scenes together and compare against native BMP evidence.
4. Convert the proven state/motion/scene ownership into readable, portable HUD reference code only after the exact scene coverage is present.

## Phase 134 Runtime Tree Export

Phase 134 did not find a loose local `ui_playscreen.yncp` payload in the current extracted layout evidence. Instead, it promoted the runtime path:

```powershell
b\rr134\Release\sward_su_ui_asset_renderer.exe --export-runtime-csd-tree --template sonic-hud
```

That command reads the latest live-bridge `sonic-hud` state and writes ignored local evidence:

```text
out/csd_runtime_exports/phase134/ui_playscreen_runtime_tree.json
```

The refreshed runtime proof is:

- live evidence: `out/ui_lab_runtime_evidence/20260428_132410/sonic-hud/ui_lab_live_state.json`
- runtime project: `ui_playscreen`
- runtime scenes: `13`
- runtime nodes: `2`
- runtime layers: `209`
- exported layer samples: `203`
- sample cap widened in the UI Lab patch layer from `48` to `512`
- later normal Sonic HUD scenes now survive export, including `ui_playscreen/so_speed_gauge`

The export names SGFX-replaceable slots beside each runtime scene:

| Runtime scene | Casts | Exported layers | SGFX slot |
|---|---:|---:|---|
| `exp_count` | `22` | `22` | `experience_counter` |
| `gauge_frame` | `20` | `20` | `side_panel` |
| `player_count` | `3` | `3` | `life_icon` |
| `ring_count` | `1` | `1` | `ring_counter` |
| `ring_get` | `3` | `3` | `ring_counter` |
| `score_count` | `12` | `12` | `score_counter` |
| `so_ringenagy_gauge` | `43` | `43` | `energy_gauge` |
| `so_speed_gauge` | `47` | `47` | `speed_gauge` |
| `time_count` | `16` | `16` | `timer` |
| `add/medal_get_m` | `5` | `5` | `medal_counter` |
| `add/medal_get_s` | `5` | `5` | `medal_counter` |
| `add/speed_count` | `16` | `16` | `speed_readout` |
| `add/u_info` | `16` | `10` | `prompt_strip` |

This is still not full drawable HUD parity. The export has scene/node/layer paths, cast indices, runtime addresses, frame anchors, and slot labels; it does not yet carry material rectangles, subimage payloads, or exact timeline channels for every layer. The next hard beat is to use this runtime tree to pull enough material/cast data for a full `ui_playscreen` compositor, or to recover the loose layout payload locally.
