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
#include "csd_player.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
int g_glyphTex = -1, g_charDayTex = -1, g_charNightTex = -1;

struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A  = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B  = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };
const UV GLYPH_LB = { 0.31250f, 0.00000f, 0.46875f, 0.07812f };   // shoulder LB (manifest btn_lb)
const UV GLYPH_RB = { 0.46875f, 0.00000f, 0.62500f, 0.07812f };   // shoulder RB (manifest btn_rb)

// ---- palette (sampled from the capture) --------------------------------------
const uint32_t C_RAIL_D_T = RGBA(46, 82, 146, 235);     // day rail (real banner core ~42,79,143)
const uint32_t C_RAIL_D_B = RGBA(38, 66, 124, 235);
const uint32_t C_RAIL_N_T = RGBA(132, 112, 168, 235);   // night rail (purple)
const uint32_t C_RAIL_N_B = RGBA(76, 50, 112, 235);
const uint32_t C_RAIL_EDGE = RGBA(146, 168, 210, 255);  // thin cyan/light-blue edge line (measured)
const uint32_t C_CHR_T   = RGBA(244, 246, 250, 255);    // chrome wordmark/labels
const uint32_t C_CHR_B   = RGBA(168, 176, 190, 255);
const uint32_t C_CHR_OUT = RGBA(22, 24, 36, 255);
const uint32_t C_PLATE_T = RGBA(54, 50, 108, 248);      // stat plate purple (night/Werehog, measured)
const uint32_t C_PLATE_B = RGBA(32, 28, 72, 248);
const uint32_t C_PLATE_D_T = RGBA(60, 72, 86, 248);     // stat plate neutral gunmetal (day/Sonic; real B-R ~10-20)
const uint32_t C_PLATE_D_B = RGBA(36, 46, 60, 248);
const uint32_t C_PLATE_RIM = RGBA(225, 220, 240, 255);  // white top rim
const uint32_t C_GOLD_RIM = RGBA(220, 215, 33, 255);    // gold outline rim tracing the plate
const uint32_t C_EXP_N_T = RGBA(152, 42, 184, 235);     // EXP plate violet-magenta (night/Werehog)
const uint32_t C_EXP_N_B = RGBA(102, 68, 116, 235);
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
constexpr float ROW_Y0 = 232;                // night first stat row top (64px pitch confirmed; round-5 over-raised to 219 -> ~13px too high, restored)
constexpr float PLATE_H = 40;
constexpr float PLATE_X = 180, PLATE_W = 188;
constexpr float BAR_END = 575, SLANT = 12;   // gold tail reaches ~x575 (measured)

// filled circle via horizontal strips (sgfxui has no circle primitive)
void FillDisc(float cx, float cy, float r, uint32_t col) {
    const int N = 14;
    for (int i = 0; i < N; ++i) {
        const float y0 = cy - r + (2.0f * r) * i / N;
        const float y1 = cy - r + (2.0f * r) * (i + 1) / N;
        const float ym = (y0 + y1) * 0.5f - cy;
        const float hw = std::sqrt(std::max(0.0f, r * r - ym * ym));
        DrawRect({ cx - hw, y0 }, { cx + hw, y1 }, col);
    }
}

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

