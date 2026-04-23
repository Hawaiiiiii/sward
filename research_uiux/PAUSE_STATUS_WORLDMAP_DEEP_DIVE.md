# <img src="../docs/assets/branding/icon_sward.png" width="30" alt="SWARD icon"/> Pause / Status / WorldMap Deep Dive

This report narrows the workspace onto three closely related UI families that matter most for template-grade UI/UX reuse:

- pause menu flow
- status overlay flow
- world map flow

It combines four evidence layers:

1. extracted layout semantics
2. direct layout-path references in readable patch code
3. readable host-state/input/audio code
4. generated PPC seam mapping where the readable patch layer already exposes a concrete bridge

> [!NOTE]
> The evidence strength is not identical across these three systems.
>
> - `pause` and `world map` now have both asset-side and readable host-state evidence.
> - `status` is still mostly asset-side plus aspect-ratio/node-remapping evidence.

## Pause

### Evidence Stack

Asset-side packages:

- `SystemCommonCore/ui_general.yncp`
- `SystemCommonCore/ui_pause.yncp`

Readable host/control files:

- `UnleashedRecomp/patches/CHudPause_patches.cpp`
- `UnleashedRecomp/ui/options_menu.cpp`
- `UnleashedRecomp/ui/imgui_utils.cpp`
- `UnleashedRecomp/apu/embedded_player.cpp`
- `UnleashedRecompLib/config/SWA.toml`

Generated seams:

- `sub_824AE690`
- `sub_824AFD28`
- `sub_824B0930`

Direct asset-name runtime evidence:

- `ui_general/bg`
- `ui_general/footer`
- `ui_pause/bg`
- `ui_pause/footer/footer_A`
- `ui_pause/footer/footer_B`
- `ui_pause/header/status_title`

### What The Layouts Say

`ui_general.yncp` is the reusable shell:

- scenes: `bg`, `window`, `window_select`, `footer`
- animation families: `Intro_Anim`, `Scroll_Anim`, `Size_Anim`, `Usual_Anim`

`ui_pause.yncp` is the pause specialization:

- semantic notes already established it as a larger `29`-scene package built on the same shell language
- direct runtime path rules expose the pause-specific background, dual footer branches, and title/header shell

Practical read:

- the game authors a generic framed-window grammar first
- pause then composes its own screen-specific header/footer/title package on top of that shell

### Readable Control Model

`CHudPause_patches.cpp` confirms that the original pause HUD remains authoritative:

- the patch wraps `SWA::CHudPause::Update`
- it does not replace pause with a custom standalone loop
- it injects behavior at menu-item and submenu seams

Confirmed pause branches:

- native pause idle
- injected options branch
- injected achievements branch
- return/hide/quit branch depending on `SWA::eMenuType_*`

`options_menu.cpp` confirms pause-context specialization:

- `OptionsMenu::Open(bool isPause, SWA::EMenuType pauseMenuType)`
- `g_isStage = isPause && pauseMenuType != SWA::eMenuType_WorldMap`
- world-map pause and non-world-map pause do not use identical motion/availability policy

### Likely Pause State Machine

1. `PauseOpenIntro`
   Asset-side intro timeline begins.
2. `PauseIdle`
   Native pause HUD is visible and owns cursor context.
3. `PauseCursorMove`
   Footer/header shell remains stable while selection moves.
4. `PauseAcceptNative`
   Original action/transition types fire for native items.
5. `PauseOpenOptionsInjected`
   Patched option item redirects into `OptionsMenu::Open(true, pHudPause->m_Menu)`.
6. `PauseOptionsVisible`
   Original host remains present, but custom menu takes interaction ownership.
7. `PauseOpenAchievementsInjected`
   `Select` opens the achievements browser and pause transitions into submenu ownership.
8. `PauseAchievementVisible`
   Achievements overlay/menu blocks native pause progression until closed.
9. `PauseReturn`
   `sub_824AFD28` and related helpers restore the original pause transition cluster.

### Animation, Input, and Audio Patterns

Animation/state vocabulary:

- `Intro_Anim`
- `Scroll_Anim`
- `Size_Anim`
- `Usual_Anim`

Readable input/lockout cues:

- achievements branch uses intro/outro timing thresholds before restoring control
- options branch requires `OptionsMenu::CanClose()` before `B` closes it
- button guide visibility is tied to whether pause is shown, not in submenu, and not transitioning

Audio cues from the embedded player:

