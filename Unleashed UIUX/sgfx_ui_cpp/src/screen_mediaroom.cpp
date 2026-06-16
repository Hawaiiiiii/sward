// =============================================================================
// screen_mediaroom.cpp — Professor Pickle's lab menus, re-authored 1:1 from
// LIVE capture (session20; spec stills mediaroom_{encyclopedia,theater,
// soundtrack}.png; full numeric spec in game_captures/MEDIAROOM_SPEC.md).
// The real menus are PARCHMENT/BOOK-styled panels floating over the live 3D
// lab — NOT the chrome-window style this file used before:
//   * shared chrome: full-width header strip (cream/gold rules over a green
//     gradient field) with a beveled slab title at x278, gold pinstripe +
//     curl ornaments, the "Pickle's Room" oval seal; a parchment panel
//     (220,121)-(1060,613) with a brown pinstripe frame, a white keyline
//     with corner hooks, and a mottled paper interior (185,176,140);
//   * ENCYCLOPEDIA text page: portrait slot on the left half, flag icon +
//     entry title on a faint highlight band, ruled double-lines (pitch 33)
//     with body text, a green page plaque "003 / 098" with gold curls, and
//     gold page chevrons straddling the panel edges;
//   * ENCYCLOPEDIA art page (LB/RB "Switch"): one large borderless image
//     slot, plaque "023 / 071";
//   * SOUNDTRACK: a 3x4 grid of video-still thumbnails (cell 152x82.7,
//     pitch 176x104.7 from (284,167.3)), sunken shadows + cream borders,
//     a white-gold selection ring with corner brackets + pulsing glow +
//     flag badge, a maroon scrollbar, divider rules and a caption line.
// Tab keys cycle the three views (the in-game lab walks between stations).
// Art slots (portraits, stills) stay empty per the art/structure boundary.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_glyphTex = -1, g_sunMedTex = -1, g_moonMedTex = -1;
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

struct UV { float u0, v0, u1, v1; };
constexpr float GTW = 512.0f, GTH = 512.0f;
const UV GLYPH_A  = { 0.00000f, 0.00781f, 0.07227f, 0.07617f };
const UV GLYPH_B  = { 0.08008f, 0.00781f, 0.15039f, 0.07422f };
const UV GLYPH_LB = { 0.32617f, 0.00781f, 0.46094f, 0.07812f };
const UV GLYPH_RB = { 0.48242f, 0.00781f, 0.61523f, 0.07812f };

// ---- palette (sampled from the capture; MEDIAROOM_SPEC.md) -------------------
const uint32_t C_PARCH     = RGBA(185, 176, 140, 255);   // parchment mean
const uint32_t C_PARCH_HI  = RGBA(196, 185, 151, 255);
const uint32_t C_PARCH_LO  = RGBA(174, 166, 132, 255);
const uint32_t C_FRAME_BRN = RGBA(66, 44, 16, 255);      // frame band dark-brown outer
const uint32_t C_PINSTRIPE = RGBA(228, 212, 166, 255);   // cream pinstripe
const uint32_t C_KEYLINE   = RGBA(236, 233, 225, 255);   // white keyline
const uint32_t C_RULE      = RGBA(215, 206, 165, 255);   // embossed ruled line
const uint32_t C_TEXT      = RGBA(9, 5, 5, 255);         // near-black body text
const uint32_t C_HDR_T     = RGBA(88, 93, 70, 235);      // header green field
const uint32_t C_HDR_B     = RGBA(56, 67, 46, 235);
const uint32_t C_HDR_CREAM = RGBA(227, 214, 180, 255);
const uint32_t C_HDR_GOLD  = RGBA(204, 181, 89, 255);
const uint32_t C_TITLE_T   = RGBA(232, 206, 108, 255);   // beveled title slab — GOLD (measured real ~193,172,91)
const uint32_t C_TITLE_B   = RGBA(196, 162, 72, 255);    // gold bottom bevel
const uint32_t C_TITLE_OUT = RGBA(65, 62, 39, 255);
const uint32_t C_PLQ_GREEN = RGBA(95, 91, 56, 255);      // page counter band = olive (G near R, not brown)
const uint32_t C_PLQ_GOLD  = RGBA(228, 199, 69, 255);
const uint32_t C_CNT_T     = RGBA(238, 234, 226, 255);   // counter digits bevel (no yellow cast)
const uint32_t C_CNT_B     = RGBA(222, 217, 208, 255);   // warm neutral cream
const uint32_t C_CHEV      = RGBA(235, 220, 87, 255);    // page chevron gold
const uint32_t C_CHEV_FADE = RGBA(191, 176, 139, 255);
const uint32_t C_THUMB_BD  = RGBA(209, 204, 184, 255);   // thumb cream border
const uint32_t C_THUMB_SH  = RGBA(150, 141, 112, 255);   // sunken shadow
const uint32_t C_SEL_W     = RGBA(240, 237, 242, 255);   // selection ring white
const uint32_t C_SEL_G     = RGBA(239, 208, 40, 255);    // selection ring gold (saturated — most real saturation lives in the ring/border)
const uint32_t C_SEL_GLOW  = RGBA(245, 210, 55, 255);    // pulsing saturated-gold glow (blue dropped for saturation)
const uint32_t C_SCR_T     = RGBA(87, 35, 21, 255);      // scrollbar maroon
const uint32_t C_SCR_B     = RGBA(34, 16, 14, 255);
const uint32_t C_SCR_HND   = RGBA(214, 204, 181, 255);
const uint32_t C_FLAG_BLUE = RGBA(50, 96, 185, 255);     // flag badge field (soundtrack crest)
const uint32_t C_FLAG_NAVY_T = RGBA(68, 68, 178, 255);   // Alexis flag navy gradient top
const uint32_t C_FLAG_NAVY_B = RGBA(22, 21, 108, 255);   // Alexis flag navy gradient bottom
const uint32_t C_SEAL_GRN  = RGBA(60, 80, 50, 255);      // Pickle's Room seal
const uint32_t C_WHITE     = RGBA(255, 255, 255, 255);
// lab-scene placeholder (the real game renders the 3D library behind)
const uint32_t C_LAB_T = RGBA(96, 74, 52, 255), C_LAB_B = RGBA(58, 42, 30, 255);

