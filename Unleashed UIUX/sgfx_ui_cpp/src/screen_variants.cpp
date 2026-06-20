// =============================================================================
// screen_variants.cpp — model / powertrain / trim coverage, green chrome. Reads
// variants::Scan() (cars/model_variants.json) and draws a model x powertrain matrix:
// each cell is the trim-variant count for that model + powertrain (a dot when that
// powertrain doesn't exist for the model). Read-only; real data from bmw_git_root.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "green_chrome.h"
#include "variants.h"

#include <cstdio>
#include <string>
#include <algorithm>

using namespace ui;

namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
variants::Catalog g_cat;
int g_off = 0;

constexpr int ROWS = 13;
int MaxOff() { return std::max(0, (int)g_cat.models.size() - ROWS); }

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() { g_cat = variants::Scan(); g_off = 0; }
void Input(const ScreenInput& in) {
    if (in.down && g_off < MaxOff()) ++g_off;
    if (in.up   && g_off > 0)        --g_off;
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "VARIANTS");
    chrome::Container(33, 130, 1246, 600, true, b.line, b.outer, b.inner, b.bg);

    if (!g_cat.configured || !g_cat.rootExists || g_cat.models.empty()) {
        SetFont(g_fSeurat);
        DrawTextAligned({ 90, 300 }, { 1190, 350 }, 22.0f, WithAlpha(chrome::C_DIM, t),
            !g_cat.configured ? "Set \"bmw_git_root\" in viewer3d.json to your digital-3d-car-models checkout."
                              : "cars/model_variants.json was not found.",
            Align::Center, true, true);
        ResetFont();
        SetFont(g_fRodin); DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back"); ResetFont();
        return;
    }

    SetFont(g_fSeurat);
    char sum[160];
    std::snprintf(sum, sizeof sum, "%zu models      %d trim variants      %zu powertrains",
                  g_cat.models.size(), g_cat.totalVariants, g_cat.powertrains.size());
    DrawTextAligned({ 70, 158 }, { 1240, 184 }, 17.0f, WithAlpha(chrome::C_DESC, t), sum, Align::Left, true, true);
    DrawRect({ 70, 200 }, { 1210, 201 }, WithAlpha(chrome::C_LINE, t));
    ResetFont();

    const float x0 = 80, y0 = 224, rowH = 28, labelW = 170;
    const int nPt = (int)g_cat.powertrains.size();
    const float colW = nPt > 0 ? (1120.0f - labelW) / (float)nPt : 0.0f;

    SetFont(g_fRodin);
    DrawText({ x0, y0 - 30 }, 13.0f, WithAlpha(RGBA(150, 190, 150, 255), t), "MODEL");
    for (int c = 0; c < nPt; ++c) {
        const float cx = x0 + labelW + colW * (float)c;
        DrawTextAligned({ cx, y0 - 32 }, { cx + colW, y0 - 12 }, 16.0f, WithAlpha(chrome::C_TITLE, t),
                        g_cat.powertrains[c].c_str(), Align::Center, true, true);
    }
    DrawRect({ x0, y0 - 8 }, { x0 + labelW + colW * (float)nPt, y0 - 7 }, WithAlpha(chrome::C_LINE, t));

    const int total = (int)g_cat.models.size();
    for (int row = 0; row < ROWS && (g_off + row) < total; ++row) {
        const variants::Model& m = g_cat.models[g_off + row];
        const float ry = y0 + rowH * (float)row;
        DrawText({ x0, ry }, 16.0f, WithAlpha(chrome::C_DESC, t), m.name.c_str());
        for (int c = 0; c < nPt; ++c) {
            const float cx = x0 + labelW + colW * (float)c;
            const auto it = m.powertrains.find(g_cat.powertrains[c]);
            if (it == m.powertrains.end()) {
                DrawTextAligned({ cx, ry }, { cx + colW, ry + 18 }, 15.0f, WithAlpha(RGBA(70, 90, 70, 160), t), ".", Align::Center, true, true);
            } else {
                char n[8]; std::snprintf(n, sizeof n, "%d", it->second);
                DrawTextAligned({ cx, ry }, { cx + colW, ry + 18 }, 16.0f, WithAlpha(chrome::C_OK, t), n, Align::Center, true, true);
            }
        }
    }
    ResetFont();

    if (MaxOff() > 0) {
        SetFont(g_fSeurat);
        char sc[48]; std::snprintf(sc, sizeof sc, "%d - %d of %d", g_off + 1, std::min(total, g_off + ROWS), total);
        DrawTextAligned({ 900, 556 }, { 1210, 580 }, 14.0f, WithAlpha(chrome::C_DIM, t), sc, Align::Right, true, true);
        ResetFont();
    }

    SetFont(g_fRodin);
    DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t),
             MaxOff() > 0 ? "Up / Down  Scroll        Esc  Back" : "Esc  Back");
    ResetFont();
}

} // namespace

void VariantsInit() { Init(); }
void VariantsDraw(double openSeconds) { Draw(openSeconds); }
void VariantsInput(const ScreenInput& in) { Input(in); }
void VariantsReset() { Reset(); }
