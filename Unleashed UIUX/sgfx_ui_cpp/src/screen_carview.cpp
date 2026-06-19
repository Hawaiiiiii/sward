// =============================================================================
// screen_carview.cpp — the in-shell 3D-car viewport, in the green operator look. A
// green viewport pane shows a rendered frame of the ACTUAL car: the external Ramses
// viewer renders it to a snapshot PNG (--readback --screenshot) and this screen
// displays it (composited as a D3D12 texture), with a side panel for the profile +
// QA view and controls. Left/Right pick the QA perspective; Enter renders it; LB
// opens the live external window; Esc backs out. Additive — touches no other screen,
// uses our green chrome. The only texture is the car snapshot (one reused slot).
//
// NOTE: a live car rendered *inside* the pane (not a still) needs Ramses rendered to
// a texture in-process; a re-parented child window does not compose over the shell's
// flip-model D3D12 swapchain. That in-process path is the planned next step.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "sgfx_data.h"
#include "green_chrome.h"
#include "viewer3d.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <filesystem>
#include <system_error>

using namespace ui;
namespace fs = std::filesystem;

namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
int g_carTex = -1;
float g_carAspect = 16.0f / 9.0f;
bool g_rendering = false;
double g_spawnStart = -100.0;
fs::file_time_type g_preMtime = fs::file_time_type::min();
std::string g_status, g_profile;
std::vector<std::string> g_views;   // "Authored" + the car's QA perspective sets
int g_viewIdx = 0;

constexpr float GRID  = chrome::GRID;
constexpr float VP_X0 = 33,  VP_Y0 = 130, VP_X1 = 900,  VP_Y1 = 600;   // viewport pane
constexpr float IP_X0 = 918, IP_Y0 = 130, IP_X1 = 1246, IP_Y1 = 600;   // info panel
const char* SNAP = "carview.png";

const std::string& CurView() { static const std::string a = "Authored"; return g_views.empty() ? a : g_views[g_viewIdx]; }

