#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace
{
inline constexpr int kDesignWidth = 1280;
inline constexpr int kDesignHeight = 720;
inline constexpr int kRendererChromeHeight = 44;
inline constexpr int kPrevButtonId = 1001;
inline constexpr int kNextButtonId = 1002;
inline constexpr int kScreenLabelId = 1003;
inline constexpr int kAtlasPrevButtonId = 1004;
inline constexpr int kAtlasNextButtonId = 1005;

struct TextureSourceCandidate
{
    std::string_view textureFileName;
    std::string_view relativePath;
};

struct DdsTextureImage
{
    std::filesystem::path path;
    std::string fileName;
    std::string format;
    int width = 0;
    int height = 0;
    std::vector<std::uint32_t> argbPixels;
};

struct SuUiRenderCast
{
    std::string_view sceneName;
    std::string_view castName;
    std::string_view textureName;
    int sourceX = 0;
    int sourceY = 0;
    int sourceWidth = 0;
    int sourceHeight = 0;
    int destinationX = 0;
    int destinationY = 0;
    int destinationWidth = 0;
    int destinationHeight = 0;
};

enum class RendererScreenKind
{
    CastCatalog,
    AtlasGallery,
};

struct SuUiRendererScreen
{
    std::string_view id;
    std::string_view displayName;
    std::string_view contractFileName;
    const SuUiRenderCast* casts = nullptr;
    std::size_t castCount = 0;
    Gdiplus::Color background = Gdiplus::Color(255, 0, 0, 0);
    RendererScreenKind kind = RendererScreenKind::CastCatalog;
};

inline constexpr std::array<TextureSourceCandidate, 7> kTextureSourceCandidates{{
    { "mat_load_comon_001.dds", "ui_extended_archives/Loading/mat_load_comon_001.dds" },
    { "ui_mm_base.dds", "ui_frontend_archives/MainMenu/ui_mm_base.dds" },
    { "ui_mm_parts1.dds", "ui_frontend_archives/MainMenu/ui_mm_parts1.dds" },
    { "ui_mm_contentstext.dds", "ui_frontend_archives/MainMenu/ui_mm_contentstext.dds" },
    { "mat_title_en_001.dds", "ui_broader_archives/Languages/English/Title/mat_title_en_001.dds" },
    { "mat_title_en_002.dds", "ui_broader_archives/Languages/English/Title/mat_title_en_002.dds" },
    { "ui_ps1_gauge1.dds", "phase16_support_archives/ExStageTails_Common/ui_ps1_gauge1.dds" },
}};

inline constexpr std::array<SuUiRenderCast, 1> kLoadingCompositeCasts{{
    { "load_composite", "full_screen", "mat_load_comon_001.dds", 0, 0, 1280, 720, 0, 0, 1280, 720 },
}};

inline constexpr std::array<SuUiRenderCast, 3> kMainMenuCompositeCasts{{
    { "mm_bg", "base_sheet", "ui_mm_base.dds", 0, 0, 1280, 720, 0, 0, 1280, 720 },
    { "mm_bg", "parts_sheet", "ui_mm_parts1.dds", 0, 0, 1280, 640, 0, 40, 1280, 640 },
    { "mm_contents", "text_sheet", "ui_mm_contentstext.dds", 0, 0, 640, 640, 248, 40, 640, 640 },
}};

inline constexpr std::array<SuUiRenderCast, 1> kSonicTitleMenuCasts{{
    { "mm_bg_usual", "black3", "ui_mm_parts1.dds", 896, 336, 16, 16, 655, 435, 368, 464 },
}};

inline constexpr std::array<SuUiRenderCast, 2> kTitleLogoSheetCasts{{
    { "title", "logo_en_001", "mat_title_en_001.dds", 0, 0, 256, 512, 320, 104, 256, 512 },
    { "title", "logo_en_002", "mat_title_en_002.dds", 0, 0, 128, 256, 704, 232, 128, 256 },
}};

inline constexpr std::array<SuUiRenderCast, 1> kSonicStageHudCasts{{
    { "so_speed_gauge", "position_hd", "ui_ps1_gauge1.dds", 4, 64, 16, 20, 752, 357, 16, 20 },
}};

inline const std::array<SuUiRendererScreen, 6> kRendererScreens{{
    {
        "VisualAtlasGallery",
        "Visual Atlas Gallery",
        "visual_atlas/atlas_index.json",
        nullptr,
        0,
        Gdiplus::Color(255, 0, 0, 0),
        RendererScreenKind::AtlasGallery,
    },
    {
        "LoadingComposite",
        "LoadingTransition Composite",
        "loading_transition_reference.json",
        kLoadingCompositeCasts.data(),
        kLoadingCompositeCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
    },
    {
        "MainMenuComposite",
        "Main Menu Composite Sheets",
        "title_menu_reference.json",
        kMainMenuCompositeCasts.data(),
        kMainMenuCompositeCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
    },
    {
        "SonicTitleMenu",
        "Sonic Title Menu",
        "title_menu_reference.json",
        kSonicTitleMenuCasts.data(),
        kSonicTitleMenuCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
    },
    {
        "TitleLogoSheet",
        "Title Logo Sheets",
        "title_menu_reference.json",
        kTitleLogoSheetCasts.data(),
        kTitleLogoSheetCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
    },
    {
        "SonicStageHud",
        "Sonic Stage HUD",
        "sonic_stage_hud_reference.json",
        kSonicStageHudCasts.data(),
        kSonicStageHudCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
    },
}};

[[nodiscard]] std::filesystem::path executableDirectory()
{
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size())
    {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }

    if (size == 0)
        return std::filesystem::current_path();

    buffer.resize(size);
    return std::filesystem::path(buffer).parent_path();
}

