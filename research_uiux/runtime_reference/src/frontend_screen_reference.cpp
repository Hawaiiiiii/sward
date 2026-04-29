#include <sward/ui_runtime/frontend_screen_reference.hpp>

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <utility>

namespace sward::ui_runtime
{
namespace
{
template <typename T>
std::string joinStrings(const std::vector<T>& values, std::string_view separator)
{
    std::ostringstream out;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index != 0)
            out << separator;
        out << values[index];
    }
    return out.str();
}

FrontendScreenMaterialSlot materialSlot(
    std::string slotId,
    std::string textureName,
    std::string placeholderFamily,
    std::string sgfxBinding)
{
    return {
        std::move(slotId),
        std::move(textureName),
        std::move(placeholderFamily),
        std::move(sgfxBinding),
        true,
    };
}

FrontendScreenMaterialSemantics defaultMaterialSemantics()
{
    // Phase 142 material parity shorthand: blend=source-over/additive, sampler=csd-point-seam.
    return {
        "source-over/additive",
        "straight-alpha",
        "packed-rgba-gradient",
        "csd-point-seam",
        "half-pixel",
        "runtime CSD/UI layer preferred",
    };
}

FrontendScreenMediaCue mediaCue(
    std::string cueId,
    std::string cueKind,
    std::string sgfxSlot,
    std::string assetName,
    double startSeconds,
    double durationSeconds,
    std::string timingSource,
    std::string evidenceStatus,
    bool runtimeVisualProven,
    bool audioPending)
{
    return {
        std::move(cueId),
        std::move(cueKind),
        std::move(sgfxSlot),
        std::move(assetName),
        startSeconds,
        durationSeconds,
        std::move(timingSource),
        std::move(evidenceStatus),
        runtimeVisualProven,
        audioPending,
    };
}

FrontendScreenTimelineChannel timeline(
    std::string animationName,
    int sampleFrame,
    int frameCount,
    std::vector<std::string> channelRoles)
{
    return {
        std::move(animationName),
        sampleFrame,
        frameCount,
        frameCount > 0 ? static_cast<double>(frameCount) / 60.0 : 0.0,
        std::move(channelRoles),
    };
}

FrontendScreenScenePolicy scene(
    std::string sceneName,
    std::string sgfxSlot,
    std::string activationEvent,
    int renderOrder,
    int drawableCommandCount,
    int structuralCommandCount,
    int sourceFreeStructuralCommandCount,
    FrontendScreenTimelineChannel timeline,
    std::vector<std::string> statePolicy)
{
    std::string scenePath = sceneName;
    return {
        std::move(scenePath),
        std::move(sceneName),
        std::move(sgfxSlot),
        std::move(activationEvent),
        renderOrder,
        drawableCommandCount,
        structuralCommandCount,
        sourceFreeStructuralCommandCount,
        std::move(timeline),
        std::move(statePolicy),
    };
}

int totalDrawableCommands(const FrontendScreenPolicy& screen)
{
    return std::accumulate(
        screen.scenes.begin(),
        screen.scenes.end(),
        0,
        [](int total, const FrontendScreenScenePolicy& scene)
        {
            return total + scene.drawableCommandCount;
        });
}

int totalStructuralCommands(const FrontendScreenPolicy& screen)
{
    return std::accumulate(
        screen.scenes.begin(),
        screen.scenes.end(),
        0,
        [](int total, const FrontendScreenScenePolicy& scene)
        {
            return total + scene.structuralCommandCount;
        });
}

int totalSourceFreeStructuralCommands(const FrontendScreenPolicy& screen)
{
    return std::accumulate(
        screen.scenes.begin(),
        screen.scenes.end(),
        0,
        [](int total, const FrontendScreenScenePolicy& scene)
        {
            return total + scene.sourceFreeStructuralCommandCount;
        });
}

