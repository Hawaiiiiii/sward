// =============================================================================
// screen_world_map.cpp — the World Map hub 2D overlay, measured from the WM
// video frames (_wmf_90..103) and UPGRADED from live capture session8 54-71s
// (full numeric spec: game_captures/WM_PAUSE_SUBSTATE_SPEC.md):
//   * gold "WORLD MAP" title on a green dot-matrix band; left totals column;
//   * the 3D rotating Earth (separate 3D pass; 2D placeholder disc here);
//   * STAGE-INFO PANEL (right, on location select): dark-green LED circuit
//     fill, green accent rails + corner stubs, stage-photo slot with flag +
//     sun-medallion overlays, 6-line description (pitch 32.9), medal counter
//     rows (yellow counts + green totals) over dashed underlines;
//   * floating STAGE NAME label (italic caps, lime->gold gradient + outline)
//     with a gradient leader rule toward the map cursor;
//   * the "Go to the village / Select stage" POPUP sub-state (accept):
//     green scanline panel, gradient highlight row, ~33% scene dim, footer lit.
// Real fonts: Seurat MSDF (desc), NewRodin MSDF (numbers), DFSoGei (title/name).
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
const UV GLYPH_B = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };
const UV GLYPH_X = { 0.16016f, 0.00781f, 0.23047f, 0.07422f };

// ---- palette ------------------------------------------------------------------
const uint32_t C_TITLE   = RGBA(255, 190, 33, 255);
const uint32_t C_PANEL   = RGBA(0, 0, 0, 200);
const uint32_t C_OUTER   = RGBA(0, 49, 0, 255);
const uint32_t C_INNER   = RGBA(0, 33, 0, 235);
const uint32_t C_LINE    = RGBA(0, 110, 30, 255);
const uint32_t C_WHITE   = RGBA(255, 255, 255, 255);
const uint32_t C_NUM     = RGBA(158, 223, 66, 255);
const uint32_t C_RING    = RGBA(232, 196, 64, 255);
const uint32_t C_LIVES   = RGBA(54, 110, 226, 255);
const uint32_t C_SUN     = RGBA(232, 120, 40, 255);
const uint32_t C_MOON    = RGBA(80, 150, 235, 255);
const uint32_t C_FOOTER  = RGBA(224, 238, 226, 255);
const uint32_t C_BG_TOP  = RGBA(6, 10, 20, 255);
const uint32_t C_BG_BOT  = RGBA(2, 4, 9, 255);
const uint32_t C_OCEAN_T = RGBA(40, 96, 168, 255);
const uint32_t C_OCEAN_B = RGBA(12, 34, 78, 255);
const uint32_t C_LAND    = RGBA(70, 120, 70, 230);
const uint32_t C_ATMO    = RGBA(120, 180, 255, 60);
// stage-info panel (measured: LED circuit board)
const uint32_t C_SIP_CELL = RGBA(16, 55, 20, 255);     // lit cell stripe
const uint32_t C_SIP_GRID = RGBA(4, 18, 5, 255);       // grid line / dark fill
const uint32_t C_SIP_RAIL = RGBA(20, 81, 18, 255);     // accent rails
const uint32_t C_SIP_DESC = RGBA(196, 232, 196, 255);  // green-white desc text
const uint32_t C_CNT_YELLOW = RGBA(206, 224, 66, 255); // medal counts
const uint32_t C_CNT_GREEN  = RGBA(81, 196, 26, 255);  // "/total"
const uint32_t C_DASH       = RGBA(34, 54, 28, 255);   // dashed underline
// floating stage label
const uint32_t C_LBL_T = RGBA(107, 162, 0, 255), C_LBL_B = RGBA(157, 145, 9, 255);
const uint32_t C_LDR_L = RGBA(192, 237, 21, 255), C_LDR_R = RGBA(39, 214, 21, 255);
// popup
const uint32_t C_POP_FILL = RGBA(20, 83, 18, 255);
const uint32_t C_POP_HI_T = RGBA(118, 148, 36, 255), C_POP_HI_B = RGBA(88, 205, 45, 255);
const uint32_t C_POP_TXT  = RGBA(236, 236, 237, 255);

// ---- layout ---------------------------------------------------------------------
constexpr float GLOBE_CX = 470, GLOBE_CY = 402, GLOBE_R = 250;
constexpr float TOT_X0 = 8,  TOT_Y0 = 114, TOT_X1 = 296, TOT_Y1 = 290;
constexpr float TROW0  = 135, TPITCH = 43;
// stage-info panel (measured from session8)
constexpr float SIP_X0 = 845.3f, SIP_Y0 = 117.3f, SIP_X1 = 1152.0f, SIP_Y1 = 602.7f;
constexpr float GRID = 9.0f;

