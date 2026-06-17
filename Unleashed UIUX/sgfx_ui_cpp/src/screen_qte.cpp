// =============================================================================
// screen_qte.cpp Ã¢â‚¬â€ the Quick-Time-Event prompt, re-authored as clean hand-written
// C++ in the UnleashedRecomp ui/options_menu idiom (NOT a CSD node dump Ã¢â‚¬â€ the raw
// qte.json is a pile of low-alpha ghost copies of the same button glyph). This is
// a dramatic, centered prompt:
//   * a LARGE green (A) button glyph (REAL Xbox art, mat_comon_x360_001), pulsing,
//   * a shrinking circular TIMING ring around it (procedural segments deplete),
//   * a shrinking horizontal timing bar in a real chrome track (DrawGameWindow),
//   * a pulsing gold "PRESS!" wordmark and a danger/bomb accent (mat_qte_001),
//   * REAL result wordmarks on resolve: "GREAT!" (success, green flash) /
//     "YOU FAILED" (timeout or cancel, red flash) from mat_qte_en_001.
//
// The QTE runs on a continuous time loop driven by ui::Now() (sin/fmod), so the
// screen always animates: a ~2.0s window counts down; (A) succeeds, (B) or a
// timeout fails; after a short resolve hold it auto-rearms for a fresh prompt.
//
// Distinctive art is the REAL extracted atlas where one exists; procedural rings/
// bars are the bounded fallback when an atlas slot is missing (-1), exactly as the
// shop/world_map DrawWindow does.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <string>

using namespace ui;

