// =============================================================================
// screen_result_ex.cpp — the screenshot-battery results. The per-filter breakdown
// of a screenshot test run over a profile: an overall battery verdict + totals,
// then a row per battery filter with its verdict and baseline diff count. Up/Down
// move between filters. Built from primitives + text only — no chrome art. Values
// are representative (real filter names and shape) until a live feed supplies the
// run. Same shape as screen_status.cpp, with battery filters in place of packs.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0, g_logoTex = -1;

const char* const ACTIVE_PROFILE = "G65";

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
const uint32_t C_RED      = RGBA(235, 96, 84, 255);    // baseline_missing
const uint32_t C_AMBER    = RGBA(235, 200, 90, 255);   // needs_review / proxy / candidate
const uint32_t C_GREEN    = RGBA(120, 230, 140, 255);  // likely_ok
const uint32_t C_DIM      = RGBA(90, 104, 120, 255);
const uint32_t C_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t C_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t C_CHIP     = RGBA(150, 196, 150, 255);

// ---- verdicts ---------------------------------------------------------------
// Colour bucket for the verdict: green = clean, amber = attention, red = blocked.
enum Verdict {
    V_LIKELY_OK,               // green
    V_NEEDS_MANUAL_REVIEW,     // amber
    V_PROXY_CANDIDATE_READY,   // amber
    V_BASELINE_CANDIDATE_READY,// amber
    V_BASELINE_MISSING,        // red
};
const char* VerdictLabel(Verdict v) {
    switch (v) {
        case V_LIKELY_OK:                return "LIKELY OK";
        case V_NEEDS_MANUAL_REVIEW:      return "NEEDS REVIEW";
        case V_PROXY_CANDIDATE_READY:    return "PROXY READY";
        case V_BASELINE_CANDIDATE_READY: return "BASELINE READY";
        case V_BASELINE_MISSING:         return "BASELINE MISSING";
    }
    return "";
}
uint32_t VerdictColour(Verdict v) {
    switch (v) {
        case V_LIKELY_OK:                return C_GREEN;
        case V_BASELINE_MISSING:         return C_RED;
        default:                         return C_AMBER;   // needs-review / proxy / candidate
    }
}

// ---- the battery filters (representative run) -------------------------------
// One row per screenshot-battery filter: its verdict and the pixel/region diff
// count against the stored baseline (0 when there is no baseline yet).
struct Filter { const char* name; Verdict verdict; int diff; };
const Filter FILTERS[] = {
    { "default",            V_LIKELY_OK,                 0 },
    { "lights_drl_front",   V_LIKELY_OK,                 0 },
    { "lights_LowBeam",     V_NEEDS_MANUAL_REVIEW,     142 },
    { "lights_HighBeam",    V_NEEDS_MANUAL_REVIEW,      88 },
    { "lights_OnlyCones",   V_PROXY_CANDIDATE_READY,     0 },
    { "openAllDoors_",      V_BASELINE_MISSING,          0 },
    { "automatic_Doors_",   V_BASELINE_CANDIDATE_READY,  0 },
    { "welcome_animation_", V_NEEDS_MANUAL_REVIEW,     310 },
    { "highlighting_Doors", V_BASELINE_MISSING,          0 },
};
constexpr int FILTER_COUNT = int(sizeof(FILTERS) / sizeof(FILTERS[0]));

int g_sel = 0;

// ---- layout -----------------------------------------------------------------
constexpr float SUM_X = 150, SUM_Y = 134, SUM_W = 980, SUM_H = 96;
constexpr float TBL_X = 150, TBL_Y = 240, TBL_W = 980, TBL_H = 364;
constexpr float ROW_H = 35, HEAD_H = 38;
constexpr float COL_FILTER = TBL_X + 28, COL_VERDICT = TBL_X + 560, COL_DIFF = TBL_X + 940;

// totals across the battery: reviewed (green), need review (amber), baseline-missing (red)
void Totals(int& ok, int& review, int& missing) {
    ok = review = missing = 0;
    for (int k = 0; k < FILTER_COUNT; ++k) {
        switch (FILTERS[k].verdict) {
            case V_LIKELY_OK:        ++ok;      break;
            case V_BASELINE_MISSING: ++missing; break;
            default:                 ++review;  break;
        }
    }
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
    if (in.down) g_sel = std::min(FILTER_COUNT - 1, g_sel + 1);
}

void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 40 }, { 40 + w, 40 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, t));
}

// a diff cell, right-aligned at rx; dimmed when zero (no baseline diff).
void DiffCount(float rx, float y, int n, float t) {
    char b[12]; std::snprintf(b, sizeof(b), "%d", n);
    SetFont(g_fRodin);
    DrawTextAligned({ rx - 90, y }, { rx, y + 30 }, 22.0f,
                    WithAlpha(n > 0 ? C_TEXT : C_DIM, t), b, Align::Right, true, true);
    ResetFont();
}

