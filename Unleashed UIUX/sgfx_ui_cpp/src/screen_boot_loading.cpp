// =============================================================================
// screen_boot_loading.cpp — the first-boot "NOW LOADING" screen, matched 1:1 to
// the retail/recomp reference (_ref_load.png). Black screen with bright-green
// "NOW LOADING" bottom-right + an animated 3x3 dot-matrix spinner.
//
// FONT: the game's own bitmap font atlas mat_comon_txt_001.dds (256x128). It is
// PROPORTIONAL (variable glyph widths, ~14px pitch), ASCII 33..126 packed in
// reading order. loading_font.h holds each glyph's tight atlas bbox; glyphs are
// drawn straight from the atlas (proportional, bbox-bottom on the baseline), so
// the wordmark is the game's exact face and is re-skinnable by changing the string.
// Measured target: text x655..1000 (w~345), baseline ~y625; green ~ (74,230,18).
//
// ANIMATION: the "NOW LOADING" text BREATHES (dims out + back) on a ~1.5s cycle
// while the spinner keeps walking the ring — the classic Unleashed loader pulse.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "loading_font.h"
#include <cmath>

using namespace ui;
namespace {

int g_txtTex = -1;   // mat_comon_txt_001 bitmap-font atlas
// colours sampled from _ref_load_bright.png: glyphs shade bright-top -> darker-
// bottom (g ~218 -> ~147) and the brightest pixels are soft (~111,205,73), not neon.
const uint32_t C_GREEN     = RGBA(74, 230, 18, 255);
const uint32_t C_GREEN_T   = RGBA(92, 218, 34, 255);    // glyph fill top
const uint32_t C_GREEN_B   = RGBA(50, 147, 12, 255);    // glyph fill bottom
const uint32_t C_GREEN_DIM = RGBA(28, 92, 8, 255);

int g_wordTex = -1;  // mat_load_en_001: the PRE-RENDERED heavy "NOW LOADING" wordmark
                     // + the spinner cell square — the game's actual boot-loader art.
// sub-rects measured from mat_load_en_001.png (512x128):
const float WM_U0 = 0.3750f, WM_V0 = 0.0078f, WM_U1 = 0.9961f, WM_V1 = 0.2812f;   // NOW LOADING
const float SQ_U0 = 0.9102f, SQ_V0 = 0.3125f, SQ_U1 = 0.9805f, SQ_V1 = 0.5859f;   // spinner cell

void Init() {
    if (g_txtTex  < 0) g_txtTex  = gfx::loadTexture("assets/loading/mat_comon_txt_001.png");
    if (g_wordTex < 0) g_wordTex = gfx::loadTexture("assets/loading/mat_load_en_001.png");
}
void Reset() {}
void Input(const ScreenInput&) {}

// ---- the game's proportional bitmap font (mat_comon_txt_001 + loading_font.h) ----
constexpr float ATLAS_W = 256.0f, ATLAS_H = 128.0f;
constexpr float LF_GAP = 5.0f, LF_SPACE = 18.0f;   // inter-glyph gap + space advance (atlas px, pre-scale)
const LGlyph* lglyph(char c) {
    int i = (unsigned char)c - LFONT_FIRST;
    if (i < 0 || i >= LFONT_COUNT) return nullptr;
    return (LFONT[i].x1 > LFONT[i].x0) ? &LFONT[i] : nullptr;
}
float AtlasTextW(float S, const char* s) {
    float w = 0;
    for (const char* p = s; *p; ++p) {
        if (*p == ' ') { w += LF_SPACE * S; continue; }
        const LGlyph* g = lglyph(*p);
        if (g) w += ((g->x1 - g->x0) + LF_GAP) * S;
    }
    return w - LF_GAP * S;   // trim trailing gap
}
void DrawAtlasText(float xLeft, float baselineY, float S, uint32_t colT, uint32_t colB,
                   const char* s, bool additive = false) {
    float x = xLeft;
    for (const char* p = s; *p; ++p) {
        if (*p == ' ') { x += LF_SPACE * S; continue; }
        const LGlyph* g = lglyph(*p); if (!g) continue;
        float gw = (g->x1 - g->x0) * S, gh = (g->y1 - g->y0) * S;
        float u0 = g->x0 / ATLAS_W, v0 = g->y0 / ATLAS_H, u1 = g->x1 / ATLAS_W, v1 = g->y1 / ATLAS_H;
        if (g_txtTex >= 0)
            DrawImageVGradient(g_txtTex, { x, baselineY - gh }, { x + gw, baselineY },
                               { u0, v0 }, { u1, v1 }, colT, colB, additive);
        x += gw + LF_GAP * S;
    }
}
// the game wordmark is HEAVY + green-bloomed: an 8-direction dilation merges into
// thick joined strokes (the ref runs are ~2x the bare atlas weight), shaded
// bright-top -> dark-bottom, with a soft additive bloom. `a` is the breathing alpha.
void DrawGlowText(float tx, float baselineY, float S, float a, const char* txt) {
    // CONCENTRIC multi-radius dilation: a single offset pass can never thicken a
    // stroke past its own width (the copies separate) — stacking radii 1.1/2.2/3.3
    // in 8 directions merges into the ref's solid heavy slab (runs ~9-12px).
    for (float d = 1.1f; d < 3.5f; d += 1.1f) {
        const float dd = d * 0.7071f;
        // the dilation body carries the same top->bottom shading as the core so the
        // bolded letter shades as ONE form (ref: g~218 top -> ~147 bottom)
        uint32_t sT = WithAlpha(C_GREEN_T, a), sB = WithAlpha(C_GREEN_B, a);
        DrawAtlasText(tx - d,  baselineY,       S, sT, sB, txt);
        DrawAtlasText(tx + d,  baselineY,       S, sT, sB, txt);
        DrawAtlasText(tx,      baselineY - d,   S, sT, sB, txt);
        DrawAtlasText(tx,      baselineY + d,   S, sT, sB, txt);
        DrawAtlasText(tx - dd, baselineY - dd,  S, sT, sB, txt);
        DrawAtlasText(tx + dd, baselineY - dd,  S, sT, sB, txt);
        DrawAtlasText(tx - dd, baselineY + dd,  S, sT, sB, txt);
        DrawAtlasText(tx + dd, baselineY + dd,  S, sT, sB, txt);
    }
    DrawAtlasText(tx, baselineY, S, WithAlpha(C_GREEN_T, a), WithAlpha(C_GREEN_B, a), txt);
    uint32_t halo = WithAlpha(C_GREEN, a * 0.08f);
    DrawAtlasText(tx - 3.4f, baselineY + 1.2f, S, halo, halo, txt, true);  // soft additive bloom
    DrawAtlasText(tx + 3.4f, baselineY + 1.2f, S, halo, halo, txt, true);
    DrawAtlasText(tx, baselineY - 3.4f, S, halo, halo, txt, true);
}

// the dot-matrix spinner: a 3x3 grid of gradient-filled green squares (center empty);
// a lit cell walks the 8-position ring clockwise with a trailing comet fade, ~10/sec.
// Measured footprint: 32x32 at (1023,593) -> cell 9, step 11.5. Stays at full brightness.
void DrawSpinner(float x0, float y0, double now) {
    static const int RING[8][2] = { {0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1} };
    const float cell = 9.0f, step = 11.5f;
    int head = (int)(now * 10.0) % 8;
    for (int i = 0; i < 8; ++i) {
        int cx = RING[i][0], cy = RING[i][1];
        int d = (head - i + 8) % 8;
        // the ref ring reads near-uniform (all cells lit ~168-195 with a brighter
        // head), so the comet trail is SHALLOW, not a deep fade-to-black
        float k = (d == 0) ? 1.0f : (d == 1 ? 0.86f : (d == 2 ? 0.76f : 0.68f));
        uint32_t c = ColourLerp(C_GREEN_DIM, C_GREEN, k);
        float gx = x0 + cx * step, gy = y0 + cy * step;
        if (g_wordTex >= 0)   // the real cell sprite from mat_load_en_001
            DrawImage(g_wordTex, { gx, gy }, { gx + cell, gy + cell },
                      { SQ_U0, SQ_V0 }, { SQ_U1, SQ_V1 }, c);
        else
            DrawVGradient({ gx, gy }, { gx + cell, gy + cell },
                          ColourLerp(C_GREEN_DIM, C_GREEN, std::min(1.0f, k * 1.1f + 0.08f)), c);
    }
}

void Draw(double openSec) {
    (void)openSec;
    const double clock = Now();              // continuous clock (loops); openSec is the screen-open stamp
    DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 255));
    // breathing "dim out" pulse, measured from NOW LOADING (first booting).mp4:
    // ~1.0s period, brightness ~0.40..1.0, holds bright then dips briefly (not a
    // symmetric sine) -> sqrt-bias a cosine toward the bright end. Spinner stays full.
    float t01 = (float)(clock - std::floor(clock));            // 0..1 over a 1.0s period
    float tri = 0.5f - 0.5f * std::cos(6.2831853f * t01);      // smooth 0..1..0
    float pulse = 0.40f + 0.60f * std::sqrt(tri);              // bright-biased
    if (g_wordTex >= 0) {
        // 1:1 path: the game's pre-rendered heavy wordmark sprite (white core +
        // green rim + dark outline), tinted with the measured bright-top ->
        // dark-bottom green shading (face g ~218 -> ~147), at the measured rect.
        DrawImageVGradient(g_wordTex, { 651, 592 }, { 1005, 628 },
                           { WM_U0, WM_V0 }, { WM_U1, WM_V1 },
                           WithAlpha(RGBA(76, 218, 16, 255), pulse),
                           WithAlpha(RGBA(50, 147, 10, 255), pulse));
    } else {
        // re-skin path (custom SGFX strings): the proportional bitmap font, bolded
        const char* txt = "NOW LOADING";
        const float S = 2.0f;
        DrawGlowText(1003.0f - AtlasTextW(S, txt), 628.0f, S, pulse, txt);
    }
    DrawSpinner(1023.0f, 593.0f, clock);
}

} // namespace

void BootLoadingInit() { Init(); }
void BootLoadingDraw(double openSeconds) { Draw(openSeconds); }
void BootLoadingInput(const ScreenInput& in) { Input(in); }
void BootLoadingReset() { Reset(); }
