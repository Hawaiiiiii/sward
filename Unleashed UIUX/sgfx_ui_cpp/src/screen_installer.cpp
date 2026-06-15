// =============================================================================
// screen_installer.cpp â€” the UnleashedRecomp INSTALLER (Language-Select page),
// ported PIXEL- and MOTION-exact from ui/installer_wizard.cpp (the literal code
// that drew it). 1280x720 WIDE so Scale(n)==n, GRID_SIZE=9. Every rect/colour/font
// and the full STAGGERED entrance timeline are transcribed from that source.
//
// Layout: black bg; left install image (161.5,103.5 512x512); two green SCANLINE
// bars (top y0..105, bottom y615..720) capped by mint divider lines; gold beveled
// "INSTALLER" title (DFSoGei 48 @122/288,54.5); Miles icon (256,80); right
// CHECKERBOARD green container (main 514,227-1040,473 + side ->1280) with a 3x2
// language-pill grid (250x22, cols x522/x780.5, rows y441/410/379, bottom-up:
// FRANÃ‡AIS/DEUTSCH/ENGLISH | ESPAÃ‘OL/ITALIANO/æ—¥æœ¬èªž; ENGLISH default-lit) + a
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
int g_installTex = -1, g_milesTex = -1;   // real recomp install_001.dds + miles_electric_icon.dds
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
int g_sel = 2;        // cursor (ENGLISH default)
int g_langSet = 2;    // the chosen language (lit toggle); set on accept

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");    // real game MSDF
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");   // real DFSoGeiStd-W7 MSDF (crisp title + buttons)
    if (g_installTex < 0) g_installTex = gfx::loadTexture("assets/recomp/inst_install_001.png");
    if (g_milesTex   < 0) g_milesTex   = gfx::loadTexture("assets/recomp/inst_miles_icon.png");
}
void Reset() { g_sel = 2; g_langSet = 2; }
void Input(const ScreenInput& in) {
    if (in.up)    g_sel = (g_sel % 3 == 0) ? g_sel : g_sel - 1;
    if (in.down)  g_sel = (g_sel % 3 == 2) ? g_sel : g_sel + 1;
    if (in.left)  g_sel = std::max(0, g_sel - 3);
    if (in.right) g_sel = std::min(5, g_sel + 3);
    if (in.accept) g_langSet = g_sel;   // commit the highlighted language (lights its toggle)
}

// a green checkerboard container (DrawContainer recipe)
void DrawContainer(float x0, float y0, float x1, float y1, uint32_t grid, float t, bool overlay) {
    SetModifier(MOD_CHECKERBOARD);
    DrawRect({ x0, y0 }, { x1, y1 }, WithAlpha(grid, t));
    ResetModifier();
    if (overlay) DrawRect({ x0, y0 }, { x1, y1 }, WithAlpha(C_GRID_OV, t));
}

// the SHARED button plate, ported EXACT from installer_wizard.cpp DrawButtonContainer
// (L917): three AddRectFilledMultiColor layers under SCANLINE_BUTTON, parameterised by
// (baser,baseg) — 0/0 for a resting button, 48/32 for the focused (hovered) one. This is
// the same 3-layer recipe as the options value cell (screen_options.cpp DrawPlate).
void DrawButtonPlate(float x0, float y0, float x1, float y1, int br, int bg, float a) {
    auto A = [&](int base) { return (uint8_t)std::clamp((int)lround(base * a), 0, 255); };
    SetModifier(MOD_SCANLINE_BUTTON);
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(br,bg+130,0,A(223)), RGBA(br,bg+130,0,A(178)), RGBA(br,bg+130,0,A(223)), RGBA(br,bg+130,0,A(178)));
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(br,bg,0,A(13)),      RGBA(br,bg,0,0),           RGBA(br,bg,0,A(55)),      RGBA(br,bg,0,A(6)));
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(br,bg+130,0,A(13)),  RGBA(br,bg+130,0,A(111)),  RGBA(br,bg+130,0,0),      RGBA(br,bg+130,0,A(55)));
    ResetModifier();
}

