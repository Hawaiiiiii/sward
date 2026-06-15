// =============================================================================
// screen_mission.cpp â€” the Mission Select, re-authored as clean hand-written C++
// in the UnleashedRecomp ui/options_menu idiom (NOT a CSD node dump â€” the raw
// mission.json is just one stretched mat_result_comon_001 bar, the "MISSION
// FAILED" wordmark, a spinning ring and one footer glyph/word pair, with several
// 1700px stretch-arms that smear if transcribed). Layout lives in named 1280x720
// constants; the mission list and the objective panel are bounded windows drawn
// from the REAL Sonic Unleashed silver-chrome frame via ui::DrawGameWindow. The
// distinctive retail art is the REAL extracted atlases the screen ships with:
//   * the gold "MISSION" wordmark              (mat_misson_en_001, top line),
//   * the spinning gold ring frame             (mat_comon_002, frame 0),
//   * the Xbox A / B button glyphs             (mat_comon_x360_001, top row),
//   * the baked English word labels Back/Start (mat_comon_en_001),
// each placed at deliberate sane rects with aspect preserved (graceful gradient /
// ASCII fallback when an atlas is missing â€” exactly like options/world_map).
//
// Fully interactive + stateful like the recomp's options_menu / the shop:
//   * Up/Down move the cursor over the mission rows (eased highlight + scrollbar),
//   * the INFO panel on the right tracks the selection (objective, target, reward
//     â€” ring missions show the real spinning ring icon next to the reward),
//   * (A) "starts" the selected mission (transient confirm flash),
//   * (B) backs out (no-op in the standalone build).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <cmath>

using namespace ui;

namespace {

// ---- per-element UV sub-rect (normalized atlas coords) ----------------------
struct UV { float u0, v0, u1, v1; };

// ---- mission catalogue ------------------------------------------------------
// Each mission has a type name, a one/two-line objective, a quantified target,
// a reward, a difficulty (1..5 stars), and whether it has been cleared. The INFO
// panel spells the target/reward out; ring-reward missions get the real ring art.
struct Mission {
    const char* name;       // mission type (list label)
    const char* obj1;       // objective text line 1
    const char* obj2;       // objective text line 2
    const char* target;     // quantified goal (e.g. "200 RINGS", "1:30.00")
    int         reward;     // ring reward (drawn with the ring icon)
    int         stars;      // difficulty 1..5
    int         rank;       // best rank achieved 0..5 (S..E); -1 = none / not cleared
    bool        cleared;
};

const Mission MISSIONS[] = {
    { "RING COLLECTION", "Gather the required rings",      "before the time runs out.", "200 RINGS",  300,  1,  1, true  },
    { "TIME ATTACK",     "Reach the goal as fast",         "as you possibly can.",      "1:30.00",    500,  2,  2, true  },
    { "ENEMY HUNT",      "Destroy every enemy",            "in the target area.",       "25 ENEMIES", 400,  2, -1, false },
    { "PERFECT RUN",     "Clear the route without",        "taking any damage.",        "NO DAMAGE",  800,  4,  0, true  },
    { "SPEED DEMON",     "Hold top speed across",          "the whole stretch.",        "300 KM/H",   600,  3, -1, false },
    { "RAINBOW RINGS",   "Pass through all of",            "the rainbow ring hoops.",   "12 HOOPS",   450,  3,  3, true  },
    { "BOOST CHAIN",     "Keep the boost gauge",           "lit the entire course.",    "FULL CHAIN", 700,  4, -1, false },
    { "TRICK MASTER",    "Score points with aerial",       "tricks off the ramps.",     "50000 PTS",  550,  3,  2, true  },
    { "SURVIVAL",        "Stay alive as the night",        "creatures keep coming.",    "3 MINUTES",  900,  5, -1, false },
};
constexpr int MISSION_COUNT = (int)(sizeof(MISSIONS) / sizeof(MISSIONS[0]));

// rank letters: 0=S 1=A 2=B 3=C 4=D 5=E
const char* const RANK_NAME[6] = { "S", "A", "B", "C", "D", "E" };

// ---- real-art atlases -------------------------------------------------------
const char* const ASSET_BASE = "assets/mission/";
int g_titleTex = -1;   // mat_misson_en_001 (512x128) â€” "MISSION" wordmark (top line)
int g_ringTex  = -1;   // mat_comon_002     (1024x128) â€” gold ring spin atlas
int g_glyphTex = -1;   // mat_comon_x360_001 (512x512) â€” Xbox button glyphs
int g_wordTex  = -1;   // mat_comon_en_001  (128x512)  â€” baked English word labels

constexpr float TITLE_TEX_W = 512.0f,  TITLE_TEX_H = 128.0f;
constexpr float RING_TEX_W  = 1024.0f, RING_TEX_H  = 128.0f;
constexpr float GLYPH_TEX_W = 512.0f,  GLYPH_TEX_H = 512.0f;
constexpr float WORD_TEX_W  = 128.0f,  WORD_TEX_H  = 512.0f;

// gold "MISSION" wordmark â€” top line of mat_misson_en_001, measured from alpha.
const UV TITLE_UV = { 0.00000f, 0.00000f, 0.50195f, 0.29688f };

// first ring frame (full circle) â€” top-left of mat_comon_002, measured from alpha.
const UV RING_UV  = { 0.00098f, 0.00781f, 0.07324f, 0.50000f };

// Xbox glyphs â€” top row of mat_comon_x360_001 (same UVs as options/world_map).
const UV GLYPH_A  = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B  = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };

