// =============================================================================
// screen_boot_logos.cpp — the cold-boot logo sequence, timed 1:1 from live
// capture session4 (0..12s):
//   ~0.0-1.0   'S' lens-flare splash (art slot)
//   ~1.0-3.5   "PRESENTED BY" + SEGA logo card (dark navy; logo = trademark slot)
//   ~4.0-6.5   SONIC TEAM logo card (trademark slot)
//   ~7.0-9.5   white screen + the dark morph ball (art slot, gentle spin)
//   ~10.0-12.5 black + the real NOW LOADING wordmark (mat_load_en_001) + spinner
// In flow mode the screen auto-advances to boot_title when the timeline ends.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "globe3d.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fRodin = 0, g_fDF = 0;
int g_wordTex = -1, g_segaTex = -1, g_stTex = -1;
const char* g_nav = nullptr;
double g_lastOpen = 0.0;

const float WM_U0 = 0.3750f, WM_V0 = 0.0078f, WM_U1 = 0.9961f, WM_V1 = 0.2812f;
const float SQ_U0 = 0.9102f, SQ_V0 = 0.3125f, SQ_U1 = 0.9805f, SQ_V1 = 0.5859f;

void Init() {
    if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db");
    if (g_fDF    == 0) g_fDF    = LoadMsdfFont("dfsogei");
    if (g_wordTex < 0) g_wordTex = gfx::loadTexture("assets/loading/mat_load_en_001.png");
    if (g_segaTex < 0) g_segaTex = gfx::loadTexture("assets/gameart/boot_logo_sega.png");        // real SEGA card logo
    if (g_stTex   < 0) g_stTex   = gfx::loadTexture("assets/gameart/boot_logo_sonicteam.png");   // real SONIC TEAM card logo
}
void Reset() { g_nav = nullptr; g_lastOpen = 0.0; }
void Input(const ScreenInput& in) {
    if (in.accept) g_nav = "boot_title";   // start button skips the logos, like retail
}

// a fade window: 0 before in, ramps over 0.35s, holds, ramps out
float Window(double t, double t0, double t1) {
    if (t < t0 || t > t1) return 0.0f;
    float a = (float)std::min(1.0, (t - t0) / 0.35);
    float b = (float)std::min(1.0, (t1 - t) / 0.35);
    return std::min(a, b);
}

void CardSlot(const char* small, const char* big, float a, uint32_t bigCol, int tex) {
    SetFont(g_fRodin);
    if (small && small[0]) {
        float w = MeasureText(16.0f, small).x;
        DrawText({ 640 - w * 0.5f, 290 }, 16.0f, WithAlpha(RGBA(208, 210, 216, 255), a), small);
    }
    if (tex >= 0) {   // the real card logo art (600x200, black keyed to transparent)
        const float lw = 360.0f, lh = lw * 200.0f / 600.0f;
        DrawImage(tex, { 640 - lw * 0.5f, 365 - lh * 0.5f }, { 640 + lw * 0.5f, 365 + lh * 0.5f },
                  { 0.f, 0.f }, { 1.f, 1.f }, WithAlpha(RGBA(255, 255, 255, 255), a));
        ResetFont();
        return;
    }
    SetFont(g_fDF);   // fallback: stylised wordmark text + the "drop art here" hint
    SetTextStretchX(1.15f);
    float w = MeasureText(64.0f, big).x * 1.15f;
    DrawTextGradient({ 640 - w * 0.5f, 330 }, 64.0f, WithAlpha(RGBA(240, 242, 248, 255), a), WithAlpha(bigCol, a), big);
    ResetTextStretchX();
    SetFont(g_fRodin);
    DrawTextAligned({ 0, 410 }, { REF_W, 430 }, 13.0f, WithAlpha(RGBA(110, 112, 120, 255), a * 0.8f),
                    "( logo trademark slot - drop the real card art in )", Align::Center, true, false);
    ResetFont();
}

