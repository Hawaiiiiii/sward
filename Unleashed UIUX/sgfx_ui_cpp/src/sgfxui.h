// =============================================================================
// sgfxui.h — a small, clean immediate-mode UI layer for the Sonic Unleashed
// screen reconstruction, written in the same spirit as UnleashedRecomp's
// ui/imgui_utils helpers but standalone: it emits gfx::Quad batches straight to
// the plume/D3D12 CSD backend (gfx_d3d12) instead of depending on Dear ImGui.
//
// Everything is authored in the Hedgehog UI's 1280x720 reference space; the CSD
// vertex shader maps reference pixels to NDC, so screens lay out in honest pixel
// coordinates. Animation is frame-based (60 fps) ease-out, matching the game's
// imgui_utils ComputeMotion (sqrt of a clamped linear ramp). Screens are plain
// classes with Init()/Draw(t) and static animation state — see screen.h.
// =============================================================================
#pragma once
#include <cstdint>
#include "gfx_d3d12.h"

namespace ui {

constexpr float REF_W = 1280.0f;   // Hedgehog UI canvas width
constexpr float REF_H = 720.0f;    // Hedgehog UI canvas height

struct V2 { float x = 0.0f, y = 0.0f; };

// 0xAARRGGBB packed colour, matching gfx::Quad::color / parseColor.
inline uint32_t RGBA(int r, int g, int b, int a = 255) {
    return (uint32_t(a & 255) << 24) | (uint32_t(r & 255) << 16) |
           (uint32_t(g & 255) << 8)  |  uint32_t(b & 255);
}
// Scale an existing colour's alpha by t in [0,1] (the common fade idiom).
uint32_t WithAlpha(uint32_t col, float t);

// ---- lifecycle -------------------------------------------------------------
// fontTtfPath: a .ttf baked into the glyph atlas. Placeholder system font is fine
// for proving layout/motion; swap to the extracted game font for final fidelity.
bool   Init(const char* fontTtfPath);          // bakes font 0 (the default)
int    LoadFont(const char* fontTtfPath);      // bake an extra font; returns its index (0 on failure)
// Load a real MSDF font from the recomp's parsed im_font_atlas. shortName is one of
// "seurat","rodin_db","rodin_ub","rodin_m". Pixel-exact glyph metrics + 3-channel MSDF
// (MOD_MSDF_TEXT) — the game's actual text rendering. Returns its index (0 on failure).
int    LoadMsdfFont(const char* shortName);
void   SetFont(int fontIndex);                 // select current font for text (state, like SetModifier)
void   ResetFont();                            // back to font 0
void   Shutdown();
void   BeginFrame(double nowSeconds);            // resets the quad list, sets the clock
void   Flush(gfx::Quad*& outQuads, int& outCount); // exposes the accumulated batch
double Now();

// ---- reference-space helper (identity at 16:9; hook for future letterboxing) -
float Scale(float referencePixels);

// ---- easing (frame-based @60fps; mirrors imgui_utils ComputeMotion) ---------
// startSec    : wall-clock second the motion begins.
// offsetFrames: per-element stagger (a cast appears a few frames after its group).
// totalFrames : motion duration in frames.
double ComputeLinearMotion(double startSec, double offsetFrames, double totalFrames);
double ComputeMotion(double startSec, double offsetFrames, double totalFrames);  // sqrt() ease-out
float    Lerp(float a, float b, float t);
float    Cubic(float a, float b, float t);                 // smoothstep-ish (3t^2-2t^3)
float    Hermite(float a, float b, float t);
V2       Lerp(V2 a, V2 b, float t);
uint32_t ColourLerp(uint32_t a, uint32_t b, float t);       // per-channel, incl. alpha

// ---- real GPU shader modifiers (csd_modifier_ps) ---------------------------
// Sets the per-quad ShaderModifier applied to every subsequent primitive until
// reset — the recomp's SetShaderModifier, running in the real CSD pixel shader.
enum Modifier : uint32_t { MOD_NONE = 0, MOD_SCANLINE = 1, MOD_CHECKERBOARD = 2, MOD_GRAYSCALE = 3,
                           MOD_SDF_TEXT = 4, MOD_TITLE_BEVEL = 5, MOD_SCANLINE_BUTTON = 6,
                           MOD_CATEGORY_BEVEL = 7, MOD_MSDF_TEXT = 8 };
void SetModifier(uint32_t m);
void PushClip(V2 min, V2 max);   // scissor: clamp subsequent quads/text to this rect (stacks)
void PopClip();
void ResetModifier();

// ---- primitives (append to the batch; later calls paint over earlier ones) --
void DrawRect(V2 min, V2 max, uint32_t col, bool additive = false);
void DrawVGradient(V2 min, V2 max, uint32_t top, uint32_t bottom);   // vertical 2-colour gradient (single quad)
void DrawHGradient(V2 min, V2 max, uint32_t left, uint32_t right);   // horizontal 2-colour gradient
// full 4-corner bilinear gradient (TL,TR,BR,BL) — the AddRectFilledMultiColor primitive.
void DrawQuadGradient(V2 min, V2 max, uint32_t cTL, uint32_t cTR, uint32_t cBR, uint32_t cBL, bool additive = false);
void DrawImage(int tex, V2 min, V2 max, V2 uv0, V2 uv1,
               uint32_t col = 0xFFFFFFFFu, bool additive = false);

// Arbitrary 4-corner textured quad — used by the CSD player for ROTATED casts (the
// game stores fully-transformed corners). corners[] and uvs[] are in TL,TR,BR,BL
// order to match the Push/index-buffer winding. Skips the axis-aligned assumption.
void DrawImageQuad(int tex, const V2 corners[4], const V2 uvs[4],
                   uint32_t col = 0xFFFFFFFFu, bool additive = false);

// Generic stretchable 9-slice. border insets (l/t/r/b) are in atlas pixels; the
// centre region tiles/stretches, the corners stay fixed. Mirrors the game's and
// the recomp's window-frame drawing (DrawPauseContainer), but data-driven.
struct Slice9 { int tex = -1; float texW = 1, texH = 1; float l = 0, t = 0, r = 0, b = 0; };
void DrawContainer(const Slice9& s, V2 min, V2 max, uint32_t col);

// Real-atlas 9-slice with EXPLICIT (possibly non-contiguous) source spans — for
// the game's window/bar frames packed into a sprite atlas (e.g. mat_result_comon_001).
// u*/v* are normalized atlas coords; the centre column/row stretch to fill while the
// corners stay fixed at the dest border insets (reference px). This is how the recomp
// draws DrawPauseContainer, but sourced from the real Sonic Unleashed frame art.
struct NineSlice {
    int   tex = -1;
    float uL0=0,uL1=0, uC0=0,uC1=0, uR0=0,uR1=0;   // left / centre / right column spans
    float vT0=0,vT1=0, vC0=0,vC1=0, vB0=0,vB1=0;   // top / centre / bottom row spans
    float bl=0, bt=0, br=0, bb=0;                  // dest border insets (px)
};
void DrawNineSlice(const NineSlice& s, V2 min, V2 max, uint32_t col = 0xFFFFFFFFu, bool additive = false);

// The standard Sonic Unleashed silver-chrome window frame, 9-slice from
// mat_result_comon_001 (1024x512). GameFrameTex() lazy-loads + caches the shared
// atlas (assets/common, falling back to shop/status); GameWindow() builds its
// NineSlice. DrawGameWindow draws a bounded chrome panel (body + brighter header).
int       GameFrameTex();
NineSlice GameWindow(int tex);
void      DrawGameWindow(V2 min, V2 max, float headerH, float alpha,
                         uint32_t bodyTint = 0xFF4A608C, uint32_t headerTint = 0xFF96B2D6);

// ---- recomp-style polish (approximated procedurally in the game-chrome style) --
// Breathe: a sine pulse in [lo,hi] with period `rateSec` — for pulsing prompts.
float    Breathe(double nowSec, float lo, float hi, float rateSec);
// Grayscale: luma-desaturate a colour (keeps alpha) — for locked/disabled items.
uint32_t Grayscale(uint32_t col);
// Scanlines: faint horizontal CRT lines across a region (drawn on top, subtle).
void     DrawScanlines(V2 min, V2 max, uint32_t lineCol, float spacingPx = 3.0f, float thickness = 1.0f);
// Beveled text: an embossed look — dark lower-right + light upper-left + the face.
void     DrawTextBevel(V2 pos, float pxSize, uint32_t col, const char* text,
                       uint32_t loCol = 0xC00A1018u, uint32_t hiCol = 0x80FFFFFFu);

// ---- text (stb_truetype atlas) ---------------------------------------------
enum class Align { Left, Center, Right };
V2   MeasureText(float pxSize, const char* text);
void DrawText(V2 pos, float pxSize, uint32_t col, const char* text);     // pos = top-left
void DrawTextShadow(V2 pos, float pxSize, uint32_t col, const char* text,
                    float offset = 2.0f, uint32_t shadowCol = 0xFF000000u);
void DrawTextAligned(V2 min, V2 max, float pxSize, uint32_t col, const char* text,
                     Align h = Align::Center, bool vCenter = true, bool shadow = true);

// Italic shear (state, like SetFont): each glyph leans right by `xPerY` px of x
// per px of height above the baseline. The game's chrome wordmarks use ~0.24.
void SetTextShear(float xPerY);
void ResetTextShear();
// Horizontal stretch (state): scales the whole string (glyphs AND gaps) about its
// pen-start x. The game's chrome wordmark faces are much wider than DFSoGei (~1.4).
void SetTextStretchX(float k);
void ResetTextStretchX();
// Vertical-gradient text — the chrome-wordmark fill (colour interpolated top->
// bottom over the string's glyph band). Obeys the current font + shear.
void DrawTextGradient(V2 pos, float pxSize, uint32_t colTop, uint32_t colBottom, const char* text);
// Solid quad from 4 explicit corners (TL,TR,BR,BL) with per-corner colours —
// gradient fills on slanted shapes (title banners).
void DrawQuadGradient(const V2 corners[4], const uint32_t cols[4], bool additive = false);
// Textured image with a vertical tint gradient (top colour -> bottom colour) —
// shaded bitmap-font glyphs and sprite fills.
void DrawImageVGradient(int tex, V2 min, V2 max, V2 uv0, V2 uv1,
                        uint32_t colTop, uint32_t colBottom, bool additive = false);
// The game's screen-wipe transition (measured from the live gate->loading
// capture): staggered horizontal black bands sweep across in alternating
// directions, each led by an arrowhead tip with a translucent shard ahead.
// progress 0 = clear, 1 = fully covered. Works symmetrically for in/out.
void DrawChevronWipe(float progress);

// ---- shared button-guide footer, ported 1:1 from ui/button_guide.cpp --------
// Region y618..720; icons 40x40 (LB/RB 70x40) from the controller glyph atlas;
// labels NewRodin 21.8 white + 4px black outline. Left-aligned buttons flow from
// the left margin, right-aligned flow inward from the right. maxWidth squashes a
// label that would otherwise overrun (FLT_MAX / 0 = no clamp). The canonical guide
// used by every screen's footer (options uses sideMargins 250).
enum class GIcon { A, B, X, Y, LB, RB, LBRB };
enum class GAlign { Left, Right };
struct GuideBtn { const char* label; GIcon icon; GAlign align; float maxWidth; };
void DrawButtonGuide(const GuideBtn* btns, int count, int rodinFont, float alpha, float sideMargins = 379.0f);

} // namespace ui
