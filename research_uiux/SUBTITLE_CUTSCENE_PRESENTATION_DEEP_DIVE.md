# <img src="../docs/assets/branding/icon_debug.png" width="30" alt="SWARD debug icon"/> Subtitle / Cutscene Presentation State Deep Dive

## Summary

Phase 19 moves from subtitle archive coverage into actual presentation-state evidence.

- extracted a focused English subtitle slice into `extracted_assets/phase19_subtitle_archives`
- generated `research_uiux/data/subtitle_cutscene_presentation.json`
- correlated extracted subtitle resources against `#Application` `PlayMovie` ownership
- confirmed readable runtime seams for subtitle enablement and cutscene aspect-ratio handling

> [!IMPORTANT]
> This phase is about how subtitle and cutscene presentation is orchestrated. It is not a dialogue-text dump. The useful layer is timing, placement, ownership, loading handoff, and scene-to-sequence structure.

## Local Evidence Used

- `extracted_assets/ui_multilingual_archives/Inspire/subtitle/*/*/*.inspire_resource.xml`
- `extracted_assets/phase19_subtitle_archives/Inspire/subtitle/English/*/*.inspire_resource.xml`
- `extracted_assets/ui_extended_archives/#Application/*.seq.xml`
- `local_build_env/ur103clean/UnleashedRecomp/app.cpp`
- `local_build_env/ur103clean/UnleashedRecomp/patches/aspect_ratio_patches.cpp`
- `local_build_env/ur103clean/UnleashedRecomp/locale/config_locale.cpp`
- `local_build_env/ur103clean/UnleashedRecomp/user/config_def.h`
- `local_build_env/ur103clean/UnleashedRecomp/api/SWA/System/ApplicationDocument.h`

## Focused Extraction Result

This pass extracted `8` English subtitle archives:

- `evrt_m1_01`
- `evrt_m3_02`
- `evrt_m4_01`
- `evrt_m5_02`
- `evrt_m8_01`
- `evrt_m8_12`
- `evrt_s2_04`
- `evrt_t0_01`

What actually landed:

- `24` extracted files total
- `6` payload-bearing subtitle bundles
- extension mix:
  - `6` `.inspire_resource.xml`
  - `6` `.fco`
  - `6` `.fte`
  - `6` `.dds`

The two empty cases matter:

- `evrt_m8_12`
- `evrt_t0_01`

Both source pairs exist, but they are tiny:

- `.arl` size: `86` bytes
- `.ar` size: `94` bytes

In practice, they behave like stub archives in this local build rather than real subtitle payload bundles.

## Machine-Readable Correlation Result

`research_uiux/data/subtitle_cutscene_presentation.json` currently covers:

- `24` subtitle resource XML files
- `9` unique subtitle scene IDs
- `9 / 9` scene IDs matched back to `PlayMovie` ownership
- `168` `PlayMovie` units with explicit `SceneName` values in the extracted `#Application` layer

Language spread in the current sample:

- English: `9`
- French: `3`
- German: `3`
- Italian: `3`
- Japanese: `3`
- Spanish: `3`

Placement result in the current sample:

- all `24` extracted subtitle resource XML files place dialogue at `BOTTOM`

## Readable Runtime Controls

Readable modern integration code confirms that subtitle and cutscene presentation are not asset-only concerns.

- `app.cpp:79-81`
  - applies the runtime subtitle option every update by writing `pApplicationDocument->m_InspireSubtitles = Config::Subtitles`
- `ApplicationDocument.h:101`
  - exposes the underlying `m_InspireSubtitles` field
- `config_def.h:6`
  - defines the `Subtitles` config entry
- `config_locale.cpp:486-488`
  - exposes the user-facing subtitle option text
- `config_def.h:75` and `config_locale.cpp:901-918`
  - expose the `CutsceneAspectRatio` option and its localized labels
- `aspect_ratio_patches.cpp:1460` and `1482`
  - enforce letterbox/pillarbox behavior depending on `Config::CutsceneAspectRatio`

That gives a clean split:

- extracted subtitle resource XML decides caption timing and placement
- `#Application` decides when a movie scene starts, what layers are hidden, and how stage/loading handoff behaves
- readable runtime/config code decides whether subtitles are shown and whether cutscenes stay locked to original framing

## Subtitle Resource Structure

Each `*.inspire_resource.xml` file encodes a timed subtitle project with:

- project frame range
- one or more `ConverseData` entries
- per-entry `GroupIDName`
- `CellID` / `CellIDName`
- `Position`
- trigger frame windows

In other words, the subtitle layer is authored as explicit timed cue windows, not as free-floating text.

Observed patterns:

- current extracted samples use `BOTTOM` consistently
- some scenes have dense, sustained dialogue windows
- some scenes contain many micro windows at `0..3` frames, which behave more like placeholder or disabled cue slots than readable caption holds
- one sampled scene, `evrt_s2_04`, includes an invalid reversed window, which strongly suggests authoring-time disabled or placeholder timing entries rather than a strict one-cue-per-line linear system

## Sequence-Side Presentation Ownership

The `#Application` layer shows that subtitle scenes live inside broader movie/state handoff sequences.

Strong global pattern:

- all `9` matched subtitle scene IDs currently correlate to `PlayMovie` usage with `KeepMovieUntilStageChange = true`

That matters because it means the subtitle-bearing movie is treated as a transitional ownership state, not just a visual clip that immediately yields back to gameplay.

Additional presentation flags found in the matched set:

