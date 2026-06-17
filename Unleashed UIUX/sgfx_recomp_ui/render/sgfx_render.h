// =============================================================================
// sgfx_render.h — the render-backend contract (was gpu/video.h's GuestTexture +
// gpu/imgui's loaders).
//
// The recomp's ui/ menus draw through Dear ImGui PLUS a custom gradient/outline/
// skew layer (render/imgui_common.h — the callback API, kept verbatim). The only
// other render coupling is the texture type: the menus hold UI sprites and pass
// them to ImGui::AddImage. We abstract that into a small backend contract a host
// implements once (the recomp's impl is plume/D3D12; SGFX swaps its own GPU).
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <imgui.h>
#include "imgui_common.h"   // SetGradient/SetShaderModifier/... callback contract

namespace sgfx { namespace render {

// Opaque GPU texture. Its address is used DIRECTLY as an ImTextureID (void* in stock
// imgui) — exactly as the recomp passed GuestTexture* — so the host's imgui renderer
// receives this pointer and binds `backend`. Complete type so std::unique_ptr<Texture>
// works inside the ui/ translation units; ~Texture is defined by the backend.
struct Texture
{
    void* backend = nullptr;   // host GPU handle
    int   width   = 0;
    int   height  = 0;
    ~Texture();                // host frees `backend`
};

// The three shared UI sprites imgui_utils needs (recomp res/images/common/*). The
// host supplies the pixels (they may be SEGA-derived, so they live with the host,
// not the lib) and uploads them.
enum class UISprite { GeneralWindow, Light, Select, OptionsStatic, OptionsStaticFlash };
std::unique_ptr<Texture> LoadUISprite(UISprite sprite);

// General texture upload from decoded image bytes (host decodes DDS/PNG + uploads).
std::unique_ptr<Texture> LoadTexture(const uint8_t* data, size_t size);

}} // namespace sgfx::render
