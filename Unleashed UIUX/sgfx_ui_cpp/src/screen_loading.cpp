// =============================================================================
// screen_loading.cpp — the hub/lab loading screen, re-authored 1:1 from LIVE
// capture (session8 @71-77s; spec still loading_miles.png): the whole screen
// IS Tails' MILES ELECTRIC tablet —
//   * yellow device shell (gradient (235,194,60)->(203,178,45)) with speaker
//     grilles, camera dot, corner screws, VOL/POWER edge rockers and two
//     yellow d-pad clusters;
//   * dark inset display (ref ~(179,47)-(1101,661)) with a glossy top glare
//     and a bright bottom reflection line, showing a green-wireframe town map
//     on a perspective grid floor, gold landmark medallion slots, a photo
//     inset slot, the NOW LOADING wordmark (the real mat_load_en_001 sprite)
//     with the walking spinner grid, and the tiny MILES ELECTRIC mark.
// The map/photo/medallion art are SLOTS (SEGA art / runtime content).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_wordTex = -1;   // mat_load_en_001 (NOW LOADING wordmark + spinner cell)
const float WM_U0 = 0.3750f, WM_V0 = 0.0078f, WM_U1 = 0.9961f, WM_V1 = 0.2812f;
const float SQ_U0 = 0.9102f, SQ_V0 = 0.3125f, SQ_U1 = 0.9805f, SQ_V1 = 0.5859f;

// ---- palette ------------------------------------------------------------------
const uint32_t C_SHELL_T  = RGBA(235, 194, 60, 255);   // device yellow top
const uint32_t C_SHELL_B  = RGBA(203, 178, 45, 255);
const uint32_t C_SHELL_DK = RGBA(140, 116, 24, 255);   // button outlines / recesses
const uint32_t C_HOLE     = RGBA(70, 58, 16, 255);     // speaker holes
const uint32_t C_BEZEL    = RGBA(26, 26, 24, 255);
const uint32_t C_SCREEN_T = RGBA(16, 19, 16, 255);     // display (slight top glare)
const uint32_t C_SCREEN_B = RGBA(6, 8, 6, 255);
const uint32_t C_WIRE     = RGBA(60, 230, 80, 255);    // wireframe green
const uint32_t C_WIRE_DIM = RGBA(24, 110, 36, 255);
const uint32_t C_GOLD     = RGBA(200, 160, 60, 255);   // medallion slots
const uint32_t C_GREEN    = RGBA(74, 230, 18, 255);    // NOW LOADING tint
const uint32_t C_GREEN_B  = RGBA(50, 147, 12, 255);

// screen inset (measured)
constexpr float SC_X0 = 179, SC_Y0 = 47, SC_X1 = 1101, SC_Y1 = 661;

void Init() { if (g_wordTex < 0) g_wordTex = gfx::loadTexture("assets/loading/mat_load_en_001.png"); }
void Reset() {}
void Input(const ScreenInput&) {}

void Screw(float cx, float cy) {
    DrawRect({ cx - 9, cy - 9 }, { cx + 9, cy + 9 }, C_SHELL_DK);
    DrawRect({ cx - 6, cy - 6 }, { cx + 6, cy + 6 }, RGBA(176, 148, 38, 255));
    DrawRect({ cx - 5, cy - 1 }, { cx + 5, cy + 1 }, C_SHELL_DK);
}

void SpeakerGrille(float x0, float y0) {
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 10; ++c)
            DrawRect({ x0 + c * 14.0f, y0 + r * 12.0f }, { x0 + c * 14.0f + 5, y0 + r * 12.0f + 5 }, C_HOLE);
}

