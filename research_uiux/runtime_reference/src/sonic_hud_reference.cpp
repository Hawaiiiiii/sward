#include <sward/ui_runtime/sonic_hud_reference.hpp>

#include <algorithm>
#include <iomanip>
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

SonicHudMaterialSlot materialSlot(
    std::string slotId,
    std::string textureName,
    std::string sgfxBinding)
{
    return {
        std::move(slotId),
        std::move(textureName),
        "normal-sonic-hud",
        std::move(sgfxBinding),
        true,
    };
}

SonicHudTimelineChannel timeline(
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

const SonicHudOwnerReference kOwnerReference{
    "CHudSonicStage",
    "sub_824D9308",
    "ui_playscreen",
    "sonic-hud-ready",
    13,
    209,
    167,
    "live bridge CCsdProject::Make tree + exact local ui_playscreen.yncp material export; Sonic assets are local placeholders",
};

const std::vector<SonicHudScenePolicy> kSonicHudScenePolicies{
    {
        "ui_playscreen/exp_count",
        "exp_count",
        "experience_counter",
        "stage-hud-ready",
        10,
        22,
        19,
        3,
        timeline("Intro_Anim", 20, 20, { "numeric-counter", "localized-label", "panel-intro" }),
        {
            materialSlot("experience_digits", "mat_comon_num_001.dds", "numeric_counter_atlas"),
            materialSlot("experience_label", "mat_playscreen_en_001.dds", "localized_hud_copy"),
            materialSlot("experience_panel", "mat_playscreen_001.dds", "counter_panel"),
            materialSlot("experience_accent", "mat_playscreen_002.dds", "counter_accent"),
        },
        {
            "Activate after stage-hud-ready when host exposes experience counter model.",
            "Keep Sonic texture IDs as swappable SGFX slot metadata, not runtime dependencies.",
        },
    },
    {
        "ui_playscreen/gauge_frame",
        "gauge_frame",
        "side_panel",
        "stage-hud-ready",
        20,
        20,
        17,
        3,
        timeline("total_quantity", 99, 100, { "gauge-frame", "quantity-window", "side-panel" }),
        {
            materialSlot("gauge_frame_copy", "mat_playscreen_en_001.dds", "localized_hud_copy"),
            materialSlot("gauge_frame_units", "mat_playscreen_en_003.dds", "localized_hud_copy"),
            materialSlot("gauge_frame_sheet", "ui_ps1_gauge1.dds", "gauge_frame_art"),
        },
        {
            "Draw before foreground gauge needles and readouts.",
            "Use SGFX slot side_panel for replaceable gameplay sidecar art.",
        },
    },
    {
        "ui_playscreen/player_count",
        "player_count",
        "life_icon",
        "stage-hud-ready",
        30,
        3,
        1,
        2,
        timeline("DefaultAnim", 99, 100, { "life-icon", "counter-host" }),
        {
            materialSlot("life_icon", "mat_comon_001.dds", "life_icon_art"),
        },
        {
            "Bind to host life/player-count model if present.",
            "Scene is optional for SGFX projects without lives.",
        },
    },
    {
        "ui_playscreen/ring_count",
        "ring_count",
        "ring_counter",
        "stage-hud-ready",
        40,
        1,
        0,
        1,
        timeline("DefaultAnim", 0, 0, { "counter-owner" }),
        {},
        {
            "Structural owner for the ring counter; drawable digits come through related counter scenes.",
            "Preserve it as a state node even when there is no direct material slot.",
        },
    },
    {
        "ui_playscreen/ring_get",
        "ring_get",
        "ring_counter",
        "stage-hud-ready",
        50,
        3,
        2,
        1,
        timeline("Egg_Shackle", 60, 60, { "reward-flash", "ring-pickup" }),
        {
            materialSlot("ring_get_icon", "mat_comon_002.dds", "reward_icon"),
            materialSlot("ring_get_flash", "ui_ps1_gauge1.dds", "reward_fx"),
        },
        {
            "Transient animation lane for ring pickup feedback.",
            "Can be replaced by SGFX reward event FX without changing HUD ownership.",
        },
    },
    {
        "ui_playscreen/score_count",
        "score_count",
        "score_counter",
        "stage-hud-ready",
        55,
        12,
        9,
        3,
        timeline("DefaultAnim", 99, 100, { "score-counter", "localized-label" }),
        {
            materialSlot("score_label", "mat_playscreen_en_001.dds", "localized_hud_copy"),
            materialSlot("score_digits", "mat_playscreen_001.dds", "score_digit_bank"),
        },
        {
            "Bind to host score model; keep the digit renderer data-driven.",
            "Render after structural counter owners but before gauge overlays.",
        },
    },
    {
        "ui_playscreen/so_ringenagy_gauge",
        "so_ringenagy_gauge",
        "energy_gauge",
        "stage-hud-ready",
        60,
        43,
        40,
        3,
        timeline("total_quantity", 99, 100, { "energy-gauge", "resource-fill", "gradient-quads" }),
        {
            materialSlot("energy_gauge", "ui_ps1_gauge1.dds", "resource_gauge_bank"),
        },
        {
            "Sample host energy quantity before drawing fill and cap segments.",
            "Keep material slot independent from Sonic-specific sheet coordinates.",
        },
    },
    {
        "ui_playscreen/so_speed_gauge",
        "so_speed_gauge",
        "speed_gauge",
        "stage-hud-ready",
        70,
        47,
        43,
        4,
        timeline("DefaultAnim", 99, 100, { "speed-gauge", "boost-lane", "gradient-quads" }),
        {
            materialSlot("speed_gauge", "ui_ps1_gauge1.dds", "speed_gauge_bank"),
        },
        {
            "Sample host speed/boost model and render gauge after energy lane.",
            "The SGFX slot owns art replacement while this policy owns ordering and timing.",
        },
    },
    {
        "ui_playscreen/time_count",
        "time_count",
        "timer",
        "stage-hud-ready",
        80,
        16,
        11,
        5,
        timeline("DefaultAnim", 99, 100, { "timer", "localized-label", "numeric-counter" }),
        {
            materialSlot("timer_label", "mat_playscreen_en_001.dds", "localized_hud_copy"),
            materialSlot("timer_digits", "mat_comon_num_001.dds", "numeric_counter_atlas"),
            materialSlot("timer_panel", "mat_playscreen_001.dds", "counter_panel"),
        },
        {
            "Bind to host stage timer.",
            "Treat numeric formatting as SGFX-local game logic, not Sonic layout code.",
        },
    },
    {
        "ui_playscreen/add/medal_get_m",
        "add/medal_get_m",
        "medal_counter",
        "stage-hud-ready",
        90,
        5,
        4,
        1,
        timeline("DefaultAnim", 5, 5, { "medal-get", "moon-medal" }),
        {
            materialSlot("moon_medal", "mat_comon_004.dds", "medal_icon"),
            materialSlot("medal_flash", "mat_hit_001.dds", "reward_fx"),
        },
        {
            "Transient reward lane for moon medal pickup.",
            "Can be mapped to any SGFX collectible reward.",
        },
    },
    {
        "ui_playscreen/add/medal_get_s",
        "add/medal_get_s",
        "medal_counter",
        "stage-hud-ready",
        100,
        5,
        4,
        1,
        timeline("DefaultAnim", 5, 5, { "medal-get", "sun-medal" }),
        {
            materialSlot("sun_medal", "mat_comon_003.dds", "medal_icon"),
            materialSlot("medal_flash", "mat_hit_001.dds", "reward_fx"),
        },
        {
            "Transient reward lane for sun medal pickup.",
            "Can be mapped to any SGFX collectible reward.",
        },
    },
    {
        "ui_playscreen/add/speed_count",
        "add/speed_count",
        "speed_readout",
        "stage-hud-ready",
        110,
        16,
        12,
        4,
        timeline("DefaultAnim", 99, 210, { "speed-readout", "localized-units", "numeric-counter" }),
        {
            materialSlot("speed_digits", "ui_ps1_gauge1.dds", "speed_digit_bank"),
            materialSlot("speed_units", "mat_playscreen_en_002.dds", "localized_hud_copy"),
            materialSlot("speed_hit_fx", "mat_hit_001.dds", "readout_fx"),
        },
        {
            "Bind to host speed model and render after gauge banks.",
            "Use SGFX-local numeric formatting for units and precision.",
        },
    },
    {
        "ui_playscreen/add/u_info",
        "add/u_info",
        "prompt_strip",
        "tutorial-hud-owner-path-ready",
        120,
        10,
        5,
        5,
        timeline("Intro_Anim", 20, 20, { "tutorial-prompt", "medal-readout", "guide-panel" }),
        {
            materialSlot("tutorial_digits", "mat_comon_num_001.dds", "numeric_counter_atlas"),
            materialSlot("tutorial_medal_icon", "mat_comon_003.dds", "medal_icon"),
            materialSlot("tutorial_prompt_bar", "mat_hit_001.dds", "prompt_panel"),
        },
        {
            "Activate only when the tutorial owner path is proven, not from generic HUD visibility.",
            "Map prompt copy and glyphs through SGFX input/action names.",
        },
    },
};
} // namespace

