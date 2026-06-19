// =============================================================================
// screen_options.cpp — the settings screen in the cinematic green look: a tabbed
// container (RUN / REVIEW / LINKS / DISPLAY), a scrolling option list with value
// plates, toggle lights and sliders, and a right info panel with a description.
// The green container / plates / scanline shading / gradients / staged entrance are
// the original menu's look, drawn entirely from primitives + the kept fonts — no
// game sprite art (the toggle light, the info slot and the footer are primitives /
// text, so nothing game-derived ships). The options are the tool's own, profile-
// agnostic settings; values persist to sgfx_settings.ini.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "settings.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace ui;

namespace {

enum Kind { TOGGLE, ENUM, SLIDER };
struct Option { const char* label; Kind kind; const char* const* choices; int choiceCount; int val;
                int sMin, sMax, sStep; const char* suffix; const char* desc; };

const char* const CH_PROF[]  = { "Auto-detect", "Ask each run" };
const char* const CH_FMT[]   = { "JSON", "JSON + HTML" };
const char* const CH_THEME[] = { "Dark", "Dark high-contrast" };

struct Category { const char* name; Option* opts; int optCount; };

Option OPTS_RUN[] = {
    { "Active profile",     ENUM,   CH_PROF, 2, 0, 0,0,0,"",  "Which car slice a run targets. Auto-detect reads it from the run, so nothing is fixed to one model.", },
    { "Report format",      ENUM,   CH_FMT,  2, 0, 0,0,0,"",  "What each run writes for the record.", },
    { "Parallel runs",      SLIDER, nullptr, 0, 1, 1,8,1,"",  "How many profiles to preflight at once.", },
    { "Screenshot battery", TOGGLE, nullptr, 0, 1, 0,0,0,"",  "Capture the standard set of test views on every run.", },
    { "Baseline compare",   TOGGLE, nullptr, 0, 1, 0,0,0,"",  "Diff new screenshots against the last accepted baseline.", },
};
Option OPTS_REVIEW[] = {
    { "Auto-open report",      TOGGLE, nullptr, 0, 0, 0,0,0,"",  "Open the report as soon as a run finishes.", },
    { "Desktop notifications", TOGGLE, nullptr, 0, 1, 0,0,0,"",  "Notify you when a run or capture completes.", },
    { "Confirm before delivery", TOGGLE, nullptr, 0, 1, 0,0,0,"","Ask for confirmation before a delivery check goes ahead.", },
    { "Diff sensitivity",      SLIDER, nullptr, 0, 60, 0,100,5,"%", "How strict the screenshot comparison is. Higher flags smaller changes.", },
};
Option OPTS_LINKS[] = {
    { "BMW Git fetch",      TOGGLE, nullptr, 0, 1, 0,0,0,"",  "Fetch the latest car-models before a run.", },
    { "Confluence sync",    TOGGLE, nullptr, 0, 0, 0,0,0,"",  "Pull workflow references from the team space.", },
    { "Weekly ticket draft",TOGGLE, nullptr, 0, 0, 0,0,0,"",  "Draft the weekly ticket list for you to review and send. Nothing is posted automatically.", },
    { "Cloud upload",       TOGGLE, nullptr, 0, 0, 0,0,0,"",  "Results stay on this machine. When on, runs are uploaded off this machine.", },
    { "AI assist",          TOGGLE, nullptr, 0, 0, 0,0,0,"",  "Heuristics run by default. When on, AI is used and data is sent to the cloud for analysis.", },
};
Option OPTS_DISPLAY[] = {
    { "Theme",            ENUM,   CH_THEME, 2, 0, 0,0,0,"",  "Dark IDE styling. There is no light mode.", },
    { "Reduce motion",    TOGGLE, nullptr,  0, 0, 0,0,0,"",  "Tone down the animations and transitions.", },
    { "Verbose logging",  TOGGLE, nullptr,  0, 0, 0,0,0,"",  "Write extra detail to the session log for troubleshooting.", },
    { "Check for updates",TOGGLE, nullptr,  0, 1, 0,0,0,"",  "Look for a newer build of the tool on launch.", },
};
Category CATEGORIES[] = {
    { "RUN", OPTS_RUN, 5 }, { "REVIEW", OPTS_REVIEW, 4 }, { "LINKS", OPTS_LINKS, 5 }, { "DISPLAY", OPTS_DISPLAY, 4 },
};
constexpr int CATEGORY_COUNT = 4;

// ---- geometry (1280x720) ----------------------------------------------------
constexpr float GRID = 9.0f;
constexpr float SP_X0 = 33, SP_Y0 = 117, SP_X1 = 843, SP_Y1 = 604;
constexpr float IP_X0 = 868, IP_Y0 = 117, IP_X1 = 1246, IP_Y1 = 604;
constexpr float CLIP_X = SP_X0 + GRID * 2;
constexpr float TABS_Y = SP_Y0 + GRID * 2;
constexpr float TAB_H  = GRID * 4;
constexpr float ROWS_TOP = SP_Y0 + GRID * 2 + GRID * 6;
constexpr float ROW_PITCH = GRID * 6;
constexpr float ROW_H   = GRID * 5.5f;
constexpr float OPT_W   = GRID * 54;
constexpr float LABEL_X = SP_X0 + GRID * 2 + GRID;
constexpr float VAL_W = 192, VAL_H = GRID * 3;
constexpr float VAL_X0 = (SP_X0 + GRID * 2) + OPT_W + ((SP_X1 - GRID * 2) - (SP_X0 + GRID * 2) - OPT_W - VAL_W) / 2.0f - 4.0f;

// ---- colours (the original menu's IM_COL32 values) --------------------------
const uint32_t C_TITLE     = RGBA(255, 190, 33, 255);
const uint32_t C_PANEL_BG  = RGBA(0, 0, 0, 223);
const uint32_t C_OUTER     = RGBA(0, 49, 0, 255);
const uint32_t C_INNER     = RGBA(0, 33, 0, 255);
const uint32_t C_LINE      = RGBA(0, 89, 0, 255);
const uint32_t C_TAB_G_T   = RGBA(128, 255, 0, 255);
const uint32_t C_TAB_G_B   = RGBA(255, 192, 0, 255);
const uint32_t C_SEL_TL    = RGBA(226, 113, 34, 128);
const uint32_t C_SEL_BR    = RGBA(146, 255, 49, 128);
const uint32_t C_LABEL     = RGBA(255, 255, 255, 255);
const uint32_t C_VAL_G_T   = RGBA(192, 255, 0, 255);
const uint32_t C_VAL_G_B   = RGBA(128, 170, 0, 255);
const uint32_t C_LIGHT_ON  = RGBA(214, 255, 64, 255);
const uint32_t C_LIGHT_OFF = RGBA(30, 52, 30, 255);
const uint32_t C_BLACK     = RGBA(0, 0, 0, 255);
const uint32_t C_DESC      = RGBA(255, 255, 255, 255);
const uint32_t C_GREEN_GLOW= RGBA(203, 255, 0, 55);
const uint32_t C_DIV_HI    = RGBA(222, 255, 189, 65);
const uint32_t C_DIV_LO    = RGBA(173, 255, 156, 65);
const uint32_t C_DIV_CORE  = RGBA(115, 178, 104, 255);
const uint32_t C_BG        = RGBA(2, 6, 3, 255);
const uint32_t C_WHITE     = RGBA(255, 255, 255, 255);
const uint32_t C_FOOTER    = RGBA(206, 226, 206, 220);

// ---- state ------------------------------------------------------------------
int    g_cat = 0, g_sel = 0, g_prevSel = 0, g_first = 0;
double g_moveStart = -100.0;
constexpr int   VIS_ROWS = 7;
constexpr float ROWS_CLIP_BOT = 589.0f;
int    g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

Category& Cat()      { return CATEGORIES[g_cat]; }
int       OptCount() { return Cat().optCount; }
Option&   Opt(int i) { return Cat().opts[i]; }
int       ValMin(const Option& o) { return o.kind == SLIDER ? o.sMin : 0; }
int       ValMax(const Option& o) { return o.kind == TOGGLE ? 1 : (o.kind == SLIDER ? o.sMax : std::max(0, o.choiceCount - 1)); }
int       ValStep(const Option& o) { return o.kind == SLIDER ? std::max(1, o.sStep) : 1; }
void      KeepSelVisible() {
    if (g_sel < g_first) g_first = g_sel;
    if (g_sel > g_first + VIS_ROWS - 1) g_first = g_sel - VIS_ROWS + 1;
    g_first = std::clamp(g_first, 0, std::max(0, OptCount() - VIS_ROWS));
}

void LoadSavedValues() {
    char key[48];
    for (int c = 0; c < CATEGORY_COUNT; ++c)
        for (int i = 0; i < CATEGORIES[c].optCount; ++i) {
            Option& o = CATEGORIES[c].opts[i];
            snprintf(key, sizeof key, "optg_%d_%d", c, i);
            o.val = std::clamp(settings::GetInt(key, o.val), ValMin(o), ValMax(o));
        }
}
void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
    static bool loaded = false;
    if (!loaded) { LoadSavedValues(); loaded = true; }
}
void Reset() { g_cat = 0; g_sel = 0; g_prevSel = 0; g_first = 0; g_moveStart = -100.0; }
void Input(const ScreenInput& in) {
    if (in.up || in.down) { g_prevSel = g_sel; if (in.up) g_sel = std::max(0, g_sel-1); if (in.down) g_sel = std::min(OptCount()-1, g_sel+1); g_moveStart = Now(); KeepSelVisible(); }
    if (in.left || in.right) {
        Option& o = Opt(std::clamp(g_sel,0,OptCount()-1));
        o.val = std::clamp(o.val + (in.right?ValStep(o):-ValStep(o)), ValMin(o), ValMax(o));
        char key[48]; snprintf(key, sizeof key, "optg_%d_%d", g_cat, std::clamp(g_sel,0,OptCount()-1));
        settings::SetInt(key, o.val);
    }
    if (in.tabLeft || in.tabRight) { g_cat = (g_cat + (in.tabRight?1:CATEGORY_COUNT-1)) % CATEGORY_COUNT; g_sel = g_prevSel = 0; g_first = 0; g_moveStart = Now(); KeepSelVisible(); }
}