void DPad(float cx, float cy) {
    const float arm = 34, th = 20;
    DrawRect({ cx - arm, cy - th * 0.5f }, { cx + arm, cy + th * 0.5f }, C_SHELL_DK);
    DrawRect({ cx - th * 0.5f, cy - arm }, { cx + th * 0.5f, cy + arm }, C_SHELL_DK);
    DrawRect({ cx - arm + 3, cy - th * 0.5f + 3 }, { cx + arm - 3, cy + th * 0.5f - 3 }, RGBA(222, 186, 52, 255));
    DrawRect({ cx - th * 0.5f + 3, cy - arm + 3 }, { cx + th * 0.5f - 3, cy + arm - 3 }, RGBA(222, 186, 52, 255));
    DrawRect({ cx - 6, cy - 6 }, { cx + 6, cy + 6 }, C_SHELL_DK);
}

void DrawSpinner(float x0, float y0, double now) {
    static const int RING[8][2] = { {0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1} };
    const float cell = 8.0f, step = 10.0f;
    int head = (int)(now * 10.0) % 8;
    for (int i = 0; i < 8; ++i) {
        int d = (head - i + 8) % 8;
        float k = (d == 0) ? 1.0f : (d == 1 ? 0.86f : (d == 2 ? 0.76f : 0.68f));
        uint32_t c = ColourLerp(RGBA(28, 92, 8, 255), C_GREEN, k);
        float gx = x0 + RING[i][0] * step, gy = y0 + RING[i][1] * step;
        if (g_wordTex >= 0)
            DrawImage(g_wordTex, { gx, gy }, { gx + cell, gy + cell }, { SQ_U0, SQ_V0 }, { SQ_U1, SQ_V1 }, c);
        else
            DrawRect({ gx, gy }, { gx + cell, gy + cell }, c);
    }
}