const SonicHudOwnerReference& sonicHudOwnerReference()
{
    return kOwnerReference;
}

const std::vector<SonicHudScenePolicy>& sonicHudScenePolicies()
{
    return kSonicHudScenePolicies;
}

const SonicHudScenePolicy* findSonicHudScenePolicy(std::string_view sceneNameOrPath)
{
    const auto& policies = sonicHudScenePolicies();
    const auto found = std::find_if(
        policies.begin(),
        policies.end(),
        [sceneNameOrPath](const SonicHudScenePolicy& policy)
        {
            return policy.sceneName == sceneNameOrPath || policy.scenePath == sceneNameOrPath;
        });

    return found == policies.end() ? nullptr : &*found;
}

SonicHudTimelineSample sampleSonicHudTimeline(const SonicHudScenePolicy& scene, int frame)
{
    SonicHudTimelineSample sample;
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

std::string formatSonicHudReferenceCatalog()
{
    std::ostringstream out;
    const auto& owner = sonicHudOwnerReference();
    const auto& policies = sonicHudScenePolicies();
    int drawableLayers = 0;
    for (const auto& policy : policies)
        drawableLayers += policy.drawableLayerCount;

    out << "Sonic HUD reference catalog\n";
    out << "owner=" << owner.ownerType
        << ":hook=" << owner.ownerHook
        << ":project=" << owner.projectName
        << ":ready=" << owner.readyEvent
        << '\n';
    out << "scenes=" << policies.size()
        << " runtime_layers=" << owner.runtimeLayerCount
        << " drawable_layers=" << drawableLayers
        << '\n';

    for (const auto& policy : policies)
    {
        out << "scene=" << policy.sceneName
            << ":slot=" << policy.sgfxSlot
            << ":activation=" << policy.activationEvent
            << ":order=" << policy.renderOrder
            << ":timeline=" << policy.timeline.animationName
            << ":runtime_layers=" << policy.runtimeLayerCount
            << ":drawable_layers=" << policy.drawableLayerCount
            << '\n';
    }
    return out.str();
}

std::string formatSonicHudSceneDetail(const SonicHudScenePolicy& scene)
{
    std::ostringstream out;
    out << "scene=" << scene.sceneName << '\n';
    out << "path=" << scene.scenePath << '\n';
    out << "slot=" << scene.sgfxSlot << '\n';
    out << "activation=" << scene.activationEvent << '\n';
    out << "render_order=" << scene.renderOrder << '\n';
    out << "timeline=" << scene.timeline.animationName
        << ':' << scene.timeline.sampleFrame
        << '/' << scene.timeline.frameCount
        << ':' << std::fixed << std::setprecision(3) << scene.timeline.durationSeconds
        << '\n';
    out << "timeline_roles=" << joinStrings(scene.timeline.channelRoles, ",") << '\n';
    for (const auto& slot : scene.materialSlots)
    {
        out << "material_slot=" << scene.sceneName
            << ':' << slot.slotId
            << ':' << slot.textureName
            << ":placeholder=" << slot.placeholderFamily
            << ":sgfx=" << slot.sgfxBinding
            << ":swappable=" << (slot.swappable ? 1 : 0)
            << '\n';
    }
    out << "state_policy=" << joinStrings(scene.statePolicy, " | ") << '\n';
    return out.str();
}
} // namespace sward::ui_runtime
