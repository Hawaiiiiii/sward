// =============================================================================
// screen_mission_screen.cpp â€” the in-mission objective HUD overlay, re-authored
// as clean hand-written C++ in the UnleashedRecomp ui/options_menu idiom (NOT a
// CSD node dump â€” the raw mission_screen.json smears mat_playscreen_001 banner
// 9-slices from x=-130 to x=+175 and a mat_hit_001 rule from x=-1 to x=+1691).
//
// This is a TRANSPARENT gameplay overlay, not a full chrome panel: a compact
// objective cluster pinned to the top-left, plus a centred "press to start"
// style mission banner that fades. The distinctive retail art is the REAL
// extracted atlas, each placed at a sane aspect-preserved rect:
//   * the mission TARGET icon â€” the gold star crate or blue wisp from
//     mat_mission_001 (128x64, two 64px cells, tight opaque boxes measured),
//   * the live PROGRESS readout "80 / 100" drawn glyph-by-glyph from the real
//     HUD number font mat_comon_num_001 (512x64: top row 0-9 ':' , bottom row
//     0-9 ':' '/' '[' ']'), so the count reads as retail rather than DFHeiStd,
//   * a spinning gold RING icon (mat_comon_002, first frame) when the objective
//     is a ring count, or the Sonic head (mat_comon_001) on the timer line,
//   * the objective LABEL wordmark from mat_playscreen_en_001 (128x128, an 8-row
//     stack: RING/ENERGY/SPEED/RINGS/TIME/SCORE/COUNT/LAP TIME) when present.
//
// Lightly interactive like the recomp HUD study screens:
//   * (A)   collects one unit toward the objective (the readout ticks up live),
//   * Left/Right OR Q/E (LB/RB) cycle the objective (RINGS -> ENEMIES -> TIME),
//   * a continuous ui::Now() pulse drives the ring spin, the bar shimmer and the
//     "COMPLETE!" flash when the goal is reached.
//
// Real art where a distinctive element exists; ASCII DFHeiStd text + bounded
// rects are the graceful fallback when an atlas is missing (-1 slot), exactly as
// the gate/status/world_map fallbacks do.
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

// ---- real-art atlases -------------------------------------------------------
const char* const ASSET_BASE = "assets/mission_screen/";
int g_iconTex  = -1;   // mat_mission_001        â€” star crate + blue wisp target icons (128x64)
int g_numTex   = -1;   // mat_comon_num_001      â€” HUD number font                     (512x64)
int g_ringTex  = -1;   // mat_comon_002          â€” gold RING spin strip                (1024x128)
int g_headTex  = -1;   // mat_comon_001          â€” Sonic / Werehog head icons          (256x128)
int g_labelTex = -1;   // mat_playscreen_en_001  â€” gold HUD label wordmark stack       (128x128)

// ---- real game fonts (MSDF sweep) -------------------------------------------
static int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// ---- atlas pixel sizes (for aspect-correct fitting / UV math) ---------------
constexpr float ICON_TEX_W  = 128.0f,  ICON_TEX_H  = 64.0f;
constexpr float NUM_TEX_W   = 512.0f,  NUM_TEX_H   = 64.0f;
constexpr float RING_TEX_W  = 1024.0f, RING_TEX_H  = 128.0f;
constexpr float HEAD_TEX_W  = 256.0f,  HEAD_TEX_H  = 128.0f;
constexpr float LABEL_TEX_W = 128.0f,  LABEL_TEX_H = 128.0f;

// ---- mission target icons (mat_mission_001) â€” tight opaque boxes measured ----
// left cell  px (4,3)-(61,60)   = gold star crate ; right cell px (81,1)-(115,63) = blue wisp.
const UV ICON_CRATE = { 4.0f / ICON_TEX_W,  3.0f / ICON_TEX_H,  61.0f / ICON_TEX_W,  60.0f / ICON_TEX_H };
const UV ICON_WISP  = { 81.0f / ICON_TEX_W, 1.0f / ICON_TEX_H, 115.0f / ICON_TEX_W,  63.0f / ICON_TEX_H };

