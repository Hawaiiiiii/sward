// =============================================================================
// screen_result_ex.cpp â€” the Extended Result screen, a variant of the stage
// Result, re-authored as clean hand-written C++ in the UnleashedRecomp
// ui/options_menu idiom (NOT a CSD node dump â€” the raw result_ex.json packs
// dozens of unclipped 9-slice stretch-arms at x=-87/-32 and overlapped rank
// stamps that smear if transcribed). Layout lives in named 1280x720 constants.
//
// Modeled closely on screen_result.cpp: a column of real Sonic Unleashed
// silver-chrome stat bars (DrawGameWindow), the TOTAL row tinted gold, a rank
// badge on the left, and a footer prompt. The distinctive game art that makes
// this read as retail is the REAL extracted atlas:
//   * the metal rank letters S/A/B/C/D/E   (mat_result_comon_002, a 2x3 grid),
//   * the Xbox (A) button glyph            (mat_comon_x360_001, top-left cell),
// each placed at a deliberate sane rect with aspect preserved (the world_map
// fit-box idiom); bounded gold text is the graceful fallback when an atlas is
// missing (-1 slot).
//
// Interactive + stateful like options_menu / shop:
//   * (A) = NEXT â€” confirms the tally and advances to the next demo result set
//     (the stat values + rank letter all change live), with a transient flash,
//   * (B) â€” would back out in-game; no-op in the standalone build.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

using namespace ui;

