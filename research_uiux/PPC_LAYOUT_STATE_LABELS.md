<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> PPC Layout / State Labels

Machine-readable inventory: `research_uiux/data/ppc_ui_state_labels.json`

## Summary

- Target systems labeled in this pass: `8`
- Total labeled translated seams: `180`
- Labeled seams with resolved generated locations: `180`
- Function windows sampled directly from translated PPC output: `180`
- Systems still without resolved translated seams: `subtitle_cutscene_presentation`

> [!NOTE]
> These labels are archaeology aids, not claims about original authored function names.
> They translate patch ownership, layout families, timing anchors, and translated function windows into reusable screen-contract labels.

## `loading_and_start` - Loading And Start/Clear

- Layout IDs: `ui_loading, ui_start`
- State focus: `idle, appear, standard_or_sonic_variant, sonic, 360, ps3, exit, event_variant, loop, button_prompt`
- Transition focus: `usual, intro, so, sonic, 360, ps3, outro, ev, loop`
- Labeled seams: `30`
- Resolved generated seams: `30`
- Role families: `layout_projection_or_scaling=20, resident_overlay_visibility=10`
- Why this group matters: Loading, transition, and start/clear handoff screens with time-of-day/cutscene adjacency.

### Representative Labels

- `sub_822D0328` -> `loading_and_start__layout_projection_or_scaling__01`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.1.cpp:13533`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:418`
  - timing anchor: `Start::Intro_Anim=270.0f`
  - function window: `165` lines, `0` unique sub-calls
- `sub_82448CF0` -> `loading_and_start__layout_projection_or_scaling__02`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:35749`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:418`
  - timing anchor: `Start::Intro_Anim=270.0f`
  - function window: `145` lines, `6` unique sub-calls
  - sampled callees: `sub_82514A50, sub_82E668C8, sub_830BA1C0, sub_830BA298, sub_82512B90, sub_8320CFF0`
- `sub_82449088` -> `loading_and_start__layout_projection_or_scaling__03`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:36296`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:418`
  - timing anchor: `Start::Intro_Anim=270.0f`
  - function window: `1150` lines, `27` unique sub-calls
  - sampled callees: `sub_82E65D80, sub_82E812A8, sub_82448F00, sub_82512CE8, sub_82DF9958, sub_8250E300`
- `sub_8250FC70` -> `loading_and_start__layout_projection_or_scaling__04`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.32.cpp:13935`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:418`
  - timing anchor: `Start::Intro_Anim=270.0f`
  - function window: `293` lines, `11` unique sub-calls
  - sampled callees: `sub_82DFB728, sub_8250F238, sub_82DFB148, sub_8252BD20, sub_8252BB90, sub_822C6A10`
- `sub_8258B558` -> `loading_and_start__layout_projection_or_scaling__05`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.40.cpp:49453`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:418`
  - timing anchor: `Start::Intro_Anim=270.0f`
  - function window: `2149` lines, `69` unique sub-calls
  - sampled callees: `sub_8315CF68, sub_82512B50, sub_824ECD10, sub_82DF9958, sub_82512CE8, sub_8250C4C8`
- `sub_825E2E60` -> `loading_and_start__layout_projection_or_scaling__06`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.48.cpp:8719`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:418`
  - timing anchor: `Start::Intro_Anim=270.0f`
  - function window: `13` lines, `1` unique sub-calls
  - sampled callees: `sub_82DF9E50`

## `world_map_stack` - World Map Stack

- Layout IDs: `ui_worldmap, ui_worldmap_help`
- State focus: `appear, idle, switch, scroll, event_variant, rev, selection, resize_or_focus, framed_overlay, button_prompt`
- Transition focus: `intro, usual, switch, scroll, ev, rev, select, size`
- Labeled seams: `22`
- Resolved generated seams: `22`
- Role families: `layout_projection_or_scaling=20, input_cursor_or_camera_owner=2`
- Why this group matters: World-map info panes, footer/header framing, and world-map help sidecars tied to cursor/camera seams.

### Representative Labels

- `sub_82486968` -> `world_map_stack__input_cursor_or_camera_owner__01`
  - source: `precise` via `UnleashedRecomp/patches/input_patches.cpp`
  - role family: `input_cursor_or_camera_owner`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.25.cpp:12860`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:758`
  - timing anchor: `cts_cursor::Intro_Anim=221.0f`
  - function window: `21` lines, `2` unique sub-calls
  - sampled callees: `sub_82486188, sub_82486258`