namespace {

// ---- per-element UV sub-rect (normalized atlas coords) ----------------------
struct UV { float u0, v0, u1, v1; };

// ---- real-art atlases -------------------------------------------------------
const char* const ASSET_BASE = "assets/qte/";
int g_glyphTex = -1;   // mat_comon_x360_001 (512x512) Ã¢â‚¬â€ Xbox button glyphs
int g_dangerTex = -1;  // mat_qte_001        (128x128) Ã¢â‚¬â€ bomb / hazard triangle
int g_wordTex   = -1;  // mat_qte_en_001     (256x256) Ã¢â‚¬â€ NICE/GREAT!/COOL!!/YOU FAILED

// ---- fonts (real game faces) -------------------------------------------------
int g_fRodin = 0, g_fDF = 0;

constexpr float GLYPH_TEX_W = 512.0f, GLYPH_TEX_H = 512.0f;
constexpr float DANGER_TEX_W = 128.0f, DANGER_TEX_H = 128.0f;
constexpr float WORD_TEX_W = 256.0f, WORD_TEX_H = 256.0f;

// LARGE green (A) button Ã¢â‚¬â€ tight opaque box measured from the atlas alpha
// (lower-left big-button grid). aspect w/h ~= 1.034.
const UV GLYPH_A_BIG = { 0.00195f, 0.54883f, 0.12109f, 0.66406f };
// small footer glyphs (top-row grid)
const UV GLYPH_A_SM  = { 0.0000f, 0.00195f, 0.07227f, 0.07617f };
const UV GLYPH_B_SM  = { 0.07227f, 0.00195f, 0.14258f, 0.07617f }; // B sits right of A, same row
const UV GLYPH_LB_SM = { 0.34766f, 0.00977f, 0.48828f, 0.07812f };
const UV GLYPH_RB_SM = { 0.50391f, 0.00977f, 0.61719f, 0.07812f };

// danger / bomb triangle Ã¢â‚¬â€ fills almost the whole 128x128 atlas
const UV DANGER_UV = { 0.0078f, 0.0078f, 0.9844f, 0.9531f };

// result wordmark bands (measured): 0=NICE 1=GREAT! 2=COOL!! 3=YOU FAILED
const UV WORD_GREAT  = { 0.01562f, 0.19141f, 0.98828f, 0.37500f };  // success
const UV WORD_FAILED = { 0.00391f, 0.57422f, 0.98828f, 0.75391f };  // failure

// ---- QTE timing (seconds, wall-clock loop) ----------------------------------
constexpr double WINDOW_SEC  = 2.0;    // length of the press window (ring/bar deplete)
constexpr double RESOLVE_SEC = 1.4;    // how long the result wordmark + flash hold
constexpr double AUTO_PRESS  = 1.05;   // if the player does nothing, demo auto-presses here
constexpr int    RING_SEGS   = 32;     // procedural timing-ring segment count

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double BG_FRAMES   = 12.0;
constexpr double PROMPT_OFF  = 4.0,  PROMPT_FRAMES = 16.0;
constexpr double FOOT_OFFSET = 10.0, FOOT_FRAMES = 12.0;

// ---- palette ----------------------------------------------------------------
const uint32_t COL_DIM     = RGBA(0, 0, 0, 170);          // dramatic gameplay dim
const uint32_t COL_VIGN    = RGBA(0, 0, 0, 130);          // edge vignette band
const uint32_t COL_TITLE   = RGBA(255, 209, 74, 255);     // Unleashed gold
const uint32_t COL_WHITE   = RGBA(255, 255, 255, 255);
const uint32_t COL_RING_HI = RGBA(255, 224, 120, 255);    // ring "remaining" (gold)
const uint32_t COL_RING_LO = RGBA(120, 70, 30, 200);      // ring "spent" (dim)
const uint32_t COL_RING_WARN = RGBA(255, 96, 72, 255);    // ring when time nearly out
const uint32_t COL_BAR_FILL_T = RGBA(120, 200, 255, 255);
const uint32_t COL_BAR_FILL_B = RGBA(40, 120, 210, 255);
const uint32_t COL_BAR_WARN_T = RGBA(255, 150, 80, 255);
const uint32_t COL_BAR_WARN_B = RGBA(210, 70, 40, 255);
const uint32_t COL_FLASH_OK   = RGBA(80, 235, 130, 255);  // success flash
const uint32_t COL_FLASH_NO   = RGBA(235, 70, 60, 255);   // fail flash
const uint32_t COL_OK_TEXT    = RGBA(120, 235, 150, 255);
const uint32_t COL_NO_TEXT    = RGBA(245, 110, 100, 255);
const uint32_t COL_FOOTER     = RGBA(200, 214, 232, 220);

// ---- layout (reference px) --------------------------------------------------
const float CX = REF_W * 0.5f;              // 640
const float CY = REF_H * 0.5f - 12.0f;      // ~348, slightly above center
const float BTN_SIZE   = 150.0f;            // large (A) glyph box (square)
const float RING_R     = 118.0f;            // timing-ring radius
const float RING_W     = 12.0f;             // ring thickness
const float BAR_W      = 420.0f;            // shrinking timing bar
const float BAR_H      = 26.0f;
const float BAR_Y      = CY + 150.0f;       // below the button

// ---- QTE state (continuous demo loop) ---------------------------------------
enum Phase { ARMED = 0, SUCCESS = 1, FAILED = 2 };
int    g_phase     = ARMED;
double g_armStart  = -1.0;   // Now() when the current press window opened
double g_resolveAt = -1.0;   // Now() when the QTE resolved (success/fail)

void Init() {
    if (g_glyphTex  < 0) g_glyphTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_x360_001.png");
    if (g_dangerTex < 0) g_dangerTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_qte_001.png");
    if (g_wordTex   < 0) g_wordTex   = gfx::loadTexture(std::string(ASSET_BASE) + "mat_qte_en_001.png");
    if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db");                  // real game MSDF
    if (g_fDF    == 0) g_fDF    = LoadMsdfFont("dfsogei");      // real DFSoGeiStd-W7 (titles)
}

void Reset() { g_phase = ARMED; g_armStart = -1.0; g_resolveAt = -1.0; }

// advance the continuous loop: arm a fresh window, auto-press, auto-resolve
void Tick() {
    double now = Now();
    if (g_armStart < 0.0) { g_armStart = now; g_phase = ARMED; }   // first frame

    if (g_phase == ARMED) {
        double elapsed = now - g_armStart;
        if (elapsed >= WINDOW_SEC) {            // ran out of time -> fail
            g_phase = FAILED; g_resolveAt = now;
        } else if (elapsed >= AUTO_PRESS) {     // demo auto-presses if the player idles
            g_phase = SUCCESS; g_resolveAt = now;
        }
    } else {
        if (now - g_resolveAt >= RESOLVE_SEC) { // re-arm a fresh prompt
            g_armStart = now; g_phase = ARMED; g_resolveAt = -1.0;
        }
    }
}

void Input(const ScreenInput& in) {
    double now = Now();
    if (g_phase == ARMED) {
        double elapsed = (g_armStart < 0.0) ? 0.0 : (now - g_armStart);
        if (in.accept) {                            // (A) pressed in time -> success
            g_phase = (elapsed <= WINDOW_SEC) ? SUCCESS : FAILED;
            g_resolveAt = now;
        } else if (in.cancel) {                     // (B) -> deliberately fail
            g_phase = FAILED; g_resolveAt = now;
        }
    } else if (in.accept || in.cancel) {            // dismiss the result early -> re-arm
        g_armStart = now; g_phase = ARMED; g_resolveAt = -1.0;
    }
}

// ---- aspect-preserving image fit into a box ---------------------------------
void DrawFitted(int tex, const UV& uv, float texW, float texH,
                float bx, float by, float bw, float bh,
                uint32_t col, bool additive = false) {
    if (tex < 0) return;
    float artW = (uv.u1 - uv.u0) * texW;
    float artH = (uv.v1 - uv.v0) * texH;
    if (artW <= 0.0f || artH <= 0.0f) return;
    float scale = std::min(bw / artW, bh / artH);
    float w = artW * scale, h = artH * scale;
    float x = bx + (bw - w) * 0.5f;
    float y = by + (bh - h) * 0.5f;
    DrawImage(tex, { x, y }, { x + w, y + h }, { uv.u0, uv.v0 }, { uv.u1, uv.v1 }, col, additive);
}

// ---- a procedural circular timing ring around the button --------------------
// `frac` in [0,1] = portion of time REMAINING. Filled segments (gold/warn) shrink
// clockwise as time runs out; spent segments dim. Each segment is a small quad
// placed on the circle (good enough to read as a depleting ring at this scale).
void DrawTimingRing(float cx, float cy, float r, float thick, float frac, float alpha, bool warn) {
    const double TWO_PI = 6.28318530718;
    uint32_t lit = warn ? COL_RING_WARN : COL_RING_HI;
    int filled = (int)std::ceil(frac * RING_SEGS);
    for (int i = 0; i < RING_SEGS; ++i) {
        // start at top (12 o'clock), go clockwise
        double a0 = -TWO_PI * 0.25 + (double)i / RING_SEGS * TWO_PI;
        float sx = cx + r * (float)std::cos(a0);
        float sy = cy + r * (float)std::sin(a0);
        bool on = (i < filled);
        float half = thick * 0.5f + (on ? 2.0f : 0.0f);
        // small square segment centred on the ring path
        uint32_t c = on ? lit : COL_RING_LO;
        DrawRect({ sx - half, sy - half }, { sx + half, sy + half }, WithAlpha(c, alpha), on);
    }
}

void Draw(double openSec) {
    Tick();

    const float bgT     = (float)ComputeMotion(openSec, 0.0, BG_FRAMES);
    const float promptT = (float)ComputeMotion(openSec, PROMPT_OFF, PROMPT_FRAMES);
    const float footT   = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);
    const double now = Now();

