// =============================================================================
// globe3d.h — the rotating 3D Earth (the World Map hub / boot title globe).
// Real 3D geometry: a CPU-tessellated lat/long sphere, axial-tilted, spun by
// yawDeg, perspective-projected, backface-culled and Lambert-lit, emitted as
// textured quads through the existing gfx::Quad pipeline (a convex body needs
// no depth buffer). The Earth surface is an ART SLOT: drop an equirectangular
// texture at assets/globe/earth.png and it is sampled per patch; without it a
// procedural blob-continent placeholder renders (engineering stand-in only).
// =============================================================================
#pragma once

namespace ui {

struct GlobeMarker { float lonDeg, latDeg; unsigned color; float size; };

// cx,cy,r in 1280x720 reference px. sunDir need not be normalized.
void DrawGlobe3D(float cx, float cy, float r, float yawDeg,
                 float sunX = 0.55f, float sunY = 0.45f, float sunZ = 0.7f,
                 const GlobeMarker* markers = nullptr, int markerCount = 0,
                 float alpha = 1.0f);

} // namespace ui
