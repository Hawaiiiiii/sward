// =============================================================================
// screen_viewport3d.cpp — the REAL 3D viewport (the SGFX QA-tool seed): a
// depth-buffered mesh pass on the plume backend (mesh_vs/mesh_ps, perspective
// camera, directional light) composing UNDER the sgfxui overlay. Demo scene:
// an orbiting camera around lit cubes on a grid floor — the slot where RaCo
// pipeline content (glTF meshes) loads next. Left/Right orbit; Up/Down zoom.
// =============================================================================
#include "sgfxui.h"
#include "gltf_loader.h"
#include "screen.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fRodin = 0, g_fDF = 0;
int g_cube = -1, g_floor = -1;
float g_orbit = 35.0f, g_dist = 9.0f;
gltf::Model g_model;          // the RaCo-pipeline content slot (assets/viewport/)

// ---- minimal row-vector matrix math (pos * M; row-major float[16]) ----------
void matIdentity(float* m) { for (int i = 0; i < 16; ++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f; }
void matMul(float* r, const float* a, const float* b) {   // r = a * b
    float t[16];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            t[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] + a[i * 4 + 1] * b[1 * 4 + j]
                         + a[i * 4 + 2] * b[2 * 4 + j] + a[i * 4 + 3] * b[3 * 4 + j];
    memcpy(r, t, sizeof t);
}
void matTranslate(float* m, float x, float y, float z) { matIdentity(m); m[12] = x; m[13] = y; m[14] = z; }
void matRotY(float* m, float a) { matIdentity(m); float c = std::cos(a), s = std::sin(a); m[0] = c; m[2] = -s; m[8] = s; m[10] = c; }
void matScale(float* m, float x, float y, float z) { matIdentity(m); m[0] = x; m[5] = y; m[10] = z; }
// left-handed look-at (camera at eye, +Z into the screen)
void matLookAt(float* m, float ex, float ey, float ez, float tx, float ty, float tz) {
    float zx = tx - ex, zy = ty - ey, zz = tz - ez;
    float zl = std::sqrt(zx * zx + zy * zy + zz * zz); zx /= zl; zy /= zl; zz /= zl;
    // x = normalize(up(0,1,0) cross z); y = z cross x
    float xx = zz, xy = 0.0f, xz = -zx;
    float xl = std::sqrt(xx * xx + xy * xy + xz * xz); xx /= xl; xy /= xl; xz /= xl;
    float yx = zy * xz - zz * xy, yy = zz * xx - zx * xz, yz = zx * xy - zy * xx;
    matIdentity(m);
    m[0] = xx; m[1] = yx; m[2] = zx;
    m[4] = xy; m[5] = yy; m[6] = zy;
    m[8] = xz; m[9] = yz; m[10] = zz;
    m[12] = -(ex * xx + ey * xy + ez * xz);
    m[13] = -(ex * yx + ey * yy + ez * yz);
    m[14] = -(ex * zx + ey * zy + ez * zz);
}
void matPerspective(float* m, float fovY, float aspect, float zn, float zf) {   // LH, D3D z 0..1
    float ys = 1.0f / std::tan(fovY * 0.5f), xs = ys / aspect;
    for (int i = 0; i < 16; ++i) m[i] = 0;
    m[0] = xs; m[5] = ys; m[10] = zf / (zf - zn); m[11] = 1.0f; m[14] = -zn * zf / (zf - zn);
}

// ---- procedural meshes --------------------------------------------------------
int makeCube() {
    // 24 verts (per-face normals), 36 indices
    gfx::MeshVertex v[24]; uint16_t idx[36];
    const float n[6][3] = { {0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0} };
    const float q[6][4][3] = {
        { {-1,-1,-1},{ 1,-1,-1},{ 1, 1,-1},{-1, 1,-1} },   // -Z
        { { 1,-1, 1},{-1,-1, 1},{-1, 1, 1},{ 1, 1, 1} },   // +Z
        { {-1,-1, 1},{-1,-1,-1},{-1, 1,-1},{-1, 1, 1} },   // -X
        { { 1,-1,-1},{ 1,-1, 1},{ 1, 1, 1},{ 1, 1,-1} },   // +X
        { {-1, 1,-1},{ 1, 1,-1},{ 1, 1, 1},{-1, 1, 1} },   // +Y
        { {-1,-1, 1},{ 1,-1, 1},{ 1,-1,-1},{-1,-1,-1} },   // -Y
    };
    const float uv[4][2] = { {0,1},{1,1},{1,0},{0,0} };
    for (int f = 0; f < 6; ++f)
        for (int k = 0; k < 4; ++k)
            v[f * 4 + k] = { q[f][k][0], q[f][k][1], q[f][k][2], n[f][0], n[f][1], n[f][2], uv[k][0], uv[k][1] };
    for (int f = 0; f < 6; ++f) {
        idx[f * 6 + 0] = (uint16_t)(f * 4 + 0); idx[f * 6 + 1] = (uint16_t)(f * 4 + 2); idx[f * 6 + 2] = (uint16_t)(f * 4 + 1);
        idx[f * 6 + 3] = (uint16_t)(f * 4 + 0); idx[f * 6 + 4] = (uint16_t)(f * 4 + 3); idx[f * 6 + 5] = (uint16_t)(f * 4 + 2);
    }
    return gfx::createMesh(v, 24, idx, 36);
}
int makeFloor() {   // a thin slab the grid lines sit on
    gfx::MeshVertex v[4] = { { -8, 0, -8, 0, 1, 0, 0, 0 }, { 8, 0, -8, 0, 1, 0, 1, 0 },
                             { 8, 0, 8, 0, 1, 0, 1, 1 },   { -8, 0, 8, 0, 1, 0, 0, 1 } };
    uint16_t idx[6] = { 0, 2, 1, 0, 3, 2 };
    return gfx::createMesh(v, 4, idx, 6);
}

void Init() {
    if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db");
    if (g_fDF    == 0) g_fDF    = LoadFont("assets/fonts/dfsoge7.ttc");
    if (g_cube < 0)  g_cube  = makeCube();
    if (g_floor < 0) g_floor = makeFloor();
    if (!g_model.ok) {   // the RaCo content slot: .glb preferred, .gltf fallback
        g_model = gltf::Load("assets/viewport/sample.glb");
        if (!g_model.ok) g_model = gltf::Load("assets/viewport/sample.gltf");
    }
}
void Reset() { g_orbit = 35.0f; g_dist = 9.0f; }
void Input(const ScreenInput& in) {
    if (in.left)  g_orbit -= 12.0f;
    if (in.right) g_orbit += 12.0f;
    if (in.up)    g_dist = std::max(4.0f, g_dist - 1.0f);
    if (in.down)  g_dist = std::min(16.0f, g_dist + 1.0f);
}

void Submit(int mesh, const float* model, float r, float g, float b, int tex,
            const float* view, const float* proj) {
    gfx::MeshDraw d;
    d.mesh = mesh;
    float vp[16]; matMul(vp, view, proj);
    matMul(d.mvp, model, vp);
    memcpy(d.model, model, sizeof d.model);
    d.lightDir[0] = 0.45f; d.lightDir[1] = 0.8f; d.lightDir[2] = -0.35f; d.lightDir[3] = 0.30f;
    d.baseColor[0] = r; d.baseColor[1] = g; d.baseColor[2] = b; d.baseColor[3] = 1.0f;
    d.texIndex = tex;
    gfx::drawMesh(d);
}

void Draw(double openSec) {
    const double now = Now();
    const float a = (float)ComputeMotion(openSec, 0.0, 10.0);

    // ---- camera: slow auto-orbit + the user's orbit/zoom offsets ----
    const float yaw = (g_orbit + (float)(now * 8.0)) * 0.0174533f;
    const float ex = std::sin(yaw) * g_dist, ez = -std::cos(yaw) * g_dist, ey = 4.6f;
    float view[16], proj[16];
    matLookAt(view, ex, ey, ez, 0, 0.8f, 0);
    matPerspective(proj, 0.9f, 1280.0f / 720.0f, 0.1f, 100.0f);

    // ---- scene: floor slab + grid bars + three reference cubes ----
    float m[16], t[16], s[16];
    matScale(s, 1, 1, 1); matTranslate(t, 0, -0.06f, 0); matMul(m, s, t);
    Submit(g_floor, m, 0.16f, 0.19f, 0.23f, -1, view, proj);
    for (int i = -4; i <= 4; ++i) {   // grid bars as thin stretched cubes
        matScale(s, 0.012f, 0.012f, 8.0f); matTranslate(t, i * 2.0f, 0.0f, 0); matMul(m, s, t);
        Submit(g_cube, m, 0.05f, 0.42f, 0.10f, -1, view, proj);
        matScale(s, 8.0f, 0.012f, 0.012f); matTranslate(t, 0, 0.0f, i * 2.0f); matMul(m, s, t);
        Submit(g_cube, m, 0.05f, 0.42f, 0.10f, -1, view, proj);
    }
    if (g_model.ok) {
        // RaCo glTF content: auto-fit the bbox to ~3.5 units, slow turntable
        const float cx = (g_model.bboxMin[0] + g_model.bboxMax[0]) * 0.5f;
        const float cyy = (g_model.bboxMin[1] + g_model.bboxMax[1]) * 0.5f;
        const float cz = (g_model.bboxMin[2] + g_model.bboxMax[2]) * 0.5f;
        const float ext = std::max({ g_model.bboxMax[0] - g_model.bboxMin[0],
                                     g_model.bboxMax[1] - g_model.bboxMin[1],
                                     g_model.bboxMax[2] - g_model.bboxMin[2], 0.001f });
        const float fit = 3.5f / ext;
        float ctr[16], rot[16], lift[16];
        matTranslate(ctr, -cx, -cyy, -cz);
        matRotY(rot, (float)(now * 0.5));
        matScale(s, fit, fit, fit);
        matTranslate(lift, 0, 1.4f, 0);
        matMul(m, ctr, s); matMul(m, m, rot); matMul(m, m, lift);
        for (const auto& p : g_model.prims) {
            gfx::MeshDraw d;
            d.mesh = p.mesh;
            float vp[16]; matMul(vp, view, proj);
            matMul(d.mvp, m, vp);
            memcpy(d.model, m, sizeof d.model);
            d.lightDir[0] = 0.45f; d.lightDir[1] = 0.8f; d.lightDir[2] = -0.35f; d.lightDir[3] = 0.30f;
            memcpy(d.baseColor, p.baseColor, sizeof d.baseColor);
            d.texIndex = p.texIndex;
            gfx::drawMesh(d);
        }
    } else {   // demo scene when no model is supplied
        float rot[16];
        matRotY(rot, (float)(now * 0.6));
        matScale(s, 1.0f, 1.0f, 1.0f); matTranslate(t, 0, 1.0f, 0);
        matMul(m, s, rot); matMul(m, m, t);
        Submit(g_cube, m, 0.22f, 0.45f, 0.85f, -1, view, proj);
        matScale(s, 0.5f, 0.5f, 0.5f); matTranslate(t, 3.2f, 0.5f, 1.4f); matMul(m, s, t);
        Submit(g_cube, m, 0.86f, 0.32f, 0.22f, -1, view, proj);
        matScale(s, 0.5f, 1.4f, 0.5f); matTranslate(t, -2.8f, 1.4f, -1.8f); matMul(m, s, t);
        Submit(g_cube, m, 0.92f, 0.78f, 0.25f, -1, view, proj);
    }

    // ---- UI overlay (proves 3D composes under the quad pass) ----
    SetFont(g_fDF);
    DrawTextBevel({ 30, 26 }, 34.0f, WithAlpha(RGBA(255, 190, 33, 255), a), "3D VIEWPORT");
    SetFont(g_fRodin);
    DrawText({ 30, 70 }, 18.0f, WithAlpha(RGBA(180, 200, 220, 255), a),
             "plume mesh pass: depth + perspective + directional light");
    DrawTextAligned({ 0, 676 }, { REF_W, 706 }, 20.0f, WithAlpha(RGBA(224, 238, 226, 255), a),
                    g_model.ok ? "[Left/Right] Orbit   [Up/Down] Zoom   -   glTF: assets/viewport/sample"
                               : "[Left/Right] Orbit   [Up/Down] Zoom   -   drop a model at assets/viewport/sample.glb",
                    Align::Center, true, true);
    ResetFont();
}

} // namespace

void Viewport3DInit() { Init(); }
void Viewport3DDraw(double openSeconds) { Draw(openSeconds); }
void Viewport3DInput(const ScreenInput& in) { Input(in); }
void Viewport3DReset() { Reset(); }
