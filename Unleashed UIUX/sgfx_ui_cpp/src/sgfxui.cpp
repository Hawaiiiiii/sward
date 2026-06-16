// =============================================================================
// sgfxui.cpp — implementation of the clean immediate-mode UI layer.
// Bakes a glyph atlas with stb_truetype, exposes ease-out motion matching the
// game's imgui_utils, and turns every primitive into gfx::Quad batches for the
// CSD backend. No Dear ImGui dependency.
// =============================================================================
#include "sgfxui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace ui {
namespace {

std::vector<gfx::Quad> g_quads;
double g_now = 0.0;
uint32_t g_modifier = 0;   // current ShaderModifier applied to pushed quads

// --- SDF font atlas (signed-distance fields baked with stb_truetype) ---
// Single-channel SDF per glyph, shelf-packed; the modifier pixel shader converts
// distance -> antialiased alpha (MOD_SDF_TEXT) or a beveled gold title (MOD_TITLE_BEVEL).
constexpr int   ATLAS_W   = 1024;
constexpr int   ATLAS_H   = 1024;
constexpr float BAKE_PX   = 72.0f;       // SDF bake height (drawn scaled); crisp at any size
constexpr int   FIRST_CH  = 32;
constexpr int   NUM_CH    = 95;          // printable ASCII 32..126
constexpr int   SDF_PAD   = 6;           // SDF spread in atlas texels (must match shader SDF_PXRANGE=12)
struct Glyph { int x0, y0, x1, y1; float xoff, yoff, advance; bool has; };
// MSDF glyph (the recomp's real im_font_atlas): metrics in baked-px (FontSize=24),
// quad top-relative (Y0/Y1 already include the -y+Ascent transform), UVs V-flipped.
struct MGlyph { float adv, x0, y0, x1, y1, u0, v0, u1, v1; };
// Multiple SDF fonts: font 0 = the default (DFHeiStd, baked in Init); extra fonts via
// LoadFont() — e.g. FOT-SeuratPro-M / FOT-NewRodinPro-DB for the recomp-exact options
// text. SetFont(idx) selects the current font (state, mirroring SetModifier).
struct Font {
    Glyph glyphs[NUM_CH] = {}; int tex = -1; float ascentPx = BAKE_PX * 0.8f; bool ready = false;
    // ---- MSDF variant (the recomp's exact im_font_atlas) ----
    bool  msdf = false;                            // when true, render via mglyphs + MOD_MSDF_TEXT
    std::unordered_map<int, MGlyph> mglyphs;       // codepoint -> glyph
    float mSize = 24.0f, mAscent = 21.12f;         // baked font size / ascent (px)
};
constexpr int MAX_FONTS = 8;
Font g_fonts[MAX_FONTS];
int  g_fontCount = 0;
int  g_curFont   = 0;
int  g_msdfAtlasTex = -1;      // shared MSDF atlas (loaded once by LoadMsdfFont)
bool g_fontReady = false;      // mirrors g_fonts[0].ready (back-compat guard)

inline uint8_t A(uint32_t c) { return uint8_t(c >> 24); }
inline uint8_t R(uint32_t c) { return uint8_t(c >> 16); }
inline uint8_t G(uint32_t c) { return uint8_t(c >> 8);  }
inline uint8_t B(uint32_t c) { return uint8_t(c);       }

// ---- clip-rect stack (scissor): CPU-clamp axis-aligned quads to the current clip
// rect + remap UVs (crop, don't squash); per-glyph for text. The recomp's PushClipRect.
struct ClipRect { float x0, y0, x1, y1; };
std::vector<ClipRect> g_clipStack;

// Global alpha multiplier stack — lets a screen fade/stagger an entrance without
// threading an alpha through every draw call (the recomp's PushAlphaModifier).
// Every emitted quad/glyph colour's alpha is scaled by the product of the stack.
std::vector<float> g_alphaStack;
float g_alphaMul = 1.0f;
inline void recomputeAlpha() { g_alphaMul = 1.0f; for (float a : g_alphaStack) g_alphaMul *= a; }
inline uint32_t applyGAlpha(uint32_t c) {
    if (g_alphaMul >= 0.999f) return c;
    uint32_t a = (uint32_t)((c >> 24) * g_alphaMul + 0.5f);
    return (c & 0x00FFFFFFu) | (a << 24);
}

// Global single-level transform — scale about a pivot, then translate. Drives
// inflate/pop (XScale/YScale from 0) and slide-in entrances. Applied to quad
// coordinates BEFORE clipping. Single-level: PushTransform sets it, PopTransform
// clears to identity (no true nesting — entrance uses are one transform at a time).
bool  g_xfActive = false;
float g_xfSx = 1, g_xfSy = 1, g_xfPx = 0, g_xfPy = 0, g_xfTx = 0, g_xfTy = 0;
inline void xf(float& x, float& y) {
    if (!g_xfActive) return;
    x = g_xfPx + (x - g_xfPx) * g_xfSx + g_xfTx;
    y = g_xfPy + (y - g_xfPy) * g_xfSy + g_xfTy;
}
// clamp [x0..y1]/[u0..v1] to the top clip rect; returns false if fully outside.
bool clipQuad(float& x0, float& y0, float& x1, float& y1, float& u0, float& v0, float& u1, float& v1) {
    if (g_clipStack.empty()) return true;
    const ClipRect& c = g_clipStack.back();
    if (x1 <= c.x0 || x0 >= c.x1 || y1 <= c.y0 || y0 >= c.y1) return false;
    const float oW = x1 - x0, oH = y1 - y0, du = u1 - u0, dv = v1 - v0;
    if (oW > 0.0f) { if (x0 < c.x0) { u0 += du * (c.x0 - x0) / oW; x0 = c.x0; }
                     if (x1 > c.x1) { u1 -= du * (x1 - c.x1) / oW; x1 = c.x1; } }
    if (oH > 0.0f) { if (y0 < c.y0) { v0 += dv * (c.y0 - y0) / oH; y0 = c.y0; }
                     if (y1 > c.y1) { v1 -= dv * (y1 - c.y1) / oH; y1 = c.y1; } }
    return true;
}

// Build one axis-aligned quad (TL,TR,BR,BL) and push it (clipped to the current clip rect).
void Push(float x0, float y0, float x1, float y1,
          float u0, float v0, float u1, float v1,
          uint32_t col, int tex, gfx::Blend blend) {
    xf(x0, y0); xf(x1, y1);
    if (!clipQuad(x0, y0, x1, y1, u0, v0, u1, v1)) return;
    col = applyGAlpha(col);
    gfx::Quad q;
    q.px[0]=x0; q.py[0]=y0;  q.px[1]=x1; q.py[1]=y0;
    q.px[2]=x1; q.py[2]=y1;  q.px[3]=x0; q.py[3]=y1;
    q.u[0]=u0; q.v[0]=v0;  q.u[1]=u1; q.v[1]=v0;
    q.u[2]=u1; q.v[2]=v1;  q.u[3]=u0; q.v[3]=v1;
    q.color[0]=q.color[1]=q.color[2]=q.color[3]=col; q.texIndex = tex; q.blend = blend; q.modifier = g_modifier;
    g_quads.push_back(q);
}

// 4-corner gradient quad (TL,TR,BR,BL) — the AddRectFilledMultiColor primitive.
void PushQuad4(float x0, float y0, float x1, float y1,
               uint32_t cTL, uint32_t cTR, uint32_t cBR, uint32_t cBL, int tex, gfx::Blend blend) {
    xf(x0, y0); xf(x1, y1);
    gfx::Quad q;
    q.px[0]=x0; q.py[0]=y0;  q.px[1]=x1; q.py[1]=y0;
    q.px[2]=x1; q.py[2]=y1;  q.px[3]=x0; q.py[3]=y1;
    q.u[0]=0; q.v[0]=0;  q.u[1]=1; q.v[1]=0;  q.u[2]=1; q.v[2]=1;  q.u[3]=0; q.v[3]=1;
    q.color[0]=applyGAlpha(cTL); q.color[1]=applyGAlpha(cTR); q.color[2]=applyGAlpha(cBR); q.color[3]=applyGAlpha(cBL);
    q.texIndex=tex; q.blend=blend; q.modifier=g_modifier;
    g_quads.push_back(q);
}

// ---- italic text shear (the game's chrome-wordmark lean, e.g. PAUSE) -------
float g_textShear    = 0.0f;    // +x per px ABOVE the baseline; 0 = upright
float g_textStretchX = 1.0f;    // horizontal stretch about the string's pen start
// shear the quad pushed since `before` about `baseY` (shear moves x only, so it
// composes with the CPU clip + the gradient tint, which read y).
inline void shearLast(size_t before, float baseY) {
    if (g_textShear == 0.0f || g_quads.size() <= before) return;
    gfx::Quad& q = g_quads.back();
    for (int k = 0; k < 4; ++k) q.px[k] += g_textShear * (baseY - q.py[k]);
}
// stretch the quad pushed since `before` about startX — scales glyphs AND gaps
// uniformly (a whole-string horizontal scale). Applied BEFORE the shear so the
// configured lean angle is preserved.
inline void stretchLast(size_t before, float startX) {
    if (g_textStretchX == 1.0f || g_quads.size() <= before) return;
    gfx::Quad& q = g_quads.back();
    for (int k = 0; k < 4; ++k) q.px[k] = startX + (q.px[k] - startX) * g_textStretchX;
}

} // namespace

