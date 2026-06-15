// =============================================================================
// screen_town.cpp â€” the Town hub menu, re-authored as clean hand-written C++ in
// the UnleashedRecomp ui/options_menu idiom (NOT a CSD node dump â€” the raw town
// nodelist is three 3095px-wide stretch-arm banners that smear across the screen,
// deliberately ignored). Layout lives in named 1280x720 constants; the action
// list and info panels are bounded windows drawn from the REAL Sonic Unleashed
// silver-chrome frame (ui::DrawGameWindow), exactly like the shop / status / world
// map. The distinctive game art that makes this read as retail is the REAL
// extracted town atlases:
//   * the town action icons (sun / moon / camera / ring) â€” mat_townscreen_001,
//   * the real "Pass Time" / "Take Photo" / "Talk / Use" label art â€” mat_comon_en_002,
//   * the rotating day/night sun & moon medallions   â€” mat_comon_003 / mat_comon_004,
//   * the little "LV" tag                              â€” mat_townscreen_en_001,
//   * the Xbox A / B button glyphs                     â€” mat_comon_x360_001,
// each placed at deliberate sane rects with aspect preserved (fit-box like status /
// world_map). Bounded chrome windows are the graceful fallback when an atlas is
// missing (-1 slot), exactly as the shop's DrawWindow does.
//
// Fully interactive + stateful like options_menu / the shop:
//   * Up/Down move the cursor over the action rows with an eased highlight,
//   * (A) performs the selected action â€” "PASS TIME" toggles the day/night cycle
//         (the header sun<->moon medallion + the time label swap live); the others
//         flash a transient confirmation,
//   * (B) backs out (no-op in the standalone build).
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
const char* const ASSET_BASE = "assets/town/";
int g_iconTex  = -1;   // mat_townscreen_001    (128x128: sun/moon/camera/ring icons)
int g_labelTex = -1;   // mat_comon_en_002      (256x256: town action label art)
int g_sunTex   = -1;   // mat_comon_003         (1024x128: rotating sun medallion)
int g_moonTex  = -1;   // mat_comon_004         (1024x128: rotating moon medallion)
int g_lvTex    = -1;   // mat_townscreen_en_001 (32x16: "LV" tag)
int g_glyphTex = -1;   // mat_comon_x360_001    (512x512: Xbox button glyphs)

// ---- atlas pixel sizes (for aspect-correct fitting) -------------------------
constexpr float ICON_TEX_W  = 128.0f,  ICON_TEX_H  = 128.0f;
constexpr float LABEL_TEX_W = 256.0f,  LABEL_TEX_H = 256.0f;
constexpr float COIN_TEX_W  = 1024.0f, COIN_TEX_H  = 128.0f;
constexpr float LV_TEX_W    = 32.0f,   LV_TEX_H    = 16.0f;
constexpr float GLYPH_TEX_W = 512.0f,  GLYPH_TEX_H = 512.0f;

// tight opaque boxes measured from mat_townscreen_001's alpha channel ----------
const UV ICON_SUN    = { 0.4922f, 0.0547f, 0.6406f, 0.4453f };  // top row, 3rd icon
const UV ICON_MOON   = { 0.6406f, 0.0547f, 0.9766f, 0.4453f };  // top row, 4th icon
const UV ICON_CAMERA = { 0.6484f, 0.4453f, 0.9766f, 0.8125f };  // bottom row, right
const UV ICON_RING   = { 0.0312f, 0.0234f, 0.3203f, 0.4453f };  // top row, ring/circle
const UV ICON_TALK   = { 0.0469f, 0.4453f, 0.3047f, 0.7969f };  // bottom row, white oval (speech)

// real label art rows in mat_comon_en_002 (measured) --------------------------
const UV LBL_PASS  = { 0.0156f, 0.0273f, 0.5078f, 0.1133f };  // "Pass Time"
const UV LBL_PHOTO = { 0.0078f, 0.1445f, 0.5664f, 0.2305f };  // "Take Photo"
const UV LBL_TALK  = { 0.0078f, 0.2617f, 0.4844f, 0.3477f };  // "Talk / Use"