const std::vector<FrontendScreenPolicy> kFrontendScreenPolicies{
    {
        "title-menu",
        "MainMenuComposite",
        "ui_mainmenu.yncp",
        "title_menu_reference.json",
        "title-menu-visible",
        "select_travel->title menu visual ready",
        "until:title-menu-visible",
        "scene-stack:background->motion-donut->selection/content",
        defaultMaterialSemantics(),
        {
            materialSlot("backdrop", "ui_mm_base.dds", "frontend-title-menu", "background_layer"),
            materialSlot("content", "ui_mm_contentstext.dds", "frontend-title-menu", "menu_copy"),
            materialSlot("menu_parts", "ui_mm_parts1.dds", "frontend-title-menu", "chrome_and_cursor"),
            materialSlot("prompt_glyphs", "mat_start_en_001.dds", "frontend-title-menu", "input_prompt_glyphs"),
        },
        {
            mediaCue(
                "title_loop_movie",
                "movie",
                "title_backdrop_movie",
                "game/movie/evmo_title_loop.sfd",
                0.0,
                0.333333,
                "runtime-ui-layer-capture + CSD timeline",
                "visual-policy-proven",
                true,
                false),
            mediaCue(
                "title_press_start_to_menu_fade",
                "fade",
                "title_transition_fade",
                "ui_mm_parts1.dds",
                0.0,
                0.333333,
                "runtime-ui-layer-capture + CSD timeline",
                "visual-policy-proven",
                true,
                false),
            mediaCue(
                "title_menu_copy",
                "text",
                "menu_copy",
                "ui_mm_contentstext.dds",
                0.333333,
                2.0,
                "runtime-ui-layer-capture + CSD timeline",
                "visual-policy-proven",
                true,
                false),
            mediaCue(
                "title_prompt_glyphs",
                "glyph",
                "prompt_glyphs",
                "mat_start_en_001.dds",
                0.333333,
                2.0,
                "runtime-ui-layer-capture + CSD timeline",
                "visual-policy-proven",
                true,
                false),
            mediaCue(
                "title_press_start_accept_sfx",
                "sfx",
                "confirm_audio",
                "host-title-confirm-sfx",
                0.333333,
                0.0,
                "title-menu-visible latch",
                "audio-id-pending",
                false,
                true),
            mediaCue(
                "title_cursor_move_sfx",
                "sfx",
                "cursor_audio",
                "host-title-cursor-sfx",
                0.666667,
                0.0,
                "title cursor owner/menu input",
                "audio-id-pending",
                false,
                true),
        },
        {
            scene(
                "mm_bg_usual",
                "title_backdrop",
                "title-menu-visible",
                10,
                47,
                11,
                0,
                timeline("DefaultAnim", 10, 120, { "background-hold", "menu-chrome", "color-fade" }),
                {
                    "Activate after the post-Press-Start title-menu-visible latch.",
                    "The input lock stays active until the title menu visual-ready event is durable.",
                }),
            scene(
                "mm_donut_move",
                "title_motion_accent",
                "title-menu-visible",
                20,
                9,
                3,
                0,
                timeline("DefaultAnim", 10, 220, { "donut-loop", "screen-space-motion" }),
                {
                    "Loop as decorative motion behind selectable menu content.",
                    "SGFX slot controls art replacement; policy controls timing.",
                }),
            scene(
                "mm_contentsitem_select",
                "title_menu_cursor",
                "title-menu-visible",
                30,
                19,
                4,
                0,
                timeline("DefaultAnim", 10, 15, { "cursor-pop", "selection-highlight" }),
                {
                    "Bind to current title cursor/menu owner.",
                    "Confirm/cancel input is accepted only after the input lock expires.",
                }),
        },
    },
    {
        "loading",
        "LoadingComposite",
        "ui_loading.yncp",
        "loading_transition_reference.json",
        "loading-display-active",
        "pda_intro->loading display active",
        "until:loading-display-active",
        "scene-stack:pda-device->localized-copy",
        defaultMaterialSemantics(),
        {
            materialSlot("device_frame", "mat_load_comon_001.dds", "frontend-loading", "loading_device_frame"),
            materialSlot("backdrop", "mat_load_comon_001.dds", "frontend-loading", "loading_backdrop"),
            materialSlot("loading_copy", "mat_load_en_001.dds", "frontend-loading", "loading_copy"),
            materialSlot("controller_variant", "mat_comon_txt_001.dds", "frontend-loading", "controller_variant"),
        },
        {
            mediaCue(
                "loading_pda_intro_fade",
                "fade",
                "loading_intro_fade",
                "mat_load_comon_001.dds",
                0.0,
                1.25,
                "runtime-ui-layer-capture + CSD timeline",
                "visual-policy-proven",
                true,
                false),
            mediaCue(
                "loading_now_loading_copy",
                "text",
                "loading_copy",
                "mat_load_en_001.dds",
                1.25,
                3.0,
                "runtime-ui-layer-capture + CSD timeline",
                "visual-policy-proven",
                true,
                false),
            mediaCue(
                "loading_controller_glyphs",
                "glyph",
                "controller_variant",
                "mat_comon_txt_001.dds",
                1.25,
                3.0,
                "runtime-ui-layer-capture + CSD timeline",
                "visual-policy-proven",
                true,
                false),
            mediaCue(
                "loading_pda_outro_fade",
                "fade",
                "loading_outro_fade",
                "mat_load_comon_001.dds",
                4.25,
                0.75,
                "runtime-ui-layer-capture + CSD timeline",
                "visual-policy-proven",
                true,
                false),
            mediaCue(
                "loading_display_open_sfx",
                "sfx",
                "loading_open_audio",
                "host-loading-open-sfx",
                0.0,
                0.0,
                "loading-display-active latch",
                "audio-id-pending",
                false,
                true),
            mediaCue(
                "loading_display_close_sfx",
                "sfx",
                "loading_close_audio",
                "host-loading-close-sfx",
                4.25,
                0.0,
                "loading-display-ended latch",
                "audio-id-pending",
                false,
                true),
        },
        {
            scene(
                "pda",
                "loading_device",
                "loading-display-active",
                10,
                54,
                26,
                0,
                timeline("Usual_Anim_3", 75, 240, { "device-intro", "now-loading-card", "screen-wipe" }),
                {
                    "Activate from the real loading display active event.",
                    "Keep route/capture automation locked until loading-display-active is durable.",
                }),
            scene(
                "pda_txt",
                "loading_text",
                "loading-display-active",
                20,
                18,
                7,
                0,
                timeline("Usual_Anim_3", 75, 240, { "localized-loading-copy", "controller-tip-copy" }),
                {
                    "Language and tip content are swappable data slots.",
                    "Sonic assets are local placeholders; text ownership is portable.",
                }),
        },
    },
    {
        "title-options",
        "TitleOptionsReference",
        "ui_mainmenu.yncp",
        "title_menu_reference.json",
        "title-options-ready",
        "select_travel->title options visual ready",
        "until:title-options-ready",
        "scene-stack:background->options-selection",
        defaultMaterialSemantics(),
        {
            materialSlot("backdrop", "ui_mm_base.dds", "frontend-title-options", "background_layer"),
            materialSlot("option_carousel", "ui_mm_contentstext.dds", "frontend-title-options", "option_copy"),
            materialSlot("option_cursor", "ui_mm_parts1.dds", "frontend-title-options", "option_cursor"),
            materialSlot("prompt_glyphs", "mat_start_en_001.dds", "frontend-title-options", "input_prompt_glyphs"),
        },
        {},
        {
            scene(
                "mm_bg_usual",
                "options_backdrop",
                "title-options-ready",
                10,
                47,
                11,
                0,
                timeline("DefaultAnim", 50, 120, { "background-hold", "options-state-underlay" }),
                {
                    "Reuse title menu background ownership while options is the active menu owner.",
                    "Input lock remains until title-options-ready is durable.",
                }),
            scene(
                "mm_contentsitem_select",
                "options_cursor",
                "title-options-ready",
                20,
                19,
                4,
                0,
                timeline("DefaultAnim", 15, 15, { "option-cursor", "selection-flash" }),
                {
                    "Bind to current title options cursor/menu owner.",
                    "SGFX can replace option copy and cursor art without changing transition policy.",
                }),
        },
    },
    {
        "pause",
        "PauseMenuReference",
        "ui_pause.yncp",
        "pause_menu_reference.json",
        "pause-ready",
        "intro_medium->pause menu visual ready",
        "until:pause-ready",
        "scene-stack:source-free-bg->panels->selection->scrollbar",
        defaultMaterialSemantics(),
        {
            materialSlot("pause_backdrop", "mat_pause_en_001.dds", "frontend-pause", "pause_backdrop"),
            materialSlot("pause_chrome", "mat_pause_en_002.dds", "frontend-pause", "pause_chrome"),
            materialSlot("pause_content", "mat_comon_001.dds", "frontend-pause", "pause_content"),
            materialSlot("prompt_strip", "mat_ex_common_002.dds", "frontend-pause", "pause_prompt_strip"),
        },
        {},
        {
            scene(
                "bg",
                "pause_backdrop",
                "pause-ready",
                10,
                1,
                1,
                1,
                timeline("Intro_Anim", 15, 15, { "source-free structural", "full-screen-dim" }),
                {
                    "Draw source-free structural quad before textured pause layers.",
                    "This is a real CSD drawable policy, not a debug-card placeholder.",
                }),
            scene(
                "bg_1",
                "pause_panel_primary",
                "pause-ready",
                20,
                13,
                4,
                0,
                timeline("Intro_Anim", 20, 20, { "pause-panel-intro", "chrome" }),
                {
                    "Intro panel lane, ordered after the dimming background.",
                    "Local Sonic texture names are placeholder material-slot labels.",
                }),
            scene(
                "bg_1_select",
                "pause_selection",
                "pause-ready",
                30,
                4,
                1,
                0,
                timeline("Scroll_Anim", 50, 120, { "selection-scroll", "cursor-highlight" }),
                {
                    "Bind to pause submenu selection state.",
                    "Scroll animation is reusable for SGFX list/menu widgets.",
                }),
            scene(
                "bg_2",
                "pause_panel_secondary",
                "pause-ready",
                40,
                67,
                15,
                0,
                timeline("Intro_Anim", 20, 20, { "secondary-panel", "button-guide-chrome" }),
                {
                    "Secondary panel and guide structure render before content text.",
                    "Keep render order deterministic across pause submenus.",
                }),
            scene(
                "text_area",
                "pause_text_area",
                "pause-ready",
                50,
                3,
                3,
                3,
                timeline("Usual_Anim", 50, 200, { "source-free structural", "text-area-band" }),
                {
                    "Draw source-free structural text bands as quads.",
                    "SGFX owns final copy/font, while this policy owns spacing and timing.",
                }),
            scene(
                "skill_select",
                "pause_skill_select",
                "pause-ready",
                60,
                27,
                0,
                0,
                timeline("Usual_Anim", 50, 60, { "submenu-items", "skill-selection" }),
                {
                    "Reusable submenu/list policy for pause-owned selections.",
                    "Input is ignored while intro_medium lock is active.",
                }),
            scene(
                "arrow",
                "pause_arrow",
                "pause-ready",
                70,
                2,
                0,
                0,
                timeline("DefaultAnim", 50, 200, { "scroll-arrow", "navigation-hint" }),
                {
                    "Navigation hint lane, drawn above item content.",
                    "Replace glyphs with SGFX input/action symbols.",
                }),
            scene(
                "skill_scroll_bar_bg",
                "pause_scrollbar",
                "pause-ready",
                80,
                6,
                3,
                0,
                timeline("unresolved", 0, 0, { "scrollbar-structure", "static-fallback" }),
                {
                    "Structural scrollbar background remains valid even without a resolved animation.",
                    "Treat unresolved timeline as static frame zero.",
                }),
        },
    },
};
} // namespace

