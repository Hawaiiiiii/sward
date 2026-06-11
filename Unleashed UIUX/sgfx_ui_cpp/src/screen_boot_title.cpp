// =============================================================================
// screen_boot_title.cpp — the FIRST-BOOT title screen, measured 1:1 from LIVE
// capture session4 (spec: game_captures/BOOT_TITLE_SPEC.md). Three states:
//   * PRESS START: black starfield, the game-logo SLOT at (326,213)-(907,430)
//     (the SONIC UNLEASHED wordmark is SEGA trademark art -> the slot is where
//     the user's own logo/SGFX branding drops in), and the measured metallic
//     capsule BUTTON: grey-olive ring, glossy yellow fill, animated green glow,
//     outline-style "PRESS START" caps;
//   * AUTOSAVE NOTICE: 75% scene dim + a translucent grey TL/BR-chamfered
//     dialog, 8 centred outlined lines (pitch 34.3) with the green disc icon,
//     (A) Next prompt kept bright;
//   * MENU: the Earth rises behind the logo (3D pass; 2D disc placeholder),
//     olive scanline letterbox bands with pale-green edge lines, and the
//     HORIZONTAL CAROUSEL: one centred entry on a green scanline selector bar,
//     flanked by solid green arrows and outboard fading wedge bars.
// Accept advances press_start -> autosave -> menu; left/right cycles entries.
// =============================================================================
#include "sgfxui.h"
#include "globe3d.h"
#include "screen.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_glyphTex = -1;
int g_fRodin = 0, g_fDF = 0;

struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };

// ---- palette (measured) -------------------------------------------------------
const uint32_t C_STAR_G   = RGBA(233, 255, 246, 255);
const uint32_t C_RING     = RGBA(90, 94, 81, 255);      // capsule ring grey-olive
const uint32_t C_RING_HI  = RGBA(155, 159, 131, 255);
const uint32_t C_CAP_TOP  = RGBA(150, 143, 48, 255);    // capsule yellow (top dark)
const uint32_t C_CAP_PEAK = RGBA(237, 225, 75, 255);    // glossy belly
const uint32_t C_GLOW     = RGBA(99, 124, 42, 255);     // green glow
const uint32_t C_PS_OUT   = RGBA(60, 66, 22, 255);      // PRESS START outline
const uint32_t C_WHITE    = RGBA(237, 237, 237, 255);
const uint32_t C_DLG_T    = RGBA(170, 170, 170, 217);   // autosave dialog fill
const uint32_t C_DLG_B    = RGBA(125, 125, 125, 217);
const uint32_t C_DLG_BD   = RGBA(200, 202, 200, 255);
const uint32_t C_ICON_GRN = RGBA(40, 188, 36, 255);
const uint32_t C_BAND_EDGE= RGBA(123, 156, 113, 255);   // letterbox edge line
const uint32_t C_BAND     = RGBA(44, 54, 15, 200);      // scanline band olive
const uint32_t C_SEL_BAR  = RGBA(0, 115, 25, 190);      // carousel selector
const uint32_t C_ENTRY    = RGBA(167, 211, 29, 255);    // entry lime fill
const uint32_t C_ENTRY_OUT= RGBA(28, 55, 12, 255);
const uint32_t C_ARROW    = RGBA(16, 130, 24, 255);
// Earth placeholder
const uint32_t C_OCEAN_T = RGBA(78, 121, 186, 255), C_OCEAN_B = RGBA(18, 34, 66, 255);
const uint32_t C_LAND    = RGBA(103, 129, 111, 235), C_CLOUD = RGBA(218, 232, 233, 160);

