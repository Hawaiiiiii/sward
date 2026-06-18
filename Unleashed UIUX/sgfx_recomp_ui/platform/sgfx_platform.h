// =============================================================================
// sgfx_platform.h — the game-runtime compatibility shim.
//
// UnleashedRecomp's ui/*.cpp is clean human C++ welded to the game runtime
// (api/SWA.h, kernel/, app.h, locale/, hid/, patches/). This header MIRRORS the
// exact game symbols the menu code references — `Config`, `g_aspectRatio`,
// `EAspectRatio`, `Localise`, `g_versionString`, ... — in the same namespaces, so
// each ui/ file stays BYTE-IDENTICAL to the recomp except for its #includes. A host
// project (SGFX, a preview app, ...) provides the definitions; defaults live in
// platform/sgfx_platform_default.cpp.
//
// This is the whole decoupling: keep Dear ImGui + the recomp's custom render layer
// (render/imgui_common.h); replace ONLY the game, here. Grows field-by-field as each
// menu is ported (the Config surface = the template's customization knobs).
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "sgfx_config.h"        // the ConfigDef customization surface + the option enums
#include "sgfx_achievements.h"  // the XDBF achievement-DB provider (host-bound)
#include "sgfx_input.h"         // the in-game input facade (SWA::CInputState)

// ---- aspect ratio (was patches/aspect_ratio_patches.h) ----------------------
inline constexpr float WIDE_ASPECT_RATIO   = 16.0f / 9.0f;
inline constexpr float NARROW_ASPECT_RATIO = 4.0f  / 3.0f;
extern float g_aspectRatio;        // current viewport aspect (host sets each frame)
extern float g_aspectRatioScale;   // UI scale factor derived from the aspect/resolution
extern float g_aspectRatioNarrowScale; // 0..1 across the narrow..wide range (options info panel)
extern float g_aspectRatioOffsetX; // letterbox/pillarbox offset (host sets each frame)
extern float g_aspectRatioOffsetY;

// ---- input device (was hid/hid.h) -------------------------------------------
namespace hid
{
    enum class EInputDevice { Unknown, Keyboard, Mouse, Xbox, PlayStation };
    extern EInputDevice g_inputDeviceController;   // active pad TYPE (icon set)
    extern EInputDevice g_inputDevice;             // device that produced the LAST input
    bool IsInputAllowed();
    bool IsInputDeviceController();
    void SetProhibitedInputs(uint16_t wButtons = 0, bool leftStick = false, bool rightStick = false);
}
inline constexpr uint16_t XAMINPUT_GAMEPAD_START = 0x0010;

// gates the MusicAttenuation option's accessibility (was patches/audio_patches.h)
namespace AudioPatches { bool CanAttenuate(); }

// ---- version string (was version.h) -----------------------------------------
extern const char* g_versionString;

// ---- localisation (was locale/locale.h) — BY REFERENCE (options does &Localise) --
std::string& Localise(const std::string_view& key);
extern std::string g_localeMissing;

// ---- host SFX hook (was exports.h Game_PlaySound) ---------------------------
void Game_PlaySound(const char* cue);

// ---- app lifecycle (was app.h) — host drives these; preview leaves defaults ---
class App
{
public:
    static inline bool      s_isInit = false;            // true once the game runtime booted (in-game vs installer)
    static inline bool      s_isMissingDLC = false;
    static inline bool      s_isLoading = false;
    static inline bool      s_isSaving = false;
    static inline bool      s_isWerehog = false;
    static inline bool      s_isSaveDataCorrupt = false;
    static inline ELanguage s_language = ELanguage::English;
    static inline double    s_deltaTime = 0.0;
    static inline double    s_time = 0.0;
    static void Restart(std::vector<std::string> restartArgs = {});
    static void Exit();
};
