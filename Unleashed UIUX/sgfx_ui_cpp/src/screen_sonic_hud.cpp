// =============================================================================
// screen_sonic_hud.cpp â€” the day-stage in-game HUD overlay, re-authored as clean
// hand-written C++ in the UnleashedRecomp ui/options_menu idiom (NOT a CSD node
// dump). The old transcription parked the score/ring plates OFF-SCREEN at y=-24
// and smeared the gauge as a flat strip; this version places every element at a
// sane on-screen rect in 1280x720 reference space.
//
// Unlike the menu screens there is NO full-screen panel â€” the HUD floats over live
// gameplay, so the background stays transparent (no DrawVGradient fill). The art
// that makes this read as retail Sonic Unleashed is the REAL extracted atlas:
//   * the blue Sonic-head ring emblem (mat_comon_001, left half),
//   * the seven-segment score/ring DIGITS (mat_comon_num_001),
//   * the "SCORE" / "RINGS" word-labels (mat_playscreen_en_001),
//   * the gold boost-gauge tire emblem + the rainbow energy bar (ui_ps1_gauge1).
//
// Light interactivity (matching the shop/status idiom): (A) toggles the boost
// drain, the boost gauge fill animates, and collecting rings (Up) / spending boost
// is reflected live. The key fix is correct ON-SCREEN placement of the real
// gauge / ring icon / score art â€” bounded rects, never an unclipped stretch.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

using namespace ui;

