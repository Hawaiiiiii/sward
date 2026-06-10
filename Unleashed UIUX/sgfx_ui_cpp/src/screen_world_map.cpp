// =============================================================================
// screen_world_map.cpp — the World Map hub, 2D UI OVERLAY matched to third video.mp4
// (frames _wmf_90..103.png, 1920x1080 -> 1280x720). The hub is a 3D rotating Earth
// (Hedgehog-Engine scene — reproduced separately as a 3D pass); THIS file is the 2D
// overlay floating over it, with all layout MEASURED from the video:
//   * gold "WORLD MAP" title on a green dot-matrix grid band (top-left);
//   * left totals column — 4 rows of [icon slot][bright-green number] on green-grid
//     bands (lives / rings / sun-medals / moon-medals). The icons are SEGA sprites
//     (Sonic head, ring, medals) -> placeholder slots here; drop in your own PNGs;
//   * the right STAGE PREVIEW panel (appears on location-select): green-grid preview
//     region + orange marker, white description paragraph, two medal count rows;
//   * the "HOLOSKA" location tag on the globe; footer (Pass Time / Select).
// Real fonts: FOT-SeuratPro MSDF (desc), FOT-NewRodinPro MSDF (numbers), DFSoGei (title).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0, g_glyphTex = -1;
struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_X = { 0.16016f, 0.00781f, 0.23047f, 0.07422f };

// ---- palette (green hub theme; greens from the WM video grid bands) ---------
const uint32_t C_TITLE   = RGBA(255, 190, 33, 255);
const uint32_t C_PANEL   = RGBA(0, 0, 0, 200);
const uint32_t C_OUTER   = RGBA(0, 49, 0, 255);
const uint32_t C_INNER   = RGBA(0, 33, 0, 235);
const uint32_t C_LINE    = RGBA(0, 110, 30, 255);
const uint32_t C_WHITE   = RGBA(255, 255, 255, 255);
const uint32_t C_DESC    = RGBA(240, 246, 238, 255);
const uint32_t C_NUM     = RGBA(158, 223, 66, 255);   // bright yellow-green number (measured)
const uint32_t C_RING    = RGBA(232, 196, 64, 255);
const uint32_t C_LIVES   = RGBA(54, 110, 226, 255);   // Sonic-head blue (placeholder)
const uint32_t C_SUN     = RGBA(232, 120, 40, 255);
const uint32_t C_MOON    = RGBA(80, 150, 235, 255);
const uint32_t C_MARK    = RGBA(244, 150, 30, 255);   // orange location marker
const uint32_t C_FOOTER  = RGBA(224, 238, 226, 255);
const uint32_t C_BG_TOP  = RGBA(6, 10, 20, 255);
const uint32_t C_BG_BOT  = RGBA(2, 4, 9, 255);
const uint32_t C_OCEAN_T = RGBA(40, 96, 168, 255);
const uint32_t C_OCEAN_B = RGBA(12, 34, 78, 255);
const uint32_t C_LAND    = RGBA(70, 120, 70, 230);
const uint32_t C_ATMO    = RGBA(120, 180, 255, 60);

// ---- layout (1280x720, measured from _wmf_103.png) --------------------------
constexpr float GLOBE_CX = 470, GLOBE_CY = 402, GLOBE_R = 250;
constexpr float TOT_X0 = 8,  TOT_Y0 = 114, TOT_X1 = 296, TOT_Y1 = 290;  // totals panel
constexpr float TROW0  = 135, TPITCH = 43;                              // first row center, pitch
constexpr float IP_X0 = 878, IP_Y0 = 150, IP_X1 = 1182, IP_Y1 = 426;    // STAGE PREVIEW panel
constexpr float GRID = 9.0f;

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}
void Reset() {}
void Input(const ScreenInput&) {}

