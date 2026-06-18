// =============================================================================
// screen_sonic_hud.cpp — the live-run HUD overlay. A minimal, corner-anchored
// overlay shown WHILE a check runs over a profile: a profile chip, the running
// action, a "pack N / total" progress line with a thin progress bar, and a small
// "running..." state. Built from primitives + text only — no chrome art, no
// third-party atlas. Unlike the menu screens there is NO full-screen panel; the
// HUD floats over whatever is behind it, so the backdrop stays transparent.
//
// Light interactivity: Up/Down step the pack counter (and the bar follows),
// Left/Right nudge the bar directly for inspection, Accept toggles a live tick so
// the running state animates. The registry's HudFlowInput backs out on cancel.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "sgfx_data.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

using namespace ui;

namespace {

// ---- real game fonts (MSDF) -------------------------------------------------
int g_fSeurat = 0;   // labels / running-action wordmark
int g_fRodin  = 0;   // values / footer hints
int g_fDF     = 0;   // gold captions

// ---- layout (reference px) --------------------------------------------------
// Top-left status chip cluster + a centred progress block. Title-safe insets.
constexpr float CHIP_X = 92.0f,  CHIP_Y = 64.0f;                 // profile chip box top-left
constexpr float CHIP_W = 132.0f, CHIP_H = 64.0f;
constexpr float ACT_X  = 240.0f;                                 // running-action column (right of chip)

constexpr float BAR_X  = 360.0f, BAR_Y  = 360.0f, BAR_W = 560.0f, BAR_H = 14.0f;  // progress track

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double CHIP_FRAMES = 12.0;
constexpr double BAR_OFFSET  = 4.0,  BAR_FRAMES = 16.0;
constexpr double FOOT_OFFSET = 10.0, FOOT_FRAMES = 12.0;
constexpr double PROG_MOVE_FRAMES = 14.0;  // progress-fill easing when it changes

// ---- palette (dark-IDE neutral) ---------------------------------------------
const uint32_t C_BG_SCRIM   = RGBA(8, 13, 24, 130);      // faint overlay scrim
const uint32_t C_CHIP_T     = RGBA(20, 30, 50, 235);     // chip panel top
const uint32_t C_CHIP_B     = RGBA(13, 20, 34, 235);     // chip panel bottom
const uint32_t C_BORDER     = RGBA(120, 170, 230, 150);
const uint32_t C_TITLE      = RGBA(255, 209, 74, 255);   // gold caption
const uint32_t C_LABEL      = RGBA(150, 170, 196, 255);
const uint32_t C_TEXT       = RGBA(214, 226, 240, 255);
const uint32_t C_WHITE      = RGBA(255, 255, 255, 255);
const uint32_t C_TRACK      = RGBA(20, 28, 44, 190);     // dim empty progress track
const uint32_t C_TRACK_EDGE = RGBA(120, 170, 230, 110);
const uint32_t C_FILL_T     = RGBA(64, 150, 235, 235);   // progress fill (cool blue)
const uint32_t C_FILL_B     = RGBA(28, 92, 180, 235);
const uint32_t C_RUNNING    = RGBA(120, 200, 255, 255);  // pulsing "running" tone
const uint32_t C_FOOTER     = RGBA(210, 220, 235, 220);
const uint32_t C_RULE       = RGBA(120, 170, 230, 90);

// the profile + action this run is reporting on (action representative until a live feed supplies it)
const char* ActiveProfile() { return sgfx::Get().run.activeProfile.c_str(); }   // from the live data bridge
const char* const ACTION_LABEL   = "PREFLIGHT";

// ---- interactive state ------------------------------------------------------
int   g_total = 4;          // total packs in the run
int   g_done  = 3;          // packs completed so far
float g_prog  = 0.75f;      // progress fraction 0..1 (3 of 4)
float g_progPrev = 0.75f;   // for eased transition
double g_progStart = -100.0;// Now() when the progress last changed
bool  g_ticking = true;     // Accept toggles the live "running" pulse

void SetProgress(int done) {
    g_done = std::clamp(done, 0, g_total);
    g_progPrev = g_prog;
    g_prog = (g_total > 0) ? (float)g_done / (float)g_total : 0.0f;
    g_progStart = Now();
}

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}

void Reset() {
    g_total = 4; g_done = 3; g_prog = 0.75f; g_progPrev = 0.75f;
    g_progStart = -100.0; g_ticking = true;
}

void Input(const ScreenInput& in) {
    if (in.accept) g_ticking = !g_ticking;           // toggle the running pulse
    if (in.down)   SetProgress(g_done + 1);          // advance a pack
    if (in.up)     SetProgress(g_done - 1);          // step back a pack
    if (in.left)   { g_progPrev = g_prog; g_prog = std::clamp(g_prog - 0.05f, 0.0f, 1.0f); g_progStart = Now(); }
    if (in.right)  { g_progPrev = g_prog; g_prog = std::clamp(g_prog + 0.05f, 0.0f, 1.0f); g_progStart = Now(); }
    // cancel: would pause the run in-flow; handled by the registry's HudFlowInput.
}

// Compute the eased progress fraction to display this frame.
float DisplayProgress() {
    float p = g_prog;
    float moveT = (float)ComputeMotion(g_progStart, 0.0, PROG_MOVE_FRAMES);
    if (g_progStart > 0.0) p = Lerp(g_progPrev, g_prog, moveT);
    return std::clamp(p, 0.0f, 1.0f);
}