- `sub_8256C938` -> `world_map_stack__input_cursor_or_camera_owner__02`
  - source: `precise` via `UnleashedRecomp/patches/input_patches.cpp`
  - role family: `input_cursor_or_camera_owner`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.39.cpp:24278`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:758`
  - timing anchor: `cts_cursor::Intro_Anim=221.0f`
  - function window: `122` lines, `3` unique sub-calls
  - sampled callees: `sub_82DF98C0, sub_82B4E2F0, sub_82DF9958`
- `sub_822D0328` -> `world_map_stack__layout_projection_or_scaling__03`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.1.cpp:13533`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:758`
  - timing anchor: `cts_cursor::Intro_Anim=221.0f`
  - function window: `165` lines, `0` unique sub-calls
- `sub_82448CF0` -> `world_map_stack__layout_projection_or_scaling__04`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:35749`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:758`
  - timing anchor: `cts_cursor::Intro_Anim=221.0f`
  - function window: `145` lines, `6` unique sub-calls
  - sampled callees: `sub_82514A50, sub_82E668C8, sub_830BA1C0, sub_830BA298, sub_82512B90, sub_8320CFF0`
- `sub_82449088` -> `world_map_stack__layout_projection_or_scaling__05`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:36296`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:758`
  - timing anchor: `cts_cursor::Intro_Anim=221.0f`
  - function window: `1150` lines, `27` unique sub-calls
  - sampled callees: `sub_82E65D80, sub_82E812A8, sub_82448F00, sub_82512CE8, sub_82DF9958, sub_8250E300`
- `sub_8250FC70` -> `world_map_stack__layout_projection_or_scaling__06`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.32.cpp:13935`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:758`
  - timing anchor: `cts_cursor::Intro_Anim=221.0f`
  - function window: `293` lines, `11` unique sub-calls
  - sampled callees: `sub_82DFB728, sub_8250F238, sub_82DFB148, sub_8252BD20, sub_8252BB90, sub_822C6A10`

## `town_ui` - Town UI

- Layout IDs: `ui_balloon, ui_shop, ui_townscreen, ui_mediaroom`
- State focus: `appear, idle, scroll, resize_or_focus, framed_overlay, selection, button_prompt, event_variant, standard_or_sonic_variant, switch, exit`
- Transition focus: `intro, usual, scroll, size, ev, so, switch, outro`
- Labeled seams: `20`
- Resolved generated seams: `20`
- Role families: `layout_projection_or_scaling=20`
- Why this group matters: Town-side balloon/dialog, shop, townscreen, and Media Room layout families extracted from town/common archives.

### Representative Labels

- `sub_822D0328` -> `town_ui__layout_projection_or_scaling__01`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.1.cpp:13533`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:369`
  - timing anchor: `time_effect::Effect_Anim=600.0f`
  - function window: `165` lines, `0` unique sub-calls
- `sub_82448CF0` -> `town_ui__layout_projection_or_scaling__02`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:35749`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:369`
  - timing anchor: `time_effect::Effect_Anim=600.0f`
  - function window: `145` lines, `6` unique sub-calls
  - sampled callees: `sub_82514A50, sub_82E668C8, sub_830BA1C0, sub_830BA298, sub_82512B90, sub_8320CFF0`
- `sub_82449088` -> `town_ui__layout_projection_or_scaling__03`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:36296`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:369`
  - timing anchor: `time_effect::Effect_Anim=600.0f`
  - function window: `1150` lines, `27` unique sub-calls
  - sampled callees: `sub_82E65D80, sub_82E812A8, sub_82448F00, sub_82512CE8, sub_82DF9958, sub_8250E300`
- `sub_8250FC70` -> `town_ui__layout_projection_or_scaling__04`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.32.cpp:13935`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:369`
  - timing anchor: `time_effect::Effect_Anim=600.0f`
  - function window: `293` lines, `11` unique sub-calls
  - sampled callees: `sub_82DFB728, sub_8250F238, sub_82DFB148, sub_8252BD20, sub_8252BB90, sub_822C6A10`
