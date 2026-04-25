<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Asset Viewer Mode

Phase 79 adds the first actual asset-viewer mode to the native SWARD UI runtime debug GUI:

```text
b/rr79/sward_ui_runtime_debug_gui.exe
```

This is the first GUI path aimed at inspecting the recovered local UI assets directly instead of drawing diagnostic renderer overlays on top of them.

## What Changed

- added `PreviewMode::Runtime` and `PreviewMode::Asset`
- added an `Asset View` / `Runtime View` toggle button
- added `drawAssetViewerPreview(...)`
- added `assetViewerSummaryText(...)`
- added `assetViewerAtlasDescriptor(...)`
- added `visualAtlasSheetFileCount()`
- draws the selected host's local atlas PNG unobstructed and aspect-preserved
- suppresses runtime layers, primitive boxes, prompt overlays, layout evidence, and sampled marker overlays while in Asset View
- added `--asset-view-smoke`

## Verified Local Atlas Inventory

The current curated viewer layer sees:

```text
10 contract-to-atlas candidates
22 local visual_atlas/sheets PNG files
```

Verified representative bindings:

```text
title:   mainmenu__ui_mainmenu.png:exact:exists=1
loading: loading__ui_loading.png:exact:exists=1
sonic:   exstagetails_common__ui_prov_playscreen.png:proxy:exists=1
```

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr79 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr79 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr79\sward_ui_runtime_debug_gui.exe --asset-view-smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui asset view smoke ok atlas_candidates=10 sheet_files=22 title_asset=mainmenu__ui_mainmenu.png:exact:exists=1 loading_asset=loading__ui_loading.png:exact:exists=1 sonic_asset=exstagetails_common__ui_prov_playscreen.png:proxy:exists=1
```

## Boundary

This is not yet a whole-game DDS browser, subimage explorer, or CSD scene renderer. It is the first product step that changes the native GUI from purely diagnostic preview toward an asset viewer. The next useful step is to add sheet/gallery navigation and then bind CSD subimage/cast records so individual UI elements can be inspected rather than only the composed atlas sheet.