// the green container panel: staged build (line -> outer ring -> inner fill -> bg).
void DrawContainer(float x0, float y0, float x1, float y1, bool rightOutline,
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

// shared 3-layer green plate (active tab + value cell), under the scanline shader.
void DrawPlate(float x0, float y0, float x1, float y1, float a) {
    auto A = [&](int base) { return (uint8_t)std::clamp((int)lround(base * a), 0, 255); };
    SetModifier(MOD_SCANLINE_BUTTON);
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(0,130,0,A(223)), RGBA(0,130,0,A(178)), RGBA(0,130,0,A(223)), RGBA(0,130,0,A(178)));
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(0,0,0,A(13)),    RGBA(0,0,0,0),         RGBA(0,0,0,A(55)),    RGBA(0,0,0,A(6)));
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(0,130,0,A(13)),  RGBA(0,130,0,A(111)),  RGBA(0,130,0,0),      RGBA(0,130,0,A(55)));
    ResetModifier();
}

void FillTri(float baseX, float apexX, float y0, float y1, uint32_t cBase, uint32_t cApex, bool add = false) {
    const float cy = (y0 + y1) * 0.5f;
    const V2 c[4] = { { baseX, y0 }, { baseX, y1 }, { apexX, cy }, { apexX, cy } };
    const uint32_t cols[4] = { cBase, cBase, cApex, cApex };
    DrawQuadGradient(c, cols, add);
}
void DrawSelectionArrows(float bx0, float by0, float bx1, float by1, float t) {
    const float pad = GRID, width = GRID * 2.5f;
    uint32_t base = WithAlpha(RGBA(0,97,0,255), t);
    uint32_t m0 = WithAlpha(RGBA(255,0,255,255), t), m1 = WithAlpha(RGBA(255,128,255,255), t);
    FillTri(bx0-pad, bx0-pad-width, by0, by1, base, base);
    FillTri(bx0-pad, bx0-pad-width, by0, by1, m0, m1, true);
    FillTri(bx1+pad, bx1+pad+width, by0, by1, base, base);
    FillTri(bx1+pad, bx1+pad+width, by0, by1, m0, m1, true);
}

