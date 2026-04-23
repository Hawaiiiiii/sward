<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Subtitle / Cutscene Runtime Contracts

Phase 37 turns the earlier subtitle/cutscene archaeology into an actual reusable runtime family.

> [!IMPORTANT]
> This is not a movie decoder or a subtitle-text dump. It is a portable state/overlay/prompt contract for cutscene ownership, preview browsing, and subtitle-layer timing behavior.

## What Landed

- bundled contract: `research_uiux/runtime_reference/contracts/subtitle_cutscene_reference.json`
- native profile enum: `ReferenceProfile::SubtitleCutscene`
- C ABI profile enum: `SWARD_UI_PROFILE_SUBTITLE_CUTSCENE`
- managed profile enum: `ReferenceProfile.SubtitleCutscene`
- selector-family bridge: `InspirePreview.cpp` now resolves through the subtitle/cutscene family

## Contract Shape

The new bundled contract models:

- `MovieAnnounce`
- `PreviewIdle`
- `SceneCycle`
- `SubtitleCuePlayback`
- `LoadingOrStageHandoff`
- `RouteReturn`

Key runtime surfaces:

- overlay roles: `backdrop`, `letterbox`, `content`, `subtitle`, `loading_gate`, `prompt`
- prompt row: `Play`, `Back`, `Previous Scene`, `Next Scene`, `Subtitles`
- source system: `subtitle_cutscene_presentation`

## Evidence Base

The contract is grounded in:

- `research_uiux/data/subtitle_cutscene_presentation.json`
- `research_uiux/SUBTITLE_CUTSCENE_PRESENTATION_DEEP_DIVE.md`
- the route-side `PlayMovie` ownership layer
- bottom-anchored subtitle cue behavior
- readable subtitle and cutscene-aspect-ratio controls

## Verified Result

- the broader source-path bridge now measures `74 / 220` contract-backed paths (`33.6%`)
- `sward_ui_runtime_debug_selector.exe --list-families` now reports a seventh bundled family: `Subtitle / Cutscene Presentation`
- `sward_ui_runtime_debug_selector.exe InspirePreview.cpp` now resolves and runs
- the C ABI example successfully loads `subtitle_cutscene_reference.json` by explicit path
- the C# managed reference builds and runs with the new subtitle/cutscene profile
