// =============================================================================
// screen_installer.cpp — the SETUP / onboarding screen. A single dark-IDE panel
// titled "Setup" listing the tool's dependency / onboarding steps and their state
// (found / install / set). Built from primitives + text only (no chrome art, no
// third-party atlas) in the dark-IDE neutral aesthetic shared with screen_status.cpp
// / screen_town.cpp / screen_options.cpp. Logo-only header (no wordmark).
// Up/Down move the cursor; Enter continues, Esc backs out (host-driven).
// State per row is colour-coded: green found / amber install / dim not-set.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0, g_logoTex = -1;

// ---- palette (dark, neutral — matches screen_status.cpp / screen_options.cpp) -
const uint32_t C_BG_TOP   = RGBA(12, 20, 38, 255), C_BG_BOT = RGBA(5, 9, 18, 255);
const uint32_t C_PANEL    = RGBA(16, 24, 40, 235);
const uint32_t C_PANEL_CAP= RGBA(10, 16, 28, 255);
const uint32_t C_SEL_TOP  = RGBA(64, 150, 235, 225), C_SEL_BOT = RGBA(28, 92, 180, 225);
const uint32_t C_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t C_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t C_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t C_RULE     = RGBA(120, 170, 230, 90);
const uint32_t C_LABEL    = RGBA(150, 170, 196, 255);
const uint32_t C_FOUND    = RGBA(120, 230, 140, 255);   // dependency present (green)
const uint32_t C_INSTALL  = RGBA(235, 200, 90, 255);    // needs installing (amber)
const uint32_t C_DIM      = RGBA(120, 138, 158, 255);   // not yet set (dim)
const uint32_t C_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t C_WHITE    = RGBA(255, 255, 255, 255);

// ---- onboarding step model --------------------------------------------------
// Each step is a dependency or configuration the tool needs before a run. The
// state drives the colour of the right-aligned status word (representative until
// a live probe supplies the real result).
enum State { FOUND, INSTALL, SET, NOTSET };
struct Step { const char* label; const char* detail; State state; };
const Step STEPS[] = {
    { "RaConverter",        "Asset converter, required for export.",     FOUND   },
    { "RaCoHeadless",       "Headless export runner for screenshots.",  INSTALL },
    { "Blender",            "Used by the geometry checks.",             FOUND   },
    { "Digital-3D-Car repo","The car-models working copy the tool reads.", SET  },
};
constexpr int STEP_COUNT = int(sizeof(STEPS) / sizeof(STEPS[0]));

int g_sel = 0;

// ---- layout (reference px) --------------------------------------------------
constexpr float RULE_Y = 118.0f;
constexpr float PANEL_X = 280.0f, PANEL_Y = 158.0f, PANEL_W = 720.0f, PANEL_H = 404.0f;
constexpr float HEADER_H = 52.0f;
constexpr float ROW_H = 78.0f;
constexpr float ROW_PAD = 14.0f;

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double TITLE_FRAMES = 14.0, PANEL_FRAMES = 16.0;
constexpr double FOOT_OFFSET = 10.0, FOOT_FRAMES = 12.0;

const char* StateWord(State s) {
    switch (s) {
        case FOUND:   return "found";
        case INSTALL: return "install";
        case SET:     return "set";
        default:      return "not set";
    }
}
uint32_t StateColour(State s) {
    switch (s) {
        case FOUND: case SET: return C_FOUND;
        case INSTALL:         return C_INSTALL;
        default:              return C_DIM;
    }
}

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
    if (g_logoTex < 0)  g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
}
void Reset() { g_sel = 0; }
void Input(const ScreenInput& in) {
    if (in.up)   g_sel = std::max(0, g_sel - 1);
    if (in.down) g_sel = std::min(STEP_COUNT - 1, g_sel + 1);
}

// a neutral dark panel with a caption strip + rule (screen_town.cpp idiom).
void DrawPanel(float x, float y, float w, float h, float t, const char* caption) {
    DrawRect({ x, y }, { x + w, y + h }, WithAlpha(C_PANEL, t));
    DrawRect({ x, y }, { x + w, y + HEADER_H }, WithAlpha(C_PANEL_CAP, t));
    DrawRect({ x + 12, y + HEADER_H - 2 }, { x + w - 12, y + HEADER_H }, WithAlpha(C_RULE, t));
    SetFont(g_fDF);
    DrawTextAligned({ x + 18, y }, { x + w - 14, y + HEADER_H }, 26.0f,
                    WithAlpha(C_TITLE, t), caption, Align::Left, true, true);
    ResetFont();
}

// host logo drops into the top-left slot; if it failed to load, draw NOTHING.
void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 40 }, { 40 + w, 40 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, t));
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);

    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float panelT = (float)ComputeMotion(openSec, 0.0, PANEL_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ===== HEADER: logo slot only (no wordmark / title text) ==================
    DrawLogoSlot(titleT);
    DrawRect({ PANEL_X, RULE_Y }, { 1000.0f, RULE_Y + 2.0f }, WithAlpha(C_RULE, titleT));

    // ===== SETUP PANEL ========================================================
    const float px = PANEL_X, py = PANEL_Y + (1.0f - panelT) * 24.0f;
    DrawPanel(px, py, PANEL_W, PANEL_H, panelT, "Setup");
    const float rowsTop = py + HEADER_H + 12.0f;
    const float rowL = px + ROW_PAD, rowR = px + PANEL_W - ROW_PAD;

    // selection highlight
    if (panelT > 0.5f) {
        float hy = rowsTop + g_sel * ROW_H;
        DrawVGradient({ rowL, hy + 3 }, { rowR, hy + ROW_H - 6 },
                      WithAlpha(C_SEL_TOP, panelT), WithAlpha(C_SEL_BOT, panelT));
    }

    for (int i = 0; i < STEP_COUNT; ++i) {
        float top = rowsTop + i * ROW_H;
        bool selected = (i == g_sel);
        const Step& s = STEPS[i];
        // label (name) on the left
        SetFont(g_fSeurat);
        DrawTextAligned({ rowL + 18.0f, top + 6.0f }, { rowR - 160.0f, top + 40.0f }, 26.0f,
                        WithAlpha(selected ? C_TEXT_SEL : C_TEXT, panelT),
                        s.label, Align::Left, true, true);
        // detail sub-line
        SetFont(g_fRodin);
        DrawTextAligned({ rowL + 18.0f, top + 40.0f }, { rowR - 160.0f, top + 70.0f }, 18.0f,
                        WithAlpha(C_LABEL, panelT), s.detail, Align::Left, true, true);
        // state word, right-aligned + colour-coded
        DrawTextAligned({ rowR - 150.0f, top + 6.0f }, { rowR - 8.0f, top + ROW_H - 12.0f }, 24.0f,
                        WithAlpha(StateColour(s.state), panelT), StateWord(s.state),
                        Align::Right, true, true);
        ResetFont();
    }

    // ===== FOOTER =============================================================
    {
        SetFont(g_fRodin);
        DrawRect({ PANEL_X, 612.0f }, { 1000.0f, 614.0f }, WithAlpha(C_RULE, footT));
        DrawText({ PANEL_X, 628.0f }, 20.0f, WithAlpha(C_FOOTER, footT), "Enter  Continue");
        DrawText({ PANEL_X + 250.0f, 628.0f }, 20.0f, WithAlpha(C_FOOTER, footT), "Esc  Back");
        ResetFont();
    }
}

} // namespace

void InstallerInit() { Init(); }
void InstallerDraw(double openSeconds) { Draw(openSeconds); }
void InstallerInput(const ScreenInput& in) { Input(in); }
void InstallerReset() { Reset(); }
