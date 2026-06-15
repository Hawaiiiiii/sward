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
const uint32_t C_BAL_T   = RGBA(162, 162, 166, 235);   // balloon silver top (neutral, de-tinted)
const uint32_t C_BAL_B   = RGBA(120, 120, 120, 235);
const uint32_t C_BAL_BD  = RGBA(240, 242, 246, 255);
const uint32_t C_TXT     = RGBA(245, 245, 247, 255);   // white outlined dialogue
const uint32_t C_TXT_OUT = RGBA(26, 28, 34, 255);
const uint32_t C_PILL_T  = RGBA(196, 165, 58, 255);    // nameplate gold (de-brightened top)
const uint32_t C_PILL_B  = RGBA(178, 140, 40, 255);
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
        // OPEN gold coin-ring icon, 3/4 perspective (outer ~15x37, inner hole ~7x22)
        const float rcx = 264.5f, rcy = 83.5f;          // ring centre in the art slot
        const float oRx = 7.5f, oRy = 18.5f;            // outer ellipse half-extents
        const float iRx = 3.5f, iRy = 11.0f;            // inner hole half-extents
        uint32_t rgold = WithAlpha(C_RING, a);
        for (int yy = (int)(rcy - oRy); yy <= (int)(rcy + oRy); ++yy) {
            float fy = yy + 0.5f, dy = fy - rcy;
            float ohw = oRx * sqrtf(fmaxf(0.0f, 1.0f - (dy * dy) / (oRy * oRy)));
            if (ohw <= 0.0f) continue;
            float inHole = 1.0f - (dy * dy) / (iRy * iRy);
            if (inHole > 0.0f) {                          // row crosses the open hole: two walls
                float ihw = iRx * sqrtf(inHole);
                DrawRect({ rcx - ohw, fy - 0.5f }, { rcx - ihw, fy + 0.5f }, rgold);  // left wall
                DrawRect({ rcx + ihw, fy - 0.5f }, { rcx + ohw, fy + 0.5f }, rgold);  // right wall
            } else {                                      // above/below the hole: solid arc
                DrawRect({ rcx - ohw, fy - 0.5f }, { rcx + ohw, fy + 0.5f }, rgold);
            }
        }
        SetFont(g_fRodin);
        DrawTextShadow({ 291, 70 }, 22.0f, WithAlpha(C_WHITE, a), "999999");
        ResetFont();
    }

    // ---- speaker nameplate: gold rounded CAPSULE, upper-right of centre ----
    {
        const float y0 = 53, y1 = 117;                  // h64
        const float cy = (y0 + y1) * 0.5f;              // 85
        const float r  = (y1 - y0) * 0.5f;              // 32 (half-height)
        const float lcx = 699;                          // left cap centre (x0 699 was 667+32)
        const float rcx = 1050 - r;                     // right cap centre = 1018 (right edge 1050)
        const float x0 = lcx, x1 = rcx;                 // straight-section span between cap centres
        uint32_t pT = WithAlpha(C_PILL_T, a), pB = WithAlpha(C_PILL_B, a);
        uint32_t rim = WithAlpha(RGBA(252, 236, 170, 255), a);
        // straight middle section (full height) with the gold vertical gradient
        DrawVGradient({ x0, y0 }, { x1, y1 }, pT, pB);
        // semicircular end-caps: stacked-strip approximation, one strip per pixel row,
        // each row's gold tone vertically lerped to match the gradient on the straight part
        for (int yy = 0; yy < (int)(y1 - y0); ++yy) {
            float fy = y0 + yy + 0.5f;
            float dy = fy - cy;
            float hw = sqrtf(fmaxf(0.0f, r * r - dy * dy));  // horizontal half-extent of the cap
            uint32_t col = WithAlpha(ColourLerp(C_PILL_T, C_PILL_B, (fy - y0) / (y1 - y0)), a);
            DrawRect({ lcx - hw, fy - 0.5f }, { lcx, fy + 0.5f }, col);   // left cap
            DrawRect({ rcx, fy - 0.5f }, { rcx + hw, fy + 0.5f }, col);   // right cap
        }
        // ---- continuous ~2px bright rim around the WHOLE capsule ----
        // top & bottom edges of the straight section
        DrawRect({ x0, y0 }, { x1, y0 + 2 }, rim);
        DrawRect({ x0, y1 - 2 }, { x1, y1 }, rim);
        // arc rims on both rounded ends (2px-thick annulus strips, per row)
        for (int yy = 0; yy < (int)(y1 - y0); ++yy) {
            float fy = y0 + yy + 0.5f;
            float dy = fy - cy;
            float hw = sqrtf(fmaxf(0.0f, r * r - dy * dy));
            DrawRect({ lcx - hw, fy - 0.5f }, { lcx - hw + 2, fy + 0.5f }, rim);   // left arc
            DrawRect({ rcx + hw - 2, fy - 0.5f }, { rcx + hw, fy + 0.5f }, rim);   // right arc
        }
        SetFont(g_fSeurat);
        const char* NAME = "Don Fachio's Apotos";
        float w = MeasureText(22.0f, NAME).x;
        const float capCx = (lcx - r + rcx + r) * 0.5f;   // visible capsule centre (~858)
        const float nx = capCx - w * 0.5f, ny = (y0 + y1) * 0.5f - 12.0f;
        // INVERTED grammar (verified vs real n1_nameplate): name is DARK letters
        // with a LIGHT/white halo on the gold pill, NOT a cream fill + dark outline.
        // offset/outline passes = light cream halo; centre fill = dark core (.###.)
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                if (dx || dy)
                    DrawText({ nx + dx * 1.4f, ny + dy * 1.4f }, 22.0f, WithAlpha(RGBA(240, 236, 224, 255), a), NAME);
        DrawText({ nx, ny }, 22.0f, WithAlpha(RGBA(40, 34, 22, 255), a), NAME);
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
            const float lx = x0 + 36;
            // stronger single drop-shadow pass biased down-and-right (real centroid
            // dY=+17.5, below-right shell dominant)
            DrawText({ lx + 1.5f, ly + 1.5f }, 24.0f, WithAlpha(C_TXT_OUT, bT), s);
            // thin ~1px outline ring so edges stay crisp without a heavy symmetric shell
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    if (dx || dy)
                        DrawText({ lx + dx * 1.0f, ly + dy * 1.0f }, 24.0f, WithAlpha(C_TXT_OUT, bT), s);
            DrawText({ lx, ly }, 24.0f, WithAlpha(C_TXT, bT), s);
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
        // footer labels share the dialogue 'line' lambda's 8-way dark outline grammar
        auto label = [&](float lx, const char* s){
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    if (dx || dy)
                        DrawText({ lx + dx * 1.4f, hcy - 13 + dy * 1.4f }, 22.0f, WithAlpha(C_TXT_OUT, a), s);
            DrawText({ lx, hcy - 13 }, 22.0f, WithAlpha(C_WHITE, a), s);
        };
        float hx = 680; hx = glyph(GLYPH_A, hx); label(hx, "Select");
        hx = 857; hx = glyph(GLYPH_B, hx); label(hx, "Back");
        ResetFont();
    }
}

} // namespace

void BalloonInit() { Init(); }
void BalloonDraw(double openSeconds) { Draw(openSeconds); }
void BalloonInput(const ScreenInput& in) { Input(in); }
void BalloonReset() { Reset(); }
