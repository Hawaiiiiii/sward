<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Full UI Archaeology Cross-Reference

Machine-readable inventory: `research_uiux/data/ui_archaeology_database.json`

Follow-on translated seam labeling: [`research_uiux/PPC_LAYOUT_STATE_LABELS.md`](./PPC_LAYOUT_STATE_LABELS.md)

## Summary

- Merged layout IDs cross-referenced: `26`
- Parsed layout files in the deep-analysis layer: `28`
- Screen/system groups assembled: `13`
- Asset-index entries after the current re-scan: `6751`
- Resolved generated PPC symbols available to the archaeology layer: `56`
- Total generated PPC symbol pool indexed for fallback seam resolution: `66392`

## Phase 23 Extraction Batch

- Dedicated extraction root: `extracted_assets/phase23_crossref_archives`
- Archive folders extracted in this batch: `15`
- Files extracted in this batch: `13898`
- New layout files surfaced in this batch: `5`
- Phase 23 layout files: `extracted_assets/phase23_crossref_archives/Town_Common/ui_balloon.yncp, extracted_assets/phase23_crossref_archives/Town_Common/ui_shop.yncp, extracted_assets/phase23_crossref_archives/Town_Common/ui_townscreen.yncp, extracted_assets/phase23_crossref_archives/Town_EggManBase_Common/ui_gate.yncp, extracted_assets/phase23_crossref_archives/Town_Labo_Common/ui_mediaroom.yncp`

> [!NOTE]
> `generated_seams` are precise symbol links already established by earlier mapping work.
> `patch_generated_symbols` are broader patch-file seam banks resolved against the local translated PPC output for this phase.

## `title_menu` - Title Menu

- Confidence: `medium_high` (`strong` evidence)
- Layout IDs: `ui_mainmenu`
- Layout files: `extracted_assets/ui_frontend_archives/MainMenu/ui_mainmenu.xncp, extracted_assets/ui_frontend_archives/MainMenu/ui_mainmenu.yncp`
- State tags: `appear, move, selection`
- Transition families: `intro, move`
- Texture families: `ui_mm_base, ui_mm_parts1, ui_mm_contentstext`
- Host code files: `5`
- Supporting files: `0`
- Precise generated seams: `3`
- Patch-derived generated seams: `0`
- Timing highlights: `mm_donut_move::DefaultAnim=220.0f, mm_donut_move::DefaultAnim=220.0f, mm_bg_usual::DefaultAnim=120.0f, mm_bg_usual::DefaultAnim=120.0f`
- Why this group matters: Title entry flow with intro handoff, menu cursor ownership, and shared fade/message support.

## `pause_stack` - Pause Stack

- Confidence: `high` (`direct` evidence)
- Layout IDs: `ui_pause, ui_general, ui_help`
- Layout files: `extracted_assets/ui_extended_archives/SystemCommonCore/ui_pause.yncp, extracted_assets/ui_extended_archives/SystemCommonCore/ui_general.yncp, extracted_assets/ui_extended_archives/SystemCommon/ui_help.yncp`
- State tags: `idle, appear, scroll, resize_or_focus, exit, framed_overlay, selection, button_prompt, chip, standard_or_sonic_variant, sonic, tails`
- Transition families: `usual, intro, scroll, size, outro, chip, so, sonic, tails`
- Prompt casts: `B, LB, LT, RB, RT, btn_1, btn_2, btn_3, btn_A, btn_B, btn_C, btn_LB, btn_RB, btn_img, select`
- Texture families: `mat_comon_en, mat_comon_num, mat_comon_x360, mat_ex_common, mat_pause_en, mat_result_comon, mat_shop_common, mat_talk_comon, mat_talk_tails, mat_talk_chip, mat_comon`
- Host code files: `7`
- Supporting files: `0`
- Precise generated seams: `23`
- Patch-derived generated seams: `20`
- Timing highlights: `btn_effect::charge_3_Outro=240.0f, arrow::DefaultAnim=200.0f, situation_text::DefaultAnim=200.0f, window_select::Scroll_Anim=200.0f`
- Why this group matters: Pause shell plus shared framed-window language and help/prompt overlays.

## `status_overlay` - Status Overlay

- Confidence: `high` (`direct` evidence)
- Layout IDs: `ui_status`
- Layout files: `extracted_assets/ui_extended_archives/SystemCommonCore/ui_status.yncp`
- State tags: `appear, event_variant, standard_or_sonic_variant, idle, selection, exit, resize_or_focus, switch, framed_overlay, button_prompt`
- Transition families: `intro, ev, so, usual, select, outro, size, switch`
- Prompt casts: `btn_A, btn_B, btn_LB, btn_RB`
- Texture families: `mat_comon, mat_comon_en, mat_comon_num, mat_comon_x360, mat_result_comon, mat_result_en, mat_status, mat_status_common, mat_status_en, mat_playscreen`
- Host code files: `2`
- Supporting files: `0`
- Precise generated seams: `20`
- Patch-derived generated seams: `20`
- Timing highlights: `medal_info::Outro_Anim=125.0f, medal_m_gauge::Outro_Anim=125.0f, medal_s_gauge::Outro_Anim=120.0f, medal_m_gauge::Intro_Anim=100.0f`
- Why this group matters: Status/progress presentation with title-shell framing, tags, and progress-bar choreography.

