# Real Screen Source Inventory

Focused on real interactive UI/UX surfaces. Pre-rendered SFD/cutscene-only presentation is intentionally out of scope for this inventory.

## Summary

| Screen family | Layout projects | Primary native/source focus |
| --- | --- | --- |
| Title / Main Menu | `ui_mainmenu`, `ui_title` | `UnleashedRecomp/patches/CTitleStateIntro_patches.cpp`, `UnleashedRecomp/patches/CTitleStateMenu_patches.cpp`, `UnleashedRecomp/ui/fader.cpp`, `UnleashedRecomp/ui/message_window.cpp` |
| Loading / Start / Clear | `ui_loading`, `ui_start` | `UnleashedRecomp/patches/resident_patches.cpp`, `UnleashedRecomp/patches/aspect_ratio_patches.cpp`, `UnleashedRecomp/ui/black_bar.cpp`, `UnleashedRecomp/locale/config_locale.cpp` |
| Hub / Town UI | `ui_balloon`, `ui_shop`, `ui_townscreen`, `ui_mediaroom` | `UnleashedRecomp/patches/aspect_ratio_patches.cpp`, `UnleashedRecomp/install/hashes/game.cpp`, `UnleashedRecomp/locale/config_locale.cpp` |
| Day / Night Gameplay HUD | `ui_playscreen`, `ui_lcursor`, `ui_itembox` | `UnleashedRecomp/patches/CHudSonicStage_patches.cpp`, `UnleashedRecomp/patches/CGameModeStage_patches.cpp`, `UnleashedRecomp/app.h` |
| Pause / Help | `ui_pause`, `ui_general`, `ui_help` | `UnleashedRecomp/patches/CHudPause_patches.cpp`, `UnleashedRecomp/patches/aspect_ratio_patches.cpp`, `UnleashedRecomp/ui/options_menu.cpp`, `UnleashedRecomp/ui/imgui_utils.cpp` |
| Status / Skill Upgrade | `ui_status` | `UnleashedRecomp/patches/aspect_ratio_patches.cpp`, `UnleashedRecomp/ui/imgui_utils.cpp` |
| World Map | `ui_worldmap`, `ui_worldmap_help` | `UnleashedRecomp/patches/input_patches.cpp`, `UnleashedRecomp/patches/aspect_ratio_patches.cpp`, `UnleashedRecompLib/config/SWA.toml` |
| Results | `ui_result`, `ui_result_ex` | `UnleashedRecomp/patches/aspect_ratio_patches.cpp`, `UnleashedRecomp/install/hashes/game.cpp` |
| Item Result | `ui_itemresult` | `UnleashedRecomp/patches/aspect_ratio_patches.cpp` |
| Tornado / EX Stage HUD | `ui_exstage`, `ui_prov_playscreen`, `ui_qte` | `UnleashedRecomp/patches/aspect_ratio_patches.cpp`, `UnleashedRecomp/patches/fps_patches.cpp`, `UnleashedRecomp/patches/object_patches.cpp`, `UnleashedRecomp/patches/misc_patches.cpp` |

## Title / Main Menu

Recover the interactive title/menu flow, not pre-rendered SFDs.

- System id: `title_menu`
- Source focus:
  - `UnleashedRecomp/patches/CTitleStateIntro_patches.cpp`
  - `UnleashedRecomp/patches/CTitleStateMenu_patches.cpp`
  - `UnleashedRecomp/ui/fader.cpp`
  - `UnleashedRecomp/ui/message_window.cpp`
  - `UnleashedRecompLib/config/SWA.toml`
- Generated PPC/native-function references:
  - `sub_822C55B0` -> `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.0.cpp:13225` (save validation or intro-gating seam wrapped by title intro patches)
  - `sub_82587E50` -> `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.40.cpp:41707` (core title intro update seam wrapped by modal/fader logic)
  - `sub_825882B8` -> `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.40.cpp:42336` (core title menu update seam wrapped by options/install logic)
- Layouts:
  - `ui_mainmenu`: title_menu
    - file: `extracted_assets/ui_frontend_archives/MainMenu/ui_mainmenu.xncp`
    - file: `extracted_assets/ui_frontend_archives/MainMenu/ui_mainmenu.yncp`
    - scene cues: `mm_base`, `mm_bg_intro`, `mm_bg_usual`, `mm_contentsitem_idle`, `mm_contentsitem_intro`, `mm_contentsitem_move`, `mm_contentsitem_select`, `mm_contentsitem_text`
    - animation cues: `DefaultAnim`, `intro`, `move`, `sel1`, `sel2`, `sel3`
  - `ui_title`: role pending