uint32_t WithAlpha(uint32_t col, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return (uint32_t(A(col) * t) << 24) | (col & 0x00FFFFFFu);
}

static bool bakeFont(Font& F, const char* fontTtfPath) {
    // load TTF/OTF
    std::vector<unsigned char> ttf;
    if (FILE* fp = fopen(fontTtfPath, "rb")) {
        fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET);
        if (n > 0) { ttf.resize((size_t)n); if (fread(ttf.data(), 1, (size_t)n, fp) != (size_t)n) ttf.clear(); }
        fclose(fp);
    }
    if (ttf.empty()) {
        fprintf(stderr, "[sgfxui] could not read font '%s' — text disabled\n", fontTtfPath);
        return false;
    }
    stbtt_fontinfo fi;
    if (!stbtt_InitFont(&fi, ttf.data(), stbtt_GetFontOffsetForIndex(ttf.data(), 0))) {
        fprintf(stderr, "[sgfxui] InitFont failed\n"); return false;
    }
    const float scale = stbtt_ScaleForPixelHeight(&fi, BAKE_PX);
    int asc, desc, lgap; stbtt_GetFontVMetrics(&fi, &asc, &desc, &lgap);
    F.ascentPx = asc * scale;

    // Signed distance field from the (CFF-safe) alpha rasterization via a brute-force
    // distance transform. stbtt_GetCodepointSDF is unreliable on this OTF/CFF font
    // (round glyphs break), but stbtt_GetCodepointBitmap rasterizes every glyph fine.
    auto sdfFromCoverage = [](const std::vector<unsigned char>& cov, int W, int H,
                              std::vector<unsigned char>& out) {
        out.assign((size_t)W * H, 0);
        const int R = SDF_PAD;
        for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
            bool inside = cov[(size_t)y * W + x] >= 128;
            int best = R * R + 1;
            for (int dy = -R; dy <= R; ++dy) { int yy = y + dy; if (yy < 0 || yy >= H) continue;
                for (int dx = -R; dx <= R; ++dx) { int xx = x + dx; if (xx < 0 || xx >= W) continue;
                    if ((cov[(size_t)yy * W + xx] >= 128) != inside) { int d2 = dx*dx + dy*dy; if (d2 < best) best = d2; } } }
            float dist = std::sqrt((float)best); if (dist > (float)R) dist = (float)R;
            int v = (int)(128.0f + (inside ? dist : -dist) * (127.0f / R) + 0.5f);
            out[(size_t)y * W + x] = (unsigned char)std::clamp(v, 0, 255);
        }
    };

    std::vector<unsigned char> atlas((size_t)ATLAS_W * ATLAS_H, 0);
    std::vector<unsigned char> cov, sdf;
    int penx = 1, peny = 1, rowH = 0, packed = 0;
    for (int c = FIRST_CH; c < FIRST_CH + NUM_CH; ++c) {
        Glyph& G = F.glyphs[c - FIRST_CH];
        int adv = 0, lsb = 0; stbtt_GetCodepointHMetrics(&fi, c, &adv, &lsb);
        G.advance = adv * scale; G.has = false;
        int bw = 0, bh = 0, bx = 0, by = 0;
        unsigned char* bmp = stbtt_GetCodepointBitmap(&fi, scale, scale, c, &bw, &bh, &bx, &by);
        if (bmp && bw > 0 && bh > 0) {
            const int W = bw + 2 * SDF_PAD, H = bh + 2 * SDF_PAD;
            cov.assign((size_t)W * H, 0);
            for (int y = 0; y < bh; ++y) for (int x = 0; x < bw; ++x)
                cov[(size_t)(y + SDF_PAD) * W + (x + SDF_PAD)] = bmp[y * bw + x];
            sdfFromCoverage(cov, W, H, sdf);
            if (penx + W + 1 > ATLAS_W) { penx = 1; peny += rowH + 1; rowH = 0; }
            if (peny + H + 1 <= ATLAS_H) {
                for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x)
                    atlas[(size_t)(peny + y) * ATLAS_W + (penx + x)] = sdf[(size_t)y * W + x];
                G.x0 = penx; G.y0 = peny; G.x1 = penx + W; G.y1 = peny + H;
                G.xoff = (float)(bx - SDF_PAD); G.yoff = (float)(by - SDF_PAD); G.has = true;
                penx += W + 1; if (H > rowH) rowH = H; ++packed;
            }
        }
        if (bmp) stbtt_FreeBitmap(bmp, nullptr);
    }
    if (packed == 0) { fprintf(stderr, "[sgfxui] SDF bake produced no glyphs\n"); return false; }

    // upload: distance in RGB (shader reads .r), opaque alpha
    std::vector<unsigned char> rgba((size_t)ATLAS_W * ATLAS_H * 4);
    for (size_t i = 0; i < atlas.size(); ++i) {
        unsigned char d = atlas[i];
        rgba[i*4+0] = d; rgba[i*4+1] = d; rgba[i*4+2] = d; rgba[i*4+3] = 255;
    }
    F.tex = gfx::loadTextureRGBA(rgba.data(), ATLAS_W, ATLAS_H);
    F.ready = (F.tex >= 0);
    if (!F.ready) fprintf(stderr, "[sgfxui] font atlas upload failed\n");
    else fprintf(stderr, "[sgfxui] SDF font baked '%s' (%d/%d glyphs)\n", fontTtfPath, packed, NUM_CH);
    return F.ready;
}

