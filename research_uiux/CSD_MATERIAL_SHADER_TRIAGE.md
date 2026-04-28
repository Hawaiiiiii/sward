<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# CSD Material Shader Triage

Phase 131 adds the first full-frame parity triage pass on top of the sampled CSD/native comparison.

The smoke command is:

```powershell
b\rr131\Release\sward_su_ui_asset_renderer.exe --csd-render-compare-smoke
```

Local-only outputs now land under:

```text
out/csd_render_compare/phase131/
```

That directory stays ignored because it contains local Sonic placeholder renders, diff BMPs, and native evidence paths.

## What Changed

- The renderer now writes a per-target diff BMP beside each rendered frame.
- The manifest records `diffFramePath` and a `visualDelta.fullFrame` block.
- Full-frame comparison uses the registered native crop from Phase 130 and samples every `1280x720` design pixel.
- Console output now includes `full_frame_delta=` lines with exact-match pixels, significant-delta pixels, mean/max RGB delta, and render/native nonblack coverage.
- Console output and the manifest now include `material_parity_triage`, separating true shader/material risks from missing native backbuffer composition.

## Current Evidence

The current Phase 131 manifest reports:

- `title-menu`: primary blocker `native-background-not-rendered`, full-frame mean RGB delta `26.527`, render coverage `0.061638`, native coverage `0.586953`.
- `loading`: primary blocker `gradient-material-delta`, full-frame mean RGB delta `31.983`, render coverage `0.421782`, native coverage `0.009362`.
- `sonic-hud`: primary blocker `stage-background-not-rendered`, full-frame mean RGB delta `88.995`, render coverage `0.003981`, native coverage `0.860169`.
- `tutorial`: primary blocker `stage-background-not-rendered`, full-frame mean RGB delta `88.577`, render coverage `0.009800`, native coverage `0.860160`.

That means the high HUD/tutorial deltas are not primarily a CSD sampler bug. The local viewer is currently drawing UI-only CSD layers over black, while the real native evidence includes the live stage/world backbuffer behind the HUD.

## Next Parity Implication

For HUD/tutorial, the next useful comparison lane is an alpha/coverage-aware UI-layer diff or a runtime-captured UI-only layer if we can expose one. Tuning CSD shader math against the full native stage frame would chase the wrong signal.

For loading/title/menu, material work is still useful because those screens are closer to full-screen UI composition.

## Verification

Fresh verification for this beat used:

```powershell
cmake -S research_uiux\runtime_reference -B b\rr131
cmake --build b\rr131 --config Release --target sward_su_ui_asset_renderer sward_sgfx_template_catalog
$env:SWARD_SU_UI_RENDERER_EXE='b\rr131\Release\sward_su_ui_asset_renderer.exe'
python research_uiux\runtime_reference\examples\test_su_ui_asset_renderer.py
$env:SWARD_SGFX_TEMPLATE_CATALOG_EXE='b\rr131\Release\sward_sgfx_template_catalog.exe'
python research_uiux\runtime_reference\examples\test_sgfx_template_catalog.py
python research_uiux\runtime_reference\examples\test_unleashed_recomp_ui_lab_contract.py
python research_uiux\runtime_reference\examples\test_ui_debug_workbench_catalog.py
python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py
```
