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
#include "globe3d.h"
#include "screen.h"
#include "csd_player.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0, g_glyphTex = -1, g_photoTex = -1, g_sunTex = -1, g_moonTex = -1;
struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };
const UV GLYPH_X = { 0.16016f, 0.00781f, 0.23047f, 0.07422f };

// ---- palette ------------------------------------------------------------------
const uint32_t C_TITLE   = RGBA(232, 180, 32, 255);
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
const uint32_t C_POP_FILL = RGBA(40, 120, 36, 255);
const uint32_t C_POP_HI_T = RGBA(118, 148, 36, 255), C_POP_HI_B = RGBA(88, 205, 45, 255);
const uint32_t C_POP_TXT  = RGBA(236, 236, 237, 255);

// ---- layout ---------------------------------------------------------------------
constexpr float GLOBE_CX = 639, GLOBE_CY = 360, GLOBE_R = 180;
constexpr float TOT_X0 = 8,  TOT_Y0 = 114, TOT_X1 = 296, TOT_Y1 = 290;
constexpr float TROW0  = 135, TPITCH = 43;
// stage-info panel (measured from session8)
constexpr float SIP_X0 = 845.3f, SIP_Y0 = 117.3f, SIP_X1 = 1152.0f, SIP_Y1 = 602.7f;
constexpr float GRID = 9.0f;

