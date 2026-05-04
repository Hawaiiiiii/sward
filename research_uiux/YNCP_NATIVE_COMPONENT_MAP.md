# YNCP Native Component Map

Generated from the fully extracted current install. This is a local R&D map for turning authored Sonic Unleashed UI projects into reusable SGFX/native screen components.

Phase 236 wires this map into the in-game SWARD UI Lab `UI Projects` browser. It is runtime-derived-native-reconstruction metadata, not original authored SEGA source.

Phase 237 adds an independent drawable/keyframe preview lane for the selected scene row. The preview is driven by parsed project/cast/animation/track metadata and is not tied to the current gameplay/title/loading route, so it can be inspected while the running game is sitting in a HUD, hub, title, loading, or other state. The adjacent audio-bank correlation lane keeps SFX as placeholder cue intents until audio banks/XML, runtime hooks, and Ghidra xrefs prove exact cue IDs.

Phase 238 binds parsed casts to real extracted DDS texture names, subimage UV rectangles, source texture dimensions, and preview destination rectangles. The in-game UI Projects canvas now has a texture-backed cast/subimage preview layer using real DDS/subimage rectangles, while the SFX lane begins with runtime-hook + Ghidra xref SFX candidates such as `se_system_worldmap/sys_worldmap_decide` from `CTitleStateMenu::Update`.

Phase 239 adds a composed scene display list from the YNCP cast hierarchy. Scene draw commands apply cast tree parent transforms, `cast_info` translation/scale, selected material subimage slots, texture UVs, and extracted DDS paths so the in-game preview renders a selected scene/component instead of only showing a raw atlas crop.

Phase 240 adds keyframe interpolation/scrubbing from real authored animation tracks. The generated native map now carries preview-useful YNCP transform keyframes (`XPosition`, `YPosition`, `XScale`, `YScale`, `Rotation`, `HideFlag`, `SubImage`) with frame, value, tangent, and interpolation provenance so the in-game canvas can sample actual authored motion while the adjacent SFX lane keeps same-scene cue candidates beside it.

Phase 241 adds the in-game foreground invoke mode: the selected YNCP scene can be projected over gameplay as a pause-menu-style summoned UI surface, outside the inspector/browser panel, while still using the reconstructed scene draw commands, keyframe scrub/playback, and same-row SFX correlation data.

Phase 242 adds the guarded Native CSD Make Probe plan to the runtime bridge: selected .yncp/.xncp project bytes are header-checked as raw CPAF/NYIF/nCPJ CSD packages, copied into guest heap, and handed to `SWA::CCsdProject::Make`/`sub_825E4068` through a cloned PPC probe context, with `MakeCsdProjectMidAsmHook` traversal used as proof that the game parsed the project into a native CSD tree before render-host attachment is attempted.

Phase 243 hardens that native probe after the crash evidence showed `sub_825E4068` reads a required fourth argument through `r6`. The button now queues work until the next real `CCsdProject::Make` call, captures that live `r6` make context, and only then attempts the cloned native call; loading/save update ticks no longer fire the native Make probe with unrelated registers.

Phase 244 changes the default probe into a non-crashing native observe mode after the next crash showed direct Make can still fail when the internal package parser returns a null temporary resource. By default, the probe now waits for the game to create the selected project naturally and captures the resulting native tree at `MakeCsdProjectMidAsmHook`; a separate `danger: execute Make` checkbox keeps the direct call path available only as an explicit experiment.

Phase 245 adds the Native Foreground Render Probe. Once native observe has resolved a selected scene pointer, `Attach Native Scene` / `Spawn Native Foreground` arms that real CSD scene and piggybacks it on the next active `CScene::Render` pass, with motion scrub/playback writing the native `m_MotionFrame`. This is deliberately not an owner pointer hijack yet; render-host proof comes first, and foreground owner replacement stays a later, riskier experiment.

Phase 246 exposes that native foreground path through the live bridge as direct operator controls: `native-foreground-status`, `native-foreground-attach`, `native-foreground-detach`, `native-motion-play`, `native-motion-stop`, and `native-motion-scrub <frame>`. The bridge returns the native project/root/scene pointers, scene motion frame/repeat fields, render-pass sightings, render count, and the explicit `ownerHijackUsed=false` safety status so render-count proof can be gathered before any foreground owner attach experiment.

Phase 247 adds a tool-facing `native-make-observe <project> <scene> [frame]` bridge command. It queues the selected YNCP/XNCP project row by project name/path and scene name, then waits for the running game to create that CSD project naturally so `MakeCsdProjectMidAsmHook` can capture the native tree without requiring manual panel clicks or the dangerous direct Make path.

Phase 250 guards the Native Foreground Render Probe with a same-project host requirement. The bridge can resolve and arm a native scene from the observed game-created tree, but render piggyback now refuses to call `CScene::Render` unless the active host pass belongs to the same captured CSD project; cross-project foreground spawning is deferred to a real owner/host attach path.

Phase 251 correlates CSD resource `Scene*` records from the native project traversal with live renderable `CScene*` manager instances from the `CScene::Render` hook. Motion scrub/playback now require `nativeManagerScenePointer` plus `nativeResourceScenePointer` provenance, so the lab no longer writes manager fields on the non-renderable resource tree object. The foreground attach path is held as a same-project compatibility probe until a real foreground owner/host is resolved, because an extra render call without ownership proof is still crash-prone.

Phase 252 adds Foreground Owner/Host Attach Discovery. The live bridge can run `native-owner-discovery` / `native-owner-scan` to inspect bounded known UI owner ranges (title owner context, CHudSonicStage owner, CHudPause owner, CGeneralWindow owner, CSaveIcon owner) for direct or indirect references to the correlated manager CScene/resource Scene pair before any owner/host attach or pointer hijack is attempted.

