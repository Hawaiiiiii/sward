// =============================================================================
// screen_start.cpp — the neutral start / splash. A clean dark field with a
// centred host logo slot and a soft subtitle line. Built from primitives + text
// only — no game atlas, no wordmarks, no countdown sequence. The host's own boot
// logo drops into the logo slot (assets/gameart/boot_logo.png); with no asset the
// slot stays empty (a clean field) — the boot_logo LogoSlot convention, the same
// id<0 => draw-nothing idiom as the boot splash.
//
// Lifecycle matches the registry: StartInit/StartDraw/StartInput/StartReset, with
// Input a no-op (the start screen is a passive splash; flow is driven elsewhere).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cmath>

using namespace ui;

namespace {

// ---- layout (reference px) --------------------------------------------------
constexpr float CX = REF_W * 0.5f;     // 640
constexpr float CY = REF_H * 0.5f;     // 360

// ---- entrance / animation tuning (frames @60fps) ----------------------------
constexpr double LOGO_OFFSET = 4.0,  LOGO_FRAMES = 16.0;
constexpr double SUB_OFFSET  = 12.0, SUB_FRAMES  = 14.0;

// ---- palette (shared dark-IDE field, matching status/boot) ------------------
const uint32_t COL_BG_TOP  = RGBA(12, 20, 38, 255);
const uint32_t COL_BG_BOT  = RGBA(5, 9, 18, 255);
const uint32_t COL_SUB     = RGBA(150, 170, 196, 255);   // soft neutral subtitle
const uint32_t COL_WHITE   = RGBA(255, 255, 255, 255);

// ---- host logo slot ---------------------------------------------------------
int g_logoTex = -1;   // host-supplied boot logo; -1 = empty slot (clean field)

// ---- fonts ------------------------------------------------------------------
static int g_fRodin = 0;

void Init() {
    if (g_logoTex < 0) g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
    if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db");
}

void Reset() {}

void Input(const ScreenInput&) {}   // passive splash — no interaction

// the centred host logo slot. The host's own boot logo drops in here; with no
// asset the slot stays empty (clean field) — no placeholder, no debug text.
void DrawLogoSlot(float a) {
    if (g_logoTex < 0 || a <= 0.0f) return;
    const float lw = 420.0f, lh = lw * 200.0f / 600.0f;
    DrawImage(g_logoTex, { CX - lw * 0.5f, CY - lh * 0.5f }, { CX + lw * 0.5f, CY + lh * 0.5f },
              { 0.f, 0.f }, { 1.f, 1.f }, WithAlpha(COL_WHITE, a));
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const float logoT = (float)ComputeMotion(openSec, LOGO_OFFSET, LOGO_FRAMES);
    const float subT  = (float)ComputeMotion(openSec, SUB_OFFSET, SUB_FRAMES);

    // centred host logo slot (empty = clean field)
    DrawLogoSlot(logoT);

    // a soft "Loading" subtitle below the slot — minimal, low key, gentle breathe
    if (subT > 0.05f) {
        float breathe = 0.55f + 0.45f * (float)(0.5 + 0.5 * std::sin(Now() * 2.2));
        SetFont(g_fRodin);
        DrawTextAligned({ CX - 300.0f, CY + 150.0f }, { CX + 300.0f, CY + 184.0f },
                        22.0f, WithAlpha(COL_SUB, subT * breathe),
                        "Loading", Align::Center, true, true);
        ResetFont();
    }
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void StartInit() { Init(); }
void StartDraw(double openSeconds) { Draw(openSeconds); }
void StartInput(const ScreenInput& in) { Input(in); }
void StartReset() { Reset(); }
