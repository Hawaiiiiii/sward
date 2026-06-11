// =============================================================================
// screen_pause.cpp — the in-game PAUSE menu, matched 1:1 to the retail/recomp
// video reference (_ref_pause.png), with all geometry MEASURED from that frame:
//   * dimmed hub behind (World Map globe + starfield, darkened);
//   * gold "PAUSE" banner top-left — a PARALLELOGRAM with a 45 deg slanted right
//     edge (TL(0,28) TR(528,28) BR(472,88) BL(0,88)), white bevel wordmark;
//   * a warm-grey translucent panel centred at (389,131)-(888,586), a HEXAGON
//     with 45 deg chamfers on the top-left + bottom-right corners (26px), a
//     vertical near-white->dark-grey gradient;
//   * the World-Map context menu: Resume / Status / Inventory / Skills /
//     Go to the Lab / Options / Quit Game (first center y=203, pitch 55px); the
//     SELECTED row gets a gold bar (inset 23px) + dark-brown text;
//   * footer: (LB) Achievements / (A) Select / (B) Back.
// Real fonts: FOT-NewRodinPro-DB MSDF (menu items + footer), DFSoGei (PAUSE bevel).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <algorithm>

using namespace ui;
namespace {

int g_fRodin = 0, g_fDF = 0, g_glyphTex = -1;
struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A  = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B  = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };
const UV GLYPH_LB = { 0.32617f, 0.00781f, 0.46094f, 0.07812f };

const char* const ITEMS[] = { "Resume", "Status", "Inventory", "Skills", "Go to the Lab", "Options", "Quit Game" };
constexpr int N_ITEMS = 7;
int g_sel = 0;   // default selection on open = "Resume" (verified from PAUSE video t=13s)

// palette — colours sampled from _ref_pause.png
const uint32_t C_DIM      = RGBA(0, 0, 0, 150);          // hub dim overlay
const uint32_t C_BG_T     = RGBA(9, 14, 5, 255);         // dimmed World-Map hub reads GREEN, not blue
const uint32_t C_BG_B     = RGBA(3, 6, 2, 255);
const uint32_t C_PANEL_T  = RGBA(180, 181, 170, 232);    // warm light-grey panel top (ref ~166 at y140)
const uint32_t C_PANEL_B  = RGBA(135, 131, 114, 232);    // warm-grey panel bottom (ref ~124)
const uint32_t C_PANEL_BD = RGBA(248, 248, 242, 255);    // bright border — ref is bright on ALL 4 sides
const uint32_t C_PANEL_BDD= RGBA(244, 245, 238, 255);    // bottom/right border (measured, also bright)
const uint32_t C_ERASE_T  = RGBA(6, 9, 18, 255);         // dimmed-bg fill for the top-left chamfer
const uint32_t C_ERASE_B  = RGBA(3, 5, 10, 255);         // dimmed-bg fill for the bottom-right chamfer
const uint32_t C_ITEM     = RGBA(244, 243, 233, 255);    // warm-white menu text (measured 243,242,232)
const uint32_t C_ITEM_SEL = RGBA(74, 32, 6, 255);        // dark-brown selected text (measured 69,31,4)
const uint32_t C_SEL_T    = RGBA(228, 202, 108, 250);    // gold selection bar top
const uint32_t C_SEL_B    = RGBA(170, 144, 72, 250);     // gold selection bar bottom
const uint32_t C_BAN_T    = RGBA(155, 146, 18, 255);     // banner gradient top (olive gold)
const uint32_t C_BAN_B    = RGBA(205, 151, 27, 255);     // banner gradient bottom (amber)
const uint32_t C_GHOST    = RGBA(199, 168, 22, 255);     // ghosted WORLD MAP wordmark
const uint32_t C_CHR_T    = RGBA(240, 240, 236, 255);    // chrome PAUSE face top
const uint32_t C_CHR_B    = RGBA(180, 176, 166, 255);    // chrome PAUSE face bottom
const uint32_t C_CHR_OUT  = RGBA(12, 9, 4, 255);         // chrome PAUSE outer outline
const uint32_t C_FOOTER   = RGBA(235, 238, 242, 255);
const uint32_t C_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t C_STAR     = RGBA(170, 220, 170, 255);   // green-tinted hub starfield

// panel geometry (1280x720) — measured from _ref_pause.png
constexpr float PX0 = 389, PY0 = 131, PX1 = 888, PY1 = 586;   // centred panel rect (cx~640)
constexpr float ITEM_C0 = 196, ITEM_PITCH = 55;               // first item CENTER y (ref row centers 195/250/...); pitch
constexpr float CHAMFER = 26;                                  // top-left + bottom-right 45 deg cuts

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/pause/mat_comon_x360_001.png");
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db");      // real game MSDF
    if (g_fDF    == 0) g_fDF    = LoadFont("assets/fonts/dfsoge7.ttc");
}
void Reset() { g_sel = 0; }
void Input(const ScreenInput& in) {
    if (in.up)   g_sel = (g_sel + N_ITEMS - 1) % N_ITEMS;
    if (in.down) g_sel = (g_sel + 1) % N_ITEMS;
}

