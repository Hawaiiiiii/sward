// =============================================================================
// sgfx_platform.h — the decoupling shim.
//
// UnleashedRecomp's ui/*.cpp menus are clean, human-written C++... but welded to
// the game runtime (api/SWA.h, kernel/, app.h, locale/, hid/, patches/). This
// header replaces that entire coupling with ~6 small, project-agnostic interfaces
// so the menu code becomes a REUSABLE library — drop it into SGFX or any project,
// implement these, and you get Sonic Unleashed's UI without the game.
//
// What stays (portable, kept by design): Dear ImGui + the recomp's custom imgui
// gradient/outline/skew render layer (gpu/imgui). What this strips: the GAME.
//
// A host project provides ONE translation unit implementing the `extern` symbols
// and the functions below. Defaults suitable for a standalone preview are shipped
// in platform/sgfx_platform_default.cpp.
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <imgui.h>

namespace sgfx {

// ---- aspect ratio / pillarboxing (was patches/aspect_ratio_patches.h) -------
inline constexpr float WIDE_ASPECT_RATIO   = 16.0f / 9.0f;
inline constexpr float NARROW_ASPECT_RATIO = 4.0f  / 3.0f;
extern float g_aspectRatio;              // host sets this each frame (viewport w/h)

// ---- window (was ui/game_window.h :: GameWindow) ----------------------------
namespace window {
    float Width();                       // backbuffer width  in pixels
    float Height();                      // backbuffer height in pixels
    bool  IsFocused();
    bool  IsFullscreen();
}

// ---- localisation (was locale/locale.h :: Localise) -------------------------
// Returns the display string for a message key in the active language. A host may
// back this with the game's locale tables, a JSON file, or an identity map.
std::string Localise(const std::string& key);

// ---- input (was hid/hid.h) --------------------------------------------------
// Abstract menu input; the menus only need edge-triggered "did the player press X
// this frame". Map your real input (keyboard / pad / SDL) onto these.
namespace input {
    enum Button { Up, Down, Left, Right, Accept, Cancel, BumperL, BumperR, Start, Count };
    bool Pressed(Button b);              // edge-triggered this frame
    bool Held(Button b);
}

// ---- app/time (was app.h) ---------------------------------------------------
namespace app {
    double Time();                       // seconds since start (== ImGui::GetTime by default)
    bool   IsInstaller();                // some menus branch on first-run installer context
}

// ---- config: the user-customizable settings the options menu reads/writes ----
// This is the template's customization surface. Designed incrementally as each
// option row is ported from options_menu.cpp; a host binds these to its own store.
struct Config; // forward — defined in platform/sgfx_config.h once options is ported
Config& GetConfig();

} // namespace sgfx
