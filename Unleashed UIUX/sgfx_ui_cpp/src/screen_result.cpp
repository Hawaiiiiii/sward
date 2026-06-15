// =============================================================================
// screen_result.cpp — the day-stage RESULTS tally, re-authored 1:1 from LIVE
// capture (session11 @66-192s; spec stills result_settled_fullres /
// result_rank_settled; geometry measured frame-by-frame, footage px / 1.5):
//   * the LIVE 3D scene stays behind (Sonic + Chip posing) — here a sky/scene
//     placeholder slot;
//   * top rail: navy band (66,84,134) y63..101 from the left edge to x635 with
//     a bright blue-white edge line under it; italic chrome "RESULTS" wordmark
//     on the rail (x263..536, cap y72..92), SLIDES IN left->right (~0.5 s);
//   * five stat rows (TIME/RINGS/SPEED/ENEMY/TRICKS): slanted navy
//     parallelogram label plates (white outline, green italic label) in a
//     DIAGONAL CASCADE (each row +12 px right), h 41.3, pitch 66.7, first top
//     y188; value digits italic chrome, LEFT-aligned ~17 px after each plate;
//     rows build top->bottom (~0.4 s) after the wordmark; values TALLY-count;
//   * TOTAL row: green gradient plate (40,142,90)->(66,161,113), navy text,
//     at y512 (taller, ~58), counts after the stat tally;
//   * RANK: green strip + a BIG gold rank letter at (331,433)-(484,609),
//     scale-pop reveal after the total lands;
//   * footer: (A) Next, bottom-right.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_glyphTex = -1;   // controller glyph atlas (for the (A) footer glyph)
int g_rankTex  = -1;   // mat_result_comon_002: the REAL metal rank letters (2x3 grid)
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

struct UV { float u0, v0, u1, v1; };
const UV GLYPH_A = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
constexpr float GTW = 512.0f, GTH = 512.0f;
// tight sub-rects measured from the atlas alpha (same table as result_ex)
const char* const RANK_NAME[6] = { "S", "A", "B", "C", "D", "E" };
const UV RANK_UV[6] = {
    { 0.0293f, 0.0234f, 0.2656f, 0.2695f }, { 0.3203f, 0.0254f, 0.5625f, 0.2676f },
    { 0.0273f, 0.3262f, 0.2617f, 0.5566f }, { 0.3125f, 0.3203f, 0.5703f, 0.5625f },
    { 0.0293f, 0.6113f, 0.2656f, 0.8438f }, { 0.3281f, 0.6191f, 0.5547f, 0.8379f },
};

// ---- palette (sampled from the live capture) --------------------------------
const uint32_t C_RAIL      = RGBA(66, 84, 134, 235);     // header rail navy
const uint32_t C_RAIL_EDGE = RGBA(169, 188, 234, 255);   // bright under-edge line
const uint32_t C_PLATE_T   = RGBA(46, 54, 88, 235);      // stat plate navy top (darker/desat, alpha up)
const uint32_t C_PLATE_B   = RGBA(34, 42, 74, 235);      // stat plate navy bottom (darker/desat, alpha up)
const uint32_t C_PLATE_BD  = RGBA(235, 240, 248, 255);   // plate white outline
const uint32_t C_LABEL     = RGBA(190, 206, 230, 255);   // white/light-blue label (measured)
const uint32_t C_TOTAL_T   = RGBA(68, 158, 124, 235);    // TOTAL teal-green top (blue lifted)
const uint32_t C_TOTAL_B   = RGBA(46, 140, 124, 235);    // TOTAL teal-green bottom (blue lifted)
const uint32_t C_TOTAL_TXT = RGBA(16, 42, 64, 255);      // navy TOTAL text
const uint32_t C_CHR_T     = RGBA(244, 246, 250, 255);   // chrome digits top
const uint32_t C_CHR_B     = RGBA(170, 178, 192, 255);   // chrome digits bottom
const uint32_t C_CHR_OUT   = RGBA(20, 24, 34, 255);      // digit outline
const uint32_t C_RANK_GOLD_T = RGBA(238, 196, 92, 255);  // rank letter gold
const uint32_t C_RANK_GOLD_B = RGBA(168, 116, 28, 255);
const uint32_t C_WHITE     = RGBA(255, 255, 255, 255);
// scene placeholder (the real game keeps the 3D goal scene live behind)
const uint32_t C_SKY_T = RGBA(96, 158, 208, 255), C_SKY_B = RGBA(176, 208, 228, 255);
const uint32_t C_GROUND = RGBA(214, 216, 212, 255);