enum BootState { BT_PRESS_START = 0, BT_AUTOSAVE, BT_MENU };
int g_state = BT_PRESS_START;
int g_entry = 0;
const char* g_nav = nullptr;
const char* const ENTRIES[] = { "CONTINUE", "NEW GAME", "OPTIONS" };
constexpr int N_ENTRIES = 3;

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db");
    if (g_fDF    == 0) g_fDF    = LoadFont("assets/fonts/dfsoge7.ttc");
}
void Reset() { g_state = BT_PRESS_START; g_entry = 0; g_nav = nullptr; }
void Input(const ScreenInput& in) {
    if (in.accept) {
        if (g_state < BT_MENU) {
            g_state++;
        } else {   // carousel accept -> the runtime flow
            if (g_entry == 2) g_nav = "options";
            else              g_nav = "world_map";   // CONTINUE / NEW GAME
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

// the game-logo SLOT (trademark art -> user branding drops in here)
void LogoSlot(float a) {
    DrawRect({ 326, 213.3f }, { 906.7f, 430 }, WithAlpha(RGBA(20, 20, 22, 255), a * 0.6f));
    uint32_t bd = WithAlpha(RGBA(90, 90, 94, 255), a);
    DrawRect({ 326, 213.3f }, { 906.7f, 215.3f }, bd);
    DrawRect({ 326, 428 }, { 906.7f, 430 }, bd);
    DrawRect({ 326, 213.3f }, { 328, 430 }, bd);
    DrawRect({ 904.7f, 213.3f }, { 906.7f, 430 }, bd);
    SetFont(g_fDF);
    DrawTextAligned({ 326, 260 }, { 906.7f, 330 }, 30.0f, WithAlpha(RGBA(120, 120, 124, 255), a),
                    "GAME LOGO SLOT", Align::Center, true, false);
    SetFont(g_fRodin);
    DrawTextAligned({ 326, 340 }, { 906.7f, 380 }, 16.0f, WithAlpha(RGBA(90, 90, 94, 255), a),
                    "(326,213)-(907,430) - drop your wordmark here", Align::Center, true, false);
    ResetFont();
}

// the measured PRESS START capsule button with its animated green glow
void PressStart(double now, float a) {
    const float glowPulse = Breathe(now, 0.4f, 1.0f, 1.0f);
    // green glow (additive, larger than the ring, strongest at the ends)
    DrawRect({ 427.3f, 466.7f }, { 815.3f, 572.0f }, WithAlpha(C_GLOW, a * glowPulse * 0.22f), true);
    DrawRect({ 460, 480 }, { 782, 560 }, WithAlpha(C_GLOW, a * glowPulse * 0.30f), true);
    // metallic ring: capsule approximated with a chamfered band
    DrawVGradient({ 490.7f, 494.7f }, { 790.7f, 548.7f }, WithAlpha(C_RING_HI, a), WithAlpha(C_RING, a));
    // inner yellow capsule (glossy belly low-center)
    DrawVGradient({ 498.7f, 504.7f }, { 781.3f, 524.0f }, WithAlpha(C_CAP_TOP, a), WithAlpha(C_CAP_PEAK, a));
    DrawVGradient({ 498.7f, 524.0f }, { 781.3f, 535.3f }, WithAlpha(C_CAP_PEAK, a), WithAlpha(RGBA(150, 143, 48, 255), a));
    // outline-style PRESS START (dark olive ring + capsule-yellow inner)
    SetFont(g_fRodin);
    const char* PS = "PRESS START";
    float w = MeasureText(22.0f, PS).x;
    float px = 640 - w * 0.5f, py = 509.0f;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ px + dx * 1.6f, py + dy * 1.6f }, 22.0f, WithAlpha(C_PS_OUT, a), PS);
    DrawText({ px, py }, 22.0f, WithAlpha(C_CAP_PEAK, a), PS);
    ResetFont();
}

void Copyright(float a) {
    SetFont(g_fRodin);
    float w = MeasureText(14.0f, "(C) SEGA").x;
    DrawText({ 640 - w * 0.5f, 634.7f }, 14.0f, WithAlpha(C_WHITE, a), "(C) SEGA");
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
    float w = MeasureText(20.0f, E).x;
    float ex = 640 - w * 0.5f, ey = 492.0f;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ ex + dx * 1.5f, ey + dy * 1.5f }, 20.0f, WithAlpha(C_ENTRY_OUT, a), E);
    DrawText({ ex, ey }, 20.0f, WithAlpha(C_ENTRY, a), E);
    ResetFont();
    // arrows (solid green triangles) + outboard fading wedge bars
    const V2 lt[4] = { { 528.7f, 490.7f }, { 528.7f, 512.7f }, { 510.7f, 501.3f }, { 528.7f, 490.7f } };
    const V2 rt[4] = { { 750.7f, 490.7f }, { 750.7f, 512.7f }, { 770.0f, 501.7f }, { 750.7f, 490.7f } };
    const uint32_t ac[4] = { WithAlpha(C_ARROW, a), WithAlpha(C_ARROW, a), WithAlpha(C_ARROW, a), WithAlpha(C_ARROW, a) };
    DrawQuadGradient(lt, ac); DrawQuadGradient(rt, ac);
    {
        const V2 lw[4] = { { 456.7f, 489.3f }, { 498.7f, 489.3f }, { 498.7f, 513.3f }, { 456.7f, 513.3f } };
        const uint32_t lc[4] = { WithAlpha(C_ARROW, 0.0f), WithAlpha(C_ARROW, a * 0.7f), WithAlpha(C_ARROW, a * 0.7f), WithAlpha(C_ARROW, 0.0f) };
        DrawQuadGradient(lw, lc);
        const V2 rw[4] = { { 781.3f, 489.3f }, { 840.0f, 489.3f }, { 840.0f, 513.3f }, { 781.3f, 513.3f } };
        const uint32_t rc[4] = { WithAlpha(C_ARROW, a * 0.7f), WithAlpha(C_ARROW, 0.0f), WithAlpha(C_ARROW, 0.0f), WithAlpha(C_ARROW, a * 0.7f) };
        DrawQuadGradient(rw, rc);
    }
}

