// =============================================================================
// screen_mediaroom.cpp â€” the Media Room / gallery (sound test + movies + art),
// re-authored as clean hand-written C++ in the UnleashedRecomp ui/options_menu
// idiom (NOT a CSD node dump â€” the raw mediaroom.json is mostly broken stretch-
// arms: dozens of |x|>1300, w>1300 quads from the 9-slice list cells). Layout
// lives in named 1280x720 constants; the entry list and the preview/info pane are
// bounded windows drawn from the REAL Sonic Unleashed silver-chrome frame via
// ui::DrawGameWindow. The distinctive retail art is the REAL extracted atlases
// this screen actually ships with:
//   * the oval "now playing" emblem               (mat_media_common_001, z55),
//   * the gold filigree / swoosh ornaments        (mat_media_common_001 / _002),
//   * the per-region art-gallery thumbnails        (mat_worldmap_ss_002, a 3x3 grid),
//   * the gold digit strip for track numbers       (mat_media_num_001),
//   * the Xbox A / B / LB / RB button glyphs        (mat_comon_x360_001, top row),
// placed at deliberate sane rects with aspect preserved (bounded gradient windows
// are the graceful fallback when an atlas is missing, like world_map/options).
//
// Fully interactive + stateful like options_menu / the shop:
//   * Q/E (LB/RB) switch the CATEGORY -> MUSIC / MOVIE / ART, each with its own
//     entry list + preview content,
//   * Up/Down move the cursor over the entry rows (eased highlight + scrollbar),
//   * (A) plays / views the selected entry (a transient "NOW PLAYING" / "VIEWING"
//     flash; for MUSIC it latches a now-playing track shown in the preview pane),
//   * (B) backs out (no-op in the standalone build).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <string>
#include <algorithm>
#include <cmath>

using namespace ui;

