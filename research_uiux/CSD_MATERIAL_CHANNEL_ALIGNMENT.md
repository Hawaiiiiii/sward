<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# CSD Material Channel Alignment

Phase 128 tightens the local CSD rendered-frame comparison loop toward shader parity.

> [!NOTE]
> Phase 129 supersedes the active compare output lane with packed RGBA timeline decoding and a software ARGB quad rasterizer under `out/csd_render_compare/phase129/`. See `CSD_PACKED_CHANNEL_SOFTWARE_QUADS.md`.

The same smoke command is used:

```powershell
b\rr128\Release\sward_su_ui_asset_renderer.exe --csd-render-compare-smoke
```

Local-only outputs now land under:

```text
out/csd_render_compare/phase128/
```

That directory stays ignored because the BMPs are rendered from local Sonic placeholder assets.

## What Improved

- CSD packed colors now follow Shuriken's RGBA byte order instead of treating the value as ARGB.
- Cast flags are promoted into render semantics:
  - normal blend: `src-alpha/inv-src-alpha`
  - additive blend flag: `src-alpha/one`
  - linear filtering flag
  - flag-driven mirror X/Y
- Static cast color alpha now contributes to the rendered frame using the corrected RGBA channel order.
- Native BMP comparison samples a centered `16:9` crop instead of the whole window/backbuffer dimensions, reducing false delta from capture aspect mismatch.
- The manifest now records `materialSemantics`, `channelSemantics`, and `nativeAlignment` blocks for every template.

## Current Channel Blockers

The local `layout_deep_analysis.json` still serializes packed Color/Gradient keyframes as non-finite `NaN` values because the original parser read those payloads as floats. Phase 128 does not fake those channels. It records them explicitly:

- `packedColorTracks`
- `packedGradientTracks`
- `unresolvedPackedKeyframes`

Observed in the current smoke:

- `loading` has packed color tracks still awaiting raw RGBA extraction.
- `sonic-hud` has many packed gradient tracks still awaiting raw RGBA extraction.
- `title-menu` and `tutorial` currently do not expose unresolved packed channel blockers in the selected sampled paths.

## Still Not Shader-Perfect

This is closer, but not final CSD parity:

- GDI+ cannot draw a textured quad with true per-vertex color gradients, so gradient defaults are currently averaged when present and reported as an approximation.
- Additive blend flags are identified and reported, but the offscreen GDI+ path still renders through source-over until a lower-level software/WebGL/D3D quad path replaces it.
- Packed Color/Gradient timeline values need raw integer extraction from the CSD binary, then channel interpolation matching Shuriken's `GetColor` behavior.
- The real UnleashedRecomp runtime remains the visual oracle.

## Verification

Fresh verification for this beat used:

```powershell
cmake -S research_uiux\runtime_reference -B b\rr128
cmake --build b\rr128 --config Release --target sward_su_ui_asset_renderer sward_sgfx_template_catalog
$env:SWARD_SU_UI_RENDERER_EXE='b\rr128\Release\sward_su_ui_asset_renderer.exe'
python research_uiux\runtime_reference\examples\test_su_ui_asset_renderer.py
python research_uiux\runtime_reference\examples\test_sgfx_template_catalog.py
python research_uiux\runtime_reference\examples\test_unleashed_recomp_ui_lab_contract.py
```
