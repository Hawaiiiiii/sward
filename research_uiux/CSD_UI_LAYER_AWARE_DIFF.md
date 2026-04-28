<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# CSD UI Layer Aware Diff

Phase 132 adds a CSD coverage-mask comparison lane on top of Phase 131's full native-frame diff.

The smoke command is:

```powershell
b\rr132\Release\sward_su_ui_asset_renderer.exe --csd-render-compare-smoke
```

Local-only outputs now land under:

```text
out/csd_render_compare/phase132/
```

That directory stays ignored because it contains Sonic placeholder renders, native-derived diff BMPs, and local evidence paths.

## What Changed

- The software CSD compositor now emits a coverage mask for pixels touched by rendered CSD draw commands.
- The compare path writes a second per-target diff BMP named `*_ui_layer_diff.bmp`.
- The manifest records `uiLayerDiffFramePath` and `visualDelta.uiLayerDelta`.
- Console output now includes `ui_layer_diff_frame_path=` and `ui_layer_delta=` lines.
- `material_parity_triage` flags now include `ui-layer-aware-diff-active` when the coverage-mask pass ran.

## Current Evidence

The current Phase 132 manifest reports:

- `title-menu`: full-frame mean RGB delta `26.527`, UI-layer mean `49.515`, masked pixels `150973`, coverage `0.163816`.
- `loading`: full-frame mean RGB delta `31.983`, UI-layer mean `72.462`, masked pixels `401699`, coverage `0.435871`.
- `sonic-hud`: full-frame mean RGB delta `88.995`, UI-layer mean `91.543`, masked pixels `4142`, coverage `0.004494`.
- `tutorial`: full-frame mean RGB delta `88.577`, UI-layer mean `48.283`, masked pixels `10396`, coverage `0.011280`.

The important split:

- `tutorial` gets a substantially cleaner signal once the native stage background is masked out.
- `sonic-hud` remains high even inside the current rendered CSD coverage, so the next issue is likely wrong/partial HUD scene selection, placement/timing, material state, or the need for a runtime UI-only capture layer.
- `loading` remains a full-screen material/gradient parity lane rather than a stage-background issue.

## Still Not Claimed

This is not UI-only native capture yet. The native side of the masked comparison still comes from the full backbuffer, sampled only where the local CSD renderer drew pixels. A true runtime UI-layer capture would be stronger if we can expose it through UnleashedRecomp.

## Verification

Fresh verification for this beat used:

```powershell
cmake -S research_uiux\runtime_reference -B b\rr132
cmake --build b\rr132 --config Release --target sward_su_ui_asset_renderer sward_sgfx_template_catalog
$env:SWARD_SU_UI_RENDERER_EXE='b\rr132\Release\sward_su_ui_asset_renderer.exe'
python research_uiux\runtime_reference\examples\test_su_ui_asset_renderer.py
$env:SWARD_SGFX_TEMPLATE_CATALOG_EXE='b\rr132\Release\sward_sgfx_template_catalog.exe'
python research_uiux\runtime_reference\examples\test_sgfx_template_catalog.py
python research_uiux\runtime_reference\examples\test_unleashed_recomp_ui_lab_contract.py
python research_uiux\runtime_reference\examples\test_ui_debug_workbench_catalog.py
python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py
```
