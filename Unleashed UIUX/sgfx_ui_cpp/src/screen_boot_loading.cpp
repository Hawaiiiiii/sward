// =============================================================================
// screen_boot_loading.cpp — the first-boot "NOW LOADING" screen, matched 1:1 to
// the retail/recomp reference (_ref_load.png). Black screen with WHITE/light
// "NOW LOADING" bottom-right + an animated 3x3 dot-matrix spinner.
//
// FONT: the game's own bitmap font atlas mat_comon_txt_001.dds (256x128). It is
// PROPORTIONAL (variable glyph widths, ~14px pitch), ASCII 33..126 packed in
// reading order. loading_font.h holds each glyph's tight atlas bbox; glyphs are
// drawn straight from the atlas (proportional, bbox-bottom on the baseline), so
// the wordmark is the game's exact face and is re-skinnable by changing the string.
// Measured target: text x655..1000 (w~345), baseline ~y625; fill WHITE/light.
//
// ANIMATION: the "NOW LOADING" text BREATHES (dims out + back) on a ~1.5s cycle
// while the spinner keeps walking the ring — the classic Unleashed loader pulse.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "loading_font.h"
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_txtTex = -1;   // mat_comon_txt_001 bitmap-font atlas
// The real game loading text is WHITE/light (not green). White wordmark/glyph
// fill with a very slight top->bottom shade, + a dark outline for the bitmap-
// font fallback so the white core reads against the black screen.
const uint32_t C_WHITE_T   = RGBA(255, 255, 255, 255);  // wordmark/glyph fill top
const uint32_t C_WHITE_B   = RGBA(225, 228, 232, 255);  // wordmark/glyph fill bottom
const uint32_t C_OUTLINE   = RGBA(12, 14, 18, 255);     // dark outline for fallback text
// light "off" grey the unlit spinner cells settle toward
const uint32_t C_WHITE_OFF = RGBA(60, 64, 70, 255);

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
// the game wordmark is HEAVY with a dark outline: an 8-direction dilation of the
// dark outline colour builds a thick rim so the WHITE core reads against the black
// screen, then the white core is drawn on top. `a` is the breathing alpha.
void DrawGlowText(float tx, float baselineY, float S, float a, const char* txt) {
    // CONCENTRIC multi-radius dilation: a single offset pass can never thicken a
    // stroke past its own width (the copies separate) — stacking radii 1.1/2.2/3.3
    // in 8 directions builds the ref's solid heavy outline rim (runs ~9-12px).
    // dark outline pass: an 8-direction offset of the dark outline colour so the
    // white core reads against the black screen.
    for (float d = 1.1f; d < 3.5f; d += 1.1f) {
        const float dd = d * 0.7071f;
        uint32_t sT = WithAlpha(C_OUTLINE, a), sB = WithAlpha(C_OUTLINE, a);
        DrawAtlasText(tx - d,  baselineY,       S, sT, sB, txt);
        DrawAtlasText(tx + d,  baselineY,       S, sT, sB, txt);
        DrawAtlasText(tx,      baselineY - d,   S, sT, sB, txt);
        DrawAtlasText(tx,      baselineY + d,   S, sT, sB, txt);
        DrawAtlasText(tx - dd, baselineY - dd,  S, sT, sB, txt);
        DrawAtlasText(tx + dd, baselineY - dd,  S, sT, sB, txt);
        DrawAtlasText(tx - dd, baselineY + dd,  S, sT, sB, txt);
        DrawAtlasText(tx + dd, baselineY + dd,  S, sT, sB, txt);
    }
    // white core on top, with the slight top->bottom shade
    DrawAtlasText(tx, baselineY, S, WithAlpha(C_WHITE_T, a), WithAlpha(C_WHITE_B, a), txt);
}