// ---- measured layout (1280x720 reference) -----------------------------------
constexpr float RAIL_Y0 = 20, RAIL_Y1 = 71, RAIL_X1 = 635;   // raised near the top edge (real RESULTS is high, only a thin sky strip above)
constexpr float WM_X = 254, WM_CAPTOP = 33;             // wordmark target position (left edge ~x258, centerY ~43)
constexpr float ROW_X0 = 619, ROW_W = 214;              // first label plate rect
constexpr float ROW_TOP0 = 188, ROW_H = 41.3f, ROW_PITCH = 66.7f;
constexpr float ROW_XSTEP = 12;                          // diagonal cascade per row
constexpr float VAL_GAP = 17;                            // digits start after plate
constexpr float TOT_TOP = 512, TOT_H = 58;
constexpr float SLANT = 10;                              // plate edge slant (px over h)
constexpr int   N_ROWS = 5;

struct Row { const char* label; int value; const char* fmt; };
// sample data = the captured run, so renders diff directly against the footage
Row ROWS[N_ROWS] = {
    { "TIME",   18176, "time" },   // 03:01.76 stored as CENTISECONDS (valid mid-tally)
    { "RINGS",  10100, "num" },
    { "SPEED",   9162, "num" },
    { "ENEMY",   4800, "num" },
    { "TRICKS",  9451, "num" },
};
int g_total = 90336;
const char* g_rank = "C";

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_rankTex  < 0) g_rankTex  = gfx::loadTexture("assets/result/mat_result_comon_002.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}

// slanted parallelogram plate (italic lean: top edge shifted right by SLANT)
void Plate(float x, float y, float w, float h, uint32_t cT, uint32_t cB, uint32_t bd, float a) {
    const V2 c[4] = { { x + SLANT, y }, { x + w + SLANT, y }, { x + w, y + h }, { x, y + h } };
    const uint32_t col[4] = { WithAlpha(cT, a), WithAlpha(cT, a), WithAlpha(cB, a), WithAlpha(cB, a) };
    DrawQuadGradient(c, col);
    auto edge = [&](V2 p0, V2 p1, V2 p2, V2 p3) {
        const V2 e[4] = { p0, p1, p2, p3 };
        const uint32_t ec[4] = { WithAlpha(bd, a), WithAlpha(bd, a), WithAlpha(bd, a), WithAlpha(bd, a) };
        DrawQuadGradient(e, ec);
    };
    edge({ x + SLANT, y }, { x + w + SLANT, y }, { x + w + SLANT - 0.4f, y + 2 }, { x + SLANT - 0.4f, y + 2 });
    edge({ x + 0.4f, y + h - 2 }, { x + w + 0.4f, y + h - 2 }, { x + w, y + h }, { x, y + h });
    edge({ x + SLANT, y }, { x + SLANT + 2, y }, { x + 2, y + h }, { x, y + h });
    edge({ x + w + SLANT - 2, y }, { x + w + SLANT, y }, { x + w, y + h }, { x + w - 2, y + h });
}

// italic chrome text (the game's wordmark/digit treatment)
void Chrome(V2 pos, float px, const char* s, float a, float stretch = 1.2f) {
    SetFont(g_fDF);
    SetTextShear(0.24f);
    SetTextStretchX(stretch);
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ pos.x + dx * 2.0f, pos.y + dy * 2.0f }, px, WithAlpha(C_CHR_OUT, a), s);
    DrawTextGradient(pos, px, WithAlpha(C_CHR_T, a), WithAlpha(C_CHR_B, a), s);
    ResetTextStretchX();
    ResetTextShear();
    ResetFont();
}

