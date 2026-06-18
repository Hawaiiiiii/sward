// =============================================================================
// screen_options.cpp — the SGFX settings screen. A scrolling, selectable list of
// operator settings on the left; a live help panel for the focused setting on the
// right. Built from primitives + text only (no chrome art, no third-party atlas)
// in the dark-IDE neutral aesthetic shared with screen_status.cpp / screen_town.cpp.
// Logo-only header (no wordmark). Up/Down move; Left/Right (or A) cycle the value.
// Toggle / enum values persist write-through to sgfx_settings.ini; path values are
// read-ish (shown, not edited here). The menu machinery (scroll, selection
// highlight, per-row value cell) is retained from the earlier ported options menu;
// the option DATA is the tool's real settings.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "settings.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

using namespace ui;

namespace {

// ---- setting model ----------------------------------------------------------
// A setting is either a CYCLE (a fixed list of choices, persisted as an index) or
// a PATH (a read-ish string value shown on the right, not edited on this screen).
enum Kind { CYCLE, PATH };

struct Setting {
    const char* label;
    Kind        kind;
    const char* const* choices;   // CYCLE: choice strings (first == default)
    int         choiceCount;
    int         val;              // CYCLE: current choice index
    const char* pathValue;       // PATH: the displayed value
    const char* help;            // right-panel help line for the focused setting
    const char* note;            // optional sub-line shown when the value warrants it
};

// choice tables (default is always the first entry)
const char* const CH_THEME[]   = { "Dark" };
const char* const CH_OFF_ON[]  = { "Off", "On" };
const char* const CH_ON_OFF[]  = { "On", "Off" };
const char* const CH_PROFILE[] = { "G65", "G70", "G45" };

// indices into SETTINGS that carry a contextual sub-line when toggled "On"
constexpr int IDX_AI = 1;

Setting SETTINGS[] = {
    { "Theme",                  CYCLE, CH_THEME,   1, 0, nullptr,
      "Dark IDE styling. There is no light mode.", nullptr },
    { "AI assist",              CYCLE, CH_OFF_ON,  2, 0, nullptr,
      "Heuristics run by default; AI is opt-in and off unless you turn it on.",
      "sends data to the cloud" },
    { "Default profile",        CYCLE, CH_PROFILE, 3, 0, nullptr,
      "The car profile new runs target unless you pick another.", nullptr },
    { "Open report after run",  CYCLE, CH_ON_OFF,  2, 0, nullptr,
      "Open the run report automatically once a preflight finishes.", nullptr },
    { "Desktop notifications",  CYCLE, CH_ON_OFF,  2, 0, nullptr,
      "Notify you when a long run or capture completes.", nullptr },
    { "Confirm before delivery",CYCLE, CH_ON_OFF,  2, 0, nullptr,
      "Ask for confirmation before a delivery check goes ahead.", nullptr },
    { "Verbose logging",        CYCLE, CH_OFF_ON,  2, 0, nullptr,
      "Write extra detail to the session log for troubleshooting.", nullptr },
    { "Check for updates",      CYCLE, CH_ON_OFF,  2, 0, nullptr,
      "Look for a newer build of the tool on launch.", nullptr },
    { "Source repo",            PATH,  nullptr,    0, 0, "C:\\repositories\\trunk",
      "Where the working copy of the car project lives.", nullptr },
    { "BMW Git",                PATH,  nullptr,    0, 0, "digital-3d-car-models",
      "The car-models repository the tool reads from.", nullptr },
};
constexpr int SETTING_COUNT = int(sizeof(SETTINGS) / sizeof(SETTINGS[0]));

// ---- palette (dark, neutral — matches screen_status.cpp / screen_town.cpp) ---
const uint32_t C_BG_TOP   = RGBA(12, 20, 38, 255), C_BG_BOT = RGBA(5, 9, 18, 255);
const uint32_t C_PANEL    = RGBA(16, 24, 40, 235);
const uint32_t C_PANEL_CAP= RGBA(10, 16, 28, 255);
const uint32_t C_SEL_TOP  = RGBA(64, 150, 235, 225), C_SEL_BOT = RGBA(28, 92, 180, 225);
const uint32_t C_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t C_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t C_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t C_DESC     = RGBA(178, 194, 214, 255);
const uint32_t C_RULE     = RGBA(120, 170, 230, 90);
const uint32_t C_LABEL    = RGBA(150, 170, 196, 255);
const uint32_t C_VALUE    = RGBA(120, 230, 140, 255);  // a cycled value (green, on)
const uint32_t C_VALUE_OFF= RGBA(150, 170, 196, 255);  // an "Off" value (muted)
const uint32_t C_PATHVAL  = RGBA(190, 205, 225, 230);  // a read-ish path value
const uint32_t C_NOTE     = RGBA(235, 200, 90, 255);   // the cloud-warning sub-line
const uint32_t C_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t C_WHITE    = RGBA(255, 255, 255, 255);

// ---- layout (reference px; mirrors screen_town.cpp two-panel idiom) ---------
constexpr float RULE_Y = 118.0f;
constexpr float LIST_X = 150.0f, LIST_Y = 158.0f, LIST_W = 600.0f, LIST_H = 404.0f;
constexpr float INFO_X = 780.0f, INFO_Y = 158.0f, INFO_W = 350.0f, INFO_H = 404.0f;
constexpr float HEADER_H = 52.0f;
constexpr float ROW_H = 56.0f;
constexpr int   VISIBLE_ROWS = 6;
constexpr float ROW_PAD = 14.0f;

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double TITLE_FRAMES = 14.0, LIST_FRAMES = 16.0;
constexpr double INFO_OFFSET = 5.0, INFO_FRAMES = 14.0;
constexpr double FOOT_OFFSET = 10.0, FOOT_FRAMES = 12.0;
constexpr double SELECT_MOVE_FRAMES = 8.0;

// ---- state ------------------------------------------------------------------
int    g_sel = 0, g_prevSel = 0, g_scroll = 0;
double g_moveStart = -100.0;
int    g_logoTex = -1;
int    g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

float ScrollLimit() { return (float)std::max(0, SETTING_COUNT - VISIBLE_ROWS); }
void  ClampScrollToSel() {
    if (g_sel < g_scroll) g_scroll = g_sel;
    if (g_sel > g_scroll + VISIBLE_ROWS - 1) g_scroll = g_sel - VISIBLE_ROWS + 1;
    g_scroll = std::clamp(g_scroll, 0, (int)ScrollLimit());
}

// CYCLE settings persist their index under "set_<row>" in sgfx_settings.ini.
void LoadSavedValues() {
    char key[32];
    for (int i = 0; i < SETTING_COUNT; ++i) {
        Setting& s = SETTINGS[i];
        if (s.kind != CYCLE) continue;
        std::snprintf(key, sizeof key, "set_%d", i);
        s.val = std::clamp(settings::GetInt(key, s.val), 0, std::max(0, s.choiceCount - 1));
    }
}

void Init() {
    if (g_logoTex < 0) g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
    static bool loaded = false;
    if (!loaded) { LoadSavedValues(); loaded = true; }
}
void Reset() {
    g_sel = 0; g_prevSel = 0; g_scroll = 0; g_moveStart = -100.0;
}
void Cycle(int dir) {   // dir = +1 forward / -1 back; only CYCLE settings change
    Setting& s = SETTINGS[std::clamp(g_sel, 0, SETTING_COUNT - 1)];
    if (s.kind != CYCLE || s.choiceCount <= 1) return;
    s.val = (s.val + (dir > 0 ? 1 : s.choiceCount - 1)) % s.choiceCount;
    char key[32];
    std::snprintf(key, sizeof key, "set_%d", std::clamp(g_sel, 0, SETTING_COUNT - 1));
    settings::SetInt(key, s.val);                 // persists immediately
}
void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = std::max(0, g_sel - 1);
        if (in.down) g_sel = std::min(SETTING_COUNT - 1, g_sel + 1);
        ClampScrollToSel();
        g_moveStart = Now();
    }
    if (in.right || in.accept) Cycle(+1);
    else if (in.left)          Cycle(-1);
}

