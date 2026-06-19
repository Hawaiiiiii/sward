// =============================================================================
// screen_ambient.cpp — Ambient Layer screenshot-test coverage, in the green operator
// look. Reads al::Scan() (the AL assets repo's tests/{expected,actuals,diff} per
// brand/screen, per Confluence "How to screenshot test AL") and draws a brand x screen
// readiness matrix: PASS / DIFF (a visual change since baseline) / not run / no
// baseline. Read-only — it reports what a test run left on disk, it runs nothing.
// Path is operator-local (viewer3d.json "al_assets_root"); unset = a clear hint.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "green_chrome.h"
#include "al_coverage.h"

#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

using namespace ui;

namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
al::Coverage g_cov;
std::vector<std::string> g_brands, g_screens;

uint32_t StatusColor(const std::string& s) {
    if (s == "pass")   return chrome::C_OK;
    if (s == "diff")   return RGBA(255, 110, 90, 255);   // a regression -> review
    if (s == "notrun") return chrome::C_WARN;
    return chrome::C_DIM;                                  // no baseline
}
const char* StatusTag(const std::string& s) {
    if (s == "pass")   return "PASS";
    if (s == "diff")   return "DIFF";
    if (s == "notrun") return "- -";
    return "no base";
}
const al::Cell* Find(const std::string& brand, const std::string& screen) {
    for (const auto& c : g_cov.cells) if (c.brand == brand && c.screen == screen) return &c;
    return nullptr;
}

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() {
    g_cov = al::Scan();
    g_brands.clear(); g_screens.clear();
    for (const auto& c : g_cov.cells) {
        if (std::find(g_brands.begin(), g_brands.end(), c.brand) == g_brands.end()) g_brands.push_back(c.brand);
        if (std::find(g_screens.begin(), g_screens.end(), c.screen) == g_screens.end()) g_screens.push_back(c.screen);
    }
    std::sort(g_screens.begin(), g_screens.end());
}
void Input(const ScreenInput&) {}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "AMBIENT");
    chrome::Container(33, 130, 1246, 600, true, b.line, b.outer, b.inner, b.bg);

    if (!g_cov.configured || !g_cov.rootExists || g_cov.cells.empty()) {
        SetFont(g_fSeurat);
        const char* msg = !g_cov.configured
            ? "Set \"al_assets_root\" in viewer3d.json to your ambient-layer-assets checkout."
            : (!g_cov.rootExists ? "Ambient Layer assets not found at the configured path."
                                 : "No exported scenes with screenshot tests were found.");
        DrawTextAligned({ 90, 300 }, { 1190, 350 }, 22.0f, WithAlpha(chrome::C_DIM, t), msg, Align::Center, true, true);
        DrawTextAligned({ 90, 360 }, { 1190, 400 }, 15.0f, WithAlpha(chrome::C_DIM, t),
            "<root>/assets/<group>/<brand>/export_<screen>/tests/{expected,actuals,diff}", Align::Center, true, true);
        ResetFont();
        SetFont(g_fRodin); DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back"); ResetFont();
        return;
    }

    const float x0 = 70, y0 = 212, labelW = 160, rowH = 44;
    const float colW = g_screens.empty() ? 0.0f : (1130.0f - labelW) / (float)g_screens.size();

    // header row
    SetFont(g_fRodin);
    DrawText({ x0, y0 - 38 }, 14.0f, WithAlpha(RGBA(150, 190, 150, 255), t), "BRAND \\ SCREEN");
    for (size_t s = 0; s < g_screens.size(); ++s) {
        const float cx0 = x0 + labelW + colW * (float)s;
        DrawTextAligned({ cx0, y0 - 40 }, { cx0 + colW, y0 - 16 }, 17.0f, WithAlpha(chrome::C_TITLE, t),
                        g_screens[s].c_str(), Align::Center, true, true);
    }
    DrawRect({ x0, y0 - 8 }, { x0 + labelW + colW * (float)g_screens.size(), y0 - 7 }, WithAlpha(chrome::C_LINE, t));

    // brand rows x screen cells
    for (size_t r = 0; r < g_brands.size(); ++r) {
        const float ry = y0 + rowH * (float)r;
        DrawText({ x0, ry + 12 }, 17.0f, WithAlpha(chrome::C_DESC, t), g_brands[r].c_str());
        for (size_t s = 0; s < g_screens.size(); ++s) {
            const float cx0 = x0 + labelW + colW * (float)s;
            const float bx0 = cx0 + 6, by0 = ry + 5, bx1 = cx0 + colW - 8, by1 = ry + rowH - 7;
            const al::Cell* c = Find(g_brands[r], g_screens[s]);
            if (!c) {
                DrawTextAligned({ bx0, by0 }, { bx1, by1 }, 16.0f, WithAlpha(RGBA(70, 90, 70, 160), t), ".", Align::Center, true, true);
                continue;
            }
            const uint32_t col = StatusColor(c->status);
            chrome::Plate(bx0, by0, bx1, by1, t * 0.5f);
            DrawRect({ bx0, by1 - 3 }, { bx1, by1 }, WithAlpha(col, t));   // status bar
            DrawTextAligned({ bx0, by0 }, { bx1, by1 - 4 }, 14.0f, WithAlpha(col, t), StatusTag(c->status), Align::Center, true, true);
        }
    }
    ResetFont();

    // summary
    SetFont(g_fSeurat);
    char sum[220];
    std::snprintf(sum, sizeof sum, "%d pass     %d changed     %d not run     %d no baseline          %zu scenes / %zu brands",
                  g_cov.pass, g_cov.diff, g_cov.notrun, g_cov.nobaseline, g_cov.cells.size(), g_brands.size());
    DrawTextAligned({ x0, 552 }, { 1240, 584 }, 16.0f, WithAlpha(chrome::C_DESC, t), sum, Align::Left, true, true);
    ResetFont();

    SetFont(g_fRodin);
    DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back");
    ResetFont();
}

} // namespace

void AmbientInit() { Init(); }
void AmbientDraw(double openSeconds) { Draw(openSeconds); }
void AmbientInput(const ScreenInput& in) { Input(in); }
void AmbientReset() { Reset(); }