namespace {

// ---- per-element UV sub-rect (normalized atlas coords) ----------------------
struct UV { float u0, v0, u1, v1; };

// ---- data model -------------------------------------------------------------
// A category (MUSIC / MOVIE / ART) owns a set of entries. MUSIC entries carry an
// artist + duration (sound test); MOVIE entries carry a length; ART entries carry
// a caption + a thumbnail cell in the worldmap flag atlas (reused as gallery art).
enum CatKind { CAT_MUSIC = 0, CAT_MOVIE = 1, CAT_ART = 2, CAT_COUNT = 3 };

struct Entry {
    const char* name;
    const char* sub;       // artist (music) / source (movie) / caption (art)
    int         seconds;   // track / clip length in whole seconds (-1 = n/a)
    int         thumb;      // art-thumbnail cell index 0..8 (-1 = none)
};

struct Category {
    const char*  name;
    CatKind      kind;
    const Entry* entries;
    int          count;
};

// ---- MUSIC (sound test) -----------------------------------------------------
const Entry MUSIC[] = {
    { "Endless Possibility",  "Bowling for Soup", 230, -1 },
    { "Windmill Isle - Day",  "Tomoya Ohtani",    188, -1 },
    { "Apotos - Night",       "Tomoya Ohtani",    201, -1 },
    { "Rooftop Run - Day",    "Tomoya Ohtani",    214, -1 },
    { "Cool Edge - Day",      "Tomoya Ohtani",    176, -1 },
    { "Dragon Road - Night",  "Tomoya Ohtani",    223, -1 },
    { "Eggmanland",           "Tomoya Ohtani",    245, -1 },
    { "The World Adventure",  "Tomoya Ohtani",    258, -1 },
    { "Dear My Friend",       "Hideaki Kobayashi",272, -1 },
};
// ---- MOVIE (event gallery) --------------------------------------------------
const Entry MOVIE[] = {
    { "Opening",              "Event 01", 142, -1 },
    { "The Werehog Awakens",  "Event 03", 96,  -1 },
    { "Meeting Chip",         "Event 05", 88,  -1 },
    { "Professor Pickle",     "Event 09", 110, -1 },
    { "The Gaia Temples",     "Event 14", 134, -1 },
    { "Dark Gaia Rises",      "Event 22", 158, -1 },
    { "Ending",               "Event 27", 176, -1 },
};
// ---- ART (souvenirs / concept gallery) â€” thumb = worldmap flag cell ---------
const Entry ART[] = {
    { "Apotos Concept",       "White Island sketch", -1, 0 },
    { "Spagonia Rooftops",    "Orange roofs study",  -1, 1 },
    { "Chun-nan Vista",       "Dragon Road art",     -1, 2 },
    { "Mazuri Savannah",      "Citadel concept",     -1, 3 },
    { "Holoska Ice",          "Cool Edge study",     -1, 4 },
    { "Shamar Sands",         "Arid Sands art",      -1, 5 },
    { "Adabat Jungle",        "Jungle Joyride art",  -1, 6 },
    { "Empire City",          "Skyscraper study",    -1, 7 },
    { "Eggmanland",           "Final concept",       -1, 8 },
};

const Category CATEGORIES[CAT_COUNT] = {
    { "MUSIC", CAT_MUSIC, MUSIC, (int)(sizeof(MUSIC) / sizeof(Entry)) },
    { "MOVIE", CAT_MOVIE, MOVIE, (int)(sizeof(MOVIE) / sizeof(Entry)) },
    { "ART",   CAT_ART,   ART,   (int)(sizeof(ART)   / sizeof(Entry)) },
};

// ---- real-art atlases -------------------------------------------------------
const char* const ASSET_BASE = "assets/mediaroom/";
int g_ornTex   = -1;   // mat_media_common_001 (oval emblem + gold filigree)
int g_numTex   = -1;   // mat_media_num_001    (gold digit strip)
int g_glyphTex = -1;   // mat_comon_x360_001   (Xbox button glyphs)
int g_thumbTex = -1;   // mat_worldmap_ss_002  (3x3 cell grid -> gallery thumbnails)

// ---- real-game fonts (MSDF sweep) --------------------------------------------
static int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// atlas pixel sizes (for aspect-correct fitting)
constexpr float ORN_TEX_W   = 256.0f, ORN_TEX_H   = 512.0f;
constexpr float NUM_TEX_W   = 256.0f, NUM_TEX_H   = 32.0f;
constexpr float GLYPH_TEX_W = 512.0f, GLYPH_TEX_H = 512.0f;
constexpr float THUMB_TEX_W = 512.0f, THUMB_TEX_H = 512.0f;

// oval "Dickle's Room" / now-playing emblem in mat_media_common_001 (z55 node).
const UV EMBLEM_UV   = { 0.53516f, 0.56445f, 0.99609f, 0.72656f };
// gold filigree scroll, lower-left of mat_media_common_001 (z214/z215 node).
const UV FILIGREE_UV = { 0.0f, 0.66406f, 0.31641f, 0.79102f };

// Xbox glyphs â€” top row of mat_comon_x360_001 (same atlas/layout as options/world_map).
const UV GLYPH_A  = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B  = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };
const UV GLYPH_LB = { 0.32617f, 0.00781f, 0.46094f, 0.07812f };
const UV GLYPH_RB = { 0.48242f, 0.00781f, 0.61523f, 0.07812f };

// digit cells in mat_media_num_001 â€” 11 even cells across (0..9 then '/').
// glyphs occupy ~the full 32px height; cell width ~ 1/11 of the strip.
UV NumUV(int d) {            // d in 0..9
    const float cw = 1.0f / 11.0f;
    float u0 = d * cw + 0.004f, u1 = (d + 1) * cw - 0.004f;
    return { u0, 0.031f, u1, 0.969f };
}

