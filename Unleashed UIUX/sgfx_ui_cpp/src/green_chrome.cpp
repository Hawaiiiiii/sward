// =============================================================================
// green_chrome.cpp — the shared cinematic-green chrome (see green_chrome.h). The
// container build, value plates, scanline backdrop and value/selection primitives
// the settings-family screens share, so the look lives in one place.
// =============================================================================
#include "green_chrome.h"

#include <algorithm>
#include <cmath>

namespace chrome {

static const uint32_t C_GREEN_GLOW = RGBA(203, 255, 0, 55);
static const uint32_t C_DIV_HI     = RGBA(222, 255, 189, 65);
static const uint32_t C_DIV_LO     = RGBA(173, 255, 156, 65);
static const uint32_t C_DIV_CORE   = RGBA(115, 178, 104, 255);

Build Stage(double openSec) {
    Build b;
    b.line  = (float)ComputeMotion(openSec, 0.0,  8.0);
    b.outer = (float)ComputeMotion(openSec, 16.0, 8.0);
    b.inner = (float)ComputeMotion(openSec, 32.0, 8.0);
    b.bg    = (float)ComputeMotion(openSec, 48.0, 12.0);
    b.title = Hermite(0.0f, 1.0f, (float)ComputeMotion(openSec, 3.0, 28.0));
    b.t     = (float)ComputeMotion(openSec, 50.0, 12.0);
    return b;
}

void Backdrop(int titleFont, float titleT, const char* title) {
    DrawRect({ 0, 0 }, { REF_W, REF_H }, C_BG);
    auto band = [&](float yTop, float yBot, bool top) {
        if (top) DrawVGradient({ 0, yTop }, { REF_W, yBot }, RGBA(0,0,0,255), RGBA(0,0,0,0));
        else     DrawVGradient({ 0, yTop }, { REF_W, yBot }, RGBA(0,0,0,0), RGBA(0,0,0,255));
        SetModifier(MOD_SCANLINE);
        if (top) DrawVGradient({ 0, yTop }, { REF_W, yBot }, RGBA(203,255,0,0), C_GREEN_GLOW);
        else     DrawVGradient({ 0, yTop }, { REF_W, yBot }, C_GREEN_GLOW, RGBA(203,255,0,0));
        ResetModifier();
    };
    band(0, 105, true); band(615, 720, false);
    auto divider = [&](float y) {
        DrawRect({ 0, y-2 }, { REF_W, y }, C_DIV_HI);
        DrawRect({ 0, y+1 }, { REF_W, y+3 }, C_DIV_LO);
        DrawRect({ 0, y }, { REF_W, y+1 }, C_DIV_CORE);
    };
    divider(105); divider(615);
    SetFont(titleFont);
    DrawTextBevel({ 122, 56 }, 48.0f, WithAlpha(C_TITLE, titleT), title);
    ResetFont();
}

void Container(float x0, float y0, float x1, float y1, bool rightOutline,
               float lineT, float outerT, float innerT, float bgT) {
    PushTransform(1.0f, lineT, { (x0 + x1) * 0.5f, (y0 + y1) * 0.5f }, { 0.0f, 0.0f });
    DrawRect({ x0, y0 }, { x1, y1 }, WithAlpha(C_PANEL_BG, bgT));
    SetModifier(MOD_CHECKERBOARD);
    DrawRect({ x0, y0 + GRID }, { x0 + GRID, y1 - GRID }, WithAlpha(C_OUTER, outerT));
    DrawRect({ x1 - GRID, y0 + GRID }, { x1, y1 - GRID }, WithAlpha(rightOutline ? C_OUTER : C_INNER, outerT));
    DrawRect({ x0, y0 }, { x1, y0 + GRID }, WithAlpha(C_OUTER, outerT));
    DrawRect({ x0, y1 - GRID }, { x1, y1 }, WithAlpha(C_OUTER, outerT));
    DrawRect({ x0 + GRID, y0 + GRID }, { x1 - GRID, y1 - GRID }, WithAlpha(C_INNER, innerT));
    ResetModifier();
    uint32_t lc = WithAlpha(C_LINE, lineT); const float g = GRID, L = 2.0f;
    DrawRect({ x0+g, y0+g }, { x0+g+L, y0+g*2 }, lc); DrawRect({ x0+g, y0+g }, { x1-g, y0+g+L }, lc); DrawRect({ x1-g-L, y0+g }, { x1-g, y0+g*2 }, lc);
    DrawRect({ x0+g, y1-g*2 }, { x0+g+L, y1-g }, lc); DrawRect({ x0+g, y1-g-L }, { x1-g, y1-g }, lc); DrawRect({ x1-g-L, y1-g*2 }, { x1-g, y1-g }, lc);
    PopTransform();
}

void Plate(float x0, float y0, float x1, float y1, float a) {
    auto A = [&](int base) { return (uint8_t)std::clamp((int)lround(base * a), 0, 255); };
    SetModifier(MOD_SCANLINE_BUTTON);
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(0,130,0,A(223)), RGBA(0,130,0,A(178)), RGBA(0,130,0,A(223)), RGBA(0,130,0,A(178)));
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(0,0,0,A(13)),    RGBA(0,0,0,0),         RGBA(0,0,0,A(55)),    RGBA(0,0,0,A(6)));
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(0,130,0,A(13)),  RGBA(0,130,0,A(111)),  RGBA(0,130,0,0),      RGBA(0,130,0,A(55)));
    ResetModifier();
}

