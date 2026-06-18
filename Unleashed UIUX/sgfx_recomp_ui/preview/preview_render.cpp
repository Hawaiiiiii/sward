// preview_render.cpp — the render-contract + leftover hooks for the standalone preview
// harness, so the REAL recomp options_menu links + renders. A demo backend:
//  - render::Texture  = an SDL_Texture (placeholder white tile; host loads real sprites)
//  - AddImGuiCallback = no-op (gradients/outlines render FLAT here; the recomp's gpu/imgui
//    shader does them in-game — fidelity layer, not needed to prove the code RUNS)
//  - GetFont = one shared font the harness loaded
#include "../render/sgfx_render.h"
#include "../platform/sgfx_platform.h"
#include "../platform/sgfx_window.h"
#include "../ui/game_window.h"
#include <SDL.h>
#include <imgui.h>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>   // decode the real recomp sprite PNGs (host-supplied assets)

SDL_Renderer* g_previewRenderer = nullptr;   // set by preview_main before OptionsMenu::Init()
ImFont*       g_previewFont = nullptr;        // set by preview_main after font upload
// the three real game fonts (set by preview_main); the recomp asks for them by file name
ImFont*       g_fontDFSoGei = nullptr;        // DFSoGeiStd-W7 (dfsoge7.ttc) — titles/tabs/option names
ImFont*       g_fontSeurat  = nullptr;        // FOT-SeuratPro-M
ImFont*       g_fontNewRodin = nullptr;       // FOT-NewRodinPro-DB/UB

