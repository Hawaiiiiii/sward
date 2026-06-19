// =============================================================================
// screen_changelogs.cpp — per-car delivery state from CHANGELOG.md, green chrome.
// Reads changelog::Scan() and lists every car with a changelog: its latest version,
// date, and the number of change bullets that version carries, newest delivery first.
// Read-only; the changelog is the delivery record. Real data straight from
// bmw_git_root (the same root the 3D viewer renders from).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "green_chrome.h"
#include "changelog.h"

#include <cstdio>
#include <string>
#include <algorithm>

using namespace ui;

namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
changelog::Log g_log;
int g_off = 0;

constexpr int ROWS = 13;
int MaxOff() { return std::max(0, (int)g_log.entries.size() - ROWS); }

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() { g_log = changelog::Scan(); g_off = 0; }
void Input(const ScreenInput& in) {
    if (in.down && g_off < MaxOff()) ++g_off;
    if (in.up   && g_off > 0)        --g_off;
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "CHANGELOGS");
    chrome::Container(33, 130, 1246, 600, true, b.line, b.outer, b.inner, b.bg);

    if (!g_log.configured || !g_log.rootExists || g_log.entries.empty()) {
        SetFont(g_fSeurat);
        DrawTextAligned({ 90, 300 }, { 1190, 350 }, 22.0f, WithAlpha(chrome::C_DIM, t),
            !g_log.configured ? "Set \"bmw_git_root\" in viewer3d.json to your digital-3d-car-models checkout."
                              : "No CHANGELOG.md files were found under <root>/cars.",
            Align::Center, true, true);
        ResetFont();
        SetFont(g_fRodin); DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back"); ResetFont();
        return;
    }

    // summary
    SetFont(g_fSeurat);
    char sum[160];
    std::snprintf(sum, sizeof sum, "%zu cars with a changelog        newest delivery  %s",
                  g_log.entries.size(), g_log.entries.front().date.empty() ? "-" : g_log.entries.front().date.c_str());
    DrawTextAligned({ 70, 158 }, { 1240, 184 }, 17.0f, WithAlpha(chrome::C_DESC, t), sum, Align::Left, true, true);
    DrawRect({ 70, 200 }, { 1210, 201 }, WithAlpha(chrome::C_LINE, t));
    ResetFont();

    // headers
    SetFont(g_fRodin);
    const float x0 = 80, y0 = 222, rowH = 27;
    DrawText({ x0, y0 - 30 }, 13.0f, WithAlpha(RGBA(150, 190, 150, 255), t), "CAR");
    DrawText({ x0 + 360, y0 - 30 }, 13.0f, WithAlpha(RGBA(150, 190, 150, 255), t), "VERSION");
    DrawText({ x0 + 600, y0 - 30 }, 13.0f, WithAlpha(RGBA(150, 190, 150, 255), t), "DATE");
    DrawText({ x0 + 860, y0 - 30 }, 13.0f, WithAlpha(RGBA(150, 190, 150, 255), t), "CHANGES");

    const int total = (int)g_log.entries.size();
    for (int row = 0; row < ROWS && (g_off + row) < total; ++row) {
        const changelog::Entry& e = g_log.entries[g_off + row];
        const float ry = y0 + rowH * (float)row;
        char label[80]; std::snprintf(label, sizeof label, "%s/%s", e.brand.c_str(), e.id.c_str());
        DrawText({ x0, ry }, 16.0f, WithAlpha(chrome::C_DESC, t), label);
        DrawText({ x0 + 360, ry }, 16.0f, WithAlpha(e.version.empty() ? chrome::C_DIM : chrome::C_TITLE, t),
                 e.version.empty() ? "-" : e.version.c_str());
        DrawText({ x0 + 600, ry }, 16.0f, WithAlpha(chrome::C_DESC, t), e.date.empty() ? "-" : e.date.c_str());
        char cb[16]; std::snprintf(cb, sizeof cb, "%d", e.changes);
        DrawText({ x0 + 870, ry }, 16.0f, WithAlpha(chrome::C_DIM, t), cb);
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

void ChangelogsInit() { Init(); }
void ChangelogsDraw(double openSeconds) { Draw(openSeconds); }
void ChangelogsInput(const ScreenInput& in) { Input(in); }
void ChangelogsReset() { Reset(); }