bool g_popup = false;     // "Go to the village / Select stage"
int  g_popupSel = 0;
// g_showInfo: TRUE = populated stage-info panel (primary ref wm_stageinfo_spagonia,
// the default no-input render); FALSE = hover/empty state after a cursor move
// (left/right) — only the empty green bracket-frame + dotted left rail, no fill.
bool g_showInfo = true;
const char* g_nav = nullptr;

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    // the selected stage's real screenshot (Spagonia here); the slot is a
    // template — swap the PNG (or key it off the selected stage) to re-skin.
    if (g_photoTex < 0) g_photoTex = gfx::loadTexture("assets/gameart/stage_spagonia_hero.png");
    if (g_sunTex   < 0) g_sunTex   = gfx::loadTexture("assets/gameart/medallion_sun.png");   // real day medal
    if (g_moonTex  < 0) g_moonTex  = gfx::loadTexture("assets/gameart/medallion_moon.png");  // real night medal
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() { g_popup = false; g_popupSel = 0; g_showInfo = true; g_nav = nullptr; }
void Input(const ScreenInput& in) {
    if (g_popup) {
        if (in.up || in.down) g_popupSel ^= 1;
        if (in.accept) {   // the runtime flow: village hub or stage select
            g_nav = (g_popupSel == 0) ? "loading>town" : "gate";
            g_popup = false; g_popupSel = 0;
        }
        if (in.cancel) { g_popup = false; g_popupSel = 0; }
        return;
    }
    // cursor move to a new stage -> hover/empty state (no committed stage info)
    if (in.left || in.right) g_showInfo = false;
    // accept opens the go-to popup = commit -> repopulate the stage-info panel
    if (in.accept) { g_popup = true; g_popupSel = 0; g_showInfo = true; }
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

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

// Clean rect-built capital "P" (the dfsogei MSDF atlas entry for 'P' is corrupt —
// its UV points at the texture's top edge, so the font renders a solid block with a
// black notch instead of a glyph). We compose the letter from primitive rects in the
// title gold so the "WORLD MAP" wordmark reads correctly without touching the shared
// font atlas. h = cap height; the proportions track the other dfsogei caps.
void DrawGlyphP(float x, float topY, float h, uint32_t col) {
    const float w    = h * 0.62f;        // glyph box width (matches dfsogei cap aspect)
    const float stem = h * 0.16f;        // vertical stem / bar thickness
    const float bowlH = h * 0.55f;       // the bowl occupies the top ~55%
    // left vertical stem (full height)
    DrawRect({ x, topY }, { x + stem, topY + h }, col);
    // bowl: top bar, right bar, and the mid bar closing it
    DrawRect({ x, topY },                 { x + w, topY + stem },          col); // top
    DrawRect({ x + w - stem, topY },      { x + w, topY + bowlH },         col); // right
    DrawRect({ x, topY + bowlH - stem },  { x + w, topY + bowlH },         col); // bottom of bowl
}

// the gold beveled "WORLD MAP" wordmark. Drawn through the dfsogei MSDF font, but the
// trailing 'P' is substituted with DrawGlyphP() because that atlas glyph is corrupt.
constexpr float TITLE_STRETCH = 1.34f;
void DrawWorldMapTitle(float x, float topY, float px, uint32_t col) {
    SetFont(g_fDF);
    SetTextStretchX(TITLE_STRETCH);
    // everything up to the broken glyph goes through the bevel font path
    DrawTextBevel({ x, topY }, px, col, "WORLD MA");
    // MeasureText returns the UNSTRETCHED advance; DrawText stretches glyph x about the
    // origin x, so the real pen position after "WORLD MA" is x + width*stretch.
    float endX = x + MeasureText(px, "WORLD MA").x * TITLE_STRETCH;
    ResetTextStretchX();
    ResetFont();
    // clean 'P' sized to the cap height of the rest of the wordmark
    const float capH = px * 0.78f;                   // measured dfsogei cap ~0.78*px
    const float capTop = topY + (px - capH) * 0.46f; // align baseline with the font caps
    DrawGlyphP(endX + px * 0.06f, capTop, capH, col);
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
    // stage photo slot (2:1 plate, flush on the fill): the selected stage's real
    // screenshot. Falls back to the dim plate + "STAGE PHOTO" label if absent.
    if (g_photoTex >= 0) {
        DrawImage(g_photoTex, { 863.3f, 139.3f }, { 1133.3f, 274.7f },
                  { 0.f, 0.f }, { 1.f, 1.f }, WithAlpha(C_WHITE, t));
    } else {
        DrawVGradient({ 863.3f, 139.3f }, { 1133.3f, 274.7f },
                      WithAlpha(RGBA(34, 52, 70, 255), t), WithAlpha(RGBA(16, 26, 38, 255), t));
        SetFont(g_fSeurat);
        DrawTextAligned({ 863.3f, 139.3f }, { 1133.3f, 274.7f }, 14.0f,
                        WithAlpha(RGBA(110, 130, 150, 255), t), "STAGE PHOTO", Align::Center, true, false);
    }
    // flag overlay (top-left of photo) + sun medallion (top-right)
    DrawRect({ 868, 143.3f }, { 908, 170.7f }, WithAlpha(RGBA(24, 32, 80, 255), t));
    DrawRect({ 870, 145.3f }, { 906, 168.7f }, WithAlpha(RGBA(235, 238, 240, 255), t));
    DrawRect({ 880, 150 }, { 896, 164 }, WithAlpha(RGBA(170, 50, 40, 255), t));
    if (g_sunTex >= 0) DrawImage(g_sunTex, { 1099.0f, 142.0f }, { 1130.0f, 175.0f }, { 0.f, 0.f }, { 1.f, 1.f }, WithAlpha(C_WHITE, t));
    else               DrawIconSlot(1114.7f, 158, 14.7f, RGBA(213, 143, 33, 255), true, t);
    // description (6 lines, pitch 32.9, left margin 864)
    SetFont(g_fSeurat);
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

// hover/empty state (g_showInfo==false): only the green corner-bracket frame +
// dotted left rail at the SIP rect, fully transparent interior so the starfield
// shows through. No fill / photo / desc / counters / stage label.
void DrawStageInfoEmpty(float t) {
    const uint32_t fc = WithAlpha(C_SIP_RAIL, t);
    const float BL = 2.7f;                 // bracket thickness
    const float BX = 38.0f, BY = 34.0f;    // corner-bracket arm length
    // top-left
    DrawRect({ SIP_X0, SIP_Y0 }, { SIP_X0 + BX, SIP_Y0 + BL }, fc);
    DrawRect({ SIP_X0, SIP_Y0 }, { SIP_X0 + BL, SIP_Y0 + BY }, fc);
    // top-right
    DrawRect({ SIP_X1 - BX, SIP_Y0 }, { SIP_X1, SIP_Y0 + BL }, fc);
    DrawRect({ SIP_X1 - BL, SIP_Y0 }, { SIP_X1, SIP_Y0 + BY }, fc);
    // bottom-left
    DrawRect({ SIP_X0, SIP_Y1 - BL }, { SIP_X0 + BX, SIP_Y1 }, fc);
    DrawRect({ SIP_X0, SIP_Y1 - BY }, { SIP_X0 + BL, SIP_Y1 }, fc);
    // bottom-right
    DrawRect({ SIP_X1 - BX, SIP_Y1 - BL }, { SIP_X1, SIP_Y1 }, fc);
    DrawRect({ SIP_X1 - BL, SIP_Y1 - BY }, { SIP_X1, SIP_Y1 }, fc);
    // dotted left rail (vertical dashes down the inner-left edge)
    for (float y = SIP_Y0 + BY; y < SIP_Y1 - BY; y += 8.0f)
        DrawRect({ SIP_X0 + 1.3f, y }, { SIP_X0 + 2.6f, y + 4.0f }, WithAlpha(C_DASH, t));
}

// the floating stage-name label + gradient leader rule (measured: SPAGONIA)
void DrawStageLabel(float t) {
    SetFont(g_fDF);
    SetTextShear(0.22f);
    SetTextStretchX(1.40f);   // measured real ~165px (1.65 overshot to ~194); 1.65*165/194=1.40
    const char* NAME = "SPAGONIA";
    // dark outline ring, then the lime->gold gradient face (left origin x355.3 kept;
    // font 34 -> 30 trims cap height ~23 -> ~19px)
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ 355.3f + dx * 2.0f, 340.0f + dy * 2.0f }, 30.0f, WithAlpha(RGBA(10, 22, 4, 255), t), NAME);
    DrawTextGradient({ 355.3f, 340.0f }, 30.0f, WithAlpha(C_LBL_T, t), WithAlpha(C_LBL_B, t), NAME);
    ResetTextStretchX();
    ResetTextShear();
    ResetFont();
    // leader rule: gradient bar + a DESCENDING elbow down to the Spagonia marker
    // (globe at GLOBE_CX/CY 639,360 r180; the bar ends ~x571 and the connector
    //  drops down-right to terminate near the marker at ~628,399)
    {
        const V2 c[4] = { { 285.3f, 378 }, { 571.0f, 378 }, { 571.0f, 380.7f }, { 285.3f, 380.7f } };
        const uint32_t col[4] = { WithAlpha(C_LDR_L, t), WithAlpha(C_LDR_R, t),
                                  WithAlpha(C_LDR_R, t), WithAlpha(C_LDR_L, t) };
        DrawQuadGradient(c, col);
        // descending elbow: drops from the bar end (~571,380) down to the marker (~628,399)
        const V2 e[4] = { { 569.0f, 378 }, { 626, 397 }, { 630, 401 }, { 573, 382 } };
        const uint32_t ec[4] = { WithAlpha(C_LDR_R, t), WithAlpha(C_LDR_R, t),
                                 WithAlpha(C_LDR_R, t), WithAlpha(C_LDR_R, t) };
        DrawQuadGradient(e, ec);
    }
}