bool Init(const char* fontTtfPath) {
    g_quads.reserve(4096);
    bakeFont(g_fonts[0], fontTtfPath);   // font 0 = default
    g_fontCount = 1;
    g_fontReady = g_fonts[0].ready;
    return g_fontReady;
}
// Load one of the recomp's real MSDF fonts ("seurat","rodin_db","rodin_ub","rodin_m")
// from the parsed im_font_atlas (assets/fonts/msdf_atlas.png + msdf_glyphs.json). Returns
// a font index for SetFont(); 0 (the default SDF font) on any failure.
int LoadMsdfFont(const char* shortName) {
    if (g_fontCount >= MAX_FONTS || !shortName) return 0;
    if (g_msdfAtlasTex < 0) g_msdfAtlasTex = gfx::loadTexture("assets/fonts/msdf_atlas.png");
    if (g_msdfAtlasTex < 0) { fprintf(stderr, "[sgfxui] MSDF atlas load failed\n"); return 0; }
    static nlohmann::json* doc = nullptr;        // parse the glyph-table JSON once, cache it
    if (!doc) {
        FILE* fp = fopen("assets/fonts/msdf_glyphs.json", "rb");
        if (!fp) { fprintf(stderr, "[sgfxui] msdf_glyphs.json not found\n"); return 0; }
        fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET);
        std::string s((size_t)(n > 0 ? n : 0), '\0');
        if (n > 0) { size_t rd = fread(&s[0], 1, (size_t)n, fp); s.resize(rd); }
        fclose(fp);
        doc = new nlohmann::json(nlohmann::json::parse(s, nullptr, false));
        if (doc->is_discarded()) { fprintf(stderr, "[sgfxui] msdf_glyphs.json parse error\n"); delete doc; doc = nullptr; return 0; }
    }
    if (!doc->contains(shortName)) { fprintf(stderr, "[sgfxui] MSDF font '%s' missing\n", shortName); return 0; }
    const auto& jf = (*doc)[shortName];
    Font& F = g_fonts[g_fontCount];
    F = Font{};
    F.msdf = true; F.tex = g_msdfAtlasTex;
    F.mSize   = jf.value("size",   24.0f);
    F.mAscent = jf.value("ascent", 21.12f);
    const auto& jg = jf["glyphs"];
    for (auto it = jg.begin(); it != jg.end(); ++it) {
        const auto& a = it.value();              // [adv,x0,y0,x1,y1,u0,v0,u1,v1]
        if (a.size() < 9) continue;
        F.mglyphs[atoi(it.key().c_str())] = MGlyph{
            a[0].get<float>(), a[1].get<float>(), a[2].get<float>(), a[3].get<float>(),
            a[4].get<float>(), a[5].get<float>(), a[6].get<float>(), a[7].get<float>(), a[8].get<float>() };
    }
    F.ready = !F.mglyphs.empty();
    if (!F.ready) return 0;
    fprintf(stderr, "[sgfxui] MSDF font '%s' loaded (%zu glyphs)\n", shortName, F.mglyphs.size());
    return g_fontCount++;
}
int LoadFont(const char* path) {
    if (g_fontCount >= MAX_FONTS) return 0;
    if (!bakeFont(g_fonts[g_fontCount], path)) { g_fonts[g_fontCount] = Font{}; return 0; }
    return g_fontCount++;
}
void SetFont(int f) { g_curFont = (f >= 0 && f < g_fontCount && g_fonts[f].ready) ? f : 0; }
void ResetFont()    { g_curFont = 0; }

