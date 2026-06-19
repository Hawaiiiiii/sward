// =============================================================================
// screen_options.cpp — the settings screen in the cinematic green look: a tabbed
// container (RUN / REVIEW / LINKS / DISPLAY), a scrolling option list with value
// plates, toggle lights and sliders, and a right info panel with a description.
// The green chrome (container, plates, backdrop, palette) is shared via
// green_chrome.h so the settings-family screens read as one console. Drawn from
// primitives + the bundled fonts only (no game sprite art). The options are the
// tool's own, profile-agnostic settings; values persist to sgfx_settings.ini.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "settings.h"
#include "green_chrome.h"

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
constexpr float GRID = chrome::GRID;
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

// tab gradient (options-specific)
const uint32_t C_TAB_G_T = RGBA(128, 255, 0, 255);
const uint32_t C_TAB_G_B = RGBA(255, 192, 0, 255);
const uint32_t C_SLOT_LBL= RGBA(150, 190, 150, 255);

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

// tab label: lime->gold gradient + black outline + bevel (options-specific)
void DrawTabText(float x, float y, const char* s, float alpha) {
    uint8_t a = (uint8_t)std::clamp((int)lround(alpha), 0, 255);
    static const float O[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
    for (auto& o : O) DrawText({ x + o[0]*1.6f, y + o[1]*1.6f }, 32.0f, RGBA(0,0,0,a), s);
    SetModifier(MOD_TITLE_BEVEL);
    DrawTextGradient({ x, y }, 32.0f, WithAlpha(C_TAB_G_T, a/255.0f), WithAlpha(C_TAB_G_B, a/255.0f), s);
    ResetModifier();
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "SETTINGS");

    chrome::Container(SP_X0, SP_Y0, SP_X1, SP_Y1, true,  b.line, b.outer, b.inner, b.bg);
    chrome::Container(IP_X0, IP_Y0, IP_X1, IP_Y1, false, b.line, b.outer, b.inner, b.bg);

    // tabs (RUN/REVIEW/LINKS/DISPLAY)
    {
        SetFont(g_fDF);
        float clipW = (SP_X1 - GRID*2) - CLIP_X;
        float widths[CATEGORY_COUNT], sum = 0;
        for (int i = 0; i < CATEGORY_COUNT; ++i) { widths[i] = MeasureText(32.0f, CATEGORIES[i].name).x; sum += widths[i]; }
        float pad = (clipW - sum) / (CATEGORY_COUNT + 1);
        float x = CLIP_X + pad;
        for (int i = 0; i < CATEGORY_COUNT; ++i) {
            if (i == g_cat) { float tabPad = std::min(pad * 0.5f, GRID * 3); chrome::Plate(x - tabPad, TABS_Y, x + widths[i] + tabPad, TABS_Y + TAB_H, t); }
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
        uint32_t gold = WithAlpha(chrome::C_SEL_TL, t), grn = WithAlpha(chrome::C_SEL_BR, t);
        uint32_t mid  = WithAlpha(ColourLerp(chrome::C_SEL_TL, chrome::C_SEL_BR, 0.5f), t);
        DrawQuadGradient({ CLIP_X, ry }, { CLIP_X + OPT_W, ry + ROW_H }, gold, mid, grn, mid);
    }
    for (int i = g_first; i < OptCount() && i <= g_first + VIS_ROWS; ++i) {
        const Option& o = Opt(i);
        float ry = ROWS_TOP + (i - g_first) * ROW_PITCH;
        bool sel = (i == g_sel);
        SetFont(g_fSeurat);
        DrawTextAligned({ LABEL_X, ry }, { VAL_X0 - 10, ry + ROW_H }, 26.0f, WithAlpha(chrome::C_LABEL, t), o.label, Align::Left, true, true);
        float vy0 = ry + (ROW_H - VAL_H) * 0.5f, vy1 = vy0 + VAL_H;
        chrome::Plate(VAL_X0, vy0, VAL_X0 + VAL_W, vy1, t);
        if (sel) chrome::SelectionArrows(VAL_X0, vy0, VAL_X0 + VAL_W, vy1, t);
        SetFont(g_fRodin);
        if (o.kind == TOGGLE) {
            bool onv = o.val != 0;
            const float ls = 15.0f, lx = VAL_X0 + 14.0f, ly = vy0 + ((VAL_H - ls) * 0.5f);
            chrome::ToggleLight(lx, ly, ls, onv, t);
            chrome::ValueText(onv ? "ON" : "OFF", VAL_X0 + 6, vy0, VAL_X0 + VAL_W - 6, vy1, t);
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
            chrome::ValueText(buf, VAL_X0 + 6, vy0, VAL_X0 + VAL_W - 6, vy1, t);
        } else {
            const char* s = (o.choices && o.val < o.choiceCount) ? o.choices[o.val] : "";
            chrome::ValueText(s, VAL_X0 + 6, vy0, VAL_X0 + VAL_W - 6, vy1, t);
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
        DrawRect({ ix0, ty0 }, { ix1, ty1 }, WithAlpha(RGBA(4,12,6,235), t));
        DrawVGradient({ ix0, ty0 }, { ix1, ty1 }, WithAlpha(RGBA(0,40,0,70), t), WithAlpha(RGBA(0,0,0,90), t));
        uint32_t kl = WithAlpha(RGBA(0,89,0,200), t); const float L = 1.0f;
        DrawRect({ ix0, ty0 }, { ix1, ty0+L }, kl); DrawRect({ ix0, ty1-L }, { ix1, ty1 }, kl);
        DrawRect({ ix0, ty0 }, { ix0+L, ty1 }, kl); DrawRect({ ix1-L, ty0 }, { ix1, ty1 }, kl);
        char val[24];
        if (s.kind == TOGGLE)      snprintf(val, sizeof val, "%s", s.val ? "ON" : "OFF");
        else if (s.kind == SLIDER) snprintf(val, sizeof val, "%d%s", s.val, s.suffix ? s.suffix : "");
        else                       snprintf(val, sizeof val, "%s", (s.choices && s.val < s.choiceCount) ? s.choices[s.val] : "");
        SetFont(g_fDF);
        DrawTextAligned({ ix0, ty0 + 18 }, { ix1, ty0 + 46 }, 16.0f, WithAlpha(C_SLOT_LBL, t), s.label, Align::Center, true, true);
        chrome::ValueText(val, ix0, ty0 + thumbH*0.45f - 4, ix1, ty0 + thumbH*0.45f + 30, t);

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
            DrawText({ ix0 + (wrapW - lw) * 0.5f, dy }, fsz, WithAlpha(chrome::C_DESC, t), ln.c_str());
            dy += lineH;
        }
    }

    // footer (text; no glyph atlas)
    SetFont(g_fRodin);
    DrawText({ 250, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "LB/RB  Switch tab");
    DrawText({ 470, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Left/Right  Change");
    DrawText({ 700, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back");
    ResetFont();
}

} // namespace

void OptionsInit() { Init(); }
void OptionsDraw(double openSeconds) { Draw(openSeconds); }
void OptionsInput(const ScreenInput& in) { Input(in); }
void OptionsReset() { Reset(); }
