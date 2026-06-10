// =============================================================================
// gfx_d3d12.cpp — the game's exact CSD GPU path, standalone, via the plume RHI.
//
// Replicates UnleashedRecomp's SurfRide/CSD draw: the real csd_vs + csd_filter_ps
// DXIL, the CSD vertex format (POSITION0 float2 @0, COLOR0 BGRA8 @8, TEXCOORD0
// float2 @12; stride 20), the bindless root signature (Texture2D[] t0/space0,
// Sampler[] s0/space3, root CBVs b0/b1/b2 space4), alpha + additive blends, and
// MSAA with a hardware resolve. See tools/wf csd-d3d12-pipeline-map for the spec.
// =============================================================================
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "gfx_d3d12.h"

#include "plume_render_interface.h"
#include "plume_render_interface_builders.h"

#include "csd_vs.hlsl.dxil.h"          // g_csd_vs_dxil
#include "csd_filter_ps.hlsl.dxil.h"   // g_csd_filter_ps_dxil
#include "csd_modifier_ps.hlsl.dxil.h" // g_csd_modifier_ps_dxil (csd_filter_ps + per-quad ShaderModifier)

#include "stb_image.h"                 // declarations only (impl lives in main.cpp)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

using namespace plume;

// CreateD3D12Interface is defined in plume_d3d12.cpp but not declared in a header.
namespace plume { std::unique_ptr<RenderInterface> CreateD3D12Interface(); }

namespace {

constexpr RenderFormat RT_FMT  = RenderFormat::B8G8R8A8_UNORM;  // matches BACKBUFFER_FORMAT
constexpr RenderFormat TEX_FMT = RenderFormat::R8G8B8A8_UNORM;  // stb gives tight RGBA
constexpr uint32_t MAX_TEX   = 1024;
constexpr uint32_t MAX_SAMP  = 4;
constexpr uint32_t MAX_QUADS = 4096;
constexpr uint32_t SHARED_STRIDE = 512;   // >=276 bytes of SharedConstants, 256-aligned
constexpr uint32_t VS_CB_SIZE = 4096;     // guest VertexShaderConstants file (0x400 dwords)

// CSD textured vertex: stride 20 — POSITION0 float2, COLOR0 BGRA8, TEXCOORD0 float2.
struct Vtx { float x, y; uint32_t color; float u, v; };

struct State {
    int w = 0, h = 0;
    bool headless = false;
    uint32_t samples = 1;
    bool canDraw = false;

    std::unique_ptr<RenderInterface> iface;
    std::unique_ptr<RenderDevice>    device;
    std::unique_ptr<RenderCommandQueue> queue;
    std::unique_ptr<RenderCommandList>  cmd;
    std::unique_ptr<RenderCommandFence> fence;

    std::unique_ptr<RenderSwapChain> swap;
    std::unique_ptr<RenderCommandSemaphore> acquireSem, renderSem;

    std::unique_ptr<RenderTexture>   msaa;        // multisample color target (CSD draws here)
    std::unique_ptr<RenderFramebuffer> msaaFB;
    std::unique_ptr<RenderTexture>   resolveTex;  // headless single-sample resolve dest
    std::unique_ptr<RenderBuffer>    readback;    // headless CPU-readable copy
    uint32_t readbackRowBytes = 0;

    std::unique_ptr<RenderPipelineLayout> layout;
    uint32_t setTex = 0, setSamp = 3, rdVS = 0, rdPS = 1, rdShared = 2;
    std::unique_ptr<RenderDescriptorSet> texSet, sampSet;
    std::unique_ptr<RenderShader> vs, ps;
    std::unique_ptr<RenderPipeline> pAlpha, pAdditive;
    std::unique_ptr<RenderSampler> sampler;

    std::unique_ptr<RenderBuffer> vsCB, psCB, sharedCB, vb, ib;
    void *sharedPtr = nullptr, *vbPtr = nullptr, *ibPtr = nullptr;

    std::vector<std::unique_ptr<RenderTexture>>     textures;   // slot 0 = 1x1 white
    std::vector<std::unique_ptr<RenderTextureView>> texViews;

