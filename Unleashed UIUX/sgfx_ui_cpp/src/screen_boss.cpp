// =============================================================================
// screen_boss.cpp â€” the Boss intro / "versus" screen, re-authored as clean
// hand-written C++ in the UnleashedRecomp ui/options_menu idiom (NOT a CSD node
// dump â€” the raw boss.json packs broken stretch-arms at x=-1308 / w=1363 for the
// chrome plate, which we ignore). Layout lives in named 1280x720 constants; the
// name-plate is the REAL Sonic Unleashed silver-chrome window (DrawGameWindow),
// and the distinctive game art that makes this read as retail is the REAL
// extracted atlases, each placed at a deliberate, centred, aspect-preserved rect:
//
//   * the boss NAME wordmarks  (mat_boss_en_001, a 512x512 atlas of 11 italic
//     chrome banners: EGG / BEETLE / DEVIL RAY / LANCER / DRAGON / CAULDRON /
//     DARK GAIA / PHOENIX / MORAY / GUARDIAN / PERFECT â€” combined into boss names),
//   * the red "BOSS" label     (mat_boss_en_003, a 128x64 wordmark) â€” the subtitle.
//
// A procedural boss HP bar (the real chrome bar tinted red, with a gold fill)
// sits under the name, and a pulsing START prompt invites the fight.
//
// Fully interactive + stateful like options_menu / status:
//   * Q/E (LB/RB) cycle the BOSS  -> name banner, subtitle and HP all change,
//   * (A) "starts" the battle (a transient BATTLE START flash),
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
// mat_boss_en_001 (512x512) packs 11 italic silver-chrome boss-name banners,
// one per ~38px row; mat_boss_en_003 (128x64) is the small red "BOSS" wordmark.
const char* const ASSET_BASE = "assets/boss/";
int g_nameTex  = -1;   // mat_boss_en_001
int g_labelTex = -1;   // mat_boss_en_003

// real game fonts (MSDF sweep): DFSoGei titles, Seurat labels, Rodin values
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

constexpr float NAME_TEX_W  = 512.0f, NAME_TEX_H  = 512.0f;
constexpr float LABEL_TEX_W = 128.0f, LABEL_TEX_H = 64.0f;

// Tight opaque sub-rects measured from the atlas alpha (band scan of en_001):
//   0 EGG       1 BEETLE    2 DEVIL RAY  3 LANCER   4 DRAGON   5 CAULDRON
//   6 DARK GAIA 7 PHOENIX   8 MORAY      9 GUARDIAN 10 PERFECT
const UV NAME_UV[11] = {
    { 0.00195f, 0.00195f, 0.27539f, 0.07617f },  // 0  EGG
    { 0.00195f, 0.08008f, 0.51367f, 0.15430f },  // 1  BEETLE
    { 0.00195f, 0.15820f, 0.66016f, 0.23242f },  // 2  DEVIL RAY
    { 0.00195f, 0.23633f, 0.51367f, 0.31055f },  // 3  LANCER
    { 0.00195f, 0.31445f, 0.59766f, 0.38867f },  // 4  DRAGON
    { 0.00195f, 0.39258f, 0.66992f, 0.46680f },  // 5  CAULDRON
    { 0.00195f, 0.47070f, 0.66992f, 0.54492f },  // 6  DARK GAIA
    { 0.00195f, 0.54883f, 0.55664f, 0.62305f },  // 7  PHOENIX
    { 0.00000f, 0.62695f, 0.43555f, 0.70117f },  // 8  MORAY
    { 0.00000f, 0.70508f, 0.61914f, 0.77930f },  // 9  GUARDIAN
    { 0.00586f, 0.78320f, 0.59375f, 0.85742f },  // 10 PERFECT
};
// red "BOSS" wordmark â€” tight box in mat_boss_en_003.
const UV LABEL_UV = { 0.01562f, 0.03125f, 0.51562f, 0.31250f };