// Baked English words â€” bands measured from mat_comon_en_001's alpha.
const UV WORD_BACK  = { 0.03125f, 0.07227f, 0.52344f, 0.11523f };
const UV WORD_START = { 0.02344f, 0.53906f, 0.53125f, 0.58398f };

// ---- layout (reference px) --------------------------------------------------
constexpr float TITLE_X = 150.0f, TITLE_Y = 50.0f, RULE_Y = 118.0f;

constexpr float LIST_X = 150.0f, LIST_Y = 150.0f, LIST_W = 600.0f, LIST_H = 410.0f;
constexpr float INFO_X = 780.0f, INFO_Y = 150.0f, INFO_W = 350.0f, INFO_H = 410.0f;
constexpr float HEADER_H = 52.0f;          // caption strip atop each window
constexpr float ROW_H    = 60.0f;
constexpr int   VISIBLE_ROWS = 6;          // 9 missions -> the list scrolls
constexpr float ROW_PAD  = 12.0f;          // inset of rows inside the list window

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double TITLE_FRAMES = 14.0;
constexpr double LIST_FRAMES  = 16.0;
constexpr double INFO_OFFSET  = 4.0,  INFO_FRAMES = 14.0;
constexpr double FOOT_OFFSET  = 10.0, FOOT_FRAMES = 12.0;
constexpr double SELECT_MOVE_FRAMES = 8.0;

// ---- palette (shared with pause/result/shop/status/world_map/options) -------
const uint32_t COL_BG_TOP    = RGBA(12, 20, 38, 255);
const uint32_t COL_BG_BOT    = RGBA(5, 9, 18, 255);
const uint32_t COL_PANEL_TOP = RGBA(18, 30, 52, 232);
const uint32_t COL_PANEL_BOT = RGBA(8, 14, 26, 232);
const uint32_t COL_HEAD_TOP  = RGBA(28, 52, 92, 244);
const uint32_t COL_HEAD_BOT  = RGBA(16, 30, 56, 244);
const uint32_t COL_SEL_TOP   = RGBA(64, 150, 235, 225);
const uint32_t COL_SEL_BOT   = RGBA(28, 92, 180, 225);
const uint32_t COL_TITLE     = RGBA(255, 209, 74, 255);
const uint32_t COL_TEXT      = RGBA(214, 226, 240, 255);
const uint32_t COL_TEXT_SEL  = RGBA(255, 255, 255, 255);
const uint32_t COL_DESC      = RGBA(178, 194, 214, 255);
const uint32_t COL_RULE      = RGBA(120, 170, 230, 90);
const uint32_t COL_VALUE     = RGBA(255, 209, 74, 255);   // reward / target (gold)
const uint32_t COL_LOCKED    = RGBA(120, 120, 132, 255);
const uint32_t COL_CLEAR     = RGBA(120, 230, 140, 255);
const uint32_t COL_STAR_ON   = RGBA(255, 209, 74, 255);
const uint32_t COL_STAR_OFF  = RGBA(70, 86, 112, 220);
const uint32_t COL_OK        = RGBA(120, 230, 140, 255);
const uint32_t COL_FOOTER    = RGBA(190, 205, 225, 220);
const uint32_t COL_WHITE     = RGBA(255, 255, 255, 255);