// a green dot-matrix grid panel (the WM/options container recipe): checkerboard fill
// + bright inner border line.
void DrawGridPanel(float x0, float y0, float x1, float y1, float t) {
    DrawRect({ x0, y0 }, { x1, y1 }, WithAlpha(C_PANEL, t));
    SetModifier(MOD_CHECKERBOARD);
    DrawRect({ x0, y0 }, { x1, y0 + GRID }, WithAlpha(C_OUTER, t));
    DrawRect({ x0, y1 - GRID }, { x1, y1 }, WithAlpha(C_OUTER, t));
    DrawRect({ x0, y0 + GRID }, { x0 + GRID, y1 - GRID }, WithAlpha(C_OUTER, t));
    DrawRect({ x1 - GRID, y0 + GRID }, { x1, y1 - GRID }, WithAlpha(C_OUTER, t));
    DrawRect({ x0 + GRID, y0 + GRID }, { x1 - GRID, y1 - GRID }, WithAlpha(C_INNER, t));
    ResetModifier();
    uint32_t lc = WithAlpha(C_LINE, t); const float g = GRID, L = 2.0f;
    DrawRect({ x0+g, y0+g }, { x1-g, y0+g+L }, lc);
    DrawRect({ x0+g, y1-g-L }, { x1-g, y1-g }, lc);
    DrawRect({ x0+g, y0+g }, { x0+g+L, y1-g }, lc);
    DrawRect({ x1-g-L, y0+g }, { x1-g, y1-g }, lc);
}

// a small placeholder icon slot (the real SEGA sprite drops in here). `ring`=draw a
// ring outline, else a filled rounded marker.
void DrawIconSlot(float cx, float cy, float r, uint32_t col, bool ring, float t) {
    if (ring) {
        DrawRect({ cx - r, cy - r }, { cx + r, cy + r }, WithAlpha(col, t));
        DrawRect({ cx - r*0.45f, cy - r*0.45f }, { cx + r*0.45f, cy + r*0.45f }, WithAlpha(RGBA(0,20,0,255), t));
    } else {
        DrawRect({ cx - r, cy - r }, { cx + r, cy + r }, WithAlpha(col, t));
        DrawRect({ cx - r*0.55f, cy - r*0.4f }, { cx + r*0.55f, cy + r*0.4f }, WithAlpha(RGBA(255,255,255,120), t));
    }
}

void DrawGlobe(float cx, float cy, float r, float t) {
    DrawRect({ cx - r - 6, cy - r - 6 }, { cx + r + 6, cy + r + 6 }, WithAlpha(C_ATMO, t * 0.5f));
    const int N = 96;
    for (int i = 0; i < N; ++i) {
        float yy = cy - r + (2 * r) * i / (float)N;
        float dy = (yy - cy) / r; float hw = r * std::sqrt(std::max(0.0f, 1.0f - dy * dy));
        if (hw <= 0) continue;
        float f = (yy - (cy - r)) / (2 * r);
        uint32_t c = ColourLerp(C_OCEAN_T, C_OCEAN_B, f);
        DrawRect({ cx - hw, yy }, { cx + hw, yy + (2 * r / N) + 1 }, WithAlpha(c, t));
    }
    auto land = [&](float ax, float ay, float w, float h){ DrawRect({ cx+ax, cy+ay }, { cx+ax+w, cy+ay+h }, WithAlpha(C_LAND, t)); };
    land(-120, -110, 90, 60); land(-40, -150, 70, 50); land(20, -40, 110, 80);
    land(-150, 20, 80, 70); land(-30, 60, 130, 90); land(90, -120, 60, 40);
    for (int i = 0; i < 24; ++i) { float xx = cx + r - i * (r/24.0f) * 0.5f; DrawRect({ xx, cy - r }, { xx + 2, cy + r }, WithAlpha(RGBA(0,0,0,255), t * (0.02f + i*0.004f))); }
}