// A boss name can be one wordmark (DARK GAIA) or two stacked side-by-side
// (EGG + BEETLE -> "EGG BEETLE"). nameB == -1 means single-word.
struct Boss {
    const char* ascii;     // ASCII fallback / readout label
    int   nameA;            // first wordmark row (always set)
    int   nameB;            // second wordmark row, or -1
    int   hpMax;            // HP for the procedural gauge
    const char* tag;        // small flavour subtitle under the banner
};
const Boss BOSSES[] = {
    { "EGG BEETLE", 0, 1,  6, "DR. EGGMAN - APOTOS"          },
    { "DARK GAIA",  6, -1, 12, "AWAKENED - PLANET'S CORE"    },
    { "DRAGON",     4, -1,  8, "EGG DRAGOON - EGGMANLAND"     },
    { "PHOENIX",    7, -1,  7, "DEVIL RAY - HOLOSKA SKIES"    },
    { "GUARDIAN",   9, -1, 10, "DARK GUARDIAN - SHAMAR"       },
    { "PERFECT",   10, -1, 14, "PERFECT DARK GAIA - FINALE"   },
};
constexpr int BOSS_COUNT = (int)(sizeof(BOSSES) / sizeof(BOSSES[0]));

// ---- layout (reference px) --------------------------------------------------
constexpr float TITLE_X = 150.0f, TITLE_Y = 50.0f, RULE_Y = 118.0f;

// the central chrome name-plate (real silver-chrome window, bounded)
constexpr float PLATE_X = 200.0f, PLATE_Y = 250.0f, PLATE_W = 880.0f, PLATE_H = 200.0f;

// the boss NAME banner is fit-boxed inside the upper portion of the plate
constexpr float NAME_BOX_X = PLATE_X + 60.0f, NAME_BOX_Y = PLATE_Y + 34.0f;
constexpr float NAME_BOX_W = PLATE_W - 120.0f, NAME_BOX_H = 108.0f;

// the red "BOSS" subtitle label sits just above the plate, left-aligned
constexpr float LABEL_X = PLATE_X + 8.0f, LABEL_Y = PLATE_Y - 56.0f, LABEL_H = 46.0f;

// procedural HP bar, beneath the name-plate
constexpr float HP_X = PLATE_X + 60.0f, HP_Y = PLATE_Y + PLATE_H + 30.0f;
constexpr float HP_W = PLATE_W - 120.0f, HP_H = 34.0f;

// ---- entrance tuning (frames @60fps), tuned to the shop/status feel ----------
constexpr double TITLE_FRAMES = 14.0;
constexpr double LABEL_OFFSET = 4.0,  LABEL_FRAMES = 12.0;
constexpr double PLATE_FRAMES = 16.0;
constexpr double NAME_OFFSET  = 6.0,  NAME_FRAMES  = 16.0;
constexpr double HP_OFFSET    = 14.0, HP_FRAMES    = 16.0;
constexpr double FOOT_OFFSET  = 22.0, FOOT_FRAMES  = 12.0;
constexpr double SWITCH_FRAMES = 10.0;   // banner re-settle when the boss changes

// ---- palette (shared with pause/result/shop/status) -------------------------
const uint32_t COL_BG_TOP   = RGBA(18, 8, 10, 255);     // a darker, redder vignette for the fight
const uint32_t COL_BG_BOT   = RGBA(4, 3, 6, 255);
const uint32_t COL_TITLE    = RGBA(255, 209, 74, 255);  // gold (shared with all screens)
const uint32_t COL_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t COL_DESC     = RGBA(190, 200, 216, 255);
const uint32_t COL_RULE     = RGBA(230, 120, 120, 90);  // reddish rule under the title
const uint32_t COL_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t COL_HP_FILL_T= RGBA(255, 120, 96, 255);  // red-orange HP fill (top)
const uint32_t COL_HP_FILL_B= RGBA(196, 40, 40, 255);   // red HP fill (bottom)
const uint32_t COL_HP_BACK  = RGBA(28, 16, 18, 235);     // empty HP trough
const uint32_t COL_HP_LABEL = RGBA(255, 224, 170, 255);
const uint32_t COL_PROMPT   = RGBA(255, 236, 170, 255);  // pulsing START prompt
const uint32_t COL_FOOTER   = RGBA(206, 196, 200, 220);
const uint32_t COL_FLASH    = RGBA(255, 90, 70, 255);    // BATTLE START wash