// DrawButton (installer_wizard.cpp L943): the plate + centred DFSoGei-20 label with a
// green vertical gradient (br+192,255,0 -> br+128,bg+170,0) and a 4px outline (br,bg,0).
void DrawButton(float x0, float y0, float x1, float y1, const char* text, bool focused, float a) {
    const int br = focused ? 48 : 0, bg = focused ? 32 : 0;
    DrawButtonPlate(x0, y0, x1, y1, br, bg, a);
    SetFont(g_fDF);
    const float sz = 20.0f; float w = MeasureText(sz, text).x; float sx = 1.0f;
    const float boxW = x1 - x0; if (w > boxW && w > 0.0f) sx = boxW / w;
    float dw = w * sx, px = x0 + (boxW - dw) * 0.5f, py = y0 + ((y1 - y0) - sz) * 0.5f - 1.0f;
    if (sx != 1.0f) SetTextStretchX(sx);
    static const float O[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
    for (auto& o : O) DrawText({ px + o[0]*1.6f, py + o[1]*1.6f }, sz, WithAlpha(RGBA(br,bg,0,255), a), text);
    DrawTextGradient({ px, py }, sz, WithAlpha(RGBA(br+192,255,0,255), a), WithAlpha(RGBA(br+128,bg+170,0,255), a), text);
    if (sx != 1.0f) ResetTextStretchX();
}

// toggle light (imgui_utils.cpp DrawToggleLight): 14px lit/dark dot + additive yellow glow
void DrawTLight(float x0, float y0, bool on, float a) {
    const float ls = 14.0f, lcx = x0 + ls*0.5f, lcy = y0 + ls*0.5f;
    if (on) { const float gs = 24.0f; float gx = x0 - gs*0.5f + 2.0f, gy = y0 - gs*0.5f;
              DrawRect({ gx, gy }, { gx+gs, gy+gs }, WithAlpha(C_GLOW, a), true); }
    auto disc = [&](float r, uint32_t c){ float k = r*0.4142f;
        DrawRect({ lcx-r, lcy-k }, { lcx+r, lcy+k }, c); DrawRect({ lcx-k, lcy-r }, { lcx+k, lcy+r }, c);
        DrawRect({ lcx-r*0.78f, lcy-r*0.78f }, { lcx+r*0.78f, lcy+r*0.78f }, c); };
    disc(ls*0.5f, WithAlpha(on ? C_LIGHT_ON : C_LIGHT_OFF, a));
}

// one animated container border (installer_wizard.cpp DrawHorizontal/VerticalBorder):
// a solid mint line that fades toward its far ends and sweeps open from the centre.
void DrawHBorder(float y, float prog) {   // prog: 0..1 entrance
    const uint32_t SOLID = RGBA(155,200,155,255), FADE = RGBA(155,200,155,0), FADE_R = RGBA(155,225,155,0);
    float bs = 1.0f - prog;
    const float CX = 513.0f, CW = 526.5f, SIDE = CW*0.5f, OVER = 36.0f;
    float midX = CX + CW/5.0f;
    float minX = Lerp(CX - 1 - OVER, midX, bs), maxX = Lerp(CX + CW + SIDE + OVER, midX, bs);
    DrawQuadGradient({minX,y},{midX,y+1}, FADE, SOLID, SOLID, FADE);
    DrawQuadGradient({midX,y},{std::min(maxX,REF_W),y+1}, SOLID, FADE_R, FADE_R, SOLID);
}
void DrawVBorder(float x, bool right, float prog) {
    const uint32_t SOLID = right ? RGBA(155,225,155,255) : RGBA(155,155,155,255);
    const uint32_t FADE  = right ? RGBA(155,225,155,0)   : RGBA(155,155,155,0);
    float bs = 1.0f - prog;
    const float CY = 226.0f, CH = 246.0f, OVER = 36.0f;
    float midY = CY + CH/2.0f;
    float minY = Lerp(CY - OVER, midY, bs), maxY = Lerp(CY + CH + OVER, midY, bs);
    DrawQuadGradient({x,minY},{x+1,midY}, FADE, FADE, SOLID, SOLID);
    DrawQuadGradient({x,midY},{x+1,maxY}, SOLID, SOLID, FADE, FADE);
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

    // ---- left install image: the REAL install_001.dds (512x512) ----
    if (mImg > 0) {
        if (g_installTex >= 0) {
            DrawImage(g_installTex, { IMG_X0, IMG_Y0 }, { IMG_X1, IMG_Y1 }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, mImg));
        } else {
            DrawRect({ IMG_X0, IMG_Y0 }, { IMG_X1, IMG_Y1 }, WithAlpha(RGBA(18,22,30,255), mImg));
            DrawRect({ IMG_X0+8, IMG_Y0+8 }, { IMG_X1-8, IMG_Y1-8 }, WithAlpha(RGBA(28,36,52,255), mImg));
            SetFont(g_fSeurat);
            DrawTextAligned({ IMG_X0, (IMG_Y0+IMG_Y1)*0.5f-16 }, { IMG_X1, (IMG_Y0+IMG_Y1)*0.5f+16 }, 22.0f, WithAlpha(RGBA(120,140,170,255), mImg), "INSTALL IMAGE", Align::Center, true, true);
        }
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

    // ---- Miles Electric icon: the REAL miles_electric_icon.dds (64x64), zoom-in ----
    if (mMiles > 0) { float s = 62.0f * (2.0f - mMiles);
        if (g_milesTex >= 0) DrawImage(g_milesTex, { 256 - s*0.5f, 80 - s*0.5f }, { 256 + s*0.5f, 80 + s*0.5f }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, mMiles*0.9f));
        else DrawRect({ 256 - s*0.5f, 80 - s*0.5f }, { 256 + s*0.5f, 80 + s*0.5f }, WithAlpha(C_MILES, mMiles*0.9f)); }

    // ---- title ----
    if (mTitle > 0) { SetFont(g_fDF); DrawTextBevel({ 288, 54.5f }, 48.0f, WithAlpha(C_TITLE, mTitle), "INSTALLER"); }

    // ---- right content container (checkerboard) ----
    if (mOuter > 0) DrawContainer(SIDE_X0, MAIN_Y0, SIDE_X1, MAIN_Y1, C_GRID, mOuter, false);
    if (mInner > 0) DrawContainer(MAIN_X0, MAIN_Y0, MAIN_X1, MAIN_Y1, C_GRID_T, mInner, true);

    // ---- language buttons (3x2) + toggle lights (installer_wizard DrawLanguagePicker) ----
    if (mInner > 0) {
        for (int i = 0; i < 6; ++i) {
            float cx0 = (i < 3) ? COLL_X0 : COLR_X0, cx1 = (i < 3) ? COLL_X1 : COLR_X1;
            int row = i % 3;
            float py0 = 441.0f - (PILL_GAP + PILL_H) * row, py1 = py0 + PILL_H;   // rows 441/410/379
            DrawButton(cx0, py0, cx1, py1, LANGS[i], i == g_sel, mInner);          // cursor = focus-brighten
            DrawTLight(cx0 + 14, py0 + (PILL_H - 14) * 0.5f + 1, i == g_langSet, mInner);  // light = chosen language
        }
        // NEXT navigation button (DFSoGei, right-aligned just below the container)
        SetFont(g_fDF); float ntw = MeasureText(20.0f, "NEXT").x, nx1 = 1035.5f;
        DrawButton(nx1 - ntw - 28.0f, 477.0f, nx1, 499.0f, "NEXT", false, mInner);
    }

    // ---- container borders (drawn on top; mint, fading at the ends, sweep from centre) ----
    if (mBord > 0) {
        DrawHBorder(225.0f, mBord); DrawHBorder(472.0f, mBord);
        DrawVBorder(512.0f, false, mBord); DrawVBorder(1039.5f, true, mBord);
    }

    // ---- footer (shared button-guide; pops at ~frame 61) + version ----
    if (mInner >= 0.999f) {
        static const GuideBtn FOOTER[] = {
            { "Select", GIcon::A, GAlign::Right, 115.0f },
            { "Quit",   GIcon::B, GAlign::Right, 0.0f   },
        };
        DrawButtonGuide(FOOTER, 2, g_fRodin, 1.0f, 379.0f);
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
