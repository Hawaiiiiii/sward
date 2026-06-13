// =============================================================================
// screen_status.cpp — the STATUS screen, re-authored 1:1 from LIVE capture
// (s09 67-134s; spec stills status_page1/2/3; stack/header crops measured):
//   * a form-colored gradient RAIL from the left edge (day = blue, night =
//     purple) ending in a swoosh tail, with the italic chrome STATUS wordmark
//     at the shared x263 convention;
//   * the LIVE 3D scene stays behind (character render right, Chip floating
//     by the stack — slots here);
//   * the stat stack (left): a magenta EXP row (plate + gem slot + gold-fill
//     bar + chrome xN count), then per-form rows (Sonic: SPEED / RING ENERGY;
//     Werehog: COMBAT / STRENGTH / LIFE / UNLEASH / SHIELD) — each a dark
//     chamfered plate (white top rim, outlined italic label, magenta chrome
//     MAX) with a slanted gold-fill bar tail; pitch 64; the SELECTED row
//     shifts left and gains a white rim;
//   * the form wordmark (SONIC THE HEDGEHOG / WEREHOG) top-right (art slot);
//   * footer: [Up/Down] Select  (A) Level Up  (Q/E) Switch Form  (B) Back.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
int g_glyphTex = -1;

struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };

// ---- palette (sampled from the capture) --------------------------------------
const uint32_t C_RAIL_D_T = RGBA(104, 142, 206, 235);   // day rail (blue)
const uint32_t C_RAIL_D_B = RGBA(44, 78, 148, 235);
const uint32_t C_RAIL_N_T = RGBA(152, 132, 186, 235);   // night rail (purple)
const uint32_t C_RAIL_N_B = RGBA(86, 56, 126, 235);
const uint32_t C_RAIL_EDGE = RGBA(228, 160, 220, 255);  // thin pink edge line
const uint32_t C_CHR_T   = RGBA(244, 246, 250, 255);    // chrome wordmark/labels
const uint32_t C_CHR_B   = RGBA(168, 176, 190, 255);
const uint32_t C_CHR_OUT = RGBA(22, 24, 36, 255);
const uint32_t C_PLATE_T = RGBA(74, 58, 110, 225);      // stat plate (dark violet)
const uint32_t C_PLATE_B = RGBA(40, 30, 64, 225);
const uint32_t C_PLATE_RIM = RGBA(225, 220, 240, 255);  // white top rim
const uint32_t C_EXP_N_T = RGBA(232, 60, 150, 235);     // EXP plate magenta (night/Werehog)
const uint32_t C_EXP_N_B = RGBA(160, 24, 96, 235);
const uint32_t C_EXP_D_T = RGBA(60, 150, 232, 235);     // EXP plate blue (day/Sonic)
const uint32_t C_EXP_D_B = RGBA(24, 88, 168, 235);
const uint32_t C_EXP_RIM = RGBA(176, 226, 255, 255);    // light-cyan EXP top border (day)
const uint32_t C_TROUGH  = RGBA(26, 20, 40, 220);       // bar trough
const uint32_t C_GOLD_T  = RGBA(252, 214, 74, 255);     // bar gold fill
const uint32_t C_GOLD_B  = RGBA(222, 158, 22, 255);
const uint32_t C_MAX_T   = RGBA(255, 120, 190, 255);    // "MAX" magenta chrome
const uint32_t C_MAX_B   = RGBA(208, 40, 120, 255);
const uint32_t C_WHITE   = RGBA(255, 255, 255, 255);
const uint32_t C_SKY_T = RGBA(86, 140, 210, 255), C_SKY_B = RGBA(170, 205, 235, 255);

// ---- measured layout (ref px) -------------------------------------------------
constexpr float RAIL_Y0 = 50, RAIL_Y1 = 107, RAIL_X1 = 607;
constexpr float WM_X = 263, WM_TOP = 60;
constexpr float EXP_Y = 168;                 // EXP row top
constexpr float ROW_Y0 = 233;                // first stat row top
constexpr float ROW_PITCH = 64, PLATE_H = 42;
constexpr float PLATE_X = 180, PLATE_W = 188;
constexpr float BAR_END = 560, SLANT = 12;

struct Row { const char* label; float fill; };   // fill 0..1 (1 = MAX)
const Row SONIC_ROWS[] = {
    { "SPEED", 1.0f }, { "RING ENERGY", 1.0f },
};
const Row WEREHOG_ROWS[] = {
    { "COMBAT", 1.0f }, { "STRENGTH", 1.0f }, { "LIFE", 1.0f },
    { "UNLEASH", 1.0f }, { "SHIELD", 1.0f },
};

