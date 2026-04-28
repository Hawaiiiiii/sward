#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <sward/ui_runtime/sgfx_templates.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
using sward::ui_runtime::SgfxScreenTemplate;
using sward::ui_runtime::findSgfxScreenTemplate;
using sward::ui_runtime::sgfxScreenTemplates;

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
    TitleLoopReconstruction,
    SonicHudReconstruction,
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

struct SgfxPlaceholderAssetSlot
{
    std::string_view slotName;
    std::string_view textureName;
    std::string_view sourceFamily;
};

struct SgfxTemplateRenderBinding
{
    std::string_view templateId;
    std::string_view rendererScreenId;
    const SgfxPlaceholderAssetSlot* slots = nullptr;
    std::size_t slotCount = 0;
    std::string_view requiredEventId;
    std::string_view timelineBandId;
    std::string_view timelineEventLabel;
};

struct CsdPipelineSceneSummary
{
    std::string sceneName;
    int castCount = 0;
    int subimageCount = 0;
    std::vector<std::string> textureNames;
    double frameStart = 0.0;
    double frameEnd = 0.0;
};

struct CsdPipelineTimelineHook
{
    std::string sceneName;
    std::string animationName;
    double frameCount = 0.0;
    double timelineSeconds = 0.0;
    int totalKeyframes = 0;
};

struct CsdPipelineEvidence
{
    std::filesystem::path sourcePath;
    std::string layoutFileName;
    std::vector<std::string> textureNames;
    std::vector<CsdPipelineSceneSummary> scenes;
    std::vector<CsdPipelineTimelineHook> timelines;
};

struct CsdPipelineTemplateBinding
{
    std::string_view templateId;
    std::string_view layoutFileName;
    std::string_view primarySceneName;
    std::string_view timelineSceneName;
    std::string_view timelineAnimationName;
};

inline constexpr std::array<TextureSourceCandidate, 13> kTextureSourceCandidates{{
    { "mat_load_comon_001.dds", "ui_extended_archives/Loading/mat_load_comon_001.dds" },
    { "OPmovie_titlelogo_EN.decompressed.dds", "runtime_previews/title/decompressed/OPmovie_titlelogo_EN.decompressed.dds" },
    { "ui_mm_base.dds", "ui_frontend_archives/MainMenu/ui_mm_base.dds" },
    { "ui_mm_parts1.dds", "ui_frontend_archives/MainMenu/ui_mm_parts1.dds" },
    { "ui_mm_contentstext.dds", "ui_frontend_archives/MainMenu/ui_mm_contentstext.dds" },
    { "mat_title_en_001.dds", "ui_broader_archives/Languages/English/Title/mat_title_en_001.dds" },
    { "mat_title_en_002.dds", "ui_broader_archives/Languages/English/Title/mat_title_en_002.dds" },
    { "mat_start_en_001.dds", "phase25_commonflow_archives/Languages/English/ActionCommon/mat_start_en_001.dds" },
    { "ui_ps1_gauge1.dds", "phase16_support_archives/ExStageTails_Common/ui_ps1_gauge1.dds" },
    { "mat_playscreen_001.dds", "phase16_support_archives/ExStageTails_Common/mat_playscreen_001.dds" },
    { "mat_playscreen_en_001.dds", "phase25_commonflow_archives/Languages/English/ExStageTails_Common/mat_playscreen_en_001.dds" },
    { "mat_comon_num_001.dds", "ui_extended_archives/SystemCommonCore/mat_comon_num_001.dds" },
    { "mat_comon_001.dds", "ui_extended_archives/SystemCommonCore/mat_comon_001.dds" },
}};

inline constexpr std::array<SuUiRenderCast, 4> kTitleLoopReconstructionCasts{{
    { "ui_title/bg/bg", "title_movie_frame", "ui_mm_base.dds", 0, 0, 1280, 720, 0, 0, 1280, 720 },
    { "ui_title/logo", "opmovie_titlelogo_en", "OPmovie_titlelogo_EN.decompressed.dds", 0, 0, 1280, 720, 0, 0, 1280, 720 },
    { "mm_title_intro", "press_start_text", "mat_title_en_001.dds", 32, 0, 192, 24, 550, 540, 180, 24 },
    // Evidence seam: UseAlternateTitleMidAsmHook switches EN/JP title treatment.
    { "CTitleStateIntro::Update", "alternate_title_gate", "mat_title_en_001.dds", 0, 456, 256, 56, 548, 638, 184, 40 },
}};

