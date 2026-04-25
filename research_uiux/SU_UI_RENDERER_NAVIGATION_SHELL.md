# SWARD SU UI Renderer Navigation Shell

Phase 90 fixes the clean renderer's first usability failure: it is no longer a hidden keyboard-only window.

The renderer is still not full SU UI parity. It now has a product-facing shell around the current asset-backed screen catalog so the next CSD/cast reconstruction work lands inside an app that can actually be driven and tested.

## Native Target

```powershell
b/rr90/sward_su_ui_asset_renderer.exe
b/rr90/sward_su_ui_asset_renderer.exe --renderer-smoke
b/rr90/sward_su_ui_asset_renderer.exe --renderer-navigation-smoke
```

Visible controls now include:

- `Prev`: previous screen sample
- `Next`: next screen sample
- screen label: current index, screen ID, and display name

Keyboard controls remain:

- `Right` / `Space`: next screen sample
- `Left`: previous screen sample
- `Esc`: close

## Verified Catalog Smoke

The no-window navigation smoke verifies the shell without opening GDI+ UI:

```text
sward_su_ui_asset_renderer navigation smoke ok screens=5 controls=3 first=LoadingComposite last=SonicStageHud label=1/5 LoadingComposite - LoadingTransition Composite
screen=LoadingComposite:casts=1:contract=loading_transition_reference.json
screen=MainMenuComposite:casts=3:contract=title_menu_reference.json
screen=SonicTitleMenu:casts=1:contract=title_menu_reference.json
screen=TitleLogoSheet:casts=2:contract=title_menu_reference.json
screen=SonicStageHud:casts=1:contract=sonic_stage_hud_reference.json
```

The old asset smoke still verifies the same `5` screen samples, `8` casts, and `8` resolved local DDS-backed blits.

## Why This Matters

The renderer's prior state could draw pixels but did not behave like a viewer. That made it easy to confuse a technical blit proof with a usable SU UI/UX product surface.

Phase 90 adds the minimum product shell needed before deeper scene reconstruction:

- native discoverable controls
- deterministic screen index text
- resized render canvas below the control chrome
- a no-window navigation/catalog regression path
- a launched-window sanity signal through the native title text

## Still Missing

This does not solve exact layout or animation parity by itself. The next renderer work should replace the sheet-like pages with decoded CSD scene batches:

1. bind full-family CSD cast batches into the clean renderer catalog
2. draw source DDS subimages by scene/cast order instead of showing whole sheets
3. sample authored keyframe channels for frame-by-frame playback
4. add screen-family navigation for Loading, Main Menu, Pause, Status, Results, World Map, and Gameplay HUD