- Input root: `C:/Users/DavidErikGarciaArena/Downloads/UI-UX Sonic World Adventure for SGFX - Project Quality Hero/extracted_assets/full_install_archives`
- Project files parsed: `41`
- Preview draw commands: `1888` real-yncp-subimage-dds-rect rows.
- Scene draw commands: `7530` real-yncp-cast-tree-subimage-scene-rect rows.
- Animation keyframes: `20091` real-yncp-animation-keyframe rows for keyframe interpolation.
- SFX cue candidates: `4` runtime-hook + Ghidra xref SFX candidates.
- SFX correlation: `sfx-correlation-pending` until audio banks/XML/runtime hooks/Ghidra xrefs prove every exact cue ID.

## Hub Town

### `ui_gate`
- Path: `game/ActionCommon/ui_gate.yncp`
- Scenes: `16`; casts: `163`; animations: `30`; textures: `13`
- Root scenes: ``
- Component roles: `result_rank=15, title_logo=1`
- Texture-backed preview commands: `51`
- Composed scene draw commands: `116`
- Animation keyframes: `181`
- Key scenes:
  - `area_tag` -> `result_rank` casts=6 anims=1 frames=[15.0, 15.0]
  - `bg` -> `result_rank` casts=76 anims=2 frames=[20.0, 20.0]
  - `stage_name` -> `result_rank` casts=2 anims=2 frames=[0.0, 20.0]
  - `info_1` -> `result_rank` casts=4 anims=2 frames=[0.0, 20.0]
  - `info_2` -> `result_rank` casts=8 anims=2 frames=[0.0, 20.0]
  - `info_3` -> `result_rank` casts=7 anims=2 frames=[20.0, 59.0]
  - `info_4` -> `result_rank` casts=7 anims=2 frames=[20.0, 59.0]
  - `info_5` -> `result_rank` casts=6 anims=2 frames=[20.0, 59.0]
  - `stage_ss` -> `result_rank` casts=4 anims=2 frames=[0.0, 20.0]
  - `info_6` -> `result_rank` casts=3 anims=2 frames=[0.0, 20.0]
  - `arrow` -> `result_rank` casts=2 anims=1 frames=[0.0, 0.0]
  - `window_bg` -> `result_rank` casts=1 anims=1 frames=[5.0, 5.0]

### `ui_missionscreen`
- Path: `game/ActionCommon/ui_missionscreen.yncp`
- Scenes: `7`; casts: `134`; animations: `19`; textures: `9`
- Root scenes: `player_count, time_count, laptime_count, score_count, item_count, position, lap_count`
- Component roles: `hud_life=1, hud_score_time=4, town_dialog=2`
- Texture-backed preview commands: `22`
- Composed scene draw commands: `93`
- Animation keyframes: `243`
- Key scenes:
  - `player_count` -> `hud_life` casts=4 anims=2 frames=[100.0, 100.0]
  - `time_count` -> `hud_score_time` casts=32 anims=4 frames=[100.0, 100.0]
  - `laptime_count` -> `hud_score_time` casts=32 anims=4 frames=[100.0, 100.0]
  - `score_count` -> `hud_score_time` casts=26 anims=4 frames=[5.0, 100.0]
  - `item_count` -> `hud_score_time` casts=27 anims=3 frames=[5.0, 29.0]
  - `position` -> `town_dialog` casts=5 anims=1 frames=[100.0, 100.0]
  - `lap_count` -> `town_dialog` casts=8 anims=1 frames=[100.0, 100.0]

### `ui_misson`
- Path: `game/ActionCommon/ui_misson.yncp`
- Scenes: `6`; casts: `38`; animations: `7`; textures: `7`
- Root scenes: `bg`
- Component roles: `pause_shell=1, result_rank=4, title_logo=1`
- Texture-backed preview commands: `18`
- Composed scene draw commands: `27`
- Animation keyframes: `56`
- Key scenes:
  - `bg` -> `result_rank` casts=1 anims=1 frames=[15.0, 15.0]
  - `bg_B1` -> `result_rank` casts=14 anims=1 frames=[20.0, 20.0]
  - `select` -> `result_rank` casts=4 anims=1 frames=[30.0, 30.0]
  - `bg_B2` -> `result_rank` casts=7 anims=2 frames=[15.0, 59.0]
  - `misson_title_B` -> `title_logo` casts=10 anims=1 frames=[69.0, 69.0]
  - `footer_B` -> `pause_shell` casts=2 anims=1 frames=[0.0, 0.0]

### `ui_balloon`
- Path: `game/Town_Common/ui_balloon.yncp`
- Scenes: `15`; casts: `175`; animations: `25`; textures: `12`
- Root scenes: ``
- Component roles: `menu_choices=1, town_dialog=14`
- Texture-backed preview commands: `50`
- Composed scene draw commands: `52`
- Animation keyframes: `291`
- Key scenes:
  - `balloon_nomal` -> `town_dialog` casts=7 anims=2 frames=[10.0, 10.0]
  - `balloon_shout` -> `town_dialog` casts=7 anims=2 frames=[7.0, 7.0]
  - `balloon_think` -> `town_dialog` casts=9 anims=2 frames=[20.0, 20.0]
  - `balloon_txt_area` -> `town_dialog` casts=4 anims=2 frames=[100.0, 100.0]
  - `balloon_sonic` -> `town_dialog` casts=19 anims=1 frames=[20.0, 20.0]
  - `balloon_sonic_select` -> `menu_choices` casts=10 anims=3 frames=[20.0, 60.0]
  - `balloon_nametag` -> `town_dialog` casts=15 anims=1 frames=[20.0, 20.0]
  - `balloon_nametag_sonic` -> `town_dialog` casts=29 anims=2 frames=[100.0, 110.0]
  - `bg` -> `town_dialog` casts=1 anims=1 frames=[15.0, 15.0]
  - `item_window` -> `town_dialog` casts=35 anims=1 frames=[15.0, 15.0]
  - `hint_window` -> `town_dialog` casts=15 anims=1 frames=[15.0, 15.0]
  - `scroll_bar` -> `town_dialog` casts=11 anims=2 frames=[100.0, 100.0]