- `sub_8258B558` -> `town_ui__layout_projection_or_scaling__05`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.40.cpp:49453`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:369`
  - timing anchor: `time_effect::Effect_Anim=600.0f`
  - function window: `2149` lines, `69` unique sub-calls
  - sampled callees: `sub_8315CF68, sub_82512B50, sub_824ECD10, sub_82DF9958, sub_82512CE8, sub_8250C4C8`
- `sub_825E2E60` -> `town_ui__layout_projection_or_scaling__06`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.48.cpp:8719`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:369`
  - timing anchor: `time_effect::Effect_Anim=600.0f`
  - function window: `13` lines, `1` unique sub-calls
  - sampled callees: `sub_82DF9E50`

## `mission_briefing_and_gate` - Mission Briefing And Gate

- Layout IDs: `ui_gate, ui_missionscreen, ui_misson`
- State focus: `idle, scroll, appear, resize_or_focus, exit, framed_overlay, selection, button_prompt, event_variant, standard_or_sonic_variant`
- Transition focus: `usual, scroll, intro, size, outro, ev, so`
- Labeled seams: `20`
- Resolved generated seams: `20`
- Role families: `layout_projection_or_scaling=20`
- Why this group matters: Mission gate/status framing plus mission briefing and mission-stat counter families.

### Representative Labels

- `sub_822D0328` -> `mission_briefing_and_gate__layout_projection_or_scaling__01`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.1.cpp:13533`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:396`
  - timing anchor: `window::Usual_Anim=200.0f`
  - function window: `165` lines, `0` unique sub-calls
- `sub_82448CF0` -> `mission_briefing_and_gate__layout_projection_or_scaling__02`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:35749`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:396`
  - timing anchor: `window::Usual_Anim=200.0f`
  - function window: `145` lines, `6` unique sub-calls
  - sampled callees: `sub_82514A50, sub_82E668C8, sub_830BA1C0, sub_830BA298, sub_82512B90, sub_8320CFF0`
- `sub_82449088` -> `mission_briefing_and_gate__layout_projection_or_scaling__03`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:36296`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:396`
  - timing anchor: `window::Usual_Anim=200.0f`
  - function window: `1150` lines, `27` unique sub-calls
  - sampled callees: `sub_82E65D80, sub_82E812A8, sub_82448F00, sub_82512CE8, sub_82DF9958, sub_8250E300`
- `sub_8250FC70` -> `mission_briefing_and_gate__layout_projection_or_scaling__04`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.32.cpp:13935`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:396`
  - timing anchor: `window::Usual_Anim=200.0f`
  - function window: `293` lines, `11` unique sub-calls
  - sampled callees: `sub_82DFB728, sub_8250F238, sub_82DFB148, sub_8252BD20, sub_8252BB90, sub_822C6A10`
- `sub_8258B558` -> `mission_briefing_and_gate__layout_projection_or_scaling__05`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.40.cpp:49453`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:396`
  - timing anchor: `window::Usual_Anim=200.0f`
  - function window: `2149` lines, `69` unique sub-calls
  - sampled callees: `sub_8315CF68, sub_82512B50, sub_824ECD10, sub_82DF9958, sub_82512CE8, sub_8250C4C8`
- `sub_825E2E60` -> `mission_briefing_and_gate__layout_projection_or_scaling__06`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.48.cpp:8719`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:396`
  - timing anchor: `window::Usual_Anim=200.0f`
  - function window: `13` lines, `1` unique sub-calls
  - sampled callees: `sub_82DF9E50`

## `mission_result_family` - Mission Result Family

- Layout IDs: `ui_result, ui_result_ex`
- State focus: `appear, event_variant, standard_or_sonic_variant, idle, framed_overlay, button_prompt`
- Transition focus: `intro, ev, so, usual`
- Labeled seams: `20`
- Resolved generated seams: `20`
- Role families: `layout_projection_or_scaling=20`
- Why this group matters: Main result and EX/result variants with score blocks, title shells, and replay/new-record highlights.

### Representative Labels

- `sub_822D0328` -> `mission_result_family__layout_projection_or_scaling__01`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.1.cpp:13533`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:619`
  - timing anchor: `result_rank_E::Intro_Anim=253.0f`
  - function window: `165` lines, `0` unique sub-calls
