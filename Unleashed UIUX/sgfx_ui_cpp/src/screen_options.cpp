// =============================================================================
// screen_options.cpp — the settings screen. A single, full-width scrolling list
// (the smoother, original menu shape): one column of rows, each a label with its
// value on the right, an eased selection highlight, smoothly eased scrolling, and a
// one-line help strip at the bottom that tracks the focused row. Built from
// primitives + text only, dark-IDE neutral. Up/Down move; Left/Right (or A) change
// the focused value. CYCLE values persist to sgfx_settings.ini; PATH values are
// shown, not edited here. No car is hard-coded — the active profile is auto-detected
// from the run.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "settings.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <algorithm>

using namespace ui;

namespace {

// ---- setting model ----------------------------------------------------------
enum Kind { CYCLE, PATH };
struct Setting {
    const char* label;
    Kind        kind;
    const char* const* choices;   // CYCLE: choice strings (first == default)
    int         choiceCount;
    int         val;              // CYCLE: current index
    const char* pathValue;       // PATH: displayed value
    const char* help;            // bottom help strip
    const char* note;            // contextual warning shown when value is "On"
};

const char* const CH_AUTO[]   = { "Auto-detect", "Ask each run" };
const char* const CH_ON_OFF[] = { "On", "Off" };
const char* const CH_OFF_ON[] = { "Off", "On" };
const char* const CH_FORMAT[] = { "JSON", "JSON + HTML" };
const char* const CH_PAR[]    = { "1", "2", "4" };
const char* const CH_THEME[]  = { "Dark" };

Setting SETTINGS[] = {
    { "Active profile",       CYCLE, CH_AUTO,   2, 0, nullptr, "Which car slice a run targets. Auto-detect reads it from the run.", nullptr },
    { "Report format",        CYCLE, CH_FORMAT, 2, 0, nullptr, "What each run writes for the record.", nullptr },
    { "Screenshot battery",   CYCLE, CH_ON_OFF, 2, 0, nullptr, "Capture the standard test views every run.", nullptr },
    { "Baseline compare",     CYCLE, CH_ON_OFF, 2, 0, nullptr, "Diff new screenshots against the last accepted baseline.", nullptr },
    { "Parallel runs",        CYCLE, CH_PAR,    3, 0, nullptr, "How many profiles to preflight at once.", nullptr },
    { "Open report after run",CYCLE, CH_OFF_ON, 2, 0, nullptr, "Open the report as soon as a run finishes.", nullptr },
    { "Desktop notifications",CYCLE, CH_ON_OFF, 2, 0, nullptr, "Notify you when a run or capture completes.", nullptr },
    { "AI assist",            CYCLE, CH_OFF_ON, 2, 0, nullptr, "Heuristics run by default; AI is opt-in and off unless you turn it on.", "sends data to the cloud" },
    { "Cloud upload",         CYCLE, CH_OFF_ON, 2, 0, nullptr, "Results stay on this machine unless you turn this on.", "sends data to the cloud" },
    { "Confluence sync",      CYCLE, CH_OFF_ON, 2, 0, nullptr, "Pull workflow references from the team space.", nullptr },
    { "Weekly ticket draft",  CYCLE, CH_OFF_ON, 2, 0, nullptr, "Draft the weekly ticket list for you to review and send.", nullptr },
    { "Theme",                CYCLE, CH_THEME,  1, 0, nullptr, "Dark IDE styling. There is no light mode.", nullptr },
    { "Source repo",          PATH,  nullptr,   0, 0, "C:\\repositories\\trunk",     "Where the working copy of the car project lives.", nullptr },
    { "BMW Git",              PATH,  nullptr,   0, 0, "digital-3d-car-models",       "The car-models repository the tool reads from.", nullptr },
    { "Output folder",        PATH,  nullptr,   0, 0, "...\\sg-preflight\\out",      "Where reports and screenshots are written.", nullptr },
};
constexpr int SETTING_COUNT = int(sizeof(SETTINGS) / sizeof(SETTINGS[0]));

// ---- palette ----------------------------------------------------------------
const uint32_t C_BG_TOP   = RGBA(12, 20, 38, 255), C_BG_BOT = RGBA(5, 9, 18, 255);
const uint32_t C_PANEL    = RGBA(16, 24, 40, 235);
const uint32_t C_PANEL_CAP= RGBA(10, 16, 28, 255);
const uint32_t C_SEL_TOP  = RGBA(64, 150, 235, 235), C_SEL_BOT = RGBA(28, 92, 180, 235);
const uint32_t C_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t C_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t C_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t C_DESC     = RGBA(190, 205, 224, 255);
const uint32_t C_RULE     = RGBA(120, 170, 230, 90);
const uint32_t C_ROWRULE  = RGBA(60, 84, 120, 70);
const uint32_t C_VALUE    = RGBA(120, 230, 140, 255);
const uint32_t C_VALUE_OFF= RGBA(150, 170, 196, 255);
const uint32_t C_PATHVAL  = RGBA(190, 205, 225, 230);
const uint32_t C_NOTE     = RGBA(235, 200, 90, 255);
const uint32_t C_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t C_WHITE    = RGBA(255, 255, 255, 255);

// ---- layout (reference px, 1280x720) ----------------------------------------
constexpr float RULE_Y   = 118.0f;
constexpr float LIST_X   = 150.0f, LIST_Y = 150.0f, LIST_W = 980.0f;
constexpr float HEADER_H = 50.0f;
constexpr float ROW_H    = 48.0f, ROW_PAD = 16.0f;
constexpr int   VISIBLE_ROWS = 7;
constexpr float ROWS_TOP = LIST_Y + HEADER_H + 8.0f;
constexpr float ROWS_H   = VISIBLE_ROWS * ROW_H;
constexpr float LIST_H   = HEADER_H + 8.0f + ROWS_H + 10.0f;
constexpr float HELP_Y   = LIST_Y + LIST_H + 12.0f, HELP_H = 50.0f;
constexpr float FOOT_Y   = 632.0f;

// ---- entrance + motion tuning (frames @60fps) -------------------------------
constexpr double TITLE_FRAMES = 14.0, LIST_FRAMES = 16.0;
constexpr double HELP_OFFSET = 6.0, HELP_FRAMES = 13.0, FOOT_OFFSET = 10.0, FOOT_FRAMES = 12.0;
constexpr double SELECT_MOVE_FRAMES = 9.0, SCROLL_FRAMES = 12.0;

// ---- state ------------------------------------------------------------------
int    g_sel = 0, g_prevSel = 0, g_scroll = 0;
double g_moveStart = -100.0, g_scrollStart = -100.0;
float  g_scrollPrev = 0.0f, g_scrollF = 0.0f;
int    g_logoTex = -1, g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

int  ScrollLimit() { return std::max(0, SETTING_COUNT - VISIBLE_ROWS); }
void RetargetScroll() {
    int target = g_scroll;
    if (g_sel < target) target = g_sel;
    if (g_sel > target + VISIBLE_ROWS - 1) target = g_sel - VISIBLE_ROWS + 1;
    target = std::clamp(target, 0, ScrollLimit());
    if (target != g_scroll) { g_scrollPrev = g_scrollF; g_scroll = target; g_scrollStart = Now(); }
}

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
    g_sel = 0; g_prevSel = 0; g_scroll = 0;
    g_moveStart = -100.0; g_scrollStart = -100.0; g_scrollPrev = 0.0f; g_scrollF = 0.0f;
}
void Cycle(int dir) {
    Setting& s = SETTINGS[std::clamp(g_sel, 0, SETTING_COUNT - 1)];
    if (s.kind != CYCLE || s.choiceCount <= 1) return;
    s.val = (s.val + (dir > 0 ? 1 : s.choiceCount - 1)) % s.choiceCount;
    char key[32];
    std::snprintf(key, sizeof key, "set_%d", std::clamp(g_sel, 0, SETTING_COUNT - 1));
    settings::SetInt(key, s.val);
}
void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = std::max(0, g_sel - 1);
        if (in.down) g_sel = std::min(SETTING_COUNT - 1, g_sel + 1);
        g_moveStart = Now();
        RetargetScroll();
    }
    if (in.right || in.accept) Cycle(+1);
    else if (in.left)          Cycle(-1);
}

