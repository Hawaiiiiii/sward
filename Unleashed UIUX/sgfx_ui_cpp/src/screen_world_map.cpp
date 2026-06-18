// =============================================================================
// screen_world_map.cpp — the QA hub. A rotating planet sits at the centre with a
// node per QA area; the left panel shows live hub totals, the right panel details
// the focused area. Left/Right move between areas (the matching node lights gold);
// the planet is our own 3D sphere. Built from primitives + text only — no chrome
// art, no third-party wordmark — so it ships on its own.
//
// The values are authentic in shape and vocabulary (real profiles, packs, verdict
// terms) but representative pending the live data feed; a host process can later
// supply them (e.g. a status JSON the way the layouts feed in).
// =============================================================================
#include "sgfxui.h"
#include "globe3d.h"
#include "screen.h"
#include "sgfx_data.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0, g_logoTex = -1;

// ---- palette ----------------------------------------------------------------
const uint32_t C_BG_TOP   = RGBA(6, 10, 20, 255);
const uint32_t C_BG_BOT   = RGBA(2, 4, 9, 255);
const uint32_t C_PANEL    = RGBA(0, 0, 0, 200);
const uint32_t C_OUTER    = RGBA(0, 49, 0, 255);
const uint32_t C_INNER    = RGBA(0, 33, 0, 235);
const uint32_t C_LINE     = RGBA(0, 110, 30, 255);
const uint32_t C_RAIL     = RGBA(20, 81, 18, 255);
const uint32_t C_HEAD     = RGBA(232, 196, 64, 255);   // area name / headers (gold)
const uint32_t C_LABEL    = RGBA(150, 196, 150, 255);  // metric labels (dim green)
const uint32_t C_VALUE    = RGBA(158, 223, 66, 255);   // metric values (lime)
const uint32_t C_DESC     = RGBA(196, 232, 196, 255);  // description text
const uint32_t C_DASH     = RGBA(34, 54, 28, 255);
const uint32_t C_FOOTER   = RGBA(190, 210, 196, 255);
const uint32_t C_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t MK_IDLE    = 0xFF2EE04C;                 // green node
const uint32_t MK_SEL     = 0xFFE0B22E;                 // gold node (focused)

// ---- layout (1280x720 reference px) -----------------------------------------
constexpr float GLOBE_CX = 612, GLOBE_CY = 384, GLOBE_R = 176;
constexpr float LP_X0 = 40,  LP_Y0 = 132, LP_X1 = 300, LP_Y1 = 356;     // left totals
constexpr float RP_X0 = 856, RP_Y0 = 120, RP_X1 = 1182, RP_Y1 = 600;    // right detail
constexpr float GRID = 9.0f;

// ---- the QA areas (hub nodes) ----------------------------------------------
struct Area {
    const char* name;
    const char* desc[4];                 // up to 4 description lines
    struct { const char* label; const char* value; } metric[3];
    float lon, lat;                      // node position on the planet
};
const Area AREAS[] = {
    { "PREFLIGHT",
      { "Run the deterministic SG-side", "checks across the project:",
        "anchors, constants, carpaints,", "project sanity." },
      { { "PACKS", "4" }, { "ERRORS", "3" }, { "WARNINGS", "12" } },
      23.0f, 36.0f },
    { "DELIVERY",
      { "Track which car models are", "delivered and which still",
        "have open changelog work.", nullptr },
      { { "MODELS", "19" }, { "DELIVERED", "7" }, { "PENDING", "12" } },
      -44.0f, 30.0f },
    { "SCREENSHOTS",
      { "Compare fresh captures", "against the approved",
        "baselines and flag the diffs.", nullptr },
      { { "PAIRS", "146" }, { "REVIEW", "8" }, { "DIFFS", "5" } },
      96.0f, 22.0f },
    { "DAILY DIGEST",
      { "The morning snapshot across", "the live profiles: config,",
        "smoke and battery runs.", nullptr },
      { { "PROFILES", "3" }, { "BLOCKED", "1" }, { "FLAGGED", "4" } },
      -96.0f, 44.0f },
    { "MANUAL REVIEW",
      { "Items that still need a", "human verdict before",
        "sign-off.", nullptr },
      { { "QUEUE", "6" }, { "P0", "1" }, { "P1", "3" } },
      140.0f, 34.0f },
    { "PROFILES",
      { "Pick a car slice to work:", "G70, G65, G45 and the wider",
        "IDCevo and classic families.", nullptr },
      { { "TOTAL", "19" }, { "IDCEVO", "9" }, { "CLASSIC", "10" } },
      40.0f, -30.0f },
};
constexpr int AREA_COUNT = int(sizeof(AREAS) / sizeof(AREAS[0]));

