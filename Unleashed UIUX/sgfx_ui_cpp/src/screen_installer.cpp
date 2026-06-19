// =============================================================================
// screen_installer.cpp — the SETUP / onboarding screen, in the cinematic green
// look shared with the settings screen (green_chrome.h): a green container listing
// the tool's dependencies and their state, plus a right info panel for the focused
// step. State is colour-coded (green found/set, amber needs-install, dim not-set).
// Up/Down move; Enter continues, Esc backs out (host-driven). Primitives + fonts
// only — no game art.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "green_chrome.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

const uint32_t C_LABEL    = RGBA(236, 244, 236, 255);
const uint32_t C_DETAIL   = RGBA(150, 178, 150, 255);
const uint32_t C_SLOT_LBL = RGBA(150, 190, 150, 255);

enum State { FOUND, INSTALL, SET, NOTSET };
struct Step { const char* label; const char* detail; const char* help; State state; };
const Step STEPS[] = {
    { "RaConverter",         "Asset converter, required for export.",   "Found on this machine. Nothing to do.",                 FOUND   },
    { "RaCoHeadless",        "Headless export runner for screenshots.", "Not found. Press Enter to fetch and install it.",       INSTALL },
    { "Blender",             "Used by the geometry checks.",            "Found on this machine. Nothing to do.",                 FOUND   },
    { "Digital-3D-Car repo", "The car-models working copy the tool reads.", "Path is set and reachable.",                       SET     },
};
constexpr int STEP_COUNT = int(sizeof(STEPS) / sizeof(STEPS[0]));
int g_sel = 0;

// geometry: the settings-family container (matches the options screen)
constexpr float GRID = chrome::GRID;
constexpr float SP_X0 = 33, SP_Y0 = 117, SP_X1 = 843, SP_Y1 = 604;
constexpr float IP_X0 = 868, IP_Y0 = 117, IP_X1 = 1246, IP_Y1 = 604;
constexpr float CLIP_X = SP_X0 + GRID * 2;
constexpr float ROWS_TOP = SP_Y0 + GRID * 2 + 30.0f;
constexpr float ROW_H = 100.0f;
constexpr float OPT_W = GRID * 54;
constexpr float LABEL_X = SP_X0 + GRID * 2 + GRID;
constexpr float VAL_W = 168, VAL_H = GRID * 3.5f;
constexpr float VAL_X0 = SP_X1 - GRID * 2 - VAL_W - 18.0f;

const char* StateWord(State s) { return s == FOUND ? "found" : s == INSTALL ? "install" : s == SET ? "set" : "not set"; }
uint32_t StateCol(State s) { return (s == FOUND || s == SET) ? chrome::C_OK : (s == INSTALL ? chrome::C_WARN : chrome::C_DIM); }

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() { g_sel = 0; }
void Input(const ScreenInput& in) {
    if (in.up)   g_sel = std::max(0, g_sel - 1);
    if (in.down) g_sel = std::min(STEP_COUNT - 1, g_sel + 1);
}