namespace {

// ---- real-art atlases -------------------------------------------------------
// All five live in assets/sonic_hud/. Sizes (px): mat_comon_001 256x128,
// mat_comon_num_001 512x64, mat_playscreen_en_001 128x128, ui_ps1_gauge1 256x128.
const char* const ASSET_BASE = "assets/sonic_hud/";
int g_headTex  = -1;   // mat_comon_001         (Sonic-head ring emblem, left = day)
int g_numTex   = -1;   // mat_comon_num_001     (digit font)
int g_enTex    = -1;   // mat_playscreen_en_001 (word labels)
int g_gaugeTex = -1;   // ui_ps1_gauge1         (gold tire + rainbow boost bar)

// ---- real game fonts (MSDF) -------------------------------------------------
static int g_fRodin = 0;   // rodin_db: values / footer control hints
static int g_fDF = 0;      // DFSoGei: the READY overlay wordmark

struct UV { float u0, v0, u1, v1; };

// --- mat_comon_001 (256x128): the DAY Sonic-head ring emblem is the left half;
//     measured tight opaque box px (7,13)-(122,113), aspect ~1.15.
const UV HEAD_DAY = { 0.02734f, 0.10156f, 0.47656f, 0.88281f };

// --- mat_playscreen_en_001 (128x128): horizontal word-label bands, measured by
//     scanning opaque rows. (Order top->bottom: RING / ENERGY / SPEED / RINGS /
//     TIME / SCORE / COUNT / LAP TIME.)
const UV LBL_SCORE = { 0.00781f, 0.57031f, 0.46875f, 0.64062f };   // "SCORE"
const UV LBL_RINGS = { 0.00781f, 0.38281f, 0.42969f, 0.45312f };   // "RINGS"

// --- ui_ps1_gauge1 (256x128):
//   * gold tire emblem  : tight gold box px (16,16)-(61,60)  -> uv below (~square)
//   * rainbow boost bar : tight box px (55,64)-(135,80)      -> 80x16 strip
const UV GAUGE_TIRE = { 0.0625f,   0.125f, 0.23828f, 0.46875f };
const UV GAUGE_BAR  = { 0.21484f,  0.5f,   0.52734f, 0.625f   };

// --- mat_comon_num_001 (512x64): row-0 digit cells, measured per glyph. Each
//     digit shares the same vertical span (v 0.01562..0.4375); the ':' is index 10.
//     Index 0..9 = '0'..'9'.
const UV NUM_GLYPH[11] = {
    { 0.00195f, 0.01562f, 0.05273f, 0.4375f },   // 0
    { 0.06836f, 0.01562f, 0.09961f, 0.4375f },   // 1 (narrow)
    { 0.11523f, 0.01562f, 0.16406f, 0.4375f },   // 2
    { 0.17188f, 0.01562f, 0.22070f, 0.4375f },   // 3
    { 0.22656f, 0.01562f, 0.27734f, 0.4375f },   // 4
    { 0.28320f, 0.01562f, 0.33398f, 0.4375f },   // 5
    { 0.33984f, 0.01562f, 0.38867f, 0.4375f },   // 6
    { 0.39648f, 0.01562f, 0.44531f, 0.4375f },   // 7
    { 0.45312f, 0.01562f, 0.50195f, 0.4375f },   // 8
    { 0.50977f, 0.01562f, 0.56250f, 0.4375f },   // 9
    { 0.57617f, 0.01562f, 0.60156f, 0.4375f },   // ':' (index 10)
};
// pixel aspect (w/h) of a digit cell so the glyph isn't stretched on screen.
float NumAspect(int g) {
    const UV& u = NUM_GLYPH[g];
    return ((u.u1 - u.u0) * 512.0f) / ((u.v1 - u.v0) * 64.0f);
}

float Aspect(const UV& u, float texW, float texH) {
    return ((u.u1 - u.u0) * texW) / ((u.v1 - u.v0) * texH);
}

// ---- layout (reference px) --------------------------------------------------
// Top-left score/ring cluster (the old dump put these at y=-24 â€” fixed to y~46).
constexpr float HEAD_X = 92.0f,  HEAD_Y = 46.0f,  HEAD_H = 56.0f;   // ring emblem box height
constexpr float CL_LABEL_X = 162.0f;                                 // labels start right of emblem
constexpr float SCORE_LBL_Y = 50.0f, SCORE_NUM_Y = 70.0f;            // SCORE label + digits
constexpr float RINGS_LBL_Y = 104.0f, RINGS_NUM_Y = 122.0f;          // RINGS label + digits
constexpr float DIGIT_H = 30.0f;                                      // on-screen digit height
constexpr float DIGIT_GAP = 2.0f;                                     // gap between digits
constexpr float NUM_X = 286.0f;                                       // left edge of digit run

// Bottom boost gauge (the curved rainbow energy bar). In retail it sweeps along
// the lower-left; here it is a bounded track, never an unclipped stretch.
constexpr float TIRE_X = 96.0f,  TIRE_Y = 600.0f, TIRE_H = 56.0f;     // gold tire emblem box
constexpr float BAR_X  = 158.0f, BAR_Y  = 616.0f, BAR_W = 470.0f, BAR_H = 24.0f;  // boost track

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double CLUSTER_FRAMES = 14.0;   // top-left score/ring
constexpr double GAUGE_OFFSET   = 4.0,  GAUGE_FRAMES = 16.0;   // bottom boost gauge
constexpr double FOOT_OFFSET    = 10.0, FOOT_FRAMES  = 12.0;
constexpr double BOOST_MOVE_FRAMES = 14.0;  // boost-fill easing when it changes

// ---- palette ----------------------------------------------------------------
const uint32_t COL_WHITE     = RGBA(255, 255, 255, 255);
const uint32_t COL_TRACK     = RGBA(20, 28, 44, 150);     // dim empty boost track
const uint32_t COL_TRACK_EDGE= RGBA(150, 178, 214, 120);  // track outline
const uint32_t COL_SCORE     = RGBA(255, 255, 255, 255);  // digit tint (art is light)
const uint32_t COL_FOOTER    = RGBA(210, 220, 235, 220);
const uint32_t COL_BOOST_GLOW= RGBA(120, 200, 255, 255);

// ---- interactive state ------------------------------------------------------
int    g_score = 124500;       // sample score
int    g_rings = 42;           // sample ring count
float  g_boost = 0.62f;        // boost gauge fill 0..1
float  g_boostPrev = 0.62f;    // for eased transition
double g_boostStart = -100.0;  // Now() when the boost fill last changed
bool   g_draining = false;     // (A) toggles drain on/off

void Commafy(int v, char* out, int n) {
    char raw[16]; std::snprintf(raw, sizeof(raw), "%d", v);
    int len = (int)std::strlen(raw), o = 0;
    for (int i = 0; i < len && o < n - 1; ++i) {
        if (i > 0 && (len - i) % 3 == 0 && o < n - 1) out[o++] = ',';
        out[o++] = raw[i];
    }
    out[o] = '\0';
}

void SetBoost(float v) {
    g_boostPrev = g_boost;
    g_boost = std::clamp(v, 0.0f, 1.0f);
    g_boostStart = Now();
}

void Init() {
    if (g_headTex  < 0) g_headTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_001.png");
    if (g_numTex   < 0) g_numTex   = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_num_001.png");
    if (g_enTex    < 0) g_enTex    = gfx::loadTexture(std::string(ASSET_BASE) + "mat_playscreen_en_001.png");
    if (g_gaugeTex < 0) g_gaugeTex = gfx::loadTexture(std::string(ASSET_BASE) + "ui_ps1_gauge1.png");
    if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db");
    if (g_fDF    == 0) g_fDF    = LoadFont("assets/fonts/dfsoge7.ttc");
}

void Reset() {
    g_score = 124500; g_rings = 42;
    g_boost = 0.62f; g_boostPrev = 0.62f; g_boostStart = -100.0; g_draining = false;
}

void Input(const ScreenInput& in) {
    // (A) toggles whether the boost is draining (so the fill animation is visible).
    if (in.accept) g_draining = !g_draining;
    // Up = pick up a ring (+1 ring, +100 score); Down = spend a chunk of boost.
    if (in.up)    { g_rings = std::min(999, g_rings + 1); g_score += 100; }
    if (in.down)  SetBoost(g_boost - 0.2f);
    // Left/Right nudge the boost meter directly (handy for inspection).
    if (in.left)  SetBoost(g_boost - 0.1f);
    if (in.right) SetBoost(g_boost + 0.1f);
    // cancel: would hide the HUD in-game; no-op in the standalone build.
}

// ---- helpers ----------------------------------------------------------------

// Draw a label word from mat_playscreen_en_001, fit to a target height, left-anchored.
void DrawLabel(const UV& u, float x, float y, float targetH, float t) {
    if (g_enTex < 0 || t <= 0.0f) return;
    float w = targetH * Aspect(u, 128.0f, 128.0f);
    DrawImage(g_enTex, { x, y }, { x + w, y + targetH },
              { u.u0, u.v0 }, { u.u1, u.v1 }, WithAlpha(COL_WHITE, t));
}

// Draw an integer using the real digit atlas, right-anchored at rightX, baseline-top at y.
// Returns the left edge x actually used (for layout). Digits keep their native aspect.
void DrawNumber(int value, float rightX, float y, float h, float t) {
    if (g_numTex < 0 || t <= 0.0f) return;
    char buf[24]; Commafy(value, buf, sizeof(buf));   // commas -> we render ',' as a thin gap
    int len = (int)std::strlen(buf);
    // measure total width first (right-to-left layout)
    float totalW = 0.0f;
    for (int i = 0; i < len; ++i) {
        char c = buf[i];
        if (c == ',') { totalW += h * 0.30f; continue; }
        int g = c - '0';
        totalW += h * NumAspect(g) + DIGIT_GAP;
    }
    float x = rightX - totalW;
    for (int i = 0; i < len; ++i) {
        char c = buf[i];
        if (c == ',') { x += h * 0.30f; continue; }
        int g = c - '0';
        const UV& u = NUM_GLYPH[g];
        float w = h * NumAspect(g);
        DrawImage(g_numTex, { x, y }, { x + w, y + h },
                  { u.u0, u.v0 }, { u.u1, u.v1 }, WithAlpha(COL_SCORE, t));
        x += w + DIGIT_GAP;
    }
}

// The bottom boost gauge: a bounded dim track, the rainbow energy bar clipped to the
// current fill fraction (UV-clipped so the art is cropped, not squashed), plus the
// gold tire emblem at the left cap. fill in [0,1].
void DrawBoostGauge(float fill, float t) {
    if (t <= 0.0f) return;
    const float bx = BAR_X, by = BAR_Y, bw = BAR_W, bh = BAR_H;

    // dim empty track behind the fill (bounded rect + thin top/bottom edge lines)
    DrawRect({ bx, by }, { bx + bw, by + bh }, WithAlpha(COL_TRACK, t));
    DrawRect({ bx, by - 1 }, { bx + bw, by + 1 },       WithAlpha(COL_TRACK_EDGE, t));
    DrawRect({ bx, by + bh - 1 }, { bx + bw, by + bh }, WithAlpha(COL_TRACK_EDGE, t));

    // rainbow energy bar, cropped to `fill` along its width via UV interpolation so
    // the gradient is the real art (red->green->blue), not a stretched smear.
    fill = std::clamp(fill, 0.0f, 1.0f);
    if (g_gaugeTex >= 0 && fill > 0.001f) {
        const UV& u = GAUGE_BAR;
        float uMid = u.u0 + (u.u1 - u.u0) * fill;     // crop the right end of the source strip
        float fw   = bw * fill;
        DrawImage(g_gaugeTex, { bx, by }, { bx + fw, by + bh },
                  { u.u0, u.v0 }, { uMid, u.v1 }, WithAlpha(COL_WHITE, t));
        // a soft additive glow at the leading edge to read as "energy"
        float gx = bx + fw - 8.0f;
        DrawImage(g_gaugeTex, { gx, by - 2 }, { gx + 16.0f, by + bh + 2 },
                  { uMid - 0.01f, u.v0 }, { uMid, u.v1 },
                  WithAlpha(COL_BOOST_GLOW, t * 0.55f), /*additive*/ true);
    }

    // gold tire emblem at the left cap (square box, native aspect ~1.0)
    if (g_gaugeTex >= 0) {
        const UV& u = GAUGE_TIRE;
        float aw = TIRE_H * Aspect(u, 256.0f, 128.0f);
        float ty = TIRE_Y + (1.0f - t) * 10.0f;       // small settle slide
        DrawImage(g_gaugeTex, { TIRE_X, ty }, { TIRE_X + aw, ty + TIRE_H },
                  { u.u0, u.v0 }, { u.u1, u.v1 }, WithAlpha(COL_WHITE, t));
    }
}

// the stage-start READY overlay (live capture s09 @173-174s): an italic chrome
// wordmark with motion-streak trails that sweeps THROUGH the screen — in fast,
// a short hold at ref (552..792, 152..205), then out right.
void DrawReadyOverlay(double el) {
    if (el > 1.7) return;
    // sweep position: slide in 0..0.25s, hold to 1.3s, slide out 1.3..1.7s
    float x = 640.0f;
    float a = 1.0f;
    if (el < 0.25)      { float p = (float)(el / 0.25); p = 1.0f - (1.0f - p) * (1.0f - p); x = -300.0f + (640.0f + 300.0f) * p; a = p; }
    else if (el > 1.3)  { float p = (float)((el - 1.3) / 0.4); p *= p; x = 640.0f + 940.0f * p; a = 1.0f - p * 0.6f; }
    SetFont(g_fDF);
    SetTextShear(0.26f);
    SetTextStretchX(1.45f);
    const char* R = "READY";
    const float fsz = 78.0f;                       // large banner (~38% screen width)
    float w = MeasureText(fsz, R).x * 1.45f;
    const V2 rp = { x - w * 0.5f, 312.0f };         // vertical centre of the screen
    // motion streaks (additive bars trailing the wordmark through its band)
    for (int i = 0; i < 4; ++i) {
        float sy = 332.0f + i * 22.0f;
        DrawRect({ rp.x - 300.0f + i * 50.0f, sy }, { rp.x + w * 0.5f, sy + 4.0f },
                 WithAlpha(RGBA(180, 205, 235, 255), a * 0.26f), true);
    }
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ rp.x + dx * 3.0f, rp.y + dy * 3.0f }, fsz, WithAlpha(RGBA(16, 22, 38, 255), a), R);
    DrawTextGradient(rp, fsz, WithAlpha(RGBA(238, 244, 252, 255), a), WithAlpha(RGBA(140, 165, 205, 255), a), R);
    ResetTextStretchX();
    ResetTextShear();
    ResetFont();
}

