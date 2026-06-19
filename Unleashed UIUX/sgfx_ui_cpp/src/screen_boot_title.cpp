// =============================================================================
// screen_boot_title.cpp — the boot title screen. Two states:
//   * SPLASH: a starfield, a centred LOGO slot (host-supplied; with no asset the
//     slot stays empty — a clean field, no placeholder text), and a glowing
//     metallic capsule prompt to continue;
//   * MENU: the planet rises behind the logo (our own 3D sphere), olive scanline
//     letterbox bands with pale-green edge lines, and the HORIZONTAL CAROUSEL:
//     one centred entry on a green scanline selector bar, flanked by arrows.
// Accept advances splash -> menu; left/right cycles entries. The presenter
// wordmark and the on-screen notices were removed so the screen reads as the
// host's own and ships without third-party art.
// =============================================================================
#include "sgfxui.h"
#include "globe3d.h"
#include "screen.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_logoTex = -1;   // host-supplied logo; -1 = empty slot (clean starfield)
int g_fRodin = 0, g_fDF = 0;

// ---- palette (measured) -------------------------------------------------------
const uint32_t C_STAR_G   = RGBA(233, 255, 246, 255);
const uint32_t C_BAND_EDGE= RGBA(123, 156, 113, 255);   // letterbox edge line
const uint32_t C_BAND     = RGBA(44, 54, 15, 200);      // scanline band olive
const uint32_t C_SEL_BAR  = RGBA(10, 140, 30, 230);     // carousel selector
const uint32_t C_ENTRY    = RGBA(167, 211, 29, 255);    // entry lime fill
const uint32_t C_ENTRY_OUT= RGBA(28, 55, 12, 255);
const uint32_t C_ARROW    = RGBA(16, 130, 24, 255);
// Earth placeholder
const uint32_t C_OCEAN_T = RGBA(78, 121, 186, 255), C_OCEAN_B = RGBA(18, 34, 66, 255);
const uint32_t C_LAND    = RGBA(103, 129, 111, 235), C_CLOUD = RGBA(218, 232, 233, 160);

enum BootState { BT_PRESS_START = 0, BT_MENU };
int g_state = BT_PRESS_START;
int g_entry = 0;
const char* g_nav = nullptr;
const char* const ENTRIES[] = { "RESUME", "NEW RUN", "SETTINGS" };
constexpr int N_ENTRIES = 3;

void Init() {
    if (g_logoTex < 0) g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
    if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db");
    if (g_fDF    == 0) g_fDF    = LoadMsdfFont("dfsogei");
}
void Reset() { g_state = BT_PRESS_START; g_entry = 0; g_nav = nullptr; }
void Input(const ScreenInput& in) {
    if (in.accept) {
        if (g_state < BT_MENU) {
            g_state = BT_MENU;        // splash -> menu (no console-storage notice)
        } else {   // carousel accept -> the runtime flow
            if (g_entry == 0)      g_nav = "world_map";   // RESUME  -> the hub
            else if (g_entry == 1) g_nav = "title";       // NEW RUN -> the launcher menu
            else                   g_nav = "options";     // SETTINGS
        }
    }
    if (in.cancel) g_state = BT_PRESS_START;
    if (g_state == BT_MENU) {
        if (in.left)  g_entry = (g_entry + N_ENTRIES - 1) % N_ENTRIES;
        if (in.right) g_entry = (g_entry + 1) % N_ENTRIES;
    }
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

void Starfield(float a) {
    DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 255));
    uint32_t s = 0x51f15eedu;
    for (int i = 0; i < 130; ++i) {
        s = s*1664525u+1013904223u; float x = (float)((s >> 9) % 1280);
        s = s*1664525u+1013904223u; float y = (float)((s >> 9) % 720);
        s = s*1664525u+1013904223u; int b = 40 + (int)((s >> 9) % 200);
        uint32_t col = (b > 180) ? C_STAR_G : RGBA(b, b + 4, b, 255);
        float sz = (b > 210) ? 2.0f : 1.0f;
        DrawRect({ x, y }, { x + sz, y + sz }, WithAlpha(col, a));
    }
}