## `loading_and_start` - Loading And Start/Clear

- Confidence: `high` (`direct` evidence)
- Layout IDs: `ui_loading, ui_start`
- Layout files: `extracted_assets/ui_extended_archives/Loading/ui_loading.yncp, extracted_assets/phase16_support_archives/ActionCommon/ui_start.yncp`
- State tags: `idle, appear, standard_or_sonic_variant, sonic, 360, ps3, exit, event_variant, loop, button_prompt`
- Transition families: `usual, intro, so, sonic, 360, ps3, outro, ev, loop`
- Prompt casts: `LB, LT, RB, RT, a, b, lb, lt, rb, rt, x, y`
- Texture families: `mat_comon_txt, mat_load_comon, mat_load_en, mat_load, mat_loadinfo_text_sonic_e, mat_loadinfo_text_evil_e, mat_loadinfo_controller, mat_load_d_n_change, mat_start_en`
- Host code files: `3`
- Supporting files: `1`
- Precise generated seams: `30`
- Patch-derived generated seams: `30`
- Timing highlights: `Start::Intro_Anim=270.0f, Start::Intro_Anim_act=270.0f, pda::Outro_Anim=240.0f, pda_txt::Outro_Anim=240.0f`
- Why this group matters: Loading, transition, and start/clear handoff screens with time-of-day/cutscene adjacency.

## `world_map_stack` - World Map Stack

- Confidence: `high` (`direct` evidence)
- Layout IDs: `ui_worldmap, ui_worldmap_help`
- Layout files: `extracted_assets/ui_extended_archives/WorldMap/ui_worldmap.yncp, extracted_assets/ui_extended_archives/WorldMap/ui_worldmap_help.yncp`
- State tags: `appear, idle, switch, scroll, event_variant, rev, selection, resize_or_focus, framed_overlay, button_prompt`
- Transition families: `intro, usual, switch, scroll, ev, rev, select, size`
- Prompt casts: `LT, RB, btn_A, btn_A_txt, btn_B, btn_B_txt, btn_X, btn_X_txt`
- Texture families: `mat_comon, mat_comon_en, mat_comon_num, mat_comon_x360, mat_mainmenu_common, mat_mainmenu_en, mat_result_comon, mat_stage_ss, mat_worldmap, mat_worldmap_en, mat_worldmap_ss, mat_talk_tails`
- Audio/event cues: `sys_worldmap_cursor, sys_worldmap_finaldecide, bgm_sys_worldmap.csb`
- Host code files: `3`
- Supporting files: `1`
- Precise generated seams: `22`
- Patch-derived generated seams: `20`
- Timing highlights: `cts_cursor::Intro_Anim=221.0f, info_img_4::Intro_Anim=200.0f, info_img_3::Intro_Anim=190.0f, worldmap_header_img::Intro_Anim=185.0f`
- Why this group matters: World-map info panes, footer/header framing, and world-map help sidecars tied to cursor/camera seams.

## `town_ui` - Town UI

- Confidence: `high` (`direct` evidence)
- Layout IDs: `ui_balloon, ui_shop, ui_townscreen, ui_mediaroom`
- Layout files: `extracted_assets/phase23_crossref_archives/Town_Common/ui_balloon.yncp, extracted_assets/phase23_crossref_archives/Town_Common/ui_shop.yncp, extracted_assets/phase23_crossref_archives/Town_Common/ui_townscreen.yncp, extracted_assets/phase23_crossref_archives/Town_Labo_Common/ui_mediaroom.yncp`
- State tags: `appear, idle, scroll, resize_or_focus, framed_overlay, selection, button_prompt, event_variant, standard_or_sonic_variant, switch, exit`
- Transition families: `intro, usual, scroll, size, ev, so, switch, outro`
- Prompt casts: `B, btn_a, btn_b, btn_A, btn_B, btn_X, x, btn_1, btn_2, btn_3, btn_4, btn_5, btn_LB, btn_RB`
- Texture families: `mat_comon, mat_comon_num, mat_comon_x360, mat_pause_en, mat_result_comon, mat_shop_common, mat_shop_en, mat_talk_comon, mat_worldmap_ss, mat_comon_en, mat_ex_common, mat_townscreen, mat_townscreen_en, mat_media_num, mat_media_common, mat_media_en`
- Audio/event cues: `bgm_sys_mediaroom.csb`
- Host code files: `1`
- Supporting files: `2`
- Precise generated seams: `20`
- Patch-derived generated seams: `20`
- Timing highlights: `time_effect::Effect_Anim=600.0f, select::Scroll_hint_Anim=140.0f, select::Scroll_item_Anim=120.0f, balloon_nametag_sonic::Usual_Anim=110.0f`
- Why this group matters: Town-side balloon/dialog, shop, townscreen, and Media Room layout families extracted from town/common archives.

