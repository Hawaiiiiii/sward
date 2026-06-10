// =============================================================================
// screen_balloon.cpp â€” the tutorial / hint speech-balloon overlay, re-authored as
// clean hand-written C++ in the UnleashedRecomp ui/options_menu idiom (NOT a CSD
// node dump â€” the raw balloon.json smears mat_result_comon_001 / mat_talk_comon_003
// 9-slice stretch-arms out to x=-1655..+2610). It dims the scene behind it (the
// pause-overlay technique) and floats a single bounded REAL Sonic Unleashed chrome
// balloon panel (ui::DrawGameWindow, 9-slice from mat_result_comon_001) carrying a
// few lines of hint text and an (A) OK prompt.
//
// The distinctive retail art is the REAL extracted talk atlases, each placed at a
// sane aspect-preserved rect:
//   * the balloon TAIL spike pointing down to the speaker (mat_talk_comon_001,
//     the down-V at u 0.721..0.818 / v 0.009..0.086, 512x1024),
//   * the speaker NAME wordmark SONIC / TAILS / CHIP (mat_talk_comon_002, the
//     left english column of a 3-row grid, 256x256),
//   * the Xbox A / B button glyphs (mat_comon_x360_001, top row).
//
// Interactive + stateful like the shop / pause:
//   * Up/Down OR Q/E (LB/RB) page through the hint entries -> the speaker name,
//     tail anchor and body text all change (eased cross-fade),
//   * (A) accept advances to the next hint (and dismisses on the last one),
//   * (B) "closes" the overlay (a transient flash in the standalone build),
//   * a staggered ease-in entrance: dim -> balloon panel pop -> text -> prompt,
//     plus a gently pulsing "(A) OK" call-to-action (ui::Now() sine).
//
// Art is the real extracted atlas where a distinctive element exists; the bounded
// chrome panel + ASCII text are the graceful fallback when an atlas is missing
// (-1 slot), exactly as the shop / gate / world_map fallbacks do.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <cmath>

using namespace ui;