- `sub_82448CF0` -> `mission_result_family__layout_projection_or_scaling__02`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:35749`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:619`
  - timing anchor: `result_rank_E::Intro_Anim=253.0f`
  - function window: `145` lines, `6` unique sub-calls
  - sampled callees: `sub_82514A50, sub_82E668C8, sub_830BA1C0, sub_830BA298, sub_82512B90, sub_8320CFF0`
- `sub_82449088` -> `mission_result_family__layout_projection_or_scaling__03`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:36296`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:619`
  - timing anchor: `result_rank_E::Intro_Anim=253.0f`
  - function window: `1150` lines, `27` unique sub-calls
  - sampled callees: `sub_82E65D80, sub_82E812A8, sub_82448F00, sub_82512CE8, sub_82DF9958, sub_8250E300`
- `sub_8250FC70` -> `mission_result_family__layout_projection_or_scaling__04`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.32.cpp:13935`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:619`
  - timing anchor: `result_rank_E::Intro_Anim=253.0f`
  - function window: `293` lines, `11` unique sub-calls
  - sampled callees: `sub_82DFB728, sub_8250F238, sub_82DFB148, sub_8252BD20, sub_8252BB90, sub_822C6A10`
- `sub_8258B558` -> `mission_result_family__layout_projection_or_scaling__05`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.40.cpp:49453`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:619`
  - timing anchor: `result_rank_E::Intro_Anim=253.0f`
  - function window: `2149` lines, `69` unique sub-calls
  - sampled callees: `sub_8315CF68, sub_82512B50, sub_824ECD10, sub_82DF9958, sub_82512CE8, sub_8250C4C8`
- `sub_825E2E60` -> `mission_result_family__layout_projection_or_scaling__06`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.48.cpp:8719`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:619`
  - timing anchor: `result_rank_E::Intro_Anim=253.0f`
  - function window: `13` lines, `1` unique sub-calls
  - sampled callees: `sub_82DF9E50`

## `save_and_ending` - Save And Ending

- Layout IDs: `ui_saveicon, ui_end`
- State focus: `appear`
- Transition focus: `intro`
- Labeled seams: `30`
- Resolved generated seams: `30`
- Role families: `layout_projection_or_scaling=20, resident_overlay_visibility=10`
- Why this group matters: Autosave icon overlay plus ending/staff-roll text presentation seams.

### Representative Labels

- `sub_822D0328` -> `save_and_ending__layout_projection_or_scaling__01`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.1.cpp:13533`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:1656`
  - timing anchor: `ending::Intro_Anim_1=420.0f`
  - function window: `165` lines, `0` unique sub-calls
- `sub_82448CF0` -> `save_and_ending__layout_projection_or_scaling__02`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:35749`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:1656`
  - timing anchor: `ending::Intro_Anim_1=420.0f`
  - function window: `145` lines, `6` unique sub-calls
  - sampled callees: `sub_82514A50, sub_82E668C8, sub_830BA1C0, sub_830BA298, sub_82512B90, sub_8320CFF0`
- `sub_82449088` -> `save_and_ending__layout_projection_or_scaling__03`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.21.cpp:36296`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:1656`
  - timing anchor: `ending::Intro_Anim_1=420.0f`
  - function window: `1150` lines, `27` unique sub-calls
  - sampled callees: `sub_82E65D80, sub_82E812A8, sub_82448F00, sub_82512CE8, sub_82DF9958, sub_8250E300`
- `sub_8250FC70` -> `save_and_ending__layout_projection_or_scaling__04`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.32.cpp:13935`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:1656`
  - timing anchor: `ending::Intro_Anim_1=420.0f`
  - function window: `293` lines, `11` unique sub-calls
  - sampled callees: `sub_82DFB728, sub_8250F238, sub_82DFB148, sub_8252BD20, sub_8252BB90, sub_822C6A10`
- `sub_8258B558` -> `save_and_ending__layout_projection_or_scaling__05`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.40.cpp:49453`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:1656`
  - timing anchor: `ending::Intro_Anim_1=420.0f`
  - function window: `2149` lines, `69` unique sub-calls
  - sampled callees: `sub_8315CF68, sub_82512B50, sub_824ECD10, sub_82DF9958, sub_82512CE8, sub_8250C4C8`
