/* ============================================================
   contracts.js  —  reusable UI screen contracts
   Translated from research_uiux/runtime_reference/contracts/*.json
   plus timing bands recovered in the UI animation/transition notes.
   These are ARCHITECTURE + TIMING only (no proprietary assets).
   Attached to window.SWARD_CONTRACTS for the babel scripts.
   ============================================================ */
(function () {
  // helper to build a state row
  const S = (state, enter, band, target, input, debug) => ({
    state, debug_name: debug || state, enter_scene: enter || "",
    timeline_band_id: band || null, timeout_target: target || null,
    input_enabled: !!input,
  });

  const CONTRACTS = {
    /* ---------------- FRONT-END ---------------- */
    TitleMenu: {
      screen_id: "TitleMenuReference", source_system: "title_stack",
      source_files: ["CTitleStateIntro_patches.cpp", "CTitleStateMenu_patches.cpp", "ui_mainmenu.yncp"],
      notes: "Title intro hands into a layered menu host (background + content item move + donut select). Custom options/modals layer over an authoritative original menu cursor.",
      timeline_bands: [
        { id: "intro_medium", seconds: 0.333333 },
        { id: "select_travel", seconds: 0.333333 },
        { id: "confirm_hold", seconds: 0.45 },
        { id: "outro_fade", seconds: 1.0 },
      ],
      states: [
        S("Boot"), S("Intro", "mm_bg_intro", "intro_medium", "Idle", false),
        S("Idle", "mm_idle", null, null, true),
        S("Navigate", "mm_contentsitem_move", "select_travel", "Idle", false),
        S("Confirm", "mm_donut_select", "confirm_hold", "Outro", false),
        S("Outro", "fade_out", "outro_fade", "Closed", false), S("Closed"),
      ],
      overlay_layers: roles(["backdrop", "chrome", "content", "prompt", "transient_fx"]),
      visible_overlay_roles: {
        Intro: ["backdrop", "chrome"], Idle: ["backdrop", "chrome", "content", "prompt"],
        Navigate: ["backdrop", "chrome", "content", "prompt", "transient_fx"],
        Confirm: ["backdrop", "chrome", "content", "transient_fx"], Outro: ["backdrop"],
      },
      prompt_slots: [
        ps("confirm_primary", "A", "Select", ["Idle"], ["can_confirm"]),
        ps("start", "START", "Start", ["Idle"], ["intro_done"]),
      ],
      walk: ["Boot", "Intro", "Idle", "Navigate", "Idle", "Confirm", "Outro", "Closed"],
    },

    PauseMenu: {
      screen_id: "PauseMenuReference", source_system: "pause_stack",
      source_files: ["CHudPause_patches.cpp", "ui_general.yncp", "ui_pause.yncp", "options_menu.cpp"],
      notes: "Framed pause shell with tab paging, prompt gating and confirm/cancel outro handoff. Original pause HUD stays host; submenus inject at cursor/action seams.",
      timeline_bands: [
        { id: "intro_medium", seconds: 0.333333 }, { id: "select_travel", seconds: 0.333333 },
        { id: "confirm_hold", seconds: 0.45 }, { id: "cancel_hold", seconds: 0.25 },
        { id: "outro_medium", seconds: 0.333333 },
      ],
      states: [
        S("Boot"), S("Intro", "reveal", "intro_medium", "Idle", false),
        S("Idle", "idle", null, null, true),
        S("Navigate", "focus_pulse", "select_travel", "Idle", false),
        S("Confirm", "confirm", "confirm_hold", "Outro", false),
        S("Cancel", "cancel", "cancel_hold", "Outro", false),
        S("Outro", "outro", "outro_medium", "Closed", false), S("Closed"),
      ],
      overlay_layers: roles(["backdrop", "chrome", "content", "prompt", "transient_fx"]),
      visible_overlay_roles: {
        Intro: ["backdrop", "chrome", "content"], Idle: ["backdrop", "chrome", "content", "prompt"],
        Navigate: ["backdrop", "chrome", "content", "prompt", "transient_fx"],
        Confirm: ["backdrop", "chrome", "content", "transient_fx"],
        Cancel: ["backdrop", "chrome", "content", "transient_fx"], Outro: ["backdrop", "chrome", "content"],
      },
      prompt_slots: [
        ps("confirm_primary", "A", "Select", ["Idle"], ["can_confirm"]),
        ps("cancel_secondary", "B", "Back", ["Idle"], ["can_cancel"]),
        ps("page_left", "LB", "Prev Tab", ["Idle"], ["has_previous_tab"]),
        ps("page_right", "RB", "Next Tab", ["Idle"], ["has_next_tab"]),
      ],
      walk: ["Boot", "Intro", "Idle", "Navigate", "Idle", "Confirm", "Outro", "Closed"],
    },

    OptionsMenu: {
      screen_id: "OptionsMenuReference", source_system: "options_stack",
      source_files: ["options_menu.cpp", "options_menu_thumbnails.cpp", "tv_static.cpp", "button_guide.cpp"],
      notes: "Category rail + option list + marquee info panel + thumbnail panel (TV static). Accept is buffered on open; CanClose() gates B while locked on an option. CONTAINER_CATEGORY_DURATION=12.0, INFO_TEXT_MARQUEE_DELAY=1.2.",
      timeline_bands: [
        { id: "appear", seconds: 0.45 }, { id: "category_travel", seconds: 0.333333 },
        { id: "marquee_delay", seconds: 1.2 }, { id: "lock_edit", seconds: 0.25 },
        { id: "close", seconds: 0.333333 },
      ],
      states: [
        S("Boot"), S("Intro", "appear", "appear", "Idle", false, "Opening"),
        S("Idle", "category_browse", null, null, true, "CategoryBrowse"),
        S("Navigate", "option_browse", "category_travel", "Idle", false, "OptionBrowse"),
        S("Confirm", "lock_on_option", "lock_edit", "Idle", false, "LockedEdit"),
        S("Outro", "closing", "close", "Closed", false, "Closing"), S("Closed", "", null, null, false, "RestartFlag?"),
      ],
      overlay_layers: roles(["backdrop", "rail", "content", "info", "thumbnail", "prompt"]),
      visible_overlay_roles: {
        Intro: ["backdrop", "rail", "content"],
        Idle: ["backdrop", "rail", "content", "info", "thumbnail", "prompt"],
        Navigate: ["backdrop", "rail", "content", "info", "thumbnail", "prompt"],
        Confirm: ["backdrop", "rail", "content", "info", "thumbnail", "prompt"],
        Outro: ["backdrop", "rail", "content"],
      },
      prompt_slots: [
        ps("confirm_primary", "A", "Edit", ["Idle"], ["can_confirm"]),
        ps("cancel_secondary", "B", "Back", ["Idle"], ["can_close"]),
        ps("cat_prev", "LB", "Prev Category", ["Idle"], ["has_prev_cat"]),
        ps("cat_next", "RB", "Next Category", ["Idle"], ["has_next_cat"]),
      ],
      walk: ["Boot", "Intro", "Idle", "Navigate", "Confirm", "Idle", "Outro", "Closed"],
    },

    WorldMap: {
      screen_id: "WorldMapReference", source_system: "world_map_stack",
      source_files: ["input_patches.cpp", "aspect_ratio_patches.cpp", "ui_worldmap.yncp", "ui_worldmap_help.yncp"],
      notes: "Layered info-canvas: header/footer bands, info pane, choices pane, help sidecar. Dense 221-frame cursor intro. Reversible Switch_Anim drives focus/unfocus of info surfaces.",
      timeline_bands: [
        { id: "cursor_intro", seconds: 3.683333 }, { id: "pane_travel", seconds: 0.6 },
        { id: "decision_flash", seconds: 0.5 }, { id: "cancel_close", seconds: 0.333333 },
        { id: "map_out", seconds: 0.4 },
      ],
      states: [
        S("Boot"), S("Intro", "cursor_reveal", "cursor_intro", "Idle", false),
        S("Idle", "worldmap_idle", null, null, true),
        S("Navigate", "pane_scroll", "pane_travel", "Idle", false),
        S("Confirm", "decision_flash", "decision_flash", "Idle", false),
        S("Cancel", "help_close", "cancel_close", "Outro", false),
        S("Outro", "worldmap_out", "map_out", "Closed", false), S("Closed"),
      ],
      overlay_layers: roles(["backdrop", "header", "content", "help_sidecar", "prompt", "transient_fx"]),
      visible_overlay_roles: {
        Intro: ["backdrop", "header", "content", "help_sidecar", "transient_fx"],
        Idle: ["backdrop", "header", "content", "help_sidecar", "prompt"],
        Navigate: ["backdrop", "header", "content", "help_sidecar", "prompt", "transient_fx"],
        Confirm: ["backdrop", "header", "content", "help_sidecar", "transient_fx"],
        Cancel: ["backdrop", "header", "content", "help_sidecar", "transient_fx"],
        Outro: ["backdrop", "header", "content", "transient_fx"],
      },
      prompt_slots: [
        ps("confirm_primary", "A", "Select", ["Idle"], ["can_confirm"]),
        ps("cancel_secondary", "B", "Back", ["Idle"], ["can_cancel"]),
        ps("details", "X", "Details", ["Idle"], ["show_details_prompt"]),
        ps("help_toggle", "RB", "Help", ["Idle"], ["show_help_prompt"]),
      ],
      walk: ["Boot", "Intro", "Idle", "Navigate", "Idle", "Confirm", "Idle", "Outro", "Closed"],
    },

    LoadingTransition: {
      screen_id: "LoadingTransitionReference", source_system: "loading_stack",
      source_files: ["resident_patches.cpp", "ui_loading.yncp", "black_bar.cpp"],
      notes: "Host-driven, non-interactive. Long pda_txt presentation banks (240f / 4.0s) loop while ultra-short (2f) platform/controller swaps select the prompt-icon variant. d_2_n / n_2_d time-of-day branch. CLoading::Update drives display type back to zero to exit.",
      timeline_bands: [
        { id: "intro", seconds: 0.5 }, { id: "card_loop", seconds: 4.0 },
        { id: "platform_swap", seconds: 0.033333 }, { id: "outro", seconds: 0.4 },
      ],
      states: [
        S("Boot"), S("Intro", "intro_transition", "intro", "Loop", false),
        S("Loop", "loop_info", "card_loop", "Loop", false, "Loop"),
        S("Outro", "cleanup", "outro", "Closed", false), S("Closed"),
      ],
      overlay_layers: roles(["black_bars", "backdrop", "card", "info", "platform_icon"]),
      visible_overlay_roles: {
        Intro: ["black_bars", "backdrop", "card"],
        Loop: ["black_bars", "backdrop", "card", "info", "platform_icon"],
        Outro: ["black_bars", "backdrop"],
      },
      prompt_slots: [],
      host_driven: true,
      walk: ["Boot", "Intro", "Loop", "Loop", "Outro", "Closed"],
    },

    MissionResult: {
      screen_id: "MissionResultReference", source_system: "result_stack",
      source_files: ["ui_result.yncp", "ui_result_ex.yncp", "ui_missionscreen.yncp", "aspect_ratio_patches.cpp"],
      notes: "Result is a family, not a screen. Rank reveal is a deliberate performance: 253-frame (4.216s) rank intro, then a 200-frame (3.333s) usual sustain on the core rank shell. Number roll precedes rank stamp.",
      timeline_bands: [
        { id: "shell_intro", seconds: 0.45 }, { id: "number_roll", seconds: 1.666667 },
        { id: "rank_reveal", seconds: 4.216667 }, { id: "rank_sustain", seconds: 3.333333 },
        { id: "result_out", seconds: 0.45 },
      ],
      states: [
        S("Boot"), S("Intro", "shell_enter", "shell_intro", "Navigate", false),
        S("Navigate", "number_roll", "number_roll", "Confirm", false, "NumberRoll"),
        S("Confirm", "rank_reveal", "rank_reveal", "Idle", false, "RankReveal"),
        S("Idle", "rank_sustain", "rank_sustain", null, true, "Sustain"),
        S("Outro", "result_out", "result_out", "Closed", false), S("Closed"),
      ],
      overlay_layers: roles(["backdrop", "chrome", "content", "rank_fx", "prompt"]),
      visible_overlay_roles: {
        Intro: ["backdrop", "chrome"], Navigate: ["backdrop", "chrome", "content"],
        Confirm: ["backdrop", "chrome", "content", "rank_fx"],
        Idle: ["backdrop", "chrome", "content", "rank_fx", "prompt"], Outro: ["backdrop"],
      },
      prompt_slots: [ps("continue", "A", "Continue", ["Idle"], ["sustain_done"]) ],
      walk: ["Boot", "Intro", "Navigate", "Confirm", "Idle", "Outro", "Closed"],
    },

    AutosaveToast: {
      screen_id: "AutosaveToastReference", source_system: "autosave_stack",
      source_files: ["ui_saveicon.yncp", "resident_patches.cpp (CSaveIcon::Update)"],
      notes: "Single-scene presentation node, not the save system. 180-frame (3.0s) Intro_Anim. Host-driven: appears while save in progress, clears when native save process ends. Non-interactive.",
      timeline_bands: [
        { id: "icon_intro", seconds: 3.0 }, { id: "icon_out", seconds: 0.5 },
      ],
      states: [
        S("Boot", "", null, "Hidden", false, "Idle"),
        S("Hidden", "", null, null, false),
        S("Intro", "icon_intro", "icon_intro", "Saving", false),
        S("Saving", "spin", null, null, false, "InProgress"),
        S("Outro", "icon_out", "icon_out", "Closed", false), S("Closed"),
      ],
      overlay_layers: roles(["icon"]),
      visible_overlay_roles: {
        Intro: ["icon"], Saving: ["icon"], Outro: ["icon"],
      },
      prompt_slots: [],
      host_driven: true,
      walk: ["Boot", "Hidden", "Intro", "Saving", "Outro", "Closed"],
    },

    /* ---------------- IN-GAME HUD ---------------- */
    SonicStageHUD: {
      screen_id: "SonicStageHUDReference", source_system: "hud_sonic_day",
      source_files: ["HudSonicStage.cpp", "SonicMainDisplay.cpp", "SonicHudGuide.cpp", "ui_playscreen"],
      notes: "Day-stage HUD. Top-left player/time/score/exp counters. Bottom-left speed + ring-energy/boost gauge frame + ring counter. Bottom-right speed-bonus / u_info / medal-get sidecars.",
      timeline_bands: [
        { id: "hud_in", seconds: 0.45 }, { id: "gauge_pulse", seconds: 0.333333 },
        { id: "medal_pop", seconds: 0.5 }, { id: "hud_out", seconds: 0.333333 },
      ],
      states: [
        S("Boot"), S("Intro", "hud_in", "hud_in", "Live", false),
        S("Live", "play", null, null, false, "Live"),
        S("Navigate", "boost_active", "gauge_pulse", "Live", false, "BoostActive"),
        S("Confirm", "medal_get", "medal_pop", "Live", false, "MedalGet"),
        S("Outro", "hud_out", "hud_out", "Closed", false), S("Closed"),
      ],
      overlay_layers: roles(["counters_tl", "gauge_bl", "ring_bl", "sidecar_br"]),
      visible_overlay_roles: {
        Intro: ["counters_tl", "gauge_bl"],
        Live: ["counters_tl", "gauge_bl", "ring_bl", "sidecar_br"],
        Navigate: ["counters_tl", "gauge_bl", "ring_bl", "sidecar_br"],
        Confirm: ["counters_tl", "gauge_bl", "ring_bl", "sidecar_br"],
        Outro: ["counters_tl"],
      },
      prompt_slots: [],
      host_driven: true,
      walk: ["Boot", "Intro", "Live", "Navigate", "Live", "Confirm", "Live", "Outro", "Closed"],
    },

    WerehogStageHUD: {
      screen_id: "WerehogStageHUDReference", source_system: "hud_werehog_night",
      source_files: ["HudEvilStage.cpp", "EvilMainDisplay.cpp", "EvilHudGuide.cpp", "ui_playscreen_ev", "ui_playscreen_ev_hit"],
      notes: "Night-stage HUD. Adds the full unleash / life / shield gauge stack (left). Right-anchored hit-counter family (ui_playscreen_ev_hit) is separate; chance-attack feedback lives in the same hit overlay shell. Direct CEvilHudGuide hook anchors.",
      timeline_bands: [
        { id: "hud_in", seconds: 0.45 }, { id: "unleash_pulse", seconds: 0.333333 },
        { id: "hit_pop", seconds: 0.25 }, { id: "hud_out", seconds: 0.333333 },
      ],
      states: [
        S("Boot"), S("Intro", "hud_in", "hud_in", "Live", false),
        S("Live", "play", null, null, false, "Live"),
        S("Navigate", "unleash_charge", "unleash_pulse", "Live", false, "UnleashCharge"),
        S("Confirm", "hit_combo", "hit_pop", "Live", false, "HitCombo"),
        S("Outro", "hud_out", "hud_out", "Closed", false), S("Closed"),
      ],
      overlay_layers: roles(["gauge_stack_l", "counters_tl", "hit_counter_r", "guide_sidecar"]),
      visible_overlay_roles: {
        Intro: ["gauge_stack_l", "counters_tl"],
        Live: ["gauge_stack_l", "counters_tl", "guide_sidecar"],
        Navigate: ["gauge_stack_l", "counters_tl", "guide_sidecar"],
        Confirm: ["gauge_stack_l", "counters_tl", "hit_counter_r", "guide_sidecar"],
        Outro: ["counters_tl"],
      },
      prompt_slots: [],
      host_driven: true,
      walk: ["Boot", "Intro", "Live", "Navigate", "Live", "Confirm", "Live", "Outro", "Closed"],
    },

    BossHUD: {
      screen_id: "BossHUDReference", source_system: "boss_final_stack",
      source_files: ["ui_boss_gauge.yncp", "ui_boss_name.yncp", "aspect_ratio_patches.cpp", "StageList.xml"],
      notes: "Sequence-owned, overlay-authored. Boss-name reveal ladder (01_Anim..05_Anim, up to 180f / 3.0s). Gauge segments (gauge_1, gauge_2, gauge_bg) + gauge_breakpoint are individually positioned CSD nodes; Size_Anim runs 100f (1.667s). HideLayer suppresses other layers during movie handoff.",
      timeline_bands: [
        { id: "name_reveal", seconds: 3.0 }, { id: "gauge_in", seconds: 0.5 },
        { id: "gauge_resize", seconds: 1.666667 }, { id: "boss_out", seconds: 0.5 },
      ],
      states: [
        S("Boot"), S("Intro", "name_reveal", "name_reveal", "Live", false, "NameReveal"),
        S("Live", "gauge_active", "gauge_in", "Idle", false, "GaugeActive"),
        S("Idle", "battle", null, null, false, "Battle"),
        S("Navigate", "gauge_break", "gauge_resize", "Idle", false, "GaugeBreak"),
        S("Outro", "boss_clear", "boss_out", "Closed", false, "Clear"), S("Closed"),
      ],
      overlay_layers: roles(["name_band", "gauge", "breakpoint", "transient_fx"]),
      visible_overlay_roles: {
        Intro: ["name_band", "transient_fx"], Live: ["gauge", "breakpoint"],
        Idle: ["gauge", "breakpoint"], Navigate: ["gauge", "breakpoint", "transient_fx"],
        Outro: ["transient_fx"],
      },
      prompt_slots: [],
      host_driven: true,
      walk: ["Boot", "Intro", "Live", "Idle", "Navigate", "Idle", "Outro", "Closed"],
    },

    SubtitleCutscene: {
      screen_id: "SubtitleCutsceneReference", source_system: "inspire_cutscene_stack",
      source_files: ["app.cpp (m_InspireSubtitles)", "aspect_ratio_patches.cpp", "*.inspire_resource.xml", "PlayMovie / KeepMovieUntilStageChange"],
      notes: "Movie ownership is a transitional STATE, not passive playback. Subtitle windows are authored frame ranges on ConverseData cells, consistently BOTTOM-anchored. Letterbox/pillarbox driven by Config::CutsceneAspectRatio. KeepMovieUntilStageChange holds ownership until ChangeStage / WaitStageEnd.",
      timeline_bands: [
        { id: "letterbox_in", seconds: 0.5 }, { id: "announce", seconds: 0.5 },
        { id: "cue_window", seconds: 3.0 }, { id: "retention", seconds: 0.6 }, { id: "handoff", seconds: 0.5 },
      ],
      states: [
        S("Boot", "", null, "MoviePrep", false, "Idle"),
        S("MoviePrep", "prep", "letterbox_in", "MovieAnnounce", false),
        S("MovieAnnounce", "announce", "announce", "MovieTakeover", false),
        S("MovieTakeover", "takeover", null, "SubtitleCue", false),
        S("SubtitleCue", "cue_playback", "cue_window", "MovieRetention", false, "SubtitleCuePlayback"),
        S("MovieRetention", "retain", "retention", "Handoff", false),
        S("Handoff", "stage_handoff", "handoff", "Closed", false, "LoadingOrStageHandoff"), S("Closed"),
      ],
      overlay_layers: roles(["letterbox", "movie", "subtitle", "skip_prompt"]),
      visible_overlay_roles: {
        MoviePrep: ["letterbox"], MovieAnnounce: ["letterbox", "movie"],
        MovieTakeover: ["letterbox", "movie", "skip_prompt"],
        SubtitleCue: ["letterbox", "movie", "subtitle", "skip_prompt"],
        MovieRetention: ["letterbox", "movie"], Handoff: ["letterbox"],
      },
      prompt_slots: [ps("skip", "START", "Skip", ["MovieTakeover", "SubtitleCue"], ["can_skip"]) ],
      host_driven: true,
      walk: ["Boot", "MoviePrep", "MovieAnnounce", "MovieTakeover", "SubtitleCue", "MovieRetention", "Handoff", "Closed"],
    },

    Overlays: {
      screen_id: "OverlaysReference", source_system: "overlay_stack",
      source_files: ["achievement_overlay.cpp", "message_window.cpp", "button_guide.cpp"],
      notes: "Three cross-screen overlays. Achievement toast: queue-driven, OVERLAY_DURATION=3, dequeues only when sound/system ready, non-reentrant close. Message window: STAGED reveal — first accept reveals controls instead of confirming. Button guide: left/right prompt groups, safe-area aware.",
      timeline_bands: [
        { id: "toast_expand", seconds: 0.333333 }, { id: "toast_hold", seconds: 3.0 },
        { id: "msg_appear", seconds: 0.333333 }, { id: "controls_appear", seconds: 0.25 },
        { id: "toast_retract", seconds: 0.333333 },
      ],
      states: [
        S("Boot", "", null, "Queue", false, "Idle"),
        S("Queue", "queue_wait", null, "ToastIn", false, "QueueWaiting"),
        S("ToastIn", "expand", "toast_expand", "ToastHold", false),
        S("ToastHold", "count_down", "toast_hold", "MsgAppear", false),
        S("MsgAppear", "msg_appear", "msg_appear", "MsgControls", false),
        S("MsgControls", "reveal_controls", "controls_appear", "Await", false),
        S("Await", "await_choice", null, null, true, "AwaitSelection"),
        S("Outro", "retract", "toast_retract", "Closed", false), S("Closed"),
      ],
      overlay_layers: roles(["toast", "modal_backdrop", "modal", "prompt_row"]),
      visible_overlay_roles: {
        Queue: [], ToastIn: ["toast"], ToastHold: ["toast"],
        MsgAppear: ["toast", "modal_backdrop", "modal"],
        MsgControls: ["toast", "modal_backdrop", "modal"],
        Await: ["modal_backdrop", "modal", "prompt_row"], Outro: ["modal_backdrop", "modal"],
      },
      prompt_slots: [
        ps("accept", "A", "Yes", ["Await"], ["controls_visible"]),
        ps("decline", "B", "No", ["Await"], ["controls_visible", "has_cancel"]),
      ],
      walk: ["Boot", "Queue", "ToastIn", "ToastHold", "MsgAppear", "MsgControls", "Await", "Outro", "Closed"],
    },
  };

  function roles(list) {
    const interactiveRoles = new Set(["content"]);
    return list.map((id) => ({ id, role: id, interactive: interactiveRoles.has(id) }));
  }
  function ps(slot_id, button, label, visible_states, required_predicates) {
    return { slot_id, button, label, visible_states, required_predicates };
  }

  // ordered groups for the sidebar
  const GROUPS = [
    { title: "Front-End", ids: ["TitleMenu", "PauseMenu", "OptionsMenu", "WorldMap"] },
    { title: "Transitions & Results", ids: ["LoadingTransition", "MissionResult", "AutosaveToast"] },
    { title: "In-Game HUD", ids: ["SonicStageHUD", "WerehogStageHUD", "BossHUD"] },
    { title: "Cinematic & Overlays", ids: ["SubtitleCutscene", "Overlays"] },
  ];

  const LABELS = {
    TitleMenu: "Title / Main Menu", PauseMenu: "Pause Menu", OptionsMenu: "Options Menu",
    WorldMap: "World Map", LoadingTransition: "Loading Transition", MissionResult: "Mission Result",
    AutosaveToast: "Autosave Toast", SonicStageHUD: "Sonic Stage HUD", WerehogStageHUD: "Werehog Stage HUD",
    BossHUD: "Boss / Final HUD", SubtitleCutscene: "Subtitle / Cutscene", Overlays: "Achievement / Message / Guide",
  };

  window.SWARD_CONTRACTS = CONTRACTS;
  window.SWARD_GROUPS = GROUPS;
  window.SWARD_LABELS = LABELS;
})();