## Loading / Start / Clear

Recover the authored transition screens and stage start/clear overlays.

- System id: `loading_and_start`
- Source focus:
  - `UnleashedRecomp/patches/resident_patches.cpp`
  - `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - `UnleashedRecomp/ui/black_bar.cpp`
  - `UnleashedRecomp/locale/config_locale.cpp`
- Layouts:
  - `ui_loading`: loading_transition
    - file: `extracted_assets/ui_extended_archives/Loading/ui_loading.yncp`
    - scene cues: `bg_1`, `bg_2`, `event_viewer`, `loadinfo`, `n_2_d`, `pda`, `pda_txt`
    - animation cues: `Intro_Anim`, `Outro_Anim`, `360_evil`, `360_robo`, `360_sonic1`, `360_sonic2`, `360_sonic3`, `360_super`
  - `ui_start`: start_clear_prompt
    - file: `extracted_assets/phase16_support_archives/ActionCommon/ui_start.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/ActionCommon/ui_start.yncp`
    - scene cues: `Clear`, `Failed`, `Game_over`, `Start`
    - animation cues: `Intro_Anim`, `Intro_Anim_act`

## Hub / Town UI

Recover hub-world overlays, dialog balloons, shop, and town status surfaces.

- System id: `town_ui`
- Source focus:
  - `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - `UnleashedRecomp/install/hashes/game.cpp`
  - `UnleashedRecomp/locale/config_locale.cpp`
- Layouts:
  - `ui_balloon`: town_dialog_balloon
    - file: `extracted_assets/phase23_crossref_archives/Town_Common/ui_balloon.yncp`
    - scene cues: `balloon_nomal`, `balloon_shout`, `balloon_think`, `balloon_txt_area`, `balloon_sonic`, `balloon_sonic_select`, `balloon_nametag`, `balloon_nametag_sonic`
    - animation cues: `Intro_Anim_Rarrow`, `Intro_Anim_Larrow`, `Usual_Anim_Rarrow`, `Usual_Anim_Larrow`, `Size_Anim`, `Usual_Anim`, `Scroll_Anim`, `Progress_Anim`
  - `ui_shop`: town_shop_menu
    - file: `extracted_assets/phase23_crossref_archives/Town_Common/ui_shop.yncp`
    - scene cues: `bg_1`, `bg_2`, `bg_3`, `bg_4`, `select_1`, `icon_2`, `icon_1`, `icon_3`
    - animation cues: `Intro_Anim`, `Intro_Anim_M`, `Scroll_Anim`, `Scroll_Anim_M`, `Usual_Anim`, `Usual_so_Anim`, `Usual_ev_Anim`, `DefaultAnim`
  - `ui_townscreen`: town_overlay
    - file: `extracted_assets/phase23_crossref_archives/Town_Common/ui_townscreen.yncp`
    - scene cues: `cam`, `footer`, `info`, `time`, `time_effect`
    - animation cues: `Size_Anim`, `Effect_Anim`, `Usual_12_Anim`, `Usual_1_Anim`, `Usual_2_Anim`, `Usual_3_Anim`, `Usual_ev_Anim`, `Usual_so_Anim`
  - `ui_mediaroom`: mediaroom_menu
    - file: `extracted_assets/phase23_crossref_archives/Town_Labo_Common/ui_mediaroom.yncp`
    - scene cues: `footer`, `header`, `window`, `book_btn`, `book`, `thumbnail_1`, `thumbnail_2`, `detail`
    - animation cues: `Intro_Anim`, `DefaultAnim`, `Switch_Anim`, `Outro_Anim`, `Usual_Anim`, `Scroll_Anim`, `Scroll_Anim_2`, `Switch_Anim_1`

## Day / Night Gameplay HUD

Recover player-control HUD behavior for Sonic and Werehog/Evil Sonic stage play.

- System id: `sonic_stage_hud`
- Source focus:
  - `UnleashedRecomp/patches/CHudSonicStage_patches.cpp`
  - `UnleashedRecomp/patches/CGameModeStage_patches.cpp`
  - `UnleashedRecomp/app.h`
- Layouts:
  - `ui_playscreen`: role pending
    - file: `extracted_assets/phase135_ui_playscreen_probe/Sonic/ui_playscreen.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/SuperSonic/ui_playscreen.yncp`
  - `ui_lcursor`: role pending
    - file: `extracted_assets/phase135_ui_playscreen_probe/Sonic/ui_lcursor.yncp`
  - `ui_itembox`: role pending
    - file: `extracted_assets/phase135_ui_playscreen_probe/SonicActionCommon/ui_itembox.yncp`

