// =============================================================================
// screen_exstage.cpp â€” the EX / Extra-Stage entry card, re-authored as clean
// hand-written C++ in the UnleashedRecomp ui/options_menu idiom (NOT a CSD node
// dump â€” the raw exstage.json is just two mat_hit_001 9-slice stretch-arms that
// run from x=-95 to x=+2100, plus two hit-combo wordmark quads). This is a sibling
// of screen_gate.cpp: a real stage THUMBNAIL plate + EX name + a short objective +
// HI-SCORE/medal + a START prompt, all inside bounded REAL Sonic Unleashed
// silver-chrome windows (ui::DrawGameWindow, 9-slice from mat_result_comon_001).
//
// The distinctive retail art the exstage atlases actually contain (and that this
// screen places at sane aspect-preserved rects) is:
//   * the bold outlined SCORE-DIGIT font  (mat_hit_001, a 4-col 3-row big-glyph
//     grid 0-9 + "/") â€” used to render the HI-SCORE as the game's own number art,
//   * the localized EX banner wordmark    (mat_hit_en_001, "CHANCE ATTACK" row) â€”
//     the most EX-flavoured banner on the atlas, laid over the stage plate,
//   * the "SHIELD" / "ENERGY" labels       (mat_ex_en_001) â€” the two EX stat names,
//   * a small silver MEDAL/ring icon        (mat_ex_common_001, bottom-right badge),
//   * the Xbox A/B/LB/RB button glyphs       (mat_comon_x360_001, shared with gate).
//
// Fully interactive + stateful like options_menu / gate / world_map:
//   * Left/Right OR Q/E (LB/RB) switch the EX ENTRY -> banner, name, objective,
//     HI-SCORE digits, shield/energy stats and rank all change,
//   * (A) "STARTs" the highlighted entry (a transient flash),
//   * (B) backs out (no-op in the standalone build),
//   * a staggered ease-in entrance: title -> shot panel -> info panel -> footer,
//   * a continuous PRESS-START pulse (ui::Now() + std::sin).
//
// Art is the real extracted atlas where a distinctive element exists; bounded
// chrome windows + ASCII text are the graceful fallback when an atlas is missing
// (-1 slot), exactly as the gate/world_map/shop fallbacks do. (There is no real
// stage-screenshot atlas in the exstage asset set, so the left "shot" is a dark
// inset plate carrying the EX banner â€” the same plate technique gate uses.)
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
const char* const ASSET_BASE = "assets/exstage/";
int g_digitTex = -1;   // mat_hit_001        â€” bold outlined score digits 0-9 + "/"  (256x256)
int g_bannerTex= -1;   // mat_hit_en_001     â€” localized banner wordmarks ("CHANCE ATTACK") (512x512)
int g_labelTex = -1;   // mat_ex_en_001      â€” "SHIELD" / "ENERGY" EX stat labels       (128x64)
int g_iconTex  = -1;   // mat_ex_common_001  â€” silver MEDAL/ring badge (bottom-right)    (256x128)
int g_glyphTex = -1;   // mat_comon_x360_001 â€” Xbox button glyphs (shared)               (512x512)

// ---- atlas pixel sizes (for aspect-correct fitting) -------------------------
constexpr float DIGIT_TEX_W  = 256.0f, DIGIT_TEX_H  = 256.0f;
constexpr float BANNER_TEX_W = 512.0f, BANNER_TEX_H = 512.0f;
constexpr float LABEL_TEX_W  = 128.0f, LABEL_TEX_H  = 64.0f;
constexpr float ICON_TEX_W   = 256.0f, ICON_TEX_H   = 128.0f;
constexpr float GLYPH_TEX_W  = 512.0f, GLYPH_TEX_H  = 512.0f;

