// =============================================================================
// screen_start.cpp â€” the start-of-stage "READY ... 3 2 1 GO!" splash, re-authored
// as clean hand-written C++ in the UnleashedRecomp ui/options_menu idiom (NOT a CSD
// node dump â€” the raw start.json just stacks low-alpha shadow copies of one text
// band of mat_start_en_001). Layout lives in named 1280x720 constants over a dark
// vertical gradient.
//
// There is exactly ONE real atlas for this screen, mat_start_en_001 (512x512): the
// in-game start/result text-banner sheet that bakes "3 2 1", "GO!", "MISSION CLEAR!",
// "YOU FAILED!", "GAME OVER" and "READY" as gold-silver chrome wordmarks. It carries
// NO "PRESS START" string and NO logo, so the distinctive REAL art we place is the
// iconic "READY" wordmark as a centred hero plate, and the PRESS-START call-to-action
// is a procedural pulsing ASCII prompt (sin breathe on alpha) â€” "mostly procedural
// over the one real plate", clean and bold.
//
// Interactive + animated like the other screens:
//   * (A)/Start kicks off the real countdown sequence using the genuine wordmarks:
//     READY -> 3 -> 2 -> 1 -> GO!, each a scale-pop + fade driven by ui::Now(),
//   * while idle, the centred "READY" hero eases in and a "PRESS  START" prompt
//     breathes; (B) just re-arms.
//
// Bounded ASCII text is the graceful fallback when the atlas is missing (-1 slot),
// exactly as the shop/gate/world_map fallbacks do.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <string>
#include <algorithm>
#include <cmath>

using namespace ui;

