// =============================================================================
// screen_options.cpp — the Options menu, ported PIXEL-EXACT from UnleashedRecomp's
// own UI source (ui/options_menu.cpp + ui/imgui_utils.cpp), the literal code that
// drew the captured video (v1.0.3 HEAD). Every rect/colour/size below is transcribed
// from that source at the 1280x720 WIDE canvas (where Scale(n)==n, GRID_SIZE=9 ->
// gridSize=9px). Round-7 rewrite: the earlier "measured-from-dim-screenshot" colours
// were a regression — the recomp draws BRIGHT green (0,130,0) plates and dims them at
// render time via the SCANLINE_BUTTON shader (even rows half-alpha). We now use the
// source colours + MOD_SCANLINE_BUTTON, which composites to the correct dim look.
//
// Source map:
//   * plate (active tab + value cell): options_menu.cpp L545-574 / L942-946 — three
//     AddRectFilledMultiColor layers under SCANLINE_BUTTON (see DrawPlate);
//   * tab text gradient: L590-598 — ALWAYS lime(128,255,0)->gold(255,192,0), alpha
//     235*motion active / 128*motion inactive, 4px black outline + bevel;
//   * value text: L1179-1199 — white base, 4px black outline, green 192,255,0->128,170,0
//     vertical gradient fill (identical selected/unselected);
//   * toggle light: imgui_utils.cpp DrawToggleLight L830-859 — 14px light + 24px additive
//     yellow glow when ON (sprite stand-in: a lit/dark disc here);
//   * selection arrows: L1010-1100 — filled triangles, base (0,97,0) + additive magenta
//     (255,0,255)->(255,128,255), drawn on the active row;
//   * title: L126-250 — DFSoGei natural width (NO stretch), size 48, + marching gold
//     cursor-square on open (255,188,0, RECTANGLE_BEVEL);
//   * info panel: L1441-1520 — single FULL-WIDTH thumbnail slot (no frame/PREVIEW/
//     medallion), description centred Seurat 28 below it.
//   * container: game_window.cpp — bg(0,0,0,223)+outer(0,49,0)+inner(0,33,0)+(0,89,0) brackets.
// Interactive (LB/RB tab, Up/Down rows, Left/Right value) like the recomp.
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
// desc = the full paragraph (word-wrapped + centred at draw time, like the recomp);
// valDescs = optional per-choice continuation, appended after a blank line.
struct Option { const char* label; Kind kind; const char* const* choices; int choiceCount; int val;
                const char* desc; const char* const* valDescs; };

const char* const CH_LANG[] = { "ENGLISH", "JAPANESE", "FRENCH", "GERMAN", "SPANISH", "ITALIAN" };
const char* const CH_TOD[]  = { "XBOX", "PLAYSTATION" };
const char* const VD_TOD[]  = {
    "Xbox: the transformation cutscene will play with artificial loading times.",
    "PlayStation: a spinning medal loading screen will be used instead." };
const char* const CH_TYPE[] = { "TYPE A", "TYPE B", "TYPE C" };
const char* const CH_VOL[]  = { "0", "2", "4", "6", "8", "10" };
const char* const CH_DISP[] = { "WINDOWED", "BORDERLESS", "FULLSCREEN" };
const char* const CH_RES[]  = { "1280x720", "1600x900", "1920x1080" };
const char* const CH_FPS[]  = { "30", "60", "VSYNC" };
const char* const CH_BRI[]  = { "-2", "-1", "0", "+1", "+2" };

