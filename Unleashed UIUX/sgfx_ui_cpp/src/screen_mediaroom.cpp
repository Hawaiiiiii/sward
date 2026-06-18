// =============================================================================
// screen_mediaroom.cpp — the screenshot-evidence review queue. A VRT-style diff
// review: a left panel scrolling-list of screenshot pairs (filename + colour-coded
// classification tag + changed-pixel %), and a right panel showing the focused
// pair in detail — two side-by-side BASELINE/CANDIDATE image slots (host-supplied
// imagery, drawn as empty bordered rects), the metrics, and a colour-coded verdict.
// Built from primitives + text only (no chrome art, no third-party atlas), matching
// screen_status.cpp / screen_town.cpp. Up/Down move the focused pair. Pair data is
// representative (real battery filenames + classification vocabulary) until a live
// feed supplies the run.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include "sgfx_data.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

using namespace ui;

namespace {

// ---- screenshot-pair model --------------------------------------------------
// classification is one of the real preflight VRT classes:
//   needs_review, dimension_mismatch, missing_candidate, missing_baseline,
//   near_identical, unchanged.
struct Pair {
    const char* file;
    const char* cls;
    float       diff;        // changed-pixel ratio, 0..1 (-1 = not applicable: a missing side)
    const char* baseSize;    // baseline dimensions ("—" when absent)
    const char* candSize;    // candidate dimensions ("—" when absent)
};
const Pair PAIRS[] = {
    { "default.png",            "needs_review",       0.0412f, "1920x1080", "1920x1080" },
    { "lights_LowBeam.png",     "near_identical",     0.0009f, "1920x1080", "1920x1080" },
    { "lights_HighBeam.png",    "needs_review",       0.0731f, "1920x1080", "1920x1080" },
    { "openAllDoors_.png",      "dimension_mismatch", 0.2280f, "1920x1080", "1600x900"  },
    { "welcome_animation_.png", "missing_candidate", -1.0000f, "1920x1080", "n/a" },
    { "highlighting_Doors.png", "unchanged",          0.0000f, "1920x1080", "1920x1080" },
};
constexpr int PAIR_COUNT = int(sizeof(PAIRS) / sizeof(PAIRS[0]));

// the profile the run was captured against — from the live data bridge (or defaults)
const char* ActiveProfile() { return sgfx::Get().run.activeProfile.c_str(); }

int g_logoTex = -1, g_baseTex = -1, g_candTex = -1;
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// ---- layout (reference px) --------------------------------------------------
constexpr float RULE_Y = 118.0f;
constexpr float LIST_X = 150.0f, LIST_Y = 158.0f, LIST_W = 470.0f, LIST_H = 404.0f;
constexpr float INFO_X = 650.0f, INFO_Y = 158.0f, INFO_W = 480.0f, INFO_H = 404.0f;
constexpr float HEADER_H = 52.0f;
constexpr float ROW_H = 60.0f;
constexpr int   VISIBLE_ROWS = 5;
constexpr float ROW_PAD = 14.0f;

// ---- entrance tuning (frames @60fps) ----------------------------------------
constexpr double TITLE_FRAMES = 14.0, LIST_FRAMES = 16.0;
constexpr double INFO_OFFSET = 5.0, INFO_FRAMES = 14.0;
constexpr double FOOT_OFFSET = 10.0, FOOT_FRAMES = 12.0;
constexpr double SELECT_MOVE_FRAMES = 8.0;

// ---- palette (dark, neutral — matches status/town) --------------------------
const uint32_t COL_BG_TOP   = RGBA(12, 20, 38, 255);
const uint32_t COL_BG_BOT   = RGBA(5, 9, 18, 255);
const uint32_t COL_PANEL    = RGBA(16, 24, 40, 235);
const uint32_t COL_PANEL_CAP= RGBA(10, 16, 28, 255);
const uint32_t COL_SEL_TOP  = RGBA(64, 150, 235, 225);
const uint32_t COL_SEL_BOT  = RGBA(28, 92, 180, 225);
const uint32_t COL_TITLE    = RGBA(255, 209, 74, 255);
const uint32_t COL_TEXT     = RGBA(214, 226, 240, 255);
const uint32_t COL_TEXT_SEL = RGBA(255, 255, 255, 255);
const uint32_t COL_LABEL    = RGBA(150, 170, 196, 255);
const uint32_t COL_DESC     = RGBA(178, 194, 214, 255);
const uint32_t COL_RULE     = RGBA(120, 170, 230, 90);
const uint32_t COL_SLOT     = RGBA(24, 34, 54, 255);     // image-slot interior
const uint32_t COL_SLOT_BD  = RGBA(96, 124, 168, 200);   // image-slot border
const uint32_t COL_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t COL_WHITE    = RGBA(255, 255, 255, 255);
const uint32_t COL_CHIP     = RGBA(150, 196, 150, 255);
// classification / verdict colour code
const uint32_t COL_AMBER    = RGBA(235, 200, 90, 255);   // needs_review
const uint32_t COL_RED      = RGBA(235, 96, 84, 255);    // mismatch / missing
const uint32_t COL_GREEN    = RGBA(120, 230, 140, 255);  // near_identical / unchanged
const uint32_t COL_DIM      = RGBA(120, 138, 158, 255);  // unchanged text / N/A

// ---- interactive state ------------------------------------------------------
int    g_sel = 0, g_prevSel = 0, g_scroll = 0;
double g_moveStart = -100.0;

float ScrollLimit() { return (float)std::max(0, PAIR_COUNT - VISIBLE_ROWS); }
void ClampScrollToSel() {
    if (g_sel < g_scroll) g_scroll = g_sel;
    if (g_sel > g_scroll + VISIBLE_ROWS - 1) g_scroll = g_sel - VISIBLE_ROWS + 1;
    g_scroll = std::clamp(g_scroll, 0, (int)ScrollLimit());
}

// classification -> (tag colour, short tag label, full label)
uint32_t ClsColour(const char* cls) {
    if (std::strcmp(cls, "needs_review") == 0)       return COL_AMBER;
    if (std::strcmp(cls, "dimension_mismatch") == 0) return COL_RED;
    if (std::strcmp(cls, "missing_candidate") == 0)  return COL_RED;
    if (std::strcmp(cls, "missing_baseline") == 0)   return COL_RED;
    if (std::strcmp(cls, "near_identical") == 0)     return COL_GREEN;
    if (std::strcmp(cls, "unchanged") == 0)          return COL_DIM;
    return COL_LABEL;
}
const char* ClsTag(const char* cls) {
    if (std::strcmp(cls, "needs_review") == 0)       return "REVIEW";
    if (std::strcmp(cls, "dimension_mismatch") == 0) return "DIM";
    if (std::strcmp(cls, "missing_candidate") == 0)  return "NO CAND";
    if (std::strcmp(cls, "missing_baseline") == 0)   return "NO BASE";
    if (std::strcmp(cls, "near_identical") == 0)     return "~SAME";
    if (std::strcmp(cls, "unchanged") == 0)          return "SAME";
    return "?";
}

void Init() {
    if (g_logoTex < 0) g_logoTex = gfx::loadTexture("assets/gameart/boot_logo.png");
    if (g_baseTex < 0) g_baseTex = gfx::loadTexture("assets/gameart/review_baseline.png");
    if (g_candTex < 0) g_candTex = gfx::loadTexture("assets/gameart/review_candidate.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");
    if (g_fDF     == 0) g_fDF     = LoadMsdfFont("dfsogei");
}
void Reset() {
    g_sel = 0; g_prevSel = 0; g_scroll = 0; g_moveStart = -100.0;
}
void Input(const ScreenInput& in) {
    if (in.up || in.down) {
        g_prevSel = g_sel;
        if (in.up)   g_sel = std::max(0, g_sel - 1);
        if (in.down) g_sel = std::min(PAIR_COUNT - 1, g_sel + 1);
        ClampScrollToSel();
        g_moveStart = Now();
    }
}

// a neutral dark panel with a caption strip + rule.
void DrawPanel(float x, float y, float w, float h, float t, const char* caption) {
    DrawRect({ x, y }, { x + w, y + h }, WithAlpha(COL_PANEL, t));
    DrawRect({ x, y }, { x + w, y + HEADER_H }, WithAlpha(COL_PANEL_CAP, t));
    DrawRect({ x + 12, y + HEADER_H - 2 }, { x + w - 12, y + HEADER_H }, WithAlpha(COL_RULE, t));
    SetFont(g_fDF);
    DrawTextAligned({ x + 18, y }, { x + w - 14, y + HEADER_H }, 26.0f,
                    WithAlpha(COL_TITLE, t), caption, Align::Left, true, true);
    ResetFont();
}

void DrawLogoSlot(float t) {
    if (g_logoTex < 0 || t <= 0.0f) return;
    const float w = 168.0f, h = w * 200.0f / 600.0f;
    DrawImage(g_logoTex, { 40, 40 }, { 40 + w, 40 + h }, { 0, 0 }, { 1, 1 }, WithAlpha(COL_WHITE, t));
}

// a small colour-coded classification tag pill, right-anchored at rx.
void DrawClsTag(float rx, float cy, const char* cls, float t) {
    const char* tag = ClsTag(cls);
    uint32_t col = ClsColour(cls);
    SetFont(g_fRodin);
    float tw = MeasureText(15.0f, tag).x;
    float padX = 8.0f, h = 22.0f;
    float x1 = rx, x0 = rx - tw - padX * 2.0f;
    DrawRect({ x0, cy - h * 0.5f }, { x1, cy + h * 0.5f }, WithAlpha(RGBA(0, 0, 0, 90), t));
    DrawRect({ x0, cy - h * 0.5f }, { x0 + 3.0f, cy + h * 0.5f }, WithAlpha(col, t));   // accent
    DrawTextAligned({ x0 + padX, cy - h * 0.5f }, { x1 - padX + 2.0f, cy + h * 0.5f }, 15.0f,
                    WithAlpha(col, t), tag, Align::Right, true, true);
    ResetFont();
}

// "4.12%" or "—" for a not-applicable (missing) side.
void DiffString(const Pair& p, char* out, size_t n) {
    if (p.diff < 0.0f) std::snprintf(out, n, "n/a");
    else               std::snprintf(out, n, "%.2f%%", p.diff * 100.0f);
}

// a labelled empty image slot (host-supplied imagery drops in like the logo slot;
// if the texture id is < 0 the bordered slot stays empty — no placeholder text).
void DrawImageSlot(float x, float y, float w, float h, const char* label, int tex, float t) {
    DrawRect({ x, y }, { x + w, y + h }, WithAlpha(COL_SLOT, t));
    DrawRect({ x, y }, { x + w, y + 2 }, WithAlpha(COL_SLOT_BD, t));
    DrawRect({ x, y + h - 2 }, { x + w, y + h }, WithAlpha(COL_SLOT_BD, t));
    DrawRect({ x, y }, { x + 2, y + h }, WithAlpha(COL_SLOT_BD, t));
    DrawRect({ x + w - 2, y }, { x + w, y + h }, WithAlpha(COL_SLOT_BD, t));
    if (tex >= 0)
        DrawImage(tex, { x + 2, y + 2 }, { x + w - 2, y + h - 2 }, { 0, 0 }, { 1, 1 }, WithAlpha(COL_WHITE, t));
    SetFont(g_fRodin);
    DrawTextAligned({ x, y + h + 6 }, { x + w, y + h + 26 }, 15.0f,
                    WithAlpha(COL_LABEL, t), label, Align::Center, true, true);
    ResetFont();
}

void Draw(double openSec) {
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, COL_BG_TOP, COL_BG_BOT);