## Pause / Help

Recover pause-stack presentation, footer prompts, help pages, and menu state.

- System id: `pause_stack`
- Source focus:
  - `UnleashedRecomp/patches/CHudPause_patches.cpp`
  - `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - `UnleashedRecomp/ui/options_menu.cpp`
  - `UnleashedRecomp/ui/imgui_utils.cpp`
  - `UnleashedRecompLib/config/SWA.toml`
  - `UnleashedRecomp/ui/message_window.cpp`
  - `UnleashedRecomp/ui/button_guide.cpp`
- Generated PPC/native-function references:
  - `sub_824AE690` -> `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.27.cpp:34794` (pause menu item helper used by patched options insertion)
  - `sub_824AFD28` -> `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.27.cpp:37962` (pause HUD helper called after closing custom pause options)
  - `sub_824B0930` -> `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.27.cpp:39777` (core CHudPause update seam wrapped by pause patches)
- Layouts:
  - `ui_pause`: pause_menu
    - file: `extracted_assets/ui_extended_archives/SystemCommonCore/ui_pause.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/SystemCommonCore/ui_pause.yncp`
    - scene cues: `bg`, `bg_1`, `bg_1_select`, `bg_2`, `text_area`, `skill_select`, `arrow`, `skill_scroll_bar_bg`
    - animation cues: `Intro_Anim`, `Size_Anim`, `Scroll_Anim`, `Usual_Anim`, `DefaultAnim`, `Intro_2_Anim`, `Intro_3_Anim`, `stick`
  - `ui_general`: shared_window_shell
    - file: `extracted_assets/ui_extended_archives/SystemCommonCore/ui_general.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/SystemCommonCore/ui_general.yncp`
    - scene cues: `bg`, `footer`, `window`, `window_select`
    - animation cues: `Intro_Anim`, `Size_Anim`, `Scroll_Anim`, `Usual_Anim`, `Usual_Anim_1`, `Usual_Anim_12`, `Usual_Anim_2`, `Usual_Anim_3`
  - `ui_help`: help_overlay
    - file: `extracted_assets/ui_extended_archives/SystemCommon/ui_help.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/SystemCommon/ui_help.yncp`
    - scene cues: `help_window`, `help_text_area`, `help_nametag`, `help_chara_1`, `help_chara_2`, `help_chara_3`
    - animation cues: `Intro_Anim`, `Usual_Anim`, `Intro_chip_Anim`, `Intro_tails_Anim`, `Intro_sonic_Anim`

## Status / Skill Upgrade

Recover status and level-up overlay behavior for Sonic/Werehog progression.

- System id: `status_overlay`
- Source focus:
  - `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - `UnleashedRecomp/ui/imgui_utils.cpp`
- Layouts:
  - `ui_status`: status_overlay
    - file: `extracted_assets/ui_extended_archives/SystemCommonCore/ui_status.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/SystemCommonCore/ui_status.yncp`
    - scene cues: `logo`, `a_efc_1`, `a_efc_2`, `a_efc_3`, `a_efc_4`, `a_efc_5`, `a_efc_6`, `prgs_bg_1`
    - animation cues: `Intro_so_Anim`, `Intro_ev_Anim`, `Switch_Anim`, `select_so_Anim`, `select_ev_Anim`, `Usual_ev_Anim`, `Usual_so_Anim`, `Usual_Anim`

## World Map

Recover map select/help screens and cursor/info-panel state.

- System id: `world_map_stack`
- Source focus:
  - `UnleashedRecomp/patches/input_patches.cpp`
  - `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - `UnleashedRecompLib/config/SWA.toml`
- Generated PPC/native-function references:
  - `sub_82486968` -> `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.25.cpp:12860` (input/world-map seam referenced by input patches)
  - `sub_8256C938` -> `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.39.cpp:24278` (input/world-map camera or cursor seam referenced by input patches)
  - `sub_82486968` -> `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.25.cpp:12860` (input/world-map seam referenced by input patches)
  - `sub_8256C938` -> `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.39.cpp:24278` (input/world-map camera or cursor seam referenced by input patches)
- Layouts:
  - `ui_worldmap`: world_map
    - file: `extracted_assets/ui_extended_archives/WorldMap/ui_worldmap.yncp`
    - scene cues: `worldmap_background`, `info_bg_1`, `cts_info_bg`, `info_img_1`, `info_img_2`, `info_img_3`, `info_img_4`, `cts_cursor_effect`
    - animation cues: `Intro_Anim`, `Usual_Anim`, `Intro_Anim_rev`, `Switch_Anim`, `Switch_Anim_rev`, `Select_Anim`, `Intro_1_Anim`, `Intro_Anim_2`
  - `ui_worldmap_help`: world_map_help_overlay
    - file: `extracted_assets/ui_extended_archives/WorldMap/ui_worldmap_help.yncp`
    - scene cues: `help_window`, `help_text_area`, `help_chara_2`
    - animation cues: `Intro_Anim`, `Usual_Anim`, `Switch_Anim`

## Results

Recover mission result screens, rankings, numbers, records, and footer flow.

- System id: `mission_result_family`
- Source focus:
  - `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - `UnleashedRecomp/install/hashes/game.cpp`
