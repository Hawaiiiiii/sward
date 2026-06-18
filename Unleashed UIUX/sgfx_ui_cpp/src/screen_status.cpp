// =============================================================================
// screen_status.cpp — the run metrics. The result of a preflight pass over a
// profile: an overall verdict + totals, then a row per check pack with its error /
// warning / info counts. Up/Down move between packs. Built from primitives + text
// only — no chrome art. Counts come from the live data bridge (sgfx_status.json),
// falling back to representative defaults when no status file is present.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "sgfx_data.h"
#include <cstdio>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0, g_logoTex = -1;

// ---- palette ----------------------------------------------------------------
const uint32_t C_BG_TOP   = RGBA(12, 20, 38, 255), C_BG_BOT = RGBA(5, 9, 18, 255);
const uint32_t C_PANEL    = RGBA(16, 24, 40, 235);
const uint32_t C_CAP      = RGBA(10, 16, 28, 255);
const uint32_t C_SEL_TOP  = RGBA(64, 150, 235, 225), C_SEL_BOT = RGBA(28, 92, 180, 225);
const uint32_t C_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t C_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t C_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t C_RULE     = RGBA(120, 170, 230, 90);
const uint32_t C_LABEL    = RGBA(150, 170, 196, 255);
const uint32_t C_ERR      = RGBA(235, 96, 84, 255);
const uint32_t C_WARN     = RGBA(235, 200, 90, 255);
const uint32_t C_INFO     = RGBA(140, 196, 150, 255);
const uint32_t C_DIM      = RGBA(90, 104, 120, 255);
const uint32_t C_OKV      = RGBA(120, 230, 140, 255);
const uint32_t C_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t C_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t C_CHIP     = RGBA(150, 196, 150, 255);

// ---- the check packs: from the live data bridge (sgfx_status.json) or defaults --
const std::vector<sgfx::Pack>& packs() { return sgfx::Get().run.packs; }
int packCount() { return (int)packs().size(); }

int g_sel = 0;

// ---- layout -----------------------------------------------------------------
constexpr float SUM_X = 150, SUM_Y = 134, SUM_W = 980, SUM_H = 96;
constexpr float TBL_X = 150, TBL_Y = 256, TBL_W = 980, TBL_H = 300;
constexpr float ROW_H = 52, HEAD_H = 44;
constexpr float COL_PACK = TBL_X + 28, COL_ERR = TBL_X + 600, COL_WARN = TBL_X + 760, COL_INFO = TBL_X + 928;

void Totals(int& e, int& w, int& i) {
    e = w = i = 0;
    for (const auto& p : packs()) { e += p.err; w += p.warn; i += p.info; }
}

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
    if (g_logoTex < 0)  g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
}
void Reset() { g_sel = 0; }
void Input(const ScreenInput& in) {
    if (in.up)   g_sel = std::max(0, g_sel - 1);
    if (in.down) g_sel = std::min(std::max(0, packCount() - 1), g_sel + 1);
}

void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 40 }, { 40 + w, 40 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, t));
}

// a count cell, right-aligned at rx; dimmed when zero.
void Count(float rx, float y, int n, uint32_t col, float t) {
    char b[12]; std::snprintf(b, sizeof(b), "%d", n);
    SetFont(g_fRodin);
    DrawTextAligned({ rx - 90, y }, { rx, y + 30 }, 24.0f,
                    WithAlpha(n > 0 ? col : C_DIM, t), b, Align::Right, true, true);
    ResetFont();
}