    const float titleT = (float)ComputeMotion(openSec, 0.0, TITLE_FRAMES);
    const float listT  = (float)ComputeMotion(openSec, 0.0, LIST_FRAMES);
    const float infoT  = (float)ComputeMotion(openSec, INFO_OFFSET, INFO_FRAMES);
    const float footT  = (float)ComputeMotion(openSec, FOOT_OFFSET, FOOT_FRAMES);

    // ===== HEADER: logo slot (left) + active-profile chip (right) =============
    DrawLogoSlot(titleT);
    {
        SetFont(g_fRodin);
        float cx = 1130.0f;
        DrawTextAligned({ cx - 220, 52 }, { cx, 78 }, 16.0f, WithAlpha(COL_CHIP, titleT),
                        "PROFILE", Align::Right, true, true);
        DrawTextAligned({ cx - 220, 74 }, { cx, 104 }, 24.0f, WithAlpha(COL_TITLE, titleT),
                        ActiveProfile(), Align::Right, true, true);
        ResetFont();
    }
    DrawRect({ LIST_X, RULE_Y }, { 1130.0f, RULE_Y + 2.0f }, WithAlpha(COL_RULE, titleT));

    // ===== LEFT: SCREENSHOT-PAIR LIST ========================================
    const float lx = LIST_X, ly = LIST_Y + (1.0f - listT) * 24.0f;
    DrawPanel(lx, ly, LIST_W, LIST_H, listT, "SCREENSHOTS");
    const float rowsTop = ly + HEADER_H + 10.0f;
    const float rowL = lx + ROW_PAD, rowR = lx + LIST_W - ROW_PAD;