// the centred logo slot. The host's own logo drops in here; with no asset the
// slot stays empty — a clean starfield, no placeholder or debug text.
void LogoSlot(float a) {
    if (g_logoTex < 0 || a <= 0.0f) return;   // empty = clean starfield, no placeholder
    const float lw = 420.0f, lh = lw * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 640 - lw * 0.5f, 250 - lh * 0.5f }, { 640 + lw * 0.5f, 250 + lh * 0.5f },
              { 0.f, 0.f }, { 1.f, 1.f }, WithAlpha(RGBA(255, 255, 255, 255), a));
}

// a horizontal CAPSULE (semicircular ends) with a vertical gradient, drawn as
// row strips so the rounded ends and the gradient come for free.
void CapsuleVGrad(float x0, float y0, float x1, float y1, uint32_t cTop, uint32_t cBot, float a, bool additive = false) {
    const float r = (y1 - y0) * 0.5f, cy = (y0 + y1) * 0.5f;
    const float lc = x0 + r, rc = x1 - r;     // end-cap centres
    const int N = 26;
    for (int i = 0; i < N; ++i) {
        float sy0 = y0 + (y1 - y0) * i / N, sy1 = y0 + (y1 - y0) * (i + 1) / N;
        float dy = (std::max(std::fabs(sy0 - cy), std::fabs(sy1 - cy))) / r;
        float inset = (dy >= 1.0f) ? r : r - r * std::sqrt(std::max(0.0f, 1.0f - dy * dy));
        float f0 = (sy0 - y0) / (y1 - y0), f1 = (sy1 - y0) / (y1 - y0);
        uint32_t c0 = WithAlpha(ColourLerp(cTop, cBot, f0), a);
        uint32_t c1 = WithAlpha(ColourLerp(cTop, cBot, f1), a);
        const V2 q[4] = { { lc - r + inset, sy0 }, { rc + r - inset, sy0 }, { rc + r - inset, sy1 }, { lc - r + inset, sy1 } };
        const uint32_t qc[4] = { c0, c0, c1, c1 };
        DrawQuadGradient(q, qc, additive);
    }
}

// the splash capsule button: a clean glowing gold/white prompt that gently pulses.
void PressStart(double now, float a) {
    const float pulse = Breathe(now, 0.35f, 0.65f, 1.0f);   // 0..1 soft breathe
    // warm GOLD glow capsule (additive, concentric, soft) — follows the pill,
    // no heavy bloom, no rectangular backing
    const uint32_t GOLD_GLOW = RGBA(255, 214, 96, 255);
    CapsuleVGrad(478, 484, 804, 560, GOLD_GLOW, GOLD_GLOW, a * pulse * 0.08f, true);
    CapsuleVGrad(486, 490, 796, 553, GOLD_GLOW, GOLD_GLOW, a * pulse * 0.12f, true);
    // thin metallic gold rim capsule + a slim translucent dark inner well so the
    // text reads — no flat lime fill, no chunky olive ring
    const uint32_t RIM_HI  = RGBA(255, 232, 150, 255);
    const uint32_t RIM_LO  = RGBA(196, 158, 60, 255);
    CapsuleVGrad(498.0f, 496.0f, 782.0f, 546.0f, RIM_HI, RIM_LO, a * 0.85f);   // gold rim
    CapsuleVGrad(503.0f, 500.0f, 777.0f, 542.0f, RGBA(18, 18, 22, 255), RGBA(8, 8, 10, 255), a * 0.55f); // inner well
    // the prompt caps: clean white-gold with a single soft dark shadow,
    // brightness gently following the breathe.
    SetFont(g_fRodin);
    SetTextStretchX(1.35f);
    const char* PS = "ENTER";
    const float fz = 26.0f;
    float w = MeasureText(fz, PS).x * 1.35f;
    float px = 640 - w * 0.5f, py = 510.0f;
    // soft drop shadow
    DrawText({ px + 1.4f, py + 1.6f }, fz, WithAlpha(RGBA(0, 0, 0, 150), a), PS);
    // warm gold under-glow tint then crisp near-white top — the pulse lifts it
    int top = 232 + (int)(23.0f * pulse);   // 232..255
    DrawText({ px, py }, fz, WithAlpha(RGBA(255, 224, 150, 255), a * 0.5f), PS);   // gold halo
    DrawText({ px, py }, fz, WithAlpha(RGBA(top, top, 240, 255), a), PS);          // white-gold face
    ResetTextStretchX();
    ResetFont();
}