// the "Go to the village / Select stage" popup (measured)
void DrawPopup() {
    // ~33% scene dim (black at 66%); the popup + footer stay full-bright
    DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 124));
    // body: faint vertical sub-gradient (no harsh scanlines) so the row-to-row
    // green delta drops from ~43-69 toward real's ~19-20 while keeping texture
    DrawVGradient({ 472, 276 }, { 808, 428 },
                  RGBA(48, 130, 44, 255), RGBA(34, 108, 30, 255));
    const char* OPT[2] = { "Go to the village", "Select stage" };
    // highlight box on the selected row (smooth gradient — real game has no scanlines here)
    {
        float hy = (g_popupSel == 0) ? 290.7f : 361.4f;
        DrawVGradient({ 484.7f, hy }, { 795.4f, hy + 50.7f }, C_POP_HI_T, C_POP_HI_B);
    }
    SetFont(g_fRodin);
    for (int i = 0; i < 2; ++i) {
        float w = MeasureText(26.0f, OPT[i]).x;
        DrawText({ 640 - w * 0.5f, 303.3f + i * 70.7f }, 26.0f, C_POP_TXT, OPT[i]);
    }
    ResetFont();
}

// the go-to-village/select-stage POPUP overlay (dynamic sub-state). Drawn LAST in
// both the CSD and hand-authored paths so its ~33% scene dim sits over EVERYTHING
// (incl. the legend band) and only its own footer stays full-bright — matching the
// original ordering exactly. Inert (no draws) unless g_popup is set.
void DrawPopupOverlay() {
    if (!g_popup) return;
    // dim sits over the scene; the footer was drawn pre-dim in the real game?
    // measured: footer stays FULL bright -> draw popup dim first, then redraw footer
    DrawPopup();
    SetFont(g_fRodin);
    // A(Select) disc starts ~x683 -> centers ~x698, widening A->B gap to ~178
    float hx = 683, hcy = 640;
    auto glyph = [&](const UV& g){ if (g_glyphTex<0) return; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=30.0f, gw=gh*asp; DrawImage(g_glyphTex,{hx,hcy-gh*0.5f},{hx+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, C_WHITE); hx+=gw+6; };
    auto word=[&](const char* w,float pad){ DrawText({hx,hcy-12},23.0f,C_WHITE,w); hx+=MeasureText(23.0f,w).x+pad; };
    glyph(GLYPH_A); word("Select", 40); glyph(GLYPH_B); word("Back", 10);
    ResetFont();
}

// DYNAMIC scene content the CSD base does NOT provide — the 3D rotating Earth, the
// live totals column (lives/rings/sun&moon counts), and the hover-vs-committed
// stage-info panel + floating stage label/leader rule. Runs OVER either base (CSD
// or the hand-authored chrome). No full-screen fills / header / footer band here
// (those are CSD chrome). The popup is drawn separately (DrawPopupOverlay) so its
// full-screen dim can land LAST over the legend band in the no-CSD path.
//
// csdBase: when TRUE the real world_map CSD is the base and it ALREADY draws the
// COMPLETE bright-green LED stage-info panel (x645..1010) with its HIGH SCORE /
// BEST TIME / RANK labels + medal-counter slots. We therefore SKIP the overlay's
// own stage-info panel (DrawStageInfo / DrawStageInfoEmpty — the LED fill, lit-cell
// loop, accent rails, the labels, the photo plate, the description blurb, and the
// three medal counters) to avoid a second, narrower, right-shifted panel doubling
// over the CSD's. The 3D globe, left totals column, and floating SPAGONIA label are
// kept either way (the CSD base lacks them). When FALSE (the hand-authored
// fallback) every draw runs exactly as before.
void DrawDynamic(double openSec, bool csdBase) {
    const float t      = (float)ComputeMotion(openSec, 0.0,  12.0);  // counters / globe
    const float tPanel = (float)ComputeMotion(openSec, 18.0, 16.0);  // stage-info LED panel flood
    const float tTitle = (float)ComputeMotion(openSec, 38.0, 18.0);  // SPAGONIA label (last)

    // the 3D Earth hub: real tessellated sphere, slow spin (matches the live
    // capture's idle rotation), with the continent stage-markers riding it
    {
        static const GlobeMarker MK[] = {
            { 23.0f, 38.0f, 0xFF2EE04C, 7.0f },   // "apotos"
            { 12.0f, 45.0f, 0xFF2EE04C, 7.0f },   // "spagonia"
            { -75.0f, 42.0f, 0xFF2EE04C, 7.0f },  // "empire city"
            { 31.0f, 30.0f, 0xFF2EE04C, 7.0f },   // "mazuri"
            { 103.0f, 1.5f, 0xFF2EE04C, 7.0f },   // "chun-nan"
            { -42.0f, 72.0f, 0xFFE0B22E, 8.0f },  // "holoska" (selected gold)
            { 138.0f, 36.0f, 0xFF2EE04C, 7.0f },  // "eggmanland"
        };
        DrawGlobe3D(GLOBE_CX, GLOBE_CY, GLOBE_R * (0.6f + 0.4f * t),
                    (float)(Now() * 6.0),   // ~6 deg/s idle spin
                    0.55f, 0.45f, 0.7f, MK, 7, t);
    }

    // ---- left totals column: icon + bright LIVE number rows (lives/rings/sun&moon
    //      counts). The green LED panel backing them is CSD chrome (drawn in Draw()
    //      only); the counts themselves are dynamic and composite on either base. ----
    {
        // each row: an icon/medal, a short LABEL, then the live value. The labels keep
        // the rows readable (and disambiguate the two "lv 7 (200)" medal rows, which are
        // otherwise identical) — SUN = day medals, MOON = night medals.
        struct Row { uint32_t icol; bool ring; const char* label; const char* val; int medTex; } rows[] = {
            { C_LIVES, false, "LIVES", "99",     -1 },
            { C_RING,  true,  "RINGS", "999999", -1 },
            { C_SUN,   true,  "SUN",   "lv 7 (200)", g_sunTex },
            { C_MOON,  true,  "MOON",  "lv 7 (200)", g_moonTex },
        };
        SetFont(g_fRodin);
        for (int i = 0; i < 4; ++i) {
            float cy = TROW0 + i * TPITCH;
            if (rows[i].medTex >= 0)   // real Sun/Moon medal emblem
                DrawImage(rows[i].medTex, { 124, cy - 16 }, { 156, cy + 16 }, { 0.f, 0.f }, { 1.f, 1.f }, WithAlpha(C_WHITE, t));
            else
                DrawIconSlot(140, cy, 14, rows[i].icol, rows[i].ring, t);
            // label (dimmer green) then the bright value to its right
            DrawText({ 178, cy - 13 }, 17.0f, WithAlpha(C_SIP_DESC, t), rows[i].label);
            DrawText({ 178 + MeasureText(17.0f, rows[i].label).x + 8, cy - 13 }, 24.0f, WithAlpha(C_NUM, t), rows[i].val);
        }
        ResetFont();
    }

    // (the CSD base draws its own gold "WORLD MAP" title; an earlier screen-local 'P'
    //  repair patch mis-measured the CSD title position and garbled it, so it was removed
    //  — the CSD title is left as-is.)

    // ---- floating stage label/leader rule: ALWAYS drawn (the CSD base lacks the
    //      floating SPAGONIA name + its gradient leader rule toward the marker) ----
    if (g_showInfo)
        DrawStageLabel(tTitle);

    // ---- stage-info PANEL: only on the hand-authored base. With the real world_map
    //      CSD loaded, the CSD ALREADY draws the COMPLETE bright-green LED stage-info
    //      panel (x645..1010) + HIGH SCORE/BEST TIME/RANK labels + medal-counter
    //      slots; redrawing the overlay's narrower, right-shifted panel here would
    //      double it (hard seam + duplicated counters + off-right-edge float). Skip
    //      both the populated panel AND the empty bracket-frame so the CSD's own
    //      panel/slots show through cleanly. ----
    if (!csdBase) {
        if (g_showInfo) {
            DrawStageInfo(tPanel);
        } else {
            // hover/empty state: only the green bracket-frame + dotted left rail
            DrawStageInfoEmpty(t);
        }
    }
}

// the full hand-authored World Map render (CSD-absent fallback): draws the
// background/header/LED panels/title/legend-band/footer CHROME, then composites
// the DYNAMIC overlay on top so the no-CSD path is byte-for-byte the original.
void Draw(double openSec) {
    // staggered entrance (CSD ~66f): bg/header/globe/counters land first, the
    // stage-info panel floods in next, and the gold WORLD MAP title + the floating
    // SPAGONIA stage label settle LAST (only clearly readable by ~1.5s in the CSD).
    const float t      = (float)ComputeMotion(openSec, 0.0,  12.0);  // bg / header / counters / globe / footer
    const float tTitle = (float)ComputeMotion(openSec, 38.0, 18.0);  // WORLD MAP title (last)
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);
    // full-width header rail (mirrors the footer legend band)
    DrawRect({ 0, 104 }, { REF_W, 107 }, WithAlpha(RGBA(94, 123, 88, 255), t));
    // head-badge [75,58,50,45] + map-logo [1075,54,127,53] art slots: the real
    // banner shows the head icon / map logo art here, NOT labelled outline boxes
    // (oracle vs the real capture) — leave the slots empty until that art is wired.
    uint32_t s = 0x2468ace1u;
    for (int i = 0; i < 110; ++i) { s = s*1664525u+1013904223u; float x=(float)((s>>9)%1280); s=s*1664525u+1013904223u; float y=(float)((s>>9)%720); s=s*1664525u+1013904223u; int b=50+(int)((s>>9)%160); DrawRect({x,y},{x+1,y+1}, WithAlpha(RGBA(b,b,b,255), t*0.7f)); }

    // ---- top-left green LED-circuit panel backing the title + counters
    //      (subtle lit-cell grid, no bright chevron border / no separators) ----
    {
        const float gx0 = 0, gy0 = 104, gx1 = 290, gy1 = 290;
        DrawRect({ gx0, gy0 }, { gx1, gy1 }, WithAlpha(RGBA(4, 18, 5, 230), t));
        for (float yy = gy0 + 2; yy < gy1 - 2; yy += 8.0f)
            for (float xx = gx0 + 2; xx < gx1 - 2; xx += 16.0f) {
                DrawRect({ xx, yy + 1.5f }, { xx + 14, yy + 3.0f }, WithAlpha(RGBA(16, 55, 20, 255), t));
                DrawRect({ xx, yy + 4.5f }, { xx + 14, yy + 6.0f }, WithAlpha(RGBA(16, 55, 20, 200), t));
            }
        DrawRect({ gx0, gy1 - 2 }, { gx1, gy1 }, WithAlpha(RGBA(20, 81, 18, 255), t));   // bottom rail
    }

    // ---- gold beveled "WORLD MAP" title (inset ~124px, bigger, wider tracking).
    //      The trailing 'P' is rect-built (the dfsogei atlas 'P' glyph is corrupt). ----
    DrawWorldMapTitle(124, 62, 42.0f, WithAlpha(C_TITLE, tTitle));

    // ---- 3D globe + totals + stage info/label (the dynamic overlay) ----
    //      csdBase=false: this is the hand-authored fallback, so the overlay draws
    //      its OWN stage-info panel here (no CSD panel underneath to double it).
    DrawDynamic(openSec, false);

    // ---- bottom legend band (full-width olive gradient + bright top edge) ----
    DrawRect({ 0, 612 }, { REF_W, 614 }, WithAlpha(RGBA(140, 168, 90, 220), t));
    DrawVGradient({ 0, 614 }, { REF_W, REF_H }, WithAlpha(RGBA(36, 48, 18, 200), t), WithAlpha(RGBA(20, 28, 8, 160), t));

    // ---- footer (Pass Time / Select), centred on the band ----
    {
        SetFont(g_fRodin);
        float hx = 598, hcy = 640;
        auto glyph = [&](const UV& g){ if (g_glyphTex<0) return; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=30.0f, gw=gh*asp; DrawImage(g_glyphTex,{hx,hcy-gh*0.5f},{hx+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, WithAlpha(C_WHITE,t)); hx+=gw+6; };
        auto word=[&](const char* w,float pad){ DrawText({hx,hcy-12},23.0f,WithAlpha(C_WHITE,t),w); hx+=MeasureText(23.0f,w).x+pad; };
        if (g_popup) { hx = 683; glyph(GLYPH_A); word("Select", 40); glyph(GLYPH_B); word("Back", 10); }
        // non-popup: X glyph starts ~x555, wide pad after "Pass Time" (40->95) so
        // the A(Select) green disc lands ~x875 (real span ~304px, not compressed)
        else         { hx = 555; glyph(GLYPH_X); word("Pass Time", 128); glyph(GLYPH_A); word("Select", 10); }   // Select disc -> ~x875 (real)
        ResetFont();
    }

    // ---- popup sub-state (drawn over everything except its own footer): its
    //      ~33% dim sits over the legend band, then its footer redraws full-bright ----
    DrawPopupOverlay();
}

} // namespace