// ---- shared panel geometry (measured) ----------------------------------------
constexpr float PNL_X0 = 220.0f, PNL_Y0 = 121.3f, PNL_X1 = 1060.0f, PNL_Y1 = 612.7f;
constexpr float INT_X0 = 237.3f, INT_Y0 = 139.3f, INT_X1 = 1042.0f, INT_Y1 = 594.0f;

enum View { VIEW_ENCY_TEXT = 0, VIEW_ENCY_ART, VIEW_SOUND, VIEW_COUNT };
int g_view = VIEW_ENCY_TEXT;
int g_selR = 2, g_selC = 3;     // soundtrack selection (matches the capture)

void Init() {
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    if (g_sunMedTex  < 0) g_sunMedTex  = gfx::loadTexture("assets/gameart/medallion_sun.png");
    if (g_moonMedTex < 0) g_moonMedTex = gfx::loadTexture("assets/gameart/medallion_moon.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() { g_view = VIEW_ENCY_TEXT; g_selR = 2; g_selC = 3; }
void Input(const ScreenInput& in) {
    if (in.tabRight) g_view = (g_view + 1) % VIEW_COUNT;
    if (in.tabLeft)  g_view = (g_view + VIEW_COUNT - 1) % VIEW_COUNT;
    if (g_view == VIEW_SOUND) {
        if (in.left)  g_selC = std::max(0, g_selC - 1);
        if (in.right) g_selC = std::min(3, g_selC + 1);
        if (in.up)    g_selR = std::max(0, g_selR - 1);
        if (in.down)  g_selR = std::min(2, g_selR + 1);
    }
}

// ---- shared chrome ------------------------------------------------------------
void HeaderStrip(const char* title, float a) {
    // layered rules over a green gradient field (y 56..110.7)
    DrawRect({ 0, 57.3f }, { REF_W, 59.3f }, WithAlpha(C_HDR_CREAM, a));
    DrawRect({ 0, 60.0f }, { REF_W, 62.3f }, WithAlpha(RGBA(95, 75, 40, 255), a));
    DrawRect({ 0, 62.7f }, { REF_W, 64.7f }, WithAlpha(C_HDR_GOLD, a));
    DrawVGradient({ 0, 65.3f }, { REF_W, 100.7f }, WithAlpha(C_HDR_T, a), WithAlpha(C_HDR_B, a));
    DrawRect({ 0, 101.3f }, { REF_W, 104.7f }, WithAlpha(C_HDR_CREAM, a));
    DrawRect({ 0, 108.0f }, { REF_W, 110.0f }, WithAlpha(RGBA(186, 160, 78, 255), a));   // bottom rule darker than the bright top rule
    // left art-deco pinstripes + curl hint (procedural approximation)
    for (int i = 0; i < 3; ++i)
        DrawRect({ 8, 70.0f + i * 9 }, { 214, 71.5f + i * 9 }, WithAlpha(C_HDR_GOLD, a * 0.9f));
    // beveled slab title at the measured x (ref band ~270 wide x 29 tall):
    // olive-brown outline ring first, then the cream->gold bevel face
    SetFont(g_fDF);
    SetTextStretchX(1.05f);   // measured real gold wordmark is compact (~140px ink), left-anchored
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                DrawText({ 140.0f + dx * 1.2f, 70.0f + dy * 1.2f }, 27.0f, WithAlpha(C_TITLE_OUT, a), title);
    SetModifier(MOD_TITLE_BEVEL);
    DrawTextGradient({ 140, 70 }, 27.0f, WithAlpha(C_TITLE_T, a), WithAlpha(C_TITLE_B, a), title);
    ResetModifier();
    ResetTextStretchX();
    ResetFont();
    // "Pickle's Room" oval seal (procedural: rings + italic script)
    const float cx = 968, cy = 81.5f, rx = 48, ry = 41.5f;
    for (int ring = 0; ring < 3; ++ring) {
        float fx = rx - ring * 5, fy = ry - ring * 5;
        uint32_t col = (ring == 1) ? C_HDR_CREAM : (ring == 0 ? RGBA(40, 52, 34, 255) : C_SEAL_GRN);
        for (int s = 0; s < 28; ++s) {   // coarse ellipse from short bars
            float a0 = (float)s / 28.0f * 6.2831853f, a1 = (float)(s + 1) / 28.0f * 6.2831853f;
            V2 p0 = { cx + std::cos(a0) * fx, cy + std::sin(a0) * fy };
            V2 p1 = { cx + std::cos(a1) * fx, cy + std::sin(a1) * fy };
            V2 n  = { (p1.y - p0.y) * 0.18f, (p0.x - p1.x) * 0.18f };
            const V2 q[4] = { p0, p1, { p1.x + n.x, p1.y + n.y }, { p0.x + n.x, p0.y + n.y } };
            const uint32_t qc[4] = { WithAlpha(col, a), WithAlpha(col, a), WithAlpha(col, a), WithAlpha(col, a) };
            DrawQuadGradient(q, qc);
        }
        if (ring == 2) {   // fill the innermost
            DrawRect({ cx - fx * 0.78f, cy - fy * 0.78f }, { cx + fx * 0.78f, cy + fy * 0.78f }, WithAlpha(C_SEAL_GRN, a));
        }
    }
    SetFont(g_fSeurat);
    SetTextShear(0.30f);
    DrawText({ cx - 36, cy - 16 }, 14.0f, WithAlpha(C_HDR_GOLD, a), "Pickle's");
    DrawText({ cx - 26, cy + 1 },  14.0f, WithAlpha(C_HDR_GOLD, a), "Room");
    ResetTextShear();
    ResetFont();
    for (int i = 0; i < 3; ++i)
        DrawRect({ 1040, 70.0f + i * 9 }, { REF_W - 8, 71.5f + i * 9 }, WithAlpha(C_HDR_GOLD, a * 0.9f));
}

void ParchmentPanel(float a) {
    // frame band, structured outward->inward. Round-5's single WIDE bright-gold
    // band was wrong: the real frame is a DOMINANT bright WHITE keyline plus a
    // finely-striped thin border (no saturated gold band). Outermost is a thin
    // dark-brown lip; then 3-4 THIN (2-3px) alternating pinstripes inward
    // (dark-brown/cream/brown/cream) whose blended mean lands near (165,153,129);
    // a single ~2px muted-tan accent replaces the old gold band. The white
    // keyline stays the dominant bright ring.
    DrawRect({ PNL_X0 + 2, PNL_Y0 }, { PNL_X1 - 2, PNL_Y1 }, WithAlpha(C_FRAME_BRN, a));    // thin dark-brown outer lip
    // finely-striped thin border: 4 alternating 2-3px pinstripes per edge, inward
    const uint32_t STRIPE[4] = {
        RGBA(66, 44, 16, 255),    // dark-brown
        RGBA(195, 170, 123, 255), // cream
        RGBA(124, 87, 26, 255),   // brown
        RGBA(190, 172, 136, 255), // cream
    };
    for (int i = 0; i < 4; ++i) {
        const float o = 4.0f + i * 2.5f;      // inward offset per stripe (~2.5px pitch)
        const uint32_t c = WithAlpha(STRIPE[i], a);
        DrawRect({ PNL_X0 + o, PNL_Y0 + o - 4.0f }, { PNL_X1 - o, PNL_Y0 + o - 2.0f }, c);  // top stripe
        DrawRect({ PNL_X0 + o, PNL_Y1 - o + 2.0f }, { PNL_X1 - o, PNL_Y1 - o + 4.0f }, c);  // bottom stripe
        DrawRect({ PNL_X0 + o - 4.0f, PNL_Y0 + o }, { PNL_X0 + o - 2.0f, PNL_Y1 - o }, c);  // left stripe
        DrawRect({ PNL_X1 - o + 2.0f, PNL_Y0 + o }, { PNL_X1 - o + 4.0f, PNL_Y1 - o }, c);  // right stripe
    }
    // single ~2px muted-tan accent just inside the stripes (replaces the gold band)
    DrawRect({ PNL_X0 + 14, PNL_Y0 + 10 }, { PNL_X1 - 14, PNL_Y0 + 12 }, WithAlpha(RGBA(190, 172, 136, 255), a));
    DrawRect({ PNL_X0 + 14, PNL_Y1 - 12 }, { PNL_X1 - 14, PNL_Y1 - 10 }, WithAlpha(RGBA(190, 172, 136, 255), a));
    DrawRect({ PNL_X0 + 14, PNL_Y0 + 10 }, { PNL_X0 + 16, PNL_Y1 - 10 }, WithAlpha(RGBA(190, 172, 136, 255), a));
    DrawRect({ PNL_X1 - 16, PNL_Y0 + 10 }, { PNL_X1 - 14, PNL_Y1 - 10 }, WithAlpha(RGBA(190, 172, 136, 255), a));
    // white keyline (DOMINANT bright ring): outside the side bands, inside top/bottom
    DrawRect({ PNL_X0, PNL_Y0 }, { PNL_X0 + 3.3f, PNL_Y1 }, WithAlpha(C_KEYLINE, a));
    DrawRect({ PNL_X1 - 3.3f, PNL_Y0 }, { PNL_X1, PNL_Y1 }, WithAlpha(C_KEYLINE, a));
    DrawRect({ PNL_X0, 134.0f }, { PNL_X1, 136.7f }, WithAlpha(C_KEYLINE, a));
    DrawRect({ PNL_X0, 597.3f }, { PNL_X1, 599.3f }, WithAlpha(C_KEYLINE, a));
    // corner hooks (stepped Greek-key hint)
    auto hook = [&](float hx, float hy, float sx, float sy) {
        DrawRect({ hx, hy }, { hx + 15 * sx, hy + 2.5f * sy }, WithAlpha(C_KEYLINE, a));
        DrawRect({ hx + 12.5f * sx, hy }, { hx + 15 * sx, hy + 15 * sy }, WithAlpha(C_KEYLINE, a));
    };
    hook(236.7f, 139.0f, 1, 1); hook(1043.3f, 139.0f, -1, 1);
    hook(236.7f, 592.0f, 1, -1); hook(1043.3f, 592.0f, -1, -1);
    // mottled parchment interior: base + soft tonal streaks + fine ageing specks
    DrawVGradient({ INT_X0, INT_Y0 }, { INT_X1, INT_Y1 }, WithAlpha(C_PARCH_HI, a), WithAlpha(C_PARCH, a));
    uint32_t s = 0x9e3779b9u;
    for (int i = 0; i < 26; ++i) {
        s = s * 1664525u + 1013904223u; float x = INT_X0 + (float)((s >> 8) % (int)(INT_X1 - INT_X0 - 60));
        s = s * 1664525u + 1013904223u; float y = INT_Y0 + (float)((s >> 8) % (int)(INT_Y1 - INT_Y0 - 90));
        s = s * 1664525u + 1013904223u; float w = 24 + (float)((s >> 8) % 60);
        s = s * 1664525u + 1013904223u; float h = 40 + (float)((s >> 8) % 60);
        DrawRect({ x, y }, { x + w, y + h }, WithAlpha(C_PARCH_LO, a * 0.14f));
    }
    for (int i = 0; i < 120; ++i) {   // fine specks (aged-paper grain)
        s = s * 1664525u + 1013904223u; float x = INT_X0 + (float)((s >> 8) % (int)(INT_X1 - INT_X0 - 4));
        s = s * 1664525u + 1013904223u; float y = INT_Y0 + (float)((s >> 8) % (int)(INT_Y1 - INT_Y0 - 4));
        s = s * 1664525u + 1013904223u; bool dark = ((s >> 10) & 1) != 0;
        DrawRect({ x, y }, { x + 2.0f, y + 2.0f },
                 WithAlpha(dark ? C_PARCH_LO : C_PARCH_HI, a * 0.5f));
    }
}

void PagePlaque(const char* counter, float a) {
    // olive plaque overlapping the bottom frame, gold rules + flanking curls
    DrawRect({ 440, 582.7f }, { 841.3f, 584.0f }, WithAlpha(RGBA(224, 215, 166, 255), a));
    DrawRect({ 440, 584.0f }, { 841.3f, 606.0f }, WithAlpha(C_PLQ_GREEN, a));
    DrawRect({ 440, 606.0f }, { 841.3f, 609.0f }, WithAlpha(C_PLQ_GOLD, a));
    DrawRect({ 440, 609.7f }, { 841.3f, 612.0f }, WithAlpha(RGBA(224, 215, 166, 255), a));
    for (int sgn = 0; sgn < 2; ++sgn) {   // curl ornament hint: 3 nested bars
        float bx = sgn ? 733.3f : 464.0f;
        for (int i = 0; i < 3; ++i)
            DrawRect({ bx + i * 6, 590.0f + i * 3 }, { bx + 82 - i * 12, 592.0f + i * 3 }, WithAlpha(C_HDR_GOLD, a));
    }
    SetFont(g_fRodin);
    float w = MeasureText(24.0f, counter).x;
    DrawTextGradient({ 640.7f - w * 0.5f, 586.0f }, 24.0f, WithAlpha(C_CNT_T, a), WithAlpha(C_CNT_B, a), counter);
    ResetFont();
}

void PageChevrons(float a, double now) {
    const float nudge = Breathe(now, 0.0f, 4.0f, 1.2f);   // sliding pulse
    auto chev = [&](float vx, float vy, float dir) {
        for (int i = 0; i < 3; ++i) {   // three nested arms
            float t = 32.0f + i * 16.0f;
            uint32_t col = (i < 2) ? C_CHEV : C_CHEV_FADE;
            float ax = vx + dir * (t * 0.55f + nudge), ay0 = vy - t, ay1 = vy + t;
            const V2 q1[4] = { { ax, ay0 }, { ax + dir * 14, ay0 + 5 }, { vx + dir * nudge + dir * 14, vy }, { vx + dir * nudge, vy } };
            const V2 q2[4] = { { vx + dir * nudge, vy }, { vx + dir * nudge + dir * 14, vy }, { ax + dir * 14, ay1 - 5 }, { ax, ay1 } };
            const uint32_t qc[4] = { WithAlpha(col, a), WithAlpha(col, a), WithAlpha(col, a), WithAlpha(col, a) };
            DrawQuadGradient(q1, qc); DrawQuadGradient(q2, qc);
        }
    };
    chev(192.0f, 366.0f, +1);    // left "<" (vertex at the panel edge, opens right)
    chev(1068.0f, 366.0f, -1);   // right ">"
}

void Glyph(const UV& g, float x, float cy, float gh, float a, float* outRight = nullptr) {
    if (g_glyphTex < 0) return;
    float asp = ((g.u1 - g.u0) * GTW) / ((g.v1 - g.v0) * GTH), gw = gh * asp;
    DrawImage(g_glyphTex, { x, cy - gh * 0.5f }, { x + gw, cy + gh * 0.5f },
              { g.u0, g.v0 }, { g.u1, g.v1 }, WithAlpha(C_WHITE, a));
    if (outRight) *outRight = x + gw;
}

void FooterEncy(float a) {   // [LB] Switch [RB] (left) | Back (right) — shared button-guide
    static const GuideBtn F[] = { { "Switch", GIcon::LBRB, GAlign::Left, 115.0f }, { "Back", GIcon::B, GAlign::Right, 0.0f } };
    DrawButtonGuide(F, 2, g_fRodin, a, 256.0f);
}
void FooterSound(float a) {   // Select | Back (right) — shared button-guide
    static const GuideBtn F[] = { { "Select", GIcon::A, GAlign::Right, 115.0f }, { "Back", GIcon::B, GAlign::Right, 0.0f } };
    DrawButtonGuide(F, 2, g_fRodin, a, 379.0f);
}

// ---- views ---------------------------------------------------------------------
void DrawEncyText(float a, double now) {
    // portrait slot (user art drops in): contact shadow + faint slot hint
    DrawRect({ 360, 565 }, { 480, 592 }, WithAlpha(RGBA(120, 110, 84, 255), a * 0.45f));
    DrawRect({ 343, 267 }, { 496, 592 }, WithAlpha(C_PARCH_LO, a * 0.35f));
    SetFont(g_fSeurat);
    DrawTextAligned({ 343, 267 }, { 496, 592 }, 14.0f, WithAlpha(RGBA(130, 120, 95, 255), a), "PORTRAIT", Align::Center, true, false);
    // title band + flag icon + entry name
    DrawRect({ 496.7f, 177.3f }, { 793.3f, 212.0f }, WithAlpha(C_WHITE, a * 0.18f));
    // Alexis flag: navy vertical gradient field, thick white stripes (white is a
    // major share of the field), and a white anchor crest in the left third.
    DrawVGradient({ 501.3f, 174.7f }, { 563.3f, 208.0f }, WithAlpha(C_FLAG_NAVY_T, a), WithAlpha(C_FLAG_NAVY_B, a)); // flag field
    for (int i = 0; i < 4; ++i)
        DrawRect({ 505, 178.0f + i * 8.2f }, { 559, 182.5f + i * 8.2f }, WithAlpha(C_WHITE, a * 0.9f));   // thick white stripes (~4.5px)
    DrawRect({ 501.3f, 174.7f }, { 563.3f, 176.0f }, WithAlpha(C_WHITE, a));               // flag outline hint
    {   // small white anchor glyph confined to the bottom-left quarter of the
        // field (not full height): shank top lowered to ~y186, a single thin
        // crossbar, and tightened fluke spread.
        const float ax = 511.6f;                                   // anchor centre x (left quarter)
        DrawRect({ ax - 1.2f, 186.0f }, { ax + 1.2f, 203.5f }, WithAlpha(C_WHITE, a));      // vertical shank (lowered top, shorter)
        DrawRect({ ax - 4.5f, 188.0f }, { ax + 4.5f, 189.6f }, WithAlpha(C_WHITE, a));      // single thin crossbar
        DrawRect({ ax - 1.6f, 184.2f }, { ax + 1.6f, 186.4f }, WithAlpha(C_WHITE, a));      // ring nub atop the shank
        for (int i = 0; i < 4; ++i) {                              // two outward-curving flukes (tighter width)
            float fy = 200.0f + i * 1.2f, dx = 2.2f + i * 1.0f;
            DrawRect({ ax - 1.2f - dx, fy }, { ax - 1.2f - dx + 1.4f, fy + 1.4f }, WithAlpha(C_WHITE, a));
            DrawRect({ ax + 1.2f + dx - 1.4f, fy }, { ax + 1.2f + dx, fy + 1.4f }, WithAlpha(C_WHITE, a));
        }
    }
    DrawText({ 568.7f, 178 }, 26.0f, WithAlpha(C_TEXT, a), "Alexis");
    // ruled double-lines + body text (pitch 33, pairs 6.7 apart, x 507..973)
    const char* L[] = { "Lambros's son, a", "wild, unruly boy.", "",
                        "Alexis hardly ever", "sees his father,",
                        "and can't help but", "miss him at times." };
    for (int i = 0; i < 9; ++i) {
        float ry = 278.0f + i * 33.0f;
        DrawRect({ 506.7f, ry }, { 973, ry + 1.3f }, WithAlpha(C_RULE, a));
        DrawRect({ 506.7f, ry + 6.7f }, { 973, ry + 8.0f }, WithAlpha(C_RULE, a));
    }
    for (int i = 0; i < 7; ++i)
        if (L[i][0])
            DrawText({ 513.3f, 278.0f + i * 33.0f - 22.5f }, 22.0f, WithAlpha(C_TEXT, a), L[i]);
    ResetFont();
    PagePlaque("003 / 098", a);
    PageChevrons(a, now);
}

void DrawEncyArt(float a, double now) {
    // large borderless image slot, horizontally centred on the interior
    DrawRect({ 338.7f, 149.3f }, { 939.3f, 572.0f }, WithAlpha(RGBA(46, 58, 84, 255), a));
    SetFont(g_fSeurat);
    DrawTextAligned({ 338.7f, 149.3f }, { 939.3f, 572.0f }, 16.0f,
                    WithAlpha(RGBA(120, 134, 160, 255), a), "ARTWORK", Align::Center, true, false);
    ResetFont();
    PagePlaque("023 / 071", a);
    PageChevrons(a, now);
}

void DrawSoundtrack(float a, double now) {
    // 3x4 grid: image 152 x 82.7, origin (284,167.3), pitch (176,104.7)
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            const float x = 284.0f + 176.0f * c, y = 167.3f + 104.7f * r;
            // sunken shadow (left/right/bottom)
            DrawRect({ x - 4, y + 3 }, { x, y + 86.7f }, WithAlpha(C_THUMB_SH, a));
            DrawRect({ x + 152, y + 3 }, { x + 156, y + 86.7f }, WithAlpha(C_THUMB_SH, a));
            DrawRect({ x - 4, y + 82.7f }, { x + 156, y + 86.7f }, WithAlpha(C_THUMB_SH, a));
            // cream border + image slot
            DrawRect({ x - 2, y - 2 }, { x + 154, y + 84.7f }, WithAlpha(C_THUMB_BD, a));
            DrawVGradient({ x, y }, { x + 152, y + 82.7f },
                          WithAlpha(RGBA(70, 84, 110, 255), a), WithAlpha(RGBA(38, 48, 66, 255), a));
            // gold scroll-curl + note ornament hint (bottom-right quadrant)
            DrawRect({ x + 96, y + 58 }, { x + 132, y + 61 }, WithAlpha(RGBA(190, 157, 107, 255), a));
            DrawRect({ x + 124, y + 46 }, { x + 127, y + 64 }, WithAlpha(RGBA(190, 157, 107, 255), a));
        }
    }
    // selection ring on (g_selR, g_selC): rails + corner brackets + pulsing glow
    {
        const float x = 284.0f + 176.0f * g_selC, y = 167.3f + 104.7f * g_selR;
        const float glow = Breathe(now, 0.35f, 0.9f, 1.1f);
        DrawRect({ x - 14.7f, y - 20.7f }, { x + 164.7f, y + 96.0f }, WithAlpha(C_SEL_GLOW, a * glow * 0.5f), true);   // saturated-gold outer glow
        DrawRect({ x - 7.0f, y - 11.0f }, { x + 157.0f, y + 86.0f }, WithAlpha(C_SEL_GLOW, a * glow * 0.45f), true);   // brighter inner pass
        auto rail = [&](float rx0, float ry0, float rx1, float ry1) {
            DrawRect({ rx0, ry0 }, { rx1, ry1 }, WithAlpha(C_SEL_G, a));
            DrawRect({ rx0, ry0 }, { rx0 + (rx1 - rx0), ry0 + 1.3f }, WithAlpha(C_SEL_W, a));
            DrawRect({ rx0, ry1 - 1.3f }, { rx1, ry1 }, WithAlpha(C_SEL_W, a));
        };
        rail(x - 8.0f, y - 13.3f, x - 0.7f, y + 97.3f);            // left rail
        rail(x + 152.7f, y - 13.3f, x + 160.0f, y + 97.3f);        // right rail
        // corner bracket arms (top/bottom edges open in the middle)
        rail(x - 13.3f, y - 13.3f, x + 20.0f, y - 1.3f);
        rail(x + 129.3f, y - 13.3f, x + 163.3f, y - 1.3f);
        rail(x - 13.3f, y + 82.0f, x + 20.0f, y + 97.3f);
        rail(x + 129.3f, y + 82.0f, x + 163.3f, y + 97.3f);
        // flag badge top-left of the selected thumb (~55x39 crest, not stripes)
        DrawRect({ x + 5.3f, y - 0.7f }, { x + 55.0f, y + 39.0f }, WithAlpha(C_FLAG_BLUE, a));
        DrawRect({ x + 5.3f, y - 0.7f }, { x + 55.0f, y + 1.0f }, WithAlpha(C_WHITE, a));
        // white shield card (dominant element) with a tiny red+green crest hint
        DrawRect({ x + 10, y + 3 }, { x + 50, y + 36 }, WithAlpha(RGBA(238, 235, 232, 255), a));
        DrawRect({ x + 18, y + 9 }, { x + 30, y + 30 }, WithAlpha(RGBA(186, 54, 46, 255), a));
        DrawRect({ x + 31, y + 9 }, { x + 43, y + 30 }, WithAlpha(RGBA(66, 132, 70, 255), a));
    }
    // maroon scrollbar (track + cream handle at the captured 61%)
    DrawRect({ 985.3f, 158.7f }, { 987.3f, 471.3f }, WithAlpha(C_PINSTRIPE, a));
    DrawRect({ 1000.0f, 158.7f }, { 1001.3f, 471.3f }, WithAlpha(C_PINSTRIPE, a));
    DrawVGradient({ 988.0f, 158.7f }, { 999.3f, 471.3f }, WithAlpha(C_SCR_T, a), WithAlpha(C_SCR_B, a));
    DrawRect({ 988.0f, 332.7f }, { 999.3f, 389.3f }, WithAlpha(C_SCR_HND, a));
    // divider rules + caption + its ruled lines
    DrawRect({ 266.7f, 461.3f }, { 973.3f, 462.6f }, WithAlpha(C_RULE, a));
    DrawRect({ 266.7f, 468.0f }, { 973.3f, 469.3f }, WithAlpha(C_RULE, a));
    SetFont(g_fSeurat);
    DrawText({ 280.7f, 494.0f }, 26.0f, WithAlpha(RGBA(16, 11, 0, 255), a), "Spagonia - Day");
    ResetFont();
    DrawRect({ 266.7f, 515.3f }, { 973.3f, 516.6f }, WithAlpha(C_RULE, a));
    DrawRect({ 266.7f, 522.7f }, { 973.3f, 524.0f }, WithAlpha(C_RULE, a));
    DrawRect({ 266.7f, 548.7f }, { 973.3f, 550.0f }, WithAlpha(C_RULE, a));
    DrawRect({ 266.7f, 557.3f }, { 973.3f, 558.6f }, WithAlpha(C_RULE, a));
}

