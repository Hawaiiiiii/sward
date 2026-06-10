// =============================================================================
// screen_installer.cpp — the UnleashedRecomp INSTALLER (Language-Select page),
// ported PIXEL- and MOTION-exact from ui/installer_wizard.cpp (the literal code
// that drew it). 1280x720 WIDE so Scale(n)==n, GRID_SIZE=9. Every rect/colour/font
// and the full STAGGERED entrance timeline are transcribed from that source.
//
// Layout: black bg; left install image (161.5,103.5 512x512); two green SCANLINE
// bars (top y0..105, bottom y615..720) capped by mint divider lines; gold beveled
// "INSTALLER" title (DFSoGei 48 @122/288,54.5); Miles icon (256,80); right
// CHECKERBOARD green container (main 514,227-1040,473 + side ->1280) with a 3x2
// language-pill grid (250x22, cols x522/x780.5, rows y441/410/379, bottom-up:
// FRANÇAIS/DEUTSCH/ENGLISH | ESPAÑOL/ITALIANO/日本語; ENGLISH default-lit) + a
// right-aligned NEXT pill; footer Select/Quit guide; version bottom-right.
// Entrance (frames@60, sqrt ease-out, ComputeMotion(openSec,offset,total)):
//   scanlines 0/15, miles 10/15, title 15/30, borders 15/23, image 25/15,
//   right-panel 38/23, inner-panel+pills+NEXT 46/15, footer pops at ~61.
// Shaders approximated with MOD_SCANLINE/MOD_CHECKERBOARD/MOD_TITLE_BEVEL.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0, g_glyphTex = -1;
struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };

// exact colours
const uint32_t C_BLACK   = RGBA(0, 0, 0, 255);
const uint32_t C_TITLE   = RGBA(255, 195, 0, 255);
const uint32_t C_SCAN0   = RGBA(203, 255, 0, 0);
const uint32_t C_SCAN1   = RGBA(203, 255, 0, 55);
const uint32_t C_DIV_T   = RGBA(222, 255, 189, 65);
const uint32_t C_DIV_B   = RGBA(173, 255, 156, 65);
const uint32_t C_DIV_C   = RGBA(115, 178, 104, 255);
const uint32_t C_GRID     = RGBA(0, 33, 0, 255);
const uint32_t C_GRID_T   = RGBA(0, 33, 0, 223);
const uint32_t C_GRID_OV  = RGBA(0, 32, 0, 128);
const uint32_t C_BORDER_L = RGBA(155, 200, 155, 255);
const uint32_t C_BORDER_R = RGBA(155, 225, 155, 255);
const uint32_t C_PILL_T   = RGBA(0, 130, 0, 223);
const uint32_t C_PILL_B   = RGBA(0, 130, 0, 150);
const uint32_t C_PILL_SEL_T = RGBA(48, 162, 0, 235);
const uint32_t C_PILL_SEL_B = RGBA(48, 162, 0, 160);
const uint32_t C_PILL_TXT = RGBA(196, 245, 40, 255);
const uint32_t C_LIGHT_ON = RGBA(206, 255, 60, 255);
const uint32_t C_LIGHT_OFF= RGBA(36, 60, 36, 255);
const uint32_t C_GLOW     = RGBA(255, 255, 0, 127);
const uint32_t C_VERSION  = RGBA(255, 255, 255, 70);
const uint32_t C_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t C_MILES    = RGBA(70, 130, 200, 255);

// geometry
constexpr float GRID = 9;
constexpr float IMG_X0 = 161.5f, IMG_Y0 = 103.5f, IMG_X1 = 673.5f, IMG_Y1 = 615.5f;
constexpr float MAIN_X0 = 514, MAIN_Y0 = 227, MAIN_X1 = 1040, MAIN_Y1 = 473;
constexpr float SIDE_X0 = 1040, SIDE_X1 = 1280;
constexpr float PILL_W = 250, PILL_H = 22, PILL_GAP = 9;
constexpr float COLL_X0 = 522, COLL_X1 = 772, COLR_X0 = 780.5f, COLR_X1 = 1030.5f;

const char* const LANGS[6] = { "FRANCAIS", "DEUTSCH", "ENGLISH", "ESPANOL", "ITALIANO", "JAPANESE" };
int g_sel = 2;   // ENGLISH default

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");    // real game MSDF
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}
void Reset() { g_sel = 2; }
void Input(const ScreenInput& in) {
    if (in.up)    g_sel = (g_sel % 3 == 0) ? g_sel : g_sel - 1;
    if (in.down)  g_sel = (g_sel % 3 == 2) ? g_sel : g_sel + 1;
    if (in.left)  g_sel = std::max(0, g_sel - 3);
    if (in.right) g_sel = std::min(5, g_sel + 3);
}

