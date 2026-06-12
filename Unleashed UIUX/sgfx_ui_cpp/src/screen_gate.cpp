// =============================================================================
// screen_gate.cpp — the Gaia-gate STAGE SELECT, re-authored 1:1 from LIVE
// capture (s01 179-214s, s09 157-164s; stills gate_stage_select /
// gate_act2_details / gate_confirm_popup):
//   * a full-width GOLD band (cream->gold->amber) with the big two-line
//     italic chrome "STAGE / SELECT" wordmark overlapping it;
//   * the stage-name plate (dark grey, white border, TL chamfer + slanted
//     right end, white italic name);
//   * ONE translucent grey act panel: "Act N" header, stat rows (HIGH SCORE /
//     BEST TIME label plates + chrome values), medal rows (sun 3/3, moon 7/7
//     with icon slots), the stage screenshot slot and the big metallic rank
//     letter (real mat_result art);
//   * accept opens the measured "Play Stage / Cancel" confirm popup (green
//     scanline panel, gradient highlight row); Play Stage -> the stage via
//     the loader; B backs out to the hub. LB/RB switches acts (live scene
//     behind stays bright).
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;
int g_glyphTex = -1, g_rankTex = -1;

struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A  = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B  = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };
const UV GLYPH_LB = { 0.32617f, 0.00781f, 0.46094f, 0.07812f };
const UV GLYPH_RB = { 0.48242f, 0.00781f, 0.61523f, 0.07812f };
const UV RANK_UV[6] = {   // mat_result_comon_002, same table as result/result_ex
    { 0.0293f, 0.0234f, 0.2656f, 0.2695f }, { 0.3203f, 0.0254f, 0.5625f, 0.2676f },
    { 0.0273f, 0.3262f, 0.2617f, 0.5566f }, { 0.3125f, 0.3203f, 0.5703f, 0.5625f },
    { 0.0293f, 0.6113f, 0.2656f, 0.8438f }, { 0.3281f, 0.6191f, 0.5547f, 0.8379f },
};

// ---- palette (sampled) --------------------------------------------------------
const uint32_t C_BAND_T = RGBA(246, 232, 160, 255);   // gold band cream top
const uint32_t C_BAND_M = RGBA(228, 178, 52, 255);
const uint32_t C_BAND_B = RGBA(188, 124, 18, 255);
const uint32_t C_CHR_T  = RGBA(246, 248, 252, 255);   // chrome wordmark
const uint32_t C_CHR_B  = RGBA(164, 172, 186, 255);
const uint32_t C_CHR_OUT= RGBA(20, 24, 32, 255);
const uint32_t C_NAME_T = RGBA(96, 100, 108, 235);    // stage-name plate
const uint32_t C_NAME_B = RGBA(52, 56, 62, 235);
const uint32_t C_PANEL_T= RGBA(112, 116, 122, 205);   // act panel translucent grey
const uint32_t C_PANEL_B= RGBA(64, 67, 72, 205);
const uint32_t C_LBLPLATE = RGBA(41, 46, 51, 230);    // stat label plate
const uint32_t C_VALUE  = RGBA(214, 216, 220, 255);   // chrome value text
const uint32_t C_WHITE  = RGBA(255, 255, 255, 255);
const uint32_t C_BORDER = RGBA(214, 218, 222, 255);
const uint32_t C_SUN    = RGBA(232, 120, 40, 255);
const uint32_t C_MOON   = RGBA(80, 150, 235, 255);
// confirm popup (the world-map popup family)
const uint32_t C_POP_FILL = RGBA(20, 83, 18, 255);
const uint32_t C_POP_HI_T = RGBA(118, 148, 36, 255), C_POP_HI_B = RGBA(88, 205, 45, 255);
// scene placeholder (Gaia temple interior: white stone + warm warp glow)
const uint32_t C_STONE_T = RGBA(214, 212, 204, 255), C_STONE_B = RGBA(150, 148, 140, 255);
const uint32_t C_WARP    = RGBA(255, 150, 40, 255);