// ---- SCORE-DIGIT font sub-rects (mat_hit_001, 256x256) ----------------------
// Big outlined glyphs laid out in a 4-col x 3-row grid in the top ~2/3 of the
// atlas; columns are ~64px wide, rows ~58px tall. Grid order (measured):
//   row0: 1 2 3 4    row1: 5 6 7 8    row2: 9 0 _ /
// We index by the displayed character. Boxes are slightly generous (the glyphs
// are italic so they overhang their nominal cell); the tight-ish opaque box keeps
// the readout monospace. Indices 0-9 map to the digit's atlas cell; SLASH is "/".
const UV DIGIT_UV[10] = {
    /* 0 */ { 0.2617f, 0.4609f, 0.4805f, 0.6602f },  // r2 c1
    /* 1 */ { 0.0078f, 0.0234f, 0.2266f, 0.2227f },  // r0 c0
    /* 2 */ { 0.2617f, 0.0234f, 0.4805f, 0.2227f },  // r0 c1
    /* 3 */ { 0.5156f, 0.0234f, 0.7344f, 0.2227f },  // r0 c2
    /* 4 */ { 0.7656f, 0.0234f, 0.9844f, 0.2227f },  // r0 c3
    /* 5 */ { 0.0078f, 0.2422f, 0.2266f, 0.4414f },  // r1 c0
    /* 6 */ { 0.2617f, 0.2422f, 0.4805f, 0.4414f },  // r1 c1
    /* 7 */ { 0.5156f, 0.2422f, 0.7344f, 0.4414f },  // r1 c2
    /* 8 */ { 0.7656f, 0.2422f, 0.9844f, 0.4414f },  // r1 c3
    /* 9 */ { 0.0078f, 0.4609f, 0.2266f, 0.6602f },  // r2 c0
};
const UV DIGIT_SLASH = { 0.7656f, 0.4609f, 0.9844f, 0.6602f };  // r2 c3

// localized EX banner â€” the "CHANCE ATTACK" row of mat_hit_en_001 (the most
// EX-flavoured wordmark on the atlas). Measured opaque band px (4,60)-(500,122).
const UV BANNER_UV = { 0.0078f, 0.1172f, 0.9766f, 0.2383f };

// EX stat labels in mat_ex_en_001 (128x64): "SHIELD" top-left, "ENERGY" lower-right.
const UV LABEL_SHIELD = { 0.0156f, 0.0312f, 0.5625f, 0.3125f };  // px (2,2)-(72,20)
const UV LABEL_ENERGY = { 0.4219f, 0.4688f, 0.9688f, 0.7500f };  // px (54,30)-(124,48)

// silver MEDAL/ring badge â€” bottom-right circular icon of mat_ex_common_001.
const UV ICON_MEDAL = { 0.8516f, 0.7031f, 0.9766f, 0.9531f };    // px (218,90)-(250,122)

// Xbox button glyphs (top row of mat_comon_x360_001) â€” copied VERBATIM from gate.
const UV GLYPH_A  = { 0.0000f, 0.0020f, 0.0684f, 0.0762f };
const UV GLYPH_B  = { 0.0801f, 0.0020f, 0.1484f, 0.0762f };
const UV GLYPH_LB = { 0.3262f, 0.0020f, 0.4570f, 0.0762f };
const UV GLYPH_RB = { 0.4824f, 0.0020f, 0.6133f, 0.0762f };

// ---- data model -------------------------------------------------------------
// One EX ENTRY = a banner kind (which localized row to fly over the plate), a name
// + sub-name, an objective line, a HI-SCORE (drawn with the real digit font), a
// shield/energy stat pair, a medal count, a rank (0..5 -> S..E ; -1 = none) and a
// locked flag. This mirrors gate's Act, retuned for the EX / extra-stage flavour.
struct ExEntry {
    const char* name;
    const char* sub;
    const char* objective;
    bool        unlocked;
    int         hiScore;
    int         shield;     // 0..100 (%)
    int         energy;     // 0..100 (%)
    int         medals;     // 0..5
    int         rank;       // 0..5 -> S..E ; -1 = no record
};

