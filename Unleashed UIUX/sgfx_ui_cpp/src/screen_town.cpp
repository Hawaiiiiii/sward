// =============================================================================
// screen_town.cpp — the operator action hub, in the cinematic green look shared
// with the settings family (green_chrome.h): a green container listing the tool's
// actions with a play affordance, and a right info panel describing the focused one
// (and the profile it runs against, from the live data bridge). Adapted for actions
// rather than settings — a run affordance, not cycling value cells. Primitives +
// fonts only. Running an action navigates in-flow (the nav targets are unchanged).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "sgfx_data.h"
#include "green_chrome.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace ui;

namespace {

struct Action { const char* label; const char* info1; const char* info2; };
const Action ACTIONS[] = {
    { "Run preflight",       "Full SG-side checks across",   "anchors, constants, carpaints." },
    { "Capture screenshots", "Export via the BMW pipeline",  "and snap the test views."       },
    { "Check delivery",      "Readiness across the car",     "models and their changelogs."   },
    { "Daily digest",        "Run every live profile and",   "summarise the morning state."   },
    { "Scan unused Lua",     "Find Lua files that survived", "into the project root."         },
    { "Manual review",       "Open the queue of items",      "awaiting a human verdict."      },
    { "Live 3D car",         "Open the live 3D view of",     "this profile's car."            },
    { "Ambient tests",       "Screenshot-test coverage for", "the Ambient Layer scenes."      },
    { "Fleet readiness",     "Export + QA-camera state",     "across every car in the repo."  },
    { "Car tests",           "Screenshot-test state across", "the exported cars."             },
};
constexpr int ACTION_COUNT = int(sizeof(ACTIONS) / sizeof(ACTIONS[0]));

const char* ActiveProfile() { return sgfx::Get().run.activeProfile.c_str(); }

const uint32_t C_LABEL  = RGBA(236, 244, 236, 255);
const uint32_t C_DESC   = RGBA(190, 210, 190, 255);
const uint32_t C_PLAY   = RGBA(0, 150, 0, 255);
const uint32_t C_RUNMSG = RGBA(146, 255, 49, 255);

// geometry: the settings-family containers
constexpr float GRID = chrome::GRID;
constexpr float SP_X0 = 33, SP_Y0 = 117, SP_X1 = 843, SP_Y1 = 604;
constexpr float IP_X0 = 868, IP_Y0 = 117, IP_X1 = 1246, IP_Y1 = 604;
constexpr float CLIP_X = SP_X0 + GRID * 2;
constexpr float ROWS_TOP = SP_Y0 + GRID * 2 + 22.0f;
constexpr float ROW_H = 66.0f;
constexpr float OPT_W = GRID * 80;          // full inner width (no value column)
constexpr float LABEL_X = SP_X0 + GRID * 2 + GRID;

int    g_sel = 0, g_prevSel = 0;
double g_moveStart = -100.0, g_msgStart = -100.0;
const char* g_msg = "";
const char* g_nav = nullptr;
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() { g_sel = 0; g_prevSel = 0; g_moveStart = -100.0; g_msgStart = -100.0; g_msg = ""; g_nav = nullptr; }
void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = std::max(0, g_sel - 1);
        if (in.down) g_sel = std::min(ACTION_COUNT - 1, g_sel + 1);
        g_moveStart = Now();
    }
    if (in.accept) {
        static const char* const TARGET[ACTION_COUNT] = {
            "loading>sonic_hud", "result_ex", "result", "result_ex", "balloon", "mediaroom", "carview", "ambient", "fleet", "qatests",
        };
        const char* t = TARGET[g_sel];
        if (t[0]) g_nav = t;
        else { g_msg = ACTIONS[g_sel].label; g_msgStart = Now(); }
    }
    if (in.cancel) g_nav = "@back";
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