static void FillTri(float baseX, float apexX, float y0, float y1, uint32_t cBase, uint32_t cApex, bool add = false) {
    const float cy = (y0 + y1) * 0.5f;
    const V2 c[4] = { { baseX, y0 }, { baseX, y1 }, { apexX, cy }, { apexX, cy } };
    const uint32_t cols[4] = { cBase, cBase, cApex, cApex };
    DrawQuadGradient(c, cols, add);
}

void SelectionArrows(float bx0, float by0, float bx1, float by1, float t) {
    const float pad = GRID, width = GRID * 2.5f;
    uint32_t base = WithAlpha(RGBA(0,97,0,255), t);
    uint32_t m0 = WithAlpha(RGBA(255,0,255,255), t), m1 = WithAlpha(RGBA(255,128,255,255), t);
    FillTri(bx0-pad, bx0-pad-width, by0, by1, base, base);
    FillTri(bx0-pad, bx0-pad-width, by0, by1, m0, m1, true);
    FillTri(bx1+pad, bx1+pad+width, by0, by1, base, base);
    FillTri(bx1+pad, bx1+pad+width, by0, by1, m0, m1, true);
}

void ValueText(const char* s, float x0, float y0, float x1, float y1, float t) {
    const float boxW = x1 - x0; float w = MeasureText(20.0f, s).x; float sx = 1.0f;
    if (w > boxW && w > 0.0f) sx = boxW / w;
    float dw = w * sx;
    float px = x0 + (boxW - dw) * 0.5f, py = y0 + ((y1 - y0) - 20.0f) * 0.5f;
    if (sx != 1.0f) SetTextStretchX(sx);
    static const float O[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
    for (auto& o : O) DrawText({ px + o[0]*1.6f, py + o[1]*1.6f }, 20.0f, WithAlpha(C_BLACK, t), s);
    DrawTextGradient({ px, py }, 20.0f, WithAlpha(C_VAL_G_T, t), WithAlpha(C_VAL_G_B, t), s);
    if (sx != 1.0f) ResetTextStretchX();
}

void ToggleLight(float lx, float ly, float ls, bool on, float t) {
    if (on) {
        const float gs = ls + 12.0f, gx = lx - 6.0f, gy = ly - 6.0f;
        DrawRect({ gx, gy }, { gx + gs, gy + gs }, WithAlpha(RGBA(255,255,0,70), t), true);
    }
    DrawRect({ lx-1, ly-1 }, { lx+ls+1, ly+ls+1 }, WithAlpha(C_BLACK, t));
    DrawRect({ lx, ly }, { lx+ls, ly+ls }, WithAlpha(on ? C_LIGHT_ON : C_LIGHT_OFF, t));
}

} // namespace chrome