void appendAncestorAssetRoots(std::vector<std::filesystem::path>& candidates, std::filesystem::path start)
{
    std::error_code error;
    start = std::filesystem::absolute(start, error);
    if (error)
        return;

    for (auto current = start; !current.empty(); current = current.parent_path())
    {
        candidates.push_back(current / "extracted_assets");
        if (current == current.root_path())
            break;
    }
}

[[nodiscard]] std::vector<std::filesystem::path> extractedAssetRootCandidates()
{
    std::vector<std::filesystem::path> candidates;
    appendAncestorAssetRoots(candidates, std::filesystem::current_path());
    appendAncestorAssetRoots(candidates, executableDirectory());
    return candidates;
}

[[nodiscard]] std::vector<std::filesystem::path> discoverAtlasSheetPaths()
{
    std::vector<std::filesystem::path> paths;
    for (const auto& root : extractedAssetRootCandidates())
    {
        std::error_code error;
        const auto sheetRoot = root / "visual_atlas" / "sheets";
        if (!std::filesystem::is_directory(sheetRoot, error))
            continue;

        for (const auto& entry : std::filesystem::directory_iterator(sheetRoot, error))
        {
            if (error)
                break;
            if (!entry.is_regular_file(error))
                continue;

            const auto extension = entry.path().extension().string();
            if (extension == ".png" || extension == ".PNG")
                paths.push_back(entry.path());
        }
    }

    std::sort(
        paths.begin(),
        paths.end(),
        [](const auto& left, const auto& right)
        {
            return left.filename().string() < right.filename().string();
        });
    paths.erase(
        std::unique(
            paths.begin(),
            paths.end(),
            [](const auto& left, const auto& right)
            {
                return left.filename() == right.filename();
            }),
        paths.end());
    return paths;
}

[[nodiscard]] const TextureSourceCandidate* textureSourceCandidateForFileName(std::string_view textureFileName)
{
    const auto found = std::find_if(
        kTextureSourceCandidates.begin(),
        kTextureSourceCandidates.end(),
        [textureFileName](const TextureSourceCandidate& candidate)
        {
            return candidate.textureFileName == textureFileName;
        });
    return found == kTextureSourceCandidates.end() ? nullptr : &*found;
}

[[nodiscard]] std::optional<std::filesystem::path> textureSourcePathForFileName(std::string_view textureFileName)
{
    const auto* candidate = textureSourceCandidateForFileName(textureFileName);
    if (!candidate)
        return std::nullopt;

    for (const auto& root : extractedAssetRootCandidates())
    {
        std::error_code error;
        const auto path = root / std::filesystem::path(candidate->relativePath);
        if (std::filesystem::is_regular_file(path, error))
            return path;
    }

    return std::nullopt;
}

[[nodiscard]] std::uint16_t readLe16(const std::uint8_t* data)
{
    return static_cast<std::uint16_t>(data[0] | (data[1] << 8));
}

[[nodiscard]] std::uint32_t readLe32(const std::uint8_t* data)
{
    return static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8)
        | (static_cast<std::uint32_t>(data[2]) << 16)
        | (static_cast<std::uint32_t>(data[3]) << 24);
}