const std::vector<FrontendScreenPolicy>& frontendScreenPolicies()
{
    return kFrontendScreenPolicies;
}

const FrontendScreenPolicy* findFrontendScreenPolicy(std::string_view screenIdOrName)
{
    const auto& policies = frontendScreenPolicies();
    const auto found = std::find_if(
        policies.begin(),
        policies.end(),
        [screenIdOrName](const FrontendScreenPolicy& policy)
        {
            return policy.screenId == screenIdOrName || policy.screenName == screenIdOrName;
        });

    return found == policies.end() ? nullptr : &*found;
}

const FrontendScreenScenePolicy* findFrontendScreenScenePolicy(
    const FrontendScreenPolicy& screen,
    std::string_view sceneNameOrPath)
{
    const auto found = std::find_if(
        screen.scenes.begin(),
        screen.scenes.end(),
        [sceneNameOrPath](const FrontendScreenScenePolicy& scene)
        {
            return scene.sceneName == sceneNameOrPath || scene.scenePath == sceneNameOrPath;
        });

    return found == screen.scenes.end() ? nullptr : &*found;
}

FrontendRuntimeAlignment defaultFrontendRuntimeAlignment(const FrontendScreenPolicy& screen)
{
    FrontendRuntimeAlignment alignment;
    alignment.screenId = screen.screenId;
    alignment.transitionBand = screen.transitionBand;
    alignment.inputLockState = screen.inputLockTiming;
    alignment.source = "frontend_screen_reference";
    for (const auto& scene : screen.scenes)
        alignment.activeScenes.push_back(scene.sceneName);

    if (screen.screenId == "title-menu")
    {
        alignment.activeMotionName = "select_travel";
        alignment.activeFrame = 10;
        alignment.cursorOwner = "CTitleStateMenu";
    }
    else if (screen.screenId == "loading")
    {
        alignment.activeMotionName = "pda_intro";
        alignment.activeFrame = 75;
        alignment.cursorOwner = "LoadingDisplay";
    }
    else if (screen.screenId == "title-options")
    {
        alignment.activeMotionName = "select_travel";
        alignment.activeFrame = 15;
        alignment.cursorOwner = "CTitleStateMenu/options";
    }
    else if (screen.screenId == "pause")
    {
        alignment.activeMotionName = "intro_medium";
        alignment.activeFrame = 15;
        alignment.cursorOwner = "CHudPause";
    }
    else
    {
        alignment.activeMotionName = screen.transitionBand;
        alignment.activeFrame = screen.scenes.empty() ? 0 : screen.scenes.front().timeline.sampleFrame;
        alignment.cursorOwner = "unknown";
    }

    return alignment;
}