// ---- real fonts (same trio as options): Seurat = labels/body, NewRodin =
// values/footers, DFSoGei = titles/captions --------------------------------
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// ---- interactive state ------------------------------------------------------
int    g_sel = 0, g_prevSel = 0, g_scroll = 0;
double g_moveStart = -100.0;        // Now() when the highlight last moved
double g_msgStart  = -100.0;        // Now() of the last "start mission" flash

float ScrollLimit() { return (float)std::max(0, MISSION_COUNT - VISIBLE_ROWS); }

void ClampScrollToSel() {
    if (g_sel < g_scroll) g_scroll = g_sel;
    if (g_sel > g_scroll + VISIBLE_ROWS - 1) g_scroll = g_sel - VISIBLE_ROWS + 1;
    g_scroll = std::clamp(g_scroll, 0, (int)ScrollLimit());
}

// thousands-separated integer -> buf (e.g. 12345 -> "12,345")  [shop idiom]
void Commafy(int v, char* out, int n) {
    char raw[16]; std::snprintf(raw, sizeof(raw), "%d", v);
    int len = (int)std::strlen(raw), o = 0;
    for (int i = 0; i < len && o < n - 1; ++i) {
        if (i > 0 && (len - i) % 3 == 0 && o < n - 1) out[o++] = ',';
        out[o++] = raw[i];
    }
    out[o] = '\0';
}

void Init() {
    GameFrameTex();   // lazy-load the shared chrome frame used by DrawGameWindow
    if (g_titleTex < 0) g_titleTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_misson_en_001.png");
    if (g_ringTex  < 0) g_ringTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_002.png");
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_x360_001.png");
    if (g_wordTex  < 0) g_wordTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_en_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF (im_font_atlas)
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");    // real game MSDF
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");   // real DFSoGeiStd-W7 (titles)
}

void Reset() {
    g_sel = 0; g_prevSel = 0; g_scroll = 0;
    g_moveStart = -100.0; g_msgStart = -100.0;
}

void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = std::max(0, g_sel - 1);
        if (in.down) g_sel = std::min(MISSION_COUNT - 1, g_sel + 1);
        ClampScrollToSel();
        g_moveStart = Now();
    }
    if (in.accept) { g_msgStart = Now(); }   // "start mission" (standalone: a flash)
    // cancel: would back out to the menu in-game; no-op in the standalone build.
}

// ---- aspect-preserving image fit into a box (world_map / options idiom) ------
void DrawFitted(int tex, const UV& uv, float texW, float texH,
                float bx, float by, float bw, float bh,
                float t, uint32_t col, float vAnchor = 0.5f, float slide = 0.0f) {
    if (tex < 0 || t <= 0.0f) return;
    float artW = (uv.u1 - uv.u0) * texW;
    float artH = (uv.v1 - uv.v0) * texH;
    if (artW <= 0.0f || artH <= 0.0f) return;
    float scale = std::min(bw / artW, bh / artH);
    float w = artW * scale, h = artH * scale;
    float x = bx + (bw - w) * 0.5f;
    float y = by + (bh - h) * vAnchor + slide;
    DrawImage(tex, { x, y }, { x + w, y + h }, { uv.u0, uv.v0 }, { uv.u1, uv.v1 }, WithAlpha(col, t));
}

