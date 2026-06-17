// preview_main.cpp — standalone harness that RUNS the real recomp OptionsMenu from the
// sgfx_recomp_ui library: SDL2 + Dear ImGui (no game) + a custom imgui->SDL_Renderer pass
// that (a) reads every ImTextureID as sgfx::render::Texture* and (b) applies the recomp's
// SetGradient callbacks per-vertex (the in-game pixel shader's closest CPU match). Opens
// the options menu, renders, and screenshots it.
#include "../ui/options_menu.h"
#include "../ui/achievement_menu.h"
#include "../ui/installer_wizard.h"
#include "../render/sgfx_render.h"
#include "../render/imgui_common.h"
#include <SDL.h>
#include <imgui.h>
#include <vector>
#include <string>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>

extern SDL_Renderer* g_previewRenderer;
extern ImFont*       g_previewFont;
extern ImFont*       g_fontDFSoGei;   // set here, read by ImFontAtlasSnapshot::GetFont
extern ImFont*       g_fontSeurat;
extern ImFont*       g_fontNewRodin;
void ResetImGuiCallbacks();
void InitImGuiUtils();   // loads the shared 9-slice/light/select sprites used by every menu

static inline int ch(ImU32 c, int s) { return (c >> s) & 0xff; }
static ImU32 Bilerp(ImU32 tl, ImU32 tr, ImU32 br, ImU32 bl, float u, float v)
{
    ImU32 out = 0;
    for (int s = 0; s < 32; s += 8)
    {
        float top = ch(tl, s) + (ch(tr, s) - ch(tl, s)) * u;
        float bot = ch(bl, s) + (ch(br, s) - ch(bl, s)) * u;
        int   val = (int)(top + (bot - top) * v + 0.5f);
        out |= (ImU32)(val < 0 ? 0 : val > 255 ? 255 : val) << s;
    }
    return out;
}

struct Grad { bool on = false; float x0, y0, x1, y1; ImU32 tl, tr, br, bl; };