FrontendScreenTimelineSample sampleFrontendScreenTimeline(
    const FrontendScreenPolicy& screen,
    const FrontendScreenScenePolicy& scene,
    int frame)
{
    FrontendScreenTimelineSample sample;
    sample.screenId = screen.screenId;
    sample.sceneName = scene.sceneName;
    sample.animationName = scene.timeline.animationName;
    sample.frameCount = scene.timeline.frameCount;

    if (scene.timeline.frameCount <= 0)
    {
        sample.frame = 0;
        sample.progress = 0.0;
        sample.clamped = frame != 0;
        return sample;
    }

    sample.frame = std::clamp(frame, 0, scene.timeline.frameCount);
    sample.clamped = sample.frame != frame;
    sample.progress = static_cast<double>(sample.frame) / static_cast<double>(scene.timeline.frameCount);
    return sample;
}

FrontendScreenMediaCueCounts frontendScreenMediaCueCounts(const FrontendScreenPolicy& screen)
{
    FrontendScreenMediaCueCounts counts;
    for (const auto& cue : screen.mediaCues)
    {
        if (cue.cueKind == "movie")
            ++counts.movie;
        else if (cue.cueKind == "text")
            ++counts.text;
        else if (cue.cueKind == "glyph")
            ++counts.glyph;
        else if (cue.cueKind == "fade")
            ++counts.fade;
        else if (cue.cueKind == "sfx")
            ++counts.sfx;

        if (cue.runtimeVisualProven)
            ++counts.visualProven;
        if (cue.audioPending)
            ++counts.audioPending;
    }
    return counts;
}

