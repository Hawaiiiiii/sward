# SU UI Renderer Reconstructed Screen

Phase 92 corrected the clean renderer product direction after the visual-atlas gallery exposed an important problem: browsing sheets is useful evidence, but it is not a reconstructed game screen.

The renderer now opens on `SonicHudReconstruction`, a family-specific Sonic HUD surface that keeps the local atlas/gallery available as a secondary view while making the first page behave like a screen renderer. The reconstructed page is backed by the recovered `ui_prov_playscreen.yncp` family, `sonic_stage_hud_reference.json`, and local DXT5 material bindings including:

- `ui_ps1_gauge1.dds`
- `mat_playscreen_001.dds`
- `mat_playscreen_en_001.dds`
- `mat_comon_num_001.dds`
- `mat_comon_001.dds`

The new path deliberately separates two layers:

- evidence casts, which remain smoke-guarded source/destination records for the recovered play-screen materials
- readable reconstruction drawing, which renders the Sonic HUD layout as portable C++ primitives and screen-space composition instead of dumping raw atlas fragments

Verified local binary:

- `b/rr92/sward_su_ui_asset_renderer.exe`

Verified smoke commands:

```powershell
python -m unittest research_uiux.runtime_reference.examples.test_su_ui_asset_renderer
b/rr92/sward_su_ui_asset_renderer.exe --renderer-reconstructed-screen-smoke
b/rr92/sward_su_ui_asset_renderer.exe --renderer-smoke
b/rr92/sward_su_ui_asset_renderer.exe --renderer-navigation-smoke
b/rr92/sward_su_ui_asset_renderer.exe --renderer-atlas-gallery-smoke
```

Current renderer inventory:

- `7` screen samples
- `16` local DDS-backed casts
- `8` Sonic HUD reconstruction evidence casts
- `22` local visual-atlas sheets retained as a support view
- `5` visible navigation controls

This is still not final `1:1` CSD playback. The next product step is to replace more family samples with dedicated reconstructed render paths, then feed them from sampled CSD channels and PPC-backed state/timeline behavior.