struct Category { const char* name; Option* opts; int optCount; };
Option OPTS_SYSTEM[] = {
    { "Language",                  ENUM,   CH_LANG, 6, 0, "Select the on-screen text language used throughout the game.", nullptr },
    { "Voice Language",            ENUM,   CH_LANG, 6, 0, "Select the spoken voice language for cutscenes and dialogue.", nullptr },
    { "Subtitles",                 TOGGLE, nullptr, 0, 1, "Show subtitles during voiced dialogue and cutscenes.", nullptr },
    { "Hints",                     TOGGLE, nullptr, 0, 1, "Display gameplay hints and tips during play.", nullptr },
    { "Control Tutorial",          TOGGLE, nullptr, 0, 1, "Show control tutorials when new actions become available.", nullptr },
    { "Achievement Notifications", TOGGLE, nullptr, 0, 1, "Pop a notification when an achievement is unlocked.", nullptr },
    { "Time of Day Transition",    ENUM,   CH_TOD,  2, 0, "Change how the loading screen appears when switching time of day in the hub areas.", VD_TOD },
};
Option OPTS_INPUT[] = {
    { "Rumble",          TOGGLE, nullptr, 0, 1, "Controller vibration feedback on impacts.", nullptr },
    { "Invert Camera X", TOGGLE, nullptr, 0, 0, "Invert horizontal camera control.", nullptr },
    { "Invert Camera Y", TOGGLE, nullptr, 0, 0, "Invert vertical camera control.", nullptr },
    { "Button Layout",   ENUM,   CH_TYPE, 3, 0, "Preset controller button mapping.", nullptr },
};
Option OPTS_AUDIO[] = {
    { "Master Volume", SLIDER, CH_VOL, 6, 5, "Overall output volume.", nullptr },
    { "Music Volume",  SLIDER, CH_VOL, 6, 4, "Background music level.", nullptr },
    { "SFX Volume",    SLIDER, CH_VOL, 6, 5, "Sound-effect level.", nullptr },
    { "Voice Volume",  SLIDER, CH_VOL, 6, 5, "Character voice level.", nullptr },
};
Option OPTS_VIDEO[] = {
    { "Display Mode", ENUM,   CH_DISP, 3, 2, "How the game fills your display.", nullptr },
    { "Resolution",   ENUM,   CH_RES,  3, 2, "Rendering resolution. Higher is sharper.", nullptr },
    { "Frame Rate",   ENUM,   CH_FPS,  3, 1, "Target frames per second.", nullptr },
    { "Brightness",   SLIDER, CH_BRI,  5, 2, "Adjust screen brightness.", nullptr },
};
Category CATEGORIES[] = {
    { "SYSTEM", OPTS_SYSTEM, 7 }, { "INPUT", OPTS_INPUT, 4 }, { "AUDIO", OPTS_AUDIO, 4 }, { "VIDEO", OPTS_VIDEO, 4 },
};
constexpr int CATEGORY_COUNT = 4;

// ---- exact geometry (1280x720, from options_menu.cpp) -----------------------
constexpr float GRID = 9.0f;
constexpr float SP_X0 = 33, SP_Y0 = 117, SP_X1 = 843, SP_Y1 = 604;   // settings panel
constexpr float IP_X0 = 868, IP_Y0 = 117, IP_X1 = 1246, IP_Y1 = 604; // info panel
constexpr float CLIP_X = SP_X0 + GRID * 2;     // 51  (settings content clip left)
constexpr float TABS_Y = SP_Y0 + GRID * 2;     // 135
constexpr float TAB_H  = GRID * 4;             // 36
constexpr float ROWS_TOP = SP_Y0 + GRID * 2 + GRID * 6;   // 189
constexpr float ROW_PITCH = GRID * 6;          // 54
constexpr float ROW_H   = GRID * 5.5f;         // 49.5
constexpr float OPT_W   = GRID * 54;           // 486 (left option width)
constexpr float LABEL_X = SP_X0 + GRID * 2 + GRID;  // 60
constexpr float VAL_W = 192, VAL_H = GRID * 3; // 27
constexpr float VAL_X0 = (SP_X0 + GRID * 2) + OPT_W + ((SP_X1 - GRID * 2) - (SP_X0 + GRID * 2) - OPT_W - VAL_W) / 2.0f - 4.0f; // 581 (measured in ref)

