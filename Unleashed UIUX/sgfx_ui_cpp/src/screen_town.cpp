// =============================================================================
// screen_town.cpp — the operator action hub. A two-panel menu: a scrolling list
// of actions on the left, a live detail of the focused action on the right. Built
// from primitives + text only (no chrome art, no third-party atlas) so it ships on
// its own. The actions are the tool's real verbs; running one shows a transient
// confirmation here (it wires to the real work in-flow).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

using namespace ui;

namespace {

// ---- action model -----------------------------------------------------------
struct Action { const char* label; const char* info1; const char* info2; };
const Action ACTIONS[] = {
    { "Run preflight",       "Full SG-side checks across",   "anchors, constants, carpaints." },
    { "Capture screenshots", "Export via the BMW pipeline",  "and snap the test views."       },
    { "Check delivery",      "Readiness across the car",     "models and their changelogs."   },
    { "Daily digest",        "Run every live profile and",   "summarise the morning state."   },
    { "Scan unused Lua",     "Find Lua files that survived", "into the project root."         },
    { "Manual review",       "Open the queue of items",      "awaiting a human verdict."      },
};
constexpr int ACTION_COUNT = int(sizeof(ACTIONS) / sizeof(ACTIONS[0]));

// the profile the actions run against (representative until a live feed supplies it)
const char* const ACTIVE_PROFILE = "G65";

int g_logoTex = -1;

// ---- layout (reference px) --------------------------------------------------
constexpr float RULE_Y = 118.0f;
constexpr float LIST_X = 150.0f, LIST_Y = 158.0f, LIST_W = 600.0f, LIST_H = 404.0f;
constexpr float INFO_X = 780.0f, INFO_Y = 158.0f, INFO_W = 350.0f, INFO_H = 404.0f;
constexpr float HEADER_H = 52.0f;
constexpr float ROW_H = 60.0f;
constexpr int   VISIBLE_ROWS = 5;
constexpr float ROW_PAD = 14.0f;

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double TITLE_FRAMES = 14.0, LIST_FRAMES = 16.0;
constexpr double INFO_OFFSET = 5.0, INFO_FRAMES = 14.0;
constexpr double FOOT_OFFSET = 10.0, FOOT_FRAMES = 12.0;
constexpr double SELECT_MOVE_FRAMES = 8.0;

// ---- palette (dark, neutral) ------------------------------------------------
const uint32_t COL_BG_TOP   = RGBA(12, 20, 38, 255);
const uint32_t COL_BG_BOT   = RGBA(5, 9, 18, 255);
const uint32_t COL_PANEL    = RGBA(16, 24, 40, 235);
const uint32_t COL_PANEL_CAP= RGBA(10, 16, 28, 255);
const uint32_t COL_SEL_TOP  = RGBA(64, 150, 235, 225);
const uint32_t COL_SEL_BOT  = RGBA(28, 92, 180, 225);
const uint32_t COL_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t COL_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t COL_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t COL_DESC     = RGBA(178, 194, 214, 255);
const uint32_t COL_RULE     = RGBA(120, 170, 230, 90);
const uint32_t COL_OK       = RGBA(120, 230, 140, 255);
const uint32_t COL_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t COL_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t COL_CHIP     = RGBA(150, 196, 150, 255);

// ---- interactive state ------------------------------------------------------
int    g_sel = 0, g_prevSel = 0, g_scroll = 0;
double g_moveStart = -100.0, g_msgStart = -100.0;
const char* g_msg = "";
const char* g_nav = nullptr;
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

float ScrollLimit() { return (float)std::max(0, ACTION_COUNT - VISIBLE_ROWS); }
void ClampScrollToSel() {
    if (g_sel < g_scroll) g_scroll = g_sel;
    if (g_sel > g_scroll + VISIBLE_ROWS - 1) g_scroll = g_sel - VISIBLE_ROWS + 1;
    g_scroll = std::clamp(g_scroll, 0, (int)ScrollLimit());
}

void Init() {
    if (g_logoTex < 0) g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() {
    g_sel = 0; g_prevSel = 0; g_scroll = 0;
    g_moveStart = -100.0; g_msgStart = -100.0; g_msg = ""; g_nav = nullptr;
}
void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = std::max(0, g_sel - 1);
        if (in.down) g_sel = std::min(ACTION_COUNT - 1, g_sel + 1);
        ClampScrollToSel();
        g_moveStart = Now();
    }
    if (in.accept) { g_msg = ACTIONS[g_sel].label; g_msgStart = Now(); }
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

// a neutral dark panel with a caption strip + rule.
void DrawPanel(float x, float y, float w, float h, float t, const char* caption) {
    DrawRect({ x, y }, { x + w, y + h }, WithAlpha(COL_PANEL, t));
    DrawRect({ x, y }, { x + w, y + HEADER_H }, WithAlpha(COL_PANEL_CAP, t));
    DrawRect({ x + 12, y + HEADER_H - 2 }, { x + w - 12, y + HEADER_H }, WithAlpha(COL_RULE, t));
    SetFont(g_fDF);
    DrawTextAligned({ x + 18, y }, { x + w - 14, y + HEADER_H }, 26.0f,
                    WithAlpha(COL_TITLE, t), caption, Align::Left, true, true);
    ResetFont();
}

void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 40 }, { 40 + w, 40 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(COL_WHITE, t));
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float listT  = (float)ComputeMotion(openSec, 0.0, LIST_FRAMES);
    const float infoT  = (float)ComputeMotion(openSec, INFO_OFFSET, INFO_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ===== HEADER: logo slot (left) + active-profile chip (right) =============
    DrawLogoSlot(titleT);
    {
        SetFont(g_fRodin);
        float cx = 1130.0f;
        DrawTextAligned({ cx - 220, 52 }, { cx, 78 }, 16.0f, WithAlpha(COL_CHIP, titleT),
                        "PROFILE", Align::Right, true, true);
        DrawTextAligned({ cx - 220, 74 }, { cx, 104 }, 24.0f, WithAlpha(COL_TITLE, titleT),
                        ACTIVE_PROFILE, Align::Right, true, true);
        ResetFont();
    }
    DrawRect({ LIST_X, RULE_Y }, { 1130.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));

    // ===== LEFT: ACTION LIST =================================================
    const float lx = LIST_X, ly = LIST_Y + (1.0f - listT) * 24.0f;
    DrawPanel(lx, ly, LIST_W, LIST_H, listT, "ACTIONS");
    const float rowsTop = ly + HEADER_H + 10.0f;
    const float rowL = lx + ROW_PAD, rowR = lx + LIST_W - ROW_PAD;

    if (listT > 0.5f) {
        float moveT = (float)ComputeMotion(g_moveStart, 0.0, SELECT_MOVE_FRAMES);
        float prevSlot = std::clamp((float)(g_prevSel - g_scroll), 0.0f, (float)(VISIBLE_ROWS - 1));
        float curSlot  = std::clamp((float)(g_sel     - g_scroll), 0.0f, (float)(VISIBLE_ROWS - 1));
        float slot = Lerp(prevSlot, curSlot, moveT);
        float hy = rowsTop + slot * ROW_H;
        DrawVGradient({ rowL, hy + 3 }, { rowR, hy + ROW_H - 5 },
                      WithAlpha(COL_SEL_TOP, listT), WithAlpha(COL_SEL_BOT, listT));
    }

    for (int row = 0; row < VISIBLE_ROWS; ++row) {
        int idx = g_scroll + row;
        if (idx >= ACTION_COUNT) break;
        float top = rowsTop + row * ROW_H;
        bool selected = (idx == g_sel);
        SetFont(g_fSeurat);
        DrawTextAligned({ rowL + 18.0f, top }, { rowR - 12.0f, top + ROW_H }, 28.0f,
                        WithAlpha(selected ? COL_TEXT_SEL : COL_TEXT, listT),
                        ACTIONS[idx].label, Align::Left, true, true);
        ResetFont();
    }

    // scrollbar (when the list overflows)
    if (ACTION_COUNT > VISIBLE_ROWS && listT > 0.5f) {
        float trackX = lx + LIST_W - 7.0f, trackTop = rowsTop, trackH = VISIBLE_ROWS * ROW_H;
        DrawRect({ trackX, trackTop }, { trackX + 3, trackTop + trackH }, WithAlpha(COL_RULE, listT));
        float thumbH = trackH * (float)VISIBLE_ROWS / (float)ACTION_COUNT;
        float denom = ScrollLimit(); if (denom < 1.0f) denom = 1.0f;
        float thumbY = trackTop + (trackH - thumbH) * ((float)g_scroll / denom);
        DrawRect({ trackX, thumbY }, { trackX + 3, thumbY + thumbH }, WithAlpha(COL_SEL_TOP, listT));
    }

    // ===== RIGHT: INFO (tracks the selection) ================================
    const float ix = INFO_X, iy = INFO_Y + (1.0f - infoT) * 24.0f;
    DrawPanel(ix, iy, INFO_W, INFO_H, infoT, "INFO");
    const Action& sel = ACTIONS[std::clamp(g_sel, 0, ACTION_COUNT - 1)];
    float textTop = iy + HEADER_H + 28.0f;
    DrawRect({ ix + 24.0f, textTop - 10.0f }, { ix + 60.0f, textTop - 6.0f }, WithAlpha(COL_SEL_TOP, infoT)); // accent
    SetFont(g_fSeurat);
    DrawTextAligned({ ix + 24.0f, textTop }, { ix + INFO_W - 18.0f, textTop + 40.0f }, 30.0f,
                    WithAlpha(COL_TEXT_SEL, infoT), sel.label, Align::Left, true, true);
    DrawText({ ix + 26.0f, textTop + 58.0f }, 20.0f, WithAlpha(COL_DESC, infoT), sel.info1);
    DrawText({ ix + 26.0f, textTop + 86.0f }, 20.0f, WithAlpha(COL_DESC, infoT), sel.info2);
    ResetFont();

    // ===== TRANSIENT FEEDBACK ===============================================
    double age = Now() - g_msgStart;
    if (g_msgStart > 0.0 && age < 1.6) {
        float ma = std::min(1.0f, (float)((1.6 - age) / 0.4));
        char line[80]; std::snprintf(line, sizeof(line), "Running %s on %s", g_msg, ACTIVE_PROFILE);
        SetFont(g_fSeurat);
        DrawTextAligned({ LIST_X, 572.0f }, { 1130.0f, 604.0f }, 24.0f,
                        WithAlpha(COL_OK, ma), line, Align::Center, true, true);
        ResetFont();
    }

    // ===== FOOTER ===========================================================
    {
        SetFont(g_fRodin);
        DrawRect({ LIST_X, 612.0f }, { 1130.0f, 614.0f }, WithAlpha(COL_RULE, footT));
        DrawText({ LIST_X, 628.0f }, 20.0f, WithAlpha(COL_FOOTER, footT), "Up/Down  Move");
        DrawText({ LIST_X + 230.0f, 628.0f }, 20.0f, WithAlpha(COL_FOOTER, footT), "Enter  Run");
        DrawText({ LIST_X + 420.0f, 628.0f }, 20.0f, WithAlpha(COL_FOOTER, footT), "Esc  Back");
        ResetFont();
    }
}

} // namespace

void TownInit() { Init(); }
void TownDraw(double openSeconds) { Draw(openSeconds); }
void TownInput(const ScreenInput& in) { Input(in); }
void TownReset() { Reset(); }
const char* TownNav() { return Nav(); }
