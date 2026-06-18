// =============================================================================
// gfx_d3d12.h — the game's exact GPU path for sgfx_ui.
//
// A standalone D3D12 backend built on the recomp's own plume RHI, driving the
// real Sonic Unleashed CSD render path: the actual csd_vs / csd_filter_ps DXIL
// shaders, the CSD vertex format (per-vertex BGRA color), the alpha/additive
// blend states, and MSAA with a hardware resolve — i.e. pixel-for-pixel the
// pipeline the game uses, not an SDL approximation.
//
// The anim engine (main.cpp) stays renderer-agnostic: it evaluates the CSD
// keyframes and hands this backend a flat list of textured, per-vertex-colored
// quads each frame.
// =============================================================================
#pragma once
#include <cstdint>
#include <string>

namespace gfx {

enum class Blend { Alpha, Additive };

// One CSD quad: 4 corners in 1280x720 reference-pixel space (the csd_vs maps
// these to NDC via g_ViewportSize). Corner order: TL, TR, BR, BL.
struct Quad {
    float px[4], py[4];   // corner positions (reference pixels)
    float u[4],  v[4];    // corner UVs (0..1)
    uint32_t color[4];    // PER-CORNER modulation color (TL,TR,BR,BL), 0xAARRGGBB — bilinear gradient
    int   texIndex;       // bindless texture slot from loadTexture(); -1 = solid (white 1x1)
    Blend blend;
    uint32_t modifier = 0; // ShaderModifier (0=none; 1=scanline 2=checkerboard 3=grayscale) -> csd_modifier_ps
};

// Create the device + (swapchain | offscreen target) + the CSD pipeline.
//   hwnd     : HWND for interactive mode; ignored when headless.
//   headless : render to an offscreen target for readbackRGBA() instead of a window.
//   msaa     : requested sample count (1/2/4/8); clamped to what the GPU supports.
// Returns false only on a hard device/swapchain failure; pipeline/shader issues
// degrade to clear-only so the infrastructure can still be validated.
bool init(void* hwnd, int width, int height, bool headless, int msaa);
void shutdown();

// Load a PNG into the bindless texture heap; returns its slot index (-1 on fail).
int  loadTexture(const std::string& pngPath);
// Upload a tight RGBA buffer (w*h*4, 8-bit straight alpha) into the bindless heap;
// returns its slot index (-1 on fail). Used for the hand-built font atlas and any
// procedurally generated art, so the clean-UI layer needs no PNG round-trip.
int  loadTextureRGBA(const uint8_t* rgba, int w, int h);
void clearTextures();   // drop all loaded textures (on screen switch)

int  sampleCount();     // resolved MSAA sample count actually in use

// ---- the real 3D pass (depth-buffered; the QA-viewport seed) -----------------
// Meshes render UNDER the UI quads each frame: mesh pass (depth on, cleared
// every frame) -> quad pass (no depth). Matrices are row-major float[16] with
// the row-vector convention (pos * M).
struct MeshVertex { float px, py, pz, nx, ny, nz, u, v; };
struct MeshDraw {
    int   mesh = -1;
    float mvp[16];        // model * view * proj
    float model[16];      // for the normal transform
    float lightDir[4];    // xyz = towards the light (normalized), w = ambient
    float baseColor[4];   // multiplies the texture (white when untextured)
    int   texIndex = -1;  // bindless texture slot; -1 = untextured
};
// Upload a static mesh; returns a handle (-1 on failure).
int  createMesh(const MeshVertex* verts, int vertCount, const uint16_t* indices, int indexCount);
// Queue a mesh for this frame (call during the screen's Draw; consumed by endFrame).
void drawMesh(const MeshDraw& draw);

// Per-frame: beginFrame(clear) -> drawQuads(...) (any number) -> endFrame().
void beginFrame(float r, float g, float b, float a);
void drawQuads(const Quad* quads, int count);
void endFrame();        // record, submit, then resolve+present (or resolve+copy when headless)

// Headless: copy the resolved frame into a tight RGBA buffer (w*h*4). Valid after endFrame().
bool readbackRGBA(uint8_t* outRGBA, int w, int h);

} // namespace gfx
