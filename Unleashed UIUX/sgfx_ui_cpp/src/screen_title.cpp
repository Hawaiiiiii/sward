// =============================================================================
// screen_title.cpp — the launcher menu. A clean, dark, neutral screen built from
// primitives + text only (no third-party chrome art): a centred LOGO slot
// (host-supplied; empty = clean field, no header text), a vertical list of menu
// entries with an eased selection highlight, and a green readout strip that
// echoes the focused entry. It ships on its own.
//   * Up/Down move the cursor (wrapping) with an eased highlight,
//   * (A) confirms the focused entry (a brief flash),
//   * (B) backs out (no-op in the standalone build).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace ui;

namespace {

// ---- menu entries -----------------------------------------------------------
struct Entry { const char* label; const char* blurb; };
const Entry ENTRIES[] = {
    { "RUN",      "Start a new preflight run."      },
    { "RESUME",   "Pick up the last session."       },
    { "SETTINGS", "Adjust paths and preferences."   },
    { "EVIDENCE", "Review screenshots and reports." },
    { "QUIT",     "Close the tool."                 },
};
constexpr int ENTRY_COUNT = int(sizeof(ENTRIES) / sizeof(ENTRIES[0]));

int g_logoTex = -1;   // host-supplied launcher logo; -1 = empty slot (clean field)
int g_fSeurat = 0, g_fRodin = 0;

// ---- layout (reference px, 1280x720) ----------------------------------------
constexpr float LOGO_CX = 640.0f, LOGO_CY = 132.0f, LOGO_W = 420.0f, LOGO_H = 140.0f;
constexpr float ROW_W = 560.0f, ROW_H = 64.0f, ROW_PITCH = 74.0f;
constexpr float ROW_X = 640.0f - ROW_W * 0.5f, ROW_TOP = 246.0f, LABEL_INSET = 28.0f;
constexpr float RD_X = 150.0f, RD_Y = 628.0f, RD_W = 980.0f, RD_H = 54.0f;

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double LOGO_FRAMES = 14.0;
constexpr double ROW_OFFSET = 6.0, ROW_FRAMES = 16.0, ROW_STEP = 4.0;
constexpr double RD_OFFSET = 20.0, RD_FRAMES = 12.0;
constexpr double SELECT_MOVE_FRAMES = 14.0;
constexpr float  ROW_SLIDE_PX = 22.0f;

// ---- palette (dark, neutral; shared idiom with the town hub) ----------------
const uint32_t COL_BG_TOP   = RGBA(12, 18, 30, 255);
const uint32_t COL_BG_BOT   = RGBA(4, 7, 14, 255);
const uint32_t COL_ROW      = RGBA(18, 26, 40, 230);   // idle row plate
const uint32_t COL_ROW_EDGE = RGBA(60, 86, 120, 90);   // row top rule
const uint32_t COL_SEL_TOP  = RGBA(54, 130, 210, 235);
const uint32_t COL_SEL_BOT  = RGBA(26, 84, 162, 235);
const uint32_t COL_TEXT     = RGBA(196, 212, 230, 255);
const uint32_t COL_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t COL_RULE     = RGBA(90, 130, 180, 90);
const uint32_t COL_FLASH    = RGBA(255, 255, 255, 255);
const uint32_t COL_RD_BG    = RGBA(6, 12, 10, 230);    // readout backing
const uint32_t COL_LED      = RGBA(120, 220, 120, 255);
const uint32_t COL_LED_DIM  = RGBA(70, 150, 90, 255);
const uint32_t COL_WHITE    = RGBA(255, 255, 255, 255);

// ---- interactive state ------------------------------------------------------
int    g_sel = 0, g_prevSel = 0;
double g_moveStart = -100.0, g_flashStart = -100.0;
int    g_flashRow = -1;

void Init() {
    if (g_logoTex < 0) g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
}

void Reset() {
    g_sel = 0; g_prevSel = 0;
    g_moveStart = -100.0; g_flashStart = -100.0; g_flashRow = -1;
}

void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up) g_sel = (g_sel + ENTRY_COUNT - 1) % ENTRY_COUNT;
        else       g_sel = (g_sel + 1) % ENTRY_COUNT;
        g_moveStart = Now();
    }
    if (in.accept) { g_flashStart = Now(); g_flashRow = g_sel; }
    // cancel: would back out of the launcher in-flow; no-op in the standalone build.
}

float RowTop(int row) { return ROW_TOP + row * ROW_PITCH; }