// ---- the profile chip: a small rounded dark panel holding "G65" -------------
void DrawProfileChip(float t) {
    if (t <= 0.0f) return;
    const float x0 = CHIP_X, y0 = CHIP_Y, x1 = CHIP_X + CHIP_W, y1 = CHIP_Y + CHIP_H;
    DrawVGradient({ x0, y0 }, { x1, y1 }, WithAlpha(C_CHIP_T, t), WithAlpha(C_CHIP_B, t));
    uint32_t bd = WithAlpha(C_BORDER, t);
    DrawRect({ x0, y0 }, { x1, y0 + 2 }, bd);
    DrawRect({ x0, y1 - 2 }, { x1, y1 }, bd);
    DrawRect({ x0, y0 }, { x0 + 2, y1 }, bd);
    DrawRect({ x1 - 2, y0 }, { x1, y1 }, bd);
    SetFont(g_fRodin);
    DrawTextAligned({ x0, y0 + 8 }, { x1, y0 + 26 }, 13.0f, WithAlpha(C_LABEL, t), "PROFILE", Align::Center, true, true);
    ResetFont();
    SetFont(g_fDF);
    DrawTextAligned({ x0, y0 + 26 }, { x1, y1 - 6 }, 30.0f, WithAlpha(C_TITLE, t), ActiveProfile(), Align::Left, true, true);
    ResetFont();
}

void Draw(double openSec) {
    const float chipT = (float)ComputeMotion(openSec, 0.0, CHIP_FRAMES);
    const float barT  = (float)ComputeMotion(openSec, BAR_OFFSET, BAR_FRAMES);
    const float footT = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // faint scrim so the overlay reads over whatever is behind it (transparent HUD,
    // no full-screen fill — just a subtle darken).
    DrawRect({ 0, 0 }, { REF_W, REF_H }, WithAlpha(C_BG_SCRIM, chipT));

    // ---- top-left: profile chip + running action ----
    DrawProfileChip(chipT);
    {
        SetFont(g_fSeurat);
        // the running action wordmark, right of the chip
        DrawTextAligned({ ACT_X, CHIP_Y + 6 }, { ACT_X + 360, CHIP_Y + 38 }, 30.0f,
                        WithAlpha(C_TEXT, chipT), ACTION_LABEL, Align::Left, true, true);
        ResetFont();
        // small "running..." state, pulsing when ticking
        float pulse = g_ticking ? Breathe(Now(), 0.45f, 1.0f, 1.1f) : 0.6f;
        SetFont(g_fRodin);
        DrawTextAligned({ ACT_X, CHIP_Y + 40 }, { ACT_X + 360, CHIP_Y + 60 }, 18.0f,
                        WithAlpha(C_RUNNING, chipT * pulse),
                        g_ticking ? "running..." : "paused", Align::Left, true, true);
        ResetFont();
    }

    // ---- centre: progress line + thin progress bar ----
    if (barT > 0.0f) {
        const float by = BAR_Y + (1.0f - barT) * 12.0f;
        // "N / total packs" line above the bar
        char line[40];
        std::snprintf(line, sizeof(line), "%d / %d packs", g_done, g_total);
        SetFont(g_fRodin);
        DrawTextAligned({ BAR_X, by - 34 }, { BAR_X + BAR_W, by - 8 }, 22.0f,
                        WithAlpha(C_TEXT, barT), line, Align::Left, true, true);
        DrawTextAligned({ BAR_X, by - 34 }, { BAR_X + BAR_W, by - 8 }, 16.0f,
                        WithAlpha(C_LABEL, barT), ACTION_LABEL, Align::Right, true, true);
        ResetFont();

        // dim empty track + thin edge lines
        DrawRect({ BAR_X, by }, { BAR_X + BAR_W, by + BAR_H }, WithAlpha(C_TRACK, barT));
        DrawRect({ BAR_X, by - 1 }, { BAR_X + BAR_W, by + 1 }, WithAlpha(C_TRACK_EDGE, barT));
        DrawRect({ BAR_X, by + BAR_H - 1 }, { BAR_X + BAR_W, by + BAR_H }, WithAlpha(C_TRACK_EDGE, barT));

        // eased fill, cool-blue vertical gradient
        float fill = DisplayProgress();
        if (fill > 0.001f) {
            float fw = BAR_W * fill;
            DrawVGradient({ BAR_X, by }, { BAR_X + fw, by + BAR_H },
                          WithAlpha(C_FILL_T, barT), WithAlpha(C_FILL_B, barT));
            // soft additive leading edge so it reads as "in motion"
            float gx = BAR_X + fw - 6.0f;
            DrawRect({ gx, by - 1 }, { gx + 6.0f, by + BAR_H + 1 },
                     WithAlpha(C_RUNNING, barT * 0.5f), /*additive*/ true);
        }
        // percent readout, right-anchored under the bar
        char pct[8]; std::snprintf(pct, sizeof(pct), "%d%%", (int)(fill * 100.0f + 0.5f));
        SetFont(g_fRodin);
        DrawTextAligned({ BAR_X, by + BAR_H + 6 }, { BAR_X + BAR_W, by + BAR_H + 28 }, 18.0f,
                        WithAlpha(C_LABEL, barT), pct, Align::Right, true, true);
        ResetFont();
    }

    // ---- footer hint ----
    {
        SetFont(g_fRodin);
        DrawRect({ BAR_X, 612.0f }, { BAR_X + BAR_W, 614.0f }, WithAlpha(C_RULE, footT));
        DrawText({ BAR_X, 628.0f }, 20.0f, WithAlpha(C_FOOTER, footT), "Up/Down  Step");
        DrawText({ BAR_X + 200.0f, 628.0f }, 20.0f, WithAlpha(C_FOOTER, footT), "Esc  Pause");
        ResetFont();
    }
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void SonicHudInit() { Init(); }
void SonicHudDraw(double openSeconds) { Draw(openSeconds); }
void SonicHudInput(const ScreenInput& in) { Input(in); }
void SonicHudReset() { Reset(); }