namespace {

// ---- per-element UV sub-rect (normalized atlas coords) ----------------------
struct UV { float u0, v0, u1, v1; };

// ---- the single real atlas --------------------------------------------------
const char* const ASSET_BASE = "assets/start/";
int   g_bannerTex = -1;                 // mat_start_en_001 (512x512)
constexpr float TEX_W = 512.0f, TEX_H = 512.0f;

// Wordmark sub-rects measured from the alpha occupancy of mat_start_en_001 (tight
// opaque bbox -> normalized). Aspect = (du*TEX_W)/(dv*TEX_H).
const UV UV_READY = { 0.142578f, 0.580078f, 0.853516f, 0.718750f }; // px (73,297)-(437,368)  aspect 5.07
const UV UV_GO    = { 0.572266f, 0.001953f, 0.929688f, 0.140625f }; // px (293,1)-(476,72)    aspect 2.56
// individual countdown digits, split from the baked "3 2 1" band (y 1..72)
const UV UV_3 = { 0.001953f, 0.001953f, 0.173828f, 0.140625f };     // px (1,1)-(88,72)    aspect 1.22
const UV UV_2 = { 0.175781f, 0.001953f, 0.355469f, 0.140625f };     // px (90,1)-(181,72)  aspect 1.28
const UV UV_1 = { 0.357422f, 0.001953f, 0.453125f, 0.140625f };     // px (183,1)-(231,72) aspect 0.68

// ---- layout (reference px) --------------------------------------------------
constexpr float CX        = REF_W * 0.5f;          // 640
constexpr float HERO_CY   = 300.0f;                // vertical centre of the hero wordmark
constexpr float HERO_H    = 150.0f;                // hero target height (digits / GO!)
constexpr float READY_H   = 96.0f;                 // "READY" is wide -> a shorter target height
constexpr float PROMPT_CY = 520.0f;                // pulsing PRESS-START prompt
constexpr float FOOTER_CY = 670.0f;

// ---- entrance / animation tuning (frames @60fps) ----------------------------
constexpr double HERO_FRAMES   = 18.0;             // idle "READY" hero ease-in
constexpr double PROMPT_OFFSET = 8.0,  PROMPT_FRAMES = 14.0;
constexpr double FOOT_OFFSET   = 12.0, FOOT_FRAMES   = 12.0;

// countdown step timing (seconds), once (A) is pressed
constexpr double STEP_SEC   = 0.85;                // each of READY/3/2/1 holds this long
constexpr double GO_SEC     = 1.20;                // "GO!" lingers a touch longer
constexpr int    STEP_COUNT = 4;                   // READY, 3, 2, 1  (then GO!)

// ---- palette (shared with pause/result/shop/status/world_map) ---------------
const uint32_t COL_BG_TOP  = RGBA(12, 20, 38, 255);
const uint32_t COL_BG_BOT  = RGBA(5, 9, 18, 255);
const uint32_t COL_TITLE   = RGBA(255, 209, 74, 255);   // Unleashed gold
const uint32_t COL_RULE    = RGBA(120, 170, 230, 90);
const uint32_t COL_FOOTER  = RGBA(190, 205, 225, 220);
const uint32_t COL_PROMPT  = RGBA(255, 236, 150, 255);  // warm gold press-start
const uint32_t COL_WHITE   = RGBA(255, 255, 255, 255);
const uint32_t COL_FLASH   = RGBA(255, 255, 255, 255);  // additive GO! flash

// ---- interactive state ------------------------------------------------------
double g_seqStart = -100.0;   // Now() the countdown sequence began (-100 = idle)
double g_open     = 0.0;      // openSec captured each Draw (for the idle pulse)

// ---- fonts (DFSoGei SDF title face + real game MSDF) -------------------------
static int g_fRodin = 0, g_fDF = 0;

void Init() {
    if (g_bannerTex < 0)
        g_bannerTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_start_en_001.png");
    if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db");
    if (g_fDF    == 0) g_fDF    = LoadMsdfFont("dfsogei");
}

void Reset() { g_seqStart = -100.0; g_open = 0.0; }

void Input(const ScreenInput& in) {
    // (A)/Start kicks off (or restarts) the countdown sequence.
    if (in.accept) g_seqStart = Now();
    // (B)/cancel re-arms the splash (no destructive action in the standalone build).
    if (in.cancel) g_seqStart = -100.0;
}

// ---- draw one wordmark centred on (cx,cy), scaled to target height, aspect kept.
// `extraScale` lets the countdown pop a glyph; `alpha` & `additive` for fades.
void DrawWordmark(const UV& uv, float cx, float cy, float targetH,
                  float alpha, float extraScale = 1.0f, bool additive = false) {
    if (g_bannerTex < 0 || alpha <= 0.0f) return;
    float artW = (uv.u1 - uv.u0) * TEX_W;
    float artH = (uv.v1 - uv.v0) * TEX_H;
    if (artW <= 0.0f || artH <= 0.0f) return;
    float aspect = artW / artH;
    float h = targetH * extraScale;
    float w = h * aspect;
    float x = cx - w * 0.5f, y = cy - h * 0.5f;
    DrawImage(g_bannerTex, { x, y }, { x + w, y + h },
              { uv.u0, uv.v0 }, { uv.u1, uv.v1 },
              additive ? WithAlpha(COL_FLASH, alpha) : WithAlpha(COL_WHITE, alpha), additive);
}

// ASCII fallback for a wordmark, big and gold, centred on (cx,cy).
void DrawWordFallback(const char* text, float cx, float cy, float pxSize, uint32_t col, float alpha) {
    SetFont(g_fDF);
    DrawTextAligned({ cx - 360.0f, cy - pxSize * 0.6f }, { cx + 360.0f, cy + pxSize * 0.6f },
                    pxSize, WithAlpha(col, alpha), text, Align::Center, true, true);
}

void Draw(double openSec) {
    g_open = openSec;
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    // faint centring rules â€” a clean, bold backdrop for the single hero plate
    const float rT = (float)ComputeMotion(openSec, 0.0, HERO_FRAMES);
    DrawRect({ 120.0f, HERO_CY - HERO_H * 0.5f - 22.0f },
             { REF_W - 120.0f, HERO_CY - HERO_H * 0.5f - 20.0f }, WithAlpha(COL_RULE, rT));
    DrawRect({ 120.0f, HERO_CY + HERO_H * 0.5f + 20.0f },
             { REF_W - 120.0f, HERO_CY + HERO_H * 0.5f + 22.0f }, WithAlpha(COL_RULE, rT));

    const bool   running = (g_seqStart > 0.0);
    const double age     = running ? (Now() - g_seqStart) : -1.0;
    const double seqEnd  = STEP_COUNT * STEP_SEC + GO_SEC;

    if (running && age >= 0.0 && age < seqEnd) {
        // ===== COUNTDOWN SEQUENCE: READY -> 3 -> 2 -> 1 -> GO! (real wordmarks) ==
        if (age < STEP_COUNT * STEP_SEC) {
            int step = (int)(age / STEP_SEC);                 // 0..3
            double local = age - step * STEP_SEC;             // 0..STEP_SEC
            float into = (float)std::min(1.0, local / 0.18);  // 0->1 pop-in
            float out  = (float)std::min(1.0, std::max(0.0, (local - (STEP_SEC - 0.22)) / 0.22));
            float alpha = into * (1.0f - out);
            // pop: start a little big, settle to 1.0
            float pop = 1.18f - 0.18f * (float)std::min(1.0, local / 0.22);

            if (step == 0) {        // READY (wide -> its own height)
                if (g_bannerTex >= 0) DrawWordmark(UV_READY, CX, HERO_CY, READY_H, alpha, pop);
                else                  DrawWordFallback("READY", CX, HERO_CY, 86.0f, COL_TITLE, alpha);
            } else {                 // 3 / 2 / 1
                const UV& d = (step == 1) ? UV_3 : (step == 2) ? UV_2 : UV_1;
                const char* dn = (step == 1) ? "3" : (step == 2) ? "2" : "1";
                if (g_bannerTex >= 0) DrawWordmark(d, CX, HERO_CY, HERO_H, alpha, pop);
                else                  DrawWordFallback(dn, CX, HERO_CY, 140.0f, COL_TITLE, alpha);
            }
        } else {
            // ===== GO! â€” a punchy scale-up + additive flash ======================
            double local = age - STEP_COUNT * STEP_SEC;       // 0..GO_SEC
            float into = (float)std::min(1.0, local / 0.12);
            float out  = (float)std::min(1.0, std::max(0.0, (local - (GO_SEC - 0.35)) / 0.35));
            float alpha = into * (1.0f - out);
            float pop   = 1.0f + 0.45f * (1.0f - (float)std::min(1.0, local / 0.30));  // overshoot then settle
            if (g_bannerTex >= 0) {
                DrawWordmark(UV_GO, CX, HERO_CY, HERO_H, alpha, pop);
                // additive bloom on the first beats
                float bloom = into * (1.0f - (float)std::min(1.0, local / 0.45));
                if (bloom > 0.01f) DrawWordmark(UV_GO, CX, HERO_CY, HERO_H, bloom * 0.6f, pop, true);
            } else {
                DrawWordFallback("GO!", CX, HERO_CY, 150.0f, COL_TITLE, alpha);
            }
        }
    } else {
        // ===== IDLE SPLASH: real "READY" hero + pulsing PRESS-START prompt =======
        const float heroT   = (float)ComputeMotion(openSec, 0.0, HERO_FRAMES);
        const float promptT = (float)ComputeMotion(openSec, PROMPT_OFFSET, PROMPT_FRAMES);
        const float footT   = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

        // hero "READY" wordmark eases in with a small downward settle
        float heroCy = HERO_CY - (1.0f - heroT) * 18.0f;
        if (g_bannerTex >= 0) DrawWordmark(UV_READY, CX, heroCy, READY_H, heroT);
        else                  DrawWordFallback("READY", CX, heroCy, 86.0f, COL_TITLE, heroT);

        // pulsing "PRESS  START" call-to-action (procedural â€” the atlas lacks it)
        if (promptT > 0.05f) {
            float breathe = 0.45f + 0.55f * (float)(0.5 + 0.5 * std::sin(g_open * 3.4));
            SetFont(g_fDF);
            DrawTextAligned({ CX - 360.0f, PROMPT_CY - 28.0f }, { CX + 360.0f, PROMPT_CY + 28.0f },
                            40.0f, WithAlpha(COL_PROMPT, promptT * breathe),
                            "PRESS  START", Align::Center, true, true);
        }

        // clean footer hint (ASCII â€” no glyph atlas ships with this single-atlas screen)
        DrawRect({ 120.0f, FOOTER_CY - 22.0f }, { REF_W - 120.0f, FOOTER_CY - 20.0f },
                 WithAlpha(COL_RULE, footT));
        SetFont(g_fRodin);
        DrawTextAligned({ CX - 300.0f, FOOTER_CY - 4.0f }, { CX + 300.0f, FOOTER_CY + 28.0f },
                        22.0f, WithAlpha(COL_FOOTER, footT),
                        "(A) Start     (B) Back", Align::Center, true, true);
    }
    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void StartInit() { Init(); }
void StartDraw(double openSeconds) { Draw(openSeconds); }
void StartInput(const ScreenInput& in) { Input(in); }
void StartReset() { Reset(); }