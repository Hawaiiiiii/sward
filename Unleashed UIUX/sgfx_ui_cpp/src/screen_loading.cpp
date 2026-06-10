// =============================================================================
// screen_loading.cpp — the inter-stage Loading screen, hand-authored in the same
// clean idiom as the rest (the batch workflow dropped this one, so it is written
// directly). A loading screen is a transient overlay: a dim backdrop, a "NOW
// LOADING" caption with animated trailing dots, and a looping progress bar drawn
// from the REAL Sonic Unleashed silver-chrome frame (ui::DrawGameWindow) with a
// gold fill — no rotation API, so motion is a time-driven fill sweep + dot cycle.
// A rotating hint line sits above it. Minimal interactivity (it is a wait screen).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <cmath>

using namespace ui;

namespace {

// ---- layout (reference px) --------------------------------------------------
constexpr float BAR_X = 720.0f, BAR_W = 480.0f, BAR_H = 26.0f, BAR_Y = 632.0f;
constexpr float CAP_Y = 588.0f;

// rotating loading hints (cycled by elapsed time)
const char* const HINTS[] = {
    "TIP: Hold the boost to blast through enemies.",
    "TIP: Time your jumps off ramps for extra air.",
    "TIP: At night, grab and throw foes with the Werehog.",
    "TIP: Collect Sun and Moon Medals to open new stages.",
    "TIP: Drift around tight corners to keep your speed.",
};
constexpr int HINT_COUNT = int(sizeof(HINTS) / sizeof(HINTS[0]));

const uint32_t COL_BG_TOP = RGBA(8, 12, 22, 255);
const uint32_t COL_BG_BOT = RGBA(3, 5, 11, 255);
const uint32_t COL_CAP    = RGBA(255, 209, 74, 255);   // gold
const uint32_t COL_HINT   = RGBA(176, 192, 212, 255);
const uint32_t COL_FILL_T = RGBA(255, 224, 120, 255);
const uint32_t COL_FILL_B = RGBA(214, 150, 30, 255);
const uint32_t COL_TRACK  = RGBA(40, 52, 72, 235);

// Real game MSDF font (im_font_atlas): FOT-SeuratPro-M for the hint line. The
// "NOW LOADING" caption stays on font 0 (bitmap wordmark bucket, per audit).
int g_fSeurat = 0;
void Init() {
    GameFrameTex(); /* shared chrome frame */
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF (im_font_atlas)
}
void Reset() {}
void Input(const ScreenInput&) { /* wait screen — no navigation */ }

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const double now = Now();
    const float inT = (float)ComputeMotion(openSec, 0.0, 12.0);   // fade the whole thing in

    // rotating hint, centred above the bar
    int hint = (int)std::fmod(now / 4.0, (double)HINT_COUNT);
    if (hint < 0 || hint >= HINT_COUNT) hint = 0;
    SetFont(g_fSeurat);
    DrawTextAligned({ 0, 360 }, { REF_W, 400 }, 24.0f, WithAlpha(COL_HINT, inT * 0.9f),
                    HINTS[hint], Align::Center, true, true);
    ResetFont();   // caption below stays on font 0 (bitmap wordmark bucket)

    // "NOW LOADING" + animated trailing dots, bottom-left of the bar
    int dots = (int)std::fmod(now / 0.4, 4.0);
    char cap[24]; std::snprintf(cap, sizeof(cap), "NOW LOADING%.*s", dots, "...");
    DrawTextShadow({ BAR_X, CAP_Y - (1.0f - inT) * 10.0f }, 32.0f, WithAlpha(COL_CAP, inT), cap);

    // progress track (real chrome frame) + a looping gold fill sweep
    DrawGameWindow({ BAR_X, BAR_Y }, { BAR_X + BAR_W, BAR_Y + BAR_H }, 0.0f, inT, COL_TRACK);
    float p = (float)std::fmod(now * 0.4, 1.0);                   // 0..1 repeating
    float innerX = BAR_X + 6.0f, innerW = (BAR_W - 12.0f) * std::clamp(p, 0.0f, 1.0f);
    if (innerW > 2.0f)
        DrawVGradient({ innerX, BAR_Y + 6 }, { innerX + innerW, BAR_Y + BAR_H - 6 },
                      WithAlpha(COL_FILL_T, inT), WithAlpha(COL_FILL_B, inT));
    ResetFont();
}

} // namespace

void LoadingInit() { Init(); }
void LoadingDraw(double openSeconds) { Draw(openSeconds); }
void LoadingInput(const ScreenInput& in) { Input(in); }
void LoadingReset() { Reset(); }