// the centred logo slot. The host's own logo drops in here; with no asset the slot
// stays empty (clean field) — no placeholder, no header text.
void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    DrawImage(g_logoTex, { LOGO_CX - LOGO_W * 0.5f, LOGO_CY - LOGO_H * 0.5f },
              { LOGO_CX + LOGO_W * 0.5f, LOGO_CY + LOGO_H * 0.5f },
              { 0, 0 }, { 1, 1 }, WithAlpha(COL_WHITE, t));
}

// one menu row: a dark plate (blue gradient when lit), a top rule, and the label.
void DrawRow(int row, float t, bool selected, float flashA) {
    float x = ROW_X + (1.0f - t) * ROW_SLIDE_PX, y = RowTop(row);
    if (selected)
        DrawVGradient({ x, y }, { x + ROW_W, y + ROW_H }, WithAlpha(COL_SEL_TOP, t), WithAlpha(COL_SEL_BOT, t));
    else
        DrawRect({ x, y }, { x + ROW_W, y + ROW_H }, WithAlpha(COL_ROW, t));
    DrawRect({ x, y }, { x + ROW_W, y + 2 }, WithAlpha(COL_ROW_EDGE, t));   // top rule
    if (flashA > 0.0f)
        DrawRect({ x, y }, { x + ROW_W, y + ROW_H }, WithAlpha(COL_FLASH, flashA * 0.5f), true);
    SetFont(g_fSeurat);
    DrawTextAligned({ x + LABEL_INSET, y }, { x + ROW_W - 20.0f, y + ROW_H }, 30.0f,
                    WithAlpha(selected ? COL_TEXT_SEL : COL_TEXT, t),
                    ENTRIES[row].label, Align::Left, true, true);
    ResetFont();
}

void Draw(double openSec) {
    const float logoT = (float)ComputeMotion(openSec, 0.0, LOGO_FRAMES);
    const float rdT   = (float)ComputeMotion(openSec, RD_OFFSET, RD_FRAMES);

    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);
    DrawLogoSlot(logoT);

    // eased selection highlight position (fractional during a move)
    float moveT   = (float)ComputeMotion(g_moveStart, 0.0, SELECT_MOVE_FRAMES);
    float litSlot = Lerp((float)g_prevSel, (float)g_sel, moveT);
    double flashAge = Now() - g_flashStart;
    bool   flashing = (g_flashStart > 0.0 && flashAge < 0.45);
    float  flashA   = flashing ? (float)std::max(0.0, (0.45 - flashAge) / 0.45) : 0.0f;

    for (int row = 0; row < ENTRY_COUNT; ++row) {
        float rt = (float)ComputeMotion(openSec, ROW_OFFSET + row * ROW_STEP, ROW_FRAMES);
        if (rt <= 0.0f) continue;
        bool selected = (std::abs(litSlot - (float)row) < 0.5f);
        float rowFlash = (flashing && g_flashRow == row) ? flashA : 0.0f;
        DrawRow(row, rt, selected, rowFlash);
    }

    // bottom readout strip: echoes the focused entry + its one-line blurb
    if (rdT > 0.0f) {
        DrawRect({ RD_X, RD_Y }, { RD_X + RD_W, RD_Y + RD_H }, WithAlpha(COL_RD_BG, rdT));
        DrawRect({ RD_X, RD_Y }, { RD_X + RD_W, RD_Y + 2 }, WithAlpha(COL_RULE, rdT));
        char line[64];
        std::snprintf(line, sizeof(line), "> %s", ENTRIES[g_sel].label);
        SetFont(g_fRodin);
        DrawTextAligned({ RD_X + 16.0f, RD_Y }, { RD_X + RD_W * 0.5f, RD_Y + RD_H }, 24.0f,
                        WithAlpha(COL_LED, rdT), line, Align::Left, true, true);
        SetFont(g_fSeurat);
        DrawTextAligned({ RD_X + RD_W * 0.5f, RD_Y }, { RD_X + RD_W - 16.0f, RD_Y + RD_H }, 19.0f,
                        WithAlpha(COL_LED_DIM, rdT), ENTRIES[g_sel].blurb, Align::Right, true, true);
        ResetFont();
    }
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void TitleInit() { Init(); }
void TitleDraw(double openSeconds) { Draw(openSeconds); }
void TitleInput(const ScreenInput& in) { Input(in); }
void TitleReset() { Reset(); }