fs::file_time_type Mtime() {
    std::error_code ec;
    return fs::exists(SNAP, ec) ? fs::last_write_time(SNAP, ec) : fs::file_time_type::min();
}
float PngAspect(const char* path, float def) {   // read the PNG IHDR w/h without decoding
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return def;
    unsigned char h[24]; const size_t n = std::fread(h, 1, 24, f); std::fclose(f);
    if (n < 24 || h[0] != 0x89) return def;
    const unsigned w  = (h[16] << 24) | (h[17] << 16) | (h[18] << 8) | h[19];
    const unsigned ht = (h[20] << 24) | (h[21] << 16) | (h[22] << 8) | h[23];
    return ht ? (float)w / (float)ht : def;
}
void LoadCar() {   // reuse one texture slot across reloads (no per-render slot leak)
    std::error_code ec;
    if (!fs::exists(SNAP, ec)) return;
    g_carTex = (g_carTex >= 0) ? gfx::reloadTexture(g_carTex, SNAP) : gfx::loadTexture(SNAP);
    g_carAspect = PngAspect(SNAP, g_carAspect);
}
void StartRender() {
    g_preMtime = Mtime();
    const std::string view = (g_viewIdx > 0) ? CurView() : "";   // idx 0 = authored
    g_status = viewer3d::RenderSnapshot(g_profile, view);
    g_rendering = (g_status.rfind("Rendering", 0) == 0);
    g_spawnStart = Now();
}

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() {
    viewer3d::LiveStop();   // never carry a live render across an entry
    g_profile = sgfx::Get().run.activeProfile;
    g_views.clear(); g_views.push_back("Authored");
    for (const auto& s : viewer3d::ListPerspectiveSets(g_profile)) g_views.push_back(s);
    g_viewIdx = 0;
    g_status.clear(); g_rendering = false; g_spawnStart = -100.0;
    LoadCar();       // g_carTex persists across entries; reused, not re-allocated
    if (std::getenv("SGFX_LIVE")) {   // demo / CI: go straight to the live orbit (no still first)
        g_preMtime = Mtime();
        g_status = viewer3d::LiveStart(g_profile) ? "Live - orbiting" : "Live start failed";
    } else {
        StartRender();   // render the still car on entry (authored)
    }
}
void Input(const ScreenInput& in) {
    const int n = (int)g_views.size();
    if ((in.left || in.right || in.accept) && viewer3d::LiveActive()) viewer3d::LiveStop();  // snapshot actions exit live
    if (n > 0 && in.left)  g_viewIdx = (g_viewIdx + n - 1) % n;   // pick a QA view
    if (n > 0 && in.right) g_viewIdx = (g_viewIdx + 1) % n;
    if (in.accept) StartRender();                                // render the selected view (still)
    if (in.tabLeft)                                              // LB: live car in a separate window
        g_status = viewer3d::Launch(g_profile, g_viewIdx > 0 ? CurView() : "authored");
    if (in.tabRight) {                                           // RB: live orbiting car IN the pane
        if (viewer3d::LiveActive()) { viewer3d::LiveStop(); StartRender(); }   // back to the still
        else { g_preMtime = Mtime(); g_status = viewer3d::LiveStart(g_profile) ? "Live - orbiting" : "Could not start live (see viewer3d.json)"; }
    }
    if (in.cancel) viewer3d::LiveStop();                         // tear down before leaving
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "3D CAR");
    chrome::Container(VP_X0, VP_Y0, VP_X1, VP_Y1, true,  b.line, b.outer, b.inner, b.bg);
    chrome::Container(IP_X0, IP_Y0, IP_X1, IP_Y1, false, b.line, b.outer, b.inner, b.bg);

    // poll: reload the pane texture when the viewer writes a new frame
    if (viewer3d::LiveActive()) {
        if (Mtime() > g_preMtime) { LoadCar(); g_preMtime = Mtime(); }   // live: each new orbit frame
    } else if (g_rendering) {
        if (Mtime() > g_preMtime) { LoadCar(); g_rendering = false; }    // still: the one snapshot
        else if (Now() - g_spawnStart > 18.0) g_rendering = false;
    }

    // viewport: the car frame, fit (16:9) and centred in the pane
    const float px0 = VP_X0 + GRID*2, py0 = VP_Y0 + GRID*2, px1 = VP_X1 - GRID*2, py1 = VP_Y1 - GRID*2;
    if (g_carTex >= 0) {
        const float pw = px1 - px0, ph = py1 - py0, ar = g_carAspect;
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
    DrawTextAligned({ ix, IP_Y0 + 114 }, { iw, IP_Y0 + 134 }, 14.0f, WithAlpha(RGBA(150,190,150,255), t), "QA VIEW", Align::Left, true, true);
    DrawTextAligned({ ix, IP_Y0 + 132 }, { iw, IP_Y0 + 164 }, 22.0f, WithAlpha(chrome::C_DESC, t), CurView().c_str(), Align::Left, true, false);
    DrawText({ ix, IP_Y0 + 170 }, 15.0f, WithAlpha(chrome::C_DIM, t), "< >  change view");
    const bool live = viewer3d::LiveActive();
    DrawText({ ix, IP_Y0 + 204 }, 18.0f,
             WithAlpha(live ? RGBA(146,255,49,255) : (g_rendering ? RGBA(255,200,90,255) : (g_carTex >= 0 ? chrome::C_OK : chrome::C_DIM)), t),
             live ? "LIVE - orbiting" : (g_rendering ? "Rendering ..." : (g_carTex >= 0 ? "Ready" : "No frame yet")));
    if (!g_status.empty() && !g_rendering)
        DrawText({ ix, IP_Y0 + 230 }, 15.0f, WithAlpha(chrome::C_DIM, t), g_status.c_str());
    ResetFont();

    // footer
    SetFont(g_fRodin);
    DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "< >  View");
    DrawText({ 320, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Enter  Render");
    DrawText({ 540, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "LB  Window");
    DrawText({ 700, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "RB  Live");
    DrawText({ 850, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back");
    ResetFont();
}

} // namespace

void CarViewInit() { Init(); }
void CarViewDraw(double openSeconds) { Draw(openSeconds); }
void CarViewInput(const ScreenInput& in) { Input(in); }
void CarViewReset() { Reset(); }
