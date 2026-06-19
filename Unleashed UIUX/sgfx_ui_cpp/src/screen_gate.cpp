// =============================================================================
// screen_gate.cpp — the profile selector. A carousel over the work profiles: the
// focused profile id sits large between arrows, a detail panel shows its label,
// family, focus and last verdict, and accept opens a confirm. Left/Right switch
// profiles. Built from primitives + text only — no chrome art — so it ships on its
// own. Profile identity/labels are real; the verdict is representative until a live
// feed supplies it.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "sgfx_data.h"
#include "viewer3d.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0, g_logoTex = -1;

// ---- palette ----------------------------------------------------------------
const uint32_t C_BG_TOP = RGBA(6, 10, 20, 255), C_BG_BOT = RGBA(2, 4, 9, 255);
const uint32_t C_PANEL  = RGBA(0, 0, 0, 200);
const uint32_t C_OUTER  = RGBA(0, 49, 0, 255), C_INNER = RGBA(0, 33, 0, 235), C_LINE = RGBA(0, 110, 30, 255);
const uint32_t C_RAIL   = RGBA(20, 81, 18, 255);
const uint32_t C_ID     = RGBA(255, 209, 74, 255);   // big profile id (gold)
const uint32_t C_LABEL  = RGBA(150, 196, 150, 255);
const uint32_t C_VALUE  = RGBA(158, 223, 66, 255);
const uint32_t C_DESC   = RGBA(206, 224, 230, 255);
const uint32_t C_ARROW  = RGBA(120, 200, 120, 235);
const uint32_t C_FOOTER = RGBA(190, 210, 196, 255);
const uint32_t C_WHITE  = RGBA(255, 255, 255, 255);
const uint32_t C_CHIP   = RGBA(120, 170, 230, 255);
const uint32_t C_POP_FILL = RGBA(16, 26, 40, 245);
const uint32_t C_POP_HI_T = RGBA(64, 150, 235, 235), C_POP_HI_B = RGBA(28, 92, 180, 235);

// ---- the real work profiles -------------------------------------------------
struct Profile { const char* id; const char* label; const char* family;
                 const char* f1; const char* f2; const char* verdict; };
const Profile PROFILES[] = {
    { "G70", "BMW G70 live slice",    "IDCevo",  "Cross-car references and unused", "Lua; shared CarPaint catalog.",  "not run"      },
    { "G65", "BMW G65 live slice",    "IDCevo",  "Pivot_Master versus exported",    "Module_constants drift.",        "not run"      },
    { "G45", "BMW G45 classic slice", "Classic", "Classic anchor families and",     "legacy project sanity.",         "not run"      },
    { "G50", "BMW G50 live slice",    "IDCevo",  "Anchor sanity, constants drift,", "shared carpaint signal.",        "not run"      },
    { "G78", "BMW G78 live slice",    "IDCevo",  "Anchor sanity, constants drift,", "shared carpaint signal.",        "not run"      },
    { "NA0", "BMW NA0 live slice",    "IDCevo",  "Anchor sanity, constants drift,", "shared carpaint signal.",        "not run"      },
    { "NA5", "BMW NA5 live slice",    "IDCevo",  "Anchor sanity, constants drift,", "shared carpaint signal.",        "not run"      },
    { "NA6", "BMW NA6 live slice",    "IDCevo",  "Anchor sanity, constants drift,", "shared carpaint signal.",        "not run"      },
    { "NA7", "BMW NA7 live slice",    "IDCevo",  "Anchor sanity, constants drift,", "shared carpaint signal.",        "not run"      },
    { "NA8", "BMW NA8 live slice",    "IDCevo",  "Anchor sanity, constants drift,", "shared carpaint signal.",        "not run"      },
    { "F70", "BMW F70 classic slice", "Classic", "Classic anchor families and",     "legacy RaCo version policy.",    "not run"      },
    { "F74", "BMW F74 classic slice", "Classic", "Classic anchor families and",     "legacy RaCo version policy.",    "not run"      },
    { "F78", "BMW F78 classic slice", "Classic", "Classic anchor families and",     "legacy RaCo version policy.",    "not run"      },
    { "G48", "BMW G48 classic slice", "Classic", "Classic anchor families and",     "legacy RaCo version policy.",    "not run"      },
    { "G68", "BMW G68 classic slice", "Classic", "Classic anchor families and",     "legacy RaCo version policy.",    "not run"      },
    { "U06", "BMW U06 classic slice", "Classic", "Classic anchor families and",     "legacy RaCo version policy.",    "not run"      },
    { "U10", "BMW U10 classic slice", "Classic", "Classic anchor families and",     "legacy RaCo version policy.",    "not run"      },
    { "U11", "BMW U11 classic slice", "Classic", "Classic anchor families and",     "legacy RaCo version policy.",    "not run"      },
    { "U12", "BMW U12 classic slice", "Classic", "Classic anchor families and",     "legacy RaCo version policy.",    "not run"      },
};
constexpr int PROFILE_COUNT = int(sizeof(PROFILES) / sizeof(PROFILES[0]));