void Draw(double openSec) {
    g_lastOpen = openSec;
    const double t = Now();   // ui::Now() is elapsed-since-screen-open (host resets the clock); the
                              // boot sequence is a one-shot timeline. (Was `= openSec`, i.e. 0, so it
                              // stuck on frame 0 and never advanced past the 'S' splash.)

    if (t < 9.75) {
        // dark navy stage for the logo cards; white stage for the morph
        if (t < 6.75) DrawVGradient({ 0, 0 }, { REF_W, REF_H }, RGBA(8, 10, 24, 255), RGBA(4, 5, 14, 255));
        else          DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(244, 246, 248, 255));

        if (float a = Window(t, 0.0, 1.0); a > 0) {   // 'S' splash slot: a soft flare burst
            DrawRect({ 560, 300 }, { 720, 420 }, WithAlpha(RGBA(190, 205, 235, 255), a * 0.35f), true);
            SetFont(g_fDF);
            DrawTextAligned({ 0, 320 }, { REF_W, 400 }, 76.0f, WithAlpha(RGBA(235, 240, 250, 255), a), "S", Align::Center, true, false);
            ResetFont();
        }
        if (float a = Window(t, 1.0, 3.5); a > 0) CardSlot("P R E S E N T E D   B Y", "SEGA", a, RGBA(70, 110, 220, 255), g_segaTex);
        if (float a = Window(t, 4.0, 6.5); a > 0) CardSlot(nullptr, "SONIC TEAM", a, RGBA(50, 90, 200, 255), g_stTex);
        if (float a = Window(t, 7.0, 9.5); a > 0) {   // the morph orb — a REAL 3D sphere
            // (the "World Adventure" planet motif). The capture shows a dark ball gently
            // turning on white; render the globe with the sun pushed behind the camera
            // face so the near hemisphere sits in shadow (eclipse look, a faint lit
            // crescent on the limb), spinning ~46 deg/s. Replaces the old 2D spiky stub.
            DrawGlobe3D(640.0f, 360.0f, 96.0f, (float)(t * 46.0),
                        -0.5f, 0.32f, -0.62f,    // back-left sun -> dark facing hemisphere
                        nullptr, 0, a);
        }
    } else {
        // black + the real NOW LOADING (same treatment as boot_loading)
        DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 255));
        float t01 = (float)(t - std::floor(t));
        float tri = 0.5f - 0.5f * std::cos(6.2831853f * t01);
        float pulse = 0.40f + 0.60f * std::sqrt(tri);
        float a = Window(t, 9.75, 12.6);
        if (g_wordTex >= 0)
            DrawImageVGradient(g_wordTex, { 651, 592 }, { 1005, 628 }, { WM_U0, WM_V0 }, { WM_U1, WM_V1 },
                               WithAlpha(RGBA(76, 218, 16, 255), pulse * a), WithAlpha(RGBA(50, 147, 10, 255), pulse * a));
        static const int RING[8][2] = { {0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1} };
        int head = (int)(t * 10.0) % 8;
        for (int i = 0; i < 8; ++i) {
            int d = (head - i + 8) % 8;
            float k = (d == 0) ? 1.0f : (d == 1 ? 0.86f : (d == 2 ? 0.76f : 0.68f));
            uint32_t c = WithAlpha(ColourLerp(RGBA(28, 92, 8, 255), RGBA(74, 230, 18, 255), k), a);
            float gx = 1023.0f + RING[i][0] * 11.5f, gy = 593.0f + RING[i][1] * 11.5f;
            if (g_wordTex >= 0)
                DrawImage(g_wordTex, { gx, gy }, { gx + 9, gy + 9 }, { SQ_U0, SQ_V0 }, { SQ_U1, SQ_V1 }, c);
        }
        if (t > 12.6) g_nav = "boot_title";   // timeline complete -> title (flow mode)
    }
}

const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

} // namespace

void BootLogosInit() { Init(); }
void BootLogosDraw(double openSeconds) { Draw(openSeconds); }
void BootLogosInput(const ScreenInput& in) { Input(in); }
void BootLogosReset() { Reset(); }
const char* BootLogosNav() { return Nav(); }