// A small, hand-built EX roster in the EX / extra-stage spirit.
const ExEntry ENTRIES[] = {
    { "CHANCE ATTACK",  "EX TRIAL 01",
      "Chain hits past the combo gate before the timer ends.",  true,  248600, 100, 80, 4, 1 },
    { "CRITICAL RUSH",  "EX TRIAL 02",
      "Land every critical strike without taking a hit.",       true,  312400,  60, 95, 5, 0 },
    { "SHIELD GAUNTLET","EX TRIAL 03",
      "Survive the gauntlet with the shield gauge intact.",     true,  176050,  40, 55, 3, 2 },
    { "ENERGY SURGE",   "EX TRIAL 04",
      "Keep the energy gauge maxed to the finish line.",        true,  205300,  75, 100, 4, 1 },
    { "FINAL EX",       "EX TRIAL 05",
      "Clear all prior EX trials to unlock the final run.",     false,      0,   0,  0, 0, -1 },
};
constexpr int ENTRY_COUNT = int(sizeof(ENTRIES) / sizeof(ENTRIES[0]));

// rank letters (ASCII fallback only â€” there is no rank-letter atlas in this set)
const char* const RANK_NAME[6] = { "S", "A", "B", "C", "D", "E" };

// ---- layout (reference px) --------------------------------------------------
constexpr float TITLE_X = 60.0f, TITLE_Y = 40.0f;        // "EXTRA STAGE" title
constexpr float RULE_Y  = 124.0f;

// left: EX banner / stage plate panel ; right: briefing info panel
constexpr float SHOT_X = 60.0f,  SHOT_Y = 156.0f, SHOT_W = 600.0f, SHOT_H = 410.0f;
constexpr float INFO_X = 690.0f, INFO_Y = 156.0f, INFO_W = 530.0f, INFO_H = 410.0f;
constexpr float HEADER_H = 52.0f;        // caption strip atop each window

// banner/stage plate inside the shot window (dark inset under the header)
constexpr float PLATE_PAD = 16.0f;
constexpr float PLATE_X   = SHOT_X + PLATE_PAD;
constexpr float PLATE_Y   = SHOT_Y + HEADER_H + 14.0f;
constexpr float PLATE_W   = SHOT_W - PLATE_PAD * 2.0f;
constexpr float PLATE_H   = 296.0f;

// ---- entrance tuning (frames @60fps), tuned to gate/world_map ---------------
constexpr double TITLE_FRAMES = 14.0;
constexpr double SHOT_FRAMES  = 16.0;
constexpr double INFO_OFFSET  = 5.0,  INFO_FRAMES = 14.0;
constexpr double FOOT_OFFSET  = 10.0, FOOT_FRAMES = 12.0;
constexpr double SWITCH_FRAMES = 10.0;   // entry cross-fade/slide on Left/Right

// ---- palette (shared with pause/result/shop/status/gate/world_map) ----------
const uint32_t COL_TITLE     = RGBA(255, 209, 74, 255);
const uint32_t COL_TEXT      = RGBA(214, 226, 240, 255);
const uint32_t COL_TEXT_SEL  = RGBA(255, 255, 255, 255);
const uint32_t COL_DESC      = RGBA(178, 194, 214, 255);
const uint32_t COL_RULE      = RGBA(120, 170, 230, 90);
const uint32_t COL_PLATE     = RGBA(6, 10, 20, 235);     // dark plate behind the banner
const uint32_t COL_LOCKED    = RGBA(120, 120, 132, 255);
const uint32_t COL_OK        = RGBA(120, 230, 140, 255);
const uint32_t COL_START     = RGBA(255, 236, 150, 255);  // pulsing START prompt
const uint32_t COL_FOOTER    = RGBA(190, 205, 225, 220);
const uint32_t COL_WHITE     = RGBA(255, 255, 255, 255);
const uint32_t COL_BG_TOP    = RGBA(20, 14, 38, 255);     // EX flavour: a violet-tinted backdrop
const uint32_t COL_BG_BOT    = RGBA(6, 5, 16, 255);
const uint32_t COL_SHIELD    = RGBA(96, 200, 255, 255);   // cyan shield gauge
const uint32_t COL_ENERGY    = RGBA(120, 230, 140, 255);  // green energy gauge
const uint32_t COL_GAUGE_BG  = RGBA(10, 16, 30, 230);

