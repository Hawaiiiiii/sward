<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Patch Hook Index

Machine-readable inventory: `research_uiux/data/patch_hooks.json`

## Scan Summary

- Patch files indexed: `21`

These are the most UI/UX-relevant hook layers in the readable patch code.

## `UnleashedRecomp/patches/CHudPause_patches.cpp`

- Script summary: pause HUD interception and pause-menu behavior overrides
- High-value original symbols:
  - `SWA::CHudPause`
  - `SWA::CHudPause::Update`
  - `SWA::eMenuType_*`
  - `AchievementMenu::Open`
  - `OptionsMenu::Open`
- High-value generated/PPC refs:
  - `sub_824AE690`
  - `sub_824B0930`
  - `sub_824AFD28`
- What it adds:
  - Injects a custom options item into pause flow
  - Opens achievements from pause via `Select`
  - Opens the custom options menu without breaking original pause transitions
  - Reopens/maintains button guide behavior when the pause HUD is active

## `UnleashedRecomp/patches/CTitleStateIntro_patches.cpp`

- Script summary: title intro hook layer and intro-state flow adjustments
- High-value original symbols:
  - `SWA::CTitleStateIntro::Update`
  - `MessageWindow::Open`
  - `Fader::FadeOut`
  - `UpdateChecker::check`
  - `AchievementManager::*`
- High-value generated/PPC refs:
  - `sub_822C55B0`
  - `sub_82587E50`
- What it adds:
  - Supervises title intro flow with custom modal messages
  - Replaces or suppresses storage prompt behavior
  - Handles corrupt save, corrupt achievements, update-available, and quit paths
  - Uses `Fader` callbacks for final exit actions

## `UnleashedRecomp/patches/CTitleStateMenu_patches.cpp`

- Script summary: title menu hook layer and menu-state flow adjustments
- High-value original symbols:
  - `SWA::CTitleStateMenu::Update`
  - `MessageWindow::Open`
  - `Fader::FadeOut`
  - `OptionsMenu::Open`
  - `OptionsMenu::CanClose`
  - `App::Restart`
- High-value generated/PPC refs:
  - `sub_825882B8`
- What it adds:
  - Injects options flow into title menu cursor logic
  - Adds missing-DLC / install modal flow
  - Forces restart path when settings require it
  - Removes or suppresses original menu entries in some cases

## `UnleashedRecomp/patches/CGameModeStageTitle_patches.cpp`

- Script summary: stage title integration hooks tied to original game mode
- High-value original symbols:
  - `SWA::CGameModeStageTitle::Update`
- High-value generated/PPC refs:
  - `sub_825518B8`
- What it appears to do:
  - Hooks the original stage title update seam
  - Readable patch evidence is thinner here than for pause/title/options
  - Full behavior mapping is blocked until generated PPC exists

## `UnleashedRecomp/patches/aspect_ratio_patches.cpp`

- Script summary: UI scaling, safe-area, and ultrawide/aspect-ratio compensation
- High-value original symbols:
  - `SWA::CGameDocument::ComputeScreenPosition`
  - `SWA::CTitleStateWorldMap::Update`
  - `Config::UIAlignmentMode`
  - `EUIAlignmentMode::Edge`
  - `CEvilHudGuide`
- High-value generated/PPC refs include many `sub_XXXXXXXX` wrappers, notably:
  - `sub_825E2E60`
  - `sub_82E169B8`
  - `sub_8250FC70`
  - `sub_8258B558`
  - `sub_830BB3D0`
  - `sub_830BC640`
  - `sub_830C6A00`
- What it adds:
  - 1280x720 reference-space to actual viewport remapping
  - Safe-area and edge/centre alignment logic
  - Fixes for fade textures, letterboxing, loading UI, result UI, pause UI, mission UI, and world map/UI nodes
  - Special-case corrections for Werehog/Evil HUD guide behavior

## `UnleashedRecomp/patches/input_patches.cpp`

- High-value original symbols:
  - `hid::IsInputAllowed`
  - `hid::g_inputDeviceExplicit`
  - `SWA::SPadState`
  - `SWA::CWorldMapCamera`
  - `SWA::CWorldMapCursor`
- High-value generated/PPC refs:
  - `sub_82486968`
  - `sub_8256C938`
- UI/UX relevance:
  - D-pad movement policy
  - World map cursor and camera behavior
  - Explicit input-device handling feeding UI icon selection or control semantics

## Cross-Cutting Hook Patterns

- Hook wrappers frequently guard original updates instead of replacing entire systems.
- Modal UI systems are layered around original title/pause states rather than implemented as separate standalone loops.
- `sub_XXXXXXXX` references are the practical bridge points between readable patch code and any future generated PPC mapping.
- The patch layer treats aspect ratio, input, and button guides as platform/runtime concerns that cut across multiple original game states.

## Best Patch Study Order

1. `CHudPause_patches.cpp`
2. `CTitleStateIntro_patches.cpp`
3. `CTitleStateMenu_patches.cpp`
4. `aspect_ratio_patches.cpp`
5. `input_patches.cpp`
6. `CGameModeStageTitle_patches.cpp`
