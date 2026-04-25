<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Asset CSD DDS Blit Preview

Phase 87 turns the selected CSD render-plan rectangles into the first local DDS-backed blit preview path:

```text
b/rr87/sward_ui_runtime_debug_gui.exe
```

What changed:

- added curated local DDS source binding for the current Sonic HUD, Loading, and Title samples
- added a small DXT5 decoder for local `.dds` source textures
- caches decoded DDS source textures for the native GUI preview path
- surfaces `CSD DDS blit:` in the GUI detail pane and runtime snapshot text
- draws the selected decoded source rect into the mini `1280x720` render-target preview when the DDS source is available
- added `--asset-csd-dds-blit-smoke` for non-windowing verification

Verified decoded blit samples:

```text
sonic=ui_ps1_gauge1.dds:DXT5:256x128:src=4,64,16x20:dst=752,357,16x20
loading=mat_load_comon_001.dds:DXT5:1280x720:src=595,121,300x240:dst=350,360,300x240
title=ui_mm_parts1.dds:DXT5:1280x640:src=896,336,16x16:dst=655,435,368x464
pause=fill:dst=0,0,1280x720
```

Evidence basis:

- DDS dimensions are read from the local extracted texture headers
- DXT5 blocks are decoded locally into `32bpp ARGB` pixels for GDI+ drawing
- source rectangles come from the Phase 85 UV/cast-derived draw-command layer
- destination rectangles come from the Phase 86 virtual render-target plan layer
- the decoded blit path is guarded by source-rectangle bounds checks before drawing

Current limit:

- this is still a selected-element blit path, not full scene composition.
- the next step is to broaden source-texture discovery and then drive multiple CSD casts through sampled timeline/channel state in a family-specific render pass.