void Shutdown() { g_quads.clear(); g_quads.shrink_to_fit(); g_fontReady = false; }

void BeginFrame(double nowSeconds) { g_now = nowSeconds; g_quads.clear(); g_modifier = 0; g_curFont = 0; g_textShear = 0.0f; g_textStretchX = 1.0f; g_clipStack.clear(); g_alphaStack.clear(); g_alphaMul = 1.0f; g_xfActive = false; g_xfSx = g_xfSy = 1.0f; g_xfTx = g_xfTy = 0.0f; }
void SetModifier(uint32_t m) { g_modifier = m; }
void ResetModifier() { g_modifier = 0; }
void PushClip(V2 min, V2 max) { g_clipStack.push_back({ min.x, min.y, max.x, max.y }); }
void PopClip() { if (!g_clipStack.empty()) g_clipStack.pop_back(); }

// Global alpha multiplier (stacks). PushAlpha(0.3f) fades everything drawn until
// PopAlpha() to 30%; nested pushes multiply. Used to drive entrance fades/staggers.
void PushAlpha(float a) { a = a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a); g_alphaStack.push_back(a); g_alphaMul *= a; }
void PopAlpha() { if (!g_alphaStack.empty()) { g_alphaStack.pop_back(); recomputeAlpha(); } }

// scale about `pivot` then translate by `translate` for subsequent draws until
// PopTransform(). Single-level (no nesting). For inflate-pop / slide-in entrances.
void PushTransform(float sx, float sy, V2 pivot, V2 translate) {
    g_xfSx = sx; g_xfSy = sy; g_xfPx = pivot.x; g_xfPy = pivot.y; g_xfTx = translate.x; g_xfTy = translate.y; g_xfActive = true;
}
void PopTransform() { g_xfActive = false; g_xfSx = g_xfSy = 1.0f; g_xfTx = g_xfTy = 0.0f; }
void Flush(gfx::Quad*& outQuads, int& outCount) { outQuads = g_quads.data(); outCount = (int)g_quads.size(); }
double Now() { return g_now; }

float Scale(float referencePixels) { return referencePixels; }  // identity at 16:9

double ComputeLinearMotion(double startSec, double offsetFrames, double totalFrames) {
    if (totalFrames <= 0.0) return 1.0;
    double v = (g_now - startSec - offsetFrames / 60.0) / totalFrames * 60.0;
    return std::clamp(v, 0.0, 1.0);
}
double ComputeMotion(double startSec, double offsetFrames, double totalFrames) {
    return std::sqrt(ComputeLinearMotion(startSec, offsetFrames, totalFrames));
}
float Lerp(float a, float b, float t) { return a + (b - a) * t; }
float Cubic(float a, float b, float t) { return Lerp(a, b, t * t * (3.0f - 2.0f * t)); }
float Hermite(float a, float b, float t) { float t2=t*t,t3=t2*t; return Lerp(a, b, 3*t2 - 2*t3); }
V2    Lerp(V2 a, V2 b, float t) { return { Lerp(a.x,b.x,t), Lerp(a.y,b.y,t) }; }
uint32_t ColourLerp(uint32_t a, uint32_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto L = [&](uint8_t x, uint8_t y) { return uint8_t(x + (y - x) * t); };
    return (uint32_t(L(A(a),A(b)))<<24) | (uint32_t(L(R(a),R(b)))<<16) |
           (uint32_t(L(G(a),G(b)))<<8)  |  uint32_t(L(B(a),B(b)));
}

void DrawRect(V2 min, V2 max, uint32_t col, bool additive) {
    Push(min.x, min.y, max.x, max.y, 0,0,1,1, col, -1,
         additive ? gfx::Blend::Additive : gfx::Blend::Alpha);
}