void Draw(double openSec) {
    const double now = Now();
    const float a = (float)ComputeMotion(openSec, 0.0, 10.0);
    // lab-scene placeholder (warm library tones; real game = live 3D room)
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_LAB_T, C_LAB_B);
    DrawRect({ 0, 540 }, { REF_W, REF_H }, RGBA(44, 32, 24, 255));

    const char* title = (g_view == VIEW_SOUND) ? "Soundtrack" : "Encyclopedia";
    HeaderStrip(title, a);
    ParchmentPanel(a);
    switch (g_view) {
        case VIEW_ENCY_TEXT: DrawEncyText(a, now); FooterEncy(a); break;
        case VIEW_ENCY_ART:  DrawEncyArt(a, now);  FooterEncy(a); break;
        default:             DrawSoundtrack(a, now); FooterSound(a); break;
    }
    // persistent hub medal-level HUD (top-left, in front of the panel frame):
    // two stacked rows, Sun then Moon, each = medal icon + "Lv7 [200]"
    {
        SetFont(g_fSeurat);
        auto chip = [&](float cy, int medTex, uint32_t icol, const char* txt) {
            if (medTex >= 0) {   // real Sun/Moon medallion sprite (gold ring + gem)
                DrawImage(medTex, { 129, cy - 14 }, { 158, cy + 14 }, { 0.f, 0.f }, { 1.f, 1.f },
                          WithAlpha(RGBA(255, 255, 255, 255), a));
            } else {             // procedural fallback
                DrawRect({ 131, cy - 12 }, { 155, cy + 12 }, WithAlpha(RGBA(40, 30, 8, 230), a));
                DrawRect({ 133, cy - 10 }, { 153, cy + 10 }, WithAlpha(icol, a));
                DrawRect({ 138, cy - 5 }, { 148, cy + 5 }, WithAlpha(RGBA(150, 116, 36, 255), a));
            }
            DrawTextShadow({ 163, cy - 9 }, 16.0f, WithAlpha(RGBA(244, 240, 230, 255), a), txt);
        };
        chip(135, g_sunMedTex,  RGBA(214, 96, 40, 255), "Lv7 [200]");    // sun
        chip(181, g_moonMedTex, RGBA(64, 120, 210, 255), "Lv7 [200]");   // moon
        ResetFont();
    }
}

} // namespace

void MediaRoomInit() { Init(); }
void MediaRoomDraw(double openSeconds) { Draw(openSeconds); }
void MediaRoomInput(const ScreenInput& in) { Input(in); }
void MediaRoomReset() { Reset(); }
