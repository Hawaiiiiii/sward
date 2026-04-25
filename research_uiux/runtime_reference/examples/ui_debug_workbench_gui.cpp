#include <sward/ui_runtime/contract_loader.hpp>
#include <sward/ui_runtime/debug_workbench_data.hpp>
#include <sward/ui_runtime/runtime.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <propidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <filesystem>
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

using namespace sward::ui_runtime;

namespace
{
struct ContractEntry
{
    std::filesystem::path path;
    ScreenContract contract;
};

struct ResolvedHostEntry
{
    const DebugWorkbenchHostEntry* metadata = nullptr;
    std::size_t contractIndex = 0;
};

struct GroupEntry
{
    std::string_view groupId;
    std::string_view groupDisplayName;
    std::string_view priority;
    std::vector<std::size_t> hostIndices;
};

struct AtlasCandidate
{
    std::string_view contractFileName;
    std::string_view shortName;
    std::string_view atlasFileName;
    std::string_view matchKind;
};

struct PreviewMotion
{
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    float alpha = 1.0F;
};

enum class PreviewFamily
{
    Generic,
    TitleMenu,
    PauseMenu,
    LoadingTransition,
};

enum class PreviewMode
{
    Runtime,
    Asset,
};

struct LayoutEvidence
{
    std::string_view contractFileName;
    std::string_view layoutId;
    std::string_view fileName;
    std::string_view verdict;
    std::string_view role;
    std::string_view sceneCueSummary;
    std::string_view animationCueSummary;
    std::string_view longestTimeline;
    int longestTimelineFrames = 0;
    int framesPerSecond = 60;
    int sceneCount = 0;
    int animationCount = 0;
    int castDictionaryCount = 0;
    int subimageCount = 0;
    int maxDepth = 0;
};

struct LayoutScenePrimitive
{
    std::string_view contractFileName;
    std::string_view scenePath;
    std::string_view sceneName;
    std::string_view animationName;
    std::string_view trackSummary;
    int frameCount = 0;
    int groupCount = 0;
    int castReferenceCount = 0;
    int maxCastCount = 0;
    int keyframeCount = 0;
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
};

struct LayoutCsdElementBinding
{
    std::string_view contractFileName;
    std::string_view packageFileName;
    std::string_view atlasFileName;
    std::string_view sceneName;
    std::string_view castName;
    std::string_view anchorCastName;
    std::string_view textureSummary;
    int sceneCastCount = 0;
    int sceneSubimageCount = 0;
    int packageSubimageCount = 0;
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
};

struct LayoutPrimitiveDrawCommand
{
    std::string_view sceneName;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::string channelSample;
};

struct LayoutAuthoredCastTransform
{
    std::string_view contractFileName;
    std::string_view packageFileName;
    std::string_view sceneName;
    std::string_view castName;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    float rotation = 0.0F;
    float scaleX = 1.0F;
    float scaleY = 1.0F;
    std::string_view color;
};

struct LayoutAuthoredKeyframeCurve
{
    std::string_view contractFileName;
    std::string_view packageFileName;
    std::string_view sceneName;
    std::string_view animationName;
    std::string_view castName;
    std::string_view trackType;
    int keyframeCount = 0;
    std::array<int, 5> frames{};
    std::array<float, 5> values{};
    std::array<std::string_view, 5> interpolationTypes{};
};

struct LayoutAuthoredKeyframeSample
{
    std::size_t curveIndex = 0;
    int frame = 0;
};

struct LayoutAuthoredSampledTransform
{
    std::size_t transformIndex = 0;
    std::size_t sampleIndex = 0;
};

struct LayoutAuthoredSampledTransformPixels
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct LayoutAuthoredSampledDrawCommand
{
    std::string sceneName;
    std::string castName;
    int baseX = 0;
    int baseY = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::string trackType;
    int sampleFrame = 0;
    float sampledValue = 0.0F;
    std::string sampledTrack;
};

struct LayoutAuthoredSampledChannelState
{
    std::string trackType;
    float sampledValue = 0.0F;
    float alpha = 1.0F;
    bool visible = true;
    int deltaX = 0;
    int deltaY = 0;
};

struct PrimitiveChannelMask
{
    bool color = false;
    bool sprite = false;
    bool transform = false;
    bool visibility = false;
};

struct PrimitiveChannelCounts
{
    int color = 0;
    int sprite = 0;
    int transform = 0;
    int visibility = 0;
    int staticPrimitive = 0;
};

struct VisualParitySummary
{
    std::string atlasBinding = "none";
    std::string layoutId = "none";
    int primitiveCount = 0;
    int keyframeCount = 0;
    PrimitiveChannelCounts channels;
};

inline constexpr const char* kPreviewPanelClassName = "SwardUiRuntimePreviewPanel";
inline constexpr UINT_PTR kPlaybackTimerId = 2001;
inline constexpr UINT kPlaybackTimerMilliseconds = 33;
inline constexpr double kPlaybackTickSeconds = 1.0 / 30.0;

inline constexpr std::array<AtlasCandidate, 10> kPreviewAtlasCandidates{{
    { "title_menu_reference.json", "title", "mainmenu__ui_mainmenu.png", "exact" },
    { "pause_menu_reference.json", "pause", "systemcommoncore__ui_pause.png", "exact" },
    { "autosave_toast_reference.json", "autosave", "autosave__ui_saveicon.png", "exact" },
    { "loading_transition_reference.json", "loading", "loading__ui_loading.png", "exact" },
    { "mission_result_reference.json", "mission_result", "actioncommon__ui_result.png", "exact" },
    { "world_map_reference.json", "world_map", "worldmap__ui_worldmap.png", "exact" },
    { "boss_hud_reference.json", "boss_hud", "bosscommon__ui_boss_gauge.png", "exact" },
    { "extra_stage_hud_reference.json", "extra_stage", "exstagetails_common__ui_prov_playscreen.png", "exact" },
    { "sonic_stage_hud_reference.json", "sonic_stage", "exstagetails_common__ui_prov_playscreen.png", "proxy" },
    { "werehog_stage_hud_reference.json", "werehog_stage", "exstagetails_common__ui_prov_playscreen.png", "proxy" },
}};

inline constexpr std::array<LayoutEvidence, 3> kLayoutEvidenceEntries{{
    {
        "title_menu_reference.json",
        "ui_mainmenu",
        "ui_mainmenu.xncp/.yncp",
        "strong",
        "title_menu",
        "mm_base, mm_bg_intro, mm_contentsitem_*, mm_donut_*, mm_title_*",
        "DefaultAnim, intro, move, sel1, sel2, sel3",
        "mm_donut_move/DefaultAnim: 220f @ 60fps",
        220,
        60,
        16,
        6,
        0,
        0,
        0,
    },
    {
        "pause_menu_reference.json",
        "ui_pause",
        "ui_pause.yncp",
        "direct",
        "pause_menu",
        "bg plus direct bg/footer/header-title layout-path anchors",
        "Intro_Anim plus 41 parsed authored animation banks",
        "btn_effect/charge_3_Outro: 240f @ 60fps",
        240,
        60,
        29,
        41,
        260,
        2871,
        4,
    },
    {
        "loading_transition_reference.json",
        "ui_loading",
        "ui_loading.yncp",
        "direct",
        "loading_transition",
        "bg_1, bg_2, event_viewer, loadinfo, n_2_d, pda, pda_txt",
        "Intro_Anim, Outro_Anim, 360_* variants, extra, ps3_* variants",
        "pda_txt/Usual_Anim_3: 240f @ 60fps",
        240,
        60,
        7,
        37,
        331,
        2240,
        0,
    },
}};

inline constexpr std::array<LayoutScenePrimitive, 36> kLayoutScenePrimitiveEntries{{
    { "title_menu_reference.json", "Root/mm_donut_move", "mm_donut_move", "DefaultAnim", "Color, Rotation, X/Y position, X/Y scale", 220, 5, 175, 35, 462, 0.08F, 0.28F, 0.44F, 0.16F },
    { "title_menu_reference.json", "Root/mm_base", "mm_base", "DefaultAnim", "Rotation, SubImage, Y position, Y scale", 120, 12, 47, 11, 105, 0.06F, 0.18F, 0.52F, 0.09F },
    { "title_menu_reference.json", "Root/mm_title_usual", "mm_title_usual", "Usual_Anim", "Color, X/Y scale", 60, 6, 34, 16, 95, 0.10F, 0.07F, 0.48F, 0.09F },
    { "title_menu_reference.json", "Root/mm_title_intro", "mm_title_intro", "Intro_Anim", "Color, X/Y position, X scale", 120, 5, 26, 15, 68, 0.12F, 0.16F, 0.42F, 0.08F },
    { "title_menu_reference.json", "Root/mm_contentsitem_select", "mm_contentsitem_select", "DefaultAnim", "Color, X/Y position, X/Y scale", 15, 4, 19, 8, 45, 0.12F, 0.62F, 0.50F, 0.12F },
    { "title_menu_reference.json", "Root/mm_donut_idle", "mm_donut_idle", "DefaultAnim", "Color, Y position", 40, 2, 46, 45, 31, 0.52F, 0.32F, 0.28F, 0.14F },

    { "pause_menu_reference.json", "Root/window_1/item/stick", "stick", "charge_3_Outro", "Color, HideFlag, Rotation, X/Y position, X/Y scale", 240, 6, 198, 33, 571, 0.34F, 0.26F, 0.46F, 0.14F },
    { "pause_menu_reference.json", "Root/footer/arrow", "arrow", "Intro_Anim", "Color, Gradient, HideFlag, X/Y position, X/Y scale", 20, 8, 136, 24, 161, 0.06F, 0.82F, 0.88F, 0.10F },
    { "pause_menu_reference.json", "Root/window_2/status_title", "status_title", "Intro_Anim", "Color, SubImage, X position", 69, 5, 10, 5, 24, 0.12F, 0.12F, 0.42F, 0.09F },
    { "pause_menu_reference.json", "Root/explanatory/bg_1_select", "bg_1_select", "Intro_Anim", "Color, X scale, Y position", 120, 3, 12, 4, 20, 0.17F, 0.56F, 0.60F, 0.12F },
    { "pause_menu_reference.json", "Root/header/select", "select", "Intro_Anim", "Color, Y position", 120, 2, 8, 4, 16, 0.08F, 0.06F, 0.50F, 0.08F },
    { "pause_menu_reference.json", "Root/explanatory/bg_1", "bg_1", "Intro_Anim", "Color, X/Y scale", 27, 2, 28, 14, 14, 0.18F, 0.44F, 0.54F, 0.10F },

    { "loading_transition_reference.json", "Root/bg_2", "bg_2", "Intro_Anim", "Color, X/Y position", 2, 52, 1378, 84, 1378, 0.05F, 0.14F, 0.90F, 0.16F },
    { "loading_transition_reference.json", "Root/pda", "pda", "Usual_Anim", "Color, X/Y position, X/Y scale", 240, 16, 152, 17, 465, 0.52F, 0.28F, 0.38F, 0.42F },
    { "loading_transition_reference.json", "Root/n_2_d", "n_2_d", "Intro_Anim", "Color, HideFlag, X/Y scale", 240, 24, 456, 44, 342, 0.08F, 0.24F, 0.36F, 0.32F },
    { "loading_transition_reference.json", "Root/loadinfo", "loadinfo", "Intro_Anim", "Color, Y position", 80, 3, 83, 79, 248, 0.14F, 0.70F, 0.72F, 0.12F },
    { "loading_transition_reference.json", "Root/event_viewer", "event_viewer", "Intro_Anim", "Gradient, SubImage, X/Y position, X/Y scale", 128, 9, 30, 5, 198, 0.06F, 0.05F, 0.88F, 0.10F },
    { "loading_transition_reference.json", "Root/pda_txt", "pda_txt", "Usual_Anim_3", "Color", 51, 2, 56, 28, 144, 0.54F, 0.62F, 0.34F, 0.13F },

    { "sonic_stage_hud_reference.json", "Root/so_speed_gauge", "so_speed_gauge", "Size_Anim", "Gradient, X/Y scale", 100, 1, 47, 47, 360, 0.55F, 0.58F, 0.36F, 0.12F },
    { "sonic_stage_hud_reference.json", "Root/so_ringenagy_gauge", "so_ringenagy_gauge", "Size_Anim", "Gradient, X scale", 100, 1, 43, 43, 240, 0.55F, 0.72F, 0.36F, 0.10F },
    { "sonic_stage_hud_reference.json", "Root/info_1", "info_1", "Count_Anim", "Gradient, HideFlag", 100, 3, 72, 24, 57, 0.07F, 0.29F, 0.42F, 0.14F },
    { "sonic_stage_hud_reference.json", "Root/info_2", "info_2", "Count_Anim", "HideFlag", 100, 3, 72, 24, 9, 0.07F, 0.66F, 0.38F, 0.12F },
    { "sonic_stage_hud_reference.json", "Root/ring_get_effect", "ring_get_effect", "Intro_Anim", "Gradient, Rotation", 5, 1, 2, 2, 14, 0.64F, 0.26F, 0.28F, 0.14F },
    { "sonic_stage_hud_reference.json", "Root/bg", "bg", "DefaultAnim", "DefaultAnim", 100, 6, 29, 21, 0, 0.08F, 0.08F, 0.50F, 0.18F },

    { "werehog_stage_hud_reference.json", "Root/so_speed_gauge", "so_speed_gauge", "Size_Anim", "Gradient, X/Y scale", 100, 1, 47, 47, 360, 0.55F, 0.58F, 0.36F, 0.12F },
    { "werehog_stage_hud_reference.json", "Root/so_ringenagy_gauge", "so_ringenagy_gauge", "Size_Anim", "Gradient, X scale", 100, 1, 43, 43, 240, 0.55F, 0.72F, 0.36F, 0.10F },
    { "werehog_stage_hud_reference.json", "Root/info_1", "info_1", "Count_Anim", "Gradient, HideFlag", 100, 3, 72, 24, 57, 0.07F, 0.29F, 0.42F, 0.14F },
    { "werehog_stage_hud_reference.json", "Root/info_2", "info_2", "Count_Anim", "HideFlag", 100, 3, 72, 24, 9, 0.07F, 0.66F, 0.38F, 0.12F },
    { "werehog_stage_hud_reference.json", "Root/ring_get_effect", "ring_get_effect", "Intro_Anim", "Gradient, Rotation", 5, 1, 2, 2, 14, 0.64F, 0.26F, 0.28F, 0.14F },
    { "werehog_stage_hud_reference.json", "Root/bg", "bg", "DefaultAnim", "DefaultAnim", 100, 6, 29, 21, 0, 0.08F, 0.08F, 0.50F, 0.18F },

    { "extra_stage_hud_reference.json", "Root/so_speed_gauge", "so_speed_gauge", "Size_Anim", "Gradient, X/Y scale", 100, 1, 47, 47, 360, 0.55F, 0.58F, 0.36F, 0.12F },
    { "extra_stage_hud_reference.json", "Root/so_ringenagy_gauge", "so_ringenagy_gauge", "Size_Anim", "Gradient, X scale", 100, 1, 43, 43, 240, 0.55F, 0.72F, 0.36F, 0.10F },
    { "extra_stage_hud_reference.json", "Root/info_1", "info_1", "Count_Anim", "Gradient, HideFlag", 100, 3, 72, 24, 57, 0.07F, 0.29F, 0.42F, 0.14F },
    { "extra_stage_hud_reference.json", "Root/info_2", "info_2", "Count_Anim", "HideFlag", 100, 3, 72, 24, 9, 0.07F, 0.66F, 0.38F, 0.12F },
    { "extra_stage_hud_reference.json", "Root/ring_get_effect", "ring_get_effect", "Intro_Anim", "Gradient, Rotation", 5, 1, 2, 2, 14, 0.64F, 0.26F, 0.28F, 0.14F },
    { "extra_stage_hud_reference.json", "Root/bg", "bg", "DefaultAnim", "DefaultAnim", 100, 6, 29, 21, 0, 0.08F, 0.08F, 0.50F, 0.18F },
}};

inline constexpr std::array<LayoutCsdElementBinding, 41> kLayoutCsdElementBindings{{
    { "title_menu_reference.json", "ui_mainmenu.xncp", "mainmenu__ui_mainmenu.png", "mm_bg_usual", "black3", "circle", "ui_mm_base.dds, ui_mm_parts1.dds, ui_mm_contentstext.dds", 47, 46, 736, 0.08F, 0.12F, 0.82F, 0.18F },
    { "title_menu_reference.json", "ui_mainmenu.xncp", "mainmenu__ui_mainmenu.png", "mm_bg_intro", "black3", "led_a2", "ui_mm_base.dds, ui_mm_parts1.dds, ui_mm_contentstext.dds", 99, 46, 736, 0.08F, 0.04F, 0.84F, 0.20F },
    { "title_menu_reference.json", "ui_mainmenu.xncp", "mainmenu__ui_mainmenu.png", "mm_donut_idle", "inner_donut", "compass", "ui_mm_base.dds, ui_mm_parts1.dds, ui_mm_contentstext.dds", 9, 46, 736, 0.52F, 0.32F, 0.28F, 0.14F },
    { "title_menu_reference.json", "ui_mainmenu.xncp", "mainmenu__ui_mainmenu.png", "mm_donut_select", "inner_donut", "compass", "ui_mm_base.dds, ui_mm_parts1.dds, ui_mm_contentstext.dds", 9, 46, 736, 0.52F, 0.46F, 0.28F, 0.14F },
    { "title_menu_reference.json", "ui_mainmenu.xncp", "mainmenu__ui_mainmenu.png", "mm_donut_move", "inner_donut", "compass", "ui_mm_base.dds, ui_mm_parts1.dds, ui_mm_contentstext.dds", 9, 46, 736, 0.08F, 0.28F, 0.44F, 0.16F },
    { "title_menu_reference.json", "ui_mainmenu.xncp", "mainmenu__ui_mainmenu.png", "mm_donut_intro", "inner_donut", "compass", "ui_mm_base.dds, ui_mm_parts1.dds, ui_mm_contentstext.dds", 9, 46, 736, 0.52F, 0.18F, 0.28F, 0.14F },
    { "title_menu_reference.json", "ui_mainmenu.xncp", "mainmenu__ui_mainmenu.png", "mm_contentsitem_select", "index_bg", "chain_l_top", "ui_mm_base.dds, ui_mm_parts1.dds, ui_mm_contentstext.dds", 19, 46, 736, 0.12F, 0.62F, 0.50F, 0.12F },
    { "title_menu_reference.json", "ui_mainmenu.xncp", "mainmenu__ui_mainmenu.png", "mm_contentsitem_idle", "index_bg", "index_bar1", "ui_mm_base.dds, ui_mm_parts1.dds, ui_mm_contentstext.dds", 13, 46, 736, 0.12F, 0.74F, 0.50F, 0.10F },

    { "pause_menu_reference.json", "ui_pause.yncp", "systemcommoncore__ui_pause.png", "bg", "img", "img", "mat_pause_en_*.dds, mat_comon_*.dds, mat_ex_common_002.dds", 1, 99, 2871, 0.04F, 0.08F, 0.92F, 0.82F },
    { "pause_menu_reference.json", "ui_pause.yncp", "systemcommoncore__ui_pause.png", "bg_1", "position", "center", "mat_pause_en_*.dds, mat_comon_*.dds, mat_ex_common_002.dds", 14, 99, 2871, 0.18F, 0.44F, 0.54F, 0.10F },
    { "pause_menu_reference.json", "ui_pause.yncp", "systemcommoncore__ui_pause.png", "bg_1_select", "position", "img", "mat_pause_en_*.dds, mat_comon_*.dds, mat_ex_common_002.dds", 4, 99, 2871, 0.17F, 0.56F, 0.60F, 0.12F },
    { "pause_menu_reference.json", "ui_pause.yncp", "systemcommoncore__ui_pause.png", "bg_2", "position_deco_1", "center", "mat_pause_en_*.dds, mat_comon_*.dds, mat_ex_common_002.dds", 68, 99, 2871, 0.10F, 0.20F, 0.80F, 0.16F },
    { "pause_menu_reference.json", "ui_pause.yncp", "systemcommoncore__ui_pause.png", "text_area", "text_area_1", "text_area_2", "mat_pause_en_*.dds, mat_comon_*.dds, mat_ex_common_002.dds", 3, 99, 2871, 0.24F, 0.36F, 0.52F, 0.12F },
    { "pause_menu_reference.json", "ui_pause.yncp", "systemcommoncore__ui_pause.png", "skill_select", "text_area_1", "TL", "mat_pause_en_*.dds, mat_comon_*.dds, mat_ex_common_002.dds", 27, 99, 2871, 0.24F, 0.48F, 0.52F, 0.16F },
    { "pause_menu_reference.json", "ui_pause.yncp", "systemcommoncore__ui_pause.png", "arrow", "L", "R", "mat_pause_en_*.dds, mat_comon_*.dds, mat_ex_common_002.dds", 2, 99, 2871, 0.06F, 0.82F, 0.88F, 0.10F },
    { "pause_menu_reference.json", "ui_pause.yncp", "systemcommoncore__ui_pause.png", "skill_scroll_bar_bg", "position", "middle", "mat_pause_en_*.dds, mat_comon_*.dds, mat_ex_common_002.dds", 6, 99, 2871, 0.82F, 0.32F, 0.04F, 0.42F },

    { "loading_transition_reference.json", "ui_loading.yncp", "loading__ui_loading.png", "bg_1", "arrow", "img_1", "mat_load_*.dds, mat_loadinfo_*.dds, mat_comon_txt_001.dds", 28, 320, 2240, 0.06F, 0.08F, 0.88F, 0.12F },
    { "loading_transition_reference.json", "ui_loading.yncp", "loading__ui_loading.png", "loadinfo", "controller", "ps3", "mat_load_*.dds, mat_loadinfo_*.dds, mat_comon_txt_001.dds", 106, 320, 2240, 0.14F, 0.70F, 0.72F, 0.12F },
    { "loading_transition_reference.json", "ui_loading.yncp", "loading__ui_loading.png", "n_2_d", "bg", "sky", "mat_load_*.dds, mat_loadinfo_*.dds, mat_comon_txt_001.dds", 10, 320, 2240, 0.08F, 0.24F, 0.36F, 0.32F },
    { "loading_transition_reference.json", "ui_loading.yncp", "loading__ui_loading.png", "event_viewer", "bg", "position", "mat_load_*.dds, mat_loadinfo_*.dds, mat_comon_txt_001.dds", 83, 320, 2240, 0.06F, 0.05F, 0.88F, 0.10F },
    { "loading_transition_reference.json", "ui_loading.yncp", "loading__ui_loading.png", "pda", "display", "imgbox", "mat_load_*.dds, mat_loadinfo_*.dds, mat_comon_txt_001.dds", 57, 320, 2240, 0.52F, 0.28F, 0.38F, 0.42F },
    { "loading_transition_reference.json", "ui_loading.yncp", "loading__ui_loading.png", "pda_txt", "display", "contens", "mat_load_*.dds, mat_loadinfo_*.dds, mat_comon_txt_001.dds", 19, 320, 2240, 0.54F, 0.62F, 0.34F, 0.13F },
    { "loading_transition_reference.json", "ui_loading.yncp", "loading__ui_loading.png", "bg_2", "arrow", "img_1", "mat_load_*.dds, mat_loadinfo_*.dds, mat_comon_txt_001.dds", 28, 320, 2240, 0.05F, 0.14F, 0.90F, 0.16F },

    { "extra_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "so_speed_gauge", "position_hd", "speed_bg", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 47, 109, 654, 0.54F, 0.58F, 0.38F, 0.12F },
    { "extra_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "so_ringenagy_gauge", "position_hd", "ringenagy_bg", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 43, 109, 654, 0.54F, 0.72F, 0.38F, 0.10F },
    { "extra_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "bg", "icon_position_hd", "so", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 29, 109, 654, 0.08F, 0.08F, 0.50F, 0.18F },
    { "extra_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "info_1", "position", "bg_1", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 24, 109, 654, 0.07F, 0.29F, 0.42F, 0.14F },
    { "extra_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "info_2", "position", "bg_1", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 24, 109, 654, 0.07F, 0.66F, 0.38F, 0.12F },
    { "extra_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "ring_get_effect", "position_hd", "flash", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 2, 109, 654, 0.64F, 0.26F, 0.28F, 0.14F },

    { "sonic_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "so_speed_gauge", "position_hd", "speed_bg", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 47, 109, 654, 0.54F, 0.58F, 0.38F, 0.12F },
    { "sonic_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "so_ringenagy_gauge", "position_hd", "ringenagy_bg", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 43, 109, 654, 0.54F, 0.72F, 0.38F, 0.10F },
    { "sonic_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "bg", "icon_position_hd", "so", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 29, 109, 654, 0.08F, 0.08F, 0.50F, 0.18F },
    { "sonic_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "info_1", "position", "bg_1", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 24, 109, 654, 0.07F, 0.29F, 0.42F, 0.14F },
    { "sonic_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "info_2", "position", "bg_1", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 24, 109, 654, 0.07F, 0.66F, 0.38F, 0.12F },
    { "sonic_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "ring_get_effect", "position_hd", "flash", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 2, 109, 654, 0.64F, 0.26F, 0.28F, 0.14F },

    { "werehog_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "so_speed_gauge", "position_hd", "speed_bg", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 47, 109, 654, 0.54F, 0.58F, 0.38F, 0.12F },
    { "werehog_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "so_ringenagy_gauge", "position_hd", "ringenagy_bg", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 43, 109, 654, 0.54F, 0.72F, 0.38F, 0.10F },
    { "werehog_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "bg", "icon_position_hd", "so", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 29, 109, 654, 0.08F, 0.08F, 0.50F, 0.18F },
    { "werehog_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "info_1", "position", "bg_1", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 24, 109, 654, 0.07F, 0.29F, 0.42F, 0.14F },
    { "werehog_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "info_2", "position", "bg_1", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 24, 109, 654, 0.07F, 0.66F, 0.38F, 0.12F },
    { "werehog_stage_hud_reference.json", "ui_prov_playscreen.yncp", "exstagetails_common__ui_prov_playscreen.png", "ring_get_effect", "position_hd", "flash", "ui_ps1_gauge1.dds, mat_playscreen_*.dds, mat_comon_num_001.dds", 2, 109, 654, 0.64F, 0.26F, 0.28F, 0.14F },
}};

inline constexpr std::array<LayoutAuthoredCastTransform, 3> kLayoutAuthoredCastTransforms{{
    { "title_menu_reference.json", "ui_mainmenu.xncp", "mm_donut_move", "index_text_pos", 408, 296, 16, 16, 0.0F, 1.0F, 1.0F, "0xFFFFFFFF" },
    { "pause_menu_reference.json", "ui_pause.yncp", "bg", "img", 0, 0, 1280, 720, 0.0F, 1.0F, 1.0F, "0x00000000" },
    { "loading_transition_reference.json", "ui_loading.yncp", "bg_2", "pos_text_sonic", 640, 360, 16, 16, 0.0F, 1.0F, 1.0F, "0xFFFFFFFF" },
}};

inline constexpr std::array<LayoutAuthoredKeyframeCurve, 3> kLayoutAuthoredKeyframeCurves{{
    {
        "title_menu_reference.json",
        "ui_mainmenu.xncp",
        "mm_donut_move",
        "intro",
        "index_text_pos",
        "YPosition",
        5,
        { 0, 15, 20, 35, 40 },
        { 0.411111F, 0.300000F, 0.300000F, 0.188889F, 0.188889F },
        { "Linear", "Linear", "Linear", "Linear", "Linear" },
    },
    {
        "pause_menu_reference.json",
        "ui_pause.yncp",
        "bg",
        "Intro_Anim",
        "img",
        "Color",
        2,
        { 0, 15, 0, 0, 0 },
        { 0.0F, 0.0F, 0.0F, 0.0F, 0.0F },
        { "Linear", "Linear", "", "", "" },
    },
    {
        "loading_transition_reference.json",
        "ui_loading.yncp",
        "bg_2",
        "360_sonic1",
        "pos_text_sonic",
        "XPosition",
        1,
        { 0, 0, 0, 0, 0 },
        { 0.5F, 0.0F, 0.0F, 0.0F, 0.0F },
        { "Linear", "", "", "", "" },
    },
}};

inline constexpr std::array<LayoutAuthoredKeyframeSample, 3> kLayoutAuthoredKeyframeSamples{{
    { 0, 30 },
    { 1, 7 },
    { 2, 1 },
}};

inline constexpr std::array<LayoutAuthoredSampledTransform, 3> kLayoutAuthoredSampledTransforms{{
    { 0, 0 },
    { 1, 1 },
    { 2, 2 },
}};

enum ControlId
{
    kGroupListId = 1001,
    kHostListId = 1002,
    kRunButtonId = 1003,
    kMoveNextButtonId = 1004,
    kConfirmButtonId = 1005,
    kCancelButtonId = 1006,
    kResetButtonId = 1007,
    kPlayPauseButtonId = 1008,
    kStepButtonId = 1009,
    kPreviewModeButtonId = 1010,
    kAssetPrevButtonId = 1011,
    kAssetNextButtonId = 1012,
    kCsdPrevButtonId = 1013,
    kCsdNextButtonId = 1014,
};

[[nodiscard]] const AtlasCandidate* atlasCandidateForContract(std::string_view contractFileName)
{
    const auto found = std::find_if(
        kPreviewAtlasCandidates.begin(),
        kPreviewAtlasCandidates.end(),
        [contractFileName](const AtlasCandidate& candidate)
        {
            return candidate.contractFileName == contractFileName;
        });
    return found == kPreviewAtlasCandidates.end() ? nullptr : &*found;
}

[[nodiscard]] const AtlasCandidate* atlasCandidateByShortName(std::string_view shortName)
{
    const auto found = std::find_if(
        kPreviewAtlasCandidates.begin(),
        kPreviewAtlasCandidates.end(),
        [shortName](const AtlasCandidate& candidate)
        {
            return candidate.shortName == shortName;
        });
    return found == kPreviewAtlasCandidates.end() ? nullptr : &*found;
}

[[nodiscard]] PreviewFamily previewFamilyForContract(std::string_view contractFileName)
{
    if (contractFileName == "title_menu_reference.json")
        return PreviewFamily::TitleMenu;
    if (contractFileName == "pause_menu_reference.json")
        return PreviewFamily::PauseMenu;
    if (contractFileName == "loading_transition_reference.json")
        return PreviewFamily::LoadingTransition;
    return PreviewFamily::Generic;
}

[[nodiscard]] std::string_view previewFamilyName(PreviewFamily family)
{
    switch (family)
    {
    case PreviewFamily::TitleMenu:
        return "title_menu";
    case PreviewFamily::PauseMenu:
        return "pause_menu";
    case PreviewFamily::LoadingTransition:
        return "loading_transition";
    case PreviewFamily::Generic:
        return "generic";
    }

    return "generic";
}

[[nodiscard]] const LayoutEvidence* layoutEvidenceForContract(std::string_view contractFileName)
{
    const auto found = std::find_if(
        kLayoutEvidenceEntries.begin(),
        kLayoutEvidenceEntries.end(),
        [contractFileName](const LayoutEvidence& evidence)
        {
            return evidence.contractFileName == contractFileName;
        });
    return found == kLayoutEvidenceEntries.end() ? nullptr : &*found;
}

[[nodiscard]] std::string assetViewerSummaryText(std::string_view contractFileName);

[[nodiscard]] std::vector<const LayoutScenePrimitive*> layoutScenePrimitivesForContract(std::string_view contractFileName)
{
    std::vector<const LayoutScenePrimitive*> primitives;
    for (const auto& primitive : kLayoutScenePrimitiveEntries)
    {
        if (primitive.contractFileName == contractFileName)
            primitives.push_back(&primitive);
    }
    return primitives;
}

[[nodiscard]] std::vector<const LayoutCsdElementBinding*> layoutCsdElementBindingsForContract(std::string_view contractFileName)
{
    std::vector<const LayoutCsdElementBinding*> bindings;
    for (const auto& binding : kLayoutCsdElementBindings)
    {
        if (binding.contractFileName == contractFileName)
            bindings.push_back(&binding);
    }
    return bindings;
}

[[nodiscard]] std::size_t selectedCsdElementBindingIndex(std::size_t requestedIndex, std::size_t bindingCount)
{
    return bindingCount == 0 ? 0 : requestedIndex % bindingCount;
}

[[nodiscard]] const LayoutCsdElementBinding* layoutCsdElementBindingAt(std::string_view contractFileName, std::size_t requestedIndex)
{
    const auto bindings = layoutCsdElementBindingsForContract(contractFileName);
    if (bindings.empty())
        return nullptr;
    return bindings[selectedCsdElementBindingIndex(requestedIndex, bindings.size())];
}

[[nodiscard]] std::string layoutCsdElementBindingDescriptor(const LayoutCsdElementBinding& binding)
{
    std::ostringstream text;
    text
        << binding.packageFileName
        << "/" << binding.sceneName
        << "/" << binding.castName
        << ":casts=" << binding.sceneCastCount
        << ":subimages=" << binding.sceneSubimageCount;
    return text.str();
}

[[nodiscard]] std::string layoutCsdElementBindingNavigationSummary(std::string_view contractFileName, std::size_t requestedIndex)
{
    const auto bindings = layoutCsdElementBindingsForContract(contractFileName);
    std::ostringstream text;
    text << "CSD element selection:\r\n";

    if (bindings.empty())
    {
        text << "  none\r\n";
        return text.str();
    }

    const std::size_t selectedIndex = selectedCsdElementBindingIndex(requestedIndex, bindings.size());
    const auto* selected = bindings[selectedIndex];
    text
        << "  " << (selectedIndex + 1) << "/" << bindings.size()
        << " " << layoutCsdElementBindingDescriptor(*selected)
        << " | anchor=" << selected->anchorCastName
        << " | textures=" << selected->textureSummary
        << "\r\n";
    return text.str();
}

[[nodiscard]] std::string layoutCsdElementBindingSummary(std::string_view contractFileName)
{
    const auto bindings = layoutCsdElementBindingsForContract(contractFileName);
    std::ostringstream text;
    text << "CSD element bindings:\r\n";

    if (bindings.empty())
    {
        text << "  none\r\n";
        return text.str();
    }

    for (const auto* binding : bindings)
    {
        text
            << "  " << layoutCsdElementBindingDescriptor(*binding)
            << " | atlas=" << binding->atlasFileName
            << " | anchor=" << binding->anchorCastName
            << " | package_subimages=" << binding->packageSubimageCount
            << "\r\n";
    }

    return text.str();
}

[[nodiscard]] std::vector<const LayoutAuthoredCastTransform*> layoutAuthoredCastTransformsForContract(std::string_view contractFileName)
{
    std::vector<const LayoutAuthoredCastTransform*> transforms;
    for (const auto& transform : kLayoutAuthoredCastTransforms)
    {
        if (transform.contractFileName == contractFileName)
            transforms.push_back(&transform);
    }
    return transforms;
}

[[nodiscard]] std::vector<const LayoutAuthoredKeyframeCurve*> layoutAuthoredKeyframeCurvesForContract(std::string_view contractFileName)
{
    std::vector<const LayoutAuthoredKeyframeCurve*> curves;
    for (const auto& curve : kLayoutAuthoredKeyframeCurves)
    {
        if (curve.contractFileName == contractFileName)
            curves.push_back(&curve);
    }
    return curves;
}

[[nodiscard]] std::vector<const LayoutAuthoredKeyframeSample*> layoutAuthoredKeyframeSamplesForContract(std::string_view contractFileName)
{
    std::vector<const LayoutAuthoredKeyframeSample*> samples;
    for (const auto& sample : kLayoutAuthoredKeyframeSamples)
    {
        if (sample.curveIndex >= kLayoutAuthoredKeyframeCurves.size())
            continue;

        if (kLayoutAuthoredKeyframeCurves[sample.curveIndex].contractFileName == contractFileName)
            samples.push_back(&sample);
    }
    return samples;
}

[[nodiscard]] std::vector<const LayoutAuthoredSampledTransform*> layoutAuthoredSampledTransformsForContract(std::string_view contractFileName)
{
    std::vector<const LayoutAuthoredSampledTransform*> transforms;
    for (const auto& sampledTransform : kLayoutAuthoredSampledTransforms)
    {
        if (sampledTransform.transformIndex >= kLayoutAuthoredCastTransforms.size())
            continue;
        if (sampledTransform.sampleIndex >= kLayoutAuthoredKeyframeSamples.size())
            continue;

        const auto& transform = kLayoutAuthoredCastTransforms[sampledTransform.transformIndex];
        const auto& sample = kLayoutAuthoredKeyframeSamples[sampledTransform.sampleIndex];
        if (sample.curveIndex >= kLayoutAuthoredKeyframeCurves.size())
            continue;

        const auto& curve = kLayoutAuthoredKeyframeCurves[sample.curveIndex];
        if (transform.contractFileName == contractFileName && curve.contractFileName == contractFileName)
            transforms.push_back(&sampledTransform);
    }
    return transforms;
}

[[nodiscard]] int layoutScenePrimitiveKeyframeTotal(const std::vector<const LayoutScenePrimitive*>& primitives)
{
    int total = 0;
    for (const auto* primitive : primitives)
        total += primitive->keyframeCount;
    return total;
}

[[nodiscard]] const LayoutScenePrimitive* layoutScenePrimitiveForScene(const std::vector<const LayoutScenePrimitive*>& primitives, std::string_view sceneName)
{
    const auto found = std::find_if(
        primitives.begin(),
        primitives.end(),
        [sceneName](const LayoutScenePrimitive* primitive)
        {
            return primitive->sceneName == sceneName;
        });
    return found == primitives.end() ? nullptr : *found;
}

[[nodiscard]] int layoutScenePrimitiveKeyframesForScene(const std::vector<const LayoutScenePrimitive*>& primitives, std::string_view sceneName)
{
    const auto* primitive = layoutScenePrimitiveForScene(primitives, sceneName);
    return primitive ? primitive->keyframeCount : -1;
}

[[nodiscard]] bool isProxyAtlasCandidate(const AtlasCandidate& candidate)
{
    return candidate.matchKind == std::string_view("proxy");
}

[[nodiscard]] std::string atlasCandidateDisplayName(const AtlasCandidate& candidate)
{
    std::string label(candidate.atlasFileName);
    if (isProxyAtlasCandidate(candidate))
        label += " (proxy)";
    return label;
}

[[nodiscard]] std::filesystem::path executableDirectory()
{
    std::array<char, MAX_PATH> buffer{};
    const DWORD length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size())
        return std::filesystem::current_path();

    return std::filesystem::path(std::string(buffer.data(), length)).parent_path();
}

void appendAssetRootCandidate(std::vector<std::filesystem::path>& candidates, const std::filesystem::path& path)
{
    if (path.empty())
        return;

    candidates.push_back(path);
    candidates.push_back(path / "visual_atlas/sheets");
    candidates.push_back(path / "extracted_assets/visual_atlas/sheets");
}

void appendAncestorAssetRootCandidates(std::vector<std::filesystem::path>& candidates, std::filesystem::path path)
{
    if (path.empty())
        return;

    if (std::filesystem::is_regular_file(path))
        path = path.parent_path();

    for (auto current = path; !current.empty(); current = current.parent_path())
    {
        candidates.push_back(current / "extracted_assets/visual_atlas/sheets");
        if (current == current.root_path())
            break;
    }
}

[[nodiscard]] std::optional<std::filesystem::path> environmentAssetRoot()
{
    const DWORD required = GetEnvironmentVariableA("SWARD_UI_ASSET_ROOT", nullptr, 0);
    if (required == 0)
        return std::nullopt;

    std::vector<char> value(required);
    const DWORD written = GetEnvironmentVariableA("SWARD_UI_ASSET_ROOT", value.data(), required);
    if (written == 0)
        return std::nullopt;

    return std::filesystem::path(std::string(value.data(), written));
}

[[nodiscard]] std::vector<std::filesystem::path> visualAtlasSheetRootCandidates()
{
    std::vector<std::filesystem::path> candidates;
    if (const auto root = environmentAssetRoot())
        appendAssetRootCandidate(candidates, *root);

    appendAncestorAssetRootCandidates(candidates, std::filesystem::current_path());
    appendAncestorAssetRootCandidates(candidates, executableDirectory());
    return candidates;
}

[[nodiscard]] std::filesystem::path visualAtlasSheetRoot()
{
    for (const auto& candidate : visualAtlasSheetRootCandidates())
    {
        std::error_code error;
        if (std::filesystem::is_directory(candidate, error))
            return candidate;
    }
    return std::filesystem::current_path() / "extracted_assets/visual_atlas/sheets";
}

[[nodiscard]] std::optional<std::filesystem::path> visualAtlasSheetForContract(std::string_view contractFileName)
{
    const auto* candidate = atlasCandidateForContract(contractFileName);
    if (!candidate)
        return std::nullopt;
    return visualAtlasSheetRoot() / std::string(candidate->atlasFileName);
}

[[nodiscard]] std::vector<std::filesystem::path> visualAtlasSheetFiles()
{
    std::vector<std::filesystem::path> files;
    const auto root = visualAtlasSheetRoot();
    std::error_code error;
    if (!std::filesystem::is_directory(root, error))
        return files;

    for (const auto& entry : std::filesystem::directory_iterator(root, error))
    {
        if (error)
            break;
        if (!entry.is_regular_file(error))
            continue;
        if (entry.path().extension() == ".png")
            files.push_back(entry.path());
    }

    std::sort(
        files.begin(),
        files.end(),
        [](const std::filesystem::path& left, const std::filesystem::path& right)
        {
            return left.filename().string() < right.filename().string();
        });
    return files;
}

[[nodiscard]] int visualAtlasSheetFileCount()
{
    return static_cast<int>(visualAtlasSheetFiles().size());
}

[[nodiscard]] std::optional<std::size_t> visualAtlasSheetIndexByFileName(std::string_view fileName)
{
    const auto files = visualAtlasSheetFiles();
    for (std::size_t index = 0; index < files.size(); ++index)
    {
        if (files[index].filename().string() == fileName)
            return index;
    }
    return std::nullopt;
}

[[nodiscard]] std::string assetViewerAtlasDescriptor(const AtlasCandidate& candidate)
{
    const bool exists = std::filesystem::exists(visualAtlasSheetRoot() / std::string(candidate.atlasFileName));
    std::ostringstream text;
    text
        << candidate.atlasFileName
        << ":" << candidate.matchKind
        << ":exists=" << (exists ? 1 : 0);
    return text.str();
}

[[nodiscard]] std::string assetViewerSummaryText(std::string_view contractFileName)
{
    std::ostringstream text;
    text
        << "Asset viewer:\r\n"
        << "  visual_atlas/sheets/*.png=" << visualAtlasSheetFileCount() << "\r\n";

    const auto* candidate = atlasCandidateForContract(contractFileName);
    if (!candidate)
    {
        text << "  selected atlas=none\r\n";
        return text.str();
    }

    text
        << "  selected atlas=" << assetViewerAtlasDescriptor(*candidate) << "\r\n";

    if (const auto* evidence = layoutEvidenceForContract(contractFileName))
    {
        text
            << "  layout=" << evidence->layoutId
            << " file=" << evidence->fileName
            << " scenes=" << evidence->sceneCount
            << " animations=" << evidence->animationCount
            << "\r\n";
    }
    else
    {
        text << "  layout=none\r\n";
    }

    const auto bindings = layoutCsdElementBindingsForContract(contractFileName);
    if (!bindings.empty())
    {
        text << "  csd_bindings=" << bindings.size();
        for (const auto* binding : bindings)
        {
            text
                << " | " << binding->packageFileName
                << "/" << binding->sceneName
                << "/" << binding->castName
                << " casts=" << binding->sceneCastCount
                << " subimages=" << binding->sceneSubimageCount;
        }
        text << "\r\n";
    }

    text
        << "  mode=unobstructed local atlas inspection; runtime/debug overlays are suppressed\r\n";
    return text.str();
}

[[nodiscard]] std::vector<ContractEntry> loadBundledEntries()
{
    std::vector<ContractEntry> result;
    for (const auto& path : bundledContractPaths())
        result.push_back(ContractEntry{ path, loadContractFromJsonFile(path) });
    return result;
}

[[nodiscard]] std::vector<ResolvedHostEntry> resolveHostEntries(const std::vector<ContractEntry>& bundledEntries)
{
    std::vector<ResolvedHostEntry> result;
    for (const auto& host : kDebugWorkbenchHostEntries)
    {
        const auto found = std::find_if(
            bundledEntries.begin(),
            bundledEntries.end(),
            [&host](const ContractEntry& entry)
            {
                return entry.path.filename() == host.primaryContractFileName;
            });
        if (found == bundledEntries.end())
            continue;

        result.push_back(
            ResolvedHostEntry{
                .metadata = &host,
                .contractIndex = static_cast<std::size_t>(std::distance(bundledEntries.begin(), found)),
            });
    }
    return result;
}

template <typename T>
void appendUnique(std::vector<T>& values, T value)
{
    if (std::find(values.begin(), values.end(), value) == values.end())
        values.push_back(value);
}

[[nodiscard]] std::vector<GroupEntry> buildGroups(const std::vector<ResolvedHostEntry>& hosts)
{
    std::vector<GroupEntry> groups;
    for (std::size_t index = 0; index < hosts.size(); ++index)
    {
        const auto& host = *hosts[index].metadata;
        auto found = std::find_if(
            groups.begin(),
            groups.end(),
            [&host](const GroupEntry& group)
            {
                return group.groupId == host.groupId;
            });
        if (found == groups.end())
        {
            groups.push_back(
                GroupEntry{
                    .groupId = host.groupId,
                    .groupDisplayName = host.groupDisplayName,
                    .priority = host.priority,
                    .hostIndices = { index },
                });
            continue;
        }
        found->hostIndices.push_back(index);
    }
    return groups;
}

[[nodiscard]] double timelineDuration(const ScreenContract& contract, ScreenState state)
{
    const auto stateIt = contract.states.find(state);
    if (stateIt == contract.states.end() || !stateIt->second.timelineBandId.has_value())
        return 0.0;

    const auto bandIt = contract.timelineBands.find(*stateIt->second.timelineBandId);
    return bandIt == contract.timelineBands.end() ? 0.0 : bandIt->second.seconds;
}

void enableAllPromptPredicates(ScreenRuntime& runtime, const ScreenContract& contract)
{
    std::vector<std::string> predicates;
    for (const auto& prompt : contract.promptSlots)
    {
        for (const auto& predicate : prompt.requiredPredicates)
            appendUnique(predicates, predicate);
    }

    for (const auto& predicate : predicates)
        runtime.setPredicate(predicate, true);
}

class GdiPlusSession
{
public:
    GdiPlusSession()
    {
        Gdiplus::GdiplusStartupInput input;
        if (Gdiplus::GdiplusStartup(&m_token, &input, nullptr) != Gdiplus::Ok)
            m_token = 0;
    }

    ~GdiPlusSession()
    {
        if (m_token != 0)
            Gdiplus::GdiplusShutdown(m_token);
    }

    GdiPlusSession(const GdiPlusSession&) = delete;
    GdiPlusSession& operator=(const GdiPlusSession&) = delete;

private:
    ULONG_PTR m_token = 0;
};

void drawTextLine(HDC dc, RECT bounds, const std::string& text, COLORREF color, UINT format = DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_VCENTER)
{
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextA(dc, text.c_str(), static_cast<int>(text.size()), &bounds, format);
}

[[nodiscard]] int layoutTimelineFrame(const LayoutEvidence& evidence, float progress)
{
    if (evidence.longestTimelineFrames <= 0)
        return 0;

    const float clampedProgress = std::clamp(progress, 0.0F, 1.0F);
    return static_cast<int>(std::round(clampedProgress * static_cast<float>(evidence.longestTimelineFrames)));
}

[[nodiscard]] int layoutScenePrimitiveFrame(const LayoutScenePrimitive& primitive, float progress)
{
    if (primitive.frameCount <= 0)
        return 0;

    const float clampedProgress = std::clamp(progress, 0.0F, 1.0F);
    return static_cast<int>(std::round(clampedProgress * static_cast<float>(primitive.frameCount)));
}

[[nodiscard]] bool trackSummaryContains(std::string_view trackSummary, std::string_view token)
{
    return trackSummary.find(token) != std::string_view::npos;
}

[[nodiscard]] PrimitiveChannelMask layoutPrimitiveChannelMask(const LayoutScenePrimitive& primitive)
{
    PrimitiveChannelMask mask{};
    mask.color =
        trackSummaryContains(primitive.trackSummary, "Color")
        || trackSummaryContains(primitive.trackSummary, "Gradient");
    mask.sprite = trackSummaryContains(primitive.trackSummary, "SubImage");
    mask.transform =
        trackSummaryContains(primitive.trackSummary, "position")
        || trackSummaryContains(primitive.trackSummary, "scale")
        || trackSummaryContains(primitive.trackSummary, "Rotation");
    mask.visibility = trackSummaryContains(primitive.trackSummary, "HideFlag");
    return mask;
}

[[nodiscard]] bool hasAnyChannel(const PrimitiveChannelMask& mask)
{
    return mask.color || mask.sprite || mask.transform || mask.visibility;
}

[[nodiscard]] std::string layoutPrimitiveChannelTags(const LayoutScenePrimitive& primitive)
{
    const PrimitiveChannelMask mask = layoutPrimitiveChannelMask(primitive);
    std::vector<std::string_view> tags;
    if (mask.color)
        tags.push_back("color");
    if (mask.sprite)
        tags.push_back("sprite");
    if (mask.transform)
        tags.push_back("transform");
    if (mask.visibility)
        tags.push_back("visibility");

    if (tags.empty())
        return "static";

    std::ostringstream text;
    for (std::size_t index = 0; index < tags.size(); ++index)
    {
        if (index > 0)
            text << "+";
        text << tags[index];
    }
    return text.str();
}

[[nodiscard]] std::string layoutPrimitiveChannelFrameToken(const LayoutScenePrimitive& primitive, float progress)
{
    std::ostringstream text;
    text
        << layoutPrimitiveChannelTags(primitive)
        << "@" << layoutScenePrimitiveFrame(primitive, progress)
        << "/" << primitive.frameCount;
    return text.str();
}

[[nodiscard]] std::string layoutPrimitiveChannelSampleToken(const LayoutScenePrimitive& primitive, float progress)
{
    return std::string(primitive.sceneName) + ":" + layoutPrimitiveChannelFrameToken(primitive, progress);
}

[[nodiscard]] PrimitiveChannelCounts layoutPrimitiveChannelCounts(const std::vector<const LayoutScenePrimitive*>& primitives)
{
    PrimitiveChannelCounts counts{};
    for (const auto* primitive : primitives)
    {
        const PrimitiveChannelMask mask = layoutPrimitiveChannelMask(*primitive);
        if (mask.color)
            ++counts.color;
        if (mask.sprite)
            ++counts.sprite;
        if (mask.transform)
            ++counts.transform;
        if (mask.visibility)
            ++counts.visibility;
        if (!hasAnyChannel(mask))
            ++counts.staticPrimitive;
    }
    return counts;
}

[[nodiscard]] std::string layoutPrimitiveChannelLegendLabel(const PrimitiveChannelCounts& counts)
{
    std::ostringstream text;
    text
        << "Channels T" << counts.transform
        << " C" << counts.color
        << " V" << counts.visibility
        << " S" << counts.sprite
        << " static" << counts.staticPrimitive;
    return text.str();
}

[[nodiscard]] std::string layoutPrimitiveChannelSampleSummary(std::string_view contractFileName, float progress)
{
    const auto primitives = layoutScenePrimitivesForContract(contractFileName);
    std::ostringstream text;
    text << "Layout primitive channel samples:\r\n";

    if (primitives.empty())
    {
        text << "  none\r\n";
        return text.str();
    }

    for (const auto* primitive : primitives)
        text << "  " << layoutPrimitiveChannelSampleToken(*primitive, progress) << "\r\n";

    return text.str();
}

[[nodiscard]] std::vector<LayoutPrimitiveDrawCommand> layoutPrimitiveDrawCommandsForContract(std::string_view contractFileName, float progress, int canvasWidth, int canvasHeight)
{
    std::vector<LayoutPrimitiveDrawCommand> commands;
    for (const auto* primitive : layoutScenePrimitivesForContract(contractFileName))
    {
        commands.push_back(
            LayoutPrimitiveDrawCommand{
                .sceneName = primitive->sceneName,
                .x = static_cast<int>(std::round(primitive->x * static_cast<float>(canvasWidth))),
                .y = static_cast<int>(std::round(primitive->y * static_cast<float>(canvasHeight))),
                .width = std::max(1, static_cast<int>(std::round(primitive->width * static_cast<float>(canvasWidth)))),
                .height = std::max(1, static_cast<int>(std::round(primitive->height * static_cast<float>(canvasHeight)))),
                .channelSample = layoutPrimitiveChannelFrameToken(*primitive, progress),
            });
    }
    return commands;
}

[[nodiscard]] std::string layoutPrimitiveDrawCommandDescriptor(const LayoutPrimitiveDrawCommand& command)
{
    std::ostringstream text;
    text
        << command.sceneName
        << ":" << command.x
        << "," << command.y
        << "," << command.width
        << "x" << command.height
        << ":" << command.channelSample;
    return text.str();
}

[[nodiscard]] std::string layoutPrimitiveDrawCommandSummary(std::string_view contractFileName, float progress, int canvasWidth, int canvasHeight)
{
    const auto commands = layoutPrimitiveDrawCommandsForContract(contractFileName, progress, canvasWidth, canvasHeight);
    std::ostringstream text;
    text << "Layout primitive draw commands:\r\n";

    if (commands.empty())
    {
        text << "  none\r\n";
        return text.str();
    }

    for (const auto& command : commands)
        text << "  " << layoutPrimitiveDrawCommandDescriptor(command) << "\r\n";

    return text.str();
}

[[nodiscard]] std::string layoutAuthoredCastTransformDescriptor(const LayoutAuthoredCastTransform& transform)
{
    std::ostringstream text;
    text << std::fixed << std::setprecision(2);
    text
        << transform.sceneName
        << "/" << transform.castName
        << ":" << transform.x
        << "," << transform.y
        << "," << transform.width
        << "x" << transform.height
        << ":r" << transform.rotation
        << ":s" << transform.scaleX
        << "," << transform.scaleY
        << ":" << transform.color;
    return text.str();
}

[[nodiscard]] std::string layoutAuthoredCastTransformSummary(std::string_view contractFileName)
{
    const auto transforms = layoutAuthoredCastTransformsForContract(contractFileName);
    std::ostringstream text;
    text << "Authored cast transforms:\r\n";

    if (transforms.empty())
    {
        text << "  none\r\n";
        return text.str();
    }

    for (const auto* transform : transforms)
    {
        text
            << "  " << transform->packageFileName
            << " :: " << layoutAuthoredCastTransformDescriptor(*transform)
            << "\r\n";
    }

    return text.str();
}

[[nodiscard]] std::string layoutAuthoredKeyframeCurveDescriptor(const LayoutAuthoredKeyframeCurve& curve)
{
    const int lastKeyframeIndex = std::max(0, curve.keyframeCount - 1);
    std::ostringstream text;
    text << std::fixed << std::setprecision(6);
    text
        << curve.sceneName
        << "/" << curve.animationName
        << "/" << curve.castName
        << "/" << curve.trackType
        << ":kf" << curve.keyframeCount
        << ":" << curve.frames[0]
        << "=" << curve.values[0]
        << "->" << curve.frames[static_cast<std::size_t>(lastKeyframeIndex)]
        << "=" << curve.values[static_cast<std::size_t>(lastKeyframeIndex)]
        << ":" << curve.interpolationTypes[0];
    return text.str();
}

[[nodiscard]] float layoutAuthoredKeyframeCurveValueAtFrame(const LayoutAuthoredKeyframeCurve& curve, int frame)
{
    if (curve.keyframeCount <= 0)
        return 0.0F;

    if (frame <= curve.frames[0])
        return curve.values[0];

    for (int index = 1; index < curve.keyframeCount; ++index)
    {
        const auto current = static_cast<std::size_t>(index);
        const auto previous = static_cast<std::size_t>(index - 1);
        if (frame > curve.frames[current])
            continue;

        const int frameDelta = curve.frames[current] - curve.frames[previous];
        if (frameDelta <= 0)
            return curve.values[current];

        const float t = static_cast<float>(frame - curve.frames[previous]) / static_cast<float>(frameDelta);
        return curve.values[previous] + ((curve.values[current] - curve.values[previous]) * t);
    }

    return curve.values[static_cast<std::size_t>(curve.keyframeCount - 1)];
}

[[nodiscard]] std::string layoutAuthoredKeyframeSampleDescriptor(const LayoutAuthoredKeyframeSample& sample)
{
    const auto& curve = kLayoutAuthoredKeyframeCurves[sample.curveIndex];
    std::ostringstream text;
    text << std::fixed << std::setprecision(6);
    text
        << curve.sceneName
        << "/" << curve.animationName
        << "/" << curve.castName
        << "/" << curve.trackType
        << "@" << sample.frame
        << "=" << layoutAuthoredKeyframeCurveValueAtFrame(curve, sample.frame);
    return text.str();
}

[[nodiscard]] LayoutAuthoredSampledTransformPixels layoutAuthoredSampledTransformPixels(const LayoutAuthoredSampledTransform& sampledTransform)
{
    const auto& transform = kLayoutAuthoredCastTransforms[sampledTransform.transformIndex];
    const auto& sample = kLayoutAuthoredKeyframeSamples[sampledTransform.sampleIndex];
    const auto& curve = kLayoutAuthoredKeyframeCurves[sample.curveIndex];
    const float sampledValue = layoutAuthoredKeyframeCurveValueAtFrame(curve, sample.frame);

    int x = transform.x;
    int y = transform.y;
    if (curve.trackType == std::string_view("XPosition"))
        x = static_cast<int>(std::lround(sampledValue * 1280.0F));
    else if (curve.trackType == std::string_view("YPosition"))
        y = static_cast<int>(std::lround(sampledValue * 720.0F));

    return LayoutAuthoredSampledTransformPixels{ x, y, transform.width, transform.height };
}

[[nodiscard]] std::string layoutAuthoredSampledTransformPreviewDescriptor(const LayoutAuthoredSampledTransform& sampledTransform)
{
    const auto& transform = kLayoutAuthoredCastTransforms[sampledTransform.transformIndex];
    const LayoutAuthoredSampledTransformPixels pixels = layoutAuthoredSampledTransformPixels(sampledTransform);

    std::ostringstream text;
    text
        << transform.sceneName
        << "/" << transform.castName
        << ":" << pixels.x
        << "," << pixels.y
        << "," << pixels.width
        << "x" << pixels.height;
    return text.str();
}

[[nodiscard]] std::string layoutAuthoredSampledTrackDescriptor(const LayoutAuthoredSampledTransform& sampledTransform)
{
    const auto& sample = kLayoutAuthoredKeyframeSamples[sampledTransform.sampleIndex];
    const auto& curve = kLayoutAuthoredKeyframeCurves[sample.curveIndex];
    const float sampledValue = layoutAuthoredKeyframeCurveValueAtFrame(curve, sample.frame);

    std::ostringstream text;
    text << std::fixed << std::setprecision(6);
    text
        << curve.trackType
        << "@" << sample.frame
        << "=" << sampledValue;
    return text.str();
}

[[nodiscard]] LayoutAuthoredSampledDrawCommand layoutAuthoredSampledDrawCommand(const LayoutAuthoredSampledTransform& sampledTransform)
{
    const auto& transform = kLayoutAuthoredCastTransforms[sampledTransform.transformIndex];
    const auto& sample = kLayoutAuthoredKeyframeSamples[sampledTransform.sampleIndex];
    const auto& curve = kLayoutAuthoredKeyframeCurves[sample.curveIndex];
    const LayoutAuthoredSampledTransformPixels pixels = layoutAuthoredSampledTransformPixels(sampledTransform);
    return LayoutAuthoredSampledDrawCommand{
        std::string(transform.sceneName),
        std::string(transform.castName),
        transform.x,
        transform.y,
        pixels.x,
        pixels.y,
        pixels.width,
        pixels.height,
        std::string(curve.trackType),
        sample.frame,
        layoutAuthoredKeyframeCurveValueAtFrame(curve, sample.frame),
        layoutAuthoredSampledTrackDescriptor(sampledTransform),
    };
}

[[nodiscard]] std::vector<LayoutAuthoredSampledDrawCommand> layoutAuthoredSampledDrawCommandsForContract(std::string_view contractFileName)
{
    std::vector<LayoutAuthoredSampledDrawCommand> commands;
    for (const auto* sampledTransform : layoutAuthoredSampledTransformsForContract(contractFileName))
        commands.push_back(layoutAuthoredSampledDrawCommand(*sampledTransform));
    return commands;
}

[[nodiscard]] std::string layoutAuthoredSampledDrawCommandDescriptor(const LayoutAuthoredSampledDrawCommand& command)
{
    std::ostringstream text;
    text
        << command.sceneName
        << "/" << command.castName
        << ":" << command.x
        << "," << command.y
        << "," << command.width
        << "x" << command.height
        << ":" << command.sampledTrack;
    return text.str();
}

[[nodiscard]] LayoutAuthoredSampledChannelState layoutAuthoredSampledDrawCommandChannelState(const LayoutAuthoredSampledDrawCommand& command)
{
    LayoutAuthoredSampledChannelState state{
        command.trackType,
        command.sampledValue,
        1.0F,
        true,
        command.x - command.baseX,
        command.y - command.baseY,
    };

    if (command.trackType == "Color"
        || command.trackType == "GradientTL"
        || command.trackType == "GradientBL"
        || command.trackType == "GradientTR"
        || command.trackType == "GradientBR")
    {
        state.alpha = std::clamp(command.sampledValue, 0.0F, 1.0F);
    }
    else if (command.trackType == "HideFlag")
    {
        state.visible = command.sampledValue < 0.5F;
    }

    return state;
}

[[nodiscard]] float layoutAuthoredSampledDrawCommandChannelAlpha(const LayoutAuthoredSampledDrawCommand& command)
{
    return layoutAuthoredSampledDrawCommandChannelState(command).alpha;
}

[[nodiscard]] std::string layoutAuthoredSampledDrawCommandChannelStateDescriptor(const LayoutAuthoredSampledDrawCommand& command)
{
    std::ostringstream text;
    text << std::fixed << std::setprecision(6);
    text
        << layoutAuthoredSampledDrawCommandDescriptor(command)
        << ":alpha=" << layoutAuthoredSampledDrawCommandChannelAlpha(command);
    return text.str();
}

[[nodiscard]] std::string layoutAuthoredSampledDrawCommandEvaluatedStateDescriptor(const LayoutAuthoredSampledDrawCommand& command)
{
    const LayoutAuthoredSampledChannelState state = layoutAuthoredSampledDrawCommandChannelState(command);
    std::ostringstream text;
    text << std::fixed << std::setprecision(6);
    text
        << layoutAuthoredSampledDrawCommandDescriptor(command)
        << ":alpha=" << state.alpha
        << ":visible=" << (state.visible ? 1 : 0)
        << ":delta=" << state.deltaX << "," << state.deltaY;
    return text.str();
}

[[nodiscard]] std::string layoutAuthoredSampledTransformDescriptor(const LayoutAuthoredSampledTransform& sampledTransform)
{
    const auto& transform = kLayoutAuthoredCastTransforms[sampledTransform.transformIndex];
    const auto& sample = kLayoutAuthoredKeyframeSamples[sampledTransform.sampleIndex];
    const auto& curve = kLayoutAuthoredKeyframeCurves[sample.curveIndex];
    const float sampledValue = layoutAuthoredKeyframeCurveValueAtFrame(curve, sample.frame);
    const LayoutAuthoredSampledTransformPixels pixels = layoutAuthoredSampledTransformPixels(sampledTransform);

    std::ostringstream text;
    text << std::fixed << std::setprecision(6);
    text
        << transform.sceneName
        << "/" << transform.castName
        << "@" << sample.frame
        << ":" << pixels.x
        << "," << pixels.y
        << "," << pixels.width
        << "x" << pixels.height
        << ":" << curve.trackType
        << "=" << sampledValue;
    return text.str();
}

[[nodiscard]] std::string layoutAuthoredKeyframeCurveSummary(std::string_view contractFileName)
{
    const auto curves = layoutAuthoredKeyframeCurvesForContract(contractFileName);
    std::ostringstream text;
    text << "Authored keyframe curves:\r\n";

    if (curves.empty())
    {
        text << "  none\r\n";
        return text.str();
    }

    for (const auto* curve : curves)
    {
        text
            << "  " << curve->packageFileName
            << " :: " << layoutAuthoredKeyframeCurveDescriptor(*curve)
            << "\r\n";
    }

    return text.str();
}

[[nodiscard]] std::string layoutAuthoredKeyframeSampleSummary(std::string_view contractFileName)
{
    const auto samples = layoutAuthoredKeyframeSamplesForContract(contractFileName);
    std::ostringstream text;
    text << "Authored keyframe samples:\r\n";

    if (samples.empty())
    {
        text << "  none\r\n";
        return text.str();
    }

    for (const auto* sample : samples)
    {
        const auto& curve = kLayoutAuthoredKeyframeCurves[sample->curveIndex];
        text
            << "  " << curve.packageFileName
            << " :: " << layoutAuthoredKeyframeSampleDescriptor(*sample)
            << "\r\n";
    }

    return text.str();
}

[[nodiscard]] std::string layoutAuthoredSampledTransformSummary(std::string_view contractFileName)
{
    const auto transforms = layoutAuthoredSampledTransformsForContract(contractFileName);
    std::ostringstream text;
    text << "Authored sampled transforms:\r\n";

    if (transforms.empty())
    {
        text << "  none\r\n";
        return text.str();
    }

    for (const auto* sampledTransform : transforms)
    {
        const auto& transform = kLayoutAuthoredCastTransforms[sampledTransform->transformIndex];
        text
            << "  " << transform.packageFileName
            << " :: " << layoutAuthoredSampledTransformDescriptor(*sampledTransform)
            << "\r\n";
    }

    return text.str();
}

[[nodiscard]] std::string layoutAuthoredSampledDrawCommandSummary(std::string_view contractFileName)
{
    const auto commands = layoutAuthoredSampledDrawCommandsForContract(contractFileName);
    std::ostringstream text;
    text << "Authored sampled draw commands:\r\n";

    if (commands.empty())
    {
        text << "  none\r\n";
        return text.str();
    }

    for (const auto& command : commands)
        text << "  " << layoutAuthoredSampledDrawCommandDescriptor(command) << "\r\n";

    return text.str();
}

[[nodiscard]] std::string visualParityChannelToken(const PrimitiveChannelCounts& counts)
{
    std::ostringstream text;
    text
        << "T" << counts.transform
        << " C" << counts.color
        << " V" << counts.visibility
        << " S" << counts.sprite
        << " static" << counts.staticPrimitive;
    return text.str();
}

[[nodiscard]] VisualParitySummary visualParitySummaryForContract(std::string_view contractFileName)
{
    const auto* candidate = atlasCandidateForContract(contractFileName);
    const auto* evidence = layoutEvidenceForContract(contractFileName);
    const auto primitives = layoutScenePrimitivesForContract(contractFileName);

    VisualParitySummary summary;
    if (candidate)
        summary.atlasBinding = std::string(candidate->matchKind);
    if (evidence)
        summary.layoutId = std::string(evidence->layoutId);
    summary.primitiveCount = static_cast<int>(primitives.size());
    summary.keyframeCount = layoutScenePrimitiveKeyframeTotal(primitives);
    summary.channels = layoutPrimitiveChannelCounts(primitives);
    return summary;
}

[[nodiscard]] bool visualParityHasChannels(const PrimitiveChannelCounts& counts)
{
    return counts.color > 0 || counts.sprite > 0 || counts.transform > 0 || counts.visibility > 0;
}

[[nodiscard]] std::string_view visualParityRendererBlocker(const VisualParitySummary& summary)
{
    if (summary.atlasBinding == "proxy")
        return "exact loose HUD payload";
    if (summary.atlasBinding == "none" && summary.layoutId == "none" && summary.primitiveCount == 0)
        return "visual evidence binding";
    if (visualParityHasChannels(summary.channels))
        return "decoded CSD channel sampling";
    if (summary.primitiveCount > 0)
        return "primitive transform sampling";
    if (summary.layoutId != "none")
        return "layout node transform decoding";
    return "visual evidence binding";
}

[[nodiscard]] std::string visualParitySummaryText(std::string_view contractFileName)
{
    const VisualParitySummary summary = visualParitySummaryForContract(contractFileName);
    std::ostringstream text;
    text
        << "Visual parity:\r\n"
        << "  atlas=" << summary.atlasBinding
        << " layout=" << summary.layoutId
        << " primitives=" << summary.primitiveCount
        << " keyframes=" << summary.keyframeCount
        << " channels=" << visualParityChannelToken(summary.channels)
        << "\r\n";
    text << "  next_renderer=" << visualParityRendererBlocker(summary) << "\r\n";

    if (summary.atlasBinding == "proxy")
        text << "  boundary=proxy atlas; exact loose HUD payload still unrecovered\r\n";
    else if (summary.atlasBinding == "exact")
        text << "  boundary=exact atlas candidate; still diagnostic until CSD channels drive draw commands\r\n";
    else
        text << "  boundary=contract-only; atlas/layout primitive evidence not yet bound\r\n";

    return text.str();
}

[[nodiscard]] std::string hostVisualReadinessBadge(const DebugWorkbenchHostEntry& host)
{
    const VisualParitySummary summary = visualParitySummaryForContract(host.primaryContractFileName);
    std::vector<std::string_view> tokens;
    if (summary.atlasBinding == "exact" || summary.atlasBinding == "proxy")
        tokens.push_back(summary.atlasBinding);
    if (summary.layoutId != "none")
        tokens.push_back("layout");
    if (summary.primitiveCount > 0)
        tokens.push_back("primitive");
    if (visualParityHasChannels(summary.channels))
        tokens.push_back("channels");
    if (tokens.empty())
        tokens.push_back("contract");

    std::ostringstream text;
    text << "[";
    for (std::size_t index = 0; index < tokens.size(); ++index)
    {
        if (index > 0)
            text << " ";
        text << tokens[index];
    }
    text << "]";
    return text.str();
}

[[nodiscard]] std::string hostDisplayLabel(const DebugWorkbenchHostEntry& host)
{
    return std::string(host.hostDisplayName) + " " + hostVisualReadinessBadge(host);
}

[[nodiscard]] std::string layoutPrimitiveCueSummary(std::string_view contractFileName, float progress)
{
    const auto primitives = layoutScenePrimitivesForContract(contractFileName);
    std::ostringstream text;
    text << "Layout primitive cues:\r\n";

    if (primitives.empty())
    {
        text << "  none\r\n";
        return text.str();
    }

    text
        << "  primitives=" << primitives.size()
        << " keyframes=" << layoutScenePrimitiveKeyframeTotal(primitives)
        << "\r\n";

    for (const auto* primitive : primitives)
    {
        text
            << "  " << primitive->sceneName
            << " / " << primitive->animationName
            << " : frame " << layoutScenePrimitiveFrame(*primitive, progress) << "/" << primitive->frameCount
            << ", keyframes=" << primitive->keyframeCount
            << ", channels=" << layoutPrimitiveChannelTags(*primitive)
            << ", tracks=" << primitive->trackSummary
            << "\r\n";
    }

    return text.str();
}

void drawLayoutTimelineBar(Gdiplus::Graphics& graphics, const Gdiplus::RectF& bar, float progress)
{
    const float clampedProgress = std::clamp(progress, 0.0F, 1.0F);
    Gdiplus::SolidBrush trackBrush(Gdiplus::Color(235, 24, 34, 44));
    Gdiplus::SolidBrush fillBrush(Gdiplus::Color(245, 247, 211, 72));
    Gdiplus::Pen barPen(Gdiplus::Color(235, 135, 205, 255), 1.0F);
    graphics.FillRectangle(&trackBrush, bar);
    graphics.FillRectangle(&fillBrush, bar.X, bar.Y, bar.Width * clampedProgress, bar.Height);
    graphics.DrawRectangle(&barPen, bar);
}

[[nodiscard]] Gdiplus::RectF clampRectToCanvas(Gdiplus::RectF rect, const Gdiplus::RectF& canvas);

[[nodiscard]] Gdiplus::RectF layoutAuthoredSampledTransformRect(const LayoutAuthoredSampledTransform& sampledTransform, const Gdiplus::RectF& canvas)
{
    const LayoutAuthoredSampledTransformPixels pixels = layoutAuthoredSampledTransformPixels(sampledTransform);
    return clampRectToCanvas(
        Gdiplus::RectF(
            canvas.X + (static_cast<float>(pixels.x) / 1280.0F * canvas.Width),
            canvas.Y + (static_cast<float>(pixels.y) / 720.0F * canvas.Height),
            std::max(2.0F, static_cast<float>(pixels.width) / 1280.0F * canvas.Width),
            std::max(2.0F, static_cast<float>(pixels.height) / 720.0F * canvas.Height)),
        canvas);
}

[[nodiscard]] Gdiplus::RectF layoutAuthoredSampledDrawCommandRect(const LayoutAuthoredSampledDrawCommand& command, const Gdiplus::RectF& canvas)
{
    return clampRectToCanvas(
        Gdiplus::RectF(
            canvas.X + (static_cast<float>(command.x) / 1280.0F * canvas.Width),
            canvas.Y + (static_cast<float>(command.y) / 720.0F * canvas.Height),
            std::max(2.0F, static_cast<float>(command.width) / 1280.0F * canvas.Width),
            std::max(2.0F, static_cast<float>(command.height) / 720.0F * canvas.Height)),
        canvas);
}

void drawAuthoredSampledTransforms(HDC dc, Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas, std::string_view contractFileName)
{
    const auto commands = layoutAuthoredSampledDrawCommandsForContract(contractFileName);
    if (commands.empty())
        return;

    for (const auto& command : commands)
    {
        const Gdiplus::RectF markerRect = layoutAuthoredSampledDrawCommandRect(command, canvas);
        const LayoutAuthoredSampledChannelState channelState = layoutAuthoredSampledDrawCommandChannelState(command);
        const BYTE diagnosticFillAlpha = static_cast<BYTE>(std::round(42.0F + (92.0F * channelState.alpha)));
        const BYTE diagnosticPenAlpha = static_cast<BYTE>(std::round(150.0F + (95.0F * channelState.alpha)));
        Gdiplus::SolidBrush markerBrush(Gdiplus::Color(diagnosticFillAlpha, 247, 211, 72));
        Gdiplus::Pen markerPen(Gdiplus::Color(diagnosticPenAlpha, 255, 247, 177), 1.6F);
        graphics.FillRectangle(&markerBrush, markerRect);
        graphics.DrawRectangle(&markerPen, markerRect);

        const float labelWidth = std::min(360.0F, std::max(190.0F, canvas.Width * 0.42F));
        const float labelHeight = 22.0F;
        float labelX = markerRect.X + markerRect.Width + 8.0F;
        if (labelX + labelWidth > canvas.X + canvas.Width - 8.0F)
            labelX = markerRect.X - labelWidth - 8.0F;
        labelX = std::max(canvas.X + 8.0F, std::min(labelX, canvas.X + canvas.Width - labelWidth - 8.0F));
        const float labelY = std::max(canvas.Y + 8.0F, std::min(markerRect.Y - 4.0F, canvas.Y + canvas.Height - labelHeight - 8.0F));

        Gdiplus::RectF labelRect(labelX, labelY, labelWidth, labelHeight);
        Gdiplus::SolidBrush labelBrush(Gdiplus::Color(230, 12, 18, 23));
        Gdiplus::Pen labelPen(Gdiplus::Color(230, 247, 211, 72), 1.0F);
        graphics.FillRectangle(&labelBrush, labelRect);
        graphics.DrawRectangle(&labelPen, labelRect);

        RECT textRect{
            static_cast<LONG>(labelRect.X + 6.0F),
            static_cast<LONG>(labelRect.Y),
            static_cast<LONG>(labelRect.X + labelRect.Width - 6.0F),
            static_cast<LONG>(labelRect.Y + labelRect.Height),
        };
        drawTextLine(dc, textRect, "authored sampled " + layoutAuthoredSampledDrawCommandEvaluatedStateDescriptor(command), RGB(255, 247, 177));
    }
}

void drawAssetCsdElementBindings(HDC dc, Gdiplus::Graphics& graphics, const Gdiplus::RectF& atlasRect, std::string_view contractFileName, std::size_t requestedIndex)
{
    const auto bindings = layoutCsdElementBindingsForContract(contractFileName);
    if (bindings.empty())
        return;

    const std::size_t activeIndex = selectedCsdElementBindingIndex(requestedIndex, bindings.size());
    for (std::size_t index = 0; index < bindings.size(); ++index)
    {
        const auto* binding = bindings[index];
        const bool selected = index == activeIndex;
        Gdiplus::RectF markerRect(
            atlasRect.X + (binding->x * atlasRect.Width),
            atlasRect.Y + (binding->y * atlasRect.Height),
            binding->width * atlasRect.Width,
            binding->height * atlasRect.Height);
        markerRect = clampRectToCanvas(markerRect, atlasRect);

        Gdiplus::SolidBrush markerBrush(selected ? Gdiplus::Color(76, 247, 211, 72) : Gdiplus::Color(34, 93, 245, 189));
        Gdiplus::Pen markerPen(selected ? Gdiplus::Color(245, 255, 247, 177) : Gdiplus::Color(170, 151, 255, 223), selected ? 2.0F : 1.0F);
        graphics.FillRectangle(&markerBrush, markerRect);
        graphics.DrawRectangle(&markerPen, markerRect);

        if (!selected)
            continue;

        const float labelWidth = std::min(430.0F, std::max(230.0F, atlasRect.Width * 0.50F));
        const float labelHeight = 42.0F;
        float labelX = markerRect.X + 8.0F;
        if (labelX + labelWidth > atlasRect.X + atlasRect.Width - 8.0F)
            labelX = atlasRect.X + atlasRect.Width - labelWidth - 8.0F;
        labelX = std::max(atlasRect.X + 8.0F, labelX);

        float labelY = markerRect.Y - labelHeight - 8.0F;
        if (labelY < atlasRect.Y + 8.0F)
            labelY = markerRect.Y + markerRect.Height + 8.0F;
        if (labelY + labelHeight > atlasRect.Y + atlasRect.Height - 8.0F)
            labelY = atlasRect.Y + atlasRect.Height - labelHeight - 8.0F;

        Gdiplus::RectF labelRect(labelX, labelY, labelWidth, labelHeight);
        Gdiplus::SolidBrush labelBrush(Gdiplus::Color(230, 8, 14, 20));
        Gdiplus::Pen labelPen(Gdiplus::Color(230, 151, 255, 223), 1.0F);
        graphics.FillRectangle(&labelBrush, labelRect);
        graphics.DrawRectangle(&labelPen, labelRect);

        RECT titleRect{
            static_cast<LONG>(labelRect.X + 8.0F),
            static_cast<LONG>(labelRect.Y + 2.0F),
            static_cast<LONG>(labelRect.X + labelRect.Width - 8.0F),
            static_cast<LONG>(labelRect.Y + 22.0F),
        };
        std::ostringstream title;
        title << "CSD " << (index + 1) << "/" << bindings.size() << " " << layoutCsdElementBindingDescriptor(*binding);
        drawTextLine(dc, titleRect, title.str(), RGB(236, 255, 248));

        std::ostringstream detail;
        detail
            << "anchor=" << binding->anchorCastName
            << " package_subimages=" << binding->packageSubimageCount;
        RECT detailRect{
            static_cast<LONG>(labelRect.X + 8.0F),
            static_cast<LONG>(labelRect.Y + 20.0F),
            static_cast<LONG>(labelRect.X + labelRect.Width - 8.0F),
            static_cast<LONG>(labelRect.Y + labelRect.Height - 2.0F),
        };
        drawTextLine(dc, detailRect, detail.str(), RGB(164, 214, 154));
    }
}

void drawLayoutScenePrimitives(HDC dc, Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas, const std::vector<const LayoutScenePrimitive*>& primitives, float timelineProgress)
{
    if (primitives.empty())
        return;

    const int maxKeyframes = std::max(
        1,
        (*std::max_element(
            primitives.begin(),
            primitives.end(),
            [](const LayoutScenePrimitive* left, const LayoutScenePrimitive* right)
            {
                return left->keyframeCount < right->keyframeCount;
            }))->keyframeCount);

    for (const auto* primitive : primitives)
    {
        Gdiplus::RectF primitiveRect(
            canvas.X + (primitive->x * canvas.Width),
            canvas.Y + (primitive->y * canvas.Height),
            primitive->width * canvas.Width,
            primitive->height * canvas.Height);
        primitiveRect = clampRectToCanvas(primitiveRect, canvas);

        const float density = std::clamp(static_cast<float>(primitive->keyframeCount) / static_cast<float>(maxKeyframes), 0.12F, 1.0F);
        const BYTE fillAlpha = static_cast<BYTE>(std::round(40.0F + (95.0F * density)));
        Gdiplus::SolidBrush primitiveBrush(Gdiplus::Color(fillAlpha, 93, 245, 189));
        Gdiplus::Pen primitivePen(Gdiplus::Color(220, 151, 255, 223), 1.2F);
        graphics.FillRectangle(&primitiveBrush, primitiveRect);
        graphics.DrawRectangle(&primitivePen, primitiveRect);

        const int frame = layoutScenePrimitiveFrame(*primitive, timelineProgress);
        std::ostringstream label;
        label
            << primitive->sceneName
            << " / " << primitive->animationName
            << " | kf=" << primitive->keyframeCount
            << " | ch=" << layoutPrimitiveChannelTags(*primitive)
            << " | f=" << frame << "/" << primitive->frameCount;

        RECT labelRect{
            static_cast<LONG>(primitiveRect.X + 6.0F),
            static_cast<LONG>(primitiveRect.Y + 2.0F),
            static_cast<LONG>(primitiveRect.X + primitiveRect.Width - 6.0F),
            static_cast<LONG>(primitiveRect.Y + std::min(24.0F, primitiveRect.Height)),
        };
        drawTextLine(dc, labelRect, label.str(), RGB(236, 255, 248));

        const float barMargin = 6.0F;
        const float barHeight = 4.0F;
        Gdiplus::RectF primitiveBar(
            primitiveRect.X + barMargin,
            primitiveRect.Y + primitiveRect.Height - barMargin - barHeight,
            std::max(1.0F, primitiveRect.Width - (barMargin * 2.0F)),
            barHeight);
        drawLayoutTimelineBar(graphics, primitiveBar, timelineProgress);
    }
}

void drawLayoutPrimitiveChannelLegend(HDC dc, Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas, const std::vector<const LayoutScenePrimitive*>& primitives)
{
    if (primitives.empty())
        return;

    const PrimitiveChannelCounts counts = layoutPrimitiveChannelCounts(primitives);
    const std::string label = layoutPrimitiveChannelLegendLabel(counts);
    const float legendWidth = std::min(292.0F, std::max(80.0F, canvas.Width - 20.0F));
    Gdiplus::RectF legendRect(
        std::max(canvas.X + 10.0F, canvas.X + canvas.Width - legendWidth - 10.0F),
        std::max(6.0F, canvas.Y - 32.0F),
        legendWidth,
        24.0F);

    Gdiplus::SolidBrush legendBrush(Gdiplus::Color(220, 8, 14, 20));
    Gdiplus::Pen legendPen(Gdiplus::Color(235, 247, 211, 72), 1.0F);
    graphics.FillRectangle(&legendBrush, legendRect);
    graphics.DrawRectangle(&legendPen, legendRect);

    RECT labelRect{
        static_cast<LONG>(legendRect.X + 8.0F),
        static_cast<LONG>(legendRect.Y),
        static_cast<LONG>(legendRect.X + legendRect.Width - 8.0F),
        static_cast<LONG>(legendRect.Y + legendRect.Height),
    };
    drawTextLine(dc, labelRect, label, RGB(248, 241, 176));
}

void drawLayoutEvidenceOverlay(HDC dc, Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas, const LayoutEvidence& evidence, float timelineProgress)
{
    const float panelWidth = std::min(340.0F, canvas.Width * 0.46F);
    const float panelHeight = 118.0F;
    const Gdiplus::RectF panel(
        canvas.X + canvas.Width - panelWidth - 14.0F,
        canvas.Y + 14.0F,
        panelWidth,
        panelHeight);

    Gdiplus::SolidBrush panelBrush(Gdiplus::Color(210, 8, 14, 20));
    Gdiplus::Pen panelPen(Gdiplus::Color(235, 111, 196, 255), 1.2F);
    graphics.FillRectangle(&panelBrush, panel);
    graphics.DrawRectangle(&panelPen, panel);

    std::ostringstream summary;
    summary
        << evidence.layoutId
        << " | " << evidence.verdict
        << " | scenes=" << evidence.sceneCount
        << " anims=" << evidence.animationCount;
    if (evidence.castDictionaryCount > 0 || evidence.subimageCount > 0)
    {
        summary
            << " casts=" << evidence.castDictionaryCount
            << " subimgs=" << evidence.subimageCount;
    }

    RECT titleRect{
        static_cast<LONG>(panel.X + 8.0F),
        static_cast<LONG>(panel.Y + 4.0F),
        static_cast<LONG>(panel.X + panel.Width - 8.0F),
        static_cast<LONG>(panel.Y + 22.0F),
    };
    drawTextLine(dc, titleRect, summary.str(), RGB(234, 247, 255));

    RECT sceneRect{
        static_cast<LONG>(panel.X + 8.0F),
        static_cast<LONG>(panel.Y + 26.0F),
        static_cast<LONG>(panel.X + panel.Width - 8.0F),
        static_cast<LONG>(panel.Y + 48.0F),
    };
    drawTextLine(dc, sceneRect, "Scenes: " + std::string(evidence.sceneCueSummary), RGB(193, 223, 240), DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);

    RECT animRect{
        static_cast<LONG>(panel.X + 8.0F),
        static_cast<LONG>(panel.Y + 50.0F),
        static_cast<LONG>(panel.X + panel.Width - 8.0F),
        static_cast<LONG>(panel.Y + 68.0F),
    };
    drawTextLine(dc, animRect, "Anim: " + std::string(evidence.animationCueSummary), RGB(247, 211, 72));

    RECT timelineRect{
        static_cast<LONG>(panel.X + 8.0F),
        static_cast<LONG>(panel.Y + 70.0F),
        static_cast<LONG>(panel.X + panel.Width - 8.0F),
        static_cast<LONG>(panel.Y + 86.0F),
    };
    drawTextLine(dc, timelineRect, "Longest: " + std::string(evidence.longestTimeline), RGB(207, 232, 245));

    const int frame = layoutTimelineFrame(evidence, timelineProgress);
    std::ostringstream frameSummary;
    frameSummary
        << "Frame: " << frame
        << "/" << evidence.longestTimelineFrames
        << " @ " << evidence.framesPerSecond
        << "fps";

    RECT frameRect{
        static_cast<LONG>(panel.X + 8.0F),
        static_cast<LONG>(panel.Y + 90.0F),
        static_cast<LONG>(panel.X + panel.Width - 8.0F),
        static_cast<LONG>(panel.Y + 106.0F),
    };
    drawTextLine(dc, frameRect, frameSummary.str(), RGB(247, 211, 72));

    Gdiplus::RectF timelineBar(panel.X + 8.0F, panel.Y + 108.0F, panel.Width - 16.0F, 5.0F);
    drawLayoutTimelineBar(graphics, timelineBar, timelineProgress);
}

[[nodiscard]] float clampUnit(float value)
{
    return std::max(0.0F, std::min(1.0F, value));
}

[[nodiscard]] float easedTimelineProgress(double elapsedSeconds, double durationSeconds)
{
    if (durationSeconds <= 0.0)
        return 1.0F;

    const float linear = clampUnit(static_cast<float>(elapsedSeconds / durationSeconds));
    const float inverse = 1.0F - linear;
    return 1.0F - (inverse * inverse * inverse);
}

[[nodiscard]] Gdiplus::RectF clampRectToCanvas(Gdiplus::RectF rect, const Gdiplus::RectF& canvas)
{
    const float maxWidth = std::max(1.0F, canvas.Width - 8.0F);
    const float maxHeight = std::max(1.0F, canvas.Height - 8.0F);
    rect.Width = std::min(rect.Width, maxWidth);
    rect.Height = std::min(rect.Height, maxHeight);

    const float minX = canvas.X + 4.0F;
    const float minY = canvas.Y + 4.0F;
    const float maxX = std::max(minX, canvas.X + canvas.Width - rect.Width - 4.0F);
    const float maxY = std::max(minY, canvas.Y + canvas.Height - rect.Height - 4.0F);
    rect.X = std::max(minX, std::min(rect.X, maxX));
    rect.Y = std::max(minY, std::min(rect.Y, maxY));
    return rect;
}

[[nodiscard]] PreviewMotion previewMotionForState(ScreenState state, std::string_view role, float progress, const Gdiplus::RectF& canvas)
{
    const float eased = clampUnit(progress);
    const float inverse = 1.0F - eased;

    switch (state)
    {
    case ScreenState::Intro:
        if (role == "logo")
            return PreviewMotion{ 0.0F, -canvas.Height * 0.1F * inverse, 0.2F + (0.8F * eased) };
        if (role == "content")
            return PreviewMotion{ 0.0F, canvas.Height * 0.08F * inverse, 0.28F + (0.72F * eased) };
        if (role == "chrome")
            return PreviewMotion{ 0.0F, -canvas.Height * 0.05F * inverse, 0.34F + (0.66F * eased) };
        if (role == "cinematic_frame")
            return PreviewMotion{ 0.0F, 0.0F, 0.45F + (0.55F * eased) };
        if (role == "counter")
            return PreviewMotion{ -canvas.Width * 0.16F * inverse, 0.0F, 0.28F + (0.72F * eased) };
        if (role == "gauge")
            return PreviewMotion{ 0.0F, canvas.Height * 0.12F * inverse, 0.32F + (0.68F * eased) };
        if (role == "sidecar")
            return PreviewMotion{ canvas.Width * 0.12F * inverse, 0.0F, 0.3F + (0.7F * eased) };
        return PreviewMotion{ 0.0F, 0.0F, 0.35F + (0.65F * eased) };
    case ScreenState::Navigate:
        if (role == "logo")
            return PreviewMotion{ 0.0F, -canvas.Height * 0.015F * inverse, 0.95F + (0.05F * eased) };
        if (role == "content" || role == "chrome")
            return PreviewMotion{ canvas.Width * 0.035F * inverse, 0.0F, 0.92F + (0.08F * eased) };
        if (role == "sidecar")
            return PreviewMotion{ canvas.Width * 0.04F * inverse, 0.0F, 1.0F };
        return PreviewMotion{ canvas.Width * 0.025F * inverse, 0.0F, 0.88F + (0.12F * eased) };
    case ScreenState::Confirm:
        if (role == "logo")
            return PreviewMotion{ 0.0F, -canvas.Height * 0.025F * inverse, 0.82F + (0.18F * eased) };
        if (role == "content" || role == "chrome")
            return PreviewMotion{ 0.0F, 0.0F, 0.76F + (0.24F * eased) };
        if (role == "transient_fx")
            return PreviewMotion{ 0.0F, -canvas.Height * 0.06F * inverse, 0.45F + (0.55F * eased) };
        return PreviewMotion{ 0.0F, 0.0F, 0.78F + (0.22F * eased) };
    case ScreenState::Cancel:
    case ScreenState::Outro:
        if (role == "logo")
            return PreviewMotion{ 0.0F, -canvas.Height * 0.08F * eased, 1.0F - (0.75F * eased) };
        if (role == "content" || role == "chrome")
            return PreviewMotion{ 0.0F, canvas.Height * 0.08F * eased, 1.0F - (0.7F * eased) };
        if (role == "counter")
            return PreviewMotion{ -canvas.Width * 0.18F * eased, 0.0F, 1.0F - (0.72F * eased) };
        if (role == "gauge")
            return PreviewMotion{ 0.0F, canvas.Height * 0.14F * eased, 1.0F - (0.68F * eased) };
        if (role == "sidecar")
            return PreviewMotion{ canvas.Width * 0.14F * eased, 0.0F, 1.0F - (0.7F * eased) };
        return PreviewMotion{ 0.0F, 0.0F, 1.0F - (0.65F * eased) };
    case ScreenState::Boot:
        return PreviewMotion{ 0.0F, 0.0F, 0.25F };
    case ScreenState::Idle:
    case ScreenState::Closed:
        return PreviewMotion{};
    }

    return PreviewMotion{};
}

[[nodiscard]] Gdiplus::RectF applyPreviewMotion(Gdiplus::RectF rect, const PreviewMotion& motion, const Gdiplus::RectF& canvas)
{
    rect.X += motion.offsetX;
    rect.Y += motion.offsetY;
    return clampRectToCanvas(rect, canvas);
}

[[nodiscard]] BYTE motionAlphaByte(float baseAlpha, const PreviewMotion& motion)
{
    const float alpha = clampUnit(baseAlpha * clampUnit(motion.alpha));
    return static_cast<BYTE>(std::round(alpha * 255.0F));
}

[[nodiscard]] float previewLayerFillAlpha(std::string_view role)
{
    if (role == "backdrop" || role == "cinematic_frame")
        return 0.0F;
    return 0.58F;
}

[[nodiscard]] Gdiplus::RectF layoutLayerRect(const OverlayLayer& layer, std::size_t roleIndex, const Gdiplus::RectF& canvas)
{
    if (layer.role == "counter")
    {
        const float width = std::min(250.0F, canvas.Width * 0.38F);
        return clampRectToCanvas(
            Gdiplus::RectF(canvas.X + 18.0F, canvas.Y + 16.0F + (static_cast<float>(roleIndex) * 22.0F), width, 17.0F),
            canvas);
    }

    if (layer.role == "gauge")
    {
        const float width = std::min(300.0F, canvas.Width * 0.48F);
        return clampRectToCanvas(
            Gdiplus::RectF(canvas.X + 18.0F, canvas.Y + canvas.Height - 88.0F + (static_cast<float>(roleIndex) * 22.0F), width, 17.0F),
            canvas);
    }

    if (layer.role == "sidecar")
    {
        const float width = std::min(210.0F, canvas.Width * 0.30F);
        return clampRectToCanvas(
            Gdiplus::RectF(canvas.X + canvas.Width - width - 18.0F, canvas.Y + 22.0F + (static_cast<float>(roleIndex) * 34.0F), width, 26.0F),
            canvas);
    }

    if (layer.role == "transient_fx")
    {
        const float width = std::min(260.0F, canvas.Width * 0.42F);
        return clampRectToCanvas(
            Gdiplus::RectF(canvas.X + ((canvas.Width - width) / 2.0F), canvas.Y + 22.0F + (static_cast<float>(roleIndex) * 30.0F), width, 24.0F),
            canvas);
    }

    if (layer.role == "prompt")
    {
        const float width = std::min(360.0F, canvas.Width - 32.0F);
        return clampRectToCanvas(
            Gdiplus::RectF(canvas.X + 16.0F, canvas.Y + canvas.Height - 38.0F - (static_cast<float>(roleIndex) * 28.0F), width, 22.0F),
            canvas);
    }

    const float width = std::min(280.0F, canvas.Width * 0.44F);
    return clampRectToCanvas(
        Gdiplus::RectF(canvas.X + 20.0F, canvas.Y + 20.0F + (static_cast<float>(roleIndex) * 24.0F), width, 20.0F),
        canvas);
}

[[nodiscard]] Gdiplus::RectF layoutTitleMenuLayerRect(const OverlayLayer& layer, std::size_t roleIndex, const Gdiplus::RectF& canvas)
{
    if (layer.role == "backdrop")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + 6.0F, canvas.Y + 6.0F, canvas.Width - 12.0F, canvas.Height - 12.0F), canvas);
    if (layer.role == "logo")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.1F), canvas.Y + (canvas.Height * 0.09F), canvas.Width * 0.5F, canvas.Height * 0.19F), canvas);
    if (layer.role == "content")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.2F), canvas.Y + (canvas.Height * 0.46F), canvas.Width * 0.6F, canvas.Height * 0.24F), canvas);
    if (layer.role == "transient_fx")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.08F), canvas.Y + (canvas.Height * 0.06F), canvas.Width * 0.56F, canvas.Height * 0.27F), canvas);
    if (layer.role == "prompt")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.18F), canvas.Y + (canvas.Height * 0.78F), canvas.Width * 0.64F, 24.0F + (static_cast<float>(roleIndex) * 2.0F)), canvas);
    return layoutLayerRect(layer, roleIndex, canvas);
}

[[nodiscard]] Gdiplus::RectF layoutPauseMenuLayerRect(const OverlayLayer& layer, std::size_t roleIndex, const Gdiplus::RectF& canvas)
{
    if (layer.role == "backdrop")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + 8.0F, canvas.Y + 8.0F, canvas.Width - 16.0F, canvas.Height - 16.0F), canvas);
    if (layer.role == "chrome")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.17F), canvas.Y + (canvas.Height * 0.16F), canvas.Width * 0.66F, canvas.Height * 0.58F), canvas);
    if (layer.role == "content")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.25F), canvas.Y + (canvas.Height * 0.28F), canvas.Width * 0.5F, canvas.Height * 0.34F), canvas);
    if (layer.role == "transient_fx")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.2F), canvas.Y + (canvas.Height * 0.18F), canvas.Width * 0.6F, canvas.Height * 0.12F), canvas);
    if (layer.role == "prompt")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.22F), canvas.Y + (canvas.Height * 0.78F), canvas.Width * 0.56F, 24.0F + (static_cast<float>(roleIndex) * 2.0F)), canvas);
    return layoutLayerRect(layer, roleIndex, canvas);
}