// front-facing (largest) frame of each rotating medallion sheet ----------------
const UV COIN_SUN  = { 0.0010f, 0.0078f, 0.0576f, 0.9297f };  // mat_comon_003 frame0
const UV COIN_MOON = { 0.0010f, 0.0156f, 0.0566f, 0.9219f };  // mat_comon_004 frame0

// "LV" tag (the opaque right half of the 32x16 atlas) -------------------------
const UV LV_UV = { 0.5000f, 0.1875f, 1.0000f, 1.0000f };

// Xbox button glyphs (top row of mat_comon_x360_001) â€” verbatim from world_map --
const UV GLYPH_A = { 0.0000f, 0.0020f, 0.0684f, 0.0762f };
const UV GLYPH_B = { 0.0801f, 0.0020f, 0.1484f, 0.0762f };

// ---- action model -----------------------------------------------------------
// Each row is a town action: a real icon, optional real label art (with an ASCII
// fallback), and an info blurb. PASS_TIME toggles the day/night cycle.
enum ActionKind { ACT_PASS, ACT_PHOTO, ACT_TALK, ACT_SHOP, ACT_RECORDS, ACT_DEPART };

struct Action {
    ActionKind  kind;
    const UV*   icon;     // mat_townscreen_001 sub-rect
    const UV*   label;    // mat_comon_en_002 sub-rect, or null -> ASCII only
    const char* ascii;    // ASCII label (fallback + footer/info use)
    const char* info1;
    const char* info2;
};
const Action ACTIONS[] = {
    { ACT_PASS,    &ICON_SUN,    &LBL_PASS,  "PASS TIME",
      "Wait for the sun to rise or set.", "Switch between day and night."   },
    { ACT_PHOTO,   &ICON_CAMERA, &LBL_PHOTO, "TAKE PHOTO",
      "Snap a picture of the town.",      "Save it to your photo album."    },
    { ACT_TALK,    &ICON_TALK,   &LBL_TALK,  "TALK / USE",
      "Speak with the townsfolk.",        "Hear rumors and gather hints."   },
    { ACT_SHOP,    &ICON_RING,   nullptr,    "VISIT SHOP",
      "Browse the local goods.",          "Spend your hard-earned rings."   },
    { ACT_RECORDS, &ICON_MOON,   nullptr,    "RECORDS",
      "Review your stage records.",       "Check ranks, times and medals."  },
    { ACT_DEPART,  &ICON_RING,   nullptr,    "DEPART",
      "Leave town for the world map.",    "Continue your adventure."        },
};
constexpr int ACTION_COUNT = int(sizeof(ACTIONS) / sizeof(ACTIONS[0]));

// ---- layout (reference px) --------------------------------------------------
constexpr float TITLE_X = 150.0f, TITLE_Y = 50.0f, RULE_Y = 118.0f;

constexpr float LIST_X = 150.0f, LIST_Y = 158.0f, LIST_W = 600.0f, LIST_H = 404.0f;
constexpr float INFO_X = 780.0f, INFO_Y = 158.0f, INFO_W = 350.0f, INFO_H = 404.0f;
constexpr float HEADER_H = 52.0f;                  // caption strip atop each window
constexpr float ROW_H    = 60.0f;
constexpr int   VISIBLE_ROWS = 5;                  // 6 actions -> the list scrolls
constexpr float ROW_PAD  = 14.0f;

// day/night medallion indicator (top-right, beside the title)
constexpr float COIN_BX = 980.0f, COIN_BY = TITLE_Y - 4.0f, COIN_BW = 64.0f, COIN_BH = 70.0f;

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double TITLE_FRAMES = 14.0;
constexpr double COIN_OFFSET  = 4.0,  COIN_FRAMES = 14.0;
constexpr double LIST_FRAMES  = 16.0;
constexpr double INFO_OFFSET  = 5.0,  INFO_FRAMES = 14.0;
constexpr double FOOT_OFFSET  = 10.0, FOOT_FRAMES = 12.0;
constexpr double SELECT_MOVE_FRAMES = 8.0;

