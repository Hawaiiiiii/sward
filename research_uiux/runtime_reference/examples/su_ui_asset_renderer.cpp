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
#include <cwchar>
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
inline constexpr double kPi = 3.14159265358979323846;
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

struct CsdColorRgba
{
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

struct CsdDrawableCommand
{
    std::string sceneName;
    std::string castName;
    std::string textureName;
    int groupIndex = 0;
    int castIndex = 0;
    int subimageIndex = -1;
    int textureIndex = -1;
    int textureWidth = 0;
    int textureHeight = 0;
    int sourceX = 0;
    int sourceY = 0;
    int sourceWidth = 0;
    int sourceHeight = 0;
    int castWidth = 0;
    int castHeight = 0;
    int destinationX = 0;
    int destinationY = 0;
    int destinationWidth = 0;
    int destinationHeight = 0;
    double translationX = 0.0;
    double translationY = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
    double rotation = 0.0;
    int drawType = 1;
    std::uint32_t castFlags = 0;
    std::uint32_t colorPackedRgba = 0xFFFFFFFF;
    CsdColorRgba colorRgba{};
    CsdColorRgba gradientTopLeftRgba{};
    CsdColorRgba gradientBottomLeftRgba{};
    CsdColorRgba gradientTopRightRgba{};
    CsdColorRgba gradientBottomRightRgba{};
    bool colorKnown = false;
    bool gradientKnown = false;
    bool gradientVarying = false;
    bool additiveBlend = false;
    bool linearFiltering = false;
    bool hidden = false;
    bool flipX = false;
    bool flipY = false;
    bool textureResolved = false;
    bool sourceFits = false;
};

struct CsdDrawableScene
{
    std::filesystem::path sourcePath;
    std::string layoutFileName;
    std::string sceneName;
    int castCount = 0;
    int subimageCount = 0;
    std::vector<std::string> textureNames;
    std::vector<CsdDrawableCommand> commands;
};

struct CsdCastDictionaryEntry
{
    int groupIndex = 0;
    int castIndex = 0;
    std::string name;
};

struct CsdSubimageBinding
{
    int textureIndex = -1;
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;
};

struct DdsTextureInfo
{
    std::string format;
    int width = 0;
    int height = 0;
};

struct CsdTimelineKeyframe
{
    double frame = 0.0;
    double value = 0.0;
    std::string interpolationType;
};

struct CsdTimelinePackedRgbaKeyframe
{
    double frame = 0.0;
    std::uint32_t packedRgba = 0xFFFFFFFF;
    CsdColorRgba color{};
    std::string interpolationType;
};

struct CsdTimelineTrackSample
{
    std::string sceneName;
    std::string castName;
    std::string trackType;
    int groupIndex = 0;
    int castIndex = 0;
    int sampleFrame = 0;
    int keyframeCount = 0;
    double value = 0.0;
};

struct CsdTimelinePackedRgbaTrackSample
{
    std::string sceneName;
    std::string castName;
    std::string trackType;
    int groupIndex = 0;
    int castIndex = 0;
    int sampleFrame = 0;
    int keyframeCount = 0;
    std::uint32_t packedRgba = 0xFFFFFFFF;
    CsdColorRgba color{};
};

struct CsdTimelinePlayback
{
    std::filesystem::path sourcePath;
    std::string layoutFileName;
    std::string sceneName;
    std::string animationName;
    int animationIndex = 0;
    int sampleFrame = 0;
    double frameCount = 0.0;
    int trackCount = 0;
    int numericTrackCount = 0;
    int keyframeCount = 0;
    int colorTrackCount = 0;
    int gradientTrackCount = 0;
    int packedColorTrackCount = 0;
    int packedGradientTrackCount = 0;
    int decodedPackedColorTrackCount = 0;
    int decodedPackedGradientTrackCount = 0;
    int decodedPackedKeyframeCount = 0;
    int unresolvedPackedKeyframeCount = 0;
    std::vector<CsdTimelineTrackSample> samples;
    std::vector<CsdTimelinePackedRgbaTrackSample> packedRgbaSamples;
};

struct BitmapSignalStats
{
    bool loaded = false;
    int width = 0;
    int height = 0;
    std::uint64_t rgbSum = 0;
    std::uint64_t alphaSum = 0;
    std::uint64_t rgbNonBlack = 0;
};

struct CsdFullFrameDeltaStats
{
    bool computed = false;
    std::string mode = "registered-full-frame-nearest";
    int width = kDesignWidth;
    int height = kDesignHeight;
    int pixelCount = 0;
    int exactMatchPixels = 0;
    int significantDeltaPixels = 0;
    int renderNonBlackPixels = 0;
    int nativeNonBlackPixels = 0;
    double meanAbsRgb = 0.0;
    int maxAbsRgb = 0;
    double renderNonBlackRatio = 0.0;
    double nativeNonBlackRatio = 0.0;
};

struct BitmapComparisonStats
{
    bool nativeFound = false;
    int sampleGridWidth = 64;
    int sampleGridHeight = 36;
    int sampleCount = 0;
    std::string alignmentMode = "search-center-crop-16x9";
    int nativeAlignmentCropX = 0;
    int nativeAlignmentCropY = 0;
    int nativeAlignmentCropWidth = 0;
    int nativeAlignmentCropHeight = 0;
    int registrationOffsetX = 0;
    int registrationOffsetY = 0;
    int registrationCandidateCount = 0;
    double registrationBaseMeanAbsRgb = 0.0;
    double meanAbsRgb = 0.0;
    int maxAbsRgb = 0;
    BitmapSignalStats rendered;
    BitmapSignalStats native;
    CsdFullFrameDeltaStats fullFrame;
};

struct CsdNativeFrameRegistration
{
    int cropX = 0;
    int cropY = 0;
    int cropWidth = 0;
    int cropHeight = 0;
    int offsetX = 0;
    int offsetY = 0;
    int candidateCount = 0;
    double baseMeanAbsRgb = 0.0;
    double bestMeanAbsRgb = 0.0;
    int bestMaxAbsRgb = 0;
};

struct CsdMaterialParityTriage
{
    std::string primaryBlocker = "not-computed";
    std::vector<std::string> riskFlags;
    double coverageGap = 0.0;
    double sampledVsFullFrameGap = 0.0;
};

struct CsdRenderedFrameComparison
{
    std::string templateId;
    std::string layoutFileName;
    std::string sceneName;
    std::string timelineSceneName;
    std::string timelineAnimationName;
    int frame = 0;
    std::filesystem::path renderedFramePath;
    std::filesystem::path diffFramePath;
    std::optional<std::filesystem::path> nativeBestPath;
    std::size_t drawCommandCount = 0;
    std::size_t sampledCommandCount = 0;
    std::size_t textureBindingCount = 0;
    std::size_t colorCommandCount = 0;
    std::size_t alphaModulatedCommandCount = 0;
    std::size_t gradientCommandCount = 0;
    std::size_t gradientApproxCommandCount = 0;
    std::size_t gradientVertexColorCommandCount = 0;
    std::size_t additiveCommandCount = 0;
    std::size_t additiveSoftwareCommandCount = 0;
    std::size_t normalBlendCommandCount = 0;
    std::size_t linearFilteringCommandCount = 0;
    std::size_t softwareQuadCommandCount = 0;
    std::size_t csdPointFilterSampleCount = 0;
    std::size_t bilinearSampleCount = 0;
    std::size_t nearestSampleCount = 0;
    std::size_t gradientTrackSampleCount = 0;
    std::size_t packedColorTrackCount = 0;
    std::size_t packedGradientTrackCount = 0;
    std::size_t decodedPackedColorTrackCount = 0;
    std::size_t decodedPackedGradientTrackCount = 0;
    std::size_t decodedPackedKeyframeCount = 0;
    std::size_t unresolvedPackedKeyframeCount = 0;
    std::vector<std::string> sgfxSlots;
    BitmapComparisonStats visualDelta;
    CsdMaterialParityTriage materialTriage;
};

struct CsdSamplerStats
{
    std::size_t csdPointFilterSampleCount = 0;
    std::size_t bilinearSampleCount = 0;
    std::size_t nearestSampleCount = 0;
};

struct CsdSoftwareRenderStats
{
    std::size_t softwareQuadCommandCount = 0;
    std::size_t gradientVertexColorCommandCount = 0;
    std::size_t additiveSoftwareCommandCount = 0;
    CsdSamplerStats samplerStats;
};

inline constexpr std::array<TextureSourceCandidate, 15> kTextureSourceCandidates{{
    { "mat_load_comon_001.dds", "ui_extended_archives/Loading/mat_load_comon_001.dds" },
    { "mat_load_en_001.dds", "ui_broader_archives/Languages/English/Loading/mat_load_en_001.dds" },
    { "mat_comon_txt_001.dds", "ui_extended_archives/Loading/mat_comon_txt_001.dds" },
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

[[nodiscard]] std::optional<std::string_view> jsonObjectFieldSpan(std::string_view text, std::string_view fieldName)
{
    const auto valueOffset = findJsonFieldValueOffset(text, fieldName);
    if (!valueOffset || *valueOffset >= text.size() || text[*valueOffset] != '{')
        return std::nullopt;

    const auto endOffset = matchJsonContainer(text, *valueOffset, '{', '}');
    if (!endOffset)
        return std::nullopt;

    return text.substr(*valueOffset, (*endOffset - *valueOffset) + 1);
}

[[nodiscard]] std::vector<double> parseJsonNumberArray(std::string_view arraySpan)
{
    std::vector<double> values;
    std::size_t offset = 0;
    while (offset < arraySpan.size())
    {
        const char ch = arraySpan[offset];
        if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '-' || ch == '+' || ch == '.'))
        {
            ++offset;
            continue;
        }

        std::size_t endOffset = offset + 1;
        while (endOffset < arraySpan.size())
        {
            const char numberChar = arraySpan[endOffset];
            if (!(std::isdigit(static_cast<unsigned char>(numberChar)) || numberChar == '-' || numberChar == '+'
                || numberChar == '.' || numberChar == 'e' || numberChar == 'E'))
                break;
            ++endOffset;
        }

        try
        {
            values.push_back(std::stod(std::string(arraySpan.substr(offset, endOffset - offset))));
        }
        catch (...)
        {
        }
        offset = endOffset;
    }

    return values;
}

[[nodiscard]] std::vector<double> jsonNumberArrayField(std::string_view text, std::string_view fieldName)
{
    const auto arraySpan = jsonArrayFieldSpan(text, fieldName);
    return arraySpan ? parseJsonNumberArray(*arraySpan) : std::vector<double>{};
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

[[nodiscard]] std::uint32_t readLe32(const std::uint8_t* data);
[[nodiscard]] std::optional<DdsTextureImage> loadDdsTextureImage(std::string_view textureFileName);

[[nodiscard]] std::optional<DdsTextureInfo> loadDdsTextureInfo(std::string_view textureFileName)
{
    const auto path = textureSourcePathForFileName(textureFileName);
    if (!path)
        return std::nullopt;

    std::ifstream file(*path, std::ios::binary);
    if (!file)
        return std::nullopt;

    std::array<std::uint8_t, 128> header{};
    file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    if (!file || std::memcmp(header.data(), "DDS ", 4) != 0)
        return std::nullopt;

    const std::uint32_t headerSize = readLe32(header.data() + 4);
    const int height = static_cast<int>(readLe32(header.data() + 12));
    const int width = static_cast<int>(readLe32(header.data() + 16));
    const std::uint32_t pixelFormatSize = readLe32(header.data() + 76);
    const std::string fourCc(reinterpret_cast<const char*>(header.data() + 84), 4);
    if (headerSize != 124 || pixelFormatSize != 32 || width <= 0 || height <= 0)
        return std::nullopt;

    DdsTextureInfo info;
    info.format = fourCc;
    info.width = width;
    info.height = height;
    return info;
}

[[nodiscard]] std::optional<std::string_view> findParsedCsdFileObjectSpan(
    std::string_view document,
    std::string_view layoutFileName)
{
    const auto parsedFiles = jsonArrayFieldSpan(document, "parsed_files");
    if (!parsedFiles)
        return std::nullopt;

    for (const auto objectSpan : jsonObjectSpansInArray(*parsedFiles))
    {
        if (jsonStringField(objectSpan, "file_name").value_or("") == layoutFileName)
            return objectSpan;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> firstJsonObjectInArrayField(
    std::string_view text,
    std::string_view fieldName)
{
    const auto arraySpan = jsonArrayFieldSpan(text, fieldName);
    if (!arraySpan)
        return std::nullopt;

    const auto objects = jsonObjectSpansInArray(*arraySpan);
    if (objects.empty())
        return std::nullopt;

    return objects.front();
}

[[nodiscard]] std::optional<std::string_view> jsonObjectInArrayAt(
    std::string_view arraySpan,
    std::size_t index)
{
    std::size_t objectIndex = 0;
    for (const auto objectSpan : jsonObjectSpansInArray(arraySpan))
    {
        if (objectIndex == index)
            return objectSpan;
        ++objectIndex;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<int> csdSceneIndexForName(std::string_view rootObject, std::string_view sceneName);

[[nodiscard]] std::optional<std::string_view> findCsdSceneObjectSpan(
    std::string_view document,
    std::string_view layoutFileName,
    std::string_view sceneName)
{
    const auto parsedFile = findParsedCsdFileObjectSpan(document, layoutFileName);
    if (!parsedFile)
        return std::nullopt;

    const auto resource = firstJsonObjectInArrayField(*parsedFile, "resources");
    const auto content = resource ? jsonObjectFieldSpan(*resource, "content") : std::optional<std::string_view>{};
    const auto project = content ? jsonObjectFieldSpan(*content, "csdm_project") : std::optional<std::string_view>{};
    const auto root = project ? jsonObjectFieldSpan(*project, "root") : std::optional<std::string_view>{};
    if (!root)
        return std::nullopt;

    const auto sceneIndex = csdSceneIndexForName(*root, sceneName);
    const auto scenes = jsonArrayFieldSpan(*root, "scenes");
    if (!sceneIndex || *sceneIndex < 0 || !scenes)
        return std::nullopt;

    return jsonObjectInArrayAt(*scenes, static_cast<std::size_t>(*sceneIndex));
}

[[nodiscard]] std::optional<int> csdSceneIndexForName(std::string_view rootObject, std::string_view sceneName)
{
    const auto sceneIds = jsonArrayFieldSpan(rootObject, "scene_ids");
    if (!sceneIds)
        return std::nullopt;

    for (const auto objectSpan : jsonObjectSpansInArray(*sceneIds))
    {
        if (jsonStringField(objectSpan, "name").value_or("") == sceneName)
            return static_cast<int>(jsonNumberField(objectSpan, "index").value_or(-1.0));
    }

    return std::nullopt;
}

[[nodiscard]] std::vector<CsdCastDictionaryEntry> parseCsdCastDictionary(std::string_view sceneObject)
{
    std::vector<CsdCastDictionaryEntry> entries;
    const auto dictionaries = jsonArrayFieldSpan(sceneObject, "cast_dictionaries");
    if (!dictionaries)
        return entries;

    for (const auto objectSpan : jsonObjectSpansInArray(*dictionaries))
    {
        CsdCastDictionaryEntry entry;
        entry.groupIndex = static_cast<int>(jsonNumberField(objectSpan, "group_index").value_or(0.0));
        entry.castIndex = static_cast<int>(jsonNumberField(objectSpan, "cast_index").value_or(0.0));
        entry.name = jsonStringField(objectSpan, "name").value_or("");
        entries.push_back(std::move(entry));
    }

    return entries;
}

[[nodiscard]] std::string csdCastNameFor(
    const std::vector<CsdCastDictionaryEntry>& dictionary,
    int groupIndex,
    int castIndex)
{
    const auto found = std::find_if(
        dictionary.begin(),
        dictionary.end(),
        [groupIndex, castIndex](const CsdCastDictionaryEntry& entry)
        {
            return entry.groupIndex == groupIndex && entry.castIndex == castIndex;
        });
    if (found != dictionary.end() && !found->name.empty())
        return found->name;

    std::ostringstream fallback;
    fallback << "Cast_" << groupIndex << "_" << castIndex;
    return fallback.str();
}

[[nodiscard]] std::vector<CsdSubimageBinding> parseCsdSubimages(std::string_view sceneObject)
{
    std::vector<CsdSubimageBinding> subimages;
    const auto subimageArray = jsonArrayFieldSpan(sceneObject, "subimages");
    if (!subimageArray)
        return subimages;

    for (const auto objectSpan : jsonObjectSpansInArray(*subimageArray))
    {
        CsdSubimageBinding subimage;
        subimage.textureIndex = static_cast<int>(jsonNumberField(objectSpan, "texture_index").value_or(-1.0));

        const auto topLeft = jsonNumberArrayField(objectSpan, "top_left");
        const auto bottomRight = jsonNumberArrayField(objectSpan, "bottom_right");
        if (topLeft.size() >= 2)
        {
            subimage.left = topLeft[0];
            subimage.top = topLeft[1];
        }
        if (bottomRight.size() >= 2)
        {
            subimage.right = bottomRight[0];
            subimage.bottom = bottomRight[1];
        }

        subimages.push_back(subimage);
    }

    return subimages;
}

[[nodiscard]] std::optional<int> firstUsedSubimageIndex(std::string_view castObject)
{
    const auto material = jsonObjectFieldSpan(castObject, "cast_material");
    if (!material)
        return std::nullopt;

    const auto usedSubimages = jsonNumberArrayField(*material, "used_subimage_indices");
    for (const double value : usedSubimages)
    {
        const int index = static_cast<int>(value);
        if (index >= 0)
            return index;
    }

    return std::nullopt;
}

[[nodiscard]] bool csdSourceRectFits(const CsdDrawableCommand& command)
{
    return command.textureResolved
        && command.sourceX >= 0
        && command.sourceY >= 0
        && command.sourceWidth > 0
        && command.sourceHeight > 0
        && command.sourceX + command.sourceWidth <= command.textureWidth
        && command.sourceY + command.sourceHeight <= command.textureHeight;
}

[[nodiscard]] std::optional<std::uint32_t> parseCsdHexColor(std::string_view text)
{
    if (text.empty())
        return std::nullopt;

    std::string value(text);
    if (value.starts_with("0x") || value.starts_with("0X"))
        value = value.substr(2);

    try
    {
        return static_cast<std::uint32_t>(std::stoul(value, nullptr, 16));
    }
    catch (...)
    {
        return std::nullopt;
    }
}

[[nodiscard]] CsdColorRgba decodeCsdPackedRgba(std::uint32_t packed)
{
    return CsdColorRgba{
        static_cast<std::uint8_t>((packed >> 24) & 0xFF),
        static_cast<std::uint8_t>((packed >> 16) & 0xFF),
        static_cast<std::uint8_t>((packed >> 8) & 0xFF),
        static_cast<std::uint8_t>(packed & 0xFF),
    };
}

[[nodiscard]] bool isDefaultWhiteRgba(const CsdColorRgba& color)
{
    return color.r == 255 && color.g == 255 && color.b == 255 && color.a == 255;
}

[[nodiscard]] bool sameRgba(const CsdColorRgba& left, const CsdColorRgba& right)
{
    return left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a;
}

void refreshCsdGradientState(CsdDrawableCommand& command)
{
    command.gradientVarying =
        command.gradientKnown
        && (!sameRgba(command.gradientTopLeftRgba, command.gradientBottomLeftRgba)
            || !sameRgba(command.gradientTopLeftRgba, command.gradientTopRightRgba)
            || !sameRgba(command.gradientTopLeftRgba, command.gradientBottomRightRgba)
            || !isDefaultWhiteRgba(command.gradientTopLeftRgba));
}

[[nodiscard]] CsdColorRgba averageGradientRgba(const CsdDrawableCommand& command)
{
    auto average = [](std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d)
    {
        return static_cast<std::uint8_t>((static_cast<int>(a) + b + c + d + 2) / 4);
    };

    return CsdColorRgba{
        average(command.gradientTopLeftRgba.r, command.gradientBottomLeftRgba.r, command.gradientTopRightRgba.r, command.gradientBottomRightRgba.r),
        average(command.gradientTopLeftRgba.g, command.gradientBottomLeftRgba.g, command.gradientTopRightRgba.g, command.gradientBottomRightRgba.g),
        average(command.gradientTopLeftRgba.b, command.gradientBottomLeftRgba.b, command.gradientTopRightRgba.b, command.gradientBottomRightRgba.b),
        average(command.gradientTopLeftRgba.a, command.gradientBottomLeftRgba.a, command.gradientTopRightRgba.a, command.gradientBottomRightRgba.a),
    };
}

[[nodiscard]] CsdColorRgba effectiveCsdDrawRgba(const CsdDrawableCommand& command)
{
    const auto gradient = averageGradientRgba(command);
    auto multiply = [](std::uint8_t left, std::uint8_t right)
    {
        return static_cast<std::uint8_t>((static_cast<int>(left) * static_cast<int>(right) + 127) / 255);
    };

    return CsdColorRgba{
        multiply(command.colorRgba.r, gradient.r),
        multiply(command.colorRgba.g, gradient.g),
        multiply(command.colorRgba.b, gradient.b),
        multiply(command.colorRgba.a, gradient.a),
    };
}

[[nodiscard]] std::optional<CsdColorRgba> parseCsdRgbaField(std::string_view object, std::string_view fieldName)
{
    const auto text = jsonStringField(object, fieldName);
    if (!text)
        return std::nullopt;
    const auto packed = parseCsdHexColor(*text);
    if (!packed)
        return std::nullopt;
    return decodeCsdPackedRgba(*packed);
}

[[nodiscard]] std::uint8_t interpolateCsdByte(std::uint8_t previous, std::uint8_t next, double t)
{
    return static_cast<std::uint8_t>(std::clamp(
        static_cast<int>(std::llround(static_cast<double>(previous) + ((static_cast<double>(next) - static_cast<double>(previous)) * t))),
        0,
        255));
}

[[nodiscard]] CsdColorRgba interpolateCsdRgba(const CsdColorRgba& previous, const CsdColorRgba& next, double t)
{
    return CsdColorRgba{
        interpolateCsdByte(previous.r, next.r, t),
        interpolateCsdByte(previous.g, next.g, t),
        interpolateCsdByte(previous.b, next.b, t),
        interpolateCsdByte(previous.a, next.a, t),
    };
}

[[nodiscard]] std::uint32_t packCsdRgba(const CsdColorRgba& color)
{
    return (static_cast<std::uint32_t>(color.r) << 24)
        | (static_cast<std::uint32_t>(color.g) << 16)
        | (static_cast<std::uint32_t>(color.b) << 8)
        | static_cast<std::uint32_t>(color.a);
}

[[nodiscard]] bool isCsdGradientTrack(std::string_view trackType)
{
    return trackType.starts_with("Gradient");
}

[[nodiscard]] bool isCsdPackedChannelTrack(std::string_view trackType)
{
    return trackType == "Color" || isCsdGradientTrack(trackType);
}

[[nodiscard]] std::vector<CsdDrawableCommand> buildCsdDrawableCommands(
    std::string_view sceneObject,
    std::string_view sceneName,
    const std::vector<std::string>& textureNames)
{
    std::vector<CsdDrawableCommand> commands;
    const auto castGroups = jsonArrayFieldSpan(sceneObject, "cast_groups");
    if (!castGroups)
        return commands;

    const auto dictionary = parseCsdCastDictionary(sceneObject);
    const auto subimages = parseCsdSubimages(sceneObject);
    const auto groupObjects = jsonObjectSpansInArray(*castGroups);
    std::vector<std::pair<std::string, std::optional<DdsTextureInfo>>> textureInfoCache;
    auto textureInfoFor = [&textureInfoCache](std::string_view textureName) -> const DdsTextureInfo*
    {
        const auto found = std::find_if(
            textureInfoCache.begin(),
            textureInfoCache.end(),
            [textureName](const std::pair<std::string, std::optional<DdsTextureInfo>>& entry)
            {
                return entry.first == textureName;
            });
        if (found != textureInfoCache.end())
            return found->second ? &*found->second : nullptr;

        textureInfoCache.emplace_back(std::string(textureName), loadDdsTextureInfo(textureName));
        return textureInfoCache.back().second ? &*textureInfoCache.back().second : nullptr;
    };

    for (std::size_t groupIndex = 0; groupIndex < groupObjects.size(); ++groupIndex)
    {
        const auto casts = jsonArrayFieldSpan(groupObjects[groupIndex], "casts");
        if (!casts)
            continue;

        const auto castObjects = jsonObjectSpansInArray(*casts);
        for (std::size_t castIndex = 0; castIndex < castObjects.size(); ++castIndex)
        {
            const auto castObject = castObjects[castIndex];
            if (static_cast<int>(jsonNumberField(castObject, "is_enabled").value_or(0.0)) == 0)
                continue;

            const auto subimageIndex = firstUsedSubimageIndex(castObject);
            if (!subimageIndex || *subimageIndex < 0 || static_cast<std::size_t>(*subimageIndex) >= subimages.size())
                continue;

            const auto& subimage = subimages[static_cast<std::size_t>(*subimageIndex)];
            if (subimage.textureIndex < 0 || static_cast<std::size_t>(subimage.textureIndex) >= textureNames.size())
                continue;

            const auto castInfo = jsonObjectFieldSpan(castObject, "cast_info");
            if (!castInfo)
                continue;

            const auto translation = jsonNumberArrayField(*castInfo, "translation");
            const auto scale = jsonNumberArrayField(*castInfo, "scale");
            const double translationX = translation.size() >= 1 ? translation[0] : 0.0;
            const double translationY = translation.size() >= 2 ? translation[1] : 0.0;
            const double scaleX = scale.size() >= 1 ? scale[0] : 1.0;
            const double scaleY = scale.size() >= 2 ? scale[1] : 1.0;
            const int hideFlag = static_cast<int>(jsonNumberField(*castInfo, "hide_flag").value_or(0.0));
            if (hideFlag != 0)
                continue;
            const int castWidth = static_cast<int>(std::llround(jsonNumberField(castObject, "width").value_or(0.0)));
            const int castHeight = static_cast<int>(std::llround(jsonNumberField(castObject, "height").value_or(0.0)));

            CsdDrawableCommand command;
            command.sceneName = std::string(sceneName);
            command.groupIndex = static_cast<int>(groupIndex);
            command.castIndex = static_cast<int>(castIndex);
            command.castName = csdCastNameFor(dictionary, command.groupIndex, command.castIndex);
            command.drawType = static_cast<int>(jsonNumberField(castObject, "field04").value_or(1.0));
            command.castFlags = static_cast<std::uint32_t>(jsonNumberField(castObject, "field38").value_or(0.0));
            command.subimageIndex = *subimageIndex;
            command.textureIndex = subimage.textureIndex;
            command.textureName = textureNames[static_cast<std::size_t>(subimage.textureIndex)];
            command.castWidth = castWidth;
            command.castHeight = castHeight;
            command.translationX = translationX;
            command.translationY = translationY;
            command.scaleX = scaleX;
            command.scaleY = scaleY;
            command.rotation = jsonNumberField(*castInfo, "rotation").value_or(0.0);
            command.hidden = hideFlag != 0;
            if (const auto colorText = jsonStringField(*castInfo, "color"))
            {
                if (const auto color = parseCsdHexColor(*colorText))
                {
                    command.colorPackedRgba = *color;
                    command.colorRgba = decodeCsdPackedRgba(*color);
                    command.colorKnown = true;
                }
            }

            const auto gradientTopLeft = parseCsdRgbaField(*castInfo, "gradient_top_left");
            const auto gradientBottomLeft = parseCsdRgbaField(*castInfo, "gradient_bottom_left");
            const auto gradientTopRight = parseCsdRgbaField(*castInfo, "gradient_top_right");
            const auto gradientBottomRight = parseCsdRgbaField(*castInfo, "gradient_bottom_right");
            if (gradientTopLeft && gradientBottomLeft && gradientTopRight && gradientBottomRight)
            {
                command.gradientTopLeftRgba = *gradientTopLeft;
                command.gradientBottomLeftRgba = *gradientBottomLeft;
                command.gradientTopRightRgba = *gradientTopRight;
                command.gradientBottomRightRgba = *gradientBottomRight;
                command.gradientKnown = true;
                refreshCsdGradientState(command);
            }

            command.additiveBlend = (command.castFlags & 0x1) != 0;
            command.linearFiltering = (command.castFlags & 0x1000) != 0;
            command.flipX = scaleX < 0.0 || (command.castFlags & 0x400) != 0;
            command.flipY = scaleY < 0.0 || (command.castFlags & 0x800) != 0;
            command.destinationWidth = std::max(1, static_cast<int>(std::llround(std::fabs(static_cast<double>(castWidth) * scaleX))));
            command.destinationHeight = std::max(1, static_cast<int>(std::llround(std::fabs(static_cast<double>(castHeight) * scaleY))));
            command.destinationX = static_cast<int>(std::llround(
                ((0.5 + translationX) * static_cast<double>(kDesignWidth)) - (static_cast<double>(command.destinationWidth) * 0.5)));
            command.destinationY = static_cast<int>(std::llround(
                ((0.5 + translationY) * static_cast<double>(kDesignHeight)) - (static_cast<double>(command.destinationHeight) * 0.5)));

            const auto* textureInfo = textureInfoFor(command.textureName);
            if (textureInfo)
            {
                command.textureResolved = true;
                command.textureWidth = textureInfo->width;
                command.textureHeight = textureInfo->height;
                const int sourceLeft = static_cast<int>(std::llround(subimage.left * static_cast<double>(textureInfo->width)));
                const int sourceTop = static_cast<int>(std::llround(subimage.top * static_cast<double>(textureInfo->height)));
                const int sourceRight = static_cast<int>(std::llround(subimage.right * static_cast<double>(textureInfo->width)));
                const int sourceBottom = static_cast<int>(std::llround(subimage.bottom * static_cast<double>(textureInfo->height)));
                command.sourceX = sourceLeft;
                command.sourceY = sourceTop;
                command.sourceWidth = std::max(1, sourceRight - sourceLeft);
                command.sourceHeight = std::max(1, sourceBottom - sourceTop);
                command.sourceFits = csdSourceRectFits(command);
            }

            commands.push_back(std::move(command));
        }
    }

    return commands;
}

[[nodiscard]] std::optional<CsdDrawableScene> loadCsdDrawableScene(const CsdPipelineTemplateBinding& binding)
{
    const auto evidencePath = layoutEvidencePath();
    if (!evidencePath)
        return std::nullopt;

    const std::string document = readTextFile(*evidencePath);
    if (document.empty())
        return std::nullopt;

    const auto pipelineEvidence = loadCsdPipelineEvidence(binding.layoutFileName);
    if (!pipelineEvidence)
        return std::nullopt;

    const auto parsedFile = findParsedCsdFileObjectSpan(document, binding.layoutFileName);
    if (!parsedFile)
        return std::nullopt;

    const auto resource = firstJsonObjectInArrayField(*parsedFile, "resources");
    const auto content = resource ? jsonObjectFieldSpan(*resource, "content") : std::optional<std::string_view>{};
    const auto project = content ? jsonObjectFieldSpan(*content, "csdm_project") : std::optional<std::string_view>{};
    const auto root = project ? jsonObjectFieldSpan(*project, "root") : std::optional<std::string_view>{};
    if (!root)
        return std::nullopt;

    const auto sceneIndex = csdSceneIndexForName(*root, binding.primarySceneName);
    const auto scenes = jsonArrayFieldSpan(*root, "scenes");
    if (!sceneIndex || *sceneIndex < 0 || !scenes)
        return std::nullopt;

    const auto sceneObject = jsonObjectInArrayAt(*scenes, static_cast<std::size_t>(*sceneIndex));
    if (!sceneObject)
        return std::nullopt;

    CsdDrawableScene scene;
    scene.sourcePath = *evidencePath;
    scene.layoutFileName = std::string(binding.layoutFileName);
    scene.sceneName = std::string(binding.primarySceneName);
    if (const auto* summary = findCsdPipelineScene(*pipelineEvidence, binding.primarySceneName))
    {
        scene.castCount = summary->castCount;
        scene.subimageCount = summary->subimageCount;
    }
    else
    {
        scene.castCount = static_cast<int>(jsonNumberField(*sceneObject, "cast_count").value_or(0.0));
        scene.subimageCount = static_cast<int>(jsonNumberField(*sceneObject, "subimages_count").value_or(0.0));
    }
    scene.textureNames = pipelineEvidence->textureNames;
    scene.commands = buildCsdDrawableCommands(*sceneObject, scene.sceneName, scene.textureNames);
    return scene;
}

[[nodiscard]] int csdTimelineSampleFrameForTemplate(std::string_view templateId)
{
    if (templateId == "title-menu")
        return 10;
    if (templateId == "loading")
        return 75;
    if (templateId == "sonic-hud")
        return 99;
    return 50;
}

[[nodiscard]] std::optional<int> csdAnimationIndexForName(
    std::string_view sceneObject,
    std::string_view animationName)
{
    const auto dictionaries = jsonArrayFieldSpan(sceneObject, "animation_dictionaries");
    if (!dictionaries)
        return std::nullopt;

    for (const auto objectSpan : jsonObjectSpansInArray(*dictionaries))
    {
        if (jsonStringField(objectSpan, "name").value_or("") == animationName)
            return static_cast<int>(jsonNumberField(objectSpan, "index").value_or(-1.0));
    }

    return std::nullopt;
}

[[nodiscard]] std::vector<CsdTimelineKeyframe> parseCsdTimelineKeyframes(std::string_view trackObject)
{
    std::vector<CsdTimelineKeyframe> keyframes;
    const auto keyframeArray = jsonArrayFieldSpan(trackObject, "keyframes");
    if (!keyframeArray)
        return keyframes;

    for (const auto keyframeObject : jsonObjectSpansInArray(*keyframeArray))
    {
        const auto frame = jsonNumberField(keyframeObject, "frame");
        const auto value = jsonNumberField(keyframeObject, "value");
        if (!frame || !value || !std::isfinite(*value))
            continue;

        CsdTimelineKeyframe keyframe;
        keyframe.frame = *frame;
        keyframe.value = *value;
        keyframe.interpolationType = jsonStringField(keyframeObject, "type").value_or("Linear");
        keyframes.push_back(std::move(keyframe));
    }

    std::sort(
        keyframes.begin(),
        keyframes.end(),
        [](const CsdTimelineKeyframe& left, const CsdTimelineKeyframe& right)
        {
            return left.frame < right.frame;
        });
    return keyframes;
}

[[nodiscard]] std::optional<std::uint32_t> parseCsdPackedRgbaKeyframeValue(std::string_view keyframeObject)
{
    for (const std::string_view fieldName : { "packed_rgba", "value_packed_rgba", "value_raw_bits" })
    {
        if (const auto text = jsonStringField(keyframeObject, fieldName))
        {
            if (const auto packed = parseCsdHexColor(*text))
                return *packed;
        }
    }

    const auto value = jsonNumberField(keyframeObject, "value");
    if (!value || !std::isfinite(*value) || *value < 0.0 || *value > 4294967295.0)
        return std::nullopt;

    return static_cast<std::uint32_t>(std::llround(*value));
}

[[nodiscard]] std::vector<CsdTimelinePackedRgbaKeyframe> parseCsdTimelinePackedRgbaKeyframes(std::string_view trackObject)
{
    std::vector<CsdTimelinePackedRgbaKeyframe> keyframes;
    const auto keyframeArray = jsonArrayFieldSpan(trackObject, "keyframes");
    if (!keyframeArray)
        return keyframes;

    for (const auto keyframeObject : jsonObjectSpansInArray(*keyframeArray))
    {
        const auto frame = jsonNumberField(keyframeObject, "frame");
        const auto packed = parseCsdPackedRgbaKeyframeValue(keyframeObject);
        if (!frame || !packed)
            continue;

        CsdTimelinePackedRgbaKeyframe keyframe;
        keyframe.frame = *frame;
        keyframe.packedRgba = *packed;
        keyframe.color = decodeCsdPackedRgba(*packed);
        keyframe.interpolationType = jsonStringField(keyframeObject, "type").value_or("Linear");
        keyframes.push_back(std::move(keyframe));
    }

    std::sort(
        keyframes.begin(),
        keyframes.end(),
        [](const CsdTimelinePackedRgbaKeyframe& left, const CsdTimelinePackedRgbaKeyframe& right)
        {
            return left.frame < right.frame;
        });
    return keyframes;
}

[[nodiscard]] std::optional<double> sampleCsdTimelineTrack(
    const std::vector<CsdTimelineKeyframe>& keyframes,
    double frame)
{
    if (keyframes.empty())
        return std::nullopt;

    if (frame <= keyframes.front().frame)
        return keyframes.front().value;

    for (std::size_t index = 1; index < keyframes.size(); ++index)
    {
        const auto& previous = keyframes[index - 1];
        const auto& next = keyframes[index];
        if (frame > next.frame)
            continue;

        if (std::fabs(next.frame - previous.frame) < 0.000001 || previous.interpolationType == "Const")
            return previous.value;

        const double t = std::clamp((frame - previous.frame) / (next.frame - previous.frame), 0.0, 1.0);
        return previous.value + ((next.value - previous.value) * t);
    }

    return keyframes.back().value;
}

[[nodiscard]] std::optional<CsdColorRgba> sampleCsdPackedRgbaTimelineTrack(
    const std::vector<CsdTimelinePackedRgbaKeyframe>& keyframes,
    double frame)
{
    if (keyframes.empty())
        return std::nullopt;

    if (frame <= keyframes.front().frame)
        return keyframes.front().color;

    for (std::size_t index = 1; index < keyframes.size(); ++index)
    {
        const auto& previous = keyframes[index - 1];
        const auto& next = keyframes[index];
        if (frame > next.frame)
            continue;

        if (std::fabs(next.frame - previous.frame) < 0.000001 || previous.interpolationType == "Const")
            return previous.color;

        const double t = std::clamp((frame - previous.frame) / (next.frame - previous.frame), 0.0, 1.0);
        return interpolateCsdRgba(previous.color, next.color, t);
    }

    return keyframes.back().color;
}

[[nodiscard]] std::optional<CsdTimelinePlayback> loadCsdTimelinePlayback(const CsdPipelineTemplateBinding& binding)
{
    const auto evidencePath = layoutEvidencePath();
    if (!evidencePath)
        return std::nullopt;

    const std::string document = readTextFile(*evidencePath);
    if (document.empty())
        return std::nullopt;

    const auto sceneObject = findCsdSceneObjectSpan(document, binding.layoutFileName, binding.timelineSceneName);
    if (!sceneObject)
        return std::nullopt;

    const auto animationIndex = csdAnimationIndexForName(*sceneObject, binding.timelineAnimationName);
    const auto frameDataArray = jsonArrayFieldSpan(*sceneObject, "animation_frame_data_list");
    const auto keyframeDataArray = jsonArrayFieldSpan(*sceneObject, "animation_keyframe_data_list");
    if (!animationIndex || *animationIndex < 0 || !frameDataArray || !keyframeDataArray)
        return std::nullopt;

    const auto frameData = jsonObjectInArrayAt(*frameDataArray, static_cast<std::size_t>(*animationIndex));
    const auto keyframeData = jsonObjectInArrayAt(*keyframeDataArray, static_cast<std::size_t>(*animationIndex));
    if (!frameData || !keyframeData)
        return std::nullopt;

    CsdTimelinePlayback playback;
    playback.sourcePath = *evidencePath;
    playback.layoutFileName = std::string(binding.layoutFileName);
    playback.sceneName = std::string(binding.timelineSceneName);
    playback.animationName = std::string(binding.timelineAnimationName);
    playback.animationIndex = *animationIndex;
    playback.frameCount = jsonNumberField(*frameData, "frame_count").value_or(0.0);
    playback.sampleFrame = std::clamp(
        csdTimelineSampleFrameForTemplate(binding.templateId),
        0,
        std::max(0, static_cast<int>(std::llround(playback.frameCount))));

    const auto dictionary = parseCsdCastDictionary(*sceneObject);
    const auto groups = jsonArrayFieldSpan(*keyframeData, "groups");
    if (!groups)
        return playback;

    const auto groupObjects = jsonObjectSpansInArray(*groups);
    for (std::size_t groupIndex = 0; groupIndex < groupObjects.size(); ++groupIndex)
    {
        const auto casts = jsonArrayFieldSpan(groupObjects[groupIndex], "casts");
        if (!casts)
            continue;

        const auto castObjects = jsonObjectSpansInArray(*casts);
        for (std::size_t castIndex = 0; castIndex < castObjects.size(); ++castIndex)
        {
            const auto subData = jsonArrayFieldSpan(castObjects[castIndex], "sub_data");
            if (!subData)
                continue;

            for (const auto trackObject : jsonObjectSpansInArray(*subData))
            {
                ++playback.trackCount;
                const auto trackType = jsonStringField(trackObject, "track_type").value_or("Unknown");
                const auto keyframeObjects = jsonArrayFieldSpan(trackObject, "keyframes");
                const int rawKeyframeCount = keyframeObjects
                    ? static_cast<int>(jsonObjectSpansInArray(*keyframeObjects).size())
                    : 0;
                playback.keyframeCount += rawKeyframeCount;

                const auto keyframes = parseCsdTimelineKeyframes(trackObject);
                const auto packedKeyframes = isCsdPackedChannelTrack(trackType)
                    ? parseCsdTimelinePackedRgbaKeyframes(trackObject)
                    : std::vector<CsdTimelinePackedRgbaKeyframe>{};
                const int decodedPackedKeyframes = static_cast<int>(packedKeyframes.size());
                int unresolvedKeyframes = std::max(0, rawKeyframeCount - static_cast<int>(keyframes.size()));
                if (isCsdPackedChannelTrack(trackType))
                    unresolvedKeyframes = std::max(0, rawKeyframeCount - decodedPackedKeyframes);
                if (trackType == "Color")
                {
                    ++playback.colorTrackCount;
                    if (rawKeyframeCount > 0)
                        ++playback.packedColorTrackCount;
                    if (decodedPackedKeyframes > 0)
                        ++playback.decodedPackedColorTrackCount;
                }
                if (isCsdGradientTrack(trackType))
                {
                    ++playback.gradientTrackCount;
                    if (rawKeyframeCount > 0)
                        ++playback.packedGradientTrackCount;
                    if (decodedPackedKeyframes > 0)
                        ++playback.decodedPackedGradientTrackCount;
                }
                playback.decodedPackedKeyframeCount += decodedPackedKeyframes;
                if (isCsdPackedChannelTrack(trackType))
                    playback.unresolvedPackedKeyframeCount += unresolvedKeyframes;

                if (const auto packedSample = sampleCsdPackedRgbaTimelineTrack(packedKeyframes, static_cast<double>(playback.sampleFrame)))
                {
                    CsdTimelinePackedRgbaTrackSample trackSample;
                    trackSample.sceneName = playback.sceneName;
                    trackSample.groupIndex = static_cast<int>(groupIndex);
                    trackSample.castIndex = static_cast<int>(castIndex);
                    trackSample.castName = csdCastNameFor(dictionary, trackSample.groupIndex, trackSample.castIndex);
                    trackSample.trackType = trackType;
                    trackSample.sampleFrame = playback.sampleFrame;
                    trackSample.keyframeCount = decodedPackedKeyframes;
                    trackSample.color = *packedSample;
                    trackSample.packedRgba = packCsdRgba(*packedSample);
                    playback.packedRgbaSamples.push_back(std::move(trackSample));
                }

                const auto sample = sampleCsdTimelineTrack(keyframes, static_cast<double>(playback.sampleFrame));
                if (!sample)
                    continue;

                ++playback.numericTrackCount;
                CsdTimelineTrackSample trackSample;
                trackSample.sceneName = playback.sceneName;
                trackSample.groupIndex = static_cast<int>(groupIndex);
                trackSample.castIndex = static_cast<int>(castIndex);
                trackSample.castName = csdCastNameFor(dictionary, trackSample.groupIndex, trackSample.castIndex);
                trackSample.trackType = trackType;
                trackSample.sampleFrame = playback.sampleFrame;
                trackSample.keyframeCount = static_cast<int>(keyframes.size());
                trackSample.value = *sample;
                playback.samples.push_back(std::move(trackSample));
            }
        }
    }

    return playback;
}

[[nodiscard]] std::optional<CsdDrawableCommand> applyCsdTimelineToDrawableCommand(
    const CsdDrawableCommand& command,
    const CsdTimelineTrackSample& sample)
{
    if (command.sceneName != sample.sceneName || command.castName != sample.castName)
        return std::nullopt;

    CsdDrawableCommand sampled = command;
    if (sample.trackType == "XPosition")
        sampled.translationX = sample.value;
    else if (sample.trackType == "YPosition")
        sampled.translationY = sample.value;
    else if (sample.trackType == "XScale")
        sampled.scaleX = sample.value;
    else if (sample.trackType == "YScale")
        sampled.scaleY = sample.value;
    else if (sample.trackType == "Rotation")
        sampled.rotation = sample.value;
    else
        return std::nullopt;

    const double castWidth = command.castWidth > 0
        ? static_cast<double>(command.castWidth)
        : (std::fabs(command.scaleX) > 0.000001
            ? static_cast<double>(command.destinationWidth) / std::fabs(command.scaleX)
            : static_cast<double>(command.destinationWidth));
    const double castHeight = command.castHeight > 0
        ? static_cast<double>(command.castHeight)
        : (std::fabs(command.scaleY) > 0.000001
            ? static_cast<double>(command.destinationHeight) / std::fabs(command.scaleY)
            : static_cast<double>(command.destinationHeight));

    sampled.destinationWidth = std::max(1, static_cast<int>(std::llround(std::fabs(castWidth * sampled.scaleX))));
    sampled.destinationHeight = std::max(1, static_cast<int>(std::llround(std::fabs(castHeight * sampled.scaleY))));
    sampled.destinationX = static_cast<int>(std::llround(
        ((0.5 + sampled.translationX) * static_cast<double>(kDesignWidth)) - (static_cast<double>(sampled.destinationWidth) * 0.5)));
    sampled.destinationY = static_cast<int>(std::llround(
        ((0.5 + sampled.translationY) * static_cast<double>(kDesignHeight)) - (static_cast<double>(sampled.destinationHeight) * 0.5)));
    return sampled;
}

[[nodiscard]] std::optional<CsdDrawableCommand> applyCsdPackedRgbaTimelineToDrawableCommand(
    const CsdDrawableCommand& command,
    const CsdTimelinePackedRgbaTrackSample& sample)
{
    if (command.sceneName != sample.sceneName || command.castName != sample.castName)
        return std::nullopt;

    CsdDrawableCommand sampled = command;
    if (sample.trackType == "Color")
    {
        sampled.colorPackedRgba = sample.packedRgba;
        sampled.colorRgba = sample.color;
        sampled.colorKnown = true;
    }
    else if (sample.trackType == "GradientTL")
    {
        sampled.gradientTopLeftRgba = sample.color;
        sampled.gradientKnown = true;
    }
    else if (sample.trackType == "GradientBL")
    {
        sampled.gradientBottomLeftRgba = sample.color;
        sampled.gradientKnown = true;
    }
    else if (sample.trackType == "GradientTR")
    {
        sampled.gradientTopRightRgba = sample.color;
        sampled.gradientKnown = true;
    }
    else if (sample.trackType == "GradientBR")
    {
        sampled.gradientBottomRightRgba = sample.color;
        sampled.gradientKnown = true;
    }
    else
    {
        return std::nullopt;
    }

    refreshCsdGradientState(sampled);
    return sampled;
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

[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap> bitmapFromArgbPixels(
    int width,
    int height,
    const std::vector<std::uint32_t>& pixels)
{
    if (width <= 0 || height <= 0 || pixels.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
        return nullptr;

    auto bitmap = std::make_unique<Gdiplus::Bitmap>(width, height, PixelFormat32bppARGB);
    Gdiplus::BitmapData bitmapData{};
    Gdiplus::Rect lockRect(0, 0, width, height);
    if (bitmap->LockBits(&lockRect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok)
        return nullptr;

    for (int y = 0; y < height; ++y)
    {
        auto* destination = static_cast<std::uint8_t*>(bitmapData.Scan0) + (static_cast<std::ptrdiff_t>(bitmapData.Stride) * y);
        const auto* source = pixels.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width));
        std::memcpy(destination, source, static_cast<std::size_t>(width) * sizeof(std::uint32_t));
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

[[nodiscard]] std::uint8_t clampCsdByte(double value)
{
    return static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::llround(value)), 0, 255));
}

[[nodiscard]] CsdColorRgba unpackArgbPixel(std::uint32_t argb)
{
    return CsdColorRgba{
        static_cast<std::uint8_t>((argb >> 16) & 0xFF),
        static_cast<std::uint8_t>((argb >> 8) & 0xFF),
        static_cast<std::uint8_t>(argb & 0xFF),
        static_cast<std::uint8_t>((argb >> 24) & 0xFF),
    };
}

[[nodiscard]] std::uint32_t packArgbPixel(const CsdColorRgba& color)
{
    return (static_cast<std::uint32_t>(color.a) << 24)
        | (static_cast<std::uint32_t>(color.r) << 16)
        | (static_cast<std::uint32_t>(color.g) << 8)
        | static_cast<std::uint32_t>(color.b);
}

[[nodiscard]] CsdColorRgba multiplyCsdRgba(const CsdColorRgba& left, const CsdColorRgba& right)
{
    auto multiply = [](std::uint8_t a, std::uint8_t b)
    {
        return static_cast<std::uint8_t>((static_cast<int>(a) * static_cast<int>(b) + 127) / 255);
    };

    return CsdColorRgba{
        multiply(left.r, right.r),
        multiply(left.g, right.g),
        multiply(left.b, right.b),
        multiply(left.a, right.a),
    };
}

[[nodiscard]] CsdColorRgba sampleTextureArgbNearest(const DdsTextureImage& image, double x, double y)
{
    const int ix = std::clamp(static_cast<int>(std::floor(x)), 0, image.width - 1);
    const int iy = std::clamp(static_cast<int>(std::floor(y)), 0, image.height - 1);
    return unpackArgbPixel(image.argbPixels[static_cast<std::size_t>(iy) * static_cast<std::size_t>(image.width) + static_cast<std::size_t>(ix)]);
}

[[nodiscard]] CsdColorRgba sampleTextureArgbBilinear(const DdsTextureImage& image, double x, double y)
{
    x = std::clamp(x, 0.0, static_cast<double>(image.width - 1));
    y = std::clamp(y, 0.0, static_cast<double>(image.height - 1));
    const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, image.width - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, image.height - 1);
    const int x1 = std::min(x0 + 1, image.width - 1);
    const int y1 = std::min(y0 + 1, image.height - 1);
    const double tx = x - static_cast<double>(x0);
    const double ty = y - static_cast<double>(y0);

    auto pixelAt = [&image](int px, int py)
    {
        return unpackArgbPixel(image.argbPixels[static_cast<std::size_t>(py) * static_cast<std::size_t>(image.width) + static_cast<std::size_t>(px)]);
    };

    const auto c00 = pixelAt(x0, y0);
    const auto c10 = pixelAt(x1, y0);
    const auto c01 = pixelAt(x0, y1);
    const auto c11 = pixelAt(x1, y1);
    auto channel = [tx, ty](std::uint8_t v00, std::uint8_t v10, std::uint8_t v01, std::uint8_t v11)
    {
        const double top = static_cast<double>(v00) + ((static_cast<double>(v10) - static_cast<double>(v00)) * tx);
        const double bottom = static_cast<double>(v01) + ((static_cast<double>(v11) - static_cast<double>(v01)) * tx);
        return clampCsdByte(top + ((bottom - top) * ty));
    };

    return CsdColorRgba{
        channel(c00.r, c10.r, c01.r, c11.r),
        channel(c00.g, c10.g, c01.g, c11.g),
        channel(c00.b, c10.b, c01.b, c11.b),
        channel(c00.a, c10.a, c01.a, c11.a),
    };
}

[[nodiscard]] double csdFilterCoordinate(double coordinate, double footprint)
{
    const double safeFootprint = std::max(std::abs(footprint), 0.000001);
    const double seam = std::floor(coordinate + 0.5);
    const double filtered = ((coordinate - seam) / safeFootprint) + seam;
    return std::clamp(filtered, seam - 0.5, seam + 0.5);
}

[[nodiscard]] CsdColorRgba sampleTextureArgbCsdFilter(
    const DdsTextureImage& image,
    double x,
    double y,
    double footprintX,
    double footprintY)
{
    const double filteredX = csdFilterCoordinate(x, footprintX);
    const double filteredY = csdFilterCoordinate(y, footprintY);
    return sampleTextureArgbBilinear(image, filteredX, filteredY);
}

[[nodiscard]] CsdColorRgba gradientVertexColor(const CsdDrawableCommand& command, double u, double v)
{
    if (!command.gradientKnown)
        return CsdColorRgba{};

    const auto top = interpolateCsdRgba(command.gradientTopLeftRgba, command.gradientTopRightRgba, std::clamp(u, 0.0, 1.0));
    const auto bottom = interpolateCsdRgba(command.gradientBottomLeftRgba, command.gradientBottomRightRgba, std::clamp(u, 0.0, 1.0));
    return interpolateCsdRgba(top, bottom, std::clamp(v, 0.0, 1.0));
}

void blendCsdPixelSrcAlphaOver(std::uint32_t& destinationArgb, const CsdColorRgba& source)
{
    const auto destination = unpackArgbPixel(destinationArgb);
    const double sourceAlpha = static_cast<double>(source.a) / 255.0;
    const double invAlpha = 1.0 - sourceAlpha;
    const CsdColorRgba blended{
        clampCsdByte((static_cast<double>(source.r) * sourceAlpha) + (static_cast<double>(destination.r) * invAlpha)),
        clampCsdByte((static_cast<double>(source.g) * sourceAlpha) + (static_cast<double>(destination.g) * invAlpha)),
        clampCsdByte((static_cast<double>(source.b) * sourceAlpha) + (static_cast<double>(destination.b) * invAlpha)),
        clampCsdByte(static_cast<double>(source.a) + (static_cast<double>(destination.a) * invAlpha)),
    };
    destinationArgb = packArgbPixel(blended);
}

void blendCsdPixelSrcAlphaOne(std::uint32_t& destinationArgb, const CsdColorRgba& source)
{
    const auto destination = unpackArgbPixel(destinationArgb);
    const double sourceAlpha = static_cast<double>(source.a) / 255.0;
    const CsdColorRgba blended{
        clampCsdByte((static_cast<double>(source.r) * sourceAlpha) + static_cast<double>(destination.r)),
        clampCsdByte((static_cast<double>(source.g) * sourceAlpha) + static_cast<double>(destination.g)),
        clampCsdByte((static_cast<double>(source.b) * sourceAlpha) + static_cast<double>(destination.b)),
        clampCsdByte(static_cast<double>(source.a) + static_cast<double>(destination.a)),
    };
    destinationArgb = packArgbPixel(blended);
}

[[nodiscard]] bool drawCsdDrawableCommandSoftware(
    std::vector<std::uint32_t>& canvasPixels,
    int canvasWidth,
    int canvasHeight,
    SwardSuUiAssetRenderer& renderer,
    const CsdDrawableCommand& command,
    CsdSoftwareRenderStats& stats)
{
    if (command.hidden)
        return true;

    const auto* texture = renderer.textureFor(command.textureName);
    if (!texture || !texture->image || !command.sourceFits || canvasWidth <= 0 || canvasHeight <= 0)
        return false;

    const double dstX = static_cast<double>(command.destinationX);
    const double dstY = static_cast<double>(command.destinationY);
    const double dstW = static_cast<double>(std::max(1, command.destinationWidth));
    const double dstH = static_cast<double>(std::max(1, command.destinationHeight));
    const double centerX = dstX + (dstW * 0.5);
    const double centerY = dstY + (dstH * 0.5);
    const double radians = command.rotation * kPi / 180.0;
    const double cosTheta = std::cos(radians);
    const double sinTheta = std::sin(radians);

    std::array<std::pair<double, double>, 4> corners{{
        { -dstW * 0.5, -dstH * 0.5 },
        { dstW * 0.5, -dstH * 0.5 },
        { -dstW * 0.5, dstH * 0.5 },
        { dstW * 0.5, dstH * 0.5 },
    }};

    double minX = static_cast<double>(canvasWidth);
    double minY = static_cast<double>(canvasHeight);
    double maxX = 0.0;
    double maxY = 0.0;
    for (const auto& [cornerX, cornerY] : corners)
    {
        const double rotatedX = centerX + (cornerX * cosTheta) - (cornerY * sinTheta);
        const double rotatedY = centerY + (cornerX * sinTheta) + (cornerY * cosTheta);
        minX = std::min(minX, rotatedX);
        minY = std::min(minY, rotatedY);
        maxX = std::max(maxX, rotatedX);
        maxY = std::max(maxY, rotatedY);
    }

    const int startX = std::clamp(static_cast<int>(std::floor(minX)), 0, canvasWidth - 1);
    const int startY = std::clamp(static_cast<int>(std::floor(minY)), 0, canvasHeight - 1);
    const int endX = std::clamp(static_cast<int>(std::ceil(maxX)), 0, canvasWidth - 1);
    const int endY = std::clamp(static_cast<int>(std::ceil(maxY)), 0, canvasHeight - 1);
    if (endX < startX || endY < startY)
        return true;

    ++stats.softwareQuadCommandCount;
    if (command.gradientKnown)
        ++stats.gradientVertexColorCommandCount;
    if (command.additiveBlend)
        ++stats.additiveSoftwareCommandCount;

    for (int y = startY; y <= endY; ++y)
    {
        for (int x = startX; x <= endX; ++x)
        {
            const double px = static_cast<double>(x) + 0.5;
            const double py = static_cast<double>(y) + 0.5;
            const double dx = px - centerX;
            const double dy = py - centerY;
            double localX = (dx * cosTheta) + (dy * sinTheta) + (dstW * 0.5);
            double localY = (-dx * sinTheta) + (dy * cosTheta) + (dstH * 0.5);
            if (command.flipX)
                localX = dstW - localX;
            if (command.flipY)
                localY = dstH - localY;
            if (localX < 0.0 || localY < 0.0 || localX >= dstW || localY >= dstH)
                continue;

            const double u = localX / dstW;
            const double v = localY / dstH;
            const double sourceX = static_cast<double>(command.sourceX) + (u * static_cast<double>(command.sourceWidth));
            const double sourceY = static_cast<double>(command.sourceY) + (v * static_cast<double>(command.sourceHeight));
            CsdColorRgba textureColor;
            if (command.linearFiltering)
            {
                textureColor = sampleTextureArgbBilinear(*texture->image, sourceX, sourceY);
                ++stats.samplerStats.bilinearSampleCount;
            }
            else
            {
                const double footprintX = static_cast<double>(std::max(1, command.sourceWidth)) / dstW;
                const double footprintY = static_cast<double>(std::max(1, command.sourceHeight)) / dstH;
                textureColor = sampleTextureArgbCsdFilter(*texture->image, sourceX, sourceY, footprintX, footprintY);
                ++stats.samplerStats.csdPointFilterSampleCount;
            }
            const auto vertexColor = multiplyCsdRgba(command.colorRgba, gradientVertexColor(command, u, v));
            const auto shaded = multiplyCsdRgba(textureColor, vertexColor);
            if (shaded.a == 0)
                continue;

            auto& destination = canvasPixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(canvasWidth) + static_cast<std::size_t>(x)];
            if (command.additiveBlend)
                blendCsdPixelSrcAlphaOne(destination, shaded);
            else
                blendCsdPixelSrcAlphaOver(destination, shaded);
        }
    }

    return true;
}

[[nodiscard]] bool drawCsdDrawableCommand(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const CsdDrawableCommand& command)
{
    if (command.hidden)
        return true;

    const auto destination = designRectToCanvas(canvas, command.destinationX, command.destinationY, command.destinationWidth, command.destinationHeight);
    const auto* texture = renderer.textureFor(command.textureName);
    if (!texture || !texture->image || !texture->bitmap || !command.sourceFits)
    {
        drawMissingCast(graphics, destination);
        return false;
    }

    // Shuriken's CSD reference path treats packed colors as RGBA and multiplies
    // cast color by per-corner gradients before sampling the texture. GDI+ has
    // no textured-quad vertex color path, so gradients are averaged here and
    // reported as an approximation in the Phase 128 manifest.
    const auto effectiveColor = effectiveCsdDrawRgba(command);
    const auto r = static_cast<float>(effectiveColor.r) / 255.0F;
    const auto g = static_cast<float>(effectiveColor.g) / 255.0F;
    const auto b = static_cast<float>(effectiveColor.b) / 255.0F;
    const auto a = static_cast<float>(effectiveColor.a) / 255.0F;
    Gdiplus::ColorMatrix matrix = {{
        { r, 0.0F, 0.0F, 0.0F, 0.0F },
        { 0.0F, g, 0.0F, 0.0F, 0.0F },
        { 0.0F, 0.0F, b, 0.0F, 0.0F },
        { 0.0F, 0.0F, 0.0F, a, 0.0F },
        { 0.0F, 0.0F, 0.0F, 0.0F, 1.0F },
    }};
    Gdiplus::ImageAttributes attributes;
    attributes.SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

    const auto state = graphics.Save();
    if (command.flipX || command.flipY)
    {
        graphics.TranslateTransform(destination.X + (destination.Width * 0.5F), destination.Y + (destination.Height * 0.5F));
        graphics.ScaleTransform(command.flipX ? -1.0F : 1.0F, command.flipY ? -1.0F : 1.0F);
        graphics.TranslateTransform(-(destination.X + (destination.Width * 0.5F)), -(destination.Y + (destination.Height * 0.5F)));
    }
    if (std::fabs(command.rotation) > 0.000001)
    {
        graphics.TranslateTransform(destination.X + (destination.Width * 0.5F), destination.Y + (destination.Height * 0.5F));
        graphics.RotateTransform(static_cast<Gdiplus::REAL>(command.rotation));
        graphics.TranslateTransform(-(destination.X + (destination.Width * 0.5F)), -(destination.Y + (destination.Height * 0.5F)));
    }

    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
    graphics.DrawImage(
        texture->bitmap.get(),
        destination,
        static_cast<float>(command.sourceX),
        static_cast<float>(command.sourceY),
        static_cast<float>(command.sourceWidth),
        static_cast<float>(command.sourceHeight),
        Gdiplus::UnitPixel,
        &attributes);
    graphics.Restore(state);
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

[[nodiscard]] const CsdDrawableScene* cachedCsdDrawableScene(const CsdPipelineTemplateBinding& binding)
{
    static std::vector<std::pair<std::string, std::optional<CsdDrawableScene>>> cache;
    const std::string key = std::string(binding.layoutFileName) + ":" + std::string(binding.primarySceneName);
    const auto found = std::find_if(
        cache.begin(),
        cache.end(),
        [&key](const std::pair<std::string, std::optional<CsdDrawableScene>>& entry)
        {
            return entry.first == key;
        });
    if (found != cache.end())
        return found->second ? &*found->second : nullptr;

    cache.emplace_back(key, loadCsdDrawableScene(binding));
    return cache.back().second ? &*cache.back().second : nullptr;
}

[[nodiscard]] bool renderCsdDrawableScene(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const SgfxTemplateRenderBinding& sgfxBinding)
{
    const auto* csdBinding = findCsdPipelineTemplateBinding(sgfxBinding.templateId);
    const auto* scene = csdBinding ? cachedCsdDrawableScene(*csdBinding) : nullptr;
    if (!scene || scene->commands.empty())
        return false;

    for (const auto& command : scene->commands)
        (void)drawCsdDrawableCommand(graphics, canvas, renderer, command);

    return true;
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

    const auto* binding = renderer.selectedSgfxTemplateBinding();
    const bool renderedCsdDrawableScene = binding
        ? renderCsdDrawableScene(graphics, canvas, renderer, *binding)
        : false;

    if (renderedCsdDrawableScene)
    {
        // The template lane is CSD draw-command driven when local scene evidence is available.
    }
    else if (screen.kind == RendererScreenKind::TitleLoopReconstruction)
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

    if (binding)
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

[[nodiscard]] std::string_view sgfxSlotNameForDrawableCommand(
    const SgfxTemplateRenderBinding& binding,
    const CsdDrawableCommand& command,
    std::size_t commandIndex)
{
    for (std::size_t index = 0; index < binding.slotCount; ++index)
    {
        if (binding.slots[index].textureName == std::string_view(command.textureName))
            return binding.slots[index].slotName;
    }

    if (binding.slotCount == 0)
        return "none";

    return binding.slots[std::min(commandIndex, binding.slotCount - 1)].slotName;
}

[[nodiscard]] std::string csdDrawableCommandDescriptor(
    std::string_view templateId,
    const CsdDrawableCommand& command)
{
    std::ostringstream descriptor;
    descriptor
        << "csd_draw_command="
        << templateId
        << ":"
        << command.sceneName
        << "/"
        << command.castName
        << "->"
        << command.castName
        << ":texture="
        << command.textureName
        << ":subimage="
        << command.subimageIndex
        << ":src="
        << command.sourceX
        << ","
        << command.sourceY
        << ","
        << command.sourceWidth
        << "x"
        << command.sourceHeight
        << ":dst="
        << command.destinationX
        << ","
        << command.destinationY
        << ","
        << command.destinationWidth
        << "x"
        << command.destinationHeight;
    if (command.flipX)
        descriptor << ":flipX=1";
    if (command.flipY)
        descriptor << ":flipY=1";
    if (!command.sourceFits)
        descriptor << ":source-out-of-bounds";
    return descriptor.str();
}

[[nodiscard]] std::string csdDrawableTransformDescriptor(
    std::string_view templateId,
    const CsdDrawableCommand& command)
{
    std::ostringstream descriptor;
    descriptor
        << "sampled_transform="
        << templateId
        << ":"
        << command.sceneName
        << "/"
        << command.castName
        << ":translation="
        << formatCsdNumber(command.translationX)
        << ","
        << formatCsdNumber(command.translationY)
        << ":scale="
        << formatCsdNumber(command.scaleX)
        << ","
        << formatCsdNumber(command.scaleY)
        << ":rotation="
        << formatCsdNumber(command.rotation);
    return descriptor.str();
}

[[nodiscard]] int runCsdDrawableSmoke(const std::optional<std::string>& templateFilter)
{
    std::size_t templateCount = 0;
    bool failed = false;
    std::vector<std::string> uniquePackages;
    std::vector<std::pair<std::string, std::optional<CsdDrawableScene>>> loadedScenes;
    std::vector<std::string> descriptors;

    auto drawableFor = [&loadedScenes](const CsdPipelineTemplateBinding& binding) -> const CsdDrawableScene*
    {
        const std::string key = std::string(binding.layoutFileName) + ":" + std::string(binding.primarySceneName);
        const auto found = std::find_if(
            loadedScenes.begin(),
            loadedScenes.end(),
            [&key](const std::pair<std::string, std::optional<CsdDrawableScene>>& entry)
            {
                return entry.first == key;
            });
        if (found != loadedScenes.end())
            return found->second ? &*found->second : nullptr;

        loadedScenes.emplace_back(key, loadCsdDrawableScene(binding));
        return loadedScenes.back().second ? &*loadedScenes.back().second : nullptr;
    };

    for (const auto& csdBinding : kCsdPipelineTemplateBindings)
    {
        if (templateFilter && csdBinding.templateId != *templateFilter)
            continue;

        ++templateCount;
        if (std::find(uniquePackages.begin(), uniquePackages.end(), csdBinding.layoutFileName) == uniquePackages.end())
            uniquePackages.emplace_back(csdBinding.layoutFileName);

        const auto* scene = drawableFor(csdBinding);
        const auto* sgfxBinding = findSgfxTemplateRenderBinding(csdBinding.templateId);
        if (!scene || !sgfxBinding || scene->commands.empty())
        {
            failed = true;
            continue;
        }

        std::ostringstream sceneDescriptor;
        sceneDescriptor
            << "csd_drawable="
            << csdBinding.templateId
            << ":layout="
            << csdBinding.layoutFileName
            << ":scene="
            << scene->sceneName
            << ":commands="
            << scene->commands.size()
            << ":casts="
            << scene->castCount
            << ":subimages="
            << scene->subimageCount;
        descriptors.push_back(sceneDescriptor.str());

        std::vector<std::string> emittedTextureBindings;
        for (std::size_t index = 0; index < scene->commands.size(); ++index)
        {
            const auto& command = scene->commands[index];
            descriptors.push_back(csdDrawableCommandDescriptor(csdBinding.templateId, command));
            descriptors.push_back(csdDrawableTransformDescriptor(csdBinding.templateId, command));

            std::ostringstream slotDescriptor;
            slotDescriptor
                << "sgfx_element_map="
                << csdBinding.templateId
                << ":scene="
                << scene->sceneName
                << ":cast="
                << command.castName
                << ":slot="
                << sgfxSlotNameForDrawableCommand(*sgfxBinding, command, index)
                << ":texture="
                << command.textureName;
            descriptors.push_back(slotDescriptor.str());

            if (std::find(emittedTextureBindings.begin(), emittedTextureBindings.end(), command.textureName) == emittedTextureBindings.end())
            {
                emittedTextureBindings.push_back(command.textureName);
                std::ostringstream textureDescriptor;
                textureDescriptor
                    << "texture_binding="
                    << csdBinding.templateId
                    << ":"
                    << command.textureName
                    << ":resolved="
                    << (command.textureResolved ? "1" : "0")
                    << ":size="
                    << command.textureWidth
                    << "x"
                    << command.textureHeight;
                descriptors.push_back(textureDescriptor.str());
            }
        }

        const auto manifest = findRuntimeEvidenceManifestForTarget(csdBinding.templateId);
        std::ostringstream evidenceDescriptor;
        evidenceDescriptor
            << "native_bmp_compare="
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
        << "sward_su_ui_asset_renderer csd drawable smoke ok "
        << "layout_source=research_uiux/data/layout_deep_analysis.json "
        << "templates=" << templateCount
        << " packages=" << uniquePackages.size()
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return failed || templateCount == 0 ? 1 : 0;
}

[[nodiscard]] std::string csdTimelineSampleDescriptor(
    std::string_view templateId,
    const CsdTimelineTrackSample& sample)
{
    std::ostringstream descriptor;
    descriptor
        << "timeline_sample="
        << templateId
        << ":"
        << sample.sceneName
        << "/"
        << sample.castName
        << ":track="
        << sample.trackType
        << ":frame="
        << sample.sampleFrame
        << ":value="
        << formatCsdNumber(sample.value);
    return descriptor.str();
}

[[nodiscard]] std::string formatPackedRgbaHex(std::uint32_t packed)
{
    std::ostringstream descriptor;
    descriptor
        << "0x"
        << std::uppercase
        << std::hex
        << std::setw(8)
        << std::setfill('0')
        << packed;
    return descriptor.str();
}

[[nodiscard]] std::string csdPackedRgbaTimelineSampleDescriptor(
    std::string_view templateId,
    const CsdTimelinePackedRgbaTrackSample& sample)
{
    std::ostringstream descriptor;
    descriptor
        << "timeline_rgba_sample="
        << templateId
        << ":"
        << sample.sceneName
        << "/"
        << sample.castName
        << ":track="
        << sample.trackType
        << ":frame="
        << sample.sampleFrame
        << ":rgba="
        << formatPackedRgbaHex(sample.packedRgba)
        << ":components="
        << static_cast<int>(sample.color.r)
        << ","
        << static_cast<int>(sample.color.g)
        << ","
        << static_cast<int>(sample.color.b)
        << ","
        << static_cast<int>(sample.color.a);
    return descriptor.str();
}

[[nodiscard]] std::string csdTimelineDrawCommandDescriptor(
    std::string_view templateId,
    const CsdTimelineTrackSample& sample,
    const CsdDrawableCommand& sampledCommand)
{
    std::ostringstream descriptor;
    descriptor
        << "timeline_draw_command="
        << templateId
        << ":"
        << sample.sceneName
        << "/"
        << sample.castName
        << ":frame="
        << sample.sampleFrame
        << ":track="
        << sample.trackType
        << ":value="
        << formatCsdNumber(sample.value)
        << ":dst="
        << sampledCommand.destinationX
        << ","
        << sampledCommand.destinationY
        << ","
        << sampledCommand.destinationWidth
        << "x"
        << sampledCommand.destinationHeight;
    return descriptor.str();
}

[[nodiscard]] int runCsdTimelineSmoke(const std::optional<std::string>& templateFilter)
{
    std::size_t templateCount = 0;
    bool failed = false;
    std::vector<std::string> uniquePackages;
    std::vector<std::pair<std::string, std::optional<CsdTimelinePlayback>>> loadedTimelines;
    std::vector<std::pair<std::string, std::optional<CsdDrawableScene>>> loadedScenes;
    std::vector<std::string> descriptors;

    auto timelineFor = [&loadedTimelines](const CsdPipelineTemplateBinding& binding) -> const CsdTimelinePlayback*
    {
        const std::string key = std::string(binding.layoutFileName) + ":" + std::string(binding.timelineSceneName) + ":" + std::string(binding.timelineAnimationName);
        const auto found = std::find_if(
            loadedTimelines.begin(),
            loadedTimelines.end(),
            [&key](const std::pair<std::string, std::optional<CsdTimelinePlayback>>& entry)
            {
                return entry.first == key;
            });
        if (found != loadedTimelines.end())
            return found->second ? &*found->second : nullptr;

        loadedTimelines.emplace_back(key, loadCsdTimelinePlayback(binding));
        return loadedTimelines.back().second ? &*loadedTimelines.back().second : nullptr;
    };

    auto drawableFor = [&loadedScenes](const CsdPipelineTemplateBinding& binding) -> const CsdDrawableScene*
    {
        const std::string key = std::string(binding.layoutFileName) + ":" + std::string(binding.primarySceneName);
        const auto found = std::find_if(
            loadedScenes.begin(),
            loadedScenes.end(),
            [&key](const std::pair<std::string, std::optional<CsdDrawableScene>>& entry)
            {
                return entry.first == key;
            });
        if (found != loadedScenes.end())
            return found->second ? &*found->second : nullptr;

        loadedScenes.emplace_back(key, loadCsdDrawableScene(binding));
        return loadedScenes.back().second ? &*loadedScenes.back().second : nullptr;
    };

    for (const auto& csdBinding : kCsdPipelineTemplateBindings)
    {
        if (templateFilter && csdBinding.templateId != *templateFilter)
            continue;

        ++templateCount;
        if (std::find(uniquePackages.begin(), uniquePackages.end(), csdBinding.layoutFileName) == uniquePackages.end())
            uniquePackages.emplace_back(csdBinding.layoutFileName);

        const auto* playback = timelineFor(csdBinding);
        const auto* sgfxBinding = findSgfxTemplateRenderBinding(csdBinding.templateId);
        if (!playback || !sgfxBinding)
        {
            failed = true;
            continue;
        }

        std::ostringstream timelineDescriptor;
        timelineDescriptor
            << "csd_timeline="
            << csdBinding.templateId
            << ":layout="
            << csdBinding.layoutFileName
            << ":scene="
            << playback->sceneName
            << ":animation="
            << playback->animationName
            << ":frame="
            << playback->sampleFrame
            << "/"
            << formatCsdNumber(playback->frameCount)
            << ":tracks="
            << playback->trackCount
            << ":numeric="
            << playback->numericTrackCount
            << ":keyframes="
            << playback->keyframeCount;
        descriptors.push_back(timelineDescriptor.str());

        for (const auto& sample : playback->samples)
            descriptors.push_back(csdTimelineSampleDescriptor(csdBinding.templateId, sample));
        for (const auto& sample : playback->packedRgbaSamples)
            descriptors.push_back(csdPackedRgbaTimelineSampleDescriptor(csdBinding.templateId, sample));

        if (const auto* drawable = drawableFor(csdBinding))
        {
            for (const auto& sample : playback->samples)
            {
                for (const auto& command : drawable->commands)
                {
                    const auto sampled = applyCsdTimelineToDrawableCommand(command, sample);
                    if (!sampled)
                        continue;

                    descriptors.push_back(csdTimelineDrawCommandDescriptor(csdBinding.templateId, sample, *sampled));
                }
            }
        }

        const auto manifest = findRuntimeEvidenceManifestForTarget(csdBinding.templateId);
        std::ostringstream comparisonDescriptor;
        comparisonDescriptor
            << "rendered_frame_compare="
            << csdBinding.templateId
            << ":target="
            << csdBinding.templateId
            << ":event="
            << sgfxBinding->requiredEventId
            << ":frame="
            << playback->sampleFrame
            << ":native="
            << (manifest ? "found" : "missing");
        descriptors.push_back(comparisonDescriptor.str());
    }

    if (templateFilter && templateCount == 0)
        failed = true;

    std::cout
        << "sward_su_ui_asset_renderer csd timeline smoke ok "
        << "layout_source=research_uiux/data/layout_deep_analysis.json "
        << "templates=" << templateCount
        << " packages=" << uniquePackages.size()
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return failed || templateCount == 0 ? 1 : 0;
}

[[nodiscard]] std::filesystem::path repoRootForOutput()
{
    for (const auto& root : repoRootCandidates())
    {
        std::error_code error;
        if (std::filesystem::is_regular_file(root / "research_uiux" / "runtime_reference" / "examples" / "su_ui_asset_renderer.cpp", error))
            return root;
    }

    return std::filesystem::current_path();
}

[[nodiscard]] std::string portablePath(const std::filesystem::path& path)
{
    std::error_code error;
    auto relative = std::filesystem::relative(path, repoRootForOutput(), error);
    std::string text = (error ? path : relative).generic_string();
    return text;
}

[[nodiscard]] std::string jsonEscape(std::string_view text)
{
    std::ostringstream escaped;
    for (const char ch : text)
    {
        switch (ch)
        {
        case '\\': escaped << "\\\\"; break;
        case '"': escaped << "\\\""; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default: escaped << ch; break;
        }
    }
    return escaped.str();
}

[[nodiscard]] std::optional<CLSID> imageEncoderClsid(const wchar_t* mimeType)
{
    UINT encoderCount = 0;
    UINT encoderBytes = 0;
    if (Gdiplus::GetImageEncodersSize(&encoderCount, &encoderBytes) != Gdiplus::Ok || encoderBytes == 0)
        return std::nullopt;

    std::vector<std::uint8_t> buffer(encoderBytes);
    auto* encoders = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
    if (Gdiplus::GetImageEncoders(encoderCount, encoderBytes, encoders) != Gdiplus::Ok)
        return std::nullopt;

    for (UINT index = 0; index < encoderCount; ++index)
    {
        if (std::wcscmp(encoders[index].MimeType, mimeType) == 0)
            return encoders[index].Clsid;
    }

    return std::nullopt;
}

[[nodiscard]] bool saveBitmapAsBmp(Gdiplus::Bitmap& bitmap, const std::filesystem::path& path)
{
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
        return false;

    const auto encoder = imageEncoderClsid(L"image/bmp");
    if (!encoder)
        return false;

    return bitmap.Save(path.wstring().c_str(), &*encoder, nullptr) == Gdiplus::Ok;
}

[[nodiscard]] std::optional<std::string> findRuntimeEvidenceRecordForTarget(
    const std::string& manifestText,
    std::string_view target)
{
    for (const auto objectSpan : jsonObjectSpansInArray(manifestText))
    {
        if (jsonStringField(objectSpan, "target").value_or("") == target)
            return std::string(objectSpan);
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> extractNativeBestBmpPathFromManifestText(
    const std::string& manifestText,
    std::string_view target)
{
    if (manifestText.empty())
        return std::nullopt;

    const auto targetRecord = findRuntimeEvidenceRecordForTarget(manifestText, target);
    if (targetRecord)
    {
        const auto summary = jsonObjectFieldSpan(*targetRecord, "nativeFrameSignalSummary");
        const auto bestPath = summary ? jsonStringField(*summary, "bestPath") : std::optional<std::string>{};
        if (bestPath && !bestPath->empty())
            return std::filesystem::path(*bestPath);
    }

    const std::string fieldNeedle = "\"target\"";
    const std::string valueNeedle = "\"" + std::string(target) + "\"";
    std::size_t offset = 0;
    while ((offset = manifestText.find(fieldNeedle, offset)) != std::string::npos)
    {
        const auto colonOffset = manifestText.find(':', offset + fieldNeedle.size());
        if (colonOffset == std::string::npos)
            break;

        const auto valueOffset = skipJsonWhitespace(manifestText, colonOffset + 1);
        const auto value = parseJsonStringAt(manifestText, valueOffset);
        if (value && *value == valueNeedle.substr(1, valueNeedle.size() - 2))
        {
            const auto summaryOffset = manifestText.find("\"nativeFrameSignalSummary\"", valueOffset);
            const auto bestPathOffset = summaryOffset == std::string::npos
                ? std::string::npos
                : manifestText.find("\"bestPath\"", summaryOffset);
            const auto bestPathColon = bestPathOffset == std::string::npos
                ? std::string::npos
                : manifestText.find(':', bestPathOffset);
            if (bestPathColon != std::string::npos)
            {
                const auto bestPathValueOffset = skipJsonWhitespace(manifestText, bestPathColon + 1);
                if (const auto fallbackBestPath = parseJsonStringAt(manifestText, bestPathValueOffset))
                    return std::filesystem::path(*fallbackBestPath);
            }
        }
        offset = colonOffset + 1;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> findNativeBestBmpPathForTarget(std::string_view target)
{
    std::optional<std::filesystem::path> bestPath;
    std::filesystem::file_time_type bestWriteTime{};
    bool hasBestWriteTime = false;

    for (const auto& root : repoRootCandidates())
    {
        std::error_code error;
        const auto evidenceRoot = root / "out" / "ui_lab_runtime_evidence";
        if (!std::filesystem::is_directory(evidenceRoot, error))
            continue;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(evidenceRoot, error))
        {
            if (error)
                break;
            if (!entry.is_regular_file(error) || entry.path().filename() != "capture_manifest.json")
                continue;

            const auto manifestText = readTextFile(entry.path());
            const auto candidate = extractNativeBestBmpPathFromManifestText(manifestText, target);
            if (!candidate)
                continue;

            const auto writeTime = std::filesystem::last_write_time(entry.path(), error);
            if (!bestPath || !hasBestWriteTime || (!error && writeTime > bestWriteTime))
            {
                bestPath = *candidate;
                if (!error)
                {
                    bestWriteTime = writeTime;
                    hasBestWriteTime = true;
                }
            }
        }
    }

    return bestPath;
}

[[nodiscard]] Gdiplus::Color bitmapPixelNearest(Gdiplus::Bitmap& bitmap, int sampleX, int sampleY, int gridWidth, int gridHeight)
{
    const int x = std::clamp(
        static_cast<int>(std::llround((static_cast<double>(sampleX) + 0.5) * static_cast<double>(bitmap.GetWidth()) / static_cast<double>(gridWidth))),
        0,
        std::max(0, static_cast<int>(bitmap.GetWidth()) - 1));
    const int y = std::clamp(
        static_cast<int>(std::llround((static_cast<double>(sampleY) + 0.5) * static_cast<double>(bitmap.GetHeight()) / static_cast<double>(gridHeight))),
        0,
        std::max(0, static_cast<int>(bitmap.GetHeight()) - 1));

    Gdiplus::Color color;
    bitmap.GetPixel(x, y, &color);
    return color;
}

[[nodiscard]] Gdiplus::Color bitmapPixelNearest(
    Gdiplus::Bitmap& bitmap,
    int sampleX,
    int sampleY,
    int gridWidth,
    int gridHeight,
    int cropX,
    int cropY,
    int cropWidth,
    int cropHeight)
{
    const int safeCropWidth = std::max(1, cropWidth);
    const int safeCropHeight = std::max(1, cropHeight);
    const int x = std::clamp(
        cropX + static_cast<int>(std::llround((static_cast<double>(sampleX) + 0.5) * static_cast<double>(safeCropWidth) / static_cast<double>(gridWidth))),
        0,
        std::max(0, static_cast<int>(bitmap.GetWidth()) - 1));
    const int y = std::clamp(
        cropY + static_cast<int>(std::llround((static_cast<double>(sampleY) + 0.5) * static_cast<double>(safeCropHeight) / static_cast<double>(gridHeight))),
        0,
        std::max(0, static_cast<int>(bitmap.GetHeight()) - 1));

    Gdiplus::Color color;
    bitmap.GetPixel(x, y, &color);
    return color;
}

void nativeAlignmentCrop(Gdiplus::Bitmap& native, BitmapComparisonStats& stats)
{
    const int nativeWidth = static_cast<int>(native.GetWidth());
    const int nativeHeight = static_cast<int>(native.GetHeight());
    stats.nativeAlignmentCropX = 0;
    stats.nativeAlignmentCropY = 0;
    stats.nativeAlignmentCropWidth = nativeWidth;
    stats.nativeAlignmentCropHeight = nativeHeight;

    if (nativeWidth <= 0 || nativeHeight <= 0)
        return;

    constexpr double kDesignAspect = static_cast<double>(kDesignWidth) / static_cast<double>(kDesignHeight);
    const double nativeAspect = static_cast<double>(nativeWidth) / static_cast<double>(nativeHeight);
    if (nativeAspect > kDesignAspect)
    {
        stats.nativeAlignmentCropWidth = std::max(1, static_cast<int>(std::llround(static_cast<double>(nativeHeight) * kDesignAspect)));
        stats.nativeAlignmentCropX = std::max(0, (nativeWidth - stats.nativeAlignmentCropWidth) / 2);
    }
    else if (nativeAspect < kDesignAspect)
    {
        stats.nativeAlignmentCropHeight = std::max(1, static_cast<int>(std::llround(static_cast<double>(nativeWidth) / kDesignAspect)));
        stats.nativeAlignmentCropY = std::max(0, (nativeHeight - stats.nativeAlignmentCropHeight) / 2);
    }
}

[[nodiscard]] std::pair<double, int> computeNativeCropDelta(
    Gdiplus::Bitmap& rendered,
    Gdiplus::Bitmap& native,
    const BitmapComparisonStats& stats,
    int cropX,
    int cropY,
    int cropWidth,
    int cropHeight)
{
    std::uint64_t totalAbs = 0;
    int maxAbs = 0;
    int sampleCount = 0;
    for (int y = 0; y < stats.sampleGridHeight; ++y)
    {
        for (int x = 0; x < stats.sampleGridWidth; ++x)
        {
            const auto left = bitmapPixelNearest(rendered, x, y, stats.sampleGridWidth, stats.sampleGridHeight);
            const auto right = bitmapPixelNearest(
                native,
                x,
                y,
                stats.sampleGridWidth,
                stats.sampleGridHeight,
                cropX,
                cropY,
                cropWidth,
                cropHeight);
            const int dr = std::abs(static_cast<int>(left.GetR()) - static_cast<int>(right.GetR()));
            const int dg = std::abs(static_cast<int>(left.GetG()) - static_cast<int>(right.GetG()));
            const int db = std::abs(static_cast<int>(left.GetB()) - static_cast<int>(right.GetB()));
            totalAbs += static_cast<std::uint64_t>(dr + dg + db);
            maxAbs = std::max(maxAbs, std::max({ dr, dg, db }));
            ++sampleCount;
        }
    }

    const double meanAbsRgb = sampleCount == 0 ? 0.0 : static_cast<double>(totalAbs) / static_cast<double>(sampleCount * 3);
    return { meanAbsRgb, maxAbs };
}

[[nodiscard]] CsdNativeFrameRegistration findBestNativeFrameRegistration(
    Gdiplus::Bitmap& rendered,
    Gdiplus::Bitmap& native,
    const BitmapComparisonStats& stats)
{
    CsdNativeFrameRegistration registration;
    registration.cropX = stats.nativeAlignmentCropX;
    registration.cropY = stats.nativeAlignmentCropY;
    registration.cropWidth = stats.nativeAlignmentCropWidth;
    registration.cropHeight = stats.nativeAlignmentCropHeight;

    const auto baseDelta = computeNativeCropDelta(
        rendered,
        native,
        stats,
        registration.cropX,
        registration.cropY,
        registration.cropWidth,
        registration.cropHeight);
    registration.baseMeanAbsRgb = baseDelta.first;
    registration.bestMeanAbsRgb = baseDelta.first;
    registration.bestMaxAbsRgb = baseDelta.second;

    const int nativeWidth = static_cast<int>(native.GetWidth());
    const int nativeHeight = static_cast<int>(native.GetHeight());
    const int maxShiftX = std::min(32, std::max(0, (nativeWidth - registration.cropWidth) / 2));
    const int maxShiftY = std::min(32, std::max(0, (nativeHeight - registration.cropHeight) / 2));
    const int stepX = std::max(1, maxShiftX / 2);
    const int stepY = std::max(1, maxShiftY / 2);
    const int baseCropX = registration.cropX;
    const int baseCropY = registration.cropY;

    for (int yOffset = -maxShiftY; yOffset <= maxShiftY; yOffset += stepY)
    {
        for (int xOffset = -maxShiftX; xOffset <= maxShiftX; xOffset += stepX)
        {
            const int candidateX = std::clamp(baseCropX + xOffset, 0, std::max(0, nativeWidth - registration.cropWidth));
            const int candidateY = std::clamp(baseCropY + yOffset, 0, std::max(0, nativeHeight - registration.cropHeight));
            ++registration.candidateCount;
            const auto delta = computeNativeCropDelta(
                rendered,
                native,
                stats,
                candidateX,
                candidateY,
                registration.cropWidth,
                registration.cropHeight);
            if (delta.first < registration.bestMeanAbsRgb)
            {
                registration.bestMeanAbsRgb = delta.first;
                registration.bestMaxAbsRgb = delta.second;
                registration.offsetX = candidateX - baseCropX;
                registration.offsetY = candidateY - baseCropY;
                registration.cropX = candidateX;
                registration.cropY = candidateY;
            }
        }
    }

    return registration;
}

[[nodiscard]] CsdFullFrameDeltaStats computeFullFrameDeltaStats(
    Gdiplus::Bitmap& rendered,
    Gdiplus::Bitmap& native,
    const BitmapComparisonStats& alignmentStats,
    const std::filesystem::path& diffFramePath)
{
    CsdFullFrameDeltaStats stats;
    std::uint64_t totalAbs = 0;
    std::vector<std::uint32_t> diffPixels(static_cast<std::size_t>(kDesignWidth) * static_cast<std::size_t>(kDesignHeight), 0xFF000000);

    for (int y = 0; y < kDesignHeight; ++y)
    {
        for (int x = 0; x < kDesignWidth; ++x)
        {
            const auto left = bitmapPixelNearest(rendered, x, y, kDesignWidth, kDesignHeight);
            const auto right = bitmapPixelNearest(
                native,
                x,
                y,
                kDesignWidth,
                kDesignHeight,
                alignmentStats.nativeAlignmentCropX,
                alignmentStats.nativeAlignmentCropY,
                alignmentStats.nativeAlignmentCropWidth,
                alignmentStats.nativeAlignmentCropHeight);

            const int dr = std::abs(static_cast<int>(left.GetR()) - static_cast<int>(right.GetR()));
            const int dg = std::abs(static_cast<int>(left.GetG()) - static_cast<int>(right.GetG()));
            const int db = std::abs(static_cast<int>(left.GetB()) - static_cast<int>(right.GetB()));
            const int channelDelta = dr + dg + db;
            totalAbs += static_cast<std::uint64_t>(channelDelta);
            stats.maxAbsRgb = std::max(stats.maxAbsRgb, std::max({ dr, dg, db }));
            if (channelDelta == 0)
                ++stats.exactMatchPixels;
            if (channelDelta > 24)
                ++stats.significantDeltaPixels;
            if (left.GetR() != 0 || left.GetG() != 0 || left.GetB() != 0)
                ++stats.renderNonBlackPixels;
            if (right.GetR() != 0 || right.GetG() != 0 || right.GetB() != 0)
                ++stats.nativeNonBlackPixels;

            diffPixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(kDesignWidth) + static_cast<std::size_t>(x)] =
                packArgbPixel(CsdColorRgba{
                    static_cast<std::uint8_t>(dr),
                    static_cast<std::uint8_t>(dg),
                    static_cast<std::uint8_t>(db),
                    255,
                });
        }
    }

    stats.pixelCount = kDesignWidth * kDesignHeight;
    stats.meanAbsRgb = stats.pixelCount == 0 ? 0.0 : static_cast<double>(totalAbs) / static_cast<double>(stats.pixelCount * 3);
    stats.renderNonBlackRatio = stats.pixelCount == 0 ? 0.0 : static_cast<double>(stats.renderNonBlackPixels) / static_cast<double>(stats.pixelCount);
    stats.nativeNonBlackRatio = stats.pixelCount == 0 ? 0.0 : static_cast<double>(stats.nativeNonBlackPixels) / static_cast<double>(stats.pixelCount);

    auto diffBitmap = bitmapFromArgbPixels(kDesignWidth, kDesignHeight, diffPixels);
    stats.computed = diffBitmap && saveBitmapAsBmp(*diffBitmap, diffFramePath);
    return stats;
}

[[nodiscard]] BitmapSignalStats computeBitmapSignalStats(Gdiplus::Bitmap& bitmap)
{
    BitmapSignalStats stats;
    stats.loaded = bitmap.GetLastStatus() == Gdiplus::Ok && bitmap.GetWidth() > 0 && bitmap.GetHeight() > 0;
    if (!stats.loaded)
        return stats;

    stats.width = static_cast<int>(bitmap.GetWidth());
    stats.height = static_cast<int>(bitmap.GetHeight());
    constexpr int kGridWidth = 64;
    constexpr int kGridHeight = 36;
    for (int y = 0; y < kGridHeight; ++y)
    {
        for (int x = 0; x < kGridWidth; ++x)
        {
            const auto color = bitmapPixelNearest(bitmap, x, y, kGridWidth, kGridHeight);
            stats.rgbSum += static_cast<std::uint64_t>(color.GetR()) + color.GetG() + color.GetB();
            stats.alphaSum += color.GetA();
            if (color.GetR() != 0 || color.GetG() != 0 || color.GetB() != 0)
                ++stats.rgbNonBlack;
        }
    }

    return stats;
}

[[nodiscard]] BitmapComparisonStats computeBitmapComparisonStats(
    Gdiplus::Bitmap& rendered,
    const std::optional<std::filesystem::path>& nativePath,
    const std::filesystem::path& diffFramePath)
{
    BitmapComparisonStats stats;
    stats.rendered = computeBitmapSignalStats(rendered);
    if (!nativePath)
        return stats;

    auto native = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(nativePath->wstring().c_str(), FALSE));
    if (!native || native->GetLastStatus() != Gdiplus::Ok)
        return stats;

    stats.nativeFound = true;
    stats.native = computeBitmapSignalStats(*native);
    nativeAlignmentCrop(*native, stats);
    const auto registration = findBestNativeFrameRegistration(rendered, *native, stats);
    stats.nativeAlignmentCropX = registration.cropX;
    stats.nativeAlignmentCropY = registration.cropY;
    stats.nativeAlignmentCropWidth = registration.cropWidth;
    stats.nativeAlignmentCropHeight = registration.cropHeight;
    stats.registrationOffsetX = registration.offsetX;
    stats.registrationOffsetY = registration.offsetY;
    stats.registrationCandidateCount = registration.candidateCount;
    stats.registrationBaseMeanAbsRgb = registration.baseMeanAbsRgb;
    stats.meanAbsRgb = registration.bestMeanAbsRgb;
    stats.maxAbsRgb = registration.bestMaxAbsRgb;
    stats.sampleCount = stats.sampleGridWidth * stats.sampleGridHeight;
    stats.fullFrame = computeFullFrameDeltaStats(rendered, *native, stats, diffFramePath);
    return stats;
}

[[nodiscard]] std::vector<CsdDrawableCommand> timelineSampledCommands(
    const CsdDrawableScene& scene,
    const CsdTimelinePlayback* playback,
    std::size_t& sampledCommandCount)
{
    std::vector<CsdDrawableCommand> commands = scene.commands;
    sampledCommandCount = 0;
    if (!playback)
        return commands;

    for (auto& command : commands)
    {
        for (const auto& sample : playback->samples)
        {
            const auto sampled = applyCsdTimelineToDrawableCommand(command, sample);
            if (!sampled)
                continue;

            command = *sampled;
            ++sampledCommandCount;
        }
        for (const auto& sample : playback->packedRgbaSamples)
        {
            const auto sampled = applyCsdPackedRgbaTimelineToDrawableCommand(command, sample);
            if (!sampled)
                continue;

            command = *sampled;
            ++sampledCommandCount;
        }
    }

    return commands;
}

[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap> renderCsdOffscreenFrame(
    const CsdDrawableScene& scene,
    const CsdTimelinePlayback* playback,
    SwardSuUiAssetRenderer& renderer,
    std::size_t& sampledCommandCount,
    CsdSoftwareRenderStats& renderStats)
{
    std::vector<std::uint32_t> canvasPixels(static_cast<std::size_t>(kDesignWidth) * static_cast<std::size_t>(kDesignHeight), 0xFF000000);
    const auto commands = timelineSampledCommands(scene, playback, sampledCommandCount);
    for (const auto& command : commands)
        (void)drawCsdDrawableCommandSoftware(canvasPixels, kDesignWidth, kDesignHeight, renderer, command, renderStats);

    return bitmapFromArgbPixels(kDesignWidth, kDesignHeight, canvasPixels);
}

[[nodiscard]] std::size_t countUniqueTextures(const std::vector<CsdDrawableCommand>& commands)
{
    std::vector<std::string> textures;
    for (const auto& command : commands)
    {
        if (std::find(textures.begin(), textures.end(), command.textureName) == textures.end())
            textures.push_back(command.textureName);
    }
    return textures.size();
}

[[nodiscard]] CsdMaterialParityTriage materialParityTriageForComparison(const CsdRenderedFrameComparison& comparison)
{
    CsdMaterialParityTriage triage;
    if (!comparison.visualDelta.nativeFound)
    {
        triage.primaryBlocker = "missing-native-oracle";
        triage.riskFlags.push_back("native-bmp-missing");
        return triage;
    }

    if (!comparison.visualDelta.fullFrame.computed)
    {
        triage.primaryBlocker = "full-frame-diff-missing";
        triage.riskFlags.push_back("diff-bmp-not-written");
        return triage;
    }

    triage.coverageGap = comparison.visualDelta.fullFrame.nativeNonBlackRatio - comparison.visualDelta.fullFrame.renderNonBlackRatio;
    triage.sampledVsFullFrameGap = comparison.visualDelta.meanAbsRgb - comparison.visualDelta.fullFrame.meanAbsRgb;

    if (triage.coverageGap > 0.35 && comparison.visualDelta.fullFrame.renderNonBlackRatio < 0.25)
    {
        const bool stageUiTarget = comparison.templateId == "sonic-hud" || comparison.templateId == "tutorial";
        triage.primaryBlocker = stageUiTarget ? "stage-background-not-rendered" : "native-background-not-rendered";
        triage.riskFlags.push_back(triage.primaryBlocker);
        triage.riskFlags.push_back("native-composite-includes-world-backbuffer");
    }
    else if (comparison.visualDelta.fullFrame.meanAbsRgb <= 12.0)
    {
        triage.primaryBlocker = "low-full-frame-delta";
    }
    else if (comparison.gradientCommandCount != 0 || comparison.gradientTrackSampleCount != 0)
    {
        triage.primaryBlocker = "gradient-material-delta";
        triage.riskFlags.push_back("gradient-material-risk");
    }
    else
    {
        triage.primaryBlocker = "shader-material-delta";
        triage.riskFlags.push_back("shader-material-risk");
    }

    if (comparison.csdPointFilterSampleCount != 0)
        triage.riskFlags.push_back("csd-point-seam-sampler-risk");
    if (comparison.linearFilteringCommandCount != 0 || comparison.bilinearSampleCount != 0)
        triage.riskFlags.push_back("linear-filtering-risk");
    if (comparison.additiveCommandCount != 0)
        triage.riskFlags.push_back("additive-blend-risk");
    if (comparison.decodedPackedKeyframeCount != 0)
        triage.riskFlags.push_back("packed-rgba-timeline-active");
    if (comparison.visualDelta.registrationOffsetX != 0 || comparison.visualDelta.registrationOffsetY != 0)
        triage.riskFlags.push_back("native-frame-registration-shift");

    return triage;
}

[[nodiscard]] CsdRenderedFrameComparison renderCsdFrameComparison(
    const CsdPipelineTemplateBinding& csdBinding,
    const SgfxTemplateRenderBinding& sgfxBinding,
    const std::filesystem::path& outputRoot)
{
    CsdRenderedFrameComparison comparison;
    comparison.templateId = std::string(csdBinding.templateId);
    comparison.layoutFileName = std::string(csdBinding.layoutFileName);
    comparison.sceneName = std::string(csdBinding.primarySceneName);
    comparison.timelineSceneName = std::string(csdBinding.timelineSceneName);
    comparison.timelineAnimationName = std::string(csdBinding.timelineAnimationName);
    comparison.frame = csdTimelineSampleFrameForTemplate(csdBinding.templateId);
    comparison.renderedFramePath = outputRoot / (std::string(csdBinding.templateId) + "_frame" + std::to_string(comparison.frame) + ".bmp");
    comparison.diffFramePath = outputRoot / (std::string(csdBinding.templateId) + "_frame" + std::to_string(comparison.frame) + "_diff.bmp");

    for (std::size_t index = 0; index < sgfxBinding.slotCount; ++index)
        comparison.sgfxSlots.emplace_back(sgfxBinding.slots[index].slotName);

    const auto scene = loadCsdDrawableScene(csdBinding);
    const auto playback = loadCsdTimelinePlayback(csdBinding);
    if (!scene)
        return comparison;

    comparison.drawCommandCount = scene->commands.size();
    comparison.textureBindingCount = countUniqueTextures(scene->commands);
    for (const auto& command : scene->commands)
    {
        if (command.colorKnown)
            ++comparison.colorCommandCount;
        if (command.colorRgba.a != 0xFF)
            ++comparison.alphaModulatedCommandCount;
        if (command.gradientKnown)
            ++comparison.gradientCommandCount;
        if (command.additiveBlend)
            ++comparison.additiveCommandCount;
        else
            ++comparison.normalBlendCommandCount;
        if (command.linearFiltering)
            ++comparison.linearFilteringCommandCount;
    }
    if (playback)
    {
        comparison.packedColorTrackCount = static_cast<std::size_t>(std::max(0, playback->packedColorTrackCount));
        comparison.packedGradientTrackCount = static_cast<std::size_t>(std::max(0, playback->packedGradientTrackCount));
        comparison.decodedPackedColorTrackCount = static_cast<std::size_t>(std::max(0, playback->decodedPackedColorTrackCount));
        comparison.decodedPackedGradientTrackCount = static_cast<std::size_t>(std::max(0, playback->decodedPackedGradientTrackCount));
        comparison.decodedPackedKeyframeCount = static_cast<std::size_t>(std::max(0, playback->decodedPackedKeyframeCount));
        comparison.unresolvedPackedKeyframeCount = static_cast<std::size_t>(std::max(0, playback->unresolvedPackedKeyframeCount));
        for (const auto& sample : playback->samples)
        {
            if (isCsdGradientTrack(sample.trackType))
                ++comparison.gradientTrackSampleCount;
        }
        for (const auto& sample : playback->packedRgbaSamples)
        {
            if (isCsdGradientTrack(sample.trackType))
                ++comparison.gradientTrackSampleCount;
        }
    }

    SwardSuUiAssetRenderer renderer;
    std::size_t sampledCommandCount = 0;
    CsdSoftwareRenderStats renderStats;
    auto bitmap = renderCsdOffscreenFrame(*scene, playback ? &*playback : nullptr, renderer, sampledCommandCount, renderStats);
    comparison.sampledCommandCount = sampledCommandCount;
    comparison.softwareQuadCommandCount = renderStats.softwareQuadCommandCount;
    comparison.gradientVertexColorCommandCount = renderStats.gradientVertexColorCommandCount;
    comparison.additiveSoftwareCommandCount = renderStats.additiveSoftwareCommandCount;
    comparison.csdPointFilterSampleCount = renderStats.samplerStats.csdPointFilterSampleCount;
    comparison.bilinearSampleCount = renderStats.samplerStats.bilinearSampleCount;
    comparison.nearestSampleCount = renderStats.samplerStats.nearestSampleCount;
    const bool saved = bitmap && saveBitmapAsBmp(*bitmap, comparison.renderedFramePath);
    comparison.nativeBestPath = findNativeBestBmpPathForTarget(csdBinding.templateId);
    comparison.visualDelta = bitmap
        ? computeBitmapComparisonStats(*bitmap, comparison.nativeBestPath, comparison.diffFramePath)
        : BitmapComparisonStats{};
    comparison.materialTriage = materialParityTriageForComparison(comparison);
    if (!saved)
        comparison.renderedFramePath.clear();
    return comparison;
}

void writeCsdRenderCompareManifest(
    const std::filesystem::path& manifestPath,
    const std::vector<CsdRenderedFrameComparison>& comparisons)
{
    std::error_code error;
    std::filesystem::create_directories(manifestPath.parent_path(), error);
    std::ofstream out(manifestPath, std::ios::binary);
    if (!out)
        return;

    out << "{\n";
    out << "  \"phase\": 131,\n";
    out << "  \"canvas\": { \"width\": " << kDesignWidth << ", \"height\": " << kDesignHeight << " },\n";
    out << "  \"records\": [\n";
    for (std::size_t index = 0; index < comparisons.size(); ++index)
    {
        const auto& comparison = comparisons[index];
        out << "    {\n";
        out << "      \"template\": \"" << jsonEscape(comparison.templateId) << "\",\n";
        out << "      \"layout\": \"" << jsonEscape(comparison.layoutFileName) << "\",\n";
        out << "      \"scene\": \"" << jsonEscape(comparison.sceneName) << "\",\n";
        out << "      \"timelineScene\": \"" << jsonEscape(comparison.timelineSceneName) << "\",\n";
        out << "      \"animation\": \"" << jsonEscape(comparison.timelineAnimationName) << "\",\n";
        out << "      \"frame\": " << comparison.frame << ",\n";
        out << "      \"renderedFramePath\": \"" << jsonEscape(portablePath(comparison.renderedFramePath)) << "\",\n";
        out << "      \"diffFramePath\": \"" << jsonEscape(portablePath(comparison.diffFramePath)) << "\",\n";
        out << "      \"nativeBestPath\": \"" << jsonEscape(comparison.nativeBestPath ? portablePath(*comparison.nativeBestPath) : std::string("")) << "\",\n";
        out << "      \"drawCommandCount\": " << comparison.drawCommandCount << ",\n";
        out << "      \"sampledCommandCount\": " << comparison.sampledCommandCount << ",\n";
        out << "      \"textureBindingCount\": " << comparison.textureBindingCount << ",\n";
        out << "      \"materialSemantics\": { \"quadRenderer\": \"software-argb\", \"samplerFilter\": \"csd-point-seam\", \"colorOrder\": \"rgba\", \"blend\": \"src-alpha/inv-src-alpha\", \"additiveBlend\": \"src-alpha/one\", \"colorCommands\": " << comparison.colorCommandCount
            << ", \"alphaModulatedCommands\": " << comparison.alphaModulatedCommandCount
            << ", \"gradientCommands\": " << comparison.gradientCommandCount
            << ", \"gradientApproxCommands\": " << comparison.gradientApproxCommandCount
            << ", \"gradientVertexColorCommands\": " << comparison.gradientVertexColorCommandCount
            << ", \"normalBlendCommands\": " << comparison.normalBlendCommandCount
            << ", \"additiveCommands\": " << comparison.additiveCommandCount
            << ", \"additiveSoftwareCommands\": " << comparison.additiveSoftwareCommandCount
            << ", \"linearFilteringCommands\": " << comparison.linearFilteringCommandCount
            << ", \"softwareQuadCommands\": " << comparison.softwareQuadCommandCount
            << ", \"csdPointFilterSamples\": " << comparison.csdPointFilterSampleCount
            << ", \"bilinearSamples\": " << comparison.bilinearSampleCount
            << ", \"nearestSamples\": " << comparison.nearestSampleCount
            << ", \"gradientTrackSamples\": " << comparison.gradientTrackSampleCount << " },\n";
        out << "      \"channelSemantics\": { \"packedColorTracks\": " << comparison.packedColorTrackCount
            << ", \"packedGradientTracks\": " << comparison.packedGradientTrackCount
            << ", \"decodedPackedColorTracks\": " << comparison.decodedPackedColorTrackCount
            << ", \"decodedPackedGradientTracks\": " << comparison.decodedPackedGradientTrackCount
            << ", \"decodedPackedKeyframes\": " << comparison.decodedPackedKeyframeCount
            << ", \"unresolvedPackedKeyframes\": " << comparison.unresolvedPackedKeyframeCount
            << ", \"status\": \"packed color/gradient keyframes decoded when raw RGBA payload fields are present\" },\n";
        out << "      \"nativeAlignment\": { \"mode\": \"" << jsonEscape(comparison.visualDelta.alignmentMode)
            << "\", \"crop\": { \"x\": " << comparison.visualDelta.nativeAlignmentCropX
            << ", \"y\": " << comparison.visualDelta.nativeAlignmentCropY
            << ", \"width\": " << comparison.visualDelta.nativeAlignmentCropWidth
            << ", \"height\": " << comparison.visualDelta.nativeAlignmentCropHeight
            << " }, \"registration\": { \"offsetX\": " << comparison.visualDelta.registrationOffsetX
            << ", \"offsetY\": " << comparison.visualDelta.registrationOffsetY
            << ", \"candidateCount\": " << comparison.visualDelta.registrationCandidateCount
            << ", \"baseMeanAbsRgb\": " << std::fixed << std::setprecision(3) << comparison.visualDelta.registrationBaseMeanAbsRgb << " } },\n";
        out << "      \"visualDelta\": { \"nativeFound\": " << (comparison.visualDelta.nativeFound ? "true" : "false")
            << ", \"sampleGrid\": \"64x36\", \"alignment\": \"" << jsonEscape(comparison.visualDelta.alignmentMode)
            << "\", \"sampleCount\": " << comparison.visualDelta.sampleCount
            << ", \"meanAbsRgb\": " << std::fixed << std::setprecision(3) << comparison.visualDelta.meanAbsRgb
            << ", \"maxAbsRgb\": " << comparison.visualDelta.maxAbsRgb
            << ", \"renderRgbSum\": " << comparison.visualDelta.rendered.rgbSum
            << ", \"nativeRgbSum\": " << comparison.visualDelta.native.rgbSum
            << ", \"fullFrame\": { \"mode\": \"" << jsonEscape(comparison.visualDelta.fullFrame.mode)
            << "\", \"computed\": " << (comparison.visualDelta.fullFrame.computed ? "true" : "false")
            << ", \"pixels\": " << comparison.visualDelta.fullFrame.pixelCount
            << ", \"exactMatchPixels\": " << comparison.visualDelta.fullFrame.exactMatchPixels
            << ", \"significantDeltaPixels\": " << comparison.visualDelta.fullFrame.significantDeltaPixels
            << ", \"meanAbsRgb\": " << std::fixed << std::setprecision(3) << comparison.visualDelta.fullFrame.meanAbsRgb
            << ", \"maxAbsRgb\": " << comparison.visualDelta.fullFrame.maxAbsRgb
            << ", \"renderNonBlackRatio\": " << std::fixed << std::setprecision(6) << comparison.visualDelta.fullFrame.renderNonBlackRatio
            << ", \"nativeNonBlackRatio\": " << comparison.visualDelta.fullFrame.nativeNonBlackRatio << " } },\n";
        out << "      \"materialParityTriage\": { \"primaryBlocker\": \"" << jsonEscape(comparison.materialTriage.primaryBlocker)
            << "\", \"coverageGap\": " << std::fixed << std::setprecision(6) << comparison.materialTriage.coverageGap
            << ", \"sampledVsFullFrameGap\": " << std::fixed << std::setprecision(3) << comparison.materialTriage.sampledVsFullFrameGap
            << ", \"riskFlags\": [";
        for (std::size_t flagIndex = 0; flagIndex < comparison.materialTriage.riskFlags.size(); ++flagIndex)
        {
            if (flagIndex != 0)
                out << ", ";
            out << "\"" << jsonEscape(comparison.materialTriage.riskFlags[flagIndex]) << "\"";
        }
        out << "] },\n";
        out << "      \"sgfxSlots\": [";
        for (std::size_t slotIndex = 0; slotIndex < comparison.sgfxSlots.size(); ++slotIndex)
        {
            if (slotIndex != 0)
                out << ", ";
            out << "\"" << jsonEscape(comparison.sgfxSlots[slotIndex]) << "\"";
        }
        out << "]\n";
        out << "    }" << (index + 1 == comparisons.size() ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
}

[[nodiscard]] std::string formatVisualDeltaLine(const CsdRenderedFrameComparison& comparison)
{
    std::ostringstream descriptor;
    descriptor
        << "visual_delta="
        << comparison.templateId
        << ":native="
        << (comparison.visualDelta.nativeFound ? "found" : "missing")
        << ":sample_grid="
        << comparison.visualDelta.sampleGridWidth
        << "x"
        << comparison.visualDelta.sampleGridHeight
        << ":alignment="
        << comparison.visualDelta.alignmentMode
        << ":mean_abs_rgb="
        << std::fixed
        << std::setprecision(3)
        << comparison.visualDelta.meanAbsRgb
        << ":max_abs_rgb="
        << comparison.visualDelta.maxAbsRgb
        << ":render_rgb_sum="
        << comparison.visualDelta.rendered.rgbSum
        << ":native_rgb_sum="
        << comparison.visualDelta.native.rgbSum;
    return descriptor.str();
}

[[nodiscard]] int runCsdRenderCompareSmoke(const std::optional<std::string>& templateFilter)
{
    std::size_t templateCount = 0;
    bool failed = false;
    std::vector<CsdRenderedFrameComparison> comparisons;
    const auto outputRoot = repoRootForOutput() / "out" / "csd_render_compare" / "phase131";
    const auto manifestPath = outputRoot / "csd_render_compare_manifest.json";

    Gdiplus::GdiplusStartupInput gdiplusInput{};
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        return 1;

    for (const auto& csdBinding : kCsdPipelineTemplateBindings)
    {
        if (templateFilter && csdBinding.templateId != *templateFilter)
            continue;

        ++templateCount;
        const auto* sgfxBinding = findSgfxTemplateRenderBinding(csdBinding.templateId);
        if (!sgfxBinding)
        {
            failed = true;
            continue;
        }

        auto comparison = renderCsdFrameComparison(csdBinding, *sgfxBinding, outputRoot);
        if (comparison.renderedFramePath.empty() || comparison.drawCommandCount == 0)
            failed = true;
        comparisons.push_back(std::move(comparison));
    }

    if (templateFilter && templateCount == 0)
        failed = true;

    writeCsdRenderCompareManifest(manifestPath, comparisons);

    std::cout
        << "sward_su_ui_asset_renderer csd render compare smoke ok "
        << "layout_source=research_uiux/data/layout_deep_analysis.json "
        << "templates=" << templateCount
        << " manifest=" << portablePath(manifestPath)
        << '\n';
    std::cout << "render_compare_manifest=" << portablePath(manifestPath) << '\n';

    for (const auto& comparison : comparisons)
    {
        std::cout
            << "rendered_frame_path="
            << comparison.templateId
            << ":"
            << portablePath(comparison.renderedFramePath)
            << '\n';
        std::cout
            << "diff_frame_path="
            << comparison.templateId
            << ":"
            << portablePath(comparison.diffFramePath)
            << '\n';
        std::cout
            << "material_semantics="
            << comparison.templateId
            << ":quad_renderer=software-argb"
            << ":sampler_filter=csd-point-seam"
            << ":color_order=rgba"
            << ":blend=src-alpha/inv-src-alpha"
            << ":additive_blend=src-alpha/one"
            << ":color_commands="
            << comparison.colorCommandCount
            << ":alpha_modulated="
            << comparison.alphaModulatedCommandCount
            << ":gradients="
            << comparison.gradientCommandCount
            << ":gradient_average_approx="
            << comparison.gradientApproxCommandCount
            << ":gradient_vertex_color="
            << comparison.gradientVertexColorCommandCount
            << ":additive_commands="
            << comparison.additiveCommandCount
            << ":additive_software="
            << comparison.additiveSoftwareCommandCount
            << ":linear_filter_commands="
            << comparison.linearFilteringCommandCount
            << ":software_quads="
            << comparison.softwareQuadCommandCount
            << ":csd_point_samples="
            << comparison.csdPointFilterSampleCount
            << ":bilinear_samples="
            << comparison.bilinearSampleCount
            << ":nearest_samples="
            << comparison.nearestSampleCount
            << ":gradient_samples="
            << comparison.gradientTrackSampleCount
            << '\n';
        std::cout
            << "channel_semantics="
            << comparison.templateId
            << ":packed_color_tracks="
            << comparison.packedColorTrackCount
            << ":packed_gradient_tracks="
            << comparison.packedGradientTrackCount
            << ":decoded_packed_color_tracks="
            << comparison.decodedPackedColorTrackCount
            << ":decoded_packed_gradient_tracks="
            << comparison.decodedPackedGradientTrackCount
            << ":decoded_packed_keyframes="
            << comparison.decodedPackedKeyframeCount
            << ":unresolved_packed_keyframes="
            << comparison.unresolvedPackedKeyframeCount
            << '\n';
        std::cout
            << "native_alignment="
            << comparison.templateId
            << ":mode="
            << comparison.visualDelta.alignmentMode
            << ":crop="
            << comparison.visualDelta.nativeAlignmentCropX
            << ","
            << comparison.visualDelta.nativeAlignmentCropY
            << ","
            << comparison.visualDelta.nativeAlignmentCropWidth
            << "x"
            << comparison.visualDelta.nativeAlignmentCropHeight
            << '\n';
        std::cout
            << "native_frame_registration="
            << comparison.templateId
            << ":mode="
            << comparison.visualDelta.alignmentMode
            << ":registration_offset="
            << comparison.visualDelta.registrationOffsetX
            << ","
            << comparison.visualDelta.registrationOffsetY
            << ":registration_candidates="
            << comparison.visualDelta.registrationCandidateCount
            << ":base_mean_abs_rgb="
            << std::fixed
            << std::setprecision(3)
            << comparison.visualDelta.registrationBaseMeanAbsRgb
            << ":best_mean_abs_rgb="
            << comparison.visualDelta.meanAbsRgb
            << '\n';
        std::cout << formatVisualDeltaLine(comparison) << '\n';
        std::cout
            << "full_frame_delta="
            << comparison.templateId
            << ":mode="
            << comparison.visualDelta.fullFrame.mode
            << ":pixels="
            << comparison.visualDelta.fullFrame.pixelCount
            << ":exact_match="
            << comparison.visualDelta.fullFrame.exactMatchPixels
            << ":significant="
            << comparison.visualDelta.fullFrame.significantDeltaPixels
            << ":mean_abs_rgb="
            << std::fixed
            << std::setprecision(3)
            << comparison.visualDelta.fullFrame.meanAbsRgb
            << ":max_abs_rgb="
            << comparison.visualDelta.fullFrame.maxAbsRgb
            << ":render_nonblack_ratio="
            << std::fixed
            << std::setprecision(6)
            << comparison.visualDelta.fullFrame.renderNonBlackRatio
            << ":native_nonblack_ratio="
            << comparison.visualDelta.fullFrame.nativeNonBlackRatio
            << '\n';
        std::cout
            << "material_parity_triage="
            << comparison.templateId
            << ":primary="
            << comparison.materialTriage.primaryBlocker
            << ":flags="
            << joinStrings(comparison.materialTriage.riskFlags)
            << ":coverage_gap="
            << std::fixed
            << std::setprecision(6)
            << comparison.materialTriage.coverageGap
            << ":sampled_vs_full_frame_gap="
            << std::fixed
            << std::setprecision(3)
            << comparison.materialTriage.sampledVsFullFrameGap
            << '\n';
        std::cout
            << "native_best_path="
            << comparison.templateId
            << ":"
            << (comparison.nativeBestPath ? portablePath(*comparison.nativeBestPath) : std::string("missing"))
            << '\n';
        std::cout
            << "render_frame_source="
            << comparison.templateId
            << ":layout="
            << comparison.layoutFileName
            << ":scene="
            << comparison.sceneName
            << ":timeline="
            << comparison.timelineSceneName
            << "/"
            << comparison.timelineAnimationName
            << ":frame="
            << comparison.frame
            << ":commands="
            << comparison.drawCommandCount
            << ":sampled_commands="
            << comparison.sampledCommandCount
            << ":textures="
            << comparison.textureBindingCount
            << '\n';
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return failed || templateCount == 0 ? 1 : 0;
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
    if (commandLineHasFlag("--csd-render-compare-smoke"))
        return runCsdRenderCompareSmoke(templateFilter);
    if (commandLineHasFlag("--csd-timeline-smoke"))
        return runCsdTimelineSmoke(templateFilter);
    if (commandLineHasFlag("--csd-drawable-smoke"))
        return runCsdDrawableSmoke(templateFilter);
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