### `ui_shop`
- Path: `game/Town_Common/ui_shop.yncp`
- Scenes: `17`; casts: `136`; animations: `24`; textures: `9`
- Root scenes: ``
- Component roles: `town_dialog=17`
- Texture-backed preview commands: `57`
- Composed scene draw commands: `33`
- Animation keyframes: `147`
- Key scenes:
  - `bg_1` -> `town_dialog` casts=17 anims=2 frames=[15.0, 15.0]
  - `bg_2` -> `town_dialog` casts=14 anims=1 frames=[15.0, 15.0]
  - `bg_3` -> `town_dialog` casts=13 anims=1 frames=[15.0, 15.0]
  - `bg_4` -> `town_dialog` casts=14 anims=1 frames=[15.0, 15.0]
  - `select_1` -> `town_dialog` casts=4 anims=3 frames=[60.0, 80.0]
  - `icon_2` -> `town_dialog` casts=9 anims=2 frames=[8.0, 8.0]
  - `icon_1` -> `town_dialog` casts=2 anims=1 frames=[45.0, 45.0]
  - `icon_3` -> `town_dialog` casts=2 anims=1 frames=[45.0, 45.0]
  - `bg_1_num` -> `town_dialog` casts=2 anims=1 frames=[100.0, 100.0]
  - `scrollbar_bg` -> `town_dialog` casts=6 anims=1 frames=[100.0, 100.0]
  - `scrollbar` -> `town_dialog` casts=4 anims=1 frames=[100.0, 100.0]
  - `bg_3_num` -> `town_dialog` casts=12 anims=1 frames=[100.0, 100.0]

### `ui_townscreen`
- Path: `game/Town_Common/ui_townscreen.yncp`
- Scenes: `5`; casts: `79`; animations: `10`; textures: `10`
- Root scenes: `time, time_effect, footer, info, cam`
- Component roles: `result_rank=1, status_skill=1, town_dialog=3`
- Texture-backed preview commands: `17`
- Composed scene draw commands: `31`
- Animation keyframes: `373`
- Key scenes:
  - `time` -> `status_skill` casts=13 anims=1 frames=[100.0, 100.0]
  - `time_effect` -> `town_dialog` casts=1 anims=1 frames=[600.0, 600.0]
  - `footer` -> `town_dialog` casts=43 anims=4 frames=[60.0, 60.0]
  - `info` -> `result_rank` casts=14 anims=2 frames=[59.0, 59.0]
  - `cam` -> `town_dialog` casts=8 anims=2 frames=[59.0, 59.0]

### `ui_gate`
- Path: `game/Town_EggManBase_Common/ui_gate.yncp`
- Scenes: `16`; casts: `163`; animations: `30`; textures: `13`
- Root scenes: ``
- Component roles: `result_rank=15, title_logo=1`
- Texture-backed preview commands: `51`
- Composed scene draw commands: `116`
- Animation keyframes: `181`
- Key scenes:
  - `area_tag` -> `result_rank` casts=6 anims=1 frames=[15.0, 15.0]
  - `bg` -> `result_rank` casts=76 anims=2 frames=[20.0, 20.0]
  - `stage_name` -> `result_rank` casts=2 anims=2 frames=[0.0, 20.0]
  - `info_1` -> `result_rank` casts=4 anims=2 frames=[0.0, 20.0]
  - `info_2` -> `result_rank` casts=8 anims=2 frames=[0.0, 20.0]
  - `info_3` -> `result_rank` casts=7 anims=2 frames=[20.0, 59.0]
  - `info_4` -> `result_rank` casts=7 anims=2 frames=[20.0, 59.0]
  - `info_5` -> `result_rank` casts=6 anims=2 frames=[20.0, 59.0]
  - `stage_ss` -> `result_rank` casts=4 anims=2 frames=[0.0, 20.0]
  - `info_6` -> `result_rank` casts=3 anims=2 frames=[0.0, 20.0]
  - `arrow` -> `result_rank` casts=2 anims=1 frames=[0.0, 0.0]
  - `window_bg` -> `result_rank` casts=1 anims=1 frames=[5.0, 5.0]

## Loading

### `ui_start`
- Path: `game/ActionCommon/ui_start.yncp`
- Scenes: `4`; casts: `39`; animations: `5`; textures: `1`
- Root scenes: `Start, Clear, Failed, Game_over`
- Component roles: `loading_device=4`
- Texture-backed preview commands: `10`
- Composed scene draw commands: `26`
- Animation keyframes: `120`
- Key scenes:
  - `Start` -> `loading_device` casts=27 anims=2 frames=[270.0, 270.0]
  - `Clear` -> `loading_device` casts=6 anims=1 frames=[155.0, 155.0]
  - `Failed` -> `loading_device` casts=3 anims=1 frames=[120.0, 120.0]
  - `Game_over` -> `loading_device` casts=3 anims=1 frames=[50.0, 50.0]

### `ui_loading`
- Path: `game/Loading/ui_loading.yncp`
- Scenes: `7`; casts: `331`; animations: `37`; textures: `10`
- Root scenes: `bg_1, loadinfo, n_2_d, event_viewer, pda, pda_txt, bg_2`
- Component roles: `hud_life=5, loading_device=2`
- Texture-backed preview commands: `28`
- Composed scene draw commands: `199`
- Animation keyframes: `773`
- Key scenes:
  - `bg_1` -> `hud_life` casts=28 anims=2 frames=[40.0, 45.0]
  - `loadinfo` -> `hud_life` casts=106 anims=13 frames=[2.0, 2.0]
  - `n_2_d` -> `hud_life` casts=10 anims=3 frames=[75.0, 128.0]
  - `event_viewer` -> `hud_life` casts=83 anims=1 frames=[80.0, 80.0]
  - `pda` -> `loading_device` casts=57 anims=8 frames=[40.0, 240.0]
  - `pda_txt` -> `loading_device` casts=19 anims=8 frames=[15.0, 240.0]
  - `bg_2` -> `hud_life` casts=28 anims=2 frames=[45.0, 51.0]