    std::vector<gfx::Quad> pending;
    float clear[4] = {0.04f, 0.05f, 0.06f, 1.0f};
} S;

bool g_dbg = false;   // set from env SGFX_DEBUG at init
#define DBG(...) do{ if(g_dbg){ fprintf(stderr,"[gfx] " __VA_ARGS__); fputc('\n',stderr); fflush(stderr);} }while(0)

uint32_t alignUp(uint32_t v, uint32_t a) { return (v + a - 1) & ~(a - 1); }

// Pack 0xAARRGGBB so that, after a B8G8R8A8_UNORM load + the csd_vs .wxyz swizzle,
// the pixel shader sees (R,G,B,A) correctly (derivation verified against csd_vs.hlsl).
uint32_t packColor(uint32_t argb) {
    uint32_t a = (argb >> 24) & 0xFF, r = (argb >> 16) & 0xFF,
             g = (argb >> 8) & 0xFF,  b = argb & 0xFF;
    return (r << 24) | (g << 16) | (b << 8) | a;
}

// Upload pixels (tight RGBA, w*h*4) into a fresh sampled Texture2D; returns slot.
int uploadTexture(const uint8_t* rgba, int w, int h) {
    auto tex = S.device->createTexture(RenderTextureDesc::Texture2D(w, h, 1, TEX_FMT));
    uint32_t rowBytes = alignUp((uint32_t)w * 4, 256);
    auto up = S.device->createBuffer(RenderBufferDesc::UploadBuffer((uint64_t)rowBytes * h));
    if (uint8_t* dst = (uint8_t*)up->map()) {
        for (int y = 0; y < h; ++y) memcpy(dst + (size_t)y * rowBytes, rgba + (size_t)y * w * 4, (size_t)w * 4);
        up->unmap();
    }
    S.cmd->begin();
    S.cmd->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(tex.get(), RenderTextureLayout::COPY_DEST));
    S.cmd->copyTextureRegion(RenderTextureCopyLocation::Subresource(tex.get(), 0),
                             RenderTextureCopyLocation::PlacedFootprint(up.get(), TEX_FMT, w, h, 1, rowBytes / 4, 0));
    S.cmd->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(tex.get(), RenderTextureLayout::SHADER_READ));
    S.cmd->end();
    S.queue->executeCommandLists(S.cmd.get(), S.fence.get());
    S.queue->waitForCommandFence(S.fence.get());

    int slot = (int)S.textures.size();
    auto view = tex->createTextureView(RenderTextureViewDesc::Texture2D(TEX_FMT));
    if (S.texSet) S.texSet->setTexture(slot, tex.get(), RenderTextureLayout::SHADER_READ, view.get());
    S.textures.push_back(std::move(tex));
    S.texViews.push_back(std::move(view));
    return slot;
}

