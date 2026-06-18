# sgfx_recomp_ui — the host contract

The library is the recomp's authentic `ui/*.cpp` (bodies byte-identical), decoupled from the
game. A host project (SGFX, a preview app) provides the symbols below; the lib then compiles +
renders. This was derived mechanically by decoupling all 12 ui/ files and recording exactly what
each touched (89 symbols + 32 deeper hooks). Symbols keep their recomp names so the menu bodies
stay byte-identical.

## Status per file
| File | State |
|---|---|
| black_bar, imgui_utils, fader, tv_static, button_guide | ✅ compiles (support layer + keystone) |
| achievement_overlay, achievement_menu, game_window, message_window, options_menu_thumbnails | 🟡 decoupled; compiles once the contract below is bound |
| options_menu (1854), installer_wizard (1885) | 🟡 decoupled; need the heaviest contract (full ConfigDef + SDL + install/NFD) |

## 1. Config — the customization surface (`platform/sgfx_config.h`)
The recomp's `IConfigDef` + `template<class T> ConfigDef<T> : IConfigDef` (clean C++; deps = `ELanguage`
+ toml for load/save, locale for label strings — both shim-able). Every option is a `ConfigDef<T>`
object: the menus take `&Config::Field` as `const IConfigDef*`, call `->GetValue()` / `GetNameLocalised`
/ `MakeDefault` / `SnapToNearestAccessibleValue` / `IsDefaultValue`, and rely on `operator T()` so
`Config::Foo == Bar` still compiles. **~34 fields** (these ARE the template knobs):
Language, VoiceLanguage, Subtitles, Hints, ControlTutorial, AchievementNotifications, TimeOfDayTransition,
HorizontalCamera, VerticalCamera, Vibration, AllowBackgroundInput, ControllerIcons, MasterVolume,
MusicVolume, EffectsVolume, ChannelConfiguration, MusicAttenuation, BattleTheme, WindowSize, Monitor,
AspectRatio, ResolutionScale, Fullscreen, VSync, FPS, Brightness, AntiAliasing, TransparencyAntiAliasing,
ShadowResolution, GITextureFiltering, MotionBlur, XboxColorCorrection, CutsceneAspectRatio, UIAlignmentMode
(+ window: WindowState/WindowX/Y/Width/Height, UseOfficialTitleOnTitleBar, UseAlternateTitle,
DisableDWMRoundedCorners, DisableLowResolutionFontOnCustomUI; misc: ShowConsole, UseArrowsForTimeOfDayTransition).
Enums: ELanguage, EVoiceLanguage, ETimeOfDayTransition, ECameraRotationMode, EControllerIcons(done),
EChannelConfiguration, EAntiAliasing, EShadowResolution, EGITextureFiltering, EMotionBlur,
ECutsceneAspectRatio, EUIAlignmentMode, EWindowState, EAspectRatio(done).

## 2. Platform hooks (`platform/sgfx_platform.h`)
- `void Game_PlaySound(const char* cue)` — host SFX (used by message_window/achievements/options/installer)
- `namespace App { bool s_isInit; ELanguage s_language; double s_deltaTime; bool s_isSaving; void Exit(); }`
- `namespace hid { EInputDevice g_inputDevice; bool IsInputAllowed(); bool IsInputDeviceController(); void SetProhibitedInputs(...); }`
- `class Video { static uint32_t s_viewportWidth, s_viewportHeight; }`  + `bool g_needsResize;`
- `class GameWindow { int s_width,s_height; GetSizeInPixels; GetDisplayCount; GetDisplayModes; SetFullscreenCursorVisibility; Update; }`
- `namespace os::user { bool IsDarkTheme(); }`  `namespace os::version { OSVersion GetOSVersion(); }`  `LOGFN*` macros
- `std::string& Localise(const std::string_view&)` (⚠ BY REFERENCE) + `std::string g_localeMissing`
- `namespace AudioPatches { bool CanAttenuate(); }`  `void VideoConfigValueChangedCallback(IConfigDef*)`
- SDL event hub: `class SDLEventListener { virtual bool OnSDLEvent(SDL_Event*)=0; }` + `GetEventListeners()` (needs SDL on the include path)
- Constants: `XAMINPUT_GAMEPAD_START`, `FPS_MIN/MAX`, `ACH_RECORDS`, `SDL_USER_EVILSONIC`

## 3. Achievements (`platform/` — host provider)
`struct Achievement { uint16_t ID; std::string Name, Description, UnlockedDescription; }` +
`XdbfWrapper g_xdbfWrapper` (GetAchievement/GetAchievements) + `AchievementManager` (IsUnlocked/GetTimestamp/
GetTotalRecords) + `xdbf::FixInvalidSequences` + `EXDBFLanguage`. Suggested clean form:
`namespace sgfx::achievements { struct Record{...}; std::vector<Record> GetAll(ELanguage); bool IsUnlocked(id); ... }`.

## 4. In-game input (the one api/SWA.h touch, in-game only)
`SWA::CInputState::GetInstance()->GetPadState()` with `IsTapped/IsDown/IsReleased(eKeyState_*)` + stick axes.
Suggested: `namespace sgfx::input { struct PadState{...}; PadState& GetPadState(); }`. Gated by `App::s_isInit`.

## 5. Render (`render/sgfx_render.h`)
- Add UISprite enumerators: Trophy, the ~50 options Thumb* sprites, Install001..008, MilesElectricIcon,
  ArrowCircle, PulseInstall, HedgeDev, OptionsMilesElectric. (Host supplies the pixels — may be SEGA-derived.)
- `std::unordered_map<uint16_t, render::Texture*> g_xdbfTextureCache` (achievement icons)
- Fonts via `ImFontAtlasSnapshot::GetFont`: FOT-SeuratPro-M, FOT-NewRodinPro-DB, FOT-NewRodinPro-UB, DFSoGeiStd-W7

## 6. Installer backend (host, not UI) — installer_wizard only
`namespace Installer { Sources/Input/Journal; install/rollback/parse*/check*; }` + `EmbeddedPlayer` +
NFD file dialog + `MessageWindow::Open` (sibling, ported). The install *pipeline* is the host's;
`InstallerWizard::Run()` is an app harness (host drives its own loop and calls Draw()).

---
**The boundary:** §1 (ConfigDef) is library code (port verbatim, stub toml/locale). §§2–6 are host
implementations the lib only *declares*. A standalone preview ships a default impl (demo config, no-op
sound, 1280×720 display, a fixed achievement list). SGFX binds them to its real systems.
