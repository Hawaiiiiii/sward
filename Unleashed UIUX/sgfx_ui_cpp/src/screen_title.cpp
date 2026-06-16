// =============================================================================
// screen_title.cpp â€” the Sonic Unleashed Title / main-menu screen, re-authored as
// clean hand-written C++ in the UnleashedRecomp ui/options_menu idiom (NOT a CSD
// node dump â€” the old transcription stretched 16x16 black plates to 368x464 and a
// 96px crank to 616px, smearing the screen). Layout lives in named 1280x720
// constants; the iconic art is the REAL extracted SEGA atlas:
//
//   * ui_mm_base.png  â€” the full-screen retail title chrome (the blue gem / gear
//     assembly on the left, the silver lower bezel + green LED display, and the
//     transparent right-hand content well where the menu carousel lives),
//   * ui_mm_parts1.png â€” the genuine "MAIN MENU" title bar and the gold riveted
//     index-bar slot art that forms each menu row (a gem boss on the left, a gold
//     trough across), placed at sane on-screen rects.
//
// Fully interactive + stateful like options_menu / shop:
//   * Up/Down move the cursor over the menu rows with an eased highlight,
//   * (A) confirms the highlighted entry (transient "flash" feedback),
//   * (B) backs out (no-op in the standalone build),
//   * the green LED strip shows the current selection as the machine's readout,
//   * a staggered ease-in entrance: backdrop -> title bar -> rows cascade -> info.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

using namespace ui;