struct Act { const char* name; const char* hiScore; const char* bestTime; int sun, sunMax, moon, moonMax; int rank; };
const Act ACTS[] = {
    { "Act 1", "128450", "02:31:08", 3, 3, 5, 7, 1 },
    { "Act 2", "159998", "01:59:79", 3, 3, 7, 7, 0 },
    { "Act 3", "94210",  "03:12:44", 2, 3, 4, 7, 2 },
};
constexpr int ACT_COUNT = 3;

int  g_act = 1;            // the captured frame shows Act 2
bool g_popup = false;
int  g_popupSel = 0;
double g_startStart = -100.0;
const char* g_nav = nullptr;

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_rankTex  < 0) g_rankTex  = gfx::loadTexture("assets/result/mat_result_comon_002.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");
}
void Reset() { g_act = 1; g_popup = false; g_popupSel = 0; g_nav = nullptr; }
void SwitchAct(int d) { g_act = (g_act + ACT_COUNT + d) % ACT_COUNT; }
void Input(const ScreenInput& in) {
    if (g_popup) {
        if (in.up || in.down) g_popupSel ^= 1;
        if (in.accept) {
            if (g_popupSel == 0) { g_startStart = Now(); g_nav = "loading>sonic_hud"; }   // Play Stage
            g_popup = false; g_popupSel = 0;
        }
        if (in.cancel) { g_popup = false; g_popupSel = 0; }
        return;
    }
    if (in.left  || in.tabLeft)  SwitchAct(-1);
    if (in.right || in.tabRight) SwitchAct(+1);
    if (in.accept) { g_popup = true; g_popupSel = 0; }    // -> Play Stage / Cancel
    if (in.cancel) g_nav = "@back";
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

void Chrome(V2 pos, float px, const char* s, float a) {
    SetFont(g_fDF);
    SetTextShear(0.26f);
    SetTextStretchX(1.35f);
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ pos.x + dx * 2.2f, pos.y + dy * 2.2f }, px, WithAlpha(C_CHR_OUT, a), s);
    DrawTextGradient(pos, px, WithAlpha(C_CHR_T, a), WithAlpha(C_CHR_B, a), s);
    ResetTextStretchX();
    ResetTextShear();
    ResetFont();
}

// grey chamfered plate: TL 45-deg chamfer, slanted (italic) right end
void NamePlate(float x0, float y0, float x1, float y1, float a) {
    const float ch = 14, sl = (y1 - y0) * 0.45f;
    const V2 c[5] = { { x0 + ch, y0 }, { x1, y0 }, { x1 - sl, y1 }, { x0, y1 }, { x0, y0 + ch } };
    // pentagon as two quads
    const V2 q1[4] = { c[0], c[1], c[2], c[3] };
    const uint32_t f1[4] = { WithAlpha(C_NAME_T, a), WithAlpha(C_NAME_T, a), WithAlpha(C_NAME_B, a), WithAlpha(C_NAME_B, a) };
    DrawQuadGradient(q1, f1);
    const V2 q2[4] = { { x0, y0 + ch }, { x0 + ch, y0 }, { x0 + ch, y1 }, { x0, y1 } };
    DrawQuadGradient(q2, f1);
    DrawRect({ x0 + ch, y0 }, { x1, y0 + 2 }, WithAlpha(C_BORDER, a));
    DrawRect({ x0, y1 - 2 }, { x1 - sl + 2, y1 }, WithAlpha(C_BORDER, a));
}

