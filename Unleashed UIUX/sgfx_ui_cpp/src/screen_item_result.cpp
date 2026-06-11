// =============================================================================
// screen_item_result.cpp Ã¢â‚¬â€ the Item-Get Results screen shown after a stage,
// re-authored as clean hand-written C++ in the UnleashedRecomp ui/options_menu
// idiom (NOT a CSD node dump Ã¢â‚¬â€ the raw item_result.json smears mat_result_comon_001
// 9-slice arms to w=857 and stretches 16px plates). Layout lives in named 1280x720
// constants; the list panel is a bounded window drawn from the REAL Sonic Unleashed
// silver-chrome frame via ui::DrawGameWindow (the same frame the shop/result use).
//
// The distinctive game art that makes this read as retail is the REAL extracted
// atlas, placed at deliberate sane rects with aspect preserved:
//   * the gold "RESULTS" wordmark            (mat_result_en_001, the big 3rd row),
//   * the Sun / Moon Medallion item icons    (mat_comon_006, a 2-cell strip),
//   * the metal "x N" count number plates    (mat_comon_num_001, per-digit blits),
//   * the Xbox (A) button glyph in the footer (mat_comon_x360_001).
//
// Fully interactive + stateful like options_menu / the shop:
//   * Up/Down move an eased selection highlight over the collected-item rows,
//   * (A) = NEXT advances past the results (transient "NEXT" flash feedback),
//   * (B) backs out (no-op in the standalone build).
//
// Real art is used wherever a distinctive element exists; a bounded chrome plate is
// the graceful fallback for items without a dedicated icon (the -1 atlas idiom),
// exactly as world_map / shop fall back when an atlas slot is missing.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

using namespace ui;