// ---- exact colours (IM_COL32 from options_menu.cpp / game_window.cpp) -------
const uint32_t C_TITLE     = RGBA(255, 190, 33, 255);
const uint32_t C_PANEL_BG  = RGBA(0, 0, 0, 223);
const uint32_t C_OUTER     = RGBA(0, 49, 0, 255);
const uint32_t C_INNER     = RGBA(0, 33, 0, 255);  // recomp inner fill IM_COL32(0,33,0)
const uint32_t C_LINE      = RGBA(0, 89, 0, 255);
const uint32_t C_TAB_G_T   = RGBA(128, 255, 0, 255);  // tab text gradient top (lime)
const uint32_t C_TAB_G_B   = RGBA(255, 192, 0, 255);  // tab text gradient bottom (gold)
const uint32_t C_SEL_TL    = RGBA(226, 113, 34, 128);  // selected-row diagonal (gold)
const uint32_t C_SEL_BR    = RGBA(146, 255, 49, 128);  // -> green
const uint32_t C_LABEL     = RGBA(255, 255, 255, 255);
const uint32_t C_VAL_G_T   = RGBA(192, 255, 0, 255);   // value text gradient top
const uint32_t C_VAL_G_B   = RGBA(128, 170, 0, 255);   // value text gradient bottom
const uint32_t C_LIGHT_ON  = RGBA(214, 255, 64, 255);  // lit toggle light (sprite stand-in)
const uint32_t C_LIGHT_OFF = RGBA(30, 52, 30, 255);    // dark toggle light
const uint32_t C_BLACK     = RGBA(0, 0, 0, 255);
const uint32_t C_DESC      = RGBA(255, 255, 255, 255);
const uint32_t C_VERSION   = RGBA(255, 255, 255, 70);
const uint32_t C_GREEN_GLOW= RGBA(203, 255, 0, 55);    // scanline band glow (inner)
const uint32_t C_DIV_HI    = RGBA(222, 255, 189, 65);
const uint32_t C_DIV_LO    = RGBA(173, 255, 156, 65);
const uint32_t C_DIV_CORE  = RGBA(115, 178, 104, 255);
const uint32_t C_BG        = RGBA(2, 6, 3, 255);       // deep base behind everything
const uint32_t C_WHITE     = RGBA(255, 255, 255, 255);

// ---- footer glyph atlas -----------------------------------------------------
struct UV { float u0, v0, u1, v1; };
int g_glyphTex = -1;
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A  = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B  = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };
const UV GLYPH_X  = { 0.16016f, 0.00781f, 0.23047f, 0.07422f };
const UV GLYPH_LB = { 0.32617f, 0.00781f, 0.46094f, 0.07812f };
const UV GLYPH_RB = { 0.48242f, 0.00781f, 0.61523f, 0.07812f };

// ---- state ------------------------------------------------------------------
int    g_cat = 0, g_sel = 6, g_prevSel = 6;
double g_moveStart = -100.0;
Category& Cat()      { return CATEGORIES[g_cat]; }
int       OptCount() { return Cat().optCount; }
Option&   Opt(int i) { return Cat().opts[i]; }
int       ValMax(const Option& o) { return o.kind == TOGGLE ? 1 : std::max(0, o.choiceCount - 1); }

// The recomp's actual fonts (from C:/swardbuild, staged to assets/fonts): FOT-SeuratPro-M
// (row labels + description), FOT-NewRodinPro-DB (values + footer + version), DFHeiStd-W7
// (~DFSoGeiStd, the title + tabs). Baked once into the sgfxui multi-font registry.
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// persistent option values: key "opt_<cat>_<row>" in sgfx_settings.ini
void LoadSavedValues() {
    char key[48];
    for (int c = 0; c < CATEGORY_COUNT; ++c)
        for (int i = 0; i < CATEGORIES[c].optCount; ++i) {
            Option& o = CATEGORIES[c].opts[i];
            snprintf(key, sizeof key, "opt_%d_%d", c, i);
            o.val = std::clamp(settings::GetInt(key, o.val), 0, ValMax(o));
        }
}
void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF (im_font_atlas)
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");    // real game MSDF
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");   // real DFSoGeiStd-W7 (title + tabs)
    static bool loaded = false;
    if (!loaded) { LoadSavedValues(); loaded = true; }
}
void Reset() { g_cat = 0; g_sel = 6; g_prevSel = 6; g_moveStart = -100.0; }
void Input(const ScreenInput& in) {
    if (in.up || in.down) { g_prevSel = g_sel; if (in.up) g_sel = std::max(0, g_sel - 1); if (in.down) g_sel = std::min(OptCount()-1, g_sel+1); g_moveStart = Now(); }
    if (in.left || in.right) {
        Option& o = Opt(std::clamp(g_sel,0,OptCount()-1));
        o.val = std::clamp(o.val + (in.right?1:-1), 0, ValMax(o));
        char key[48];
        snprintf(key, sizeof key, "opt_%d_%d", g_cat, std::clamp(g_sel,0,OptCount()-1));
        settings::SetInt(key, o.val);                 // persists immediately
    }
    if (in.tabLeft || in.tabRight) { g_cat = (g_cat + (in.tabRight?1:CATEGORY_COUNT-1)) % CATEGORY_COUNT; g_sel = g_prevSel = std::min(g_sel, OptCount()-1); g_moveStart = Now(); }
}