// ---- palette (shared with pause / result / shop / status / world_map) -------
const uint32_t COL_BG_TOP   = RGBA(12, 20, 38, 255);
const uint32_t COL_BG_BOT   = RGBA(5, 9, 18, 255);
const uint32_t COL_SEL_TOP  = RGBA(64, 150, 235, 225);
const uint32_t COL_SEL_BOT  = RGBA(28, 92, 180, 225);
const uint32_t COL_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t COL_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t COL_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t COL_DESC     = RGBA(178, 194, 214, 255);
const uint32_t COL_RULE     = RGBA(120, 170, 230, 90);
const uint32_t COL_PLATE    = RGBA(6, 10, 20, 235);   // dark plate behind the big icon
const uint32_t COL_OK       = RGBA(120, 230, 140, 255);
const uint32_t COL_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t COL_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t COL_SUN      = RGBA(255, 196, 96, 255);    // "DAY" tint
const uint32_t COL_MOON     = RGBA(150, 196, 255, 255);   // "NIGHT" tint

// ---- interactive state ------------------------------------------------------
int    g_sel = 0, g_prevSel = 0, g_scroll = 0;
bool   g_night = false;             // false=day (sun), true=night (moon)
double g_moveStart = -100.0;        // Now() when the highlight last moved
double g_msgStart  = -100.0;        // Now() of the last action feedback
const char* g_msg = "";

float ScrollLimit() { return (float)std::max(0, ACTION_COUNT - VISIBLE_ROWS); }

void ClampScrollToSel() {
    if (g_sel < g_scroll) g_scroll = g_sel;
    if (g_sel > g_scroll + VISIBLE_ROWS - 1) g_scroll = g_sel - VISIBLE_ROWS + 1;
    g_scroll = std::clamp(g_scroll, 0, (int)ScrollLimit());
}

// The recomp's actual fonts (same registry as options): FOT-SeuratPro-M (labels +
// blurbs), FOT-NewRodinPro-DB (values + footer), DFHeiStd-W7 (~DFSoGeiStd, the
// title + window captions).
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

void Init() {
    GameFrameTex();   // lazy-load the shared chrome frame
    if (g_iconTex  < 0) g_iconTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_townscreen_001.png");
    if (g_labelTex < 0) g_labelTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_en_002.png");
    if (g_sunTex   < 0) g_sunTex   = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_003.png");
    if (g_moonTex  < 0) g_moonTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_004.png");
    if (g_lvTex    < 0) g_lvTex    = gfx::loadTexture(std::string(ASSET_BASE) + "mat_townscreen_en_001.png");
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF (im_font_atlas)
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");    // real game MSDF
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");   // real DFSoGeiStd-W7 (title + captions)
}

void Reset() {
    g_sel = 0; g_prevSel = 0; g_scroll = 0;
    g_night = false;
    g_moveStart = -100.0; g_msgStart = -100.0; g_msg = "";
}

const char* g_nav = nullptr;
void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = std::max(0, g_sel - 1);
        if (in.down) g_sel = std::min(ACTION_COUNT - 1, g_sel + 1);
        ClampScrollToSel();
        g_moveStart = Now();
    }
    if (in.accept) {
        const Action& a = ACTIONS[g_sel];
        if (a.kind == ACT_PASS) {
            g_night = !g_night;
            g_msg = g_night ? "Night falls over the town." : "The sun rises over the town.";
        } else if (a.kind == ACT_SHOP)   { g_nav = "shop"; }              // the runtime flow
        else if (a.kind == ACT_TALK)     { g_nav = "balloon"; }
        else if (a.kind == ACT_DEPART)   { g_nav = "loading>world_map"; }
        else {
            g_msg = a.info1;
        }
        g_msgStart = Now();
    }
    // cancel: closes the town menu back to free-roam in-game; no-op here.
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

