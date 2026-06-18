// =============================================================================
// screen_pause.cpp — the QUICK-MENU overlay. A ~55% dark scrim over whatever scene
// is behind, plus a centered dark-IDE menu panel with a short list of entries:
//   Resume / Metrics / Settings / Back to hub / Quit.
// Built from primitives + text only (no chrome art, no third-party atlas) in the
// dark-IDE neutral aesthetic shared with screen_status.cpp / screen_options.cpp.
// Pause keeps its OWN navigation: PauseInput records the target, PauseNav() polls-
// and-clears it (the host drives the wipe + screen switch). Entries map to:
//   Resume -> "@back", Metrics -> "status", Settings -> "options",
//   Back to hub -> "world_map", Quit -> "@back".
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// ---- palette (dark, neutral — matches screen_status.cpp / screen_options.cpp) -
const uint32_t C_SCRIM    = RGBA(0, 0, 0, 140);          // ~55% dark scrim over the scene
const uint32_t C_PANEL    = RGBA(16, 24, 40, 245);
const uint32_t C_PANEL_CAP= RGBA(10, 16, 28, 255);
const uint32_t C_BORDER   = RGBA(64, 150, 235, 120);
const uint32_t C_SEL_TOP  = RGBA(64, 150, 235, 225), C_SEL_BOT = RGBA(28, 92, 180, 225);
const uint32_t C_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t C_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t C_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t C_RULE     = RGBA(120, 170, 230, 90);
const uint32_t C_FOOTER   = RGBA(190, 205, 225, 220);

// ---- quick-menu entries -----------------------------------------------------
// Each entry carries the nav target it surfaces through PauseNav() on accept.
// "@back" pops to the previous screen (the host's back-stack).
struct Entry { const char* label; const char* nav; };
const Entry ENTRIES[] = {
    { "Resume",      "@back"     },
    { "Metrics",     "status"    },
    { "Settings",    "options"   },
    { "Back to hub", "world_map" },
    { "Quit",        "@back"     },
};
constexpr int N_ENTRIES = int(sizeof(ENTRIES) / sizeof(ENTRIES[0]));

int g_sel = 0;
const char* g_nav = nullptr;

// ---- panel geometry (1280x720) — centred quick menu -------------------------
constexpr float PANEL_W = 420.0f;
constexpr float HEADER_H = 52.0f;
constexpr float ROW_H = 56.0f;
constexpr float ROW_PAD = 16.0f;
constexpr float PANEL_H = HEADER_H + N_ENTRIES * ROW_H + 2 * ROW_PAD;
const float PANEL_X = (REF_W - PANEL_W) * 0.5f;
const float PANEL_Y = (REF_H - PANEL_H) * 0.5f;

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() { g_sel = 0; g_nav = nullptr; }
void Input(const ScreenInput& in) {
    if (in.up)   g_sel = (g_sel + N_ENTRIES - 1) % N_ENTRIES;
    if (in.down) g_sel = (g_sel + 1) % N_ENTRIES;
    if (in.accept) g_nav = ENTRIES[std::clamp(g_sel, 0, N_ENTRIES - 1)].nav;
    if (in.cancel) g_nav = "@back";    // B resumes
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

void Draw(double openSec) {
    const float t = (float)ComputeMotion(openSec, 0.0, 8.0);

    // ---- scrim over the scene behind (the host draws the scene; we dim it) ----
    DrawRect({ 0, 0 }, { REF_W, REF_H }, WithAlpha(C_SCRIM, t));

    // ---- centred menu panel: body + caption strip + thin accent border -------
    const float px = PANEL_X, py = PANEL_Y;
    DrawRect({ px, py }, { px + PANEL_W, py + PANEL_H }, WithAlpha(C_PANEL, t));
    DrawRect({ px, py }, { px + PANEL_W, py + HEADER_H }, WithAlpha(C_PANEL_CAP, t));
    DrawRect({ px + 12, py + HEADER_H - 2 }, { px + PANEL_W - 12, py + HEADER_H }, WithAlpha(C_RULE, t));
    // 1px accent border around the panel
    DrawRect({ px, py }, { px + PANEL_W, py + 1 }, WithAlpha(C_BORDER, t));
    DrawRect({ px, py + PANEL_H - 1 }, { px + PANEL_W, py + PANEL_H }, WithAlpha(C_BORDER, t));
    DrawRect({ px, py }, { px + 1, py + PANEL_H }, WithAlpha(C_BORDER, t));
    DrawRect({ px + PANEL_W - 1, py }, { px + PANEL_W, py + PANEL_H }, WithAlpha(C_BORDER, t));
    // caption
    SetFont(g_fDF);
    DrawTextAligned({ px + 18, py }, { px + PANEL_W - 14, py + HEADER_H }, 26.0f,
                    WithAlpha(C_TITLE, t), "Menu", Align::Left, true, true);
    ResetFont();

    // ---- menu entries --------------------------------------------------------
    const float rowsTop = py + HEADER_H + ROW_PAD;
    const float rowL = px + ROW_PAD, rowR = px + PANEL_W - ROW_PAD;
    for (int i = 0; i < N_ENTRIES; ++i) {
        float top = rowsTop + i * ROW_H;
        bool sel = (i == g_sel);
        if (sel && t > 0.5f)
            DrawVGradient({ rowL, top + 4 }, { rowR, top + ROW_H - 6 },
                          WithAlpha(C_SEL_TOP, t), WithAlpha(C_SEL_BOT, t));
        SetFont(g_fSeurat);
        DrawTextAligned({ rowL + 18.0f, top }, { rowR - 18.0f, top + ROW_H }, 28.0f,
                        WithAlpha(sel ? C_TEXT_SEL : C_TEXT, t), ENTRIES[i].label,
                        Align::Left, true, true);
        ResetFont();
    }

    // ---- footer --------------------------------------------------------------
    {
        SetFont(g_fRodin);
        float fy = py + PANEL_H + 22.0f;
        DrawTextAligned({ px, fy }, { px + PANEL_W, fy + 26.0f }, 20.0f,
                        WithAlpha(C_FOOTER, t), "Up/Down  Move    Enter  Select    Esc  Resume",
                        Align::Center, true, true);
        ResetFont();
    }
}

} // namespace

void PauseInit() { Init(); }
void PauseDraw(double openSeconds) { Draw(openSeconds); }
void PauseInput(const ScreenInput& in) { Input(in); }
void PauseReset() { Reset(); }
const char* PauseNav() { return Nav(); }