void Draw(double openSec) {
    const float a = (float)ComputeMotion(openSec, 0.0, 12.0);
    const float tb = (float)ComputeMotion(openSec, 8.0, 14.0);
    int te, tw, ti; Totals(te, tw, ti);
    const char* verdict = te > 0 ? "NEEDS REVIEW" : (tw > 0 ? "WARNINGS" : "LIKELY OK");
    uint32_t vcol = te > 0 ? C_ERR : (tw > 0 ? C_WARN : C_OKV);

    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);
    DrawLogoSlot(a);
    // profile chip top-right
    SetFont(g_fRodin);
    DrawTextAligned({ 910, 52 }, { 1130, 76 }, 16.0f, WithAlpha(C_CHIP, a), "PROFILE", Align::Right, true, true);
    DrawTextAligned({ 910, 74 }, { 1130, 104 }, 24.0f, WithAlpha(C_TITLE, a), sgfx::Get().run.activeProfile.c_str(), Align::Right, true, true);
    ResetFont();

    // ===== SUMMARY: verdict + totals =========================================
    DrawRect({ SUM_X, SUM_Y }, { SUM_X + SUM_W, SUM_Y + SUM_H }, WithAlpha(C_PANEL, a));
    DrawRect({ SUM_X, SUM_Y }, { SUM_X + 6, SUM_Y + SUM_H }, WithAlpha(vcol, a));   // verdict accent
    SetFont(g_fDF);
    DrawTextAligned({ SUM_X + 28, SUM_Y + 16 }, { SUM_X + 560, SUM_Y + SUM_H - 14 }, 38.0f,
                    WithAlpha(vcol, a), verdict, Align::Left, true, true);
    ResetFont();
    // totals (E/W/I) on the right of the summary
    auto total = [&](float cx, const char* lbl, int n, uint32_t col) {
        char b[12]; std::snprintf(b, sizeof(b), "%d", n);
        SetFont(g_fRodin);
        DrawTextAligned({ cx - 70, SUM_Y + 20 }, { cx + 70, SUM_Y + 44 }, 30.0f,
                        WithAlpha(n > 0 ? col : C_DIM, a), b, Align::Center, true, true);
        DrawTextAligned({ cx - 70, SUM_Y + 54 }, { cx + 70, SUM_Y + 76 }, 15.0f,
                        WithAlpha(C_LABEL, a), lbl, Align::Center, true, true);
        ResetFont();
    };
    total(SUM_X + 700, "ERRORS",   te, C_ERR);
    total(SUM_X + 820, "WARNINGS", tw, C_WARN);
    total(SUM_X + 930, "INFO",     ti, C_INFO);

    // ===== PACK TABLE ========================================================
    DrawRect({ TBL_X, TBL_Y }, { TBL_X + TBL_W, TBL_Y + TBL_H }, WithAlpha(C_PANEL, tb));
    DrawRect({ TBL_X, TBL_Y }, { TBL_X + TBL_W, TBL_Y + HEAD_H }, WithAlpha(C_CAP, tb));
    SetFont(g_fRodin);
    DrawText({ COL_PACK, TBL_Y + 12 }, 16.0f, WithAlpha(C_LABEL, tb), "CHECK PACK");
    DrawTextAligned({ COL_ERR - 90, TBL_Y + 12 }, { COL_ERR, TBL_Y + 36 }, 16.0f, WithAlpha(C_LABEL, tb), "ERRORS", Align::Right, true, true);
    DrawTextAligned({ COL_WARN - 90, TBL_Y + 12 }, { COL_WARN, TBL_Y + 36 }, 16.0f, WithAlpha(C_LABEL, tb), "WARN", Align::Right, true, true);
    DrawTextAligned({ COL_INFO - 90, TBL_Y + 12 }, { COL_INFO, TBL_Y + 36 }, 16.0f, WithAlpha(C_LABEL, tb), "INFO", Align::Right, true, true);
    ResetFont();

    const float rowsTop = TBL_Y + HEAD_H + 6;
    for (int k = 0; k < packCount(); ++k) {
        const sgfx::Pack& p = packs()[k];
        float y = rowsTop + k * ROW_H;
        bool sel = (k == g_sel);
        if (sel && tb > 0.5f)
            DrawVGradient({ TBL_X + 6, y + 2 }, { TBL_X + TBL_W - 6, y + ROW_H - 4 },
                          WithAlpha(C_SEL_TOP, tb), WithAlpha(C_SEL_BOT, tb));
        SetFont(g_fSeurat);
        DrawTextAligned({ COL_PACK, y }, { COL_ERR - 110, y + ROW_H }, 26.0f,
                        WithAlpha(sel ? C_TEXT_SEL : C_TEXT, tb), p.name.c_str(), Align::Left, true, true);
        ResetFont();
        float cy = y + (ROW_H - 30) * 0.5f;
        Count(COL_ERR,  cy, p.err,  C_ERR,  tb);
        Count(COL_WARN, cy, p.warn, C_WARN, tb);
        Count(COL_INFO, cy, p.info, C_INFO, tb);
    }

    // ===== FOOTER ============================================================
    SetFont(g_fRodin);
    DrawRect({ 150, 612 }, { 1130, 614 }, WithAlpha(C_RULE, a));
    DrawText({ 158, 628 }, 20.0f, WithAlpha(C_FOOTER, a), "Up/Down  Select pack");
    DrawText({ 470, 628 }, 20.0f, WithAlpha(C_FOOTER, a), "Esc  Back");
    ResetFont();
}

} // namespace

void StatusInit() { Init(); }
void StatusDraw(double openSeconds) { Draw(openSeconds); }
void StatusInput(const ScreenInput& in) { Input(in); }
void StatusReset() { Reset(); }
bool StatusOnStatRow() { return true; }   // the cursor is always on a pack row; cancel backs out
