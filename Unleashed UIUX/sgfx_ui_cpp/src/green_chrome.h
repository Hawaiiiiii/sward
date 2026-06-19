#pragma once
#include "sgfxui.h"

// Shared "cinematic green" operator-console chrome — the container panels, value
// plates, scanline backdrop, palette and value/selection primitives used by the
// settings-family screens (options, setup, ...). One source of truth so the family
// reads as one console. Drawn from primitives + the bundled fonts only (no game art).
namespace chrome {
using namespace ui;

constexpr float GRID = 9.0f;

inline const uint32_t C_TITLE     = RGBA(255, 190, 33, 255);
inline const uint32_t C_PANEL_BG  = RGBA(0, 0, 0, 223);
inline const uint32_t C_OUTER     = RGBA(0, 49, 0, 255);
inline const uint32_t C_INNER     = RGBA(0, 33, 0, 255);
inline const uint32_t C_LINE      = RGBA(0, 89, 0, 255);
inline const uint32_t C_SEL_TL    = RGBA(226, 113, 34, 128);   // selected-row diagonal (gold)
inline const uint32_t C_SEL_BR    = RGBA(146, 255, 49, 128);   // -> green
inline const uint32_t C_LABEL     = RGBA(255, 255, 255, 255);
inline const uint32_t C_VAL_G_T   = RGBA(192, 255, 0, 255);    // value text gradient top
inline const uint32_t C_VAL_G_B   = RGBA(128, 170, 0, 255);
inline const uint32_t C_LIGHT_ON  = RGBA(214, 255, 64, 255);
inline const uint32_t C_LIGHT_OFF = RGBA(30, 52, 30, 255);
inline const uint32_t C_BLACK     = RGBA(0, 0, 0, 255);
inline const uint32_t C_DESC      = RGBA(255, 255, 255, 255);
inline const uint32_t C_FOOTER    = RGBA(206, 226, 206, 220);
inline const uint32_t C_BG        = RGBA(2, 6, 3, 255);
inline const uint32_t C_WHITE     = RGBA(255, 255, 255, 255);
inline const uint32_t C_OK        = RGBA(146, 255, 49, 255);   // found / pass
inline const uint32_t C_WARN      = RGBA(255, 192, 0, 255);    // needs action
inline const uint32_t C_DIM       = RGBA(120, 150, 120, 255);  // not set / muted

// staged-entrance alphas from the open elapsed time (line -> outer -> inner -> bg,
// the title Hermite, and `t` for content once the panel has settled).
struct Build { float line, outer, inner, bg, title, t; };
Build Stage(double openSec);

// deep base + top/bottom scanline bands + dividers + the gold bevel title.
void Backdrop(int titleFont, float titleT, const char* title);

// a green container panel (staged build). rightOutline = solid right edge.
void Container(float x0, float y0, float x1, float y1, bool rightOutline,
               float lineT, float outerT, float innerT, float bgT);

// the 3-layer green plate (value cells, tab backings) under the scanline shader.
void Plate(float x0, float y0, float x1, float y1, float a);

// value text: white base + black outline + green vertical gradient (uses current font).
void ValueText(const char* s, float x0, float y0, float x1, float y1, float t);

// magenta selection arrows flanking a value box.
void SelectionArrows(float bx0, float by0, float bx1, float by1, float t);

// a primitive toggle light: a lit/dark square with an additive glow when on.
void ToggleLight(float lx, float ly, float ls, bool on, float t);

} // namespace chrome