void DrawVGradient(V2 min, V2 max, uint32_t top, uint32_t bottom) {
    PushQuad4(min.x, min.y, max.x, max.y, top, top, bottom, bottom, -1, gfx::Blend::Alpha);
}
void DrawHGradient(V2 min, V2 max, uint32_t left, uint32_t right) {
    PushQuad4(min.x, min.y, max.x, max.y, left, right, right, left, -1, gfx::Blend::Alpha);
}
// full 4-corner gradient (TL,TR,BR,BL) — the AddRectFilledMultiColor primitive.
void DrawQuadGradient(V2 min, V2 max, uint32_t cTL, uint32_t cTR, uint32_t cBR, uint32_t cBL, bool additive) {
    PushQuad4(min.x, min.y, max.x, max.y, cTL, cTR, cBR, cBL, -1,
              additive ? gfx::Blend::Additive : gfx::Blend::Alpha);
}

void DrawImage(int tex, V2 min, V2 max, V2 uv0, V2 uv1, uint32_t col, bool additive) {
    Push(min.x, min.y, max.x, max.y, uv0.x, uv0.y, uv1.x, uv1.y, col, tex,
         additive ? gfx::Blend::Additive : gfx::Blend::Alpha);
}

void DrawImageQuad(int tex, const V2 corners[4], const V2 uvs[4], uint32_t col, bool additive) {
    gfx::Quad q;
    for (int k = 0; k < 4; ++k) {
        float cx = corners[k].x, cy = corners[k].y; xf(cx, cy);
        q.px[k] = cx; q.py[k] = cy;
        q.u[k]  = uvs[k].x;     q.v[k]  = uvs[k].y;
    }
    q.color[0]=q.color[1]=q.color[2]=q.color[3]=applyGAlpha(col); q.texIndex = tex; q.modifier = g_modifier;
    q.blend = additive ? gfx::Blend::Additive : gfx::Blend::Alpha;
    g_quads.push_back(q);
}

void DrawContainer(const Slice9& s, V2 min, V2 max, uint32_t col) {
    if (s.tex < 0) { DrawRect(min, max, col); return; }
    const float xs[4] = { min.x, min.x + s.l, max.x - s.r, max.x };
    const float ys[4] = { min.y, min.y + s.t, max.y - s.b, max.y };
    const float us[4] = { 0.0f, s.l / s.texW, (s.texW - s.r) / s.texW, 1.0f };
    const float vs[4] = { 0.0f, s.t / s.texH, (s.texH - s.b) / s.texH, 1.0f };
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            Push(xs[c], ys[r], xs[c+1], ys[r+1], us[c], vs[r], us[c+1], vs[r+1],
                 col, s.tex, gfx::Blend::Alpha);
}

void DrawNineSlice(const NineSlice& s, V2 min, V2 max, uint32_t col, bool additive) {
    if (s.tex < 0) { DrawRect(min, max, col, additive); return; }
    const gfx::Blend blend = additive ? gfx::Blend::Additive : gfx::Blend::Alpha;
    const float xs[4] = { min.x, min.x + s.bl, max.x - s.br, max.x };
    const float ys[4] = { min.y, min.y + s.bt, max.y - s.bb, max.y };
    const float us0[3] = { s.uL0, s.uC0, s.uR0 };
    const float us1[3] = { s.uL1, s.uC1, s.uR1 };
    const float vs0[3] = { s.vT0, s.vC0, s.vB0 };
    const float vs1[3] = { s.vT1, s.vC1, s.vB1 };
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            Push(xs[c], ys[r], xs[c+1], ys[r+1], us0[c], vs0[r], us1[c], vs1[r], col, s.tex, blend);
}

int GameFrameTex() {
    static int tex = -2;   // -2 = not yet attempted
    if (tex == -2) {
        const char* paths[] = {
            "assets/common/mat_result_comon_001.png",
            "assets/shop/mat_result_comon_001.png",
            "assets/status/mat_result_comon_001.png",
        };
        tex = -1;
        for (const char* p : paths) { int t = gfx::loadTexture(p); if (t >= 0) { tex = t; break; } }
    }
    return tex;
}

NineSlice GameWindow(int tex) {
    NineSlice s; s.tex = tex;
    // UV spans straight from the parsed CSD nine-slice of mat_result_comon_001.
    s.uL0=0.5625f;  s.uL1=0.59668f; s.uC0=0.6123f;  s.uC1=0.61719f; s.uR0=0.63086f; s.uR1=0.66504f;
    s.vT0=0.13281f; s.vT1=0.20117f; s.vC0=0.20117f; s.vC1=0.66016f; s.vB0=0.66016f; s.vB1=0.73828f;
    s.bl=16.0f; s.bt=16.0f; s.br=16.0f; s.bb=18.0f;
    return s;
}

void DrawGameWindow(V2 min, V2 max, float headerH, float alpha, uint32_t bodyTint, uint32_t headerTint) {
    NineSlice fr = GameWindow(GameFrameTex());
    if (fr.tex >= 0) {
        DrawNineSlice(fr, min, max, WithAlpha(bodyTint, alpha));
        if (headerH > 0.0f)
            DrawNineSlice(fr, min, { max.x, min.y + headerH }, WithAlpha(headerTint, alpha));
    } else {
        DrawVGradient(min, max, WithAlpha(bodyTint, alpha), WithAlpha(bodyTint, alpha));
        if (headerH > 0.0f)
            DrawVGradient(min, { max.x, min.y + headerH }, WithAlpha(headerTint, alpha), WithAlpha(headerTint, alpha));
    }
    // subtle recomp-style checkerboard texture over the panel (real GPU shader modifier)
    SetModifier(MOD_CHECKERBOARD);
    DrawRect(min, max, WithAlpha(RGBA(0, 0, 0, 255), alpha * 0.10f));
    ResetModifier();
}

