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

int g_fRodin = 0, g_fSeurat = 0, g_fDF = 0, g_glyphTex = -1;
struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A  = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B  = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };
const UV GLYPH_LB = { 0.32617f, 0.00781f, 0.46094f, 0.07812f };

const char* const ITEMS[] = { "Resume", "Status", "Inventory", "Skills", "Go to the Lab", "Options", "Quit Game" };
constexpr int N_ITEMS = 7;
int  g_sel = 0;        // default selection on open = "Resume" (verified from PAUSE video t=13s)
bool g_confirm = false;    // "Enter the Lab?" Yes/No dialog (opens from Go to the Lab)
int  g_confirmSel = 1;     // 0 = Yes, 1 = No (the capture shows No selected by default)
// sub-screens (measured spec: game_captures/PAUSE_SUBSCREEN_SPEC.md)
enum SubView { SV_NONE = 0, SV_ACHIEVEMENTS, SV_INVENTORY };
int g_sub = SV_NONE;
int g_achSel = 0;          // selected achievement row (of the 4 visible)
int g_invSel = 2;          // selected inventory row (capture shows row 3)
bool g_invPopup = false;   // "Give to Sonic / Give to Chip" (measured popup)
int  g_invPopupSel = 0;
const char* g_nav = nullptr;

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
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF (sub-screens)
    if (g_fDF    == 0) g_fDF    = LoadFont("assets/fonts/dfsoge7.ttc");
}
void Reset() { g_sel = 0; g_confirm = false; g_confirmSel = 1; g_sub = SV_NONE; g_achSel = 0; g_invSel = 2; g_invPopup = false; g_invPopupSel = 0; g_nav = nullptr; }
void Input(const ScreenInput& in) {
    if (g_confirm) {
        if (in.up || in.down) g_confirmSel ^= 1;
        if (in.accept && g_confirmSel == 0) { g_confirm = false; g_nav = "loading>mediaroom"; }   // Yes -> the lab
        if (in.cancel || (in.accept && g_confirmSel == 1)) { g_confirm = false; g_confirmSel = 1; }
        return;
    }
    if (g_sub == SV_ACHIEVEMENTS) {
        if (in.up)   g_achSel = std::max(0, g_achSel - 1);
        if (in.down) g_achSel = std::min(3, g_achSel + 1);
        if (in.cancel) g_sub = SV_NONE;
        return;
    }
    if (g_sub == SV_INVENTORY) {
        if (g_invPopup) {   // "Give to Sonic / Give to Chip"
            if (in.up || in.down) g_invPopupSel ^= 1;
            if (in.accept || in.cancel) g_invPopup = false;
            return;
        }
        if (in.up)   g_invSel = std::max(0, g_invSel - 1);
        if (in.down) g_invSel = std::min(6, g_invSel + 1);
        if (in.accept) { g_invPopup = true; g_invPopupSel = 0; }
        if (in.cancel) g_sub = SV_NONE;
        return;
    }
    if (in.up)   g_sel = (g_sel + N_ITEMS - 1) % N_ITEMS;
    if (in.down) g_sel = (g_sel + 1) % N_ITEMS;
    if (in.accept) {
        switch (g_sel) {                       // the runtime pause flow
            case 0: g_nav = "@back"; break;                          // Resume
            case 1: g_nav = "status"; break;                         // Status
            case 2: g_sub = SV_INVENTORY; break;                     // Inventory (sub-screen)
            case 4: g_confirm = true; g_confirmSel = 1; break;       // Go to the Lab
            case 5: g_nav = "options"; break;                        // Options
            case 6: g_nav = "world_map"; break;                      // Quit Game
            default: break;                                          // Skills (not built)
        }
    }
    if (in.cancel) g_nav = "@back";                                  // B resumes, like retail
    if (in.tabLeft) g_sub = SV_ACHIEVEMENTS;                          // (Back) Achievements
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

// solid (untextured) quad from 4 explicit corners (TL,TR,BR,BL) — used for the
// slanted title banner and the chamfer corner triangles (degenerate -> triangle).
void SolidQuad(V2 a, V2 b, V2 c, V2 d, uint32_t col) {
    const V2 corners[4] = { a, b, c, d };
    const V2 uv[4] = { {0,0},{1,0},{1,1},{0,1} };
    DrawImageQuad(-1, corners, uv, col, false);
}

// full-width translucent letterbox bands framing every pause state (measured:
// top band y0..78 + bottom band y642..720, each with a light olive edge line)
void DrawPauseBands(float t) {
    DrawRect({ 0, 0 }, { REF_W, 76 }, WithAlpha(RGBA(10, 12, 6, 150), t));
    DrawRect({ 0, 76 }, { REF_W, 78.5f }, WithAlpha(RGBA(168, 172, 120, 255), t));
    DrawRect({ 0, 642 }, { REF_W, 644.5f }, WithAlpha(RGBA(168, 172, 120, 255), t));
    DrawRect({ 0, 644.5f }, { REF_W, REF_H }, WithAlpha(RGBA(10, 12, 6, 150), t));
}

// the gold WORLD-MAP banner + ghost wordmark + italic chrome PAUSE (measured)
void DrawBanner(float t) {
    {
        const V2 bc[4] = { { 0, 28 }, { 528, 28 }, { 472, 88 }, { 0, 88 } };
        const uint32_t bcol[4] = { WithAlpha(C_BAN_T, t), WithAlpha(C_BAN_T, t),
                                   WithAlpha(C_BAN_B, t), WithAlpha(C_BAN_B, t) };
        DrawQuadGradient(bc, bcol);
        DrawRect({ 0, 28 }, { 528, 29.5f }, WithAlpha(RGBA(110, 100, 14, 255), t));
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
        const V2 pp = { 212.5f, 28.0f }; const float ps = 68.0f;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                if (dx || dy)
                    DrawText({ pp.x + dx * 3.0f, pp.y + dy * 3.0f }, ps, WithAlpha(C_CHR_OUT, t), "PAUSE");
        DrawTextGradient(pp, ps, WithAlpha(C_CHR_T, t), WithAlpha(C_CHR_B, t), "PAUSE");
    }
    ResetTextStretchX();
    ResetTextShear();
}

// "Enter the Lab?" confirm — two stacked chamfered plates (measured spec in
// game_captures/WM_PAUSE_SUBSTATE_SPEC.md): grey back prompt plate (TL chamfer)
// + silver front dialog (TL+BR chamfers) with engraved Yes and a gold-pill No.
void DrawConfirm(float a) {
    const uint32_t SCENE_DK = RGBA(3, 5, 2, 255);   // chamfer erase = dimmed scene
    // back prompt plate (497.3,302.7) 284x111.3, TL chamfer 22
    DrawVGradient({ 497.3f, 302.7f }, { 781.3f, 414.0f },
                  WithAlpha(RGBA(116, 120, 124, 255), a), WithAlpha(RGBA(96, 100, 104, 255), a));
    DrawRect({ 497.3f, 302.7f }, { 781.3f, 304.0f }, WithAlpha(RGBA(140, 142, 144, 255), a));
    DrawRect({ 497.3f, 412.7f }, { 781.3f, 414.0f }, WithAlpha(RGBA(112, 114, 116, 255), a));
    SolidQuad({ 497.3f, 302.7f }, { 519.3f, 302.7f }, { 497.3f, 324.7f }, { 497.3f, 302.7f }, SCENE_DK);
    // prompt: engraved dark grey (light bevel under dark core)
    SetFont(g_fRodin);
    {
        const char* P = "Enter the Lab?";
        float w = MeasureText(26.0f, P).x;
        DrawText({ 639 - w * 0.5f + 1, 351.3f + 1 }, 26.0f, WithAlpha(RGBA(106, 105, 107, 255), a), P);
        DrawText({ 639 - w * 0.5f, 351.3f }, 26.0f, WithAlpha(RGBA(42, 44, 44, 255), a), P);
    }
    // front dialog (541.3,280.7) 196.7x157.3, chamfers TL+BR 22, silver border
    const float dx0 = 541.3f, dy0 = 280.7f, dx1 = 738.0f, dy1 = 438.0f, ch = 22.0f;
    DrawVGradient({ dx0, dy0 }, { dx1, dy1 },
                  WithAlpha(RGBA(164, 165, 166, 255), a), WithAlpha(RGBA(140, 141, 142, 255), a));
    SolidQuad({ dx0, dy0 }, { dx0 + ch, dy0 }, { dx0, dy0 + ch }, { dx0, dy0 }, SCENE_DK);
    SolidQuad({ dx1 - ch, dy1 }, { dx1, dy1 }, { dx1, dy1 - ch }, { dx1 - ch, dy1 }, SCENE_DK);
    uint32_t bd = WithAlpha(RGBA(224, 226, 226, 255), a);
    DrawRect({ dx0 + ch, dy0 }, { dx1, dy0 + 1.5f }, bd);
    DrawRect({ dx0, dy1 - 1.5f }, { dx1 - ch, dy1 }, bd);
    DrawRect({ dx0, dy0 + ch }, { dx0 + 1.5f, dy1 }, bd);
    DrawRect({ dx1 - 1.5f, dy0 }, { dx1, dy1 - ch }, bd);
    SolidQuad({ dx0 + ch, dy0 }, { dx0 + ch + 2, dy0 + 2 }, { dx0 + 2, dy0 + ch + 2 }, { dx0, dy0 + ch }, bd);
    SolidQuad({ dx1 - ch, dy1 }, { dx1 - ch - 2, dy1 - 2 }, { dx1 - 2, dy1 - ch - 2 }, { dx1, dy1 - ch }, bd);
    // options: unselected = white fill + dark outline; SELECTED = orange fill +
    // dark outline on a bright rounded-end yellow bar (verified vs the capture)
    auto option = [&](const char* s, float cy, bool selTxt) {
        float w = MeasureText(24.0f, s).x;
        float lx = 640.3f - w * 0.5f;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                if (dx || dy)
                    DrawText({ lx + dx * 1.4f, cy + dy * 1.4f }, 24.0f, WithAlpha(RGBA(20, 16, 10, 255), a), s);
        DrawText({ lx, cy }, 24.0f,
                 WithAlpha(selTxt ? RGBA(238, 120, 18, 255) : RGBA(238, 238, 238, 255), a), s);
    };
    {   // bright yellow selection bar with a lighter top edge + rounded-end hint
        const float by0 = (g_confirmSel == 0) ? 320.0f : 369.0f, by1 = by0 + 43.0f;
        DrawVGradient({ 562, by0 + 3 }, { 718.7f, by1 - 3 },
                      WithAlpha(RGBA(244, 210, 70, 255), a), WithAlpha(RGBA(222, 172, 34, 255), a));
        DrawVGradient({ 566, by0 }, { 714.7f, by0 + 3 },
                      WithAlpha(RGBA(252, 236, 150, 255), a), WithAlpha(RGBA(244, 210, 70, 255), a));
        DrawRect({ 566, by1 - 3 }, { 714.7f, by1 }, WithAlpha(RGBA(206, 152, 26, 255), a));
        DrawRect({ 558, by0 + 8 }, { 562, by1 - 8 }, WithAlpha(RGBA(238, 196, 52, 255), a));
        DrawRect({ 718.7f, by0 + 8 }, { 722.7f, by1 - 8 }, WithAlpha(RGBA(238, 196, 52, 255), a));
    }
    option("Yes", 326.0f, g_confirmSel == 0);
    option("No", 375.3f, g_confirmSel == 1);
    ResetFont();
}

// ---- pause sub-screens (Achievements / Inventory) — measured spec in
// ---- game_captures/PAUSE_SUBSCREEN_SPEC.md ----------------------------------
const uint32_t SUB_SCENE_DK = RGBA(3, 5, 2, 255);

// the shared plate grammar: TL+BR 45-deg chamfers, silver double border,
// grey vertical gradient (152,153,158)->(110,108,108)
void SubPanel(float x0, float y0, float x1, float y1, float ch, float a) {
    DrawVGradient({ x0, y0 }, { x1, y1 },
                  WithAlpha(RGBA(152, 153, 158, 255), a), WithAlpha(RGBA(110, 108, 108, 255), a));
    SolidQuad({ x0, y0 }, { x0 + ch, y0 }, { x0, y0 + ch }, { x0, y0 }, SUB_SCENE_DK);
    SolidQuad({ x1 - ch, y1 }, { x1, y1 }, { x1, y1 - ch }, { x1 - ch, y1 }, SUB_SCENE_DK);
    uint32_t bd = WithAlpha(RGBA(218, 222, 222, 255), a);
    DrawRect({ x0 + ch, y0 }, { x1, y0 + 2.7f }, bd);
    DrawRect({ x0, y1 - 2.7f }, { x1 - ch, y1 }, bd);
    DrawRect({ x0, y0 + ch }, { x0 + 2.7f, y1 }, bd);
    DrawRect({ x1 - 2.7f, y0 }, { x1, y1 - ch }, bd);
    SolidQuad({ x0 + ch, y0 }, { x0 + ch + 2.7f, y0 + 2.7f }, { x0 + 2.7f, y0 + ch + 2.7f }, { x0, y0 + ch }, bd);
    SolidQuad({ x1 - ch, y1 }, { x1 - ch - 2.7f, y1 - 2.7f }, { x1 - 2.7f, y1 - ch - 2.7f }, { x1, y1 - ch }, bd);
}

// section label plate: TL chamfer + italic-slant right end, white italic caps
void LabelPlate(float x0, float y0, float x1, float y1, const char* text, float a) {
    const float ch = 21.0f, slant = (y1 - y0) * 0.49f * 0.6f;
    const V2 c[4] = { { x0 + ch, y0 }, { x1, y0 }, { x1 - slant, y1 }, { x0, y1 } };
    const uint32_t fT = WithAlpha(RGBA(110, 111, 111, 255), a), fB = WithAlpha(RGBA(72, 72, 72, 255), a);
    const uint32_t col[4] = { fT, fT, fB, fB };
    DrawQuadGradient(c, col);
    SolidQuad({ x0, y0 }, { x0 + ch, y0 }, { x0, y0 + ch }, { x0, y0 }, SUB_SCENE_DK);
    DrawRect({ x0 + ch, y0 }, { x1, y0 + 2 }, WithAlpha(RGBA(208, 210, 210, 255), a));
    DrawRect({ x0 + ch, y0 + 2 }, { x1, y0 + 5 }, WithAlpha(RGBA(165, 165, 165, 255), a));
    SetFont(g_fRodin);
    SetTextShear(0.18f);
    float w = MeasureText(24.0f, text).x;
    DrawText({ x0 + 27 + 1.5f, y0 + 11 + 1.5f }, 24.0f, WithAlpha(RGBA(15, 15, 15, 255), a), text);
    DrawText({ x0 + 27, y0 + 11 }, 24.0f, WithAlpha(RGBA(236, 236, 236, 255), a), text);
    (void)w;
    ResetTextShear();
    ResetFont();
}

void SubScrollbar(float x0, float y0, float x1, float y1, float handleY, float handleH, float a) {
    DrawRect({ x0 - 1.3f, y0 - 1.3f }, { x1 + 1.3f, y1 + 1.3f }, WithAlpha(RGBA(196, 198, 198, 255), a));
    DrawVGradient({ x0, y0 }, { x1, y1 }, WithAlpha(RGBA(121, 123, 123, 255), a), WithAlpha(RGBA(107, 107, 107, 255), a));
    float hx0 = x0 + ((x1 - x0) - 8.7f) * 0.5f;
    DrawRect({ hx0, handleY }, { hx0 + 8.7f, handleY + handleH }, WithAlpha(RGBA(237, 237, 237, 255), a));
}

void SubFooter(bool withSelect, float a) {
    SetFont(g_fRodin);
    float hcy = 637;
    auto cg = [&](const UV& g, float x){ if (g_glyphTex<0) return x; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=31.3f, gw=gh*asp; DrawImage(g_glyphTex,{x,hcy-gh*0.5f},{x+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, WithAlpha(C_WHITE, a)); return x+gw+10; };
    if (withSelect) { float hx = cg(GLYPH_A, 683.3f); DrawText({ hx, hcy - 13 }, 24.0f, WithAlpha(C_FOOTER, a), "Select"); }
    float hx = cg(GLYPH_B, 861.3f); DrawText({ hx, hcy - 13 }, 24.0f, WithAlpha(C_FOOTER, a), "Back");
    ResetFont();
}

void DrawAchievements(float a) {
    LabelPlate(255.3f, 137.3f, 540.0f, 184.7f, "ACHIEVEMENTS", a);
    // counter "50 / 50" + trophy slot (gold art drops in)
    SetFont(g_fRodin);
    DrawTextShadow({ 888, 152 }, 22.0f, WithAlpha(C_WHITE, a), "50 / 50");
    DrawVGradient({ 989.3f, 140 }, { 1016.7f, 182 }, WithAlpha(RGBA(195, 155, 65, 255), a), WithAlpha(RGBA(138, 127, 49, 255), a));
    // main panel + 4 rows (pitch 94.7 from y209.3)
    SubPanel(255.3f, 191.3f, 1023.3f, 598.7f, 24.0f, a);
    struct Ach { const char* name; const char* date; const char* desc; };
    const Ach ROWS[4] = {
        { "Blue Meteor",   "2025/03/03 02:20", "Reached the Goal of Windmill Isle, Act 2 as" },
        { "Hyperdrive",    "2025/03/03 01:57", "Have mastered the Lightspeed Dash techniq" },
        { "Partly Cloudy", "2025/03/03 01:52", "Collected half of the Sun Medals" },
        { "Half Moon",     "2025/03/03 01:49", "Collected half of the Moon Medals" },
    };
    for (int i = 0; i < 4; ++i) {
        const float ry = 209.3f + i * 94.7f;
        const bool sel = (i == g_achSel);
        if (i) DrawRect({ 268.7f, ry - 0.7f }, { 986.7f, ry + 0.7f }, WithAlpha(RGBA(175, 176, 178, 255), a));
        if (sel) {   // opaque yellow plate filling the cell, small TL/BR chamfers
            DrawVGradient({ 268.7f, ry }, { 986.7f, ry + 94.7f },
                          WithAlpha(RGBA(196, 194, 90, 255), a), WithAlpha(RGBA(184, 170, 88, 255), a));
            SolidQuad({ 268.7f, ry }, { 278.7f, ry }, { 268.7f, ry + 10 }, { 268.7f, ry }, WithAlpha(RGBA(140, 141, 144, 255), a));
            SolidQuad({ 977.7f, ry + 94.7f }, { 986.7f, ry + 94.7f }, { 986.7f, ry + 85.7f }, { 977.7f, ry + 94.7f }, WithAlpha(RGBA(122, 120, 118, 255), a));
        }
        // icon slot (achievement art drops in)
        DrawRect({ 294, ry + 19.3f }, { 354, ry + 79.3f }, WithAlpha(RGBA(29, 28, 30, 255), a));
        // name (yellow, outlined)
        SetFont(g_fSeurat);
        DrawText({ 392 + 1.3f, ry + 22 + 1.3f }, 24.0f, WithAlpha(RGBA(15, 15, 15, 255), a), ROWS[i].name);
        DrawText({ 392, ry + 22 }, 24.0f, WithAlpha(RGBA(242, 230, 18, 255), a), ROWS[i].name);
        // date stamp plate
        DrawRect({ 825.3f, ry + 20 }, { 976.7f, ry + 46 }, WithAlpha(RGBA(20, 20, 22, 140), a));
        SetFont(g_fRodin);
        DrawTextAligned({ 825.3f, ry + 20 }, { 976.7f, ry + 46 }, 15.0f, WithAlpha(RGBA(238, 238, 238, 255), a), ROWS[i].date, Align::Center, true, false);
        // description (white; near-black on the selected plate)
        SetFont(g_fSeurat);
        DrawText({ 391.3f, ry + 54.7f }, 20.0f,
                 WithAlpha(sel ? RGBA(30, 27, 20, 255) : RGBA(245, 245, 245, 255), a), ROWS[i].desc);
    }
    ResetFont();
    SubScrollbar(992, 209.3f, 1006, 576, 436 - (3 - g_achSel) * 12.0f, 29.3f, a);
    SubFooter(false, a);
}

void DrawInventory(float a) {
    LabelPlate(228.0f, 140.0f, 427.0f, 188.0f, "INVENTORY", a);
    // list panel + detail panel (abutting, dark seam)
    SubPanel(230.0f, 194.0f, 769.3f, 599.3f, 23.3f, a);
    DrawRect({ 769.3f, 194 }, { 772.0f, 417.3f }, WithAlpha(RGBA(32, 33, 35, 255), a));
    SubPanel(772.0f, 194.0f, 1050.7f, 417.3f, 23.7f, a);
    // item render slot in the detail panel (SEGA art drops in)
    SetFont(g_fSeurat);
    DrawTextAligned({ 780, 203 }, { 1011, 367 }, 15.0f, WithAlpha(RGBA(86, 88, 92, 255), a), "ITEM RENDER", Align::Center, true, false);
    const char* ITEMS_INV[7] = { "Big G Steak", "Popcake", "Nuclear Taquo", "Empire Coffee",
                                 "Banana", "Tropic Juice", "Live Honker" };
    for (int i = 0; i < 7; ++i) {
        const float ty = 232.7f + i * 50.0f;        // row text band top
        const bool sel = (i == g_invSel);
        if (sel) {   // borderless yellow bar, soft top fade, inset in the 50px cell
            DrawVGradient({ 245.3f, ty - 10 }, { 726.7f, ty - 3 },
                          WithAlpha(RGBA(180, 172, 110, 0), a), WithAlpha(RGBA(180, 172, 110, 255), a));
            DrawVGradient({ 245.3f, ty - 3 }, { 726.7f, ty + 36 },
                          WithAlpha(RGBA(180, 172, 110, 255), a), WithAlpha(RGBA(195, 186, 106, 255), a));
            DrawRect({ 245.3f, ty + 36 }, { 726.7f, ty + 38 }, WithAlpha(RGBA(190, 170, 90, 255), a));
        }
        // food icon slot
        DrawRect({ 270, ty + 0.6f }, { 309.3f, ty + 27.3f }, WithAlpha(RGBA(196, 156, 84, 255), a));
        // name (white; saturated orange when selected)
        SetFont(g_fSeurat);
        uint32_t nameCol = sel ? RGBA(234, 126, 3, 255) : RGBA(245, 243, 244, 255);
        DrawText({ 326.7f + 1.3f, ty + 1.3f }, 24.0f, WithAlpha(RGBA(20, 14, 8, 255), a), ITEMS_INV[i]);
        DrawText({ 326.7f, ty }, 24.0f, WithAlpha(nameCol, a), ITEMS_INV[i]);
        // x glyph + count (right-aligned at 705.3)
        SetFont(g_fRodin);
        DrawText({ 616, ty + 2 }, 20.0f, WithAlpha(RGBA(211, 212, 216, 255), a), "x");
        float cw = MeasureText(24.0f, "99").x;
        DrawText({ 705.3f - cw, ty }, 24.0f, WithAlpha(RGBA(225, 226, 228, 255), a), "99");
    }
    ResetFont();
    SubScrollbar(737.3f, 217.3f, 750.7f, 586, 377.3f - (2 - g_invSel) * 14.0f, 31.3f, a);
    SubFooter(true, a);

    // ---- "Give to Sonic / Give to Chip" popup (measured: a small grey dialog
    //      beside the selected row, GOLD highlight band on the chosen option) ----
    if (g_invPopup) {
        const float ty = 232.7f + g_invSel * 50.0f;
        const float px0 = 486, py0 = std::min(ty - 6.0f, 540.0f), px1 = 680, py1 = py0 + 78;
        DrawVGradient({ px0, py0 }, { px1, py1 }, WithAlpha(RGBA(168, 170, 170, 248), a), WithAlpha(RGBA(118, 120, 120, 248), a));
        DrawRect({ px0, py0 }, { px1, py0 + 2 }, WithAlpha(RGBA(222, 224, 224, 255), a));
        DrawRect({ px0, py1 - 2 }, { px1, py1 }, WithAlpha(RGBA(210, 212, 212, 255), a));
        const char* OPT[2] = { "Give to Sonic", "Give to Chip" };
        const float rowY[2] = { py0 + 10, py0 + 44 };
        DrawVGradient({ px0 + 8, rowY[g_invPopupSel] - 4 }, { px1 - 8, rowY[g_invPopupSel] + 26 },
                      WithAlpha(RGBA(238, 204, 92, 250), a), WithAlpha(RGBA(206, 160, 44, 250), a));
        SetFont(g_fSeurat);
        for (int i = 0; i < 2; ++i) {
            bool sel = (i == g_invPopupSel);
            float w = MeasureText(20.0f, OPT[i]).x;
            DrawText({ (px0 + px1) * 0.5f - w * 0.5f + 1, rowY[i] + 1 }, 20.0f,
                     WithAlpha(sel ? RGBA(120, 90, 20, 255) : RGBA(24, 24, 24, 200), a), OPT[i]);
            DrawText({ (px0 + px1) * 0.5f - w * 0.5f, rowY[i] }, 20.0f,
                     WithAlpha(sel ? RGBA(40, 28, 4, 255) : RGBA(238, 238, 238, 255), a), OPT[i]);
        }
        ResetFont();
    }
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

    // ---- Achievements / Inventory sub-screens (scene dims; banner stays lit) ----
    if (g_sub != SV_NONE) {
        DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 128));
        DrawPauseBands(1.0f);
        DrawBanner(1.0f);
        if (g_sub == SV_ACHIEVEMENTS) DrawAchievements(1.0f);
        else                          DrawInventory(1.0f);
        return;
    }

    // ---- "Enter the Lab?" confirm sub-state: the item list is REPLACED by the
    //      dialog stack; banner dims WITH the scene (~44% black); footer stays lit
    if (g_confirm) {
        DrawBanner(1.0f);
        DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 112));
        DrawPauseBands(1.0f);
        DrawConfirm(1.0f);
        SetFont(g_fRodin);
        float hcy = 668;
        auto cg = [&](const UV& g, float x){ if (g_glyphTex<0) return x; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=30.0f, gw=gh*asp; DrawImage(g_glyphTex,{x,hcy-gh*0.5f},{x+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, C_WHITE); return x+gw+8; };
        float hx = 700; hx = cg(GLYPH_A, hx); DrawText({hx,hcy-13},24.0f,C_FOOTER,"Select");
        hx = 895; hx = cg(GLYPH_B, hx); DrawText({hx,hcy-13},24.0f,C_FOOTER,"Back");
        ResetFont();
        return;
    }
    DrawRect({ 0, 0 }, { REF_W, REF_H }, WithAlpha(C_DIM, dimT));
    DrawPauseBands(dimT);

    DrawBanner(t);

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
const char* PauseNav() { return Nav(); }