[[nodiscard]] Gdiplus::RectF layoutLoadingLayerRect(const OverlayLayer& layer, std::size_t roleIndex, const Gdiplus::RectF& canvas)
{
    if (layer.role == "backdrop")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + 4.0F, canvas.Y + 4.0F, canvas.Width - 8.0F, canvas.Height - 8.0F), canvas);
    if (layer.role == "cinematic_frame")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + 6.0F, canvas.Y + (canvas.Height * 0.09F), canvas.Width - 12.0F, canvas.Height * 0.82F), canvas);
    if (layer.role == "content")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.21F), canvas.Y + (canvas.Height * 0.26F), canvas.Width * 0.58F, canvas.Height * 0.3F), canvas);
    if (layer.role == "tip_copy")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.24F), canvas.Y + (canvas.Height * 0.62F), canvas.Width * 0.52F, canvas.Height * 0.12F), canvas);
    if (layer.role == "controller_variant")
        return clampRectToCanvas(Gdiplus::RectF(canvas.X + (canvas.Width * 0.32F), canvas.Y + (canvas.Height * 0.77F), canvas.Width * 0.36F, canvas.Height * 0.08F), canvas);
    return layoutLayerRect(layer, roleIndex, canvas);
}

[[nodiscard]] Gdiplus::RectF layoutFamilyLayerRect(PreviewFamily family, const OverlayLayer& layer, std::size_t roleIndex, const Gdiplus::RectF& canvas)
{
    switch (family)
    {
    case PreviewFamily::TitleMenu:
        return layoutTitleMenuLayerRect(layer, roleIndex, canvas);
    case PreviewFamily::PauseMenu:
        return layoutPauseMenuLayerRect(layer, roleIndex, canvas);
    case PreviewFamily::LoadingTransition:
        return layoutLoadingLayerRect(layer, roleIndex, canvas);
    case PreviewFamily::Generic:
        return layoutLayerRect(layer, roleIndex, canvas);
    }

    return layoutLayerRect(layer, roleIndex, canvas);
}