void WorldMapInit() { Init(); }
// CSD-base composite: when the real world_map CSD is loaded (the host drew it as
// the base — WORLD MAP header + green grid panels + score labels + button guide),
// we DON'T redraw that chrome; we only composite the DYNAMIC content the CSD lacks
// (3D globe, live totals, hover-vs-committed stage info + label, go-to popup) ON TOP.
// With no CSD, the full hand-authored Draw() runs unchanged (the validated fallback).
void WorldMapDraw(double openSeconds) {
    const bool csdBase = csd::LoadedId() && std::strcmp(csd::LoadedId(), "world_map") == 0;
    if (!csdBase) {
        Draw(openSeconds);   // full hand-authored screen (chrome + dynamic + popup)
    } else {
        // CSD already drew the chrome/background AND the complete bright-green LED
        // stage-info panel (labels + medal-counter slots); layer only the dynamic
        // content the CSD lacks (3D globe, live totals, floating SPAGONIA label) +
        // popup on top. csdBase=true gates the overlay's own stage-info panel so it
        // does NOT double over the CSD's. NO full-screen bg fill (would occlude CSD).
        DrawDynamic(openSeconds, true);
        DrawPopupOverlay();
    }
}
void WorldMapInput(const ScreenInput& in) { Input(in); }
void WorldMapReset() { Reset(); }
const char* WorldMapNav() { return Nav(); }
