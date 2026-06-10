// =============================================================================
// screen_world_map_help.cpp - the World-Map CONTROLS help overlay, re-authored as
// clean hand-written C++ in the UnleashedRecomp ui/options_menu idiom (NOT a CSD
// node dump - the raw world_map_help.json is just a stretched mat_mainmenu_common
// bar, a couple of thin divider strips and a Tails portrait; transcribing it
// smears placeholder green/strip quads across the screen).
//
// It is the dim-overlay sibling of screen_pause.cpp: the live world-map dims
// behind a single bounded chrome panel (the REAL Sonic Unleashed silver-chrome
// frame via ui::DrawGameWindow), titled "CONTROLS", listing the world-map button
// hints - each row = a REAL Xbox button glyph (mat_comon_x360_001, the same atlas
// the world_map / options footers use) + an ASCII action label, exactly the
// world_map DrawHint glyph+label idiom but stacked vertically. (A)/(B) closes
// (a short confirm flash in the standalone build).
//
// Art is the real extracted glyph atlas where a distinctive element exists; if it
// is missing (-1 slot) the rows fall back to ASCII tokens, exactly as world_map's
// DrawHint / options' DrawWindow fallbacks do.
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

// ---- real-art atlas ---------------------------------------------------------
// mat_comon_x360_001 (512x512): top row of Xbox button glyphs (A B X Y / LB RB).
// This screen's own asset dir ships no glyph atlas, so we load it from the sibling
// world_map asset dir (the same file the world_map footer uses); if neither path
// resolves, the rows degrade to ASCII tokens.
int g_glyphTex = -1;   // mat_comon_x360_001

// ---- fonts (MSDF sweep, screen_options.cpp pattern) -------------------------
static int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

constexpr float GLYPH_TEX_W = 512.0f, GLYPH_TEX_H = 512.0f;

// Xbox glyphs - top row of mat_comon_x360_001, measured from the alpha channel
// (copied VERBATIM from the verified screen_options.cpp).
const UV GLYPH_A  = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B  = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };
const UV GLYPH_LB = { 0.32617f, 0.00781f, 0.46094f, 0.07812f };
const UV GLYPH_RB = { 0.48242f, 0.00781f, 0.61523f, 0.07812f };

// glyph pixel aspect (w/h) for height-locked drawing
float Aspect(const UV& g) {
    return ((g.u1 - g.u0) * GLYPH_TEX_W) / ((g.v1 - g.v0) * GLYPH_TEX_H);
}

// ---- the control-hint table -------------------------------------------------
// kind: 0 = single glyph, 1 = a glyph PAIR (e.g. LB + RB) sharing one label.
struct Hint {
    int         kind;        // 0 single, 1 pair
    const UV*   g0;          // first glyph
    const UV*   g1;          // second glyph (pair only; nullptr otherwise)
    const char* token;       // ASCII fallback token when the atlas is missing
    const char* label;       // action description
};

const Hint HINTS[] = {
    { 0, &GLYPH_A,  nullptr,   "(A)",     "Enter Stage" },
    { 0, &GLYPH_B,  nullptr,   "(B)",     "Back" },
    { 1, &GLYPH_LB, &GLYPH_RB, "[LB/RB]", "Switch Area" },
    { 0, nullptr,   nullptr,   "[Start]", "Map Overview" },   // no Start glyph in atlas -> ASCII token
    { 0, nullptr,   nullptr,   "[D-Pad]", "Move Cursor" },    // d-pad: keyboard-style ASCII token
};
constexpr int HINT_COUNT = (int)(sizeof(HINTS) / sizeof(HINTS[0]));

// ---- layout (reference px) --------------------------------------------------
constexpr float PANEL_W  = 560.0f;
constexpr float HEADER_H = 70.0f;
constexpr float ROW_H    = 60.0f;
constexpr float ROW_TOP_PAD = 22.0f;    // gap below the header rule to the first row
constexpr float FOOTER_PAD  = 56.0f;    // reserved strip at the panel bottom for the footer hint
// panel height = header + rows + footer strip + a little breathing room
constexpr float PANEL_H  = HEADER_H + ROW_TOP_PAD + HINT_COUNT * ROW_H + FOOTER_PAD + 14.0f;