// a neutral dark panel with a caption strip + rule (screen_town.cpp idiom).
void DrawPanel(float x, float y, float w, float h, float t, const char* caption) {
    DrawRect({ x, y }, { x + w, y + h }, WithAlpha(C_PANEL, t));
    DrawRect({ x, y }, { x + w, y + HEADER_H }, WithAlpha(C_PANEL_CAP, t));
    DrawRect({ x + 12, y + HEADER_H - 2 }, { x + w - 12, y + HEADER_H }, WithAlpha(C_RULE, t));
    SetFont(g_fDF);
    DrawTextAligned({ x + 18, y }, { x + w - 14, y + HEADER_H }, 26.0f,
                    WithAlpha(C_TITLE, t), caption, Align::Left, true, true);
    ResetFont();
}

// host logo drops into the top-left slot; if it failed to load, draw NOTHING.
void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 40 }, { 40 + w, 40 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, t));
}

// the right-hand value for a row, drawn right-aligned within the list panel.
void DrawRowValue(const Setting& s, float rowL, float rowR, float top, bool selected, float t) {
    SetFont(g_fRodin);
    if (s.kind == PATH) {
        DrawTextAligned({ rowL + 200.0f, top }, { rowR - 8.0f, top + ROW_H }, 20.0f,
                        WithAlpha(C_PATHVAL, t), s.pathValue ? s.pathValue : "", Align::Right, true, true);
    } else {
        const char* v = (s.choices && s.val < s.choiceCount) ? s.choices[s.val] : "";
        bool isOff = (std::strcmp(v, "Off") == 0);
        uint32_t vc = isOff ? C_VALUE_OFF : C_VALUE;
        // a focused, multi-choice value flags it can be cycled with arrows
        if (selected && s.choiceCount > 1) {
            char buf[48];
            std::snprintf(buf, sizeof buf, "< %s >", v);
            DrawTextAligned({ rowL + 200.0f, top }, { rowR - 8.0f, top + ROW_H }, 22.0f,
                            WithAlpha(vc, t), buf, Align::Right, true, true);
        } else {
            DrawTextAligned({ rowL + 200.0f, top }, { rowR - 8.0f, top + ROW_H }, 22.0f,
                            WithAlpha(vc, t), v, Align::Right, true, true);
        }
    }
    ResetFont();
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);

    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float listT  = (float)ComputeMotion(openSec, 0.0, LIST_FRAMES);
    const float infoT  = (float)ComputeMotion(openSec, INFO_OFFSET, INFO_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ===== HEADER: logo slot only (no wordmark / title text) ==================
    DrawLogoSlot(titleT);
    DrawRect({ LIST_X, RULE_Y }, { 1130.0f, RULE_Y + 2.0f }, WithAlpha(C_RULE, titleT));

    // ===== LEFT: SETTINGS LIST ===============================================
    const float lx = LIST_X, ly = LIST_Y + (1.0f - listT) * 24.0f;
    DrawPanel(lx, ly, LIST_W, LIST_H, listT, "SETTINGS");
    const float rowsTop = ly + HEADER_H + 10.0f;
    const float rowL = lx + ROW_PAD, rowR = lx + LIST_W - ROW_PAD;

    // selection highlight (eased between previous + current slot)
    if (listT > 0.5f) {
        float moveT = (float)ComputeMotion(g_moveStart, 0.0, SELECT_MOVE_FRAMES);
        float prevSlot = std::clamp((float)(g_prevSel - g_scroll), 0.0f, (float)(VISIBLE_ROWS - 1));
        float curSlot  = std::clamp((float)(g_sel     - g_scroll), 0.0f, (float)(VISIBLE_ROWS - 1));
        float slot = Lerp(prevSlot, curSlot, moveT);
        float hy = rowsTop + slot * ROW_H;
        DrawVGradient({ rowL, hy + 3 }, { rowR, hy + ROW_H - 5 },
                      WithAlpha(C_SEL_TOP, listT), WithAlpha(C_SEL_BOT, listT));
    }

    for (int row = 0; row < VISIBLE_ROWS; ++row) {
        int idx = g_scroll + row;
        if (idx >= SETTING_COUNT) break;
        float top = rowsTop + row * ROW_H;
        bool selected = (idx == g_sel);
        const Setting& s = SETTINGS[idx];
        SetFont(g_fSeurat);
        DrawTextAligned({ rowL + 18.0f, top }, { rowL + 200.0f, top + ROW_H }, 26.0f,
                        WithAlpha(selected ? C_TEXT_SEL : C_TEXT, listT),
                        s.label, Align::Left, true, true);
        ResetFont();
        DrawRowValue(s, rowL, rowR, top, selected, listT);
    }

    // scrollbar (when the list overflows)
    if (SETTING_COUNT > VISIBLE_ROWS && listT > 0.5f) {
        float trackX = lx + LIST_W - 7.0f, trackTop = rowsTop, trackH = VISIBLE_ROWS * ROW_H;
        DrawRect({ trackX, trackTop }, { trackX + 3, trackTop + trackH }, WithAlpha(C_RULE, listT));
        float thumbH = trackH * (float)VISIBLE_ROWS / (float)SETTING_COUNT;
        float denom = ScrollLimit(); if (denom < 1.0f) denom = 1.0f;
        float thumbY = trackTop + (trackH - thumbH) * ((float)g_scroll / denom);
        DrawRect({ trackX, thumbY }, { trackX + 3, thumbY + thumbH }, WithAlpha(C_SEL_TOP, listT));
    }

    // ===== RIGHT: HELP (tracks the selection) ================================
    const float ix = INFO_X, iy = INFO_Y + (1.0f - infoT) * 24.0f;
    DrawPanel(ix, iy, INFO_W, INFO_H, infoT, "ABOUT");
    const Setting& sel = SETTINGS[std::clamp(g_sel, 0, SETTING_COUNT - 1)];
    float textTop = iy + HEADER_H + 28.0f;
    DrawRect({ ix + 24.0f, textTop - 10.0f }, { ix + 60.0f, textTop - 6.0f }, WithAlpha(C_SEL_TOP, infoT)); // accent
    SetFont(g_fSeurat);
    DrawTextAligned({ ix + 24.0f, textTop }, { ix + INFO_W - 18.0f, textTop + 40.0f }, 30.0f,
                    WithAlpha(C_TEXT_SEL, infoT), sel.label, Align::Left, true, true);
    // help line (word-wrapped to the panel width)
    {
        const float wrapW = INFO_W - 48.0f;
        const float fsz = 20.0f, lineH = fsz + 6.0f;
        const char* help = sel.help ? sel.help : "";
        std::string cur; float dy = textTop + 58.0f;
        std::string text = help;
        size_t w0 = 0;
        while (w0 <= text.size()) {
            size_t w1 = text.find(' ', w0); if (w1 == std::string::npos) w1 = text.size();
            std::string word = text.substr(w0, w1 - w0);
            std::string trial = cur.empty() ? word : cur + " " + word;
            if (!cur.empty() && MeasureText(fsz, trial.c_str()).x > wrapW) {
                DrawText({ ix + 26.0f, dy }, fsz, WithAlpha(C_DESC, infoT), cur.c_str());
                dy += lineH; cur = word;
            } else cur = trial;
            if (w1 >= text.size()) break;
            w0 = w1 + 1;
        }
        if (!cur.empty()) { DrawText({ ix + 26.0f, dy }, fsz, WithAlpha(C_DESC, infoT), cur.c_str()); dy += lineH; }

        // contextual sub-line: the cloud warning when AI assist is On
        bool aiOn = (std::clamp(g_sel, 0, SETTING_COUNT - 1) == IDX_AI) && (sel.val != 0);
        if (sel.note && aiOn) {
            dy += 8.0f;
            DrawText({ ix + 26.0f, dy }, fsz, WithAlpha(C_NOTE, infoT), sel.note);
        }
    }
    ResetFont();

    // ===== FOOTER (plain text) ===============================================
    {
        SetFont(g_fRodin);
        DrawRect({ LIST_X, 612.0f }, { 1130.0f, 614.0f }, WithAlpha(C_RULE, footT));
        DrawText({ LIST_X, 628.0f }, 20.0f, WithAlpha(C_FOOTER, footT), "Up/Down  Select");
        DrawText({ LIST_X + 230.0f, 628.0f }, 20.0f, WithAlpha(C_FOOTER, footT), "Left/Right  Change");
        DrawText({ LIST_X + 480.0f, 628.0f }, 20.0f, WithAlpha(C_FOOTER, footT), "Esc  Back");
        ResetFont();
    }
}

} // namespace

void OptionsInit() { Init(); }
void OptionsDraw(double openSeconds) { Draw(openSeconds); }
void OptionsInput(const ScreenInput& in) { Input(in); }
void OptionsReset() { Reset(); }