[[nodiscard]] std::string snapshotText(const ScreenRuntime& runtime, const ContractEntry& entry, const DebugWorkbenchHostEntry& host)
{
    std::ostringstream text;
    text
        << "Host group: " << host.groupDisplayName << " [" << host.priority << "]\r\n"
        << "Host path: " << host.relativeSourcePath << "\r\n"
        << "Primary contract: " << host.primaryContractFileName << "\r\n"
        << "Contract source: " << entry.path.string() << "\r\n"
        << "Screen id: " << entry.contract.screenId << "\r\n"
        << "Local atlas candidate: ";

    if (const auto* candidate = atlasCandidateForContract(host.primaryContractFileName))
        text << atlasCandidateDisplayName(*candidate);
    else
        text << "none";

    text
        << "\r\n"
        << visualParitySummaryText(host.primaryContractFileName)
        << "\r\n"
        << assetViewerSummaryText(host.primaryContractFileName)
        << "\r\n"
        << layoutCsdElementBindingNavigationSummary(host.primaryContractFileName, 0)
        << "\r\n"
        << layoutCsdElementBindingSummary(host.primaryContractFileName)
        << "\r\n"
        << "State: " << toString(runtime.state()) << "\r\n"
        << "Input locked: " << (runtime.isInputLocked() ? "yes" : "no") << "\r\n\r\n"
        << "Visible layers:\r\n";

    const auto layers = runtime.visibleLayers();
    if (layers.empty())
        text << "  none\r\n";
    for (const auto& layer : layers)
        text << "  " << layer.id << " : " << layer.role << "\r\n";

    text << "\r\nVisible prompts:\r\n";
    const auto prompts = runtime.visiblePrompts();
    if (prompts.empty())
        text << "  none\r\n";
    for (const auto& prompt : prompts)
        text << "  [" << toString(prompt.button) << "] " << prompt.label << "\r\n";

    const double stateDuration = timelineDuration(entry.contract, runtime.state());
    const float primitiveProgress = stateDuration > 0.0
        ? static_cast<float>(std::min(1.0, runtime.stateElapsedSeconds() / stateDuration))
        : (runtime.state() == ScreenState::Idle ? 1.0F : 0.0F);
    text << "\r\n" << layoutPrimitiveCueSummary(host.primaryContractFileName, primitiveProgress);
    text << "\r\n" << layoutPrimitiveChannelSampleSummary(host.primaryContractFileName, primitiveProgress);
    text << "\r\n" << layoutPrimitiveDrawCommandSummary(host.primaryContractFileName, primitiveProgress, 1280, 720);
    text << "\r\n" << layoutAuthoredCastTransformSummary(host.primaryContractFileName);
    text << "\r\n" << layoutAuthoredKeyframeCurveSummary(host.primaryContractFileName);
    text << "\r\n" << layoutAuthoredKeyframeSampleSummary(host.primaryContractFileName);
    text << "\r\n" << layoutAuthoredSampledTransformSummary(host.primaryContractFileName);
    text << "\r\n" << layoutAuthoredSampledDrawCommandSummary(host.primaryContractFileName);

    text << "\r\nNotes:\r\n" << host.notes << "\r\n";
    return text.str();
}