constexpr float GLYPH_H   = 34.0f;      // on-screen glyph row height
constexpr float ROW_LABEL_X = 180.0f;   // label inset — clears the LB/RB pair + the [D-Pad] token

// ---- entrance / animation tuning (frames @60fps) ----------------------------
constexpr double DIM_FRAMES   = 12.0;
constexpr double PANEL_OFFSET = 4.0,  PANEL_FRAMES = 16.0;
constexpr double ROW_STAGGER  = 3.0,  ROW_FRAMES   = 12.0;
constexpr double FOOT_OFFSET  = 10.0, FOOT_FRAMES  = 12.0;

// ---- palette (shared with pause/options/world_map) --------------------------
const uint32_t COL_DIM       = RGBA(0, 0, 0, 150);
const uint32_t COL_TITLE     = RGBA(255, 209, 74, 255);   // Unleashed gold
const uint32_t COL_RULE      = RGBA(120, 170, 230, 90);
const uint32_t COL_LABEL     = RGBA(220, 230, 244, 255);
const uint32_t COL_TOKEN     = RGBA(255, 236, 150, 255);  // ASCII glyph-token tint
const uint32_t COL_FOOTER    = RGBA(190, 205, 225, 220);
const uint32_t COL_OK        = RGBA(120, 230, 140, 255);
const uint32_t COL_WHITE     = RGBA(255, 255, 255, 255);

// ---- interactive state ------------------------------------------------------
double g_closeStart = -100.0;   // Now() when (A)/(B) was last pressed (confirm flash)

void Init() {
    GameFrameTex();   // lazy-load the shared chrome frame used by DrawGameWindow
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/world_map_help/mat_comon_x360_001.png");
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/world_map/mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}

void Reset() { g_closeStart = -100.0; }

void Input(const ScreenInput& in) {
    // a help overlay: (A) or (B) closes it (just a confirm flash in the standalone build).
    if (in.accept || in.cancel) g_closeStart = Now();
}

// ---- draw a single button glyph height-locked at GLYPH_H, left edge at x. -----
// Returns the x just past the glyph. Caller draws an ASCII token when the atlas is missing.
float DrawGlyph(float x, float cy, const UV& g, float t, float h = GLYPH_H) {
    if (g_glyphTex >= 0) {
        float gw = h * Aspect(g);
        DrawImage(g_glyphTex, { x, cy - h * 0.5f }, { x + gw, cy + h * 0.5f },
                  { g.u0, g.v0 }, { g.u1, g.v1 }, WithAlpha(COL_WHITE, t));
        return x + gw;
    }
    return x;
}