void Draw(double openSec) {
    const float t = (float)ComputeMotion(openSec, 0.0, 14.0);
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);
    uint32_t s = 0x2468ace1u;
    for (int i = 0; i < 110; ++i) { s = s*1664525u+1013904223u; float x=(float)((s>>9)%1280); s=s*1664525u+1013904223u; float y=(float)((s>>9)%720); s=s*1664525u+1013904223u; int b=50+(int)((s>>9)%160); DrawRect({x,y},{x+1,y+1}, WithAlpha(RGBA(b,b,b,255), t*0.7f)); }

    DrawGlobe(GLOBE_CX, GLOBE_CY, GLOBE_R * (0.6f + 0.4f * t), t);

    // ---- gold "WORLD MAP" title on a green-grid band ----
    DrawGridPanel(8, 56, 408, 104, t);
    SetFont(g_fDF);
    DrawTextBevel({ 30, 60 }, 30.0f, WithAlpha(C_TITLE, t), "WORLD MAP");

    // ---- left totals column (4 rows: icon slot + bright-green number) ----
    DrawGridPanel(TOT_X0, TOT_Y0, TOT_X1, TOT_Y1, t);
    {
        struct Row { uint32_t icol; bool ring; const char* val; } rows[] = {
            { C_LIVES, false, "99" }, { C_RING, true, "999999" },
            { C_SUN, true, "180" }, { C_MOON, true, "180" },
        };
        SetFont(g_fRodin);
        for (int i = 0; i < 4; ++i) {
            float cy = TROW0 + i * TPITCH;
            if (i) DrawRect({ TOT_X0 + GRID, cy - TPITCH*0.5f }, { TOT_X1 - GRID, cy - TPITCH*0.5f + 1 }, WithAlpha(C_LINE, t * 0.6f));
            DrawIconSlot(70, cy, 14, rows[i].icol, rows[i].ring, t);
            DrawText({ 118, cy - 13 }, 24.0f, WithAlpha(C_NUM, t), rows[i].val);
        }
    }

    // ---- HOLOSKA location tag on the globe ----
    {
        float bx = 232, by = 360, bw = 150, bh = 26;
        DrawRect({ bx, by }, { bx + bw, by + bh }, WithAlpha(RGBA(8,26,14,235), t));
        DrawRect({ bx, by }, { bx + bw, by + 2 }, WithAlpha(C_LINE, t));
        DrawRect({ bx, by + bh - 2 }, { bx + bw, by + bh }, WithAlpha(C_LINE, t));
        SetFont(g_fSeurat);
        DrawTextAligned({ bx + 8, by }, { bx + bw - 6, by + bh }, 17.0f, WithAlpha(C_NUM, t), "HOLOSKA", Align::Center, true, true);
        DrawRect({ bx + bw, by + bh*0.5f - 1 }, { GLOBE_CX + 30, by + bh*0.5f + 1 }, WithAlpha(C_LINE, t));
        DrawRect({ GLOBE_CX + 26, by + bh*0.5f - 5 }, { GLOBE_CX + 36, by + bh*0.5f + 5 }, WithAlpha(C_MARK, t));
    }

    // ---- right STAGE PREVIEW panel (appears on location-select) ----
    DrawGridPanel(IP_X0, IP_Y0, IP_X1, IP_Y1, t);
    {
        float ix0 = IP_X0 + 16, ix1 = IP_X1 - 16;
        // preview-grid region (top) with an orange location marker; the real stage
        // thumbnail drops in here (SEGA art) — the green grid shows through otherwise.
        float py0 = IP_Y0 + 14, py1 = py0 + 96;
        SetModifier(MOD_CHECKERBOARD);
        DrawRect({ ix0, py0 }, { ix1, py1 }, WithAlpha(RGBA(0, 60, 8, 255), t));
        ResetModifier();
        DrawRect({ ix1 - 22, py0 + 8 }, { ix1 - 10, py0 + 20 }, WithAlpha(C_MARK, t));   // marker dot
        // description paragraph (white)
        SetFont(g_fSeurat);
        const char* desc[] = { "Covered in snow, this northern", "land is gripped by extreme cold", "and dotted with houses of ice." };
        float dy = py1 + 14;
        for (auto* d : desc) { DrawText({ ix0, dy }, 20.0f, WithAlpha(C_DESC, t), d); dy += 27; }
        // two medal count rows (icon + green count)
        SetFont(g_fRodin);
        float sy = IP_Y1 - 44;
        DrawIconSlot(ix0 + 12, sy + 9, 11, C_SUN, true, t);  DrawText({ ix0 + 30, sy }, 20.0f, WithAlpha(C_NUM, t), "18 / 18");
        DrawIconSlot(ix0 + 150, sy + 9, 11, C_RING, true, t); DrawText({ ix0 + 168, sy }, 20.0f, WithAlpha(C_NUM, t), "22 / 22");
    }

    // ---- footer (Pass Time / Select) ----
    {
        SetFont(g_fRodin);
        float hx = 470, hcy = 690;
        auto glyph = [&](const UV& g){ if (g_glyphTex<0) return; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=26.0f, gw=gh*asp; DrawImage(g_glyphTex,{hx,hcy-gh*0.5f},{hx+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, WithAlpha(C_WHITE,t)); hx+=gw+6; };
        auto word=[&](const char* w,float pad){ DrawText({hx,hcy-11},20.0f,WithAlpha(C_FOOTER,t),w); hx+=MeasureText(20.0f,w).x+pad; };
        glyph(GLYPH_X); word("Pass Time", 40);
        glyph(GLYPH_A); word("Select", 10);
    }
    ResetFont();
}

} // namespace

void WorldMapInit() { Init(); }
void WorldMapDraw(double openSeconds) { Draw(openSeconds); }
void WorldMapInput(const ScreenInput& in) { Input(in); }
void WorldMapReset() { Reset(); }