namespace {

// ---- per-element UV sub-rect (normalized atlas coords) ----------------------
struct UV { float u0, v0, u1, v1; };

// ---- real-art atlases -------------------------------------------------------
const char* const ASSET_BASE = "assets/item_result/";
int g_titleTex  = -1;   // mat_result_en_001    (512x512) Ã¢â‚¬â€ gold "RESULTS" wordmark
int g_iconTex   = -1;   // mat_comon_006        (256x128) Ã¢â‚¬â€ Sun / Moon medallion icons
int g_numTex    = -1;   // mat_comon_num_001    (512x64)  Ã¢â‚¬â€ metal digit plates
int g_glyphTex  = -1;   // mat_comon_x360_001   (512x512) Ã¢â‚¬â€ Xbox button glyphs

// ---- fonts (real game MSDF + DFSoGei SDF title font) ------------------------
static int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// atlas pixel sizes (for aspect-correct fitting)
constexpr float TITLE_TEX_W = 512.0f, TITLE_TEX_H = 512.0f;
constexpr float ICON_TEX_W  = 256.0f, ICON_TEX_H  = 128.0f;
constexpr float NUM_TEX_W   = 512.0f, NUM_TEX_H   = 64.0f;
constexpr float GLYPH_TEX_W = 512.0f, GLYPH_TEX_H = 512.0f;

// gold "RESULTS" wordmark Ã¢â‚¬â€ tight box measured from mat_result_en_001 (px 1,353..288,391)
const UV TITLE_UV = { 0.00195f, 0.68945f, 0.56250f, 0.76367f };

// Sun / Moon medallion icons Ã¢â‚¬â€ tight opaque boxes measured from mat_comon_006.
// (left cell = Sun Medallion, right cell = Moon Medallion; each ~119x120 px)
const UV ICON_SUN  = { 0.02344f, 0.02344f, 0.48828f, 0.96094f };   // px (6,3)-(125,123)
const UV ICON_MOON = { 0.51953f, 0.03125f, 0.98438f, 0.96875f };   // px (133,4)-(252,124)

// metal "(A)" button glyph Ã¢â‚¬â€ tight box from mat_comon_x360_001 (px 0,1..37,39)
const UV GLYPH_A = { 0.0f, 0.00195f, 0.07227f, 0.07617f };

// digit plates 0..9 from mat_comon_num_001 top row. Per-glyph x-spans measured from
// the atlas; all share ONE common cap-height y-band (px 1..29, v 0.01562..0.45312)
// so the digits sit on a single baseline (digit 9's small tail dips ~4px past this
// band and is gently clipped Ã¢â‚¬â€ invisible at the on-screen sizes used here).
constexpr float NUM_CELL_PXH = 28.0f;   // shared digit-cell pixel height (px 1..29)
const UV NUM_DIGIT[10] = {
    /* 0 */ { 0.00195f, 0.01562f, 0.05273f, 0.45312f },   // px  1..27
    /* 1 */ { 0.06641f, 0.01562f, 0.09961f, 0.45312f },   // px 34..51
    /* 2 */ { 0.11523f, 0.01562f, 0.16406f, 0.45312f },   // px 59..84
    /* 3 */ { 0.17188f, 0.01562f, 0.22070f, 0.45312f },   // px 88..113
    /* 4 */ { 0.22656f, 0.01562f, 0.27930f, 0.45312f },   // px 116..143
    /* 5 */ { 0.28320f, 0.01562f, 0.33398f, 0.45312f },   // px 145..171
    /* 6 */ { 0.33984f, 0.01562f, 0.39062f, 0.45312f },   // px 174..200
    /* 7 */ { 0.39648f, 0.01562f, 0.44531f, 0.45312f },   // px 203..228
    /* 8 */ { 0.45312f, 0.01562f, 0.50195f, 0.45312f },   // px 232..257
    /* 9 */ { 0.50781f, 0.01562f, 0.56445f, 0.45312f },   // px 260..289
};
// digit advance widths in atlas px (used to keep proportional spacing in the plate)
const float NUM_DIGIT_PXW[10] = { 26, 17, 25, 25, 27, 26, 26, 25, 25, 29 };

// ---- collected-item model ---------------------------------------------------
// Each row: a display name, a count, and an optional real icon (ICON_NONE => a
// bounded chrome plate fallback, like world_map's missing-atlas case). The Sun/Moon
// Medallions get their genuine extracted icons; the rest use the chrome fallback.
enum IconId { ICON_NONE = 0, ICON_ID_SUN, ICON_ID_MOON };

struct CollectedItem {
    const char* name;
    int         count;
    int         icon;     // IconId
};
const CollectedItem ITEMS[] = {
    { "SUN MEDALLION",   3, ICON_ID_SUN  },
    { "MOON MEDALLION",  2, ICON_ID_MOON },
    { "GAIA TEMPLE KEY", 1, ICON_NONE    },
    { "CHILI DOG",       5, ICON_NONE    },
    { "EXP GEM",        12, ICON_NONE    },
};
constexpr int ITEM_COUNT = int(sizeof(ITEMS) / sizeof(ITEMS[0]));

// ---- layout (reference px) --------------------------------------------------
constexpr float TITLE_X = 150.0f, TITLE_Y = 54.0f, RULE_Y = 130.0f;

constexpr float PANEL_X = 260.0f, PANEL_Y = 168.0f, PANEL_W = 760.0f, PANEL_H = 412.0f;
constexpr float HEADER_H = 52.0f;             // caption strip atop the window
constexpr float ROW_H    = 60.0f;
constexpr float ROW_PAD  = 18.0f;             // inset of rows inside the window
constexpr float ICON_BOX = 46.0f;             // square icon plate inside each row

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double TITLE_FRAMES = 14.0;
constexpr double PANEL_FRAMES = 16.0;
constexpr double ROW_OFFSET   = 8.0,  ROW_FRAMES = 13.0, ROW_STEP = 4.0;   // cascade
constexpr double TOTAL_OFFSET = 8.0 + ITEM_COUNT * ROW_STEP + 6.0, TOTAL_FRAMES = 14.0;
constexpr double FOOT_OFFSET  = TOTAL_OFFSET + 6.0, FOOT_FRAMES = 12.0;
constexpr double SELECT_MOVE_FRAMES = 8.0;

// ---- palette (shared with pause/result/shop/status/world_map) ---------------
const uint32_t COL_BG_TOP   = RGBA(12, 20, 38, 255);
const uint32_t COL_BG_BOT   = RGBA(5, 9, 18, 255);
const uint32_t COL_SEL_TOP  = RGBA(64, 150, 235, 225);
const uint32_t COL_SEL_BOT  = RGBA(28, 92, 180, 225);
const uint32_t COL_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t COL_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t COL_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t COL_COUNT    = RGBA(255, 224, 120, 255);   // fallback "x N" text colour
const uint32_t COL_RULE     = RGBA(120, 170, 230, 90);
const uint32_t COL_PLATE    = RGBA(8, 14, 26, 235);       // dark plate behind an icon
const uint32_t COL_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t COL_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t COL_OK       = RGBA(120, 230, 140, 255);

// gold tint for the chrome TOTAL bar (matches result's TOTAL row)
constexpr uint32_t TINT_GOLD = 0xFFA07828u;

// ---- interactive state ------------------------------------------------------
int    g_sel = 0, g_prevSel = 0;
double g_moveStart = -100.0;   // Now() when the highlight last moved
double g_nextStart = -100.0;   // Now() of the last (A) NEXT press

int TotalItems() { int s = 0; for (int i = 0; i < ITEM_COUNT; ++i) s += ITEMS[i].count; return s; }

void Init() {
    GameFrameTex();   // lazy-load the shared chrome frame (assets/common|shop|status)
    if (g_titleTex < 0) g_titleTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_result_en_001.png");
    if (g_iconTex  < 0) g_iconTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_006.png");
    if (g_numTex   < 0) g_numTex   = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_num_001.png");
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}

void Reset() {
    g_sel = 0; g_prevSel = 0; g_moveStart = -100.0; g_nextStart = -100.0;
}

void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = std::max(0, g_sel - 1);
        if (in.down) g_sel = std::min(ITEM_COUNT - 1, g_sel + 1);
        g_moveStart = Now();
    }
    if (in.accept) { g_nextStart = Now(); }   // (A) NEXT -> advance past results (flash)
    // cancel: would back out in-game; no-op in the standalone build.
}