// a filled play triangle (run affordance) pointing right.
void PlayTri(float x, float cy, float h, uint32_t c, bool add = false) {
    const float w = h * 0.85f;
    const V2 v[4] = { { x, cy - h*0.5f }, { x, cy + h*0.5f }, { x + w, cy }, { x + w, cy } };
    const uint32_t cc[4] = { c, c, c, c };
    DrawQuadGradient(v, cc, add);
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "ACTIONS");
    chrome::Container(SP_X0, SP_Y0, SP_X1, SP_Y1, true,  b.line, b.outer, b.inner, b.bg);
    chrome::Container(IP_X0, IP_Y0, IP_X1, IP_Y1, false, b.line, b.outer, b.inner, b.bg);

    // selection bar (eased gold->green)
    if (t > 0.4f) {
        float mt = (float)ComputeMotion(g_moveStart, 0.0, 8.0);
        float slot = Lerp((float)g_prevSel, (float)g_sel, mt);
        float ry = ROWS_TOP + slot * ROW_H;
        uint32_t gold = WithAlpha(chrome::C_SEL_TL, t), grn = WithAlpha(chrome::C_SEL_BR, t);
        uint32_t mid  = WithAlpha(ColourLerp(chrome::C_SEL_TL, chrome::C_SEL_BR, 0.5f), t);
        DrawQuadGradient({ CLIP_X, ry + 3 }, { CLIP_X + OPT_W, ry + ROW_H - 6 }, gold, mid, grn, mid);
    }

    for (int i = 0; i < ACTION_COUNT; ++i) {
        float top = ROWS_TOP + i * ROW_H;
        bool sel = (i == g_sel);
        SetFont(g_fSeurat);
        DrawTextAligned({ LABEL_X, top }, { SP_X1 - GRID*4 - 40, top + ROW_H }, 28.0f,
                        WithAlpha(C_LABEL, t), ACTIONS[i].label, Align::Left, true, true);
        // play affordance, right edge — brighter on the focused row
        float cy = top + ROW_H * 0.5f, px = SP_X1 - GRID*2 - 34.0f;
        if (sel) { PlayTri(px - 2, cy, 22.0f, WithAlpha(RGBA(255,128,255,255), t), true); }   // magenta halo
        PlayTri(px, cy, 18.0f, WithAlpha(sel ? chrome::C_OK : C_PLAY, t));
    }

    // right info panel: the focused action + the profile it runs against
    {
        const float ix0 = IP_X0 + GRID*2, ix1 = IP_X1 - GRID*2;
        const Action& s = ACTIONS[std::clamp(g_sel, 0, ACTION_COUNT - 1)];
        SetFont(g_fRodin);
        DrawTextAligned({ ix0, IP_Y0 + 26.0f }, { ix1, IP_Y0 + 48.0f }, 15.0f, WithAlpha(RGBA(150,190,150,255), t), "PROFILE", Align::Left, true, true);
        DrawTextAligned({ ix0, IP_Y0 + 46.0f }, { ix1, IP_Y0 + 78.0f }, 26.0f, WithAlpha(chrome::C_TITLE, t), ActiveProfile(), Align::Left, true, true);
        DrawRect({ ix0, IP_Y0 + 96.0f }, { ix1, IP_Y0 + 97.0f }, WithAlpha(RGBA(0,89,0,180), t));
        SetFont(g_fDF);
        DrawTextAligned({ ix0, IP_Y0 + 120.0f }, { ix1, IP_Y0 + 156.0f }, 30.0f, WithAlpha(chrome::C_TITLE, t), s.label, Align::Left, true, true);
        SetFont(g_fSeurat);
        DrawText({ ix0, IP_Y0 + 170.0f }, 22.0f, WithAlpha(C_DESC, t), s.info1);
        DrawText({ ix0, IP_Y0 + 198.0f }, 22.0f, WithAlpha(C_DESC, t), s.info2);
        DrawText({ ix0, IP_Y0 + 250.0f }, 20.0f, WithAlpha(chrome::C_OK, t), "Press Enter to run.");
    }

    // transient "running" feedback for the no-nav action
    double age = Now() - g_msgStart;
    if (g_msgStart > 0.0 && age < 1.6) {
        float ma = std::min(1.0f, (float)((1.6 - age) / 0.4));
        char line[96]; std::snprintf(line, sizeof line, "Running %s on %s", g_msg, ActiveProfile());
        SetFont(g_fSeurat);
        DrawTextAligned({ SP_X0, 628.0f }, { SP_X1, 654.0f }, 22.0f, WithAlpha(C_RUNMSG, ma), line, Align::Center, true, true);
        ResetFont();
    }

    // footer
    SetFont(g_fRodin);
    DrawText({ 250, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Up/Down  Move");
    DrawText({ 470, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Enter  Run");
    DrawText({ 660, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back");
    ResetFont();
}

} // namespace

void TownInit() { Init(); }
void TownDraw(double openSeconds) { Draw(openSeconds); }
void TownInput(const ScreenInput& in) { Input(in); }
void TownReset() { Reset(); }
const char* TownNav() { return Nav(); }