// ---- aspect-preserving image fit into a box (status / world_map idiom) ------
// Draws the atlas sub-rect (uv) centred inside [bx,by,bx+bw,by+bh], scaled to fit,
// with optional vertical anchor (0=top,0.5=center,1=bottom). `slide` shifts it for
// the entrance. Returns silently if the texture is missing.
void DrawFitted(int tex, const UV& uv, float texW, float texH,
                float bx, float by, float bw, float bh,
                float t, float vAnchor = 0.5f, float slide = 0.0f, uint32_t col = COL_WHITE) {
    if (tex < 0 || t <= 0.0f) return;
    float artW = (uv.u1 - uv.u0) * texW;
    float artH = (uv.v1 - uv.v0) * texH;
    if (artW <= 0.0f || artH <= 0.0f) return;
    float scale = std::min(bw / artW, bh / artH);
    float w = artW * scale, h = artH * scale;
    float x = bx + (bw - w) * 0.5f;
    float y = by + (bh - h) * vAnchor + slide;
    DrawImage(tex, { x, y }, { x + w, y + h }, { uv.u0, uv.v0 }, { uv.u1, uv.v1 },
              WithAlpha(col, t));
}

// a bounded window from the REAL game frame (9-slice) with a header caption strip
// â€” the recomp's DrawPauseContainer technique on Unleashed art (copied from shop).
void DrawWindow(float x, float y, float w, float h, float t, const char* caption) {
    DrawGameWindow({ x, y }, { x + w, y + h }, HEADER_H, t);
    DrawRect({ x + 12, y + HEADER_H - 2 }, { x + w - 12, y + HEADER_H }, WithAlpha(COL_RULE, t));
    if (caption) {
        SetFont(g_fDF);
        DrawTextAligned({ x + 18, y }, { x + w - 14, y + HEADER_H }, 26.0f,
                        WithAlpha(COL_TITLE, t), caption, Align::Left, true, true);
    }
}

