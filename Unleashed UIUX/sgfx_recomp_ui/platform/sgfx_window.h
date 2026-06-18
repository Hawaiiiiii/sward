// =============================================================================
// sgfx_window.h — window / display / video platform services (was
// gpu/rhi/plume_render_interface_types.h + os/* + sdl_events.h + gpu/video.h's Video).
// These are HOST infrastructure (SGFX provides its own window/GPU); the lib declares
// the contract so game_window.cpp (the recomp's reference impl) + the menus' display
// options compile. Pulls SDL, so it's included only by the window/menu files that
// already depend on SDL — never by the core UI headers.
// =============================================================================
#pragma once

#include <cstdint>
#include <SDL.h>
#include "sgfx_config.h"   // IConfigDef (VideoConfigValueChangedCallback)

// native render-window handle (was plume::RenderWindow). On Win32 it wraps an HWND.
namespace plume { using RenderWindow = void*; }

// OS queries (was os/user.h + os/version.h)
namespace os::user    { bool IsDarkTheme(); }
namespace os::version { struct OSVersion { uint32_t Major{}; uint32_t Build{}; }; OSVersion GetOSVersion(); }

// synthetic SDL events the window pushes (was sdl_events.h)
void SDL_ResizeEvent(SDL_Window* pWindow, int width, int height);
void SDL_MoveEvent(SDL_Window* pWindow, int x, int y);
#define SDL_USER_EVILSONIC (SDL_USEREVENT + 1)

// GPU swapchain flag (host's renderer sets it) + viewport size (was gpu/video.h)
extern bool g_needsResize;
class Video
{
public:
    static inline uint32_t s_viewportWidth = 1280;
    static inline uint32_t s_viewportHeight = 720;
    static void WaitForGPU();
    static void WaitOnSwapChain();
    static void Present();
};
void VideoConfigValueChangedCallback(IConfigDef* def);   // host renderer apply hook

// window/taskbar icon bytes — host-supplied (SEGA-derived; not in the lib)
extern const unsigned char g_game_icon[];
extern const unsigned char g_game_icon_night[];
extern const size_t g_game_icon_size;
extern const size_t g_game_icon_night_size;