namespace {

// ---- per-element UV sub-rect (normalized atlas coords) ----------------------
struct UV { float u0, v0, u1, v1; };

// ---- real-art atlases -------------------------------------------------------
const char* const ASSET_BASE = "assets/balloon/";
int g_talkTex  = -1;   // mat_talk_comon_001   â€” balloon shapes / tail spike   (512x1024)
int g_nameTex  = -1;   // mat_talk_comon_002   â€” SONIC / TAILS / CHIP wordmarks (256x256)
int g_glyphTex = -1;   // mat_comon_x360_001   â€” Xbox button glyphs            (512x512)

// ---- real game fonts (MSDF) + DFSoGei title SDF ------------------------------
int g_fSeurat = 0, g_fRodin = 0, g_fDF = 0;

// ---- atlas pixel sizes (for aspect-correct fitting) -------------------------
constexpr float TALK_TEX_W  = 512.0f,  TALK_TEX_H  = 1024.0f;
constexpr float NAME_TEX_W  = 256.0f,  NAME_TEX_H  = 256.0f;
constexpr float GLYPH_TEX_W = 512.0f,  GLYPH_TEX_H = 512.0f;

// balloon TAIL spike â€” the down-pointing V measured from mat_talk_comon_001
// (px (369,9)-(419,88) -> normalized; aspect ~0.63, taller than wide).
const UV TAIL_UV = { 0.7207f, 0.00879f, 0.81836f, 0.08594f };

// speaker NAME wordmarks â€” the LEFT english column of mat_talk_comon_002, measured
// per-row tight bbox (the right column is Japanese katakana we ignore).
//   SONIC : px x 1..95   y  1..30
//   TAILS : px x 1..81   y 33..62
//   CHIP  : px x 2..70   y 65..94
const UV NAME_UV[3] = {
    /* SONIC */ { 0.00391f, 0.00391f, 0.37109f, 0.11719f },
    /* TAILS */ { 0.00391f, 0.12891f, 0.31641f, 0.24219f },
    /* CHIP  */ { 0.00781f, 0.25391f, 0.27344f, 0.36719f },
};
const char* const NAME_ASCII[3] = { "SONIC", "TAILS", "CHIP" };

// Xbox button glyphs (top row of mat_comon_x360_001).
const UV GLYPH_A = { 0.0000f, 0.0020f, 0.0684f, 0.0762f };
const UV GLYPH_B = { 0.0801f, 0.0020f, 0.1484f, 0.0762f };

// ---- data model -------------------------------------------------------------
// One HINT = a speaker (index into NAME_UV) and up to three lines of body text.
struct Hint {
    int         speaker;     // 0=SONIC 1=TAILS 2=CHIP
    const char* line1;
    const char* line2;
    const char* line3;       // may be "" (skipped)
};

const Hint HINTS[] = {
    { 2, "Hold the boost button to dash forward",
         "at high speed! Watch your boost gauge",
         "so you don't run out at the wrong time." },
    { 0, "Press jump in mid-air to perform a",
         "Homing Attack on a nearby enemy.",
         "" },
    { 1, "Collect rings to keep yourself safe.",
         "Losing all your rings when hit means",
         "you'll have to start over!" },
    { 2, "Find the Sun and Moon Medals hidden",
         "in each stage to unlock new areas",
         "on the World Map." },
    { 0, "Use the drift to take sharp corners",
         "without losing your speed.",
         "" },
};
constexpr int HINT_COUNT = int(sizeof(HINTS) / sizeof(HINTS[0]));

// ---- layout (reference px) â€” a centred floating balloon ----------------------
constexpr float PANEL_W = 720.0f, PANEL_H = 250.0f;
constexpr float PANEL_X = (REF_W - PANEL_W) * 0.5f;          // 280
constexpr float PANEL_Y = (REF_H - PANEL_H) * 0.5f - 18.0f;  // ~206
constexpr float HEADER_H = 56.0f;                            // name-plate strip

constexpr float TEXT_INDENT = 40.0f;     // body-text left inset inside the panel
constexpr float LINE_H      = 40.0f;     // hint line spacing

// the balloon tail spike hangs off the panel's lower-left, pointing at the speaker
constexpr float TAIL_H = 56.0f;          // on-screen tail height (px)
constexpr float TAIL_X = PANEL_X + 96.0f;

// ---- entrance tuning (frames @60fps), tuned to pause/shop/gate ---------------
constexpr double DIM_FRAMES    = 12.0;   // background dim-in
constexpr double PANEL_OFFSET  = 4.0,  PANEL_FRAMES = 16.0;   // balloon pop-in
constexpr double NAME_OFFSET   = 8.0,  NAME_FRAMES  = 12.0;
constexpr double TEXT_OFFSET   = 10.0, TEXT_FRAMES  = 12.0;
constexpr double TEXT_STAGGER  = 3.0;    // each line a few frames after the last
constexpr double PROMPT_OFFSET = 14.0, PROMPT_FRAMES = 12.0;
constexpr double PAGE_FRAMES   = 10.0;   // hint cross-fade/slide on page change

// ---- palette (shared with pause/result/shop/status/gate/world_map) ----------
const uint32_t COL_DIM      = RGBA(0, 0, 0, 150);
const uint32_t COL_TITLE    = RGBA(255, 209, 74, 255);    // Unleashed gold (speaker name fallback)
const uint32_t COL_TEXT     = RGBA(236, 242, 250, 255);   // body hint text
const uint32_t COL_DESC     = RGBA(178, 194, 214, 255);
const uint32_t COL_RULE     = RGBA(120, 170, 230, 90);
const uint32_t COL_PROMPT   = RGBA(255, 236, 150, 255);   // pulsing (A) OK prompt
const uint32_t COL_OK       = RGBA(120, 230, 140, 255);
const uint32_t COL_FOOTER   = RGBA(190, 205, 225, 220);
const uint32_t COL_WHITE    = RGBA(255, 255, 255, 255);

// warm-silver balloon chrome (a touch brighter / warmer than the cold blue menus,
// to read as a friendly speech bubble); header strip is the gold tint.
const uint32_t COL_BODY_TINT = RGBA(86, 104, 140, 255);
const uint32_t COL_HEAD_TINT = RGBA(160, 120, 40, 255);   // gold-ish name plate

// ---- interactive state ------------------------------------------------------
int    g_hint       = 0;
double g_pageStart  = -100.0;   // Now() when the hint last changed
double g_closeStart = -100.0;   // Now() of the last (B) close press
double g_open       = 0.0;      // openSec captured each Draw (for the prompt pulse)

const Hint& Cur() { return HINTS[std::clamp(g_hint, 0, HINT_COUNT - 1)]; }

void Init() {
    GameFrameTex();   // lazy-load the shared chrome frame used by DrawGameWindow
    if (g_talkTex  < 0) g_talkTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_talk_comon_001.png");
    if (g_nameTex  < 0) g_nameTex  = gfx::loadTexture(std::string(ASSET_BASE) + "mat_talk_comon_002.png");
    if (g_glyphTex < 0) g_glyphTex = gfx::loadTexture(std::string(ASSET_BASE) + "mat_comon_x360_001.png");
    if (g_fSeurat == 0) g_fSeurat = LoadMsdfFont("seurat");      // real game MSDF (im_font_atlas)
    if (g_fRodin  == 0) g_fRodin  = LoadMsdfFont("rodin_db");    // real game MSDF
    if (g_fDF     == 0) g_fDF     = LoadFont("assets/fonts/dfsoge7.ttc");   // real DFSoGeiStd-W7 (titles)
}

void Reset() {
    g_hint = 0;
    g_pageStart = -100.0; g_closeStart = -100.0; g_open = 0.0;
}

void Page(int dir) {
    g_hint = (g_hint + HINT_COUNT + dir) % HINT_COUNT;
    g_pageStart = Now();
}

void Input(const ScreenInput& in) {
    if (in.up   || in.tabLeft)  Page(-1);
    if (in.down || in.tabRight) Page(+1);
    if (in.accept) Page(+1);                 // (A) OK -> advance to the next hint
    if (in.cancel) g_closeStart = Now();     // (B) close (standalone: a flash)
}

// ---- aspect-preserving image fit into a box ---------------------------------
// Draws the atlas sub-rect (uv) centred inside [bx,by,bx+bw,by+bh], scaled to fit,
// with a vertical anchor (0=top,0.5=center,1=bottom). `slide` shifts it for entrance.
void DrawFitted(int tex, const UV& uv, float texW, float texH,
                float bx, float by, float bw, float bh,
                float t, float vAnchor = 0.5f, float slide = 0.0f) {
    if (tex < 0 || t <= 0.0f) return;
    float artW = (uv.u1 - uv.u0) * texW;
    float artH = (uv.v1 - uv.v0) * texH;
    if (artW <= 0.0f || artH <= 0.0f) return;
    float scale = std::min(bw / artW, bh / artH);
    float w = artW * scale, h = artH * scale;
    float x = bx + (bw - w) * 0.5f;
    float y = by + (bh - h) * vAnchor + slide;
    DrawImage(tex, { x, y }, { x + w, y + h }, { uv.u0, uv.v0 }, { uv.u1, uv.v1 },
              WithAlpha(COL_WHITE, t));
}

// one footer hint: real button glyph + ASCII label. Returns the x past the label.
float DrawGlyphHint(float x, float cy, const UV& g, float gAspect, const char* label, float t) {
    const float gh = 28.0f;
    if (g_glyphTex >= 0) {
        float gw = gh * gAspect;
        DrawImage(g_glyphTex, { x, cy - gh * 0.5f }, { x + gw, cy + gh * 0.5f },
                  { g.u0, g.v0 }, { g.u1, g.v1 }, WithAlpha(COL_WHITE, t));
        x += gw + 8.0f;
    }
    DrawText({ x, cy - 12.0f }, 21.0f, WithAlpha(COL_FOOTER, t), label);
    x += MeasureText(21.0f, label).x + 30.0f;
    return x;
}

void Draw(double openSec) {
    g_open = openSec;

    const float dimT    = (float)ComputeMotion(openSec, 0.0, DIM_FRAMES);
    const float panelT  = (float)ComputeMotion(openSec, PANEL_OFFSET, PANEL_FRAMES);
    const float nameT   = (float)ComputeMotion(openSec, NAME_OFFSET, NAME_FRAMES);
    const float promptT = (float)ComputeMotion(openSec, PROMPT_OFFSET, PROMPT_FRAMES);
    // page-change animation: content slides up + fades a touch when you change hints
    const float pageT   = (float)ComputeMotion(g_pageStart, 0.0, PAGE_FRAMES);
    const float pgSlide = (1.0f - pageT) * 18.0f;
    const float pgFade  = 0.30f + 0.70f * pageT;

    const Hint& h = Cur();

    // ===== background dim (the pause-overlay technique) ======================
    DrawRect({ 0, 0 }, { REF_W, REF_H }, WithAlpha(COL_DIM, dimT));

    // ===== balloon panel: a short upward settle as it pops in ================
    const float panelTop = PANEL_Y + (1.0f - panelT) * 22.0f;
    const float panelBot = panelTop + PANEL_H;
    const float L = PANEL_X, R = PANEL_X + PANEL_W;

    // balloon TAIL spike under the panel, pointing down at the speaker (drawn first
    // so the panel body overlaps its top edge cleanly).
    if (panelT > 0.35f) {
        float tailAspect = ((TAIL_UV.u1 - TAIL_UV.u0) * TALK_TEX_W) /
                           ((TAIL_UV.v1 - TAIL_UV.v0) * TALK_TEX_H);
        float th = TAIL_H, tw = th * tailAspect;
        float tx = TAIL_X;
        float ty = panelBot - 8.0f;   // overlap the panel's lower edge by 8px
        DrawImage(g_talkTex, { tx, ty }, { tx + tw, ty + th },
                  { TAIL_UV.u0, TAIL_UV.v0 }, { TAIL_UV.u1, TAIL_UV.v1 },
                  WithAlpha(COL_WHITE, panelT));
    }

    // the bounded REAL chrome balloon (body + gold name-plate header strip)
    DrawGameWindow({ L, panelTop }, { R, panelBot }, HEADER_H, panelT,
                   COL_BODY_TINT, COL_HEAD_TINT);
    // header underline rule
    DrawRect({ L + 16, panelTop + HEADER_H - 2 }, { R - 16, panelTop + HEADER_H },
             WithAlpha(COL_RULE, panelT));

    // ===== speaker NAME wordmark on the gold plate (real art, left-anchored) =
    {
        const float nameBoxX = L + 26.0f;
        const float nameBoxY = panelTop + 10.0f;
        const float nameBoxW = 240.0f, nameBoxH = HEADER_H - 20.0f;
        const UV& nu = NAME_UV[std::clamp(h.speaker, 0, 2)];
        if (g_nameTex >= 0 && nameT > 0.0f) {
            DrawFitted(g_nameTex, nu, NAME_TEX_W, NAME_TEX_H,
                       nameBoxX, nameBoxY, nameBoxW, nameBoxH,
                       nameT * pgFade, 0.0f, pgSlide * 0.5f);   // left/top-anchored
        } else {
            SetFont(g_fDF);
            DrawTextShadow({ nameBoxX, nameBoxY + 4.0f }, 30.0f,
                           WithAlpha(COL_TITLE, nameT), NAME_ASCII[std::clamp(h.speaker, 0, 2)]);
        }
        // "HINT" tag + page index on the plate's right
        char idx[24];
        std::snprintf(idx, sizeof(idx), "HINT  %d / %d", g_hint + 1, HINT_COUNT);
        SetFont(g_fRodin);
        DrawTextAligned({ R - 240, panelTop }, { R - 18, panelTop + HEADER_H }, 22.0f,
                        WithAlpha(COL_FOOTER, nameT), idx, Align::Right, true, true);
    }

    // ===== body hint text (staggered fade-in, slides on page change) =========
    {
        const float bx = L + TEXT_INDENT;
        float by = panelTop + HEADER_H + 24.0f;
        const float lineW = PANEL_W - TEXT_INDENT * 2.0f;
        const char* const lines[3] = { h.line1, h.line2, h.line3 };
        SetFont(g_fSeurat);
        for (int i = 0; i < 3; ++i) {
            if (!lines[i] || lines[i][0] == '\0') continue;
            float lineT = (float)ComputeMotion(openSec, TEXT_OFFSET + i * TEXT_STAGGER, TEXT_FRAMES);
            DrawTextAligned({ bx, by + i * LINE_H + pgSlide },
                            { bx + lineW, by + i * LINE_H + LINE_H + pgSlide },
                            27.0f, WithAlpha(COL_TEXT, lineT * pgFade),
                            lines[i], Align::Left, true, true);
        }
    }

    // ===== pulsing "(A) OK" prompt at the balloon's lower-right ==============
    if (panelT > 0.5f) {
        float pulse = 0.60f + 0.40f * (float)(0.5 + 0.5 * std::sin(g_open * 4.5));
        float py = panelBot - 44.0f;
        // glyph + label, right-aligned by laying it out from a measured start x
        const float aAsp = 0.921f;
        const char* okLabel = "OK";
        float gh = 28.0f, gw = gh * aAsp;
        SetFont(g_fRodin);
        float labelW = MeasureText(23.0f, okLabel).x;
        float blockW = gw + 8.0f + labelW;
        float startX = R - 28.0f - blockW;
        float cy = py + 14.0f;
        float a = promptT * pulse;
        if (g_glyphTex >= 0) {
            DrawImage(g_glyphTex, { startX, cy - gh * 0.5f }, { startX + gw, cy + gh * 0.5f },
                      { GLYPH_A.u0, GLYPH_A.v0 }, { GLYPH_A.u1, GLYPH_A.v1 },
                      WithAlpha(COL_WHITE, a));
        } else {
            DrawText({ startX, cy - 13.0f }, 23.0f, WithAlpha(COL_PROMPT, a), "(A)");
        }
        DrawText({ startX + gw + 8.0f, cy - 13.0f }, 23.0f, WithAlpha(COL_PROMPT, a), okLabel);
    }

    // ===== footer guide (below the balloon) â€” real glyphs + ASCII labels =====
    if (promptT > 0.0f) {
        float hx = PANEL_X + 8.0f, hcy = panelBot + 36.0f;
        const float aAsp = 0.921f;
        SetFont(g_fRodin);
        hx = DrawGlyphHint(hx, hcy, GLYPH_A, aAsp, "Next",  promptT);
        hx = DrawGlyphHint(hx, hcy, GLYPH_B, aAsp, "Close", promptT);
        DrawText({ hx, hcy - 12.0f }, 21.0f, WithAlpha(COL_FOOTER, promptT),
                 "[Up/Down] Browse Hints");
    }

    // ===== transient "close" flash ===========================================
    double age = Now() - g_closeStart;
    if (g_closeStart > 0.0 && age < 1.2) {
        float ma = std::min(1.0f, (float)((1.2 - age) / 0.35));
        SetFont(g_fRodin);
        DrawTextAligned({ PANEL_X, panelBot + 64.0f }, { PANEL_X + PANEL_W, panelBot + 92.0f },
                        22.0f, WithAlpha(COL_OK, ma), "Closing hint...", Align::Center, true, true);
    }

    ResetFont();
}

} // namespace

// ---- exposed to the registry ------------------------------------------------
void BalloonInit() { Init(); }
void BalloonDraw(double openSeconds) { Draw(openSeconds); }
void BalloonInput(const ScreenInput& in) { Input(in); }
void BalloonReset() { Reset(); }