bool g_popup = false;     // "Go to the village / Select stage"
int  g_popupSel = 0;

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}
void Reset() { g_popup = false; g_popupSel = 0; }
void Input(const ScreenInput& in) {
    if (g_popup) {
        if (in.up || in.down) g_popupSel ^= 1;
        if (in.cancel || in.accept) { g_popup = false; g_popupSel = 0; }
        return;
    }
    if (in.accept) { g_popup = true; g_popupSel = 0; }
}

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

// the measured stage-info panel: LED circuit fill + rails + photo + desc + medals
void DrawStageInfo(float t) {
    // circuit-board fill: dark base + lit cell stripes in a 16x8 grid
    DrawRect({ SIP_X0, SIP_Y0 }, { SIP_X1, SIP_Y1 }, WithAlpha(C_SIP_GRID, t));
    for (float y = SIP_Y0 + 2; y < SIP_Y1 - 2; y += 8.0f)
        for (float x = SIP_X0 + 2; x < SIP_X1 - 2; x += 16.0f) {
            DrawRect({ x, y + 1.5f }, { x + 14, y + 3.0f }, WithAlpha(C_SIP_CELL, t));
            DrawRect({ x, y + 4.5f }, { x + 14, y + 6.0f }, WithAlpha(C_SIP_CELL, t * 0.8f));
        }
    // accent rails + inward corner stubs
    auto rail = [&](float ry) {
        DrawRect({ 853.3f, ry }, { 1143.3f, ry + 2.7f }, WithAlpha(C_SIP_RAIL, t));
    };
    rail(124.7f); rail(593.3f);
    DrawRect({ 853.3f, 124.7f }, { 856.0f, 135.4f }, WithAlpha(C_SIP_RAIL, t));
    DrawRect({ 1140.6f, 124.7f }, { 1143.3f, 135.4f }, WithAlpha(C_SIP_RAIL, t));
    DrawRect({ 853.3f, 585.3f }, { 856.0f, 596.0f }, WithAlpha(C_SIP_RAIL, t));
    DrawRect({ 1140.6f, 585.3f }, { 1143.3f, 596.0f }, WithAlpha(C_SIP_RAIL, t));
    // stage photo slot (real screenshot drops in; 2:1 plate, flush on the fill)
    DrawVGradient({ 863.3f, 139.3f }, { 1133.3f, 274.7f },
                  WithAlpha(RGBA(34, 52, 70, 255), t), WithAlpha(RGBA(16, 26, 38, 255), t));
    SetFont(g_fSeurat);
    DrawTextAligned({ 863.3f, 139.3f }, { 1133.3f, 274.7f }, 14.0f,
                    WithAlpha(RGBA(110, 130, 150, 255), t), "STAGE PHOTO", Align::Center, true, false);
    // flag overlay (top-left of photo) + sun medallion (top-right)
    DrawRect({ 868, 143.3f }, { 908, 170.7f }, WithAlpha(RGBA(24, 32, 80, 255), t));
    DrawRect({ 870, 145.3f }, { 906, 168.7f }, WithAlpha(RGBA(235, 238, 240, 255), t));
    DrawRect({ 880, 150 }, { 896, 164 }, WithAlpha(RGBA(170, 50, 40, 255), t));
    DrawIconSlot(1114.7f, 158, 14.7f, RGBA(213, 143, 33, 255), true, t);
    // description (6 lines, pitch 32.9, left margin 864)
    {
        const char* L[] = { "The world's art", "capital and home", "to a university",
                            "that attracts", "those in search", "of knowledge." };
        for (int i = 0; i < 6; ++i)
            DrawText({ 864, 301.3f + i * 32.9f }, 22.0f, WithAlpha(C_SIP_DESC, t), L[i]);
    }
    ResetFont();
    // medal counter rows + dashed underlines
    auto dashes = [&](float dy, float dx0, float dx1) {
        for (float x = dx0; x < dx1; x += 8.0f)
            DrawRect({ x, dy }, { x + 4.0f, dy + 1.3f }, WithAlpha(C_DASH, t));
    };
    SetFont(g_fRodin);
    auto counter = [&](float icx, float icy, uint32_t icol, const char* n, const char* tot, float tx) {
        DrawIconSlot(icx, icy, 14.7f, icol, true, t);
        DrawText({ tx, icy - 9 }, 20.0f, WithAlpha(C_CNT_YELLOW, t), n);
        DrawText({ tx + MeasureText(20.0f, n).x + 6, icy - 9 }, 20.0f, WithAlpha(C_CNT_GREEN, t), tot);
    };
    counter(877.3f, 530, C_RING, "16", "/16", 906.7f);
    dashes(540, 864.7f, 990);
    counter(879.3f, 566.7f, C_SUN, "30", "/30", 902.7f);
    counter(1021.3f, 564, C_MOON, "35", "/35", 1046);
    dashes(576, 864, 1134);
    ResetFont();
}