void decodeDxt5Block(const std::uint8_t* block, int blockX, int blockY, int width, int height, std::vector<std::uint32_t>& pixels)
{
    std::array<std::uint8_t, 8> alpha{};
    alpha[0] = block[0];
    alpha[1] = block[1];
    if (alpha[0] > alpha[1])
    {
        alpha[2] = static_cast<std::uint8_t>((6 * alpha[0] + alpha[1]) / 7);
        alpha[3] = static_cast<std::uint8_t>((5 * alpha[0] + 2 * alpha[1]) / 7);
        alpha[4] = static_cast<std::uint8_t>((4 * alpha[0] + 3 * alpha[1]) / 7);
        alpha[5] = static_cast<std::uint8_t>((3 * alpha[0] + 4 * alpha[1]) / 7);
        alpha[6] = static_cast<std::uint8_t>((2 * alpha[0] + 5 * alpha[1]) / 7);
        alpha[7] = static_cast<std::uint8_t>((alpha[0] + 6 * alpha[1]) / 7);
    }
    else
    {
        alpha[2] = static_cast<std::uint8_t>((4 * alpha[0] + alpha[1]) / 5);
        alpha[3] = static_cast<std::uint8_t>((3 * alpha[0] + 2 * alpha[1]) / 5);
        alpha[4] = static_cast<std::uint8_t>((2 * alpha[0] + 3 * alpha[1]) / 5);
        alpha[5] = static_cast<std::uint8_t>((alpha[0] + 4 * alpha[1]) / 5);
        alpha[6] = 0;
        alpha[7] = 255;
    }

    std::uint64_t alphaBits = 0;
    for (int index = 0; index < 6; ++index)
        alphaBits |= static_cast<std::uint64_t>(block[2 + index]) << (index * 8);

    const std::uint16_t color0 = readLe16(block + 8);
    const std::uint16_t color1 = readLe16(block + 10);
    auto unpackColor = [](std::uint16_t color)
    {
        const std::uint8_t r5 = static_cast<std::uint8_t>((color >> 11) & 0x1F);
        const std::uint8_t g6 = static_cast<std::uint8_t>((color >> 5) & 0x3F);
        const std::uint8_t b5 = static_cast<std::uint8_t>(color & 0x1F);
        return std::array<std::uint8_t, 3>{
            static_cast<std::uint8_t>((r5 << 3) | (r5 >> 2)),
            static_cast<std::uint8_t>((g6 << 2) | (g6 >> 4)),
            static_cast<std::uint8_t>((b5 << 3) | (b5 >> 2)),
        };
    };

    std::array<std::array<std::uint8_t, 3>, 4> colors{};
    colors[0] = unpackColor(color0);
    colors[1] = unpackColor(color1);
    colors[2] = {
        static_cast<std::uint8_t>((2 * colors[0][0] + colors[1][0]) / 3),
        static_cast<std::uint8_t>((2 * colors[0][1] + colors[1][1]) / 3),
        static_cast<std::uint8_t>((2 * colors[0][2] + colors[1][2]) / 3),
    };
    colors[3] = {
        static_cast<std::uint8_t>((colors[0][0] + 2 * colors[1][0]) / 3),
        static_cast<std::uint8_t>((colors[0][1] + 2 * colors[1][1]) / 3),
        static_cast<std::uint8_t>((colors[0][2] + 2 * colors[1][2]) / 3),
    };

    const std::uint32_t colorBits = readLe32(block + 12);
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            const int pixelIndex = (y * 4) + x;
            const int targetX = (blockX * 4) + x;
            const int targetY = (blockY * 4) + y;
            if (targetX >= width || targetY >= height)
                continue;

            const std::uint8_t alphaValue = alpha[(alphaBits >> (3 * pixelIndex)) & 0x7];
            const auto& color = colors[(colorBits >> (2 * pixelIndex)) & 0x3];
            pixels[static_cast<std::size_t>(targetY * width + targetX)] =
                (static_cast<std::uint32_t>(alphaValue) << 24)
                | (static_cast<std::uint32_t>(color[0]) << 16)
                | (static_cast<std::uint32_t>(color[1]) << 8)
                | static_cast<std::uint32_t>(color[2]);
        }
    }
}