// worldmap flag cells (3x3 grid in mat_worldmap_ss_002), reused as art thumbnails.
const UV THUMB_UV[9] = {
    { 0.55664f, 0.00195f, 0.83008f, 0.18359f },  // top-right cell (anchor flag)
    { 0.00195f, 0.00195f, 0.27539f, 0.18359f },  // top-left (crest)
    { 0.27930f, 0.18750f, 0.55273f, 0.36914f },  // mid (eagle)
    { 0.27930f, 0.37305f, 0.55273f, 0.55469f },  // bottom-mid (baobab)
    { 0.00195f, 0.18750f, 0.27539f, 0.36914f },  // mid-left (sun)
    { 0.27930f, 0.00195f, 0.55273f, 0.18359f },  // top-mid (gear)
    { 0.55664f, 0.18750f, 0.83008f, 0.36914f },  // mid-right (bands)
    { 0.00195f, 0.37305f, 0.27539f, 0.55469f },  // bottom-left (US stripes)
    { 0.55664f, 0.37305f, 0.83008f, 0.55469f },  // bottom-right (skull)
};

// ---- layout (reference px) --------------------------------------------------
constexpr float TITLE_X = 150.0f, TITLE_Y = 50.0f, RULE_Y = 118.0f;

constexpr float LIST_X = 150.0f, LIST_Y = 150.0f, LIST_W = 600.0f, LIST_H = 410.0f;
constexpr float INFO_X = 780.0f, INFO_Y = 150.0f, INFO_W = 350.0f, INFO_H = 410.0f;
constexpr float HEADER_H = 52.0f;          // caption strip atop each window
constexpr float ROW_H    = 58.0f;
constexpr int   VISIBLE_ROWS = 6;          // up to 9 entries -> the list scrolls
constexpr float ROW_PAD  = 12.0f;

// preview thumbnail plate inside the info window (ART category)
constexpr float THUMB_X = 798.0f, THUMB_Y = 218.0f, THUMB_W = 314.0f, THUMB_H = 168.0f;

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double TITLE_FRAMES = 14.0;
constexpr double LIST_FRAMES  = 16.0;
constexpr double INFO_OFFSET  = 4.0,  INFO_FRAMES = 14.0;
constexpr double FOOT_OFFSET  = 10.0, FOOT_FRAMES = 12.0;
constexpr double SELECT_MOVE_FRAMES = 8.0;

// ---- palette (shared with pause/result/shop/status/options/world_map) -------
const uint32_t COL_BG_TOP   = RGBA(12, 20, 38, 255);
const uint32_t COL_BG_BOT   = RGBA(5, 9, 18, 255);
const uint32_t COL_SEL_TOP  = RGBA(64, 150, 235, 225);
const uint32_t COL_SEL_BOT  = RGBA(28, 92, 180, 225);
const uint32_t COL_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t COL_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t COL_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t COL_DESC     = RGBA(178, 194, 214, 255);
const uint32_t COL_RULE     = RGBA(120, 170, 230, 90);
const uint32_t COL_PLATE    = RGBA(6, 10, 20, 235);     // dark plate behind a thumbnail
const uint32_t COL_TIME     = RGBA(255, 209, 74, 255);   // duration / number (gold)
const uint32_t COL_NOWPLAY  = RGBA(120, 230, 140, 255);  // now-playing accent
const uint32_t COL_OK       = RGBA(120, 230, 140, 255);
const uint32_t COL_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t COL_TAB_ON   = RGBA(255, 209, 74, 255);
const uint32_t COL_TAB_OFF  = RGBA(120, 134, 158, 255);
const uint32_t COL_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t COL_SEG_TOP  = RGBA(120, 200, 255, 255);  // progress bar fill
const uint32_t COL_SEG_BOT  = RGBA(40, 120, 210, 255);
const uint32_t COL_SEG_EMPTY= RGBA(36, 46, 64, 220);