void Draw(double openSec) {
    const float cx = REF_W * 0.5f;
    const float panelLeft = cx - PANEL_W * 0.5f;
    const float panelTopSettled = (REF_H - PANEL_H) * 0.5f - 6.0f;

    // ---- background dim (the live world-map sits behind this overlay) ----
    float dim = (float)ComputeMotion(openSec, 0.0, DIM_FRAMES);
    DrawRect({ 0, 0 }, { REF_W, REF_H }, WithAlpha(COL_DIM, dim));

    // ---- container ease-in: fade + a short upward settle (pause idiom) ----
    float panelT = (float)ComputeMotion(openSec, PANEL_OFFSET, PANEL_FRAMES);
    float panelTop = panelTopSettled + (1.0f - panelT) * 26.0f;
    float panelBot = panelTop + PANEL_H;

    DrawGameWindow({ panelLeft, panelTop }, { panelLeft + PANEL_W, panelBot }, HEADER_H, panelT);
    DrawRect({ panelLeft + 16, panelTop + HEADER_H - 2 }, { panelLeft + PANEL_W - 16, panelTop + HEADER_H },
             WithAlpha(COL_RULE, panelT));

    // ---- title ----
    SetFont(g_fDF);
    DrawTextAligned({ panelLeft, panelTop }, { panelLeft + PANEL_W, panelTop + HEADER_H },
                    40.0f, WithAlpha(COL_TITLE, panelT), "CONTROLS", Align::Center, true, true);

    // ---- hint rows (staggered fade-in) ----
    const float rowsTop = panelTop + HEADER_H + ROW_TOP_PAD;
    const float glyphX  = panelLeft + 44.0f;
    const float labelX  = panelLeft + ROW_LABEL_X;

    for (int i = 0; i < HINT_COUNT; ++i) {
        const Hint& h = HINTS[i];
        float rowT = (float)ComputeMotion(openSec, PANEL_OFFSET + 6.0 + i * ROW_STAGGER, ROW_FRAMES);
        if (rowT <= 0.0f) continue;
        float cy = rowsTop + i * ROW_H + ROW_H * 0.5f;

        bool drewGlyph = false;
        if (g_glyphTex >= 0 && h.g0) {
            const float gh = (h.kind == 1) ? GLYPH_H * 0.82f : GLYPH_H;   // pairs draw smaller to fit
            float gx = DrawGlyph(glyphX, cy, *h.g0, rowT, gh);
            if (h.kind == 1 && h.g1) {
                // a glyph pair (LB + RB) with a small "/" separator between them
                SetFont(g_fRodin);
                DrawTextAligned({ gx + 2.0f, cy - 14.0f }, { gx + 14.0f, cy + 14.0f }, 22.0f,
                                WithAlpha(COL_LABEL, rowT), "/", Align::Center, true, true);
                DrawGlyph(gx + 16.0f, cy, *h.g1, rowT, gh);
            }
            drewGlyph = true;
        }
        if (!drewGlyph) {
            // ASCII token fallback (also used for Start / D-Pad which have no glyph)
            SetFont(g_fRodin);
            DrawTextAligned({ glyphX, cy - 16.0f }, { labelX - 14.0f, cy + 16.0f }, 24.0f,
                            WithAlpha(COL_TOKEN, rowT), h.token, Align::Left, true, true);
        }

        // action label
        SetFont(g_fSeurat);
        DrawTextAligned({ labelX, cy - 18.0f }, { panelLeft + PANEL_W - 28.0f, cy + 18.0f }, 28.0f,
                        WithAlpha(COL_LABEL, rowT), h.label, Align::Left, true, true);
    }

    // ---- footer hint: (B) Close ----
    float footT = (float)ComputeMotion(openSec, PANEL_OFFSET + FOOT_OFFSET, FOOT_FRAMES);
    DrawRect({ panelLeft + 16, panelBot - 46 }, { panelLeft + PANEL_W - 16, panelBot - 44 },
             WithAlpha(COL_RULE, panelT));
    {
        float hcy = panelBot - 24.0f;
        float hx = panelLeft + 32.0f;
        SetFont(g_fRodin);
        if (g_glyphTex >= 0) {
            hx = DrawGlyph(hx, hcy, GLYPH_B, footT) + 8.0f;
        } else {
            DrawText({ hx, hcy - 12.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "(B)");
            hx += MeasureText(22.0f, "(B)").x + 8.0f;
        }
        DrawText({ hx, hcy - 12.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "Close");
    }

    // ---- transient "close" confirm flash ----
    double age = Now() - g_closeStart;
    if (g_closeStart > 0.0 && age < 1.0) {
        float ca = std::min(1.0f, (float)((1.0 - age) / 0.35));
        SetFont(g_fRodin);
        DrawTextAligned({ panelLeft, panelTop + HEADER_H * 0.5f - 60.0f },
                        { panelLeft + PANEL_W, panelTop + HEADER_H * 0.5f - 28.0f }, 22.0f,
                        WithAlpha(COL_OK, ca * panelT), "Closing...", Align::Center, true, true);
    }
    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void WorldMapHelpInit() { Init(); }
void WorldMapHelpDraw(double openSeconds) { Draw(openSeconds); }
void WorldMapHelpInput(const ScreenInput& in) { Input(in); }
void WorldMapHelpReset() { Reset(); }