void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 40 }, { 40 + w, 40 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, t));
}

// the right-hand value cell for a row.
void DrawRowValue(const Setting& s, float rowR, float top, bool selected, float t) {
    SetFont(g_fRodin);
    if (s.kind == PATH) {
        DrawTextAligned({ rowR - 520.0f, top }, { rowR, top + ROW_H }, 20.0f,
                        WithAlpha(C_PATHVAL, t), s.pathValue ? s.pathValue : "", Align::Right, true, true);
    } else {
        const char* v = (s.choices && s.val < s.choiceCount) ? s.choices[s.val] : "";
        uint32_t vc = (std::strcmp(v, "Off") == 0) ? C_VALUE_OFF : C_VALUE;
        char buf[48];
        if (selected && s.choiceCount > 1) { std::snprintf(buf, sizeof buf, "< %s >", v); v = buf; }
        DrawTextAligned({ rowR - 360.0f, top }, { rowR, top + ROW_H }, 22.0f,
                        WithAlpha(vc, t), v, Align::Right, true, true);
    }
    ResetFont();
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);

    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float listT  = (float)ComputeMotion(openSec, 0.0, LIST_FRAMES);
    const float helpT  = (float)ComputeMotion(openSec, HELP_OFFSET, HELP_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // eased scroll position (fractional during a move)
    float scrollT = (float)ComputeMotion(g_scrollStart, 0.0, SCROLL_FRAMES);
    g_scrollF = Lerp(g_scrollPrev, (float)g_scroll, scrollT);

    // header: logo slot only, then the rule
    DrawLogoSlot(titleT);
    DrawRect({ LIST_X, RULE_Y }, { LIST_X + LIST_W, RULE_Y + 2.0f }, WithAlpha(C_RULE, titleT));

    // list panel + caption
    const float lx = LIST_X, ly = LIST_Y + (1.0f - listT) * 22.0f;
    DrawRect({ lx, ly }, { lx + LIST_W, ly + LIST_H }, WithAlpha(C_PANEL, listT));
    DrawRect({ lx, ly }, { lx + LIST_W, ly + HEADER_H }, WithAlpha(C_PANEL_CAP, listT));
    DrawRect({ lx + 12, ly + HEADER_H - 2 }, { lx + LIST_W - 12, ly + HEADER_H }, WithAlpha(C_RULE, listT));
    SetFont(g_fDF);
    DrawTextAligned({ lx + 20, ly }, { lx + LIST_W - 14, ly + HEADER_H }, 26.0f,
                    WithAlpha(C_TITLE, listT), "SETTINGS", Align::Left, true, true);
    ResetFont();

    const float rowsTop = ly + HEADER_H + 8.0f;
    const float rowL = lx + ROW_PAD, rowR = lx + LIST_W - ROW_PAD;

    // clip the rows to the list viewport, so fractional scrolling clips cleanly
    PushClip({ lx, rowsTop }, { lx + LIST_W, rowsTop + ROWS_H });

    // eased selection highlight, in scrolled coordinates
    if (listT > 0.4f) {
        float moveT  = (float)ComputeMotion(g_moveStart, 0.0, SELECT_MOVE_FRAMES);
        float litSel = Lerp((float)g_prevSel, (float)g_sel, moveT);
        float hy = rowsTop + (litSel - g_scrollF) * ROW_H;
        DrawVGradient({ rowL, hy + 3 }, { rowR, hy + ROW_H - 5 },
                      WithAlpha(C_SEL_TOP, listT), WithAlpha(C_SEL_BOT, listT));
    }

    int first = std::max(0, (int)std::floor(g_scrollF) - 1);
    int last  = std::min(SETTING_COUNT, (int)std::ceil(g_scrollF) + VISIBLE_ROWS + 1);
    for (int idx = first; idx < last; ++idx) {
        float top = rowsTop + ((float)idx - g_scrollF) * ROW_H;
        bool selected = (idx == g_sel);
        const Setting& s = SETTINGS[idx];
        if (idx > 0) DrawRect({ rowL, top }, { rowR, top + 1.0f }, WithAlpha(C_ROWRULE, listT));
        SetFont(g_fSeurat);
        DrawTextAligned({ rowL + 14.0f, top }, { rowR - 360.0f, top + ROW_H }, 25.0f,
                        WithAlpha(selected ? C_TEXT_SEL : C_TEXT, listT), s.label, Align::Left, true, true);
        ResetFont();
        DrawRowValue(s, rowR, top, selected, listT);
    }
    PopClip();

    // scrollbar (smooth, follows the eased scroll)
    if (SETTING_COUNT > VISIBLE_ROWS && listT > 0.4f) {
        float trackX = lx + LIST_W - 7.0f;
        DrawRect({ trackX, rowsTop }, { trackX + 3, rowsTop + ROWS_H }, WithAlpha(C_RULE, listT * 0.7f));
        float thumbH = ROWS_H * (float)VISIBLE_ROWS / (float)SETTING_COUNT;
        float denom  = std::max(1, ScrollLimit());
        float thumbY = rowsTop + (ROWS_H - thumbH) * (g_scrollF / denom);
        DrawRect({ trackX, thumbY }, { trackX + 3, thumbY + thumbH }, WithAlpha(C_SEL_TOP, listT));
    }

    // bottom help strip: the focused row's one-liner + any contextual warning
    if (helpT > 0.0f) {
        DrawRect({ LIST_X, HELP_Y }, { LIST_X + LIST_W, HELP_Y + HELP_H }, WithAlpha(C_PANEL, helpT));
        DrawRect({ LIST_X, HELP_Y }, { LIST_X + 4.0f, HELP_Y + HELP_H }, WithAlpha(C_SEL_TOP, helpT));
        const Setting& sel = SETTINGS[std::clamp(g_sel, 0, SETTING_COUNT - 1)];
        SetFont(g_fSeurat);
        DrawTextAligned({ LIST_X + 20.0f, HELP_Y }, { LIST_X + LIST_W - 360.0f, HELP_Y + HELP_H }, 20.0f,
                        WithAlpha(C_DESC, helpT), sel.help ? sel.help : "", Align::Left, true, true);
        bool on = sel.kind == CYCLE && sel.choices && sel.val < sel.choiceCount &&
                  std::strcmp(sel.choices[sel.val], "On") == 0;
        if (sel.note && on)
            DrawTextAligned({ LIST_X + LIST_W - 360.0f, HELP_Y }, { LIST_X + LIST_W - 16.0f, HELP_Y + HELP_H }, 19.0f,
                            WithAlpha(C_NOTE, helpT), sel.note, Align::Right, true, true);
        ResetFont();
    }

    // footer
    SetFont(g_fRodin);
    DrawRect({ LIST_X, FOOT_Y - 16.0f }, { LIST_X + LIST_W, FOOT_Y - 14.0f }, WithAlpha(C_RULE, footT));
    DrawText({ LIST_X, FOOT_Y }, 20.0f, WithAlpha(C_FOOTER, footT), "Up/Down  Select");
    DrawText({ LIST_X + 230.0f, FOOT_Y }, 20.0f, WithAlpha(C_FOOTER, footT), "Left/Right  Change");
    DrawText({ LIST_X + 490.0f, FOOT_Y }, 20.0f, WithAlpha(C_FOOTER, footT), "Esc  Back");
    ResetFont();
}

} // namespace

void OptionsInit() { Init(); }
void OptionsDraw(double openSeconds) { Draw(openSeconds); }
void OptionsInput(const ScreenInput& in) { Input(in); }
void OptionsReset() { Reset(); }
