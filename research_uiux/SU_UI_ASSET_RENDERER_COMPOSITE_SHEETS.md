# SWARD SU UI Asset Renderer Composite Sheets

Phase 89 fixes the first clean renderer mistake: the renderer no longer opens on a single cropped `arrow` cast.

The debug workbench can still inspect individual CSD subimages, but the product-facing renderer needs to show useful UI surfaces. This beat changes the first selected screen to a full local DDS-backed loading composition and widens the gallery to more complete menu/title sheet surfaces.

## Native Target

```powershell
b/rr89/sward_su_ui_asset_renderer.exe
b/rr89/sward_su_ui_asset_renderer.exe --renderer-smoke
```

Keyboard controls remain:

- `Right` / `Space`: next screen sample
- `Left`: previous screen sample
- `Esc`: close

## Renderer Inventory

Phase 89 smoke output verifies:

```text
sward_su_ui_asset_renderer smoke ok screens=5 casts=8 textures=8 full_screen_casts=1
```

The renderer now includes:

| Screen | Asset-backed draw set |
| --- | --- |
| `LoadingComposite` | full-screen `mat_load_comon_001.dds` |
| `MainMenuComposite` | `ui_mm_base.dds`, `ui_mm_parts1.dds`, `ui_mm_contentstext.dds` sheet surfaces |
| `SonicTitleMenu` | the prior `mm_bg_usual/black3` CSD crop kept for regression contrast |
| `TitleLogoSheet` | English title texture sheets `mat_title_en_001.dds` and `mat_title_en_002.dds` |
| `SonicStageHud` | the prior `so_speed_gauge/position_hd` CSD crop kept for regression contrast |

## Why This Matters

The previous Phase 88 result was too debug-shaped: it proved DDS decode and CSD source rects, but it did not look like a useful UI viewer.

Phase 89 changes the renderer rule:

- product renderer pages should prefer composed/full-screen or full-sheet surfaces first
- isolated CSD crops remain useful only as evidence or regression samples
- smoke coverage now explicitly verifies `full_screen_casts=1`
- the standalone `LoadingTransition:bg_1/arrow` page is removed from the clean renderer path

## Still Missing

This is not complete SU UI parity. The next step is to stop treating menu/title/status/result assets as loose sheets and instead feed the renderer exact CSD scene transforms, draw order, alpha/visibility channels, text payloads, and transition timelines.

The correct next product lane is:

1. shared renderer catalog extracted from the debug workbench evidence
2. full-family CSD cast/draw-command batches for Loading, Main Menu, Pause, Status, Results, World Map, and Gameplay HUD
3. timeline playback in the clean renderer
4. a real viewer shell with screen/family navigation rather than only keyboard cycling