// The recomp's actual fonts (staged to assets/fonts): FOT-SeuratPro-M (labels +
// body), FOT-NewRodinPro-DB (values + footer), DFSoGeiStd-W7 (title + captions).
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// ---- interactive state ------------------------------------------------------
int    g_entry = 0;
double g_switchStart = -100.0;   // Now() when the entry last changed
double g_startStart  = -100.0;   // Now() of the last (A) START press
double g_open        = 0.0;      // openSec captured each Draw (for the START pulse)

const ExEntry& Cur() { return ENTRIES[g_entry]; }

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
    if (g_digitTex  < 0) g_digitTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_hit_001.png");
    if (g_bannerTex < 0) g_bannerTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_hit_en_001.png");
    if (g_labelTex  < 0) g_labelTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_ex_en_001.png");
    if (g_iconTex   < 0) g_iconTex   = gfx::loadTexture(std::string(ASSET_BASE) + "mat_ex_common_001.png");
    // shared button-glyph atlas (lives in the gate/world_map asset dirs); try a couple of locations.
    if (g_glyphTex  < 0) g_glyphTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_x360_001.png");
    if (g_glyphTex  < 0) g_glyphTex  = gfx::loadTexture("assets/gate/mat_comon_x360_001.png");
    if (g_glyphTex  < 0) g_glyphTex  = gfx::loadTexture("assets/world_map/mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF (im_font_atlas)
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");    // real game MSDF
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");   // real DFSoGeiStd-W7 (title + captions)
}

void Reset() {
    g_entry = 0;
    g_switchStart = -100.0; g_startStart = -100.0; g_open = 0.0;
}

void SwitchEntry(int dir) {
    g_entry = (g_entry + ENTRY_COUNT + dir) % ENTRY_COUNT;
    g_switchStart = Now();
}

void Input(const ScreenInput& in) {
    if (in.left  || in.tabLeft)  SwitchEntry(-1);
    if (in.right || in.tabRight) SwitchEntry(+1);
    if (in.accept) g_startStart = Now();     // "START" (standalone: a flash)
    // cancel: would back out to the world map in-game; no-op in the standalone build.
}