// the profile's last verdict: from the live data bridge (by id), or the static fallback.
const char* ProfileVerdict(const Profile& p) {
    for (const auto& ps : sgfx::Get().profileStatus)
        if (ps.id == p.id) return ps.verdict.c_str();
    return p.verdict;
}

// ---- layout -----------------------------------------------------------------
constexpr float ID_CY = 168;                          // big id baseline band
constexpr float DP_X0 = 340, DP_Y0 = 268, DP_X1 = 940, DP_Y1 = 520;   // detail panel
constexpr float GRID = 9.0f;

int g_sel = 0;          // default focus: first profile (no car privileged)
bool g_popup = false; int g_popupSel = 0;
const char* g_nav = nullptr;
std::string g_previewMsg; double g_previewStart = -100.0;

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
    if (g_logoTex < 0)  g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
}
void Reset() { g_sel = 0; g_popup = false; g_popupSel = 0; g_nav = nullptr; }
void Switch(int d) { g_sel = (g_sel + PROFILE_COUNT + d) % PROFILE_COUNT; }
void Input(const ScreenInput& in) {
    if (g_popup) {
        if (in.down) g_popupSel = (g_popupSel + 1) % 3;
        if (in.up)   g_popupSel = (g_popupSel + 2) % 3;
        if (in.accept) {
            if (g_popupSel == 0) g_nav = "town";                       // Open -> the action hub
            else if (g_popupSel == 1) {                                // Preview in 3D -> the Ramses viewer
                g_previewMsg = viewer3d::Launch(PROFILES[g_sel].id);
                g_previewStart = Now();
            }
            g_popup = false; g_popupSel = 0;
        }
        if (in.cancel) { g_popup = false; g_popupSel = 0; }
        return;
    }
    if (in.left  || in.tabLeft)  Switch(-1);
    if (in.right || in.tabRight) Switch(+1);
    if (in.accept) { g_popup = true; g_popupSel = 0; }
    if (in.cancel) g_nav = "@back";
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

void DrawPanel(float x0, float y0, float x1, float y1, float t) {
    DrawRect({ x0, y0 }, { x1, y1 }, WithAlpha(C_PANEL, t));
    SetModifier(MOD_CHECKERBOARD);
    DrawRect({ x0, y0 }, { x1, y0 + GRID }, WithAlpha(C_OUTER, t));
    DrawRect({ x0, y1 - GRID }, { x1, y1 }, WithAlpha(C_OUTER, t));
    DrawRect({ x0, y0 + GRID }, { x0 + GRID, y1 - GRID }, WithAlpha(C_OUTER, t));
    DrawRect({ x1 - GRID, y0 + GRID }, { x1, y1 - GRID }, WithAlpha(C_OUTER, t));
    DrawRect({ x0 + GRID, y0 + GRID }, { x1 - GRID, y1 - GRID }, WithAlpha(C_INNER, t));
    ResetModifier();
    uint32_t lc = WithAlpha(C_LINE, t); const float g = GRID, L = 2.0f;
    DrawRect({ x0+g, y0+g }, { x1-g, y0+g+L }, lc);
    DrawRect({ x0+g, y1-g-L }, { x1-g, y1-g }, lc);
    DrawRect({ x0+g, y0+g }, { x0+g+L, y1-g }, lc);
    DrawRect({ x1-g-L, y0+g }, { x1-g, y1-g }, lc);
}

void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 40 }, { 40 + w, 40 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, t));
}

void DrawArrow(float cx, float cy, int dir, float t) {
    float s = 20.0f;
    const V2 a[4] = { { cx + dir*s, cy - s }, { cx + dir*s, cy + s }, { cx - dir*s, cy }, { cx + dir*s, cy - s } };
    const uint32_t c[4] = { WithAlpha(C_ARROW, t), WithAlpha(C_ARROW, t), WithAlpha(C_ARROW, t), WithAlpha(C_ARROW, t) };
    DrawQuadGradient(a, c);
}

void DrawConfirm() {
    if (!g_popup) return;
    DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 120));
    const float x0 = 500, y0 = 276, x1 = 780, y1 = 462, cx = (x0 + x1) * 0.5f;
    DrawRect({ x0, y0 }, { x1, y1 }, C_POP_FILL);
    DrawRect({ x0, y0 }, { x1, y0 + 2 }, RGBA(64, 150, 235, 200));
    char head[48]; std::snprintf(head, sizeof(head), "Open %s", PROFILES[g_sel].id);
    const char* OPT[3] = { head, "Preview in 3D", "Cancel" };
    const float rowY[3] = { y0 + 42, y0 + 90, y0 + 138 };
    DrawVGradient({ x0 + 14, rowY[g_popupSel] - 6 }, { x1 - 14, rowY[g_popupSel] + 34 }, C_POP_HI_T, C_POP_HI_B);
    SetFont(g_fRodin);
    for (int i = 0; i < 3; ++i) {
        float w = MeasureText(26.0f, OPT[i]).x;
        DrawText({ cx - w * 0.5f, rowY[i] }, 26.0f,
                 (i == g_popupSel) ? C_WHITE : RGBA(200, 214, 230, 255), OPT[i]);
    }
    ResetFont();
}