- `sys_actstg_pausecursor`
- `sys_actstg_pausedecide`
- `sys_actstg_pausecansel`
- `sys_actstg_pausewinopen`
- `sys_actstg_pausewinclose`

Transferable takeaway:

- keep the original host state alive
- inject submenu branches at cursor/action seams
- let shell animations remain asset-authored while code only arbitrates ownership and lockouts

## Status

### Evidence Stack

Asset-side package:

- `SystemCommonCore/ui_status.yncp`

Readable runtime evidence:

- `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
- `UnleashedRecomp/ui/imgui_utils.cpp`

Direct asset-name runtime evidence:

- `ui_status/footer/status_footer`
- `ui_status/header/status_title`
- `ui_status/logo/logo/bg_position/c_1`
- `ui_status/logo/logo/bg_position/c_2`
- `ui_status/main/arrow_effect/a_efc_1`
- `ui_status/main/progless/bg/prgs_bg_1`
- `ui_status/main/progless/prgs/prgs_bar_1`
- `ui_status/main/tag/bg/tag_bg_1`
- `ui_status/main/tag/txt/tag_txt_1`
- `ui_status/window/bg`

### What The Layout Says

From the semantic pass:

- `42` scenes
- `15` textures
- key animation families:
  - `Intro_so_Anim`
  - `Intro_ev_Anim`
  - `Switch_Anim`
  - `select_so_Anim`
  - `select_ev_Anim`

Practical read:

- status is not a single flat overlay
- it is composed of footer, title shell, logo lane, progress lane, tag lane, arrow effect, and window background
- variant naming strongly suggests at least two authored presentation branches: `so` and `ev`

### Likely Status State Machine

1. `StatusHidden`
2. `StatusIntroSO` or `StatusIntroEV`
   Variant-specific intro branch.
3. `StatusIdle`
   Stable title/logo/progress/tag presentation.
4. `StatusSelectSO` or `StatusSelectEV`
   Highlighted or focused branch for whichever variant is active.
5. `StatusSwitch`
   `Switch_Anim` transitions between status presentations or pages.
6. `StatusExit`
   Exit timing is less explicit in readable code than in pause/loading, but the layout naming implies staged transitions rather than instant hide.

### What Readable Code Actually Proves

What is strong:

- node-level runtime handling of title/footer/logo/progress/tag elements is explicit
- the title shell uses the same left-extend/right-corner treatment seen in other authored UI packages
- the readable helper layer still reuses a similar pause-header framing language

What is still weak:

- no dedicated `CStatus...` readable patch wrapper has been surfaced yet
- no direct generated seam has been mapped for `ui_status` specifically
- timing beyond asset animation names is still more inferred than pause/world-map timing

Transferable takeaway:

- author the status surface as a set of named lanes, not one texture stack
- keep mode/form variants in animation naming rather than burying them entirely in code
- separate shell/title treatment from the changing data lanes such as progress/tag/logo

## World Map

### Evidence Stack

Asset-side packages:

- `WorldMap/ui_worldmap.yncp`
- `WorldMap/ui_worldmap_help.yncp`

Readable host/control files:

- `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
- `UnleashedRecomp/patches/input_patches.cpp`
- `UnleashedRecomp/apu/embedded_player.cpp`
- `UnleashedRecompLib/config/SWA.toml`

Generated seams:

- `sub_82486968`
- `sub_8256C938`

Direct asset-name runtime evidence:

- `ui_worldmap/contents/choices/cts_choices_bg`
- `ui_worldmap/contents/info/bg/cts_info_bg`
- `ui_worldmap/contents/info/bg/info_bg_1`
- `ui_worldmap/contents/info/img/info_img_1`
- `ui_worldmap/footer/worldmap_footer_bg`
- `ui_worldmap/header/worldmap_header_bg`
- `ui_worldmap_help/balloon/help_window/position/msg_bg_l`
- `ui_worldmap_help/balloon/help_window/position/msg_bg_r`

### What The Layouts Say

From the semantic pass:

- `ui_worldmap.yncp`: `35` scenes, `37` textures
- `ui_worldmap_help.yncp`: `3` scenes, `5` textures

Key scene families:

- `worldmap_background`
- `info_bg_1`
- `cts_info_bg`
- `info_img_1`

Key animation families:

- `Intro_Anim`
- `Usual_Anim`
- `Intro_Anim_rev`
- `Switch_Anim`
- `Switch_Anim_rev`

Practical read:

