<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="96" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> SU UI Renderer Title Loop Reconstruction

Phase 93 moves the clean `sward_su_ui_asset_renderer.exe` title page from a symbolic approximation into a real local title-loop composition.

The renderer now opens on `TitleLoopReconstruction`, which layers:

- a local-only frame decoded from `game/movie/evmo_title_loop.sfd`
- the local-only `OPmovie_titlelogo_EN` texture after Xbox LZX expansion through `tools/x_decompress`
- the `PRESS START` and `SEGA`/copyright text crops from `mat_title_en_001.dds`
- source-family evidence from `ui_title/bg/bg`, `mm_title_intro`, `CTitleStateIntro::Update`, and `UseAlternateTitleMidAsmHook`

The important correction is architectural: the machine-translated PPC and Unleashed Recompiled patch layer do matter, but they do not produce a clean standalone `DrawTitleScreen()` source file. They identify state ownership, input gates, region/title hooks, scene names, and timing/control seams. The renderer still has to bind the real Hedgehog Engine substrates: SFD movie frames, XCompress/LZX payloads, DDS/CSD materials, and authored layout/timeline data.

## Local-Only Inputs

The title-loop visual path intentionally consumes local generated previews under ignored asset roots:

- `extracted_assets/runtime_previews/title/evmo_title_loop_00_00_35_000.png`
- `extracted_assets/runtime_previews/title/decompressed/OPmovie_titlelogo_EN.decompressed.dds`
- `extracted_assets/runtime_previews/title/decompressed/OPmovie_titlelogo_EN.decompressed.png`

Those files stay local-only. They are derived from the operator's installed game files and are not committed.

## Verification

Verified under `b/rr93`:

```text
python research_uiux\runtime_reference\examples\test_su_ui_asset_renderer.py
b\rr93\sward_su_ui_asset_renderer.exe --renderer-smoke
b\rr93\sward_su_ui_asset_renderer.exe --renderer-title-screen-smoke
b\rr93\sward_su_ui_asset_renderer.exe --renderer-navigation-smoke
b\rr93\sward_su_ui_asset_renderer.exe --renderer-reconstructed-screen-smoke
b\rr93\sward_su_ui_asset_renderer.exe --renderer-atlas-gallery-smoke
```

Current smoke-guarded renderer inventory:

- `8` screen samples
- `20` local DDS-backed evidence casts
- `4` title-loop reconstruction casts
- `22` local visual-atlas sheets
- title smoke confirms `movie_frame=exists`, `title_logo_preview=exists`, `title_logo_preview_bitmap=loads`, `title_logo=exists`

## Remaining Gap

This is still not broad SU UI parity. The next renderer-grade work is:

- native XCompress/LZX decode inside the renderer instead of relying on pre-decoded local PNG previews
- exact CSD cast transform/timeline playback instead of hand-placed title composition coordinates
- frame cycling/playback for `evmo_title_loop.sfd`
- family-by-family reconstructed screens for loading, main menu, HUD, tutorial/help, result/status, world-map, and message windows