[[nodiscard]] int runSmoke()
{
    const auto contracts = loadBundledEntries();
    const auto hosts = resolveHostEntries(contracts);
    const auto groups = buildGroups(hosts);
    const auto supportHosts = std::count_if(
        hosts.begin(),
        hosts.end(),
        [](const ResolvedHostEntry& host)
        {
            return host.metadata && host.metadata->groupId == std::string_view("support_substrate_hosts");
        });

    std::cout
        << "sward_ui_runtime_debug_gui smoke ok "
        << "contracts=" << contracts.size()
        << " hosts=" << hosts.size()
        << " groups=" << groups.size()
        << " support_hosts=" << supportHosts
        << '\n';
    return 0;
}

[[nodiscard]] int runPreviewSmoke()
{
    std::size_t existingAtlasSheets = 0;
    std::size_t proxyCandidates = 0;
    for (const auto& candidate : kPreviewAtlasCandidates)
    {
        if (isProxyAtlasCandidate(candidate))
            ++proxyCandidates;
        if (std::filesystem::exists(visualAtlasSheetRoot() / std::string(candidate.atlasFileName)))
            ++existingAtlasSheets;
    }

    const auto* title = atlasCandidateByShortName("title");
    const auto* pause = atlasCandidateByShortName("pause");
    const auto* sonicStage = atlasCandidateByShortName("sonic_stage");
    std::cout
        << "sward_ui_runtime_debug_gui preview smoke ok "
        << "atlas_candidates=" << kPreviewAtlasCandidates.size()
        << " proxy_candidates=" << proxyCandidates
        << " existing_local_atlas=" << existingAtlasSheets
        << " title=" << (title ? title->atlasFileName : "none")
        << " pause=" << (pause ? pause->atlasFileName : "none")
        << " sonic_stage=" << (sonicStage ? sonicStage->atlasFileName : "none")
        << '\n';
    return 0;
}

