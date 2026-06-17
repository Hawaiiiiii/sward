// preview_main.cpp — a standalone harness that RUNS the real recomp OptionsMenu from
// the sgfx_recomp_ui library: SDL window + Dear ImGui (no backend, manual frame) + a
// custom imgui->SDL_Renderer pass that treats every ImTextureID as sgfx::render::Texture*.
// Renders the open options menu and writes a screenshot. Gradients are FLAT here (the
// preview stubs the shader callbacks); this proves the authentic menu code RUNS.
#include "../ui/options_menu.h"
#include "../render/sgfx_render.h"
#include <SDL.h>
#include <imgui.h>
#include <vector>
#include <cstdio>
#include <cstddef>

extern SDL_Renderer* g_previewRenderer;
extern ImFont*       g_previewFont;

static void RenderImGui(ImDrawData* dd, SDL_Renderer* r)
{
    for (int n = 0; n < dd->CmdListsCount; ++n)
    {
        const ImDrawList* cl  = dd->CmdLists[n];
        const ImDrawVert* vtx = cl->VtxBuffer.Data;
        const ImDrawIdx*  idx = cl->IdxBuffer.Data;
        for (const ImDrawCmd& cmd : cl->CmdBuffer)
        {
            if (cmd.UserCallback) { cmd.UserCallback(cl, &cmd); continue; }
            SDL_Texture* tex = nullptr;
            if (void* id = (void*)cmd.TextureId)
                tex = (SDL_Texture*)((sgfx::render::Texture*)id)->backend;
            SDL_Rect clip = { (int)cmd.ClipRect.x, (int)cmd.ClipRect.y,
                              (int)(cmd.ClipRect.z - cmd.ClipRect.x), (int)(cmd.ClipRect.w - cmd.ClipRect.y) };
            if (clip.w <= 0 || clip.h <= 0) continue;
            SDL_RenderSetClipRect(r, &clip);
            const char* base = (const char*)(vtx + cmd.VtxOffset);
            SDL_RenderGeometryRaw(r, tex,
                (const float*)(base + offsetof(ImDrawVert, pos)), sizeof(ImDrawVert),
                (const SDL_Color*)(base + offsetof(ImDrawVert, col)), sizeof(ImDrawVert),
                (const float*)(base + offsetof(ImDrawVert, uv)), sizeof(ImDrawVert),
                cl->VtxBuffer.Size - cmd.VtxOffset,
                idx + cmd.IdxOffset, cmd.ElemCount, sizeof(ImDrawIdx));
        }
    }
    SDL_RenderSetClipRect(r, nullptr);
}

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { printf("SDL_Init failed: %s\n", SDL_GetError()); return 1; }
    SDL_Window* win = SDL_CreateWindow("sgfx_recomp_ui preview", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_HIDDEN);
    SDL_Renderer* r = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (!r) { printf("SDL_CreateRenderer failed: %s\n", SDL_GetError()); return 1; }
    g_previewRenderer = r;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime   = 1.0f / 60.0f;
    io.IniFilename = nullptr;

    ImFont* f = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 22.0f);
    if (!f) f = io.Fonts->AddFontDefault();
    g_previewFont = f;

    unsigned char* pixels = nullptr; int fw = 0, fh = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &fw, &fh);
    SDL_Texture* fontTex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, fw, fh);
    SDL_UpdateTexture(fontTex, nullptr, pixels, fw * 4);
    SDL_SetTextureBlendMode(fontTex, SDL_BLENDMODE_BLEND);
    static sgfx::render::Texture fontWrap; fontWrap.backend = fontTex; fontWrap.width = fw; fontWrap.height = fh;
    io.Fonts->SetTexID((ImTextureID)(void*)&fontWrap);

    OptionsMenu::Init();
    OptionsMenu::Open(false);

    for (int frame = 0; frame < 40; ++frame)
    {
        ImGui::NewFrame();
        SDL_SetRenderDrawColor(r, 18, 20, 28, 255);
        SDL_RenderClear(r);
        OptionsMenu::Draw();
        ImGui::Render();
        RenderImGui(ImGui::GetDrawData(), r);
        SDL_RenderPresent(r);
    }

    SDL_Surface* shot = SDL_CreateRGBSurfaceWithFormat(0, 1280, 720, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_RenderReadPixels(r, nullptr, SDL_PIXELFORMAT_ARGB8888, shot->pixels, shot->pitch);
    const char* out = (argc > 1) ? argv[1] : "preview_options.bmp";
    SDL_SaveBMP(shot, out);
    printf("wrote preview screenshot: %s (%dx%d font atlas)\n", out, fw, fh);
    SDL_FreeSurface(shot);
    return 0;
}
