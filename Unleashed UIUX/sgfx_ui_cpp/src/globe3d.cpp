// =============================================================================
// globe3d.cpp — see globe3d.h. The first real-3D pass of the project: the
// sphere is tessellated on the CPU (28 longitude x 18 latitude patches),
// rotated (spin about the tilted axis), perspective-projected and lit per
// patch, then emitted through ui::DrawImageQuad — the same quad stream every
// 2D screen uses, so it composes under/over any UI layer.
// =============================================================================
#include "sgfxui.h"
#include "globe3d.h"
#include <cmath>
#include <algorithm>
#include <vector>

namespace ui {
namespace {

constexpr int   SEG_LON = 28, SEG_LAT = 18;
constexpr float TILT = 0.40f;                  // ~23 deg axial tilt (radians)
constexpr float CAM_D = 4.0f;                  // camera distance in radii

int g_earthTex = -2;                            // -2 = not probed yet
int g_glowTex = -2;                             // procedural radial halo (built once)

int buildGlowTexture() {
    // 128x128 radial intensity map: peak at the limb (d ~0.74), soft falloff
    // outwards — drawn ADDITIVELY, so RGB carries the intensity.
    const int N = 128;
    std::vector<uint8_t> px((size_t)N * N * 4, 0);
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            float dx = (x + 0.5f) / N * 2.0f - 1.0f, dy = (y + 0.5f) / N * 2.0f - 1.0f;
            float d = std::sqrt(dx * dx + dy * dy);
            float rim = std::exp(-((d - 0.74f) * (d - 0.74f)) / (0.018f));     // limb band
            float halo = (d > 0.74f) ? std::exp(-(d - 0.74f) * 7.5f) : rim;    // outer fade
            float v = std::max(rim, halo) * ((d > 1.0f) ? std::max(0.0f, 1.3f - d) * 2.0f : 1.0f);
            v = std::clamp(v, 0.0f, 1.0f);
            uint8_t* p = &px[((size_t)y * N + x) * 4];
            p[0] = (uint8_t)(120 * v); p[1] = (uint8_t)(180 * v); p[2] = (uint8_t)(255 * v); p[3] = 255;
        }
    }
    return gfx::loadTextureRGBA(px.data(), N, N);
}

struct V3 { float x, y, z; };

V3 spherePoint(float lon, float lat) {          // radians; r=1
    float cl = std::cos(lat);
    return { cl * std::sin(lon), std::sin(lat), cl * std::cos(lon) };
}
V3 rotate(const V3& p, float yaw) {             // spin about Y, then tilt about X
    float cy = std::cos(yaw), sy = std::sin(yaw);
    V3 a = { p.x * cy + p.z * sy, p.y, -p.x * sy + p.z * cy };
    float ct = std::cos(TILT), st = std::sin(TILT);
    return { a.x, a.y * ct - a.z * st, a.y * st + a.z * ct };
}

// procedural blob continents (placeholder when no Earth texture is supplied):
// land where the angular distance to any blob centre is under its radius.
struct Blob { float lon, lat, rad; };
const Blob BLOBS[] = {
    { -1.7f,  0.70f, 0.40f },   // "north america"
    { -1.0f, -0.35f, 0.30f },   // "south america"
    {  0.30f, 0.62f, 0.30f },   // "europe"
    {  0.45f, 0.00f, 0.40f },   // "africa"
    {  1.55f, 0.58f, 0.48f },   // "asia"
    {  2.35f, -0.45f, 0.24f },  // "oceania"
};
int surfaceClass(const V3& p) {                 // 0 = ocean, 1 = land, 2 = ice
    float lat = std::asin(std::clamp(p.y, -1.0f, 1.0f));
    float lon = std::atan2(p.x, p.z);
    if (std::fabs(lat) > 1.30f) return 2;       // polar caps
    for (const Blob& b : BLOBS) {
        float dlon = lon - b.lon;
        while (dlon > 3.14159265f) dlon -= 6.2831853f;
        while (dlon < -3.14159265f) dlon += 6.2831853f;
        float dx = dlon * std::cos((lat + b.lat) * 0.5f);
        if (dx * dx + (lat - b.lat) * (lat - b.lat) < b.rad * b.rad) return 1;
    }
    return 0;
}

uint32_t shade(int r, int g, int b, float l, float a) {
    return RGBA(int(r * l), int(g * l), int(b * l), int(255 * a));
}

} // namespace

