// =============================================================================
// screen_shop.cpp — the in-game Shop, re-authored as clean hand-written C++ in the
// UnleashedRecomp ui/options_menu idiom (NOT a CSD node dump). Layout lives in
// named constants in 1280x720 reference space; panels are bounded 9-slice-style
// gradient windows (no unclipped stretch-arms), text via the glyph atlas, and the
// screen is fully interactive + stateful exactly like options_menu:
//
//   * a scrollable item list with an eased selection highlight + scrollbar,
//   * a live ring balance that decreases when you buy,
//   * per-item affordability (price greys out when you can't afford it),
//   * a description panel that tracks the selection,
//   * a transient purchase/decline message,
//   * a footer button guide.
//
// Controls: Up/Down move the cursor, (A)=Enter/Z buys, (B)=Backspace/X backs out.
// Art is procedural (gradients + atlas text) to match the accepted pause/result
// look; swapping in the extracted ring/cursor/window atlases is the final polish.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

using namespace ui;

namespace {

// ---- catalogue --------------------------------------------------------------
struct Item { const char* name; int price; const char* desc1; const char* desc2; };
const Item ITEMS[] = {
    { "Chili Dog",      100,  "Sonic's favorite snack.",      "Restores a little energy."   },
    { "Hamburger",      120,  "A hearty classic burger.",     "Fills you right up."         },
    { "Hot Dog",        110,  "Grilled street-stand frank.",  "Quick and tasty."            },
    { "Special Spice",  400,  "A rare aromatic blend.",       "Chip can't get enough."      },
    { "Sun Medallion", 1000,  "A glittering sun token.",      "Said to hold solar power."   },
    { "Moon Medallion",1000,  "A pale lunar token.",          "Said to hold lunar power."   },
    { "Local Map",      250,  "Charts the surrounding land.", "Reveals hidden paths."       },
    { "Art Book",       800,  "Concept art collection.",      "For the gallery."            },
    { "Soundtrack CD",  800,  "The stage music, on disc.",    "For the sound test."         },
};
constexpr int ITEM_COUNT = int(sizeof(ITEMS) / sizeof(ITEMS[0]));

// ---- layout (reference px) --------------------------------------------------
constexpr float TITLE_X = 150.0f, TITLE_Y = 50.0f;
constexpr float RULE_Y  = 118.0f;

constexpr float LIST_X = 150.0f, LIST_Y = 150.0f, LIST_W = 620.0f, LIST_H = 410.0f;
constexpr float DESC_X = 800.0f, DESC_Y = 150.0f, DESC_W = 330.0f, DESC_H = 410.0f;
constexpr float HEADER_H = 52.0f;          // caption strip atop each window
constexpr float ROW_H    = 56.0f;
constexpr int   VISIBLE_ROWS = 6;          // 9 items -> the list scrolls
constexpr float ROW_PAD  = 12.0f;          // inset of rows inside the list window

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double LIST_FRAMES = 16.0, DESC_OFFSET = 4.0, DESC_FRAMES = 14.0;
constexpr double FOOT_OFFSET = 10.0, FOOT_FRAMES = 12.0, SELECT_MOVE_FRAMES = 8.0;

// ---- palette (shared with pause/result) -------------------------------------
const uint32_t COL_BG_TOP   = RGBA(12, 20, 38, 255);
const uint32_t COL_BG_BOT   = RGBA(5, 9, 18, 255);
const uint32_t COL_PANEL_TOP= RGBA(18, 30, 52, 232);
const uint32_t COL_PANEL_BOT= RGBA(8, 14, 26, 232);
const uint32_t COL_HEAD_TOP = RGBA(28, 52, 92, 244);
const uint32_t COL_HEAD_BOT = RGBA(16, 30, 56, 244);
const uint32_t COL_SEL_TOP  = RGBA(64, 150, 235, 225);
const uint32_t COL_SEL_BOT  = RGBA(28, 92, 180, 225);
const uint32_t COL_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t COL_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t COL_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t COL_DESC     = RGBA(178, 194, 214, 255);
const uint32_t COL_PRICE    = RGBA(255, 209, 74, 255);   // affordable
const uint32_t COL_PRICE_NO = RGBA(120, 120, 132, 255);  // can't afford
const uint32_t COL_RULE     = RGBA(120, 170, 230, 90);
const uint32_t COL_OK       = RGBA(120, 230, 140, 255);
const uint32_t COL_NO       = RGBA(235, 110, 110, 255);
const uint32_t COL_FOOTER   = RGBA(190, 205, 225, 220);

// ---- fonts (real game faces; loaded once, see screen_options.cpp) -----------
static int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// ---- interactive state ------------------------------------------------------
int    g_sel = 0, g_prevSel = 0, g_scroll = 0;
int    g_rings = 2500;
double g_moveStart = -100.0;   // Now() when the highlight last moved
double g_msgStart  = -100.0;   // Now() of the last buy/decline message
bool   g_msgOk = false;

// thousands-separated integer -> buf (e.g. 12345 -> "12,345")
void Commafy(int v, char* out, int n) {
    char raw[16]; std::snprintf(raw, sizeof(raw), "%d", v);
    int len = (int)std::strlen(raw), o = 0;
    for (int i = 0; i < len && o < n - 1; ++i) {
        if (i > 0 && (len - i) % 3 == 0 && o < n - 1) out[o++] = ',';
        out[o++] = raw[i];
    }
    out[o] = '\0';
}

float ScrollLimit() { return (float)std::max(0, ITEM_COUNT - VISIBLE_ROWS); }

void ClampScrollToSel() {
    if (g_sel < g_scroll) g_scroll = g_sel;
    if (g_sel > g_scroll + VISIBLE_ROWS - 1) g_scroll = g_sel - VISIBLE_ROWS + 1;
}

void Init() {
    GameFrameTex(); /* lazy-load the shared chrome frame */
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}

void Reset() {
    g_sel = 0; g_prevSel = 0; g_scroll = 0;
    g_rings = 2500; g_moveStart = -100.0; g_msgStart = -100.0; g_msgOk = false;
}

void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = std::max(0, g_sel - 1);
        if (in.down) g_sel = std::min(ITEM_COUNT - 1, g_sel + 1);
        ClampScrollToSel();
        g_moveStart = Now();
    }
    if (in.accept) {
        int price = ITEMS[g_sel].price;
        if (g_rings >= price) { g_rings -= price; g_msgOk = true; }
        else                  { g_msgOk = false; }
        g_msgStart = Now();
    }
    // cancel: would close the shop in-game; no-op in the standalone build.
}