// ---- hub totals (left column): from the live data bridge, or defaults -----------

// ---- state ------------------------------------------------------------------
int g_sel = 0;
const char* g_nav = nullptr;
double g_msgStart = -100.0;
const char* g_msg = "";

void Init() {
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
    if (g_logoTex < 0)  g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
}
void Reset() { g_sel = 0; g_nav = nullptr; g_msgStart = -100.0; g_msg = ""; }
void Input(const ScreenInput& in) {
    if (in.left)  { g_sel = (g_sel + AREA_COUNT - 1) % AREA_COUNT; }
    if (in.right) { g_sel = (g_sel + 1) % AREA_COUNT; }
    if (in.accept) {   // enter the focused area
        static const char* const TARGET[AREA_COUNT] = {
            "status",      // PREFLIGHT    -> run metrics
            "result",      // DELIVERY     -> verdict / readiness
            "mediaroom",   // SCREENSHOTS  -> evidence review
            "result_ex",   // DAILY DIGEST -> battery results
            "mediaroom",   // MANUAL REVIEW-> evidence review
            "gate",        // PROFILES     -> profile select
        };
        g_nav = TARGET[g_sel];
    }
    if (in.cancel) g_nav = "@back";
}
const char* Nav() { const char* n = g_nav; g_nav = nullptr; return n; }

// a bounded LED panel: dark fill, lit-cell grid border, inner line frame.
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

void DrawDashes(float dy, float dx0, float dx1, float t) {
    for (float x = dx0; x < dx1; x += 8.0f)
        DrawRect({ x, dy }, { x + 4.0f, dy + 1.3f }, WithAlpha(C_DASH, t));
}

// top-left logo slot (host-supplied; empty = clean field, no header text).
void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 44 }, { 40 + w, 44 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(C_WHITE, t));
}

// left column: global hub totals on an LED panel.
void DrawTotals(float t) {
    DrawPanel(LP_X0, LP_Y0, LP_X1, LP_Y1, t);
    SetFont(g_fRodin);
    const auto& totals = sgfx::Get().hubTotals;
    const int n = (int)totals.size();
    const float rowTop = LP_Y0 + 26, pitch = 50;
    for (int i = 0; i < n; ++i) {
        float y = rowTop + i * pitch;
        DrawText({ LP_X0 + 22, y }, 15.0f, WithAlpha(C_LABEL, t), totals[i].label.c_str());
        DrawTextAligned({ LP_X0 + 22, y + 16 }, { LP_X1 - 22, y + 40 }, 22.0f,
                        WithAlpha(C_VALUE, t), totals[i].value.c_str(), Align::Left, true, false);
        if (i < n - 1) DrawDashes(y + 44, LP_X0 + 20, LP_X1 - 18, t);
    }
    ResetFont();
}