// a bounded window from the REAL game frame (9-slice), tinted blue-silver, with a
// brighter header strip â€” the recomp's DrawPauseContainer technique on Unleashed art.
void DrawWindow(float x, float y, float w, float h, float t, const char* caption) {
    DrawGameWindow({ x, y }, { x + w, y + h }, HEADER_H, t);
    DrawRect({ x + 12, y + HEADER_H - 2 }, { x + w - 12, y + HEADER_H }, WithAlpha(COL_RULE, t));
    if (caption) {
        SetFont(g_fDF);
        DrawTextAligned({ x + 18, y }, { x + w - 14, y + HEADER_H }, 26.0f,
                        WithAlpha(COL_TITLE, t), caption, Align::Left, true, true);
    }
}

// a small procedural difficulty star (filled diamond) â€” no triangle prim, so we
// stack short bars into a diamond. `on` = lit (gold), else a dim slot.
void DrawStar(float cx, float cy, bool on, float t) {
    const float s = 8.0f;   // half-extent
    uint32_t col = WithAlpha(on ? COL_STAR_ON : COL_STAR_OFF, t);
    for (int i = 0; i <= 6; ++i) {
        float fy = (float)i / 6.0f;             // 0..1 top->mid
        float hw = s * fy;                       // widen toward the middle
        DrawRect({ cx - hw, cy - s + fy * s }, { cx + hw, cy - s + fy * s + 2.0f }, col);
        DrawRect({ cx - hw, cy + s - fy * s - 2.0f }, { cx + hw, cy + s - fy * s }, col);
    }
}

// the real spinning-ring icon at a sane box (frame 0 only â€” a clean upright ring).
void DrawRingIcon(float cx, float cy, float h, float t) {
    if (g_ringTex < 0 || t <= 0.0f) {
        // graceful fallback: a gold ring outline made of four bars
        uint32_t c = WithAlpha(COL_VALUE, t);
        float r = h * 0.5f;
        DrawRect({ cx - r, cy - r }, { cx + r, cy - r + 3 }, c);
        DrawRect({ cx - r, cy + r - 3 }, { cx + r, cy + r }, c);
        DrawRect({ cx - r, cy - r }, { cx - r + 3, cy + r }, c);
        DrawRect({ cx + r - 3, cy - r }, { cx + r, cy + r }, c);
        return;
    }
    float aspect = ((RING_UV.u1 - RING_UV.u0) * RING_TEX_W) /
                   ((RING_UV.v1 - RING_UV.v0) * RING_TEX_H);
    float w = h * aspect;
    DrawImage(g_ringTex, { cx - w * 0.5f, cy - h * 0.5f }, { cx + w * 0.5f, cy + h * 0.5f },
              { RING_UV.u0, RING_UV.v0 }, { RING_UV.u1, RING_UV.v1 }, WithAlpha(COL_WHITE, t));
}