// ---- recomp-style polish ----
float Breathe(double nowSec, float lo, float hi, float rateSec) {
    if (rateSec <= 0.0f) return hi;
    float s = 0.5f * (std::sin((float)(nowSec * (2.0 * 3.14159265358979 / rateSec))) + 1.0f);
    return lo + (hi - lo) * s;
}
uint32_t Grayscale(uint32_t col) {
    float l = 0.299f * R(col) + 0.587f * G(col) + 0.114f * B(col);
    uint8_t g = (uint8_t)std::clamp(l, 0.0f, 255.0f);
    return (uint32_t(A(col)) << 24) | (uint32_t(g) << 16) | (uint32_t(g) << 8) | uint32_t(g);
}
void DrawScanlines(V2 min, V2 max, uint32_t lineCol, float spacingPx, float thickness) {
    if (spacingPx < 1.0f) spacingPx = 1.0f;
    for (float y = min.y; y < max.y; y += spacingPx)
        Push(min.x, y, max.x, y + thickness, 0, 0, 1, 1, lineCol, -1, gfx::Blend::Alpha);
}
void DrawTextBevel(V2 pos, float pxSize, uint32_t col, const char* text, uint32_t loCol, uint32_t hiCol) {
    (void)loCol; (void)hiCol;   // the gold rim/shadow now come from the SDF shader (MOD_TITLE_BEVEL)
    const uint32_t prev = g_modifier;
    g_modifier = MOD_TITLE_BEVEL;
    DrawText(pos, pxSize, col, text);
    g_modifier = prev;
}

// ---- text ----
V2 MeasureText(float pxSize, const char* text) {
    const Font& F = g_fonts[g_curFont];
    if (!F.ready || !text) return { 0, pxSize };
    if (F.msdf) {
        float sc = pxSize / F.mSize, w = 0;
        for (const char* p = text; *p; ++p) {
            int c = (unsigned char)*p;
            auto it = F.mglyphs.find(c);
            if (it == F.mglyphs.end()) it = F.mglyphs.find('?');
            if (it != F.mglyphs.end()) w += it->second.adv;
        }
        return { w * sc, pxSize };
    }
    float sc = pxSize / BAKE_PX, w = 0;
    for (const char* p = text; *p; ++p) {
        int c = (unsigned char)*p;
        if (c < FIRST_CH || c >= FIRST_CH + NUM_CH) c = '?';
        w += F.glyphs[c - FIRST_CH].advance;
    }
    return { w * sc, pxSize };
}

void DrawText(V2 pos, float pxSize, uint32_t col, const char* text) {
    const Font& F = g_fonts[g_curFont];
    if (!F.ready || !text) return;
    if (F.msdf) {
        // MSDF path: glyph quads are baked top-relative at FontSize; UVs are V-flipped
        // (Push maps TL->u0,v0 / BR->u1,v1, so passing them straight reproduces the flip).
        const float sc = pxSize / F.mSize;
        const float baseMs = pos.y + F.mAscent * sc;   // baseline (italic-shear pivot)
        const uint32_t prev = g_modifier;
        float x = pos.x;
        for (const char* p = text; *p; ++p) {
            int c = (unsigned char)*p;
            auto it = F.mglyphs.find(c);
            if (it == F.mglyphs.end()) it = F.mglyphs.find('?');
            if (it == F.mglyphs.end()) continue;
            const MGlyph& g = it->second;
            if (g.x1 > g.x0 && g.y1 > g.y0) {          // skip zero-area glyphs (space/whitespace)
                float gx0 = x + g.x0 * sc, gx1 = x + g.x1 * sc;
                float gy0 = pos.y + g.y0 * sc, gy1 = pos.y + g.y1 * sc;
                g_modifier = MOD_MSDF_TEXT;
                const size_t before = g_quads.size();
                Push(gx0, gy0, gx1, gy1, g.u0, g.v0, g.u1, g.v1, col, F.tex, gfx::Blend::Alpha);
                stretchLast(before, pos.x);
                shearLast(before, baseMs);
            }
            x += g.adv * sc;
        }
        g_modifier = prev;
        return;
    }
    const float sc = pxSize / BAKE_PX;
    const float baseY = pos.y + F.ascentPx * sc;          // baseline in screen space
    // text goes through the SDF shader path; keep an outer TITLE_BEVEL if a caller set one.
    const uint32_t prev = g_modifier;
    const uint32_t tm = (prev == MOD_TITLE_BEVEL) ? MOD_TITLE_BEVEL : MOD_SDF_TEXT;
    float x = pos.x;
    for (const char* p = text; *p; ++p) {
        int c = (unsigned char)*p;
        if (c < FIRST_CH || c >= FIRST_CH + NUM_CH) c = '?';
        const Glyph& G = F.glyphs[c - FIRST_CH];
        if (G.has) {
            float gx0 = x + G.xoff * sc,           gy0 = baseY + G.yoff * sc;
            float gx1 = gx0 + (G.x1 - G.x0) * sc,  gy1 = gy0 + (G.y1 - G.y0) * sc;
            float u0 = G.x0 / (float)ATLAS_W, v0 = G.y0 / (float)ATLAS_H;
            float u1 = G.x1 / (float)ATLAS_W, v1 = G.y1 / (float)ATLAS_H;
            g_modifier = tm;
            const size_t before = g_quads.size();
            Push(gx0, gy0, gx1, gy1, u0, v0, u1, v1, col, F.tex, gfx::Blend::Alpha);
            stretchLast(before, pos.x);
            shearLast(before, baseY);
        }
        x += G.advance * sc;
    }
    g_modifier = prev;
}

