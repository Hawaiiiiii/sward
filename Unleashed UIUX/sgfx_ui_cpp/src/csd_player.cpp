// =============================================================================
// csd_player.cpp — runtime player for the real game's CSD layout (see csd_player.h).
// Loads data/<id>.json once into structs, then each frame evaluates the cast
// hierarchy at the current animation time and emits the real quads via ui::DrawImage.
// =============================================================================
#include "csd_player.h"
#include "sgfxui.h"
#include "gfx_d3d12.h"

#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <array>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>

using nlohmann::json;

namespace csd {
namespace {

constexpr float REF_W = 1280.0f, REF_H = 720.0f;

enum Interp { CONST_, LINEAR, HERMITE };
// v is a DOUBLE, not float: Color/Gradient tracks store packed RRGGBBAA as large
// integers (e.g. 0xFFFFFFFF = 4294967295). float32 is only exact to 2^24, so such
// values round to 2^32 and decode to a 0 alpha (invisible). double is exact to 2^53.
struct KF { float f; double v; Interp t; float in_, out_; };
using Track = std::vector<KF>;

struct Anim { float frames = 0; std::unordered_map<std::string, Track> tracks; };

struct Cast {
    int   parent = -1;
    std::string id;                // cast name (used to detect 9-slice slices: L/C/R, top/middle/bottom)
    bool  hasQuad = false;
    float quad[4] = { 0,0,0,0 };
    int   texSlot = -1;            // -1 = transform-only node (no draw) or failed load
    bool  hasTex = false;          // a tex NAME was present (for pose scoring, à la resolve_nodes.py)
    float uv[4] = { 0,0,1,1 };
    bool  hasUV = false;
    std::vector<std::array<float, 4>> cells;   // sprite-sheet cell UV rects; base sub / SubImage track picks one
    int   subIdx = 0;              // base 'sub' = default cell index into cells[]
    // 9-slice: a left/right (anchorX) or top/bottom (anchorY) frame slice should keep a FIXED size on that
    // axis (using the AMBIENT scale, stretch removed) while still being POSITIONED by the full stretched
    // layout — so window corners stay crisp and only the centre fill stretches, instead of smearing.
    bool  anchorX = false, anchorY = false;
    float tx = 0, ty = 0, sx = 1, sy = 1, rot = 0;
    uint8_t br = 255, bg = 255, bb = 255, ba = 255;   // base RRGGBBAA
    bool  hide = false;
    bool  additive = false;
    std::unordered_map<std::string, Anim> anims;
};

struct Scene {
    bool        draw = true;       // "rest":false scenes are skipped (alt-state overlays)
    bool        stateHidden = false;  // hidden in the current state (e.g. an _ev_-only scene under "so")
    std::vector<std::pair<std::string, float>> animList;   // every (animName, frames) the scene declares
    std::string restAnim;          // resolved best-pose animation (resolve_rest port)
    float       restFrame = 0;     // resolved best-pose frame
    std::vector<Cast> casts;
};

// Day/night (Sonic/Werehog) state bias for resolveRest. Sonic Unleashed CSDs
// carry per-scene "_so_" (Sonic/day) and "_ev_" (evil/Werehog/night) animation
// variants; with no bias resolveRest picks the fullest pose (usually the ev one),
// so the raw render shows the day+night SUPERSET. SetState("so"/"ev") restricts
// each scene to its matching variant and hides scenes that exist only in the
// opposite state — yielding a clean single-state base to composite C++ on top of.
static std::string g_state;   // "" = no bias (legacy superset); "so" = day; "ev" = night

struct Doc {
    std::string id;
    float framerate = 60.0f;
    std::vector<Scene> scenes;
};

Doc g_doc;
bool g_loaded = false;
std::unordered_map<std::string, int> g_texCache;   // tex name -> slot

Interp parseInterp(const std::string& s) {
    if (s == "Const")  return CONST_;
    if (s == "Hermite") return HERMITE;
    return LINEAR;
}
void decodeRGBA(uint32_t n, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
    r = (n >> 24) & 255; g = (n >> 16) & 255; b = (n >> 8) & 255; a = n & 255;   // RRGGBBAA
}

// ---- track evaluation (matches resolve_nodes.py / the browser player) --------
float evalTrack(const Track& k, float frame) {
    if (k.empty()) return 0.0f;
    if (frame <= k.front().f) return k.front().v;
    if (frame >= k.back().f)  return k.back().v;
    for (size_t i = 0; i + 1 < k.size(); ++i) {
        const KF& a = k[i]; const KF& b = k[i + 1];
        if (frame >= a.f && frame <= b.f) {
            float dt = b.f - a.f; if (dt <= 0) return a.v;
            float t = (frame - a.f) / dt;
            if (a.t == CONST_)  return a.v;
            if (a.t == LINEAR)  return a.v + (b.v - a.v) * t;
            float t2 = t * t, t3 = t2 * t;                                       // Hermite
            return (2*t3 - 3*t2 + 1) * a.v + (t3 - 2*t2 + t) * (a.out_ * dt)
                 + (-2*t3 + 3*t2) * b.v + (t3 - t2) * (b.in_ * dt);
        }
    }
    return k.back().v;
}
// per-channel interpolation of a packed-uint colour track
void evalColorTrack(const Track& k, float frame, uint8_t out[4]) {
    auto dec = [](double v, uint8_t o[4]) { decodeRGBA((uint32_t)(int64_t)v, o[0], o[1], o[2], o[3]); };
    if (k.empty()) { out[0]=out[1]=out[2]=out[3]=255; return; }
    if (frame <= k.front().f) { dec(k.front().v, out); return; }
    if (frame >= k.back().f)  { dec(k.back().v, out); return; }
    for (size_t i = 0; i + 1 < k.size(); ++i) {
        const KF& a = k[i]; const KF& b = k[i + 1];
        if (frame >= a.f && frame <= b.f) {
            float dt = b.f - a.f; if (dt <= 0 || a.t == CONST_) { dec(a.v, out); return; }
            float t = (frame - a.f) / dt; uint8_t A[4], B[4]; dec(a.v, A); dec(b.v, B);
            for (int j = 0; j < 4; ++j) out[j] = (uint8_t)(A[j] + (B[j] - A[j]) * t);
            return;
        }
    }
    dec(k.back().v, out);
}

struct Ev { float tx, ty, sx, sy, rot; uint8_t r, g, b, a; bool vis; int cell; };
Ev evalCast(const Cast& c, const std::string& anim, float frame) {
    Ev e{ c.tx, c.ty, c.sx, c.sy, c.rot, c.br, c.bg, c.bb, c.ba, !c.hide, c.subIdx };
    auto ai = c.anims.find(anim);
    if (ai != c.anims.end()) {
        const auto& tr = ai->second.tracks;
        auto get = [&](const char* k, float& dst) { auto it = tr.find(k); if (it != tr.end()) dst = evalTrack(it->second, frame); };
        get("XPosition", e.tx); get("YPosition", e.ty); get("XScale", e.sx); get("YScale", e.sy);
        get("Rotation", e.rot);
        { auto it = tr.find("HideFlag"); if (it != tr.end()) e.vis = evalTrack(it->second, frame) < 0.5f; }
        // SubImage track selects the sprite-sheet cell (overrides base sub); values are integers
        { auto it = tr.find("SubImage"); if (it != tr.end()) e.cell = (int)std::lround(evalTrack(it->second, frame)); }
        // colour: a Color track, else the average of any Gradient* corner tracks
        const char* grads[4] = { "GradientTL", "GradientTR", "GradientBL", "GradientBR" };
        if (auto it = tr.find("Color"); it != tr.end()) {
            uint8_t col[4]; evalColorTrack(it->second, frame, col);
            e.r = col[0]; e.g = col[1]; e.b = col[2]; e.a = col[3];
        } else {
            int n = 0; int acc[4] = { 0,0,0,0 };
            for (const char* gk : grads) { auto g = tr.find(gk); if (g != tr.end()) { uint8_t col[4]; evalColorTrack(g->second, frame, col); for (int j=0;j<4;++j) acc[j]+=col[j]; ++n; } }
            if (n) { e.r=acc[0]/n; e.g=acc[1]/n; e.b=acc[2]/n; e.a=acc[3]/n; }
        }
    }
    return e;
}

// sx/sy = full world scale (for POSITION composition). bx/by = ambient scale (stretch removed).
// esx/esy = EFFECTIVE SIZE scale used to draw the quad: it inherits the parent's effective scale but
// an anchored cast resets to ambient (fixed size), and that cap then propagates to its descendants —
// so a "stretch" slice nested under an anchored frame corner stays fixed instead of re-inheriting the
// window stretch (e.g. shop's line_left under the anchored bg_left).
struct World { float ox, oy, sx, sy, a; bool vis; uint8_t r, g, b; float rot; int cell; float bx, by, esx, esy; };
// ambient scale: a 9-slice stretch driver (|scale|>3) contributes UNIT scale, so anchored
// frame slices below it keep their intrinsic size instead of inheriting the stretch.
inline float ambScale(float s) { return (std::fabs(s) > 3.0f) ? (s < 0 ? -1.0f : 1.0f) : s; }
World worldOf(const Scene& sc, std::vector<int>& state, std::vector<World>& cache, int i, const std::string& anim, float frame) {
    if (state[i] == 2) return cache[i];
    const Cast& c = sc.casts[i];
    Ev e = evalCast(c, anim, frame);
    // resolve sprite cell against this cast's cell table (cell index does not compose through parents)
    int cell = e.cell;
    if (!c.cells.empty()) { if (cell < 0) cell = 0; if (cell >= (int)c.cells.size()) cell = (int)c.cells.size() - 1; }
    World w;
    if (c.parent < 0 || c.parent >= (int)sc.casts.size()) {
        float bx = ambScale(e.sx), by = ambScale(e.sy);
        w = { e.tx, e.ty, e.sx, e.sy, e.a / 255.0f, e.vis, e.r, e.g, e.b, e.rot, cell,
              bx, by, (c.anchorX ? bx : e.sx), (c.anchorY ? by : e.sy) };
    } else {
        World pw = worldOf(sc, state, cache, c.parent, anim, frame);
        float bx = pw.bx * ambScale(e.sx), by = pw.by * ambScale(e.sy);
        w = { pw.ox + e.tx * pw.sx, pw.oy + e.ty * pw.sy, pw.sx * e.sx, pw.sy * e.sy,
              pw.a * (e.a / 255.0f), pw.vis && e.vis, e.r, e.g, e.b,
              pw.rot + e.rot, cell,                                    // rotation composes ADDITIVELY
              bx, by,
              (c.anchorX ? bx : pw.esx * e.sx),                        // anchored => fixed; else inherit parent effective
              (c.anchorY ? by : pw.esy * e.sy) };
    }
    cache[i] = w; state[i] = 2;
    return w;
}

// Detect bounded 9-slice frames and flag their anchored slices. A parent with BOTH an L and an
// R child is a horizontal 3-slice (L/R anchored, C stretches); a parent with both top and bottom
// children is a vertical 3-slice (its top/bottom rows' pieces are Y-anchored). Lone spanning
// dividers (no L+R / top+bottom sibling set) are left alone, so legit full-width bars still span.
void detectNineSlice(Scene& sc) {
    const int N = (int)sc.casts.size();
    std::vector<std::vector<int>> kids(N);
    for (int i = 0; i < N; ++i) { int p = sc.casts[i].parent; if (p >= 0 && p < N) kids[p].push_back(i); }
    auto isL = [](const std::string& s) { return s == "L" || s == "TL" || s == "BL"; };
    auto isR = [](const std::string& s) { return s == "R" || s == "TR" || s == "BR"; };
    std::vector<char> hasLR(N, 0), hasRows(N, 0);
    for (int p = 0; p < N; ++p) {
        bool l = false, r = false, t = false, b = false;
        for (int ch : kids[p]) { const std::string& id = sc.casts[ch].id;
            if (isL(id)) l = true; if (isR(id)) r = true; if (id == "top") t = true; if (id == "bottom") b = true; }
        hasLR[p] = (l && r); hasRows[p] = (t && b);
    }
    for (int i = 0; i < N; ++i) {
        int p = sc.casts[i].parent; if (p < 0 || p >= N) continue;
        const std::string& id = sc.casts[i].id;
        if (hasLR[p] && (isL(id) || isR(id))) sc.casts[i].anchorX = true;       // left/right column = fixed width
        int gp = sc.casts[p].parent;
        if (gp >= 0 && gp < N && hasRows[gp]) {                                  // p is a row under a 9-slice driver
            const std::string& pid = sc.casts[p].id;
            if (pid == "top" || pid == "bottom") sc.casts[i].anchorY = true;     // top/bottom row = fixed height
        }
    }
}

// Build the 4 rotated screen-pixel corners (TL,TR,BR,BL) for a cast, about its
// world origin: scale the normalized quad offsets, rotate by world rot (DEGREES,
// composed additively), translate by origin, then *REF. rot==0 reduces exactly to
// the axis-aligned rect, so this is a strict superset. (Pivot = cast origin.)
void rotatedCorners(const World& w, const float quad[4], float sx, float sy, ui::V2 out[4]) {
    const float lx[4] = { quad[0], quad[2], quad[2], quad[0] };   // TL,TR,BR,BL x-offsets
    const float ly[4] = { quad[1], quad[1], quad[3], quad[3] };
    const float a = w.rot * (3.14159265358979323846f / 180.0f);
    const float ca = std::cos(a), sa = std::sin(a);
    for (int k = 0; k < 4; ++k) {
        const float ex = lx[k] * sx, ey = ly[k] * sy;             // scale (anchor-aware)
        const float rx = ex * ca - ey * sa, ry = ex * sa + ey * ca; // rotate about origin
        out[k].x = (w.ox + rx) * REF_W;                            // translate + to ref-pixels
        out[k].y = (w.oy + ry) * REF_H;
    }
}

// ---- rest-pose resolution (faithful port of resolve_nodes.py) ----------------
// A CSD scene can hold several animations and many sub-states; the "resting"
// layout is the (anim, frame) pair that shows the MOST content on-screen. We try
// the base pose plus 9 evenly-spaced frames of every declared anim and keep the
// highest-scoring one. This is why pause/start/exstage were blank under naive
// clamp-to-last: their fullest pose is a *different* anim (e.g. Usual_Anim) or an
// earlier frame, not the last frame of anims[0].
int poseScore(const Scene& sc, const std::string& anim, float frame) {
    const int N = (int)sc.casts.size();
    std::vector<int> state(N, 0);
    std::vector<World> cache(N);
    int score = 0;
    for (int i = 0; i < N; ++i) {
        const Cast& c = sc.casts[i];
        if (!c.hasTex || !c.hasQuad || !c.hasUV) continue;   // count by tex NAME (matches the python tool)
        World w = worldOf(sc, state, cache, i, anim, frame);
        if (!w.vis || w.a <= 0.003f) continue;
        float qx = w.ox + c.quad[0] * w.sx, qy = w.oy + c.quad[1] * w.sy;
        float qw = (c.quad[2] - c.quad[0]) * w.sx, qh = (c.quad[3] - c.quad[1]) * w.sy;
        float x0 = qx * REF_W, y0 = qy * REF_H, x1 = (qx + qw) * REF_W, y1 = (qy + qh) * REF_H;
        if (x1 < x0) std::swap(x0, x1);
        if (y1 < y0) std::swap(y0, y1);
        float bw = x1 - x0, bh = y1 - y0;
        if (bw < 0.5f || bh < 0.5f || bw > 1.5f * REF_W || bh > 1.5f * REF_H) continue;
        if (x1 <= 0 || y1 <= 0 || x0 >= REF_W || y0 >= REF_H) continue;
        ++score;
    }
    return score;
}
void resolveRest(Scene& sc) {
    sc.stateHidden = false;
    std::string matchTag, oppTag;
    if (!g_state.empty()) {
        matchTag = "_" + g_state + "_";                          // "_so_" / "_ev_"
        oppTag   = (g_state == "so") ? "_ev_" : "_so_";
        bool hasMatch = false, hasOpp = false, hasNeutral = false;
        for (const auto& [name, maxf] : sc.animList) {
            if (name.find(matchTag) != std::string::npos)      hasMatch = true;
            else if (name.find(oppTag) != std::string::npos)   hasOpp   = true;
            else                                               hasNeutral = true;
        }
        // a scene that only exists in the opposite state (e.g. the extra Werehog
        // stat labels under "so") is not present in this state at all.
        if (hasOpp && !hasMatch && !hasNeutral) { sc.stateHidden = true; sc.restAnim.clear(); sc.restFrame = 0; return; }
    }
    std::string bestAnim; float bestFrame = 0.0f;
    int best = poseScore(sc, "", 0.0f);                 // base pose (no anim) at frame 0
    for (const auto& [name, maxf] : sc.animList) {
        if (!oppTag.empty() && name.find(oppTag) != std::string::npos) continue;   // skip opposite-state anims
        for (int k = 0; k < 9; ++k) {
            float f = (maxf > 0.0f) ? maxf * (float)k / 8.0f : 0.0f;
            int s = poseScore(sc, name, f);
            if (s > best || (s == best && s > 0 && f > bestFrame)) { best = s; bestAnim = name; bestFrame = f; }
            if (maxf <= 0.0f) break;
        }
    }
    sc.restAnim = bestAnim; sc.restFrame = bestFrame;
}

int texFor(const std::string& tex) {
    if (tex.empty()) return -1;
    auto it = g_texCache.find(tex);
    if (it != g_texCache.end()) return it->second;
    int slot = gfx::loadTexture("assets/" + g_doc.id + "/" + tex + ".png");
    g_texCache[tex] = slot;
    return slot;
}

} // namespace

bool Load(const char* id) {
    Unload();
    std::string path = std::string("data/") + id + ".json";   // try cwd/data first
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) {                                                  // fall back to the source tree
        path = std::string("C:/swardbuild/sgfx_ui/data/") + id + ".json";
        fp = fopen(path.c_str(), "rb");
    }
    if (!fp) { fprintf(stderr, "[csd] no data for '%s'\n", id); return false; }
    fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET);
    std::string buf((size_t)n, '\0'); if (fread(&buf[0], 1, (size_t)n, fp) != (size_t)n) { fclose(fp); return false; }
    fclose(fp);

    json j;
    try { j = json::parse(buf); } catch (...) { fprintf(stderr, "[csd] parse failed for '%s'\n", id); return false; }

    g_doc.id = id;
    g_doc.framerate = j.value("framerate", 60.0f);
    for (const auto& js : j.value("scenes", json::array())) {
        Scene sc;
        sc.draw = js.value("rest", true);
        if (js.contains("anims") && js["anims"].is_array()) {
            for (const auto& a : js["anims"])
                sc.animList.emplace_back(a.value("name", std::string()), a.value("frames", 0.0f));
        }
        for (const auto& jc : js.value("casts", json::array())) {
            Cast c;
            c.parent = jc.value("parent", -1);
            c.id = jc.value("id", std::string());
            if (jc.contains("quad") && jc["quad"].is_array() && jc["quad"].size() == 4) {
                for (int k = 0; k < 4; ++k) c.quad[k] = jc["quad"][k].get<float>();
                c.hasQuad = true;
            }
            std::string tex = jc.value("tex", std::string());
            c.hasTex = !tex.empty();
            c.texSlot = texFor(tex);
            if (jc.contains("uv") && jc["uv"].is_array() && jc["uv"].size() == 4) {
                for (int k = 0; k < 4; ++k) c.uv[k] = jc["uv"][k].get<float>();
                c.hasUV = true;
            }
            // sprite-sheet cells: the full per-cast UV-rect table the base sub / SubImage track indexes
            if (jc.contains("cells") && jc["cells"].is_array()) {
                for (const auto& jcell : jc["cells"]) {
                    if (jcell.is_array() && jcell.size() == 4) {
                        std::array<float, 4> cell{};
                        for (int k = 0; k < 4; ++k) cell[k] = jcell[k].get<float>();
                        c.cells.push_back(cell);
                    }
                }
            }
            c.subIdx = jc.value("subIdx", 0);
            { int anc = jc.value("anc", 0);   // 9-slice anchor mask from field34: 1=anchorX, 2=anchorY
              c.anchorX = (anc & 1) != 0; c.anchorY = (anc & 2) != 0; }
            if (!c.hasUV && !c.cells.empty()) { for (int k = 0; k < 4; ++k) c.uv[k] = c.cells[0][k]; c.hasUV = true; }
            const auto& b = jc.at("base");
            c.tx = b.value("tx", 0.0f); c.ty = b.value("ty", 0.0f);
            c.sx = b.value("sx", 1.0f); c.sy = b.value("sy", 1.0f);
            c.rot = b.value("rot", 0.0f);
            c.hide = b.value("hide", 0) != 0;
            { std::string col = b.value("color", std::string("0xFFFFFFFF"));
              if (col.size() == 10) { uint32_t v = (uint32_t)strtoul(col.c_str() + 2, nullptr, 16); decodeRGBA(v, c.br, c.bg, c.bb, c.ba); } }
            c.additive = jc.value("add", 0) != 0;
            if (jc.contains("anims") && jc["anims"].is_object()) {
                for (auto it = jc["anims"].begin(); it != jc["anims"].end(); ++it) {
                    Anim an; an.frames = it.value().value("frames", 0.0f);
                    if (it.value().contains("tracks") && it.value()["tracks"].is_object()) {
                        for (auto t = it.value()["tracks"].begin(); t != it.value()["tracks"].end(); ++t) {
                            Track tk;
                            for (const auto& jk : t.value()) {
                                KF kf; kf.f = jk.value("f", 0.0f); kf.v = jk.value("v", 0.0);
                                kf.t = parseInterp(jk.value("t", std::string("Linear")));
                                kf.in_ = jk.value("it", 0.0f); kf.out_ = jk.value("ot", 0.0f);
                                tk.push_back(kf);
                            }
                            an.tracks[t.key()] = std::move(tk);
                        }
                    }
                    c.anims[it.key()] = std::move(an);
                }
            }
            sc.casts.push_back(std::move(c));
        }
        detectNineSlice(sc);            // flag 9-slice frame slices so they don't smear
        if (sc.draw) resolveRest(sc);   // pick the fullest (anim, frame) for the resting layout
        g_doc.scenes.push_back(std::move(sc));
    }
    g_loaded = !g_doc.scenes.empty();
    fprintf(stderr, "[csd] loaded '%s': %zu scenes\n", id, g_doc.scenes.size());
    return g_loaded;
}