void Draw(double openSec) {
    // NO background fill: the HUD is a transparent overlay over gameplay.

    const float clusterT = (float)ComputeMotion(openSec, 0.0, CLUSTER_FRAMES);
    const float gaugeT   = (float)ComputeMotion(openSec, GAUGE_OFFSET, GAUGE_FRAMES);
    const float footT    = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);
    DrawReadyOverlay(Now() - openSec >= 0 ? (Now() - openSec) : 0.0);

    // ---- top-left score / ring cluster ----
    // Sonic-head ring emblem (real art), slides in from the left.
    if (g_headTex >= 0 && clusterT > 0.0f) {
        float hw = HEAD_H * Aspect(HEAD_DAY, 256.0f, 128.0f);
        float hx = HEAD_X - (1.0f - clusterT) * 18.0f;
        DrawImage(g_headTex, { hx, HEAD_Y }, { hx + hw, HEAD_Y + HEAD_H },
                  { HEAD_DAY.u0, HEAD_DAY.v0 }, { HEAD_DAY.u1, HEAD_DAY.v1 },
                  WithAlpha(COL_WHITE, clusterT));
    }

    // SCORE label + digits (top row of the cluster)
    DrawLabel(LBL_SCORE, CL_LABEL_X, SCORE_LBL_Y, 14.0f, clusterT);
    DrawNumber(g_score, NUM_X + 300.0f, SCORE_NUM_Y, DIGIT_H, clusterT);

    // RINGS label + count (second row)
    DrawLabel(LBL_RINGS, CL_LABEL_X, RINGS_LBL_Y, 14.0f, clusterT);
    DrawNumber(g_rings, NUM_X + 300.0f, RINGS_NUM_Y, DIGIT_H, clusterT);

    // ---- bottom boost gauge (eased fill) ----
    float displayBoost = g_boost;
    {
        // ease the fill between previous and current when it changes
        float moveT = (float)ComputeMotion(g_boostStart, 0.0, BOOST_MOVE_FRAMES);
        if (g_boostStart > 0.0) displayBoost = Lerp(g_boostPrev, g_boost, moveT);
        // when "draining" is toggled on, the boost animates down over time (visible motion)
        if (g_draining) {
            double age = Now() - (g_boostStart > 0.0 ? g_boostStart : openSec);
            float drained = g_boost - (float)age * 0.18f;   // ~5.5s to empty
            displayBoost = std::clamp(std::min(displayBoost, drained), 0.0f, 1.0f);
        }
    }
    DrawBoostGauge(displayBoost, gaugeT);

    // ---- footer control hint (sits just under the boost track, on-screen) ----
    SetFont(g_fRodin);
    DrawTextAligned({ TIRE_X, 668 }, { 760, 700 }, 20.0f, WithAlpha(COL_FOOTER, footT),
                    "(A) Boost   [Up] Ring   [L/R] Meter", Align::Left, true, true);
    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
// Init/Draw are wired today; Input/Reset are provided for the interactive HUD â€”
// the integrator can swap the registry row to { ..., &SonicHudInput, &SonicHudReset }.
void SonicHudInit() { Init(); }
void SonicHudDraw(double openSeconds) { Draw(openSeconds); }
void SonicHudInput(const ScreenInput& in) { Input(in); }
void SonicHudReset() { Reset(); }