[[nodiscard]] std::optional<DdsTextureImage> loadDdsTextureImage(std::string_view textureFileName)
{
    const auto path = textureSourcePathForFileName(textureFileName);
    if (!path)
        return std::nullopt;

    std::ifstream file(*path, std::ios::binary | std::ios::ate);
    if (!file)
        return std::nullopt;

    const auto fileSize = file.tellg();
    if (fileSize < 128)
        return std::nullopt;

    std::vector<std::uint8_t> data(static_cast<std::size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file || std::memcmp(data.data(), "DDS ", 4) != 0)
        return std::nullopt;

    const std::uint32_t headerSize = readLe32(data.data() + 4);
    const int height = static_cast<int>(readLe32(data.data() + 12));
    const int width = static_cast<int>(readLe32(data.data() + 16));
    const std::uint32_t pixelFormatSize = readLe32(data.data() + 76);
    const std::string fourCc(reinterpret_cast<const char*>(data.data() + 84), 4);
    if (headerSize != 124 || pixelFormatSize != 32 || fourCc != "DXT5" || width <= 0 || height <= 0)
        return std::nullopt;

    const int blockCountX = (width + 3) / 4;
    const int blockCountY = (height + 3) / 4;
    const std::size_t requiredBytes = 128 + (static_cast<std::size_t>(blockCountX) * static_cast<std::size_t>(blockCountY) * 16);
    if (data.size() < requiredBytes)
        return std::nullopt;

    DdsTextureImage image;
    image.path = *path;
    image.fileName = path->filename().string();
    image.format = fourCc;
    image.width = width;
    image.height = height;
    image.argbPixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);

    const std::uint8_t* block = data.data() + 128;
    for (int blockY = 0; blockY < blockCountY; ++blockY)
    {
        for (int blockX = 0; blockX < blockCountX; ++blockX)
        {
            decodeDxt5Block(block, blockX, blockY, width, height, image.argbPixels);
            block += 16;
        }
    }

    return image;
}

[[nodiscard]] bool castSourceFits(const SuUiRenderCast& cast, const DdsTextureImage& image)
{
    return cast.sourceX >= 0
        && cast.sourceY >= 0
        && cast.sourceWidth > 0
        && cast.sourceHeight > 0
        && cast.sourceX + cast.sourceWidth <= image.width
        && cast.sourceY + cast.sourceHeight <= image.height;
}

[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap> bitmapFromDdsTextureImage(const DdsTextureImage& image)
{
    auto bitmap = std::make_unique<Gdiplus::Bitmap>(image.width, image.height, PixelFormat32bppARGB);
    Gdiplus::BitmapData bitmapData{};
    Gdiplus::Rect lockRect(0, 0, image.width, image.height);
    if (bitmap->LockBits(&lockRect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok)
        return nullptr;

    for (int y = 0; y < image.height; ++y)
    {
        auto* destination = static_cast<std::uint8_t*>(bitmapData.Scan0) + (static_cast<std::ptrdiff_t>(bitmapData.Stride) * y);
        const auto* source = image.argbPixels.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width));
        std::memcpy(destination, source, static_cast<std::size_t>(image.width) * sizeof(std::uint32_t));
    }

    bitmap->UnlockBits(&bitmapData);
    return bitmap;
}

struct CachedTexture
{
    std::string textureFileName;
    std::optional<DdsTextureImage> image;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
};

class SwardSuUiAssetRenderer
{
public:
    SwardSuUiAssetRenderer()
        : atlasSheets_(discoverAtlasSheetPaths())
    {
    }

    [[nodiscard]] std::size_t selectedScreenIndex() const
    {
        return selectedScreenIndex_ % kRendererScreens.size();
    }

    [[nodiscard]] std::size_t screenCount() const
    {
        return kRendererScreens.size();
    }

    [[nodiscard]] const SuUiRendererScreen& selectedScreen() const
    {
        return kRendererScreens[selectedScreenIndex()];
    }

    void selectNext()
    {
        selectedScreenIndex_ = (selectedScreenIndex_ + 1) % kRendererScreens.size();
    }

    void selectPrevious()
    {
        selectedScreenIndex_ = (selectedScreenIndex_ + kRendererScreens.size() - 1) % kRendererScreens.size();
    }

    void selectNextAtlas()
    {
        if (atlasSheets_.empty())
            return;
        selectedScreenIndex_ = 0;
        selectedAtlasIndex_ = (selectedAtlasIndex_ + 1) % atlasSheets_.size();
        atlasBitmap_.reset();
    }

