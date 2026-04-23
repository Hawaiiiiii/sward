# <img src="../docs/assets/branding/icon_debug.png" width="30" alt="SWARD debug icon"/> Code-to-Layout Correlation

Machine-readable inventory: `research_uiux/data/layout_code_correlation.json`

## Summary

- Layout files correlated: `13`
- Direct-correlation layouts: `9`
- Strong-correlation layouts: `3`
- Context-only layouts: `1`
- Unresolved layouts: `0`

> [!NOTE]
> `direct` means the readable code names the extracted layout or scene path explicitly.
> `strong` means the readable code wraps the same game subsystem/state without exposing the raw layout string.
> `contextual` means the readable code is adjacent support infrastructure rather than a direct asset-name seam.

## `ui_boss_gauge`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_broader_archives/BossCommon/ui_boss_gauge.yncp`
- Archive group: `BossCommon`
- Inferred role: `boss_hud_gauge`
- Correlation verdict: `direct`
- Scene cues: `gauge_1, gauge_2, gauge_bg, gauge_breakpoint`
- Animation cues: `Intro_Anim, total_size, Size_Anim, position`

### Readable Correlations

- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:373`
  - evidence line: `// ui_boss_gauge`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:374`
  - evidence line: `{ HashStr("ui_boss_gauge/gauge_bg"), { ALIGN_TOP_RIGHT | SCALE | SKIP_INSPIRE} },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path. Direct layout-path rules expose the boss gauge background, segments, and breakpoint elements by name.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:375`
  - evidence line: `{ HashStr("ui_boss_gauge/gauge_2"), { ALIGN_TOP_RIGHT | SCALE | SKIP_INSPIRE} },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:376`
  - evidence line: `{ HashStr("ui_boss_gauge/gauge_1"), { ALIGN_TOP_RIGHT | SCALE | SKIP_INSPIRE} },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.

## `ui_boss_name`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_broader_archives/BossCommon/ui_boss_name.yncp`
- Archive group: `BossCommon`
- Inferred role: `boss_hud_name`
- Correlation verdict: `direct`
- Scene cues: `name_ev, name_so`
- Animation cues: `01_Anim, 02_Anim, 03_Anim, 04_Anim, 05_Anim`

### Readable Correlations

- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:379`
  - evidence line: `// ui_boss_name`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:380`
  - evidence line: `{ HashStr("ui_boss_name/name_so/bg"), { UNSTRETCH_HORIZONTAL } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path. Direct layout-path rules expose the extracted boss-name layout's `name_so` branch and its horizontal unstretch handling.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:381`
  - evidence line: `{ HashStr("ui_boss_name/name_so/pale"), { UNSTRETCH_HORIZONTAL } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.

## `ui_help`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_extended_archives/SystemCommon/ui_help.yncp`
- Archive group: `SystemCommon`
- Inferred role: `help_overlay`
- Correlation verdict: `contextual`

### Readable Correlations

- `contextual` `UnleashedRecomp/ui/button_guide.cpp:298`
  - evidence line: `void ButtonGuide::Open(Button button)`
  - why it matters: No exact `ui_help` layout-name hit exists in readable code, but the help layouts sit closest to the generic prompt/guide systems that surface control hints.

## `ui_itemresult`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_extended_archives/SystemCommon/ui_itemresult.yncp`
- Archive group: `SystemCommon`
- Inferred role: `item_result_overlay`
- Correlation verdict: `direct`

### Readable Correlations

- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:408`
  - evidence line: `// ui_itemresult`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:409`
  - evidence line: `{ HashStr("ui_itemresult/footer/result_footer"), { ALIGN_BOTTOM } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path. Direct layout-path rules expose the footer prompt row and title treatment from the extracted item-result layout.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:410`
  - evidence line: `{ HashStr("ui_itemresult/main/iresult_title"), { ALIGN_TOP | OFFSET_SCALE_LEFT, 688.0f } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:411`
  - evidence line: `{ HashStr("ui_itemresult/main/iresult_title/title_bg/center"), { ALIGN_TOP | EXTEND_LEFT } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.

## `ui_loading`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_extended_archives/Loading/ui_loading.yncp`
- Archive group: `Loading`
- Inferred role: `loading_transition`
- Correlation verdict: `direct`
- Scene cues: `bg_1, bg_2, event_viewer, loadinfo, n_2_d, pda, pda_txt`
- Animation cues: `Intro_Anim, Outro_Anim, 360_evil, 360_robo, 360_sonic1, 360_sonic2, 360_sonic3, 360_super, extra, ps3_evil`

### Readable Correlations

- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:418`
  - evidence line: `// ui_loading`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:419`
  - evidence line: `{ HashStr("ui_loading/bg_1"), { STRETCH } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path. Direct layout-path rules expose the loading background, event-viewer black bars, and PDA frame branches from the extracted layout.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:420`
  - evidence line: `{ HashStr("ui_loading/bg_1/arrow"), { STRETCH | LOADING_ARROW } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:421`
  - evidence line: `{ HashStr("ui_loading/bg_2"), { STRETCH } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/resident_patches.cpp:54`
  - evidence line: `// Patch "ui_loading.yncp" to remove the medal swinging animation.`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path. Direct readable patch evidence references `ui_loading.yncp`, patches its animation data, and swaps controller-platform string variants inside the same loading project.
- `contextual` `UnleashedRecomp/ui/black_bar.cpp`
  - why it matters: The readable black-bar layer aligns with the loading layout's authored letterbox and black-bar scene families.

## `ui_pause`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_extended_archives/SystemCommonCore/ui_pause.yncp`
- Archive group: `SystemCommonCore`
- Inferred role: `pause_menu`
- Correlation verdict: `direct`
- Scene cues: `bg`
- Animation cues: `Intro_Anim`

### Readable Correlations

- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:487`
  - evidence line: `// ui_pause`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:488`
  - evidence line: `{ HashStr("ui_pause/bg"), { STRETCH } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path. Direct layout-path rules cover the pause background, footer, and header-title shell.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:489`
  - evidence line: `{ HashStr("ui_pause/footer/footer_A"), { ALIGN_BOTTOM } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:490`
  - evidence line: `{ HashStr("ui_pause/footer/footer_B"), { ALIGN_BOTTOM } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `strong` `UnleashedRecomp/patches/CHudPause_patches.cpp:118`
  - evidence line: `// SWA::CHudPause::Update`
  - why it matters: Wraps the original pause HUD update seam and injects options/achievement behavior without replacing the native pause state machine.
- `strong` `UnleashedRecomp/ui/options_menu.cpp:1795`
  - evidence line: `void OptionsMenu::Open(bool isPause, SWA::EMenuType pauseMenuType)`
  - why it matters: Implements the custom submenu branch opened from pause, with separate world-map and non-world-map pause behavior.
- `strong` `UnleashedRecompLib/config/SWA.toml:527`
  - evidence line: `# World Map Pause Menu`
  - why it matters: Declares the pause-menu injection hooks for world map, village, stage, hub, and misc pause variants.
- `contextual` `UnleashedRecomp/ui/imgui_utils.cpp:173`
  - evidence line: `void DrawPauseContainer(ImVec2 min, ImVec2 max, float alpha)`
  - why it matters: Implements the shared frame language reused by pause-adjacent custom UI.

### Generated Seams

- `sub_824AE690` via `UnleashedRecomp/patches/CHudPause_patches.cpp`
  - generated location: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.27.cpp:34794`
  - readable relationship: pause menu item helper used by patched options insertion
- `sub_824AFD28` via `UnleashedRecomp/patches/CHudPause_patches.cpp`
  - generated location: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.27.cpp:37962`
  - readable relationship: pause HUD helper called after closing custom pause options
- `sub_824B0930` via `UnleashedRecomp/patches/CHudPause_patches.cpp`
  - generated location: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.27.cpp:39777`
  - readable relationship: core CHudPause update seam wrapped by pause patches

## `ui_saveicon`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_extended_archives/AutoSave/ui_saveicon.yncp`
- Archive group: `AutoSave`
- Inferred role: `save_overlay`
- Correlation verdict: `strong`
- Scene cues: `icon`
- Animation cues: `Intro_Anim`

### Readable Correlations

- `strong` `UnleashedRecomp/patches/resident_patches.cpp:91`
  - evidence line: `// SWA::CSaveIcon::Update`
  - why it matters: Directly hooks the save-icon visibility/update seam that drives the extracted save icon overlay.

## `ui_general`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_extended_archives/SystemCommonCore/ui_general.yncp`
- Archive group: `SystemCommonCore`
- Inferred role: `shared_window_shell`
- Correlation verdict: `direct`
- Scene cues: `bg, footer, window, window_select`
- Animation cues: `Intro_Anim, Size_Anim, Scroll_Anim, Usual_Anim, Usual_Anim_1, Usual_Anim_12, Usual_Anim_2, Usual_Anim_3`

### Readable Correlations

- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:404`
  - evidence line: `// ui_general`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:405`
  - evidence line: `{ HashStr("ui_general/bg"), { STRETCH } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path. Direct layout-path rules prove the generic background/footer shell is treated as a reusable CSD package rather than screen-specific one-off geometry.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:406`
  - evidence line: `{ HashStr("ui_general/footer"), { ALIGN_BOTTOM } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `strong` `UnleashedRecomp/ui/imgui_utils.cpp:173`
  - evidence line: `void DrawPauseContainer(ImVec2 min, ImVec2 max, float alpha)`
  - why it matters: Implements a reusable pause-style shell that mirrors the asset-side `ui_general` grammar of dark background, framed window, and footer row.
- `contextual` `UnleashedRecomp/ui/message_window.cpp:191`
  - evidence line: `DrawPauseContainer(_min, _max, alpha);`
  - why it matters: Renders modal prompts inside the same shared shell language instead of hardcoding a separate message-box frame.

## `ui_end`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_extended_archives/StaffRoll/ui_end.yncp`
- Archive group: `StaffRoll`
- Inferred role: `staff_roll_or_ending`
- Correlation verdict: `strong`
- Scene cues: `ending`
- Animation cues: `Intro_Anim_1, Intro_Anim_2`

### Readable Correlations

- `strong` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:1656`
  - evidence line: `void EndingTextAllocMidAsmHook(PPCRegister& r3)`
  - why it matters: Readable ending-text hooks are the closest native seam to the extracted ending/staff-roll layout package.
- `strong` `UnleashedRecompLib/config/SWA.toml:1111`
  - evidence line: `name = "EndingTextAllocMidAsmHook"`
  - why it matters: Declares the ending/staff-roll hook sites that align with the extracted `ui_end` project.

## `ui_status`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_extended_archives/SystemCommonCore/ui_status.yncp`
- Archive group: `SystemCommonCore`
- Inferred role: `status_overlay`
- Correlation verdict: `direct`

### Readable Correlations

- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:721`
  - evidence line: `// ui_status`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:722`
  - evidence line: `{ HashStr("ui_status/footer/status_footer"), { ALIGN_BOTTOM } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path. Direct layout-path rules expose footer, title shell, progress bar, tag, and arrow-effect elements from the extracted status layout.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:723`
  - evidence line: `{ HashStr("ui_status/header/status_title"), { ALIGN_TOP | OFFSET_SCALE_LEFT, 617.0f } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:724`
  - evidence line: `{ HashStr("ui_status/header/status_title/title_bg/center"), { ALIGN_TOP | EXTEND_LEFT } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `contextual` `UnleashedRecomp/ui/imgui_utils.cpp:204`
  - evidence line: `void DrawPauseHeaderContainer(ImVec2 min, ImVec2 max, float alpha)`
  - why it matters: The readable UI layer reuses a title/header frame style that closely matches the extracted `ui_status` title-shell structure.

## `ui_mainmenu`

- Asset variants: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_frontend_archives/MainMenu/ui_mainmenu.xncp, C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_frontend_archives/MainMenu/ui_mainmenu.yncp`
- Archive group: `MainMenu`
- Inferred role: `title_menu`
- Correlation verdict: `strong`
- Scene cues: `mm_base, mm_bg_intro, mm_bg_usual, mm_contentsitem_idle, mm_contentsitem_intro, mm_contentsitem_move, mm_contentsitem_select, mm_contentsitem_text, mm_donut_idle, mm_donut_intro`
- Animation cues: `DefaultAnim, intro, move, sel1, sel2, sel3`

### Readable Correlations

- `strong` `UnleashedRecomp/patches/CTitleStateMenu_patches.cpp:51`
  - evidence line: `// SWA::CTitleStateMenu::Update`
  - why it matters: Wraps the original title-menu update seam and injects options/install logic over the native main-menu cursor flow.
- `contextual` `UnleashedRecomp/patches/CTitleStateIntro_patches.cpp:149`
  - evidence line: `// SWA::CTitleStateIntro::Update`
  - why it matters: Owns the modal and fade handoff immediately before the title menu becomes interactive.
- `contextual` `UnleashedRecomp/ui/fader.cpp:76`
  - evidence line: `void Fader::FadeOut(float duration, std::function<void()> endCallback, float endCallbackDelay)`
  - why it matters: Provides shared fade ownership used when title flow exits, restarts, or jumps into installer/update actions.
- `contextual` `UnleashedRecomp/ui/message_window.cpp:550`
  - evidence line: `bool MessageWindow::Open(std::string text, int* result, std::span<std::string> buttons, int defaultButtonIndex, int cancelButtonIndex)`
  - why it matters: Provides the prompt layer used by the title intro and title menu wrappers for quit/install/update flows.
- `contextual` `UnleashedRecompLib/config/SWA.toml:925`
  - evidence line: `# Title`
  - why it matters: Declares title-specific hook sites on the original title render path even though the layout name itself is not exposed in readable C++.