std::string formatFrontendScreenReferenceCatalog()
{
    std::ostringstream out;
    const auto& policies = frontendScreenPolicies();

    out << "Frontend screen reference catalog\n";
    out << "screens=" << policies.size() << '\n';
    for (const auto& policy : policies)
    {
        out << "screen=" << policy.screenId
            << ":name=" << policy.screenName
            << ":layout=" << policy.layoutName
            << ":activation=" << policy.activationEvent
            << ":transition=" << policy.transitionBand
            << ":input_lock=" << policy.inputLockTiming
            << ":scenes=" << policy.scenes.size()
            << ":commands=" << totalDrawableCommands(policy)
            << ":structural=" << totalStructuralCommands(policy)
            << ":source_free=" << totalSourceFreeStructuralCommands(policy)
            << '\n';
        out << "material_semantics=" << policy.screenId
            << ":blend=" << policy.materialSemantics.blendModel
            << ":alpha=" << policy.materialSemantics.alphaModel
            << ":color=" << policy.materialSemantics.colorModel
            << ":filter=" << policy.materialSemantics.filteringModel
            << ":offset=" << policy.materialSemantics.pixelOffsetModel
            << ":oracle=" << policy.materialSemantics.oraclePolicy
            << '\n';
        out << "runtime_alignment=" << formatFrontendRuntimeAlignment(defaultFrontendRuntimeAlignment(policy)) << '\n';
        if (!policy.mediaCues.empty())
        {
            const auto mediaCounts = frontendScreenMediaCueCounts(policy);
            out << "media_timing_status=" << policy.screenId
                << ":movie=" << mediaCounts.movie
                << ":text=" << mediaCounts.text
                << ":glyph=" << mediaCounts.glyph
                << ":fade=" << mediaCounts.fade
                << ":sfx=" << mediaCounts.sfx
                << ":visual_proven=" << mediaCounts.visualProven
                << ":audio_pending=" << mediaCounts.audioPending
                << '\n';
        }
        for (const auto& scene : policy.scenes)
        {
            out << "scene=" << policy.screenId << '/' << scene.sceneName
                << ":slot=" << scene.sgfxSlot
                << ":activation=" << scene.activationEvent
                << ":order=" << scene.renderOrder
                << ":timeline=" << scene.timeline.animationName
                << ":commands=" << scene.drawableCommandCount
                << ":structural=" << scene.structuralCommandCount
                << ":source_free=" << scene.sourceFreeStructuralCommandCount
                << '\n';
        }
    }
    return out.str();
}