    // ---- dramatic dim + top/bottom vignette bands (gameplay overlay) ----
    DrawRect({ 0, 0 }, { REF_W, REF_H }, WithAlpha(COL_DIM, bgT));
    DrawVGradient({ 0, 0 }, { REF_W, 150.0f }, WithAlpha(COL_VIGN, bgT), WithAlpha(COL_VIGN, 0.0f));
    DrawVGradient({ 0, REF_H - 150.0f }, { REF_W, REF_H }, WithAlpha(COL_VIGN, 0.0f), WithAlpha(COL_VIGN, bgT));

    // time remaining fraction (only meaningful while ARMED)
    double elapsed = (g_armStart < 0.0) ? 0.0 : (now - g_armStart);
    float remain = (g_phase == ARMED)
                       ? std::clamp(1.0f - (float)(elapsed / WINDOW_SEC), 0.0f, 1.0f)
                       : (g_phase == SUCCESS ? 1.0f : 0.0f);
    bool warn = (g_phase == ARMED) && (remain < 0.34f);

    // continuous pulse for the prompt (speeds up as time runs out)
    float pulseHz = warn ? 6.0f : 3.0f;
    float pulse = 0.5f + 0.5f * (float)std::sin(now * pulseHz * 6.28318530718 * 0.5);
    float btnScale = 1.0f + 0.10f * pulse;                 // breathing button
    float ringA = promptT * (0.85f + 0.15f * pulse);