### Generated Seams

- `sub_822C55B0` via `UnleashedRecomp/patches/CTitleStateIntro_patches.cpp`
  - generated location: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.0.cpp:13225`
  - readable relationship: save validation or intro-gating seam wrapped by title intro patches
- `sub_82587E50` via `UnleashedRecomp/patches/CTitleStateIntro_patches.cpp`
  - generated location: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.40.cpp:41707`
  - readable relationship: core title intro update seam wrapped by modal/fader logic
- `sub_825882B8` via `UnleashedRecomp/patches/CTitleStateMenu_patches.cpp`
  - generated location: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.40.cpp:42336`
  - readable relationship: core title menu update seam wrapped by options/install logic

## `ui_worldmap`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_extended_archives/WorldMap/ui_worldmap.yncp`
- Archive group: `WorldMap`
- Inferred role: `world_map`
- Correlation verdict: `direct`
- Scene cues: `worldmap_background`
- Animation cues: `Intro_Anim, Usual_Anim`

### Readable Correlations

- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:758`
  - evidence line: `// ui_worldmap`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:759`
  - evidence line: `{ HashStr("ui_worldmap/contents/choices/cts_choices_bg"), { STRETCH } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:760`
  - evidence line: `{ HashStr("ui_worldmap/contents/info/bg/cts_info_bg"), { ALIGN_TOP_LEFT | WORLD_MAP } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path. Direct layout-path rules expose the extracted world-map info panes, header, and footer structures by name.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:761`
  - evidence line: `{ HashStr("ui_worldmap/contents/info/bg/info_bg_1"), { ALIGN_TOP_LEFT | WORLD_MAP } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `strong` `UnleashedRecomp/patches/input_patches.cpp:401`
  - evidence line: `// SWA::CWorldMapCamera::Update`
  - why it matters: Wraps world-map cursor/camera behavior, which is the interactive layer that drives the extracted world-map layout states.
- `strong` `UnleashedRecompLib/config/SWA.toml:815`
  - evidence line: `name = "WorldMapDeadzoneMidAsmHook"`
  - why it matters: Declares multiple world-map-specific hook sites that line up with the extracted info-pane and projection-sensitive layout content.
- `contextual` `UnleashedRecomp/apu/embedded_player.cpp:6`
  - evidence line: `#include <res/sounds/sys_worldmap_cursor.ogg.h>`
  - why it matters: Provides the world-map interaction sound hooks that likely accompany the extracted choice/info scene transitions.

### Generated Seams

- `sub_82486968` via `UnleashedRecomp/patches/input_patches.cpp`
  - generated location: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.25.cpp:12860`
  - readable relationship: input/world-map seam referenced by input patches
- `sub_8256C938` via `UnleashedRecomp/patches/input_patches.cpp`
  - generated location: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.39.cpp:24278`
  - readable relationship: input/world-map camera or cursor seam referenced by input patches

## `ui_worldmap_help`

- Asset path: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/ui_extended_archives/WorldMap/ui_worldmap_help.yncp`
- Archive group: `WorldMap`
- Inferred role: `world_map_help_overlay`
- Correlation verdict: `direct`

### Readable Correlations

- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:788`
  - evidence line: `// ui_worldmap_help`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:789`
  - evidence line: `{ HashStr("ui_worldmap_help/balloon/help_window/position/msg_bg_l"), { EXTEND_LEFT } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path. Direct layout-path rules expose the left/right expandable help-window background pieces from the world-map help layout.
- `direct` `UnleashedRecomp/patches/aspect_ratio_patches.cpp:790`
  - evidence line: `{ HashStr("ui_worldmap_help/balloon/help_window/position/msg_bg_r"), { EXTEND_RIGHT } },`
  - why it matters: Readable source references the extracted layout name or an exact layout/scene path.
- `contextual` `UnleashedRecomp/patches/input_patches.cpp:417`
  - evidence line: `auto pWorldMapCursor = (SWA::CWorldMapCursor*)g_memory.Translate(ctx.r3.u32);`
  - why it matters: World-map help overlays sit alongside the same world-map interaction loop that these readable input patches modify.

### Generated Seams

- `sub_82486968` via `UnleashedRecomp/patches/input_patches.cpp`
  - generated location: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.25.cpp:12860`
  - readable relationship: input/world-map seam referenced by input patches
- `sub_8256C938` via `UnleashedRecomp/patches/input_patches.cpp`
  - generated location: `C:/Users/DavidErikGarciaArena/Documents/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.39.cpp:24278`
  - readable relationship: input/world-map camera or cursor seam referenced by input patches