std::string formatFrontendScreenReferenceDetail(const FrontendScreenPolicy& screen)
{
    std::ostringstream out;
    out << "screen=" << screen.screenId << '\n';
    out << "name=" << screen.screenName << '\n';
    out << "layout=" << screen.layoutName << '\n';
    out << "contract=" << screen.referenceContract << '\n';
    out << "activation=" << screen.activationEvent << '\n';
    out << "transition=" << screen.transitionBand << '\n';
    out << "input_lock=" << screen.inputLockTiming << '\n';
    out << "render_order=" << screen.renderOrderPolicy << '\n';
    out << "material_semantics=blend=" << screen.materialSemantics.blendModel
        << ":alpha=" << screen.materialSemantics.alphaModel
        << ":color=" << screen.materialSemantics.colorModel
        << ":filter=" << screen.materialSemantics.filteringModel
        << ":offset=" << screen.materialSemantics.pixelOffsetModel
        << ":oracle=" << screen.materialSemantics.oraclePolicy
        << '\n';
    out << "runtime_alignment=" << formatFrontendRuntimeAlignment(defaultFrontendRuntimeAlignment(screen)) << '\n';
    out << "scene_count=" << screen.scenes.size() << '\n';
    out << "command_count=" << totalDrawableCommands(screen) << '\n';
    out << "structural_command_count=" << totalStructuralCommands(screen) << '\n';
    out << "source_free_structural_command_count=" << totalSourceFreeStructuralCommands(screen) << '\n';
    if (!screen.mediaCues.empty())
    {
        const auto mediaCounts = frontendScreenMediaCueCounts(screen);
        out << "media_timing_status=" << screen.screenId
            << ":movie=" << mediaCounts.movie
            << ":text=" << mediaCounts.text
            << ":glyph=" << mediaCounts.glyph
            << ":fade=" << mediaCounts.fade
            << ":sfx=" << mediaCounts.sfx
            << ":visual_proven=" << mediaCounts.visualProven
            << ":audio_pending=" << mediaCounts.audioPending
            << '\n';
    }
    for (const auto& slot : screen.materialSlots)
    {
        out << "material_slot=" << screen.screenId
            << ':' << slot.slotId
            << ':' << slot.textureName
            << ":placeholder=" << slot.placeholderFamily
            << ":sgfx=" << slot.sgfxBinding
            << ":swappable=" << (slot.swappable ? 1 : 0)
            << '\n';
    }
    for (const auto& scene : screen.scenes)
    {
        out << "scene_policy=" << screen.screenId
            << '/' << scene.sceneName
            << ":order=" << scene.renderOrder
            << ":slot=" << scene.sgfxSlot
            << ":timeline=" << scene.timeline.animationName
            << '\n';
    }
    for (const auto& cue : screen.mediaCues)
        out << formatFrontendScreenMediaCueDetail(screen, cue);
    return out.str();
}

