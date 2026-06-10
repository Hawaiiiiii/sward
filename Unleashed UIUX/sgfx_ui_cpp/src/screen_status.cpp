// =============================================================================
// screen_status.cpp — the Status / abilities screen, re-authored as clean
// hand-written C++ in the UnleashedRecomp ui/options_menu idiom (NOT a CSD node
// dump — the old transcription smeared unclipped 9-slice stretch-arms across the
// screen). Layout lives in named 1280x720 constants; the stat panel is a bounded
// gradient window; gauges are drawn procedurally. The distinctive game art that
// makes this read as retail — the Werehog / Sonic hero portrait and the matching
// "SONIC THE WEREHOG" / "...HEDGEHOG" wordmark — is the REAL extracted atlas
// (mat_comon_005, mat_status_common_001), placed at deliberate sane rects.
//
// Fully interactive + stateful like options_menu:
//   * Q/E (LB/RB) switch the DAY (Sonic) / NIGHT (Werehog) form -> portrait, logo,
//     stat set and EXP pool all change,
//   * Up/Down move the cursor over the ability rows (eased highlight),
//   * (A) spends EXP to level the selected ability up (gauge fills live),
//   * a live LV / EXP readout.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

using namespace ui;

namespace {

// ---- forms ------------------------------------------------------------------
enum Form { NIGHT = 0, DAY = 1, FORM_COUNT };   // NIGHT (Werehog) is the iconic status look
constexpr int STAT_COUNT = 4;
constexpr int MAX_LV = 10;

struct FormData {
    const char* name;
    const char* stats[STAT_COUNT];
};
const FormData FORMS[FORM_COUNT] = {
    { "WEREHOG", { "LIFE", "STRENGTH", "UNLEASH", "SHIELD" } },
    { "SONIC",   { "LIFE", "RING ENERGY", "SPEED", "BOOST" } },
};

// ---- real-art atlases -------------------------------------------------------
// mat_comon_005 is 1024x512 (Werehog portrait left, day-Sonic right);
// mat_status_common_001 is 256x256 (HEDGEHOG wordmark top, WEREHOG wordmark below).
const char* const ASSET_BASE = "assets/status/";
int g_portraitTex = -1, g_logoTex = -1;

// ---- fonts --------------------------------------------------------------
static int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

struct UV { float u0, v0, u1, v1; };
// tight per-element opaque boxes measured from the atlas alpha channel
// (the atlas also holds a white chevron at u>0.71 — excluded)
const UV PORTRAIT_UV[FORM_COUNT] = {
    /* NIGHT werehog */ { 0.002f, 0.012f, 0.400f, 0.611f },
    /* DAY   sonic   */ { 0.410f, 0.004f, 0.696f, 0.654f },
};
const UV LOGO_UV[FORM_COUNT] = {
    /* NIGHT werehog */ { 0.0f, 0.430f, 0.859f, 0.800f },
    /* DAY   hedgehog*/ { 0.0f, 0.000f, 0.859f, 0.367f },
};
// pixel aspect (w/h) of each sub-rect, so the portrait isn't stretched.
float PortraitAspect(Form f) {
    const UV& u = PORTRAIT_UV[f];
    return ((u.u1 - u.u0) * 1024.0f) / ((u.v1 - u.v0) * 512.0f);
}
float LogoAspect(Form f) {
    const UV& u = LOGO_UV[f];
    return ((u.u1 - u.u0) * 256.0f) / ((u.v1 - u.v0) * 256.0f);
}

// ---- layout (reference px) --------------------------------------------------
constexpr float TITLE_X = 150.0f, TITLE_Y = 50.0f, RULE_Y = 118.0f;
constexpr float PANEL_X = 150.0f, PANEL_Y = 175.0f, PANEL_W = 560.0f, PANEL_H = 400.0f;
constexpr float HEADER_H = 52.0f;
constexpr float ROW_TOP  = PANEL_Y + HEADER_H + 78.0f;   // first ability row (below LV/EXP line)
constexpr float ROW_H    = 64.0f;

// ---- entrance tuning (frames) -----------------------------------------------
constexpr double PANEL_FRAMES = 16.0, PORTRAIT_OFFSET = 3.0, PORTRAIT_FRAMES = 16.0;
constexpr double LOGO_OFFSET = 6.0, LOGO_FRAMES = 14.0, FOOT_OFFSET = 10.0, FOOT_FRAMES = 12.0;
constexpr double SELECT_MOVE_FRAMES = 8.0;

// ---- palette ----------------------------------------------------------------
const uint32_t COL_BG_TOP    = RGBA(14, 18, 30, 255);
const uint32_t COL_BG_BOT    = RGBA(5, 7, 13, 255);
const uint32_t COL_PANEL_TOP = RGBA(18, 30, 52, 232);
const uint32_t COL_PANEL_BOT = RGBA(8, 14, 26, 232);
const uint32_t COL_HEAD_TOP  = RGBA(28, 52, 92, 244);
const uint32_t COL_HEAD_BOT  = RGBA(16, 30, 56, 244);
const uint32_t COL_SEL_TOP   = RGBA(64, 150, 235, 220);
const uint32_t COL_SEL_BOT   = RGBA(28, 92, 180, 220);
const uint32_t COL_TITLE     = RGBA(255, 209, 74, 255);
const uint32_t COL_TEXT      = RGBA(214, 226, 240, 255);
const uint32_t COL_TEXT_SEL  = RGBA(255, 255, 255, 255);
const uint32_t COL_RULE      = RGBA(120, 170, 230, 90);
const uint32_t COL_SEG_TOP   = RGBA(120, 200, 255, 255);   // filled gauge segment
const uint32_t COL_SEG_BOT   = RGBA(40, 120, 210, 255);
const uint32_t COL_SEG_EMPTY = RGBA(36, 46, 64, 220);
const uint32_t COL_EXP       = RGBA(255, 209, 74, 255);
const uint32_t COL_FOOTER    = RGBA(190, 205, 225, 220);
const uint32_t COL_TAB_ON    = RGBA(255, 209, 74, 255);
const uint32_t COL_TAB_OFF   = RGBA(120, 134, 158, 255);

// ---- interactive state ------------------------------------------------------
int    g_form = NIGHT;
int    g_sel = 0, g_prevSel = 0;
double g_moveStart = -100.0;
int    g_lv[FORM_COUNT][STAT_COUNT]  = { { 7, 6, 5, 4 }, { 6, 5, 7, 8 } };
int    g_exp[FORM_COUNT]             = { 1500, 1200 };

void Commafy(int v, char* out, int n) {
    char raw[16]; std::snprintf(raw, sizeof(raw), "%d", v);
    int len = (int)std::strlen(raw), o = 0;
    for (int i = 0; i < len && o < n - 1; ++i) {
        if (i > 0 && (len - i) % 3 == 0 && o < n - 1) out[o++] = ',';
        out[o++] = raw[i];
    }
    out[o] = '\0';
}
int LevelCost(int lv) { return lv * 100; }            // cost to go from lv -> lv+1
int CharacterLevel(int f) {                            // overall LV = sum of ability levels
    int s = 0; for (int i = 0; i < STAT_COUNT; ++i) s += g_lv[f][i]; return s;
}

void Init() {
    if (g_portraitTex < 0) g_portraitTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_005.png");
    if (g_logoTex < 0)     g_logoTex     = gfx::loadTexture(std::string(ASSET_BASE) + "mat_status_common_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}

void Reset() { g_form = NIGHT; g_sel = 0; g_prevSel = 0; g_moveStart = -100.0;
               g_lv[0][0]=7; g_lv[0][1]=6; g_lv[0][2]=5; g_lv[0][3]=4;
               g_lv[1][0]=6; g_lv[1][1]=5; g_lv[1][2]=7; g_lv[1][3]=8;
               g_exp[0]=1500; g_exp[1]=1200; }

void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = (g_sel + STAT_COUNT - 1) % STAT_COUNT;
        else         g_sel = (g_sel + 1) % STAT_COUNT;
        g_moveStart = Now();
    }
    if (in.tabLeft || in.tabRight) { g_form ^= 1; }       // two forms: either tab toggles
    if (in.accept) {                                       // spend EXP to level up
        int lv = g_lv[g_form][g_sel];
        int cost = LevelCost(lv);
        if (lv < MAX_LV && g_exp[g_form] >= cost) { g_lv[g_form][g_sel] = lv + 1; g_exp[g_form] -= cost; }
    }
    // cancel: would back out in-game; no-op here.
}

// a bounded window from the REAL game frame (9-slice) with a header caption strip
void DrawWindow(float x, float y, float w, float h, float t, const char* caption) {
    DrawGameWindow({ x, y }, { x + w, y + h }, HEADER_H, t);
    DrawRect({ x + 12, y + HEADER_H - 2 }, { x + w - 12, y + HEADER_H }, WithAlpha(COL_RULE, t));
    if (caption) {
        SetFont(g_fDF);
        DrawTextAligned({ x + 18, y }, { x + w - 14, y + HEADER_H }, 26.0f,
                        WithAlpha(COL_TITLE, t), caption, Align::Left, true, true);
    }
}

// a 10-segment ability gauge; `level` segments are filled
void DrawGauge(float x, float y, float w, float h, int level, float t) {
    const float gap = 3.0f;
    const float segW = (w - gap * (MAX_LV - 1)) / MAX_LV;
    for (int i = 0; i < MAX_LV; ++i) {
        float sx = x + i * (segW + gap);
        if (i < level)
            DrawVGradient({ sx, y }, { sx + segW, y + h }, WithAlpha(COL_SEG_TOP, t), WithAlpha(COL_SEG_BOT, t));
        else
            DrawRect({ sx, y }, { sx + segW, y + h }, WithAlpha(COL_SEG_EMPTY, t));
    }
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const Form f = (Form)g_form;
    const float titleT = (float)ComputeMotion(openSec, 0.0, 14.0);
    const float panelT = (float)ComputeMotion(openSec, 0.0, PANEL_FRAMES);
    const float portT  = (float)ComputeMotion(openSec, PORTRAIT_OFFSET, PORTRAIT_FRAMES);
    const float logoT  = (float)ComputeMotion(openSec, LOGO_OFFSET, LOGO_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ---- title + form tabs ----
    SetFont(g_fDF);
    DrawTextBevel({ TITLE_X, TITLE_Y - (1.0f - titleT) * 16.0f }, 46.0f,
                  WithAlpha(COL_TITLE, titleT), "STATUS");
    DrawRect({ TITLE_X, RULE_Y }, { 1130.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));
    // tab indicator (top-right): NIGHT | DAY
    DrawTextAligned({ 740, TITLE_Y + 8 }, { 920, TITLE_Y + 44 }, 24.0f,
                    WithAlpha(f == NIGHT ? COL_TAB_ON : COL_TAB_OFF, titleT), "NIGHT", Align::Right, true, true);
    DrawTextAligned({ 928, TITLE_Y + 8 }, { 948, TITLE_Y + 44 }, 24.0f,
                    WithAlpha(COL_TAB_OFF, titleT), "/", Align::Center, true, true);
    DrawTextAligned({ 956, TITLE_Y + 8 }, { 1130, TITLE_Y + 44 }, 24.0f,
                    WithAlpha(f == DAY ? COL_TAB_ON : COL_TAB_OFF, titleT), "DAY", Align::Left, true, true);

    // ---- ability panel (left) ----
    const float px = PANEL_X, py = PANEL_Y + (1.0f - panelT) * 24.0f;
    DrawWindow(px, py, PANEL_W, PANEL_H, panelT, "ABILITIES");

    // LV / EXP readout line
    {
        char expbuf[16], line[48];
        Commafy(g_exp[f], expbuf, sizeof(expbuf));
        std::snprintf(line, sizeof(line), "LV  %d", CharacterLevel(f));
        SetFont(g_fRodin);
        DrawText({ px + 22, py + HEADER_H + 16 }, 26.0f, WithAlpha(COL_TEXT_SEL, panelT), line);
        std::snprintf(line, sizeof(line), "EXP  %s", expbuf);
        DrawTextAligned({ px + 200, py + HEADER_H + 12 }, { px + PANEL_W - 20, py + HEADER_H + 48 }, 26.0f,
                        WithAlpha(COL_EXP, panelT), line, Align::Right, true, true);
    }

    const float rowTop = py + HEADER_H + 78.0f;
    const float rowL = px + 16.0f, rowR = px + PANEL_W - 16.0f;

    // selection highlight (eased)
    if (panelT > 0.5f) {
        float moveT = (float)ComputeMotion(g_moveStart, 0.0, SELECT_MOVE_FRAMES);
        float slot = Lerp((float)g_prevSel, (float)g_sel, moveT);
        float hy = rowTop + slot * ROW_H;
        DrawVGradient({ rowL, hy + 2 }, { rowR, hy + ROW_H - 6 },
                      WithAlpha(COL_SEL_TOP, panelT), WithAlpha(COL_SEL_BOT, panelT));
    }

    // ability rows: label + 10-seg gauge + Lv N
    for (int i = 0; i < STAT_COUNT; ++i) {
        float top = rowTop + i * ROW_H;
        bool selected = (i == g_sel);
        SetFont(g_fSeurat);
        DrawText({ rowL + 16, top + 6 }, 24.0f,
                 WithAlpha(selected ? COL_TEXT_SEL : COL_TEXT, panelT), FORMS[f].stats[i]);
        DrawGauge(rowL + 200, top + 14, 250.0f, 22.0f, g_lv[f][i], panelT);
        char lvb[12]; std::snprintf(lvb, sizeof(lvb), "Lv %d", g_lv[f][i]);
        SetFont(g_fRodin);
        DrawTextAligned({ rowR - 70, top + 6 }, { rowR - 6, top + 34 }, 22.0f,
                        WithAlpha(selected ? COL_TEXT_SEL : COL_TEXT, panelT), lvb, Align::Right, true, true);
    }

    // ---- hero portrait (right, real art) — fit into a fixed box, bottom-anchored ----
    if (g_portraitTex >= 0 && portT > 0.0f) {
        const UV& u = PORTRAIT_UV[f];
        const float boxX = 728.0f, boxY = 232.0f, boxW = 452.0f, boxH = 392.0f;
        float artW = (u.u1 - u.u0) * 1024.0f, artH = (u.v1 - u.v0) * 512.0f;
        float scale = std::min(boxW / artW, boxH / artH);
        float pw = artW * scale, ph = artH * scale;
        float prx = boxX + (boxW - pw) * 0.5f;
        float pry = boxY + (boxH - ph) + (1.0f - portT) * 18.0f;   // bottom-anchored + slide
        DrawImage(g_portraitTex, { prx, pry }, { prx + pw, pry + ph },
                  { u.u0, u.v0 }, { u.u1, u.v1 }, WithAlpha(RGBA(255,255,255,255), portT));
    }

    // ---- form wordmark (real art, above the portrait) ----
    if (g_logoTex >= 0 && logoT > 0.0f) {
        const UV& u = LOGO_UV[f];
        float lw = 300.0f, lh = lw / LogoAspect(f);
        float lxp = 770.0f, lyp = 150.0f - (1.0f - logoT) * 12.0f;
        DrawImage(g_logoTex, { lxp, lyp }, { lxp + lw, lyp + lh },
                  { u.u0, u.v0 }, { u.u1, u.v1 }, WithAlpha(RGBA(255,255,255,255), logoT));
    }

    // ---- footer ----
    SetFont(g_fRodin);
    DrawTextAligned({ TITLE_X, 612 }, { 1130, 656 }, 22.0f, WithAlpha(COL_FOOTER, footT),
                    "[Up/Down] Select   (A) Level Up   (Q/E) Switch Form   (B) Back", Align::Left, true, true);
    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void StatusInit() { Init(); }
void StatusDraw(double openSeconds) { Draw(openSeconds); }
void StatusInput(const ScreenInput& in) { Input(in); }
void StatusReset() { Reset(); }