namespace sgfx { namespace render {

Texture::~Texture() { if (backend) SDL_DestroyTexture((SDL_Texture*)backend); }

static std::unique_ptr<Texture> MakeTile(uint32_t rgba = 0xFFFFFFFFu, int w = 64, int h = 64)
{
    auto t = std::make_unique<Texture>();
    SDL_Texture* tex = SDL_CreateTexture(g_previewRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
    std::vector<uint32_t> px((size_t)w * h, rgba);          // white -> AddImage colour tints it
    SDL_UpdateTexture(tex, nullptr, px.data(), w * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    t->backend = tex; t->width = w; t->height = h;
    return t;
}

// Real recomp sprite PNGs, indexed by UISprite (MUST match the enum order in
// render/sgfx_render.h). Host supplies these (SEGA-derived); the preview loads them from
// $SGFX_SPRITE_ROOT (default = the extracted recomp set). "" = no asset -> tile fallback.
static const char* const k_spriteFiles[] = {
    "general_window.png", "light.png", "select.png", "options_static.png", "options_static_flash.png",
    "controller.png", "kbm.png", "trophy.png",
    "thumbnails/default.png", "thumbnails/control_tutorial_xb.png", "thumbnails/control_tutorial_ps.png",
    "thumbnails/vibration_xb.png", "thumbnails/vibration_ps.png", "thumbnails/allow_background_input_xb.png",
    "thumbnails/allow_background_input_ps.png", "thumbnails/language.png", "thumbnails/voice_language.png",
    "thumbnails/hints.png", "thumbnails/achievement_notifications.png", "thumbnails/time_transition_xb.png",
    "thumbnails/time_transition_ps.png", "thumbnails/horizontal_camera.png", "thumbnails/vertical_camera.png",
    "thumbnails/controller_icons.png", "thumbnails/master_volume.png", "thumbnails/music_volume.png",
    "thumbnails/effects_volume.png", "thumbnails/channel_stereo.png", "thumbnails/channel_surround.png",
    "thumbnails/music_attenuation.png", "thumbnails/battle_theme.png", "thumbnails/window_size.png",
    "thumbnails/monitor.png", "thumbnails/aspect_ratio.png", "thumbnails/fullscreen.png",
    "thumbnails/xbox_color_correction.png", "thumbnails/vsync_off.png", "thumbnails/vsync_on.png",
    "thumbnails/fps.png", "thumbnails/brightness.png", "thumbnails/antialiasing_none.png",
    "thumbnails/antialiasing_2x.png", "thumbnails/antialiasing_4x.png", "thumbnails/antialiasing_8x.png",
    "thumbnails/transparency_antialiasing_false.png", "thumbnails/transparency_antialiasing_true.png",
    "thumbnails/shadow_resolution_x512.png", "thumbnails/shadow_resolution_x1024.png",
    "thumbnails/shadow_resolution_x2048.png", "thumbnails/shadow_resolution_x4096.png",
    "thumbnails/shadow_resolution_x8192.png", "thumbnails/gi_texture_filtering_bilinear.png",
    "thumbnails/gi_texture_filtering_bicubic.png", "thumbnails/motion_blur_off.png",
    "thumbnails/motion_blur_original.png", "thumbnails/motion_blur_enhanced.png", "thumbnails/movie_scale_fit.png",
    "thumbnails/movie_scale_fill.png", "thumbnails/ui_alignment_centre.png", "thumbnails/ui_alignment_edge.png",
    "miles_electric.png", "inst_install_001.png", "inst_install_002.png", "inst_install_003.png",
    "inst_install_004.png", "inst_install_005.png", "inst_install_006.png", "inst_install_007.png",
    "inst_install_008.png", "inst_miles_icon.png", "inst_arrow_circle.png", "inst_pulse.png",
    "" /* HedgeDev: not extracted */,
};

static std::string SpriteRoot()
{
    if (const char* env = std::getenv("SGFX_SPRITE_ROOT")) return env;
    return "C:/swardbuild/sgfx_ui/assets/recomp";   // the extracted recomp set (dev default)
}

std::unique_ptr<Texture> LoadUISprite(UISprite s)
{
    int idx = (int)s;
    const char* file = (idx >= 0 && idx < (int)(sizeof(k_spriteFiles) / sizeof(*k_spriteFiles))) ? k_spriteFiles[idx] : "";
    if (file && *file)
    {
        std::string path = SpriteRoot() + "/" + file;
        int w = 0, h = 0, comp = 0;
        if (unsigned char* px = stbi_load(path.c_str(), &w, &h, &comp, 4))
        {
            auto t = std::make_unique<Texture>();
            SDL_Texture* tex = SDL_CreateTexture(g_previewRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
            SDL_UpdateTexture(tex, nullptr, px, w * 4);
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            stbi_image_free(px);
            t->backend = tex; t->width = w; t->height = h;
            return t;
        }
    }
    // fallback: GeneralWindow as a dark frame, everything else white (AddImage tints it)
    return MakeTile(s == UISprite::GeneralWindow ? 0xEB1E1814u : 0xFFFFFFFFu);
}
std::unique_ptr<Texture> LoadTexture(const uint8_t* data, size_t size)
{
    int w = 0, h = 0, comp = 0;
    if (unsigned char* px = stbi_load_from_memory(data, (int)size, &w, &h, &comp, 4))
    {
        auto t = std::make_unique<Texture>();
        SDL_Texture* tex = SDL_CreateTexture(g_previewRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
        SDL_UpdateTexture(tex, nullptr, px, w * 4);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        stbi_image_free(px);
        t->backend = tex; t->width = w; t->height = h;
        return t;
    }
    return MakeTile();
}

}} // namespace sgfx::render

// font registry: the recomp fetches fonts by file name (e.g. "DFSoGeiStd-W7.otf",
// "FOT-SeuratPro-M.otf", "FOT-NewRodinPro-DB.otf"); map each to the real game font.
ImFont* ImFontAtlasSnapshot::GetFont(const char* name)
{
    if (name)
    {
        if (std::strstr(name, "Seurat")   && g_fontSeurat)   return g_fontSeurat;
        if (std::strstr(name, "NewRodin") && g_fontNewRodin) return g_fontNewRodin;
        if ((std::strstr(name, "DFSoGei") || std::strstr(name, "DFSo")) && g_fontDFSoGei) return g_fontDFSoGei;
    }
    return g_fontDFSoGei ? g_fontDFSoGei : g_previewFont;
}

// the custom imgui render layer's callback API — AUTHENTIC mechanism (verbatim from the
// recomp's gpu/imgui/imgui_common.cpp): a per-frame ring of ImGuiCallbackData, each added
// to the background draw list as a sentinel callback. preview_main's render reads the
// SetGradient sentinels and applies the gradient per-vertex (the shader does it per-pixel
// in-game; per-vertex is a close match for the menus' tessellated geometry).
#include <vector>
#include <memory>
static std::vector<std::unique_ptr<ImGuiCallbackData>> g_callbackData;
static unsigned g_callbackDataIndex = 0;
ImGuiCallbackData* AddImGuiCallback(ImGuiCallback callback)
{
    if (g_callbackDataIndex >= g_callbackData.size())
        g_callbackData.emplace_back(std::make_unique<ImGuiCallbackData>());
    auto& cd = g_callbackData[g_callbackDataIndex++];
    ImGui::GetBackgroundDrawList()->AddCallback(reinterpret_cast<ImDrawCallback>(callback), cd.get());
    return cd.get();
}
void ResetImGuiCallbacks() { g_callbackDataIndex = 0; }

// options_menu's window/video deps (host infra; demo values)
std::vector<SDL_DisplayMode> GameWindow::GetDisplayModes(bool, bool)
{
    SDL_DisplayMode m{}; m.w = 1280; m.h = 720; m.refresh_rate = 60;
    return { m };
}
void GameWindow::GetSizeInPixels(int* w, int* h) { if (w) *w = s_width; if (h) *h = s_height; }
int  GameWindow::GetDisplayCount() { return 1; }
void VideoConfigValueChangedCallback(IConfigDef*) {}
