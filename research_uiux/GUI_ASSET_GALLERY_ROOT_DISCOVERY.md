<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Asset Gallery Root Discovery

Phase 80 hardens the native GUI asset-viewer path and starts the atlas gallery lane:

```text
b/rr80/sward_ui_runtime_debug_gui.exe
```

## What Changed

- fixed Asset View resolving `sheets=0` when the GUI is launched from `b/rrXX` instead of the repo root
- added `executableDirectory()`
- added `visualAtlasSheetRootCandidates()`
- added `visualAtlasSheetFiles()`
- supports `SWARD_UI_ASSET_ROOT` for explicit local asset-root override
- walks ancestor directories from both current working directory and executable directory until it finds `extracted_assets/visual_atlas/sheets`
- added sorted atlas-sheet gallery indexing
- added `Asset Prev` / `Asset Next` buttons
- syncs the gallery selection back to the selected host's atlas candidate when changing host
- added `--asset-gallery-smoke`

## Verified Behavior

The same asset-view smoke now succeeds from both the repo root and the build directory:

```powershell
b\rr80\sward_ui_runtime_debug_gui.exe --asset-view-smoke

cd b\rr80
.\sward_ui_runtime_debug_gui.exe --asset-view-smoke
```

Verified output:

```text
sward_ui_runtime_debug_gui asset view smoke ok atlas_candidates=10 sheet_files=22 title_asset=mainmenu__ui_mainmenu.png:exact:exists=1 loading_asset=loading__ui_loading.png:exact:exists=1 sonic_asset=exstagetails_common__ui_prov_playscreen.png:proxy:exists=1
```

The first gallery smoke verifies deterministic sorted navigation around the Sonic gameplay-HUD proxy sheet:

```text
sward_ui_runtime_debug_gui asset gallery smoke ok sheet_files=22 first=actioncommon__ui_gate.png selected=exstagetails_common__ui_prov_playscreen.png previous=exstagetails_common__ui_exstage.png next=exstagetails_common__ui_qte.png
```

## Boundary

This is still atlas-sheet browsing, not a decoded DDS/subimage/CSD scene browser. The important product shift is that the native GUI now has stable local asset discovery and the first operator controls for walking the recovered sheet inventory.

Phase 81 continues that lane in [`GUI_ASSET_CSD_ELEMENT_BINDINGS.md`](./GUI_ASSET_CSD_ELEMENT_BINDINGS.md), binding selected atlas sheets to recovered CSD package/scene/cast/subimage evidence so Asset View can start behaving like an element inspector.
