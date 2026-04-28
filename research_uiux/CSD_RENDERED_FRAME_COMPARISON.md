<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# CSD Rendered Frame Comparison

Phase 127 adds the first hard pixel-output loop to the local SU UI pipeline viewer.

> [!NOTE]
> Phase 128 keeps this command but upgrades the active output lane to `out/csd_render_compare/phase128/` with RGBA color order, cast-flag material semantics, packed channel blockers, and 16:9 native-crop alignment. See `CSD_MATERIAL_CHANNEL_ALIGNMENT.md`.

The new command is:

```powershell
b\rr127\Release\sward_su_ui_asset_renderer.exe --csd-render-compare-smoke
```

It renders CSD-derived frames for:

- `title-menu` from `ui_mainmenu.yncp / mm_bg_usual` at frame `10`
- `loading` from `ui_loading.yncp / pda` at frame `75`
- `sonic-hud` from `ui_prov_playscreen.yncp / so_speed_gauge` at frame `99`
- `tutorial` from `ui_prov_playscreen.yncp / info_1` at frame `50`

Local-only outputs are written under:

```text
out/csd_render_compare/phase127/
```

That folder is ignored and must stay local-only because it contains rendered frames from local Sonic assets.

## What Is Real Now

- The renderer writes actual `1280x720` BMP frames, not only descriptor text.
- Draws use the parsed CSD scene/cast/subimage command stream.
- DDS source rectangles are resolved from local extracted files.
- CSD `cast_info.color` is applied as base RGB/alpha modulation.
- GDI+ source-over blending is used for the current material pass.
- Timeline transform samples are applied when the sampled timeline scene matches the drawable scene.
- Native BMP evidence is resolved from `out/ui_lab_runtime_evidence/`.
- The manifest records render path, native best path, command counts, SGFX slots, material semantics, and sampled RGB deltas.

## Still Not Claimed

This is not full CSD renderer parity yet. The current hard-pixel loop does not fully decode every original renderer semantic:

- gradient/color keyframe payloads that still parse as `NaN`
- non-source-over blend modes if hidden in fields not decoded yet
- exact Xenos shader/material behavior
- movie-backed scene composition beyond already extracted preview evidence
- full offscreen/native image alignment beyond a fixed `64x36` sampled RGB grid

So Phase 127 is the first pixel-real comparison gate, not the final replacement for UnleashedRecomp. The real runtime remains the oracle.

## Verification

Fresh local verification for this beat used:

```powershell
cmake -S research_uiux\runtime_reference -B b\rr127
cmake --build b\rr127 --config Release --target sward_su_ui_asset_renderer sward_sgfx_template_catalog
$env:SWARD_SU_UI_RENDERER_EXE='b\rr127\Release\sward_su_ui_asset_renderer.exe'
python research_uiux\runtime_reference\examples\test_su_ui_asset_renderer.py
python research_uiux\runtime_reference\examples\test_sgfx_template_catalog.py
python research_uiux\runtime_reference\examples\test_unleashed_recomp_ui_lab_contract.py
```