## Pause

### `ui_help`
- Path: `game/SystemCommon/ui_help.yncp`
- Scenes: `6`; casts: `30`; animations: `11`; textures: `7`
- Root scenes: ``
- Component roles: `town_dialog=6`
- Texture-backed preview commands: `21`
- Composed scene draw commands: `4`
- Animation keyframes: `52`
- Key scenes:
  - `help_window` -> `town_dialog` casts=13 anims=1 frames=[10.0, 10.0]
  - `help_text_area` -> `town_dialog` casts=4 anims=1 frames=[0.0, 0.0]
  - `help_nametag` -> `town_dialog` casts=7 anims=3 frames=[15.0, 15.0]
  - `help_chara_1` -> `town_dialog` casts=2 anims=2 frames=[0.0, 15.0]
  - `help_chara_2` -> `town_dialog` casts=2 anims=2 frames=[0.0, 15.0]
  - `help_chara_3` -> `town_dialog` casts=2 anims=2 frames=[0.0, 15.0]

### `ui_general`
- Path: `game/SystemCommonCore/ui_general.yncp`
- Scenes: `4`; casts: `26`; animations: `10`; textures: `3`
- Root scenes: `bg, window, window_select, footer`
- Component roles: `menu_choices=2, pause_shell=1, result_rank=1`
- Texture-backed preview commands: `11`
- Composed scene draw commands: `19`
- Animation keyframes: `53`
- Key scenes:
  - `bg` -> `result_rank` casts=1 anims=1 frames=[10.0, 10.0]
  - `window` -> `pause_shell` casts=15 anims=2 frames=[20.0, 100.0]
  - `window_select` -> `menu_choices` casts=4 anims=3 frames=[60.0, 200.0]
  - `footer` -> `menu_choices` casts=6 anims=4 frames=[0.0, 0.0]

### `ui_pause`
- Path: `game/SystemCommonCore/ui_pause.yncp`
- Scenes: `29`; casts: `260`; animations: `41`; textures: `9`
- Root scenes: `bg`
- Component roles: `pause_shell=23, status_skill=5, title_logo=1`
- Texture-backed preview commands: `82`
- Composed scene draw commands: `151`
- Animation keyframes: `559`
- Key scenes:
  - `bg` -> `pause_shell` casts=1 anims=1 frames=[15.0, 15.0]
  - `bg_1` -> `pause_shell` casts=14 anims=2 frames=[20.0, 27.0]
  - `bg_1_select` -> `pause_shell` casts=4 anims=3 frames=[27.0, 120.0]
  - `bg_2` -> `pause_shell` casts=68 anims=2 frames=[15.0, 20.0]
  - `text_area` -> `pause_shell` casts=3 anims=1 frames=[200.0, 200.0]
  - `skill_select` -> `pause_shell` casts=27 anims=1 frames=[60.0, 60.0]
  - `arrow` -> `pause_shell` casts=2 anims=1 frames=[200.0, 200.0]
  - `skill_scroll_bar_bg` -> `pause_shell` casts=6 anims=1 frames=[20.0, 20.0]
  - `skill_scroll_bar` -> `pause_shell` casts=4 anims=1 frames=[100.0, 100.0]
  - `bg_3` -> `pause_shell` casts=16 anims=1 frames=[20.0, 20.0]
  - `bg_4` -> `pause_shell` casts=14 anims=1 frames=[20.0, 20.0]
  - `scroll_bar_bg` -> `pause_shell` casts=6 anims=1 frames=[20.0, 20.0]

## Result

### `ui_result`
- Path: `game/ActionCommon/ui_result.yncp`
- Scenes: `17`; casts: `290`; animations: `29`; textures: `7`
- Root scenes: ``
- Component roles: `menu_choices=1, result_rank=15, title_logo=1`
- Texture-backed preview commands: `56`
- Composed scene draw commands: `176`
- Animation keyframes: `1516`
- Key scenes:
  - `result_title` -> `title_logo` casts=10 anims=2 frames=[85.0, 85.0]
  - `result_num_1` -> `result_rank` casts=22 anims=2 frames=[40.0, 40.0]
  - `result_num_2` -> `result_rank` casts=22 anims=2 frames=[49.0, 49.0]
  - `result_num_3` -> `result_rank` casts=22 anims=2 frames=[60.0, 60.0]
  - `result_num_4` -> `result_rank` casts=22 anims=2 frames=[69.0, 69.0]
  - `result_num_5` -> `result_rank` casts=22 anims=2 frames=[76.0, 76.0]
  - `result_num_6` -> `result_rank` casts=22 anims=2 frames=[93.0, 93.0]
  - `result_newR` -> `menu_choices` casts=51 anims=2 frames=[57.0, 57.0]
  - `result_newR_position` -> `result_rank` casts=6 anims=1 frames=[0.0, 0.0]
  - `result_rank` -> `result_rank` casts=1 anims=2 frames=[10.0, 100.0]
  - `result_rank_E` -> `result_rank` casts=32 anims=1 frames=[253.0, 253.0]
  - `result_rank_D` -> `result_rank` casts=2 anims=1 frames=[45.0, 45.0]