[[nodiscard]] int runAssetViewSmoke()
{
    const auto* title = atlasCandidateByShortName("title");
    const auto* loading = atlasCandidateByShortName("loading");
    const auto* sonic = atlasCandidateByShortName("sonic_stage");
    if (!title || !loading || !sonic)
    {
        std::cerr << "sward_ui_runtime_debug_gui asset view smoke failed missing atlas candidates\n";
        return 1;
    }

    const std::string titleAsset = assetViewerAtlasDescriptor(*title);
    const std::string loadingAsset = assetViewerAtlasDescriptor(*loading);
    const std::string sonicAsset = assetViewerAtlasDescriptor(*sonic);
    const int sheetFiles = visualAtlasSheetFileCount();

    std::cout
        << "sward_ui_runtime_debug_gui asset view smoke ok "
        << "atlas_candidates=" << kPreviewAtlasCandidates.size()
        << " sheet_files=" << sheetFiles
        << " title_asset=" << titleAsset
        << " loading_asset=" << loadingAsset
        << " sonic_asset=" << sonicAsset
        << '\n';

    return sheetFiles == 22
        && titleAsset == "mainmenu__ui_mainmenu.png:exact:exists=1"
        && loadingAsset == "loading__ui_loading.png:exact:exists=1"
        && sonicAsset == "exstagetails_common__ui_prov_playscreen.png:proxy:exists=1"
        ? 0
        : 1;
}

[[nodiscard]] int runAssetGallerySmoke()
{
    const auto files = visualAtlasSheetFiles();
    const auto* sonic = atlasCandidateByShortName("sonic_stage");
    const auto selected = sonic ? visualAtlasSheetIndexByFileName(sonic->atlasFileName) : std::nullopt;
    const std::size_t selectedIndex = selected.value_or(0);
    const std::size_t previousIndex = files.empty() ? 0 : (selectedIndex + files.size() - 1) % files.size();
    const std::size_t nextIndex = files.empty() ? 0 : (selectedIndex + 1) % files.size();

    const std::string firstName = files.empty() ? "none" : files.front().filename().string();
    const std::string selectedName = files.empty() ? "none" : files[selectedIndex].filename().string();
    const std::string previousName = files.empty() ? "none" : files[previousIndex].filename().string();
    const std::string nextName = files.empty() ? "none" : files[nextIndex].filename().string();

    std::cout
        << "sward_ui_runtime_debug_gui asset gallery smoke ok "
        << "sheet_files=" << files.size()
        << " first=" << firstName
        << " selected=" << selectedName
        << " previous=" << previousName
        << " next=" << nextName
        << '\n';

    return files.size() == 22
        && firstName == "actioncommon__ui_gate.png"
        && selectedName == "exstagetails_common__ui_prov_playscreen.png"
        && previousName == "exstagetails_common__ui_exstage.png"
        && nextName == "exstagetails_common__ui_qte.png"
        ? 0
        : 1;
}

[[nodiscard]] int runAssetCsdBindingSmoke()
{
    const auto sonicBindings = layoutCsdElementBindingsForContract("sonic_stage_hud_reference.json");
    const auto loadingBindings = layoutCsdElementBindingsForContract("loading_transition_reference.json");
    const auto pauseBindings = layoutCsdElementBindingsForContract("pause_menu_reference.json");
    const auto titleBindings = layoutCsdElementBindingsForContract("title_menu_reference.json");

    if (sonicBindings.empty() || loadingBindings.empty() || pauseBindings.empty() || titleBindings.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui asset csd binding smoke failed missing binding\n";
        return 1;
    }

    const auto* sonic = sonicBindings.front();
    const auto* loading = loadingBindings.front();
    const auto* pause = pauseBindings.front();
    const auto* title = titleBindings.front();

    std::cout
        << "sward_ui_runtime_debug_gui asset csd binding smoke ok "
        << "bindings=" << kLayoutCsdElementBindings.size()
        << " sonic=" << layoutCsdElementBindingDescriptor(*sonic)
        << " loading=" << layoutCsdElementBindingDescriptor(*loading)
        << " pause=" << layoutCsdElementBindingDescriptor(*pause)
        << " title=" << layoutCsdElementBindingDescriptor(*title)
        << '\n';

    return sonic->sceneCastCount == 47
        && sonic->sceneSubimageCount == 109
        && loading->sceneCastCount == 28
        && loading->sceneSubimageCount == 320
        && pause->sceneCastCount == 1
        && pause->sceneSubimageCount == 99
        && title->sceneCastCount == 47
        && title->sceneSubimageCount == 46
        ? 0
        : 1;
}

[[nodiscard]] int runAssetCsdNavigationSmoke()
{
    const auto sonicBindings = layoutCsdElementBindingsForContract("sonic_stage_hud_reference.json");
    const auto loadingBindings = layoutCsdElementBindingsForContract("loading_transition_reference.json");
    const auto titleBindings = layoutCsdElementBindingsForContract("title_menu_reference.json");
    const auto pauseBindings = layoutCsdElementBindingsForContract("pause_menu_reference.json");
    if (sonicBindings.empty() || loadingBindings.empty() || titleBindings.empty() || pauseBindings.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui asset csd navigation smoke failed missing binding set\n";
        return 1;
    }

    const auto* selected = sonicBindings[selectedCsdElementBindingIndex(0, sonicBindings.size())];
    const auto* next = sonicBindings[selectedCsdElementBindingIndex(1, sonicBindings.size())];
    const auto* previous = sonicBindings[selectedCsdElementBindingIndex(sonicBindings.size() - 1, sonicBindings.size())];

    std::cout
        << "sward_ui_runtime_debug_gui asset csd navigation smoke ok "
        << "sonic_count=" << sonicBindings.size()
        << " selected=" << layoutCsdElementBindingDescriptor(*selected)
        << " next=" << layoutCsdElementBindingDescriptor(*next)
        << " previous=" << layoutCsdElementBindingDescriptor(*previous)
        << " loading_count=" << loadingBindings.size()
        << " title_count=" << titleBindings.size()
        << " pause_count=" << pauseBindings.size()
        << '\n';

    return sonicBindings.size() == 6
        && loadingBindings.size() == 7
        && titleBindings.size() == 8
        && pauseBindings.size() == 8
        && selected->sceneName == "so_speed_gauge"
        && next->sceneName == "so_ringenagy_gauge"
        && previous->sceneName == "ring_get_effect"
        ? 0
        : 1;
}

[[nodiscard]] int runPlaybackSmoke()
{
    const auto contracts = loadBundledEntries();
    const auto found = std::find_if(
        contracts.begin(),
        contracts.end(),
        [](const ContractEntry& entry)
        {
            return entry.path.filename() == "sonic_stage_hud_reference.json";
        });
    if (found == contracts.end())
    {
        std::cerr << "sward_ui_runtime_debug_gui playback smoke failed missing sonic_stage_hud_reference.json\n";
        return 1;
    }

    ScreenRuntime runtime(found->contract);
    enableAllPromptPredicates(runtime, found->contract);
    runtime.dispatch(RuntimeEventType::ResourcesReady);
    const std::string intro(toString(runtime.state()));
    runtime.tick(0.12);
    runtime.tick(1.0);
    const std::string afterIntro(toString(runtime.state()));
    runtime.requestAction(InputAction::MoveNext);
    const std::string action(toString(runtime.state()));
    runtime.tick(1.0);
    const std::string afterAction(toString(runtime.state()));

    std::cout
        << "sward_ui_runtime_debug_gui playback smoke ok "
        << "intro=" << intro
        << " after_intro=" << afterIntro
        << " action=" << action
        << " after_action=" << afterAction
        << '\n';
    return afterIntro == "Idle" && action == "Navigate" && afterAction == "Idle" ? 0 : 1;
}

[[nodiscard]] int runMotionSmoke()
{
    const Gdiplus::RectF canvas(0.0F, 0.0F, 1280.0F, 720.0F);
    const float introStartProgress = easedTimelineProgress(0.0, 0.35);
    const float introEndProgress = easedTimelineProgress(0.35, 0.35);
    const PreviewMotion introStart = previewMotionForState(ScreenState::Intro, "counter", introStartProgress, canvas);
    const PreviewMotion introEnd = previewMotionForState(ScreenState::Intro, "counter", introEndProgress, canvas);
    const PreviewMotion idle = previewMotionForState(ScreenState::Idle, "counter", 1.0F, canvas);
    const PreviewMotion outro = previewMotionForState(ScreenState::Outro, "gauge", 1.0F, canvas);

    std::cout
        << "sward_ui_runtime_debug_gui motion smoke ok "
        << "intro_alpha=" << introStart.alpha
        << " intro_offset_x=" << introStart.offsetX
        << " intro_end_alpha=" << introEnd.alpha
        << " idle_alpha=" << idle.alpha
        << " outro_alpha=" << outro.alpha
        << '\n';

    const bool introMovesIn = introStart.offsetX < introEnd.offsetX && introStart.alpha < introEnd.alpha;
    const bool idleStable = idle.alpha == 1.0F && idle.offsetX == 0.0F && idle.offsetY == 0.0F;
    const bool outroFades = outro.alpha < 1.0F && outro.offsetY > 0.0F;
    return introMovesIn && idleStable && outroFades ? 0 : 1;
}

[[nodiscard]] int runFamilyPreviewSmoke()
{
    const Gdiplus::RectF canvas(0.0F, 0.0F, 1280.0F, 720.0F);
    const OverlayLayer titleLogo{ "logo", "logo", false };
    const OverlayLayer titleContent{ "carousel", "content", true };
    const OverlayLayer pauseChrome{ "chrome", "chrome", false };
    const OverlayLayer pauseContent{ "content", "content", true };
    const OverlayLayer loadingFrame{ "pda_frame", "content", false };

    const PreviewFamily titleFamily = previewFamilyForContract("title_menu_reference.json");
    const PreviewFamily pauseFamily = previewFamilyForContract("pause_menu_reference.json");
    const PreviewFamily loadingFamily = previewFamilyForContract("loading_transition_reference.json");
    const Gdiplus::RectF titleLogoRect = layoutFamilyLayerRect(titleFamily, titleLogo, 0, canvas);
    const Gdiplus::RectF titleContentRect = layoutFamilyLayerRect(titleFamily, titleContent, 0, canvas);
    const Gdiplus::RectF pauseChromeRect = layoutFamilyLayerRect(pauseFamily, pauseChrome, 0, canvas);
    const Gdiplus::RectF pauseContentRect = layoutFamilyLayerRect(pauseFamily, pauseContent, 0, canvas);
    const Gdiplus::RectF loadingFrameRect = layoutFamilyLayerRect(loadingFamily, loadingFrame, 0, canvas);

    std::cout
        << "sward_ui_runtime_debug_gui family preview smoke ok "
        << "title=" << previewFamilyName(titleFamily)
        << " pause=" << previewFamilyName(pauseFamily)
        << " loading=" << previewFamilyName(loadingFamily)
        << " title_logo_y=" << titleLogoRect.Y
        << " title_content_y=" << titleContentRect.Y
        << " pause_chrome_w=" << pauseChromeRect.Width
        << " loading_frame_y=" << loadingFrameRect.Y
        << '\n';

    const bool titleSeparated = titleFamily == PreviewFamily::TitleMenu && titleLogoRect.Y < titleContentRect.Y;
    const bool pauseFramed = pauseFamily == PreviewFamily::PauseMenu && pauseChromeRect.Width > pauseContentRect.Width;
    const bool loadingCentered = loadingFamily == PreviewFamily::LoadingTransition && loadingFrameRect.Y > canvas.Y;
    return titleSeparated && pauseFramed && loadingCentered ? 0 : 1;
}

[[nodiscard]] int runLayoutEvidenceSmoke()
{
    const auto* title = layoutEvidenceForContract("title_menu_reference.json");
    const auto* pause = layoutEvidenceForContract("pause_menu_reference.json");
    const auto* loading = layoutEvidenceForContract("loading_transition_reference.json");
    if (!title || !pause || !loading)
    {
        std::cerr << "sward_ui_runtime_debug_gui layout evidence smoke failed missing evidence entry\n";
        return 1;
    }

    std::cout
        << "sward_ui_runtime_debug_gui layout evidence smoke ok "
        << "title=" << title->layoutId
        << " scenes=" << title->sceneCount
        << " animations=" << title->animationCount
        << " pause=" << pause->layoutId
        << " scenes=" << pause->sceneCount
        << " animations=" << pause->animationCount
        << " loading=" << loading->layoutId
        << " scenes=" << loading->sceneCount
        << " animations=" << loading->animationCount
        << '\n';

    const bool titleMatches = title->layoutId == "ui_mainmenu" && title->sceneCount == 16 && title->animationCount == 6;
    const bool pauseMatches = pause->layoutId == "ui_pause" && pause->sceneCount == 29 && pause->animationCount == 41;
    const bool loadingMatches = loading->layoutId == "ui_loading" && loading->sceneCount == 7 && loading->animationCount == 37;
    return titleMatches && pauseMatches && loadingMatches ? 0 : 1;
}

[[nodiscard]] int runLayoutTimelineSmoke()
{
    const auto* title = layoutEvidenceForContract("title_menu_reference.json");
    const auto* pause = layoutEvidenceForContract("pause_menu_reference.json");
    const auto* loading = layoutEvidenceForContract("loading_transition_reference.json");
    if (!title || !pause || !loading)
    {
        std::cerr << "sward_ui_runtime_debug_gui layout timeline smoke failed missing evidence entry\n";
        return 1;
    }

    const int titleFrame = layoutTimelineFrame(*title, 0.5F);
    const int pauseFrame = layoutTimelineFrame(*pause, 0.5F);
    const int loadingFrame = layoutTimelineFrame(*loading, 0.75F);

    std::cout
        << "sward_ui_runtime_debug_gui layout timeline smoke ok "
        << "title_frame=" << titleFrame << "/" << title->longestTimelineFrames
        << " pause_frame=" << pauseFrame << "/" << pause->longestTimelineFrames
        << " loading_frame=" << loadingFrame << "/" << loading->longestTimelineFrames
        << " fps=" << title->framesPerSecond
        << '\n';

    const bool titleMatches = titleFrame == 110 && title->longestTimelineFrames == 220;
    const bool pauseMatches = pauseFrame == 120 && pause->longestTimelineFrames == 240;
    const bool loadingMatches = loadingFrame == 180 && loading->longestTimelineFrames == 240;
    return titleMatches && pauseMatches && loadingMatches && title->framesPerSecond == 60 ? 0 : 1;
}

