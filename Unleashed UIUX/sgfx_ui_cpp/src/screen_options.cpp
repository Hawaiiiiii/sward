// =============================================================================
// screen_options.cpp — the Options menu, ported PIXEL-EXACT from UnleashedRecomp's
// own UI source (ui/options_menu.cpp + ui/game_window.cpp), which is the literal
// code that drew the captured video (v1.0.3 HEAD). Every rect/colour/size below is
// transcribed from that source at the 1280x720 WIDE canvas (where Scale(n)==n,
// GRID_SIZE=9 -> gridSize=9px). This replaces the earlier eyeballed-from-video
// version. The options UI is CODE-DRAWN (the ui_general CSD is just a generic
// frame), so this procedural reconstruction is the faithful path.
//
// Exact layout (options_menu.cpp:1768-1786, DrawContainer:366-414, DrawCategories,
// DrawConfigOption, DrawScanlineBars, DrawTitle):
//   * settings panel (33,117)-(843,604), info panel (868,117)-(1246,604);
//   * panel = bg(0,0,0,223) + outer border(0,49,0) gridSize wide + inner(0,33,0)
//     + (0,89,0) 2px corner brackets; content inset 18px;
//   * tabs band y[135..171] (DFSoGei 32, active green gradient + bevel text);
//   * 7 rows from y=189 pitch 54; label x=60 (Seurat 26); value cell (581,+11.25)
//     192x27 green-gradient box + value text (NewRodin 20); selected row gold->green
//     diagonal bar; ON/OFF = lime toggle light + glow;
//   * top/bottom 105px scanline bands (green glow 203,255,0 + vignette + feathered
//     mint divider lines at y=105 / y=615); gold "OPTIONS" title (DFSoGei 48) @ (122,56);
//   * footer button-guide + version string bottom-right.
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
#include <string>
#include <vector>

using namespace ui;