// Vertical layout is FORM-dependent. Night packs 5 rows on a uniform 64px pitch
// (measured) anchored at the night EXP/ROW_Y0 constants. Day spreads its 2 stat
// rows with NON-uniform gaps (EXP center 238, SPEED 339, RING 413 -> gaps 101
// then 74); we model day with EXP_Y=218, ROW_Y0=319 and a 74px stat pitch.
inline float RowPitch()  { return g_night ? 64.0f : 74.0f; }   // stat-row pitch
inline float ExpTop()    { return g_night ? EXP_Y  : 218.0f; } // EXP plate top
inline float StatTop0()  { return g_night ? ROW_Y0 : 304.0f; } // first stat-row top (day raised -15 to lift the stack)

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    // the form's real character render (live 3D in-game), background-keyed to a
    // clean cutout so it stands on the status sky. Swap to re-skin.
    if (g_charDayTex   < 0) g_charDayTex   = gfx::loadTexture("assets/gameart/status_sonic_day_cut.png");
    if (g_charNightTex < 0) g_charNightTex = gfx::loadTexture("assets/gameart/status_werehog_night_cut.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
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
    } else {
        // non-EXP plate is form-dependent: gunmetal (day/Sonic) vs purple (night/Werehog)
        pT = g_night ? C_PLATE_T : C_PLATE_D_T;
        pB = g_night ? C_PLATE_B : C_PLATE_D_B;
    }
    // label plate (parallelogram)
    const V2 pc[4] = { { x0 + SLANT, y }, { x0 + PLATE_W + SLANT, y }, { x0 + PLATE_W, y + PLATE_H }, { x0, y + PLATE_H } };
    const uint32_t pcol[4] = { WithAlpha(pT, a), WithAlpha(pT, a), WithAlpha(pB, a), WithAlpha(pB, a) };
    DrawQuadGradient(pc, pcol);
    // gold rim: ~2px stroke tracing all 4 edges of the parallelogram, strongest
    // on the active/selected row (in addition to the white top rim below)
    {
        const float rg = sel ? a : a * 0.55f;
        const uint32_t gr = WithAlpha(C_GOLD_RIM, rg);
        // top edge (along the slanted top)
        DrawRect({ pc[0].x, pc[0].y }, { pc[1].x, pc[0].y + 2 }, gr);
        // bottom edge
        DrawRect({ pc[3].x, pc[2].y - 2 }, { pc[2].x, pc[2].y }, gr);
        // left slanted edge (top-left -> bottom-left)
        const V2 le[4] = { { pc[0].x, pc[0].y }, { pc[0].x + 2, pc[0].y }, { pc[3].x + 2, pc[3].y }, { pc[3].x, pc[3].y } };
        const uint32_t lec[4] = { gr, gr, gr, gr };
        DrawQuadGradient(le, lec);
        // right slanted edge (top-right -> bottom-right)
        const V2 re[4] = { { pc[1].x - 2, pc[1].y }, { pc[1].x, pc[1].y }, { pc[2].x, pc[2].y }, { pc[2].x - 2, pc[2].y } };
        const uint32_t rec[4] = { gr, gr, gr, gr };
        DrawQuadGradient(re, rec);
    }
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

    // live 3D scene slot (sky placeholder + soft ground gradient — no hard slab
    // behind the footer prompts)
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_SKY_T, C_SKY_B);
    DrawVGradient({ 0, 560 }, { REF_W, REF_H }, WithAlpha(RGBA(200, 206, 204, 255), 0.55f), WithAlpha(RGBA(150, 158, 156, 255), 0.55f));
    // the form's character render fills the right-side slot (keyed cutout, stands
    // on the sky); falls back to the "(character render: live 3D)" label if absent.
    {
        int charTex = g_night ? g_charNightTex : g_charDayTex;
        if (charTex >= 0) {
            const float asp = g_night ? 0.469f : 0.393f;   // cut-render W/H
            const float ch = 332.0f, cw = ch * asp, cx = 1035.0f, by = 576.0f;
            DrawImage(charTex, { cx - cw * 0.5f, by - ch }, { cx + cw * 0.5f, by },
                      { 0.f, 0.f }, { 1.f, 1.f }, WithAlpha(RGBA(255, 255, 255, 255), a));
        } else {
            SetFont(g_fSeurat);
            DrawTextAligned({ 860, 240 }, { 1200, 560 }, 14.0f, WithAlpha(RGBA(120, 140, 165, 255), a),
                            "( character render: live 3D )", Align::Center, true, false);
            ResetFont();
        }
    }

    // ---- form-colored header rail (ends near the banner terminus ~x640 with a
    //      short swoosh tail) + cyan top+bottom edge highlights + chrome STATUS ----
    const uint32_t railT = g_night ? C_RAIL_N_T : C_RAIL_D_T;
    const uint32_t railB = g_night ? C_RAIL_N_B : C_RAIL_D_B;
    const float railEnd = 640;   // rail ends near the banner terminus (real navy ends ~x640)
    DrawVGradient({ 0, RAIL_Y0 }, { railEnd - 40, RAIL_Y1 }, WithAlpha(railT, a), WithAlpha(railB, a));
    {   // short tapering swoosh tail terminating the banner (no long right extension)
        const V2 sw[4] = { { railEnd - 40, RAIL_Y0 }, { railEnd, RAIL_Y0 + 12 }, { railEnd - 16, RAIL_Y1 - 8 }, { railEnd - 40, RAIL_Y1 } };
        const uint32_t sc[4] = { WithAlpha(railT, a), WithAlpha(railT, a), WithAlpha(railB, a), WithAlpha(railB, a) };
        DrawQuadGradient(sw, sc);
    }
    DrawRect({ 0, RAIL_Y0 - 1 }, { railEnd - 40, RAIL_Y0 + 1.5f }, WithAlpha(C_RAIL_EDGE, a * 0.7f));   // top edge
    DrawRect({ 0, RAIL_Y1 }, { railEnd - 40, RAIL_Y1 + 2.5f }, WithAlpha(C_RAIL_EDGE, a));              // bottom highlight
    Chrome({ WM_X, WM_TOP - 2 }, 50.0f, "STATUS", a, C_CHR_T, C_CHR_B, 1.7f);   // bigger (measured)

    // ---- form wordmark slot (mid-right): only shown as a labelled placeholder
    //      when the character render is absent — with the render present the
    //      figure itself conveys the form, so the box would just occlude it.
    if ((g_night ? g_charNightTex : g_charDayTex) < 0) {
        DrawRect({ 950, 470 }, { 1240, 554 }, WithAlpha(RGBA(30, 24, 48, 120), a));
        SetFont(g_fSeurat);
        DrawTextAligned({ 950, 470 }, { 1240, 554 }, 14.0f, WithAlpha(RGBA(190, 180, 210, 255), a),
                        g_night ? "SONIC THE WEREHOG (art slot)" : "SONIC THE HEDGEHOG (art slot)",
                        Align::Center, true, false);
        ResetFont();
    }

    // ---- top-right level gauges (Werehog/night form only): a medal disc at the
    //      capsule LEFT, a bold white "LV 7" label, then the yellow pill gauge
    //      bar (measured: medal disc center ~x806, LEFT of the x814 capsule) ----
    if (g_night) {
        auto lvchip = [&](float cy, bool top) {
            // silver capsule track (rounded ends) with the lv label seated left
            DrawVGradient({ 818, cy - 12 }, { 1000, cy + 12 }, WithAlpha(RGBA(206, 210, 214, 235), a), WithAlpha(RGBA(150, 154, 160, 235), a));
            DrawRect({ 814, cy - 6 }, { 822, cy + 6 }, WithAlpha(RGBA(180, 184, 190, 235), a));   // rounded-end hint
            DrawRect({ 996, cy - 6 }, { 1004, cy + 6 }, WithAlpha(RGBA(180, 184, 190, 235), a));
            // medal disc at the capsule LEFT (center ~x806, r~7): warm red/gold
            // radial for the top chip, blue/teal radial for the bottom
            {
                const float mcx = 806.0f;
                const uint32_t ring = top ? WithAlpha(RGBA(168, 34, 28, 235), a)
                                          : WithAlpha(RGBA(28, 78, 150, 235), a);
                const uint32_t core = top ? WithAlpha(RGBA(248, 196, 70, 235), a)
                                          : WithAlpha(RGBA(96, 178, 214, 235), a);
                FillDisc(mcx, cy, 7.5f, ring);
                FillDisc(mcx, cy, 4.5f, core);
            }
            // bold "LV 7" label, white with a dark outline, seated right of the medal
            SetFont(g_fRodin);
            const uint32_t lvOut = WithAlpha(RGBA(28, 30, 36, 255), a);
            const uint32_t lvFill = WithAlpha(C_WHITE, a);
            const V2 lvP{ 826.0f, cy - 8.0f };
            const V2 svP{ 850.0f, cy - 11.0f };
            for (float dx = -1.5f; dx <= 1.5f; dx += 1.5f)
                for (float dy = -1.5f; dy <= 1.5f; dy += 1.5f)
                    if (dx != 0.0f || dy != 0.0f) {
                        DrawText({ lvP.x + dx, lvP.y + dy }, 15.0f, lvOut, "LV");
                        DrawText({ svP.x + dx, svP.y + dy }, 24.0f, lvOut, "7");
                    }
            DrawText(lvP, 15.0f, lvFill, "LV");
            DrawText(svP, 24.0f, lvFill, "7");
            ResetFont();
            // yellow pill gauge fill (rounded), right of the label
            DrawVGradient({ 872, cy - 7 }, { 988, cy + 7 }, WithAlpha(C_GOLD_T, a), WithAlpha(C_GOLD_B, a));
        };
        lvchip(140, true); lvchip(176, false);
    }

    // ---- EXP row: magenta plate + gem slot + bar + chrome count ----
    if (rowT > 0.0f) {
        const float ey = ExpTop();
        StatRow(ey, "EXP.", 0.95f, false, rowT, true);   // gold nearly full (real x325-575 span)
        // EXP gem: rotated yellow-lime diamond (measured x322-347)
        {
            const float gcx = 334.5f, gcy = ey + 3.0f, grx = 12.5f, gry = 13.0f;
            const uint32_t gemT = WithAlpha(RGBA(224, 236, 140, 235), rowT);
            const uint32_t gemB = WithAlpha(RGBA(210, 221, 108, 235), rowT);
            const V2 gem[4] = { { gcx, gcy - gry }, { gcx + grx, gcy }, { gcx, gcy + gry }, { gcx - grx, gcy } };
            const uint32_t gcol[4] = { gemT, gemT, gemB, gemB };
            DrawQuadGradient(gem, gcol);
        }
        char buf[8]; snprintf(buf, sizeof buf, "x%d", g_expCount);
        Chrome({ BAR_END + 16, ey + 4 }, 30.0f, buf, rowT, C_CHR_T, C_CHR_B, 1.2f);
    }

    // ---- per-form stat rows (staggered entrance) ----
    const Row* rows = g_night ? WEREHOG_ROWS : SONIC_ROWS;
    const int n = g_night ? 5 : 2;
    for (int i = 0; i < n; ++i) {
        const float rt = (float)ComputeMotion(openSec, 6.0 + i * 3.0, 8.0);
        if (rt > 0.0f) StatRow(StatTop0() + i * RowPitch(), rows[i].label, rows[i].fill, i == g_sel, rt, false);
    }

    // ---- QUIT plate below the stat list (a small chamfered button + curl tail) ----
    {
        const float qt = (float)ComputeMotion(openSec, 6.0 + n * 3.0, 8.0);
        if (qt > 0.0f) {
            // QUIT plate centered near manifest decide_bg (center x316, y554) for
            // BOTH forms; this removes the day~425/night~509 drift off the stack.
            const float qy = 540, qx = 224, qw = 190, qh = 38;   // widened (plate ~199px incl slant)
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
            // label is center-right with a visible ~50px tail before the chevron,
            // NOT flush right: qx0 + qw - textW - 20  (lands start ~x257-272)
            const float qtw = MeasureText(22.0f, "QUIT").x;
            const float qlx = qx0 + qw - qtw - 20.0f;
            DrawText({ qlx + 1.5f, qy + 8 + 1.5f }, 22.0f, WithAlpha(RGBA(14, 14, 22, 255), qt), "QUIT");
            DrawText({ qlx, qy + 8 }, 22.0f, WithAlpha(C_WHITE, qt), "QUIT");
            ResetTextShear();
            ResetFont();
        }
    }

    // ---- footer (RETAIL prompts, measured from manifest rects) ----
    // bottom-left:  LB(244,618,80x40) + "Switch"(320,625) + RB(440,618,80x40)
    // bottom-right: green A(836,618,40x40) + "Select"(876,625)
    // The Q/E form-switch and Up/Down still drive Input(); only the displayed
    // prompts change to match retail. Both forms show the identical footer.
    {
        // glyph as an image at an exact manifest rect; falls back to a button-pill
        // placeholder when the x360 glyph sheet is missing.
        auto glyph = [&](const UV& g, float x, float y, float w, float h, uint32_t fill){
            if (g_glyphTex >= 0) {
                DrawImage(g_glyphTex, { x, y }, { x + w, y + h }, { g.u0, g.v0 }, { g.u1, g.v1 }, WithAlpha(C_WHITE, a));
            } else {
                DrawRect({ x, y }, { x + w, y + h }, WithAlpha(fill, a * 0.9f));
                DrawRect({ x, y }, { x + w, y + 2 }, WithAlpha(C_WHITE, a * 0.6f));
            }
        };
        SetFont(g_fRodin);
        // bottom-left: switch-form cluster
        glyph(GLYPH_LB, 244, 618, 80, 40, RGBA(64, 70, 82, 235));
        DrawText({ 320, 625 }, 20.0f, WithAlpha(C_WHITE, a), "Switch");
        glyph(GLYPH_RB, 440, 618, 80, 40, RGBA(64, 70, 82, 235));
        // bottom-right: select cluster (green A)
        glyph(GLYPH_A, 836, 618, 40, 40, RGBA(48, 168, 72, 235));
        DrawText({ 876, 625 }, 20.0f, WithAlpha(C_WHITE, a), "Select");
        ResetFont();
    }
}

// CSD cast-state tag: the host renders the real status CSD as the base layer in
// the matching day(Sonic)/night(Werehog) variant; this reports which to pick.
const char* CsdState() { return g_night ? "ev" : "so"; }

} // namespace

void StatusInit() { Init(); }
// CSD-base composite: when the real status CSD is loaded (host drew it as the
// base), the screen is the game's own layout — our Draw adds nothing yet. Only
// when the CSD is unavailable do we fall back to the hand-authored layout.
void StatusDraw(double openSeconds) {
    if (std::strcmp(csd::LoadedId(), "status") == 0) return;   // CSD base IS the screen
    Draw(openSeconds);                                          // hand-authored fallback
}
void StatusInput(const ScreenInput& in) { Input(in); }
void StatusReset() { Reset(); }
const char* StatusCsdState() { return CsdState(); }