[[nodiscard]] int runLayoutPrimitiveSmoke()
{
    const auto title = layoutScenePrimitivesForContract("title_menu_reference.json");
    const auto pause = layoutScenePrimitivesForContract("pause_menu_reference.json");
    const auto loading = layoutScenePrimitivesForContract("loading_transition_reference.json");
    const auto sonicStage = layoutScenePrimitivesForContract("sonic_stage_hud_reference.json");
    const auto werehogStage = layoutScenePrimitivesForContract("werehog_stage_hud_reference.json");
    const auto extraStage = layoutScenePrimitivesForContract("extra_stage_hud_reference.json");

    const int titleKeyframes = layoutScenePrimitiveKeyframeTotal(title);
    const int pauseKeyframes = layoutScenePrimitiveKeyframeTotal(pause);
    const int loadingKeyframes = layoutScenePrimitiveKeyframeTotal(loading);
    const int sonicStageKeyframes = layoutScenePrimitiveKeyframeTotal(sonicStage);
    const int werehogStageKeyframes = layoutScenePrimitiveKeyframeTotal(werehogStage);
    const int extraStageKeyframes = layoutScenePrimitiveKeyframeTotal(extraStage);

    std::cout
        << "sward_ui_runtime_debug_gui layout primitive smoke ok "
        << "title_primitives=" << title.size()
        << " keyframes=" << titleKeyframes
        << " pause_primitives=" << pause.size()
        << " keyframes=" << pauseKeyframes
        << " loading_primitives=" << loading.size()
        << " keyframes=" << loadingKeyframes
        << " sonic_stage_primitives=" << sonicStage.size()
        << " keyframes=" << sonicStageKeyframes
        << " werehog_stage_primitives=" << werehogStage.size()
        << " keyframes=" << werehogStageKeyframes
        << " extra_stage_primitives=" << extraStage.size()
        << " keyframes=" << extraStageKeyframes
        << " sonic_speed_gauge_kf=" << layoutScenePrimitiveKeyframesForScene(sonicStage, "so_speed_gauge")
        << " sonic_ring_energy_gauge_kf=" << layoutScenePrimitiveKeyframesForScene(sonicStage, "so_ringenagy_gauge")
        << " sonic_ring_get_effect_kf=" << layoutScenePrimitiveKeyframesForScene(sonicStage, "ring_get_effect")
        << " sonic_bg_kf=" << layoutScenePrimitiveKeyframesForScene(sonicStage, "bg")
        << '\n';

    const bool titleMatches = title.size() == 6 && titleKeyframes == 806;
    const bool pauseMatches = pause.size() == 6 && pauseKeyframes == 806;
    const bool loadingMatches = loading.size() == 6 && loadingKeyframes == 2775;
    const bool sonicStageMatches = sonicStage.size() == 6 && sonicStageKeyframes == 680;
    const bool werehogStageMatches = werehogStage.size() == 6 && werehogStageKeyframes == 680;
    const bool extraStageMatches = extraStage.size() == 6 && extraStageKeyframes == 680;
    const bool sonicOwnershipMatches =
        layoutScenePrimitiveKeyframesForScene(sonicStage, "so_speed_gauge") == 360
        && layoutScenePrimitiveKeyframesForScene(sonicStage, "so_ringenagy_gauge") == 240
        && layoutScenePrimitiveKeyframesForScene(sonicStage, "ring_get_effect") == 14
        && layoutScenePrimitiveKeyframesForScene(sonicStage, "bg") == 0;
    return titleMatches && pauseMatches && loadingMatches && sonicStageMatches && werehogStageMatches && extraStageMatches && sonicOwnershipMatches ? 0 : 1;
}

[[nodiscard]] int runLayoutPrimitivePlaybackSmoke()
{
    const auto sonicStage = layoutScenePrimitivesForContract("sonic_stage_hud_reference.json");
    const auto* speedGauge = layoutScenePrimitiveForScene(sonicStage, "so_speed_gauge");
    const auto* ringEnergyGauge = layoutScenePrimitiveForScene(sonicStage, "so_ringenagy_gauge");
    const auto* infoPanel = layoutScenePrimitiveForScene(sonicStage, "info_1");
    const auto* ringGetEffect = layoutScenePrimitiveForScene(sonicStage, "ring_get_effect");
    const auto* background = layoutScenePrimitiveForScene(sonicStage, "bg");
    if (!speedGauge || !ringEnergyGauge || !infoPanel || !ringGetEffect || !background)
    {
        std::cerr << "sward_ui_runtime_debug_gui layout primitive playback smoke failed missing Sonic HUD primitive\n";
        return 1;
    }

    constexpr float kSampleProgress = 0.5F;
    const int speedFrame = layoutScenePrimitiveFrame(*speedGauge, kSampleProgress);
    const int ringEnergyFrame = layoutScenePrimitiveFrame(*ringEnergyGauge, kSampleProgress);
    const int infoFrame = layoutScenePrimitiveFrame(*infoPanel, kSampleProgress);
    const int ringGetFrame = layoutScenePrimitiveFrame(*ringGetEffect, kSampleProgress);
    const int backgroundFrame = layoutScenePrimitiveFrame(*background, kSampleProgress);

    std::cout
        << "sward_ui_runtime_debug_gui layout primitive playback smoke ok "
        << "speed_anim=" << speedGauge->animationName
        << " speed_frame=" << speedFrame << "/" << speedGauge->frameCount
        << " energy_anim=" << ringEnergyGauge->animationName
        << " energy_frame=" << ringEnergyFrame << "/" << ringEnergyGauge->frameCount
        << " info_anim=" << infoPanel->animationName
        << " info_frame=" << infoFrame << "/" << infoPanel->frameCount
        << " ring_fx_anim=" << ringGetEffect->animationName
        << " ring_fx_frame=" << ringGetFrame << "/" << ringGetEffect->frameCount
        << " bg_anim=" << background->animationName
        << " bg_frame=" << backgroundFrame << "/" << background->frameCount
        << '\n';

    const bool animationsMatch =
        speedGauge->animationName == "Size_Anim"
        && ringEnergyGauge->animationName == "Size_Anim"
        && infoPanel->animationName == "Count_Anim"
        && ringGetEffect->animationName == "Intro_Anim"
        && background->animationName == "DefaultAnim";
    const bool framesMatch =
        speedFrame == 50
        && ringEnergyFrame == 50
        && infoFrame == 50
        && ringGetFrame == 3
        && backgroundFrame == 50;
    return animationsMatch && framesMatch ? 0 : 1;
}

[[nodiscard]] int runLayoutPrimitiveDetailSmoke()
{
    constexpr float kSampleProgress = 0.5F;
    const std::string summary = layoutPrimitiveCueSummary("sonic_stage_hud_reference.json", kSampleProgress);

    const bool hasHeader = summary.find("Layout primitive cues:") != std::string::npos;
    const bool hasAggregate = summary.find("primitives=6 keyframes=680") != std::string::npos;
    const bool hasSpeedGauge = summary.find("so_speed_gauge / Size_Anim : frame 50/100") != std::string::npos;
    const bool hasRingEffect = summary.find("ring_get_effect / Intro_Anim : frame 3/5") != std::string::npos;

    std::cout
        << "sward_ui_runtime_debug_gui layout primitive detail smoke ok "
        << "primitives=6 keyframes=680 "
        << "speed=so_speed_gauge/Size_Anim frame=50/100 "
        << "ring_fx=ring_get_effect/Intro_Anim frame=3/5"
        << '\n';

    return hasHeader && hasAggregate && hasSpeedGauge && hasRingEffect ? 0 : 1;
}

[[nodiscard]] int runLayoutPrimitiveChannelSmoke()
{
    const auto sonicStage = layoutScenePrimitivesForContract("sonic_stage_hud_reference.json");
    const auto* speedGauge = layoutScenePrimitiveForScene(sonicStage, "so_speed_gauge");
    const auto* infoPanel2 = layoutScenePrimitiveForScene(sonicStage, "info_2");
    const auto* background = layoutScenePrimitiveForScene(sonicStage, "bg");
    if (!speedGauge || !infoPanel2 || !background)
    {
        std::cerr << "sward_ui_runtime_debug_gui layout primitive channel smoke failed missing Sonic HUD primitive\n";
        return 1;
    }

    int transformCount = 0;
    int colorCount = 0;
    int visibilityCount = 0;
    int staticCount = 0;
    for (const auto* primitive : sonicStage)
    {
        const PrimitiveChannelMask mask = layoutPrimitiveChannelMask(*primitive);
        if (mask.transform)
            ++transformCount;
        if (mask.color)
            ++colorCount;
        if (mask.visibility)
            ++visibilityCount;
        if (!hasAnyChannel(mask))
            ++staticCount;
    }

    const std::string speedChannels = layoutPrimitiveChannelTags(*speedGauge);
    const std::string info2Channels = layoutPrimitiveChannelTags(*infoPanel2);
    const std::string bgChannels = layoutPrimitiveChannelTags(*background);
    std::cout
        << "sward_ui_runtime_debug_gui layout primitive channel smoke ok "
        << "sonic_transform=" << transformCount
        << " sonic_color=" << colorCount
        << " sonic_visibility=" << visibilityCount
        << " sonic_static=" << staticCount
        << " speed_channels=" << speedChannels
        << " info2_channels=" << info2Channels
        << " bg_channels=" << bgChannels
        << '\n';

    return transformCount == 3
        && colorCount == 4
        && visibilityCount == 2
        && staticCount == 1
        && speedChannels == "color+transform"
        && info2Channels == "visibility"
        && bgChannels == "static"
        ? 0
        : 1;
}

[[nodiscard]] int runLayoutPrimitiveChannelLegendSmoke()
{
    const auto sonicStage = layoutScenePrimitivesForContract("sonic_stage_hud_reference.json");
    const PrimitiveChannelCounts counts = layoutPrimitiveChannelCounts(sonicStage);
    const std::string label = layoutPrimitiveChannelLegendLabel(counts);

    std::cout
        << "sward_ui_runtime_debug_gui layout primitive channel legend smoke ok "
        << "legend=T" << counts.transform
        << " C" << counts.color
        << " V" << counts.visibility
        << " S" << counts.sprite
        << " static" << counts.staticPrimitive
        << " label=" << label
        << '\n';

    return counts.transform == 3
        && counts.color == 4
        && counts.visibility == 2
        && counts.sprite == 0
        && counts.staticPrimitive == 1
        && label == "Channels T3 C4 V2 S0 static1"
        ? 0
        : 1;
}

[[nodiscard]] int runVisualParitySmoke()
{
    const VisualParitySummary sonic = visualParitySummaryForContract("sonic_stage_hud_reference.json");
    const VisualParitySummary title = visualParitySummaryForContract("title_menu_reference.json");
    const std::string sonicChannels = visualParityChannelToken(sonic.channels);

    std::cout
        << "sward_ui_runtime_debug_gui visual parity smoke ok "
        << "sonic_atlas=" << sonic.atlasBinding
        << " sonic_layout=" << sonic.layoutId
        << " sonic_primitives=" << sonic.primitiveCount
        << " sonic_channels=" << sonicChannels
        << " title_atlas=" << title.atlasBinding
        << " title_layout=" << title.layoutId
        << " title_primitives=" << title.primitiveCount
        << '\n';

    return sonic.atlasBinding == "proxy"
        && sonic.layoutId == "none"
        && sonic.primitiveCount == 6
        && sonicChannels == "T3 C4 V2 S0 static1"
        && title.atlasBinding == "exact"
        && title.layoutId == "ui_mainmenu"
        && title.primitiveCount == 6
        ? 0
        : 1;
}

[[nodiscard]] const DebugWorkbenchHostEntry* findWorkbenchHostByDisplayName(std::string_view displayName)
{
    const auto found = std::find_if(
        kDebugWorkbenchHostEntries.begin(),
        kDebugWorkbenchHostEntries.end(),
        [displayName](const DebugWorkbenchHostEntry& host)
        {
            return host.hostDisplayName == displayName;
        });
    return found == kDebugWorkbenchHostEntries.end() ? nullptr : &*found;
}

[[nodiscard]] int runHostReadinessSmoke()
{
    const auto* sonic = findWorkbenchHostByDisplayName("SonicMainDisplay.cpp");
    const auto* title = findWorkbenchHostByDisplayName("GameModeMainMenu_Test.cpp");
    const auto* support = findWorkbenchHostByDisplayName("AchievementManager.cpp");
    if (!sonic || !title || !support)
    {
        std::cerr << "sward_ui_runtime_debug_gui host readiness smoke failed missing host\n";
        return 1;
    }

    const std::string sonicLabel = hostDisplayLabel(*sonic);
    const std::string titleLabel = hostDisplayLabel(*title);
    const std::string supportLabel = hostDisplayLabel(*support);

    std::cout
        << "sward_ui_runtime_debug_gui host readiness smoke ok "
        << "sonic_label=" << sonicLabel
        << " title_label=" << titleLabel
        << " support_label=" << supportLabel
        << '\n';

    return sonicLabel == "SonicMainDisplay.cpp [proxy primitive channels]"
        && titleLabel == "GameModeMainMenu_Test.cpp [exact layout primitive channels]"
        && supportLabel == "AchievementManager.cpp [contract]"
        ? 0
        : 1;
}

[[nodiscard]] int runRendererBlockerSmoke()
{
    const VisualParitySummary sonic = visualParitySummaryForContract("sonic_stage_hud_reference.json");
    const VisualParitySummary title = visualParitySummaryForContract("title_menu_reference.json");
    const VisualParitySummary support = visualParitySummaryForContract("achievement_unlock_support_reference.json");

    const std::string_view sonicBlocker = visualParityRendererBlocker(sonic);
    const std::string_view titleBlocker = visualParityRendererBlocker(title);
    const std::string_view supportBlocker = visualParityRendererBlocker(support);

    std::cout
        << "sward_ui_runtime_debug_gui renderer blocker smoke ok "
        << "sonic_blocker=" << sonicBlocker
        << " title_blocker=" << titleBlocker
        << " support_blocker=" << supportBlocker
        << '\n';

    return sonicBlocker == "exact loose HUD payload"
        && titleBlocker == "decoded CSD channel sampling"
        && supportBlocker == "visual evidence binding"
        ? 0
        : 1;
}

[[nodiscard]] int runLayoutChannelSampleSmoke()
{
    const auto title = layoutScenePrimitivesForContract("title_menu_reference.json");
    const auto pause = layoutScenePrimitivesForContract("pause_menu_reference.json");
    const auto loading = layoutScenePrimitivesForContract("loading_transition_reference.json");

    const auto* titlePrimitive = layoutScenePrimitiveForScene(title, "mm_donut_move");
    const auto* pausePrimitive = layoutScenePrimitiveForScene(pause, "stick");
    const auto* loadingPrimitive = layoutScenePrimitiveForScene(loading, "bg_2");
    if (!titlePrimitive || !pausePrimitive || !loadingPrimitive)
    {
        std::cerr << "sward_ui_runtime_debug_gui layout channel sample smoke failed missing exact-family primitive\n";
        return 1;
    }

    constexpr float kSampleProgress = 0.5F;
    const std::string titleSample = layoutPrimitiveChannelSampleToken(*titlePrimitive, kSampleProgress);
    const std::string pauseSample = layoutPrimitiveChannelSampleToken(*pausePrimitive, kSampleProgress);
    const std::string loadingSample = layoutPrimitiveChannelSampleToken(*loadingPrimitive, kSampleProgress);

    std::cout
        << "sward_ui_runtime_debug_gui layout channel sample smoke ok "
        << "title_sample=" << titleSample
        << " pause_sample=" << pauseSample
        << " loading_sample=" << loadingSample
        << '\n';

    return titleSample == "mm_donut_move:color+transform@110/220"
        && pauseSample == "stick:color+transform+visibility@120/240"
        && loadingSample == "bg_2:color+transform@1/2"
        ? 0
        : 1;
}

[[nodiscard]] int runLayoutDrawCommandSmoke()
{
    constexpr float kSampleProgress = 0.5F;
    constexpr int kCanvasWidth = 1280;
    constexpr int kCanvasHeight = 720;

    const auto title = layoutPrimitiveDrawCommandsForContract("title_menu_reference.json", kSampleProgress, kCanvasWidth, kCanvasHeight);
    const auto pause = layoutPrimitiveDrawCommandsForContract("pause_menu_reference.json", kSampleProgress, kCanvasWidth, kCanvasHeight);
    const auto loading = layoutPrimitiveDrawCommandsForContract("loading_transition_reference.json", kSampleProgress, kCanvasWidth, kCanvasHeight);
    if (title.empty() || pause.empty() || loading.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui layout draw command smoke failed missing exact-family draw command\n";
        return 1;
    }

    const std::string titleFirst = layoutPrimitiveDrawCommandDescriptor(title.front());
    const std::string pauseFirst = layoutPrimitiveDrawCommandDescriptor(pause.front());
    const std::string loadingFirst = layoutPrimitiveDrawCommandDescriptor(loading.front());

    std::cout
        << "sward_ui_runtime_debug_gui layout draw command smoke ok "
        << "title_commands=" << title.size()
        << " title_first=" << titleFirst
        << " pause_first=" << pauseFirst
        << " loading_first=" << loadingFirst
        << '\n';

    return title.size() == 6
        && titleFirst == "mm_donut_move:102,202,563x115:color+transform@110/220"
        && pauseFirst == "stick:435,187,589x101:color+transform+visibility@120/240"
        && loadingFirst == "bg_2:64,101,1152x115:color+transform@1/2"
        ? 0
        : 1;
}

[[nodiscard]] int runAuthoredKeyframeCurveSmoke()
{
    const auto title = layoutAuthoredKeyframeCurvesForContract("title_menu_reference.json");
    const auto pause = layoutAuthoredKeyframeCurvesForContract("pause_menu_reference.json");
    const auto loading = layoutAuthoredKeyframeCurvesForContract("loading_transition_reference.json");
    if (title.empty() || pause.empty() || loading.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui authored keyframe curve smoke failed missing exact-family authored curve\n";
        return 1;
    }

    const std::string titleCurve = layoutAuthoredKeyframeCurveDescriptor(*title.front());
    const std::string pauseCurve = layoutAuthoredKeyframeCurveDescriptor(*pause.front());
    const std::string loadingCurve = layoutAuthoredKeyframeCurveDescriptor(*loading.front());

    std::cout
        << "sward_ui_runtime_debug_gui authored keyframe curve smoke ok "
        << "title_curve=" << titleCurve
        << " pause_curve=" << pauseCurve
        << " loading_curve=" << loadingCurve
        << '\n';

    return titleCurve == "mm_donut_move/intro/index_text_pos/YPosition:kf5:0=0.411111->40=0.188889:Linear"
        && pauseCurve == "bg/Intro_Anim/img/Color:kf2:0=0.000000->15=0.000000:Linear"
        && loadingCurve == "bg_2/360_sonic1/pos_text_sonic/XPosition:kf1:0=0.500000->0=0.500000:Linear"
        ? 0
        : 1;
}

[[nodiscard]] int runAuthoredKeyframeSampleSmoke()
{
    const auto title = layoutAuthoredKeyframeSamplesForContract("title_menu_reference.json");
    const auto pause = layoutAuthoredKeyframeSamplesForContract("pause_menu_reference.json");
    const auto loading = layoutAuthoredKeyframeSamplesForContract("loading_transition_reference.json");
    if (title.empty() || pause.empty() || loading.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui authored keyframe sample smoke failed missing exact-family sample\n";
        return 1;
    }

    const std::string titleSample = layoutAuthoredKeyframeSampleDescriptor(*title.front());
    const std::string pauseSample = layoutAuthoredKeyframeSampleDescriptor(*pause.front());
    const std::string loadingSample = layoutAuthoredKeyframeSampleDescriptor(*loading.front());

    std::cout
        << "sward_ui_runtime_debug_gui authored keyframe sample smoke ok "
        << "title_sample=" << titleSample
        << " pause_sample=" << pauseSample
        << " loading_sample=" << loadingSample
        << '\n';

    return titleSample == "mm_donut_move/intro/index_text_pos/YPosition@30=0.225926"
        && pauseSample == "bg/Intro_Anim/img/Color@7=0.000000"
        && loadingSample == "bg_2/360_sonic1/pos_text_sonic/XPosition@1=0.500000"
        ? 0
        : 1;
}

[[nodiscard]] int runAuthoredSampledTransformSmoke()
{
    const auto title = layoutAuthoredSampledTransformsForContract("title_menu_reference.json");
    const auto loading = layoutAuthoredSampledTransformsForContract("loading_transition_reference.json");
    if (title.empty() || loading.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui authored sampled transform smoke failed missing exact-family transform sample\n";
        return 1;
    }

    const std::string titleTransform = layoutAuthoredSampledTransformDescriptor(*title.front());
    const std::string loadingTransform = layoutAuthoredSampledTransformDescriptor(*loading.front());

    std::cout
        << "sward_ui_runtime_debug_gui authored sampled transform smoke ok "
        << "title_transform=" << titleTransform
        << " loading_transform=" << loadingTransform
        << '\n';

    return titleTransform == "mm_donut_move/index_text_pos@30:408,163,16x16:YPosition=0.225926"
        && loadingTransform == "bg_2/pos_text_sonic@1:640,360,16x16:XPosition=0.500000"
        ? 0
        : 1;
}

[[nodiscard]] int runAuthoredSampledTransformPreviewSmoke()
{
    const auto title = layoutAuthoredSampledTransformsForContract("title_menu_reference.json");
    const auto loading = layoutAuthoredSampledTransformsForContract("loading_transition_reference.json");
    if (title.empty() || loading.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui authored sampled transform preview smoke failed missing exact-family transform sample\n";
        return 1;
    }

    const std::string titlePreview = layoutAuthoredSampledTransformPreviewDescriptor(*title.front());
    const std::string loadingPreview = layoutAuthoredSampledTransformPreviewDescriptor(*loading.front());

    std::cout
        << "sward_ui_runtime_debug_gui authored sampled transform preview smoke ok "
        << "title_preview=" << titlePreview
        << " loading_preview=" << loadingPreview
        << '\n';

    return titlePreview == "mm_donut_move/index_text_pos:408,163,16x16"
        && loadingPreview == "bg_2/pos_text_sonic:640,360,16x16"
        ? 0
        : 1;
}

[[nodiscard]] int runAuthoredSampledDrawCommandSmoke()
{
    const auto title = layoutAuthoredSampledDrawCommandsForContract("title_menu_reference.json");
    const auto pause = layoutAuthoredSampledDrawCommandsForContract("pause_menu_reference.json");
    const auto loading = layoutAuthoredSampledDrawCommandsForContract("loading_transition_reference.json");
    if (title.empty() || pause.empty() || loading.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui authored sampled draw command smoke failed missing exact-family draw command\n";
        return 1;
    }

    const std::string titleDraw = layoutAuthoredSampledDrawCommandDescriptor(title.front());
    const std::string pauseDraw = layoutAuthoredSampledDrawCommandDescriptor(pause.front());
    const std::string loadingDraw = layoutAuthoredSampledDrawCommandDescriptor(loading.front());

    std::cout
        << "sward_ui_runtime_debug_gui authored sampled draw command smoke ok "
        << "title_draw=" << titleDraw
        << " pause_draw=" << pauseDraw
        << " loading_draw=" << loadingDraw
        << '\n';

    return titleDraw == "mm_donut_move/index_text_pos:408,163,16x16:YPosition@30=0.225926"
        && pauseDraw == "bg/img:0,0,1280x720:Color@7=0.000000"
        && loadingDraw == "bg_2/pos_text_sonic:640,360,16x16:XPosition@1=0.500000"
        ? 0
        : 1;
}

