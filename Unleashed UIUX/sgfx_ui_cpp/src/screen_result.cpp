// =============================================================================
// screen_result.cpp — the stage Result screen, reconstructed as clean C++ in the
// UnleashedRecomp idiom. Row Y positions are the real values parsed from the game
// layout (ui_result.yncp -> manifests/result.json): six stat bars at y =
// 155/222/289/356/423/489 (47px tall), a rank badge at the left, a footer prompt.
// Bars/labels are procedural for now; swap DrawVGradient/DrawText for DrawImage on
// mat_result_comon_*/mat_result_en_* for final 1:1 (assets last).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

using namespace ui;

namespace {

// real per-row top Y (reference px) from the parsed layout
constexpr float ROW_Y[6]   = { 155, 222, 289, 356, 423, 489 };
constexpr float ROW_H      = 47.0f;
constexpr float BAR_L      = 150.0f, BAR_R = 980.0f;     // narrowed to clear the rank column
constexpr float LABEL_X    = 184.0f, VALUE_R = 950.0f;

constexpr double TITLE_FRAMES = 16.0;
constexpr double ROW_STAGGER  = 4.0;     // frames between rows sliding in
constexpr double ROW_FRAMES   = 14.0;
constexpr double RANK_DELAY    = 30.0;   // rank pops after the rows
constexpr double RANK_FRAMES   = 18.0;

struct Stat { const char* label; const char* value; };
const Stat STATS[6] = {
    { "TIME",        "01:23.45" },
    { "RINGS",       "120"      },
    { "SCORE",       "45,600"   },
    { "SPEED BONUS", "10,000"   },
    { "RING BONUS",  "12,000"   },
    { "TOTAL",       "67,600"   },
};

const uint32_t COL_BG_TOP  = RGBA(10, 16, 30, 255);
const uint32_t COL_BG_BOT  = RGBA(4, 7, 14, 255);
const uint32_t COL_BAR_TOP = RGBA(30, 52, 92, 210);
const uint32_t COL_BAR_BOT = RGBA(14, 26, 50, 210);
const uint32_t COL_BAR_TOT = RGBA(150, 110, 30, 230);   // TOTAL row tinted gold
const uint32_t COL_LABEL   = RGBA(206, 222, 240, 255);
const uint32_t COL_VALUE   = RGBA(255, 255, 255, 255);
const uint32_t COL_TITLE   = RGBA(255, 209, 74, 255);
const uint32_t COL_RANK    = RGBA(255, 224, 92, 255);
const uint32_t COL_RULE    = RGBA(120, 170, 230, 90);

static int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    // title
    float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    SetFont(g_fDF);
    DrawTextBevel({ 150, 70 - (1.0f - titleT) * 16.0f }, 46.0f, WithAlpha(COL_TITLE, titleT), "RESULT");
    DrawRect({ 150, 122 }, { BAR_R, 124 }, WithAlpha(COL_RULE, titleT));

    // stat rows (slide in from the right, staggered)
    for (int i = 0; i < 6; ++i) {
        float t = (float)ComputeMotion(openSec, 8.0 + i * ROW_STAGGER, ROW_FRAMES);
        if (t <= 0.0f) continue;
        float slide = (1.0f - t) * 60.0f;     // ease in from +60px right
        float top = ROW_Y[i];
        bool total = (i == 5);
        // real Unleashed silver-chrome bar (gold-tinted for the TOTAL row)
        DrawGameWindow({ BAR_L + slide, top }, { BAR_R + slide, top + ROW_H }, 0.0f, t,
                       total ? 0xFFA07828u : 0xFF4A608Cu);
        SetFont(g_fSeurat);
        DrawTextAligned({ LABEL_X + slide, top }, { 700, top + ROW_H }, total ? 30.0f : 27.0f,
                        WithAlpha(COL_LABEL, t), STATS[i].label, Align::Left, true, true);
        SetFont(g_fRodin);
        DrawTextAligned({ 700, top }, { VALUE_R + slide, top + ROW_H }, total ? 32.0f : 28.0f,
                        WithAlpha(COL_VALUE, t), STATS[i].value, Align::Right, true, true);
    }

    // rank panel (right column, beside the stat bars) — pops in after the rows
    float rankT = (float)ComputeMotion(openSec, RANK_DELAY, RANK_FRAMES);
    if (rankT > 0.0f) {
        const float rx = 1010.0f, ry = 250.0f, rw = 240.0f, rh = 240.0f;
        DrawGameWindow({ rx, ry }, { rx + rw, ry + rh }, 0.0f, rankT);   // real chrome frame
        SetFont(g_fRodin);
        DrawTextAligned({ rx, ry - 42 }, { rx + rw, ry - 8 }, 24.0f,
                        WithAlpha(COL_LABEL, rankT), "RANK", Align::Center, true, true);
        SetFont(g_fDF);
        DrawTextAligned({ rx, ry }, { rx + rw, ry + rh }, 150.0f * rankT,
                        WithAlpha(COL_RANK, rankT), "S", Align::Center, true, true);
    }

    // footer prompt
    float footT = (float)ComputeMotion(openSec, RANK_DELAY + 6.0, ROW_FRAMES);
    float footPulse = footT * Breathe(Now(), 0.55f, 1.0f, 1.3f);   // gentle call-to-action pulse
    SetFont(g_fRodin);
    DrawTextAligned({ 760, 620 }, { 1100, 660 }, 24.0f, WithAlpha(RGBA(210, 222, 240, 230), footPulse),
                    "(A) NEXT", Align::Right, true, true);
    ResetFont();
}

} // namespace

void ResultInit() { Init(); }
void ResultDraw(double openSeconds) { Draw(openSeconds); }