bool buildPipeline() {
    // --- bindless root signature: sets 0..3 map to register spaces 0..3 ---
    RenderDescriptorSetBuilder texB;  texB.begin();  texB.addTexture(0, MAX_TEX);   texB.end(true, MAX_TEX);
    RenderDescriptorSetBuilder d1;    d1.begin();    d1.addTexture(0, 1);           d1.end(true, 1);   // space1 (unused)
    RenderDescriptorSetBuilder d2;    d2.begin();    d2.addTexture(0, 1);           d2.end(true, 1);   // space2 (unused)
    RenderDescriptorSetBuilder sampB; sampB.begin(); sampB.addSampler(0, MAX_SAMP); sampB.end(true, MAX_SAMP);

    RenderPipelineLayoutBuilder lb; lb.begin(false, true);
    S.setTex  = lb.addDescriptorSet(texB);
    lb.addDescriptorSet(d1);
    lb.addDescriptorSet(d2);
    S.setSamp = lb.addDescriptorSet(sampB);
    S.rdVS     = lb.addRootDescriptor(0, 4, RenderRootDescriptorType::CONSTANT_BUFFER); // b0 space4
    S.rdPS     = lb.addRootDescriptor(1, 4, RenderRootDescriptorType::CONSTANT_BUFFER); // b1 space4
    S.rdShared = lb.addRootDescriptor(2, 4, RenderRootDescriptorType::CONSTANT_BUFFER); // b2 space4
    lb.end();
    S.layout = lb.create(S.device.get());
    if (!S.layout) { fprintf(stderr, "[gfx] pipeline layout failed\n"); return false; }
    S.texSet  = texB.create(S.device.get());
    S.sampSet = sampB.create(S.device.get());

    S.vs = S.device->createShader(g_csd_vs_dxil, sizeof(g_csd_vs_dxil), "main", RenderShaderFormat::DXIL);
    S.ps = S.device->createShader(g_csd_modifier_ps_dxil, sizeof(g_csd_modifier_ps_dxil), "main", RenderShaderFormat::DXIL);
    if (!S.vs || !S.ps) { fprintf(stderr, "[gfx] shader create failed\n"); return false; }

    RenderInputElement elems[] = {
        RenderInputElement("POSITION", 0, 0, RenderFormat::R32G32_FLOAT,   0, 0),
        RenderInputElement("COLOR",    0, 8, RenderFormat::B8G8R8A8_UNORM, 0, 8),
        RenderInputElement("TEXCOORD", 0, 4, RenderFormat::R32G32_FLOAT,   0, 12),
    };
    RenderInputSlot slot(0, sizeof(Vtx));

    RenderGraphicsPipelineDesc pd;
    pd.pipelineLayout = S.layout.get();
    pd.vertexShader = S.vs.get();
    pd.pixelShader = S.ps.get();
    pd.renderTargetFormat[0] = RT_FMT;
    pd.renderTargetCount = 1;
    pd.renderTargetBlend[0] = RenderBlendDesc::AlphaBlend();
    pd.multisampling.sampleCount = S.samples;
    pd.depthEnabled = false;
    pd.cullMode = RenderCullMode::NONE;
    pd.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
    pd.inputElements = elems; pd.inputElementsCount = 3;
    pd.inputSlots = &slot;    pd.inputSlotsCount = 1;
    S.pAlpha = S.device->createGraphicsPipeline(pd);
    pd.renderTargetBlend[0].dstBlend = RenderBlend::ONE;  // additive
    S.pAdditive = S.device->createGraphicsPipeline(pd);
    if (!S.pAlpha || !S.pAdditive) { fprintf(stderr, "[gfx] graphics pipeline failed\n"); return false; }

    S.sampler = S.device->createSampler(RenderSamplerDesc{});
    S.sampSet->setSampler(0, S.sampler.get());

    // constant buffers
    S.vsCB     = S.device->createBuffer(RenderBufferDesc::UploadBuffer(VS_CB_SIZE, RenderBufferFlag::CONSTANT));
    S.psCB     = S.device->createBuffer(RenderBufferDesc::UploadBuffer(256, RenderBufferFlag::CONSTANT));
    S.sharedCB = S.device->createBuffer(RenderBufferDesc::UploadBuffer((uint64_t)SHARED_STRIDE * MAX_QUADS, RenderBufferFlag::CONSTANT));
    if (uint8_t* p = (uint8_t*)S.vsCB->map()) {
        memset(p, 0, VS_CB_SIZE);
        float vp[4] = { (float)1280.0f, 720.0f, 1.0f / 1280.0f, 1.0f / 720.0f };  // g_ViewportSize @ c180 (byte 2880)
        memcpy(p + 2880, vp, sizeof(vp));
        float z[4] = { 0, 0, 0, 0 };                                              // g_Z @ c246 (byte 3936)
        memcpy(p + 3936, z, sizeof(z));
        S.vsCB->unmap();
    }
    if (uint8_t* p = (uint8_t*)S.psCB->map()) { memset(p, 0, 256); S.psCB->unmap(); }
    S.sharedPtr = S.sharedCB->map();
    if (S.sharedPtr) memset(S.sharedPtr, 0, (size_t)SHARED_STRIDE * MAX_QUADS);

    // dynamic vertex / index buffers (persistently mapped UPLOAD heap)
    S.vb = S.device->createBuffer(RenderBufferDesc::UploadBuffer((uint64_t)sizeof(Vtx) * 4 * MAX_QUADS, RenderBufferFlag::VERTEX));
    S.ib = S.device->createBuffer(RenderBufferDesc::UploadBuffer((uint64_t)sizeof(uint16_t) * 6 * MAX_QUADS, RenderBufferFlag::INDEX));
    S.vbPtr = S.vb->map();
    S.ibPtr = S.ib->map();
    return true;
}