    if (listT > 0.5f) {
        float moveT = (float)ComputeMotion(g_moveStart, 0.0, SELECT_MOVE_FRAMES);
        float prevSlot = std::clamp((float)(g_prevSel - g_scroll), 0.0f, (float)(VISIBLE_ROWS - 1));
        float curSlot  = std::clamp((float)(g_sel     - g_scroll), 0.0f, (float)(VISIBLE_ROWS - 1));
        float slot = Lerp(prevSlot, curSlot, moveT);
        float hy = rowsTop + slot * ROW_H;
        DrawVGradient({ rowL, hy + 3 }, { rowR, hy + ROW_H - 5 },
                      WithAlpha(COL_SEL_TOP, listT), WithAlpha(COL_SEL_BOT, listT));
    }

    for (int row = 0; row < VISIBLE_ROWS; ++row) {
        int idx = g_scroll + row;
        if (idx >= PAIR_COUNT) break;
        const Pair& p = PAIRS[idx];
        float top = rowsTop + row * ROW_H;
        bool selected = (idx == g_sel);
        // filename (upper line)
        SetFont(g_fSeurat);
        DrawTextAligned({ rowL + 14.0f, top + 6.0f }, { rowR - 14.0f, top + 34.0f }, 22.0f,
                        WithAlpha(selected ? COL_TEXT_SEL : COL_TEXT, listT),
                        p.file, Align::Left, true, true);
        ResetFont();
        // classification tag (lower-left) + diff% (lower-right)
        float cy = top + 44.0f;
        DrawClsTag(rowL + 14.0f + 84.0f, cy, p.cls, listT);
        char db[16]; DiffString(p, db, sizeof(db));
        uint32_t dcol = (p.diff < 0.0f) ? COL_RED
                      : (p.diff >= 0.02f) ? COL_AMBER
                      : (p.diff > 0.0f)  ? COL_GREEN : COL_DIM;
        SetFont(g_fRodin);
        DrawTextAligned({ rowR - 110.0f, cy - 11.0f }, { rowR - 14.0f, cy + 11.0f }, 18.0f,
                        WithAlpha(dcol, listT), db, Align::Right, true, true);
        ResetFont();
    }