// one footer hint: real button glyph + ASCII label. Returns the next x.
float DrawHint(float x, float cy, const UV& g, float gAspect, const char* label, float t) {
    const float gh = 30.0f;
    if (g_glyphTex >= 0) {
        float gw = gh * gAspect;
        DrawImage(g_glyphTex, { x, cy - gh * 0.5f }, { x + gw, cy + gh * 0.5f },
                  { g.u0, g.v0 }, { g.u1, g.v1 }, WithAlpha(COL_WHITE, t));
        x += gw + 8.0f;
    }
    DrawText({ x, cy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, t), label);
    x += MeasureText(22.0f, label).x + 34.0f;
    return x;
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float coinT  = (float)ComputeMotion(openSec, COIN_OFFSET, COIN_FRAMES);
    const float listT  = (float)ComputeMotion(openSec, 0.0, LIST_FRAMES);
    const float infoT  = (float)ComputeMotion(openSec, INFO_OFFSET, INFO_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ===== TITLE + DAY/NIGHT INDICATOR =======================================
    SetFont(g_fDF);
    DrawTextShadow({ TITLE_X, TITLE_Y - (1.0f - titleT) * 16.0f }, 46.0f,
                   WithAlpha(COL_TITLE, titleT), "TOWN");
    DrawRect({ TITLE_X, RULE_Y }, { 1130.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));

    // day/night medallion (real rotating-coin art) + time label, top-right
    {
        int   coinTex = g_night ? g_moonTex : g_sunTex;
        const UV& cuv = g_night ? COIN_MOON : COIN_SUN;
        DrawFitted(coinTex, cuv, COIN_TEX_W, COIN_TEX_H,
                   COIN_BX, COIN_BY, COIN_BW, COIN_BH, coinT, 0.5f, (1.0f - coinT) * 8.0f);
        // "LV" tag echo (real art) sits to the left of the coin as a small label
        SetFont(g_fRodin);
        if (g_lvTex >= 0) {
            float lh = 22.0f;
            float aspect = ((LV_UV.u1 - LV_UV.u0) * LV_TEX_W) / ((LV_UV.v1 - LV_UV.v0) * LV_TEX_H);
            float lw = lh * aspect;
            float lx = TITLE_X + 360.0f, ly = TITLE_Y + 12.0f;
            DrawImage(g_lvTex, { lx, ly }, { lx + lw, ly + lh },
                      { LV_UV.u0, LV_UV.v0 }, { LV_UV.u1, LV_UV.v1 }, WithAlpha(COL_WHITE, titleT));
            DrawText({ lx + lw + 8.0f, ly - 4.0f }, 26.0f, WithAlpha(COL_TEXT_SEL, titleT), "12");
        }
        DrawTextAligned({ COIN_BX - 150.0f, TITLE_Y + 6.0f }, { COIN_BX - 8.0f, TITLE_Y + 44.0f }, 28.0f,
                        WithAlpha(g_night ? COL_MOON : COL_SUN, titleT),
                        g_night ? "NIGHT" : "DAY", Align::Right, true, true);
    }

    // ===== LEFT: ACTION LIST WINDOW ==========================================
    const float listSlide = (1.0f - listT) * 24.0f;
    const float lx = LIST_X, ly = LIST_Y + listSlide;
    DrawWindow(lx, ly, LIST_W, LIST_H, listT, "ACTIONS");

    const float rowsTop = ly + HEADER_H + 10.0f;
    const float rowL = lx + ROW_PAD, rowR = lx + LIST_W - ROW_PAD;

    // eased selection highlight (clamped to the visible window) â€” shop idiom
    if (listT > 0.5f) {
        float moveT = (float)ComputeMotion(g_moveStart, 0.0, SELECT_MOVE_FRAMES);
        float prevSlot = std::clamp((float)(g_prevSel - g_scroll), 0.0f, (float)(VISIBLE_ROWS - 1));
        float curSlot  = std::clamp((float)(g_sel     - g_scroll), 0.0f, (float)(VISIBLE_ROWS - 1));
        float slot = Lerp(prevSlot, curSlot, moveT);
        float hy = rowsTop + slot * ROW_H;
        DrawVGradient({ rowL, hy + 3 }, { rowR, hy + ROW_H - 5 },
                      WithAlpha(COL_SEL_TOP, listT), WithAlpha(COL_SEL_BOT, listT));
    }

    // visible action rows: real icon (left) + real label art / ASCII (centre)
    for (int row = 0; row < VISIBLE_ROWS; ++row) {
        int idx = g_scroll + row;
        if (idx >= ACTION_COUNT) break;
        const Action& a = ACTIONS[idx];
        float top = rowsTop + row * ROW_H;
        bool selected = (idx == g_sel);

        // real town icon, aspect-fit into a square cell on the left of the row
        const float iconBX = rowL + 8.0f, iconBY = top + 6.0f, iconBox = ROW_H - 14.0f;
        DrawFitted(g_iconTex, *a.icon, ICON_TEX_W, ICON_TEX_H,
                   iconBX, iconBY, iconBox, iconBox, listT, 0.5f);

        const float labX = rowL + ROW_H + 6.0f;
        if (a.label && g_labelTex >= 0) {
            // real label art ("Pass Time" / "Take Photo" / "Talk / Use"), left-anchored
            float lh = 30.0f;
            float aspect = ((a.label->u1 - a.label->u0) * LABEL_TEX_W) /
                           ((a.label->v1 - a.label->v0) * LABEL_TEX_H);
            float lw = lh * aspect;
            float lyp = top + (ROW_H - lh) * 0.5f;
            DrawImage(g_labelTex, { labX, lyp }, { labX + lw, lyp + lh },
                      { a.label->u0, a.label->v0 }, { a.label->u1, a.label->v1 },
                      WithAlpha(COL_WHITE, listT));
        } else {
            // ASCII fallback for actions without baked label art
            SetFont(g_fSeurat);
            DrawTextAligned({ labX, top }, { rowR - 12.0f, top + ROW_H }, 28.0f,
                            WithAlpha(selected ? COL_TEXT_SEL : COL_TEXT, listT),
                            a.ascii, Align::Left, true, true);
        }
    }

    // scrollbar (only when the action list overflows)
    if (ACTION_COUNT > VISIBLE_ROWS && listT > 0.5f) {
        float trackX = lx + LIST_W - 7.0f, trackTop = rowsTop, trackH = VISIBLE_ROWS * ROW_H;
        DrawRect({ trackX, trackTop }, { trackX + 3, trackTop + trackH }, WithAlpha(COL_RULE, listT));
        float frac = (float)VISIBLE_ROWS / (float)ACTION_COUNT;
        float thumbH = trackH * frac;
        float denom = ScrollLimit(); if (denom < 1.0f) denom = 1.0f;
        float thumbY = trackTop + (trackH - thumbH) * ((float)g_scroll / denom);
        DrawRect({ trackX, thumbY }, { trackX + 3, thumbY + thumbH }, WithAlpha(COL_SEL_TOP, listT));
    }

    // ===== RIGHT: INFO WINDOW (tracks the selection) =========================
    const float ix = INFO_X, iy = INFO_Y + (1.0f - infoT) * 24.0f;
    DrawWindow(ix, iy, INFO_W, INFO_H, infoT, "INFO");
    const Action& sel = ACTIONS[std::clamp(g_sel, 0, ACTION_COUNT - 1)];

    // dark plate + the selected action's real icon (large, aspect-fit)
    const float plateX = ix + 26.0f, plateY = iy + HEADER_H + 18.0f;
    const float plateW = INFO_W - 52.0f, plateH = 150.0f;
    DrawRect({ plateX, plateY }, { plateX + plateW, plateY + plateH }, WithAlpha(COL_PLATE, infoT));
    DrawRect({ plateX, plateY }, { plateX + plateW, plateY + 2 }, WithAlpha(COL_RULE, infoT));
    DrawRect({ plateX, plateY + plateH - 2 }, { plateX + plateW, plateY + plateH }, WithAlpha(COL_RULE, infoT));
    DrawFitted(g_iconTex, *sel.icon, ICON_TEX_W, ICON_TEX_H,
               plateX + 10.0f, plateY + 10.0f, plateW - 20.0f, plateH - 20.0f, infoT, 0.5f);

    // action name + two-line blurb
    float textTop = plateY + plateH + 16.0f;
    SetFont(g_fSeurat);
    DrawTextAligned({ ix + 22.0f, textTop }, { ix + INFO_W - 18.0f, textTop + 38.0f }, 30.0f,
                    WithAlpha(COL_TEXT_SEL, infoT), sel.ascii, Align::Left, true, true);
    DrawText({ ix + 24.0f, textTop + 48.0f }, 20.0f, WithAlpha(COL_DESC, infoT), sel.info1);
    DrawText({ ix + 24.0f, textTop + 76.0f }, 20.0f, WithAlpha(COL_DESC, infoT), sel.info2);

    // ===== TRANSIENT ACTION FEEDBACK =========================================
    double age = Now() - g_msgStart;
    if (g_msgStart > 0.0 && age < 1.6) {
        float ma = std::min(1.0f, (float)((1.6 - age) / 0.4));
        SetFont(g_fSeurat);
        DrawTextAligned({ LIST_X, 572.0f }, { 1130.0f, 604.0f }, 24.0f,
                        WithAlpha(COL_OK, ma), g_msg, Align::Center, true, true);
    }

    // ===== FOOTER ============================================================
    {
        SetFont(g_fRodin);
        const float aAsp = 0.921f;
        float hx = TITLE_X, hcy = 634.0f;
        DrawRect({ TITLE_X, 612.0f }, { 1130.0f, 614.0f }, WithAlpha(COL_RULE, footT));
        hx = DrawHint(hx, hcy, GLYPH_A, aAsp, "Select", footT);
        hx = DrawHint(hx, hcy, GLYPH_B, aAsp, "Back",   footT);
        DrawText({ hx, hcy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "[Up/Down] Move");
    }
    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void TownInit() { Init(); }
void TownDraw(double openSeconds) { Draw(openSeconds); }
void TownInput(const ScreenInput& in) { Input(in); }
void TownReset() { Reset(); }
const char* TownNav() { return Nav(); }