// Resolve (or copy when 1x) the MSAA target into `dst`, leaving dst in `after`.
void resolveInto(RenderTexture* dst, RenderTextureLayout after) {
    if (S.samples > 1) {
        RenderTextureBarrier pre[2] = { RenderTextureBarrier(S.msaa.get(), RenderTextureLayout::RESOLVE_SOURCE),
                                        RenderTextureBarrier(dst,          RenderTextureLayout::RESOLVE_DEST) };
        S.cmd->barriers(RenderBarrierStage::GRAPHICS, pre, 2);
        DBG("  resolveTexture");
        S.cmd->resolveTexture(dst, S.msaa.get());
    } else {
        RenderTextureBarrier pre[2] = { RenderTextureBarrier(S.msaa.get(), RenderTextureLayout::COPY_SOURCE),
                                        RenderTextureBarrier(dst,          RenderTextureLayout::COPY_DEST) };
        S.cmd->barriers(RenderBarrierStage::COPY, pre, 2);
        DBG("  copyTexture");
        S.cmd->copyTexture(dst, S.msaa.get());
    }
    RenderTextureBarrier post(dst, after);
    S.cmd->barriers(RenderBarrierStage::GRAPHICS, &post, 1);
    DBG("  resolveInto done");
}

} // namespace

namespace gfx {

bool init(void* hwnd, int width, int height, bool headless, int msaa) {
    S.w = width; S.h = height; S.headless = headless;
    g_dbg = (getenv("SGFX_DEBUG") != nullptr);
    DBG("CreateD3D12Interface...");
    S.iface = plume::CreateD3D12Interface();
    if (!S.iface) { fprintf(stderr, "[gfx] CreateD3D12Interface failed\n"); return false; }
    DBG("createDevice...");
    S.device = S.iface->createDevice();
    if (!S.device) { fprintf(stderr, "[gfx] createDevice failed\n"); return false; }
    DBG("queue/list/fence...");
    S.queue = S.device->createCommandQueue(RenderCommandListType::DIRECT);
    S.cmd   = S.device->createCommandList(RenderCommandListType::DIRECT);
    S.fence = S.device->createCommandFence();

    // pick the MSAA level the GPU actually supports
    DBG("getSampleCountsSupported...");
    RenderSampleCounts sup = S.device->getSampleCountsSupported(RT_FMT);
    uint32_t want = (uint32_t)msaa;
    S.samples = 1;
    for (uint32_t c : {8u, 4u, 2u}) if (c <= want && (sup & c)) { S.samples = c; break; }
    DBG("samples=%u", S.samples);

    RenderClearValue cv = RenderClearValue::Color(RenderColor(0, 0, 0, 1), RT_FMT);
    DBG("create msaa target...");
    S.msaa = S.device->createTexture(RenderTextureDesc::ColorTarget(width, height, RT_FMT, RenderMultisampling(S.samples), &cv));
    const RenderTexture* fbColor[1] = { S.msaa.get() };
    DBG("create framebuffer...");
    S.msaaFB = S.device->createFramebuffer(RenderFramebufferDesc(fbColor, 1));

    DBG("targets...");
    if (headless) {
        S.resolveTex = S.device->createTexture(RenderTextureDesc::ColorTarget(width, height, RT_FMT));
        S.readbackRowBytes = alignUp((uint32_t)width * 4, 256);
        S.readback = S.device->createBuffer(RenderBufferDesc::ReadbackBuffer((uint64_t)S.readbackRowBytes * height));
    } else {
        S.swap = S.queue->createSwapChain((RenderWindow)hwnd, 2, RT_FMT, 1);
        S.acquireSem = S.device->createCommandSemaphore();
        S.renderSem  = S.device->createCommandSemaphore();
    }

    DBG("buildPipeline...");
    S.canDraw = buildPipeline();
    if (!S.canDraw) fprintf(stderr, "[gfx] pipeline unavailable — clear-only mode\n");
    DBG("buildPipeline done canDraw=%d", (int)S.canDraw);

    // slot 0 = 1x1 opaque white (for untextured / solid quads)
    if (S.canDraw) { const uint8_t white[4] = {255,255,255,255}; uploadTexture(white, 1, 1); }
    DBG("white texture done");

    printf("[gfx] D3D12 ready: %dx%d MSAA x%u%s draw=%d\n",
           width, height, S.samples, headless ? " (headless)" : "", (int)S.canDraw);
    return true;
}

void clearTextures() {
    S.textures.clear(); S.texViews.clear();
    if (S.canDraw) { const uint8_t white[4] = {255,255,255,255}; uploadTexture(white, 1, 1); }
}

int loadTexture(const std::string& path) {
    if (!S.canDraw) return -1;
    int w, h, ch; unsigned char* px = stbi_load(path.c_str(), &w, &h, &ch, 4);
    if (!px) return -1;
    int slot = uploadTexture(px, w, h);
    stbi_image_free(px);
    return slot;
}

int loadTextureRGBA(const uint8_t* rgba, int w, int h) {
    if (!S.canDraw || !rgba || w <= 0 || h <= 0) return -1;
    return uploadTexture(rgba, w, h);
}

int sampleCount() { return (int)S.samples; }

void beginFrame(float r, float g, float b, float a) {
    S.clear[0]=r; S.clear[1]=g; S.clear[2]=b; S.clear[3]=a;
    S.pending.clear();
}

void drawQuads(const Quad* quads, int count) {
    for (int i = 0; i < count && S.pending.size() < MAX_QUADS; ++i) S.pending.push_back(quads[i]);
}

void endFrame() {
    DBG("endFrame: %zu quads", S.pending.size());
    uint32_t backIdx = 0;
    RenderTexture* backTex = nullptr;
    if (!S.headless) {
        if (S.swap->needsResize()) { S.device->waitIdle(); S.swap->resize(); }
        if (!S.swap->acquireTexture(S.acquireSem.get(), &backIdx)) return;
        backTex = S.swap->getTexture(backIdx);
    }

    // fill vertex/index/shared buffers from the pending quads
    uint32_t n = (uint32_t)S.pending.size();
    if (S.canDraw && n) {
        Vtx* vtx = (Vtx*)S.vbPtr; uint16_t* idx = (uint16_t*)S.ibPtr; uint8_t* sh = (uint8_t*)S.sharedPtr;
        for (uint32_t i = 0; i < n; ++i) {
            const Quad& q = S.pending[i];
            for (int k = 0; k < 4; ++k) vtx[i*4+k] = { q.px[k], q.py[k], packColor(q.color[k]), q.u[k], q.v[k] };
            idx[i*6+0]=i*4+0; idx[i*6+1]=i*4+1; idx[i*6+2]=i*4+2;
            idx[i*6+3]=i*4+0; idx[i*6+4]=i*4+2; idx[i*6+5]=i*4+3;
            uint32_t texIndex = (q.texIndex < 0 || q.texIndex >= (int)S.textures.size()) ? 0u : (uint32_t)q.texIndex;
            uint8_t* e = sh + (size_t)i * SHARED_STRIDE;
            memset(e, 0, SHARED_STRIDE);
            memcpy(e + 0,   &texIndex, 4);   // s0_Texture2DDescriptorIndex (c0.x)
            uint32_t mod = q.modifier; memcpy(e + 16, &mod, 4);  // g_Modifier (c1.x)
            uint32_t samp = 0; memcpy(e + 192, &samp, 4);  // s0_SamplerDescriptorIndex (c12.x)
            // g_AlphaThreshold (c17.x, byte 272) left 0
        }
    }

    DBG("buffers filled, begin record");
    S.cmd->begin();
    S.cmd->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(S.msaa.get(), RenderTextureLayout::COLOR_WRITE));
    S.cmd->setFramebuffer(S.msaaFB.get());
    S.cmd->clearColor(0, RenderColor(S.clear[0], S.clear[1], S.clear[2], S.clear[3]));
    S.cmd->setViewports(RenderViewport(0, 0, (float)S.w, (float)S.h));
    S.cmd->setScissors(RenderRect(0, 0, S.w, S.h));

