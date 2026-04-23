<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> CSD / UI Foundation Humanization

Phase 31 closes the next naming gap after the gameplay-HUD pass: the lower-level CSD/UI foundation that sits under title, pause, status, town, and world-map presentation.

> [!IMPORTANT]
> This report does not claim the translated code is now clean original source. It maps the original source-family scaffold and the reusable scene/widget abstractions that are now defensible from the local evidence.

## Snapshot

- Direct `CSD/*` / `Menu/*` seed paths: `5`
- Closely related consumer/widget paths: `5`
- Mirrored support paths under local-only `SONIC UNLEASHED/`: `12`
- Layout families tied into the foundation map: `11`
- Direct or strong layout families: `10`
- Contextual-only layout families: `1`
- Foundation hooks / seam anchors: `5`
- Modern readable bridge points: `2`
- Translated seam symbols grouped into the foundation layer: `4`

## Local-Only Mirror Scaffold

The local-only mirror under `SONIC UNLEASHED/` is now explicitly carrying the original-family scaffold for:

- `CSD/CsdPlatformMirage.cpp`
- `CSD/CsdProject.cpp`
- `CSD/CsdTexListMirage.cpp`
- `CSD/GameObjectCSD.cpp`
- `Menu/MenuTextBox.cpp`

Mirrored support paths pulled from the root dump include:

- `_inferred/relative_source/source/Core/csdAccess.cpp` (`relative_source`, `medium`)
- `_inferred/relative_source/source/Core/csdLoader.cpp` (`relative_source`, `medium`)
- `_inferred/relative_source/source/Core/csdMatrix.cpp` (`relative_source`, `medium`)
- `_inferred/relative_source/source/Core/csdMotionPalette.cpp` (`relative_source`, `medium`)
- `_inferred/relative_source/source/Manager/csdmMotionPalette.cpp` (`relative_source`, `medium`)
- `_inferred/relative_source/source/Manager/csdmMotionPattern.cpp` (`relative_source`, `medium`)
- `_inferred/relative_source/source/Manager/csdmNode.cpp` (`relative_source`, `medium`)
- `_inferred/relative_source/source/Manager/csdmProject.cpp` (`relative_source`, `medium`)
- `_inferred/relative_source/source/Manager/csdmScene.cpp` (`relative_source`, `medium`)
- `_inferred/relative_include/include/Manager/csdmProject.h` (`relative_include`, `medium`)
- `_inferred/relative_include/include/Manager/csdmRCObject.h` (`relative_include`, `medium`)
- `_inferred/relative_library/library/CSD/include/Manager/csdmRCObject.h` (`relative_library`, `medium`)

## Reusable Abstractions

### CSD Project Pipeline

Project creation, scene/node ownership, mirage render bridging, and low-level motion/palette graph support for authored CSD layouts.

Direct source paths:

- `CSD/CsdPlatformMirage.cpp`
- `CSD/CsdProject.cpp`
- `CSD/CsdTexListMirage.cpp`
- `CSD/GameObjectCSD.cpp`

Mirrored support layer:

- `_inferred/relative_source/source/Core/csdAccess.cpp`
- `_inferred/relative_source/source/Core/csdLoader.cpp`
- `_inferred/relative_source/source/Core/csdMatrix.cpp`
- `_inferred/relative_source/source/Core/csdMotionPalette.cpp`
- `_inferred/relative_source/source/Manager/csdmMotionPalette.cpp`
- `_inferred/relative_source/source/Manager/csdmMotionPattern.cpp`
- `_inferred/relative_source/source/Manager/csdmNode.cpp`
- `_inferred/relative_source/source/Manager/csdmProject.cpp`
- `_inferred/relative_source/source/Manager/csdmScene.cpp`
- `_inferred/relative_include/include/Manager/csdmProject.h`
- `_inferred/relative_include/include/Manager/csdmRCObject.h`
- `_inferred/relative_library/library/CSD/include/Manager/csdmRCObject.h`

Layout bridge: `ui_balloon`, `ui_general`, `ui_mainmenu`, `ui_mediaroom`, `ui_pause`, `ui_shop`, `ui_status`, `ui_townscreen`, `ui_worldmap`, `ui_worldmap_help`

Current consumer systems: `Title Menu`, `Pause Stack`, `Status Overlay`, `World Map Stack`, `Town UI`

### Framed Window / Help Widget Stack

Reusable darkened background, framed content window, header/footer prompt rails, and routed help-request handling.

Direct source paths:

- `Menu/MenuTextBox.cpp`

Consumer/widget paths:

- `HUD/GeneralWindow/GeneralWindow.cpp`
- `HUD/HelpWindow/HelpWindow.cpp`
- `Sequence/Unit/SequenceUnitCallHelpWindow.cpp`

Layout bridge: `ui_general`, `ui_help`, `ui_pause`, `ui_status`, `ui_worldmap_help`

Current consumer systems: `Pause Stack`, `Status Overlay`, `World Map Stack`

### Town Dialog / Shop Widgets

Balloon, talk, shop, and Media Room shell widgets layered on top of the same CSD project/render foundation.