    void selectPreviousAtlas()
    {
        if (atlasSheets_.empty())
            return;
        selectedScreenIndex_ = 0;
        selectedAtlasIndex_ = (selectedAtlasIndex_ + atlasSheets_.size() - 1) % atlasSheets_.size();
        atlasBitmap_.reset();
    }

    [[nodiscard]] std::size_t atlasSheetCount() const
    {
        return atlasSheets_.size();
    }

    [[nodiscard]] std::string selectedAtlasFileName() const
    {
        if (atlasSheets_.empty())
            return "none";
        return atlasSheets_[selectedAtlasIndex_ % atlasSheets_.size()].filename().string();
    }

    [[nodiscard]] std::string selectedScreenIndexText() const
    {
        const auto& screen = selectedScreen();
        std::ostringstream text;
        text
            << (selectedScreenIndex() + 1)
            << "/"
            << screenCount()
            << " "
            << screen.id
            << " - "
            << screen.displayName;
        if (screen.kind == RendererScreenKind::AtlasGallery)
        {
            text << " | atlas ";
            if (atlasSheets_.empty())
            {
                text << "0/0 missing visual_atlas/sheets";
            }
            else
            {
                text
                    << ((selectedAtlasIndex_ % atlasSheets_.size()) + 1)
                    << "/"
                    << atlasSheets_.size()
                    << " "
                    << selectedAtlasFileName();
            }
        }
        return text.str();
    }

    [[nodiscard]] const CachedTexture* textureFor(std::string_view textureFileName)
    {
        const auto found = std::find_if(
            textureCache_.begin(),
            textureCache_.end(),
            [textureFileName](const auto& cached)
            {
                return cached->textureFileName == textureFileName;
            });
        if (found != textureCache_.end())
            return found->get();

        auto cached = std::make_unique<CachedTexture>();
        cached->textureFileName = std::string(textureFileName);
        cached->image = loadDdsTextureImage(textureFileName);
        if (cached->image)
            cached->bitmap = bitmapFromDdsTextureImage(*cached->image);

        textureCache_.push_back(std::move(cached));
        return textureCache_.back().get();
    }

    [[nodiscard]] Gdiplus::Bitmap* currentAtlasBitmap()
    {
        if (atlasSheets_.empty())
            return nullptr;

        const auto& path = atlasSheets_[selectedAtlasIndex_ % atlasSheets_.size()];
        if (atlasBitmap_ && atlasBitmapPath_ == path)
            return atlasBitmap_.get();

        auto bitmap = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(path.wstring().c_str(), FALSE));
        if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok)
        {
            atlasBitmap_.reset();
            atlasBitmapPath_.clear();
            return nullptr;
        }

        atlasBitmapPath_ = path;
        atlasBitmap_ = std::move(bitmap);
        return atlasBitmap_.get();
    }

private:
    std::size_t selectedScreenIndex_ = 0;
    std::size_t selectedAtlasIndex_ = 0;
    std::vector<std::filesystem::path> atlasSheets_;
    std::filesystem::path atlasBitmapPath_;
    std::unique_ptr<Gdiplus::Bitmap> atlasBitmap_;
    std::vector<std::unique_ptr<CachedTexture>> textureCache_;
};

[[nodiscard]] Gdiplus::RectF designRectToCanvas(const Gdiplus::RectF& canvas, int x, int y, int width, int height)
{
    return Gdiplus::RectF(
        canvas.X + (static_cast<float>(x) / static_cast<float>(kDesignWidth) * canvas.Width),
        canvas.Y + (static_cast<float>(y) / static_cast<float>(kDesignHeight) * canvas.Height),
        static_cast<float>(width) / static_cast<float>(kDesignWidth) * canvas.Width,
        static_cast<float>(height) / static_cast<float>(kDesignHeight) * canvas.Height);
}

[[nodiscard]] Gdiplus::RectF canvasRectForClient(const RECT& client)
{
    const float clientWidth = static_cast<float>(std::max(1L, client.right - client.left));
    const float clientHeight = static_cast<float>(std::max(1L, client.bottom - client.top));
    const float scale = std::min(clientWidth / static_cast<float>(kDesignWidth), clientHeight / static_cast<float>(kDesignHeight));
    const float width = static_cast<float>(kDesignWidth) * scale;
    const float height = static_cast<float>(kDesignHeight) * scale;
    return Gdiplus::RectF(
        static_cast<float>(client.left) + ((clientWidth - width) * 0.5F),
        static_cast<float>(client.top) + ((clientHeight - height) * 0.5F),
        width,
        height);
}