    if (S.canDraw && n) {
        S.cmd->setGraphicsPipelineLayout(S.layout.get());
        S.cmd->setGraphicsDescriptorSet(S.texSet.get(), S.setTex);
        S.cmd->setGraphicsDescriptorSet(S.sampSet.get(), S.setSamp);
        S.cmd->setGraphicsRootDescriptor(S.vsCB->at(0), S.rdVS);
        S.cmd->setGraphicsRootDescriptor(S.psCB->at(0), S.rdPS);
        RenderVertexBufferView vbv(S.vb->at(0), sizeof(Vtx) * 4 * n);
        RenderInputSlot vslot(0, sizeof(Vtx));
        RenderIndexBufferView ibv(S.ib->at(0), sizeof(uint16_t) * 6 * n, RenderFormat::R16_UINT);
        S.cmd->setVertexBuffers(0, &vbv, 1, &vslot);
        S.cmd->setIndexBuffer(&ibv);
        for (uint32_t i = 0; i < n; ++i) {
            S.cmd->setPipeline((S.pending[i].blend == Blend::Additive ? S.pAdditive : S.pAlpha).get());
            S.cmd->setGraphicsRootDescriptor(S.sharedCB->at((uint64_t)i * SHARED_STRIDE), S.rdShared);
            S.cmd->drawIndexedInstanced(6, 1, i * 6, 0, 0);
        }
    }
    S.cmd->setFramebuffer(nullptr);
    DBG("draws recorded, resolving");