### `ui_result_ex`
- Path: `game/ExStageTails_Common/ui_result_ex.yncp`
- Scenes: `15`; casts: `204`; animations: `20`; textures: `7`
- Root scenes: ``
- Component roles: `menu_choices=1, result_rank=13, title_logo=1`
- Texture-backed preview commands: `44`
- Composed scene draw commands: `123`
- Animation keyframes: `907`
- Key scenes:
  - `result_title` -> `title_logo` casts=10 anims=1 frames=[85.0, 85.0]
  - `result_num_1` -> `result_rank` casts=35 anims=1 frames=[92.0, 92.0]
  - `result_num` -> `result_rank` casts=10 anims=1 frames=[93.0, 93.0]
  - `result_tag_1` -> `result_rank` casts=35 anims=1 frames=[70.0, 70.0]
  - `result_tag` -> `result_rank` casts=10 anims=1 frames=[70.0, 70.0]
  - `result_newR` -> `menu_choices` casts=7 anims=2 frames=[0.0, 57.0]
  - `result_newR_position` -> `result_rank` casts=6 anims=1 frames=[0.0, 0.0]
  - `result_rank` -> `result_rank` casts=1 anims=2 frames=[100.0, 200.0]
  - `result_rank_E` -> `result_rank` casts=32 anims=1 frames=[253.0, 253.0]
  - `result_rank_D` -> `result_rank` casts=2 anims=1 frames=[45.0, 45.0]
  - `result_rank_C` -> `result_rank` casts=2 anims=1 frames=[20.0, 20.0]
  - `result_rank_B` -> `result_rank` casts=11 anims=2 frames=[75.0, 120.0]

### `ui_itemresult`
- Path: `game/SystemCommon/ui_itemresult.yncp`
- Scenes: `4`; casts: `73`; animations: `16`; textures: `8`
- Root scenes: ``
- Component roles: `result_rank=3, title_logo=1`
- Texture-backed preview commands: `15`
- Composed scene draw commands: `49`
- Animation keyframes: `181`
- Key scenes:
  - `iresult_title` -> `title_logo` casts=10 anims=6 frames=[26.0, 85.0]
  - `window` -> `result_rank` casts=36 anims=6 frames=[5.0, 5.0]
  - `contents` -> `result_rank` casts=25 anims=3 frames=[60.0, 60.0]
  - `result_footer` -> `result_rank` casts=2 anims=1 frames=[100.0, 100.0]

## Sonic Hud

### `ui_playscreen_su`
- Path: `game/BossDarkGaia1_1Air/ui_playscreen_su.yncp`
- Scenes: `3`; casts: `34`; animations: `6`; textures: `5`
- Root scenes: `su_sonic_gauge, gaia_gauge, footer`
- Component roles: `hud_speed=2, pause_shell=1`
- Texture-backed preview commands: `12`
- Composed scene draw commands: `31`
- Animation keyframes: `33`
- Key scenes:
  - `su_sonic_gauge` -> `hud_speed` casts=14 anims=2 frames=[30.0, 100.0]
  - `gaia_gauge` -> `hud_speed` casts=14 anims=3 frames=[30.0, 100.0]
  - `footer` -> `pause_shell` casts=6 anims=1 frames=[100.0, 100.0]

### `ui_playscreen_ev_hit`
- Path: `game/BossFinalDarkGaia/ui_playscreen_ev_hit.yncp`
- Scenes: `5`; casts: `63`; animations: `19`; textures: `2`
- Root scenes: `hit_counter_bg, hit_counter_num, hit_counter_txt_1, hit_counter_txt_2, chance_attack`
- Component roles: `hud_score_time=4, scene_chance-attack=1`
- Texture-backed preview commands: `19`
- Composed scene draw commands: `48`
- Animation keyframes: `279`
- Key scenes:
  - `hit_counter_bg` -> `hud_score_time` casts=7 anims=4 frames=[10.0, 12.0]
  - `hit_counter_num` -> `hud_score_time` casts=7 anims=2 frames=[10.0, 20.0]
  - `hit_counter_txt_1` -> `hud_score_time` casts=9 anims=5 frames=[12.0, 12.0]
  - `hit_counter_txt_2` -> `hud_score_time` casts=16 anims=2 frames=[30.0, 30.0]
  - `chance_attack` -> `scene_chance-attack` casts=24 anims=6 frames=[30.0, 30.0]

### `ui_playscreen_su`
- Path: `game/BossFinalDarkGaia/ui_playscreen_su.yncp`
- Scenes: `3`; casts: `34`; animations: `6`; textures: `5`
- Root scenes: `su_sonic_gauge, gaia_gauge, footer`
- Component roles: `hud_speed=2, pause_shell=1`
- Texture-backed preview commands: `12`
- Composed scene draw commands: `31`
- Animation keyframes: `33`
- Key scenes:
  - `su_sonic_gauge` -> `hud_speed` casts=14 anims=2 frames=[30.0, 100.0]
  - `gaia_gauge` -> `hud_speed` casts=14 anims=3 frames=[30.0, 100.0]
  - `footer` -> `pause_shell` casts=6 anims=1 frames=[100.0, 100.0]

### `ui_playscreen_ev_hit`
- Path: `game/EvilActionCommon/ui_playscreen_ev_hit.yncp`
- Scenes: `5`; casts: `64`; animations: `19`; textures: `3`
- Root scenes: `hit_counter_bg, hit_counter_num, hit_counter_txt_1, hit_counter_txt_2, chance_attack`
- Component roles: `hud_score_time=4, scene_chance-attack=1`
- Texture-backed preview commands: `19`
- Composed scene draw commands: `49`
- Animation keyframes: `281`
- Key scenes:
  - `hit_counter_bg` -> `hud_score_time` casts=7 anims=4 frames=[10.0, 12.0]
  - `hit_counter_num` -> `hud_score_time` casts=7 anims=2 frames=[10.0, 20.0]
  - `hit_counter_txt_1` -> `hud_score_time` casts=9 anims=5 frames=[12.0, 12.0]
  - `hit_counter_txt_2` -> `hud_score_time` casts=17 anims=2 frames=[30.0, 30.0]
  - `chance_attack` -> `scene_chance-attack` casts=24 anims=6 frames=[30.0, 30.0]