// the green DrawContainer panel (game_window.cpp recipe)
void DrawContainer(float x0, float y0, float x1, float y1, bool rightOutline, float t) {
    DrawRect({ x0, y0 }, { x1, y1 }, WithAlpha(C_PANEL_BG, t));               // bg
    SetModifier(MOD_CHECKERBOARD);
    DrawRect({ x0, y0 + GRID }, { x0 + GRID, y1 - GRID }, WithAlpha(C_OUTER, t));                    // left
    DrawRect({ x1 - GRID, y0 + GRID }, { x1, y1 - GRID }, WithAlpha(rightOutline ? C_OUTER : C_INNER, t)); // right
    DrawRect({ x0, y0 }, { x1, y0 + GRID }, WithAlpha(C_OUTER, t));                                  // top
    DrawRect({ x0, y1 - GRID }, { x1, y1 }, WithAlpha(C_OUTER, t));                                  // bottom
    DrawRect({ x0 + GRID, y0 + GRID }, { x1 - GRID, y1 - GRID }, WithAlpha(C_INNER, t));             // inner
    ResetModifier();
    // 2px corner brackets (lineColor) — top-left/right + bottom-left/right
    uint32_t lc = WithAlpha(C_LINE, t); const float g = GRID, L = 2.0f;
    DrawRect({ x0+g, y0+g }, { x0+g+L, y0+g*2 }, lc); DrawRect({ x0+g, y0+g }, { x1-g, y0+g+L }, lc); DrawRect({ x1-g-L, y0+g }, { x1-g, y0+g*2 }, lc);
    DrawRect({ x0+g, y1-g*2 }, { x0+g+L, y1-g }, lc); DrawRect({ x0+g, y1-g-L }, { x1-g, y1-g }, lc); DrawRect({ x1-g-L, y1-g*2 }, { x1-g, y1-g }, lc);
}

// ---- shared 3-layer green plate (active tab + value cell) -------------------
// options_menu.cpp L545-574 (tab) / L942-946 (value): three AddRectFilledMultiColor
// layers under SCANLINE_BUTTON. The bright (0,130,0) is dimmed at render by the
// even-row half-alpha scanline shader -> composites to the on-screen dim plate.
void DrawPlate(float x0, float y0, float x1, float y1, float a) {
    auto A = [&](int base) { return (uint8_t)std::clamp((int)lround(base * a), 0, 255); };
    SetModifier(MOD_SCANLINE_BUTTON);
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(0,130,0,A(223)), RGBA(0,130,0,A(178)), RGBA(0,130,0,A(223)), RGBA(0,130,0,A(178)));
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(0,0,0,A(13)),    RGBA(0,0,0,0),         RGBA(0,0,0,A(55)),    RGBA(0,0,0,A(6)));
    DrawQuadGradient({x0,y0},{x1,y1}, RGBA(0,130,0,A(13)),  RGBA(0,130,0,A(111)),  RGBA(0,130,0,0),      RGBA(0,130,0,A(55)));
    ResetModifier();
}

// ---- filled horizontal-pointing arrow triangle (rows approximate the fill) ---
void FillTri(float baseX, float apexX, float y0, float y1, uint32_t col, bool add = false) {
    const int N = 18; const float cy = (y0+y1)*0.5f, hh = (y1-y0)*0.5f;
    for (int i = 0; i < N; ++i) {
        float ry0 = y0 + (y1-y0)*i/N, ry1 = y0 + (y1-y0)*(i+1)/N;
        float f = 1.0f - std::fabs(((ry0+ry1)*0.5f)-cy)/hh;     // 1 at centre -> 0 at edges
        float xe = baseX + (apexX-baseX)*f;
        DrawRect({ std::min(baseX,xe), ry0 }, { std::max(baseX,xe), ry1 }, col, add);
    }
}
// selection arrows (options_menu.cpp DrawSelectionArrows): dark-green base + additive
// magenta gradient, on the active value box. Drawn at the static (settled) state.
void DrawSelectionArrows(float bx0, float by0, float bx1, float by1, float t) {
    const float pad = GRID, width = GRID * 2.5f;          // L: base at min-pad, apex at -width
    uint32_t base = WithAlpha(RGBA(0,97,0,255), t);
    uint32_t mag  = WithAlpha(RGBA(255,64,255,255), 1.0f);  // additive mid of (255,0,255)->(255,128,255)
    // left arrow: base vertical at bx0-pad, apex to the left
    FillTri(bx0-pad, bx0-pad-width, by0, by1, base);
    FillTri(bx0-pad, bx0-pad-width, by0, by1, WithAlpha(mag, t), true);
    // right arrow: base vertical at bx1+pad, apex to the right
    FillTri(bx1+pad, bx1+pad+width, by0, by1, base);
    FillTri(bx1+pad, bx1+pad+width, by0, by1, WithAlpha(mag, t), true);
}