// the dot-matrix spinner: a FULL 3x3 grid of gradient-filled white/light squares
// (incl. the centre cell). NOT a ring walk — each of the 9 cells (img_01..09,
// row-major) blinks on its own diagonal-cascade schedule, driven by a 4.0s loop.
// The retail loader reads near-binary: a cell is either lit (~0.9, white) or dark
// (~0.12, dim grey), with a short ~10-frame ramp at each window edge.
// Measured footprint: cell 8.7, step 10 (unchanged).
//
// Per-cell lit windows over frame in [0,240] (4.0s * 60). Indexed by [row][col],
// so img_01(0,0) img_02(1,0) img_03(2,0) top, img_04..06 mid, img_07..09 bottom.
struct LitWin { float a, b; };
struct CellSched { int n; LitWin w[5]; };
static const CellSched SPIN_SCHED[3][3] = {
    // row 0 (top):    img_01(0,0)            img_02(1,0)            img_03(2,0)
    { { 2, {{0,40},{160,200}} },         { 2, {{20,60},{180,220}} },   { 2, {{40,80},{200,240}} } },
    // row 1 (mid):    img_04(0,1)            img_05(1,1 centre)                                  img_06(2,1)
    { { 2, {{20,60},{140,180}} },        { 5, {{0,15},{40,80},{105,135},{160,200},{225,240}} },  { 2, {{60,100},{180,220}} } },
    // row 2 (bottom): img_07(0,2)            img_08(1,2)            img_09(2,2)
    { { 2, {{40,80},{120,160}} },        { 2, {{60,100},{140,180}} },  { 2, {{80,120},{157,197}} } },
};
void DrawSpinner(float x0, float y0, double now) {
    const float cell = 8.7f, step = 10.0f;
    const float frame = (float)(std::fmod(now, 4.0) * 60.0);   // 0..240 over the 4.0s loop
    const float RAMP = 10.0f;                                  // edge ramp width (frames)
    for (int row = 0; row < 3; ++row)
    for (int col = 0; col < 3; ++col) {
        const CellSched& cs = SPIN_SCHED[row][col];
        // near-binary level: 1.0 inside a window, 0.0 outside, with a linear ~10f
        // ramp on each edge so the blink isn't a hard pop (cheap soft on/off).
        float lit = 0.0f;
        for (int j = 0; j < cs.n; ++j) {
            const LitWin& w = cs.w[j];
            float on = std::min((frame - w.a) / RAMP, (w.b - frame) / RAMP);   // >0 within [a,b]
            lit = std::max(lit, std::min(1.0f, std::max(0.0f, on)));
        }
        // lit -> ~0.9 toward white; unlit -> ~0.12 toward the dim off grey, to
        // match the white loading text (the retail loader is not green).
        float k = 0.04f + 0.86f * lit;                          // 0.04 (off) .. 0.90 (on)
        uint32_t c = ColourLerp(C_WHITE_OFF, C_WHITE_T, k);
        float gx = x0 + col * step, gy = y0 + row * step;
        if (g_wordTex >= 0)   // the real cell sprite from mat_load_en_001
            DrawImage(g_wordTex, { gx, gy }, { gx + cell, gy + cell },
                      { SQ_U0, SQ_V0 }, { SQ_U1, SQ_V1 }, c);
        else
            DrawVGradient({ gx, gy }, { gx + cell, gy + cell },
                          ColourLerp(C_WHITE_OFF, C_WHITE_T, std::min(1.0f, k * 1.1f + 0.08f)), c);
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
        // dark outline) is a WHITE/luminance mask. The retail loader text is
        // WHITE/light (not green) — so tint with white and let the sprite's own
        // baked dark outline read through. A faint top->bottom shade keeps it
        // from looking flat. The breathing `pulse` drives the alpha.
        DrawImageVGradient(g_wordTex, { 650, 568 }, { 969, 602 },
                           { WM_U0, WM_V0 }, { WM_U1, WM_V1 },
                           WithAlpha(C_WHITE_T, pulse),
                           WithAlpha(C_WHITE_B, pulse));
    } else {
        // re-skin path (custom SGFX strings): the proportional bitmap font, bolded
        const char* txt = "NOW LOADING";
        const float S = 2.0f;
        DrawGlowText(1003.0f - AtlasTextW(S, txt), 628.0f, S, pulse, txt);
    }
    DrawSpinner(984.0f, 569.0f, clock);
}

} // namespace

void BootLoadingInit() { Init(); }
void BootLoadingDraw(double openSeconds) { Draw(openSeconds); }
void BootLoadingInput(const ScreenInput& in) { Input(in); }
void BootLoadingReset() { Reset(); }
