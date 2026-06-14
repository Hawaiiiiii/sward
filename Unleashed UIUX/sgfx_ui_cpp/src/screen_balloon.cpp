// =============================================================================
// screen_balloon.cpp — the NPC conversation balloon, re-authored 1:1 from LIVE
// capture (s01 120-134s + 138-154s; still balloon_npc1.png):
//   * a SILVER chamfered dialogue panel bottom-left (TL+BR chamfer grammar,
//     light grey gradient, thin bright border, no tail), two white outlined
//     text lines;
//   * the speaker NAMEPLATE: a gold rounded pill top-right ("Don Fachio's
//     Apotos") with dark text;
//   * the rings counter top-left (HUD remnant slot);
//   * footer (A) Select / (B) Back; the live town scene stays bright behind.
// (A) advances through the sample conversation lines; B backs out (flow).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0;
int g_glyphTex = -1;

struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };

// ---- palette (sampled from balloon_npc1) --------------------------------------
const uint32_t C_BAL_T   = RGBA(214, 216, 220, 235);   // balloon silver top
const uint32_t C_BAL_B   = RGBA(154, 156, 162, 235);
const uint32_t C_BAL_BD  = RGBA(240, 242, 246, 255);
const uint32_t C_TXT     = RGBA(245, 245, 247, 255);   // white outlined dialogue
const uint32_t C_TXT_OUT = RGBA(26, 28, 34, 255);
const uint32_t C_PILL_T  = RGBA(244, 204, 60, 255);    // nameplate gold
const uint32_t C_PILL_B  = RGBA(206, 152, 22, 255);
const uint32_t C_PILL_TXT= RGBA(54, 36, 6, 255);
const uint32_t C_WHITE   = RGBA(255, 255, 255, 255);
const uint32_t C_RING    = RGBA(232, 196, 64, 255);
// town scene placeholder
const uint32_t C_SKY_T = RGBA(96, 158, 212, 255), C_SKY_B = RGBA(168, 204, 232, 255);

// sample conversation (the captured ice-cream vendor exchange)
struct Line2 { const char* a; const char* b; };
const Line2 SCRIPT[] = {
    { "That's the ticket!", "You completed all of the missions in style!" },
    { "The milk of human kindness...", "makes the ice cream smooth and creamy." },
    { "Come back any time!", "Apotos is always glad to see you." },
};
constexpr int SCRIPT_N = 3;
int g_line = 0;

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
}
void Reset() { g_line = 0; }
void Input(const ScreenInput& in) {
    if (in.accept) g_line = (g_line + 1) % SCRIPT_N;
}