// value text: white base + black outline + green vertical gradient fill
void DrawValueText(const char* s, float x0, float y0, float x1, float y1, float t) {
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

// tab label: lime->gold gradient + black outline + bevel
void DrawTabText(float x, float y, const char* s, float alpha) {
    uint8_t a = (uint8_t)std::clamp((int)lround(alpha), 0, 255);
    static const float O[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
    for (auto& o : O) DrawText({ x + o[0]*1.6f, y + o[1]*1.6f }, 32.0f, RGBA(0,0,0,a), s);
    SetModifier(MOD_TITLE_BEVEL);
    DrawTextGradient({ x, y }, 32.0f, WithAlpha(C_TAB_G_T, a/255.0f), WithAlpha(C_TAB_G_B, a/255.0f), s);
    ResetModifier();
}

// primitive toggle light (no sprite): a small lit/dark square, additive glow when on.
void DrawToggleLight(float lx, float ly, float ls, bool on, float t) {
    if (on) {
        const float gs = ls + 12.0f, gx = lx - 6.0f, gy = ly - 6.0f;
        DrawRect({ gx, gy }, { gx + gs, gy + gs }, WithAlpha(RGBA(255,255,0,70), t), true);
    }
    DrawRect({ lx-1, ly-1 }, { lx+ls+1, ly+ls+1 }, WithAlpha(C_BLACK, t));
    DrawRect({ lx, ly }, { lx+ls, ly+ls }, WithAlpha(on ? C_LIGHT_ON : C_LIGHT_OFF, t));
}

void Draw(double openSec) {
    const float cLine  = (float)ComputeMotion(openSec, 0.0,  8.0);
    const float cOuter = (float)ComputeMotion(openSec, 16.0, 8.0);
    const float cInner = (float)ComputeMotion(openSec, 32.0, 8.0);
    const float cBg    = (float)ComputeMotion(openSec, 48.0, 12.0);
    const float titleT = Hermite(0.0f, 1.0f, (float)ComputeMotion(openSec, 3.0, 28.0));
    const float t      = (float)ComputeMotion(openSec, 50.0, 12.0);
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

    SetFont(g_fDF);
    DrawTextBevel({ 122, 56 }, 48.0f, WithAlpha(C_TITLE, titleT), "SETTINGS");

    DrawContainer(SP_X0, SP_Y0, SP_X1, SP_Y1, true,  cLine, cOuter, cInner, cBg);
    DrawContainer(IP_X0, IP_Y0, IP_X1, IP_Y1, false, cLine, cOuter, cInner, cBg);

    // tabs (RUN/REVIEW/LINKS/DISPLAY)
    {
        SetFont(g_fDF);
        float clipW = (SP_X1 - GRID*2) - CLIP_X;
        float widths[CATEGORY_COUNT], sum = 0;
        for (int i = 0; i < CATEGORY_COUNT; ++i) { widths[i] = MeasureText(32.0f, CATEGORIES[i].name).x; sum += widths[i]; }
        float pad = (clipW - sum) / (CATEGORY_COUNT + 1);
        float x = CLIP_X + pad;
        for (int i = 0; i < CATEGORY_COUNT; ++i) {
            if (i == g_cat) { float tabPad = std::min(pad * 0.5f, GRID * 3); DrawPlate(x - tabPad, TABS_Y, x + widths[i] + tabPad, TABS_Y + TAB_H, t); }
            x += widths[i] + pad;
        }
        x = CLIP_X + pad;
        for (int i = 0; i < CATEGORY_COUNT; ++i) { DrawTabText(x, TABS_Y, CATEGORIES[i].name, (i == g_cat ? 235.0f : 128.0f) * t); x += widths[i] + pad; }
    }

    // rows
    PushClip({ CLIP_X - 2, ROWS_TOP - 2 }, { SP_X1 - GRID, ROWS_CLIP_BOT });
    {
        float mt = (float)ComputeMotion(g_moveStart, 0.0, 8.0);
        float slot = Lerp((float)g_prevSel, (float)g_sel, mt) - (float)g_first;
        float ry = ROWS_TOP + slot * ROW_PITCH;
        uint32_t gold = WithAlpha(C_SEL_TL, t), grn = WithAlpha(C_SEL_BR, t);
        uint32_t mid  = WithAlpha(ColourLerp(C_SEL_TL, C_SEL_BR, 0.5f), t);
        DrawQuadGradient({ CLIP_X, ry }, { CLIP_X + OPT_W, ry + ROW_H }, gold, mid, grn, mid);
    }
    for (int i = g_first; i < OptCount() && i <= g_first + VIS_ROWS; ++i) {
        const Option& o = Opt(i);
        float ry = ROWS_TOP + (i - g_first) * ROW_PITCH;
        bool sel = (i == g_sel);
        SetFont(g_fSeurat);
        DrawTextAligned({ LABEL_X, ry }, { VAL_X0 - 10, ry + ROW_H }, 26.0f, WithAlpha(C_LABEL, t), o.label, Align::Left, true, true);
        float vy0 = ry + (ROW_H - VAL_H) * 0.5f, vy1 = vy0 + VAL_H;
        DrawPlate(VAL_X0, vy0, VAL_X0 + VAL_W, vy1, t);
        if (sel) DrawSelectionArrows(VAL_X0, vy0, VAL_X0 + VAL_W, vy1, t);
        SetFont(g_fRodin);
        if (o.kind == TOGGLE) {
            bool onv = o.val != 0;
            const float ls = 15.0f, lx = VAL_X0 + 14.0f, ly = vy0 + ((VAL_H - ls) * 0.5f);
            DrawToggleLight(lx, ly, ls, onv, t);
            DrawValueText(onv ? "ON" : "OFF", VAL_X0 + 6, vy0, VAL_X0 + VAL_W - 6, vy1, t);
        } else if (o.kind == SLIDER) {
            float factor = (o.sMax > o.sMin) ? (float)(o.val - o.sMin) / (float)(o.sMax - o.sMin) : 0.0f;
            float cx0 = VAL_X0 + 6, cy0 = vy0 + 3, cx1 = VAL_X0 + VAL_W - 6, cy1 = vy1 - 3;
            SetModifier(MOD_SCANLINE_BUTTON);
            DrawQuadGradient({cx0,cy0},{cx1,cy1}, WithAlpha(RGBA(0,65,0,255),t), WithAlpha(RGBA(0,65,0,255),t), WithAlpha(RGBA(0,32,0,255),t), WithAlpha(RGBA(0,32,0,255),t));
            float fx0 = cx0 + 2, fy0 = cy0 + 2, fy1 = cy1 - 2, fx1 = fx0 + ((cx1 - 2) - fx0) * factor;
            if (fx1 > fx0 + 0.5f)
                DrawQuadGradient({fx0,fy0},{fx1,fy1}, WithAlpha(RGBA(57,241,0,255),t), WithAlpha(RGBA(57,241,0,255),t), WithAlpha(RGBA(2,106,0,255),t), WithAlpha(RGBA(2,106,0,255),t));
            ResetModifier();
            char buf[16]; snprintf(buf, sizeof buf, "%d%s", o.val, o.suffix ? o.suffix : "");
            DrawValueText(buf, VAL_X0 + 6, vy0, VAL_X0 + VAL_W - 6, vy1, t);
        } else {
            const char* s = (o.choices && o.val < o.choiceCount) ? o.choices[o.val] : "";
            DrawValueText(s, VAL_X0 + 6, vy0, VAL_X0 + VAL_W - 6, vy1, t);
        }
    }
    PopClip();
    if (OptCount() > VIS_ROWS) {
        float totalH = (ROWS_CLIP_BOT - ROWS_TOP) - 2.0f;
        float hr = (float)VIS_ROWS / (float)OptCount();
        float minY = ((float)g_first / (float)OptCount()) * totalH + ROWS_TOP;
        DrawRect({ SP_X1 - GRID*2, minY }, { SP_X1 - GRID - 1, minY + totalH*hr }, WithAlpha(RGBA(0,128,0,255), t));
    }

    // info panel: a slot showing the focused option's name + value, then its description
    {
        const float ix0 = IP_X0 + GRID*2, ix1 = IP_X1 - GRID*2;
        const float wrapW = ix1 - ix0;
        const float thumbH = wrapW * 9.0f / 16.0f;
        const float ty0 = IP_Y0 + GRID*2 + GRID*0.5f, ty1 = ty0 + thumbH;
        const Option& s = Opt(std::clamp(g_sel, 0, OptCount() - 1));
        // slot (primitive, green-framed)
        DrawRect({ ix0, ty0 }, { ix1, ty1 }, WithAlpha(RGBA(4,12,6,235), t));
        DrawVGradient({ ix0, ty0 }, { ix1, ty1 }, WithAlpha(RGBA(0,40,0,70), t), WithAlpha(RGBA(0,0,0,90), t));
        uint32_t kl = WithAlpha(RGBA(0,89,0,200), t); const float L = 1.0f;
        DrawRect({ ix0, ty0 }, { ix1, ty0+L }, kl); DrawRect({ ix0, ty1-L }, { ix1, ty1 }, kl);
        DrawRect({ ix0, ty0 }, { ix0+L, ty1 }, kl); DrawRect({ ix1-L, ty0 }, { ix1, ty1 }, kl);
        // current state, large + centred in the slot
        char val[24];
        if (s.kind == TOGGLE)      snprintf(val, sizeof val, "%s", s.val ? "ON" : "OFF");
        else if (s.kind == SLIDER) snprintf(val, sizeof val, "%d%s", s.val, s.suffix ? s.suffix : "");
        else                       snprintf(val, sizeof val, "%s", (s.choices && s.val < s.choiceCount) ? s.choices[s.val] : "");
        SetFont(g_fDF);
        DrawTextAligned({ ix0, ty0 + 18 }, { ix1, ty0 + 46 }, 16.0f, WithAlpha(RGBA(150,190,150,255), t), s.label, Align::Center, true, true);
        DrawValueText(val, ix0, ty0 + thumbH*0.45f - 4, ix1, ty0 + thumbH*0.45f + 30, t);

        SetFont(g_fSeurat);
        std::string full = s.desc ? s.desc : "";
        const float fsz = 26.0f, lineH = fsz + 5.0f;
        std::vector<std::string> lines;
        std::string cur; size_t w0 = 0;
        while (w0 <= full.size()) {
            size_t w1 = full.find(' ', w0); if (w1 == std::string::npos) w1 = full.size();
            std::string word = full.substr(w0, w1 - w0);
            std::string trial = cur.empty() ? word : cur + " " + word;
            if (!cur.empty() && MeasureText(fsz, trial.c_str()).x > wrapW) { lines.push_back(cur); cur = word; }
            else cur = trial;
            if (w1 >= full.size()) break;
            w0 = w1 + 1;
        }
        if (!cur.empty()) lines.push_back(cur);
        float dy = ty1 + 24.0f;
        for (const std::string& ln : lines) {
            float lw = MeasureText(fsz, ln.c_str()).x;
            DrawText({ ix0 + (wrapW - lw) * 0.5f, dy }, fsz, WithAlpha(C_DESC, t), ln.c_str());
            dy += lineH;
        }
    }

    // footer (text; no glyph atlas)
    SetFont(g_fRodin);
    DrawText({ 250, 662 }, 20.0f, WithAlpha(C_FOOTER, t), "LB/RB  Switch tab");
    DrawText({ 470, 662 }, 20.0f, WithAlpha(C_FOOTER, t), "Left/Right  Change");
    DrawText({ 700, 662 }, 20.0f, WithAlpha(C_FOOTER, t), "Esc  Back");
    ResetFont();
}

} // namespace

void OptionsInit() { Init(); }
void OptionsDraw(double openSeconds) { Draw(openSeconds); }
void OptionsInput(const ScreenInput& in) { Input(in); }
void OptionsReset() { Reset(); }