// ---- gold RING first frame (mat_comon_002) â€” measured px (1,1)-(64,64) -------
const UV RING_F0 = { 1.0f / RING_TEX_W, 1.0f / RING_TEX_H, 64.0f / RING_TEX_W, 64.0f / RING_TEX_H };

// ---- Sonic head (mat_comon_001 left) â€” measured px (7,12)-(123,113) ----------
const UV HEAD_SONIC = { 7.0f / HEAD_TEX_W, 12.0f / HEAD_TEX_H, 123.0f / HEAD_TEX_W, 113.0f / HEAD_TEX_H };

// ---- HUD label wordmarks (mat_playscreen_en_001) â€” measured row bands --------
// 8-row stack: 0 RING, 1 ENERGY, 2 SPEED, 3 RINGS, 4 TIME, 5 SCORE, 6 COUNT, 7 LAP TIME.
// (x extent / y band measured from the alpha channel; v* normalized over 128px.)
const UV LBL_RINGS = { 1.0f / LABEL_TEX_W, 49.0f / LABEL_TEX_H, 55.0f / LABEL_TEX_W, 58.0f / LABEL_TEX_H };
const UV LBL_TIME  = { 1.0f / LABEL_TEX_W, 61.0f / LABEL_TEX_H, 43.0f / LABEL_TEX_W, 70.0f / LABEL_TEX_H };
const UV LBL_COUNT = { 1.0f / LABEL_TEX_W, 85.0f / LABEL_TEX_H, 60.0f / LABEL_TEX_W, 94.0f / LABEL_TEX_H };

// ---- HUD number font (mat_comon_num_001) ------------------------------------
// Top row (y 1..32 of 64) holds 0-9 and ':' ; the bottom row (y 32..63) adds '/'.
// Per-glyph tight x-bands measured from the alpha channel â€” drawn proportionally
// so the count keeps the real font's kerning. v spans are normalized over 64px.
struct Glyph { float x0, x1; float v0, v1; };   // px x-band + normalized v-band
constexpr float NUM_TOP_V0 = 1.0f  / NUM_TEX_H, NUM_TOP_V1 = 32.0f / NUM_TEX_H;
constexpr float NUM_BOT_V0 = 32.0f / NUM_TEX_H, NUM_BOT_V1 = 63.0f / NUM_TEX_H;
// digits 0-9 from the TOP row (cleaner outline), ':' top row, '/' from the BOTTOM row.
const Glyph DIGIT[10] = {
    {   1.0f,  27.0f, NUM_TOP_V0, NUM_TOP_V1 },  // 0
    {  35.0f,  51.0f, NUM_TOP_V0, NUM_TOP_V1 },  // 1
    {  59.0f,  84.0f, NUM_TOP_V0, NUM_TOP_V1 },  // 2
    {  88.0f, 113.0f, NUM_TOP_V0, NUM_TOP_V1 },  // 3
    { 116.0f, 143.0f, NUM_TOP_V0, NUM_TOP_V1 },  // 4
    { 145.0f, 171.0f, NUM_TOP_V0, NUM_TOP_V1 },  // 5
    { 174.0f, 200.0f, NUM_TOP_V0, NUM_TOP_V1 },  // 6
    { 203.0f, 228.0f, NUM_TOP_V0, NUM_TOP_V1 },  // 7
    { 232.0f, 257.0f, NUM_TOP_V0, NUM_TOP_V1 },  // 8
    { 260.0f, 288.0f, NUM_TOP_V0, NUM_TOP_V1 },  // 9
};
const Glyph GLYPH_COLON = { 295.0f, 308.0f, NUM_TOP_V0, NUM_TOP_V1 };   // ':' (top row)
const Glyph GLYPH_SLASH = { 252.0f, 271.0f, NUM_BOT_V0, NUM_BOT_V1 };   // '/' (bottom row)