// a bounded window from the REAL game frame (9-slice), tinted blue-silver, with a
// brighter header strip — the recomp's DrawPauseContainer technique on Unleashed art.
void DrawWindow(float x, float y, float w, float h, float t, const char* caption) {
    DrawGameWindow({ x, y }, { x + w, y + h }, HEADER_H, t);
    DrawRect({ x + 12, y + HEADER_H - 2 }, { x + w - 12, y + HEADER_H }, WithAlpha(COL_RULE, t));
    if (caption) {
        SetFont(g_fDF);
        DrawTextAligned({ x + 18, y }, { x + w - 14, y + HEADER_H }, 26.0f,
                        WithAlpha(COL_TITLE, t), caption, Align::Left, true, true);
    }
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const float titleT = (float)ComputeMotion(openSec, 0.0, 14.0);
    const float listT  = (float)ComputeMotion(openSec, 0.0, LIST_FRAMES);
    const float descT  = (float)ComputeMotion(openSec, DESC_OFFSET, DESC_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ---- title + ring balance ----
    SetFont(g_fDF);
    DrawTextBevel({ TITLE_X, TITLE_Y - (1.0f - titleT) * 16.0f }, 46.0f,
                  WithAlpha(COL_TITLE, titleT), "SHOP");
    DrawRect({ TITLE_X, RULE_Y }, { 1130.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));
    {
        char ringbuf[24], line[40];
        Commafy(g_rings, ringbuf, sizeof(ringbuf));
        std::snprintf(line, sizeof(line), "RINGS  %s", ringbuf);
        SetFont(g_fRodin);
        DrawTextAligned({ 740, TITLE_Y + 6 }, { 1130, TITLE_Y + 44 }, 30.0f,
                        WithAlpha(COL_PRICE, titleT), line, Align::Right, true, true);
    }

    // ---- item list window ----
    const float listSlide = (1.0f - listT) * 24.0f;
    const float lx = LIST_X, ly = LIST_Y + listSlide;
    DrawWindow(lx, ly, LIST_W, LIST_H, listT, "ITEMS");

    const float rowsTop = ly + HEADER_H + 10.0f;
    const float rowL = lx + ROW_PAD, rowR = lx + LIST_W - ROW_PAD;

    // selection highlight (eased between slots; clamped to the visible window)
    if (listT > 0.5f) {
        float moveT = (float)ComputeMotion(g_moveStart, 0.0, SELECT_MOVE_FRAMES);
        float prevSlot = std::clamp((float)(g_prevSel - g_scroll), 0.0f, (float)(VISIBLE_ROWS - 1));
        float curSlot  = std::clamp((float)(g_sel     - g_scroll), 0.0f, (float)(VISIBLE_ROWS - 1));
        float slot = Lerp(prevSlot, curSlot, moveT);
        float hy = rowsTop + slot * ROW_H;
        DrawVGradient({ rowL, hy + 3 }, { rowR, hy + ROW_H - 5 },
                      WithAlpha(COL_SEL_TOP, listT), WithAlpha(COL_SEL_BOT, listT));
    }

    // visible rows
    for (int row = 0; row < VISIBLE_ROWS; ++row) {
        int item = g_scroll + row;
        if (item >= ITEM_COUNT) break;
        float top = rowsTop + row * ROW_H;
        bool selected   = (item == g_sel);
        bool affordable = (g_rings >= ITEMS[item].price);

        SetFont(g_fSeurat);
        DrawTextAligned({ rowL + 16, top }, { rowL + 360, top + ROW_H }, 28.0f,
                        WithAlpha(selected ? COL_TEXT_SEL : COL_TEXT, listT),
                        ITEMS[item].name, Align::Left, true, true);
        char pbuf[16]; Commafy(ITEMS[item].price, pbuf, sizeof(pbuf));
        SetFont(g_fRodin);
        DrawTextAligned({ rowR - 200, top }, { rowR - 12, top + ROW_H }, 26.0f,
                        WithAlpha(affordable ? COL_PRICE : COL_PRICE_NO, listT),
                        pbuf, Align::Right, true, true);
    }

    // scrollbar (only when the list overflows)
    if (ITEM_COUNT > VISIBLE_ROWS && listT > 0.5f) {
        float trackX = lx + LIST_W - 7.0f, trackTop = rowsTop, trackH = VISIBLE_ROWS * ROW_H;
        DrawRect({ trackX, trackTop }, { trackX + 3, trackTop + trackH }, WithAlpha(COL_RULE, listT));
        float frac = (float)VISIBLE_ROWS / (float)ITEM_COUNT;
        float thumbH = trackH * frac;
        float thumbY = trackTop + (trackH - thumbH) * (g_scroll / ScrollLimit());
        DrawRect({ trackX, thumbY }, { trackX + 3, thumbY + thumbH }, WithAlpha(COL_SEL_TOP, listT));
    }

    // ---- description window (tracks the selection) ----
    const float dx = DESC_X, dy = DESC_Y + (1.0f - descT) * 24.0f;
    DrawWindow(dx, dy, DESC_W, DESC_H, descT, "INFO");
    const Item& it = ITEMS[g_sel];
    SetFont(g_fSeurat);
    DrawTextAligned({ dx + 18, dy + HEADER_H + 14 }, { dx + DESC_W - 14, dy + HEADER_H + 52 }, 30.0f,
                    WithAlpha(COL_TEXT_SEL, descT), it.name, Align::Left, true, true);
    DrawText({ dx + 18, dy + HEADER_H + 70 }, 21.0f, WithAlpha(COL_DESC, descT), it.desc1);
    DrawText({ dx + 18, dy + HEADER_H + 98 }, 21.0f, WithAlpha(COL_DESC, descT), it.desc2);
    {
        char pbuf[16], line[32];
        Commafy(it.price, pbuf, sizeof(pbuf));
        std::snprintf(line, sizeof(line), "PRICE  %s", pbuf);
        bool affordable = (g_rings >= it.price);
        SetFont(g_fRodin);
        DrawText({ dx + 18, dy + DESC_H - 100 }, 24.0f,
                 WithAlpha(affordable ? COL_PRICE : COL_PRICE_NO, descT), line);
        if (!affordable)
            DrawText({ dx + 18, dy + DESC_H - 66 }, 19.0f, WithAlpha(COL_NO, descT), "Not enough rings");
    }

    // ---- transient purchase/decline message ----
    double age = Now() - g_msgStart;
    if (g_msgStart > 0.0 && age < 1.5) {
        float ma = std::min(1.0f, (float)((1.5 - age) / 0.4));
        SetFont(g_fRodin);
        DrawTextAligned({ LIST_X, 568 }, { 770, 600 }, 24.0f,
                        WithAlpha(g_msgOk ? COL_OK : COL_NO, ma),
                        g_msgOk ? "Purchased!" : "Not enough rings!", Align::Center, true, true);
    }

    // ---- footer button guide ----
    SetFont(g_fRodin);
    DrawTextAligned({ TITLE_X, 612 }, { 1130, 656 }, 22.0f, WithAlpha(COL_FOOTER, footT),
                    "[Up/Down] Select     (A) Buy     (B) Back", Align::Left, true, true);

    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void ShopInit() { Init(); }
void ShopDraw(double openSeconds) { Draw(openSeconds); }
void ShopInput(const ScreenInput& in) { Input(in); }
void ShopReset() { Reset(); }