// the floating stage-name label + gradient leader rule (measured: SPAGONIA)
void DrawStageLabel(float t) {
    SetFont(g_fDF);
    SetTextShear(0.22f);
    SetTextStretchX(1.25f);
    const char* NAME = "SPAGONIA";
    // dark outline ring, then the lime->gold gradient face (x355..519, cap y347..371)
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ 355.3f + dx * 2.0f, 340.0f + dy * 2.0f }, 34.0f, WithAlpha(RGBA(10, 22, 4, 255), t), NAME);
    DrawTextGradient({ 355.3f, 340.0f }, 34.0f, WithAlpha(C_LBL_T, t), WithAlpha(C_LBL_B, t), NAME);
    ResetTextStretchX();
    ResetTextShear();
    ResetFont();
    // leader rule: gradient bar + 45-degree elbow toward the cursor
    {
        const V2 c[4] = { { 285.3f, 378 }, { 723.3f, 378 }, { 723.3f, 380.7f }, { 285.3f, 380.7f } };
        const uint32_t col[4] = { WithAlpha(C_LDR_L, t), WithAlpha(C_LDR_R, t),
                                  WithAlpha(C_LDR_R, t), WithAlpha(C_LDR_L, t) };
        DrawQuadGradient(c, col);
        const V2 e[4] = { { 723.3f, 378 }, { 752, 406.7f }, { 750, 410.7f }, { 721.3f, 382 } };
        const uint32_t ec[4] = { WithAlpha(C_LDR_R, t), WithAlpha(C_LDR_R, t),
                                 WithAlpha(C_LDR_R, t), WithAlpha(C_LDR_R, t) };
        DrawQuadGradient(e, ec);
    }
}

// the "Go to the village / Select stage" popup (measured)
void DrawPopup() {
    // ~33% scene dim (black at 66%); the popup + footer stay full-bright
    DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 168));
    SetModifier(MOD_SCANLINE);
    DrawRect({ 472, 276 }, { 808, 428 }, C_POP_FILL);
    ResetModifier();
    const char* OPT[2] = { "Go to the village", "Select stage" };
    // highlight box on the selected row (gradient + scanlines)
    {
        float hy = (g_popupSel == 0) ? 290.7f : 361.4f;
        SetModifier(MOD_SCANLINE);
        DrawVGradient({ 484.7f, hy }, { 795.4f, hy + 50.7f }, C_POP_HI_T, C_POP_HI_B);
        ResetModifier();
    }
    SetFont(g_fRodin);
    for (int i = 0; i < 2; ++i) {
        float w = MeasureText(26.0f, OPT[i]).x;
        DrawText({ 640 - w * 0.5f, 303.3f + i * 70.7f }, 26.0f, C_POP_TXT, OPT[i]);
    }
    ResetFont();
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

    // ---- left totals column ----
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

    DrawStageLabel(t);
    DrawStageInfo(t);

    // ---- footer (Pass Time / Select) ----
    {
        SetFont(g_fRodin);
        float hx = 470, hcy = 690;
        auto glyph = [&](const UV& g){ if (g_glyphTex<0) return; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=26.0f, gw=gh*asp; DrawImage(g_glyphTex,{hx,hcy-gh*0.5f},{hx+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, WithAlpha(C_WHITE,t)); hx+=gw+6; };
        auto word=[&](const char* w,float pad){ DrawText({hx,hcy-11},20.0f,WithAlpha(C_FOOTER,t),w); hx+=MeasureText(20.0f,w).x+pad; };
        if (g_popup) { hx = 540; glyph(GLYPH_A); word("Select", 40); glyph(GLYPH_B); word("Back", 10); }
        else         { glyph(GLYPH_X); word("Pass Time", 40); glyph(GLYPH_A); word("Select", 10); }
        ResetFont();
    }

    // ---- popup sub-state (drawn over everything except its own footer) ----
    if (g_popup) {
        // dim sits over the scene; the footer was drawn pre-dim in the real game?
        // measured: footer stays FULL bright -> draw popup dim first, then redraw footer
        DrawPopup();
        SetFont(g_fRodin);
        float hx = 540, hcy = 690;
        auto glyph = [&](const UV& g){ if (g_glyphTex<0) return; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=26.0f, gw=gh*asp; DrawImage(g_glyphTex,{hx,hcy-gh*0.5f},{hx+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, C_WHITE); hx+=gw+6; };
        auto word=[&](const char* w,float pad){ DrawText({hx,hcy-11},20.0f,C_FOOTER,w); hx+=MeasureText(20.0f,w).x+pad; };
        glyph(GLYPH_A); word("Select", 40); glyph(GLYPH_B); word("Back", 10);
        ResetFont();
    }
}

} // namespace

void WorldMapInit() { Init(); }
void WorldMapDraw(double openSeconds) { Draw(openSeconds); }
void WorldMapInput(const ScreenInput& in) { Input(in); }
void WorldMapReset() { Reset(); }
