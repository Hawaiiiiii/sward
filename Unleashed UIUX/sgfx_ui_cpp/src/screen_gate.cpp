// =============================================================================
// screen_gate.cpp â€” the Stage Gate / pre-stage briefing card, re-authored as clean
// hand-written C++ in the UnleashedRecomp ui/options_menu idiom (NOT a CSD node
// dump â€” the raw gate.json smears mat_result_comon_001 9-slice stretch-arms from
// x=-1993 to x=+3112). Layout lives in named 1280x720 constants; the two panels
// are bounded REAL Sonic Unleashed silver-chrome windows (ui::DrawGameWindow,
// 9-slice from mat_result_comon_001). The distinctive retail art is the REAL
// extracted atlases, each placed at a sane aspect-preserved rect:
//   * the gold "STAGE SELECT" wordmark      (mat_gate_en_002, 261x80 tight),
//   * the per-stage NAME + SUB-NAME wordmarks (mat_gate_en_001, a 9-row 2-col grid),
//   * the LARGE real stage screenshot        (mat_stage_ss, top-left + bottom-row cells),
//   * the gold RING / red SUN / blue MOON collectible icons (mat_comon_002/003/004,
//     first frame of each spin strip),
//   * the metal rank letter S/A/B/C/D/E      (mat_result_comon_002, the same 2x3 grid
//     world_map/result use),
//   * the Xbox A/B/LB/RB button glyphs       (mat_comon_x360_001).
//
// Fully interactive + stateful like options_menu / the shop / world_map:
//   * Left/Right OR Q/E (LB/RB) switch the ACT -> screenshot, name banner, objective,
//     HI-SCORE, medal counts and rank all change,
//   * (A) "STARTs" the highlighted act (a transient flash),
//   * (B) backs out (no-op in the standalone build),
//   * a staggered ease-in entrance: title -> shot panel -> info panel -> footer.
//
// Art is the real extracted atlas where a distinctive element exists; bounded
// chrome windows + ASCII text are the graceful fallback when an atlas is missing
// (-1 slot), exactly as the shop/status/world_map fallbacks do.
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
const char* const ASSET_BASE = "assets/gate/";
int g_titleTex = -1;   // mat_gate_en_002       â€” gold "STAGE SELECT" wordmark (512x128)
int g_nameTex  = -1;   // mat_gate_en_001       â€” stage NAME + SUB-NAME grid    (512x512)
int g_shotTex  = -1;   // mat_stage_ss          â€” real stage screenshots        (1024x512)
int g_ringTex  = -1;   // mat_comon_002         â€” gold RING spin strip          (1024x128)
int g_sunTex   = -1;   // mat_comon_003         â€” red SUN MEDAL spin strip       (1024x128)
int g_moonTex  = -1;   // mat_comon_004         â€” blue MOON MEDAL spin strip      (1024x128)
int g_rankTex  = -1;   // mat_result_comon_002  â€” metal rank letters S..E         (512x512)
int g_glyphTex = -1;   // mat_comon_x360_001    â€” Xbox button glyphs             (512x512)

// ---- real game fonts (MSDF) + DFSoGei SDF title face -------------------------
static int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// ---- atlas pixel sizes (for aspect-correct fitting) -------------------------
constexpr float TITLE_TEX_W = 512.0f, TITLE_TEX_H = 128.0f;
constexpr float NAME_TEX_W  = 512.0f, NAME_TEX_H  = 512.0f;
constexpr float SHOT_TEX_W  = 1024.0f, SHOT_TEX_H = 512.0f;
constexpr float ICON_TEX_W  = 1024.0f, ICON_TEX_H = 128.0f;
constexpr float RANK_TEX_W  = 512.0f, RANK_TEX_H  = 512.0f;
constexpr float GLYPH_TEX_W = 512.0f, GLYPH_TEX_H = 512.0f;

// "STAGE SELECT" wordmark â€” tight opaque bbox measured from mat_gate_en_002:
// px (0,0)-(261,80) -> normalized; aspect ~3.26.
const UV TITLE_UV = { 0.0f, 0.0f, 0.50977f, 0.625f };

