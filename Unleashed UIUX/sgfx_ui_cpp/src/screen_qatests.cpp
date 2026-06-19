// =============================================================================
// screen_qatests.cpp — 3D-car screenshot-test state, in the green operator look. The
// core QA-Hero workflow: each exported car keeps export/tests/{expected,actuals,diff};
// a non-empty diff is a visual regression. This reads fleet::Scan() (which counts those)
// and lists every tested car with its status -- DIFF (regression) first, then not run,
// then pass -- with its baseline + diff counts. Read-only; reports what a run left on
// disk. Real data straight from bmw_git_root (the same root the 3D viewer renders from).
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
fleet::Roster g_r;
std::vector<const fleet::Car*> g_rows;   // tested cars, regressions first
int g_off = 0;
int g_cursor = 0;

constexpr int ROWS = 13;

int Prio(const std::string& s) { return s == "diff" ? 0 : s == "notrun" ? 1 : 2; }
int MaxOff() { return std::max(0, (int)g_rows.size() - ROWS); }

uint32_t StatusColor(const std::string& s) {
    if (s == "pass")   return chrome::C_OK;
    if (s == "diff")   return RGBA(255, 110, 90, 255);
    if (s == "notrun") return chrome::C_WARN;
    return chrome::C_DIM;
}
const char* StatusTag(const std::string& s) {
    if (s == "pass")   return "MATCHES";   // a local run that matches the committed baseline
    if (s == "diff")   return "DIFFERS";   // local actuals differ from baseline -> review
    if (s == "notrun") return "NOT RUN";
    return "-";
}

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() {
    g_r = fleet::Scan();
    g_rows.clear();
    for (const auto& c : g_r.cars) if (c.testExpected > 0) g_rows.push_back(&c);
    std::sort(g_rows.begin(), g_rows.end(), [](const fleet::Car* a, const fleet::Car* b) {
        const int pa = Prio(a->testStatus), pb = Prio(b->testStatus);
        if (pa != pb) return pa < pb;
        return a->brand != b->brand ? a->brand < b->brand : a->id < b->id;
    });
    g_off = 0; g_cursor = 0;
}
void Input(const ScreenInput& in) {
    const int total = (int)g_rows.size();
    if (total == 0) return;
    if (in.down) g_cursor = std::min(g_cursor + 1, total - 1);
    if (in.up)   g_cursor = std::max(g_cursor - 1, 0);
    if (g_cursor < g_off)         g_off = g_cursor;
    if (g_cursor >= g_off + ROWS) g_off = g_cursor - ROWS + 1;
    if (in.accept) fleet::SetSelected(g_rows[g_cursor]->id);   // hand off to the test detail
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "QA TESTS");
    chrome::Container(33, 130, 1246, 600, true, b.line, b.outer, b.inner, b.bg);

    if (!g_r.configured || !g_r.rootExists || g_rows.empty()) {
        SetFont(g_fSeurat);
        DrawTextAligned({ 90, 300 }, { 1190, 350 }, 22.0f, WithAlpha(chrome::C_DIM, t),
            !g_r.configured ? "Set \"bmw_git_root\" in viewer3d.json to your digital-3d-car-models checkout."
                            : "No cars with screenshot tests (export/tests/expected) were found.",
            Align::Center, true, true);
        ResetFont();
        SetFont(g_fRodin); DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back"); ResetFont();
        return;
    }

    // summary
    SetFont(g_fSeurat);
    char sum[200];
    std::snprintf(sum, sizeof sum, "%d cars with tests      %d match baseline      %d differ      %d not run",
                  g_r.tested, g_r.testPass, g_r.testDiff, g_r.testNotrun);
    DrawTextAligned({ 70, 158 }, { 1240, 184 }, 17.0f, WithAlpha(chrome::C_DESC, t), sum, Align::Left, true, true);
    DrawRect({ 70, 200 }, { 1210, 201 }, WithAlpha(chrome::C_LINE, t));
    ResetFont();

    // column headers
    SetFont(g_fRodin);
    const float x0 = 80, y0 = 222, rowH = 27;
    DrawText({ x0, y0 - 30 }, 13.0f, WithAlpha(RGBA(150, 190, 150, 255), t), "CAR");
    DrawText({ x0 + 360, y0 - 30 }, 13.0f, WithAlpha(RGBA(150, 190, 150, 255), t), "STATE");
    DrawText({ x0 + 560, y0 - 30 }, 13.0f, WithAlpha(RGBA(150, 190, 150, 255), t), "BASELINES");
    DrawText({ x0 + 760, y0 - 30 }, 13.0f, WithAlpha(RGBA(150, 190, 150, 255), t), "DIFFS");

    const int total = (int)g_rows.size();
    for (int row = 0; row < ROWS && (g_off + row) < total; ++row) {
        const fleet::Car* c = g_rows[g_off + row];
        const float ry = y0 + rowH * (float)row;
        const bool sel = (g_off + row == g_cursor);
        if (sel) {
            chrome::Plate(x0 - 10, ry - 3, 1185, ry + rowH - 7, t * 0.6f);
            DrawRect({ x0 - 10, ry - 3 }, { x0 - 7, ry + rowH - 7 }, WithAlpha(chrome::C_TITLE, t));
        }
        const uint32_t col = StatusColor(c->testStatus);
        char label[80]; std::snprintf(label, sizeof label, "%s/%s", c->brand.c_str(), c->id.c_str());
        DrawText({ x0, ry }, 16.0f, WithAlpha(sel ? chrome::C_LABEL : chrome::C_DESC, t), label);
        DrawText({ x0 + 360, ry }, 16.0f, WithAlpha(col, t), StatusTag(c->testStatus));
        char eb[16]; std::snprintf(eb, sizeof eb, "%d", c->testExpected);
        DrawText({ x0 + 560, ry }, 16.0f, WithAlpha(chrome::C_DIM, t), eb);
        char db[16]; std::snprintf(db, sizeof db, "%d", c->testDiff);
        DrawText({ x0 + 760, ry }, 16.0f, WithAlpha(c->testDiff > 0 ? RGBA(255, 110, 90, 255) : chrome::C_DIM, t), db);
    }
    ResetFont();

    if (MaxOff() > 0) {
        SetFont(g_fSeurat);
        char sc[48]; std::snprintf(sc, sizeof sc, "%d - %d of %d", g_off + 1, std::min(total, g_off + ROWS), total);
        DrawTextAligned({ 900, 556 }, { 1210, 580 }, 14.0f, WithAlpha(chrome::C_DIM, t), sc, Align::Right, true, true);
        ResetFont();
    }

    SetFont(g_fRodin);
    DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Up / Down  Move        Enter  Which tests        Esc  Back");
    ResetFont();
}

} // namespace

void QaTestsInit() { Init(); }
void QaTestsDraw(double openSeconds) { Draw(openSeconds); }
void QaTestsInput(const ScreenInput& in) { Input(in); }
void QaTestsReset() { Reset(); }