namespace {

enum Kind { TOGGLE, ENUM };
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
    { "Master Volume", ENUM, CH_VOL, 6, 5, "Overall output volume.", nullptr },
    { "Music Volume",  ENUM, CH_VOL, 6, 4, "Background music level.", nullptr },
    { "SFX Volume",    ENUM, CH_VOL, 6, 5, "Sound-effect level.", nullptr },
    { "Voice Volume",  ENUM, CH_VOL, 6, 5, "Character voice level.", nullptr },
};
Option OPTS_VIDEO[] = {
    { "Display Mode", ENUM, CH_DISP, 3, 2, "How the game fills your display.", nullptr },
    { "Resolution",   ENUM, CH_RES,  3, 2, "Rendering resolution. Higher is sharper.", nullptr },
    { "Frame Rate",   ENUM, CH_FPS,  3, 1, "Target frames per second.", nullptr },
    { "Brightness",   ENUM, CH_BRI,  5, 2, "Adjust screen brightness.", nullptr },
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
const uint32_t C_INNER     = RGBA(0, 33, 0, 255);
const uint32_t C_LINE      = RGBA(0, 89, 0, 255);
const uint32_t C_TAB_BG    = RGBA(0, 130, 0, 223);
const uint32_t C_TAB_BG2   = RGBA(0, 130, 0, 150);
const uint32_t C_TAB_TXT_T = RGBA(126, 230, 15, 255);  // tab text gradient top
const uint32_t C_TAB_TXT_B = RGBA(199, 127, 12, 255);  // tab text gradient bottom
const uint32_t C_TAB_OFF   = RGBA(128, 135, 41, 220);  // inactive tab olive-gold (measured ~102,108,33)
const uint32_t C_SEL_TL    = RGBA(226, 113, 34, 126);  // selected row diagonal (gold)
const uint32_t C_SEL_BR    = RGBA(146, 255, 49, 126);  // -> green
const uint32_t C_LABEL     = RGBA(255, 255, 255, 255);
const uint32_t C_VAL_BG    = RGBA(0, 70, 0, 205);
const uint32_t C_VAL_BG_B  = RGBA(0, 52, 0, 195);
const uint32_t C_VAL_TXT_T = RGBA(105, 140, 18, 255);
const uint32_t C_VAL_TXT_B = RGBA(82, 108, 12, 255);
const uint32_t C_VAL_TXT_SEL = RGBA(195, 225, 100, 255);  // selected enum row brightens (measured)
const uint32_t C_LIGHT_ON  = RGBA(170, 182, 12, 255);  // dim olive-yellow toggle light (measured)
const uint32_t C_LIGHT_OFF = RGBA(40, 70, 40, 255);
const uint32_t C_CARET     = RGBA(140, 230, 60, 255);  // selected-value carets are green in ref
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

void DrawCaret(float cx, float cy, bool right, uint32_t col) {
    for (int i = 0; i < 5; ++i) { float hh = 9.0f - i*1.8f, dx = i*2.0f; float bx = right ? (cx-6+dx) : (cx+6-dx); DrawRect({ bx, cy-hh }, { bx+2, cy+hh }, col); }
}

void DrawValueText(const char* s, float x0, float y0, float x1, float y1, float t, bool sel = false) {
    // the recomp's green value-text vertical gradient; selected row brightens toward the measured peak
    float w = MeasureText(20.0f, s).x;
    float px = x0 + ((x1 - x0) - w) * 0.5f, py = y0 + ((y1 - y0) - 20.0f) * 0.5f;
    uint32_t top = sel ? C_VAL_TXT_SEL : C_VAL_TXT_T;
    uint32_t bot = sel ? ColourLerp(C_VAL_TXT_B, C_VAL_TXT_SEL, 0.85f) : C_VAL_TXT_B;
    DrawTextGradient({ px, py }, 20.0f, WithAlpha(top, t), WithAlpha(bot, t), s);
}

void Draw(double openSec) {
    const float t = (float)ComputeMotion(openSec, 0.0, 14.0);
    DrawRect({ 0, 0 }, { REF_W, REF_H }, C_BG);   // deep base (the live game would be behind in-game)

    // ---- top & bottom scanline bands (105px) : vignette + green glow + divider ----
    auto band = [&](float yTop, float yBot, bool top) {
        // vignette: black (outer) -> transparent (inner)
        if (top) DrawVGradient({ 0, yTop }, { REF_W, yBot }, RGBA(0,0,0,255), RGBA(0,0,0,0));
        else     DrawVGradient({ 0, yTop }, { REF_W, yBot }, RGBA(0,0,0,0), RGBA(0,0,0,255));
        // green scanline glow: transparent (outer) -> faint green (inner)
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

    // ---- title ----
    SetFont(g_fDF);
    SetTextStretchX(1.30f);   // the ref OPTIONS is ~30% wider than DFSoGei's natural set
    DrawTextBevel({ 122, 56 }, 48.0f, WithAlpha(C_TITLE, t), "OPTIONS");
    ResetTextStretchX();

    // ---- panels ----
    DrawContainer(SP_X0, SP_Y0, SP_X1, SP_Y1, true, t);
    DrawContainer(IP_X0, IP_Y0, IP_X1, IP_Y1, false, t);

    // ---- tabs (SYSTEM/INPUT/AUDIO/VIDEO) ----
    {
        SetFont(g_fDF);
        SetTextStretchX(1.30f);   // ref tabs are wider too (MeasureText ignores stretch -> scale widths)
        float clipW = (SP_X1 - GRID*2) - CLIP_X;     // 826-51 = 775
        float widths[CATEGORY_COUNT], sum = 0;
        for (int i = 0; i < CATEGORY_COUNT; ++i) { widths[i] = MeasureText(32.0f, CATEGORIES[i].name).x * 1.30f; sum += widths[i]; }
        float pad = (clipW - sum) / (CATEGORY_COUNT + 1);
        float x = CLIP_X + pad;
        for (int i = 0; i < CATEGORY_COUNT; ++i) {
            bool on = (i == g_cat);
            float tabPad = std::min(pad * 0.5f, GRID * 3);
            if (on) {   // green active-tab button
                DrawVGradient({ x - tabPad, TABS_Y }, { x + widths[i] + tabPad, TABS_Y + TAB_H },
                              WithAlpha(C_TAB_BG, t), WithAlpha(C_TAB_BG2, t));
            }
            // active tab text = the recomp's lime->gold vertical gradient; inactive olive-gold
            float ty = TABS_Y + (TAB_H - 32.0f) * 0.5f;
            if (on) DrawTextGradient({ x, ty }, 32.0f, WithAlpha(C_TAB_TXT_T, t), WithAlpha(C_TAB_TXT_B, t), CATEGORIES[i].name);
            else    DrawText({ x, ty }, 32.0f, WithAlpha(C_TAB_OFF, t), CATEGORIES[i].name);
            x += widths[i] + pad;
        }
        ResetTextStretchX();
    }

    // ---- selected-row gold->green diagonal bar (eased) ----
    {
        float mt = (float)ComputeMotion(g_moveStart, 0.0, 8.0);
        float slot = Lerp((float)g_prevSel, (float)g_sel, mt);
        float ry = ROWS_TOP + slot * ROW_PITCH;
        DrawVGradient({ CLIP_X, ry }, { CLIP_X + OPT_W, ry + ROW_H }, WithAlpha(C_SEL_TL, t), WithAlpha(C_SEL_BR, t));
    }

    // ---- rows ----
    for (int i = 0; i < OptCount(); ++i) {
        const Option& o = Opt(i);
        float ry = ROWS_TOP + i * ROW_PITCH, cy = ry + ROW_H * 0.5f;
        bool sel = (i == g_sel);
        SetFont(g_fSeurat);
        DrawTextAligned({ LABEL_X, ry }, { VAL_X0 - 10, ry + ROW_H }, 26.0f, WithAlpha(C_LABEL, t),
                        o.label, Align::Left, true, true);
        // value cell
        float vy0 = ry + (ROW_H - VAL_H) * 0.5f, vy1 = vy0 + VAL_H;
        DrawVGradient({ VAL_X0, vy0 }, { VAL_X0 + VAL_W, vy1 }, WithAlpha(C_VAL_BG, t), WithAlpha(C_VAL_BG_B, t));
        if (sel) { DrawCaret(VAL_X0 - 24, cy, false, WithAlpha(C_CARET, t)); DrawCaret(VAL_X0 + VAL_W + 24, cy, true, WithAlpha(C_CARET, t)); }
        SetFont(g_fRodin);
        if (o.kind == TOGGLE) {
            bool onv = o.val != 0;
            // ref: a small dim olive-yellow ~10px dot with a faint halo (x596..606)
            float lx = VAL_X0 + 15, ly = vy0 + (VAL_H - 10) * 0.5f;
            if (onv) DrawRect({ lx-3, ly-3 }, { lx+13, ly+13 }, WithAlpha(RGBA(255,255,0,50), t));
            DrawRect({ lx, ly }, { lx + 10, ly + 10 }, WithAlpha(onv ? C_LIGHT_ON : C_LIGHT_OFF, t));
            DrawTextAligned({ lx + 18, vy0 }, { VAL_X0 + VAL_W - 8, vy1 }, 20.0f, WithAlpha(onv ? C_VAL_TXT_T : RGBA(150,160,150,255), t), onv ? "ON" : "OFF", Align::Center, true, true);
        } else {
            const char* s = (o.choices && o.val < o.choiceCount) ? o.choices[o.val] : "";
            DrawValueText(s, VAL_X0 + 6, vy0, VAL_X0 + VAL_W - 6, vy1, t, sel);
        }
    }

    // ---- info panel: green-bordered preview-image region + description ----
    // The real per-value screenshot (SEGA art) drops into this bordered box; until
    // then it shows the dark placeholder. Matches options_menu's preview thumbnail.
    {
        float ix0 = IP_X0 + 18, ix1 = IP_X1 - 17;
        // preview box: the real video thumb is 240x140 (~16:9), centred in the panel
        float pvx0 = (IP_X0 + IP_X1) * 0.5f - 120.0f, pvx1 = pvx0 + 240.0f;
        float ty0 = IP_Y0 + 27, ty1 = ty0 + 141.0f;
        DrawRect({ pvx0, ty0 }, { pvx1, ty1 }, WithAlpha(RGBA(18,28,22,255), t));   // image placeholder fill
        uint32_t gb = WithAlpha(RGBA(0,168,46,255), t); const float L = 2.0f;       // bright-green border
        DrawRect({ pvx0, ty0 }, { pvx1, ty0 + L }, gb);
        DrawRect({ pvx0, ty1 - L }, { pvx1, ty1 }, gb);
        DrawRect({ pvx0, ty0 }, { pvx0 + L, ty1 }, gb);
        DrawRect({ pvx1 - L, ty0 }, { pvx1, ty1 }, gb);
        DrawTextAligned({ pvx0, ty0 }, { pvx1, ty1 }, 15.0f, WithAlpha(RGBA(110,140,116,255), t), "PREVIEW", Align::Center, true, true);
        const Option& s = Opt(std::clamp(g_sel, 0, OptCount() - 1));
        // description: centred word-wrapped paragraph, Seurat 28 white, line spacing 5,
        // per-value description appended after a blank line, clipped to the panel with
        // an auto-marquee when it overflows — the recomp's exact recipe (options_menu).
        SetFont(g_fSeurat);
        std::string full = s.desc ? s.desc : "";
        if (s.kind == ENUM && s.valDescs && s.val < s.choiceCount && s.valDescs[s.val] && s.valDescs[s.val][0]) {
            full += "\n\n"; full += s.valDescs[s.val];
        }
        const float fsz = 28.0f, lineH = fsz + 5.0f, wrapW = ix1 - ix0;
        std::vector<std::string> lines;
        size_t p = 0;
        while (p <= full.size()) {
            size_t nl = full.find('\n', p);
            std::string para = full.substr(p, nl == std::string::npos ? std::string::npos : nl - p);
            std::string cur;
            size_t w0 = 0;
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
        const float textTop = ty1 + 48, clipY0 = ty1 + 24, clipY1 = IP_Y1 - 14;
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
            // LEFT-aligned at a fixed margin (verified against the live capture)
            if (!ln.empty() && dy + lineH > clipY0 && dy < clipY1)
                DrawText({ ix0, dy }, fsz, WithAlpha(C_DESC, t), ln.c_str());
            dy += lineH;
        }
        PopClip();
    }

    // ---- footer button-guide + version ----
    // ref layout: [LB] Switch [RB] at left (x~176), (A) Select at x~816, (B) Back
    // right-aligned ending ~1087; guide row centred at y~634, just under the divider.
    {
        SetFont(g_fRodin);
        float hx = 176.0f, hcy = 634.0f;
        auto glyph = [&](const UV& g) { if (g_glyphTex < 0) return; float asp = ((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gh = 30.0f, gw = gh*asp; DrawImage(g_glyphTex, { hx, hcy-gh*0.5f }, { hx+gw, hcy+gh*0.5f }, { g.u0, g.v0 }, { g.u1, g.v1 }, WithAlpha(C_WHITE, t)); hx += gw + 8; };
        auto word = [&](const char* w, float pad){ DrawText({ hx, hcy-13 }, 24.0f, WithAlpha(C_WHITE, t), w); hx += MeasureText(24.0f, w).x + pad; };
        glyph(GLYPH_LB); word("Switch", 8); glyph(GLYPH_RB);
        hx = 816; glyph(GLYPH_A); word("Select", 0);
        hx = 994; glyph(GLYPH_B); word("Back", 0);
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
