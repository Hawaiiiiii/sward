<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# CSD Packed Channels And Software Quads

Phase 129 moves the CSD rendered-frame comparison from material reporting into a more faithful local draw path.

> [!NOTE]
> Phase 130 keeps the software ARGB quad path but moves the active compare lane to `out/csd_render_compare/phase130/`, adds the local `csd-point-seam` sampler approximation, and registers native BMP crops before scoring. See `CSD_SAMPLER_REGISTRATION_PARITY.md`.

The smoke command is:

```powershell
b\rr129\Release\sward_su_ui_asset_renderer.exe --csd-render-compare-smoke
```

Local-only outputs now land under:

```text
out/csd_render_compare/phase129/
```

That directory stays ignored because the BMPs are rendered from local Sonic placeholder assets.

## What Changed

- `inspect_xncp_yncp.py` now preserves each keyframe value's raw `u32` payload as `value_raw_bits`.
- Color and Gradient keyframes also get a `packed_rgba` field so sidecar tooling no longer has to infer color channels from a serialized `NaN`.
- The renderer decodes packed timeline Color/Gradient tracks through Shuriken-compatible RGBA byte order.
- Timeline samples now include `timeline_rgba_sample=` descriptors with RGBA hex and channel components.
- The offscreen compare path renders CSD commands through a software ARGB quad rasterizer instead of the old GDI+ textured-rect approximation.
- Per-pixel CSD color is now texture sample multiplied by cast color and bilinear per-corner gradient color.
- `src-alpha/inv-src-alpha` and `src-alpha/one` additive paths are implemented in the local software compositor.

## Current Evidence

After regenerating ignored local `research_uiux/data/layout_deep_analysis.json` from `extracted_assets`, the current compare smoke reports:

- `loading`: `12` packed color tracks, `86` decoded packed keyframes, `0` unresolved packed keyframes.
- `sonic-hud`: `172` packed gradient tracks, `356` decoded packed keyframes, `0` unresolved packed keyframes.
- `tutorial`: `16` packed gradient tracks, `16` decoded packed keyframes, `0` unresolved packed keyframes.
- `title-menu`: no packed timeline channel blockers in the selected sampled path.

## Still Not Shader-Perfect

- Phase 130 adds a named `csd-point-seam` sampler approximation, but it is still not a byte-for-byte Xenos/D3D shader clone.
- Current selected title/loading/HUD/tutorial scenes do not exercise additive CSD commands after regenerated field decoding, but the software `src-alpha/one` path is now present for scenes that do.
- Native BMP comparison now performs a small centered registration search, but it is still a sampled-grid delta, not a full perceptual or exact frame diff.
- The real UnleashedRecomp runtime remains the visual oracle.

## Verification

Fresh verification for this beat used:

```powershell
python research_uiux\tools\inspect_xncp_yncp.py --root extracted_assets --output research_uiux\data\layout_deep_analysis.json
cmake -S research_uiux\runtime_reference -B b\rr129
cmake --build b\rr129 --config Release --target sward_su_ui_asset_renderer sward_sgfx_template_catalog
$env:SWARD_SU_UI_RENDERER_EXE='b\rr129\Release\sward_su_ui_asset_renderer.exe'
python research_uiux\runtime_reference\examples\test_su_ui_asset_renderer.py
python research_uiux\runtime_reference\examples\test_sgfx_template_catalog.py
python research_uiux\runtime_reference\examples\test_unleashed_recomp_ui_lab_contract.py
```
