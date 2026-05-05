// Phase 346: SGFX text/font rendering for the native CSD framebuffer.
//
// Sonic Unleashed's retail approach to in-UI text is to pre-bake
// localized text strips per language (mat_*_en_*.dds, fte_ConverseMain_*.dds)
// and UV-crop the appropriate strip into a cast at runtime. SGFX's
// Phase 296 native renderer composites those pre-baked textures
// correctly when a cast points at one.
//
// What it CAN'T do is rasterize arbitrary runtime text (e.g. the
// help_chara_1/2/3 + help_text_area scenes that show dynamic body
// text from a localization table). For those we need a real glyph
// rasterizer. Phase 346 adds one via stb_truetype (single-header,
// public domain, vendored at
// local_build_env/ur103clean/thirdparty/stb/stb_truetype.h).
//
// API:
//   loadFontFromFile(path, pixelSize) -> SgfxBakedFont
//     Reads a TTF / OTF, bakes a 512x512 ASCII glyph atlas at the
//     requested pixel size. One-shot allocation; reuse the result.
//   compositeText(fb, font, text, x, y, rgba)
//     Composites UTF-8 text into the framebuffer at (x, y) using
//     the baked glyph atlas. Source-over alpha blend, top-left
//     origin baseline auto-adjusted via font ascent.
//   measureText(font, text) -> {width, height}
//     Pre-flight measurement so callers can center / right-align.
//
// Implementation lives in sgfx_hud_font_renderer.cpp -- one TU
// owns the STB_TRUETYPE_IMPLEMENTATION symbol so the header stays
// inline-friendly.

#pragma once

#include "sgfx_hud_native_csd_renderer.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Phase 346: per-glyph metric. Mirrors stbtt_bakedchar 1:1 plus
    // the ascent so callers can lift the baseline without re-querying
    // stb. Atlas pixels are 8-bit alpha (greyscale glyph mask).
    struct SgfxBakedGlyph
    {
        std::uint16_t x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        float xoff = 0.0f, yoff = 0.0f;
        float xadvance = 0.0f;
    };

    struct SgfxBakedFont
    {
        std::uint32_t            atlasWidth = 0;
        std::uint32_t            atlasHeight = 0;
        std::vector<std::uint8_t> atlasAlpha; // atlasWidth * atlasHeight, 8-bit
        // Glyph index = codepoint - kFirstCodepoint. kCodepointCount
        // covers printable ASCII (0x20..0x7E inclusive = 95 entries).
        static constexpr std::uint32_t kFirstCodepoint = 0x20;
        static constexpr std::uint32_t kCodepointCount = 0x7F - 0x20;
        std::array<SgfxBakedGlyph, kCodepointCount> glyphs{};
        float pixelSize = 0.0f;
        float ascent = 0.0f;   // in pixels at pixelSize
        float descent = 0.0f;
        float lineGap = 0.0f;
        bool  loaded = false;
    };

    // Load + bake. Returns a font with .loaded=false on failure
    // (missing file / bad TTF). Default atlas size is 512x512 which
    // fits ASCII at sizes up to ~32px comfortably.
    SgfxBakedFont loadFontFromFile(const std::filesystem::path& ttfPath,
                                   float pixelSize,
                                   std::uint32_t atlasWidth = 512,
                                   std::uint32_t atlasHeight = 512);

    // Pre-flight measurement of UTF-8 text using the baked font.
    // Multi-line not supported: caller splits on '\n' and measures
    // each line independently.
    struct SgfxTextMeasurement
    {
        float widthPx = 0.0f;
        float heightPx = 0.0f; // = ascent - descent
    };
    SgfxTextMeasurement measureText(const SgfxBakedFont& font,
                                    std::string_view text) noexcept;

    // Composite UTF-8 text into `fb` at framebuffer pixel (x, y).
    // (x, y) is the top-left of the bounding box; the baseline is
    // computed as y + ascent. RGBA is the foreground color
    // (pre-multiplied alpha is NOT required; we modulate by glyph
    // mask). Returns the post-render advance in pixels (so callers
    // can chain "draw label + draw value next to it").
    float compositeText(CsdNativeFramebuffer& fb,
                        const SgfxBakedFont& font,
                        std::string_view text,
                        std::int32_t x, std::int32_t y,
                        std::array<std::uint8_t, 4> rgba) noexcept;

    // Convenience: paint text centered horizontally within a box.
    void compositeTextCentered(CsdNativeFramebuffer& fb,
                               const SgfxBakedFont& font,
                               std::string_view text,
                               std::int32_t boxLeft, std::int32_t boxTop,
                               std::int32_t boxWidth, std::int32_t boxHeight,
                               std::array<std::uint8_t, 4> rgba) noexcept;

} // namespace sward::ui_runtime::generated::sgfx_hud
