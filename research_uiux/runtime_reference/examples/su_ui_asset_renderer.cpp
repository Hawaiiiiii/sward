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

struct SuUiRendererScreen
{
    std::string_view id;
    std::string_view displayName;
    std::string_view contractFileName;
    const SuUiRenderCast* casts = nullptr;
    std::size_t castCount = 0;
    Gdiplus::Color background = Gdiplus::Color(255, 0, 0, 0);
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

inline const std::array<SuUiRendererScreen, 5> kRendererScreens{{
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
    [[nodiscard]] const SuUiRendererScreen& selectedScreen() const
    {
        return kRendererScreens[selectedScreenIndex_ % kRendererScreens.size()];
    }

    void selectNext()
    {
        selectedScreenIndex_ = (selectedScreenIndex_ + 1) % kRendererScreens.size();
    }

    void selectPrevious()
    {
        selectedScreenIndex_ = (selectedScreenIndex_ + kRendererScreens.size() - 1) % kRendererScreens.size();
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

private:
    std::size_t selectedScreenIndex_ = 0;
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
    return Gdiplus::RectF((clientWidth - width) * 0.5F, (clientHeight - height) * 0.5F, width, height);
}

void drawMissingCast(Gdiplus::Graphics& graphics, const Gdiplus::RectF& destination)
{
    Gdiplus::SolidBrush fill(Gdiplus::Color(180, 28, 42, 28));
    Gdiplus::Pen outline(Gdiplus::Color(220, 98, 205, 98), 1.0F);
    graphics.FillRectangle(&fill, destination);
    graphics.DrawRectangle(&outline, destination);
}

void renderCleanScreen(HWND hwnd, HDC dc, SwardSuUiAssetRenderer& renderer)
{
    RECT client{};
    GetClientRect(hwnd, &client);

    Gdiplus::Graphics graphics(dc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    Gdiplus::SolidBrush windowBrush(Gdiplus::Color(255, 0, 0, 0));
    graphics.FillRectangle(&windowBrush, 0, 0, client.right - client.left, client.bottom - client.top);

    const auto canvas = canvasRectForClient(client);
    const auto& screen = renderer.selectedScreen();
    Gdiplus::SolidBrush canvasBrush(screen.background);
    graphics.FillRectangle(&canvasBrush, canvas);

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
        auto* create = reinterpret_cast<CREATESTRUCTA*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    auto* renderer = reinterpret_cast<SwardSuUiAssetRenderer*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message)
    {
    case WM_KEYDOWN:
        if (!renderer)
            break;
        if (wParam == VK_RIGHT || wParam == VK_SPACE)
        {
            renderer->selectNext();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (wParam == VK_LEFT)
        {
            renderer->selectPrevious();
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

    return runRendererWindow(instance, showCommand);
}