    // scrollbar (when the list overflows)
    if (PAIR_COUNT > VISIBLE_ROWS && listT > 0.5f) {
        float trackX = lx + LIST_W - 7.0f, trackTop = rowsTop, trackH = VISIBLE_ROWS * ROW_H;
        DrawRect({ trackX, trackTop }, { trackX + 3, trackTop + trackH }, WithAlpha(COL_RULE, listT));
        float thumbH = trackH * (float)VISIBLE_ROWS / (float)PAIR_COUNT;
        float denom = ScrollLimit(); if (denom < 1.0f) denom = 1.0f;
        float thumbY = trackTop + (trackH - thumbH) * ((float)g_scroll / denom);
        DrawRect({ trackX, thumbY }, { trackX + 3, thumbY + thumbH }, WithAlpha(COL_SEL_TOP, listT));
    }

    // ===== RIGHT: FOCUSED-PAIR DETAIL ========================================
    const float ix = INFO_X, iy = INFO_Y + (1.0f - infoT) * 24.0f;
    DrawPanel(ix, iy, INFO_W, INFO_H, infoT, "DIFF");
    const Pair& sel = PAIRS[std::clamp(g_sel, 0, PAIR_COUNT - 1)];

    // focused filename
    float bodyTop = iy + HEADER_H + 18.0f;
    SetFont(g_fSeurat);
    DrawTextAligned({ ix + 24.0f, bodyTop }, { ix + INFO_W - 18.0f, bodyTop + 30.0f }, 24.0f,
                    WithAlpha(COL_TEXT_SEL, infoT), sel.file, Align::Left, true, true);
    ResetFont();