Consumer/widget paths:

- `Town/ShopWindow.cpp`
- `Town/TalkWindow.cpp`

Layout bridge: `ui_balloon`, `ui_shop`, `ui_townscreen`, `ui_mediaroom`

Current consumer systems: `Town UI`

## Layout Bridge

| Layout | Role | Verdict | Consumer systems |
|---|---|---|---|
| `ui_balloon` | `town_dialog_balloon` | `direct` | `Town UI` |
| `ui_general` | `shared_window_shell` | `direct` | `Pause Stack`, `Status Overlay` |
| `ui_help` | `help_overlay` | `contextual` | `Pause Stack` |
| `ui_mainmenu` | `title_menu` | `strong` | `Title Menu` |
| `ui_mediaroom` | `mediaroom_menu` | `direct` | `Town UI` |
| `ui_pause` | `pause_menu` | `direct` | `Pause Stack` |
| `ui_shop` | `town_shop_menu` | `direct` | `Town UI` |
| `ui_status` | `status_overlay` | `direct` | `Status Overlay` |
| `ui_townscreen` | `town_overlay` | `direct` | `Town UI` |
| `ui_worldmap` | `world_map` | `direct` | `World Map Stack` |
| `ui_worldmap_help` | `world_map_help_overlay` | `direct` | `World Map Stack`, `Pause Stack` |

## Hook And Seam Anchors

- `MakeCsdProjectMidAsmHook` at [`UnleashedRecompLib/config/SWA.toml`](./UnleashedRecompLib/config/SWA.toml):884
  Purpose: Intercepts `CCsdProject::Make` mid-construction so layout names and project payloads can be inspected and patched safely.
- `SWA::CCsdProject::Make` at [`UnleashedRecomp/patches/resident_patches.cpp`](./UnleashedRecomp/patches/resident_patches.cpp):55
  Purpose: Resident patch point over project creation, currently used to rewrite `ui_loading.yncp` behavior without changing the authored CSD package format.
- `SWA::CCsdPlatformMirage::Draw` at [`UnleashedRecomp/patches/aspect_ratio_patches.cpp`](./UnleashedRecomp/patches/aspect_ratio_patches.cpp):1171
  Purpose: Primary render bridge for CSD layout projection/scaling. This is the recurring seam that shows up across title, pause, world map, town, mission, and save/load families.
- `SWA::CCsdPlatformMirage::DrawNoTex` at [`UnleashedRecomp/patches/aspect_ratio_patches.cpp`](./UnleashedRecomp/patches/aspect_ratio_patches.cpp):1178
  Purpose: No-texture variant of the same CSD platform bridge, still part of the same scene/render abstraction family.
- `SWA::CHelpWindow::MsgRequestHelp::Impl` at [`UnleashedRecomp/patches/misc_patches.cpp`](./UnleashedRecomp/patches/misc_patches.cpp):143
  Purpose: Help-window dispatch/filter seam that proves the generic help shell is a real routed UI service rather than a one-off screen decoration.

Modern readable bridge points:

- `g_texGeneralWindow` at [`UnleashedRecomp/ui/imgui_utils.cpp`](./UnleashedRecomp/ui/imgui_utils.cpp):17
  Purpose: Modern reusable nine-slice shell that mirrors the original `ui_general` / `ui_pause` framing language.
- `m_pGeneralWindow->m_SelectedIndex` at [`UnleashedRecomp/patches/CTitleStateMenu_patches.cpp`](./UnleashedRecomp/patches/CTitleStateMenu_patches.cpp):74
  Purpose: Readable menu-state bridge showing the generic window shell participates in title-menu confirmation routing, not only pause/status flows.

Translated seam reuse across the active archaeology layer:

- `sub_824C1E60` -> roles `prompt_filter_or_tutorial_gate`; systems `tornado_defense`
- `sub_825E2E70` -> roles `layout_projection_or_scaling`; systems `loading_and_start`, `mission_briefing_and_gate`, `mission_result_family`, `save_and_ending`, `tornado_defense`, `town_ui`, `world_map_stack`
- `sub_825E2E88` -> roles `layout_projection_or_scaling`; systems `loading_and_start`, `mission_briefing_and_gate`, `mission_result_family`, `save_and_ending`, `tornado_defense`, `town_ui`, `world_map_stack`
- `sub_825E4068` -> roles `resident_overlay_visibility`; systems `loading_and_start`, `save_and_ending`

## What Changed In Practice

- The path family `CSD / UI Foundation` is now a first-class mapped system instead of a named-only bucket.
- The local-only source-tree mirror has a stable destination for future translated-file cleanup under the original 2008-style source-family layout.
- The reusable foundation below title/pause/status/world-map/town UI is now described as project pipeline, window/help shell, and town dialog widgets instead of a loose pile of `sub_XXXXXXXX` seams.

## Remaining Gap

- None of this means the translated C++ is already humanized into those folders yet.
- The foundation is mapped and named, but most of it is still not runtime-contract-backed like the current selector/workbench families.
- The next value is to place renamed translated findings into the local-only `SONIC UNLEASHED/` scaffold and then widen host coverage around those named families.