[[nodiscard]] int runAuthoredSampledChannelCommandSmoke()
{
    const auto pause = layoutAuthoredSampledDrawCommandsForContract("pause_menu_reference.json");
    if (pause.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui authored sampled channel command smoke failed missing Pause color draw command\n";
        return 1;
    }

    const std::string pauseChannel = layoutAuthoredSampledDrawCommandChannelStateDescriptor(pause.front());

    std::cout
        << "sward_ui_runtime_debug_gui authored sampled channel command smoke ok "
        << "pause_channel=" << pauseChannel
        << '\n';

    return pauseChannel == "bg/img:0,0,1280x720:Color@7=0.000000:alpha=0.000000"
        ? 0
        : 1;
}

[[nodiscard]] int runAuthoredSampledChannelEvalSmoke()
{
    const auto title = layoutAuthoredSampledDrawCommandsForContract("title_menu_reference.json");
    const auto pause = layoutAuthoredSampledDrawCommandsForContract("pause_menu_reference.json");
    const auto loading = layoutAuthoredSampledDrawCommandsForContract("loading_transition_reference.json");
    if (title.empty() || pause.empty() || loading.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui authored sampled channel eval smoke failed missing exact-family draw command\n";
        return 1;
    }

    const std::string titleEval = layoutAuthoredSampledDrawCommandEvaluatedStateDescriptor(title.front());
    const std::string pauseEval = layoutAuthoredSampledDrawCommandEvaluatedStateDescriptor(pause.front());
    const std::string loadingEval = layoutAuthoredSampledDrawCommandEvaluatedStateDescriptor(loading.front());

    std::cout
        << "sward_ui_runtime_debug_gui authored sampled channel eval smoke ok "
        << "title_eval=" << titleEval
        << " pause_eval=" << pauseEval
        << " loading_eval=" << loadingEval
        << '\n';

    return titleEval == "mm_donut_move/index_text_pos:408,163,16x16:YPosition@30=0.225926:alpha=1.000000:visible=1:delta=0,-133"
        && pauseEval == "bg/img:0,0,1280x720:Color@7=0.000000:alpha=0.000000:visible=1:delta=0,0"
        && loadingEval == "bg_2/pos_text_sonic:640,360,16x16:XPosition@1=0.500000:alpha=1.000000:visible=1:delta=0,0"
        ? 0
        : 1;
}

[[nodiscard]] int runAuthoredCastTransformSmoke()
{
    const auto title = layoutAuthoredCastTransformsForContract("title_menu_reference.json");
    const auto pause = layoutAuthoredCastTransformsForContract("pause_menu_reference.json");
    const auto loading = layoutAuthoredCastTransformsForContract("loading_transition_reference.json");
    if (title.empty() || pause.empty() || loading.empty())
    {
        std::cerr << "sward_ui_runtime_debug_gui authored cast transform smoke failed missing exact-family authored cast\n";
        return 1;
    }

    const std::string titleCast = layoutAuthoredCastTransformDescriptor(*title.front());
    const std::string pauseCast = layoutAuthoredCastTransformDescriptor(*pause.front());
    const std::string loadingCast = layoutAuthoredCastTransformDescriptor(*loading.front());

    std::cout
        << "sward_ui_runtime_debug_gui authored cast transform smoke ok "
        << "title_cast=" << titleCast
        << " pause_cast=" << pauseCast
        << " loading_cast=" << loadingCast
        << '\n';

    return titleCast == "mm_donut_move/index_text_pos:408,296,16x16:r0.00:s1.00,1.00:0xFFFFFFFF"
        && pauseCast == "bg/img:0,0,1280x720:r0.00:s1.00,1.00:0x00000000"
        && loadingCast == "bg_2/pos_text_sonic:640,360,16x16:r0.00:s1.00,1.00:0xFFFFFFFF"
        ? 0
        : 1;
}

[[nodiscard]] int runLayerFillSmoke()
{
    const float backdropAlpha = previewLayerFillAlpha("backdrop");
    const float cinematicAlpha = previewLayerFillAlpha("cinematic_frame");
    const float contentAlpha = previewLayerFillAlpha("content");

    std::cout
        << "sward_ui_runtime_debug_gui layer fill smoke ok "
        << "backdrop_alpha=" << backdropAlpha
        << " cinematic_alpha=" << cinematicAlpha
        << " content_alpha=" << contentAlpha
        << '\n';

    return backdropAlpha == 0.0F && cinematicAlpha == 0.0F && contentAlpha > 0.0F ? 0 : 1;
}

class WorkbenchGui
{
public:
    explicit WorkbenchGui(HINSTANCE instance)
        : m_instance(instance)
        , m_contracts(loadBundledEntries())
        , m_hosts(resolveHostEntries(m_contracts))
        , m_groups(buildGroups(m_hosts))
    {
    }

    int run(int showCommand)
    {
        WNDCLASSA windowClass{};
        windowClass.lpfnWndProc = &WorkbenchGui::windowProc;
        windowClass.hInstance = m_instance;
        windowClass.lpszClassName = "SwardUiRuntimeDebugGui";
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

        registerPreviewClass();
        RegisterClassA(&windowClass);

        RECT workArea{ 0, 0, 1240, 760 };
        SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0);
        const int workAreaWidth = std::max(900, static_cast<int>(workArea.right - workArea.left));
        const int workAreaHeight = std::max(640, static_cast<int>(workArea.bottom - workArea.top));
        const int windowWidth = std::min(1240, std::max(900, workAreaWidth - 40));
        const int windowHeight = std::min(760, std::max(640, workAreaHeight - 40));
        const int windowX = workArea.left + std::max(0, (workAreaWidth - windowWidth) / 2);
        const int windowY = workArea.top + std::max(0, (workAreaHeight - windowHeight) / 2);

        m_window = CreateWindowExA(
            0,
            windowClass.lpszClassName,
            "SWARD UI Runtime Debug Workbench",
            WS_OVERLAPPEDWINDOW,
            windowX,
            windowY,
            windowWidth,
            windowHeight,
            nullptr,
            nullptr,
            m_instance,
            this);

        if (!m_window)
            return 1;

        applyDefaultFont();
        ShowWindow(m_window, showCommand);
        UpdateWindow(m_window);

        MSG message{};
        while (GetMessageA(&message, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        return static_cast<int>(message.wParam);
    }

private:
    void registerPreviewClass()
    {
        WNDCLASSA previewClass{};
        previewClass.lpfnWndProc = &WorkbenchGui::previewWindowProc;
        previewClass.hInstance = m_instance;
        previewClass.lpszClassName = kPreviewPanelClassName;
        previewClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        previewClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        RegisterClassA(&previewClass);
    }

    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* app = reinterpret_cast<WorkbenchGui*>(GetWindowLongPtrA(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            const auto* create = reinterpret_cast<CREATESTRUCTA*>(lParam);
            app = reinterpret_cast<WorkbenchGui*>(create->lpCreateParams);
            SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
            app->m_window = window;
        }

        if (!app)
            return DefWindowProcA(window, message, wParam, lParam);
        return app->handleMessage(message, wParam, lParam);
    }

    static LRESULT CALLBACK previewWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* app = reinterpret_cast<WorkbenchGui*>(GetWindowLongPtrA(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            const auto* create = reinterpret_cast<CREATESTRUCTA*>(lParam);
            app = reinterpret_cast<WorkbenchGui*>(create->lpCreateParams);
            SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }

        if (message == WM_ERASEBKGND)
            return 1;

        if (message == WM_PAINT)
        {
            PAINTSTRUCT paint{};
            const HDC dc = BeginPaint(window, &paint);
            if (app)
                app->paintPreview(dc);
            EndPaint(window, &paint);
            return 0;
        }

        if (message == WM_PRINTCLIENT || message == WM_PRINT)
        {
            if (app)
                app->paintPreview(reinterpret_cast<HDC>(wParam));
            return 0;
        }

        return DefWindowProcA(window, message, wParam, lParam);
    }

    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
            createControls();
            populateGroups();
            return 0;
        case WM_SIZE:
            layoutControls(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_COMMAND:
            handleCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_TIMER:
            if (wParam == kPlaybackTimerId)
            {
                tickPlaybackFrame(kPlaybackTickSeconds);
                return 0;
            }
            return DefWindowProcA(m_window, message, wParam, lParam);
        case WM_DESTROY:
            stopPlayback();
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcA(m_window, message, wParam, lParam);
        }
    }

    void createControls()
    {
        m_groupLabel = createStatic("Groups");
        m_groupList = createListBox(kGroupListId);
        m_hostLabel = createStatic("Hosts");
        m_hostList = createListBox(kHostListId);
        m_runButton = createButton("Run Host", kRunButtonId);
        m_moveNextButton = createButton("Move Next", kMoveNextButtonId);
        m_confirmButton = createButton("Confirm", kConfirmButtonId);
        m_cancelButton = createButton("Cancel", kCancelButtonId);
        m_resetButton = createButton("Reset", kResetButtonId);
        m_playPauseButton = createButton("Play", kPlayPauseButtonId);
        m_stepButton = createButton("Step", kStepButtonId);
        m_previewModeButton = createButton("Asset View", kPreviewModeButtonId);
        m_assetPrevButton = createButton("Asset Prev", kAssetPrevButtonId);
        m_assetNextButton = createButton("Asset Next", kAssetNextButtonId);
        m_csdPrevButton = createButton("Element Prev", kCsdPrevButtonId);
        m_csdNextButton = createButton("Element Next", kCsdNextButtonId);
        m_previewPanel = createPreviewPanel();
        m_detailText = createEdit();
        m_logText = createEdit();
    }

    void applyDefaultFont()
    {
        const auto font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        const HWND controls[] = {
            m_groupLabel,
            m_groupList,
            m_hostLabel,
            m_hostList,
            m_runButton,
            m_moveNextButton,
            m_confirmButton,
            m_cancelButton,
            m_resetButton,
            m_playPauseButton,
            m_stepButton,
            m_previewModeButton,
            m_assetPrevButton,
            m_assetNextButton,
            m_csdPrevButton,
            m_csdNextButton,
            m_previewPanel,
            m_detailText,
            m_logText,
        };

        for (const HWND control : controls)
        {
            if (control)
                SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
    }

    HWND createStatic(const char* text)
    {
        return CreateWindowExA(0, "STATIC", text, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_window, nullptr, m_instance, nullptr);
    }

    HWND createButton(const char* text, int id)
    {
        return CreateWindowExA(0, "BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, m_window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), m_instance, nullptr);
    }

    HWND createListBox(int id)
    {
        return CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "LISTBOX",
            "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
            0,
            0,
            0,
            0,
            m_window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            m_instance,
            nullptr);
    }

    HWND createPreviewPanel()
    {
        return CreateWindowExA(
            WS_EX_CLIENTEDGE,
            kPreviewPanelClassName,
            "",
            WS_CHILD | WS_VISIBLE,
            0,
            0,
            0,
            0,
            m_window,
            nullptr,
            m_instance,
            this);
    }

    HWND createEdit()
    {
        return CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "EDIT",
            "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0,
            0,
            0,
            0,
            m_window,
            nullptr,
            m_instance,
            nullptr);
    }

    void layoutControls(int width, int height)
    {
        const int margin = 12;
        const int labelHeight = 20;
        const int buttonHeight = 30;
        const int groupWidth = 270;
        const int hostWidth = 300;
        const int rightX = margin + groupWidth + margin + hostWidth + margin;
        const int rightWidth = std::max(260, width - rightX - margin);
        const int listTop = margin + labelHeight;
        const int listHeight = std::max(160, height - listTop - margin);

        MoveWindow(m_groupLabel, margin, margin, groupWidth, labelHeight, TRUE);
        MoveWindow(m_groupList, margin, listTop, groupWidth, listHeight, TRUE);
        MoveWindow(m_hostLabel, margin + groupWidth + margin, margin, hostWidth, labelHeight, TRUE);
        MoveWindow(m_hostList, margin + groupWidth + margin, listTop, hostWidth, listHeight, TRUE);

        const int buttonY = margin;
        const int buttonWidth = 92;
        MoveWindow(m_runButton, rightX, buttonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_playPauseButton, rightX + 100, buttonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_stepButton, rightX + 200, buttonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_resetButton, rightX + 300, buttonY, buttonWidth, buttonHeight, TRUE);

        const int actionButtonY = buttonY + buttonHeight + 8;
        MoveWindow(m_moveNextButton, rightX, actionButtonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_confirmButton, rightX + 100, actionButtonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_cancelButton, rightX + 200, actionButtonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_previewModeButton, rightX + 300, actionButtonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_assetPrevButton, rightX + 400, actionButtonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_assetNextButton, rightX + 500, actionButtonY, buttonWidth, buttonHeight, TRUE);

        const int elementButtonY = actionButtonY + buttonHeight + 8;
        MoveWindow(m_csdPrevButton, rightX, elementButtonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_csdNextButton, rightX + 100, elementButtonY, buttonWidth, buttonHeight, TRUE);

        const int previewTop = elementButtonY + buttonHeight + margin;
        const int previewWantedHeight = (rightWidth * 9 / 16) + 46;
        const int previewMaxHeight = std::max(220, height * 45 / 100);
        const int previewHeight = std::min(std::max(220, previewWantedHeight), previewMaxHeight);
        MoveWindow(m_previewPanel, rightX, previewTop, rightWidth, previewHeight, TRUE);

        const int detailsTop = previewTop + previewHeight + margin;
        const int remainingHeight = std::max(160, height - detailsTop - margin);
        const int detailsHeight = std::max(110, (remainingHeight - margin) / 2);
        MoveWindow(m_detailText, rightX, detailsTop, rightWidth, detailsHeight, TRUE);
        MoveWindow(m_logText, rightX, detailsTop + detailsHeight + margin, rightWidth, std::max(80, remainingHeight - detailsHeight - margin), TRUE);
    }

    [[nodiscard]] const ResolvedHostEntry* selectedHostEntry() const
    {
        if (!m_selectedHostIndex.has_value() || *m_selectedHostIndex >= m_hosts.size())
            return nullptr;
        return &m_hosts[*m_selectedHostIndex];
    }

    [[nodiscard]] std::optional<std::size_t> selectedAssetGalleryIndex(const DebugWorkbenchHostEntry* host, const std::vector<std::filesystem::path>& files) const
    {
        if (files.empty())
            return std::nullopt;

        if (m_assetGalleryIndex.has_value() && *m_assetGalleryIndex < files.size())
            return *m_assetGalleryIndex;

        if (host)
        {
            if (const auto* candidate = atlasCandidateForContract(host->primaryContractFileName))
            {
                const auto found = std::find_if(
                    files.begin(),
                    files.end(),
                    [candidate](const std::filesystem::path& path)
                    {
                        return path.filename().string() == candidate->atlasFileName;
                    });
                if (found != files.end())
                    return static_cast<std::size_t>(std::distance(files.begin(), found));
            }
        }

        return 0;
    }

    void syncAssetGalleryToSelectedHost()
    {
        const auto* selected = selectedHostEntry();
        const auto* host = selected ? selected->metadata : nullptr;
        const auto files = visualAtlasSheetFiles();
        m_assetGalleryIndex.reset();
        m_assetGalleryIndex = selectedAssetGalleryIndex(host, files);
    }

    void syncCsdElementToSelectedHost()
    {
        m_csdElementIndex = 0;
    }

    void shiftAssetGallery(int delta)
    {
        const auto files = visualAtlasSheetFiles();
        if (files.empty())
            return;

        const auto* selected = selectedHostEntry();
        const auto* host = selected ? selected->metadata : nullptr;
        const std::size_t current = selectedAssetGalleryIndex(host, files).value_or(0);
        const int count = static_cast<int>(files.size());
        int next = static_cast<int>(current) + delta;
        while (next < 0)
            next += count;
        next %= count;
        m_assetGalleryIndex = static_cast<std::size_t>(next);

        showHostSummary();
        if (m_previewPanel)
            InvalidateRect(m_previewPanel, nullptr, TRUE);
    }

    [[nodiscard]] std::size_t currentCsdElementIndex(const DebugWorkbenchHostEntry& host) const
    {
        const auto bindings = layoutCsdElementBindingsForContract(host.primaryContractFileName);
        return selectedCsdElementBindingIndex(m_csdElementIndex.value_or(0), bindings.size());
    }

    void shiftCsdElementBinding(int delta)
    {
        const auto* selected = selectedHostEntry();
        const auto* host = selected ? selected->metadata : nullptr;
        if (!host)
            return;

        const auto bindings = layoutCsdElementBindingsForContract(host->primaryContractFileName);
        if (bindings.empty())
            return;

        const int count = static_cast<int>(bindings.size());
        int next = static_cast<int>(currentCsdElementIndex(*host)) + delta;
        while (next < 0)
            next += count;
        next %= count;
        m_csdElementIndex = static_cast<std::size_t>(next);

        showHostSummary();
        if (m_previewPanel)
            InvalidateRect(m_previewPanel, nullptr, TRUE);
    }

    [[nodiscard]] std::string assetGallerySummaryText(const DebugWorkbenchHostEntry& host) const
    {
        const auto files = visualAtlasSheetFiles();
        const auto index = selectedAssetGalleryIndex(&host, files);
        if (!index.has_value())
            return "Gallery asset: none";

        std::ostringstream text;
        text
            << "Gallery asset: " << (*index + 1) << "/" << files.size()
            << " " << files[*index].filename().string();
        return text.str();
    }

    [[nodiscard]] std::string csdElementGallerySummaryText(const DebugWorkbenchHostEntry& host) const
    {
        const auto bindings = layoutCsdElementBindingsForContract(host.primaryContractFileName);
        if (bindings.empty())
            return "CSD element: none";

        const std::size_t index = currentCsdElementIndex(host);
        std::ostringstream text;
        text
            << "CSD element: " << (index + 1) << "/" << bindings.size()
            << " " << layoutCsdElementBindingDescriptor(*bindings[index]);
        return text.str();
    }

    void drawAssetViewerPreview(HDC dc, Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas, const DebugWorkbenchHostEntry* host, const LayoutEvidence* layoutEvidence)
    {
        Gdiplus::SolidBrush backingBrush(Gdiplus::Color(255, 6, 10, 12));
        graphics.FillRectangle(&backingBrush, canvas);

        std::string label = "Asset View: select a host with a local atlas";
        bool drewAtlas = false;
        std::optional<Gdiplus::RectF> drawnAtlasRect;
        const auto files = visualAtlasSheetFiles();
        const auto selectedAssetIndex = selectedAssetGalleryIndex(host, files);
        if (selectedAssetIndex.has_value())
        {
            const auto& atlasPath = files[*selectedAssetIndex];
            label = "Asset View: " + atlasPath.filename().string();
            if (host)
            {
                if (const auto* candidate = atlasCandidateForContract(host->primaryContractFileName);
                    candidate && atlasPath.filename().string() == candidate->atlasFileName)
                {
                    label = "Asset View: " + assetViewerAtlasDescriptor(*candidate);
                }
            }

            if (std::filesystem::exists(atlasPath))
            {
                Gdiplus::Image image(atlasPath.wstring().c_str());
                if (image.GetLastStatus() == Gdiplus::Ok && image.GetWidth() > 0 && image.GetHeight() > 0)
                {
                    const float padding = 18.0F;
                    const float availableWidth = std::max(1.0F, canvas.Width - (padding * 2.0F));
                    const float availableHeight = std::max(1.0F, canvas.Height - (padding * 2.0F));
                    const float imageAspect = static_cast<float>(image.GetWidth()) / static_cast<float>(image.GetHeight());
                    float drawWidth = availableWidth;
                    float drawHeight = drawWidth / imageAspect;
                    if (drawHeight > availableHeight)
                    {
                        drawHeight = availableHeight;
                        drawWidth = drawHeight * imageAspect;
                    }

                    Gdiplus::RectF imageRect(
                        canvas.X + ((canvas.Width - drawWidth) / 2.0F),
                        canvas.Y + ((canvas.Height - drawHeight) / 2.0F),
                        drawWidth,
                        drawHeight);

                    graphics.DrawImage(&image, imageRect.X, imageRect.Y, imageRect.Width, imageRect.Height);
                    Gdiplus::Pen imagePen(Gdiplus::Color(180, 134, 218, 126), 1.0F);
                    graphics.DrawRectangle(&imagePen, imageRect);
                    drawnAtlasRect = imageRect;
                    drewAtlas = true;
                }
            }
        }

        if (!drewAtlas)
        {
            RECT missingText{
                static_cast<LONG>(canvas.X + 20.0F),
                static_cast<LONG>(canvas.Y + 20.0F),
                static_cast<LONG>(canvas.X + canvas.Width - 20.0F),
                static_cast<LONG>(canvas.Y + canvas.Height - 20.0F),
            };
            drawTextLine(dc, missingText, "No local atlas sheet is available for this host yet.", RGB(230, 235, 238), DT_LEFT | DT_WORDBREAK);
        }

        if (host && drawnAtlasRect.has_value())
            drawAssetCsdElementBindings(dc, graphics, *drawnAtlasRect, host->primaryContractFileName, currentCsdElementIndex(*host));

        Gdiplus::Pen canvasPen(Gdiplus::Color(255, 70, 88, 70), 2.0F);
        graphics.DrawRectangle(&canvasPen, canvas);

        Gdiplus::RectF labelPanel(canvas.X + 12.0F, canvas.Y + 12.0F, std::min(canvas.Width - 24.0F, 520.0F), 52.0F);
        Gdiplus::SolidBrush labelBrush(Gdiplus::Color(220, 8, 20, 12));
        Gdiplus::Pen labelPen(Gdiplus::Color(210, 134, 218, 126), 1.0F);
        graphics.FillRectangle(&labelBrush, labelPanel);
        graphics.DrawRectangle(&labelPen, labelPanel);

        RECT labelText{
            static_cast<LONG>(labelPanel.X + 10.0F),
            static_cast<LONG>(labelPanel.Y),
            static_cast<LONG>(labelPanel.X + labelPanel.Width - 10.0F),
            static_cast<LONG>(labelPanel.Y + 24.0F),
        };
        drawTextLine(dc, labelText, label, RGB(238, 246, 232));

        RECT subText{
            static_cast<LONG>(labelPanel.X + 10.0F),
            static_cast<LONG>(labelPanel.Y + 24.0F),
            static_cast<LONG>(labelPanel.X + labelPanel.Width - 10.0F),
            static_cast<LONG>(labelPanel.Y + labelPanel.Height),
        };
        std::ostringstream detail;
        detail << "sheets=" << files.size();
        if (selectedAssetIndex.has_value())
            detail << " | asset=" << (*selectedAssetIndex + 1) << "/" << files.size();
        if (layoutEvidence)
            detail << " | layout=" << layoutEvidence->layoutId << " scenes=" << layoutEvidence->sceneCount << " anims=" << layoutEvidence->animationCount;
        if (host)
        {
            const auto bindings = layoutCsdElementBindingsForContract(host->primaryContractFileName);
            if (!bindings.empty())
            {
                const std::size_t elementIndex = currentCsdElementIndex(*host);
                const auto* selected = bindings[elementIndex];
                detail << " | csd=" << (elementIndex + 1) << "/" << bindings.size() << " " << selected->packageFileName << "/" << selected->sceneName << "/" << selected->castName;
            }
        }
        drawTextLine(dc, subText, detail.str(), RGB(164, 214, 154));
    }