### `ui_playscreen_ev`
- Path: `game/EvilSonic/ui_playscreen_ev.yncp`
- Scenes: `33`; casts: `280`; animations: `60`; textures: `11`
- Root scenes: `player_count, score_count, ring_count, ring_get, exp_count`
- Component roles: `hud_life=2, hud_ring=2, hud_score_time=1, hud_speed=24, result_rank=3, status_skill=1`
- Texture-backed preview commands: `103`
- Composed scene draw commands: `159`
- Animation keyframes: `923`
- Key scenes:
  - `player_count` -> `hud_life` casts=3 anims=1 frames=[100.0, 100.0]
  - `score_count` -> `hud_score_time` casts=12 anims=1 frames=[100.0, 100.0]
  - `ring_count` -> `hud_ring` casts=12 anims=1 frames=[100.0, 100.0]
  - `ring_get` -> `hud_ring` casts=4 anims=1 frames=[29.0, 29.0]
  - `exp_count` -> `status_skill` casts=22 anims=2 frames=[20.0, 100.0]
  - `u_info` -> `result_rank` casts=16 anims=3 frames=[20.0, 30.0]
  - `medal_get_s` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]
  - `medal_get_m` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]
  - `unleash_bg` -> `hud_speed` casts=5 anims=1 frames=[100.0, 100.0]
  - `life_bg` -> `hud_life` casts=5 anims=1 frames=[100.0, 100.0]
  - `unleash_body` -> `hud_speed` casts=6 anims=2 frames=[100.0, 100.0]
  - `unleash_bar_1` -> `hud_speed` casts=4 anims=2 frames=[100.0, 100.0]

### `ui_playscreen`
- Path: `game/Sonic/ui_playscreen.yncp`
- Scenes: `13`; casts: `209`; animations: `18`; textures: `12`
- Root scenes: `so_speed_gauge, so_ringenagy_gauge, gauge_frame, player_count, time_count, score_count, ring_count, ring_get, exp_count`
- Component roles: `hud_life=1, hud_ring=3, hud_score_time=2, hud_speed=3, result_rank=3, status_skill=1`
- Texture-backed preview commands: `45`
- Composed scene draw commands: `145`
- Animation keyframes: `239`
- Key scenes:
  - `so_speed_gauge` -> `hud_speed` casts=47 anims=1 frames=[100.0, 100.0]
  - `so_ringenagy_gauge` -> `hud_ring` casts=43 anims=2 frames=[100.0, 100.0]
  - `gauge_frame` -> `hud_speed` casts=20 anims=1 frames=[100.0, 100.0]
  - `player_count` -> `hud_life` casts=3 anims=1 frames=[100.0, 100.0]
  - `time_count` -> `hud_score_time` casts=16 anims=1 frames=[100.0, 100.0]
  - `score_count` -> `hud_score_time` casts=12 anims=1 frames=[100.0, 100.0]
  - `ring_count` -> `hud_ring` casts=1 anims=1 frames=[100.0, 100.0]
  - `ring_get` -> `hud_ring` casts=3 anims=2 frames=[10.0, 60.0]
  - `exp_count` -> `status_skill` casts=22 anims=2 frames=[20.0, 100.0]
  - `u_info` -> `result_rank` casts=16 anims=3 frames=[20.0, 30.0]
  - `medal_get_s` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]
  - `medal_get_m` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]

### `ui_playscreen`
- Path: `game/SuperSonic/ui_playscreen.yncp`
- Scenes: `13`; casts: `208`; animations: `16`; textures: `12`
- Root scenes: `so_speed_gauge, so_ringenagy_gauge, gauge_frame, player_count, time_count, score_count, ring_count, ring_get, exp_count`
- Component roles: `hud_life=1, hud_ring=3, hud_score_time=2, hud_speed=3, result_rank=3, status_skill=1`
- Texture-backed preview commands: `45`
- Composed scene draw commands: `145`
- Animation keyframes: `167`
- Key scenes:
  - `so_speed_gauge` -> `hud_speed` casts=47 anims=1 frames=[100.0, 100.0]
  - `so_ringenagy_gauge` -> `hud_ring` casts=43 anims=2 frames=[100.0, 100.0]
  - `gauge_frame` -> `hud_speed` casts=20 anims=1 frames=[100.0, 100.0]
  - `player_count` -> `hud_life` casts=3 anims=1 frames=[100.0, 100.0]
  - `time_count` -> `hud_score_time` casts=16 anims=1 frames=[100.0, 100.0]
  - `score_count` -> `hud_score_time` casts=12 anims=1 frames=[100.0, 100.0]
  - `ring_count` -> `hud_ring` casts=1 anims=1 frames=[100.0, 100.0]
  - `ring_get` -> `hud_ring` casts=3 anims=2 frames=[5.0, 60.0]
  - `exp_count` -> `status_skill` casts=22 anims=2 frames=[20.0, 100.0]
  - `u_info` -> `result_rank` casts=16 anims=1 frames=[20.0, 20.0]
  - `medal_get_s` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]
  - `medal_get_m` -> `result_rank` casts=5 anims=1 frames=[5.0, 5.0]

## Status

### `ui_status`
- Path: `game/SystemCommonCore/ui_status.yncp`
- Scenes: `42`; casts: `227`; animations: `109`; textures: `15`
- Root scenes: ``
- Component roles: `result_rank=2, status_skill=39, title_logo=1`
- Texture-backed preview commands: `74`
- Composed scene draw commands: `123`
- Animation keyframes: `1030`
- Key scenes:
  - `logo` -> `status_skill` casts=11 anims=3 frames=[20.0, 20.0]
  - `a_efc_1` -> `status_skill` casts=2 anims=2 frames=[60.0, 60.0]
  - `a_efc_2` -> `status_skill` casts=1 anims=2 frames=[0.0, 0.0]
  - `a_efc_3` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]
  - `a_efc_4` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]
  - `a_efc_5` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]
  - `a_efc_6` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]
  - `prgs_bg_1` -> `status_skill` casts=7 anims=4 frames=[15.0, 50.0]
  - `prgs_bg_2` -> `status_skill` casts=10 anims=6 frames=[10.0, 60.0]
  - `prgs_bg_3` -> `status_skill` casts=1 anims=2 frames=[0.0, 0.0]
  - `prgs_bg_4` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]
  - `prgs_bg_5` -> `status_skill` casts=1 anims=1 frames=[0.0, 0.0]