void DrawTextShadow(V2 pos, float pxSize, uint32_t col, const char* text, float offset, uint32_t shadowCol) {
    DrawText({ pos.x + offset, pos.y + offset }, pxSize, WithAlpha(shadowCol, A(col)/255.0f), text);
    DrawText(pos, pxSize, col, text);
}

void DrawTextAligned(V2 min, V2 max, float pxSize, uint32_t col, const char* text,
                     Align h, bool vCenter, bool shadow) {
    V2 sz = MeasureText(pxSize, text);
    float x = min.x;
    if (h == Align::Center) x = min.x + ((max.x - min.x) - sz.x) * 0.5f;
    else if (h == Align::Right) x = max.x - sz.x;
    float y = vCenter ? (min.y + ((max.y - min.y) - sz.y) * 0.5f) : min.y;
    if (shadow) DrawTextShadow({ x, y }, pxSize, col, text);
    else        DrawText({ x, y }, pxSize, col, text);
}

void SetTextShear(float xPerY) { g_textShear = xPerY; }
void ResetTextShear()          { g_textShear = 0.0f; }
void SetTextStretchX(float k)  { g_textStretchX = (k > 0.01f) ? k : 1.0f; }
void ResetTextStretchX()       { g_textStretchX = 1.0f; }

// Emit via DrawText (so font/shear/clip all apply), then re-tint each glyph
// quad's corners along the string's emitted vertical band. Shear only moves x,
// so corner y is still the gradient coordinate.
void DrawTextGradient(V2 pos, float pxSize, uint32_t colTop, uint32_t colBottom, const char* text) {
    const size_t start = g_quads.size();
    DrawText(pos, pxSize, colTop, text);
    if (g_quads.size() == start) return;
    float y0 = 1e9f, y1 = -1e9f;
    for (size_t i = start; i < g_quads.size(); ++i)
        for (int k = 0; k < 4; ++k) { y0 = std::min(y0, g_quads[i].py[k]); y1 = std::max(y1, g_quads[i].py[k]); }
    if (y1 <= y0) return;
    for (size_t i = start; i < g_quads.size(); ++i)
        for (int k = 0; k < 4; ++k)
            g_quads[i].color[k] = ColourLerp(colTop, colBottom, (g_quads[i].py[k] - y0) / (y1 - y0));
}

void DrawChevronWipe(float progress) {
    if (progress <= 0.001f) return;
    // band layout measured from the live wipe (heights sum to 720; directions
    // alternate; staggers <= 0.35 so every band completes by progress 1)
    struct Band { float y0, h, stagger; bool fromLeft; };
    static const Band B[7] = {
        { 0,    96, 0.00f, true  }, { 96,  120, 0.10f, false }, { 216,  86, 0.22f, true },
        { 302, 130, 0.05f, false }, { 432,  96, 0.15f, true  }, { 528, 104, 0.28f, false },
        { 632,  88, 0.08f, true  },
    };
    const float CH = 70.0f, SHARD = 58.0f;   // arrowhead length; shard lead distance
    const uint32_t BK = 0xFF000000u;
    for (const Band& b : B) {
        float lp = std::clamp((progress * 1.35f - b.stagger) / 1.0f, 0.0f, 1.0f);
        if (lp <= 0.0f) continue;
        const float y0 = b.y0, y1 = b.y0 + b.h, ym = b.y0 + b.h * 0.5f;
        // edge travels across the full width plus both tips
        const float span = REF_W + 2.0f * CH + SHARD;
        auto tip = [&](float e, float a, bool left) {
            // arrowhead: two triangles meeting at (e, ym); body side at e -/+ CH
            const float bx = left ? e - CH : e + CH;
            const uint32_t c = WithAlpha(BK, a);
            const V2 q1[4] = { { bx, y0 }, { e, ym }, { bx, ym }, { bx, y0 } };
            const V2 q2[4] = { { bx, ym }, { e, ym }, { bx, y1 }, { bx, ym } };
            const uint32_t qc[4] = { c, c, c, c };
            DrawQuadGradient(q1, qc);
            DrawQuadGradient(q2, qc);
        };
        if (b.fromLeft) {
            const float e = -CH - SHARD + span * lp;      // arrow tip x
            if (e - CH > 0) DrawRect({ 0, y0 }, { std::min(e - CH, (float)REF_W), y1 }, BK);
            tip(e, 1.0f, true);
            tip(std::min(e + SHARD, (float)REF_W + CH), 0.45f, true);   // leading shard
        } else {
            const float e = REF_W + CH + SHARD - span * lp;
            if (e + CH < REF_W) DrawRect({ std::max(e + CH, 0.0f), y0 }, { REF_W, y1 }, BK);
            tip(e, 1.0f, false);
            tip(std::max(e - SHARD, -CH), 0.45f, false);
        }
    }
}

void DrawQuadGradient(const V2 corners[4], const uint32_t cols[4], bool additive) {
    gfx::Quad q;
    const float uvx[4] = { 0, 1, 1, 0 }, uvy[4] = { 0, 0, 1, 1 };
    for (int k = 0; k < 4; ++k) {
        float cx = corners[k].x, cy = corners[k].y; xf(cx, cy);
        q.px[k] = cx; q.py[k] = cy;
        q.u[k] = uvx[k]; q.v[k] = uvy[k]; q.color[k] = applyGAlpha(cols[k]);
    }
    q.texIndex = -1; q.modifier = g_modifier;
    q.blend = additive ? gfx::Blend::Additive : gfx::Blend::Alpha;
    g_quads.push_back(q);
}