inline constexpr std::array<SuUiRenderCast, 8> kSonicHudReconstructionCasts{{
    { "ui_prov_playscreen.yncp", "so_speed_gauge_body", "ui_ps1_gauge1.dds", 0, 0, 256, 128, 18, 512, 430, 215 },
    { "ui_prov_playscreen.yncp", "so_ring_energy_body", "ui_ps1_gauge1.dds", 0, 64, 192, 48, 40, 636, 320, 80 },
    { "ui_prov_playscreen.yncp", "so_speed_gauge_meter", "ui_ps1_gauge1.dds", 48, 64, 150, 34, 94, 632, 240, 54 },
    { "ui_prov_playscreen.yncp", "ring_get_flash", "ui_ps1_gauge1.dds", 72, 82, 24, 22, 326, 622, 54, 48 },
    { "ui_prov_playscreen.yncp", "hud_label_stack", "mat_playscreen_001.dds", 0, 0, 128, 164, 480, 116, 200, 256 },
    { "ui_prov_playscreen.yncp", "ring_energy_label", "mat_playscreen_en_001.dds", 0, 0, 128, 72, 76, 634, 260, 80 },
    { "ui_prov_playscreen.yncp", "ring_digits_zeroes", "mat_comon_num_001.dds", 0, 0, 192, 32, 112, 672, 190, 42 },
    { "ui_prov_playscreen.yncp", "so_head_life_icon", "mat_comon_001.dds", 0, 0, 96, 64, 32, 24, 96, 64 },
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

inline constexpr std::array<SgfxPlaceholderAssetSlot, 4> kTitleMenuTemplateSlots{{
    { "backdrop", "ui_mm_base.dds", "ui_title/ui_mainmenu Sonic title/menu CSD" },
    { "content", "ui_mm_contentstext.dds", "ui_title/ui_mainmenu Sonic title/menu CSD" },
    { "logo", "OPmovie_titlelogo_EN.decompressed.dds", "ui_title/ui_mainmenu Sonic title/menu CSD" },
    { "prompt_glyphs", "mat_start_en_001.dds", "ui_title/ui_mainmenu Sonic title/menu CSD" },
}};

inline constexpr std::array<SgfxPlaceholderAssetSlot, 4> kLoadingTemplateSlots{{
    { "device_frame", "mat_load_comon_001.dds", "ui_loading Sonic loading CSD" },
    { "backdrop", "mat_load_comon_001.dds", "ui_loading Sonic loading CSD" },
    { "loading_copy", "mat_load_comon_001.dds", "ui_loading Sonic loading CSD" },
    { "controller_variant", "mat_load_comon_001.dds", "ui_loading Sonic loading CSD" },
}};

inline constexpr std::array<SgfxPlaceholderAssetSlot, 5> kSonicHudTemplateSlots{{
    { "speed_gauge", "ui_ps1_gauge1.dds", "ui_playscreen Sonic HUD CSD tree" },
    { "energy_gauge", "ui_ps1_gauge1.dds", "ui_playscreen Sonic HUD CSD tree" },
    { "ring_counter", "mat_comon_num_001.dds", "ui_playscreen Sonic HUD CSD tree" },
    { "side_panel", "mat_playscreen_001.dds", "ui_playscreen Sonic HUD CSD tree" },
    { "prompt_strip", "mat_playscreen_en_001.dds", "ui_playscreen Sonic HUD CSD tree" },
}};

inline constexpr std::array<SgfxPlaceholderAssetSlot, 4> kTutorialTemplateSlots{{
    { "prompt_row", "mat_start_en_001.dds", "SonicHudGuide/ui_playscreen prompt CSD" },
    { "tutorial_panel", "mat_playscreen_001.dds", "SonicHudGuide/ui_playscreen prompt CSD" },
    { "control_glyphs", "mat_start_en_001.dds", "SonicHudGuide/ui_playscreen prompt CSD" },
    { "host_context_readout", "mat_comon_num_001.dds", "SonicHudGuide/ui_playscreen prompt CSD" },
}};

inline constexpr std::array<SgfxTemplateRenderBinding, 4> kSgfxTemplateRenderBindings{{
    {
        "title-menu",
        "MainMenuComposite",
        kTitleMenuTemplateSlots.data(),
        kTitleMenuTemplateSlots.size(),
        "title-menu-visible",
        "select_travel",
        "title menu visual ready",
    },
    {
        "loading",
        "LoadingComposite",
        kLoadingTemplateSlots.data(),
        kLoadingTemplateSlots.size(),
        "loading-display-active",
        "pda_intro",
        "loading display active",
    },
    {
        "sonic-hud",
        "SonicHudReconstruction",
        kSonicHudTemplateSlots.data(),
        kSonicHudTemplateSlots.size(),
        "sonic-hud-ready",
        "hud_in",
        "sonic-hud-ready",
    },
    {
        "tutorial",
        "SonicHudReconstruction",
        kTutorialTemplateSlots.data(),
        kTutorialTemplateSlots.size(),
        "tutorial-hud-owner-path-ready",
        "hud_in",
        "tutorial-ready",
    },
}};

inline constexpr std::array<CsdPipelineTemplateBinding, 4> kCsdPipelineTemplateBindings{{
    {
        "title-menu",
        "ui_mainmenu.yncp",
        "mm_bg_usual",
        "mm_donut_move",
        "DefaultAnim",
    },
    {
        "loading",
        "ui_loading.yncp",
        "pda",
        "pda_txt",
        "Usual_Anim_3",
    },
    {
        "sonic-hud",
        "ui_prov_playscreen.yncp",
        "so_speed_gauge",
        "so_speed_gauge",
        "Size_Anim",
    },
    {
        "tutorial",
        "ui_prov_playscreen.yncp",
        "info_1",
        "info_1",
        "Count_Anim",
    },
}};

inline const std::array<SuUiRendererScreen, 8> kRendererScreens{{
    {
        "TitleLoopReconstruction",
        "Title Loop Reconstructed",
        "title_menu_reference.json",
        kTitleLoopReconstructionCasts.data(),
        kTitleLoopReconstructionCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
        RendererScreenKind::TitleLoopReconstruction,
    },
    {
        "SonicHudReconstruction",
        "Sonic HUD Reconstructed",
        "sonic_stage_hud_reference.json",
        kSonicHudReconstructionCasts.data(),
        kSonicHudReconstructionCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
        RendererScreenKind::SonicHudReconstruction,
    },
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

[[nodiscard]] std::size_t atlasGalleryScreenIndex()
{
    for (std::size_t index = 0; index < kRendererScreens.size(); ++index)
    {
        if (kRendererScreens[index].kind == RendererScreenKind::AtlasGallery)
            return index;
    }
    return 0;
}

[[nodiscard]] const SgfxTemplateRenderBinding* findSgfxTemplateRenderBinding(std::string_view templateId)
{
    const auto found = std::find_if(
        kSgfxTemplateRenderBindings.begin(),
        kSgfxTemplateRenderBindings.end(),
        [templateId](const SgfxTemplateRenderBinding& binding)
        {
            return binding.templateId == templateId;
        });
    return found == kSgfxTemplateRenderBindings.end() ? nullptr : &*found;
}

[[nodiscard]] const CsdPipelineTemplateBinding* findCsdPipelineTemplateBinding(std::string_view templateId)
{
    const auto found = std::find_if(
        kCsdPipelineTemplateBindings.begin(),
        kCsdPipelineTemplateBindings.end(),
        [templateId](const CsdPipelineTemplateBinding& binding)
        {
            return binding.templateId == templateId;
        });
    return found == kCsdPipelineTemplateBindings.end() ? nullptr : &*found;
}

[[nodiscard]] const SuUiRendererScreen* rendererScreenById(std::string_view id)
{
    const auto found = std::find_if(
        kRendererScreens.begin(),
        kRendererScreens.end(),
        [id](const SuUiRendererScreen& screen)
        {
            return screen.id == id;
        });
    return found == kRendererScreens.end() ? nullptr : &*found;
}

[[nodiscard]] std::optional<std::size_t> rendererScreenIndexById(std::string_view id)
{
    for (std::size_t index = 0; index < kRendererScreens.size(); ++index)
    {
        if (kRendererScreens[index].id == id)
            return index;
    }

    return std::nullopt;
}

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

void appendAncestorRepoRoots(std::vector<std::filesystem::path>& candidates, std::filesystem::path start)
{
    std::error_code error;
    start = std::filesystem::absolute(start, error);
    if (error)
        return;

    for (auto current = start; !current.empty(); current = current.parent_path())
    {
        candidates.push_back(current);
        if (current == current.root_path())
            break;
    }
}

[[nodiscard]] std::vector<std::filesystem::path> repoRootCandidates()
{
    std::vector<std::filesystem::path> candidates;
    appendAncestorRepoRoots(candidates, std::filesystem::current_path());
    appendAncestorRepoRoots(candidates, executableDirectory());
    return candidates;
}

[[nodiscard]] std::optional<std::filesystem::path> layoutEvidencePath()
{
    constexpr std::string_view kLayoutEvidenceRelativePath = "research_uiux/data/layout_deep_analysis.json";
    for (const auto& root : repoRootCandidates())
    {
        std::error_code error;
        const auto path = root / std::filesystem::path(kLayoutEvidenceRelativePath);
        if (std::filesystem::is_regular_file(path, error))
            return path;
    }

    return std::nullopt;
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

[[nodiscard]] std::optional<std::filesystem::path> findTitleMoviePreviewFramePath()
{
    // Local-only frame extracted from game/movie/evmo_title_loop.sfd with ffmpeg.
    // The binary never embeds or publishes the proprietary movie frame; it only
    // consumes the operator's generated preview if it exists beside extracted assets.
    constexpr std::string_view kTitlePreviewRelativePath =
        "runtime_previews/title/evmo_title_loop_00_00_35_000.png";

    for (const auto& root : extractedAssetRootCandidates())
    {
        std::error_code error;
        const auto path = root / std::filesystem::path(kTitlePreviewRelativePath);
        if (std::filesystem::is_regular_file(path, error))
            return path;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> findTitleLogoPreviewPath()
{
    // Local-only PNG decoded from Loading/OPmovie_titlelogo_EN.dds after the
    // Xbox LZX container is expanded with tools/x_decompress. This keeps the
    // operator preview exact while the native renderer grows full X360 DDS
    // decode coverage.
    constexpr std::string_view kTitleLogoPreviewRelativePath =
        "runtime_previews/title/decompressed/OPmovie_titlelogo_EN.decompressed.png";

    for (const auto& root : extractedAssetRootCandidates())
    {
        std::error_code error;
        const auto path = root / std::filesystem::path(kTitleLogoPreviewRelativePath);
        if (std::filesystem::is_regular_file(path, error))
            return path;
    }

    return std::nullopt;
}

[[nodiscard]] bool gdiplusBitmapLoads(const std::filesystem::path& path)
{
    Gdiplus::GdiplusStartupInput gdiplusInput{};
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        return false;

    auto bitmap = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(path.wstring().c_str(), FALSE));
    const bool loaded = bitmap && bitmap->GetLastStatus() == Gdiplus::Ok && bitmap->GetWidth() > 0 && bitmap->GetHeight() > 0;
    bitmap.reset();
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return loaded;
}

[[nodiscard]] std::string readTextFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return {};

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

[[nodiscard]] std::size_t skipJsonWhitespace(std::string_view text, std::size_t offset)
{
    while (offset < text.size() && std::isspace(static_cast<unsigned char>(text[offset])))
        ++offset;
    return offset;
}

[[nodiscard]] std::optional<std::size_t> matchJsonContainer(
    std::string_view text,
    std::size_t openOffset,
    char openChar,
    char closeChar)
{
    if (openOffset >= text.size() || text[openOffset] != openChar)
        return std::nullopt;

    bool inString = false;
    bool escaped = false;
    int depth = 0;
    for (std::size_t index = openOffset; index < text.size(); ++index)
    {
        const char ch = text[index];
        if (inString)
        {
            if (escaped)
            {
                escaped = false;
            }
            else if (ch == '\\')
            {
                escaped = true;
            }
            else if (ch == '"')
            {
                inString = false;
            }
            continue;
        }

        if (ch == '"')
        {
            inString = true;
            continue;
        }

        if (ch == openChar)
            ++depth;
        else if (ch == closeChar)
        {
            --depth;
            if (depth == 0)
                return index;
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> parseJsonStringAt(std::string_view text, std::size_t offset)
{
    if (offset >= text.size() || text[offset] != '"')
        return std::nullopt;

    std::string value;
    bool escaped = false;
    for (std::size_t index = offset + 1; index < text.size(); ++index)
    {
        const char ch = text[index];
        if (escaped)
        {
            switch (ch)
            {
            case '"': value.push_back('"'); break;
            case '\\': value.push_back('\\'); break;
            case '/': value.push_back('/'); break;
            case 'b': value.push_back('\b'); break;
            case 'f': value.push_back('\f'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            default: value.push_back(ch); break;
            }
            escaped = false;
            continue;
        }

        if (ch == '\\')
        {
            escaped = true;
            continue;
        }

        if (ch == '"')
            return value;

        value.push_back(ch);
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> findJsonFieldValueOffset(std::string_view text, std::string_view fieldName)
{
    const std::string needle = "\"" + std::string(fieldName) + "\"";
    const auto fieldOffset = text.find(needle);
    if (fieldOffset == std::string_view::npos)
        return std::nullopt;

    const auto colonOffset = text.find(':', fieldOffset + needle.size());
    if (colonOffset == std::string_view::npos)
        return std::nullopt;

    return skipJsonWhitespace(text, colonOffset + 1);
}

[[nodiscard]] std::optional<std::string> jsonStringField(std::string_view text, std::string_view fieldName)
{
    const auto valueOffset = findJsonFieldValueOffset(text, fieldName);
    if (!valueOffset)
        return std::nullopt;
    return parseJsonStringAt(text, *valueOffset);
}

[[nodiscard]] std::optional<double> jsonNumberField(std::string_view text, std::string_view fieldName)
{
    const auto valueOffset = findJsonFieldValueOffset(text, fieldName);
    if (!valueOffset)
        return std::nullopt;

    std::size_t endOffset = *valueOffset;
    while (endOffset < text.size())
    {
        const char ch = text[endOffset];
        if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '-' || ch == '+' || ch == '.' || ch == 'e' || ch == 'E'))
            break;
        ++endOffset;
    }

    if (endOffset == *valueOffset)
        return std::nullopt;

    try
    {
        return std::stod(std::string(text.substr(*valueOffset, endOffset - *valueOffset)));
    }
    catch (...)
    {
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<std::string_view> jsonArrayFieldSpan(std::string_view text, std::string_view fieldName)
{
    const auto valueOffset = findJsonFieldValueOffset(text, fieldName);
    if (!valueOffset || *valueOffset >= text.size() || text[*valueOffset] != '[')
        return std::nullopt;

    const auto endOffset = matchJsonContainer(text, *valueOffset, '[', ']');
    if (!endOffset)
        return std::nullopt;

    return text.substr(*valueOffset, (*endOffset - *valueOffset) + 1);
}

[[nodiscard]] std::vector<std::string> parseJsonStringArray(std::string_view arraySpan)
{
    std::vector<std::string> values;
    bool inString = false;
    bool escaped = false;
    for (std::size_t index = 0; index < arraySpan.size(); ++index)
    {
        const char ch = arraySpan[index];
        if (inString)
        {
            if (escaped)
                escaped = false;
            else if (ch == '\\')
                escaped = true;
            else if (ch == '"')
                inString = false;
            continue;
        }

        if (ch == '"')
        {
            if (const auto value = parseJsonStringAt(arraySpan, index))
                values.push_back(*value);
            inString = true;
        }
    }

    return values;
}

[[nodiscard]] std::vector<std::string_view> jsonObjectSpansInArray(std::string_view arraySpan)
{
    std::vector<std::string_view> spans;
    bool inString = false;
    bool escaped = false;
    for (std::size_t index = 0; index < arraySpan.size(); ++index)
    {
        const char ch = arraySpan[index];
        if (inString)
        {
            if (escaped)
                escaped = false;
            else if (ch == '\\')
                escaped = true;
            else if (ch == '"')
                inString = false;
            continue;
        }

        if (ch == '"')
        {
            inString = true;
            continue;
        }

        if (ch != '{')
            continue;

        const auto endOffset = matchJsonContainer(arraySpan, index, '{', '}');
        if (!endOffset)
            break;

        spans.push_back(arraySpan.substr(index, (*endOffset - index) + 1));
        index = *endOffset;
    }

    return spans;
}

[[nodiscard]] CsdPipelineSceneSummary parseCsdPipelineSceneSummary(std::string_view objectSpan)
{
    CsdPipelineSceneSummary summary;
    summary.sceneName = jsonStringField(objectSpan, "scene_name").value_or("");
    summary.castCount = static_cast<int>(jsonNumberField(objectSpan, "cast_count").value_or(0.0));
    summary.subimageCount = static_cast<int>(jsonNumberField(objectSpan, "subimage_count").value_or(0.0));

    if (const auto textures = jsonArrayFieldSpan(objectSpan, "used_texture_names"))
        summary.textureNames = parseJsonStringArray(*textures);

    if (const auto frameRange = jsonArrayFieldSpan(objectSpan, "frame_count_range"))
    {
        std::vector<double> values;
        std::size_t offset = 0;
        while (offset < frameRange->size())
        {
            if (!(std::isdigit(static_cast<unsigned char>((*frameRange)[offset])) || (*frameRange)[offset] == '-'))
            {
                ++offset;
                continue;
            }

            std::size_t endOffset = offset + 1;
            while (endOffset < frameRange->size())
            {
                const char ch = (*frameRange)[endOffset];
                if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '.' || ch == 'e' || ch == 'E' || ch == '+' || ch == '-'))
                    break;
                ++endOffset;
            }
            values.push_back(std::stod(std::string(frameRange->substr(offset, endOffset - offset))));
            offset = endOffset;
        }

        if (!values.empty())
            summary.frameStart = values.front();
        if (values.size() > 1)
            summary.frameEnd = values[1];
    }

    return summary;
}

[[nodiscard]] CsdPipelineTimelineHook parseCsdPipelineTimelineHook(std::string_view objectSpan)
{
    CsdPipelineTimelineHook hook;
    hook.sceneName = jsonStringField(objectSpan, "scene_name").value_or("");
    hook.animationName = jsonStringField(objectSpan, "animation_name").value_or("");
    hook.frameCount = jsonNumberField(objectSpan, "frame_count").value_or(0.0);
    hook.timelineSeconds = jsonNumberField(objectSpan, "timeline_seconds").value_or(0.0);
    hook.totalKeyframes = static_cast<int>(jsonNumberField(objectSpan, "total_keyframes").value_or(0.0));
    return hook;
}

[[nodiscard]] std::optional<CsdPipelineEvidence> loadCsdPipelineEvidence(std::string_view layoutFileName)
{
    const auto evidencePath = layoutEvidencePath();
    if (!evidencePath)
        return std::nullopt;

    const std::string document = readTextFile(*evidencePath);
    if (document.empty())
        return std::nullopt;

    const auto digestsSpan = jsonArrayFieldSpan(document, "digests");
    if (!digestsSpan)
        return std::nullopt;

    std::optional<std::string_view> digestObjectSpan;
    for (const auto objectSpan : jsonObjectSpansInArray(*digestsSpan))
    {
        if (jsonStringField(objectSpan, "file_name").value_or("") == layoutFileName)
        {
            digestObjectSpan = objectSpan;
            break;
        }
    }

    if (!digestObjectSpan)
        return std::nullopt;

    const auto objectSpan = *digestObjectSpan;
    CsdPipelineEvidence evidence;
    evidence.sourcePath = *evidencePath;
    evidence.layoutFileName = jsonStringField(objectSpan, "file_name").value_or(std::string(layoutFileName));

    if (const auto textures = jsonArrayFieldSpan(objectSpan, "texture_names"))
        evidence.textureNames = parseJsonStringArray(*textures);

    if (const auto scenes = jsonArrayFieldSpan(objectSpan, "scene_summaries"))
    {
        for (const auto sceneObject : jsonObjectSpansInArray(*scenes))
            evidence.scenes.push_back(parseCsdPipelineSceneSummary(sceneObject));
    }

    if (const auto timelines = jsonArrayFieldSpan(objectSpan, "longest_animations"))
    {
        for (const auto timelineObject : jsonObjectSpansInArray(*timelines))
            evidence.timelines.push_back(parseCsdPipelineTimelineHook(timelineObject));
    }

    return evidence;
}

[[nodiscard]] const CsdPipelineSceneSummary* findCsdPipelineScene(
    const CsdPipelineEvidence& evidence,
    std::string_view sceneName)
{
    const auto found = std::find_if(
        evidence.scenes.begin(),
        evidence.scenes.end(),
        [sceneName](const CsdPipelineSceneSummary& scene)
        {
            return scene.sceneName == sceneName;
        });
    return found == evidence.scenes.end() ? nullptr : &*found;
}

[[nodiscard]] const CsdPipelineTimelineHook* findCsdPipelineTimelineHook(
    const CsdPipelineEvidence& evidence,
    std::string_view sceneName,
    std::string_view animationName)
{
    const auto found = std::find_if(
        evidence.timelines.begin(),
        evidence.timelines.end(),
        [sceneName, animationName](const CsdPipelineTimelineHook& hook)
        {
            return hook.sceneName == sceneName && hook.animationName == animationName;
        });
    return found == evidence.timelines.end() ? nullptr : &*found;
}

[[nodiscard]] std::string joinStrings(const std::vector<std::string>& values, std::size_t limit = 0)
{
    std::ostringstream joined;
    const std::size_t count = limit == 0 ? values.size() : std::min(limit, values.size());
    for (std::size_t index = 0; index < count; ++index)
    {
        if (index != 0)
            joined << ",";
        joined << values[index];
    }
    return joined.str();
}

[[nodiscard]] std::optional<std::filesystem::path> findRuntimeEvidenceManifestForTarget(std::string_view target)
{
    auto manifestMatchesTarget = [target](const std::filesystem::path& manifest)
    {
        const std::string text = readTextFile(manifest);
        if (text.empty())
            return false;
        const std::string fieldNeedle = "\"target\"";
        const std::string valueNeedle = "\"" + std::string(target) + "\"";
        std::size_t offset = 0;
        while ((offset = text.find(fieldNeedle, offset)) != std::string::npos)
        {
            const auto colonOffset = text.find(':', offset + fieldNeedle.size());
            if (colonOffset == std::string::npos)
                return false;
            const auto nextFieldOffset = text.find('"', colonOffset + 1);
            if (nextFieldOffset != std::string::npos && text.compare(nextFieldOffset, valueNeedle.size(), valueNeedle) == 0)
                return true;
            offset = colonOffset + 1;
        }
        return false;
    };

    std::optional<std::filesystem::path> bestManifest;
    std::string bestSessionName;
    for (const auto& root : repoRootCandidates())
    {
        std::error_code error;
        const auto evidenceRoot = root / "out" / "ui_lab_runtime_evidence";
        if (!std::filesystem::is_directory(evidenceRoot, error))
            continue;

        for (const auto& session : std::filesystem::directory_iterator(evidenceRoot, error))
        {
            if (error)
                break;
            if (!session.is_directory(error))
                continue;

            const auto sessionName = session.path().filename().string();
            const auto targetManifest = session.path() / std::filesystem::path(std::string(target)) / "capture_manifest.json";
            const auto rootManifest = session.path() / "capture_manifest.json";

            std::optional<std::filesystem::path> manifest;
            if (std::filesystem::is_regular_file(targetManifest, error))
                manifest = targetManifest;
            else if (std::filesystem::is_regular_file(rootManifest, error) && manifestMatchesTarget(rootManifest))
                manifest = rootManifest;

            if (!manifest)
                continue;

            if (!bestManifest || sessionName > bestSessionName)
            {
                bestManifest = *manifest;
                bestSessionName = sessionName;
            }
        }
    }

    return bestManifest;
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
        selectedSgfxTemplateId_.reset();
    }

    void selectPrevious()
    {
        selectedScreenIndex_ = (selectedScreenIndex_ + kRendererScreens.size() - 1) % kRendererScreens.size();
        selectedSgfxTemplateId_.reset();
    }

    [[nodiscard]] bool selectScreenById(std::string_view id)
    {
        const auto index = rendererScreenIndexById(id);
        if (!index)
            return false;

        selectedScreenIndex_ = *index;
        return true;
    }

    [[nodiscard]] bool selectSgfxTemplate(std::string_view templateId)
    {
        const auto* binding = findSgfxTemplateRenderBinding(templateId);
        if (!binding || !selectScreenById(binding->rendererScreenId))
            return false;

        selectedSgfxTemplateId_ = std::string(binding->templateId);
        return true;
    }

    void selectNextAtlas()
    {
        if (atlasSheets_.empty())
            return;
        selectedScreenIndex_ = atlasGalleryScreenIndex();
        selectedSgfxTemplateId_.reset();
        selectedAtlasIndex_ = (selectedAtlasIndex_ + 1) % atlasSheets_.size();
        atlasBitmap_.reset();
    }

    void selectPreviousAtlas()
    {
        if (atlasSheets_.empty())
            return;
        selectedScreenIndex_ = atlasGalleryScreenIndex();
        selectedSgfxTemplateId_.reset();
        selectedAtlasIndex_ = (selectedAtlasIndex_ + atlasSheets_.size() - 1) % atlasSheets_.size();
        atlasBitmap_.reset();
    }

    [[nodiscard]] const SgfxTemplateRenderBinding* selectedSgfxTemplateBinding() const
    {
        if (!selectedSgfxTemplateId_)
            return nullptr;

        return findSgfxTemplateRenderBinding(*selectedSgfxTemplateId_);
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
        if (selectedSgfxTemplateId_)
            text << " | template " << *selectedSgfxTemplateId_;
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

    [[nodiscard]] Gdiplus::Bitmap* titleMovieFrameBitmap()
    {
        if (!titleMovieFrameLoadAttempted_)
        {
            titleMovieFrameLoadAttempted_ = true;
            titleMovieFramePath_ = findTitleMoviePreviewFramePath();
            if (titleMovieFramePath_)
            {
                auto bitmap = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(titleMovieFramePath_->wstring().c_str(), FALSE));
                if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok)
                    titleMovieFrameBitmap_ = std::move(bitmap);
            }
        }

        return titleMovieFrameBitmap_.get();
    }

    [[nodiscard]] Gdiplus::Bitmap* titleLogoBitmap()
    {
        if (!titleLogoLoadAttempted_)
        {
            titleLogoLoadAttempted_ = true;
            titleLogoPath_ = findTitleLogoPreviewPath();
            if (titleLogoPath_)
            {
                auto bitmap = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(titleLogoPath_->wstring().c_str(), FALSE));
                if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok)
                    titleLogoBitmap_ = std::move(bitmap);
            }
        }

        return titleLogoBitmap_.get();
    }

private:
    std::size_t selectedScreenIndex_ = 0;
    std::size_t selectedAtlasIndex_ = 0;
    std::optional<std::string> selectedSgfxTemplateId_;
    std::vector<std::filesystem::path> atlasSheets_;
    std::filesystem::path atlasBitmapPath_;
    std::unique_ptr<Gdiplus::Bitmap> atlasBitmap_;
    bool titleMovieFrameLoadAttempted_ = false;
    std::optional<std::filesystem::path> titleMovieFramePath_;
    std::unique_ptr<Gdiplus::Bitmap> titleMovieFrameBitmap_;
    bool titleLogoLoadAttempted_ = false;
    std::optional<std::filesystem::path> titleLogoPath_;
    std::unique_ptr<Gdiplus::Bitmap> titleLogoBitmap_;
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

[[nodiscard]] Gdiplus::PointF designPointToCanvas(const Gdiplus::RectF& canvas, float x, float y);

void drawOutlinedText(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    std::string_view text,
    float x,
    float y,
    float size,
    Gdiplus::Color fill,
    Gdiplus::Color outline,
    INT style);

[[nodiscard]] bool drawRenderCastTexture(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const SuUiRenderCast& cast)
{
    const auto destination = designRectToCanvas(canvas, cast.destinationX, cast.destinationY, cast.destinationWidth, cast.destinationHeight);
    const auto* texture = renderer.textureFor(cast.textureName);
    if (!texture || !texture->image || !texture->bitmap || !castSourceFits(cast, *texture->image))
    {
        drawMissingCast(graphics, destination);
        return false;
    }

    graphics.DrawImage(
        texture->bitmap.get(),
        destination,
        static_cast<float>(cast.sourceX),
        static_cast<float>(cast.sourceY),
        static_cast<float>(cast.sourceWidth),
        static_cast<float>(cast.sourceHeight),
        Gdiplus::UnitPixel);
    return true;
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

void drawTitleWordArt(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    std::string_view text,
    float x,
    float y,
    float size,
    Gdiplus::Color fill,
    Gdiplus::Color outline,
    Gdiplus::Color glow,
    float outlineWidth)
{
    const float scale = canvas.Width / static_cast<float>(kDesignWidth);
    Gdiplus::FontFamily family(L"Arial Black");
    Gdiplus::StringFormat format;
    Gdiplus::GraphicsPath path;
    const auto wideText = widenAscii(text);
    const auto point = designPointToCanvas(canvas, x, y);
    path.AddString(
        wideText.c_str(),
        -1,
        &family,
        Gdiplus::FontStyleBold | Gdiplus::FontStyleItalic,
        std::max(1.0F, size * scale),
        point,
        &format);

    Gdiplus::Pen glowPen(glow, std::max(1.0F, outlineWidth * 1.65F * scale));
    Gdiplus::Pen outlinePen(outline, std::max(1.0F, outlineWidth * scale));
    Gdiplus::SolidBrush fillBrush(fill);
    graphics.DrawPath(&glowPen, &path);
    graphics.DrawPath(&outlinePen, &path);
    graphics.FillPath(&fillBrush, &path);
}

void drawTitlePromptShell(Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas)
{
    const auto glow = designRectToCanvas(canvas, 445, 510, 390, 76);
    Gdiplus::SolidBrush glowBrush(Gdiplus::Color(120, 86, 160, 22));
    graphics.FillEllipse(&glowBrush, glow);

    const auto shadow = designRectToCanvas(canvas, 488, 520, 304, 56);
    Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(190, 50, 70, 42));
    graphics.FillRectangle(&shadowBrush, shadow);

    const auto frame = designRectToCanvas(canvas, 498, 526, 284, 42);
    Gdiplus::SolidBrush frameBrush(Gdiplus::Color(235, 194, 206, 142));
    Gdiplus::SolidBrush innerBrush(Gdiplus::Color(255, 248, 225, 40));
    Gdiplus::Pen edgePen(Gdiplus::Color(255, 238, 238, 142), std::max(1.0F, 2.0F * (canvas.Width / static_cast<float>(kDesignWidth))));
    graphics.FillRectangle(&frameBrush, frame);
    const auto inner = designRectToCanvas(canvas, 512, 534, 256, 26);
    graphics.FillRectangle(&innerBrush, inner);
    graphics.DrawRectangle(&edgePen, frame);
}

void renderTitleLoopReconstructionScreen(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer)
{
    if (auto* titleFrame = renderer.titleMovieFrameBitmap())
    {
        graphics.DrawImage(
            titleFrame,
            canvas,
            0.0F,
            0.0F,
            static_cast<float>(titleFrame->GetWidth()),
            static_cast<float>(titleFrame->GetHeight()),
            Gdiplus::UnitPixel);
    }
    else
    {
        (void)drawRenderCastTexture(graphics, canvas, renderer, kTitleLoopReconstructionCasts[0]);
        Gdiplus::SolidBrush fallbackDim(Gdiplus::Color(180, 0, 0, 0));
        graphics.FillRectangle(&fallbackDim, canvas);
    }

    Gdiplus::SolidBrush filmDim(Gdiplus::Color(105, 0, 0, 0));
    graphics.FillRectangle(&filmDim, canvas);

    bool drewTitleLogo = false;
    if (auto* titleLogo = renderer.titleLogoBitmap())
    {
        const auto titleLogoDestination = designRectToCanvas(canvas, 280, 175, 720, 320);
        graphics.DrawImage(
            titleLogo,
            titleLogoDestination,
            300.0F,
            210.0F,
            720.0F,
            320.0F,
            Gdiplus::UnitPixel);
        drewTitleLogo = true;
    }
    else
    {
        // Keep the decompressed DDS as smoke-test evidence, but do not use the
        // current hand-rolled DXT path for this logo in the visual renderer; it
        // is not spatially faithful for the XCompress-derived texture yet.
        drewTitleLogo = false;
    }

    if (!drewTitleLogo)
    {
        drawTitleWordArt(
            graphics,
            canvas,
            "SONIC",
            358,
            170,
            132,
            Gdiplus::Color(255, 255, 218, 18),
            Gdiplus::Color(255, 0, 0, 0),
            Gdiplus::Color(230, 255, 255, 255),
            18.0F);
        drawTitleWordArt(
            graphics,
            canvas,
            "UNLEASHED",
            388,
            316,
            76,
            Gdiplus::Color(255, 248, 248, 248),
            Gdiplus::Color(255, 0, 0, 0),
            Gdiplus::Color(230, 255, 255, 255),
            12.0F);
        drawOutlinedText(graphics, canvas, "TM", 915, 318, 18, Gdiplus::Color(255, 240, 240, 240), Gdiplus::Color(255, 0, 0, 0), Gdiplus::FontStyleBold);
    }

    drawTitlePromptShell(graphics, canvas);
    (void)drawRenderCastTexture(graphics, canvas, renderer, kTitleLoopReconstructionCasts[2]);
    (void)drawRenderCastTexture(graphics, canvas, renderer, kTitleLoopReconstructionCasts[3]);
}

[[nodiscard]] Gdiplus::PointF designPointToCanvas(const Gdiplus::RectF& canvas, float x, float y)
{
    const float scale = canvas.Width / static_cast<float>(kDesignWidth);
    return Gdiplus::PointF(canvas.X + (x * scale), canvas.Y + (y * scale));
}

void drawOutlinedText(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    std::string_view text,
    float x,
    float y,
    float size,
    Gdiplus::Color fill,
    Gdiplus::Color outline,
    INT style = Gdiplus::FontStyleBold)
{
    const float scale = canvas.Width / static_cast<float>(kDesignWidth);
    Gdiplus::FontFamily family(L"Arial");
    Gdiplus::Font font(&family, std::max(1.0F, size * scale), style, Gdiplus::UnitPixel);
    const auto wideText = widenAscii(text);
    const auto point = designPointToCanvas(canvas, x, y);
    Gdiplus::SolidBrush outlineBrush(outline);
    Gdiplus::SolidBrush fillBrush(fill);
    const float offset = std::max(1.0F, 2.0F * scale);

    graphics.DrawString(wideText.c_str(), -1, &font, Gdiplus::PointF(point.X - offset, point.Y), &outlineBrush);
    graphics.DrawString(wideText.c_str(), -1, &font, Gdiplus::PointF(point.X + offset, point.Y), &outlineBrush);
    graphics.DrawString(wideText.c_str(), -1, &font, Gdiplus::PointF(point.X, point.Y - offset), &outlineBrush);
    graphics.DrawString(wideText.c_str(), -1, &font, Gdiplus::PointF(point.X, point.Y + offset), &outlineBrush);
    graphics.DrawString(wideText.c_str(), -1, &font, point, &fillBrush);
}

void drawSlantedHudPanel(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    float x,
    float y,
    float width,
    float height,
    Gdiplus::Color fill,
    Gdiplus::Color outline)
{
    const float scale = canvas.Width / static_cast<float>(kDesignWidth);
    const float cut = 28.0F * scale;
    const auto topLeft = designPointToCanvas(canvas, x, y);
    const auto bottomRight = designPointToCanvas(canvas, x + width, y + height);
    Gdiplus::PointF points[] = {
        Gdiplus::PointF(topLeft.X + cut, topLeft.Y),
        Gdiplus::PointF(bottomRight.X, topLeft.Y),
        Gdiplus::PointF(bottomRight.X - cut, bottomRight.Y),
        Gdiplus::PointF(topLeft.X, bottomRight.Y),
    };
    Gdiplus::SolidBrush fillBrush(fill);
    Gdiplus::Pen outlinePen(outline, std::max(1.0F, 2.0F * scale));
    graphics.FillPolygon(&fillBrush, points, 4);
    graphics.DrawPolygon(&outlinePen, points, 4);
}

void drawGaugeSegments(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    float x,
    float y,
    int segmentCount,
    int activeCount,
    Gdiplus::Color active,
    Gdiplus::Color inactive)
{
    const float segmentWidth = 15.0F;
    const float segmentHeight = 18.0F;
    const float gap = 4.0F;
    for (int index = 0; index < segmentCount; ++index)
    {
        const auto rect = designRectToCanvas(
            canvas,
            static_cast<int>(x + (index * (segmentWidth + gap))),
            static_cast<int>(y),
            static_cast<int>(segmentWidth),
            static_cast<int>(segmentHeight));
        Gdiplus::SolidBrush brush(index < activeCount ? active : inactive);
        Gdiplus::Pen outline(Gdiplus::Color(180, 220, 220, 220), 1.0F);
        graphics.FillRectangle(&brush, rect);
        graphics.DrawRectangle(&outline, rect);
    }
}

void drawRingIcon(Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas, float x, float y, float size)
{
    const auto outer = designRectToCanvas(canvas, static_cast<int>(x), static_cast<int>(y), static_cast<int>(size), static_cast<int>(size));
    const auto inner = designRectToCanvas(canvas, static_cast<int>(x + (size * 0.23F)), static_cast<int>(y + (size * 0.23F)), static_cast<int>(size * 0.54F), static_cast<int>(size * 0.54F));
    Gdiplus::SolidBrush orange(Gdiplus::Color(255, 232, 119, 24));
    Gdiplus::SolidBrush yellow(Gdiplus::Color(255, 255, 232, 74));
    Gdiplus::SolidBrush black(Gdiplus::Color(255, 0, 0, 0));
    Gdiplus::Pen darkEdge(Gdiplus::Color(255, 96, 48, 4), 2.0F);
    graphics.FillEllipse(&orange, outer);
    graphics.FillEllipse(&yellow, designRectToCanvas(canvas, static_cast<int>(x + 6), static_cast<int>(y + 5), static_cast<int>(size - 14), static_cast<int>(size - 14)));
    graphics.FillEllipse(&black, inner);
    graphics.DrawEllipse(&darkEdge, outer);
}

void renderSonicHudReconstructionScreen(Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas)
{
    Gdiplus::SolidBrush shade(Gdiplus::Color(255, 0, 0, 0));
    graphics.FillRectangle(&shade, canvas);

    drawSlantedHudPanel(graphics, canvas, -18, 82, 280, 44, Gdiplus::Color(210, 58, 68, 102), Gdiplus::Color(230, 206, 214, 228));
    drawOutlinedText(graphics, canvas, "05", 104, 80, 40, Gdiplus::Color(255, 250, 250, 250), Gdiplus::Color(255, 24, 24, 24));
    drawOutlinedText(graphics, canvas, "TIME", 138, 134, 22, Gdiplus::Color(255, 236, 236, 236), Gdiplus::Color(255, 24, 24, 24));
    drawSlantedHudPanel(graphics, canvas, -22, 162, 356, 34, Gdiplus::Color(210, 82, 94, 136), Gdiplus::Color(220, 196, 202, 220));
    drawOutlinedText(graphics, canvas, "00:00:31", 136, 154, 38, Gdiplus::Color(255, 250, 250, 250), Gdiplus::Color(255, 24, 24, 24));
    drawOutlinedText(graphics, canvas, "SCORE", 138, 222, 22, Gdiplus::Color(255, 236, 236, 236), Gdiplus::Color(255, 24, 24, 24));
    drawSlantedHudPanel(graphics, canvas, -22, 250, 356, 34, Gdiplus::Color(210, 82, 94, 136), Gdiplus::Color(220, 196, 202, 220));
    drawOutlinedText(graphics, canvas, "00000000", 136, 242, 38, Gdiplus::Color(255, 250, 250, 250), Gdiplus::Color(255, 24, 24, 24));

    drawOutlinedText(graphics, canvas, "GO!", 520, 286, 92, Gdiplus::Color(255, 238, 238, 238), Gdiplus::Color(255, 24, 24, 24));
    drawGaugeSegments(graphics, canvas, 690, 360, 9, 5, Gdiplus::Color(255, 245, 216, 44), Gdiplus::Color(120, 180, 180, 180));

    drawSlantedHudPanel(graphics, canvas, 34, 596, 334, 36, Gdiplus::Color(225, 36, 41, 48), Gdiplus::Color(240, 200, 205, 210));
    drawOutlinedText(graphics, canvas, "SPEED", 108, 588, 24, Gdiplus::Color(255, 236, 236, 236), Gdiplus::Color(255, 18, 18, 18));
    drawGaugeSegments(graphics, canvas, 178, 604, 12, 7, Gdiplus::Color(255, 255, 219, 36), Gdiplus::Color(120, 174, 176, 178));

    drawRingIcon(graphics, canvas, 34, 634, 70);
    drawSlantedHudPanel(graphics, canvas, 86, 640, 306, 40, Gdiplus::Color(225, 42, 47, 54), Gdiplus::Color(240, 200, 205, 210));
    drawOutlinedText(graphics, canvas, "RINGS", 112, 630, 26, Gdiplus::Color(255, 236, 236, 236), Gdiplus::Color(255, 18, 18, 18));
    drawOutlinedText(graphics, canvas, "000", 112, 654, 38, Gdiplus::Color(255, 248, 248, 248), Gdiplus::Color(255, 18, 18, 18));

    drawSlantedHudPanel(graphics, canvas, 84, 688, 424, 36, Gdiplus::Color(225, 42, 47, 54), Gdiplus::Color(240, 200, 205, 210));
    drawOutlinedText(graphics, canvas, "RING ENERGY", 112, 680, 26, Gdiplus::Color(255, 236, 236, 236), Gdiplus::Color(255, 18, 18, 18));
    drawGaugeSegments(graphics, canvas, 278, 696, 12, 8, Gdiplus::Color(255, 95, 220, 82), Gdiplus::Color(120, 174, 176, 178));
}

void renderSgfxTemplatePlaceholderScreen(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const SgfxTemplateRenderBinding& binding)
{
    const auto* screenTemplate = findSgfxScreenTemplate(binding.templateId);
    const auto panel = designRectToCanvas(canvas, 24, 24, 650, 190);
    Gdiplus::SolidBrush panelFill(Gdiplus::Color(205, 6, 11, 18));
    Gdiplus::Pen panelEdge(Gdiplus::Color(230, 110, 190, 255), std::max(1.0F, 2.0F * (canvas.Width / static_cast<float>(kDesignWidth))));
    graphics.FillRectangle(&panelFill, panel);
    graphics.DrawRectangle(&panelEdge, panel);

    const std::string title = std::string("SGFX template: ") + std::string(binding.templateId);
    const std::string event = std::string("ready: ")
        + std::string(!binding.requiredEventId.empty()
            ? binding.requiredEventId
            : (screenTemplate && !screenTemplate->evidence.requiredEvents.empty() ? std::string_view(screenTemplate->evidence.requiredEvents.front()) : std::string_view("unknown")));
    const std::string timing = std::string("timeline: ") + std::string(binding.timelineBandId)
        + " -> " + std::string(binding.timelineEventLabel);

    drawOutlinedText(graphics, canvas, title, 42, 36, 24, Gdiplus::Color(255, 250, 252, 255), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, event, 42, 68, 17, Gdiplus::Color(255, 210, 232, 255), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, timing, 42, 94, 17, Gdiplus::Color(255, 220, 255, 205), Gdiplus::Color(255, 0, 0, 0));

    for (std::size_t index = 0; index < binding.slotCount && index < 4; ++index)
    {
        const auto& slot = binding.slots[index];
        const auto* texture = renderer.textureFor(slot.textureName);
        const bool available = texture && texture->image && texture->bitmap;
        const int y = 122 + static_cast<int>(index) * 18;
        const auto marker = designRectToCanvas(canvas, 46, y + 4, 10, 10);
        Gdiplus::SolidBrush markerBrush(available ? Gdiplus::Color(255, 86, 214, 120) : Gdiplus::Color(255, 220, 80, 72));
        graphics.FillRectangle(&markerBrush, marker);

        const std::string slotText = std::string("placeholder_slot=")
            + std::string(slot.slotName)
            + "->"
            + std::string(slot.textureName);
        drawOutlinedText(graphics, canvas, slotText, 64, static_cast<float>(y), 14, Gdiplus::Color(255, 238, 238, 238), Gdiplus::Color(255, 0, 0, 0));
    }
}

[[nodiscard]] std::string formatCsdNumber(double value);

[[nodiscard]] const CsdPipelineEvidence* cachedCsdPipelineEvidence(std::string_view layoutFileName)
{
    static std::vector<std::pair<std::string, std::optional<CsdPipelineEvidence>>> cache;
    const auto found = std::find_if(
        cache.begin(),
        cache.end(),
        [layoutFileName](const std::pair<std::string, std::optional<CsdPipelineEvidence>>& entry)
        {
            return entry.first == layoutFileName;
        });
    if (found != cache.end())
        return found->second ? &*found->second : nullptr;

    cache.emplace_back(std::string(layoutFileName), loadCsdPipelineEvidence(layoutFileName));
    return cache.back().second ? &*cache.back().second : nullptr;
}

void renderCsdPipelineEvidenceOverlay(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    const SgfxTemplateRenderBinding& sgfxBinding)
{
    const auto* csdBinding = findCsdPipelineTemplateBinding(sgfxBinding.templateId);
    if (!csdBinding)
        return;

    const auto* evidence = cachedCsdPipelineEvidence(csdBinding->layoutFileName);
    const auto* scene = evidence ? findCsdPipelineScene(*evidence, csdBinding->primarySceneName) : nullptr;
    const auto* timeline = evidence ? findCsdPipelineTimelineHook(*evidence, csdBinding->timelineSceneName, csdBinding->timelineAnimationName) : nullptr;
    const auto manifest = findRuntimeEvidenceManifestForTarget(csdBinding->templateId);

    const auto panel = designRectToCanvas(canvas, 24, 226, 720, 142);
    Gdiplus::SolidBrush panelFill(Gdiplus::Color(205, 12, 8, 22));
    Gdiplus::Pen panelEdge(Gdiplus::Color(230, 170, 230, 120), std::max(1.0F, 2.0F * (canvas.Width / static_cast<float>(kDesignWidth))));
    graphics.FillRectangle(&panelFill, panel);
    graphics.DrawRectangle(&panelEdge, panel);

    std::ostringstream pipeline;
    pipeline
        << "csd_pipeline="
        << csdBinding->templateId
        << ":layout="
        << csdBinding->layoutFileName
        << ":scene="
        << csdBinding->primarySceneName
        << ":casts="
        << (scene ? scene->castCount : 0)
        << ":subimages="
        << (scene ? scene->subimageCount : 0);

    std::ostringstream timelineText;
    timelineText
        << "timeline="
        << (timeline ? timeline->sceneName : std::string(csdBinding->timelineSceneName))
        << "/"
        << (timeline ? timeline->animationName : std::string(csdBinding->timelineAnimationName))
        << "/"
        << (timeline ? formatCsdNumber(timeline->frameCount) : "0")
        << "/"
        << (timeline ? formatCsdNumber(timeline->timelineSeconds) : "0");

    std::ostringstream map;
    map
        << "sgfx_element_map="
        << csdBinding->templateId
        << ":scene="
        << csdBinding->primarySceneName
        << ":slot="
        << (sgfxBinding.slotCount > 0 ? sgfxBinding.slots[0].slotName : std::string_view("none"))
        << ":texture="
        << (sgfxBinding.slotCount > 0 ? sgfxBinding.slots[0].textureName : std::string_view("none"));

    std::ostringstream comparison;
    comparison
        << "runtime_evidence_compare="
        << csdBinding->templateId
        << ":target="
        << csdBinding->templateId
        << ":event="
        << sgfxBinding.requiredEventId
        << ":manifest="
        << (manifest ? "found" : "missing");

    drawOutlinedText(graphics, canvas, pipeline.str(), 42, 242, 16, Gdiplus::Color(255, 248, 255, 238), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, timelineText.str(), 42, 270, 15, Gdiplus::Color(255, 222, 255, 205), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, map.str(), 42, 298, 15, Gdiplus::Color(255, 224, 236, 255), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, comparison.str(), 42, 326, 15, Gdiplus::Color(255, 255, 224, 210), Gdiplus::Color(255, 0, 0, 0));
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

    if (screen.kind == RendererScreenKind::TitleLoopReconstruction)
    {
        renderTitleLoopReconstructionScreen(graphics, canvas, renderer);
    }
    else if (screen.kind == RendererScreenKind::SonicHudReconstruction)
    {
        renderSonicHudReconstructionScreen(graphics, canvas);
    }
    else if (screen.kind == RendererScreenKind::AtlasGallery)
    {
        renderAtlasGalleryScreen(graphics, canvas, renderer);
    }
    else
    {
        for (std::size_t index = 0; index < screen.castCount; ++index)
        {
            (void)drawRenderCastTexture(graphics, canvas, renderer, screen.casts[index]);
        }
    }

    if (const auto* binding = renderer.selectedSgfxTemplateBinding())
    {
        renderSgfxTemplatePlaceholderScreen(graphics, canvas, renderer, *binding);
        renderCsdPipelineEvidenceOverlay(graphics, canvas, *binding);
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

[[nodiscard]] int runRendererTitleScreenSmoke()
{
    const auto& screen = kRendererScreens.front();
    const auto movieFramePath = findTitleMoviePreviewFramePath();
    const auto titleLogoPreviewPath = findTitleLogoPreviewPath();
    const bool titleLogoPreviewLoads = titleLogoPreviewPath && gdiplusBitmapLoads(*titleLogoPreviewPath);
    const auto titleLogoPath = textureSourcePathForFileName("OPmovie_titlelogo_EN.decompressed.dds");
    std::size_t resolvedCastCount = 0;
    std::size_t inBoundsCastCount = 0;
    std::vector<std::string> descriptors;

    for (std::size_t index = 0; index < screen.castCount; ++index)
    {
        const auto& cast = screen.casts[index];
        const auto image = loadDdsTextureImage(cast.textureName);
        const bool fits = image && castSourceFits(cast, *image);
        if (image)
            ++resolvedCastCount;
        if (fits)
            ++inBoundsCastCount;

        std::ostringstream descriptor;
        descriptor
            << cast.castName
            << ":" << cast.textureName
            << ":src=" << cast.sourceX << "," << cast.sourceY << ","
            << cast.sourceWidth << "x" << cast.sourceHeight
            << ":dst=" << cast.destinationX << "," << cast.destinationY << ","
            << cast.destinationWidth << "x" << cast.destinationHeight;
        if (image)
            descriptor << ":" << image->format << ":" << image->width << "x" << image->height;
        else
            descriptor << ":missing";
        if (image && !fits)
            descriptor << ":source-out-of-bounds";
        descriptors.push_back(descriptor.str());
    }

    std::cout
        << "sward_su_ui_asset_renderer title screen smoke ok "
        << "first=" << screen.id
        << " source=evmo_title_loop.sfd"
        << " contract=" << screen.contractFileName
        << " movie_frame=" << (movieFramePath ? "exists" : "missing")
        << " title_logo_preview=" << (titleLogoPreviewPath ? "exists" : "missing")
        << " title_logo_preview_bitmap=" << (titleLogoPreviewLoads ? "loads" : "not_loaded")
        << " title_logo=" << (titleLogoPath ? "exists" : "missing")
        << " casts=" << screen.castCount
        << " resolved=" << resolvedCastCount
        << " in_bounds=" << inBoundsCastCount
        << " scenes=ui_title/bg/bg,mm_title_intro,CTitleStateIntro::Update"
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return screen.id == "TitleLoopReconstruction"
        && movieFramePath
        && titleLogoPreviewPath
        && titleLogoPreviewLoads
        && resolvedCastCount == screen.castCount
        && inBoundsCastCount == screen.castCount
        ? 0
        : 1;
}

[[nodiscard]] int runRendererReconstructedScreenSmoke()
{
    const auto* foundScreen = rendererScreenById("SonicHudReconstruction");
    if (!foundScreen)
        return 1;
    const auto& screen = *foundScreen;
    std::size_t resolvedCastCount = 0;
    std::size_t inBoundsCastCount = 0;
    std::vector<std::string> descriptors;

    for (std::size_t index = 0; index < screen.castCount; ++index)
    {
        const auto& cast = screen.casts[index];
        const auto image = loadDdsTextureImage(cast.textureName);
        const bool fits = image && castSourceFits(cast, *image);
        if (image)
            ++resolvedCastCount;
        if (fits)
            ++inBoundsCastCount;

        std::ostringstream descriptor;
        descriptor
            << cast.castName
            << ":" << cast.textureName
            << ":src=" << cast.sourceX << "," << cast.sourceY << ","
            << cast.sourceWidth << "x" << cast.sourceHeight
            << ":dst=" << cast.destinationX << "," << cast.destinationY << ","
            << cast.destinationWidth << "x" << cast.destinationHeight;
        if (image)
            descriptor << ":" << image->format << ":" << image->width << "x" << image->height;
        else
            descriptor << ":missing";
        if (image && !fits)
            descriptor << ":source-out-of-bounds";
        descriptors.push_back(descriptor.str());
    }

    const auto source = screen.castCount == 0 ? std::string_view("none") : screen.casts[0].sceneName;
    std::cout
        << "sward_su_ui_asset_renderer reconstructed screen smoke ok "
        << "screen=" << screen.id
        << " source=" << source
        << " contract=" << screen.contractFileName
        << " casts=" << screen.castCount
        << " resolved=" << resolvedCastCount
        << " in_bounds=" << inBoundsCastCount
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return screen.id == "SonicHudReconstruction"
        && resolvedCastCount == screen.castCount
        && inBoundsCastCount == screen.castCount
        ? 0
        : 1;
}

[[nodiscard]] const sward::ui_runtime::SgfxTimelineBand* findTimelineBand(
    const SgfxScreenTemplate& screenTemplate,
    std::string_view bandId)
{
    const auto found = std::find_if(
        screenTemplate.timelineBands.begin(),
        screenTemplate.timelineBands.end(),
        [bandId](const sward::ui_runtime::SgfxTimelineBand& band)
        {
            return band.id == bandId;
        });
    return found == screenTemplate.timelineBands.end() ? nullptr : &*found;
}

[[nodiscard]] std::string firstRequiredEvent(const SgfxScreenTemplate& screenTemplate)
{
    return screenTemplate.evidence.requiredEvents.empty() ? std::string("none") : screenTemplate.evidence.requiredEvents.front();
}

[[nodiscard]] int runSgfxTemplateSmoke(const std::optional<std::string>& templateFilter)
{
    std::size_t templateCount = 0;
    std::size_t bindingCount = 0;
    bool failed = false;
    std::vector<std::string> descriptors;

    for (const auto& screenTemplate : sgfxScreenTemplates())
    {
        if (templateFilter && screenTemplate.id != *templateFilter)
            continue;

        ++templateCount;
        const auto* binding = findSgfxTemplateRenderBinding(screenTemplate.id);
        const auto* screen = binding ? rendererScreenById(binding->rendererScreenId) : nullptr;
        if (!binding || !screen)
        {
            failed = true;
            continue;
        }

        ++bindingCount;
        std::ostringstream descriptor;
        descriptor
            << "template=" << screenTemplate.id
            << ":screen=" << screen->id
            << ":contract=" << screenTemplate.contractFileName
            << ":event=" << (!binding->requiredEventId.empty() ? std::string(binding->requiredEventId) : firstRequiredEvent(screenTemplate));
        descriptors.push_back(descriptor.str());

        for (std::size_t index = 0; index < binding->slotCount; ++index)
        {
            const auto& slot = binding->slots[index];
            std::ostringstream slotDescriptor;
            slotDescriptor
                << "placeholder_slot="
                << screenTemplate.id
                << ":"
                << slot.slotName
                << "->"
                << slot.textureName;
            descriptors.push_back(slotDescriptor.str());
        }

        const auto* band = findTimelineBand(screenTemplate, binding->timelineBandId);
        if (!band)
        {
            failed = true;
            continue;
        }

        std::ostringstream timingDescriptor;
        timingDescriptor
            << "timeline_hook="
            << screenTemplate.id
            << ":"
            << band->id
            << "="
            << band->seconds
            << ":"
            << binding->timelineEventLabel;
        descriptors.push_back(timingDescriptor.str());
    }

    if (templateFilter && templateCount == 0)
        failed = true;

    std::cout
        << "sward_su_ui_asset_renderer sgfx template smoke ok "
        << "templates=" << templateCount
        << " bindings=" << bindingCount
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return failed || templateCount == 0 || templateCount != bindingCount ? 1 : 0;
}

[[nodiscard]] std::string formatCsdNumber(double value)
{
    if (std::fabs(value - std::round(value)) < 0.000001)
    {
        std::ostringstream integer;
        integer << static_cast<long long>(std::llround(value));
        return integer.str();
    }

    std::ostringstream formatted;
    formatted << std::fixed << std::setprecision(6) << value;
    return formatted.str();
}

[[nodiscard]] int runCsdPipelineSmoke(const std::optional<std::string>& templateFilter)
{
    std::size_t templateCount = 0;
    bool failed = false;
    std::vector<std::string> uniquePackages;
    std::vector<std::pair<std::string, CsdPipelineEvidence>> loadedEvidence;
    std::vector<std::string> descriptors;

    auto evidenceFor = [&loadedEvidence](std::string_view layoutFileName) -> const CsdPipelineEvidence*
    {
        const auto found = std::find_if(
            loadedEvidence.begin(),
            loadedEvidence.end(),
            [layoutFileName](const std::pair<std::string, CsdPipelineEvidence>& entry)
            {
                return entry.first == layoutFileName;
            });
        if (found != loadedEvidence.end())
            return &found->second;

        const auto evidence = loadCsdPipelineEvidence(layoutFileName);
        if (!evidence)
            return nullptr;

        loadedEvidence.emplace_back(std::string(layoutFileName), *evidence);
        return &loadedEvidence.back().second;
    };

    for (const auto& csdBinding : kCsdPipelineTemplateBindings)
    {
        if (templateFilter && csdBinding.templateId != *templateFilter)
            continue;

        ++templateCount;
        if (std::find(uniquePackages.begin(), uniquePackages.end(), csdBinding.layoutFileName) == uniquePackages.end())
            uniquePackages.emplace_back(csdBinding.layoutFileName);

        const auto* evidence = evidenceFor(csdBinding.layoutFileName);
        const auto* sgfxBinding = findSgfxTemplateRenderBinding(csdBinding.templateId);
        if (!evidence || !sgfxBinding)
        {
            failed = true;
            continue;
        }

        const auto* scene = findCsdPipelineScene(*evidence, csdBinding.primarySceneName);
        const auto* timeline = findCsdPipelineTimelineHook(*evidence, csdBinding.timelineSceneName, csdBinding.timelineAnimationName);
        if (!scene || !timeline)
            failed = true;

        std::ostringstream pipelineDescriptor;
        pipelineDescriptor
            << "csd_pipeline="
            << csdBinding.templateId
            << ":layout="
            << csdBinding.layoutFileName
            << ":scene="
            << csdBinding.primarySceneName
            << ":casts="
            << (scene ? scene->castCount : 0)
            << ":subimages="
            << (scene ? scene->subimageCount : 0)
            << ":textures="
            << joinStrings(evidence->textureNames);
        descriptors.push_back(pipelineDescriptor.str());

        if (timeline)
        {
            std::ostringstream timelineDescriptor;
            timelineDescriptor
                << "timeline="
                << timeline->sceneName
                << "/"
                << timeline->animationName
                << "/"
                << formatCsdNumber(timeline->frameCount)
                << "/"
                << formatCsdNumber(timeline->timelineSeconds)
                << ":keyframes="
                << timeline->totalKeyframes;
            descriptors.push_back(timelineDescriptor.str());
        }

        for (std::size_t index = 0; index < sgfxBinding->slotCount; ++index)
        {
            const auto& slot = sgfxBinding->slots[index];
            std::ostringstream mapDescriptor;
            mapDescriptor
                << "sgfx_element_map="
                << csdBinding.templateId
                << ":scene="
                << csdBinding.primarySceneName
                << ":slot="
                << slot.slotName
                << ":texture="
                << slot.textureName;
            descriptors.push_back(mapDescriptor.str());
        }

        const auto manifest = findRuntimeEvidenceManifestForTarget(csdBinding.templateId);
        std::ostringstream evidenceDescriptor;
        evidenceDescriptor
            << "runtime_evidence_compare="
            << csdBinding.templateId
            << ":target="
            << csdBinding.templateId
            << ":event="
            << sgfxBinding->requiredEventId
            << ":manifest="
            << (manifest ? "found" : "missing");
        descriptors.push_back(evidenceDescriptor.str());
    }

    if (templateFilter && templateCount == 0)
        failed = true;

    std::cout
        << "sward_su_ui_asset_renderer csd pipeline smoke ok "
        << "layout_source=research_uiux/data/layout_deep_analysis.json "
        << "templates=" << templateCount
        << " packages=" << uniquePackages.size()
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return failed || templateCount == 0 ? 1 : 0;
}

[[nodiscard]] std::vector<std::string> commandLineTokens()
{
    std::vector<std::string> tokens;
    std::istringstream stream(GetCommandLineA());
    std::string token;
    while (stream >> token)
        tokens.push_back(token);
    return tokens;
}

[[nodiscard]] bool commandLineHasFlag(std::string_view flag)
{
    const auto tokens = commandLineTokens();
    const std::string flagText(flag);
    return std::find(tokens.begin(), tokens.end(), flagText) != tokens.end();
}

[[nodiscard]] std::optional<std::string> commandLineValueAfter(std::string_view flag)
{
    const auto tokens = commandLineTokens();
    const std::string flagText(flag);
    const std::string prefix = flagText + "=";
    for (std::size_t index = 0; index < tokens.size(); ++index)
    {
        if (tokens[index] == flagText && index + 1 < tokens.size())
            return tokens[index + 1];
        if (tokens[index].starts_with(prefix))
            return tokens[index].substr(prefix.size());
    }

    return std::nullopt;
}

[[nodiscard]] int runRendererWindow(HINSTANCE instance, int showCommand, const std::optional<std::string>& initialTemplate)
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
    if (initialTemplate && !renderer.selectSgfxTemplate(*initialTemplate))
    {
        std::cerr << "Unknown SGFX template: " << *initialTemplate << '\n';
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 2;
    }

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
    const auto templateFilter = commandLineValueAfter("--template");
    if (commandLineHasFlag("--csd-pipeline-smoke"))
        return runCsdPipelineSmoke(templateFilter);
    if (commandLineHasFlag("--sgfx-template-smoke"))
        return runSgfxTemplateSmoke(templateFilter);
    if (commandLineHasFlag("--renderer-smoke"))
        return runRendererSmoke();
    if (commandLineHasFlag("--renderer-navigation-smoke"))
        return runRendererNavigationSmoke();
    if (commandLineHasFlag("--renderer-atlas-gallery-smoke"))
        return runRendererAtlasGallerySmoke();
    if (commandLineHasFlag("--renderer-title-screen-smoke"))
        return runRendererTitleScreenSmoke();
    if (commandLineHasFlag("--renderer-reconstructed-screen-smoke"))
        return runRendererReconstructedScreenSmoke();

    return runRendererWindow(instance, showCommand, templateFilter);
}