// ---- objective model --------------------------------------------------------
enum IconKind { IK_CRATE, IK_WISP, IK_RING, IK_SONIC };
struct Mission {
    const char* nameAscii;    // headline objective (DFHeiStd, always drawn)
    const char* unitAscii;    // fallback unit label if the wordmark atlas is missing
    const UV*   unitWordmark; // real HUD label wordmark (nullptr -> ASCII unit)
    IconKind    icon;         // which real target icon leads the cluster
    bool        isTimer;      // true -> readout is mm:ss counting down, no '/ goal'
    int         goal;         // target count (or seconds for a timer)
};
const Mission MISSIONS[] = {
    { "COLLECT RINGS",   "RINGS", &LBL_RINGS, IK_RING,  false, 100 },
    { "DEFEAT ENEMIES",  "COUNT", &LBL_COUNT, IK_CRATE, false,  20 },
    { "REACH THE GOAL",  "TIME",  &LBL_TIME,  IK_SONIC, true,  120 },
};
constexpr int MISSION_COUNT = int(sizeof(MISSIONS) / sizeof(MISSIONS[0]));

// ---- layout (reference px) â€” pinned to the top-left HUD corner --------------
constexpr float HUD_X    = 64.0f;    // left margin of the objective cluster
constexpr float HUD_Y    = 56.0f;    // top of the icon
constexpr float ICON_SZ  = 56.0f;    // target icon box
constexpr float NUM_H    = 44.0f;    // progress digit height (on-screen px)
constexpr float BAR_W    = 300.0f, BAR_H = 12.0f;   // progress bar under the readout

// ---- entrance / continuous-anim tuning --------------------------------------
constexpr double HUD_FRAMES    = 14.0;   // objective cluster slide/fade-in
constexpr double NUM_OFFSET    = 3.0,  NUM_FRAMES   = 14.0;
constexpr double BANNER_FRAMES = 16.0;   // centred mission banner fade-in
constexpr double SWITCH_FRAMES = 10.0;   // objective swap slide

// ---- palette (shared with pause/result/gate/world_map) ----------------------
const uint32_t COL_TITLE   = RGBA(255, 209, 74, 255);    // Unleashed gold
const uint32_t COL_TEXT    = RGBA(232, 240, 250, 255);
const uint32_t COL_DESC    = RGBA(190, 205, 225, 235);
const uint32_t COL_WHITE   = RGBA(255, 255, 255, 255);
const uint32_t COL_SHADOW  = RGBA(0, 0, 0, 235);
const uint32_t COL_BAR_BG  = RGBA(10, 16, 30, 170);      // track behind the progress bar
const uint32_t COL_BAR_TOP = RGBA(120, 200, 255, 255);   // fill (gradient)
const uint32_t COL_BAR_BOT = RGBA(40, 120, 210, 255);
const uint32_t COL_BAR_DONE_TOP = RGBA(255, 226, 120, 255);  // gold fill once complete
const uint32_t COL_BAR_DONE_BOT = RGBA(200, 150, 30, 255);
const uint32_t COL_DONE    = RGBA(255, 236, 150, 255);   // pulsing "COMPLETE!"
const uint32_t COL_FOOTER  = RGBA(190, 205, 225, 220);

// ---- interactive state ------------------------------------------------------
int    g_mission     = 0;
int    g_collected[MISSION_COUNT] = { 0, 0, 0 };
double g_switchStart  = -100.0;   // Now() when the objective last changed
double g_doneStart    = -100.0;   // Now() the current objective first completed
double g_open         = 0.0;      // openSec captured each Draw (continuous anim)

const Mission& Cur() { return MISSIONS[g_mission]; }
bool Complete(int m) { return g_collected[m] >= MISSIONS[m].goal; }

void Init() {
    if (g_iconTex  < 0) g_iconTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_mission_001.png");
    if (g_numTex   < 0) g_numTex   = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_num_001.png");
    if (g_ringTex  < 0) g_ringTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_002.png");
    if (g_headTex  < 0) g_headTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_001.png");
    if (g_labelTex < 0) g_labelTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_playscreen_en_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}