// ---- icon -> UV lookup (ICON_NONE => fall back to a chrome plate) -----------
const UV* IconUV(int icon) {
    switch (icon) {
        case ICON_ID_SUN:  return &ICON_SUN;
        case ICON_ID_MOON: return &ICON_MOON;
        default:           return nullptr;
    }
}

// ---- aspect-preserving image fit into a box (world_map idiom) ---------------
void DrawFitted(int tex, const UV& uv, float texW, float texH,
                float bx, float by, float bw, float bh, float t) {
    if (tex < 0 || t <= 0.0f) return;
    float artW = (uv.u1 - uv.u0) * texW;
    float artH = (uv.v1 - uv.v0) * texH;
    if (artW <= 0.0f || artH <= 0.0f) return;
    float scale = std::min(bw / artW, bh / artH);
    float w = artW * scale, h = artH * scale;
    float x = bx + (bw - w) * 0.5f;
    float y = by + (bh - h) * 0.5f;
    DrawImage(tex, { x, y }, { x + w, y + h }, { uv.u0, uv.v0 }, { uv.u1, uv.v1 },
              WithAlpha(COL_WHITE, t));
}

// ---- a metal "x N" count plate, blitted digit-by-digit from mat_comon_num_001 -
// Right-anchored at (rightX, cy). Draws an ASCII "x" then the real number plates.
// Returns left edge x of the drawn group. Falls back to ASCII text if no atlas.
float DrawCountPlate(float rightX, float cy, int count, float glyphH, float t) {
    char digs[16];
    std::snprintf(digs, sizeof(digs), "%d", count);
    int n = (int)std::strlen(digs);

    SetFont(g_fRodin);   // values use Rodin (measure + draw with the same font)

    if (g_numTex < 0) {
        // graceful fallback: plain ASCII "x N"
        char line[20]; std::snprintf(line, sizeof(line), "x %d", count);
        float w = MeasureText(glyphH, line).x;
        DrawTextAligned({ rightX - w, cy - glyphH * 0.5f }, { rightX, cy + glyphH * 0.5f },
                        glyphH, WithAlpha(COL_COUNT, t), line, Align::Right, true, true);
        return rightX - w;
    }

    const float gap = 2.0f;
    // A glyph drawn glyphH px tall (matching the shared cell height) has on-screen
    // width = glyphH * (pxWidth / NUM_CELL_PXH). First measure the whole group's
    // width so we can right-anchor it.
    float totalW = 0.0f;
    for (int i = 0; i < n; ++i) {
        int d = digs[i] - '0';
        float dw = glyphH * (NUM_DIGIT_PXW[d] / NUM_CELL_PXH);
        totalW += dw + gap;
    }
    totalW -= gap;

    // ASCII "x " prefix sits to the left of the number group
    const char* xs = "x ";
    float xw = MeasureText(glyphH * 0.82f, xs).x;

    float x = rightX - totalW;
    // draw the digits left-to-right
    for (int i = 0; i < n; ++i) {
        int d = digs[i] - '0';
        const UV& u = NUM_DIGIT[d];
        float dw = glyphH * (NUM_DIGIT_PXW[d] / NUM_CELL_PXH);
        DrawImage(g_numTex, { x, cy - glyphH * 0.5f }, { x + dw, cy + glyphH * 0.5f },
                  { u.u0, u.v0 }, { u.u1, u.v1 }, WithAlpha(COL_WHITE, t));
        x += dw + gap;
    }
    // "x " prefix
    DrawText({ rightX - totalW - xw, cy - glyphH * 0.42f }, glyphH * 0.82f,
             WithAlpha(COL_COUNT, t), xs);
    return rightX - totalW - xw;
}

