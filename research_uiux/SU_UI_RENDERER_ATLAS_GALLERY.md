# SWARD SU UI Renderer Atlas Gallery

Phase 91 moves the clean renderer from a fixed five-sample viewer toward a real local SU UI asset viewer.

It now starts on a `VisualAtlasGallery` page that discovers the local, ignored visual-atlas PNG sheets at runtime and renders them directly inside the clean native viewer.

## Native Target

```powershell
b/rr91/sward_su_ui_asset_renderer.exe
b/rr91/sward_su_ui_asset_renderer.exe --renderer-smoke
b/rr91/sward_su_ui_asset_renderer.exe --renderer-navigation-smoke
b/rr91/sward_su_ui_asset_renderer.exe --renderer-atlas-gallery-smoke
```

Visible controls now include:

- `Prev` / `Next`: renderer screen sample navigation
- `Atlas Prev` / `Atlas Next`: local atlas-sheet navigation
- screen label: current renderer page plus selected atlas sheet when in gallery mode

## Verified Local Atlas Inventory

The atlas-gallery smoke verifies the local ignored asset inventory without opening the window:

```text
sward_su_ui_asset_renderer atlas gallery smoke ok sheets=22 first=actioncommon__ui_gate.png loading=loading__ui_loading.png mainmenu=mainmenu__ui_mainmenu.png status=systemcommoncore__ui_status.png
```

The gallery is discovered from:

```text
extracted_assets/visual_atlas/sheets/*.png
```

Those PNG sheets stay local-only and ignored. The committed renderer code only contains the discovery and rendering path.

## Why This Matters

The previous clean renderer still behaved like a curated proof. Phase 91 gives it a broad asset-viewer lane over the current extracted UI corpus:

- `22` local atlas sheets are browseable from the clean renderer
- Loading, Main Menu, Status, World Map, Pause, Result, gameplay HUD proxy, and support UI sheets are reachable without the debug workbench panes
- the hardcoded DDS-backed composition samples remain available as separate renderer screens
- the app now starts somewhere visibly browsable instead of on a single loading texture sheet

## Still Missing

This is still an atlas/gallery viewer, not exact scene playback. The next renderer beat should turn selected atlas/layout families into ordered scene draw calls:

1. load one full CSD package family at a time
2. draw scene/cast/subimage batches in authored order
3. evaluate sampled transform/color/visibility keyframes
4. expose family filters for Loading, Main Menu, Pause, Status, Results, World Map, and Gameplay HUD