## `mission_briefing_and_gate` - Mission Briefing And Gate

- Confidence: `high` (`direct` evidence)
- Layout IDs: `ui_gate, ui_missionscreen, ui_misson`
- Layout files: `extracted_assets/phase16_support_archives/ActionCommon/ui_gate.yncp, extracted_assets/phase23_crossref_archives/Town_EggManBase_Common/ui_gate.yncp, extracted_assets/phase16_support_archives/ActionCommon/ui_missionscreen.yncp, extracted_assets/phase16_support_archives/ActionCommon/ui_misson.yncp`
- State tags: `idle, scroll, appear, resize_or_focus, exit, framed_overlay, selection, button_prompt, event_variant, standard_or_sonic_variant`
- Transition families: `usual, scroll, intro, size, outro, ev, so`
- Prompt casts: `LB, LT, RB, btn_A, btn_B, btn_LB, btn_RB`
- Texture families: `mat_comon_num, mat_comon_x360, mat_comon_en, mat_comon, mat_result_en, mat_result_comon, mat_gate_en, mat_stage_ss, mat_gate, mat_hit, mat_playscreen, mat_playscreen_en, mat_shop_common, mat_mission, mat_misson_en`
- Audio/event cues: `Hint/BossGate.dds`
- Host code files: `1`
- Supporting files: `1`
- Precise generated seams: `20`
- Patch-derived generated seams: `20`
- Timing highlights: `window::Usual_Anim=200.0f, window::Usual_Anim=200.0f, lap_count::DefaultAnim=100.0f, position::DefaultAnim=100.0f`
- Why this group matters: Mission gate/status framing plus mission briefing and mission-stat counter families.

## `boss_hud` - Boss HUD

- Confidence: `high` (`direct` evidence)
- Layout IDs: `ui_boss_gauge, ui_boss_name`
- Layout files: `extracted_assets/ui_broader_archives/BossCommon/ui_boss_gauge.yncp, extracted_assets/ui_broader_archives/BossCommon/ui_boss_name.yncp`
- State tags: `resize_or_focus, appear`
- Transition families: `size, intro`
- Texture families: `mat_boss_en, mat_boss_common, mat_result_comon`
- Audio/event cues: `bgm_boss_day.csb, bgm_boss_night.csb, vs_boss_sonic_e.csb, vs_boss_evil_e.csb`
- Visual taxonomy families: `boss_hud`
- Host code files: `1`
- Supporting files: `0`
- Precise generated seams: `20`
- Patch-derived generated seams: `20`
- Timing highlights: `name_ev::01_Anim=180.0f, name_so::01_Anim=180.0f, name_ev::02_Anim=180.0f, name_so::02_Anim=180.0f`
- Why this group matters: Boss-health/name presentation with authored gauge segments and variant-specific name branches.

## `item_result` - Item Result

- Confidence: `high` (`direct` evidence)
- Layout IDs: `ui_itemresult`
- Layout files: `extracted_assets/ui_extended_archives/SystemCommon/ui_itemresult.yncp`
- State tags: `appear, event_variant, standard_or_sonic_variant, idle, exit, framed_overlay, button_prompt`
- Transition families: `intro, ev, so, usual, outro`
- Prompt casts: `btn_A`
- Texture families: `mat_comon_num, mat_comon_x360, mat_comon_en, mat_comon, mat_result_comon, mat_result_en`
- Audio/event cues: `bgm_sys_result.csb`
- Visual taxonomy families: `result`
- Host code files: `1`
- Supporting files: `0`
- Precise generated seams: `20`
- Patch-derived generated seams: `20`
- Timing highlights: `result_footer::Usual_Anim=100.0f, iresult_title::Intro_ev_Anim=85.0f, iresult_title::Intro_ev_Anim_old=85.0f, iresult_title::Outro_ev_Anim=85.0f`
- Why this group matters: Compact item-result window with framed title/footer and reuse of the general pause-style shell language.

## `mission_result_family` - Mission Result Family

