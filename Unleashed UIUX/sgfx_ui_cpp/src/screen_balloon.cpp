// =============================================================================
// screen_balloon.cpp — the notification toast. A transient banner shown after a
// long-running action finishes: a rounded dark panel lower-centre with a small
// gold title line and a one/two-line message, plus a neutral Select/Back footer
// hint. Built from primitives + text only — no chrome art, no third-party atlas.
//   * a dark rounded panel (TL+BR chamfer grammar), thin gold rule under the
//     title, two message lines;
//   * the title is a short gold caption ("Preflight complete");
//   * the footer carries the neutral Select / Back hint.
// Accept cycles the sample toasts; the registry's BalloonFlowInput backs out on
// cancel.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "sgfx_data.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0, g_logoTex = -1;

// ---- palette (dark-IDE neutral, matching the menu screens) ------------------
const uint32_t C_BG_TOP   = RGBA(12, 20, 38, 255), C_BG_BOT = RGBA(5, 9, 18, 255);
const uint32_t C_PANEL_T  = RGBA(20, 30, 50, 244);     // toast panel top
const uint32_t C_PANEL_B  = RGBA(13, 20, 34, 244);     // toast panel bottom
const uint32_t C_BORDER   = RGBA(120, 170, 230, 150);  // thin cool border
const uint32_t C_TITLE    = RGBA(255, 209, 74, 255);   // gold caption
const uint32_t C_RULE     = RGBA(120, 170, 230, 110);
const uint32_t C_MSG      = RGBA(214, 226, 240, 255);
const uint32_t C_MSG_DIM  = RGBA(170, 186, 208, 255);
const uint32_t C_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t C_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t C_CHIP     = RGBA(150, 196, 150, 255);

// the profile chip reads the live data bridge; the toast copy stays profile-agnostic

// sample notifications (representative tool vocabulary)
struct Toast { const char* title; const char* a; const char* b; };
const Toast TOASTS[] = {
    { "Preflight complete", "3 errors, 12 warnings logged.", "Open the verdict?" },
    { "Screenshots captured", "4 of 4 test views snapped.", "Ready for review." },
    { "Delivery checked", "Changelog and Ramses size", "look in range." },
};
constexpr int TOAST_N = int(sizeof(TOASTS) / sizeof(TOASTS[0]));
int g_idx = 0;

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
    if (g_logoTex < 0)  g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
}
void Reset() { g_idx = 0; }
void Input(const ScreenInput& in) {
    if (in.accept) g_idx = (g_idx + 1) % TOAST_N;
}

void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 40 }, { 40 + w, 40 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, t));
}

void Draw(double openSec) {
    const float a = (float)ComputeMotion(openSec, 0.0, 8.0);

    // ---- dark-IDE backdrop ----
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);
    DrawLogoSlot(a);
    // profile chip top-right
    SetFont(g_fRodin);
    DrawTextAligned({ 910, 52 }, { 1130, 76 }, 16.0f, WithAlpha(C_CHIP, a), "PROFILE", Align::Right, true, true);
    DrawTextAligned({ 910, 74 }, { 1130, 104 }, 24.0f, WithAlpha(C_TITLE, a), sgfx::Get().run.activeProfile.c_str(), Align::Right, true, true);
    ResetFont();

    // ---- the notification toast: rounded dark panel, lower-centre ----
    {
        const float x0 = 360, y0 = 452, x1 = 920, y1 = 596, ch = 18;
        // window inflate-pop: the panel scales open from near-zero about its centre
        // before the text shows.
        const float si = (float)ComputeLinearMotion(openSec, 2.0, 14.0);
        const float ps = Cubic(0.08f, 1.0f, si);
        const V2 pivot = { (x0 + x1) * 0.5f, (y0 + y1) * 0.5f };
        PushTransform(ps, ps, pivot, { 0, 0 });

        const float bT = (float)ComputeMotion(openSec, 2.0, 8.0);
        DrawVGradient({ x0, y0 }, { x1, y1 }, WithAlpha(C_PANEL_T, bT), WithAlpha(C_PANEL_B, bT));
        // TL+BR chamfer cuts back to the backdrop tone (rounded-corner read)
        const V2 c1[4] = { { x0, y0 }, { x0 + ch, y0 }, { x0, y0 + ch }, { x0, y0 } };
        const V2 c2[4] = { { x1 - ch, y1 }, { x1, y1 }, { x1, y1 - ch }, { x1 - ch, y1 } };
        const uint32_t bgc[4] = { WithAlpha(C_BG_BOT, bT), WithAlpha(C_BG_BOT, bT), WithAlpha(C_BG_BOT, bT), WithAlpha(C_BG_BOT, bT) };
        DrawQuadGradient(c1, bgc); DrawQuadGradient(c2, bgc);
        // thin cool border around the straight edges
        uint32_t bd = WithAlpha(C_BORDER, bT);
        DrawRect({ x0 + ch, y0 }, { x1, y0 + 2 }, bd);
        DrawRect({ x0, y1 - 2 }, { x1 - ch, y1 }, bd);
        DrawRect({ x0, y0 + ch }, { x0 + 2, y1 }, bd);
        DrawRect({ x1 - 2, y0 }, { x1, y1 - ch }, bd);
        PopTransform();

        // ---- content, appearing AFTER the panel inflates (full size) ----
        const float tx = (float)ComputeMotion(openSec, 14.0, 8.0);
        const Toast& T = TOASTS[g_idx];
        const float pad = 30.0f;
        // gold title caption
        SetFont(g_fDF);
        DrawTextAligned({ x0 + pad, y0 + 18 }, { x1 - pad, y0 + 52 }, 28.0f,
                        WithAlpha(C_TITLE, tx), T.title, Align::Left, true, true);
        ResetFont();
        // thin gold rule under the title
        DrawRect({ x0 + pad, y0 + 58 }, { x1 - pad, y0 + 60 }, WithAlpha(C_RULE, tx));
        // one/two message lines
        SetFont(g_fSeurat);
        DrawText({ x0 + pad, y0 + 74 }, 24.0f, WithAlpha(C_MSG, tx), T.a);
        DrawText({ x0 + pad, y0 + 104 }, 24.0f, WithAlpha(C_MSG_DIM, tx), T.b);
        ResetFont();
    }

    // ---- footer: neutral Select / Back hint ----
    {
        SetFont(g_fRodin);
        DrawRect({ 360, 624 }, { 920, 626 }, WithAlpha(C_RULE, a));
        DrawText({ 366, 640 }, 20.0f, WithAlpha(C_FOOTER, a), "Enter  Select");
        DrawText({ 560, 640 }, 20.0f, WithAlpha(C_FOOTER, a), "Esc  Back");
        ResetFont();
    }
}

} // namespace

void BalloonInit() { Init(); }
void BalloonDraw(double openSeconds) { Draw(openSeconds); }
void BalloonInput(const ScreenInput& in) { Input(in); }
void BalloonReset() { Reset(); }