void Draw(double openSec) {
    const float a  = (float)ComputeMotion(openSec, 0.0, 12.0);
    const float dp = (float)ComputeMotion(openSec, 10.0, 14.0);
    const Profile& p = PROFILES[g_sel];

    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);
    uint32_t s = 0x13572468u;
    for (int i = 0; i < 110; ++i) {
        s=s*1664525u+1013904223u; float x=(float)((s>>9)%1280);
        s=s*1664525u+1013904223u; float y=(float)((s>>9)%720);
        s=s*1664525u+1013904223u; int b=50+(int)((s>>9)%150);
        DrawRect({x,y},{x+1,y+1}, WithAlpha(RGBA(b,b,b,255), a*0.7f));
    }
    DrawLogoSlot(a);

    // index (n / total), top-right
    SetFont(g_fRodin);
    char idx[16]; std::snprintf(idx, sizeof(idx), "%d / %d", g_sel + 1, PROFILE_COUNT);
    DrawTextAligned({ 980, 52 }, { 1130, 84 }, 20.0f, WithAlpha(C_LABEL, a), idx, Align::Right, true, true);
    ResetFont();

    // big focused profile id between arrows
    SetFont(g_fDF);
    SetTextStretchX(1.2f);
    float w = MeasureText(64.0f, p.id).x * 1.2f;
    DrawTextShadow({ 640 - w * 0.5f, ID_CY - 36 }, 64.0f, WithAlpha(C_ID, a), p.id);
    ResetTextStretchX();
    ResetFont();
    if (!g_popup) { DrawArrow(560 - w * 0.5f, ID_CY - 4, +1, a); DrawArrow(720 + w * 0.5f, ID_CY - 4, -1, a); }
    // family chip under the id
    SetFont(g_fRodin);
    DrawTextAligned({ 440, ID_CY + 30 }, { 840, ID_CY + 58 }, 20.0f, WithAlpha(C_CHIP, a), p.family, Align::Center, true, true);
    ResetFont();

    // detail panel
    if (dp > 0.0f) {
        DrawPanel(DP_X0, DP_Y0, DP_X1, DP_Y1, dp);
        SetFont(g_fSeurat);
        DrawTextAligned({ DP_X0 + 26, DP_Y0 + 22 }, { DP_X1 - 26, DP_Y0 + 56 }, 26.0f,
                        WithAlpha(C_WHITE, dp), p.label, Align::Left, true, true);
        ResetFont();
        DrawRect({ DP_X0 + 24, DP_Y0 + 62 }, { DP_X1 - 24, DP_Y0 + 64 }, WithAlpha(C_RAIL, dp));
        SetFont(g_fSeurat);
        DrawText({ DP_X0 + 28, DP_Y0 + 84 }, 20.0f, WithAlpha(C_DESC, dp), p.f1);
        DrawText({ DP_X0 + 28, DP_Y0 + 112 }, 20.0f, WithAlpha(C_DESC, dp), p.f2);
        ResetFont();
        SetFont(g_fRodin);
        DrawText({ DP_X0 + 28, DP_Y0 + 162 }, 16.0f, WithAlpha(C_LABEL, dp), "LAST VERDICT");
        DrawTextAligned({ DP_X0 + 28, DP_Y0 + 178 }, { DP_X1 - 26, DP_Y0 + 206 }, 24.0f,
                        WithAlpha(C_VALUE, dp), ProfileVerdict(p), Align::Left, true, false);
        ResetFont();
    }

    // footer
    SetFont(g_fRodin);
    DrawRect({ 150, 612 }, { 1130, 614 }, WithAlpha(C_RAIL, a));
    if (g_popup) {
        DrawTextAligned({ 150, 628 }, { 1130, 656 }, 20.0f, WithAlpha(C_FOOTER, a), "Enter  Confirm     Esc  Cancel", Align::Right, true, true);
    } else {
        DrawText({ 158, 628 }, 20.0f, WithAlpha(C_FOOTER, a), "< >  Switch profile");
        DrawText({ 470, 628 }, 20.0f, WithAlpha(C_FOOTER, a), "Enter  Open");
        DrawText({ 700, 628 }, 20.0f, WithAlpha(C_FOOTER, a), "Esc  Back");
    }
    ResetFont();

    // transient 3D-preview launch feedback
    double pa = Now() - g_previewStart;
    if (g_previewStart > 0.0 && pa < 2.4 && !g_previewMsg.empty()) {
        float ma = std::min(1.0f, (float)((2.4 - pa) / 0.5));
        SetFont(g_fRodin);
        DrawTextAligned({ 150, 566 }, { 1130, 596 }, 22.0f, WithAlpha(RGBA(146,255,49,255), ma),
                        g_previewMsg.c_str(), Align::Center, true, true);
        ResetFont();
    }

    DrawConfirm();
}

} // namespace

void GateInit() { Init(); }
void GateDraw(double openSeconds) { Draw(openSeconds); }
void GateInput(const ScreenInput& in) { Input(in); }
void GateReset() { Reset(); }
const char* GateNav() { return Nav(); }