void AutosaveDialog(float a) {
    const uint32_t DK = RGBA(0, 0, 0, 255);
    const float x0 = 418, y0 = 191.3f, x1 = 859.3f, y1 = 526, ch = 25;
    DrawVGradient({ x0, y0 }, { x1, y1 }, WithAlpha(C_DLG_T, a), WithAlpha(C_DLG_B, a));
    // TL + BR chamfer cuts back to black
    {
        const V2 c1[4] = { { x0, y0 }, { x0 + ch, y0 }, { x0, y0 + ch }, { x0, y0 } };
        const V2 c2[4] = { { x1 - ch, y1 }, { x1, y1 }, { x1, y1 - ch }, { x1 - ch, y1 } };
        const uint32_t dc[4] = { DK, DK, DK, DK };
        DrawQuadGradient(c1, dc); DrawQuadGradient(c2, dc);
    }
    uint32_t bd = WithAlpha(C_DLG_BD, a);
    DrawRect({ x0 + ch, y0 }, { x1, y0 + 2.5f }, bd);
    DrawRect({ x0, y1 - 2.5f }, { x1 - ch, y1 }, WithAlpha(RGBA(218, 220, 218, 255), a));
    DrawRect({ x0, y0 + ch }, { x0 + 2.5f, y1 }, bd);
    DrawRect({ x1 - 2.5f, y0 }, { x1, y1 - ch }, WithAlpha(RGBA(222, 224, 222, 255), a));
    // 8 centred outlined lines, pitch 34.3
    SetFont(g_fRodin);
    const char* L[8] = { "This game utilizes", "an autosave feature.", "Please do not turn off",
                         "the console or remove", "any storage device when", "the autosave icon",
                         "appears", "on the screen." };
    for (int i = 0; i < 8; ++i) {
        float ly = 230.7f + i * 34.3f;
        float w = MeasureText(22.0f, L[i]).x;
        float lx = 638.7f - w * 0.5f;
        if (i == 6) lx += 16;   // line 7 carries the inline icon left of the word
        DrawText({ lx + 1.3f, ly + 1.3f }, 22.0f, WithAlpha(RGBA(13, 13, 11, 255), a), L[i]);
        DrawText({ lx, ly }, 22.0f, WithAlpha(RGBA(241, 240, 239, 255), a), L[i]);
        if (i == 6) {   // green autosave disc icon slot
            DrawRect({ lx - 33, ly - 2 }, { lx - 6.3f, ly + 24.7f }, WithAlpha(C_ICON_GRN, a));
            DrawRect({ lx - 26, ly + 5 }, { lx - 13, ly + 18 }, WithAlpha(RGBA(20, 120, 20, 255), a));
        }
    }
    ResetFont();
    // (A) Next, bottom-right, NOT dimmed
    SetFont(g_fRodin);
    if (g_glyphTex >= 0) {
        float asp = ((GLYPH_A.u1 - GLYPH_A.u0) * GTW) / ((GLYPH_A.v1 - GLYPH_A.v0) * GTH), gh = 32.7f, gw = gh * asp;
        DrawImage(g_glyphTex, { 859.3f, 637 - gh * 0.5f }, { 859.3f + gw, 637 + gh * 0.5f },
                  { GLYPH_A.u0, GLYPH_A.v0 }, { GLYPH_A.u1, GLYPH_A.v1 }, WithAlpha(C_WHITE, a));
    }
    DrawText({ 903.3f, 629.3f }, 20.0f, WithAlpha(RGBA(225, 225, 225, 255), a), "Next");
    ResetFont();
}

void Draw(double openSec) {
    const double now = Now();
    const float a = (float)ComputeMotion(openSec, 0.0, 12.0);
    Starfield(a);

    if (g_state == BT_MENU) {
        Earth(a);
        LetterboxBands(a);
        LogoSlot(a);
        Carousel(a);
        Copyright(a);
        return;
    }

    LogoSlot(a);
    PressStart(now, a);
    Copyright(a);

    if (g_state == BT_AUTOSAVE) {
        DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 192));   // 75% dim
        AutosaveDialog(1.0f);
    }
}

} // namespace

void BootTitleInit() { Init(); }
void BootTitleDraw(double openSeconds) { Draw(openSeconds); }
void BootTitleInput(const ScreenInput& in) { Input(in); }
void BootTitleReset() { Reset(); }
const char* BootTitleNav() { return Nav(); }