    if (S.headless) {
        resolveInto(S.resolveTex.get(), RenderTextureLayout::COPY_SOURCE);
        DBG("copyTextureRegion -> readback");
        // dst is a buffer footprint; plume's copyTextureRegion derefs dstLocation.texture
        // in setSamplePositions (no null check in release), so point it at a valid
        // single-sample texture — toD3D12 ignores .texture for PLACED_FOOTPRINT.
        auto dst = RenderTextureCopyLocation::PlacedFootprint(S.readback.get(), RT_FMT, S.w, S.h, 1, S.readbackRowBytes / 4, 0);
        dst.texture = S.resolveTex.get();
        S.cmd->copyTextureRegion(dst, RenderTextureCopyLocation::Subresource(S.resolveTex.get(), 0));
        S.cmd->end();
        DBG("submit (headless)");
        S.queue->executeCommandLists(S.cmd.get(), S.fence.get());
        S.queue->waitForCommandFence(S.fence.get());
        DBG("gpu done");
    } else {
        resolveInto(backTex, RenderTextureLayout::PRESENT);
        S.cmd->end();
        RenderCommandSemaphore* waitS[1]   = { S.acquireSem.get() };
        RenderCommandSemaphore* signalS[1] = { S.renderSem.get() };
        const RenderCommandList* cl = S.cmd.get();
        S.queue->executeCommandLists(&cl, 1, waitS, 1, signalS, 1, S.fence.get());
        S.swap->present(backIdx, signalS, 1);
        S.queue->waitForCommandFence(S.fence.get());
    }
}

bool readbackRGBA(uint8_t* out, int w, int h) {
    if (!S.headless || !S.readback) return false;
    uint8_t* src = (uint8_t*)S.readback->map();
    if (!src) return false;
    for (int y = 0; y < h; ++y) {
        const uint8_t* sr = src + (size_t)y * S.readbackRowBytes;
        uint8_t* dr = out + (size_t)y * w * 4;
        for (int x = 0; x < w; ++x) {   // B8G8R8A8 -> RGBA
            dr[x*4+0] = sr[x*4+2]; dr[x*4+1] = sr[x*4+1]; dr[x*4+2] = sr[x*4+0]; dr[x*4+3] = sr[x*4+3];
        }
    }
    S.readback->unmap();
    return true;
}

void shutdown() {
    if (S.device) S.device->waitIdle();
    // Destroy in dependency order: every D3D12 resource before the device that owns it.
    S.textures.clear(); S.texViews.clear();
    S.vb.reset(); S.ib.reset(); S.vsCB.reset(); S.psCB.reset(); S.sharedCB.reset();
    S.pAlpha.reset(); S.pAdditive.reset(); S.vs.reset(); S.ps.reset();
    S.sampler.reset(); S.texSet.reset(); S.sampSet.reset(); S.layout.reset();
    S.msaaFB.reset(); S.msaa.reset(); S.resolveTex.reset(); S.readback.reset();
    S.acquireSem.reset(); S.renderSem.reset(); S.swap.reset();
    S.cmd.reset(); S.fence.reset(); S.queue.reset();
    S.device.reset(); S.iface.reset();
    S = State{};
}

} // namespace gfx
