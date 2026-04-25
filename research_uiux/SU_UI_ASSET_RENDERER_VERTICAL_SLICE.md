# SWARD SU UI Asset Renderer Vertical Slice

Phase 88 adds the first separate clean renderer product path beside the reconstruction/debug workbench.

The important boundary is intentional:

- `sward_ui_runtime_debug_gui.exe` remains the evidence workbench: host ownership, runtime contracts, CSD cues, translated-source correlation, and diagnostic overlays.
- `sward_su_ui_asset_renderer.exe` is the clean viewer path: asset-backed screen rendering with no archaeology overlays by default.

This is not yet a whole-game UI renderer. It is the first native executable proving that recovered CSD/cast/source-rect evidence can be drawn as actual local DDS-backed UI content in a portable, readable C++ surface.

## Native Target

```powershell
b/rr88/sward_su_ui_asset_renderer.exe
b/rr88/sward_su_ui_asset_renderer.exe --renderer-smoke
```

The executable opens a `1280x720` design canvas scaled into a native Win32/GDI+ window titled `SWARD SU UI Asset Renderer`.

Keyboard controls:

- `Right` / `Space`: next recovered screen sample
- `Left`: previous recovered screen sample
- `Esc`: close

## Current Asset-Backed Screen Samples

The Phase 88 vertical slice binds three exact local DDS sources already recovered by the CSD blit work:

| Screen | Layout/Cast | Texture | Source |
| --- | --- | --- | --- |
| `LoadingTransition` | `bg_1/arrow` | `mat_load_comon_001.dds` | `extracted_assets/ui_extended_archives/Loading/mat_load_comon_001.dds` |
| `SonicTitleMenu` | `mm_bg_usual/black3` | `ui_mm_parts1.dds` | `extracted_assets/ui_frontend_archives/MainMenu/ui_mm_parts1.dds` |
| `SonicStageHud` | `so_speed_gauge/position_hd` | `ui_ps1_gauge1.dds` | `extracted_assets/phase16_support_archives/ExStageTails_Common/ui_ps1_gauge1.dds` |

The smoke probe verifies the same path without opening the window:

```text
sward_su_ui_asset_renderer smoke ok screens=3 casts=3 textures=3
LoadingTransition:bg_1/arrow:mat_load_comon_001.dds:DXT5:1280x720
SonicTitleMenu:mm_bg_usual/black3:ui_mm_parts1.dds:DXT5:1280x640
SonicStageHud:so_speed_gauge/position_hd:ui_ps1_gauge1.dds:DXT5:256x128
```

## What This Unlocks

This turns the prior diagnostic chain into a product-facing route:

1. The debug workbench recovers and verifies host/layout/CSD/translated-source behavior.
2. The renderer consumes cleaned screen/cast/draw-command facts.
3. Each family can graduate from "diagnostic overlay" to "portable readable renderer code."

That is the path toward a reusable SU-style UI framework for other C++ projects: the renderer should eventually own screen catalog data, texture loading, cast transforms, timeline playback, text/button prompts, state transitions, and family-level behaviors without depending on the Sonic-specific debug UI.

## Current Limits

- The renderer has real local DDS blits, but only three seed casts are bound.
- It does not yet composite complete title/loading/HUD scenes.
- It does not yet evaluate complete CSD timelines or text localization flows.
- It does not yet package a generic host API for other projects.

## Next Product Beats

- Move the CSD cast/draw-command structs into a shared renderer-facing catalog instead of duplicating evidence in the debug GUI and renderer.
- Widen from `3` seed casts to full family batches for Title, Loading, Pause, Sonic HUD, Result, Status, World Map, and tutorial overlays.
- Add timeline playback to the renderer itself, using sampled transforms/alpha/visibility from the authored CSD channel work.
- Add a gallery/screen picker that looks like an actual viewer shell rather than a debug workbench.
- Add exportable, Sonic-independent C++ template modules for screen flow, prompts, transitions, and text presentation.