bool g_night = false;
int  g_sel = 0;
int  g_expCount = 99;

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}
void Reset() { g_night = false; g_sel = 0; }
void Input(const ScreenInput& in) {
    int n = g_night ? 5 : 2;   // stat rows; the QUIT plate is index n (selectable)
    if (in.up)   g_sel = std::max(0, g_sel - 1);
    if (in.down) g_sel = std::min(n, g_sel + 1);
    if (in.tabLeft || in.tabRight) { g_night = !g_night; g_sel = 0; }
    // (A) Level Up: stats are showcased at MAX, matching the captured save
}

// italic chrome text (shared recipe)
void Chrome(V2 pos, float px, const char* s, float a, uint32_t tT, uint32_t tB, float stretch = 1.3f) {
    SetFont(g_fDF);
    SetTextShear(0.24f);
    SetTextStretchX(stretch);
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ pos.x + dx * 2.0f, pos.y + dy * 2.0f }, px, WithAlpha(C_CHR_OUT, a), s);
    DrawTextGradient(pos, px, WithAlpha(tT, a), WithAlpha(tB, a), s);
    ResetTextStretchX();
    ResetTextShear();
    ResetFont();
}

// a slanted stat plate + bar tail. sel shifts it left with a white rim.
void StatRow(float y, const char* label, float fill, bool sel, float a, bool exp) {
    const float xoff = sel ? -12.0f : 0.0f;
    const float x0 = PLATE_X + xoff;
    // EXP plate colour is FORM-dependent: blue (day/Sonic) vs magenta (night/Werehog)
    uint32_t pT, pB, rim = C_PLATE_RIM;
    if (exp) {
        pT = g_night ? C_EXP_N_T : C_EXP_D_T;
        pB = g_night ? C_EXP_N_B : C_EXP_D_B;
        rim = g_night ? C_PLATE_RIM : C_EXP_RIM;
    } else { pT = C_PLATE_T; pB = C_PLATE_B; }
    // label plate (parallelogram)
    const V2 pc[4] = { { x0 + SLANT, y }, { x0 + PLATE_W + SLANT, y }, { x0 + PLATE_W, y + PLATE_H }, { x0, y + PLATE_H } };
    const uint32_t pcol[4] = { WithAlpha(pT, a), WithAlpha(pT, a), WithAlpha(pB, a), WithAlpha(pB, a) };
    DrawQuadGradient(pc, pcol);
    DrawRect({ x0 + SLANT, y }, { x0 + PLATE_W + SLANT, y + 2 }, WithAlpha(rim, a));   // top rim
    if (sel) {   // white selection rim around the plate
        DrawRect({ x0, y + PLATE_H - 2 }, { x0 + PLATE_W, y + PLATE_H }, WithAlpha(C_WHITE, a));
        DrawRect({ x0 + 1, y }, { x0 + 3, y + PLATE_H }, WithAlpha(C_WHITE, a * 0.8f));
    }
    // bar tail: trough + gold fill, slanted italic end
    const float bx0 = x0 + PLATE_W + SLANT + 4, bx1 = BAR_END + xoff;
    const V2 tc[4] = { { bx0, y + 8 }, { bx1 + SLANT, y + 8 }, { bx1, y + PLATE_H - 6 }, { bx0 - 6, y + PLATE_H - 6 } };
    const uint32_t tcol[4] = { WithAlpha(C_TROUGH, a), WithAlpha(C_TROUGH, a), WithAlpha(C_TROUGH, a), WithAlpha(C_TROUGH, a) };
    DrawQuadGradient(tc, tcol);
    if (fill > 0.0f) {
        const float fx1 = bx0 + (bx1 - bx0) * fill;
        const V2 fc[4] = { { bx0, y + 10 }, { fx1 + SLANT * fill, y + 10 }, { fx1, y + PLATE_H - 8 }, { bx0 - 4, y + PLATE_H - 8 } };
        const uint32_t fcol[4] = { WithAlpha(C_GOLD_T, a), WithAlpha(C_GOLD_T, a), WithAlpha(C_GOLD_B, a), WithAlpha(C_GOLD_B, a) };
        DrawQuadGradient(fc, fcol);
    }
    // label (white outlined italic), AUTO-FIT so longer labels (RING ENERGY)
    // never overrun the MAX badge zone
    const float maxZone = (!exp && fill >= 1.0f) ? 46.0f : 12.0f;   // reserve for MAX
    const float availW = PLATE_W - 24.0f - maxZone;
    SetFont(g_fSeurat);
    SetTextShear(0.18f);
    float fsz = 22.0f;
    while (fsz > 14.0f && MeasureText(fsz, label).x > availW) fsz -= 1.0f;
    const float ly = y + 9 + (22.0f - fsz) * 0.5f;
    DrawText({ x0 + 22 + 1.5f, ly + 1.5f }, fsz, WithAlpha(RGBA(16, 12, 24, 255), a), label);
    DrawText({ x0 + 22, ly }, fsz, WithAlpha(C_WHITE, a), label);
    ResetTextShear();
    ResetFont();
    if (!exp && fill >= 1.0f)
        Chrome({ x0 + PLATE_W - 36, y + 6 }, 24.0f, "MAX", a, C_MAX_T, C_MAX_B, 1.1f);
}