// per-stage NAME wordmark (left column) + SUB-NAME wordmark (right column) of
// mat_gate_en_001. Measured 9 text bands (alpha occupancy); the left names span
// x 0..253 (widest = "Skyscraper Scamper"), the sub-names span x 256..433. Each
// row's vertical span is taken from the measured band, padded a touch.
struct NameRow { float v0, v1; };
const NameRow NAME_ROW[9] = {
    { 0.00586f, 0.05859f },  // 0 row y  3..30
    { 0.06641f, 0.12695f },  // 1 row y 34..65
    { 0.13086f, 0.18945f },  // 2 row y 67..97
    { 0.19141f, 0.25000f },  // 3 row y 98..128
    { 0.25391f, 0.31445f },  // 4 row y130..161
    { 0.31641f, 0.37109f },  // 5 row y162..190
    { 0.38086f, 0.43945f },  // 6 row y195..225
    { 0.44141f, 0.50195f },  // 7 row y226..257
    { 0.50586f, 0.56445f },  // 8 row y259..289
};
constexpr float NAME_L_U0 = 0.0f,     NAME_L_U1 = 0.49805f;   // left  column (stage name), px 0..255
constexpr float NAME_R_U0 = 0.50195f, NAME_R_U1 = 0.84766f;   // right column (sub-name),  px 257..434

UV NameUV(int row)    { return { NAME_L_U0, NAME_ROW[row].v0, NAME_L_U1, NAME_ROW[row].v1 }; }
UV SubNameUV(int row) { return { NAME_R_U0, NAME_ROW[row].v0, NAME_R_U1, NAME_ROW[row].v1 }; }

// real stage screenshots packed into mat_stage_ss (the rest of the atlas is white
// "SUB STAGE" label boxes we ignore). Four distinct shots measured from content
// variance: the top-left hero cell + the three bottom-row cells.
const UV SHOT_UV[4] = {
    /* A top-left  px (0,0)-(270,135)   */ { 0.0f,     0.0f,    0.26367f, 0.26367f },
    /* B bottom-1  px (83,272)-(242,406)*/ { 0.08105f, 0.53125f, 0.23633f, 0.79297f },
    /* C bottom-2  px (271,288)-(541,406)*/{ 0.26465f, 0.5625f,  0.52832f, 0.79297f },
    /* D bottom-3  px (542,272)-(798,406)*/{ 0.52930f, 0.53125f, 0.77930f, 0.79297f },
};

// metal rank letters: 0=S 1=A 2=B 3=C 4=D 5=E â€” the same 2x3 grid as world_map.
const char* const RANK_NAME[6] = { "S", "A", "B", "C", "D", "E" };
const UV RANK_UV[6] = {
    /* S */ { 0.0293f, 0.0117f, 0.2637f, 0.2812f },
    /* A */ { 0.3203f, 0.0117f, 0.5605f, 0.2812f },
    /* B */ { 0.0273f, 0.3086f, 0.2598f, 0.5742f },
    /* C */ { 0.3125f, 0.3086f, 0.5664f, 0.5742f },
    /* D */ { 0.0293f, 0.6016f, 0.2637f, 0.8613f },
    /* E */ { 0.3281f, 0.6016f, 0.5527f, 0.8613f },
};

// collectible icons â€” first (full-face) frame of each 1024x128 spin strip.
// px (0,0)-(60,60) -> normalized.
const UV ICON_RING = { 0.0f, 0.0f, 0.05859f, 0.46875f };
const UV ICON_SUN  = { 0.0f, 0.0f, 0.05859f, 0.46875f };
const UV ICON_MOON = { 0.0f, 0.0f, 0.05859f, 0.46875f };

// Xbox button glyphs (top row of mat_comon_x360_001).
const UV GLYPH_A  = { 0.0000f, 0.0020f, 0.0684f, 0.0762f };
const UV GLYPH_B  = { 0.0801f, 0.0020f, 0.1484f, 0.0762f };
const UV GLYPH_LB = { 0.3262f, 0.0020f, 0.4570f, 0.0762f };
const UV GLYPH_RB = { 0.4824f, 0.0020f, 0.6133f, 0.0762f };

