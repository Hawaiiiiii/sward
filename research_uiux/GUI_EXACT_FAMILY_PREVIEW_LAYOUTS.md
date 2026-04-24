<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Exact-Family Preview Layouts

Phase 56 adds the first exact-family visual placement adapter to the native GUI workbench:

```text
b/rr56/sward_ui_runtime_debug_gui.exe
```

This beat targets families with exact local atlas candidates first: Title, Pause, and Loading. It does not decode original `.xncp` / `.yncp` node transforms yet. The workbench now has a separate placement layer where those decoded transforms can land later without being mixed into the generic HUD role projection.

## What Changed

- added `PreviewFamily` classification for exact Title, Pause, and Loading contracts
- added family-specific layout projection functions for:
  - `title_menu_reference.json`
  - `pause_menu_reference.json`
  - `loading_transition_reference.json`
- placed Title `logo`, `content`, `prompt`, and `transient_fx` roles in title-like regions rather than generic stacked bars
- placed Pause `chrome`, `content`, `prompt`, and `transient_fx` roles into framed pause-shell regions
- placed Loading `cinematic_frame`, PDA/content, tip copy, and controller variant roles into loading-shell regions
- kept the existing generic role layout for gameplay HUD, boss HUD, town, camera, application/world, support-substrate, and other families
- added `--family-preview-smoke` so automation can verify the exact-family layout path without opening the GUI window

Verified family-preview smoke:

```text
sward_ui_runtime_debug_gui family preview smoke ok title=title_menu pause=pause_menu loading=loading_transition title_logo_y=64.8 title_content_y=331.2 pause_chrome_w=844.8 loading_frame_y=187.2
```

## Why It Matters

Phase 55 made preview overlays move with state. Phase 56 starts splitting visual placement by recovered family, which is the right direction for a proper UI/UX workbench:

- exact-atlas menu families stop sharing the same generic layer stack as HUD families
- the renderer now has a named adapter boundary for family-specific visual reconstruction
- future decoded layout/CSD nodes can replace a single family adapter instead of rewriting the whole preview path
- Title/Pause/Loading become better first targets for parity checks because their atlas bindings are exact rather than proxy

## Current Limits

- Family placement is still a projected diagnostic layout, not original CSD node placement.
- The adapter uses contract role names and source-family evidence, not recovered keyframe curves.
- Local atlas sheets remain ignored under `extracted_assets/visual_atlas`.
- This does not change the Sonic/Werehog HUD proxy boundary; gameplay HUD exact `ui_playscreen*` payload recovery is still open.

## Verification

Fresh verification on `2026-04-24`:

```text
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr56 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr56 --config Release"
python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py
b\rr56\sward_ui_runtime_debug_gui.exe --family-preview-smoke
b\rr56\sward_ui_runtime_debug_gui.exe --motion-smoke
b\rr56\sward_ui_runtime_debug_gui.exe --playback-smoke
b\rr56\sward_ui_runtime_debug_gui.exe --preview-smoke
b\rr56\sward_ui_runtime_debug_gui.exe --smoke
```

The GUI was also launch/capture-checked by selecting `Menu / Stage Debug Hosts -> GameModeMainMenu_Test.cpp`, pressing `Run Host`, and capturing the rr56 window while the title contract was in the `Intro` band.
