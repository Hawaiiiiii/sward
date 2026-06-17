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

// ---- aspect ratio (was patches/aspect_ratio_patches.h) ----------------------
inline constexpr float WIDE_ASPECT_RATIO   = 16.0f / 9.0f;
inline constexpr float NARROW_ASPECT_RATIO = 4.0f  / 3.0f;
extern float g_aspectRatio;        // current viewport aspect (host sets each frame)
extern float g_aspectRatioScale;   // UI scale factor derived from the aspect/resolution
extern float g_aspectRatioOffsetX; // letterbox/pillarbox offset (host sets each frame)
extern float g_aspectRatioOffsetY;

enum class EAspectRatio : uint32_t { Auto, Wide, Narrow, OriginalWide, OriginalNarrow };

// ---- input device (was hid/hid.h) -------------------------------------------
namespace hid
{
    enum class EInputDevice { Unknown, Keyboard, Mouse, Xbox, PlayStation };
    extern EInputDevice g_inputDeviceController;   // host sets the active pad type
}

enum class EControllerIcons : uint32_t { Auto, Xbox, PlayStation };

// ---- config: the customization surface (was the game's user/config.h) -------
// The menus read these as `Config::Field`. Defined as a namespace of plain values
// (the recomp uses ConfigDef<T> objects that implicitly convert; for the menus'
// read sites a plain value is equivalent). Extended as options_menu is ported.
namespace Config
{
    inline bool             DisableLowResolutionFontOnCustomUI = false;
    inline EAspectRatio     AspectRatio = EAspectRatio::Wide;
    inline EControllerIcons ControllerIcons = EControllerIcons::Auto;
}

// ---- version string (was version.h) -----------------------------------------
extern const char* g_versionString;

// ---- localisation (was locale/locale.h) -------------------------------------
// The menus call Localise(key). Host binds it to its strings; default = identity.
std::string Localise(const std::string& key);
