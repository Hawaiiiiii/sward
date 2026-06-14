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
constexpr float SC_X0 = 179, SC_Y0 = 107, SC_X1 = 1101, SC_Y1 = 661;

void Init() { if (g_wordTex < 0) g_wordTex = gfx::loadTexture("assets/loading/mat_load_en_001.png"); }
void Reset() {}
void Input(const ScreenInput&) {}

// filled circle via horizontal strips (sgfxui has no circle primitive)
void FillDisc(float cx, float cy, float r, uint32_t col) {
    const int N = 16;
    for (int i = 0; i < N; ++i) {
        float y0 = cy - r + (2.0f * r) * i / N;
        float y1 = cy - r + (2.0f * r) * (i + 1) / N;
        float ym = (y0 + y1) * 0.5f - cy;
        float hw = std::sqrt(std::max(0.0f, r * r - ym * ym));
        DrawRect({ cx - hw, y0 }, { cx + hw, y1 }, col);
    }
}

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
    // round recessed base (the real Miles-Electric pads are circular clusters)
    FillDisc(cx, cy, 42, C_SHELL_DK);
    FillDisc(cx, cy, 38, RGBA(206, 172, 46, 255));
    // four directional wedges (trapezoid pads) separated by diagonal gaps, with a
    // raised central hub disc — the real pad reads as 4 keys around a hub, not a '+'
    const float hub = 9.0f, rOut = 32.0f;
    const float halfIn = 7.0f, halfOut = 22.0f;   // gap-defining half-widths (in / out)
    auto wedge = [&](float ax, float ay) {
        // (ax,ay) is the unit outward direction (axis-aligned); perp is the side axis
        float px = -ay, py = ax;
        const V2 q[4] = {
            { cx + ax * hub  - px * halfIn,  cy + ay * hub  - py * halfIn  },
            { cx + ax * hub  + px * halfIn,  cy + ay * hub  + py * halfIn  },
            { cx + ax * rOut + px * halfOut, cy + ay * rOut + py * halfOut },
            { cx + ax * rOut - px * halfOut, cy + ay * rOut - py * halfOut },
        };
        const uint32_t qc[4] = { RGBA(228, 192, 56, 255), RGBA(228, 192, 56, 255),
                                 C_SHELL_DK, C_SHELL_DK };
        DrawQuadGradient(q, qc);
    };
    wedge(0, -1); wedge(1, 0); wedge(0, 1); wedge(-1, 0);
    FillDisc(cx, cy, hub + 1, C_SHELL_DK);          // hub recess ring
    FillDisc(cx, cy, hub - 1, RGBA(228, 192, 56, 255)); // raised central hub
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
    FillDisc(640, 56, 22, C_BEZEL);                                   // round camera lens
    FillDisc(640, 56, 16, RGBA(150, 120, 36, 255));                   // gold ring
    FillDisc(640, 56, 10, RGBA(40, 46, 52, 255));                     // dark glass
    FillDisc(636, 52, 3.2f, RGBA(180, 200, 210, 255));               // catch-light
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
    DrawText({ SC_X1 - 96, SC_Y1 + 1 }, 10.0f, RGBA(150, 152, 148, 255), "MILES ELECTRIC");

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

    // ---- green terrain (translucent undulating valley) + solid landmark blocks ----
    // An open, asymmetric valley of grid-conforming quads receding toward horizonY
    // (NOT a closed racetrack ring). Translucent green near->far, centroid ~(632,450).
    {
        const float horizonY = 300, vanishX = 640;   // matches the perspective grid
        const float nearY = 568;                      // near edge of the valley floor
        // depth f in [0,1]: 0 = near (bottom), 1 = far (horizon). Quadratic ease
        // mirrors the grid's row spacing so the terrain conforms to the floor lines.
        auto rowY = [&](float f) { return nearY + (horizonY - nearY) * f * f; };
        // valley half-width shrinks toward the horizon (perspective foreshortening),
        // its center drifts left of the vanishing point near the camera so the whole
        // mass biases toward the measured centroid (~632,450).
        auto edges = [&](float f, float& xl, float& xr) {
            float pull = 1.0f - f;                              // 1 near -> 0 far
            float cxF  = vanishX + (-30.0f) * pull;             // center drifts left near
            float halfW = 22.0f + 230.0f * pull * pull;         // wide near, pinched far
            // undulating banks: each side waves independently so it reads organic
            float wob = halfW * 0.18f;
            xl = cxF - halfW - wob * std::sin(f * 7.4f + 0.6f);
            xr = cxF + halfW + wob * std::sin(f * 6.1f + 2.1f);
        };
        const int NB = 7;
        for (int b = 0; b < NB; ++b) {
            float f0 = (float)b / NB, f1 = (float)(b + 1) / NB;
            float y0 = rowY(f0), y1 = rowY(f1);
            float l0, r0, l1, r1; edges(f0, l0, r0); edges(f1, l1, r1);
            // brighter / more opaque near the camera, fading toward the horizon
            float a0 = 0.22f * (1.0f - f0) + 0.05f;
            float a1 = 0.22f * (1.0f - f1) + 0.05f;
            const V2 q[4] = { { l1, y1 }, { r1, y1 }, { r0, y0 }, { l0, y0 } };
            const uint32_t qc[4] = { WithAlpha(C_WIRE, a1), WithAlpha(C_WIRE, a1),
                                     WithAlpha(C_WIRE, a0), WithAlpha(C_WIRE, a0) };
            DrawQuadGradient(q, qc, true);
        }
        // solid bright-green landmark blocks rising out of the terrain.
        // (base x, base y on the floor, width, height) — drawn as a lit front face
        // plus a darker side sliver for a hint of volume.
        auto block = [&](float bx, float by, float w, float h) {
            DrawRect({ bx, by - h }, { bx + w, by }, C_WIRE);                 // front face
            DrawRect({ bx + w, by - h + 6 }, { bx + w + 6, by }, C_WIRE_DIM); // side sliver
            DrawRect({ bx, by - h }, { bx + w, by - h + 3 }, WithAlpha(RGBA(180,255,190,255), 0.9f)); // lit top edge
        };
        block(748, 472, 26, 84);   // tower near ~(760,460)
        block(556, 506, 30, 40);   // low building, left of centre
        // windmill hint: a slim mast block + two crossed sail bars
        float mx = 868, my = 470;
        DrawRect({ mx, my - 56 }, { mx + 10, my }, C_WIRE);                   // mast
        DrawRect({ mx - 26, my - 56 }, { mx + 36, my - 52 }, C_WIRE);        // sail bar (horizontal)
        DrawRect({ mx + 3, my - 84 }, { mx + 7, my - 28 }, C_WIRE);          // sail bar (vertical)
    }

    // ---- gold landmark medallion slots (round, like the real map markers) ----
    auto medallion = [&](float cx, float cy) {
        FillDisc(cx, cy, 29, WithAlpha(RGBA(40, 30, 8, 255), 0.8f));
        FillDisc(cx, cy, 24, C_GOLD);
        FillDisc(cx, cy, 14, RGBA(150, 116, 36, 255));
    };
    medallion(329, 450); medallion(535, 322); medallion(914, 348);

    // ---- photo inset slot (real town snapshot drops in) ----
    // bright-green wire border framing the inset (was a solid white frame)
    DrawRect({ 366, 145 }, { 542, 276 }, C_WIRE);
    DrawVGradient({ 369, 148 }, { 539, 273 }, RGBA(70, 86, 104, 255), RGBA(36, 46, 58, 255));
    SetFont(0);
    DrawTextAligned({ 369, 148 }, { 539, 273 }, 12.0f, RGBA(130, 144, 158, 255), "PHOTO", Align::Center, true, false);

    // ---- NOW LOADING wordmark (real sprite) + spinner, pulsing like boot ----
    float t01 = (float)(now - std::floor(now));
    float tri = 0.5f - 0.5f * std::cos(6.2831853f * t01);
    float pulse = 0.40f + 0.60f * std::sqrt(tri);
    if (g_wordTex >= 0)
        DrawImageVGradient(g_wordTex, { 593, 547 }, { 963, 598 }, { WM_U0, WM_V0 }, { WM_U1, WM_V1 },
                           WithAlpha(RGBA(76, 218, 16, 255), pulse), WithAlpha(C_GREEN_B, pulse));
    DrawSpinner(985, 558, now);
}

} // namespace

void LoadingInit() { Init(); }
void LoadingDraw(double openSeconds) { Draw(openSeconds); }
void LoadingInput(const ScreenInput& in) { Input(in); }
void LoadingReset() { Reset(); }