// value text: white base + 4px black outline + green vertical gradient fill
void DrawValueText(const char* s, float x0, float y0, float x1, float y1, float t) {
    const float boxW = x1 - x0; float w = MeasureText(20.0f, s).x; float sx = 1.0f;
    if (w > boxW && w > 0.0f) sx = boxW / w;             // recomp squashes overflowing values
    float dw = w * sx;
    float px = x0 + (boxW - dw) * 0.5f, py = y0 + ((y1 - y0) - 20.0f) * 0.5f;
    if (sx != 1.0f) SetTextStretchX(sx);
    static const float O[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
    for (auto& o : O) DrawText({ px + o[0]*1.6f, py + o[1]*1.6f }, 20.0f, WithAlpha(C_BLACK, t), s);
    DrawTextGradient({ px, py }, 20.0f, WithAlpha(C_VAL_G_T, t), WithAlpha(C_VAL_G_B, t), s);
    if (sx != 1.0f) ResetTextStretchX();
}

// tab label: lime->gold gradient + 4px black outline + bevel (options_menu.cpp L590-610)
void DrawTabText(float x, float y, const char* s, float alpha) {
    uint8_t a = (uint8_t)std::clamp((int)lround(alpha), 0, 255);
    static const float O[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
    for (auto& o : O) DrawText({ x + o[0]*1.6f, y + o[1]*1.6f }, 32.0f, RGBA(0,0,0,a), s);
    SetModifier(MOD_TITLE_BEVEL);
    DrawTextGradient({ x, y }, 32.0f, WithAlpha(C_TAB_G_T, a/255.0f), WithAlpha(C_TAB_G_B, a/255.0f), s);
    ResetModifier();
}

// marching gold cursor-square on open (options_menu.cpp DrawTitle L186-245).
// Steps across the title by its own width, then blinks + fades; gone once settled.
// NB: `openSec` is the open-START TIMESTAMP (like ComputeMotion), so elapsed = Now()-openSec.
void DrawTitleCursor(double openSec, float ox, float oy) {
    const float e = (float)(Now() - openSec);      // seconds since the screen opened
    if (e < 0.0f) return;
    const float rectSize = 32.0f, rectY = -2.0f;   // y=Scale(10) below title top; title pen ~oy
    float titleW = MeasureText(48.0f, "OPTIONS").x;
    if (titleW <= 1.0f) return;
    int steps = std::clamp((int)std::ceil(titleW / rectSize), 1, 12);
    const float stepDur = 0.085f;                  // ~5 frames/step @60fps
    const float marchEnd = stepDur * steps;
    static const float RECT_SCALES[] = { 1.2f, 1.1f, 1.1f, 1.0f, 1.15f, 0.4f, 1.2f, 1.1f, 1.05f, 1.0f, 1.5f, 1.2f, 1.0f };
    float a = 1.0f, rx;
    if (e < marchEnd) {                            // marching
        int step = std::clamp((int)(e / stepDur), 0, steps-1);
        rx = step * rectSize;
    } else {                                        // blink then fade out
        float after = e - marchEnd;
        if (after > 0.85f) return;                  // fully gone (settled state)
        rx = (steps-1) * rectSize;
        a = (0.55f + 0.45f*std::cos(after*22.0f)) * std::clamp(1.0f - (after-0.45f)/0.40f, 0.0f, 1.0f);
    }
    float sc = RECT_SCALES[((int)lround(rx / rectSize)) % (int)(sizeof(RECT_SCALES)/sizeof(float))];
    float mx0 = ox + rx, my0 = oy + rectY, mx1 = mx0 + rectSize*sc, my1 = my0 + rectSize;
    const float m = 2.5f;
    DrawRect({ mx0-m, my0-m }, { mx1+m, my1+m }, WithAlpha(C_BLACK, a));     // rounded black outline
    SetModifier(MOD_TITLE_BEVEL);
    DrawRect({ mx0, my0 }, { mx1, my1 }, WithAlpha(RGBA(255,188,0,255), a)); // gold cursor square
    ResetModifier();
}

void Draw(double openSec) {
    const float t = (float)ComputeMotion(openSec, 0.0, 14.0);
    DrawRect({ 0, 0 }, { REF_W, REF_H }, C_BG);   // deep base (the live game would be behind in-game)

    // ---- top & bottom scanline bands (105px) : vignette + green glow + divider ----
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
        DrawRect({ 0, y-2 }, { REF_W, y }, WithAlpha(C_DIV_HI, t));
        DrawRect({ 0, y+1 }, { REF_W, y+3 }, WithAlpha(C_DIV_LO, t));
        DrawRect({ 0, y }, { REF_W, y+1 }, WithAlpha(C_DIV_CORE, t));
    };
    divider(105); divider(615);

    // ---- title (DFSoGei natural width, NO stretch) + marching cursor-square ----
    SetFont(g_fDF);
    DrawTextBevel({ 122, 56 }, 48.0f, WithAlpha(C_TITLE, t), "OPTIONS");
    DrawTitleCursor(openSec, 122, 56);

    // ---- panels ----
    DrawContainer(SP_X0, SP_Y0, SP_X1, SP_Y1, true, t);
    DrawContainer(IP_X0, IP_Y0, IP_X1, IP_Y1, false, t);

    // ---- tabs (SYSTEM/INPUT/AUDIO/VIDEO) : natural width, shared plate, gradient text ----
    {
        SetFont(g_fDF);
        float clipW = (SP_X1 - GRID*2) - CLIP_X;     // 826-51 = 775
        float widths[CATEGORY_COUNT], sum = 0;
        for (int i = 0; i < CATEGORY_COUNT; ++i) { widths[i] = MeasureText(32.0f, CATEGORIES[i].name).x; sum += widths[i]; }
        float pad = (clipW - sum) / (CATEGORY_COUNT + 1);
        // pass 1: active-tab plate (drawn first so text sits on top, like the recomp)
        float x = CLIP_X + pad;
        for (int i = 0; i < CATEGORY_COUNT; ++i) {
            if (i == g_cat) {
                float tabPad = std::min(pad * 0.5f, GRID * 3);
                DrawPlate(x - tabPad, TABS_Y, x + widths[i] + tabPad, TABS_Y + TAB_H, t);
            }
            x += widths[i] + pad;
        }
        // pass 2: tab text — ALWAYS lime->gold gradient, alpha 235 active / 128 inactive
        x = CLIP_X + pad;
        for (int i = 0; i < CATEGORY_COUNT; ++i) {
            float alpha = (i == g_cat ? 235.0f : 128.0f) * t;
            DrawTabText(x, TABS_Y, CATEGORIES[i].name, alpha);   // top-aligned at band top
            x += widths[i] + pad;
        }
    }

    // ---- selected-row gold->green diagonal bar (eased) ----
    {
        float mt = (float)ComputeMotion(g_moveStart, 0.0, 8.0);
        float slot = Lerp((float)g_prevSel, (float)g_sel, mt);
        float ry = ROWS_TOP + slot * ROW_PITCH;
        uint32_t gold = WithAlpha(C_SEL_TL, t), grn = WithAlpha(C_SEL_BR, t);
        uint32_t mid  = WithAlpha(ColourLerp(C_SEL_TL, C_SEL_BR, 0.5f), t);
        DrawQuadGradient({ CLIP_X, ry }, { CLIP_X + OPT_W, ry + ROW_H }, gold, mid, grn, mid);  // TL gold -> BR green diagonal
    }

    // ---- rows ----
    for (int i = 0; i < OptCount(); ++i) {
        const Option& o = Opt(i);
        float ry = ROWS_TOP + i * ROW_PITCH, cy = ry + ROW_H * 0.5f;
        bool sel = (i == g_sel);
        SetFont(g_fSeurat);
        DrawTextAligned({ LABEL_X, ry }, { VAL_X0 - 10, ry + ROW_H }, 26.0f, WithAlpha(C_LABEL, t),
                        o.label, Align::Left, true, true);
        // value cell = the shared 3-layer plate
        float vy0 = ry + (ROW_H - VAL_H) * 0.5f, vy1 = vy0 + VAL_H;
        DrawPlate(VAL_X0, vy0, VAL_X0 + VAL_W, vy1, t);
        if (sel) DrawSelectionArrows(VAL_X0, vy0, VAL_X0 + VAL_W, vy1, t);
        SetFont(g_fRodin);
        if (o.kind == TOGGLE) {
            // toggle light: 14px lit/dark disc + 24px additive yellow glow when ON
            bool onv = o.val != 0;
            const float ls = 14.0f, lx = VAL_X0 + 14.0f, ly = vy0 + ((VAL_H - ls) * 0.5f) + 1.0f;
            const float lcx = lx + ls*0.5f, lcy = ly + ls*0.5f;
            if (onv) {  // additive yellow glow (255,255,0,127), 24px, offset like the source
                const float gs = 24.0f; float gx = lx - gs*0.5f + 2.0f, gy = ly - gs*0.5f;
                DrawRect({ gx, gy }, { gx + gs, gy + gs }, WithAlpha(RGBA(255,255,0,127), t), true);
            }
            auto disc = [&](float r, uint32_t c){      // octagon disc stand-in for g_texLight
                float k = r * 0.4142f;
                DrawRect({ lcx-r, lcy-k }, { lcx+r, lcy+k }, c);
                DrawRect({ lcx-k, lcy-r }, { lcx+k, lcy+r }, c);
                DrawRect({ lcx-r*0.78f, lcy-r*0.78f }, { lcx+r*0.78f, lcy+r*0.78f }, c);
            };
            disc(ls*0.5f, WithAlpha(onv ? C_LIGHT_ON : C_LIGHT_OFF, t));
        } else if (o.kind == SLIDER) {
            // continuous slider (recomp DrawConfigOption isSlider, options_menu.cpp L959-994):
            // inner channel 0,65,0 -> 0,32,0 (inset 6x/3y); fill 57,241,0 -> 2,106,0 (inset +2);
            // factor = val/max (linear; centre-split is identity for these symmetric ranges).
            float factor = (ValMax(o) > 0) ? (float)o.val / (float)ValMax(o) : 0.0f;
            float cx0 = VAL_X0 + 6, cy0 = vy0 + 3, cx1 = VAL_X0 + VAL_W - 6, cy1 = vy1 - 3;
            SetModifier(MOD_SCANLINE_BUTTON);
            DrawQuadGradient({cx0,cy0},{cx1,cy1}, WithAlpha(RGBA(0,65,0,255),t), WithAlpha(RGBA(0,65,0,255),t),
                                                  WithAlpha(RGBA(0,32,0,255),t), WithAlpha(RGBA(0,32,0,255),t));
            float fx0 = cx0 + 2, fy0 = cy0 + 2, fy1 = cy1 - 2;
            float fx1 = fx0 + ((cx1 - 2) - fx0) * factor;
            if (fx1 > fx0 + 0.5f)
                DrawQuadGradient({fx0,fy0},{fx1,fy1}, WithAlpha(RGBA(57,241,0,255),t), WithAlpha(RGBA(57,241,0,255),t),
                                                      WithAlpha(RGBA(2,106,0,255),t), WithAlpha(RGBA(2,106,0,255),t));
            ResetModifier();
        } else {
            const char* s = (o.choices && o.val < o.choiceCount) ? o.choices[o.val] : "";
            DrawValueText(s, VAL_X0 + 6, vy0, VAL_X0 + VAL_W - 6, vy1, t);
        }
    }

    // ---- info panel: single FULL-WIDTH thumbnail slot + centred description ----
    // (recomp DrawInfoPanel L1441-1520 — NO green frame, NO 'PREVIEW' label, NO medallion)
    {
        const float ix0 = IP_X0 + GRID*2, ix1 = IP_X1 - GRID*2;     // content clip (886..1228)
        const float wrapW = ix1 - ix0;
        // thumbnail slot: full content width, 16:9, top inset gridSize/2 (art drops here)
        const float thumbH = wrapW * 9.0f / 16.0f;
        const float ty0 = IP_Y0 + GRID*2 + GRID*0.5f, ty1 = ty0 + thumbH;
        DrawRect({ ix0, ty0 }, { ix1, ty1 }, WithAlpha(RGBA(6,10,8,235), t));        // dark empty slot
        DrawVGradient({ ix0, ty0 }, { ix1, ty1 }, WithAlpha(RGBA(0,30,0,60), t), WithAlpha(RGBA(0,0,0,90), t));
        uint32_t kl = WithAlpha(RGBA(0,49,0,160), t); const float L = 1.0f;          // faint 1px slot edge
        DrawRect({ ix0, ty0 }, { ix1, ty0+L }, kl); DrawRect({ ix0, ty1-L }, { ix1, ty1 }, kl);
        DrawRect({ ix0, ty0 }, { ix0+L, ty1 }, kl); DrawRect({ ix1-L, ty0 }, { ix1, ty1 }, kl);

        const Option& s = Opt(std::clamp(g_sel, 0, OptCount() - 1));
        SetFont(g_fSeurat);
        std::string full = s.desc ? s.desc : "";
        if (s.kind == ENUM && s.valDescs && s.val < s.choiceCount && s.valDescs[s.val] && s.valDescs[s.val][0]) {
            full += "\n\n"; full += s.valDescs[s.val];
        }
        const float fsz = 28.0f, lineH = fsz + 5.0f;
        std::vector<std::string> lines;
        size_t p = 0;
        while (p <= full.size()) {
            size_t nl = full.find('\n', p);
            std::string para = full.substr(p, nl == std::string::npos ? std::string::npos : nl - p);
            std::string cur; size_t w0 = 0;
            while (w0 < para.size()) {
                size_t w1 = para.find(' ', w0); if (w1 == std::string::npos) w1 = para.size();
                std::string word = para.substr(w0, w1 - w0);
                std::string trial = cur.empty() ? word : cur + " " + word;
                if (!cur.empty() && MeasureText(fsz, trial.c_str()).x > wrapW) { lines.push_back(cur); cur = word; }
                else cur = trial;
                w0 = w1 + 1;
            }
            lines.push_back(cur);            // empty paragraph -> blank line
            if (nl == std::string::npos) break;
            p = nl + 1;
        }
        const float textTop = ty1 + 24.0f, clipY0 = ty1 + 12.0f, clipY1 = IP_Y1 - 14;
        float scroll = 0;
        const float scrollMax = lines.size() * lineH - (clipY1 - textTop);
        if (scrollMax > 0) {   // hold -> scroll down -> hold -> scroll back (ping-pong marquee)
            const float speed = 50.0f, hold = 2.0f, leg = scrollMax / speed, cycle = 2 * (hold + leg);
            float ph = (float)std::fmod(Now(), (double)cycle);
            if      (ph < hold)            scroll = 0;
            else if (ph < hold + leg)      scroll = (ph - hold) * speed;
            else if (ph < hold * 2 + leg)  scroll = scrollMax;
            else                           scroll = scrollMax - (ph - hold * 2 - leg) * speed;
        }
        PushClip({ ix0 - 2, clipY0 }, { ix1 + 2, clipY1 });
        float dy = textTop - scroll;
        for (const std::string& ln : lines) {
            if (!ln.empty() && dy + lineH > clipY0 && dy < clipY1) {
                float lw = MeasureText(fsz, ln.c_str()).x;               // CENTRE each wrapped line
                DrawText({ ix0 + (wrapW - lw) * 0.5f, dy }, fsz, WithAlpha(C_DESC, t), ln.c_str());
            }
            dy += lineH;
        }
        PopClip();
    }

    // ---- footer button-guide + version ----
    // recomp footer: [LB/RB] Switch | [X] Reset | [A] Select | [B] Back ; version bottom-right.
    {
        SetFont(g_fRodin);
        float hx = 150.0f, hcy = 634.0f;
        auto glyph = [&](const UV& g) { if (g_glyphTex < 0) return; float asp = ((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh = 30.0f, gw = gh*asp; DrawImage(g_glyphTex, { hx, hcy-gh*0.5f }, { hx+gw, hcy+gh*0.5f }, { g.u0, g.v0 }, { g.u1, g.v1 }, WithAlpha(C_WHITE, t)); hx += gw + 8; };
        auto word = [&](const char* w, float pad){ DrawText({ hx, hcy-13 }, 24.0f, WithAlpha(C_WHITE, t), w); hx += MeasureText(24.0f, w).x + pad; };
        glyph(GLYPH_LB); word("Switch", 10); glyph(GLYPH_RB); hx += 18;
        glyph(GLYPH_X);  word("Reset", 14);
        glyph(GLYPH_A);  word("Select", 14);
        glyph(GLYPH_B);  word("Back", 0);
        DrawTextAligned({ REF_W - 420, REF_H - 22 }, { REF_W - 4, REF_H - 6 }, 12.0f, WithAlpha(C_VERSION, t),
                        "v1.0.3.325e4d3-HEAD (RelWithDebInfo)", Align::Right, true, true);
    }
    ResetFont();
}

} // namespace

void OptionsInit() { Init(); }
void OptionsDraw(double openSeconds) { Draw(openSeconds); }
void OptionsInput(const ScreenInput& in) { Input(in); }
void OptionsReset() { Reset(); }
