// =============================================================================
// screen_boot_logos.cpp — the cold-boot splash. The original sequence's cadence is
// kept, but the content is brand-neutral / host-owned:
//   ~0.0-1.0   soft flare open
//   ~1.0-3.5   centred LOGO slot (host-supplied assets/gameart/boot_logo.png; with
//              no asset the slot stays empty — a clean field, the boot_title
//              LogoSlot convention, no placeholder text or debug string)
//   ~7.0-9.5   the rotating planet (our own 3D sphere — the world-map hub motif)
//   ~9.75-12.6 a neutral "Loading" line + a drawn dot-ring spinner
// In flow mode the screen auto-advances to boot_title when the timeline ends.
// No third-party wordmarks/logos are drawn: the two presenter cards and the 'S'
// splash were removed so the boot reads as the host's own and ships without them.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "globe3d.h"
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fRodin = 0;
int g_logoTex = -1;   // host-supplied boot logo; -1 = empty slot (clean field)
const char* g_nav = nullptr;
double g_lastOpen = 0.0;

void Init() {
    if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db");
    if (g_logoTex < 0) g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
}
void Reset() { g_nav = nullptr; g_lastOpen = 0.0; }
void Input(const ScreenInput& in) {
    if (in.accept) g_nav = "boot_title";   // start skips the splash, like a normal boot
}

// a fade window: 0 before in, ramps over 0.35s, holds, ramps out
float Window(double t, double t0, double t1) {
    if (t < t0 || t > t1) return 0.0f;
    float a = (float)std::min(1.0, (t - t0) / 0.35);
    float b = (float)std::min(1.0, (t1 - t) / 0.35);
    return std::min(a, b);
}

// the centred logo slot. The host's own boot logo drops in here; with no asset the
// slot stays empty (clean field) — no placeholder, no debug text.
void LogoBeat(float a) {
    if (g_logoTex < 0) return;
    const float lw = 420.0f, lh = lw * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 640 - lw * 0.5f, 360 - lh * 0.5f }, { 640 + lw * 0.5f, 360 + lh * 0.5f },
              { 0.f, 0.f }, { 1.f, 1.f }, WithAlpha(RGBA(255, 255, 255, 255), a));
}

void Draw(double openSec) {
    g_lastOpen = openSec;
    const double t = Now();   // elapsed-since-open; the splash is a one-shot timeline

    if (t < 9.75) {
        DrawVGradient({ 0, 0 }, { REF_W, REF_H }, RGBA(8, 10, 24, 255), RGBA(4, 5, 14, 255));

        if (float a = Window(t, 0.0, 1.0); a > 0)   // soft neutral flare open
            DrawRect({ 560, 300 }, { 720, 420 }, WithAlpha(RGBA(150, 180, 150, 255), a * 0.30f), true);
        if (float a = Window(t, 1.0, 3.5); a > 0)   // host logo slot (empty = clean field)
            LogoBeat(a);
        if (float a = Window(t, 7.0, 9.5); a > 0) {   // the rotating planet (our own 3D
            // sphere — the world-map hub motif): a dark globe gently turning, lit back-left
            DrawGlobe3D(640.0f, 360.0f, 96.0f, (float)(t * 46.0),
                        -0.5f, 0.32f, -0.62f, nullptr, 0, a);
        }
    } else {
        // black + a neutral Loading line and a drawn dot-ring spinner (no game atlas)
        DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 255));
        float t01 = (float)(t - std::floor(t));
        float tri = 0.5f - 0.5f * std::cos(6.2831853f * t01);
        float pulse = 0.40f + 0.60f * std::sqrt(tri);
        float a = Window(t, 9.75, 12.6);
        SetFont(g_fRodin);
        const char* MSG = "Loading";
        float w = MeasureText(24.0f, MSG).x;
        DrawText({ 1011.0f - w, 596 }, 24.0f, WithAlpha(RGBA(120, 210, 90, 255), pulse * a), MSG);
        ResetFont();
        // dot-ring spinner just right of the word: 8 cells, a lit head sweeping
        static const int RING[8][2] = { {0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1} };
        int head = (int)(t * 10.0) % 8;
        for (int i = 0; i < 8; ++i) {
            int d = (head - i + 8) % 8;
            float k = (d == 0) ? 1.0f : (d == 1 ? 0.86f : (d == 2 ? 0.76f : 0.55f));
            uint32_t c = WithAlpha(ColourLerp(RGBA(28, 92, 8, 255), RGBA(120, 210, 90, 255), k), a);
            float gx = 1023.0f + RING[i][0] * 11.5f, gy = 593.0f + RING[i][1] * 11.5f;
            DrawRect({ gx, gy }, { gx + 9, gy + 9 }, c, true);
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