// ---- data model -------------------------------------------------------------
// One ACT = a name-grid row (its NAME + SUB-NAME wordmark), a screenshot cell, an
// objective line, a HI-SCORE, sun/moon medal counts, a rank, and a locked flag.
struct Act {
    int         nameRow;     // index into mat_gate_en_001 (also the ASCII fallback name)
    const char* nameAscii;   // fallback if the wordmark atlas is missing
    const char* subAscii;
    int         shot;        // index into SHOT_UV
    const char* objective;
    bool        unlocked;
    int         hiScore;
    int         rings;        // ring count to collect / collected
    int         sunMedals;    // 0..max
    int         moonMedals;
    int         rank;         // 0..5 -> S..E ; -1 = no record
};

// A small, hand-built act roster drawn from the real name/sub-name grid order.
const Act ACTS[] = {
    { 0, "WINDMILL ISLE",      "WHITE ISLAND",       0,
      "Reach the goal ring at the end of the act.",       true,   86420, 240, 3, 0, 1 },
    { 1, "ROOFTOP RUN",        "ORANGE ROOFS",       2,
      "Dash across the rooftops to the goal.",            true,  154200, 312, 4, 0, 0 },
    { 4, "COOL EDGE",          "COOL EDGE",          3,
      "Carve down the glacier without falling.",          true,  140250, 198, 2, 1, 2 },
    { 6, "JUNGLE JOYRIDE",     "JUNGLE JOYRIDE",     1,
      "Ride the rails through the jungle canopy.",        true,  133900, 276, 3, 0, 1 },
    { 8, "EGGMANLAND",         "EGGMANLAND",         0,
      "Survive Eggman's gauntlet and reach the core.",    false,      0,   0, 0, 0, -1 },
};
constexpr int ACT_COUNT = int(sizeof(ACTS) / sizeof(ACTS[0]));

// ---- layout (reference px) --------------------------------------------------
constexpr float TITLE_X = 60.0f, TITLE_Y = 40.0f;        // "STAGE SELECT" wordmark
constexpr float RULE_Y  = 124.0f;

// left: hero screenshot panel ; right: briefing info panel
constexpr float SHOT_X = 60.0f,  SHOT_Y = 156.0f, SHOT_W = 600.0f, SHOT_H = 410.0f;
constexpr float INFO_X = 690.0f, INFO_Y = 156.0f, INFO_W = 530.0f, INFO_H = 410.0f;
constexpr float HEADER_H = 52.0f;        // caption strip atop each window

// screenshot plate inside the shot window (dark inset under the header)
constexpr float PLATE_PAD   = 16.0f;
constexpr float PLATE_X     = SHOT_X + PLATE_PAD;
constexpr float PLATE_Y     = SHOT_Y + HEADER_H + 14.0f;
constexpr float PLATE_W     = SHOT_W - PLATE_PAD * 2.0f;
constexpr float PLATE_H     = 296.0f;

// ---- entrance tuning (frames @60fps), tuned to shop/status/world_map ---------
constexpr double TITLE_FRAMES = 14.0;
constexpr double SHOT_FRAMES  = 16.0;
constexpr double INFO_OFFSET  = 5.0,  INFO_FRAMES = 14.0;
constexpr double FOOT_OFFSET  = 10.0, FOOT_FRAMES = 12.0;
constexpr double SWITCH_FRAMES = 10.0;   // act cross-fade/slide on Left/Right

