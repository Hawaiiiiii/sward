// Phase 346: stb_truetype-backed implementation of sgfx_hud_font_renderer.hpp.

#include "sward/ui_runtime/sgfx_hud_font_renderer.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace sward::ui_runtime::generated::sgfx_hud
{
    namespace
    {
        std::vector<std::uint8_t> readAll(const std::filesystem::path& path)
        {
            std::ifstream f(path, std::ios::binary);
            if (!f) return {};
            f.seekg(0, std::ios::end);
            const auto size = static_cast<std::size_t>(f.tellg());
            f.seekg(0, std::ios::beg);
            std::vector<std::uint8_t> bytes(size);
            f.read(reinterpret_cast<char*>(bytes.data()), size);
            return bytes;
        }
    }

    SgfxBakedFont loadFontFromFile(const std::filesystem::path& ttfPath,
                                   float pixelSize,
                                   std::uint32_t atlasWidth,
                                   std::uint32_t atlasHeight)
    {
        SgfxBakedFont out;
        const auto ttfBytes = readAll(ttfPath);
        if (ttfBytes.empty()) return out;

        out.atlasWidth = atlasWidth;
        out.atlasHeight = atlasHeight;
        out.atlasAlpha.assign(static_cast<std::size_t>(atlasWidth) * atlasHeight, 0);
        out.pixelSize = pixelSize;

        std::array<stbtt_bakedchar, SgfxBakedFont::kCodepointCount> baked{};
        const int rc = stbtt_BakeFontBitmap(
            ttfBytes.data(), 0,
            pixelSize,
            out.atlasAlpha.data(),
            static_cast<int>(atlasWidth),
            static_cast<int>(atlasHeight),
            static_cast<int>(SgfxBakedFont::kFirstCodepoint),
            static_cast<int>(SgfxBakedFont::kCodepointCount),
            baked.data());
        if (rc <= 0) return out; // atlas too small, but partial bake is unsafe to trust

        for (std::size_t i = 0; i < baked.size(); ++i)
        {
            out.glyphs[i].x0 = baked[i].x0;
            out.glyphs[i].y0 = baked[i].y0;
            out.glyphs[i].x1 = baked[i].x1;
            out.glyphs[i].y1 = baked[i].y1;
            out.glyphs[i].xoff = baked[i].xoff;
            out.glyphs[i].yoff = baked[i].yoff;
            out.glyphs[i].xadvance = baked[i].xadvance;
        }

        // Pull ascent/descent/lineGap so callers can position by baseline.
        stbtt_fontinfo info{};
        if (stbtt_InitFont(&info, ttfBytes.data(),
                           stbtt_GetFontOffsetForIndex(ttfBytes.data(), 0)))
        {
            int asc = 0, desc = 0, gap = 0;
            stbtt_GetFontVMetrics(&info, &asc, &desc, &gap);
            const float scale = stbtt_ScaleForPixelHeight(&info, pixelSize);
            out.ascent  = asc * scale;
            out.descent = desc * scale;
            out.lineGap = gap * scale;
        }

        out.loaded = true;
        return out;
    }

    namespace
    {
        // Resolve UTF-8 byte to a glyph index. We only support ASCII
        // (codepoints 0x20..0x7E); anything outside maps to '?'.
        int glyphIndexFor(std::uint32_t codepoint) noexcept
        {
            if (codepoint < SgfxBakedFont::kFirstCodepoint
                || codepoint >= SgfxBakedFont::kFirstCodepoint
                                 + SgfxBakedFont::kCodepointCount)
                return static_cast<int>('?' - SgfxBakedFont::kFirstCodepoint);
            return static_cast<int>(codepoint - SgfxBakedFont::kFirstCodepoint);
        }
    }

    SgfxTextMeasurement measureText(const SgfxBakedFont& font,
                                    std::string_view text) noexcept
    {
        SgfxTextMeasurement m;
        if (!font.loaded) return m;
        float width = 0.0f;
        for (char c : text)
            width += font.glyphs[glyphIndexFor(static_cast<std::uint8_t>(c))].xadvance;
        m.widthPx = width;
        m.heightPx = font.ascent - font.descent;
        return m;
    }

    float compositeText(CsdNativeFramebuffer& fb,
                        const SgfxBakedFont& font,
                        std::string_view text,
                        std::int32_t x, std::int32_t y,
                        std::array<std::uint8_t, 4> rgba) noexcept
    {
        if (!font.loaded || fb.rgba.empty()) return 0.0f;
        const float baselineY = static_cast<float>(y) + font.ascent;
        float penX = static_cast<float>(x);

        for (char ch : text)
        {
            const int gi = glyphIndexFor(static_cast<std::uint8_t>(ch));
            const auto& g = font.glyphs[gi];
            // Skip whitespace glyphs that have no atlas footprint
            // (e.g. space, glyphs the bake skipped).
            const std::int32_t glyphW = static_cast<std::int32_t>(g.x1) - g.x0;
            const std::int32_t glyphH = static_cast<std::int32_t>(g.y1) - g.y0;
            if (glyphW > 0 && glyphH > 0)
            {
                const std::int32_t gxStart =
                    static_cast<std::int32_t>(penX + g.xoff);
                const std::int32_t gyStart =
                    static_cast<std::int32_t>(baselineY + g.yoff);
                for (std::int32_t py = 0; py < glyphH; ++py)
                {
                    const std::int32_t fbY = gyStart + py;
                    if (fbY < 0 || static_cast<std::uint32_t>(fbY) >= fb.height) continue;
                    const std::uint32_t srcRow = g.y0 + py;
                    if (srcRow >= font.atlasHeight) continue;
                    for (std::int32_t px = 0; px < glyphW; ++px)
                    {
                        const std::int32_t fbX = gxStart + px;
                        if (fbX < 0 || static_cast<std::uint32_t>(fbX) >= fb.width) continue;
                        const std::uint32_t srcCol = g.x0 + px;
                        if (srcCol >= font.atlasWidth) continue;
                        const std::uint8_t mask =
                            font.atlasAlpha[static_cast<std::size_t>(srcRow)
                                            * font.atlasWidth + srcCol];
                        if (mask == 0) continue;
                        const std::size_t dstIdx =
                            (static_cast<std::size_t>(fbY) * fb.width
                             + static_cast<std::uint32_t>(fbX)) * 4;
                        const std::uint32_t a = (mask * rgba[3]) / 255u;
                        if (a == 0) continue;
                        const std::uint32_t ia = 255u - a;
                        fb.rgba[dstIdx + 0] = static_cast<std::uint8_t>(
                            (rgba[0] * a + fb.rgba[dstIdx + 0] * ia) / 255u);
                        fb.rgba[dstIdx + 1] = static_cast<std::uint8_t>(
                            (rgba[1] * a + fb.rgba[dstIdx + 1] * ia) / 255u);
                        fb.rgba[dstIdx + 2] = static_cast<std::uint8_t>(
                            (rgba[2] * a + fb.rgba[dstIdx + 2] * ia) / 255u);
                        fb.rgba[dstIdx + 3] = static_cast<std::uint8_t>(std::min<std::uint32_t>(
                            255u, fb.rgba[dstIdx + 3] + a));
                    }
                }
            }
            penX += g.xadvance;
        }
        return penX - static_cast<float>(x);
    }

    void compositeTextCentered(CsdNativeFramebuffer& fb,
                               const SgfxBakedFont& font,
                               std::string_view text,
                               std::int32_t boxLeft, std::int32_t boxTop,
                               std::int32_t boxWidth, std::int32_t boxHeight,
                               std::array<std::uint8_t, 4> rgba) noexcept
    {
        if (!font.loaded) return;
        const auto m = measureText(font, text);
        const std::int32_t x = boxLeft + (boxWidth  - static_cast<std::int32_t>(m.widthPx))  / 2;
        const std::int32_t y = boxTop  + (boxHeight - static_cast<std::int32_t>(m.heightPx)) / 2;
        compositeText(fb, font, text, x, y, rgba);
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