void Draw(double openSec) {
    const float a = (float)ComputeMotion(openSec, 0.0, 10.0);
    const float rowT = (float)ComputeMotion(openSec, 4.0, 10.0);

    // live 3D scene slot (sky placeholder + ground band)
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_SKY_T, C_SKY_B);
    DrawRect({ 0, 600 }, { REF_W, REF_H }, RGBA(208, 212, 208, 255));
    SetFont(g_fSeurat);
    DrawTextAligned({ 860, 240 }, { 1200, 560 }, 14.0f, WithAlpha(RGBA(120, 140, 165, 255), a),
                    "( character render: live 3D )", Align::Center, true, false);
    ResetFont();

    // ---- form-colored header rail + swoosh + chrome STATUS ----
    const uint32_t railT = g_night ? C_RAIL_N_T : C_RAIL_D_T;
    const uint32_t railB = g_night ? C_RAIL_N_B : C_RAIL_D_B;
    DrawVGradient({ 0, RAIL_Y0 }, { RAIL_X1 - 60, RAIL_Y1 }, WithAlpha(railT, a), WithAlpha(railB, a));
    {   // swoosh tail: a tapering quad + upward curl hint
        const V2 sw[4] = { { RAIL_X1 - 60, RAIL_Y0 }, { RAIL_X1, RAIL_Y0 + 14 }, { RAIL_X1 - 18, RAIL_Y1 - 8 }, { RAIL_X1 - 60, RAIL_Y1 } };
        const uint32_t sc[4] = { WithAlpha(railT, a), WithAlpha(C_RAIL_EDGE, a * 0.8f), WithAlpha(railB, a), WithAlpha(railB, a) };
        DrawQuadGradient(sw, sc);
        const V2 curl[4] = { { RAIL_X1 - 8, RAIL_Y0 + 4 }, { RAIL_X1 + 16, RAIL_Y0 - 6 }, { RAIL_X1 + 10, RAIL_Y0 + 8 }, { RAIL_X1 - 12, RAIL_Y0 + 16 } };
        const uint32_t cc[4] = { WithAlpha(C_RAIL_EDGE, a), WithAlpha(C_RAIL_EDGE, a * 0.4f), WithAlpha(C_RAIL_EDGE, a * 0.4f), WithAlpha(C_RAIL_EDGE, a) };
        DrawQuadGradient(curl, cc);
    }
    DrawRect({ 0, RAIL_Y0 }, { RAIL_X1 - 60, RAIL_Y0 + 1.5f }, WithAlpha(C_RAIL_EDGE, a * 0.7f));
    Chrome({ WM_X, WM_TOP }, 46.0f, "STATUS", a, C_CHR_T, C_CHR_B, 1.5f);

    // ---- form wordmark slot (top-right; SEGA art drops in) ----
    DrawRect({ 950, 36 }, { 1240, 120 }, WithAlpha(RGBA(30, 24, 48, 120), a));
    SetFont(g_fSeurat);
    DrawTextAligned({ 950, 36 }, { 1240, 120 }, 14.0f, WithAlpha(RGBA(190, 180, 210, 255), a),
                    g_night ? "SONIC THE WEREHOG (art slot)" : "SONIC THE HEDGEHOG (art slot)",
                    Align::Center, true, false);
    ResetFont();

    // ---- EXP row: magenta plate + gem slot + bar + chrome count ----
    if (rowT > 0.0f) {
        StatRow(EXP_Y, "EXP.", 0.82f, false, rowT, true);
        // gem slot riding the plate's right end
        DrawRect({ PLATE_X + PLATE_W - 8, EXP_Y - 10 }, { PLATE_X + PLATE_W + 18, EXP_Y + 16 }, WithAlpha(RGBA(255, 226, 120, 230), rowT));
        char buf[8]; snprintf(buf, sizeof buf, "x%d", g_expCount);
        Chrome({ BAR_END + 16, EXP_Y + 4 }, 30.0f, buf, rowT, C_CHR_T, C_CHR_B, 1.2f);
    }

    // ---- per-form stat rows (staggered entrance) ----
    const Row* rows = g_night ? WEREHOG_ROWS : SONIC_ROWS;
    const int n = g_night ? 5 : 2;
    for (int i = 0; i < n; ++i) {
        const float rt = (float)ComputeMotion(openSec, 6.0 + i * 3.0, 8.0);
        if (rt > 0.0f) StatRow(ROW_Y0 + i * ROW_PITCH, rows[i].label, rows[i].fill, i == g_sel, rt, false);
    }

    // ---- QUIT plate below the stat list (a small chamfered button + curl tail) ----
    {
        const float qt = (float)ComputeMotion(openSec, 6.0 + n * 3.0, 8.0);
        if (qt > 0.0f) {
            const float qy = ROW_Y0 + n * ROW_PITCH + 6, qx = PLATE_X, qw = 132, qh = 38;
            const bool qsel = (g_sel == n);
            const float qx0 = qsel ? qx - 12 : qx;
            uint32_t qT = g_night ? RGBA(96, 64, 140, 235) : RGBA(56, 104, 168, 235);
            uint32_t qB = g_night ? RGBA(58, 36, 96, 235)  : RGBA(28, 60, 116, 235);
            const V2 qc[4] = { { qx0 + SLANT, qy }, { qx0 + qw + SLANT, qy }, { qx0 + qw, qy + qh }, { qx0, qy + qh } };
            const uint32_t qcol[4] = { WithAlpha(qT, qt), WithAlpha(qT, qt), WithAlpha(qB, qt), WithAlpha(qB, qt) };
            DrawQuadGradient(qc, qcol);
            DrawRect({ qx0 + SLANT, qy }, { qx0 + qw + SLANT, qy + 2 }, WithAlpha(C_PLATE_RIM, qt));
            if (qsel) DrawRect({ qx0, qy + qh - 2 }, { qx0 + qw, qy + qh }, WithAlpha(C_WHITE, qt));
            // little curl tail at the right
            DrawRect({ qx0 + qw + SLANT, qy + qh * 0.4f }, { qx0 + qw + SLANT + 14, qy + qh * 0.4f + 3 }, WithAlpha(qT, qt));
            SetFont(g_fSeurat);
            SetTextShear(0.18f);
            DrawText({ qx0 + 26 + 1.5f, qy + 8 + 1.5f }, 22.0f, WithAlpha(RGBA(14, 14, 22, 255), qt), "QUIT");
            DrawText({ qx0 + 26, qy + 8 }, 22.0f, WithAlpha(C_WHITE, qt), "QUIT");
            ResetTextShear();
            ResetFont();
        }
    }

    // ---- footer ----
    {
        SetFont(g_fRodin);
        float hcy = 688;
        auto glyph = [&](const UV& g, float x){ if (g_glyphTex<0) return x; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=28.0f, gw=gh*asp; DrawImage(g_glyphTex,{x,hcy-gh*0.5f},{x+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, WithAlpha(C_WHITE,a)); return x+gw+8; };
        float hx = 150;
        DrawText({ hx, hcy - 12 }, 20.0f, WithAlpha(C_WHITE, a), "[Up/Down] Select"); hx += 220;
        hx = glyph(GLYPH_A, hx); DrawText({ hx, hcy - 12 }, 20.0f, WithAlpha(C_WHITE, a), "Level Up"); hx += 130;
        DrawText({ hx, hcy - 12 }, 20.0f, WithAlpha(C_WHITE, a), "(Q/E) Switch Form"); hx += 230;
        hx = glyph(GLYPH_B, hx); DrawText({ hx, hcy - 12 }, 20.0f, WithAlpha(C_WHITE, a), "Back");
        ResetFont();
    }
}

} // namespace

void StatusInit() { Init(); }
void StatusDraw(double openSeconds) { Draw(openSeconds); }
void StatusInput(const ScreenInput& in) { Input(in); }
void StatusReset() { Reset(); }
