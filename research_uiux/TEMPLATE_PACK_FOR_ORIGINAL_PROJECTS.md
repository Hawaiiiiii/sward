<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Template Pack For Original Projects

## Summary

Phase 18 distills the recovered UI/UX research into a reusable template pack for original work.

The pack is intentionally structural, not extractive:

- no proprietary textures
- no copied scene graphs
- no translated generated PPC code
- no direct reuse of SEGA-authored naming that would turn into content duplication

Instead, the pack keeps the transferable parts:

- explicit screen-state machines
- timer-banded transitions
- input lock windows
- overlay layering contracts
- prompt-guide abstraction
- screen host contracts

> [!IMPORTANT]
> Treat this pack as architecture and timing guidance. Replace every art asset, string set, scene identifier, and audio cue with project-local equivalents.

## Pack Contents

- `research_uiux/templates/README.md`
- `research_uiux/templates/screen_state_machine_template.yaml`
- `research_uiux/templates/overlay_stack_template.yaml`
- `research_uiux/templates/transition_timeline_template.yaml`
- `research_uiux/templates/button_prompt_template.json`
- `research_uiux/templates/screen_contract_template.yaml`

## What The Templates Preserve

From the recovered research, the pack keeps these ideas intact:

- a screen is usually not a single state; it is a short choreography:
  - `Boot`
  - `Intro`
  - `Idle`
  - `Navigate`
  - `Confirm`
  - `Cancel`
  - `Outro`
- input should not be uniformly available.
  - lock input during intro, confirm, cancel, and certain resize/scroll accents
- texture count and scene complexity are separate levers.
  - small overlays can stay visually rich through timing, hierarchy, and subimage swaps
- overlays work best as layered contracts, not loose ad hoc widgets.
  - backdrop, frame, content, prompt row, transient FX, and notification channels should be separated

## Timing Bands Worth Reusing

The template pack turns the recovered timing bands into reusable defaults:

- `0.0833s` to `0.25s`
  - flashes, quick fades, and title-card accents
- `0.3333s` to `0.45s`
  - menu shell entry, card movement, selection bar travel, resize accents
- `0.9833s` to `1.6667s`
  - mission cue dwell, loading-card body motion, gauge resizing, short overlay attention windows
- `3.0s` to `3.3333s`
  - boss-name emphasis, save toast persistence, long sustain holds
- `4.0s` to `4.2167s`
  - loading-card loops and result-rank flourish windows

These are not hard rules. They are strong starting bands that already proved readable under fast gameplay context.

## Pseudo-Architecture

```text
UIScreenHost
|-- ScreenStateMachine
|   |-- current_state
|   |-- pending_state
|   |-- transition_clock
|   `-- input_lock_policy
|-- OverlayStack
|   |-- backdrop_layer
|   |-- chrome_layer
|   |-- content_layer
|   |-- prompt_layer
|   `-- transient_fx_layer
|-- PromptGuideModel
|   |-- action_slots
|   |-- platform_icon_family
|   `-- visibility_rules
`-- ScreenTelemetry
    |-- state_entry
    |-- state_exit
    |-- blocked_input
    `-- animation_completion
```

## Adaptation Rules

### Keep

- state names that describe behavior, not brand content
- timer bands
- input-lock policy
- overlay/layer separation
- host-to-overlay contracts

### Replace

- all texture files
- all layout scene names copied from extracted packages
- all button icon art
- all strings, subtitles, flavor text, and title cards
- all audio event identifiers

### Rename

Use your own vocabulary, for example:

- `Intro` -> `Reveal`
- `Usual` -> `Loop`
- `Size_Anim` -> `FocusPulse`
- `status_footer` -> `PromptRow`
- `window_bg` -> `ModalBackdrop`

## Suggested Use Cases

- pause menu systems
- mission or objective overlays
- achievement / save toasts
- world-map or hub info panels
- results or grade-summary screens
- title-card and loading-card systems

## Recommended Workflow

1. Start with `screen_contract_template.yaml`.
2. Fill in ownership, screen purpose, layer map, and audio/input hooks.
3. Add the state graph from `screen_state_machine_template.yaml`.
4. Pull timing presets from `transition_timeline_template.yaml`.
5. Define prompt behavior with `button_prompt_template.json`.
6. Only after the contract is stable, build art and scene naming for your own project.

> [!TIP]
> The strongest companion docs for this pack are `UI_UX_INSPIRATION_NOTES.md`, `CODE_TO_LAYOUT_CORRELATION.md`, `PAUSE_STATUS_WORLDMAP_DEEP_DIVE.md`, `BOSS_RESULT_SAVE_LOAD_DEEP_DIVE.md`, and `VISUAL_ATLAS_DOCS.md`.