void Draw(double openSec) {
    const float a = (float)ComputeMotion(openSec, 0.0, 12.0);
    const float tb = (float)ComputeMotion(openSec, 8.0, 14.0);

    int tok, treview, tmissing; Totals(tok, treview, tmissing);
    // overall battery verdict: any missing baseline blocks; else any review pending; else clean.
    const char* verdict = tmissing > 0 ? "BASELINE MISSING"
                        : (treview > 0 ? "NEEDS REVIEW" : "LIKELY OK");
    uint32_t vcol = tmissing > 0 ? C_RED : (treview > 0 ? C_AMBER : C_GREEN);

    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);
    DrawLogoSlot(a);
    // profile chip top-right
    SetFont(g_fRodin);
    DrawTextAligned({ 910, 52 }, { 1130, 76 }, 16.0f, WithAlpha(C_CHIP, a), "PROFILE", Align::Right, true, true);
    DrawTextAligned({ 910, 74 }, { 1130, 104 }, 24.0f, WithAlpha(C_TITLE, a), ACTIVE_PROFILE, Align::Right, true, true);
    ResetFont();

    // ===== SUMMARY: overall verdict + totals =================================
    DrawRect({ SUM_X, SUM_Y }, { SUM_X + SUM_W, SUM_Y + SUM_H }, WithAlpha(C_PANEL, a));
    DrawRect({ SUM_X, SUM_Y }, { SUM_X + 6, SUM_Y + SUM_H }, WithAlpha(vcol, a));   // verdict accent
    SetFont(g_fDF);
    DrawTextAligned({ SUM_X + 28, SUM_Y + 16 }, { SUM_X + 560, SUM_Y + SUM_H - 14 }, 34.0f,
                    WithAlpha(vcol, a), verdict, Align::Left, true, true);
    ResetFont();
    // totals (reviewed / need review / baseline-missing) on the right of the summary
    auto total = [&](float cx, const char* lbl, int n, uint32_t col) {
        char b[12]; std::snprintf(b, sizeof(b), "%d", n);
        SetFont(g_fRodin);
        DrawTextAligned({ cx - 70, SUM_Y + 20 }, { cx + 70, SUM_Y + 44 }, 30.0f,
                        WithAlpha(n > 0 ? col : C_DIM, a), b, Align::Center, true, true);
        DrawTextAligned({ cx - 70, SUM_Y + 54 }, { cx + 70, SUM_Y + 76 }, 15.0f,
                        WithAlpha(C_LABEL, a), lbl, Align::Center, true, true);
        ResetFont();
    };
    total(SUM_X + 690, "REVIEWED",   tok,      C_GREEN);
    total(SUM_X + 810, "NEED REVIEW", treview, C_AMBER);
    total(SUM_X + 940, "NO BASELINE", tmissing,C_RED);

    // ===== FILTER TABLE ======================================================
    DrawRect({ TBL_X, TBL_Y }, { TBL_X + TBL_W, TBL_Y + TBL_H }, WithAlpha(C_PANEL, tb));
    DrawRect({ TBL_X, TBL_Y }, { TBL_X + TBL_W, TBL_Y + HEAD_H }, WithAlpha(C_CAP, tb));
    SetFont(g_fRodin);
    DrawText({ COL_FILTER, TBL_Y + 12 }, 16.0f, WithAlpha(C_LABEL, tb), "BATTERY FILTER");
    DrawText({ COL_VERDICT, TBL_Y + 12 }, 16.0f, WithAlpha(C_LABEL, tb), "VERDICT");
    DrawTextAligned({ COL_DIFF - 90, TBL_Y + 12 }, { COL_DIFF, TBL_Y + 36 }, 16.0f, WithAlpha(C_LABEL, tb), "DIFF", Align::Right, true, true);
    ResetFont();

    const float rowsTop = TBL_Y + HEAD_H + 6;
    for (int k = 0; k < FILTER_COUNT; ++k) {
        float y = rowsTop + k * ROW_H;
        bool sel = (k == g_sel);
        if (sel && tb > 0.5f)
            DrawVGradient({ TBL_X + 6, y + 2 }, { TBL_X + TBL_W - 6, y + ROW_H - 4 },
                          WithAlpha(C_SEL_TOP, tb), WithAlpha(C_SEL_BOT, tb));
        SetFont(g_fSeurat);
        DrawTextAligned({ COL_FILTER, y }, { COL_VERDICT - 20, y + ROW_H }, 22.0f,
                        WithAlpha(sel ? C_TEXT_SEL : C_TEXT, tb), FILTERS[k].name, Align::Left, true, true);
        ResetFont();
        // colour-coded verdict label
        SetFont(g_fRodin);
        DrawTextAligned({ COL_VERDICT, y }, { COL_DIFF - 110, y + ROW_H }, 16.0f,
                        WithAlpha(VerdictColour(FILTERS[k].verdict), tb),
                        VerdictLabel(FILTERS[k].verdict), Align::Left, true, true);
        ResetFont();
        float cy = y + (ROW_H - 30) * 0.5f;
        DiffCount(COL_DIFF, cy, FILTERS[k].diff, tb);
    }

    // ===== FOOTER ============================================================
    SetFont(g_fRodin);
    DrawRect({ 150, 612 }, { 1130, 614 }, WithAlpha(C_RULE, a));
    DrawText({ 158, 628 }, 20.0f, WithAlpha(C_FOOTER, a), "Up/Down  Select");
    DrawText({ 410, 628 }, 20.0f, WithAlpha(C_FOOTER, a), "Esc  Back");
    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void ResultExInit() { Init(); }
void ResultExDraw(double openSeconds) { Draw(openSeconds); }
void ResultExInput(const ScreenInput& in) { Input(in); }
void ResultExReset() { Reset(); }
