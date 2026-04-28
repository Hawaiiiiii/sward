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
