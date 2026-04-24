<p align="right">
    <img src="../../../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../../../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Portable Runtime Contracts

This directory contains the Phase 27/37/39/42/45/50 portable runtime contract files consumed by the native C++ runtime, the C ABI layer, and the C# reference port.

> [!NOTE]
> These contracts are structural ports only. They encode reusable state, timing, overlay, and prompt patterns. They do not contain extracted assets or translated proprietary code.

## Schema

Each contract file uses the same JSON shape:

- `screen_id`
- `source_system`
- `notes`
- `timeline_bands`
- `states`
- `overlay_layers`
- `visible_overlay_roles`
- `prompt_slots`

The native and managed loaders only require the runtime-facing fields. The archaeology-oriented metadata stays there to make the contracts self-describing.

## Bundled Contracts

- `pause_menu_reference.json`
- `title_menu_reference.json`
- `autosave_toast_reference.json`
- `loading_transition_reference.json`
- `mission_result_reference.json`
- `world_map_reference.json`
- `subtitle_cutscene_reference.json`
- `sonic_stage_hud_reference.json`
- `werehog_stage_hud_reference.json`
- `extra_stage_hud_reference.json`
- `super_sonic_hud_reference.json`
- `boss_hud_reference.json`
- `town_ui_reference.json`
- `camera_shell_reference.json`
- `application_world_shell_reference.json`
- `frontend_sequence_shell_reference.json`
- `achievement_unlock_support_reference.json`
- `audio_cue_support_reference.json`
- `xml_data_loading_support_reference.json`