void Reset() {
    g_mission = 0;
    for (int i = 0; i < MISSION_COUNT; ++i) g_collected[i] = 0;
    g_switchStart = -100.0; g_doneStart = -100.0; g_open = 0.0;
}

void SwitchMission(int dir) {
    g_mission = (g_mission + MISSION_COUNT + dir) % MISSION_COUNT;
    g_switchStart = Now();
}

void Input(const ScreenInput& in) {
    if (in.left  || in.tabLeft)  SwitchMission(-1);
    if (in.right || in.tabRight) SwitchMission(+1);
    if (in.accept) {                                   // (A) collects one unit
        int m = g_mission;
        bool wasDone = Complete(m);
        if (g_collected[m] < MISSIONS[m].goal) g_collected[m] += MISSIONS[m].isTimer ? 5 : 1;
        if (!wasDone && Complete(m)) g_doneStart = Now();
    }
    // cancel: would back out to gameplay in-game; no-op in the standalone build.
}

// ---- aspect-preserving image fit into a box (top/centre/bottom anchored) -----
void DrawFitted(int tex, const UV& uv, float texW, float texH,
                float bx, float by, float bw, float bh,
                float t, float vAnchor, uint32_t col, bool additive = false) {
    if (tex < 0 || t <= 0.0f) return;
    float artW = (uv.u1 - uv.u0) * texW;
    float artH = (uv.v1 - uv.v0) * texH;
    if (artW <= 0.0f || artH <= 0.0f) return;
    float scale = std::min(bw / artW, bh / artH);
    float w = artW * scale, h = artH * scale;
    float x = bx + (bw - w) * 0.5f;
    float y = by + (bh - h) * vAnchor;
    DrawImage(tex, { x, y }, { x + w, y + h }, { uv.u0, uv.v0 }, { uv.u1, uv.v1 },
              WithAlpha(col, t), additive);
}

// ---- one glyph from the HUD number font; returns advance (on-screen px) ------
float DrawNumGlyph(const Glyph& g, float x, float yTop, float h, float t) {
    float texGlyphW = g.x1 - g.x0;
    float texGlyphH = (g.v1 - g.v0) * NUM_TEX_H;
    float w = h * (texGlyphW / texGlyphH);              // preserve glyph aspect
    DrawImage(g_numTex, { x, yTop }, { x + w, yTop + h },
              { g.x0 / NUM_TEX_W, g.v0 }, { g.x1 / NUM_TEX_W, g.v1 },
              WithAlpha(COL_WHITE, t));
    return w;
}

// ---- draw a count string ("80 / 100", "01:23", ...) from the real number font.
// Returns the total drawn width so callers can right/centre-align. Falls back to
// DrawTextShadow (DFHeiStd) when the number atlas is missing.
float DrawNumString(const char* s, float x, float yTop, float h, float t) {
    if (g_numTex < 0) {
        SetFont(g_fRodin);
        DrawTextShadow({ x, yTop }, h, WithAlpha(COL_WHITE, t), s);
        return MeasureText(h, s).x;
    }
    const float space = h * 0.30f;        // gap for ' '
    const float kern  = h * 0.04f;         // tight inter-glyph gap
    float cx = x;
    for (const char* p = s; *p; ++p) {
        char c = *p;
        if (c == ' ') { cx += space; continue; }
        const Glyph* g = nullptr;
        if (c >= '0' && c <= '9') g = &DIGIT[c - '0'];
        else if (c == ':')        g = &GLYPH_COLON;
        else if (c == '/')        g = &GLYPH_SLASH;
        if (!g) { cx += space; continue; }
        cx += DrawNumGlyph(*g, cx, yTop, h, t) + kern;
    }
    return (cx - kern) - x;
}