// ---- palette (shared with pause/result/shop/status/world_map) ---------------
const uint32_t COL_BG_TOP    = RGBA(12, 20, 38, 255);
const uint32_t COL_BG_BOT    = RGBA(5, 9, 18, 255);
const uint32_t COL_TITLE     = RGBA(255, 209, 74, 255);
const uint32_t COL_TEXT      = RGBA(214, 226, 240, 255);
const uint32_t COL_TEXT_SEL  = RGBA(255, 255, 255, 255);
const uint32_t COL_DESC      = RGBA(178, 194, 214, 255);
const uint32_t COL_RULE      = RGBA(120, 170, 230, 90);
const uint32_t COL_PLATE     = RGBA(6, 10, 20, 235);     // dark plate behind the screenshot
const uint32_t COL_LOCKED    = RGBA(120, 120, 132, 255);
const uint32_t COL_OK        = RGBA(120, 230, 140, 255);
const uint32_t COL_START     = RGBA(255, 236, 150, 255);  // pulsing START prompt
const uint32_t COL_FOOTER    = RGBA(190, 205, 225, 220);
const uint32_t COL_WHITE     = RGBA(255, 255, 255, 255);

// ---- interactive state ------------------------------------------------------
int    g_act = 0;
double g_switchStart = -100.0;   // Now() when the act last changed
double g_startStart  = -100.0;   // Now() of the last (A) START press
double g_open        = 0.0;      // openSec captured each Draw (for the START pulse)

const Act& Cur() { return ACTS[g_act]; }

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
    if (g_titleTex < 0) g_titleTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_gate_en_002.png");
    if (g_nameTex  < 0) g_nameTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_gate_en_001.png");
    if (g_shotTex  < 0) g_shotTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_stage_ss.png");
    if (g_ringTex  < 0) g_ringTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_002.png");
    if (g_sunTex   < 0) g_sunTex   = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_003.png");
    if (g_moonTex  < 0) g_moonTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_004.png");
    if (g_rankTex  < 0) g_rankTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_result_comon_002.png");
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}

void Reset() {
    g_act = 0;
    g_switchStart = -100.0; g_startStart = -100.0; g_open = 0.0;
}

void SwitchAct(int dir) {
    g_act = (g_act + ACT_COUNT + dir) % ACT_COUNT;
    g_switchStart = Now();
}

void Input(const ScreenInput& in) {
    if (in.left  || in.tabLeft)  SwitchAct(-1);
    if (in.right || in.tabRight) SwitchAct(+1);
    if (in.accept) g_startStart = Now();     // "START" (standalone: a flash)
    // cancel: would back out to the world map in-game; no-op in the standalone build.
}