    // ===== resolve flash (full-screen tint on success/fail) ==================
    if (g_phase != ARMED) {
        float age = (float)(now - g_resolveAt);
        float flash = std::clamp(1.0f - age / 0.30f, 0.0f, 1.0f);   // quick burst
        if (flash > 0.0f) {
            uint32_t fc = (g_phase == SUCCESS) ? COL_FLASH_OK : COL_FLASH_NO;
            DrawRect({ 0, 0 }, { REF_W, REF_H }, WithAlpha(fc, flash * 0.45f), true);
        }
    }

    // ===== centered prompt ===================================================
    if (g_phase == ARMED) {
        // timing ring (procedural) Ã¢â‚¬â€ depletes around the button
        DrawTimingRing(CX, CY, RING_R, RING_W, remain, ringA, warn);

        // danger / bomb accent above the button (real art), gently bobbing
        if (g_dangerTex >= 0) {
            float bob = 6.0f * (float)std::sin(now * 2.2);
            float dz = 64.0f;
            DrawFitted(g_dangerTex, DANGER_UV, DANGER_TEX_W, DANGER_TEX_H,
                       CX - dz * 0.5f, CY - RING_R - 108.0f + bob, dz, dz,   // clear of PRESS!
                       WithAlpha(COL_WHITE, promptT));
        }

        // LARGE green (A) button (real art), pulsing about its centre
        if (g_glyphTex >= 0) {
            float aspect = ((GLYPH_A_BIG.u1 - GLYPH_A_BIG.u0) * GLYPH_TEX_W) /
                           ((GLYPH_A_BIG.v1 - GLYPH_A_BIG.v0) * GLYPH_TEX_H);
            float h = BTN_SIZE * btnScale;
            float w = h * aspect;
            DrawImage(g_glyphTex, { CX - w * 0.5f, CY - h * 0.5f }, { CX + w * 0.5f, CY + h * 0.5f },
                      { GLYPH_A_BIG.u0, GLYPH_A_BIG.v0 }, { GLYPH_A_BIG.u1, GLYPH_A_BIG.v1 },
                      WithAlpha(COL_WHITE, promptT));
        } else {
            // procedural ringed button fallback
            float rr = BTN_SIZE * 0.5f * btnScale;
            DrawRect({ CX - rr, CY - rr }, { CX + rr, CY + rr }, WithAlpha(RGBA(40, 170, 70, 255), promptT));
            DrawTextAligned({ CX - rr, CY - rr }, { CX + rr, CY + rr }, 90.0f * btnScale,
                            WithAlpha(COL_WHITE, promptT), "A", Align::Center, true, true);
        }

        // pulsing gold "PRESS!" above the ring
        {
            float ts = 44.0f + 6.0f * pulse;
            uint32_t pc = warn ? COL_RING_WARN : COL_TITLE;
            SetFont(g_fDF);
            DrawTextAligned({ CX - 260.0f, CY - RING_R - 30.0f }, { CX + 260.0f, CY - RING_R + 6.0f },
                            ts, WithAlpha(pc, promptT * (0.8f + 0.2f * pulse)),
                            "PRESS!", Align::Center, true, true);
        }

        // ---- shrinking horizontal timing bar (real chrome track) ----
        const float bx = CX - BAR_W * 0.5f, by = BAR_Y;
        DrawGameWindow({ bx, by }, { bx + BAR_W, by + BAR_H }, 0.0f, promptT);   // chrome track
        float fillW = (BAR_W - 8.0f) * remain;
        if (fillW > 0.0f) {
            DrawVGradient({ bx + 4.0f, by + 4.0f }, { bx + 4.0f + fillW, by + BAR_H - 4.0f },
                          WithAlpha(warn ? COL_BAR_WARN_T : COL_BAR_FILL_T, promptT),
                          WithAlpha(warn ? COL_BAR_WARN_B : COL_BAR_FILL_B, promptT));
        }
    } else {
        // ===== resolved: real result wordmark, scaling/fading in ============
        float age = (float)(now - g_resolveAt);
        float pop = std::clamp(age / 0.18f, 0.0f, 1.0f);          // quick scale-in
        float scale = 0.6f + 0.4f * (float)std::sqrt(pop);
        float fade  = std::clamp(1.0f - (age - (float)RESOLVE_SEC + 0.4f) / 0.4f, 0.0f, 1.0f);
        const UV& wm = (g_phase == SUCCESS) ? WORD_GREAT : WORD_FAILED;
        bool ok = (g_phase == SUCCESS);

        if (g_wordTex >= 0) {
            float boxW = 540.0f * scale, boxH = 150.0f * scale;
            DrawFitted(g_wordTex, wm, WORD_TEX_W, WORD_TEX_H,
                       CX - boxW * 0.5f, CY - boxH * 0.5f, boxW, boxH,
                       WithAlpha(COL_WHITE, fade));
        } else {
            SetFont(g_fDF);
            DrawTextAligned({ CX - 300.0f, CY - 40.0f }, { CX + 300.0f, CY + 40.0f },
                            70.0f * scale, WithAlpha(ok ? COL_OK_TEXT : COL_NO_TEXT, fade),
                            ok ? "GREAT!" : "FAILED", Align::Center, true, true);
        }
    }

