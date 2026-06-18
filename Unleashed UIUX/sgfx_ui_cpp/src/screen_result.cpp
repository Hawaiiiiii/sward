// =============================================================================
// screen_result.cpp — the QA verdict card. The outcome of a run: a colour-coded
// verdict emblem, a breakdown of the signals behind it, and a one-line
// recommendation. Built from primitives + text only — no chrome art. The verdict
// and counts are representative (real vocabulary) until a live feed supplies them.
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
const uint32_t C_BG_TOP = RGBA(8, 12, 24, 255), C_BG_BOT = RGBA(3, 5, 12, 255);
const uint32_t C_PANEL  = RGBA(16, 24, 40, 235);
const uint32_t C_CAP    = RGBA(10, 16, 28, 255);
const uint32_t C_TITLE  = RGBA(255, 209, 74, 255);
const uint32_t C_TEXT   = RGBA(214, 226, 240, 255);
const uint32_t C_LABEL  = RGBA(150, 170, 196, 255);
const uint32_t C_VALUE  = RGBA(230, 238, 248, 255);
const uint32_t C_RULE   = RGBA(120, 170, 230, 90);
const uint32_t C_FOOTER = RGBA(190, 205, 225, 220);
const uint32_t C_WHITE  = RGBA(255, 255, 255, 255);
const uint32_t C_CHIP   = RGBA(150, 196, 150, 255);
const uint32_t C_ERR    = RGBA(235, 96, 84, 255);
const uint32_t C_WARN   = RGBA(235, 200, 90, 255);
const uint32_t C_OKV    = RGBA(120, 230, 140, 255);

// ---- the verdict: from the live data bridge (sgfx_status.json) or defaults ---
const char* VerdictText() { return sgfx::Get().run.verdict.c_str(); }
uint32_t VerdictColor() {
    const std::string& v = sgfx::Get().run.verdict;
    if (v.find("OK")    != std::string::npos) return C_OKV;
    if (v.find("BLOCK") != std::string::npos) return C_ERR;
    return C_WARN;
}

// ---- the signal rows: from the live data bridge (the last row is the total) --
const std::vector<sgfx::Signal>& signals() { return sgfx::Get().run.signals; }
const char* Recommendation()   { return sgfx::Get().run.recommendation.c_str(); }
const char* ActiveProfile()    { return sgfx::Get().run.activeProfile.c_str(); }

// ---- layout -----------------------------------------------------------------
constexpr float EM_X0 = 150, EM_Y0 = 150, EM_X1 = 560, EM_Y1 = 470;     // verdict emblem
constexpr float SG_X0 = 600, SG_Y0 = 150, SG_X1 = 1130, SG_Y1 = 470;    // signal list
constexpr float RC_X0 = 150, RC_Y0 = 496, RC_X1 = 1130, RC_Y1 = 566;    // recommendation

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
    if (g_logoTex < 0)  g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
}

void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 40 }, { 40 + w, 40 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, t));
}