// ---- aspect-preserving image fit into a box ---------------------------------
// Draws the atlas sub-rect (uv) centred inside [bx,by,bx+bw,by+bh], scaled to fit,
// with a vertical anchor (0=top,0.5=center,1=bottom). `slide` shifts it for entrance.
void DrawFitted(int tex, const UV& uv, float texW, float texH,
                float bx, float by, float bw, float bh,
                float t, float vAnchor = 0.5f, float slide = 0.0f) {
    if (tex < 0 || t <= 0.0f) return;
    float artW = (uv.u1 - uv.u0) * texW;
    float artH = (uv.v1 - uv.v0) * texH;
    if (artW <= 0.0f || artH <= 0.0f) return;
    float scale = std::min(bw / artW, bh / artH);
    float w = artW * scale, h = artH * scale;
    float x = bx + (bw - w) * 0.5f;
    float y = by + (bh - h) * vAnchor + slide;
    DrawImage(tex, { x, y }, { x + w, y + h }, { uv.u0, uv.v0 }, { uv.u1, uv.v1 },
              WithAlpha(COL_WHITE, t));
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

// one footer hint: real button glyph + ASCII label. Returns the x past the label.
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

// one medal stat: a real collectible icon + "x N" count (or "--" when no record).
void DrawMedal(float x, float y, int icon, const UV& uv, int count, bool unlocked, float t) {
    const float ih = 30.0f;
    float iw = ih;   // icons are square (full-face frame)
    if (icon >= 0 && t > 0.0f)
        DrawImage(icon, { x, y }, { x + iw, y + ih }, { uv.u0, uv.v0 }, { uv.u1, uv.v1 },
                  WithAlpha(COL_WHITE, t));
    char buf[16];
    if (unlocked) std::snprintf(buf, sizeof(buf), "x %d", count);
    else          std::snprintf(buf, sizeof(buf), "x --");
    SetFont(g_fRodin);
    DrawText({ x + iw + 8.0f, y + 3.0f }, 24.0f,
             WithAlpha(unlocked ? COL_TEXT_SEL : COL_LOCKED, t), buf);
}

void Draw(double openSec) {
    g_open = openSec;
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const Act& a = Cur();
    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float shotT  = (float)ComputeMotion(openSec, 0.0, SHOT_FRAMES);
    const float infoT  = (float)ComputeMotion(openSec, INFO_OFFSET, INFO_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);
    // act-switch animation: content slides + fades a touch when you change acts
    const float switchT = (float)ComputeMotion(g_switchStart, 0.0, SWITCH_FRAMES);
    const float swSlide = (1.0f - switchT) * 26.0f;
    const float swFade  = 0.35f + 0.65f * switchT;

    // ===== TITLE: gold "STAGE SELECT" wordmark (real art) ====================
    if (g_titleTex >= 0 && titleT > 0.0f) {
        float aspect = ((TITLE_UV.u1 - TITLE_UV.u0) * TITLE_TEX_W) /
                       ((TITLE_UV.v1 - TITLE_UV.v0) * TITLE_TEX_H);
        float lh = 56.0f, lw = lh * aspect;
        float lx = TITLE_X, ly = TITLE_Y - (1.0f - titleT) * 14.0f;
        DrawImage(g_titleTex, { lx, ly }, { lx + lw, ly + lh },
                  { TITLE_UV.u0, TITLE_UV.v0 }, { TITLE_UV.u1, TITLE_UV.v1 }, WithAlpha(COL_WHITE, titleT));
    } else {
        SetFont(g_fDF);
        DrawTextShadow({ TITLE_X, TITLE_Y + 6.0f - (1.0f - titleT) * 16.0f }, 46.0f,
                       WithAlpha(COL_TITLE, titleT), "STAGE SELECT");
    }
    DrawRect({ TITLE_X, RULE_Y }, { 1220.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));
    // act index (top-right)
    {
        char idx[24];
        std::snprintf(idx, sizeof(idx), "STAGE  %d / %d", g_act + 1, ACT_COUNT);
        SetFont(g_fRodin);
        DrawTextAligned({ 920.0f, TITLE_Y + 8.0f }, { 1220.0f, TITLE_Y + 52.0f }, 26.0f,
                        WithAlpha(COL_DESC, titleT), idx, Align::Right, true, true);
    }

    // ===== LEFT: HERO STAGE SCREENSHOT PANEL =================================
    const float sx = SHOT_X, sy = SHOT_Y + (1.0f - shotT) * 24.0f;
    DrawWindow(sx, sy, SHOT_W, SHOT_H, shotT, "STAGE");

    // dark plate + the LARGE real stage screenshot (aspect-fit), inset under the header
    DrawRect({ PLATE_X, PLATE_Y + (1.0f - shotT) * 24.0f },
             { PLATE_X + PLATE_W, PLATE_Y + PLATE_H + (1.0f - shotT) * 24.0f },
             WithAlpha(COL_PLATE, shotT));
    DrawFitted(g_shotTex, SHOT_UV[std::clamp(a.shot, 0, 3)], SHOT_TEX_W, SHOT_TEX_H,
               PLATE_X, PLATE_Y + (1.0f - shotT) * 24.0f, PLATE_W, PLATE_H,
               shotT * swFade, 0.5f, swSlide);
    // thin frame rules around the plate
    DrawRect({ PLATE_X, PLATE_Y + (1.0f - shotT) * 24.0f },
             { PLATE_X + PLATE_W, PLATE_Y + 2 + (1.0f - shotT) * 24.0f }, WithAlpha(COL_RULE, shotT));
    DrawRect({ PLATE_X, PLATE_Y + PLATE_H - 2 + (1.0f - shotT) * 24.0f },
             { PLATE_X + PLATE_W, PLATE_Y + PLATE_H + (1.0f - shotT) * 24.0f }, WithAlpha(COL_RULE, shotT));

    // objective line under the screenshot, inside the shot panel
    {
        float oy = PLATE_Y + PLATE_H + 22.0f + (1.0f - shotT) * 24.0f;
        SetFont(g_fSeurat);
        DrawText({ PLATE_X + 4.0f, oy }, 20.0f, WithAlpha(COL_DESC, shotT), "OBJECTIVE");
        DrawText({ PLATE_X + 4.0f, oy + 26.0f }, 21.0f,
                 WithAlpha(a.unlocked ? COL_TEXT : COL_LOCKED, shotT),
                 a.unlocked ? a.objective : "Clear the previous stage to unlock.");
    }

    // ===== RIGHT: BRIEFING INFO PANEL ========================================
    const float ix = INFO_X, iy = INFO_Y + (1.0f - infoT) * 24.0f;
    DrawWindow(ix, iy, INFO_W, INFO_H, infoT, "ACT");

    // ---- stage NAME wordmark (real art, fit-boxed) ----
    const float nameBoxX = ix + 22.0f, nameBoxY = iy + HEADER_H + 16.0f;
    const float nameBoxW = INFO_W - 44.0f, nameBoxH = 42.0f;
    if (g_nameTex >= 0 && infoT > 0.0f) {
        DrawFitted(g_nameTex, NameUV(a.nameRow), NAME_TEX_W, NAME_TEX_H,
                   nameBoxX, nameBoxY, nameBoxW, nameBoxH,
                   infoT * swFade, 0.0f, swSlide);   // left/top-anchored
    } else {
        SetFont(g_fDF);
        DrawTextShadow({ nameBoxX, nameBoxY }, 32.0f, WithAlpha(COL_TEXT_SEL, infoT), a.nameAscii);
    }
    // ---- sub-name wordmark (smaller, just below) ----
    const float subBoxY = nameBoxY + nameBoxH + 6.0f, subBoxH = 26.0f;
    if (g_nameTex >= 0 && infoT > 0.0f) {
        DrawFitted(g_nameTex, SubNameUV(a.nameRow), NAME_TEX_W, NAME_TEX_H,
                   nameBoxX, subBoxY, nameBoxW * 0.7f, subBoxH,
                   infoT * swFade, 0.0f, swSlide);
    } else {
        SetFont(g_fSeurat);
        DrawText({ nameBoxX, subBoxY }, 21.0f, WithAlpha(COL_DESC, infoT), a.subAscii);
    }

    // ---- records block: HI-SCORE / RINGS / MEDALS / RANK ----
    float bx = ix + 24.0f, by = subBoxY + subBoxH + 22.0f;
    const float labelW = 150.0f, lineH = 36.0f;

    auto recLine = [&](const char* label, const char* value, uint32_t valCol) {
        SetFont(g_fSeurat);
        DrawText({ bx, by }, 23.0f, WithAlpha(COL_DESC, infoT), label);
        SetFont(g_fRodin);
        DrawTextAligned({ bx + labelW, by - 4 }, { ix + INFO_W - 24, by + 28 }, 26.0f,
                        WithAlpha(valCol, infoT), value, Align::Right, true, true);
        by += lineH;
    };

    if (a.unlocked) {
        char buf[24];
        Commafy(a.hiScore, buf, sizeof(buf));
        recLine("HI-SCORE", buf, COL_TITLE);
        char rb[16]; std::snprintf(rb, sizeof(rb), "%d", a.rings);
        recLine("RING GOAL", rb, COL_TEXT_SEL);
    } else {
        recLine("HI-SCORE", "-- NO RECORD --", COL_LOCKED);
        recLine("RING GOAL", "--", COL_LOCKED);
    }

    // medals row: real RING / SUN / MOON icons + counts
    {
        SetFont(g_fSeurat);
        DrawText({ bx, by }, 23.0f, WithAlpha(COL_DESC, infoT), "MEDALS");
        float mx = bx + labelW - 36.0f, my = by - 2.0f;
        DrawMedal(mx,          my, g_sunTex,  ICON_SUN,  a.sunMedals,  a.unlocked, infoT);
        DrawMedal(mx + 120.0f, my, g_moonTex, ICON_MOON, a.moonMedals, a.unlocked, infoT);
        by += lineH + 6.0f;
    }

    // RANK row: label + the REAL metal rank letter art (or ASCII fallback)
    {
        SetFont(g_fSeurat);
        DrawText({ bx, by }, 23.0f, WithAlpha(COL_DESC, infoT), "RANK");
        if (a.unlocked && a.rank >= 0 && a.rank <= 5) {
            if (g_rankTex >= 0) {
                const UV& ru = RANK_UV[a.rank];
                float rh = 50.0f;
                float aspect = ((ru.u1 - ru.u0) * RANK_TEX_W) / ((ru.v1 - ru.v0) * RANK_TEX_H);
                float rw = rh * aspect;
                float rxp = ix + INFO_W - 24.0f - rw;
                float ryp = by - 14.0f;
                DrawImage(g_rankTex, { rxp, ryp }, { rxp + rw, ryp + rh },
                          { ru.u0, ru.v0 }, { ru.u1, ru.v1 }, WithAlpha(COL_WHITE, infoT));
            } else {
                SetFont(g_fDF);
                DrawTextAligned({ bx + labelW, by - 8 }, { ix + INFO_W - 24, by + 32 }, 38.0f,
                                WithAlpha(COL_TITLE, infoT), RANK_NAME[a.rank], Align::Right, true, true);
            }
        } else {
            SetFont(g_fRodin);
            DrawTextAligned({ bx + labelW, by - 4 }, { ix + INFO_W - 24, by + 28 }, 26.0f,
                            WithAlpha(COL_LOCKED, infoT), "--", Align::Right, true, true);
        }
    }

    // ---- START prompt: a pulsing call-to-action at the panel's foot ----
    if (infoT > 0.5f) {
        float pulse = 0.55f + 0.45f * (float)(0.5 + 0.5 * std::sin((g_open) * 4.0));
        uint32_t pc = a.unlocked ? COL_START : COL_LOCKED;
        SetFont(g_fRodin);
        DrawTextAligned({ ix + 20.0f, iy + INFO_H - 56.0f }, { ix + INFO_W - 20.0f, iy + INFO_H - 14.0f },
                        34.0f, WithAlpha(pc, infoT * (a.unlocked ? pulse : 0.7f)),
                        a.unlocked ? "PRESS  (A)  TO START" : "STAGE LOCKED",
                        Align::Center, true, true);
    }

    // ===== FOOTER (real button glyphs + ASCII labels) ========================
    {
        float hx = TITLE_X, hcy = 648.0f;
        const float aAsp = 0.921f, lrAsp = 1.763f;
        DrawRect({ TITLE_X, 614.0f }, { 1220.0f, 616.0f }, WithAlpha(COL_RULE, footT));
        hx = DrawHint(hx, hcy, GLYPH_A,  aAsp,  "Start",         footT);
        hx = DrawHint(hx, hcy, GLYPH_B,  aAsp,  "Back",          footT);
        hx = DrawHint(hx, hcy, GLYPH_LB, lrAsp, "",              footT);
        hx = DrawHint(hx, hcy, GLYPH_RB, lrAsp, "Switch Stage",  footT);
    }

    // ===== transient "START!" flash ==========================================
    double age = Now() - g_startStart;
    if (g_startStart > 0.0 && age < 1.4) {
        float ma = std::min(1.0f, (float)((1.4 - age) / 0.4));
        char line[80];
        if (a.unlocked) std::snprintf(line, sizeof(line), "Starting %s...", a.nameAscii);
        else            std::snprintf(line, sizeof(line), "%s is locked!", a.nameAscii);
        SetFont(g_fRodin);
        DrawTextAligned({ SHOT_X, 572.0f }, { 1220.0f, 604.0f }, 24.0f,
                        WithAlpha(a.unlocked ? COL_OK : COL_LOCKED, ma), line, Align::Center, true, true);
    }

    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void GateInit() { Init(); }
void GateDraw(double openSeconds) { Draw(openSeconds); }
void GateInput(const ScreenInput& in) { Input(in); }
void GateReset() { Reset(); }