// ---- interactive state ------------------------------------------------------
int    g_boss = 0, g_prevBoss = 0;
double g_switchStart = -100.0;   // Now() when the boss last changed
double g_flashStart  = -100.0;   // Now() of the last (A) "battle start"

const Boss& Cur() { return BOSSES[g_boss]; }

void Init() {
    if (g_nameTex  < 0) g_nameTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_boss_en_001.png");
    if (g_labelTex < 0) g_labelTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_boss_en_003.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF (im_font_atlas)
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");    // real game MSDF
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");   // real DFSoGeiStd-W7 (titles)
}

void Reset() {
    g_boss = 0; g_prevBoss = 0;
    g_switchStart = -100.0; g_flashStart = -100.0;
}

void Input(const ScreenInput& in) {
    if (in.tabLeft || in.tabRight) {
        g_prevBoss = g_boss;
        if (in.tabLeft)  g_boss = (g_boss + BOSS_COUNT - 1) % BOSS_COUNT;
        if (in.tabRight) g_boss = (g_boss + 1) % BOSS_COUNT;
        g_switchStart = Now();
    }
    if (in.accept) { g_flashStart = Now(); }   // "battle start" -> a transient flash
    // cancel: would back out in-game; no-op in the standalone build.
}

// aspect (w/h) of an en_001 banner sub-rect, so it is never stretched.
float NameAspect(const UV& u) {
    return ((u.u1 - u.u0) * NAME_TEX_W) / ((u.v1 - u.v0) * NAME_TEX_H);
}