void DrawImageVGradient(int tex, V2 min, V2 max, V2 uv0, V2 uv1,
                        uint32_t colTop, uint32_t colBottom, bool additive) {
    const size_t before = g_quads.size();
    Push(min.x, min.y, max.x, max.y, uv0.x, uv0.y, uv1.x, uv1.y, colTop, tex,
         additive ? gfx::Blend::Additive : gfx::Blend::Alpha);
    if (g_quads.size() == before) return;            // fully clipped
    gfx::Quad& q = g_quads.back();
    // re-tint by each (possibly clip-adjusted) corner's position in the ORIGINAL band
    for (int k = 0; k < 4; ++k) {
        float f = (max.y > min.y) ? (q.py[k] - min.y) / (max.y - min.y) : 0.0f;
        q.color[k] = applyGAlpha(ColourLerp(colTop, colBottom, f));
    }
}

// ---- shared button-guide footer (ported 1:1 from ui/button_guide.cpp) -------
void DrawButtonGuide(const GuideBtn* btns, int count, int rodinFont, float alpha, float sideMargins) {
    static int s_iconTex = -2;
    if (s_iconTex == -2) s_iconTex = gfx::loadTexture("assets/options/mat_comon_x360_001.png");
    struct IconUV { float u0, v0, u1, v1; };
    // controller glyph atlas mat_comon_x360_001.png (512x512): A B X Y | LB RB
    static const IconUV UV[6] = {
        { 0.00000f, 0.00781f, 0.07227f, 0.07617f },  // A
        { 0.08008f, 0.00781f, 0.15039f, 0.07422f },  // B
        { 0.16016f, 0.00781f, 0.23047f, 0.07422f },  // X
        { 0.24023f, 0.00781f, 0.31055f, 0.07422f },  // Y (estimated; footers rarely use Y)
        { 0.32617f, 0.00781f, 0.46094f, 0.07812f },  // LB
        { 0.48242f, 0.00781f, 0.61523f, 0.07812f },  // RB
    };
    auto iconW = [](GIcon ic) { return (ic == GIcon::LB || ic == GIcon::RB || ic == GIcon::LBRB) ? 70.0f : 40.0f; };
    const float regMinX = sideMargins, regMaxX = REF_W - sideMargins;
    const float regMinY = REF_H - 102.0f;            // 618
    const float iconH = 40.0f, fontSz = 21.8f, marginX = 21.25f, iconGap = 4.0f, textY = regMinY + 9.0f;
    const uint32_t white = WithAlpha(RGBA(255,255,255,255), alpha), black = WithAlpha(RGBA(0,0,0,255), alpha);
    SetFont(rodinFont);
    auto icon = [&](GIcon ic, float x) {             // draw a single glyph at [x, x+w], y618..658
        const IconUV& u = UV[ic == GIcon::LBRB ? (int)GIcon::LB : (int)ic];
        if (s_iconTex >= 0) DrawImage(s_iconTex, { x, regMinY }, { x + iconW(ic), regMinY + iconH }, { u.u0, u.v0 }, { u.u1, u.v1 }, white);
    };
    auto label = [&](const char* s, float x, float maxW) {    // outlined NewRodin label; returns drawn width
        float tw = MeasureText(fontSz, s).x; float sx = (maxW > 0.0f && tw > maxW) ? maxW / tw : 1.0f;
        if (sx != 1.0f) SetTextStretchX(sx);
        static const float O[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,-1},{-1,1},{1,1}};
        for (auto& o : O) DrawText({ x + o[0]*1.6f, textY + o[1]*1.6f }, fontSz, black, s);
        DrawText({ x, textY }, fontSz, white, s);
        if (sx != 1.0f) ResetTextStretchX();
        return tw * sx;
    };
    // left-aligned group flows left -> right from the left margin
    float lx = regMinX;
    for (int i = 0; i < count; ++i) {
        if (btns[i].align != GAlign::Left) continue;
        const GuideBtn& b = btns[i]; float w = iconW(b.icon);
        if (b.icon == GIcon::LBRB) {
            icon(GIcon::LB, lx); lx += w + iconGap;
            float tw = label(b.label, lx, b.maxWidth); lx += tw + iconGap;
            icon(GIcon::RB, lx); lx += w + marginX;
        } else {
            icon(b.icon, lx); lx += w + iconGap;
            float tw = label(b.label, lx, b.maxWidth); lx += tw + marginX;
        }
    }
    // right-aligned group flows right -> left (reverse order: the last button is rightmost)
    float rx = regMaxX;
    for (int i = count - 1; i >= 0; --i) {
        if (btns[i].align != GAlign::Right) continue;
        const GuideBtn& b = btns[i]; float w = iconW(b.icon);
        float tw = MeasureText(fontSz, b.label).x; float sx = (b.maxWidth > 0.0f && tw > b.maxWidth) ? b.maxWidth / tw : 1.0f; float dtw = tw * sx;
        if (b.icon == GIcon::LBRB) {
            float total = w + iconGap + dtw + iconGap + w; rx -= total;
            icon(GIcon::LB, rx); label(b.label, rx + w + iconGap, b.maxWidth); icon(GIcon::RB, rx + w + iconGap + dtw + iconGap);
        } else {
            float total = w + iconGap + dtw; rx -= total;
            icon(b.icon, rx); label(b.label, rx + w + iconGap, b.maxWidth);
        }
        rx -= marginX;
    }
    ResetFont();
}

} // namespace ui