- `NeedLoadingDisplayAfter = true`
  - `evrt_m1_01`
  - `evrt_m3_02`
  - `evrt_s2_04`
- `AnnounceBeforePrepare = true`
  - `evrt_m1_01`
  - `evrt_m1_03_2`
  - `evrt_m3_02`
  - `evrt_m8_15`
  - `evrt_s2_04`
  - `evrt_t0_04`
- hide-layer control exists for:
  - `evrt_m1_03_2` -> `tails`
  - `evrt_m3_02` -> `3D`, `Boss`, `Design`
  - `evrt_t0_04` -> `2D`, `3D`, `Boss`, `Design`

The generic wrapper layer is also visible:

- `PlayMovie_All.seq.xml` references many of the same event scenes

But the more useful ownership evidence comes from route-specific sequences such as:

- `SR_Main01_MykonosNightAction.seq.xml`
- `SR_Main03_AfricaDayBoss.seq.xml`
- `SR_Main04A_SaveEmy.seq.xml`
- `SR_Main05_ChinaNightBoss.seq.xml`
- `SR_Main14_DarkGaia2nd.seq.xml`

Those route-specific files are where subtitle-bearing movies get tied into mission flow, boss transitions, loading display, stage change, and autosave.

## Representative Scene Profiles

### `evrt_m1_03_2`

- languages present in the current sample: all `6`
- cue count: `37`
- active cues: `24`
- micro cues: `13`
- project duration: `52.816667s`
- max cue duration: `2.516667s`
- hide-layer behavior: hides `tails`
- matched route sequence: `SR_Main01_MykonosNightAction.seq.xml`

Interpretation:

- this is a long dialogue-driven scene with real subtitle cadence
- the `tails` hide layer shows that character-layer cleanup can happen at movie-entry time, not just inside the movie asset itself

### `evrt_m3_02`

- cue count: `17`
- project duration: `28.0s`
- `NeedLoadingDisplayAfter = true`
- hide layers: `3D`, `Boss`, `Design`
- matched route sequence: `SR_Main03_AfricaDayBoss.seq.xml`

Interpretation:

- this is a boss-adjacent movie handoff where subtitle presentation is only one part of a broader screen-ownership transition
- the route explicitly suppresses gameplay-facing layers and requests a loading display after the movie

### `evrt_t0_04`

- languages present in the current sample: all `6`
- cue count: `15`
- active cues: `2`
- micro cues: `13`
- project duration: `9.316667s`
- hide layers: `2D`, `3D`, `Boss`, `Design`
- matched route sequence: `SR_Main03_AfricaDayBoss.seq.xml`

Interpretation:

- this scene behaves more like a short transition bridge than a dialogue-heavy movie
- most authored cue slots are effectively micro windows, so the subtitle layer here is sparse despite the project having multiple cue entries
- the layer-hiding pattern confirms that this scene is used as a hard handoff between presentation contexts

### `evrt_s2_04`

- cue count: `23`
- active cues: `11`
- micro cues: `12`
- invalid cues: `1`
- project duration: `22.016667s`
- `NeedLoadingDisplayAfter = true`
- matched route sequence: `SR_Main04A_SaveEmy.seq.xml`

Interpretation:

- this is one of the clearest examples of subtitle presentation living inside a mission/save/load chain rather than a standalone cinematic shell
- the invalid cue window suggests that subtitle authoring data can include muted or disabled entries that should not be interpreted as final visible dialogue timing

### `evrt_m4_01`

- cue count: `57`
- active cues: `42`
- micro cues: `15`
- project duration: `81.033333s`
- max cue duration: `2.4s`
- matched route sequence: `SR_Main05_ChinaNightBoss.seq.xml`

Interpretation:

- this is a dense subtitle project with sustained dialogue coverage
- even without explicit hide-layer flags, it still participates in the same movie-to-stage-change ownership model as the shorter transition scenes

## Reconstructed Presentation State Model

From the combined evidence, the reusable subtitle/cutscene state model looks like this:

1. `MoviePrep`
   - route sequence sets flags, may change stage, may enter a helper micro-sequence
2. `MovieAnnounce`
   - optional `AnnounceBeforePrepare`
3. `MovieTakeover`
   - `PlayMovie` begins and can hide gameplay/UI layers
4. `SubtitleCuePlayback`
   - `ConverseData` windows activate bottom-anchored subtitle cells over frame ranges
5. `MovieRetention`
   - `KeepMovieUntilStageChange` keeps ownership on the movie side through handoff
6. `LoadingOrStageHandoff`
   - optional loading display, change stage, or wait-for-stage-end path
7. `RouteReturn`
   - sequence continues to town/world-map/event/save branches

This is a useful distinction:

- subtitles are not an always-on HUD system
- they are scoped to movie-state ownership and timed cue windows
- loading display, layer suppression, and return routing are first-class parts of subtitle-bearing cutscene presentation

## Practical Takeaways

- subtitle presentation should be modeled as timed scene ownership, not as a single global text overlay
- bottom anchoring is consistent in the current sample, which suggests a stable cinematic safe-area rule
- micro cue windows and invalid windows need filtering logic during tooling work; not every authored cue entry is a real readable subtitle dwell
- boss, mission, and temple transitions use the same movie wrapper system but with different hide-layer and loading-display policies

> [!TIP]
> The best companion files for this report are `MULTILINGUAL_UI_AND_SUBTITLE_COVERAGE.md`, `UI_STATE_MACHINES.md`, `UI_ANIMATION_AND_TRANSITION_NOTES.md`, `app.cpp`, and `aspect_ratio_patches.cpp`.
