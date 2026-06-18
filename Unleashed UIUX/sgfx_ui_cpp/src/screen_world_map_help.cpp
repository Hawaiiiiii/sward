// =============================================================================
// screen_world_map_help.cpp — the QA-hub HELP overlay. A dim scrim over the live
// hub, with a single centred dark panel titled "Help" that explains the hub
// controls in neutral prose. Built from primitives + text only — no game atlas,
// no button-glyph sprites, no chrome frame art. The dark-IDE panel idiom from
// screen_status.cpp (body + header-cap rects + a thin rule).
//
// It is the dim-overlay sibling of the pause card: the live hub dims behind a
// bounded dark panel listing the hub's keyboard controls, each row = an action
// key + a neutral description. The registry's WmHelpFlowInput closes it on cancel
// (-> @back); a short confirm flash plays in the standalone build.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <algorithm>
#include <cmath>

using namespace ui;

namespace {

// ---- fonts ------------------------------------------------------------------
static int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// ---- the control-hint table (neutral hub controls) --------------------------
struct Hint {
    const char* key;     // the control key, drawn in the left column
    const char* label;   // the action description
};

const Hint HINTS[] = {
    { "Left / Right", "Move between QA areas" },
    { "Enter",        "Open the focused area" },
    { "Esc",          "Back" },
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

constexpr float KEY_X    = 44.0f;       // key-column inset from the panel left
constexpr float LABEL_X  = 230.0f;      // action-label inset — clears the key column

// ---- entrance / animation tuning (frames @60fps) ----------------------------
constexpr double DIM_FRAMES   = 12.0;
constexpr double PANEL_OFFSET = 4.0,  PANEL_FRAMES = 16.0;
constexpr double ROW_STAGGER  = 3.0,  ROW_FRAMES   = 12.0;
constexpr double FOOT_OFFSET  = 10.0, FOOT_FRAMES  = 12.0;

// ---- palette (dark-IDE neutral, shared with status) -------------------------
const uint32_t COL_DIM       = RGBA(0, 0, 0, 150);
const uint32_t COL_PANEL     = RGBA(16, 24, 40, 235);
const uint32_t COL_CAP       = RGBA(10, 16, 28, 255);
const uint32_t COL_TITLE     = RGBA(255, 209, 74, 255);   // gold
const uint32_t COL_RULE      = RGBA(120, 170, 230, 90);
const uint32_t COL_LABEL     = RGBA(220, 230, 244, 255);
const uint32_t COL_KEY       = RGBA(255, 236, 150, 255);  // warm key tint
const uint32_t COL_FOOTER    = RGBA(190, 205, 225, 220);
const uint32_t COL_OK        = RGBA(120, 230, 140, 255);

// ---- interactive state ------------------------------------------------------
double g_closeStart = -100.0;   // Now() when close was last pressed (confirm flash)

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}

void Reset() { g_closeStart = -100.0; }

void Input(const ScreenInput& in) {
    // a help overlay: accept or cancel closes it (cancel -> @back via the registry
    // flow wrapper; a short confirm flash plays in the standalone build).
    if (in.accept || in.cancel) g_closeStart = Now();
}

// a bounded dark panel: body + a darker header cap + a thin rule under the header.
void DrawPanel(V2 min, V2 max, float headerH, float a) {
    DrawRect(min, max, WithAlpha(COL_PANEL, a));
    DrawRect(min, { max.x, min.y + headerH }, WithAlpha(COL_CAP, a));
    DrawRect({ min.x + 16, min.y + headerH - 2 }, { max.x - 16, min.y + headerH },
             WithAlpha(COL_RULE, a));
}

void Draw(double openSec) {
    const float cx = REF_W * 0.5f;
    const float panelLeft = cx - PANEL_W * 0.5f;
    const float panelTopSettled = (REF_H - PANEL_H) * 0.5f - 6.0f;

    // ---- background dim (the live hub sits behind this overlay) ----
    float dim = (float)ComputeMotion(openSec, 0.0, DIM_FRAMES);
    DrawRect({ 0, 0 }, { REF_W, REF_H }, WithAlpha(COL_DIM, dim));

    // ---- container ease-in: fade + a short upward settle ----
    float panelT = (float)ComputeMotion(openSec, PANEL_OFFSET, PANEL_FRAMES);
    float panelTop = panelTopSettled + (1.0f - panelT) * 26.0f;
    float panelBot = panelTop + PANEL_H;

    DrawPanel({ panelLeft, panelTop }, { panelLeft + PANEL_W, panelBot }, HEADER_H, panelT);

    // ---- title ----
    SetFont(g_fDF);
    DrawTextAligned({ panelLeft, panelTop }, { panelLeft + PANEL_W, panelTop + HEADER_H },
                    40.0f, WithAlpha(COL_TITLE, panelT), "Help", Align::Center, true, true);
    ResetFont();

    // ---- hint rows (staggered fade-in) ----
    const float rowsTop = panelTop + HEADER_H + ROW_TOP_PAD;
    const float keyX    = panelLeft + KEY_X;
    const float labelX  = panelLeft + LABEL_X;

    for (int i = 0; i < HINT_COUNT; ++i) {
        const Hint& h = HINTS[i];
        float rowT = (float)ComputeMotion(openSec, PANEL_OFFSET + 6.0 + i * ROW_STAGGER, ROW_FRAMES);
        if (rowT <= 0.0f) continue;
        float cy = rowsTop + i * ROW_H + ROW_H * 0.5f;

        // the control key (left column)
        SetFont(g_fRodin);
        DrawTextAligned({ keyX, cy - 18.0f }, { labelX - 14.0f, cy + 18.0f }, 26.0f,
                        WithAlpha(COL_KEY, rowT), h.key, Align::Left, true, true);
        ResetFont();

        // the action label
        SetFont(g_fSeurat);
        DrawTextAligned({ labelX, cy - 18.0f }, { panelLeft + PANEL_W - 28.0f, cy + 18.0f }, 28.0f,
                        WithAlpha(COL_LABEL, rowT), h.label, Align::Left, true, true);
        ResetFont();
    }

    // ---- footer hint: Esc  Close ----
    float footT = (float)ComputeMotion(openSec, PANEL_OFFSET + FOOT_OFFSET, FOOT_FRAMES);
    DrawRect({ panelLeft + 16, panelBot - 46 }, { panelLeft + PANEL_W - 16, panelBot - 44 },
             WithAlpha(COL_RULE, panelT));
    {
        float hcy = panelBot - 24.0f;
        float hx  = panelLeft + 32.0f;
        SetFont(g_fRodin);
        DrawText({ hx, hcy - 12.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "Esc");
        hx += MeasureText(22.0f, "Esc").x + 10.0f;
        DrawText({ hx, hcy - 12.0f }, 22.0f, WithAlpha(COL_FOOTER, footT), "Close");
        ResetFont();
    }

    // ---- transient "close" confirm flash ----
    double age = Now() - g_closeStart;
    if (g_closeStart > 0.0 && age < 1.0) {
        float ca = std::min(1.0f, (float)((1.0 - age) / 0.35));
        SetFont(g_fRodin);
        DrawTextAligned({ panelLeft, panelTop + HEADER_H * 0.5f - 60.0f },
                        { panelLeft + PANEL_W, panelTop + HEADER_H * 0.5f - 28.0f }, 22.0f,
                        WithAlpha(COL_OK, ca * panelT), "Closing...", Align::Center, true, true);
        ResetFont();
    }
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void WorldMapHelpInit() { Init(); }
void WorldMapHelpDraw(double openSeconds) { Draw(openSeconds); }
void WorldMapHelpInput(const ScreenInput& in) { Input(in); }
void WorldMapHelpReset() { Reset(); }