void DrawGlobe3D(float cx, float cy, float r, float yawDeg,
                 float sunX, float sunY, float sunZ,
                 const GlobeMarker* markers, int markerCount, float alpha) {
    if (g_earthTex == -2) g_earthTex = gfx::loadTexture("assets/globe/earth.png");
    if (g_glowTex == -2) g_glowTex = buildGlowTexture();
    const float yaw = yawDeg * 0.0174533f;
    float sl = std::sqrt(sunX * sunX + sunY * sunY + sunZ * sunZ);
    const float sx = sunX / sl, sy = sunY / sl, sz = sunZ / sl;

    // atmosphere: an additive radial halo hugging the limb (procedural texture)
    if (g_glowTex >= 0) {
        const float gr = r * 1.32f;
        DrawImage(g_glowTex, { cx - gr, cy - gr }, { cx + gr, cy + gr },
                  { 0, 0 }, { 1, 1 }, WithAlpha(RGBA(255, 255, 255, 255), alpha * 0.85f), true);
    }

    (void)0;   // patch colours are shaded per-face below
    for (int j = 0; j < SEG_LAT; ++j) {
        float lat0 = -1.5707963f + 3.14159265f * j / SEG_LAT;
        float lat1 = -1.5707963f + 3.14159265f * (j + 1) / SEG_LAT;
        for (int i = 0; i < SEG_LON; ++i) {
            float lon0 = 6.2831853f * i / SEG_LON;
            float lon1 = 6.2831853f * (i + 1) / SEG_LON;
            V3 p[4] = { rotate(spherePoint(lon0, lat1), yaw), rotate(spherePoint(lon1, lat1), yaw),
                        rotate(spherePoint(lon1, lat0), yaw), rotate(spherePoint(lon0, lat0), yaw) };
            // backface cull on the projected winding (front faces wind CW here)
            V2 q[4];
            for (int k = 0; k < 4; ++k) {
                float persp = CAM_D / (CAM_D - p[k].z);
                q[k] = { cx + p[k].x * r * persp, cy - p[k].y * r * persp };
            }
            float cross = (q[1].x - q[0].x) * (q[3].y - q[0].y) - (q[1].y - q[0].y) * (q[3].x - q[0].x);
            if (cross <= 0.0f) continue;
            // Lambert from the patch-centre normal
            V3 n = rotate(spherePoint((lon0 + lon1) * 0.5f, (lat0 + lat1) * 0.5f), yaw);
            float ndl = std::max(0.0f, n.x * sx + n.y * sy + n.z * sz);
            float l = 0.22f + 0.78f * ndl;
            if (g_earthTex >= 0) {
                // the user-supplied equirect texture (the real SEGA globe art slot)
                const V2 uv[4] = { { lon0 / 6.2831853f, 0.5f - lat1 / 3.14159265f },
                                   { lon1 / 6.2831853f, 0.5f - lat1 / 3.14159265f },
                                   { lon1 / 6.2831853f, 0.5f - lat0 / 3.14159265f },
                                   { lon0 / 6.2831853f, 0.5f - lat0 / 3.14159265f } };
                DrawImageQuad(g_earthTex, q, uv, shade(255, 255, 255, l, alpha), false);
            } else {
                const V2 uv[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
                int sc = surfaceClass(n);
                uint32_t col = (sc == 2) ? shade(222, 230, 236, l, alpha)
                             : (sc == 1) ? shade(96, 138, 92, l, alpha)
                                         : shade(56, 110, 178, l, alpha);
                DrawImageQuad(-1, q, uv, col, false);
            }
        }
    }

    // markers (stage dots etc.): anchored to lat/lon, culled on the far side
    for (int m = 0; m < markerCount; ++m) {
        V3 p = rotate(spherePoint(markers[m].lonDeg * 0.0174533f, markers[m].latDeg * 0.0174533f), yaw);
        if (p.z < 0.10f) continue;
        float persp = CAM_D / (CAM_D - p.z);
        float mx = cx + p.x * r * persp, my = cy - p.y * r * persp;
        float s = markers[m].size * (0.7f + 0.3f * p.z);
        DrawRect({ mx - s, my - s }, { mx + s, my + s }, WithAlpha(RGBA(0, 0, 0, 160), alpha));
        DrawRect({ mx - s + 1.5f, my - s + 1.5f }, { mx + s - 1.5f, my + s - 1.5f },
                 WithAlpha(markers[m].color, alpha));
    }

    // (day-side sheen likewise waits for the radial-glow texture slot)
}

} // namespace ui