// ---- the procedural boss HP bar ---------------------------------------------
// A bounded real chrome bar (DrawGameWindow, no header) as the trough, with a
// red-orange gradient fill inset inside it. `frac` in [0,1] fills left->right.
void DrawHpBar(float x, float y, float w, float h, float frac, float t) {
    // chrome trough (real frame), tinted dark
    DrawGameWindow({ x, y }, { x + w, y + h }, 0.0f, t, 0xFF3A2A2Cu);
    // inset empty backing
    const float pad = 5.0f;
    float ix = x + pad, iy = y + pad, iw = w - pad * 2.0f, ih = h - pad * 2.0f;
    DrawRect({ ix, iy }, { ix + iw, iy + ih }, WithAlpha(COL_HP_BACK, t));
    // red HP fill
    float fw = iw * std::clamp(frac, 0.0f, 1.0f);
    if (fw > 1.0f)
        DrawVGradient({ ix, iy }, { ix + fw, iy + ih },
                      WithAlpha(COL_HP_FILL_T, t), WithAlpha(COL_HP_FILL_B, t));
    // additive sheen along the top of the fill
    if (fw > 1.0f)
        DrawRect({ ix, iy }, { ix + fw, iy + ih * 0.45f }, WithAlpha(COL_WHITE, t * 0.18f), true);
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const Boss& b = Cur();
    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float labelT = (float)ComputeMotion(openSec, LABEL_OFFSET, LABEL_FRAMES);
    const float plateT = (float)ComputeMotion(openSec, 0.0, PLATE_FRAMES);
    const float nameT  = (float)ComputeMotion(openSec, NAME_OFFSET, NAME_FRAMES);
    const float hpT    = (float)ComputeMotion(openSec, HP_OFFSET, HP_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // banner re-settle ease when the boss is switched (Q/E)
    const float switchT = (float)ComputeMotion(g_switchStart, 0.0, SWITCH_FRAMES);

    // ===== TITLE =============================================================
    SetFont(g_fDF);
    DrawTextShadow({ TITLE_X, TITLE_Y - (1.0f - titleT) * 16.0f }, 46.0f,
                   WithAlpha(COL_TITLE, titleT), "BOSS BATTLE");
    DrawRect({ TITLE_X, RULE_Y }, { 1130.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));
    // boss index readout, top-right
    {
        char idx[24];
        std::snprintf(idx, sizeof(idx), "BOSS  %d / %d", g_boss + 1, BOSS_COUNT);
        SetFont(g_fRodin);
        DrawTextAligned({ 880.0f, TITLE_Y + 8.0f }, { 1130.0f, TITLE_Y + 44.0f }, 24.0f,
                        WithAlpha(COL_DESC, titleT), idx, Align::Right, true, true);
    }

    // ===== RED "BOSS" SUBTITLE LABEL (real art, above the plate) =============
    if (labelT > 0.0f) {
        float ly = LABEL_Y - (1.0f - labelT) * 10.0f;
        if (g_labelTex >= 0) {
            float aspect = ((LABEL_UV.u1 - LABEL_UV.u0) * LABEL_TEX_W) /
                           ((LABEL_UV.v1 - LABEL_UV.v0) * LABEL_TEX_H);
            float lh = LABEL_H, lw = lh * aspect;
            DrawImage(g_labelTex, { LABEL_X, ly }, { LABEL_X + lw, ly + lh },
                      { LABEL_UV.u0, LABEL_UV.v0 }, { LABEL_UV.u1, LABEL_UV.v1 },
                      WithAlpha(COL_WHITE, labelT));
        } else {
            SetFont(g_fDF);
            DrawTextShadow({ LABEL_X, ly + 6.0f }, 34.0f,
                           WithAlpha(RGBA(232, 64, 56, 255), labelT), "BOSS");
        }
    }

    // ===== CENTRAL CHROME NAME-PLATE (real silver-chrome window) =============
    const float plateSlide = (1.0f - plateT) * 22.0f;
    const float px = PLATE_X, py = PLATE_Y + plateSlide;
    DrawGameWindow({ px, py }, { px + PLATE_W, py + PLATE_H }, 0.0f, plateT);
    // a thin reddish rule across the top, echoing the title rule
    DrawRect({ px + 16.0f, py + 14.0f }, { px + PLATE_W - 16.0f, py + 16.0f },
             WithAlpha(COL_RULE, plateT));

    // ===== BOSS NAME BANNER (real art, fit-boxed, centred) ===================
    // Combine one or two italic wordmarks side-by-side; scale to fit the name box
    // (preserving aspect), centre horizontally, and pop with a tiny zoom on switch.
    if (g_nameTex >= 0 && nameT > 0.0f) {
        const float nbx = NAME_BOX_X, nby = NAME_BOX_Y + plateSlide;
        const float nbw = NAME_BOX_W, nbh = NAME_BOX_H;
        const float gap = 26.0f;        // space between two stacked wordmarks

        const UV& uA = NAME_UV[b.nameA];
        bool two = (b.nameB >= 0);
        const UV  uB = two ? NAME_UV[b.nameB] : UV{ 0, 0, 0, 0 };

        float aspA = NameAspect(uA);
        float aspB = two ? NameAspect(uB) : 0.0f;

        // choose one common banner height so both words share a baseline scale,
        // then fit the total width into the box.
        float wantH = nbh;                                  // try full box height
        float totalW = aspA * wantH + (two ? gap + aspB * wantH : 0.0f);
        if (totalW > nbw) { float k = nbw / totalW; wantH *= k; totalW *= k; }

        // entrance zoom (settle to 1.0) + tiny pop when switching bosses
        float zoom = 0.82f + 0.18f * nameT;
        float popExtra = (g_switchStart > 0.0) ? (1.0f - switchT) * 0.10f : 0.0f;
        zoom = std::min(1.0f, zoom) + popExtra;             // settle <=1, small overshoot on switch
        float drawH = wantH * zoom;
        float drawTotalW = totalW * zoom;

        float startX = nbx + (nbw - drawTotalW) * 0.5f;
        float cy = nby + nbh * 0.5f;
        uint32_t col = WithAlpha(COL_WHITE, nameT);

        float wA = aspA * drawH;
        DrawImage(g_nameTex, { startX, cy - drawH * 0.5f }, { startX + wA, cy + drawH * 0.5f },
                  { uA.u0, uA.v0 }, { uA.u1, uA.v1 }, col);
        if (two) {
            float bx = startX + wA + gap * zoom;
            float wB = aspB * drawH;
            DrawImage(g_nameTex, { bx, cy - drawH * 0.5f }, { bx + wB, cy + drawH * 0.5f },
                      { uB.u0, uB.v0 }, { uB.u1, uB.v1 }, col);
        }
    } else if (nameT > 0.0f) {
        SetFont(g_fDF);
        DrawTextAligned({ NAME_BOX_X, NAME_BOX_Y + plateSlide },
                        { NAME_BOX_X + NAME_BOX_W, NAME_BOX_Y + plateSlide + NAME_BOX_H },
                        72.0f, WithAlpha(COL_WHITE, nameT), b.ascii, Align::Center, true, true);
    }

    // flavour subtitle in the lower band of the plate
    if (nameT > 0.0f) {
        SetFont(g_fSeurat);
        DrawTextAligned({ px + 40.0f, py + PLATE_H - 52.0f }, { px + PLATE_W - 40.0f, py + PLATE_H - 14.0f },
                        24.0f, WithAlpha(COL_DESC, nameT), b.tag, Align::Center, true, true);
    }

    // ===== PROCEDURAL HP BAR =================================================
    if (hpT > 0.0f) {
        const float hy = HP_Y + (1.0f - hpT) * 14.0f;
        SetFont(g_fRodin);
        DrawTextAligned({ HP_X, hy - 30.0f }, { HP_X + 140.0f, hy }, 22.0f,
                        WithAlpha(COL_HP_LABEL, hpT), "BOSS HP", Align::Left, true, true);
        DrawHpBar(HP_X, hy, HP_W, HP_H, 1.0f, hpT);   // full HP at the intro
        // segment ticks to read as the game's notched gauge
        int seg = std::max(1, b.hpMax);
        for (int i = 1; i < seg; ++i) {
            float tx = HP_X + 5.0f + (HP_W - 10.0f) * ((float)i / (float)seg);
            DrawRect({ tx, hy + 5.0f }, { tx + 1.0f, hy + HP_H - 5.0f },
                     WithAlpha(RGBA(10, 6, 8, 200), hpT));
        }
    }

    // ===== PULSING START PROMPT ==============================================
    if (footT > 0.0f) {
        // sin-like pulse via a triangle wave on Now()
        double tnow = Now();
        float phase = (float)(tnow - (long)tnow);           // 0..1 each second
        float pulse = 0.55f + 0.45f * (1.0f - std::abs(phase * 2.0f - 1.0f));
        SetFont(g_fRodin);
        DrawTextAligned({ TITLE_X, 600.0f }, { 1130.0f, 644.0f }, 30.0f,
                        WithAlpha(COL_PROMPT, footT * pulse), "PRESS  (A)  TO START",
                        Align::Center, true, true);
    }

    // ===== FOOTER ============================================================
    if (footT > 0.0f) {
        SetFont(g_fRodin);
        DrawTextAligned({ TITLE_X, 660.0f }, { 1130.0f, 700.0f }, 22.0f,
                        WithAlpha(COL_FOOTER, footT),
                        "(A) Start   (Q/E) Change Boss   (B) Back", Align::Left, true, true);
    }

    // ===== TRANSIENT "BATTLE START" FLASH ====================================
    double age = Now() - g_flashStart;
    if (g_flashStart > 0.0 && age < 0.9) {
        float a = (float)std::max(0.0, (0.9 - age) / 0.9);
        // a red full-screen wash that fades, plus the words
        DrawRect({ 0, 0 }, { REF_W, REF_H }, WithAlpha(COL_FLASH, a * 0.35f), true);
        float za = std::min(1.0f, a * 1.6f);
        SetFont(g_fDF);
        DrawTextAligned({ 0, 300.0f }, { REF_W, 420.0f }, 80.0f,
                        WithAlpha(COL_WHITE, za), "BATTLE START", Align::Center, true, true);
    }

    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void BossInit() { Init(); }
void BossDraw(double openSeconds) { Draw(openSeconds); }
void BossInput(const ScreenInput& in) { Input(in); }
void BossReset() { Reset(); }
