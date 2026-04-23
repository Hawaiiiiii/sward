# UI Code Index

Machine-readable inventory: `research_uiux/data/ui_code_index.json`

## Scan Scope

Scanned directories:

- `UnleashedRecomp/ui/`
- `UnleashedRecomp/patches/`
- `UnleashedRecomp/config/`
- `UnleashedRecompLib/config/`

Summary:

- Files indexed: `45`
- Top UI/UX terms by hit count:
  - `ui`: `935`
  - `static`: `524`
  - `button`: `392`
  - `font`: `334`
  - `result`: `324`
  - `aspect`: `293`
  - `window`: `274`
  - `fade`: `212`
  - `option`: `212`
  - `thumbnail`: `209`
  - `cursor`: `201`
  - `menu`: `185`

## Highest-Value Readable UI Systems

## `UnleashedRecomp/ui/options_menu.cpp`

- Main custom options flow.
- Script summary: options menu state, categories, navigation, values, and confirmation logic.
- High-value functions:
  - `DrawCategories`
  - `DrawConfigOption`
  - `DrawConfigOptions`
  - `DrawContainer`
  - `DrawFadeTransition`
  - `DrawInfoPanel`
  - `DrawSettingsPanel`
  - `DrawTitle`
- What it appears to control:
  - Four top-level categories: System, Input, Audio, Video
  - Category switching and option locking
  - Pause-mode vs title-mode behavior
  - Fade layer and TV-static assisted transitions
  - Restart-required detection for language/audio config changes
  - Button-guide opening and safe-area adjustment

## `UnleashedRecomp/ui/options_menu_thumbnails.cpp`

- Thumbnail-backed option preview system.
- Script summary: thumbnail-backed options UI visual content and selection helpers.
- What it appears to control:
  - Value-specific preview textures for `ETimeOfDayTransition`, `EChannelConfiguration`, `EAntiAliasing`, `EShadowResolution`, `ECutsceneAspectRatio`, `EUIAlignmentMode`, and several booleans
  - Device-sensitive tutorial/control thumbnails depending on controller icon mode
- Research value:
  - Strong evidence that the options menu is intentionally visual, not just textual

## `UnleashedRecomp/ui/message_window.cpp`

- Main modal prompt/message box system.
- Script summary: message/prompt window display, input gating, and modal sequencing.
- High-value functions:
  - `MessageWindow::Open`
  - `MessageWindow::Draw`
  - `MessageWindow::Close`
  - `DrawNextButtonGuide`
  - `OnSDLEvent`
- What it appears to control:
  - Two-stage prompt flow
  - Deferred control reveal
  - Selection reset/default handling
  - Keyboard, mouse, and controller support
  - Shared button-guide coordination

## `UnleashedRecomp/ui/button_guide.cpp`

- Prompt abstraction layer for action hints.
- Script summary: controller prompt abstraction and button-guide rendering/state updates.
- High-value functions:
  - `ButtonGuide::Init`
  - `ButtonGuide::Open`
  - `ButtonGuide::SetSideMargins`
  - `ButtonGuide::Draw`
  - `ButtonGuide::Close`
- What it appears to control:
  - Left/right-aligned prompt layout
  - Device-sensitive icon selection
  - Safe-area width control via side margins
  - Visibility predicates and per-entry styling constraints

## `UnleashedRecomp/ui/achievement_overlay.cpp`

- Achievement toast/overlay layer.
- Script summary: transient achievement toast/overlay timing and presentation logic.
- High-value functions:
  - `AchievementOverlay::Init`
  - `AchievementOverlay::Open`
  - `AchievementOverlay::Draw`
  - `AchievementOverlay::Close`
  - `CanDequeueAchievement`
- What it appears to control:
  - Queue-driven overlay dispatch
  - Visibility, close gating, and timing
  - Appear/disappear animation
  - Sound-gated dequeue behavior

## `UnleashedRecomp/ui/achievement_menu.cpp`

- Full achievements browser.
- Script summary: ImGui-driven achievements menu flow, filtering, layout, and rendering.
- High-value functions:
  - `AchievementMenu::Init`
  - `AchievementMenu::Open`
  - `AchievementMenu::Draw`
  - `AchievementMenu::Close`
  - `DrawAchievement`
  - `DrawAchievementTotal`
- What it appears to control:
  - Scrollable/filterable achievements view
  - Selection reset and marquee behavior
  - Button guide integration

## `UnleashedRecomp/ui/fader.cpp`

- Shared full-screen fade layer.
- Script summary: shared fade-in/fade-out alpha progression and timing helpers.
- High-value functions:
  - `Fader::FadeIn`
  - `Fader::FadeOut`
  - `Fader::Draw`
  - `Fader::SetFadeColour`
- What it appears to control:
  - Full-screen alpha lerp
  - Serialized fade ownership
  - End callbacks and optional callback delay

## `UnleashedRecomp/ui/black_bar.cpp`

- Letterbox/pillarbox layer.
- Script summary: letterbox or cinematic black-bar presentation effect.
- What it appears to control:
  - Inspire-style pillarboxing
  - Loading black-bar alpha overlays
  - Aspect-conditional black rectangles

## `UnleashedRecomp/ui/tv_static.cpp`

- Decorative transition effect used by the options UI.
- Script summary: TV static overlay animation/effect logic.
- High-value functions:
  - `TVStatic::Init`
  - `TVStatic::ComputeThumbnailAlpha`
  - `TVStatic::Draw`
- What it appears to control:
  - Keyframed noise/flash curves
  - Shared appear-time clock
  - Thumbnail reveal blending

## `UnleashedRecomp/ui/game_window.cpp`

- SDL/windowing wrapper used by the PC/runtime layer.
- Script summary: main debug/game window orchestration and screen composition hooks.
- What it appears to control:
  - Window creation and sizing
  - Fullscreen/maximize behavior
  - Display mode discovery
  - Window state transitions relevant to UI scaling and layout

## `UnleashedRecomp/ui/imgui_utils.cpp`

- Shared motion and drawing helpers.
- Script summary: low-level ImGui helpers, styling glue, and reusable drawing utilities.
- High-value helpers:
  - `ComputeLinearMotion`
  - `ComputeMotion`
  - `DrawPauseContainer`
  - `DrawPauseHeaderContainer`
  - `DrawSelectionContainer`
  - marquee fade helpers
- Research value:
  - Central place for reusable motion curves, pause panel framing, and annotated text behavior

## Key UI/UX Patterns Seen Across Readable Code

- `Open` / `Draw` / `Close` / `Init` pattern is reused heavily.
- Systems commonly expose `s_isVisible` or equivalent static visibility gates.
- Time-driven behavior commonly keys off `ImGui::GetTime()` and local `g_appearTime` or `g_startTime`.
- Input is often gated through `CInputState`, SDL event listeners, or both.
- Button prompts are decoupled from individual screens through `ButtonGuide`.
- Aspect-ratio and safe-area handling are treated as a cross-cutting concern rather than isolated per-widget logic.

## Best Manual Study Order

1. `options_menu.cpp`
2. `message_window.cpp`
3. `button_guide.cpp`
4. `achievement_overlay.cpp`
5. `fader.cpp`
6. `options_menu_thumbnails.cpp`
7. `imgui_utils.cpp`
8. `achievement_menu.cpp`