// ---- interactive state ------------------------------------------------------
int    g_cat = 0;                  // selected category (tab)
int    g_sel = 0, g_prevSel = 0;   // selected entry within the category
int    g_scroll = 0;
double g_moveStart = -100.0;        // Now() when the highlight last moved
double g_msgStart  = -100.0;        // Now() of the last play/view flash
int    g_nowCat = -1, g_nowIdx = -1;// latched now-playing MUSIC track
double g_nowStart = -100.0;          // Now() the latched track started (for the elapsed clock)

const Category& Cat()  { return CATEGORIES[g_cat]; }
int   EntryCount()     { return Cat().count; }
const Entry& Sel()     { return Cat().entries[std::clamp(g_sel, 0, EntryCount() - 1)]; }

float ScrollLimit() { return (float)std::max(0, EntryCount() - VISIBLE_ROWS); }

void ClampScrollToSel() {
    if (g_sel < g_scroll) g_scroll = g_sel;
    if (g_sel > g_scroll + VISIBLE_ROWS - 1) g_scroll = g_sel - VISIBLE_ROWS + 1;
    g_scroll = std::clamp(g_scroll, 0, (int)ScrollLimit());
}

// seconds -> "M:SS"
void TimeStr(int sec, char* out, int n) {
    if (sec < 0) { std::snprintf(out, n, "--:--"); return; }
    std::snprintf(out, n, "%d:%02d", sec / 60, sec % 60);
}

void Init() {
    GameFrameTex();   // lazy-load the shared chrome frame used by DrawGameWindow
    if (g_ornTex   < 0) g_ornTex   = gfx::loadTexture(std::string(ASSET_BASE) + "mat_media_common_001.png");
    if (g_numTex   < 0) g_numTex   = gfx::loadTexture(std::string(ASSET_BASE) + "mat_media_num_001.png");
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_x360_001.png");
    if (g_thumbTex < 0) g_thumbTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_worldmap_ss_002.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}

void Reset() {
    g_cat = 0; g_sel = 0; g_prevSel = 0; g_scroll = 0;
    g_moveStart = -100.0; g_msgStart = -100.0;
    g_nowCat = -1; g_nowIdx = -1; g_nowStart = -100.0;
}

void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = std::max(0, g_sel - 1);
        if (in.down) g_sel = std::min(EntryCount() - 1, g_sel + 1);
        ClampScrollToSel();
        g_moveStart = Now();
    }
    if (in.tabLeft || in.tabRight) {
        if (in.tabLeft)  g_cat = (g_cat + CAT_COUNT - 1) % CAT_COUNT;
        if (in.tabRight) g_cat = (g_cat + 1) % CAT_COUNT;
        g_sel = 0; g_prevSel = 0; g_scroll = 0;   // reset the entry cursor for the new tab
        g_moveStart = Now();
    }
    if (in.accept) {                               // play / view the selected entry
        g_msgStart = Now();
        if (Cat().kind == CAT_MUSIC) {             // latch a now-playing track
            g_nowCat = g_cat; g_nowIdx = g_sel; g_nowStart = Now();
        }
    }
    // cancel: would back out to the menu in-game; no-op in the standalone build.
}

// ---- aspect-preserving image fit into a box (world_map idiom) ---------------
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

// gold track-number from the real digit strip (mat_media_num_001), right of `x`.
// Returns the x just past the drawn digits. Falls back to atlas-less no-op.
float DrawNum(int value, float x, float cy, float gh, float t) {
    if (g_numTex < 0 || t <= 0.0f) return x;
    char buf[8]; std::snprintf(buf, sizeof(buf), "%02d", std::clamp(value, 0, 99));
    float digW = gh * ((1.0f / 11.0f - 0.008f) * NUM_TEX_W) / (0.938f * NUM_TEX_H);
    for (const char* p = buf; *p; ++p) {
        if (*p < '0' || *p > '9') continue;
        UV u = NumUV(*p - '0');
        DrawImage(g_numTex, { x, cy - gh * 0.5f }, { x + digW, cy + gh * 0.5f },
                  { u.u0, u.v0 }, { u.u1, u.v1 }, WithAlpha(COL_WHITE, t));
        x += digW + 1.0f;
    }
    return x;
}