## Title

### `ui_mainmenu`
- Path: `game/MainMenu/ui_mainmenu.xncp`
- Scenes: `16`; casts: `399`; animations: `20`; textures: `3`
- Root scenes: `mm_bg_usual, mm_bg_intro, mm_donut_idle, mm_donut_select, mm_donut_move, mm_donut_intro, mm_contentsitem_select, mm_contentsitem_idle, mm_contentsitem_move, mm_contentsitem_intro, mm_contentsitem_text, mm_base, mm_title_usual, mm_title_intro, mm_front_usual, mm_front_intro`
- Component roles: `menu_choices=2, result_rank=2, scene_mm-base=1, scene_mm-contentsitem-idle=1, scene_mm-contentsitem-intro=1, scene_mm-contentsitem-move=1, scene_mm-contentsitem-text=1, scene_mm-donut-idle=1, scene_mm-donut-intro=1, scene_mm-donut-move=1, scene_mm-front-intro=1, scene_mm-front-usual=1, title_logo=2`
- Texture-backed preview commands: `61`
- Composed scene draw commands: `287`
- Animation keyframes: `677`
- Key scenes:
  - `mm_bg_usual` -> `result_rank` casts=47 anims=1 frames=[120.0, 120.0]
  - `mm_bg_intro` -> `result_rank` casts=99 anims=1 frames=[60.0, 60.0]
  - `mm_donut_idle` -> `scene_mm-donut-idle` casts=9 anims=1 frames=[120.0, 120.0]
  - `mm_donut_select` -> `menu_choices` casts=9 anims=1 frames=[15.0, 15.0]
  - `mm_donut_move` -> `scene_mm-donut-move` casts=9 anims=1 frames=[220.0, 220.0]
  - `mm_donut_intro` -> `scene_mm-donut-intro` casts=9 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_select` -> `menu_choices` casts=19 anims=1 frames=[15.0, 15.0]
  - `mm_contentsitem_idle` -> `scene_mm-contentsitem-idle` casts=13 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_move` -> `scene_mm-contentsitem-move` casts=46 anims=1 frames=[40.0, 40.0]
  - `mm_contentsitem_intro` -> `scene_mm-contentsitem-intro` casts=28 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_text` -> `scene_mm-contentsitem-text` casts=35 anims=5 frames=[15.0, 60.0]
  - `mm_base` -> `scene_mm-base` casts=3 anims=1 frames=[60.0, 60.0]

### `ui_mainmenu`
- Path: `game/MainMenu/ui_mainmenu.yncp`
- Scenes: `16`; casts: `399`; animations: `20`; textures: `3`
- Root scenes: `mm_bg_usual, mm_bg_intro, mm_donut_idle, mm_donut_select, mm_donut_move, mm_donut_intro, mm_contentsitem_select, mm_contentsitem_idle, mm_contentsitem_move, mm_contentsitem_intro, mm_contentsitem_text, mm_base, mm_title_usual, mm_title_intro, mm_front_usual, mm_front_intro`
- Component roles: `menu_choices=2, result_rank=2, scene_mm-base=1, scene_mm-contentsitem-idle=1, scene_mm-contentsitem-intro=1, scene_mm-contentsitem-move=1, scene_mm-contentsitem-text=1, scene_mm-donut-idle=1, scene_mm-donut-intro=1, scene_mm-donut-move=1, scene_mm-front-intro=1, scene_mm-front-usual=1, title_logo=2`
- Texture-backed preview commands: `61`
- Composed scene draw commands: `287`
- Animation keyframes: `677`
- Key scenes:
  - `mm_bg_usual` -> `result_rank` casts=47 anims=1 frames=[120.0, 120.0]
  - `mm_bg_intro` -> `result_rank` casts=99 anims=1 frames=[60.0, 60.0]
  - `mm_donut_idle` -> `scene_mm-donut-idle` casts=9 anims=1 frames=[120.0, 120.0]
  - `mm_donut_select` -> `menu_choices` casts=9 anims=1 frames=[15.0, 15.0]
  - `mm_donut_move` -> `scene_mm-donut-move` casts=9 anims=1 frames=[220.0, 220.0]
  - `mm_donut_intro` -> `scene_mm-donut-intro` casts=9 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_select` -> `menu_choices` casts=19 anims=1 frames=[15.0, 15.0]
  - `mm_contentsitem_idle` -> `scene_mm-contentsitem-idle` casts=13 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_move` -> `scene_mm-contentsitem-move` casts=46 anims=1 frames=[40.0, 40.0]
  - `mm_contentsitem_intro` -> `scene_mm-contentsitem-intro` casts=28 anims=1 frames=[60.0, 60.0]
  - `mm_contentsitem_text` -> `scene_mm-contentsitem-text` casts=35 anims=5 frames=[15.0, 60.0]
  - `mm_base` -> `scene_mm-base` casts=3 anims=1 frames=[60.0, 60.0]

### `ui_title`
- Path: `game/Title/ui_title.yncp`
- Scenes: `8`; casts: `502`; animations: `49`; textures: `7`
- Root scenes: `bg, logo, title_1, title_2, menu, menu_scroll, txt, progress`
- Component roles: `title_logo=8`
- Texture-backed preview commands: `28`
- Composed scene draw commands: `265`
- Animation keyframes: `1447`
- Key scenes:
  - `bg` -> `title_logo` casts=14 anims=5 frames=[60.0, 120.0]
  - `logo` -> `title_logo` casts=2 anims=2 frames=[180.0, 180.0]
  - `title_1` -> `title_logo` casts=14 anims=3 frames=[5.0, 300.0]
  - `title_2` -> `title_logo` casts=14 anims=3 frames=[5.0, 105.0]
  - `menu` -> `title_logo` casts=203 anims=21 frames=[15.0, 180.0]
  - `menu_scroll` -> `title_logo` casts=188 anims=8 frames=[15.0, 15.0]
  - `txt` -> `title_logo` casts=64 anims=4 frames=[55.0, 55.0]
  - `progress` -> `title_logo` casts=3 anims=3 frames=[55.0, 100.0]

### `ui_title`
- Path: `game/TitleE3/ui_title.yncp`
- Scenes: `9`; casts: `418`; animations: `41`; textures: `5`
- Root scenes: `sample, bg, logo, title_1, title_2, menu, menu_scroll, txt, progress`
- Component roles: `title_logo=9`
- Texture-backed preview commands: `29`
- Composed scene draw commands: `251`
- Animation keyframes: `790`
- Key scenes:
  - `sample` -> `title_logo` casts=1 anims=1 frames=[100.0, 100.0]
  - `bg` -> `title_logo` casts=14 anims=5 frames=[60.0, 120.0]
  - `logo` -> `title_logo` casts=2 anims=2 frames=[180.0, 180.0]
  - `title_1` -> `title_logo` casts=16 anims=3 frames=[5.0, 300.0]
  - `title_2` -> `title_logo` casts=20 anims=3 frames=[5.0, 105.0]
  - `menu` -> `title_logo` casts=158 anims=16 frames=[15.0, 180.0]
  - `menu_scroll` -> `title_logo` casts=159 anims=6 frames=[15.0, 15.0]
  - `txt` -> `title_logo` casts=45 anims=3 frames=[50.0, 55.0]
  - `progress` -> `title_logo` casts=3 anims=2 frames=[55.0, 100.0]

## World Map

### `ui_worldmap`
- Path: `dlc/Apotos & Shamar Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Apotos & Shamar Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `dlc/Chun-nan Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Chun-nan Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `dlc/Empire City & Adabat Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Empire City & Adabat Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `dlc/Holoska Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Holoska Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `dlc/Mazuri Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Mazuri Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `dlc/Spagonia Adventure Pack/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `dlc/Spagonia Adventure Pack/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