    void paintPreview(HDC dc)
    {
        RECT client{};
        GetClientRect(m_previewPanel, &client);
        const int width = client.right - client.left;
        const int height = client.bottom - client.top;
        if (width <= 0 || height <= 0)
            return;

        Gdiplus::Graphics graphics(dc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.Clear(Gdiplus::Color(255, 246, 247, 244));

        const auto* selected = selectedHostEntry();
        const auto* host = selected ? selected->metadata : nullptr;
        PreviewFamily previewFamily = PreviewFamily::Generic;
        const LayoutEvidence* layoutEvidence = nullptr;
        std::vector<const LayoutScenePrimitive*> layoutPrimitives;
        if (host)
        {
            previewFamily = previewFamilyForContract(host->primaryContractFileName);
            layoutEvidence = layoutEvidenceForContract(host->primaryContractFileName);
            layoutPrimitives = layoutScenePrimitivesForContract(host->primaryContractFileName);
        }

        RECT titleRect{ 12, 8, width - 12, 30 };
        if (!layoutPrimitives.empty())
            titleRect.right = std::max<LONG>(titleRect.left + 180, static_cast<LONG>(width - 330));
        std::string title = m_previewMode == PreviewMode::Asset ? "Asset View" : "Visual Preview";
        if (host)
            title += " - " + std::string(host->hostDisplayName);
        drawTextLine(dc, titleRect, title, RGB(34, 40, 46));

        Gdiplus::RectF bounds(14.0F, 38.0F, static_cast<float>(std::max(1, width - 28)), static_cast<float>(std::max(1, height - 78)));
        const float targetAspect = 16.0F / 9.0F;
        float canvasWidth = bounds.Width;
        float canvasHeight = canvasWidth / targetAspect;
        if (canvasHeight > bounds.Height)
        {
            canvasHeight = bounds.Height;
            canvasWidth = canvasHeight * targetAspect;
        }

        Gdiplus::RectF canvas(
            bounds.X + ((bounds.Width - canvasWidth) / 2.0F),
            bounds.Y + ((bounds.Height - canvasHeight) / 2.0F),
            canvasWidth,
            canvasHeight);

        if (m_previewMode == PreviewMode::Asset)
        {
            drawAssetViewerPreview(dc, graphics, canvas, host, layoutEvidence);
            return;
        }

        bool drewAtlas = false;
        std::string atlasLabel = "Local atlas: none";
        if (host)
        {
            if (const auto* candidate = atlasCandidateForContract(host->primaryContractFileName))
            {
                const auto atlasPath = visualAtlasSheetRoot() / std::string(candidate->atlasFileName);
                atlasLabel = "Local atlas: " + atlasCandidateDisplayName(*candidate);
                if (std::filesystem::exists(atlasPath))
                {
                    Gdiplus::Image image(atlasPath.wstring().c_str());
                    if (image.GetLastStatus() == Gdiplus::Ok)
                    {
                        Gdiplus::SolidBrush atlasBackingBrush(Gdiplus::Color(255, 12, 18, 23));
                        graphics.FillRectangle(&atlasBackingBrush, canvas);
                        graphics.DrawImage(&image, canvas.X, canvas.Y, canvas.Width, canvas.Height);
                        drewAtlas = true;
                    }
                }
            }
        }

        if (!drewAtlas)
        {
            Gdiplus::SolidBrush canvasBrush(Gdiplus::Color(255, 30, 34, 38));
            graphics.FillRectangle(&canvasBrush, canvas);

            RECT fallbackText{
                static_cast<LONG>(canvas.X + 16.0F),
                static_cast<LONG>(canvas.Y + 16.0F),
                static_cast<LONG>(canvas.X + canvas.Width - 16.0F),
                static_cast<LONG>(canvas.Y + canvas.Height - 16.0F),
            };
            drawTextLine(dc, fallbackText, "No local atlas sheet matched. Rendering contract layers only.", RGB(230, 235, 238), DT_LEFT | DT_WORDBREAK);
        }

        Gdiplus::Pen canvasPen(Gdiplus::Color(255, 38, 45, 52), 2.0F);
        graphics.DrawRectangle(&canvasPen, canvas);

        double stateDuration = 0.0;
        float stateLinearProgress = 0.0F;
        float stateProgress = 1.0F;
        if (m_runtime && m_runningContractIndex.has_value())
        {
            const auto& contract = m_contracts[*m_runningContractIndex].contract;
            stateDuration = timelineDuration(contract, m_runtime->state());
            stateLinearProgress = stateDuration > 0.0
                ? static_cast<float>(std::min(1.0, m_runtime->stateElapsedSeconds() / stateDuration))
                : (m_runtime->state() == ScreenState::Idle ? 1.0F : 0.0F);
            stateProgress = easedTimelineProgress(m_runtime->stateElapsedSeconds(), stateDuration);
        }

        drawLayoutScenePrimitives(dc, graphics, canvas, layoutPrimitives, stateLinearProgress);
        if (host)
            drawAuthoredSampledTransforms(dc, graphics, canvas, host->primaryContractFileName);
        drawLayoutPrimitiveChannelLegend(dc, graphics, canvas, layoutPrimitives);

        if (m_runtime)
        {
            const auto layers = m_runtime->visibleLayers();
            for (std::size_t index = 0; index < layers.size(); ++index)
            {
                std::size_t roleIndex = 0;
                for (std::size_t previous = 0; previous < index; ++previous)
                {
                    if (layers[previous].role == layers[index].role)
                        ++roleIndex;
                }
                const PreviewMotion motion = previewMotionForState(m_runtime->state(), layers[index].role, stateProgress, canvas);
                const Gdiplus::RectF layerRect = applyPreviewMotion(layoutFamilyLayerRect(previewFamily, layers[index], roleIndex, canvas), motion, canvas);

                Gdiplus::SolidBrush layerBrush(Gdiplus::Color(motionAlphaByte(previewLayerFillAlpha(layers[index].role), motion), 57, 166, 218));
                Gdiplus::Pen layerPen(Gdiplus::Color(motionAlphaByte(0.95F, motion), 245, 248, 250), 1.5F);
                if (previewLayerFillAlpha(layers[index].role) > 0.0F)
                    graphics.FillRectangle(&layerBrush, layerRect);
                graphics.DrawRectangle(&layerPen, layerRect);

                RECT layerText{
                    static_cast<LONG>(layerRect.X + 8.0F),
                    static_cast<LONG>(layerRect.Y),
                    static_cast<LONG>(layerRect.X + layerRect.Width - 8.0F),
                    static_cast<LONG>(layerRect.Y + layerRect.Height),
                };
                drawTextLine(dc, layerText, layers[index].id + " : " + layers[index].role, RGB(255, 255, 255));
            }

            const auto prompts = m_runtime->visiblePrompts();
            const float promptTop = canvas.Y + canvas.Height - 38.0F;
            const float promptWidth = prompts.empty() ? 0.0F : std::min(132.0F, (canvas.Width - 32.0F) / static_cast<float>(prompts.size()));
            for (std::size_t index = 0; index < prompts.size(); ++index)
            {
                const PreviewMotion promptMotion = previewMotionForState(m_runtime->state(), "prompt", stateProgress, canvas);
                Gdiplus::RectF promptRect(
                    canvas.X + 16.0F + (static_cast<float>(index) * promptWidth),
                    promptTop,
                    std::max(44.0F, promptWidth - 8.0F),
                    26.0F);
                promptRect = applyPreviewMotion(promptRect, promptMotion, canvas);
                Gdiplus::SolidBrush promptBrush(Gdiplus::Color(motionAlphaByte(0.9F, promptMotion), 247, 211, 72));
                Gdiplus::Pen promptPen(Gdiplus::Color(motionAlphaByte(1.0F, promptMotion), 35, 41, 47), 1.2F);
                graphics.FillRectangle(&promptBrush, promptRect);
                graphics.DrawRectangle(&promptPen, promptRect);

                RECT promptText{
                    static_cast<LONG>(promptRect.X + 6.0F),
                    static_cast<LONG>(promptRect.Y),
                    static_cast<LONG>(promptRect.X + promptRect.Width - 6.0F),
                    static_cast<LONG>(promptRect.Y + promptRect.Height),
                };
                drawTextLine(dc, promptText, std::string(toString(prompts[index].button)) + " " + prompts[index].label, RGB(24, 29, 34));
            }
        }

        if (layoutEvidence)
            drawLayoutEvidenceOverlay(dc, graphics, canvas, *layoutEvidence, stateLinearProgress);

        if (m_runtime && m_runningContractIndex.has_value())
        {
            const double progress = stateDuration > 0.0 ? std::min(1.0, m_runtime->stateElapsedSeconds() / stateDuration) : (m_runtime->state() == ScreenState::Idle ? 1.0 : 0.0);
            Gdiplus::RectF track(canvas.X, canvas.Y + canvas.Height + 10.0F, canvas.Width, 8.0F);
            Gdiplus::SolidBrush trackBrush(Gdiplus::Color(255, 203, 209, 213));
            Gdiplus::SolidBrush fillBrush(Gdiplus::Color(255, 45, 130, 216));
            graphics.FillRectangle(&trackBrush, track);
            graphics.FillRectangle(&fillBrush, track.X, track.Y, static_cast<float>(track.Width * progress), track.Height);
        }

        RECT footerRect{ 12, height - 30, width - 12, height - 8 };
        if (m_runtime)
        {
            std::ostringstream footer;
            footer
                << atlasLabel
                << " | Family " << previewFamilyName(previewFamily)
                << " | Layout " << (layoutEvidence ? layoutEvidence->layoutId : "none")
                << " | State " << toString(m_runtime->state())
                << " | " << (m_playbackRunning ? "playing" : "paused")
                << " | prompts=" << m_runtime->visiblePrompts().size();
            drawTextLine(dc, footerRect, footer.str(), RGB(68, 75, 82));
        }
        else
        {
            drawTextLine(dc, footerRect, atlasLabel + " | Family " + std::string(previewFamilyName(previewFamily)) + " | Layout " + std::string(layoutEvidence ? layoutEvidence->layoutId : "none") + " | Run Host to preview runtime layers and prompt rows.", RGB(68, 75, 82));
        }
    }

    void populateGroups()
    {
        SendMessageA(m_groupList, LB_RESETCONTENT, 0, 0);
        for (const auto& group : m_groups)
        {
            std::ostringstream label;
            label << group.groupDisplayName << " [" << group.priority << "] (" << group.hostIndices.size() << ")";
            const std::string labelText = label.str();
            SendMessageA(m_groupList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(labelText.c_str()));
        }

        if (!m_groups.empty())
        {
            SendMessageA(m_groupList, LB_SETCURSEL, 0, 0);
            populateHosts(0);
        }
    }

    void populateHosts(std::size_t groupIndex)
    {
        m_visibleHostIndices = m_groups[groupIndex].hostIndices;
        SendMessageA(m_hostList, LB_RESETCONTENT, 0, 0);
        for (const auto hostIndex : m_visibleHostIndices)
        {
            const std::string label = hostDisplayLabel(*m_hosts[hostIndex].metadata);
            SendMessageA(m_hostList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }

        if (!m_visibleHostIndices.empty())
        {
            SendMessageA(m_hostList, LB_SETCURSEL, 0, 0);
            m_selectedHostIndex = m_visibleHostIndices.front();
            syncAssetGalleryToSelectedHost();
            syncCsdElementToSelectedHost();
            showHostSummary();
        }
    }

    void handleCommand(int id, int notification)
    {
        if (id == kGroupListId && notification == LBN_SELCHANGE)
        {
            const auto selected = SendMessageA(m_groupList, LB_GETCURSEL, 0, 0);
            if (selected >= 0)
                populateHosts(static_cast<std::size_t>(selected));
            return;
        }

        if (id == kHostListId && notification == LBN_SELCHANGE)
        {
            const auto selected = SendMessageA(m_hostList, LB_GETCURSEL, 0, 0);
            if (selected >= 0 && static_cast<std::size_t>(selected) < m_visibleHostIndices.size())
            {
                m_selectedHostIndex = m_visibleHostIndices[static_cast<std::size_t>(selected)];
                syncAssetGalleryToSelectedHost();
                syncCsdElementToSelectedHost();
                showHostSummary();
            }
            return;
        }

        if (notification != BN_CLICKED)
            return;

        if (id == kRunButtonId || id == kResetButtonId)
            runSelectedHost();
        else if (id == kPlayPauseButtonId)
            togglePlayback();
        else if (id == kStepButtonId)
            stepPlaybackFrame();
        else if (id == kPreviewModeButtonId)
            togglePreviewMode();
        else if (id == kAssetPrevButtonId)
            shiftAssetGallery(-1);
        else if (id == kAssetNextButtonId)
            shiftAssetGallery(1);
        else if (id == kCsdPrevButtonId)
            shiftCsdElementBinding(-1);
        else if (id == kCsdNextButtonId)
            shiftCsdElementBinding(1);
        else if (id == kMoveNextButtonId)
            requestAction(InputAction::MoveNext);
        else if (id == kConfirmButtonId)
            requestAction(InputAction::Confirm);
        else if (id == kCancelButtonId)
            requestAction(InputAction::Cancel);
    }

    void togglePreviewMode()
    {
        m_previewMode = m_previewMode == PreviewMode::Runtime ? PreviewMode::Asset : PreviewMode::Runtime;
        updatePreviewModeButton();
        showHostSummary();
        if (m_previewPanel)
            InvalidateRect(m_previewPanel, nullptr, TRUE);
    }

    void showHostSummary()
    {
        if (!m_selectedHostIndex.has_value())
            return;

        const auto& host = *m_hosts[*m_selectedHostIndex].metadata;
        std::ostringstream text;
        text
            << "Selected host:\r\n"
            << host.hostDisplayName << "\r\n\r\n"
            << "Group: " << host.groupDisplayName << "\r\n"
            << "Path: " << host.relativeSourcePath << "\r\n"
            << "Contract: " << host.primaryContractFileName << "\r\n";

        if (const auto* candidate = atlasCandidateForContract(host.primaryContractFileName))
            text << "Atlas candidate: " << atlasCandidateDisplayName(*candidate) << "\r\n\r\n";
        else
            text << "Atlas candidate: none\r\n\r\n";

        text
            << assetViewerSummaryText(host.primaryContractFileName) << "\r\n"
            << layoutCsdElementBindingNavigationSummary(host.primaryContractFileName, currentCsdElementIndex(host)) << "\r\n"
            << layoutCsdElementBindingSummary(host.primaryContractFileName) << "\r\n"
            << assetGallerySummaryText(host) << "\r\n"
            << csdElementGallerySummaryText(host) << "\r\n"
            << host.notes << "\r\n\r\n"
            << "Click Run Host to drive this contract through the runtime.";
        SetWindowTextA(m_detailText, text.str().c_str());
        if (m_previewPanel)
            InvalidateRect(m_previewPanel, nullptr, TRUE);
    }

    void runSelectedHost()
    {
        if (!m_selectedHostIndex.has_value())
            return;

        stopPlayback();
        const auto& resolved = m_hosts[*m_selectedHostIndex];
        const auto& entry = m_contracts[resolved.contractIndex];
        const auto* host = resolved.metadata;

        m_log.clear();
        appendLogLine("Run Host: " + std::string(host->hostDisplayName));

        m_runtime = std::make_unique<ScreenRuntime>(
            entry.contract,
            RuntimeCallbacks{
                .onStateEntered = [this](ScreenState state) { appendLogLine("Enter " + std::string(toString(state))); },
                .onStateChanged = [this](ScreenState from, ScreenState to)
                {
                    appendLogLine("Transition " + std::string(toString(from)) + " -> " + std::string(toString(to)));
                },
                .onInputBlocked = [this](InputAction action) { appendLogLine("Blocked input: " + std::string(toString(action))); },
                .onSceneRequested = [this](std::string_view scene) { appendLogLine("Request scene: " + std::string(scene)); },
            });

        m_runningContractIndex = resolved.contractIndex;
        enableAllPromptPredicates(*m_runtime, entry.contract);
        m_runtime->dispatch(RuntimeEventType::ResourcesReady);
        startPlayback();
        updateRuntimeDetails();
    }

    void requestAction(InputAction action)
    {
        if (!m_runtime || !m_runningContractIndex.has_value() || !m_selectedHostIndex.has_value())
            return;

        appendLogLine("Action: " + std::string(toString(action)));
        if (m_runtime->requestAction(action))
            startPlayback();
        updateRuntimeDetails();
    }

    void togglePlayback()
    {
        if (m_playbackRunning)
            stopPlayback();
        else
            startPlayback();
    }

    void stepPlaybackFrame()
    {
        stopPlayback();
        tickPlaybackFrame(kPlaybackTickSeconds);
    }

    void startPlayback()
    {
        if (!m_runtime || !m_window || m_runtime->state() == ScreenState::Closed)
        {
            updatePlaybackButton();
            return;
        }

        if (!m_playbackRunning)
        {
            SetTimer(m_window, kPlaybackTimerId, kPlaybackTimerMilliseconds, nullptr);
            m_playbackRunning = true;
            appendLogLine("Playback: start");
        }
        updatePlaybackButton();
    }

    void stopPlayback()
    {
        if (m_playbackRunning && m_window)
            KillTimer(m_window, kPlaybackTimerId);

        if (m_playbackRunning)
            appendLogLine("Playback: pause");

        m_playbackRunning = false;
        updatePlaybackButton();
    }

    void updatePlaybackButton()
    {
        if (m_playPauseButton)
            SetWindowTextA(m_playPauseButton, m_playbackRunning ? "Pause" : "Play");
    }

    void updatePreviewModeButton()
    {
        if (m_previewModeButton)
            SetWindowTextA(m_previewModeButton, m_previewMode == PreviewMode::Asset ? "Runtime View" : "Asset View");
    }

    [[nodiscard]] double activeTimelineDuration() const
    {
        if (!m_runtime || !m_runningContractIndex.has_value())
            return 0.0;

        const auto& contract = m_contracts[*m_runningContractIndex].contract;
        return timelineDuration(contract, m_runtime->state());
    }

    void tickPlaybackFrame(double deltaSeconds)
    {
        if (!m_runtime || !m_runningContractIndex.has_value())
        {
            stopPlayback();
            return;
        }

        m_runtime->tick(deltaSeconds);
        if (m_runtime->state() == ScreenState::Closed || activeTimelineDuration() <= 0.0)
            stopPlayback();

        updateRuntimeDetails();
    }

    void updateRuntimeDetails()
    {
        if (!m_runtime || !m_runningContractIndex.has_value() || !m_selectedHostIndex.has_value())
            return;

        const auto& entry = m_contracts[*m_runningContractIndex];
        const auto& host = *m_hosts[*m_selectedHostIndex].metadata;
        SetWindowTextA(m_detailText, snapshotText(*m_runtime, entry, host).c_str());
        SetWindowTextA(m_logText, m_log.c_str());
        if (m_previewPanel)
            InvalidateRect(m_previewPanel, nullptr, TRUE);
    }

    void appendLogLine(const std::string& line)
    {
        m_log += line;
        m_log += "\r\n";
        if (m_logText)
            SetWindowTextA(m_logText, m_log.c_str());
    }

    HINSTANCE m_instance = nullptr;
    HWND m_window = nullptr;
    HWND m_groupLabel = nullptr;
    HWND m_groupList = nullptr;
    HWND m_hostLabel = nullptr;
    HWND m_hostList = nullptr;
    HWND m_runButton = nullptr;
    HWND m_moveNextButton = nullptr;
    HWND m_confirmButton = nullptr;
    HWND m_cancelButton = nullptr;
    HWND m_resetButton = nullptr;
    HWND m_playPauseButton = nullptr;
    HWND m_stepButton = nullptr;
    HWND m_previewModeButton = nullptr;
    HWND m_assetPrevButton = nullptr;
    HWND m_assetNextButton = nullptr;
    HWND m_csdPrevButton = nullptr;
    HWND m_csdNextButton = nullptr;
    HWND m_previewPanel = nullptr;
    HWND m_detailText = nullptr;
    HWND m_logText = nullptr;

    std::vector<ContractEntry> m_contracts;
    std::vector<ResolvedHostEntry> m_hosts;
    std::vector<GroupEntry> m_groups;
    std::vector<std::size_t> m_visibleHostIndices;
    std::optional<std::size_t> m_selectedHostIndex;
    std::optional<std::size_t> m_assetGalleryIndex;
    std::optional<std::size_t> m_csdElementIndex;
    std::optional<std::size_t> m_runningContractIndex;
    std::unique_ptr<ScreenRuntime> m_runtime;
    PreviewMode m_previewMode = PreviewMode::Runtime;
    bool m_playbackRunning = false;
    std::string m_log;
};
} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR commandLine, int showCommand)
{
    try
    {
        const std::string command = commandLine ? commandLine : "";
        if (command.find("--layer-fill-smoke") != std::string::npos)
            return runLayerFillSmoke();
        if (command.find("--layout-evidence-smoke") != std::string::npos)
            return runLayoutEvidenceSmoke();
        if (command.find("--layout-timeline-smoke") != std::string::npos)
            return runLayoutTimelineSmoke();
        if (command.find("--layout-primitive-playback-smoke") != std::string::npos)
            return runLayoutPrimitivePlaybackSmoke();
        if (command.find("--layout-primitive-detail-smoke") != std::string::npos)
            return runLayoutPrimitiveDetailSmoke();
        if (command.find("--layout-primitive-channel-legend-smoke") != std::string::npos)
            return runLayoutPrimitiveChannelLegendSmoke();
        if (command.find("--visual-parity-smoke") != std::string::npos)
            return runVisualParitySmoke();
        if (command.find("--host-readiness-smoke") != std::string::npos)
            return runHostReadinessSmoke();
        if (command.find("--renderer-blocker-smoke") != std::string::npos)
            return runRendererBlockerSmoke();
        if (command.find("--layout-channel-sample-smoke") != std::string::npos)
            return runLayoutChannelSampleSmoke();
        if (command.find("--layout-draw-command-smoke") != std::string::npos)
            return runLayoutDrawCommandSmoke();
        if (command.find("--authored-cast-transform-smoke") != std::string::npos)
            return runAuthoredCastTransformSmoke();
        if (command.find("--authored-keyframe-curve-smoke") != std::string::npos)
            return runAuthoredKeyframeCurveSmoke();
        if (command.find("--authored-keyframe-sample-smoke") != std::string::npos)
            return runAuthoredKeyframeSampleSmoke();
        if (command.find("--authored-sampled-channel-eval-smoke") != std::string::npos)
            return runAuthoredSampledChannelEvalSmoke();
        if (command.find("--authored-sampled-channel-command-smoke") != std::string::npos)
            return runAuthoredSampledChannelCommandSmoke();
        if (command.find("--authored-sampled-draw-command-smoke") != std::string::npos)
            return runAuthoredSampledDrawCommandSmoke();
        if (command.find("--authored-sampled-transform-preview-smoke") != std::string::npos)
            return runAuthoredSampledTransformPreviewSmoke();
        if (command.find("--authored-sampled-transform-smoke") != std::string::npos)
            return runAuthoredSampledTransformSmoke();
        if (command.find("--layout-primitive-channel-smoke") != std::string::npos)
            return runLayoutPrimitiveChannelSmoke();
        if (command.find("--layout-primitive-smoke") != std::string::npos)
            return runLayoutPrimitiveSmoke();
        if (command.find("--family-preview-smoke") != std::string::npos)
            return runFamilyPreviewSmoke();
        if (command.find("--motion-smoke") != std::string::npos)
            return runMotionSmoke();
        if (command.find("--playback-smoke") != std::string::npos)
            return runPlaybackSmoke();
        if (command.find("--asset-csd-navigation-smoke") != std::string::npos)
            return runAssetCsdNavigationSmoke();
        if (command.find("--asset-csd-binding-smoke") != std::string::npos)
            return runAssetCsdBindingSmoke();
        if (command.find("--asset-gallery-smoke") != std::string::npos)
            return runAssetGallerySmoke();
        if (command.find("--asset-view-smoke") != std::string::npos)
            return runAssetViewSmoke();
        if (command.find("--preview-smoke") != std::string::npos)
            return runPreviewSmoke();
        if (command.find("--smoke") != std::string::npos)
            return runSmoke();

        GdiPlusSession gdiPlus;
        WorkbenchGui app(instance);
        return app.run(showCommand);
    }
    catch (const std::exception& exception)
    {
        MessageBoxA(nullptr, exception.what(), "SWARD UI Runtime Debug GUI failed", MB_ICONERROR | MB_OK);
        return 1;
    }
}