// one footer hint: real button glyph + ASCII label, left-to-right (world_map idiom).
float DrawHint(float x, float cy, const UV& g, float gAspect, const char* label, float t) {
    const float gh = 30.0f;
    if (g_glyphTex >= 0) {
        float gw = gh * gAspect;
        DrawImage(g_glyphTex, { x, cy - gh * 0.5f }, { x + gw, cy + gh * 0.5f },
                  { g.u0, g.v0 }, { g.u1, g.v1 }, WithAlpha(COL_WHITE, t));
        x += gw + 8.0f;
    }
    SetFont(g_fRodin);
    DrawText({ x, cy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, t), label);
    x += MeasureText(22.0f, label).x + 34.0f;
    return x;
}

// a slim progress bar (filled fraction f in [0,1]).
void DrawProgress(float x, float y, float w, float h, float f, float t) {
    DrawRect({ x, y }, { x + w, y + h }, WithAlpha(COL_SEG_EMPTY, t));
    float fw = w * std::clamp(f, 0.0f, 1.0f);
    if (fw > 1.0f)
        DrawVGradient({ x, y }, { x + fw, y + h }, WithAlpha(COL_SEG_TOP, t), WithAlpha(COL_SEG_BOT, t));
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const Category& c = Cat();
    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float listT  = (float)ComputeMotion(openSec, 0.0, LIST_FRAMES);
    const float infoT  = (float)ComputeMotion(openSec, INFO_OFFSET, INFO_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ---- title + gold filigree ornament + category tabs ----
    SetFont(g_fDF);
    DrawTextShadow({ TITLE_X, TITLE_Y - (1.0f - titleT) * 16.0f }, 46.0f,
                   WithAlpha(COL_TITLE, titleT), "MEDIA ROOM");
    DrawRect({ TITLE_X, RULE_Y }, { 1130.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));

    // real gold filigree scroll tucked just past the title word
    if (g_ornTex >= 0 && titleT > 0.0f) {
        float aspect = ((FILIGREE_UV.u1 - FILIGREE_UV.u0) * ORN_TEX_W) /
                       ((FILIGREE_UV.v1 - FILIGREE_UV.v0) * ORN_TEX_H);
        float fh = 40.0f, fw = fh * aspect;
        float fx = TITLE_X + MeasureText(46.0f, "MEDIA ROOM").x + 24.0f;
        float fy = TITLE_Y + 8.0f - (1.0f - titleT) * 10.0f;
        DrawImage(g_ornTex, { fx, fy }, { fx + fw, fy + fh },
                  { FILIGREE_UV.u0, FILIGREE_UV.v0 }, { FILIGREE_UV.u1, FILIGREE_UV.v1 },
                  WithAlpha(COL_WHITE, titleT));
    }

    // tab row across the top-right: MUSIC | MOVIE | ART (LB/RB switch)
    {
        const float tabsR = 1130.0f, tabsTop = TITLE_Y + 8.0f, tabsBot = TITLE_Y + 44.0f;
        const float gap = 22.0f;
        SetFont(g_fDF);
        float widths[CAT_COUNT], total = 0.0f;
        for (int i = 0; i < CAT_COUNT; ++i) {
            widths[i] = MeasureText(24.0f, CATEGORIES[i].name).x;
            total += widths[i] + (i ? gap : 0.0f);
        }
        float x = tabsR - total;
        for (int i = 0; i < CAT_COUNT; ++i) {
            uint32_t col = (i == g_cat) ? COL_TAB_ON : COL_TAB_OFF;
            DrawTextAligned({ x, tabsTop }, { x + widths[i], tabsBot }, 24.0f,
                            WithAlpha(col, titleT), CATEGORIES[i].name, Align::Left, true, true);
            if (i == g_cat)
                DrawRect({ x, tabsBot - 2.0f }, { x + widths[i], tabsBot }, WithAlpha(COL_TAB_ON, titleT));
            x += widths[i] + gap;
        }
    }

    // ===== LEFT: ENTRY LIST WINDOW ===========================================
    const float listSlide = (1.0f - listT) * 24.0f;
    const float lx = LIST_X, ly = LIST_Y + listSlide;
    {
        char cap[24]; std::snprintf(cap, sizeof(cap), "%s LIST", c.name);
        DrawWindow(lx, ly, LIST_W, LIST_H, listT, cap);
    }

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

    // visible entry rows: index # + name (left) + duration/now-playing (right)
    for (int row = 0; row < VISIBLE_ROWS; ++row) {
        int idx = g_scroll + row;
        if (idx >= EntryCount()) break;
        const Entry& e = c.entries[idx];
        float top = rowsTop + row * ROW_H;
        float cy  = top + ROW_H * 0.5f;
        bool selected   = (idx == g_sel);
        bool nowPlaying = (g_nowCat == g_cat && g_nowIdx == idx);

        // gold track number from the real digit strip
        DrawNum(idx + 1, rowL + 14, cy, 26.0f, listT);

        SetFont(g_fSeurat);
        DrawTextAligned({ rowL + 70, top }, { rowR - 130, top + ROW_H }, 26.0f,
                        WithAlpha(selected ? COL_TEXT_SEL : COL_TEXT, listT),
                        e.name, Align::Left, true, true);

        SetFont(g_fRodin);
        if (nowPlaying) {
            DrawTextAligned({ rowR - 130, top }, { rowR - 10, top + ROW_H }, 20.0f,
                            WithAlpha(COL_NOWPLAY, listT), "PLAYING", Align::Right, true, true);
        } else if (e.seconds >= 0) {
            char tb[12]; TimeStr(e.seconds, tb, sizeof(tb));
            DrawTextAligned({ rowR - 110, top }, { rowR - 10, top + ROW_H }, 22.0f,
                            WithAlpha(selected ? COL_TIME : COL_DESC, listT),
                            tb, Align::Right, true, true);
        } else {
            DrawTextAligned({ rowR - 130, top }, { rowR - 10, top + ROW_H }, 20.0f,
                            WithAlpha(selected ? COL_TIME : COL_DESC, listT),
                            "VIEW", Align::Right, true, true);
        }
    }

    // scrollbar (only when the list overflows the window)
    if (EntryCount() > VISIBLE_ROWS && listT > 0.5f) {
        float trackX = lx + LIST_W - 7.0f, trackTop = rowsTop, trackH = VISIBLE_ROWS * ROW_H;
        DrawRect({ trackX, trackTop }, { trackX + 3, trackTop + trackH }, WithAlpha(COL_RULE, listT));
        float frac = (float)VISIBLE_ROWS / (float)EntryCount();
        float thumbH = trackH * frac;
        float denom = ScrollLimit(); if (denom < 1.0f) denom = 1.0f;
        float thumbY = trackTop + (trackH - thumbH) * ((float)g_scroll / denom);
        DrawRect({ trackX, thumbY }, { trackX + 3, thumbY + thumbH }, WithAlpha(COL_SEL_TOP, listT));
    }

    // ===== RIGHT: PREVIEW / INFO WINDOW ======================================
    const float ix = INFO_X, iy = INFO_Y + (1.0f - infoT) * 24.0f;
    DrawWindow(ix, iy, INFO_W, INFO_H, infoT, "PREVIEW");

    const Entry& sel = Sel();
    const float contentTop = iy + HEADER_H + 14.0f;

    if (c.kind == CAT_ART) {
        // dark plate + real gallery thumbnail (aspect-fit), inset under the header
        DrawRect({ THUMB_X, THUMB_Y }, { THUMB_X + THUMB_W, THUMB_Y + THUMB_H }, WithAlpha(COL_PLATE, infoT));
        if (sel.thumb >= 0 && sel.thumb < 9)
            DrawFitted(g_thumbTex, THUMB_UV[sel.thumb], THUMB_TEX_W, THUMB_TEX_H,
                       THUMB_X, THUMB_Y, THUMB_W, THUMB_H, infoT, COL_WHITE, 0.5f);
        DrawRect({ THUMB_X, THUMB_Y }, { THUMB_X + THUMB_W, THUMB_Y + 2 }, WithAlpha(COL_RULE, infoT));
        DrawRect({ THUMB_X, THUMB_Y + THUMB_H - 2 }, { THUMB_X + THUMB_W, THUMB_Y + THUMB_H }, WithAlpha(COL_RULE, infoT));

        float ty = THUMB_Y + THUMB_H + 18.0f;
        SetFont(g_fSeurat);
        DrawTextAligned({ ix + 20, ty }, { ix + INFO_W - 16, ty + 36 }, 28.0f,
                        WithAlpha(COL_TEXT_SEL, infoT), sel.name, Align::Left, true, true);
        DrawText({ ix + 22, ty + 42 }, 20.0f, WithAlpha(COL_DESC, infoT), sel.sub);
        char idxb[24]; std::snprintf(idxb, sizeof(idxb), "PIECE  %d / %d", g_sel + 1, EntryCount());
        SetFont(g_fRodin);
        DrawText({ ix + 22, iy + INFO_H - 56 }, 22.0f, WithAlpha(COL_TIME, infoT), idxb);
    } else {
        // MUSIC / MOVIE: real oval emblem + now-playing (music) or clip info (movie)
        if (g_ornTex >= 0 && infoT > 0.0f) {
            float aspect = ((EMBLEM_UV.u1 - EMBLEM_UV.u0) * ORN_TEX_W) /
                           ((EMBLEM_UV.v1 - EMBLEM_UV.v0) * ORN_TEX_H);
            float ew = INFO_W - 80.0f, eh = ew / aspect;
            float ex = ix + (INFO_W - ew) * 0.5f, ey = contentTop + 6.0f;
            DrawImage(g_ornTex, { ex, ey }, { ex + ew, ey + eh },
                      { EMBLEM_UV.u0, EMBLEM_UV.v0 }, { EMBLEM_UV.u1, EMBLEM_UV.v1 },
                      WithAlpha(COL_WHITE, infoT));
        }

        float ty = contentTop + 116.0f;
        const char* label = (c.kind == CAT_MUSIC) ? "TRACK" : "EVENT";
        SetFont(g_fRodin);
        DrawText({ ix + 22, ty }, 19.0f, WithAlpha(COL_DESC, infoT), label);
        SetFont(g_fSeurat);
        DrawTextAligned({ ix + 20, ty + 22 }, { ix + INFO_W - 16, ty + 60 }, 27.0f,
                        WithAlpha(COL_TEXT_SEL, infoT), sel.name, Align::Left, true, true);
        DrawText({ ix + 22, ty + 64 }, 20.0f, WithAlpha(COL_DESC, infoT), sel.sub);

        // duration line
        {
            char tb[12], line[24];
            TimeStr(sel.seconds, tb, sizeof(tb));
            std::snprintf(line, sizeof(line), "LENGTH  %s", tb);
            SetFont(g_fRodin);
            DrawText({ ix + 22, ty + 100 }, 22.0f, WithAlpha(COL_TIME, infoT), line);
        }

        // now-playing block (MUSIC only, when a track is latched): live elapsed
        // clock + a looping progress bar driven by ui::Now() (continuous anim).
        if (c.kind == CAT_MUSIC && g_nowCat == g_cat && g_nowIdx >= 0 && g_nowIdx < EntryCount()) {
            const Entry& np = c.entries[g_nowIdx];
            float by = ty + 142.0f;
            DrawRect({ ix + 18, by - 6 }, { ix + INFO_W - 18, by - 4 }, WithAlpha(COL_RULE, infoT));
            SetFont(g_fRodin);
            DrawText({ ix + 22, by }, 19.0f, WithAlpha(COL_NOWPLAY, infoT), "NOW PLAYING");
            SetFont(g_fSeurat);
            DrawTextAligned({ ix + 20, by + 22 }, { ix + INFO_W - 16, by + 56 }, 23.0f,
                            WithAlpha(COL_TEXT_SEL, infoT), np.name, Align::Left, true, true);

            // elapsed time wraps over the track length; progress bar tracks it.
            float dur = (np.seconds > 0) ? (float)np.seconds : 1.0f;
            float elapsed = (float)std::fmod(std::max(0.0, Now() - g_nowStart), (double)dur);
            float frac = elapsed / dur;
            char eb[12], db[12], line[28];
            TimeStr((int)elapsed, eb, sizeof(eb));
            TimeStr(np.seconds, db, sizeof(db));
            std::snprintf(line, sizeof(line), "%s / %s", eb, db);
            DrawProgress(ix + 22, by + 64, INFO_W - 44, 8.0f, frac, infoT);
            SetFont(g_fRodin);
            DrawTextAligned({ ix + 20, by + 76 }, { ix + INFO_W - 18, by + 100 }, 20.0f,
                            WithAlpha(COL_DESC, infoT), line, Align::Right, true, true);
        }
    }

    // ===== FOOTER (real glyph + label hints) =================================
    {
        float hx = TITLE_X, hcy = 632.0f;
        const float aAsp  = ((GLYPH_A.u1  - GLYPH_A.u0)  * GLYPH_TEX_W) / ((GLYPH_A.v1  - GLYPH_A.v0)  * GLYPH_TEX_H);
        const float bAsp  = ((GLYPH_B.u1  - GLYPH_B.u0)  * GLYPH_TEX_W) / ((GLYPH_B.v1  - GLYPH_B.v0)  * GLYPH_TEX_H);
        const float lrAsp = ((GLYPH_LB.u1 - GLYPH_LB.u0) * GLYPH_TEX_W) / ((GLYPH_LB.v1 - GLYPH_LB.v0) * GLYPH_TEX_H);

        // [Up/Down] is keyboard-only -> ASCII; pad actions use real glyphs.
        SetFont(g_fRodin);
        DrawText({ hx, hcy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "[Up/Down] Select");
        hx += MeasureText(22.0f, "[Up/Down] Select").x + 34.0f;
        const char* aLabel = (c.kind == CAT_MUSIC) ? "Play" : (c.kind == CAT_MOVIE ? "Watch" : "View");
        hx = DrawHint(hx, hcy, GLYPH_A,  aAsp,  aLabel,       footT);
        hx = DrawHint(hx, hcy, GLYPH_B,  bAsp,  "Back",       footT);
        hx = DrawHint(hx, hcy, GLYPH_LB, lrAsp, "",           footT);
        hx = DrawHint(hx, hcy, GLYPH_RB, lrAsp, "Category",   footT);
    }

    // ===== transient play / view flash =======================================
    double age = Now() - g_msgStart;
    if (g_msgStart > 0.0 && age < 1.4) {
        float ma = std::min(1.0f, (float)((1.4 - age) / 0.4));
        char line[80];
        const char* verb = (c.kind == CAT_MUSIC) ? "Now playing" :
                           (c.kind == CAT_MOVIE) ? "Playing event" : "Viewing";
        std::snprintf(line, sizeof(line), "%s: %s", verb, sel.name);
        SetFont(g_fSeurat);
        DrawTextAligned({ LIST_X, 570 }, { 1130, 600 }, 24.0f,
                        WithAlpha(COL_OK, ma), line, Align::Center, true, true);
    }

    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void MediaRoomInit() { Init(); }
void MediaRoomDraw(double openSeconds) { Draw(openSeconds); }
void MediaRoomInput(const ScreenInput& in) { Input(in); }
void MediaRoomReset() { Reset(); }