void Draw(double openSec) {
    g_open = openSec;
    // NOTE: a HUD overlay â€” NO full-screen fill / chrome panel. Transparent over gameplay.

    const Mission& mi = Cur();
    const int m = g_mission;
    const float hudT    = (float)ComputeMotion(openSec, 0.0, HUD_FRAMES);
    const float numT    = (float)ComputeMotion(openSec, NUM_OFFSET, NUM_FRAMES);
    const float bannerT = (float)ComputeMotion(openSec, 0.0, BANNER_FRAMES);
    const float switchT = (float)ComputeMotion(g_switchStart, 0.0, SWITCH_FRAMES);
    const float swSlide = (1.0f - switchT) * 22.0f;
    const float swFade  = 0.35f + 0.65f * switchT;

    const bool done   = Complete(m);
    const float frac  = std::min(1.0f, (float)g_collected[m] / (float)mi.goal);
    const float pulse = 0.55f + 0.45f * (float)(0.5 + 0.5 * std::sin(g_open * 5.0));

    // ===== TOP-LEFT OBJECTIVE CLUSTER ========================================
    const float clx = HUD_X - (1.0f - hudT) * 28.0f;   // slide in from the left
    float cury = HUD_Y;

    // --- target icon (real art) leading the cluster ---
    const float ix = clx, iy = cury;
    if (mi.icon == IK_RING && g_ringTex >= 0) {
        // gentle additive shimmer on the spinning ring
        DrawFitted(g_ringTex, RING_F0, RING_TEX_W, RING_TEX_H, ix, iy, ICON_SZ, ICON_SZ,
                   hudT, 0.5f, COL_WHITE);
        DrawFitted(g_ringTex, RING_F0, RING_TEX_W, RING_TEX_H, ix, iy, ICON_SZ, ICON_SZ,
                   hudT * (0.25f * pulse), 0.5f, COL_WHITE, /*additive*/ true);
    } else if (mi.icon == IK_SONIC && g_headTex >= 0) {
        DrawFitted(g_headTex, HEAD_SONIC, HEAD_TEX_W, HEAD_TEX_H, ix, iy, ICON_SZ, ICON_SZ,
                   hudT, 0.5f, COL_WHITE);
    } else if (g_iconTex >= 0) {
        const UV& u = (mi.icon == IK_WISP) ? ICON_WISP : ICON_CRATE;
        DrawFitted(g_iconTex, u, ICON_TEX_W, ICON_TEX_H, ix, iy, ICON_SZ, ICON_SZ,
                   hudT, 0.5f, COL_WHITE);
    }

    // --- objective headline (DFHeiStd, gold, shadowed) to the right of the icon ---
    const float textX = clx + ICON_SZ + 16.0f;
    SetFont(g_fDF);
    DrawTextShadow({ textX, cury - 2.0f }, 30.0f, WithAlpha(COL_TITLE, hudT), mi.nameAscii, 2.0f, COL_SHADOW);

    // --- unit label wordmark (real art) just under the headline ---
    const float lblY = cury + 30.0f;
    if (g_labelTex >= 0 && mi.unitWordmark) {
        // place at native-ish height (~18px), left-anchored
        DrawFitted(g_labelTex, *mi.unitWordmark, LABEL_TEX_W, LABEL_TEX_H,
                   textX, lblY, 120.0f, 18.0f, hudT, 0.0f, COL_DESC);
    } else {
        SetFont(g_fSeurat);
        DrawTextShadow({ textX, lblY }, 18.0f, WithAlpha(COL_DESC, hudT), mi.unitAscii, 1.5f, COL_SHADOW);
    }

    // ===== PROGRESS READOUT (real number font) ===============================
    // build "80 / 100" (counts) or "mm:ss" (timer, counting down from the goal).
    char readout[24];
    if (mi.isTimer) {
        int remain = mi.goal - g_collected[m];
        if (remain < 0) remain = 0;
        std::snprintf(readout, sizeof(readout), "%d:%02d", remain / 60, remain % 60);
    } else {
        std::snprintf(readout, sizeof(readout), "%d / %d", g_collected[m], mi.goal);
    }
    const float readY = lblY + 26.0f + swSlide;
    DrawNumString(readout, textX, readY, NUM_H, numT * swFade);

    // ===== PROGRESS BAR (procedural, bounded; not for timers) ================
    if (!mi.isTimer) {
        const float barX = textX, barY = readY + NUM_H + 10.0f;
        const float barRight = barX + BAR_W;
        DrawRect({ barX, barY }, { barRight, barY + BAR_H }, WithAlpha(COL_BAR_BG, hudT));
        float fillW = BAR_W * frac;
        if (fillW > 1.0f) {
            uint32_t top = done ? COL_BAR_DONE_TOP : COL_BAR_TOP;
            uint32_t bot = done ? COL_BAR_DONE_BOT : COL_BAR_BOT;
            DrawVGradient({ barX, barY }, { barX + fillW, barY + BAR_H },
                          WithAlpha(top, hudT), WithAlpha(bot, hudT));
            // moving shimmer highlight along the fill
            float sh = barX + fillW * (0.5f + 0.5f * (float)std::sin(g_open * 3.0));
            DrawRect({ sh - 2.0f, barY }, { sh + 2.0f, barY + BAR_H },
                     WithAlpha(RGBA(255, 255, 255, 90), hudT), /*additive*/ true);
        }
        // thin top rule on the track for a framed read
        DrawRect({ barX, barY }, { barRight, barY + 1.5f }, WithAlpha(RGBA(150, 180, 220, 110), hudT));
    }

    // ===== CENTRED MISSION BANNER (fades after the entrance) =================
    // "MISSION" caption + objective name, mid-screen â€” fades out so it doesn't
    // obscure gameplay (like the in-game start-of-mission card).
    {
        double age = g_open;                          // seconds since the screen opened
        float bannerFade = 1.0f;
        if (age > 2.4) bannerFade = std::max(0.0f, (float)((3.4 - age) / 1.0));
        float a = bannerT * bannerFade;
        if (a > 0.01f) {
            SetFont(g_fRodin);
            DrawTextAligned({ 0, 250 }, { REF_W, 296 }, 26.0f,
                            WithAlpha(COL_DESC, a), "MISSION", Align::Center, true, true);
            SetFont(g_fDF);
            V2 nameSz = MeasureText(56.0f, mi.nameAscii);
            DrawTextShadow({ (REF_W - nameSz.x) * 0.5f, 304.0f }, 56.0f,
                           WithAlpha(COL_TITLE, a), mi.nameAscii, 3.0f, COL_SHADOW);
        }
    }

    // ===== "COMPLETE!" flash on reaching the goal ============================
    if (done) {
        double age = Now() - g_doneStart;
        float flash = (g_doneStart > 0.0 && age < 1.6)
                      ? std::min(1.0f, (float)((1.6 - age) / 0.4)) : 0.0f;
        float a = std::max(flash, 0.55f * pulse);     // keep a soft steady glow once done
        SetFont(g_fSeurat);
        DrawTextShadow({ textX, readY + NUM_H + 28.0f }, 24.0f,
                       WithAlpha(COL_DONE, std::min(1.0f, a)), "OBJECTIVE COMPLETE!", 2.0f, COL_SHADOW);
    }

    // ===== FOOTER PROMPT (bottom, DFHeiStd) ==================================
    float footT = (float)ComputeMotion(openSec, 10.0, 12.0);
    SetFont(g_fRodin);
    DrawTextShadow({ HUD_X, 666.0f }, 22.0f, WithAlpha(COL_FOOTER, footT),
                   "(A) Collect    (Q/E) Switch Objective    (B) Back", 2.0f, COL_SHADOW);
    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void MissionScreenInit() { Init(); }
void MissionScreenDraw(double openSeconds) { Draw(openSeconds); }
void MissionScreenInput(const ScreenInput& in) { Input(in); }
void MissionScreenReset() { Reset(); }
