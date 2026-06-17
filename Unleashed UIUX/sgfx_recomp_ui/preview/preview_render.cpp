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

SDL_Renderer* g_previewRenderer = nullptr;   // set by preview_main before OptionsMenu::Init()
ImFont*       g_previewFont = nullptr;        // set by preview_main after font upload

namespace sgfx { namespace render {

Texture::~Texture() { if (backend) SDL_DestroyTexture((SDL_Texture*)backend); }

static std::unique_ptr<Texture> MakeTile(int w = 64, int h = 64)
{
    auto t = std::make_unique<Texture>();
    SDL_Texture* tex = SDL_CreateTexture(g_previewRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
    std::vector<uint32_t> px((size_t)w * h, 0xFFFFFFFFu);    // white -> AddImage colour tints it
    SDL_UpdateTexture(tex, nullptr, px.data(), w * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    t->backend = tex; t->width = w; t->height = h;
    return t;
}
std::unique_ptr<Texture> LoadUISprite(UISprite)              { return MakeTile(); }
std::unique_ptr<Texture> LoadTexture(const uint8_t*, size_t) { return MakeTile(); }

}} // namespace sgfx::render

// font registry: one shared font for the preview
ImFont* ImFontAtlasSnapshot::GetFont(const char*) { return g_previewFont; }

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