// Time model: CSD scenes are play-once-and-hold entrance animations (the game
// plays the intro, then the UI rests at the final pose). So we CLAMP elapsed time
// to the scene's last frame rather than looping it (fmod) — looping made short
// scenes replay forever and never let long blocks like result's 253-frame stat
// assembly settle. g_loop forces the legacy looping mode for ambient/decorative
// anims that genuinely cycle (e.g. title LED strip) when explicitly requested.
bool g_loop = false;
void SetLoop(bool on) { g_loop = on; }

// Select the day ("so") / night ("ev") cast state and re-resolve every scene's
// resting pose to the matching variant (hiding opposite-only scenes). Pass "" to
// clear the bias (legacy superset). Call after Load(), before Draw().
void SetState(const char* tag) {
    g_state = tag ? tag : "";
    for (Scene& sc : g_doc.scenes) if (sc.draw) resolveRest(sc);
}

void Draw(double elapsedSec) {
    if (!g_loaded) return;
    bool dbg = getenv("SGFX_CSD_DEBUG") != nullptr;
    int sceneIdx = -1;
    for (const Scene& sc : g_doc.scenes) {
        ++sceneIdx;
        if (!sc.draw || sc.stateHidden) continue;
        // Play the resolved rest anim from frame 0 toward restFrame, then HOLD the
        // settled pose. g_loop replays continuously instead of holding.
        float t = (float)(elapsedSec * g_doc.framerate);
        float frame = sc.restFrame;
        if (sc.restFrame > 0.0f)
            frame = g_loop ? (float)std::fmod(t, sc.restFrame) : (t < sc.restFrame ? t : sc.restFrame);
        const int N = (int)sc.casts.size();
        std::vector<int> state(N, 0);
        std::vector<World> cache(N);
        int emitted = 0, noTex = 0, invis = 0, culled = 0;
        for (int i = 0; i < N; ++i) {
            const Cast& c = sc.casts[i];
            if (!c.hasQuad || !c.hasUV) continue;                    // transform-only node
            if (c.texSlot < 0) { if (c.hasTex) ++noTex; continue; }  // texture failed to load
            World w = worldOf(sc, state, cache, i, sc.restAnim, frame);
            if (!w.vis || w.a <= 0.003f) { ++invis; continue; }
            // resolved sprite-sheet cell -> UV (base sub / SubImage track); fallback = baked c.uv
            const float* uv = c.uv;
            if (!c.cells.empty()) uv = c.cells[(size_t)w.cell].data();
            const uint32_t col = ui::RGBA(w.r, w.g, w.b, (int)(w.a * 255.0f + 0.5f));
            // 9-slice: draw the quad at the EFFECTIVE SIZE scale (anchored frame slices stay fixed, even
            // when nested under a stretched window); POSITION origin (w.ox/oy) is already fully stretched.
            const float sX = w.esx;
            const float sY = w.esy;

            if (std::fabs(w.rot) > 0.05f) {
                // ROTATED cast: emit 4 transformed corners; the render target clips them.
                ui::V2 cor[4]; rotatedCorners(w, c.quad, sX, sY, cor);
                float mnx = cor[0].x, mxx = cor[0].x, mny = cor[0].y, mxy = cor[0].y;
                for (int k = 1; k < 4; ++k) {
                    mnx = std::min(mnx, cor[k].x); mxx = std::max(mxx, cor[k].x);
                    mny = std::min(mny, cor[k].y); mxy = std::max(mxy, cor[k].y);
                }
                if (mxx <= 0 || mxy <= 0 || mnx >= REF_W || mny >= REF_H) { ++culled; continue; }
                if (mxx - mnx > 5.0f * REF_W || mxy - mny > 5.0f * REF_H) { ++culled; continue; } // scale² artifact
                const ui::V2 uvs[4] = { {uv[0],uv[1]}, {uv[2],uv[1]}, {uv[2],uv[3]}, {uv[0],uv[3]} };
                ui::DrawImageQuad(c.texSlot, cor, uvs, col, c.additive);
                ++emitted;
            } else {
                // AXIS-ALIGNED: clip the dest rect to the 1280x720 screen and remap UVs so
                // over-spanning bars/borders are CROPPED to screen (not culled, not squashed).
                float qx = w.ox + c.quad[0] * sX, qy = w.oy + c.quad[1] * sY;
                float qw = (c.quad[2] - c.quad[0]) * sX, qh = (c.quad[3] - c.quad[1]) * sY;
                float x0 = qx * REF_W, y0 = qy * REF_H, x1 = (qx + qw) * REF_W, y1 = (qy + qh) * REF_H;
                if (x1 < x0) std::swap(x0, x1);
                if (y1 < y0) std::swap(y0, y1);
                if (x1 <= 0 || y1 <= 0 || x0 >= REF_W || y0 >= REF_H) { ++culled; continue; }
                if (x1 - x0 > 5.0f * REF_W || y1 - y0 > 5.0f * REF_H) { ++culled; continue; } // scale² artifact
                float u0 = uv[0], v0 = uv[1], u1 = uv[2], v1 = uv[3];
                const float oW = x1 - x0, oH = y1 - y0, du = u1 - u0, dv = v1 - v0;  // pre-clamp spans
                if (oW > 0.0f) { if (x0 < 0.0f) { u0 += du * (0.0f - x0) / oW; x0 = 0.0f; }
                                 if (x1 > REF_W) { u1 -= du * (x1 - REF_W) / oW; x1 = REF_W; } }
                if (oH > 0.0f) { if (y0 < 0.0f) { v0 += dv * (0.0f - y0) / oH; y0 = 0.0f; }
                                 if (y1 > REF_H) { v1 -= dv * (y1 - REF_H) / oH; y1 = REF_H; } }
                ui::DrawImage(c.texSlot, { x0, y0 }, { x1, y1 }, { u0, v0 }, { u1, v1 }, col, c.additive);
                ++emitted;
            }
        }
        if (dbg && (emitted || noTex || invis || culled))
            fprintf(stderr, "[csd] scene%d anim=%s frame=%.0f emit=%d noTex=%d invis=%d culled=%d\n",
                    sceneIdx, sc.restAnim.empty() ? "(base)" : sc.restAnim.c_str(), frame, emitted, noTex, invis, culled);
    }
}

void Unload() {
    g_doc = Doc{};
    g_texCache.clear();
    g_loaded = false;
}

const char* LoadedId() { return g_loaded ? g_doc.id.c_str() : ""; }

} // namespace csd