namespace {

// ---- per-element UV sub-rect (normalized atlas coords) ----------------------
struct UV { float u0, v0, u1, v1; };

// ---- real-art atlases -------------------------------------------------------
// mat_result_comon_002 (512x512) packs the six metal rank letters in a 2x3 grid:
//   S A   (row 0)
//   B C   (row 1)
//   D E   (row 2)
// mat_comon_x360_001 (512x512) packs the Xbox button glyphs; (A) is the top-left
// cell. The shared silver-chrome window frame (mat_result_comon_001, 1024x512) is
// loaded + cached by ui::GameFrameTex() inside DrawGameWindow â€” we don't touch it.
const char* const ASSET_BASE = "assets/result_ex/";
int g_rankTex  = -1;   // mat_result_comon_002
int g_glyphTex = -1;   // mat_comon_x360_001

constexpr float RANK_TEX_W = 512.0f, RANK_TEX_H = 512.0f;
constexpr float GLYPH_TEX_W = 512.0f, GLYPH_TEX_H = 512.0f;

// rank letters: 0=S 1=A 2=B 3=C 4=D 5=E. Tight opaque sub-rects measured from the
// atlas alpha channel (px box -> normalized UV); each cell is ~square.
const char* const RANK_NAME[6] = { "S", "A", "B", "C", "D", "E" };
const UV RANK_UV[6] = {
    /* S */ { 0.0293f, 0.0234f, 0.2656f, 0.2695f },
    /* A */ { 0.3203f, 0.0254f, 0.5625f, 0.2676f },
    /* B */ { 0.0273f, 0.3262f, 0.2617f, 0.5566f },
    /* C */ { 0.3125f, 0.3203f, 0.5703f, 0.5625f },
    /* D */ { 0.0293f, 0.6113f, 0.2656f, 0.8438f },
    /* E */ { 0.3281f, 0.6191f, 0.5547f, 0.8379f },
};

// Xbox (A) button glyph â€” tight box from the top-left of mat_comon_x360_001.
const UV GLYPH_A = { 0.0000f, 0.0039f, 0.0879f, 0.0742f };

// ---- result data set --------------------------------------------------------
// SCORE / TIME / RINGS / BONUS / TOTAL â€” the five extended-result rows. (A)=NEXT
// cycles through these demo sets so the screen visibly responds.
struct ResultSet {
    const char* time;      // "MM:SS.cc"
    int         rings;     // ring count
    int         score;     // base stage score
    int         bonus;     // speed/ring bonus pool
    int         total;     // grand total
    int         rank;      // 0..5 -> S,A,B,C,D,E
};
const ResultSet RESULTS[] = {
    { "01:23.45", 120,  45600, 22000,  67600, 0 },   // S
    { "02:04.10",  88,  31200, 14000,  45200, 1 },   // A
    { "03:18.72",  54,  20800,  8500,  29300, 2 },   // B
    { "04:46.05",  31,  12400,  4200,  16600, 3 },   // C
};
constexpr int RESULT_COUNT = int(sizeof(RESULTS) / sizeof(RESULTS[0]));

// ---- row model (label + which field of the ResultSet it shows) --------------
enum Field { F_SCORE, F_TIME, F_RINGS, F_BONUS, F_TOTAL };
struct Row { const char* label; Field field; };
const Row ROWS[5] = {
    { "SCORE", F_SCORE },
    { "TIME",  F_TIME  },
    { "RINGS", F_RINGS },
    { "BONUS", F_BONUS },
    { "TOTAL", F_TOTAL },
};
constexpr int ROW_COUNT = 5;

// ---- layout (reference px) --------------------------------------------------
// Stat bars stacked on the right column, clearing the rank badge on the left
// (the same column split screen_result.cpp uses, narrowed for a 5-row stack).
constexpr float TITLE_X = 150.0f, TITLE_Y = 50.0f, RULE_Y = 122.0f;

constexpr float BAR_L     = 360.0f, BAR_R = 1140.0f;   // bar span (clears the rank column)
constexpr float ROW_TOP   = 168.0f;                    // top of the first stat bar
constexpr float ROW_PITCH = 76.0f;                     // vertical spacing between bars
constexpr float ROW_H     = 56.0f;                     // bar height
constexpr float LABEL_X   = BAR_L + 34.0f;             // label inset
constexpr float VALUE_R   = BAR_R - 28.0f;             // right edge for values

// rank badge column (left), aligned with the stat stack
constexpr float RANK_X = 96.0f,  RANK_Y = 250.0f, RANK_W = 224.0f, RANK_H = 224.0f;

// footer prompt
constexpr float FOOT_X = 150.0f, FOOT_Y = 640.0f, FOOT_R = 1140.0f, FOOT_H = 40.0f;

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double TITLE_FRAMES = 16.0;
constexpr double ROW_OFFSET   = 8.0;     // first row begins after the title
constexpr double ROW_STAGGER  = 4.0;     // frames between rows sliding in
constexpr double ROW_FRAMES   = 14.0;
constexpr double RANK_DELAY   = 30.0;    // rank badge pops after the rows
constexpr double RANK_FRAMES  = 18.0;
constexpr double FOOT_OFFSET  = 36.0, FOOT_FRAMES = 12.0;

// ---- palette (shared with pause/result/shop/status/world_map) ---------------
const uint32_t COL_BG_TOP   = RGBA(10, 16, 30, 255);
const uint32_t COL_BG_BOT   = RGBA(4, 7, 14, 255);
const uint32_t COL_TITLE    = RGBA(255, 209, 74, 255);   // gold
const uint32_t COL_LABEL    = RGBA(206, 222, 240, 255);
const uint32_t COL_VALUE    = RGBA(255, 255, 255, 255);
const uint32_t COL_TOTAL_V  = RGBA(255, 233, 150, 255);  // bright gold TOTAL value
const uint32_t COL_RANK     = RGBA(255, 224, 92, 255);   // gold rank-letter fallback
const uint32_t COL_RULE     = RGBA(120, 170, 230, 90);
const uint32_t COL_PLATE    = RGBA(6, 10, 20, 235);      // dark plate behind the rank art
const uint32_t COL_FOOTER   = RGBA(210, 222, 240, 230);
const uint32_t COL_OK       = RGBA(120, 230, 140, 255);
const uint32_t COL_WHITE    = RGBA(255, 255, 255, 255);

// bar body tints (silver-chrome via DrawGameWindow): TOTAL row gold-tinted, like
// screen_result.cpp's TOTAL bar (0xFFA07828).
constexpr uint32_t BAR_TINT_NORMAL = 0xFF4A608Cu;
constexpr uint32_t BAR_TINT_TOTAL  = 0xFFA07828u;

// ---- fonts (real game MSDF + DFSoGei SDF, options pattern) -------------------
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// ---- interactive state ------------------------------------------------------
int    g_set = 0;               // which RESULTS entry is shown
double g_msgStart = -100.0;     // Now() of the last (A) NEXT confirm

void Commafy(int v, char* out, int n) {
    char raw[16]; std::snprintf(raw, sizeof(raw), "%d", v);
    int len = (int)std::strlen(raw), o = 0;
    for (int i = 0; i < len && o < n - 1; ++i) {
        if (i > 0 && (len - i) % 3 == 0 && o < n - 1) out[o++] = ',';
        out[o++] = raw[i];
    }
    out[o] = '\0';
}

// fill `out` with the displayed value for a row's field
void FieldValue(const ResultSet& r, Field f, char* out, int n) {
    switch (f) {
        case F_TIME:  std::snprintf(out, n, "%s", r.time); break;
        case F_RINGS: Commafy(r.rings, out, n);            break;
        case F_SCORE: Commafy(r.score, out, n);            break;
        case F_BONUS: Commafy(r.bonus, out, n);            break;
        case F_TOTAL: Commafy(r.total, out, n);            break;
        default:      out[0] = '\0';                       break;
    }
}

void Init() {
    GameFrameTex();   // lazy-load + cache the shared chrome frame for DrawGameWindow
    if (g_rankTex  < 0) g_rankTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_result_comon_002.png");
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF (im_font_atlas)
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");    // real game MSDF
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");   // real DFSoGeiStd-W7 (title)
}

void Reset() { g_set = 0; g_msgStart = -100.0; }

void Input(const ScreenInput& in) {
    if (in.accept) {                                   // (A) = NEXT
        g_set = (g_set + 1) % RESULT_COUNT;            // advance to the next result set
        g_msgStart = Now();
    }
    // cancel: would back out of the result tally in-game; no-op in the standalone build.
}

// ---- aspect-preserving image fit into a box (world_map idiom) ---------------
void DrawFitted(int tex, const UV& uv, float texW, float texH,
                float bx, float by, float bw, float bh,
                float t, float vAnchor = 0.5f, float slide = 0.0f) {
    if (tex < 0 || t <= 0.0f) return;
    float artW = (uv.u1 - uv.u0) * texW;
    float artH = (uv.v1 - uv.v0) * texH;
    if (artW <= 0.0f || artH <= 0.0f) return;
    float scale = std::min(bw / artW, bh / artH);
    float w = artW * scale, h = artH * scale;
    float x = bx + (bw - w) * 0.5f;
    float y = by + (bh - h) * vAnchor + slide;
    DrawImage(tex, { x, y }, { x + w, y + h }, { uv.u0, uv.v0 }, { uv.u1, uv.v1 },
              WithAlpha(COL_WHITE, t));
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const ResultSet& r = RESULTS[g_set];
    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float rankT  = (float)ComputeMotion(openSec, RANK_DELAY, RANK_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ---- title (gold, with shadow) + rule, like result/shop ----
    SetFont(g_fDF);
    DrawTextShadow({ TITLE_X, TITLE_Y - (1.0f - titleT) * 16.0f }, 46.0f,
                   WithAlpha(COL_TITLE, titleT), "RESULT");
    DrawRect({ TITLE_X, RULE_Y }, { BAR_R, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));

    // ---- stat bars (silver-chrome, slide in from the right, staggered) ----
    for (int i = 0; i < ROW_COUNT; ++i) {
        float t = (float)ComputeMotion(openSec, ROW_OFFSET + i * ROW_STAGGER, ROW_FRAMES);
        if (t <= 0.0f) continue;
        float slide = (1.0f - t) * 60.0f;            // ease in from +60px right
        float top   = ROW_TOP + i * ROW_PITCH;
        bool  total = (ROWS[i].field == F_TOTAL);

        // real Unleashed silver-chrome bar (gold-tinted for the TOTAL row)
        DrawGameWindow({ BAR_L + slide, top }, { BAR_R + slide, top + ROW_H }, 0.0f, t,
                       total ? BAR_TINT_TOTAL : BAR_TINT_NORMAL);

        SetFont(g_fSeurat);
        DrawTextAligned({ LABEL_X + slide, top }, { BAR_L + slide + 360.0f, top + ROW_H },
                        total ? 32.0f : 28.0f, WithAlpha(COL_LABEL, t),
                        ROWS[i].label, Align::Left, true, true);

        char vbuf[24];
        FieldValue(r, ROWS[i].field, vbuf, sizeof(vbuf));
        SetFont(g_fRodin);
        DrawTextAligned({ BAR_L + slide + 360.0f, top }, { VALUE_R + slide, top + ROW_H },
                        total ? 34.0f : 30.0f,
                        WithAlpha(total ? COL_TOTAL_V : COL_VALUE, t),
                        vbuf, Align::Right, true, true);
    }

    // ---- rank badge (left column) â€” pops in after the rows ----
    if (rankT > 0.0f) {
        // dark plate behind the badge, then the real metal rank letter art (fit-boxed)
        DrawRect({ RANK_X, RANK_Y }, { RANK_X + RANK_W, RANK_Y + RANK_H }, WithAlpha(COL_PLATE, rankT));
        // chrome frame around the plate (real Unleashed window 9-slice)
        DrawGameWindow({ RANK_X, RANK_Y }, { RANK_X + RANK_W, RANK_Y + RANK_H }, 0.0f, rankT);

        SetFont(g_fSeurat);
        DrawTextAligned({ RANK_X, RANK_Y - 44 }, { RANK_X + RANK_W, RANK_Y - 8 }, 26.0f,
                        WithAlpha(COL_LABEL, rankT), "RANK", Align::Center, true, true);

        int rank = std::clamp(r.rank, 0, 5);
        if (g_rankTex >= 0) {
            // inset the letter inside the chrome frame, centred + a small pop-in slide
            const float inset = 30.0f;
            DrawFitted(g_rankTex, RANK_UV[rank], RANK_TEX_W, RANK_TEX_H,
                       RANK_X + inset, RANK_Y + inset,
                       RANK_W - inset * 2.0f, RANK_H - inset * 2.0f,
                       rankT, 0.5f, (1.0f - rankT) * 10.0f);
        } else {
            // gold-letter fallback (grows in)
            SetFont(g_fDF);
            DrawTextAligned({ RANK_X, RANK_Y }, { RANK_X + RANK_W, RANK_Y + RANK_H },
                            150.0f * rankT, WithAlpha(COL_RANK, rankT),
                            RANK_NAME[rank], Align::Center, true, true);
        }
    }

    // ---- transient (A) NEXT confirm flash ----
    double age = Now() - g_msgStart;
    if (g_msgStart > 0.0 && age < 1.2) {
        float ma = std::min(1.0f, (float)((1.2 - age) / 0.35));
        SetFont(g_fRodin);
        DrawTextAligned({ BAR_L, 596 }, { BAR_R, 626 }, 24.0f,
                        WithAlpha(COL_OK, ma), "NEXT", Align::Center, true, true);
    }

    // ---- footer prompt: real (A) glyph + "NEXT" (right-aligned), like world_map ----
    if (footT > 0.0f) {
        SetFont(g_fRodin);   // rodin for the footer label + "(A)" fallback (measure in the same font)
        const float gh = 30.0f, cy = FOOT_Y + FOOT_H * 0.5f;
        const char* label = "NEXT";
        float labelW = MeasureText(24.0f, label).x;
        float gAspect = ((GLYPH_A.u1 - GLYPH_A.u0) * GLYPH_TEX_W) /
                        ((GLYPH_A.v1 - GLYPH_A.v0) * GLYPH_TEX_H);
        float gw = (g_glyphTex >= 0) ? gh * gAspect : MeasureText(24.0f, "(A)").x;
        float gap = 8.0f;
        float blockW = gw + gap + labelW;
        float x = FOOT_R - blockW;

        if (g_glyphTex >= 0) {
            DrawImage(g_glyphTex, { x, cy - gh * 0.5f }, { x + gw, cy + gh * 0.5f },
                      { GLYPH_A.u0, GLYPH_A.v0 }, { GLYPH_A.u1, GLYPH_A.v1 },
                      WithAlpha(COL_WHITE, footT));
        } else {
            DrawText({ x, cy - 14.0f }, 24.0f, WithAlpha(COL_FOOTER, footT), "(A)");
        }
        DrawText({ x + gw + gap, cy - 14.0f }, 24.0f, WithAlpha(COL_FOOTER, footT), label);
    }

    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void ResultExInit() { Init(); }
void ResultExDraw(double openSeconds) { Draw(openSeconds); }
void ResultExInput(const ScreenInput& in) { Input(in); }
void ResultExReset() { Reset(); }