// ---- aspect-preserving image fit into a box ---------------------------------
// Draws the atlas sub-rect (uv) centred inside [bx,by,bx+bw,by+bh], scaled to fit,
// with a vertical anchor (0=top,0.5=center,1=bottom). `slide` shifts it for entrance.
void DrawFitted(int tex, const UV& uv, float texW, float texH,
                float bx, float by, float bw, float bh,
                float t, float vAnchor = 0.5f, float slide = 0.0f, uint32_t tint = COL_WHITE) {
    if (tex < 0 || t <= 0.0f) return;
    float artW = (uv.u1 - uv.u0) * texW;
    float artH = (uv.v1 - uv.v0) * texH;
    if (artW <= 0.0f || artH <= 0.0f) return;
    float scale = std::min(bw / artW, bh / artH);
    float w = artW * scale, h = artH * scale;
    float x = bx + (bw - w) * 0.5f;
    float y = by + (bh - h) * vAnchor + slide;
    DrawImage(tex, { x, y }, { x + w, y + h }, { uv.u0, uv.v0 }, { uv.u1, uv.v1 },
              WithAlpha(tint, t));
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

// ---- HI-SCORE rendered with the REAL outlined digit font (mat_hit_001) ------
// Lays out the comma-grouped score string right-aligned ending at xRight, sitting
// on a baseline near `cy`. Digits use the atlas glyphs; commas use the text font
// (the digit atlas has no comma cell). Falls back to plain DrawText when the atlas
// is missing. Returns nothing â€” purely decorative big-number readout.
void DrawScoreDigits(const char* s, float xRight, float cy, float glyphH, float t) {
    if (t <= 0.0f) return;
    // glyph cell aspect from one representative digit box ("0")
    const UV& d0 = DIGIT_UV[0];
    float aspect = ((d0.u1 - d0.u0) * DIGIT_TEX_W) / ((d0.v1 - d0.v0) * DIGIT_TEX_H);
    float glyphW = glyphH * aspect;
    const float advance = glyphW * 0.82f;   // overlap the italic glyphs a touch
    const float commaAdv = glyphW * 0.40f;

    if (g_digitTex < 0) {
        // graceful fallback: plain right-aligned text number
        SetFont(g_fRodin);
        DrawTextAligned({ xRight - 240.0f, cy - glyphH * 0.5f }, { xRight, cy + glyphH * 0.5f },
                        glyphH * 0.7f, WithAlpha(COL_TITLE, t), s, Align::Right, true, true);
        return;
    }
    // measure total width first (right-align), iterating the string
    SetFont(g_fRodin);   // comma glyphs between digit groups
    int len = (int)std::strlen(s);
    float total = 0.0f;
    for (int i = 0; i < len; ++i) total += (s[i] == ',') ? commaAdv : advance;
    float x = xRight - total;
    float top = cy - glyphH * 0.5f;
    for (int i = 0; i < len; ++i) {
        char ch = s[i];
        if (ch == ',') {
            DrawText({ x + 1.0f, cy - glyphH * 0.32f }, glyphH * 0.55f, WithAlpha(COL_TITLE, t), ",");
            x += commaAdv;
        } else if (ch >= '0' && ch <= '9') {
            const UV& uv = DIGIT_UV[ch - '0'];
            DrawImage(g_digitTex, { x, top }, { x + glyphW, top + glyphH },
                      { uv.u0, uv.v0 }, { uv.u1, uv.v1 }, WithAlpha(COL_WHITE, t));
            x += advance;
        } else if (ch == '/') {
            DrawImage(g_digitTex, { x, top }, { x + glyphW, top + glyphH },
                      { DIGIT_SLASH.u0, DIGIT_SLASH.v0 }, { DIGIT_SLASH.u1, DIGIT_SLASH.v1 },
                      WithAlpha(COL_WHITE, t));
            x += advance;
        } else {
            x += advance;   // space / unknown
        }
    }
}

// ---- one EX stat gauge: real label art + a bounded fill bar -----------------
void DrawGauge(float x, float y, float w, const UV& label, int pct, uint32_t fillCol,
               bool unlocked, float t) {
    const float labelH = 22.0f;
    // real label wordmark (SHIELD / ENERGY), left-anchored
    if (g_labelTex >= 0 && t > 0.0f) {
        float aspect = ((label.u1 - label.u0) * LABEL_TEX_W) / ((label.v1 - label.v0) * LABEL_TEX_H);
        float lw = labelH * aspect;
        DrawImage(g_labelTex, { x, y }, { x + lw, y + labelH },
                  { label.u0, label.v0 }, { label.u1, label.v1 }, WithAlpha(COL_WHITE, t));
    }
    // gauge track + fill
    float gy = y + labelH + 6.0f, gh = 16.0f;
    DrawRect({ x, gy }, { x + w, gy + gh }, WithAlpha(COL_GAUGE_BG, t));
    float frac = unlocked ? std::clamp(pct / 100.0f, 0.0f, 1.0f) : 0.0f;
    if (frac > 0.0f)
        DrawRect({ x + 2.0f, gy + 2.0f }, { x + 2.0f + (w - 4.0f) * frac, gy + gh - 2.0f },
                 WithAlpha(fillCol, t));
    DrawRect({ x, gy }, { x + w, gy + 2.0f }, WithAlpha(COL_RULE, t));
    // numeric percent on the right of the track
    char buf[8];
    if (unlocked) std::snprintf(buf, sizeof(buf), "%d%%", pct);
    else          std::snprintf(buf, sizeof(buf), "--");
    SetFont(g_fRodin);
    DrawTextAligned({ x + w - 60.0f, y - 2.0f }, { x + w, y + labelH }, 20.0f,
                    WithAlpha(unlocked ? COL_TEXT_SEL : COL_LOCKED, t), buf, Align::Right, true, true);
}

void Draw(double openSec) {
    g_open = Now();   // elapsed-since-open for the continuous PRESS-START pulse (openSec is pinned to 0)
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const ExEntry& e = Cur();
    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float shotT  = (float)ComputeMotion(openSec, 0.0, SHOT_FRAMES);
    const float infoT  = (float)ComputeMotion(openSec, INFO_OFFSET, INFO_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);
    // entry-switch animation: content slides + fades a touch when you change entries
    const float switchT = (float)ComputeMotion(g_switchStart, 0.0, SWITCH_FRAMES);
    const float swSlide = (1.0f - switchT) * 26.0f;
    const float swFade  = 0.35f + 0.65f * switchT;

    // ===== TITLE: "EXTRA STAGE" (ASCII gold; no title-wordmark atlas in this set) =
    SetFont(g_fDF);
    DrawTextShadow({ TITLE_X, TITLE_Y + 6.0f - (1.0f - titleT) * 16.0f }, 46.0f,
                   WithAlpha(COL_TITLE, titleT), "EXTRA STAGE");
    DrawRect({ TITLE_X, RULE_Y }, { 1220.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));
    // entry index (top-right)
    {
        char idx[24];
        std::snprintf(idx, sizeof(idx), "EX  %d / %d", g_entry + 1, ENTRY_COUNT);
        SetFont(g_fRodin);
        DrawTextAligned({ 920.0f, TITLE_Y + 8.0f }, { 1220.0f, TITLE_Y + 52.0f }, 26.0f,
                        WithAlpha(COL_DESC, titleT), idx, Align::Right, true, true);
    }

    // ===== LEFT: EX BANNER / STAGE PLATE PANEL ===============================
    const float sx = SHOT_X, sy = SHOT_Y + (1.0f - shotT) * 24.0f;
    DrawWindow(sx, sy, SHOT_W, SHOT_H, shotT, "EX STAGE");

    // dark plate (no real screenshot atlas in this set) carrying the EX banner art
    const float plateY = PLATE_Y + (1.0f - shotT) * 24.0f;
    DrawRect({ PLATE_X, plateY }, { PLATE_X + PLATE_W, plateY + PLATE_H }, WithAlpha(COL_PLATE, shotT));
    // thin frame rules around the plate
    DrawRect({ PLATE_X, plateY }, { PLATE_X + PLATE_W, plateY + 2.0f }, WithAlpha(COL_RULE, shotT));
    DrawRect({ PLATE_X, plateY + PLATE_H - 2.0f }, { PLATE_X + PLATE_W, plateY + PLATE_H }, WithAlpha(COL_RULE, shotT));

    // the localized EX banner wordmark ("CHANCE ATTACK"), centred on the plate
    DrawFitted(g_bannerTex, BANNER_UV, BANNER_TEX_W, BANNER_TEX_H,
               PLATE_X + 24.0f, plateY + 40.0f, PLATE_W - 48.0f, 120.0f,
               shotT * swFade, 0.5f, swSlide);
    // when the banner atlas is missing, fall back to the ASCII EX name on the plate
    if (g_bannerTex < 0) {
        SetFont(g_fDF);
        DrawTextAligned({ PLATE_X + 16.0f, plateY + 40.0f }, { PLATE_X + PLATE_W - 16.0f, plateY + 140.0f },
                        40.0f, WithAlpha(COL_TEXT_SEL, shotT * swFade), e.name, Align::Center, true, true);
    }
    // sub-name caption + the silver medal badge, lower on the plate
    SetFont(g_fSeurat);
    DrawTextAligned({ PLATE_X + 16.0f, plateY + PLATE_H - 96.0f },
                    { PLATE_X + PLATE_W - 16.0f, plateY + PLATE_H - 64.0f },
                    24.0f, WithAlpha(COL_DESC, shotT * swFade), e.sub, Align::Center, true, true);
    {
        // real silver MEDAL/ring badge + "MEDALS x N" centred at the plate foot
        const float ih = 36.0f;
        float aspect = ((ICON_MEDAL.u1 - ICON_MEDAL.u0) * ICON_TEX_W) /
                       ((ICON_MEDAL.v1 - ICON_MEDAL.v0) * ICON_TEX_H);
        float iw = ih * aspect;
        char mb[16];
        if (e.unlocked) std::snprintf(mb, sizeof(mb), "x %d", e.medals);
        else            std::snprintf(mb, sizeof(mb), "x --");
        SetFont(g_fRodin);
        float labelW = MeasureText(24.0f, mb).x;
        float groupW = iw + 8.0f + labelW;
        float gx = PLATE_X + (PLATE_W - groupW) * 0.5f;
        float gy = plateY + PLATE_H - 52.0f;
        if (g_iconTex >= 0)
            DrawImage(g_iconTex, { gx, gy }, { gx + iw, gy + ih },
                      { ICON_MEDAL.u0, ICON_MEDAL.v0 }, { ICON_MEDAL.u1, ICON_MEDAL.v1 },
                      WithAlpha(COL_WHITE, shotT * swFade));
        DrawText({ gx + iw + 8.0f, gy + 6.0f }, 24.0f,
                 WithAlpha(e.unlocked ? COL_TEXT_SEL : COL_LOCKED, shotT * swFade), mb);
    }

    // objective line under the plate, inside the shot panel
    {
        float oy = plateY + PLATE_H + 22.0f;
        SetFont(g_fSeurat);
        DrawText({ PLATE_X + 4.0f, oy }, 20.0f, WithAlpha(COL_DESC, shotT), "OBJECTIVE");
        DrawText({ PLATE_X + 4.0f, oy + 26.0f }, 21.0f,
                 WithAlpha(e.unlocked ? COL_TEXT : COL_LOCKED, shotT),
                 e.unlocked ? e.objective : "Clear all prior EX trials to unlock.");
    }

    // ===== RIGHT: BRIEFING INFO PANEL ========================================
    const float ix = INFO_X, iy = INFO_Y + (1.0f - infoT) * 24.0f;
    DrawWindow(ix, iy, INFO_W, INFO_H, infoT, "EX TRIAL");

    // ---- EX name + sub-name (ASCII; the name wordmark grid is the localized banner
    //      already shown on the plate, so the panel uses readable DFHeiStd text) ----
    const float nameBoxX = ix + 22.0f, nameBoxY = iy + HEADER_H + 16.0f;
    SetFont(g_fDF);
    DrawTextShadow({ nameBoxX, nameBoxY }, 32.0f, WithAlpha(COL_TEXT_SEL, infoT), e.name);
    SetFont(g_fSeurat);
    DrawText({ nameBoxX, nameBoxY + 40.0f }, 21.0f, WithAlpha(COL_DESC, infoT), e.sub);

    // ---- HI-SCORE: label + the REAL outlined digit-font readout ----
    float bx = ix + 24.0f, by = nameBoxY + 78.0f;
    DrawText({ bx, by }, 23.0f, WithAlpha(COL_DESC, infoT), "HI-SCORE");
    if (e.unlocked) {
        char buf[24];
        Commafy(e.hiScore, buf, sizeof(buf));
        DrawScoreDigits(buf, ix + INFO_W - 24.0f, by + 14.0f, 40.0f, infoT * swFade);
    } else {
        SetFont(g_fRodin);
        DrawTextAligned({ bx + 120.0f, by - 4.0f }, { ix + INFO_W - 24.0f, by + 28.0f }, 26.0f,
                        WithAlpha(COL_LOCKED, infoT), "-- NO RECORD --", Align::Right, true, true);
    }
    by += 56.0f;

    // ---- EX stat gauges: SHIELD / ENERGY (real label art + fill bars) ----
    const float gaugeW = INFO_W - 48.0f;
    DrawGauge(bx, by, gaugeW, LABEL_SHIELD, e.shield, COL_SHIELD, e.unlocked, infoT);
    by += 62.0f;
    DrawGauge(bx, by, gaugeW, LABEL_ENERGY, e.energy, COL_ENERGY, e.unlocked, infoT);
    by += 66.0f;

    // ---- RANK row: label + ASCII rank letter (no rank-letter atlas in this set) ----
    {
        SetFont(g_fSeurat);
        DrawText({ bx, by }, 23.0f, WithAlpha(COL_DESC, infoT), "RANK");
        SetFont(g_fRodin);
        if (e.unlocked && e.rank >= 0 && e.rank <= 5) {
            DrawTextAligned({ bx + 120.0f, by - 10.0f }, { ix + INFO_W - 24.0f, by + 34.0f }, 40.0f,
                            WithAlpha(COL_TITLE, infoT), RANK_NAME[e.rank], Align::Right, true, true);
        } else {
            DrawTextAligned({ bx + 120.0f, by - 4.0f }, { ix + INFO_W - 24.0f, by + 28.0f }, 26.0f,
                            WithAlpha(COL_LOCKED, infoT), "--", Align::Right, true, true);
        }
    }

    // ---- START prompt: a pulsing call-to-action at the panel's foot ----
    if (infoT > 0.5f) {
        float pulse = 0.55f + 0.45f * (float)(0.5 + 0.5 * std::sin(g_open * 4.0));
        uint32_t pc = e.unlocked ? COL_START : COL_LOCKED;
        SetFont(g_fRodin);
        DrawTextAligned({ ix + 20.0f, iy + INFO_H - 56.0f }, { ix + INFO_W - 20.0f, iy + INFO_H - 14.0f },
                        34.0f, WithAlpha(pc, infoT * (e.unlocked ? pulse : 0.7f)),
                        e.unlocked ? "PRESS  (A)  TO START" : "EX STAGE LOCKED",
                        Align::Center, true, true);
    }

    // ===== FOOTER (real button glyphs + ASCII labels) ========================
    {
        float hx = TITLE_X, hcy = 648.0f;
        const float aAsp = 0.921f, lrAsp = 1.763f;
        DrawRect({ TITLE_X, 614.0f }, { 1220.0f, 616.0f }, WithAlpha(COL_RULE, footT));
        hx = DrawHint(hx, hcy, GLYPH_A,  aAsp,  "Start",        footT);
        hx = DrawHint(hx, hcy, GLYPH_B,  aAsp,  "Back",         footT);
        hx = DrawHint(hx, hcy, GLYPH_LB, lrAsp, "",             footT);
        hx = DrawHint(hx, hcy, GLYPH_RB, lrAsp, "Switch Trial", footT);
    }

    // ===== transient "START!" flash ==========================================
    double age = Now() - g_startStart;
    if (g_startStart > 0.0 && age < 1.4) {
        float ma = std::min(1.0f, (float)((1.4 - age) / 0.4));
        char line[80];
        if (e.unlocked) std::snprintf(line, sizeof(line), "Starting %s...", e.name);
        else            std::snprintf(line, sizeof(line), "%s is locked!", e.name);
        SetFont(g_fSeurat);
        DrawTextAligned({ SHOT_X, 572.0f }, { 1220.0f, 604.0f }, 24.0f,
                        WithAlpha(e.unlocked ? COL_OK : COL_LOCKED, ma), line, Align::Center, true, true);
    }

    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void ExStageInit() { Init(); }
void ExStageDraw(double openSeconds) { Draw(openSeconds); }
void ExStageInput(const ScreenInput& in) { Input(in); }
void ExStageReset() { Reset(); }