### `ui_worldmap`
- Path: `game/WorldMap/ui_worldmap.yncp`
- Scenes: `35`; casts: `1173`; animations: `65`; textures: `37`
- Root scenes: `worldmap_background`
- Component roles: `world_map_marker=35`
- Texture-backed preview commands: `112`
- Composed scene draw commands: `637`
- Animation keyframes: `1082`
- Key scenes:
  - `worldmap_background` -> `world_map_marker` casts=47 anims=2 frames=[0.0, 60.0]
  - `info_bg_1` -> `world_map_marker` casts=212 anims=2 frames=[0.0, 140.0]
  - `cts_info_bg` -> `world_map_marker` casts=120 anims=2 frames=[0.0, 150.0]
  - `info_img_1` -> `world_map_marker` casts=5 anims=4 frames=[20.0, 125.0]
  - `info_img_2` -> `world_map_marker` casts=2 anims=2 frames=[60.0, 180.0]
  - `info_img_3` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 190.0]
  - `info_img_4` -> `world_map_marker` casts=4 anims=2 frames=[60.0, 200.0]
  - `cts_cursor_effect` -> `world_map_marker` casts=7 anims=1 frames=[65.0, 65.0]
  - `cts_name` -> `world_map_marker` casts=9 anims=2 frames=[50.0, 60.0]
  - `cts_cursor` -> `world_map_marker` casts=44 anims=2 frames=[71.0, 221.0]
  - `cts_guide_bg` -> `world_map_marker` casts=138 anims=2 frames=[0.0, 20.0]
  - `cts_guide_window` -> `world_map_marker` casts=195 anims=3 frames=[0.0, 65.0]

### `ui_worldmap_help`
- Path: `game/WorldMap/ui_worldmap_help.yncp`
- Scenes: `3`; casts: `22`; animations: `4`; textures: `5`
- Root scenes: ``
- Component roles: `world_map_marker=3`
- Texture-backed preview commands: `9`
- Composed scene draw commands: `5`
- Animation keyframes: `44`
- Key scenes:
  - `help_window` -> `world_map_marker` casts=14 anims=1 frames=[40.0, 40.0]
  - `help_text_area` -> `world_map_marker` casts=2 anims=1 frames=[0.0, 0.0]
  - `help_chara_2` -> `world_map_marker` casts=6 anims=2 frames=[10.0, 62.0]

## Runtime-Hook + Ghidra Xref SFX Candidates

- `ui_title/menu` `se_system_worldmap/sys_worldmap_window` via `CTitleStateMenu::Update/Game_PlaySound`; xref `sub_825882B8 -> Game_PlaySound("sys_worldmap_window")`
- `ui_title/menu` `se_system_worldmap/sys_worldmap_decide` via `CTitleStateMenu::Update/Game_PlaySound`; xref `sub_825882B8 -> Game_PlaySound("sys_worldmap_decide")`
- `ui_title/menu` `se_system_worldmap/sys_worldmap_cansel` via `CTitleStateMenu::Update/Game_PlaySound`; xref `sub_825882B8 -> Game_PlaySound("sys_worldmap_cansel")`
- `ui_mainmenu/mm_contentsitem_select` `se_system_worldmap/sys_worldmap_decide` via `CTitleStateMenu::Update/Game_PlaySound`; xref `sub_825882B8 -> Game_PlaySound("sys_worldmap_decide")`