// solid (untextured) quad from 4 explicit corners (TL,TR,BR,BL) — used for the
// slanted title banner and the chamfer corner triangles (degenerate -> triangle).
void SolidQuad(V2 a, V2 b, V2 c, V2 d, uint32_t col) {
    const V2 corners[4] = { a, b, c, d };
    const V2 uv[4] = { {0,0},{1,0},{1,1},{0,1} };
    DrawImageQuad(-1, corners, uv, col, false);
}

void Draw(double openSec) {
    // MEASURED open animation (live capture session8 @78s, 60fps frame terms):
    // the dim snaps in over ~2 frames; the EMPTY chamfered panel scales up
    // ~0.83 -> 1.0 while alpha-fading over ~6 frames (a translucent ghost frame
    // with no text); ALL content (banner + menu + footer) pops in at frames
    // 8..10. Total open ~= 0.15 s.
    const float dimT   = (float)ComputeMotion(openSec, 0.0, 2.0);
    const float panelT = (float)ComputeMotion(openSec, 0.0, 6.0);
    const float t      = (float)ComputeMotion(openSec, 8.0, 2.0);   // content pop
    // stand-in for the live scene behind pause (the real game keeps rendering it)
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_T, C_BG_B);
    uint32_t s = 0x1357acefu;
    for (int i = 0; i < 70; ++i) { s = s*1664525u+1013904223u; float x=(float)((s>>9)%1280); s=s*1664525u+1013904223u; float y=(float)((s>>9)%720); DrawRect({x,y},{x+1,y+1}, WithAlpha(C_STAR, 0.5f)); }
    DrawRect({ 0, 0 }, { REF_W, REF_H }, WithAlpha(C_DIM, dimT));

    // ---- gold banner (parallelogram, 45 deg slanted right edge) ----
    // measured from _ref_pause.png: vertical olive->amber gradient; the WORLD MAP
    // wordmark stays ghosted in darker gold; PAUSE is stamped over it in italic
    // chrome (white->silver gradient face + near-black outline), bbox 219..456 x 39..73.
    {
        const V2 bc[4] = { { 0, 28 }, { 528, 28 }, { 472, 88 }, { 0, 88 } };
        const uint32_t bcol[4] = { WithAlpha(C_BAN_T, t), WithAlpha(C_BAN_T, t),
                                   WithAlpha(C_BAN_B, t), WithAlpha(C_BAN_B, t) };
        DrawQuadGradient(bc, bcol);
        // thin dark line along the banner's top edge (measured in ref)
        DrawRect({ 0, 28 }, { 528, 29.5f }, WithAlpha(RGBA(110, 100, 14, 255), t));
        // detached slanted highlight sliver just beyond the right slant
        const V2 sc2[4] = { { 538, 28 }, { 547, 28 }, { 491, 88 }, { 482, 88 } };
        const uint32_t scol[4] = { WithAlpha(C_BAN_T, t * 0.85f), WithAlpha(C_BAN_T, t * 0.85f),
                                   WithAlpha(C_BAN_B, t * 0.85f), WithAlpha(C_BAN_B, t * 0.85f) };
        DrawQuadGradient(sc2, scol);
    }
    SetFont(g_fDF);
    SetTextShear(0.24f);
    SetTextStretchX(1.37f);   // the game's wordmark face is far wider than DFSoGei
    DrawText({ 128, 38 }, 40.0f, WithAlpha(C_GHOST, t), "WORLD MAP");      // ghost wordmark
    {   // chrome PAUSE: 8-direction outline ring, then the gradient face
        // sized/positioned so the face bbox lands at the measured 219..456 x 39..73
        const V2 pp = { 212.5f, 28.0f }; const float ps = 68.0f;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                if (dx || dy)
                    DrawText({ pp.x + dx * 3.0f, pp.y + dy * 3.0f }, ps, WithAlpha(C_CHR_OUT, t), "PAUSE");
        DrawTextGradient(pp, ps, WithAlpha(C_CHR_T, t), WithAlpha(C_CHR_B, t), "PAUSE");
    }
    ResetTextStretchX();
    ResetTextShear();

    // ---- grey menu panel (hexagon: top-left + bottom-right chamfered) ----
    // ghost-frame open: the panel scales 0.83 -> 1.0 about its centre while its
    // alpha ramps with panelT (it appears as an empty translucent frame first).
    const float ps9 = 0.83f + 0.17f * panelT;
    const float pcx = (PX0 + PX1) * 0.5f, pcy = (PY0 + PY1) * 0.5f;
    const float x0 = pcx - (pcx - PX0) * ps9, x1 = pcx + (PX1 - pcx) * ps9;
    const float y0 = pcy - (pcy - PY0) * ps9, y1 = pcy + (PY1 - pcy) * ps9;
    const float ch = CHAMFER * ps9;
    DrawVGradient({ x0, y0 }, { x1, y1 }, WithAlpha(C_PANEL_T, panelT), WithAlpha(C_PANEL_B, panelT));
    uint32_t bd = WithAlpha(C_PANEL_BD, panelT), bdd = WithAlpha(C_PANEL_BDD, panelT);
    DrawRect({ x0, y0 }, { x1, y0 + 2 }, bd);      // top border
    DrawRect({ x0, y1 - 2 }, { x1, y1 }, bdd);     // bottom border
    DrawRect({ x0, y0 }, { x0 + 2, y1 }, bd);      // left border
    DrawRect({ x1 - 2, y0 }, { x1, y1 }, bdd);     // right border
    // chamfers: erase the TL + BR corner triangles back to (dimmed) background
    SolidQuad({ x0, y0 }, { x0 + ch, y0 }, { x0, y0 + ch }, { x0, y0 }, WithAlpha(C_ERASE_T, panelT));
    SolidQuad({ x1 - ch, y1 }, { x1, y1 }, { x1, y1 - ch }, { x1 - ch, y1 }, WithAlpha(C_ERASE_B, panelT));
    // bright diagonal edge along each chamfer (2px)
    SolidQuad({ x0 + ch, y0 }, { x0 + ch + 2, y0 + 2 }, { x0 + 2, y0 + ch + 2 }, { x0, y0 + ch }, bd);
    SolidQuad({ x1 - ch, y1 }, { x1 - ch - 2, y1 - 2 }, { x1 - 2, y1 - ch - 2 }, { x1, y1 - ch }, bdd);

    // ---- menu items ----
    SetFont(g_fRodin);
    for (int i = 0; i < N_ITEMS; ++i) {
        float cyc = ITEM_C0 + i * ITEM_PITCH;   // item CENTER y
        bool sel = (i == g_sel);
        if (sel) {
            // ref bar is ~46px tall and centered on the text (nearly fills the 55px pitch)
            DrawVGradient({ PX0 + 23, cyc - 23 }, { PX1 - 23, cyc + 23 },
                          WithAlpha(C_SEL_T, t), WithAlpha(C_SEL_B, t));
        }
        DrawTextAligned({ PX0, cyc - 27 }, { PX1, cyc + 27 }, 28.0f,
                        WithAlpha(sel ? C_ITEM_SEL : C_ITEM, t), ITEMS[i], Align::Center, true, true);
    }

    // ---- footer (Achievements / Select / Back) — ref groups at x~305 / ~700 / ~895 ----
    {
        float hcy = 668;
        auto glyph = [&](const UV& g, float x){ if (g_glyphTex<0) return x; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=30.0f, gw=gh*asp; DrawImage(g_glyphTex,{x,hcy-gh*0.5f},{x+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, WithAlpha(C_WHITE,t)); return x+gw+8; };
        // Xbox 360 BACK-button glyph: light oval (the atlas' tintable white circle,
        // stretched) + a dark left-pointing arrow — the ref's Achievements glyph.
        auto backGlyph = [&](float x){
            const float gw = 38.0f, gh = 26.0f;
            if (g_glyphTex >= 0)
                DrawImage(g_glyphTex, { x, hcy - gh*0.5f }, { x + gw, hcy + gh*0.5f },
                          { 0.0020f, 0.8105f }, { 0.1270f, 0.9355f }, WithAlpha(RGBA(225,228,222,255), t));
            else
                DrawRect({ x, hcy - gh*0.5f }, { x + gw, hcy + gh*0.5f }, WithAlpha(RGBA(225,228,222,255), t));
            const float ax = x + gw*0.5f;
            SolidQuad({ ax - 7, hcy }, { ax + 4, hcy - 7 }, { ax + 4, hcy + 7 }, { ax - 7, hcy },
                      WithAlpha(RGBA(40, 44, 40, 255), t));
            return x + gw + 8;
        };
        float hx = 305; hx = backGlyph(hx); DrawText({hx,hcy-13},24.0f,WithAlpha(C_FOOTER,t),"Achievements");
        hx = 700; hx = glyph(GLYPH_A, hx); DrawText({hx,hcy-13},24.0f,WithAlpha(C_FOOTER,t),"Select");
        hx = 895; hx = glyph(GLYPH_B, hx); DrawText({hx,hcy-13},24.0f,WithAlpha(C_FOOTER,t),"Back");
    }
    ResetFont();
}

} // namespace

void PauseInit() { Init(); }
void PauseDraw(double openSeconds) { Draw(openSeconds); }
void PauseInput(const ScreenInput& in) { Input(in); }
void PauseReset() { Reset(); }