// a green checkerboard container (DrawContainer recipe)
void DrawContainer(float x0, float y0, float x1, float y1, uint32_t grid, float t, bool overlay) {
    SetModifier(MOD_CHECKERBOARD);
    DrawRect({ x0, y0 }, { x1, y1 }, WithAlpha(grid, t));
    ResetModifier();
    if (overlay) DrawRect({ x0, y0 }, { x1, y1 }, WithAlpha(C_GRID_OV, t));
}

// a green language/NEXT pill (scanline-button)
void DrawPill(float x0, float y0, float x1, float y1, bool sel, float t) {
    SetModifier(MOD_SCANLINE);
    DrawVGradient({ x0, y0 }, { x1, y1 }, WithAlpha(sel ? C_PILL_SEL_T : C_PILL_T, t), WithAlpha(sel ? C_PILL_SEL_B : C_PILL_B, t));
    ResetModifier();
}

void Draw(double openSec) {
    // ---- staggered entrance motions (frames @60) ----
    float mScan  = (float)ComputeMotion(openSec, 0.0, 15.0);
    float mMiles = (float)ComputeMotion(openSec, 10.0, 15.0);
    float mTitle = (float)ComputeMotion(openSec, 15.0, 30.0);
    float mBord  = (float)ComputeMotion(openSec, 15.0, 23.0);
    float mImg   = (float)ComputeMotion(openSec, 25.0, 15.0);
    float mOuter = (float)ComputeMotion(openSec, 38.0, 23.0);
    float mInner = (float)ComputeMotion(openSec, 46.0, 15.0);

    DrawRect({ 0, 0 }, { REF_W, REF_H }, C_BLACK);

    // ---- left install image (placeholder) ----
    if (mImg > 0) {
        DrawRect({ IMG_X0, IMG_Y0 }, { IMG_X1, IMG_Y1 }, WithAlpha(RGBA(18,22,30,255), mImg));
        DrawRect({ IMG_X0+8, IMG_Y0+8 }, { IMG_X1-8, IMG_Y1-8 }, WithAlpha(RGBA(28,36,52,255), mImg));
        SetFont(g_fSeurat);
        DrawTextAligned({ IMG_X0, (IMG_Y0+IMG_Y1)*0.5f-16 }, { IMG_X1, (IMG_Y0+IMG_Y1)*0.5f+16 }, 22.0f, WithAlpha(RGBA(120,140,170,255), mImg), "INSTALL IMAGE", Align::Center, true, true);
    }

    // ---- scanline bars (grow) + divider lines ----
    float h = 105.0f * mScan;
    if (h > 0.5f) {
        SetModifier(MOD_SCANLINE);
        DrawVGradient({ 0, 0 }, { REF_W, h }, WithAlpha(C_SCAN0, mScan), WithAlpha(C_SCAN1, mScan));
        DrawVGradient({ 0, REF_H - h }, { REF_W, REF_H }, WithAlpha(C_SCAN1, mScan), WithAlpha(C_SCAN0, mScan));
        ResetModifier();
        auto divline = [&](float y){
            DrawRect({ 0, y-2 }, { REF_W, y }, WithAlpha(C_DIV_T, mScan));
            DrawRect({ 0, y+1 }, { REF_W, y+3 }, WithAlpha(C_DIV_B, mScan));
            DrawRect({ 0, y }, { REF_W, y+1 }, WithAlpha(C_DIV_C, mScan));
        };
        divline(h); divline(REF_H - h);
    }

    // ---- Miles icon (placeholder disc, zoom-in) ----
    if (mMiles > 0) { float s = 62.0f * (2.0f - mMiles); DrawRect({ 256 - s*0.5f, 80 - s*0.5f }, { 256 + s*0.5f, 80 + s*0.5f }, WithAlpha(C_MILES, mMiles*0.9f)); }

    // ---- title ----
    if (mTitle > 0) { SetFont(g_fDF); DrawTextBevel({ 288, 54.5f }, 48.0f, WithAlpha(C_TITLE, mTitle), "INSTALLER"); }

    // ---- right content container (checkerboard) ----
    if (mOuter > 0) DrawContainer(SIDE_X0, MAIN_Y0, SIDE_X1, MAIN_Y1, C_GRID, mOuter, false);
    if (mInner > 0) DrawContainer(MAIN_X0, MAIN_Y0, MAIN_X1, MAIN_Y1, C_GRID_T, mInner, true);

    // ---- container borders (sweep open from centre) ----
    if (mBord > 0) {
        float bs = 1.0f - mBord;   // 1 -> 0
        float midX = 618.3f, midY = 349.0f;
        auto hbar = [&](float y){ float x0 = Lerp(476.0f, midX, bs), x1 = Lerp(1338.75f, midX, bs); DrawRect({ x0, y }, { std::min(x1, REF_W), y + 1 }, WithAlpha(C_BORDER_L, mBord)); };
        auto vbar = [&](float x, uint32_t c){ float y0 = Lerp(190.0f, midY, bs), y1 = Lerp(508.0f, midY, bs); DrawRect({ x, y0 }, { x + 1, y1 }, WithAlpha(c, mBord)); };
        hbar(225); hbar(472); vbar(512, C_BORDER_L); vbar(1039.5f, C_BORDER_R);
    }

    // ---- language pills (3x2) + toggle lights ----
    if (mInner > 0) {
        for (int i = 0; i < 6; ++i) {
            float cx0 = (i < 3) ? COLL_X0 : COLR_X0, cx1 = (i < 3) ? COLL_X1 : COLR_X1;
            int row = i % 3;
            float py0 = (MAIN_Y0 + MAIN_Y1 - MAIN_Y0) ; // placeholder; recompute below
            py0 = (227 + 246 - PILL_GAP - PILL_H) - (PILL_GAP + PILL_H) * row;   // 441 - 31*row
            float py1 = py0 + PILL_H;
            bool sel = (i == g_sel);
            DrawPill(cx0, py0, cx1, py1, sel, mInner);
            // toggle light
            float lx = cx0 + 14, ly = py0 + (PILL_H - 14) * 0.5f + 1;
            if (sel) DrawRect({ lx-5, ly-5 }, { lx+19, ly+19 }, WithAlpha(C_GLOW, mInner));
            DrawRect({ lx, ly }, { lx + 14, ly + 14 }, WithAlpha(sel ? C_LIGHT_ON : C_LIGHT_OFF, mInner));
            SetFont(g_fDF);
            DrawTextAligned({ cx0 + 36, py0 }, { cx1 - 8, py1 }, 20.0f, WithAlpha(C_PILL_TXT, mInner), LANGS[i], Align::Left, true, true);
        }
        // NEXT pill (right-aligned under panel)
        float nx1 = 1035.5f, ny0 = 477, ny1 = 499, nw = 118;
        DrawPill(nx1 - nw, ny0, nx1, ny1, false, mInner);
        SetFont(g_fDF);
        DrawTextAligned({ nx1 - nw + 8, ny0 }, { nx1 - 8, ny1 }, 20.0f, WithAlpha(C_PILL_TXT, mInner), "NEXT", Align::Center, true, true);
    }

    // ---- footer (pops at ~frame 61) + version ----
    if (mInner >= 0.999f) {
        SetFont(g_fRodin);
        float hx = 470, hcy = 690;
        auto glyph = [&](const UV& g){ if (g_glyphTex<0) return; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=26.0f, gw=gh*asp; DrawImage(g_glyphTex,{hx,hcy-gh*0.5f},{hx+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, WithAlpha(C_WHITE,1)); hx+=gw+6; };
        auto word=[&](const char* w,float pad){ DrawText({hx,hcy-11},20.0f,WithAlpha(C_WHITE,1),w); hx+=MeasureText(20.0f,w).x+pad; };
        glyph(GLYPH_A); word("Select", 40); glyph(GLYPH_B); word("Quit", 10);
    }
    SetFont(g_fRodin);
    DrawTextAligned({ REF_W - 360, REF_H - 20 }, { REF_W - 2, REF_H - 4 }, 12.0f, WithAlpha(C_VERSION, mTitle),
                    "v1.0.3.325e4d3-HEAD (RelWithDebInfo)", Align::Right, true, true);
    ResetFont();
}

} // namespace

void InstallerInit() { Init(); }
void InstallerDraw(double openSeconds) { Draw(openSeconds); }
void InstallerInput(const ScreenInput& in) { Input(in); }
void InstallerReset() { Reset(); }
