<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Asset CSD Subimage Draw Descriptors

Phase 84 moves Asset View past selected-element footprint/crop inspection and into renderer-facing drawable evidence:

```text
b/rr84/sward_ui_runtime_debug_gui.exe
```

What changed:

- added `LayoutCsdCastSubimageBinding` descriptors for selected CSD element -> drawable cast/subimage evidence
- bound first draw descriptors for Sonic HUD, Extra Stage HUD, Werehog HUD, Loading, Title, and the Pause no-subimage backing case
- surfaced `CSD cast/subimage:` in the GUI detail pane and runtime snapshot text
- added an Asset View cue for the selected cast/subimage descriptor
- added `--asset-csd-subimage-smoke` for non-windowing verification

Verified smoke descriptor examples:

```text
sonic=so_speed_gauge/position_hd->Cast_0506_bg:sub=1:tex=ui_ps1_gauge1.dds:uv=0.015625,0.500000-0.078125,0.656250:cast=16x20
loading=bg_1/arrow->img_1:sub=198:tex=mat_load_comon_001.dds:uv=0.464844,0.168056-0.699219,0.501389:cast=300x240
title=mm_bg_usual/black3->black3:sub=14:tex=ui_mm_parts1.dds:uv=0.700000,0.525000-0.712500,0.550000:cast=16x16
pause=bg/img->img:sub=none:tex=none:cast=1280x720
```

Evidence basis:

- `ui_prov_playscreen.yncp` / `so_speed_gauge` -> `position_hd` first drawable descendant `Cast_0506_bg`, subimage `1`, texture `ui_ps1_gauge1.dds`
- `ui_prov_playscreen.yncp` / `ring_get_effect` -> `position_hd` first drawable descendant `flash`, subimage `52`, texture `ui_ps1_gauge1.dds`
- `ui_loading.yncp` / `bg_1` -> `arrow` first drawable descendant `img_1`, subimage `198`, texture `mat_load_comon_001.dds`
- `ui_mainmenu.xncp` / `mm_bg_usual` -> `black3` direct drawable subimage `14`, texture `ui_mm_parts1.dds`
- `ui_pause.yncp` / `bg` -> `img` has cast geometry but no decoded subimage payload in the selected parsed scene

Current limit:

- the descriptor is not yet a full renderer command stream; atlas-packing and CSD texture-page placement are still represented as evidence descriptors/cues.
- the next step is to turn these descriptors into family-specific subimage/cast draw commands, then bind sampled CSD channels and timeline playback to the same draw path.

Follow-on:

- Phase 85 derives the first selected source/destination draw-command descriptors in [`GUI_ASSET_CSD_SUBIMAGE_DRAW_COMMANDS.md`](./GUI_ASSET_CSD_SUBIMAGE_DRAW_COMMANDS.md).