[[nodiscard]] Gdiplus::RectF fitBitmapRectToCanvas(const Gdiplus::RectF& canvas, UINT width, UINT height)
{
    if (width == 0 || height == 0)
        return canvas;

    const float scale = std::min(canvas.Width / static_cast<float>(width), canvas.Height / static_cast<float>(height));
    const float fittedWidth = static_cast<float>(width) * scale;
    const float fittedHeight = static_cast<float>(height) * scale;
    return Gdiplus::RectF(
        canvas.X + ((canvas.Width - fittedWidth) * 0.5F),
        canvas.Y + ((canvas.Height - fittedHeight) * 0.5F),
        fittedWidth,
        fittedHeight);
}

[[nodiscard]] std::wstring widenAscii(std::string_view text)
{
    return std::wstring(text.begin(), text.end());
}

void updateRendererStatus(HWND hwnd, const SwardSuUiAssetRenderer& renderer)
{
    const auto status = renderer.selectedScreenIndexText();
    const auto wideStatus = widenAscii(status);
    if (HWND label = GetDlgItem(hwnd, kScreenLabelId))
        SetWindowTextW(label, wideStatus.c_str());

    const std::wstring title = L"SWARD SU UI Asset Renderer - " + wideStatus;
    SetWindowTextW(hwnd, title.c_str());
}

void layoutRendererControls(HWND hwnd)
{
    RECT client{};
    GetClientRect(hwnd, &client);

    const int margin = 10;
    const int buttonWidth = 88;
    const int atlasButtonWidth = 112;
    const int buttonHeight = 28;
    const int top = 8;
    const int nextLeft = margin + buttonWidth + 8;
    const int atlasPrevLeft = nextLeft + buttonWidth + 8;
    const int atlasNextLeft = atlasPrevLeft + atlasButtonWidth + 8;
    const int labelLeft = atlasNextLeft + atlasButtonWidth + 14;
    const int labelWidth = std::max(160L, client.right - labelLeft - margin);

    if (HWND previous = GetDlgItem(hwnd, kPrevButtonId))
        MoveWindow(previous, margin, top, buttonWidth, buttonHeight, TRUE);
    if (HWND next = GetDlgItem(hwnd, kNextButtonId))
        MoveWindow(next, nextLeft, top, buttonWidth, buttonHeight, TRUE);
    if (HWND atlasPrevious = GetDlgItem(hwnd, kAtlasPrevButtonId))
        MoveWindow(atlasPrevious, atlasPrevLeft, top, atlasButtonWidth, buttonHeight, TRUE);
    if (HWND atlasNext = GetDlgItem(hwnd, kAtlasNextButtonId))
        MoveWindow(atlasNext, atlasNextLeft, top, atlasButtonWidth, buttonHeight, TRUE);
    if (HWND label = GetDlgItem(hwnd, kScreenLabelId))
        MoveWindow(label, labelLeft, top, labelWidth, buttonHeight, TRUE);
}

void createRendererControls(HWND hwnd)
{
    const auto instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));

    CreateWindowExW(0, L"BUTTON", L"Prev",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPrevButtonId)),
        instance,
        nullptr);

    CreateWindowExW(0, L"BUTTON", L"Next",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kNextButtonId)),
        instance,
        nullptr);

    CreateWindowExW(0, L"BUTTON", L"Atlas Prev",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAtlasPrevButtonId)),
        instance,
        nullptr);

    CreateWindowExW(0, L"BUTTON", L"Atlas Next",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAtlasNextButtonId)),
        instance,
        nullptr);

    CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kScreenLabelId)),
        instance,
        nullptr);

    layoutRendererControls(hwnd);
}

void drawMissingCast(Gdiplus::Graphics& graphics, const Gdiplus::RectF& destination)
{
    Gdiplus::SolidBrush fill(Gdiplus::Color(180, 28, 42, 28));
    Gdiplus::Pen outline(Gdiplus::Color(220, 98, 205, 98), 1.0F);
    graphics.FillRectangle(&fill, destination);
    graphics.DrawRectangle(&outline, destination);
}