void Draw(double openSec) {
    const float a = (float)ComputeMotion(openSec, 0.0, 10.0);
    const float panT = (float)ComputeMotion(openSec, 4.0, 10.0);

    // ---- live scene slot: temple stone + warp glow column ----
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_STONE_T, C_STONE_B);
    DrawVGradient({ 760, 60 }, { 980, 560 }, WithAlpha(C_WARP, 0.55f), WithAlpha(RGBA(255, 220, 120, 255), 0.25f));
    DrawRect({ 700, 540 }, { 1040, 600 }, RGBA(170, 150, 120, 255));

    // ---- gold band + two-line chrome wordmark ----
    DrawVGradient({ 0, 61 }, { REF_W, 69 }, WithAlpha(C_BAND_T, a), WithAlpha(C_BAND_M, a));
    DrawVGradient({ 0, 69 }, { REF_W, 77 }, WithAlpha(C_BAND_M, a), WithAlpha(C_BAND_B, a));
    Chrome({ 253, 50 }, 42.0f, "STAGE", a);
    Chrome({ 274, 88 }, 42.0f, "SELECT", a);

    // ---- stage-name plate ----
    if (panT > 0.0f) {
        NamePlate(273, 183, 557, 220, panT);
        SetFont(g_fSeurat);
        SetTextShear(0.18f);
        DrawText({ 300 + 1.5f, 189 + 1.5f }, 24.0f, WithAlpha(RGBA(14, 14, 16, 255), panT), "Windmill Isle");
        DrawText({ 300, 189 }, 24.0f, WithAlpha(C_WHITE, panT), "Windmill Isle");
        ResetTextShear();
        ResetFont();

        // ---- act panel ----
        const Act& act = ACTS[g_act];
        DrawVGradient({ 253, 233 }, { 600, 470 }, WithAlpha(C_PANEL_T, panT), WithAlpha(C_PANEL_B, panT));
        DrawRect({ 253, 233 }, { 600, 235 }, WithAlpha(C_BORDER, panT));
        // header: Act N
        SetFont(g_fSeurat);
        DrawText({ 280 + 1.5f, 246 + 1.5f }, 28.0f, WithAlpha(RGBA(14, 14, 16, 255), panT), act.name);
        DrawText({ 280, 246 }, 28.0f, WithAlpha(C_WHITE, panT), act.name);
        DrawRect({ 266, 284 }, { 588, 285.5f }, WithAlpha(C_BORDER, panT * 0.6f));
        ResetFont();
        // stat rows: label plate + chrome value
        auto stat = [&](float y, const char* lbl, const char* val) {
            DrawRect({ 266, y }, { 396, y + 24 }, WithAlpha(C_LBLPLATE, panT));
            SetFont(g_fRodin);
            DrawText({ 276, y + 3 }, 16.0f, WithAlpha(RGBA(225, 228, 232, 255), panT), lbl);
            ResetFont();
            Chrome({ 410, y - 4 }, 26.0f, val, panT);
        };
        stat(296, "HIGH SCORE", act.hiScore);
        stat(330, "BEST TIME", act.bestTime);
        // medal rows (icon slots + counts)
        DrawRect({ 266, 364 }, { 396, 388 }, WithAlpha(C_LBLPLATE, panT));
        SetFont(g_fRodin);
        DrawText({ 276, 367 }, 16.0f, WithAlpha(RGBA(225, 228, 232, 255), panT), "MEDALS");
        char buf[16];
        DrawRect({ 412, 364 }, { 432, 384 }, WithAlpha(C_SUN, panT));
        snprintf(buf, sizeof buf, "%d / %d", act.sun, act.sunMax);
        DrawText({ 444, 366 }, 18.0f, WithAlpha(C_VALUE, panT), buf);
        DrawRect({ 412, 396 }, { 432, 416 }, WithAlpha(C_MOON, panT));
        snprintf(buf, sizeof buf, "%d / %d", act.moon, act.moonMax);
        DrawText({ 444, 398 }, 18.0f, WithAlpha(C_VALUE, panT), buf);
        ResetFont();
        // screenshot slot + the big metallic rank letter
        DrawVGradient({ 444, 244 }, { 588, 326 }, WithAlpha(RGBA(60, 76, 98, 255), panT), WithAlpha(RGBA(30, 40, 54, 255), panT));
        SetFont(g_fSeurat);
        DrawTextAligned({ 444, 244 }, { 588, 326 }, 12.0f, WithAlpha(RGBA(140, 156, 176, 255), panT),
                        "STAGE PHOTO", Align::Center, true, false);
        ResetFont();
        if (g_rankTex >= 0)
            DrawImage(g_rankTex, { 466, 318 }, { 580, 452 },
                      { RANK_UV[act.rank].u0, RANK_UV[act.rank].v0 }, { RANK_UV[act.rank].u1, RANK_UV[act.rank].v1 },
                      WithAlpha(RGBA(225, 228, 235, 255), panT * 0.92f));
    }

    // ---- footer: [LB] Switch [RB]   (A) Select   (B) Back ----
    {
        SetFont(g_fRodin);
        float hcy = 660;
        auto glyph = [&](const UV& g, float x, float gh){ if (g_glyphTex<0) return x; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gw=gh*asp; DrawImage(g_glyphTex,{x,hcy-gh*0.5f},{x+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, WithAlpha(C_WHITE,a)); return x+gw+8; };
        float hx = 300;
        hx = glyph(GLYPH_LB, hx, 28);
        DrawText({ hx, hcy - 11 }, 20.0f, WithAlpha(C_WHITE, a), "Switch"); hx += 90;
        hx = glyph(GLYPH_RB, hx, 28); hx += 60;
        hx = glyph(GLYPH_A, hx, 30);
        DrawText({ hx, hcy - 11 }, 20.0f, WithAlpha(C_WHITE, a), "Select"); hx += 110;
        hx = glyph(GLYPH_B, hx, 30);
        DrawText({ hx, hcy - 11 }, 20.0f, WithAlpha(C_WHITE, a), "Back");
        ResetFont();
    }

    // ---- "Play Stage / Cancel" confirm popup (the GREY dialog family, like
    //      the pause confirm — measured from gate_confirm_popup.png) ----
    if (g_popup) {
        DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 110));
        const float px0 = 648, py0 = 186, px1 = 902, py1 = 300;
        DrawVGradient({ px0, py0 }, { px1, py1 },
                      RGBA(158, 160, 160, 245), RGBA(105, 107, 107, 245));
        DrawRect({ px0, py0 }, { px1, py0 + 2 }, C_BORDER);
        DrawRect({ px0, py1 - 2 }, { px1, py1 }, C_BORDER);
        const char* OPT[2] = { "Play Stage", "Cancel" };
        const float rowY[2] = { 206, 252 };
        // light highlight band on the selected row
        DrawVGradient({ px0 + 14, rowY[g_popupSel] - 6 }, { px1 - 14, rowY[g_popupSel] + 32 },
                      RGBA(228, 226, 214, 250), RGBA(196, 190, 168, 250));
        SetFont(g_fRodin);
        for (int i = 0; i < 2; ++i) {
            float w = MeasureText(24.0f, OPT[i]).x;
            bool sel = (i == g_popupSel);
            DrawText({ 775 - w * 0.5f + 1, rowY[i] + 1 }, 24.0f,
                     sel ? RGBA(120, 110, 70, 255) : RGBA(20, 20, 20, 200), OPT[i]);
            DrawText({ 775 - w * 0.5f, rowY[i] }, 24.0f,
                     sel ? RGBA(34, 30, 16, 255) : RGBA(235, 235, 235, 255), OPT[i]);
        }
        ResetFont();
    }
}

} // namespace

void GateInit() { Init(); }
void GateDraw(double openSeconds) { Draw(openSeconds); }
void GateInput(const ScreenInput& in) { Input(in); }
void GateReset() { Reset(); }
const char* GateNav() { return Nav(); }