- the world map is a layered information surface
- info panes and header/footer bands are authored as named CSD structures
- help is a separate sidecar layout, not embedded inline into the main world-map project
- reversible animations strongly imply focus/unfocus or open/close choreography

### Readable Control Model

`input_patches.cpp` proves the world map has a dedicated interaction loop:

- custom cursor velocity, smoothing, damping, and flick acceleration
- D-pad participation in cursor/camera motion
- explicit camera pitch ceiling behavior
- explicit cursor-moving flag management

`aspect_ratio_patches.cpp` proves runtime node awareness:

- world-map info panel positions are adjusted inside `SWA::CTitleStateWorldMap::Update`
- many world-map nodes are individually remapped with `WORLD_MAP`, `REPEAT_LEFT`, and header/footer alignment rules

`SWA.toml` proves dedicated hook coverage:

- `WorldMapDeadzoneMidAsmHook`
- `WorldMapMagnetismMidAsmHook`
- `WorldMapHidSupportXMidAsmHook`
- `WorldMapHidSupportYMidAsmHook`
- `WorldMapInfoMidAsmHook`
- `WorldMapProjectionMidAsmHook`

### Likely World Map State Machine

1. `WorldMapEnterIntro`
   `Intro_Anim` establishes the header/footer/info surface.
2. `WorldMapUsual`
   Stable background plus current info panel and cursor context.
3. `WorldMapCursorMove`
   Cursor and/or camera movement active; information focus can shift.
4. `WorldMapSwitch`
   `Switch_Anim` changes active info branch, choice panel, or focus target.
5. `WorldMapHelpOpen`
   Sidecar help balloon is shown using `ui_worldmap_help`.
6. `WorldMapHelpClose`
   Reversible or paired transition back to the main map shell.
7. `WorldMapExitOrFinalDecide`
   Final decision or exit branch likely uses the same authored shell plus dedicated audio/UI feedback.

### Animation, Input, and Audio Patterns

Animation/state vocabulary:

- `Intro_Anim`
- `Usual_Anim`
- `Intro_Anim_rev`
- `Switch_Anim`
- `Switch_Anim_rev`

Readable input patterns:

- world-map input is not just a binary cursor move; it has damping, smoothing, threshold, and flick logic
- camera and cursor movement are coupled but not identical
- help overlays likely sit on top of this interaction layer, not inside it

Audio cues from the embedded player:

- `sys_worldmap_cursor`
- `sys_worldmap_finaldecide`

Transferable takeaway:

- treat the world map as a stateful information canvas with its own motion language
- keep camera/cursor math separate from info/help presentation logic
- use reversible animation naming when overlays can open and close over the same host surface

## Comparative Patterns

### Shared Shell Language

Pause and status both expose a title/footer/window grammar:

- title shell with a stretchable or extendable center segment
- footer prompt row
- darkened/stretched background plane

World map shares the same authored-header/authored-footer philosophy, but distributes its content more horizontally through info lanes and help sidecars.

### Original Host Preservation

The strongest reusable engineering pattern in all three systems is this:

- original game state remains the host
- readable patch code intervenes at transition/input/render seams
- authored CSD layouts still own most of the visual grammar

That is the correct template for extending legacy UI without snapping original flow.

### Evidence Confidence

Most certain:

- pause ownership and submenu intervention
- loading/world-map direct asset-name runtime mapping
- boss/status/world-map node naming from aspect-ratio remapping rules

Moderately certain:

- main-menu/state host mapping
- world-map help open/close choreography
- ending/staff-roll hook alignment

Still weaker:

- exact status update ownership in readable handwritten code
- full scene enumeration for some nested `.yncp` projects through the current parser path

## Best Next Commands

Focused follow-up that stays within this phase's scope:

```powershell
python research_uiux/tools/correlate_code_layouts.py --repo-root .
rg -n "ui_pause|ui_status|ui_worldmap|ui_worldmap_help" UnleashedRecomp/patches/aspect_ratio_patches.cpp
rg -n "CHudPause|WorldMap|Status|Pause|worldmap" UnleashedRecomp UnleashedRecompLib/config/SWA.toml
```

If going deeper after this report:

- use the generated seams for pause/world-map to inspect nearby original-state helpers in `ppc_recomp.27.cpp`, `ppc_recomp.25.cpp`, and `ppc_recomp.39.cpp`
- expand parser coverage for nested scene-node projects so `ui_status.yncp` and `ui_worldmap_help.yncp` expose a fuller scene list directly in the semantic JSON
