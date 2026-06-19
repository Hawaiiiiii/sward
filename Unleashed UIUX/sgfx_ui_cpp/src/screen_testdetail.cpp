// =============================================================================
// screen_testdetail.cpp — per-car screenshot-test detail, green chrome. Opened from the
// QA Tests board (it hands off the picked car); shows every baseline test for that car
// and whether the latest run differs from it, differing tests first. The actionable
// detail behind "this car differs" — exactly which views regressed. Read-only.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "green_chrome.h"
#include "fleet.h"

#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

using namespace ui;

namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
std::string g_car;
std::vector<fleet::TestResult> g_tests;
int g_off = 0, g_differ = 0;

constexpr int COLS = 2, ROWS = 13;
int MaxOff() { const int rows = ((int)g_tests.size() + COLS - 1) / COLS; return std::max(0, rows - ROWS); }

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() {
    g_car = fleet::TakeSelected();
    g_tests = fleet::CarTests(g_car);
    g_differ = 0; for (const auto& r : g_tests) if (r.differs) ++g_differ;
    g_off = 0;
}
void Input(const ScreenInput& in) {
    if (in.down && g_off < MaxOff()) ++g_off;
    if (in.up   && g_off > 0)        --g_off;
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, g_car.empty() ? "TEST DETAIL" : g_car.c_str());
    chrome::Container(33, 130, 1246, 600, true, b.line, b.outer, b.inner, b.bg);

    if (g_tests.empty()) {
        SetFont(g_fSeurat);
        DrawTextAligned({ 90, 300 }, { 1190, 350 }, 22.0f, WithAlpha(chrome::C_DIM, t),
            g_car.empty() ? "Open this from the QA Tests board (Enter on a car)."
                          : "No screenshot-test baselines found for this car.",
            Align::Center, true, true);
        ResetFont();
        SetFont(g_fRodin); DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back"); ResetFont();
        return;
    }

    SetFont(g_fSeurat);
    char sum[160];
    std::snprintf(sum, sizeof sum, "%zu tests        %d differ from baseline", g_tests.size(), g_differ);
    DrawTextAligned({ 70, 158 }, { 1240, 184 }, 17.0f, WithAlpha(chrome::C_DESC, t), sum, Align::Left, true, true);
    DrawRect({ 70, 200 }, { 1210, 201 }, WithAlpha(chrome::C_LINE, t));
    ResetFont();

    const float x0 = 80, y0 = 220, colW = 575, rowH = 27;
    const int total = (int)g_tests.size();
    SetFont(g_fRodin);
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            const int idx = (g_off + row) * COLS + col;
            if (idx >= total) continue;
            const fleet::TestResult& r = g_tests[idx];
            const float cx = x0 + colW * (float)col;
            const float cy = y0 + rowH * (float)row;
            DrawText({ cx, cy }, 14.0f, WithAlpha(r.differs ? RGBA(255, 130, 110, 255) : chrome::C_DIM, t), r.name.c_str());
            if (r.differs) DrawText({ cx + colW - 70, cy }, 14.0f, WithAlpha(RGBA(255, 110, 90, 255), t), "DIFF");
        }
    }
    ResetFont();

    if (MaxOff() > 0) {
        SetFont(g_fSeurat);
        char sc[48]; std::snprintf(sc, sizeof sc, "%d - %d of %d", g_off * COLS + 1, std::min(total, (g_off + ROWS) * COLS), total);
        DrawTextAligned({ 900, 556 }, { 1210, 580 }, 14.0f, WithAlpha(chrome::C_DIM, t), sc, Align::Right, true, true);
        ResetFont();
    }

    SetFont(g_fRodin);
    DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t),
             MaxOff() > 0 ? "Up / Down  Scroll        Esc  Back" : "Esc  Back");
    ResetFont();
}

} // namespace

void TestDetailInit() { Init(); }
void TestDetailDraw(double openSeconds) { Draw(openSeconds); }
void TestDetailInput(const ScreenInput& in) { Input(in); }
void TestDetailReset() { Reset(); }