- Confidence: `high` (`direct` evidence)
- Layout IDs: `ui_result, ui_result_ex`
- Layout files: `extracted_assets/phase16_support_archives/ActionCommon/ui_result.yncp, extracted_assets/phase16_support_archives/ExStageTails_Common/ui_result_ex.yncp`
- State tags: `appear, event_variant, standard_or_sonic_variant, idle, framed_overlay, button_prompt`
- Transition families: `intro, ev, so, usual`
- Prompt casts: `btn_A`
- Texture families: `mat_comon_num, mat_comon_x360, mat_comon_en, mat_result_comon, mat_result_en`
- Audio/event cues: `bgm_sys_result.csb, vs_result_sonic_e.csb, vs_result_evil_e.csb`
- Visual taxonomy families: `result`
- Host code files: `1`
- Supporting files: `1`
- Precise generated seams: `20`
- Patch-derived generated seams: `20`
- Timing highlights: `result_rank_E::Intro_Anim=253.0f, result_rank_E::Intro_Anim=253.0f, result_rank::Usual_Anim=200.0f, result_rank_A::Usual_Anim=120.0f`
- Why this group matters: Main result and EX/result variants with score blocks, title shells, and replay/new-record highlights.

## `save_and_ending` - Save And Ending

- Confidence: `medium_high` (`strong` evidence)
- Layout IDs: `ui_saveicon, ui_end`
- Layout files: `extracted_assets/ui_extended_archives/AutoSave/ui_saveicon.yncp, extracted_assets/ui_extended_archives/StaffRoll/ui_end.yncp`
- State tags: `appear`
- Transition families: `intro`
- Texture families: `mat_save, mat_ex_common, mat_end`
- Visual taxonomy families: `save`
- Host code files: `3`
- Supporting files: `0`
- Precise generated seams: `30`
- Patch-derived generated seams: `30`
- Timing highlights: `ending::Intro_Anim_1=420.0f, ending::Intro_Anim_2=420.0f, icon::Intro_Anim=180.0f`
- Why this group matters: Autosave icon overlay plus ending/staff-roll text presentation seams.

## `tornado_defense` - Tornado Defense / EX Stage

- Confidence: `high` (`direct` evidence)
- Layout IDs: `ui_exstage, ui_prov_playscreen, ui_qte`
- Layout files: `extracted_assets/phase16_support_archives/ExStageTails_Common/ui_exstage.yncp, extracted_assets/phase16_support_archives/ExStageTails_Common/ui_prov_playscreen.yncp, extracted_assets/phase16_support_archives/ExStageTails_Common/ui_qte.yncp`
- State tags: `appear, idle, resize_or_focus, exit, button_prompt`
- Transition families: `intro, usual, size, outro`
- Prompt casts: `btn_1, btn_2, btn_3, btn_4, btn_5, btn_position`
- Texture families: `mat_comon_num, mat_ex_common, mat_ex_en, mat_hit, mat_hit_en, ui_ps1_gauge1, mat_comon, mat_playscreen_en, mat_playscreen, mat_comon_x360, mat_qte_en, mat_qte`
- Host code files: `4`
- Supporting files: `0`
- Precise generated seams: `38`
- Patch-derived generated seams: `38`
- Timing highlights: `info_1::Count_Anim=100.0f, info_2::Count_Anim=100.0f, bg::DefaultAnim=100.0f, ex_bg::Effect_Anim=100.0f`
- Why this group matters: EX-stage/Tornado Defense HUD, combat counters, and prompt/QTE-specific visual feedback.

## `subtitle_cutscene_presentation` - Subtitle / Cutscene Presentation

- Confidence: `high` (`direct` evidence)
- State tags: `prepare_movie, subtitle_window, loading_handoff, stage_change`
- Transition families: `playmovie, hide_layer, autosave_handoff`
- Host code files: `3`
- Supporting files: `0`
- Precise generated seams: `0`
- Patch-derived generated seams: `0`
- Timing highlights: `subtitle_windows::subtitle_max_duration=178.0f, playmovie_sequences::playmovie_project_duration=4862f`
- Why this group matters: Subtitle XML plus PlayMovie sequence correlation lives outside the CSD layout layer but belongs in the same cross-reference database for cutscene UI archaeology.

## Phase 30 Follow-On: Gameplay HUD Core

The new gameplay-HUD-specific pass now groups the previously scattered in-stage HUD evidence into:

- `sonic_stage_hud`
- `werehog_stage_hud`
- `extra_stage_hud`
- `super_sonic_hud`

That follow-on map resolves `6` gameplay HUD layout families:

- loose-layout-backed: `ui_prov_playscreen`, `ui_qte`
- hash/source-path-backed: `ui_playscreen`, `ui_playscreen_ev`, `ui_playscreen_ev_hit`, `ui_playscreen_su`

The machine-readable payload lives in `research_uiux/data/gameplay_hud_core_map.json`, and the human-readable breakdown lives in `research_uiux/GAMEPLAY_HUD_CORE_RECOVERY.md`.