// one footer hint: real button glyph + ASCII label, laid out left-to-right.
float DrawHint(float x, float cy, const UV& g, float gAspect, const char* token,
               const char* label, float t) {
    const float gh = 30.0f;
    SetFont(g_fRodin);
    if (g_glyphTex >= 0) {
        float gw = gh * gAspect;
        DrawImage(g_glyphTex, { x, cy - gh * 0.5f }, { x + gw, cy + gh * 0.5f },
                  { g.u0, g.v0 }, { g.u1, g.v1 }, WithAlpha(COL_WHITE, t));
        x += gw + 8.0f;
    } else {
        DrawText({ x, cy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, t), token);
        x += MeasureText(22.0f, token).x + 8.0f;
    }
    DrawText({ x, cy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, t), label);
    x += MeasureText(22.0f, label).x + 34.0f;
    return x;
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float listT  = (float)ComputeMotion(openSec, 0.0, LIST_FRAMES);
    const float infoT  = (float)ComputeMotion(openSec, INFO_OFFSET, INFO_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ---- title (real "MISSION" wordmark) + "SELECT" tail ----
    SetFont(g_fDF);
    if (g_titleTex >= 0 && titleT > 0.0f) {
        float aspect = ((TITLE_UV.u1 - TITLE_UV.u0) * TITLE_TEX_W) /
                       ((TITLE_UV.v1 - TITLE_UV.v0) * TITLE_TEX_H);
        float lh = 46.0f, lw = lh * aspect;
        float lx = TITLE_X;
        float ly = TITLE_Y - (1.0f - titleT) * 16.0f;
        DrawImage(g_titleTex, { lx, ly }, { lx + lw, ly + lh },
                  { TITLE_UV.u0, TITLE_UV.v0 }, { TITLE_UV.u1, TITLE_UV.v1 }, WithAlpha(COL_WHITE, titleT));
        DrawTextShadow({ lx + lw + 16.0f, ly + 2.0f }, 40.0f, WithAlpha(COL_TITLE, titleT), "SELECT");
    } else {
        DrawTextShadow({ TITLE_X, TITLE_Y - (1.0f - titleT) * 16.0f }, 46.0f,
                       WithAlpha(COL_TITLE, titleT), "MISSION SELECT");
    }
    DrawRect({ TITLE_X, RULE_Y }, { 1130.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));

    // cleared count on the title rule's right
    {
        int cleared = 0;
        for (int i = 0; i < MISSION_COUNT; ++i) if (MISSIONS[i].cleared) ++cleared;
        char line[40];
        std::snprintf(line, sizeof(line), "CLEARED  %d / %d", cleared, MISSION_COUNT);
        SetFont(g_fRodin);
        DrawTextAligned({ 740, TITLE_Y + 6 }, { 1130, TITLE_Y + 46 }, 28.0f,
                        WithAlpha(COL_CLEAR, titleT), line, Align::Right, true, true);
    }

    // ---- mission list window (left) ----
    const float listSlide = (1.0f - listT) * 24.0f;
    const float lx = LIST_X, ly = LIST_Y + listSlide;
    DrawWindow(lx, ly, LIST_W, LIST_H, listT, "MISSIONS");

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

    // visible mission rows: name (left) + cleared rank / locked state (right)
    for (int row = 0; row < VISIBLE_ROWS; ++row) {
        int idx = g_scroll + row;
        if (idx >= MISSION_COUNT) break;
        const Mission& m = MISSIONS[idx];
        float top = rowsTop + row * ROW_H;
        bool selected = (idx == g_sel);

        SetFont(g_fSeurat);
        DrawTextAligned({ rowL + 16, top }, { rowR - 130, top + ROW_H }, 27.0f,
                        WithAlpha(selected ? COL_TEXT_SEL : COL_TEXT, listT),
                        m.name, Align::Left, true, true);

        // right side: best rank letter if cleared, else NEW
        SetFont(g_fRodin);
        if (m.cleared && m.rank >= 0 && m.rank <= 5) {
            char rb[24];
            std::snprintf(rb, sizeof(rb), "RANK %s", RANK_NAME[m.rank]);
            DrawTextAligned({ rowR - 130, top }, { rowR - 14, top + ROW_H }, 23.0f,
                            WithAlpha(COL_VALUE, listT), rb, Align::Right, true, true);
        } else {
            DrawTextAligned({ rowR - 130, top }, { rowR - 14, top + ROW_H }, 22.0f,
                            WithAlpha(COL_LOCKED, listT), "NEW", Align::Right, true, true);
        }
    }

    // scrollbar (only when the list overflows)
    if (MISSION_COUNT > VISIBLE_ROWS && listT > 0.5f) {
        float trackX = lx + LIST_W - 7.0f, trackTop = rowsTop, trackH = VISIBLE_ROWS * ROW_H;
        DrawRect({ trackX, trackTop }, { trackX + 3, trackTop + trackH }, WithAlpha(COL_RULE, listT));
        float frac = (float)VISIBLE_ROWS / (float)MISSION_COUNT;
        float thumbH = trackH * frac;
        float denom = ScrollLimit(); if (denom < 1.0f) denom = 1.0f;
        float thumbY = trackTop + (trackH - thumbH) * ((float)g_scroll / denom);
        DrawRect({ trackX, thumbY }, { trackX + 3, thumbY + thumbH }, WithAlpha(COL_SEL_TOP, listT));
    }

    // ---- objective / info window (right, tracks the selection) ----
    const float ix = INFO_X, iy = INFO_Y + (1.0f - infoT) * 24.0f;
    DrawWindow(ix, iy, INFO_W, INFO_H, infoT, "INFO");
    const Mission& m = MISSIONS[std::clamp(g_sel, 0, MISSION_COUNT - 1)];

    float ty = iy + HEADER_H + 16.0f;
    const float il = ix + 20.0f, ir = ix + INFO_W - 18.0f;

    // mission name
    SetFont(g_fSeurat);
    DrawTextAligned({ il, ty }, { ir, ty + 38 }, 30.0f,
                    WithAlpha(COL_TEXT_SEL, infoT), m.name, Align::Left, true, true);
    ty += 46.0f;

    // difficulty stars row
    DrawText({ il, ty }, 21.0f, WithAlpha(COL_DESC, infoT), "DIFFICULTY");
    {
        float sx = il + 140.0f, sy = ty + 11.0f;
        for (int s = 0; s < 5; ++s)
            DrawStar(sx + s * 24.0f, sy, s < m.stars, infoT);
    }
    ty += 38.0f;

    // thin divider
    DrawRect({ il, ty }, { ir, ty + 2 }, WithAlpha(COL_RULE, infoT));
    ty += 14.0f;

    // objective text (two lines)
    DrawText({ il, ty }, 22.0f, WithAlpha(COL_DESC, infoT), "OBJECTIVE");
    ty += 30.0f;
    DrawText({ il, ty }, 21.0f, WithAlpha(COL_TEXT, infoT), m.obj1);
    ty += 28.0f;
    DrawText({ il, ty }, 21.0f, WithAlpha(COL_TEXT, infoT), m.obj2);
    ty += 38.0f;

    // target line
    DrawText({ il, ty }, 22.0f, WithAlpha(COL_DESC, infoT), "TARGET");
    SetFont(g_fRodin);
    DrawTextAligned({ il + 110.0f, ty - 4.0f }, { ir, ty + 26.0f }, 26.0f,
                    WithAlpha(COL_VALUE, infoT), m.target, Align::Right, true, true);
    ty += 38.0f;

    // reward line (real ring icon + commafied amount)
    SetFont(g_fSeurat);
    DrawText({ il, ty }, 22.0f, WithAlpha(COL_DESC, infoT), "REWARD");
    {
        SetFont(g_fRodin);   // before MeasureText so the ring icon hugs the number
        char rb[24]; Commafy(m.reward, rb, sizeof(rb));
        // reward number right-aligned, ring icon just left of it
        float numR = ir;
        V2 nsz = MeasureText(26.0f, rb);
        float numL = numR - nsz.x;
        DrawTextAligned({ numL, ty - 4.0f }, { numR, ty + 26.0f }, 26.0f,
                        WithAlpha(COL_VALUE, infoT), rb, Align::Right, true, true);
        DrawRingIcon(numL - 22.0f, ty + 11.0f, 26.0f, infoT);
    }
    ty += 40.0f;

    // best-record / status footer inside the panel
    SetFont(g_fRodin);
    if (m.cleared) {
        char line[48];
        if (m.rank >= 0 && m.rank <= 5)
            std::snprintf(line, sizeof(line), "CLEARED  -  BEST RANK %s", RANK_NAME[m.rank]);
        else
            std::snprintf(line, sizeof(line), "CLEARED");
        DrawText({ il, ty }, 21.0f, WithAlpha(COL_CLEAR, infoT), line);
    } else {
        DrawText({ il, ty }, 21.0f, WithAlpha(COL_LOCKED, infoT), "NOT YET CLEARED");
    }

    // ---- footer button guide (real glyphs + real word art where it fits) ----
    {
        SetFont(g_fRodin);
        float hx = TITLE_X, hcy = 634.0f;
        const float aAsp = ((GLYPH_A.u1 - GLYPH_A.u0) * GLYPH_TEX_W) / ((GLYPH_A.v1 - GLYPH_A.v0) * GLYPH_TEX_H);
        const float bAsp = ((GLYPH_B.u1 - GLYPH_B.u0) * GLYPH_TEX_W) / ((GLYPH_B.v1 - GLYPH_B.v0) * GLYPH_TEX_H);

        // [Up/Down] is keyboard-only -> ASCII; the pad actions use real glyphs +
        // baked Start/Back word art where the atlas is present.
        DrawText({ hx, hcy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "[Up/Down] Select");
        hx += MeasureText(22.0f, "[Up/Down] Select").x + 34.0f;

        // (A) Start â€” prefer the real "Start" word art beside the glyph
        if (g_glyphTex >= 0) {
            float gw = 30.0f * aAsp;
            DrawImage(g_glyphTex, { hx, hcy - 15.0f }, { hx + gw, hcy + 15.0f },
                      { GLYPH_A.u0, GLYPH_A.v0 }, { GLYPH_A.u1, GLYPH_A.v1 }, WithAlpha(COL_WHITE, footT));
            hx += gw + 8.0f;
        } else {
            DrawText({ hx, hcy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "(A)");
            hx += MeasureText(22.0f, "(A)").x + 8.0f;
        }
        if (g_wordTex >= 0) {
            float wAsp = ((WORD_START.u1 - WORD_START.u0) * WORD_TEX_W) /
                         ((WORD_START.v1 - WORD_START.v0) * WORD_TEX_H);
            float wh = 22.0f, ww = wh * wAsp;
            DrawImage(g_wordTex, { hx, hcy - wh * 0.5f }, { hx + ww, hcy + wh * 0.5f },
                      { WORD_START.u0, WORD_START.v0 }, { WORD_START.u1, WORD_START.v1 }, WithAlpha(COL_WHITE, footT));
            hx += ww + 34.0f;
        } else {
            DrawText({ hx, hcy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "Start");
            hx += MeasureText(22.0f, "Start").x + 34.0f;
        }

        // (B) Back â€” glyph + baked "Back" word art
        if (g_glyphTex >= 0) {
            float gw = 30.0f * bAsp;
            DrawImage(g_glyphTex, { hx, hcy - 15.0f }, { hx + gw, hcy + 15.0f },
                      { GLYPH_B.u0, GLYPH_B.v0 }, { GLYPH_B.u1, GLYPH_B.v1 }, WithAlpha(COL_WHITE, footT));
            hx += gw + 8.0f;
        } else {
            DrawText({ hx, hcy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "(B)");
            hx += MeasureText(22.0f, "(B)").x + 8.0f;
        }
        if (g_wordTex >= 0) {
            float wAsp = ((WORD_BACK.u1 - WORD_BACK.u0) * WORD_TEX_W) /
                         ((WORD_BACK.v1 - WORD_BACK.v0) * WORD_TEX_H);
            float wh = 22.0f, ww = wh * wAsp;
            DrawImage(g_wordTex, { hx, hcy - wh * 0.5f }, { hx + ww, hcy + wh * 0.5f },
                      { WORD_BACK.u0, WORD_BACK.v0 }, { WORD_BACK.u1, WORD_BACK.v1 }, WithAlpha(COL_WHITE, footT));
            hx += ww + 34.0f;
        } else {
            DrawText({ hx, hcy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "Back");
        }
    }

    // ---- transient "start mission" flash ----
    double age = Now() - g_msgStart;
    if (g_msgStart > 0.0 && age < 1.4) {
        float ma = std::min(1.0f, (float)((1.4 - age) / 0.4));
        // gentle pulse so it reads as a confirmation
        float pulse = 0.85f + 0.15f * (float)std::sin(Now() * 10.0);
        char line[64];
        std::snprintf(line, sizeof(line), "Starting %s", m.name);
        SetFont(g_fSeurat);
        DrawTextAligned({ LIST_X, 580 }, { 1130, 612 }, 26.0f,
                        WithAlpha(COL_OK, ma * pulse), line, Align::Center, true, true);
    }

    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void MissionInit() { Init(); }
void MissionDraw(double openSeconds) { Draw(openSeconds); }
void MissionInput(const ScreenInput& in) { Input(in); }
void MissionReset() { Reset(); }
