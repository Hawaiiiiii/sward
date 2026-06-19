// =============================================================================
// screen_carview.cpp — the in-shell 3D-car viewport, in the green operator look. A
// green viewport pane shows a rendered frame of the ACTUAL car: the external Ramses
// viewer renders it to a snapshot PNG (--readback --screenshot) and this screen
// displays it, with a side panel for the profile and controls. Enter re-renders;
// LB/RB opens the live external window; Esc backs out. Additive — it touches no
// other screen, and uses our green chrome (not the deprecated cinematic look). The
// only texture is the car snapshot; no game art.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "sgfx_data.h"
#include "green_chrome.h"
#include "viewer3d.h"

#include <cstdio>
#include <string>
#include <filesystem>
#include <system_error>

using namespace ui;
namespace fs = std::filesystem;

namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
int g_carTex = -1;
bool g_rendering = false;
double g_spawnStart = -100.0;
fs::file_time_type g_preMtime = fs::file_time_type::min();
std::string g_status, g_profile;

constexpr float GRID  = chrome::GRID;
constexpr float VP_X0 = 33,  VP_Y0 = 130, VP_X1 = 900,  VP_Y1 = 600;   // viewport pane
constexpr float IP_X0 = 918, IP_Y0 = 130, IP_X1 = 1246, IP_Y1 = 600;   // info panel
const char* SNAP = "carview.png";

fs::file_time_type Mtime() {
    std::error_code ec;
    return fs::exists(SNAP, ec) ? fs::last_write_time(SNAP, ec) : fs::file_time_type::min();
}
void LoadIfPresent() {
    std::error_code ec;
    if (fs::exists(SNAP, ec)) g_carTex = gfx::loadTexture(SNAP);
}
void StartRender() {
    g_preMtime = Mtime();
    g_status = viewer3d::RenderSnapshot(g_profile);
    g_rendering = (g_status.rfind("Rendering", 0) == 0);
    g_spawnStart = Now();
}

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() {
    g_profile = sgfx::Get().run.activeProfile;
    g_carTex = -1; g_status.clear(); g_rendering = false; g_spawnStart = -100.0;
    LoadIfPresent();
    StartRender();   // refresh the car on entry
}
void Input(const ScreenInput& in) {
    if (in.accept) StartRender();                                  // re-render the in-pane car
    if (in.tabLeft || in.tabRight) g_status = viewer3d::Launch(g_profile);  // live external window
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "3D CAR");
    chrome::Container(VP_X0, VP_Y0, VP_X1, VP_Y1, true,  b.line, b.outer, b.inner, b.bg);
    chrome::Container(IP_X0, IP_Y0, IP_X1, IP_Y1, false, b.line, b.outer, b.inner, b.bg);

    // poll: when the snapshot is rewritten by the viewer, (re)load it
    if (g_rendering) {
        if (Mtime() > g_preMtime) { g_carTex = gfx::loadTexture(SNAP); g_rendering = false; }
        else if (Now() - g_spawnStart > 18.0) g_rendering = false;   // give up waiting
    }

    // viewport: the car frame, fit (16:9) and centred in the pane
    const float px0 = VP_X0 + GRID*2, py0 = VP_Y0 + GRID*2, px1 = VP_X1 - GRID*2, py1 = VP_Y1 - GRID*2;
    if (g_carTex >= 0) {
        const float pw = px1 - px0, ph = py1 - py0, ar = 16.0f / 9.0f;
        float fw = pw, fh = pw / ar; if (fh > ph) { fh = ph; fw = ph * ar; }
        const float cx = (px0 + px1) * 0.5f, cy = (py0 + py1) * 0.5f;
        DrawImage(g_carTex, { cx - fw*0.5f, cy - fh*0.5f }, { cx + fw*0.5f, cy + fh*0.5f },
                  { 0, 0 }, { 1, 1 }, WithAlpha(chrome::C_WHITE, t));
    } else {
        SetFont(g_fSeurat);
        DrawTextAligned({ px0, py0 }, { px1, py1 }, 26.0f, WithAlpha(chrome::C_DIM, t),
                        g_rendering ? "Rendering the car ..." : "Press Enter to render the car",
                        Align::Center, true, true);
        ResetFont();
    }

    // info panel
    const float ix = IP_X0 + GRID*2, iw = IP_X1 - GRID*2;
    SetFont(g_fRodin);
    DrawTextAligned({ ix, IP_Y0 + 26 }, { iw, IP_Y0 + 48 }, 15.0f, WithAlpha(RGBA(150,190,150,255), t), "PROFILE", Align::Left, true, true);
    DrawTextAligned({ ix, IP_Y0 + 46 }, { iw, IP_Y0 + 82 }, 30.0f, WithAlpha(chrome::C_TITLE, t), g_profile.c_str(), Align::Left, true, true);
    DrawRect({ ix, IP_Y0 + 96 }, { iw, IP_Y0 + 97 }, WithAlpha(RGBA(0,89,0,180), t));
    SetFont(g_fSeurat);
    DrawText({ ix, IP_Y0 + 118 }, 20.0f, WithAlpha(chrome::C_DESC, t), "Authored camera view.");
    DrawText({ ix, IP_Y0 + 152 }, 18.0f,
             WithAlpha(g_rendering ? RGBA(255,200,90,255) : (g_carTex >= 0 ? chrome::C_OK : chrome::C_DIM), t),
             g_rendering ? "Rendering ..." : (g_carTex >= 0 ? "Ready" : "No frame yet"));
    if (!g_status.empty() && !g_rendering)
        DrawText({ ix, IP_Y0 + 182 }, 16.0f, WithAlpha(chrome::C_DIM, t), g_status.c_str());
    ResetFont();

    // footer
    SetFont(g_fRodin);
    DrawText({ 250, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Enter  Render");
    DrawText({ 470, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "LB/RB  Live window");
    DrawText({ 720, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back");
    ResetFont();
}

} // namespace

void CarViewInit() { Init(); }
void CarViewDraw(double openSeconds) { Draw(openSeconds); }
void CarViewInput(const ScreenInput& in) { Input(in); }
void CarViewReset() { Reset(); }