    // two side-by-side BASELINE / CANDIDATE slots (empty bordered rects)
    const float slotY = bodyTop + 44.0f, slotW = (INFO_W - 24.0f * 2.0f - 18.0f) * 0.5f, slotH = 118.0f;
    const float slotXL = ix + 24.0f, slotXR = slotXL + slotW + 18.0f;
    int baseTex = (std::strcmp(sel.cls, "missing_baseline") == 0)  ? -1 : g_baseTex;
    int candTex = (std::strcmp(sel.cls, "missing_candidate") == 0) ? -1 : g_candTex;
    DrawImageSlot(slotXL, slotY, slotW, slotH, "BASELINE",  baseTex, infoT);
    DrawImageSlot(slotXR, slotY, slotW, slotH, "CANDIDATE", candTex, infoT);

    // metrics block
    const float mTop = slotY + slotH + 36.0f, mL = ix + 24.0f, mR = ix + INFO_W - 24.0f;
    auto metric = [&](float y, const char* k, const char* v, uint32_t vcol) {
        SetFont(g_fRodin);
        DrawText({ mL, y }, 17.0f, WithAlpha(COL_LABEL, infoT), k);
        DrawTextAligned({ mL, y }, { mR, y + 20.0f }, 19.0f, WithAlpha(vcol, infoT), v, Align::Right, true, true);
        ResetFont();
    };
    char db[16]; DiffString(sel, db, sizeof(db));
    metric(mTop,        "CLASSIFICATION", sel.cls,      ClsColour(sel.cls));
    metric(mTop + 26.0f, "CHANGED PIXELS", db,          (sel.diff < 0.0f) ? COL_RED
                                                       : (sel.diff >= 0.02f) ? COL_AMBER
                                                       : (sel.diff > 0.0f) ? COL_GREEN : COL_DIM);
    metric(mTop + 52.0f, "BASELINE",       sel.baseSize, COL_TEXT);
    metric(mTop + 78.0f, "CANDIDATE",      sel.candSize, COL_TEXT);

    // colour-coded verdict line
    uint32_t vcol = ClsColour(sel.cls);
    const char* verdict =
        (std::strcmp(sel.cls, "needs_review") == 0)       ? "NEEDS REVIEW"  :
        (std::strcmp(sel.cls, "dimension_mismatch") == 0) ? "MISMATCH"      :
        (std::strcmp(sel.cls, "missing_candidate") == 0)  ? "CANDIDATE MISSING" :
        (std::strcmp(sel.cls, "missing_baseline") == 0)   ? "BASELINE MISSING"  :
        (std::strcmp(sel.cls, "near_identical") == 0)     ? "LIKELY OK"     :
        (std::strcmp(sel.cls, "unchanged") == 0)          ? "UNCHANGED"     : "—";
    const float vY = iy + INFO_H - 44.0f;
    DrawRect({ mL, vY }, { mR, vY + 30.0f }, WithAlpha(RGBA(0, 0, 0, 90), infoT));
    DrawRect({ mL, vY }, { mL + 4.0f, vY + 30.0f }, WithAlpha(vcol, infoT));
    SetFont(g_fDF);
    DrawTextAligned({ mL + 14.0f, vY }, { mR - 10.0f, vY + 30.0f }, 22.0f,
                    WithAlpha(vcol, infoT), verdict, Align::Left, true, true);
    ResetFont();

    // ===== FOOTER ===========================================================
    {
        SetFont(g_fRodin);
        DrawRect({ LIST_X, 612.0f }, { 1130.0f, 614.0f }, WithAlpha(COL_RULE, footT));
        DrawText({ LIST_X, 628.0f }, 20.0f, WithAlpha(COL_FOOTER, footT), "Up/Down  Select");
        DrawText({ LIST_X + 240.0f, 628.0f }, 20.0f, WithAlpha(COL_FOOTER, footT), "Enter  Open diff");
        DrawText({ LIST_X + 510.0f, 628.0f }, 20.0f, WithAlpha(COL_FOOTER, footT), "Esc  Back");
        ResetFont();
    }
}

} // namespace

void MediaRoomInit() { Init(); }
void MediaRoomDraw(double openSeconds) { Draw(openSeconds); }
void MediaRoomInput(const ScreenInput& in) { Input(in); }
void MediaRoomReset() { Reset(); }