// ---- a bounded window from the REAL game frame (9-slice) with a header strip --
void DrawWindow(float x, float y, float w, float h, float t, const char* caption) {
    DrawGameWindow({ x, y }, { x + w, y + h }, HEADER_H, t);
    DrawRect({ x + 12, y + HEADER_H - 2 }, { x + w - 12, y + HEADER_H }, WithAlpha(COL_RULE, t));
    if (caption) {
        SetFont(g_fDF);   // DFSoGei for the window header caption
        DrawTextAligned({ x + 18, y }, { x + w - 14, y + HEADER_H }, 26.0f,
                        WithAlpha(COL_TITLE, t), caption, Align::Left, true, true);
    }
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float panelT = (float)ComputeMotion(openSec, 0.0, PANEL_FRAMES);
    const float totalT = (float)ComputeMotion(openSec, TOTAL_OFFSET, TOTAL_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ===== TITLE: real gold "RESULTS" wordmark (or text fallback) =============
    if (g_titleTex >= 0 && titleT > 0.0f) {
        float aspect = ((TITLE_UV.u1 - TITLE_UV.u0) * TITLE_TEX_W) /
                       ((TITLE_UV.v1 - TITLE_UV.v0) * TITLE_TEX_H);
        float th = 52.0f, tw = th * aspect;
        float tx = TITLE_X;
        float ty = TITLE_Y - (1.0f - titleT) * 14.0f;
        DrawImage(g_titleTex, { tx, ty }, { tx + tw, ty + th },
                  { TITLE_UV.u0, TITLE_UV.v0 }, { TITLE_UV.u1, TITLE_UV.v1 },
                  WithAlpha(COL_WHITE, titleT));
    } else {
        SetFont(g_fDF);   // DFSoGei is the authentic title font
        DrawTextShadow({ TITLE_X, TITLE_Y - (1.0f - titleT) * 16.0f }, 46.0f,
                       WithAlpha(COL_TITLE, titleT), "RESULTS");
    }
    DrawRect({ TITLE_X, RULE_Y }, { 1130.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));

    // "ITEMS GET" caption to the right of the title
    SetFont(g_fDF);
    DrawTextAligned({ 740, TITLE_Y + 6 }, { 1130, TITLE_Y + 50 }, 30.0f,
                    WithAlpha(COL_TITLE, titleT), "ITEMS GET", Align::Right, true, true);

    // ===== ITEM LIST WINDOW ===================================================
    const float px = PANEL_X, py = PANEL_Y + (1.0f - panelT) * 24.0f;
    DrawWindow(px, py, PANEL_W, PANEL_H, panelT, "ITEMS COLLECTED");

    const float rowsTop = py + HEADER_H + 12.0f;
    const float rowL = px + ROW_PAD, rowR = px + PANEL_W - ROW_PAD;

    // eased selection highlight (shop/status idiom)
    if (panelT > 0.5f) {
        float moveT = (float)ComputeMotion(g_moveStart, 0.0, SELECT_MOVE_FRAMES);
        float slot  = Lerp((float)g_prevSel, (float)g_sel, moveT);
        float hy = rowsTop + slot * ROW_H;
        DrawVGradient({ rowL, hy + 3 }, { rowR, hy + ROW_H - 5 },
                      WithAlpha(COL_SEL_TOP, panelT), WithAlpha(COL_SEL_BOT, panelT));
    }

    // item rows: icon plate + real icon (or chrome fallback) + name + "x N" plate
    for (int i = 0; i < ITEM_COUNT; ++i) {
        float rt = (float)ComputeMotion(openSec, ROW_OFFSET + i * ROW_STEP, ROW_FRAMES);
        if (rt <= 0.0f) continue;
        float slide = (1.0f - rt) * 28.0f;          // ease in from +28px right
        float top = rowsTop + i * ROW_H;
        bool selected = (i == g_sel);
        const CollectedItem& it = ITEMS[i];

        // icon plate (dark inset) Ã¢â‚¬â€ always a bounded plate, never an unclipped quad
        float ix = rowL + 12.0f + slide;
        float iy = top + (ROW_H - ICON_BOX) * 0.5f;
        DrawRect({ ix, iy }, { ix + ICON_BOX, iy + ICON_BOX }, WithAlpha(COL_PLATE, rt));
        const UV* iconUV = IconUV(it.icon);
        if (iconUV && g_iconTex >= 0) {
            DrawFitted(g_iconTex, *iconUV, ICON_TEX_W, ICON_TEX_H,
                       ix + 2, iy + 2, ICON_BOX - 4, ICON_BOX - 4, rt);
        } else {
            // chrome plate fallback for items without a dedicated icon
            DrawGameWindow({ ix + 3, iy + 3 }, { ix + ICON_BOX - 3, iy + ICON_BOX - 3 }, 0.0f, rt);
        }
        // thin frame around the icon plate
        DrawRect({ ix, iy }, { ix + ICON_BOX, iy + 2 }, WithAlpha(COL_RULE, rt));
        DrawRect({ ix, iy + ICON_BOX - 2 }, { ix + ICON_BOX, iy + ICON_BOX }, WithAlpha(COL_RULE, rt));

        // item name (left, past the icon)
        SetFont(g_fSeurat);   // labels use Seurat (re-set each row; count plate switches to Rodin)
        DrawTextAligned({ ix + ICON_BOX + 18.0f, top }, { rowR - 160.0f, top + ROW_H }, 28.0f,
                        WithAlpha(selected ? COL_TEXT_SEL : COL_TEXT, rt),
                        it.name, Align::Left, true, true);

        // "x N" count plate (right-anchored, real metal digits)
        DrawCountPlate(rowR - 14.0f - slide, top + ROW_H * 0.5f, it.count, 30.0f, rt);
    }

    // ===== TOTAL bar (gold chrome, below the rows) ============================
    if (totalT > 0.0f) {
        float slide = (1.0f - totalT) * 28.0f;
        float ty = py + PANEL_H + 14.0f;
        float bx = PANEL_X + slide, bw = PANEL_W;
        DrawGameWindow({ bx, ty }, { bx + bw, ty + ROW_H - 6 }, 0.0f, totalT, TINT_GOLD);
        SetFont(g_fSeurat);   // label on the gold TOTAL bar
        DrawTextAligned({ bx + 22, ty }, { bx + 360, ty + ROW_H - 6 }, 30.0f,
                        WithAlpha(COL_TEXT_SEL, totalT), "TOTAL ITEMS", Align::Left, true, true);
        DrawCountPlate(bx + bw - 18.0f, ty + (ROW_H - 6) * 0.5f, TotalItems(), 34.0f, totalT);
    }

    // ===== FOOTER: real (A) glyph + NEXT ======================================
    {
        float cy = 674.0f;   // clear of the TOTAL bar (594..648)
        float gh = 34.0f;
        float fx = 760.0f;
        SetFont(g_fRodin);   // footer button-guide text uses Rodin
        if (g_glyphTex >= 0 && footT > 0.0f) {
            float gAsp = ((GLYPH_A.u1 - GLYPH_A.u0) * GLYPH_TEX_W) /
                         ((GLYPH_A.v1 - GLYPH_A.v0) * GLYPH_TEX_H);
            float gw = gh * gAsp;
            DrawImage(g_glyphTex, { fx, cy - gh * 0.5f }, { fx + gw, cy + gh * 0.5f },
                      { GLYPH_A.u0, GLYPH_A.v0 }, { GLYPH_A.u1, GLYPH_A.v1 },
                      WithAlpha(COL_WHITE, footT));
            fx += gw + 12.0f;
        } else if (footT > 0.0f) {
            DrawText({ fx, cy - 15.0f }, 26.0f, WithAlpha(COL_FOOTER, footT), "(A)");
            fx += MeasureText(26.0f, "(A)").x + 12.0f;
        }
        DrawTextAligned({ fx, cy - 18.0f }, { 1130.0f, cy + 18.0f }, 26.0f,
                        WithAlpha(COL_FOOTER, footT), "NEXT", Align::Left, true, true);
        // secondary hint
        DrawTextAligned({ PANEL_X, cy - 14.0f }, { 700.0f, cy + 14.0f }, 22.0f,
                        WithAlpha(COL_FOOTER, footT), "[Up/Down] Select", Align::Left, true, true);
    }

    // ===== transient "NEXT" flash on (A) ======================================
    double age = Now() - g_nextStart;
    if (g_nextStart > 0.0 && age < 1.2) {
        float ma = std::min(1.0f, (float)((1.2 - age) / 0.4));
        SetFont(g_fRodin);
        DrawTextAligned({ PANEL_X, 588 }, { PANEL_X + PANEL_W, 618 }, 26.0f,
                        WithAlpha(COL_OK, ma), "NEXT", Align::Center, true, true);
    }

    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void ItemResultInit() { Init(); }
void ItemResultDraw(double openSeconds) { Draw(openSeconds); }
void ItemResultInput(const ScreenInput& in) { Input(in); }
void ItemResultReset() { Reset(); }