void FormatValue(const Row& r, int v, char* buf, size_t n) {
    if (!strcmp(r.fmt, "time")) {   // v = centiseconds -> mm:ss:cc (always valid mid-tally)
        snprintf(buf, n, "%02d:%02d:%02d", v / 6000, (v / 100) % 60, v % 100);
    } else {
        snprintf(buf, n, "%d", v);
    }
}

void Draw(double openSec) {
    // ---- scene placeholder (live 3D goal scene slot) ----
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_SKY_T, C_SKY_B);
    DrawVGradient({ 0, 560 }, { REF_W, REF_H }, C_GROUND, RGBA(190, 192, 188, 255));

    // ---- entrance timeline (measured): wordmark slides ~0.5 s; rows build
    //      top->bottom after it; tally counts ~1.2 s; rank pops after total ----
    const float wmT = (float)ComputeMotion(openSec, 0.0, 30.0);
    DrawRect({ 0, RAIL_Y0 }, { RAIL_X1, RAIL_Y1 }, WithAlpha(C_RAIL, wmT));
    DrawRect({ 0, RAIL_Y0 - 2 }, { RAIL_X1, RAIL_Y0 + 1 }, WithAlpha(C_RAIL_EDGE, wmT));   // ~3px top-edge highlight (y~61)
    DrawRect({ 0, RAIL_Y1 + 1 }, { RAIL_X1, RAIL_Y1 + 3.5f }, WithAlpha(C_RAIL_EDGE, wmT));   // under-edge (y~115)
    {
        float wx = Lerp(-220.0f, WM_X, wmT);   // slides in from off-left
        Chrome({ wx, WM_CAPTOP - 12 }, 48.0f, "RESULTS", wmT, 1.70f);
    }

    const float VAL_R = 1033;   // common right edge the value strips align to (measured: shared Chrome right edge ~x1015)
    const uint32_t C_VSTRIP_T = RGBA(18, 26, 44, 180), C_VSTRIP_B = RGBA(8, 14, 28, 180);
    for (int i = 0; i < N_ROWS; ++i) {
        const float rowT = (float)ComputeMotion(openSec, 26.0 + i * 5.0, 10.0);
        if (rowT <= 0.0f) continue;
        const float x = ROW_X0 + i * ROW_XSTEP;
        const float y = ROW_TOP0 + i * ROW_PITCH;
        // long dark translucent value strip extending from the plate to VAL_R
        const float sx0 = x + ROW_W + SLANT - 6;
        const V2 vs[4] = { { sx0 + SLANT, y + 4 }, { VAL_R, y + 4 }, { VAL_R, y + ROW_H - 4 }, { sx0, y + ROW_H - 4 } };
        const uint32_t vsc[4] = { WithAlpha(C_VSTRIP_T, rowT), WithAlpha(C_VSTRIP_T, rowT), WithAlpha(C_VSTRIP_B, rowT), WithAlpha(C_VSTRIP_B, rowT) };
        DrawQuadGradient(vs, vsc);
        DrawRect({ sx0 + SLANT, y + 4 }, { VAL_R, y + 5.5f }, WithAlpha(RGBA(120, 140, 175, 200), rowT));   // thin top edge
        Plate(x, y, ROW_W, ROW_H, C_PLATE_T, C_PLATE_B, C_PLATE_BD, rowT);
        SetFont(g_fSeurat);
        SetTextShear(0.20f);
        {   // dark 8-direction outline pass behind the label (Chrome-style), then the light fill
            const V2 lp = { x + 38, y + (ROW_H - 24) * 0.5f };
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    if (dx || dy)
                        DrawText({ lp.x + dx * 1.5f, lp.y + dy * 1.5f }, 24.0f, WithAlpha(RGBA(20, 28, 44, 255), rowT), ROWS[i].label);
            DrawText(lp, 24.0f, WithAlpha(C_LABEL, rowT), ROWS[i].label);
        }
        ResetTextShear();
        ResetFont();
        // tally value, RIGHT-aligned inside the strip to the common edge
        const float tallyT = (float)ComputeMotion(openSec, 60.0 + i * 3.0, 72.0);
        char buf[24];
        FormatValue(ROWS[i], (int)std::lround(ROWS[i].value * tallyT), buf, sizeof buf);
        SetFont(g_fDF);
        SetTextShear(0.24f); SetTextStretchX(1.2f);
        float vw = MeasureText(32.0f, buf).x * 1.2f;
        ResetTextStretchX(); ResetTextShear(); ResetFont();
        Chrome({ VAL_R - vw - 18, y + ROW_H * 0.5f - 16 }, 32.0f, buf, rowT);
    }

    // ---- TOTAL (one long green bar stepping LEFT, value right-aligned inside) ----
    {
        const float totT = (float)ComputeMotion(openSec, 50.0, 10.0);
        if (totT > 0.0f) {
            const float x = 625.0f;        // green plate left (real green plate x628-797)
            const float TOT_PLATE_W = 165;  // compact green label plate (green ends ~x793)
            // teal value strip from the green plate right edge to VAL_R (real is teal, not navy)
            const uint32_t C_TSTRIP_T = RGBA(72, 150, 138, 200), C_TSTRIP_B = RGBA(46, 118, 112, 200);   // brighter teal (green +~32)
            const float sx0 = x + TOT_PLATE_W + SLANT - 6;
            const V2 vs[4] = { { sx0 + SLANT, TOT_TOP + 4 }, { VAL_R, TOT_TOP + 4 }, { VAL_R, TOT_TOP + TOT_H - 4 }, { sx0, TOT_TOP + TOT_H - 4 } };
            const uint32_t vsc[4] = { WithAlpha(C_TSTRIP_T, totT), WithAlpha(C_TSTRIP_T, totT), WithAlpha(C_TSTRIP_B, totT), WithAlpha(C_TSTRIP_B, totT) };
            DrawQuadGradient(vs, vsc);
            DrawRect({ sx0 + SLANT, TOT_TOP + 4 }, { VAL_R, TOT_TOP + 5.5f }, WithAlpha(RGBA(120, 160, 158, 200), totT));   // thin teal top edge
            Plate(x, TOT_TOP, TOT_PLATE_W, TOT_H, C_TOTAL_T, C_TOTAL_B, C_PLATE_BD, totT);
            SetFont(g_fRodin);
            SetTextShear(0.20f);
            DrawText({ x + 40, TOT_TOP + (TOT_H - 26) * 0.5f }, 26.0f, WithAlpha(C_TOTAL_TXT, totT), "TOTAL");
            ResetTextShear();
            ResetFont();
            const float totTally = (float)ComputeMotion(openSec, 132.0, 40.0);
            char buf[16]; snprintf(buf, sizeof buf, "%d", (int)std::lround(g_total * totTally));
            SetFont(g_fDF); SetTextShear(0.24f); SetTextStretchX(1.2f);
            float vw = MeasureText(36.0f, buf).x * 1.2f;
            ResetTextStretchX(); ResetTextShear(); ResetFont();
            Chrome({ VAL_R - vw - 18, TOT_TOP + TOT_H * 0.5f - 18 }, 36.0f, buf, totT);
        }
    }

    // ---- RANK reveal: wide teal band behind the big gold letter (scale pop) ----
    {
        const float rkT = (float)ComputeMotion(openSec, 178.0, 10.0);
        if (rkT > 0.0f) {
            // wide layered teal band behind the rank letter (measured y~520-564)
            DrawVGradient({ 0, 519 }, { 560, 524 }, WithAlpha(RGBA(78, 176, 158, 225), rkT), WithAlpha(RGBA(78, 176, 158, 225), rkT));
            DrawVGradient({ 0, 524 }, { 560, 565 }, WithAlpha(RGBA(64, 146, 135, 205), rkT), WithAlpha(RGBA(46, 114, 108, 205), rkT));   // brighter band body (green +~30)
            SetFont(g_fSeurat);
            SetTextShear(0.22f);
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    if (dx || dy)
                        DrawText({ 256 + dx * 1.4f, 533 + dy * 1.4f }, 24.0f, WithAlpha(RGBA(180, 184, 106, 255), rkT), "RANK");
            DrawTextGradient({ 256, 533 }, 24.0f, WithAlpha(RGBA(245, 225, 130, 255), rkT), WithAlpha(RGBA(205, 170, 70, 255), rkT), "RANK");
            ResetTextShear();
            ResetFont();
            // rank letter pops with an overshoot: scale 1.6 -> 1.0 about its centre
            // measured letter bbox (331,433)-(484,609): ~153 wide x 176 tall
            const float s = 1.0f + (1.0f - rkT) * 0.6f;
            const float cx = 424.0f, cy = 532.0f, hw = 63.0f * s, hh = 62.0f * s;   // widen C: w~126 (real ~127)
            int ri = 0; while (ri < 5 && RANK_NAME[ri][0] != g_rank[0]) ++ri;
            if (g_rankTex >= 0) {   // the REAL metal letter art, warm-gold tinted (as captured)
                DrawImage(g_rankTex, { cx - hw, cy - hh }, { cx + hw, cy + hh },
                          { RANK_UV[ri].u0, RANK_UV[ri].v0 }, { RANK_UV[ri].u1, RANK_UV[ri].v1 },
                          WithAlpha(RGBA(255, 194, 84, 255), rkT));   // stronger warm-gold tint (was washed-out silver)
            } else {                // fallback: chrome-recipe letter
                SetFont(g_fDF);
                SetTextShear(0.10f);
                SetTextStretchX(1.25f);
                const float ps = hh * 2.0f * 1.42f;
                const V2 rp = { cx - hh * 0.9f, cy + hh - hh * 2.0f * 1.28f };
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        if (dx || dy)
                            DrawText({ rp.x + dx * 3.0f, rp.y + dy * 3.0f }, ps, WithAlpha(RGBA(60, 36, 6, 255), rkT), g_rank);
                DrawTextGradient(rp, ps, WithAlpha(C_RANK_GOLD_T, rkT), WithAlpha(C_RANK_GOLD_B, rkT), g_rank);
                ResetTextStretchX();
                ResetTextShear();
                ResetFont();
            }
        }
    }

    // ---- footer: (A) Next (below the TOTAL value, ~70% / 87%) ----
    {
        const float fT = (float)ComputeMotion(openSec, 60.0, 10.0);
        float hx = 856, hcy = 638;   // glyph center ~876,638 (manifest btn_a x856 y618 h40)
        if (g_glyphTex >= 0) {
            float asp = ((GLYPH_A.u1 - GLYPH_A.u0) * GTW) / ((GLYPH_A.v1 - GLYPH_A.v0) * GTH), gh = 34.0f, gw = gh * asp;   // (A) orb ~w31/h34 (was 28; matches btn_a 40px region)
            DrawImage(g_glyphTex, { hx, hcy - gh * 0.5f }, { hx + gw, hcy + gh * 0.5f },
                      { GLYPH_A.u0, GLYPH_A.v0 }, { GLYPH_A.u1, GLYPH_A.v1 }, WithAlpha(C_WHITE, fT));
            hx += gw + 8;
        }
        SetFont(g_fRodin);
        {   // dark 8-direction outline pass behind 'Next' (Chrome-style), then the white fill
            const V2 np = { hx, hcy - 12 };
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    if (dx || dy)
                        DrawText({ np.x + dx * 1.5f, np.y + dy * 1.5f }, 22.0f, WithAlpha(RGBA(20, 28, 44, 255), fT), "Next");
            DrawText(np, 22.0f, WithAlpha(C_WHITE, fT), "Next");
        }
        ResetFont();
    }
}

} // namespace

void ResultInit() { Init(); }
void ResultDraw(double openSeconds) { Draw(openSeconds); }
