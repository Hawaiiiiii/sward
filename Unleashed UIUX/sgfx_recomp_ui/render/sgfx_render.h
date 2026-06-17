// =============================================================================
// sgfx_render.h — the render-backend contract (was gpu/video.h's GuestTexture +
// gpu/imgui's loaders).
//
// The recomp's ui/ menus draw through Dear ImGui PLUS a custom gradient/outline/
// skew layer (render/imgui_common.h — the callback API, kept verbatim). The only
// other render coupling is the texture type: the menus hold UI sprites and pass
// them to ImGui::AddImage. We abstract that into a small backend contract a host
// implements once (the recomp's impl is plume/D3D12; SGFX swaps its own GPU).
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <imgui.h>
#include "imgui_common.h"   // SetGradient/SetShaderModifier/... callback contract

namespace sgfx { namespace render {

// Opaque GPU texture. Its address is used DIRECTLY as an ImTextureID (void* in stock
// imgui) — exactly as the recomp passed GuestTexture* — so the host's imgui renderer
// receives this pointer and binds `backend`. Complete type so std::unique_ptr<Texture>
// works inside the ui/ translation units; ~Texture is defined by the backend.
struct Texture
{
    void* backend = nullptr;   // host GPU handle
    int   width   = 0;
    int   height  = 0;
    ~Texture();                // host frees `backend`
};

// The three shared UI sprites imgui_utils needs (recomp res/images/common/*). The
// host supplies the pixels (they may be SEGA-derived, so they live with the host,
// not the lib) and uploads them.
enum class UISprite
{
    // shared chrome (imgui_utils / tv_static / button_guide)
    GeneralWindow, Light, Select, OptionsStatic, OptionsStaticFlash, Controller, KBM,
    // achievement_menu
    Trophy,
    // options_menu_thumbnails — the option preview tiles (host supplies pixels; SEGA-derived)
    ThumbDefault, ThumbControlTutorialXB, ThumbControlTutorialPS, ThumbVibrationXB, ThumbVibrationPS,
    ThumbAllowBackgroundInputXB, ThumbAllowBackgroundInputPS, ThumbLanguage, ThumbVoiceLanguage, ThumbHints,
    ThumbAchievementNotifications, ThumbTimeTransitionXB, ThumbTimeTransitionPS, ThumbHorizontalCamera,
    ThumbVerticalCamera, ThumbControllerIcons, ThumbMasterVolume, ThumbMusicVolume, ThumbEffectsVolume,
    ThumbChannelStereo, ThumbChannelSurround, ThumbMusicAttenuation, ThumbBattleTheme, ThumbWindowSize,
    ThumbMonitor, ThumbAspectRatio, ThumbFullscreen, ThumbXboxColorCorrection, ThumbVSyncOff, ThumbVSyncOn,
    ThumbFPS, ThumbBrightness, ThumbAntialiasingNone, ThumbAntialiasing2x, ThumbAntialiasing4x, ThumbAntialiasing8x,
    ThumbTransparencyAntialiasingFalse, ThumbTransparencyAntialiasingTrue, ThumbShadowResolutionX512,
    ThumbShadowResolutionX1024, ThumbShadowResolutionX2048, ThumbShadowResolutionX4096, ThumbShadowResolutionX8192,
    ThumbGITextureFilteringBilinear, ThumbGITextureFilteringBicubic, ThumbMotionBlurOff, ThumbMotionBlurOriginal,
    ThumbMotionBlurEnhanced, ThumbMovieScaleFit, ThumbMovieScaleFill, ThumbUIAlignmentCentre, ThumbUIAlignmentEdge,
    // options_menu / installer_wizard
    OptionsMilesElectric, Install001, Install002, Install003, Install004, Install005, Install006, Install007,
    Install008, MilesElectricIcon, ArrowCircle, PulseInstall, HedgeDev,
};
std::unique_ptr<Texture> LoadUISprite(UISprite sprite);

// General texture upload from decoded image bytes (host decodes DDS/PNG + uploads).
std::unique_ptr<Texture> LoadTexture(const uint8_t* data, size_t size);

}} // namespace sgfx::render

// Font registry shim (was gpu/imgui/imgui_snapshot.h :: ImFontAtlasSnapshot). The menus
// fetch fonts by file name; the host registers the real .otf atlases. Same symbol as the
// recomp so the menu bodies stay unchanged.
struct ImFontAtlasSnapshot
{
    static ImFont* GetFont(const char* name);
};