void Draw(double openSec) {
    (void)openSec;
    const double now = Now();

    // ---- yellow device shell ----
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_SHELL_T, C_SHELL_B);
    SpeakerGrille(28, 10);  SpeakerGrille(1112, 10);
    DrawRect({ 632, 20 }, { 648, 36 }, C_BEZEL);                       // camera dot
    DrawRect({ 635, 23 }, { 645, 33 }, RGBA(50, 56, 60, 255));
    Screw(20, 20); Screw(1260, 20); Screw(20, 700); Screw(1260, 700);
    DrawRect({ 0, 280 }, { 14, 380 }, C_SHELL_DK);                     // VOL rocker
    DrawRect({ 2, 286 }, { 12, 374 }, RGBA(190, 158, 40, 255));
    DrawRect({ 1266, 280 }, { 1280, 380 }, C_SHELL_DK);                // POWER
    DrawRect({ 1268, 286 }, { 1278, 374 }, RGBA(190, 158, 40, 255));
    SetFont(0);
    DrawText({ 18, 392 }, 12.0f, C_SHELL_DK, "VOL");
    DrawText({ 1218, 392 }, 12.0f, C_SHELL_DK, "POWER");
    DPad(96, 600); DPad(1184, 600);

    // ---- display bezel + screen ----
    DrawRect({ SC_X0 - 12, SC_Y0 - 10 }, { SC_X1 + 12, SC_Y1 + 14 }, C_BEZEL);
    DrawVGradient({ SC_X0, SC_Y0 }, { SC_X1, SC_Y1 }, C_SCREEN_T, C_SCREEN_B);
    DrawRect({ SC_X0, SC_Y1 + 6 }, { SC_X1, SC_Y1 + 8 }, RGBA(120, 122, 118, 160));   // reflection line
    SetFont(0);
    DrawText({ SC_X1 - 96, SC_Y1 + 1 }, 10.0f, RGBA(150, 152, 148, 255), "MILES=ELECTRIC");

    // ---- perspective grid floor (converging green lines) ----
    {
        const float horizonY = 300, baseY = SC_Y1 - 24, cx = 640;
        for (int i = -8; i <= 8; ++i) {
            float bx = cx + i * 92.0f, hx = cx + i * 26.0f;
            const V2 q[4] = { { hx, horizonY }, { hx + 1.5f, horizonY }, { bx + 2.5f, baseY }, { bx, baseY } };
            const uint32_t qc[4] = { WithAlpha(C_WIRE_DIM, 0.35f), WithAlpha(C_WIRE_DIM, 0.35f),
                                     WithAlpha(C_WIRE_DIM, 0.8f), WithAlpha(C_WIRE_DIM, 0.8f) };
            DrawQuadGradient(q, qc);
        }
        for (int i = 0; i < 7; ++i) {
            float f = i / 6.0f, y = horizonY + (baseY - horizonY) * f * f;
            DrawRect({ SC_X0 + 40, y }, { SC_X1 - 40, y + 1.2f }, WithAlpha(C_WIRE_DIM, 0.3f + 0.5f * f));
        }
    }

    // ---- wireframe town path (stylized loop slot) ----
    {
        const V2 loop[8] = { { 430, 470 }, { 560, 410 }, { 760, 400 }, { 880, 450 },
                             { 820, 520 }, { 680, 555 }, { 540, 545 }, { 440, 510 } };
        for (int i = 0; i < 8; ++i) {
            const V2& p0 = loop[i]; const V2& p1 = loop[(i + 1) % 8];
            V2 n = { (p1.y - p0.y), (p0.x - p1.x) };
            float len = std::sqrt(n.x * n.x + n.y * n.y); n.x = n.x / len * 7; n.y = n.y / len * 7;
            const V2 q[4] = { p0, p1, { p1.x + n.x, p1.y + n.y }, { p0.x + n.x, p0.y + n.y } };
            const uint32_t qc[4] = { C_WIRE, C_WIRE, WithAlpha(C_WIRE, 0.6f), WithAlpha(C_WIRE, 0.6f) };
            DrawQuadGradient(q, qc);
        }
        // wireframe building hints
        auto bld = [&](float bx, float by, float w, float h) {
            DrawRect({ bx, by - h }, { bx + w, by - h + 2 }, C_WIRE);
            DrawRect({ bx, by - h }, { bx + 2, by }, C_WIRE);
            DrawRect({ bx + w - 2, by - h }, { bx + w, by }, C_WIRE);
            DrawRect({ bx, by - 2 }, { bx + w, by }, WithAlpha(C_WIRE, 0.7f));
        };
        bld(620, 520, 56, 70); bld(806, 500, 44, 52);
    }

    // ---- gold landmark medallion slots ----
    auto medallion = [&](float cx, float cy) {
        DrawRect({ cx - 17, cy - 17 }, { cx + 17, cy + 17 }, WithAlpha(RGBA(40, 30, 8, 255), 0.8f));
        DrawRect({ cx - 15, cy - 15 }, { cx + 15, cy + 15 }, C_GOLD);
        DrawRect({ cx - 10, cy - 10 }, { cx + 10, cy + 10 }, RGBA(150, 116, 36, 255));
    };
    medallion(404, 432); medallion(700, 330); medallion(914, 348);

    // ---- photo inset slot (real town snapshot drops in) ----
    DrawRect({ 270, 98 }, { 450, 202 }, RGBA(230, 232, 230, 255));
    DrawVGradient({ 274, 102 }, { 446, 198 }, RGBA(70, 86, 104, 255), RGBA(36, 46, 58, 255));
    SetFont(0);
    DrawTextAligned({ 274, 102 }, { 446, 198 }, 12.0f, RGBA(130, 144, 158, 255), "PHOTO", Align::Center, true, false);

    // ---- NOW LOADING wordmark (real sprite) + spinner, pulsing like boot ----
    float t01 = (float)(now - std::floor(now));
    float tri = 0.5f - 0.5f * std::cos(6.2831853f * t01);
    float pulse = 0.40f + 0.60f * std::sqrt(tri);
    if (g_wordTex >= 0)
        DrawImageVGradient(g_wordTex, { 700, 553 }, { 947, 592 }, { WM_U0, WM_V0 }, { WM_U1, WM_V1 },
                           WithAlpha(RGBA(76, 218, 16, 255), pulse), WithAlpha(C_GREEN_B, pulse));
    DrawSpinner(962, 556, now);
}

} // namespace

void LoadingInit() { Init(); }
void LoadingDraw(double openSeconds) { Draw(openSeconds); }
void LoadingInput(const ScreenInput& in) { Input(in); }
void LoadingReset() { Reset(); }