// right column: the focused area's detail on an LED panel.
void DrawAreaDetail(float t) {
    const Area& a = AREAS[g_sel];
    DrawPanel(RP_X0, RP_Y0, RP_X1, RP_Y1, t);
    // header: the area name + an "n / total" index
    SetFont(g_fDF);
    DrawTextAligned({ RP_X0 + 22, RP_Y0 + 18 }, { RP_X1 - 22, RP_Y0 + 56 }, 28.0f,
                    WithAlpha(C_HEAD, t), a.name, Align::Left, true, true);
    ResetFont();
    SetFont(g_fRodin);
    char idx[16]; std::snprintf(idx, sizeof(idx), "%d / %d", g_sel + 1, AREA_COUNT);
    DrawTextAligned({ RP_X0 + 22, RP_Y0 + 22 }, { RP_X1 - 22, RP_Y0 + 50 }, 16.0f,
                    WithAlpha(C_LABEL, t), idx, Align::Right, true, true);
    DrawRect({ RP_X0 + 20, RP_Y0 + 64 }, { RP_X1 - 20, RP_Y0 + 66 }, WithAlpha(C_RAIL, t));
    // description
    SetFont(g_fSeurat);
    for (int i = 0; i < 4; ++i)
        if (a.desc[i])
            DrawText({ RP_X0 + 24, RP_Y0 + 92.0f + i * 30.0f }, 20.0f, WithAlpha(C_DESC, t), a.desc[i]);
    ResetFont();
    // metric rows (label .... value) with dashed underlines
    SetFont(g_fRodin);
    const float mTop = RP_Y0 + 250, mPitch = 56;
    for (int i = 0; i < 3; ++i) {
        float y = mTop + i * mPitch;
        DrawText({ RP_X0 + 24, y }, 18.0f, WithAlpha(C_LABEL, t), a.metric[i].label);
        DrawTextAligned({ RP_X0 + 24, y - 4 }, { RP_X1 - 26, y + 22 }, 26.0f,
                        WithAlpha(C_VALUE, t), a.metric[i].value, Align::Right, true, false);
        DrawDashes(y + 28, RP_X0 + 22, RP_X1 - 22, t);
    }
    ResetFont();
}

// the planet with one node per area; the focused area's node lights gold.
void DrawPlanet(float t) {
    GlobeMarker mk[AREA_COUNT];
    for (int i = 0; i < AREA_COUNT; ++i) {
        mk[i].lonDeg = AREAS[i].lon; mk[i].latDeg = AREAS[i].lat;
        mk[i].color  = (i == g_sel) ? MK_SEL : MK_IDLE;
        mk[i].size   = (i == g_sel) ? 9.0f : 6.5f;
    }
    DrawGlobe3D(GLOBE_CX, GLOBE_CY, GLOBE_R * (0.7f + 0.3f * t),
                (float)(Now() * 6.0), 0.55f, 0.45f, 0.7f, mk, AREA_COUNT, t);
}

void DrawFooter(float t) {
    SetFont(g_fRodin);
    const float y = 662;
    DrawRect({ 40, 648 }, { 1240, 650 }, WithAlpha(C_RAIL, t));
    DrawText({ 48, y }, 19.0f, WithAlpha(C_FOOTER, t), "< >  Select area");
    DrawText({ 300, y }, 19.0f, WithAlpha(C_FOOTER, t), "Enter  Open");
    // transient open feedback
    double age = Now() - g_msgStart;
    if (g_msgStart > 0.0 && age < 1.6) {
        float ma = std::min(1.0f, (float)((1.6 - age) / 0.4));
        char line[64]; std::snprintf(line, sizeof(line), "Opening %s", g_msg);
        DrawTextAligned({ 600, y - 2 }, { 1232, y + 24 }, 19.0f,
                        WithAlpha(C_VALUE, ma), line, Align::Right, true, false);
    }
    ResetFont();
}

void Draw(double openSec) {
    const float t      = (float)ComputeMotion(openSec, 0.0,  12.0);
    const float tPanel = (float)ComputeMotion(openSec, 14.0, 16.0);
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, C_BG_TOP, C_BG_BOT);
    // starfield
    uint32_t s = 0x2468ace1u;
    for (int i = 0; i < 120; ++i) {
        s = s*1664525u+1013904223u; float x=(float)((s>>9)%1280);
        s = s*1664525u+1013904223u; float y=(float)((s>>9)%720);
        s = s*1664525u+1013904223u; int b=50+(int)((s>>9)%160);
        DrawRect({x,y},{x+1,y+1}, WithAlpha(RGBA(b,b,b,255), t*0.7f));
    }
    DrawLogoSlot(t);
    DrawPlanet(t);
    DrawTotals(t);
    DrawAreaDetail(tPanel);
    DrawFooter(t);
}

} // namespace

void WorldMapInit() { Init(); }
void WorldMapDraw(double openSeconds) { Draw(openSeconds); }
void WorldMapInput(const ScreenInput& in) { Input(in); }
void WorldMapReset() { Reset(); }
const char* WorldMapNav() { return Nav(); }