void Draw(double openSec) {
    const float a = (float)ComputeMotion(openSec, 0.0, 8.0);

    // ---- live town scene slot ----
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_SKY_T, C_SKY_B);
    DrawRect({ 0, 520 }, { REF_W, REF_H }, RGBA(214, 212, 200, 255));
    SetFont(g_fSeurat);
    DrawTextAligned({ 700, 180 }, { 1100, 480 }, 14.0f, WithAlpha(RGBA(120, 140, 165, 255), a),
                    "( NPC render: live 3D )", Align::Center, true, false);
    ResetFont();

    // ---- rings counter slot (top-left HUD, title-safe inset) ----
    {
        // thin angled gold ring icon (art slot ~x256-273 y64-103); digits to its right
        const V2 ring[4] = { { 259, 64 }, { 273, 64 }, { 270, 103 }, { 256, 103 } };
        const uint32_t rc[4] = { WithAlpha(C_RING, a), WithAlpha(C_RING, a),
                                 WithAlpha(C_RING, a), WithAlpha(C_RING, a) };
        DrawQuadGradient(ring, rc);
        SetFont(g_fRodin);
        DrawTextShadow({ 291, 70 }, 22.0f, WithAlpha(C_WHITE, a), "999999");
        ResetFont();
    }

    // ---- speaker nameplate: gold rounded pill, upper-right of centre ----
    {
        const float x0 = 667, y0 = 53, x1 = 1066, y1 = 117;
        DrawVGradient({ x0 + 4, y0 }, { x1 - 4, y1 }, WithAlpha(C_PILL_T, a), WithAlpha(C_PILL_B, a));
        DrawRect({ x0, y0 + 4 }, { x0 + 4, y1 - 4 }, WithAlpha(C_PILL_B, a));     // rounded-end hint
        DrawRect({ x1 - 4, y0 + 4 }, { x1, y1 - 4 }, WithAlpha(C_PILL_B, a));
        DrawRect({ x0 + 4, y0 }, { x1 - 4, y0 + 2 }, WithAlpha(RGBA(252, 234, 150, 255), a));
        SetFont(g_fSeurat);
        const char* NAME = "Don Fachio's Apotos";
        float w = MeasureText(22.0f, NAME).x;
        const float nx = (x0 + x1) * 0.5f - w * 0.5f, ny = (y0 + y1) * 0.5f - 12.0f;
        // outlined grammar to match the dialogue 'line' lambda: dark stroke + cream fill
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                if (dx || dy)
                    DrawText({ nx + dx * 1.4f, ny + dy * 1.4f }, 22.0f, WithAlpha(RGBA(26, 28, 34, 255), a), NAME);
        DrawText({ nx, ny }, 22.0f, WithAlpha(RGBA(231, 225, 215, 255), a), NAME);
        ResetFont();
    }

    // ---- the silver dialogue balloon (TL+BR chamfer, no tail), lower third ----
    {
        const float x0 = 208, y0 = 445, x1 = 1093, y1 = 613, ch = 20;
        const float bT = (float)ComputeMotion(openSec, 2.0, 8.0);
        DrawVGradient({ x0, y0 }, { x1, y1 }, WithAlpha(C_BAL_T, bT), WithAlpha(C_BAL_B, bT));
        // chamfer cuts back to the scene (approximate with sky tone)
        const V2 c1[4] = { { x0, y0 }, { x0 + ch, y0 }, { x0, y0 + ch }, { x0, y0 } };
        const V2 c2[4] = { { x1 - ch, y1 }, { x1, y1 }, { x1, y1 - ch }, { x1 - ch, y1 } };
        const uint32_t sky[4] = { WithAlpha(C_SKY_B, bT), WithAlpha(C_SKY_B, bT), WithAlpha(C_SKY_B, bT), WithAlpha(C_SKY_B, bT) };
        DrawQuadGradient(c1, sky); DrawQuadGradient(c2, sky);
        uint32_t bd = WithAlpha(C_BAL_BD, bT);
        DrawRect({ x0 + ch, y0 }, { x1, y0 + 2 }, bd);
        DrawRect({ x0, y1 - 2 }, { x1 - ch, y1 }, bd);
        DrawRect({ x0, y0 + ch }, { x0 + 2, y1 }, bd);
        DrawRect({ x1 - 2, y0 }, { x1, y1 - ch }, bd);
        // two outlined dialogue lines, upper area of the window (room below for a 3rd)
        SetFont(g_fSeurat);
        const Line2& L = SCRIPT[g_line];
        auto line = [&](const char* s, float ly) {
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    if (dx || dy)
                        DrawText({ x0 + 36 + dx * 1.4f, ly + dy * 1.4f }, 24.0f, WithAlpha(C_TXT_OUT, bT), s);
            DrawText({ x0 + 36, ly }, 24.0f, WithAlpha(C_TXT, bT), s);
        };
        line(L.a, y0 + 28);
        line(L.b, y0 + 62);
        ResetFont();
    }

    // ---- footer: (A) Select  (B) Back — bottom-right, under the dialogue box ----
    {
        SetFont(g_fRodin);
        float hcy = 638;
        auto glyph = [&](const UV& g, float x){ if (g_glyphTex<0) return x; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh=30.0f, gw=gh*asp; DrawImage(g_glyphTex,{x,hcy-gh*0.5f},{x+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, WithAlpha(C_WHITE,a)); return x+gw+8; };
        float hx = 680; hx = glyph(GLYPH_A, hx); DrawText({ hx, hcy - 13 }, 22.0f, WithAlpha(C_WHITE, a), "Select");
        hx = 857; hx = glyph(GLYPH_B, hx); DrawText({ hx, hcy - 13 }, 22.0f, WithAlpha(C_WHITE, a), "Back");
        ResetFont();
    }
}

} // namespace

void BalloonInit() { Init(); }
void BalloonDraw(double openSeconds) { Draw(openSeconds); }
void BalloonInput(const ScreenInput& in) { Input(in); }
void BalloonReset() { Reset(); }
