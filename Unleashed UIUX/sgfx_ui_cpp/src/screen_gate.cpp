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
    if (in.accept) { g_popup = true; g_popupSel = 1; }    // -> Play Stage / Cancel (defaults to Cancel, per capture)
    if (in.cancel) g_nav = "@back";
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

// Chrome wordmark: dark-outlined chrome-gradient face. `italic` controls the
// shear (wordmarks + stat VALUES lean; stat LABELS sit upright). `outline` is the
// per-pass dark-halo offset (smaller for the compact stat text).
void Chrome(V2 pos, float px, const char* s, float a, bool italic = true, float outline = 2.2f) {
    SetFont(g_fDF);
    if (italic) SetTextShear(0.26f);
    SetTextStretchX(1.35f);
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ pos.x + dx * outline, pos.y + dy * outline }, px, WithAlpha(C_CHR_OUT, a), s);
    DrawTextGradient(pos, px, WithAlpha(C_CHR_T, a), WithAlpha(C_CHR_B, a), s);
    ResetTextStretchX();
    if (italic) ResetTextShear();
    ResetFont();
}
// right-aligned chrome value: lands the string's RIGHT edge on rx (italic face).
void ChromeRight(float rx, float y, float px, const char* s, float a) {
    SetFont(g_fDF);
    SetTextShear(0.26f);
    SetTextStretchX(1.35f);
    float w = MeasureText(px, s).x;
    ResetTextStretchX();
    ResetTextShear();
    ResetFont();
    // MeasureText omits the shear lean + the dark outline halo, so the rendered
    // glyphs overshoot ~25-29px past rx; pull the anchor left to land the real
    // right edge on rx (~661 target).
    Chrome({ rx - w - 29, y }, px, s, a, true, 2.2f);
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

    // ---- live scene slot: Gaia-temple interior — stone walls, a glowing warp
    //      portal cylinder (upper centre, behind the panel) and an ornate rune-
    //      ring stone platform at the base (matches the gate_stage_select scene) ----
    {
        // stone temple backdrop + faint side pillars for depth
        DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_STONE_T, C_STONE_B);
        DrawVGradient({ 24, 0 }, { 150, REF_H }, WithAlpha(RGBA(130, 128, 122, 255), 0.32f), WithAlpha(RGBA(100, 98, 92, 255), 0.32f));
        DrawVGradient({ 1130, 0 }, { 1256, REF_H }, WithAlpha(RGBA(130, 128, 122, 255), 0.32f), WithAlpha(RGBA(100, 98, 92, 255), 0.32f));

        // additive vertical-gradient bar (top->bottom alpha fade)
        auto vbar = [&](float x0, float y0, float x1, float y1, uint32_t top, uint32_t bot) {
            const V2 q[4] = { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };
            const uint32_t c[4] = { top, top, bot, bot };
            DrawQuadGradient(q, c, true);
        };
        // filled perspective ellipse via horizontal strips
        auto ellipse = [&](float cx, float cy, float rx, float ry, uint32_t col) {
            const int N = 22;
            for (int i = 0; i < N; ++i) {
                float y0 = cy - ry + (2 * ry) * i / N, y1 = cy - ry + (2 * ry) * (i + 1) / N;
                float ym = (y0 + y1) * 0.5f - cy;
                float t = 1.0f - (ym * ym) / (ry * ry); if (t < 0) t = 0;
                float hw = rx * std::sqrt(t);
                DrawRect({ cx - hw, y0 }, { cx + hw, y1 }, col);
            }
        };

        // --- warp portal cylinder: centre-bright additive amber column ---
        const float pcx = 712.0f, ptop = -12.0f, pbot = 252.0f;
        auto column = [&](float halfW, uint32_t top, uint32_t bot) {
            const V2 lq[4] = { { pcx - halfW, ptop }, { pcx, ptop }, { pcx, pbot }, { pcx - halfW, pbot } };
            const uint32_t lc[4] = { WithAlpha(top, 0.0f), top, bot, WithAlpha(bot, 0.0f) };
            DrawQuadGradient(lq, lc, true);
            const V2 rq[4] = { { pcx, ptop }, { pcx + halfW, ptop }, { pcx + halfW, pbot }, { pcx, pbot } };
            const uint32_t rc[4] = { top, WithAlpha(top, 0.0f), WithAlpha(bot, 0.0f), bot };
            DrawQuadGradient(rq, rc, true);
        };
        column(152.0f, WithAlpha(RGBA(255, 150, 40, 255), 0.42f), WithAlpha(RGBA(255, 120, 30, 255), 0.10f));   // soft outer halo
        column(66.0f,  WithAlpha(RGBA(255, 198, 96, 255), 0.62f), WithAlpha(RGBA(255, 150, 40, 255), 0.20f));   // bright core
        vbar(pcx - 68, ptop, pcx - 60, pbot, WithAlpha(RGBA(255, 232, 178, 255), 0.55f), WithAlpha(RGBA(255, 232, 178, 255), 0.0f));  // tube wall L
        vbar(pcx + 60, ptop, pcx + 68, pbot, WithAlpha(RGBA(255, 232, 178, 255), 0.55f), WithAlpha(RGBA(255, 232, 178, 255), 0.0f));  // tube wall R

        // --- rune-ring stone platform at the base ---
        const float gx = 628.0f, gy = 602.0f, grx = 148.0f, gry = 45.0f;
        ellipse(gx, gy + 3, grx + 4, gry + 3, WithAlpha(RGBA(40, 30, 18, 255), 0.5f));   // soft drop shadow
        ellipse(gx, gy, grx, gry, RGBA(120, 110, 92, 255));                              // outer stone rim
        ellipse(gx, gy, grx - 9, gry - 3, RGBA(152, 140, 118, 255));                     // lit stone ring
        ellipse(gx, gy, grx - 25, gry - 8, RGBA(68, 52, 34, 255));                       // dark rune channel
        for (int i = 0; i < 18; ++i) {                                                   // glowing rune marks
            float th = i * (6.2831853f / 18.0f), rx = grx - 25, ry = gry - 8;
            float dx = gx + std::cos(th) * (rx - 4), dy = gy + std::sin(th) * (ry - 2);
            DrawRect({ dx - 4, dy - 2 }, { dx + 4, dy + 2 }, RGBA(255, 154, 44, 255));
        }
        ellipse(gx, gy, grx - 44, gry - 14, RGBA(150, 138, 116, 255));                   // inner stone disc
        ellipse(gx, gy, grx - 70, gry - 23, RGBA(60, 46, 30, 255));                      // dark centre well
        ellipse(gx, gy, grx - 80, gry - 26, WithAlpha(RGBA(255, 150, 40, 255), 0.55f));  // centre glow
    }

    // ---- thick angled gold ribbon (from the left edge, slanted right cut) +
    //      two-line chrome wordmark sitting on it ----
    {
        const V2 rb[4] = { { 0, 50 }, { 632, 50 }, { 590, 110 }, { 0, 110 } };
        const uint32_t rc[4] = { WithAlpha(C_BAND_T, a), WithAlpha(C_BAND_M, a), WithAlpha(C_BAND_B, a), WithAlpha(C_BAND_B, a) };
        DrawQuadGradient(rb, rc);
        DrawRect({ 0, 50 }, { 632, 52.5f }, WithAlpha(RGBA(252, 236, 150, 255), a));   // bright top edge
        const V2 sl[4] = { { 642, 50 }, { 654, 50 }, { 612, 110 }, { 600, 110 } };     // detached slash
        const uint32_t sc[4] = { WithAlpha(C_BAND_M, a), WithAlpha(C_BAND_M, a), WithAlpha(C_BAND_B, a), WithAlpha(C_BAND_B, a) };
        DrawQuadGradient(sl, sc);
    }
    Chrome({ 250, 46 }, 40.0f, "STAGE", a);
    Chrome({ 270, 82 }, 40.0f, "SELECT", a);

    if (panT > 0.0f) {
        const Act& act = ACTS[g_act];
        // ---- wide act panel (x268-1003, real borders) with chamfered right corners ----
        const float px0 = 268, py0 = 176, px1 = 1003, py1 = 540, pch = 24;
        DrawVGradient({ px0, py0 }, { px1, py1 }, WithAlpha(C_PANEL_T, panT), WithAlpha(C_PANEL_B, panT));
        // chamfer the top-right + bottom-right corners back to the scene
        const V2 ctr[4] = { { px1 - pch, py0 }, { px1, py0 }, { px1, py0 + pch }, { px1 - pch, py0 } };
        const V2 cbr[4] = { { px1 - pch, py1 }, { px1, py1 }, { px1, py1 - pch }, { px1 - pch, py1 } };
        const uint32_t st[4] = { WithAlpha(C_STONE_B, panT), WithAlpha(C_STONE_B, panT), WithAlpha(C_STONE_B, panT), WithAlpha(C_STONE_B, panT) };
        DrawQuadGradient(ctr, st); DrawQuadGradient(cbr, st);
        DrawRect({ px0, py0 }, { px1 - pch, py0 + 2 }, WithAlpha(C_BORDER, panT));

        // stage-name plate (top-left of the panel)
        NamePlate(273, 197, 557, 234, panT);
        SetFont(g_fSeurat);
        SetTextShear(0.18f);
        DrawText({ 300 + 1.5f, 203 + 1.5f }, 24.0f, WithAlpha(RGBA(14, 14, 16, 255), panT), "Windmill Isle");
        DrawText({ 300, 203 }, 24.0f, WithAlpha(C_WHITE, panT), "Windmill Isle");
        ResetTextShear();
        // Act N header
        DrawText({ 280 + 1.5f, 248 + 1.5f }, 28.0f, WithAlpha(RGBA(14, 14, 16, 255), panT), act.name);
        DrawText({ 280, 248 }, 28.0f, WithAlpha(C_WHITE, panT), act.name);
        ResetFont();
        // stat-block dividers (thin chrome rules, alpha ~0.4): under HIGH SCORE,
        // under BEST TIME, under the medal rows.
        const float STAT_R = 660;                       // shared value right-edge column
        DrawRect({ 266, 349 }, { STAT_R, 350.5f }, WithAlpha(C_BORDER, panT * 0.4f));
        DrawRect({ 266, 399 }, { STAT_R, 400.5f }, WithAlpha(C_BORDER, panT * 0.4f));
        DrawRect({ 266, 501 }, { STAT_R, 502.5f }, WithAlpha(C_BORDER, panT * 0.4f));
        // stat rows: outlined-chrome UPRIGHT label (left x276) + italic chrome VALUE
        // right-aligned to STAT_R.
        auto stat = [&](float ly, const char* lbl, float vy, const char* val) {
            Chrome({ 276, ly }, 22.0f, lbl, panT, false, 1.4f);
            ChromeRight(STAT_R, vy, 26.0f, val, panT);
        };
        stat(326, "HIGH SCORE", 322, act.hiScore);
        stat(376, "BEST TIME",  372, act.bestTime);
        // medal rows: upright chrome MEDALS label, a sun/moon icon ellipse just
        // right of the label, and the count as an italic chrome value at STAT_R.
        Chrome({ 276, 420 }, 22.0f, "MEDALS", panT, false, 1.4f);
        char buf[16];
        // filled vertical ellipse — gold core, coloured rim per row. Tall/narrow
        // oval (~14x33, aspect 1:2.5) so both rows seat inside the medal band (~405-495).
        auto medalEllipse = [&](float cx, float cy, float rx, float ry, uint32_t col) {
            const int N = 18;
            for (int i = 0; i < N; ++i) {
                float y0 = cy - ry + (2 * ry) * i / N, y1 = cy - ry + (2 * ry) * (i + 1) / N;
                float ym = (y0 + y1) * 0.5f - cy;
                float t = 1.0f - (ym * ym) / (ry * ry); if (t < 0) t = 0;
                float hw = rx * std::sqrt(t);
                DrawRect({ cx - hw, y0 }, { cx + hw, y1 }, col);
            }
        };
        const uint32_t C_MEDAL_CORE = RGBA(255, 212, 96, 255);   // gold core
        auto medal = [&](float iy, float vy, uint32_t rim, int got, int tot) {
            snprintf(buf, sizeof buf, "%d / %d", got, tot);
            medalEllipse(425, iy, 7.0f, 16.0f, WithAlpha(rim, panT));             // coloured rim (tall oval ~14x33)
            medalEllipse(425, iy, 4.5f, 11.0f, WithAlpha(C_MEDAL_CORE, panT));    // gold core
            ChromeRight(STAT_R, vy, 26.0f, buf, panT);
        };
        medal(430, 426, C_SUN,  act.sun,  act.sunMax);   // sun row (gold/orange)
        medal(482, 478, C_MOON, act.moon, act.moonMax);  // moon row (gold/blue)
        // RANK label (upright chrome, below the medal divider)
        Chrome({ 276, 516 }, 22.0f, "RANK", panT, false, 1.4f);
        // stage screenshot slot (landscape 270x135, aspect 2.0 — measured stage_ss rect 702,320..972,455)
        // + the big metallic S-rank to its RIGHT, overlapping the photo's bottom-right
        DrawVGradient({ 702, 320 }, { 972, 455 }, WithAlpha(RGBA(60, 76, 98, 255), panT), WithAlpha(RGBA(30, 40, 54, 255), panT));
        DrawRect({ 702, 320 }, { 972, 322 }, WithAlpha(C_BORDER, panT));
        SetFont(g_fSeurat);
        DrawTextAligned({ 702, 320 }, { 972, 455 }, 13.0f, WithAlpha(RGBA(140, 156, 176, 255), panT),
                        "STAGE PHOTO", Align::Center, true, false);
        ResetFont();
        if (g_rankTex >= 0)
            DrawImage(g_rankTex, { 900, 400 }, { 1044, 534 },
                      { RANK_UV[act.rank].u0, RANK_UV[act.rank].v0 }, { RANK_UV[act.rank].u1, RANK_UV[act.rank].v1 },
                      WithAlpha(RGBA(228, 230, 236, 255), panT * 0.95f));

        // ---- carousel act arrows flanking the panel ----
        if (!g_popup) {
            const float ay = 385;
            const V2 la[4] = { { 240, ay - 18 }, { 240, ay + 18 }, { 218, ay }, { 240, ay - 18 } };
            const V2 ra[4] = { { 987, ay - 18 }, { 987, ay + 18 }, { 1009, ay }, { 987, ay - 18 } };
            const uint32_t arc[4] = { WithAlpha(RGBA(228, 232, 238, 200), panT), WithAlpha(RGBA(228, 232, 238, 200), panT), WithAlpha(RGBA(228, 232, 238, 200), panT), WithAlpha(RGBA(228, 232, 238, 200), panT) };
            DrawQuadGradient(la, arc); DrawQuadGradient(ra, arc);
        }
    }

    // ---- footer: [LB] Switch [RB]   (A) Select   (B) Back ----
    //      (the LB/RB switch entry is hidden while the confirm popup is open) ----
    {
        SetFont(g_fRodin);
        float hcy = 636;
        auto glyph = [&](const UV& g, float x, float gh){ if (g_glyphTex<0) return x; float asp=((g.u1-g.u0)*GTW)/((g.v1-g.v0)*GTH), gw=gh*asp; DrawImage(g_glyphTex,{x,hcy-gh*0.5f},{x+gw,hcy+gh*0.5f},{g.u0,g.v0},{g.u1,g.v1}, WithAlpha(C_WHITE,a)); return x+gw+8; };
        // measured glyph anchors: [LB] Switch [RB] -> x244, (A) Select -> x~700, (B) Back -> x~877
        float hx = g_popup ? 520 : 244;
        if (!g_popup) {
            hx = glyph(GLYPH_LB, hx, 28);
            DrawText({ hx, hcy - 11 }, 20.0f, WithAlpha(C_WHITE, a), "Switch"); hx += 90;
            hx = glyph(GLYPH_RB, hx, 28); hx += 244;
        }
        hx = glyph(GLYPH_A, hx, 30);
        DrawText({ hx, hcy - 11 }, 20.0f, WithAlpha(C_WHITE, a), "Select"); hx += 137;
        hx = glyph(GLYPH_B, hx, 30);
        DrawText({ hx, hcy - 11 }, 20.0f, WithAlpha(C_WHITE, a), "Back");
        ResetFont();
    }

    // ---- "Play Stage / Cancel" confirm popup (the GREY dialog family, like
    //      the pause confirm — measured from gate_confirm_popup.png) ----
    if (g_popup) {
        DrawRect({ 0, 0 }, { REF_W, REF_H }, RGBA(0, 0, 0, 68));
        // screen-centred, tighter TL+BR chamfered grey dialog (flatter, dimmer
        // border — closer to the real near-borderless silver box)
        const float px0 = 531, py0 = 281, px1 = 748, py1 = 437, pch = 16, cx = (px0 + px1) * 0.5f;
        DrawVGradient({ px0, py0 }, { px1, py1 },
                      RGBA(152, 154, 156, 240), RGBA(122, 124, 126, 240));
        const V2 c1[4] = { { px0, py0 }, { px0 + pch, py0 }, { px0, py0 + pch }, { px0, py0 } };
        const V2 c2[4] = { { px1 - pch, py1 }, { px1, py1 }, { px1, py1 - pch }, { px1 - pch, py1 } };
        const uint32_t dk[4] = { RGBA(8, 8, 8, 110), RGBA(8, 8, 8, 110), RGBA(8, 8, 8, 110), RGBA(8, 8, 8, 110) };
        DrawQuadGradient(c1, dk); DrawQuadGradient(c2, dk);
        uint32_t pbd = RGBA(168, 170, 172, 220);
        DrawRect({ px0 + pch, py0 }, { px1, py0 + 1.5f }, pbd);
        DrawRect({ px0, py1 - 1.5f }, { px1 - pch, py1 }, pbd);
        DrawRect({ px0, py0 + pch }, { px0 + 1.5f, py1 }, pbd);
        DrawRect({ px1 - 1.5f, py0 }, { px1, py1 - pch }, pbd);
        const char* OPT[2] = { "Play Stage", "Cancel" };
        const float rowY[2] = { py0 + 50, py0 + 98 };
        DrawVGradient({ px0 + 14, rowY[g_popupSel] - 6 }, { px1 - 14, rowY[g_popupSel] + 34 },
                      RGBA(196, 176, 104, 196), RGBA(168, 142, 72, 196));
        SetFont(g_fRodin);
        for (int i = 0; i < 2; ++i) {
            float w = MeasureText(27.0f, OPT[i]).x;
            bool sel = (i == g_popupSel);
            DrawText({ cx - w * 0.5f + 1, rowY[i] + 1 }, 27.0f,
                     sel ? RGBA(150, 70, 16, 255) : RGBA(20, 20, 20, 200), OPT[i]);
            DrawText({ cx - w * 0.5f, rowY[i] }, 27.0f,
                     sel ? RGBA(232, 120, 30, 255) : RGBA(235, 235, 235, 255), OPT[i]);
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