void renderAtlasGalleryScreen(Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas, SwardSuUiAssetRenderer& renderer)
{
    auto* bitmap = renderer.currentAtlasBitmap();
    if (!bitmap)
    {
        drawMissingCast(graphics, canvas);
        return;
    }

    const auto destination = fitBitmapRectToCanvas(canvas, bitmap->GetWidth(), bitmap->GetHeight());
    graphics.DrawImage(
        bitmap,
        destination,
        0.0F,
        0.0F,
        static_cast<float>(bitmap->GetWidth()),
        static_cast<float>(bitmap->GetHeight()),
        Gdiplus::UnitPixel);
}

void renderCleanScreen(HWND hwnd, HDC dc, SwardSuUiAssetRenderer& renderer)
{
    RECT client{};
    GetClientRect(hwnd, &client);
    RECT renderClient = client;
    renderClient.top = std::min(renderClient.bottom, renderClient.top + kRendererChromeHeight);

    Gdiplus::Graphics graphics(dc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    Gdiplus::SolidBrush windowBrush(Gdiplus::Color(255, 0, 0, 0));
    graphics.FillRectangle(&windowBrush, 0, 0, client.right - client.left, client.bottom - client.top);

    Gdiplus::SolidBrush chromeBrush(Gdiplus::Color(255, 236, 236, 236));
    graphics.FillRectangle(&chromeBrush, 0, 0, client.right - client.left, kRendererChromeHeight);

    const auto canvas = canvasRectForClient(renderClient);
    const auto& screen = renderer.selectedScreen();
    Gdiplus::SolidBrush canvasBrush(screen.background);
    graphics.FillRectangle(&canvasBrush, canvas);

    if (screen.kind == RendererScreenKind::AtlasGallery)
    {
        renderAtlasGalleryScreen(graphics, canvas, renderer);
        return;
    }

    for (std::size_t index = 0; index < screen.castCount; ++index)
    {
        const auto& cast = screen.casts[index];
        const auto destination = designRectToCanvas(canvas, cast.destinationX, cast.destinationY, cast.destinationWidth, cast.destinationHeight);
        const auto* texture = renderer.textureFor(cast.textureName);
        if (!texture || !texture->image || !texture->bitmap || !castSourceFits(cast, *texture->image))
        {
            drawMissingCast(graphics, destination);
            continue;
        }

        graphics.DrawImage(
            texture->bitmap.get(),
            destination,
            static_cast<float>(cast.sourceX),
            static_cast<float>(cast.sourceY),
            static_cast<float>(cast.sourceWidth),
            static_cast<float>(cast.sourceHeight),
            Gdiplus::UnitPixel);
    }
}

[[nodiscard]] LRESULT CALLBACK rendererWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    auto* renderer = reinterpret_cast<SwardSuUiAssetRenderer*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message)
    {
    case WM_CREATE:
        createRendererControls(hwnd);
        if (renderer)
            updateRendererStatus(hwnd, *renderer);
        return 0;
    case WM_SIZE:
        layoutRendererControls(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_COMMAND:
        if (!renderer)
            break;
        if (LOWORD(wParam) == kPrevButtonId)
        {
            renderer->selectPrevious();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (LOWORD(wParam) == kNextButtonId)
        {
            renderer->selectNext();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (LOWORD(wParam) == kAtlasPrevButtonId)
        {
            renderer->selectPreviousAtlas();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (LOWORD(wParam) == kAtlasNextButtonId)
        {
            renderer->selectNextAtlas();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (!renderer)
            break;
        if (wParam == VK_RIGHT || wParam == VK_SPACE)
        {
            renderer->selectNext();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (wParam == VK_LEFT)
        {
            renderer->selectPrevious();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_PAINT:
        if (renderer)
        {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(hwnd, &paint);
            renderCleanScreen(hwnd, dc, *renderer);
            EndPaint(hwnd, &paint);
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

[[nodiscard]] int runRendererSmoke()
{
    std::size_t castCount = 0;
    std::size_t resolvedTextureCount = 0;
    std::size_t fullScreenCastCount = 0;
    std::vector<std::string> descriptors;

    for (const auto& screen : kRendererScreens)
    {
        castCount += screen.castCount;
        for (std::size_t index = 0; index < screen.castCount; ++index)
        {
            const auto& cast = screen.casts[index];
            const auto image = loadDdsTextureImage(cast.textureName);
            if (screen.id == "LoadingComposite"
                && cast.destinationX == 0
                && cast.destinationY == 0
                && cast.destinationWidth == kDesignWidth
                && cast.destinationHeight == kDesignHeight)
            {
                ++fullScreenCastCount;
            }

            std::ostringstream descriptor;
            descriptor
                << screen.id
                << ":" << cast.sceneName
                << "/" << cast.castName
                << ":" << cast.textureName;

            if (image)
            {
                ++resolvedTextureCount;
                descriptor << ":" << image->format << ":" << image->width << "x" << image->height;
                if (!castSourceFits(cast, *image))
                    descriptor << ":source-out-of-bounds";
            }
            else
            {
                descriptor << ":missing";
            }
            descriptor
                << ":dst=" << cast.destinationX << "," << cast.destinationY << ","
                << cast.destinationWidth << "x" << cast.destinationHeight;
            descriptors.push_back(descriptor.str());
        }
    }

    std::cout
        << "sward_su_ui_asset_renderer smoke ok "
        << "screens=" << kRendererScreens.size()
        << " casts=" << castCount
        << " textures=" << resolvedTextureCount
        << " full_screen_casts=" << fullScreenCastCount
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return resolvedTextureCount == castCount ? 0 : 1;
}

[[nodiscard]] int runRendererNavigationSmoke()
{
    SwardSuUiAssetRenderer renderer;
    std::cout
        << "sward_su_ui_asset_renderer navigation smoke ok "
        << "screens=" << renderer.screenCount()
        << " controls=5"
        << " first=" << kRendererScreens.front().id
        << " last=" << kRendererScreens.back().id
        << " label=" << renderer.selectedScreenIndexText()
        << '\n';

    for (const auto& screen : kRendererScreens)
    {
        std::cout
            << "screen=" << screen.id
            << ":casts=" << screen.castCount
            << ":contract=" << screen.contractFileName
            << '\n';
    }

    return 0;
}

[[nodiscard]] std::string findAtlasSheetFileName(const std::vector<std::filesystem::path>& sheets, std::string_view fileName)
{
    const auto found = std::find_if(
        sheets.begin(),
        sheets.end(),
        [fileName](const auto& path)
        {
            return path.filename().string() == fileName;
        });
    return found == sheets.end() ? "missing" : found->filename().string();
}

[[nodiscard]] int runRendererAtlasGallerySmoke()
{
    const auto sheets = discoverAtlasSheetPaths();
    const auto first = sheets.empty() ? std::string("none") : sheets.front().filename().string();
    const auto loading = findAtlasSheetFileName(sheets, "loading__ui_loading.png");
    const auto mainMenu = findAtlasSheetFileName(sheets, "mainmenu__ui_mainmenu.png");
    const auto status = findAtlasSheetFileName(sheets, "systemcommoncore__ui_status.png");

    std::cout
        << "sward_su_ui_asset_renderer atlas gallery smoke ok "
        << "sheets=" << sheets.size()
        << " first=" << first
        << " loading=" << loading
        << " mainmenu=" << mainMenu
        << " status=" << status
        << '\n';

    return sheets.empty() || loading == "missing" || mainMenu == "missing" || status == "missing" ? 1 : 0;
}

[[nodiscard]] bool commandLineHasFlag(std::string_view flag)
{
    const std::string commandLine = GetCommandLineA();
    return commandLine.find(flag) != std::string::npos;
}

[[nodiscard]] int runRendererWindow(HINSTANCE instance, int showCommand)
{
    Gdiplus::GdiplusStartupInput gdiplusInput{};
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        return 1;

    const wchar_t* className = L"SwardSuUiAssetRendererWindow";
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = rendererWindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    windowClass.lpszClassName = className;
    RegisterClassW(&windowClass);

    SwardSuUiAssetRenderer renderer;
    HWND window = CreateWindowExW(
        0,
        className,
        L"SWARD SU UI Asset Renderer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1380,
        820,
        nullptr,
        nullptr,
        instance,
        &renderer);

    if (!window)
    {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return static_cast<int>(message.wParam);
}
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand)
{
    if (commandLineHasFlag("--renderer-smoke"))
        return runRendererSmoke();
    if (commandLineHasFlag("--renderer-navigation-smoke"))
        return runRendererNavigationSmoke();
    if (commandLineHasFlag("--renderer-atlas-gallery-smoke"))
        return runRendererAtlasGallerySmoke();

    return runRendererWindow(instance, showCommand);
}