- Layouts:
  - `ui_result`: mission_result_overlay
    - file: `extracted_assets/phase16_support_archives/ActionCommon/ui_result.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/ActionCommon/ui_result.yncp`
    - scene cues: `result_title`, `result_num_1`, `result_num_2`, `result_num_3`, `result_num_4`, `result_num_5`, `result_num_6`, `result_newR`
    - animation cues: `Intro_so_Anim`, `Intro_ev_Anim`, `Usula_Anim`, `Usual_Anim`, `Intro_Anim`
  - `ui_result_ex`: exstage_result_overlay
    - file: `extracted_assets/phase16_support_archives/ExStageTails_Common/ui_result_ex.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/ExStageTails_Common/ui_result_ex.yncp`
    - scene cues: `result_title`, `result_num_1`, `result_num`, `result_tag_1`, `result_tag`, `result_newR`, `result_newR_position`, `result_rank`
    - animation cues: `Intro_so_Anim`, `Intro_Anim`, `Usula_Anim`, `Usual_Anim`

## Item Result

Recover item/acquisition result overlays that differ from full mission results.

- System id: `item_result`
- Source focus:
  - `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
- Layouts:
  - `ui_itemresult`: item_result_overlay
    - file: `extracted_assets/ui_extended_archives/SystemCommon/ui_itemresult.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/SystemCommon/ui_itemresult.yncp`
    - scene cues: `iresult_title`, `window`, `contents`, `result_footer`
    - animation cues: `Intro_so_Anim`, `Intro_ev_Anim`, `Outro_so_Anim`, `Outro_ev_Anim`, `Intro_so_Anim_old`, `Intro_ev_Anim_old`, `Intro_so_etf_Anim`, `Intro_ev_etf_Anim`

## Tornado / EX Stage HUD

Recover EX/Tails/Tornado HUD and QTE-adjacent authored overlays.

- System id: `tornado_defense`
- Source focus:
  - `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - `UnleashedRecomp/patches/fps_patches.cpp`
  - `UnleashedRecomp/patches/object_patches.cpp`
  - `UnleashedRecomp/patches/misc_patches.cpp`
- Layouts:
  - `ui_exstage`: exstage_hud
    - file: `extracted_assets/phase16_support_archives/ExStageTails_Common/ui_exstage.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/ExStageTails_Common/ui_exstage.yncp`
    - scene cues: `L_gauge`, `L_gauge_effect`, `L_gauge_effect_2`, `R_gauge`, `R_gauge_effect`, `R_gauge_effect_2`, `hit_counter_bg`, `hit_counter_num`
    - animation cues: `Intro_Anim`, `Size_Anim`, `Usual_Anim`, `Intro_Anim_en`, `Intro_Anim_fr`, `Intro_Anim_ge`, `Intro_Anim_it`, `Intro_Anim_sp`
  - `ui_prov_playscreen`: tornado_defense_hud
    - file: `extracted_assets/phase16_support_archives/ExStageTails_Common/ui_prov_playscreen.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/ExStageTails_Common/ui_prov_playscreen.yncp`
    - scene cues: `bg`, `info_1`, `info_2`, `ring_get_effect`, `so_ringenagy_gauge`, `so_speed_gauge`
    - animation cues: `Size_Anim`, `DefaultAnim`, `Count_Anim`, `Score_Anim`, `Time_Anim`, `Intro_Anim`
  - `ui_qte`: tornado_defense_qte
    - file: `extracted_assets/phase16_support_archives/ExStageTails_Common/ui_qte.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/ExStageTails_Common/ui_qte.yncp`
    - file: `extracted_assets/phase135_ui_playscreen_probe/Sonic/ui_qte.yncp`
    - scene cues: `btn_position`, `qs_R`, `qs_L`, `boost`, `sliding`, `door`, `chaser`, `m_bg`
    - animation cues: `5`, `4`, `wide_5`, `Intro_Anim`, `Size_Anim_5`, `Size_Anim_4`, `Size_Anim_3`, `Timer_Anim`
