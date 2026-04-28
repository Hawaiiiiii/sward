<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# CSD Sampler And Registration Parity

Phase 130 tightens the offscreen CSD rendered-frame comparison after the packed Color/Gradient channel blocker was cleared in Phase 129.

The smoke command is:

```powershell
b\rr130\Release\sward_su_ui_asset_renderer.exe --csd-render-compare-smoke
```

Local-only outputs now land under:

```text
out/csd_render_compare/phase130/
```

That directory stays ignored because the rendered frames use local Sonic placeholder assets and compare against local native BMP evidence.

## What Changed

- Non-linear CSD texture draws now use a dedicated `csd-point-seam` sampler path instead of plain nearest sampling.
- The software renderer records CSD point-seam, bilinear, and nearest sample counts in console output and the manifest.
- Native BMP comparison now uses `search-center-crop-16x9` registration instead of a fixed centered crop only.
- The manifest records the selected native crop, registration offset, candidate count, base sampled delta, and best sampled delta.
- Refreshed runtime evidence from `out/ui_lab_runtime_evidence/20260428_115131/` was used as the current native oracle for `title-menu`, `loading`, `sonic-hud`, and `tutorial`.

## Current Evidence

The Phase 130 compare manifest resolves the refreshed native BMPs as:

- `title-menu`: `native_frame_title-menu_14_1582x853.bmp`, offset `0,0`, mean sampled RGB delta `25.787`.
- `loading`: `native_frame_loading_5_1582x853.bmp`, offset `16,0`, mean sampled RGB delta `30.260`.
- `sonic-hud`: `native_frame_sonic-hud_2_1582x853.bmp`, offset `-17,0`, mean sampled RGB delta `88.467`.
- `tutorial`: `native_frame_tutorial_3_1582x853.bmp`, offset `-16,0`, mean sampled RGB delta `87.922`.

Observed CSD point-seam sample counts in the same manifest:

- `title-menu`: `246758`
- `loading`: `544442`
- `sonic-hud`: `8788`
- `tutorial`: `18818`

## Still Not Shader-Perfect

- `csd-point-seam` is a local software approximation of the CSD/UI shader sampler; it is not yet a byte-for-byte Xenos/D3D shader clone.
- Registration searches a small centered crop neighborhood and scores a `64x36` sampled grid, not a full-frame exact diff.
- Runtime-native BMPs remain the visual oracle, and the sidecar renderer remains a local portable reconstruction lane.

## Verification

Fresh verification for this beat used:

```powershell
cmake -S research_uiux\runtime_reference -B b\rr130
cmake --build b\rr130 --config Release --target sward_su_ui_asset_renderer sward_sgfx_template_catalog
powershell -ExecutionPolicy Bypass -Command "& { & 'research_uiux\runtime_reference\tools\capture_unleashed_recomp_ui_lab.ps1' -TargetSet custom -Targets title-menu,loading,sonic-hud,tutorial -InitialWaitSeconds 6 -SecondCaptureDelaySeconds 8 -AutoExitSeconds 55 -HideOverlay -NoBuild -NativeCapture:`$true -NativeCaptureCount 4 -NativeCaptureIntervalFrames 60 -SkipWindowScreenshots -RequireNativeRgbSignal -UseLiveBridgeReadiness -UseUniqueLiveBridgeName }"
$env:SWARD_SU_UI_RENDERER_EXE='b\rr130\Release\sward_su_ui_asset_renderer.exe'
python research_uiux\runtime_reference\examples\test_su_ui_asset_renderer.py
$env:SWARD_SGFX_TEMPLATE_CATALOG_EXE='b\rr130\Release\sward_sgfx_template_catalog.exe'
python research_uiux\runtime_reference\examples\test_sgfx_template_catalog.py
python research_uiux\runtime_reference\examples\test_unleashed_recomp_ui_lab_contract.py
python research_uiux\runtime_reference\examples\test_ui_debug_workbench_catalog.py
python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py
```
