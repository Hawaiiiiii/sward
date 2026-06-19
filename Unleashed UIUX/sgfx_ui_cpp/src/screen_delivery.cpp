// =============================================================================
// screen_delivery.cpp — the delivery workbook, green chrome. One row per car in the
// official-workbook shape used for 3D-car deliveries: version + date (from CHANGELOG.md),
// Ramses size + Logic size (the exported.ramses / exported.rlogic file sizes), and the
// screenshot-test state. Joins fleet::Scan() (sizes + test state) with changelog::Scan()
// (version + date), newest delivery first. Read-only; real data from bmw_git_root.
// (SVN revision + free-text comment are external and stay out of the read-only view.)
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "green_chrome.h"
#include "fleet.h"
#include "changelog.h"

#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

using namespace ui;

namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
struct Row { std::string brand, id, version, date; long long ramses = 0, logic = 0; std::string test; };
std::vector<Row> g_rows;
int g_off = 0;
bool g_configured = false;

constexpr int ROWS = 13;
int MaxOff() { return std::max(0, (int)g_rows.size() - ROWS); }

std::string MB(long long b) {
    if (b <= 0) return "-";
    char s[24]; std::snprintf(s, sizeof s, "%.1f MB", (double)b / 1048576.0); return s;
}
uint32_t TestColor(const std::string& s) {
    if (s == "pass")   return chrome::C_OK;
    if (s == "diff")   return RGBA(255, 130, 110, 255);
    if (s == "notrun") return chrome::C_WARN;
    return chrome::C_DIM;
}
const char* TestTag(const std::string& s) {
    if (s == "pass")   return "match";
    if (s == "diff")   return "differs";
    if (s == "notrun") return "not run";
    return "-";
}

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() {
    fleet::Roster fr = fleet::Scan();
    changelog::Log cl = changelog::Scan();
    g_configured = fr.configured && fr.rootExists;
    g_rows.clear();
    for (const auto& c : fr.cars) {
        Row r; r.brand = c.brand; r.id = c.id; r.ramses = c.ramsesBytes; r.logic = c.logicBytes; r.test = c.testStatus;
        for (const auto& e : cl.entries) if (e.id == c.id) { r.version = e.version; r.date = e.date; break; }
        g_rows.push_back(std::move(r));
    }
    std::sort(g_rows.begin(), g_rows.end(), [](const Row& a, const Row& b) {
        if (a.date != b.date) return a.date > b.date;      // newest delivery first
        return a.id < b.id;
    });
    g_off = 0;
}
void Input(const ScreenInput& in) {
    if (in.down && g_off < MaxOff()) ++g_off;
    if (in.up   && g_off > 0)        --g_off;
}

void Draw(double openSec) {
    const chrome::Build b = chrome::Stage(openSec);
    const float t = b.t;
    chrome::Backdrop(g_fDF, b.title, "WORKBOOK");
    chrome::Container(33, 130, 1246, 600, true, b.line, b.outer, b.inner, b.bg);

    if (!g_configured || g_rows.empty()) {
        SetFont(g_fSeurat);
        DrawTextAligned({ 90, 300 }, { 1190, 350 }, 22.0f, WithAlpha(chrome::C_DIM, t),
            g_configured ? "No cars found under <root>/cars."
                         : "Set \"bmw_git_root\" in viewer3d.json to your digital-3d-car-models checkout.",
            Align::Center, true, true);
        ResetFont();
        SetFont(g_fRodin); DrawText({ 150, 662 }, 20.0f, WithAlpha(chrome::C_FOOTER, t), "Esc  Back"); ResetFont();
        return;
    }

    const float x0 = 70, y0 = 222, rowH = 27;
    const float cVer = x0 + 250, cDate = x0 + 410, cRam = x0 + 630, cLog = x0 + 800, cShot = x0 + 970;

    SetFont(g_fRodin);
    const uint32_t hc = RGBA(150, 190, 150, 255);
    DrawText({ x0, y0 - 30 }, 13.0f, WithAlpha(hc, t), "CAR");
    DrawText({ cVer, y0 - 30 }, 13.0f, WithAlpha(hc, t), "VERSION");
    DrawText({ cDate, y0 - 30 }, 13.0f, WithAlpha(hc, t), "DATE");
    DrawText({ cRam, y0 - 30 }, 13.0f, WithAlpha(hc, t), "RAMSES");
    DrawText({ cLog, y0 - 30 }, 13.0f, WithAlpha(hc, t), "LOGIC");
    DrawText({ cShot, y0 - 30 }, 13.0f, WithAlpha(hc, t), "SHOTS");
    DrawRect({ x0, y0 - 8 }, { 1200, y0 - 7 }, WithAlpha(chrome::C_LINE, t));

    const int total = (int)g_rows.size();
    for (int row = 0; row < ROWS && (g_off + row) < total; ++row) {
        const Row& r = g_rows[g_off + row];
        const float ry = y0 + rowH * (float)row;
        char label[80]; std::snprintf(label, sizeof label, "%s/%s", r.brand.c_str(), r.id.c_str());
        DrawText({ x0, ry }, 15.0f, WithAlpha(chrome::C_DESC, t), label);
        DrawText({ cVer, ry }, 15.0f, WithAlpha(r.version.empty() ? chrome::C_DIM : chrome::C_TITLE, t), r.version.empty() ? "-" : r.version.c_str());
        DrawText({ cDate, ry }, 15.0f, WithAlpha(chrome::C_DESC, t), r.date.empty() ? "-" : r.date.c_str());
        DrawText({ cRam, ry }, 15.0f, WithAlpha(chrome::C_DIM, t), MB(r.ramses).c_str());
        DrawText({ cLog, ry }, 15.0f, WithAlpha(chrome::C_DIM, t), MB(r.logic).c_str());
        DrawText({ cShot, ry }, 15.0f, WithAlpha(TestColor(r.test), t), TestTag(r.test));
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

void DeliveryInit() { Init(); }
void DeliveryDraw(double openSeconds) { Draw(openSeconds); }
void DeliveryInput(const ScreenInput& in) { Input(in); }
void DeliveryReset() { Reset(); }