    // ===== footer hint (real button glyph + press prompt) ====================
    // Single in-game prompt: the (A) button followed by "Press". No "(B) Fail"
    // entry Ã¢â‚¬â€ that was a debug/harness label; the game never tells you to fail.
    {
        const float gh = 30.0f, cy = REF_H - 46.0f;
        const float aAsp = 0.976f;   // A glyph ~square
        SetFont(g_fRodin);
        const char* label = "Press";
        float gw = gh * aAsp;
        float lw = MeasureText(22.0f, label).x;
        // centre the glyph + label group along the footer
        float hx = CX - (gw + 8.0f + lw) * 0.5f;
        if (g_glyphTex >= 0) {
            DrawImage(g_glyphTex, { hx, cy - gh * 0.5f }, { hx + gw, cy + gh * 0.5f },
                      { GLYPH_A_SM.u0, GLYPH_A_SM.v0 }, { GLYPH_A_SM.u1, GLYPH_A_SM.v1 },
                      WithAlpha(COL_WHITE, footT));
            hx += gw + 8.0f;
        }
        DrawText({ hx, cy - 13.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), label);
    }
    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void QteInit() { Init(); }
void QteDraw(double openSeconds) { Draw(openSeconds); }
void QteInput(const ScreenInput& in) { Input(in); }
void QteReset() { Reset(); }
