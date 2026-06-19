// =============================================================================
// screen_fleet.cpp — fleet readiness across the 3D-car models repo, green chrome.
// Reads fleet::Scan() and lays the cars out in a multi-column grid: each shows the
// brand/id, an "export ready" dot (export/exported.ramses present), and the number of
// QA camera-perspective sets defined. Up/Down scroll. Read-only; it reports repo state
// (the same root the 3D viewer renders from). bmw_git_root unset = a clear hint.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "green_chrome.h"
#include "fleet.h"

#include <cstdio>
#include <string>
#include <algorithm>

using namespace ui;

namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
fleet::Roster g_r;
int g_off = 0;   // grid-row scroll offset

constexpr int COLS = 3;
constexpr int ROWS = 12;

int MaxOff() {
    const int rows = ((int)g_r.cars.size() + COLS - 1) / COLS;
    return std::max(0, rows - ROWS);
}

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() { g_r = fleet::Scan(); g_off = 0; }
void Input(const ScreenInput& in) {
    if (in.down && g_off < MaxOff()) ++g_off;
    if (in.up   && g_off > 0)        --g_off;
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "FLEET");
    chrome::Container(33, 130, 1246, 600, true, b.line, b.outer, b.inner, b.bg);

    if (!g_r.configured || !g_r.rootExists || g_r.cars.empty()) {
        SetFont(g_fSeurat);
        DrawTextAligned({ 90, 300 }, { 1190, 350 }, 22.0f, WithAlpha(chrome::C_DIM, t),
            !g_r.configured ? "Set \"bmw_git_root\" in viewer3d.json to your digital-3d-car-models checkout."
                            : "No cars found under <root>/cars.", Align::Center, true, true);
        ResetFont();
        SetFont(g_fRodin); DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back"); ResetFont();
        return;
    }

    // summary + legend
    SetFont(g_fSeurat);
    char sum[200];
    std::snprintf(sum, sizeof sum, "%zu cars      %d exported      %d with QA cameras      %zu brands",
                  g_r.cars.size(), g_r.exportedCount, g_r.withPersp, g_r.brands.size());
    DrawTextAligned({ 70, 158 }, { 1240, 184 }, 17.0f, WithAlpha(chrome::C_DESC, t), sum, Align::Left, true, true);
    DrawRect({ 1056, 168 }, { 1066, 178 }, WithAlpha(chrome::C_OK, t));
    DrawText({ 1074, 162 }, 13.0f, WithAlpha(chrome::C_DIM, t), "export ready");
    DrawText({ 1180, 162 }, 13.0f, WithAlpha(chrome::C_TITLE, t), "N");
    DrawText({ 1196, 162 }, 13.0f, WithAlpha(chrome::C_DIM, t), "QA sets");
    DrawRect({ 70, 200 }, { 1210, 201 }, WithAlpha(chrome::C_LINE, t));
    ResetFont();

    // grid
    const float x0 = 70, y0 = 216, colW = 380, rowH = 27;
    const int total = (int)g_r.cars.size();
    SetFont(g_fRodin);
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            const int idx = (g_off + row) * COLS + col;
            if (idx >= total) continue;
            const fleet::Car& c = g_r.cars[idx];
            const float cx = x0 + colW * (float)col;
            const float cy = y0 + rowH * (float)row;
            char label[80]; std::snprintf(label, sizeof label, "%s/%s", c.brand.c_str(), c.id.c_str());
            DrawText({ cx, cy }, 15.0f, WithAlpha(chrome::C_DESC, t), label);
            DrawRect({ cx + 250, cy + 4 }, { cx + 260, cy + 14 },
                     WithAlpha(c.exported ? chrome::C_OK : RGBA(80, 100, 80, 255), t));
            char pc[8]; std::snprintf(pc, sizeof pc, "%d", c.perspSets);
            DrawText({ cx + 286, cy }, 15.0f, WithAlpha(c.perspSets > 0 ? chrome::C_TITLE : chrome::C_DIM, t), pc);
        }
    }
    ResetFont();

    if (MaxOff() > 0) {
        SetFont(g_fSeurat);
        char sc[48];
        std::snprintf(sc, sizeof sc, "%d - %d of %d", g_off * COLS + 1,
                      std::min(total, (g_off + ROWS) * COLS), total);
        DrawTextAligned({ 900, 556 }, { 1210, 580 }, 14.0f, WithAlpha(chrome::C_DIM, t), sc, Align::Right, true, true);
        ResetFont();
    }

    SetFont(g_fRodin);
    DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t),
             MaxOff() > 0 ? "Up / Down  Scroll        Esc  Back" : "Esc  Back");
    ResetFont();
}

} // namespace

void FleetInit() { Init(); }
void FleetDraw(double openSeconds) { Draw(openSeconds); }
void FleetInput(const ScreenInput& in) { Input(in); }
void FleetReset() { Reset(); }