namespace {

// ---- menu entries -----------------------------------------------------------
struct Entry { const char* label; const char* blurb; };
const Entry ENTRIES[] = {
    { "NEW GAME", "Begin a new adventure."          },
    { "CONTINUE", "Resume from your last save."     },
    { "OPTIONS",  "Adjust display and sound."       },
    { "EXTRAS",   "Gallery, sound test and more."   },
    { "QUIT",     "Return to the console menu."     },
};
constexpr int ENTRY_COUNT = int(sizeof(ENTRIES) / sizeof(ENTRIES[0]));

// ---- real-art atlases -------------------------------------------------------
// ui_mm_base.png is the full 1280x720 retail title plate; ui_mm_parts1.png (1280x640)
// packs the title bar + the index-bar slot art used for every menu row.
const char* const ASSET_BASE = "assets/title/";
int g_baseTex = -1, g_partsTex = -1;

// ---- real-game fonts (MSDF sweep) --------------------------------------------
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

struct UV { float u0, v0, u1, v1; };
// "MAIN MENU" title bar â€” tight box measured from ui_mm_parts1 (974x79 px source).
const UV TITLEBAR_UV = { 0.00000f, 0.63594f, 0.76094f, 0.75938f };
// index-bar menu-slot art â€” the gold riveted bar with a gem boss on the left
// (766x81 px source). One copy is blitted per visible menu row.
const UV INDEXBAR_UV = { 0.00000f, 0.50781f, 0.59844f, 0.63438f };

constexpr float PARTS_W = 1280.0f, PARTS_H = 640.0f;   // ui_mm_parts1 atlas size
float TitleBarAspect() {
    return ((TITLEBAR_UV.u1 - TITLEBAR_UV.u0) * PARTS_W) /
           ((TITLEBAR_UV.v1 - TITLEBAR_UV.v0) * PARTS_H);
}

// ---- layout (reference px) --------------------------------------------------
// The right-hand content well of ui_mm_base is transparent from roughly x 460..1255,
// y 150..455; the carousel sits there. These match the retail index-bar rects
// (manifest x=464, y=104/184/264/344/424, w=766, h=81) nudged to fit the well.
constexpr float BAR_X      = 462.0f;     // left edge of each index bar
constexpr float BAR_W      = 762.0f;     // bar width on screen
constexpr float BAR_H      = 56.0f;      // bar height on screen
constexpr float ROW_TOP    = 156.0f;     // top of the first row
constexpr float ROW_PITCH  = 60.0f;      // vertical spacing (5 rows fit the content well, clear of the LED strip)
constexpr float LABEL_INSET= 104.0f;     // text starts past the left gem boss

constexpr float TITLEBAR_W = 660.0f;     // "MAIN MENU" bar width on screen
constexpr float TITLEBAR_X = BAR_X + (BAR_W - TITLEBAR_W) * 0.5f;
constexpr float TITLEBAR_Y = 96.0f;

// green LED readout strip baked into the lower bezel of ui_mm_base (y ~549..629)
constexpr float LED_X = 168.0f, LED_Y = 560.0f, LED_W = 988.0f, LED_H = 58.0f;

// ---- entrance tuning (frames @60fps), tuned to the shop/status feel ----------
constexpr double BASE_FRAMES   = 14.0;   // backdrop plate fades in first
constexpr double TITLE_OFFSET  = 4.0,  TITLE_FRAMES = 14.0;
constexpr double ROW_OFFSET    = 6.0,  ROW_FRAMES   = 16.0, ROW_STEP = 4.0; // CSD: index bars slide in from the right, staggered ~4f
constexpr double INFO_OFFSET   = 20.0, INFO_FRAMES  = 12.0;
constexpr double SELECT_MOVE_FRAMES = 40.0;   // CSD cursor carousel is a 40f eased move (was 8f, ~5x too fast)
constexpr float  ROW_SLIDE_PX  = 22.0f;  // rows slide in from +22px to the right

// ---- palette ----------------------------------------------------------------
const uint32_t COL_BG_TOP    = RGBA(10, 14, 24, 255);   // fallback fill if base missing
const uint32_t COL_BG_BOT    = RGBA(4, 6, 12, 255);
const uint32_t COL_WHITE      = RGBA(255, 255, 255, 255);
const uint32_t COL_LABEL      = RGBA(70, 52, 14, 255);   // dark text on the gold trough
const uint32_t COL_LABEL_SEL  = RGBA(40, 28, 6, 255);    // crisper on the lit row
const uint32_t COL_TITLE      = RGBA(255, 209, 74, 255); // gold (shared with all screens)
const uint32_t COL_SEL_GLOW   = RGBA(255, 236, 150, 255);// additive highlight on the lit bar
const uint32_t COL_LED        = RGBA(150, 240, 150, 255);// green nixie-display text
const uint32_t COL_LED_DIM    = RGBA(60, 150, 70, 255);
const uint32_t COL_FOOTER     = RGBA(70, 52, 14, 255);   // ASCII guide over the yellow footer
const uint32_t COL_FLASH      = RGBA(255, 255, 255, 255);

// fallback when the index-bar art is missing â€” a bounded gold gradient slot.
const uint32_t COL_SLOT_TOP   = RGBA(214, 170, 36, 235);
const uint32_t COL_SLOT_BOT   = RGBA(150, 112, 18, 235);
const uint32_t COL_SLOT_SELT  = RGBA(252, 226, 120, 245);
const uint32_t COL_SLOT_SELB  = RGBA(220, 168, 40, 245);

// ---- interactive state ------------------------------------------------------
int    g_sel = 0, g_prevSel = 0;
double g_moveStart  = -100.0;   // Now() when the highlight last moved
double g_flashStart = -100.0;   // Now() of the last confirm
int    g_flashRow   = -1;

void Init() {
    if (g_baseTex  < 0) g_baseTex  = gfx::loadTexture(std::string(ASSET_BASE) + "ui_mm_base.png");
    if (g_partsTex < 0) g_partsTex = gfx::loadTexture(std::string(ASSET_BASE) + "ui_mm_parts1.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}

void Reset() {
    g_sel = 0; g_prevSel = 0;
    g_moveStart = -100.0; g_flashStart = -100.0; g_flashRow = -1;
}

void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = (g_sel + ENTRY_COUNT - 1) % ENTRY_COUNT;   // wrap, carousel feel
        else         g_sel = (g_sel + 1) % ENTRY_COUNT;
        g_moveStart = Now();
    }
    if (in.accept) { g_flashStart = Now(); g_flashRow = g_sel; }   // confirm -> flash the row
    // cancel: would back out of the title in-game; no-op in the standalone build.
}

float RowTop(int row) { return ROW_TOP + row * ROW_PITCH; }

// one menu row: the real gold index-bar slot art (or a bounded gradient fallback),
// the entry label in the trough, and an additive glow + label brighten when lit.
void DrawRow(int row, float t, bool selected, float flashA) {
    float slide = (1.0f - t) * ROW_SLIDE_PX;
    float x = BAR_X + slide;
    float y = RowTop(row);
    float a = t;

    if (g_partsTex >= 0) {
        DrawImage(g_partsTex, { x, y }, { x + BAR_W, y + BAR_H },
                  { INDEXBAR_UV.u0, INDEXBAR_UV.v0 }, { INDEXBAR_UV.u1, INDEXBAR_UV.v1 },
                  WithAlpha(COL_WHITE, a));
        if (selected)   // additive sheen over the lit bar's trough
            DrawImage(g_partsTex, { x, y }, { x + BAR_W, y + BAR_H },
                      { INDEXBAR_UV.u0, INDEXBAR_UV.v0 }, { INDEXBAR_UV.u1, INDEXBAR_UV.v1 },
                      WithAlpha(COL_SEL_GLOW, a * 0.45f), /*additive*/ true);
    } else {
        // fallback bounded slot (never an unclipped stretch)
        if (selected)
            DrawVGradient({ x, y }, { x + BAR_W, y + BAR_H },
                          WithAlpha(COL_SLOT_SELT, a), WithAlpha(COL_SLOT_SELB, a));
        else
            DrawVGradient({ x, y }, { x + BAR_W, y + BAR_H },
                          WithAlpha(COL_SLOT_TOP, a), WithAlpha(COL_SLOT_BOT, a));
    }

    // confirm flash: a quick white wash that fades out
    if (flashA > 0.0f)
        DrawRect({ x, y }, { x + BAR_W, y + BAR_H }, WithAlpha(COL_FLASH, flashA * 0.5f), true);

    // label in the gold trough, past the left gem boss
    SetFont(g_fSeurat);
    DrawTextAligned({ x + LABEL_INSET, y }, { x + BAR_W - 24.0f, y + BAR_H }, 30.0f,
                    WithAlpha(selected ? COL_LABEL_SEL : COL_LABEL, a),
                    ENTRIES[row].label, Align::Left, true, true);
}

void Draw(double openSec) {
    const float baseT  = (float)ComputeMotion(openSec, 0.0, BASE_FRAMES);
    const float titleT = (float)ComputeMotion(openSec, TITLE_OFFSET, TITLE_FRAMES);
    const float infoT  = (float)ComputeMotion(openSec, INFO_OFFSET, INFO_FRAMES);

    // ---- backdrop: the real full-screen retail title plate --------------------
    if (g_baseTex >= 0)
        DrawImage(g_baseTex, { 0, 0 }, { REF_W, REF_H }, { 0, 0 }, { 1, 1 },
                  WithAlpha(COL_WHITE, baseT));
    else
        DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    // ---- "MAIN MENU" title bar (real art) -------------------------------------
    if (g_partsTex >= 0 && titleT > 0.0f) {
        float tw = TITLEBAR_W, th = tw / TitleBarAspect();
        // CSD: the MAIN MENU bar SLIDES IN from the left (with a small overshoot settle)
        float tx = Lerp(TITLEBAR_X - 340.0f, TITLEBAR_X, Cubic(0.0f, 1.0f, titleT)), ty = TITLEBAR_Y;
        DrawImage(g_partsTex, { tx, ty }, { tx + tw, ty + th },
                  { TITLEBAR_UV.u0, TITLEBAR_UV.v0 }, { TITLEBAR_UV.u1, TITLEBAR_UV.v1 },
                  WithAlpha(COL_WHITE, titleT));
    } else if (titleT > 0.0f) {
        SetFont(g_fDF);
        DrawTextShadow({ TITLEBAR_X, TITLEBAR_Y }, 44.0f, WithAlpha(COL_TITLE, titleT), "MAIN MENU");
    }

    // ---- eased selection highlight glow (tracks between rows) ------------------
    // We light the *art* per-row in DrawRow; the eased "slot" position drives which
    // row reads as selected mid-move, matching the shop's lerp-between-slots feel.
    float moveT   = (float)ComputeMotion(g_moveStart, 0.0, SELECT_MOVE_FRAMES);
    float litSlot = Lerp((float)g_prevSel, (float)g_sel, moveT);   // fractional during a move

    // confirm flash age -> per-row alpha
    double flashAge = Now() - g_flashStart;
    bool   flashing = (g_flashStart > 0.0 && flashAge < 0.45);
    float  flashA   = flashing ? (float)std::max(0.0, (0.45 - flashAge) / 0.45) : 0.0f;

    // ---- menu rows (cascade in top-to-bottom) ---------------------------------
    for (int row = 0; row < ENTRY_COUNT; ++row) {
        float rt = (float)ComputeMotion(openSec, ROW_OFFSET + row * ROW_STEP, ROW_FRAMES);
        if (rt <= 0.0f) continue;
        // a row reads as "selected" when the eased highlight is closest to it
        bool selected = (std::abs(litSlot - (float)row) < 0.5f);
        float rowFlash = (flashing && g_flashRow == row) ? flashA : 0.0f;
        // CSD: each index bar SLIDES IN FROM THE RIGHT into place (off-right -> rest)
        PushTransform(1.0f, 1.0f, { 0.0f, 0.0f }, { (1.0f - rt) * 330.0f, 0.0f });
        DrawRow(row, rt, selected, rowFlash);
        PopTransform();
    }

    // ---- green LED readout: the machine echoes the current selection ----------
    if (infoT > 0.0f) {
        char line[64];
        std::snprintf(line, sizeof(line), "> %s", ENTRIES[g_sel].label);
        SetFont(g_fRodin);
        DrawTextAligned({ LED_X, LED_Y }, { LED_X + LED_W, LED_Y + LED_H }, 26.0f,
                        WithAlpha(COL_LED, infoT), line, Align::Left, true, false);
        SetFont(g_fSeurat);
        DrawTextAligned({ LED_X, LED_Y }, { LED_X + LED_W - 12.0f, LED_Y + LED_H }, 20.0f,
                        WithAlpha(COL_LED_DIM, infoT), ENTRIES[g_sel].blurb, Align::Right, true, false);
    }

    // ---- footer button guide ---------------------------------------------------
    // ui_mm_base bakes the COMPLETE guide into the plate art (up/down-select at
    // x~175..330, (A)/(B) at x~915..1220). The retail screen shows exactly that,
    // so no ASCII echo is drawn — an English guide is a localized-plate swap
    // (the SGFX template path), not an overlay.

    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void TitleInit() { Init(); }
void TitleDraw(double openSeconds) { Draw(openSeconds); }
void TitleInput(const ScreenInput& in) { Input(in); }
void TitleReset() { Reset(); }