// the state word, centred on a value plate: black outline + the state colour.
void DrawStateWord(const char* s, uint32_t col, float x0, float y0, float x1, float y1, float t) {
    float w = MeasureText(20.0f, s).x;
    float px = x0 + ((x1 - x0) - w) * 0.5f, py = y0 + ((y1 - y0) - 20.0f) * 0.5f;
    static const float O[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
    for (auto& o : O) DrawText({ px + o[0]*1.6f, py + o[1]*1.6f }, 20.0f, WithAlpha(chrome::C_BLACK, t), s);
    DrawText({ px, py }, 20.0f, WithAlpha(col, t), s);
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "SETUP");
    chrome::Container(SP_X0, SP_Y0, SP_X1, SP_Y1, true,  b.line, b.outer, b.inner, b.bg);
    chrome::Container(IP_X0, IP_Y0, IP_X1, IP_Y1, false, b.line, b.outer, b.inner, b.bg);

    // selection bar (gold->green) on the focused row
    if (t > 0.4f) {
        float ry = ROWS_TOP + g_sel * ROW_H;
        uint32_t gold = WithAlpha(chrome::C_SEL_TL, t), grn = WithAlpha(chrome::C_SEL_BR, t);
        uint32_t mid  = WithAlpha(ColourLerp(chrome::C_SEL_TL, chrome::C_SEL_BR, 0.5f), t);
        DrawQuadGradient({ CLIP_X, ry }, { CLIP_X + OPT_W, ry + ROW_H - 12.0f }, gold, mid, grn, mid);
    }

    for (int i = 0; i < STEP_COUNT; ++i) {
        const Step& s = STEPS[i];
        float top = ROWS_TOP + i * ROW_H;
        bool sel = (i == g_sel);
        SetFont(g_fSeurat);
        DrawTextAligned({ LABEL_X, top + 14.0f }, { VAL_X0 - 16.0f, top + 50.0f }, 28.0f,
                        WithAlpha(C_LABEL, t), s.label, Align::Left, true, true);
        SetFont(g_fRodin);
        DrawTextAligned({ LABEL_X, top + 50.0f }, { VAL_X0 - 16.0f, top + 80.0f }, 19.0f,
                        WithAlpha(C_DETAIL, t), s.detail, Align::Left, true, true);
        // value cell: plate + state light + state word
        float vy0 = top + (ROW_H - 12.0f - VAL_H) * 0.5f, vy1 = vy0 + VAL_H;
        chrome::Plate(VAL_X0, vy0, VAL_X0 + VAL_W, vy1, t);
        if (sel) chrome::SelectionArrows(VAL_X0, vy0, VAL_X0 + VAL_W, vy1, t);
        const float ls = 13.0f, lx = VAL_X0 + 14.0f, ly = vy0 + (VAL_H - ls) * 0.5f;
        if (s.state == FOUND || s.state == SET) { const float gs = ls+12, gx = lx-6, gy = ly-6; DrawRect({gx,gy},{gx+gs,gy+gs}, WithAlpha(RGBA(120,255,80,70), t), true); }
        DrawRect({ lx-1, ly-1 }, { lx+ls+1, ly+ls+1 }, WithAlpha(chrome::C_BLACK, t));
        DrawRect({ lx, ly }, { lx+ls, ly+ls }, WithAlpha(StateCol(s.state), t));
        SetFont(g_fRodin);
        DrawStateWord(StateWord(s.state), StateCol(s.state), VAL_X0 + 22.0f, vy0, VAL_X0 + VAL_W - 4.0f, vy1, t);
    }

    // right info panel: the focused step
    {
        const float ix0 = IP_X0 + GRID*2, ix1 = IP_X1 - GRID*2;
        const float wrapW = ix1 - ix0;
        const Step& s = STEPS[std::clamp(g_sel, 0, STEP_COUNT - 1)];
        SetFont(g_fDF);
        DrawTextAligned({ ix0, IP_Y0 + 40.0f }, { ix1, IP_Y0 + 78.0f }, 30.0f, WithAlpha(chrome::C_TITLE, t), s.label, Align::Left, true, true);
        SetFont(g_fRodin);
        DrawText({ ix0, IP_Y0 + 92.0f }, 22.0f, WithAlpha(StateCol(s.state), t), StateWord(s.state));
        // help text, wrapped
        SetFont(g_fSeurat);
        std::string full = s.help ? s.help : "";
        const float fsz = 24.0f, lineH = fsz + 6.0f;
        std::vector<std::string> lines; std::string cur; size_t w0 = 0;
        while (w0 <= full.size()) {
            size_t w1 = full.find(' ', w0); if (w1 == std::string::npos) w1 = full.size();
            std::string word = full.substr(w0, w1 - w0), trial = cur.empty() ? word : cur + " " + word;
            if (!cur.empty() && MeasureText(fsz, trial.c_str()).x > wrapW) { lines.push_back(cur); cur = word; } else cur = trial;
            if (w1 >= full.size()) break; w0 = w1 + 1;
        }
        if (!cur.empty()) lines.push_back(cur);
        float dy = IP_Y0 + 150.0f;
        for (const std::string& ln : lines) { DrawText({ ix0, dy }, fsz, WithAlpha(chrome::C_DESC, t), ln.c_str()); dy += lineH; }
    }

    // footer
    SetFont(g_fRodin);
    DrawText({ 250, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Up/Down  Select");
    DrawText({ 470, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Enter  Install / Continue");
    DrawText({ 760, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back");
    ResetFont();
}

} // namespace

void InstallerInit() { Init(); }
void InstallerDraw(double openSeconds) { Draw(openSeconds); }
void InstallerInput(const ScreenInput& in) { Input(in); }
void InstallerReset() { Reset(); }