static void RenderImGui(ImDrawData* dd, SDL_Renderer* r)
{
    Grad g{};
    std::vector<SDL_Vertex> sv;
    for (int n = 0; n < dd->CmdListsCount; ++n)
    {
        const ImDrawList* cl  = dd->CmdLists[n];
        const ImDrawVert* vtx = cl->VtxBuffer.Data;
        const ImDrawIdx*  idx = cl->IdxBuffer.Data;
        for (const ImDrawCmd& cmd : cl->CmdBuffer)
        {
            if (cmd.UserCallback)
            {
                auto which = (ImGuiCallback)(intptr_t)cmd.UserCallback;
                auto* d = (ImGuiCallbackData*)cmd.UserCallbackData;
                if (which == ImGuiCallback::SetGradient && d)
                {
                    auto& sg = d->setGradient;
                    bool zero = !sg.boundsMin[0] && !sg.boundsMin[1] && !sg.boundsMax[0] && !sg.boundsMax[1] &&
                                !sg.gradientTopLeft && !sg.gradientTopRight && !sg.gradientBottomRight && !sg.gradientBottomLeft;
                    if (zero) g.on = false;
                    else g = { true, sg.boundsMin[0], sg.boundsMin[1], sg.boundsMax[0], sg.boundsMax[1],
                               sg.gradientTopLeft, sg.gradientTopRight, sg.gradientBottomRight, sg.gradientBottomLeft };
                }
                continue;   // other sentinels (shader modifiers / marquee) ignored — flat
            }
            SDL_Texture* tex = nullptr;
            if (void* id = (void*)cmd.TextureId) tex = (SDL_Texture*)((sgfx::render::Texture*)id)->backend;
            SDL_Rect clip = { (int)cmd.ClipRect.x, (int)cmd.ClipRect.y,
                              (int)(cmd.ClipRect.z - cmd.ClipRect.x), (int)(cmd.ClipRect.w - cmd.ClipRect.y) };
            if (clip.w <= 0 || clip.h <= 0) continue;
            SDL_RenderSetClipRect(r, &clip);

            sv.clear(); sv.reserve(cmd.ElemCount);
            for (unsigned e = 0; e < cmd.ElemCount; ++e)
            {
                const ImDrawVert& v = vtx[cmd.VtxOffset + idx[cmd.IdxOffset + e]];
                ImU32 col = v.col;
                if (g.on && g.x1 > g.x0 && g.y1 > g.y0)
                {
                    float u = (v.pos.x - g.x0) / (g.x1 - g.x0); u = u < 0 ? 0 : u > 1 ? 1 : u;
                    float w = (v.pos.y - g.y0) / (g.y1 - g.y0); w = w < 0 ? 0 : w > 1 ? 1 : w;
                    col = Bilerp(g.tl, g.tr, g.br, g.bl, u, w);   // gradient replaces (× white sprite)
                }
                SDL_Vertex out;
                out.position = { v.pos.x, v.pos.y };
                out.tex_coord = { v.uv.x, v.uv.y };
                out.color = *(SDL_Color*)&col;
                sv.push_back(out);
            }
            SDL_RenderGeometry(r, tex, sv.data(), (int)sv.size(), nullptr, 0);
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
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    g_previewRenderer = r;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime   = 1.0f / 60.0f;
    io.IniFilename = nullptr;

    // Fonts: load the REAL game typefaces the recomp asks for by name. DFSoGei is the
    // title/tab/option-name face (dfsoge7.ttc), Seurat + NewRodin the body faces. $SGFX_FONT_DIR
    // overrides the dir; default = the extracted game font set. The recomp renders these via an
    // MSDF shader in-game; baking them as ordinary imgui atlases here is a close CPU match.
    std::string fontDir = std::getenv("SGFX_FONT_DIR") ? std::getenv("SGFX_FONT_DIR")
                                                       : "C:/swardbuild/sgfx_ui/assets/fonts";
    auto loadFont = [&](const char* file, float size) -> ImFont* {
        std::string path = fontDir + "/" + file;
        return io.Fonts->AddFontFromFileTTF(path.c_str(), size);
    };
    g_fontDFSoGei  = loadFont("dfsoge7.ttc", 24.0f);            // DFSoGeiStd-W7 (TrueType collection)
    g_fontSeurat   = loadFont("FOT-SeuratPro-M.otf", 22.0f);
    g_fontNewRodin = loadFont("FOT-NewRodinPro-DB.otf", 22.0f);
    ImFont* fallback = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 22.0f);
    if (!fallback) fallback = io.Fonts->AddFontDefault();
    if (!g_fontDFSoGei)  g_fontDFSoGei  = fallback;
    if (!g_fontSeurat)   g_fontSeurat   = fallback;
    if (!g_fontNewRodin) g_fontNewRodin = fallback;
    g_previewFont = g_fontDFSoGei;

    unsigned char* pixels = nullptr; int fw = 0, fh = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &fw, &fh);
    SDL_Texture* fontTex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, fw, fh);
    SDL_UpdateTexture(fontTex, nullptr, pixels, fw * 4);
    SDL_SetTextureBlendMode(fontTex, SDL_BLENDMODE_BLEND);
    static sgfx::render::Texture fontWrap; fontWrap.backend = fontTex; fontWrap.width = fw; fontWrap.height = fh;
    io.Fonts->SetTexID((ImTextureID)(void*)&fontWrap);

    InitImGuiUtils();   // bind the shared sprites before any menu draws (else 9-slice = white)

    // pick the screen to render: preview.exe <out.bmp> [options|achievements|installer]
    const char* screen = (argc > 2) ? argv[2] : "options";
    bool achievements = std::strcmp(screen, "achievements") == 0;
    bool installer    = std::strcmp(screen, "installer") == 0;
    if      (installer)    { InstallerWizard::Init();  InstallerWizard::s_isVisible = true; }
    else if (achievements) { AchievementMenu::Init();  AchievementMenu::Open(); }
    else                   { OptionsMenu::Init();      OptionsMenu::Open(false); }

    // 150 frames @ 1/60s => ~2.5s of menu time: the container intro finishes at frame 60
    // and the category tabs + option rows settle after, so the screenshot shows the full menu.
    for (int frame = 0; frame < 150; ++frame)
    {
        ResetImGuiCallbacks();
        ImGui::NewFrame();
        SDL_SetRenderDrawColor(r, 16, 18, 26, 255);
        SDL_RenderClear(r);
        if      (installer)    InstallerWizard::Draw();
        else if (achievements) AchievementMenu::Draw();
        else                   OptionsMenu::Draw();
        ImGui::Render();
        RenderImGui(ImGui::GetDrawData(), r);
        SDL_RenderPresent(r);
    }

    SDL_Surface* shot = SDL_CreateRGBSurfaceWithFormat(0, 1280, 720, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_RenderReadPixels(r, nullptr, SDL_PIXELFORMAT_ARGB8888, shot->pixels, shot->pitch);
    const char* out = (argc > 1) ? argv[1] : "preview_options.bmp";
    SDL_SaveBMP(shot, out);
    printf("wrote preview screenshot: %s\n", out);
    SDL_FreeSurface(shot);
    return 0;
}