std::string formatFrontendScreenSceneDetail(
    const FrontendScreenPolicy& screen,
    const FrontendScreenScenePolicy& scene)
{
    std::ostringstream out;
    out << "screen=" << screen.screenId << '\n';
    out << "scene=" << scene.sceneName << '\n';
    out << "slot=" << scene.sgfxSlot << '\n';
    out << "activation=" << scene.activationEvent << '\n';
    out << "render_order=" << scene.renderOrder << '\n';
    out << "commands=" << scene.drawableCommandCount << '\n';
    out << "structural=" << scene.structuralCommandCount << '\n';
    out << "source_free=" << scene.sourceFreeStructuralCommandCount << '\n';
    out << "timeline=" << scene.timeline.animationName
        << ':' << scene.timeline.sampleFrame
        << '/' << scene.timeline.frameCount
        << ':' << std::fixed << std::setprecision(3) << scene.timeline.durationSeconds
        << '\n';
    out << "timeline_roles=" << joinStrings(scene.timeline.channelRoles, ",") << '\n';
    out << "state_policy=" << joinStrings(scene.statePolicy, " | ") << '\n';
    return out.str();
}

std::string formatFrontendScreenMediaTimingCatalog()
{
    std::ostringstream out;
    for (const auto& policy : frontendScreenPolicies())
    {
        if (policy.mediaCues.empty())
            continue;

        const auto mediaCounts = frontendScreenMediaCueCounts(policy);
        out << "media_timing_status=" << policy.screenId
            << ":movie=" << mediaCounts.movie
            << ":text=" << mediaCounts.text
            << ":glyph=" << mediaCounts.glyph
            << ":fade=" << mediaCounts.fade
            << ":sfx=" << mediaCounts.sfx
            << ":visual_proven=" << mediaCounts.visualProven
            << ":audio_pending=" << mediaCounts.audioPending
            << '\n';
        for (const auto& cue : policy.mediaCues)
            out << formatFrontendScreenMediaCueDetail(policy, cue);
    }
    return out.str();
}

std::string formatFrontendScreenMediaCueDetail(
    const FrontendScreenPolicy& screen,
    const FrontendScreenMediaCue& cue)
{
    std::ostringstream out;
    out << "media_cue=" << screen.screenId
        << ':' << cue.cueId
        << ":kind=" << cue.cueKind
        << ":slot=" << cue.sgfxSlot
        << ":asset=" << cue.assetName
        << ":start=" << std::fixed << std::setprecision(3) << cue.startSeconds
        << ":duration=" << std::fixed << std::setprecision(3) << cue.durationSeconds
        << ":source=" << cue.timingSource
        << ":status=" << cue.evidenceStatus
        << '\n';
    return out.str();
}

std::string formatFrontendRuntimeAlignment(const FrontendRuntimeAlignment& alignment)
{
    std::ostringstream out;
    out << alignment.screenId
        << ":active_screen=" << alignment.screenId
        << ":active_scenes=" << joinStrings(alignment.activeScenes, ",")
        << ":motion=" << alignment.activeMotionName
        << ":frame=" << alignment.activeFrame
        << ":cursor_owner=" << alignment.cursorOwner
        << ":transition=" << alignment.transitionBand
        << ":input_lock=" << alignment.inputLockState;
    return out.str();
}
} // namespace sward::ui_runtime