void Draw(double openSec) {
    const float a  = (float)ComputeMotion(openSec, 0.0, 12.0);
    const float sg = (float)ComputeMotion(openSec, 8.0, 14.0);
    uint32_t vcol = VerdictColor();

    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);
    DrawLogoSlot(a);
    SetFont(g_fRodin);
    DrawTextAligned({ 910, 52 }, { 1130, 76 }, 16.0f, WithAlpha(C_CHIP, a), "PROFILE", Align::Right, true, true);
    DrawTextAligned({ 910, 74 }, { 1130, 104 }, 24.0f, WithAlpha(C_TITLE, a), ActiveProfile(), Align::Right, true, true);
    ResetFont();

    // ===== VERDICT EMBLEM ====================================================
    DrawRect({ EM_X0, EM_Y0 }, { EM_X1, EM_Y1 }, WithAlpha(C_PANEL, a));
    DrawRect({ EM_X0, EM_Y0 }, { EM_X1, EM_Y0 + 8 }, WithAlpha(vcol, a));    // colour bar
    SetFont(g_fRodin);
    DrawTextAligned({ EM_X0, EM_Y0 + 40 }, { EM_X1, EM_Y0 + 66 }, 18.0f, WithAlpha(C_LABEL, a), "VERDICT", Align::Center, true, true);
    ResetFont();
    {
        float cx = (EM_X0 + EM_X1) * 0.5f, cy = (EM_Y0 + EM_Y1) * 0.5f + 6;
        float rw = 150, rh = 70;
        DrawRect({ cx - rw, cy - rh }, { cx + rw, cy - rh + 3 }, WithAlpha(vcol, a));
        DrawRect({ cx - rw, cy + rh - 3 }, { cx + rw, cy + rh }, WithAlpha(vcol, a));
        DrawRect({ cx - rw, cy - rh }, { cx - rw + 3, cy + rh }, WithAlpha(vcol, a));
        DrawRect({ cx + rw - 3, cy - rh }, { cx + rw, cy + rh }, WithAlpha(vcol, a));
        SetFont(g_fDF);
        SetTextStretchX(1.05f);
        float w = MeasureText(34.0f, VerdictText()).x * 1.05f;
        DrawTextShadow({ cx - w * 0.5f, cy - 22 }, 34.0f, WithAlpha(vcol, a), VerdictText());
        ResetTextStretchX();
        ResetFont();
    }

    // ===== SIGNAL BREAKDOWN ==================================================
    if (sg > 0.0f) {
        DrawRect({ SG_X0, SG_Y0 }, { SG_X1, SG_Y1 }, WithAlpha(C_PANEL, sg));
        DrawRect({ SG_X0, SG_Y0 }, { SG_X1, SG_Y0 + 44 }, WithAlpha(C_CAP, sg));
        SetFont(g_fDF);
        DrawTextAligned({ SG_X0 + 22, SG_Y0 }, { SG_X1 - 18, SG_Y0 + 44 }, 24.0f,
                        WithAlpha(C_TITLE, sg), "SIGNALS", Align::Left, true, true);
        ResetFont();
        const float top = SG_Y0 + 64, pitch = 48;
        const auto& rows = signals();
        const int count = (int)rows.size();
        for (int i = 0; i < count; ++i) {
            float y = top + i * pitch;
            bool isTotal = (i == count - 1);
            if (isTotal) DrawRect({ SG_X0 + 22, y - 8 }, { SG_X1 - 22, y - 6 }, WithAlpha(C_RULE, sg));
            SetFont(g_fSeurat);
            DrawText({ SG_X0 + 26, y }, isTotal ? 24.0f : 22.0f,
                     WithAlpha(isTotal ? C_TITLE : C_TEXT, sg), rows[i].label.c_str());
            ResetFont();
            SetFont(g_fRodin);
            DrawTextAligned({ SG_X1 - 140, y - 2 }, { SG_X1 - 26, y + 28 }, isTotal ? 26.0f : 24.0f,
                            WithAlpha(isTotal ? C_TITLE : C_VALUE, sg), rows[i].value.c_str(), Align::Right, true, false);
            ResetFont();
        }
    }

    // ===== RECOMMENDATION ====================================================
    DrawRect({ RC_X0, RC_Y0 }, { RC_X1, RC_Y1 }, WithAlpha(C_PANEL, a));
    DrawRect({ RC_X0, RC_Y0 }, { RC_X0 + 6, RC_Y1 }, WithAlpha(vcol, a));
    SetFont(g_fRodin);
    DrawText({ RC_X0 + 22, RC_Y0 + 12 }, 14.0f, WithAlpha(C_LABEL, a), "RECOMMENDATION");
    ResetFont();
    SetFont(g_fSeurat);
    DrawText({ RC_X0 + 22, RC_Y0 + 34 }, 20.0f, WithAlpha(C_TEXT, a), Recommendation());
    ResetFont();

    // ===== FOOTER ============================================================
    SetFont(g_fRodin);
    DrawRect({ 150, 612 }, { 1130, 614 }, WithAlpha(C_RULE, a));
    DrawText({ 158, 628 }, 20.0f, WithAlpha(C_FOOTER, a), "Enter  Continue");
    DrawText({ 470, 628 }, 20.0f, WithAlpha(C_FOOTER, a), "Esc  Back");
    ResetFont();
}

} // namespace

void ResultInit() { Init(); }
void ResultDraw(double openSeconds) { Draw(openSeconds); }