- `sub_825E2E60` -> `save_and_ending__layout_projection_or_scaling__06`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
  - role family: `layout_projection_or_scaling`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.48.cpp:8719`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:1656`
  - timing anchor: `ending::Intro_Anim_1=420.0f`
  - function window: `13` lines, `1` unique sub-calls
  - sampled callees: `sub_82DF9E50`

## `tornado_defense` - Tornado Defense / EX Stage

- Layout IDs: `ui_exstage, ui_prov_playscreen, ui_qte`
- State focus: `appear, idle, resize_or_focus, exit, button_prompt`
- Transition focus: `intro, usual, size, outro`
- Labeled seams: `38`
- Resolved generated seams: `38`
- Role families: `layout_projection_or_scaling=20, prompt_filter_or_tutorial_gate=10, feedback_or_prompt_fx=6, delta_time_or_timing_owner=2`
- Why this group matters: EX-stage/Tornado Defense HUD, combat counters, and prompt/QTE-specific visual feedback.

### Representative Labels

- `sub_82B00D00` -> `tornado_defense__delta_time_or_timing_owner__01`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/fps_patches.cpp`
  - role family: `delta_time_or_timing_owner`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.130.cpp:56124`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:383`
  - timing anchor: `info_1::Count_Anim=100.0f`
  - function window: `1197` lines, `25` unique sub-calls
  - sampled callees: `sub_8315CF68, sub_824ED808, sub_824EC488, sub_82DF9958, sub_82DFB728, sub_82E1E040`
- `sub_8312DBF8` -> `tornado_defense__delta_time_or_timing_owner__02`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/fps_patches.cpp`
  - role family: `delta_time_or_timing_owner`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.233.cpp:26713`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:383`
  - timing anchor: `info_1::Count_Anim=100.0f`
  - function window: `67` lines, `2` unique sub-calls
  - sampled callees: `sub_8312DAE0, sub_82BD42F0`
- `sub_82608E60` -> `tornado_defense__feedback_or_prompt_fx__03`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/object_patches.cpp`
  - role family: `feedback_or_prompt_fx`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.50.cpp:33684`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:383`
  - timing anchor: `info_1::Count_Anim=100.0f`
  - function window: `143` lines, `1` unique sub-calls
  - sampled callees: `sub_82515C40`
- `sub_82614228` -> `tornado_defense__feedback_or_prompt_fx__04`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/object_patches.cpp`
  - role family: `feedback_or_prompt_fx`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.51.cpp:19943`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:383`
  - timing anchor: `info_1::Count_Anim=100.0f`
  - function window: `133` lines, `2` unique sub-calls
  - sampled callees: `sub_82515B70, sub_822DD670`
- `sub_826145D8` -> `tornado_defense__feedback_or_prompt_fx__05`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/object_patches.cpp`
  - role family: `feedback_or_prompt_fx`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.51.cpp:20500`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:383`
  - timing anchor: `info_1::Count_Anim=100.0f`
  - function window: `384` lines, `21` unique sub-calls
  - sampled callees: `sub_825154B8, sub_82E84BE0, sub_82E84A10, sub_82DFA0B0, sub_82515448, sub_82B5C9C8`
- `sub_8271AA30` -> `tornado_defense__feedback_or_prompt_fx__06`
  - source: `precise+patch_bank` via `UnleashedRecomp/patches/object_patches.cpp`
  - role family: `feedback_or_prompt_fx`
  - generated location: `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.67.cpp:22454`
  - host anchor: `UnleashedRecomp/patches/aspect_ratio_patches.cpp:383`
  - timing anchor: `info_1::Count_Anim=100.0f`
  - function window: `218` lines, `9` unique sub-calls
  - sampled callees: `sub_82AEDA48, sub_82AE9F50, sub_826D05A0, sub_82AE9F60, sub_82512D38, sub_8252B698`

## `subtitle_cutscene_presentation` - Subtitle / Cutscene Presentation

- State focus: `prepare_movie, subtitle_window, loading_handoff, stage_change`
- Transition focus: `playmovie, hide_layer, autosave_handoff`
- Labeled seams: `0`
- Resolved generated seams: `0`
- Why this group matters: Subtitle XML plus PlayMovie sequence correlation lives outside the CSD layout layer but belongs in the same cross-reference database for cutscene UI archaeology.

- No translated PPC seam is currently resolved for this system; it remains readable-code and asset-driven.
