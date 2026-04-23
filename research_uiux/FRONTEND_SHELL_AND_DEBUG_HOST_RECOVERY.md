<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Frontend Shell And Debug Host Recovery

Phase 35 tightens the newly widened shell/debug families instead of leaving them as a pile of named-but-unowned paths.

> [!IMPORTANT]
> This report still does not claim the original frontend/debug source is fully recovered. It narrows the host layer so the future debug executable has defensible landing zones and the translated cleanup can follow clearer ownership.

## Snapshot

- Targeted shell/debug paths in this pass: `46`
- Paths with current archaeology bridges: `13` (`28.3%`)
- Paths already landing on current runtime contracts: `4` (`8.7%`)
- Paths still shell-only after this pass: `10` (`21.7%`)

## Local Hint Set Used

- `install_debug_menu_from_title`: The supplied local TCRF-derived note set says a preview-build Install debug menu was reachable from the title screen, which strengthens the case for title/menu-adjacent debug hosts.
- `debug_operation_tool`: The supplied local TCRF-derived note set says a Debug Operation Tool could stay open through gameplay/cutscene transitions, which makes the debug-host layer relevant for a future mixed gameplay/frontend sandbox.
- `inspire_preview_hosts`: The supplied local TCRF-derived note set explicitly names InspirePreview and InspirePreview2nd as launchable preview tools, which strongly supports treating those files as cutscene/subtitle preview hosts.
- `unused_trials_via_debug`: The supplied local TCRF-derived note set says unused trials could be reached through debug tools, which supports prioritizing menu/stage debug game modes as future UI sandbox hosts.

## Priority Groups

| Group | Priority | Paths | Bridged | Contract-backed |
|---|---|---:|---:|---:|
| Cutscene / Preview Hosts | `immediate` | `12` | `5` | `0` |
| Menu / Stage Debug Hosts | `immediate` | `3` | `0` | `0` |
| Stage Test / Validation Hosts | `high` | `9` | `0` | `0` |
| Pause / Help / Loading Dispatch | `high` | `4` | `4` | `4` |
| Town / Media-Room Dispatch Hosts | `high` | `4` | `4` | `0` |
| Application / World Shell | `medium` | `9` | `0` | `0` |
| Free Camera / Replay Hosts | `medium` | `5` | `0` | `0` |

## Group Notes

### Cutscene / Preview Hosts

These preview, movie, and sequence surfaces are the strongest current path toward a cutscene/subtitle-capable debug sandbox instead of a title-only selector.

Likely target systems:
- `subtitle_cutscene_presentation`

Representative paths:
- `Tool/InspirePreview/InspirePreview.cpp`
- `Tool/InspirePreview/InspirePreviewMenu.cpp`
- `Tool/InspirePreview/InspireObject.cpp`
- `Tool/InspirePreview2nd/InspirePreview2nd.cpp`
- `Tool/InspirePreview2nd/InspirePreview2ndMenu.cpp`
- `Tool/MotionCameraTool/MotionCameraTool.cpp`

### Menu / Stage Debug Hosts

These are the most obvious menu-level and stage-selection debug hosts for a future standalone UI browser executable.

Likely target systems:
- `title_menu`
- `loading_and_start`

Likely runtime contracts:
- `title_menu_reference.json`
- `loading_transition_reference.json`

Representative paths:
- `System/GameMode/GameModeMainMenu_Test.cpp`
- `System/GameMode/GameModeMenuSelectDebug.cpp`
- `System/GameMode/GameModeStageSelectDebug.cpp`

### Stage Test / Validation Hosts

These hosts look like the bridge between frontend/menu debug control and in-stage validation surfaces such as loading, save flow, screenshots, and gameplay HUD probes.

Likely target systems:
- `loading_and_start`
- `save_and_ending`
- `sonic_stage_hud`
- `werehog_stage_hud`

Likely runtime contracts:
- `loading_transition_reference.json`
- `autosave_toast_reference.json`

Representative paths:
- `System/GameMode/GameModeStageAchievementTest.cpp`
- `System/GameMode/GameModeStageEvilTest.cpp`
- `System/GameMode/GameModeStageForwardTest.cpp`
- `System/GameMode/GameModeStageInstallTest.cpp`
- `System/GameMode/GameModeStageLoadXML.cpp`
- `System/GameMode/GameModeStageMotionTest.cpp`

### Pause / Help / Loading Dispatch

These sequence-layer dispatchers are now concrete bridges into the current pause/help and loading/start runtime contracts.

Likely target systems:
- `pause_stack`
- `loading_and_start`

Likely runtime contracts:
- `pause_menu_reference.json`
- `loading_transition_reference.json`

Representative paths:
- `Sequence/Unit/SequenceUnitCallHelpWindow.cpp`
- `Sequence/Unit/SequenceUnitChangeStage.cpp`
- `Sequence/Unit/SequenceUnitSwapDisk.cpp`
- `Sequence/Utility/SequenceChangeStageUnit.cpp`

### Town / Media-Room Dispatch Hosts

These camera and sequence dispatch surfaces now form a tighter bridge into the town/media-room frontend family.

Likely target systems:
- `town_ui`

Representative paths:
- `Camera/Controller/TownShopCamera.cpp`
- `Camera/Controller/TownTalkCamera.cpp`
- `Sequence/Unit/SequenceUnitSendMediaRoomMessage.cpp`
- `Sequence/Unit/SequenceUnitSendTownMessage.cpp`

### Application / World Shell

These files look like app/world ownership shells rather than screen implementations, but they are the layer that will eventually need named translated ownership once the debug executable stops being purely menu-bound.

Representative paths:
- `System/Application.cpp`
- `System/ApplicationDocument.cpp`
- `System/ApplicationSetting.cpp`
- `System/Game.cpp`
- `System/GameDocument.cpp`
- `System/NextStagePreloadingManager.cpp`

### Free Camera / Replay Hosts

These are better treated as inspection and traversal hosts for the future debug sandbox than as ordinary frontend camera ownership.

Representative paths:
- `Tool/FreeCameraTool/FreeCameraTool.cpp`
- `Camera/Controller/FreeCamera.cpp`
- `Camera/Controller/GoalCamera.cpp`
- `Replay/Camera/ReplayFreeCamera.cpp`
- `Replay/Camera/ReplayRelativeCamera.cpp`

## Best Immediate Hosts For The Future Debug Executable

- `System/GameMode/GameModeMenuSelectDebug.cpp`: Best current menu-debug host for a future selector-driven frontend executable.
- `System/GameMode/GameModeStageSelectDebug.cpp`: Best current stage-debug host for jumping from frontend control into gameplay-facing validation.
- `Tool/InspirePreview/InspirePreview.cpp`: Strongest cutscene preview host named directly by the supplied local note set.
- `Tool/InspirePreview2nd/InspirePreview2nd.cpp`: Newer cutscene preview host for isolated prerendered sequence checks.
- `Tool/FreeCameraTool/FreeCameraTool.cpp`: Inspection host for a future debug sandbox where menu/UI overlays and world traversal coexist.
- `Tool/MotionCameraTool/MotionCameraTool.cpp`: Camera-preview host that complements cutscene/timeline validation.

## What This Changes

- The project now has a tighter answer for where the future debug executable should live first: menu/stage debug game modes plus cutscene-preview tools, not the entire system shell at once.
- The widened path seed is no longer just a count exercise; several shell-level files now have plausible ownership targets and, in some cases, current contract aliases already usable in the selector.
- The unresolved frontier is clearer: application/world shell ownership and the broader debug/editor surface still need deeper translated cleanup before they become named runtime screens.