void Earth(float a) {
    // real 3D sphere (measured disc: centre (652,371.3), r 186.7), lit from the
    // lower-right exactly as the capture shows, drifting slowly
    DrawGlobe3D(652, 371.3f, 186.7f, (float)(Now() * 3.0), 0.6f, -0.35f, 0.72f, nullptr, 0, a);
}

void LetterboxBands(float a) {
    SetModifier(MOD_SCANLINE);
    DrawVGradient({ 0, 0 }, { REF_W, 104.7f }, WithAlpha(C_BAND, a * 0.6f), WithAlpha(C_BAND, a));
    DrawVGradient({ 0, 616 }, { REF_W, REF_H }, WithAlpha(C_BAND, a), WithAlpha(C_BAND, a * 0.2f));
    ResetModifier();
    DrawRect({ 0, 103.3f }, { REF_W, 106.0f }, WithAlpha(C_BAND_EDGE, a));
    DrawRect({ 0, 613.3f }, { REF_W, 615.3f }, WithAlpha(C_BAND_EDGE, a));
}

void Carousel(float a) {
    // green scanline selector bar
    SetModifier(MOD_SCANLINE);
    DrawRect({ 544, 489.3f }, { 735.3f, 513.3f }, WithAlpha(C_SEL_BAR, a));
    ResetModifier();
    // entry text: outlined lime caps, centred
    SetFont(g_fRodin);
    const char* E = ENTRIES[g_entry];
    float w = MeasureText(19.0f, E).x;
    float ex = 640 - w * 0.5f, ey = 496.0f;   // cy ~506 with 19px caps (caph~15)
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ ex + dx * 1.2f, ey + dy * 1.2f }, 19.0f, WithAlpha(C_ENTRY_OUT, a), E);
    DrawText({ ex, ey }, 19.0f, WithAlpha(C_ENTRY, a), E);
    ResetFont();
    // arrows (solid green triangles) + outboard fading wedge bars
    const V2 lt[4] = { { 528.7f, 490.7f }, { 528.7f, 512.7f }, { 510.7f, 501.3f }, { 528.7f, 490.7f } };
    const V2 rt[4] = { { 750.7f, 490.7f }, { 750.7f, 512.7f }, { 770.0f, 501.7f }, { 750.7f, 490.7f } };
    const uint32_t ac[4] = { WithAlpha(C_ARROW, a), WithAlpha(C_ARROW, a), WithAlpha(C_ARROW, a), WithAlpha(C_ARROW, a) };
    DrawQuadGradient(lt, ac); DrawQuadGradient(rt, ac);
    {
        const V2 lw[4] = { { 467.0f, 489.3f }, { 498.7f, 489.3f }, { 498.7f, 513.3f }, { 467.0f, 513.3f } };
        const uint32_t lc[4] = { WithAlpha(C_ARROW, 0.0f), WithAlpha(C_ARROW, a * 0.7f), WithAlpha(C_ARROW, a * 0.7f), WithAlpha(C_ARROW, 0.0f) };
        DrawQuadGradient(lw, lc);
        const V2 rw[4] = { { 781.3f, 489.3f }, { 885.0f, 489.3f }, { 885.0f, 513.3f }, { 781.3f, 513.3f } };
        const uint32_t rc[4] = { WithAlpha(C_ARROW, a * 0.7f), WithAlpha(C_ARROW, 0.0f), WithAlpha(C_ARROW, 0.0f), WithAlpha(C_ARROW, a * 0.7f) };
        DrawQuadGradient(rw, rc);
    }
}

void Draw(double openSec) {
    const double now = Now();
    const float a = (float)ComputeMotion(openSec, 0.0, 12.0);
    Starfield(a);

    if (g_state == BT_MENU) {
        Earth(a);
        LetterboxBands(a);
        Carousel(a);
        return;
    }

    LogoSlot(a);
    PressStart(now, a);
}

} // namespace

void BootTitleInit() { Init(); }
void BootTitleDraw(double openSeconds) { Draw(openSeconds); }
void BootTitleInput(const ScreenInput& in) { Input(in); }
void BootTitleReset() { Reset(); }
const char* BootTitleNav() { return Nav(); }
