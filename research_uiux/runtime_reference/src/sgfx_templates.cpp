#include <sward/ui_runtime/sgfx_templates.hpp>

#include <algorithm>
#include <sstream>

namespace sward::ui_runtime
{
namespace
{
template <typename T>
std::string joinStrings(const std::vector<T>& values, std::string_view separator)
{
    std::ostringstream out;

    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i != 0)
            out << separator;
        out << values[i];
    }

    return out.str();
}

std::string firstEvent(const SgfxScreenTemplate& screenTemplate)
{
    if (screenTemplate.evidence.requiredEvents.empty())
        return "";

    return screenTemplate.evidence.requiredEvents.front();
}

const std::vector<SgfxScreenTemplate> kSgfxScreenTemplates =
{
    {
        "title-menu",
        "Title Menu / Press Start Handoff",
        "title_menu_reference.json",
        "title-menu",
        "System/GameMode/Title",
        {
            "api/SWA/System/GameMode/Title/TitleMenu.h",
            "api/SWA/System/GameMode/Title/TitleStateBase.h",
            "System/GameMode/Title/TitleStateIntro.cpp",
            "System/GameMode/Title/TitleMenu.cpp",
        },
        {
            { "logo_hold", 1.2, "portable contract; runtime latch title-menu-visible" },
            { "select_travel", 0.333333, "portable contract; title menu visual ready" },
            { "confirm_hold", 0.45, "portable contract; direct-context handoff" },
        },
        {
            { "backdrop", "scene-background", false },
            { "logo", "brand-layer", false },
            { "content", "menu-stack", true },
            { "prompt", "operator-prompt-row", false },
            { "transient_fx", "transition-fx", false },
        },
        {
            "Boot -> Intro when ui_title project is observed",
            "Intro -> Idle after title menu visual ready",
            "Idle accepts real ENTER/Start through SendInput",
            "Confirm hands off to loading/stage route",
            "Outro closes after host transition",
        },
        {
            "Expose confirm/cancel prompts only in Idle",
            "Keep input locked during visual latch and transition holds",
            "Treat cursor owner and CSD completion as route state, not render state",
        },
        {
            "Render Sonic title/menu assets as local placeholders until SGFX art exists.",
            "ui_title/ui_mainmenu Sonic title/menu CSD and local extracted atlas/DDS evidence",
            "custom SGFX title logo, menu stack, prompt row, and transition FX",
            {
                "backdrop",
                "logo",
                "menu_item_normal",
                "menu_item_selected",
                "prompt_glyphs",
                "transition_fx",
            },
            {
                "Placeholder Sonic assets stay local-only and are never shipped in the repo-safe kit.",
                "Renderer must consume slot bindings, not hardcoded Sonic atlas names.",
                "State flow and timing survive asset replacement.",
            },
        },
        {
            "real-runtime/live bridge/JSONL/native BMP",
            "title-menu",
            {
                "title-menu-visible",
                "title menu visual ready",
                "title-menu-post-press-start-ready",
            },
            {
                "JSONL durable title-menu-visible",
                "live bridge readiness",
                "native BMP title menu visual ready",
                "foreground verified SendInput ENTER",
            },
            {
                "out/ui_lab_runtime_evidence/20260428_083651/title-menu",
            },
        },
        {
            "Use SGFX menu controller with explicit Intro/Idle/Confirm/Outro bands.",
            "Swap SU logo/menu assets for SGFX brand/menu content; keep input lock timing shape.",
            "Drive readiness from host scene ownership instead of screenshots.",
        },
    },
    {
        "loading",
        "Miles Electric Loading / Host Handoff",
        "loading_transition_reference.json",
        "loading",
        "SWA.HUD.Loading",
        {
            "api/SWA/HUD/Loading/Loading.h",
            "api/SWA/Inspire/InspireScene.h",
            "api/SWA/Movie/MovieDisplayer.h",
        },
        {
            { "pda_intro", 4.5, "portable contract; loading display active" },
            { "pda_idle_loop", 4.0, "portable contract; host keeps screen open" },
            { "fade_out", 4.0, "portable contract; loading display ended" },
        },
        {
            { "backdrop", "scene-background", false },
            { "cinematic_frame", "safe-frame", false },
            { "content", "loading-device-frame", false },
            { "tip_copy", "localized-tip-copy", false },
            { "controller_variant", "input-hint-variant", false },
        },
        {
            "Boot -> Intro when ui_loading is requested",
            "Intro -> Idle after pda_intro",
            "Idle loops until host transition closes",
            "Outro fades once loading display ends",
        },
        {
            "No player input required",
            "Tip/controller rows are data-driven host variants",
            "Close is host-owned, not button-owned",
        },
        {
            "Render Sonic Miles Electric loading assets as local placeholders until SGFX art exists.",
            "ui_loading Sonic loading CSD, Inspire/Movie metadata, and local extracted atlas/DDS evidence",
            "custom SGFX loading device/frame, tip-copy system, and controller variant art",
            {
                "backdrop",
                "device_frame",
                "loading_copy",
                "tip_copy",
                "controller_variant",
                "fade_mask",
            },
            {
                "Placeholder loading art stays local-only and evidence-derived.",
                "Host transition timing must not depend on Sonic texture names.",
                "Tip and controller variant content are data slots.",
            },
        },
        {
            "real-runtime/live bridge/JSONL/native BMP",
            "loading",
            {
                "loading-display-active",
                "target-csd-project-made",
                "loading-requested",
            },
            {
                "JSONL loading-display-active",
                "native BMP NOW LOADING / loading display active",
                "CLoading.m_LoadingDisplayType inspector",
            },
            {
                "out/ui_lab_runtime_evidence/20260428_083651/loading",
            },
        },
        {
            "Use this as a generic SGFX blocking transition shell.",
            "Keep loading identity, tip copy, and controller variant as data slots.",
            "Host owns open/close; template owns intro/loop/outro timing.",
        },
    },
    {
        "sonic-hud",
        "Normal Sonic Stage HUD",
        "sonic_stage_hud_reference.json",
        "sonic-hud",
        "SWA.HUD.Sonic.CHudSonicStage",
        {
            "api/SWA/HUD/Sonic/HudSonicStage.h",
            "Player/Character/Sonic/Hud/SonicHudGuide.cpp",
            "SWA.CSD.CCsdProject",
        },
        {
            { "hud_in", 0.35, "portable contract; sonic-hud-ready" },
            { "lane_shift", 0.18, "portable contract; gauge/counter lane shift" },
            { "medal_burst", 0.45, "portable contract; transient reward burst" },
            { "hud_out", 0.25, "portable contract; host hide/close" },
        },
        {
            { "counter", "score-counter-bank", false },
            { "gauge", "resource-gauge-bank", false },
            { "sidecar", "context-side-panel", false },
            { "prompt", "operator-prompt-row", false },
            { "transient_fx", "reward-fx", false },
        },
        {
            "Boot -> Intro when ui_playscreen project is bound",
            "Intro -> Idle after hud_in",
            "Idle updates counters/gauges from host model",
            "Navigate/Confirm are transient animation lanes",
            "Outro closes or hides on host request",
        },
        {
            "Gameplay input remains host-owned",
            "HUD prompt row is optional and predicate-gated",
            "Gauge/counter ownership should be sampled from host model paths",
        },
        {
            "Render Sonic HUD assets as local placeholders until SGFX gameplay HUD art exists.",
            "ui_playscreen Sonic HUD CSD tree and local extracted atlas/DDS evidence",
            "custom SGFX gameplay counter bank, gauge bank, side panel, prompt strip, and reward FX",
            {
                "score_counter_bank",
                "ring_counter",
                "speed_gauge",
                "energy_gauge",
                "gauge_frame",
                "side_panel",
                "prompt_strip",
                "reward_fx",
            },
            {
                "Placeholder Sonic HUD assets stay local-only and can be rendered for lab previews.",
                "Runtime data binding uses typed host model adapters, not Sonic-specific asset IDs.",
                "Gauge/counter lanes must remain replaceable by SGFX game systems.",
            },
        },
        {
            "real-runtime/live bridge/JSONL/native BMP",
            "sonic-hud",
            {
                "sonic-hud-ready",
                "stage-target-csd-bound",
                "target-csd-project-made",
                "raw CHudSonicStage owner hook",
            },
            {
                "live bridge sonic-hud-ready",
                "native BMP stage target ready",
                "CCsdProject::Make traversal",
                "raw CHudSonicStage owner hook owner-only",
            },
            {
                "out/ui_lab_runtime_evidence/20260428_082902/sonic-hud",
                "out/ui_lab_runtime_evidence/20260428_083651/sonic-hud",
            },
        },
        {
            "Use this as SGFX gameplay HUD shell: counter bank, gauge bank, sidecar, prompt strip.",
            "Keep rendering independent from game-specific CSD; bind host counters/gauges through typed model adapters.",
            "Do not require embedded CHudSonicStage RCPtr slots; current proof comes from CCsdProject::Make traversal.",
        },
    },
    {
        "tutorial",
        "Tutorial / HUD Guide Route",
        "sonic_stage_hud_reference.json",
        "tutorial",
        "SonicHudGuide owner path",
        {
            "Player/Character/Sonic/Hud/SonicHudGuide.cpp",
            "api/SWA/HUD/Sonic/HudSonicStage.h",
            "api/SWA/System/InputState.h",
            "api/SWA/System/PadState.h",
        },
        {
            { "hud_in", 0.35, "portable contract; tutorial-ready" },
            { "lane_shift", 0.18, "portable contract; guide prompt swap" },
            { "hud_out", 0.25, "portable contract; host hide/close" },
        },
        {
            { "counter", "host-context-readout", false },
            { "gauge", "resource-gauge-bank", false },
            { "sidecar", "tutorial-panel", false },
            { "prompt", "control-guide-prompt-row", false },
        },
        {
            "Boot -> Intro when ui_playscreen and Sonic HUD owner are live",
            "Intro -> Idle after tutorial-hud-owner-path-ready",
            "Idle follows host guide predicates",
            "Navigate swaps guide/page prompt content",
            "Outro closes on host tutorial completion",
        },
        {
            "Input text is SGFX-localized and host-mapped",
            "Prompt visibility depends on active tutorial step",
            "Guide route must require owner-path proof, not generic HUD visibility",
        },
        {
            "Render Sonic tutorial/HUD-guide assets as local placeholders until SGFX guide art exists.",
            "SonicHudGuide/ui_playscreen prompt CSD and local extracted atlas/DDS evidence",
            "custom SGFX tutorial panel, control glyphs, prompt row, and host-context readouts",
            {
                "tutorial_panel",
                "control_glyphs",
                "prompt_row",
                "step_copy",
                "host_context_readout",
            },
            {
                "Placeholder tutorial glyphs stay local-only and must map through input-slot names.",
                "Tutorial content remains separate from HUD shell timing.",
                "Owner-path readiness stays required before rendering guide state as real.",
            },
        },
        {
            "real-runtime/live bridge/JSONL/native BMP",
            "tutorial",
            {
                "tutorial-hud-owner-path-ready",
                "tutorial-target-ready",
                "tutorial-ready",
                "sonic-hud-owner-hooked",
            },
            {
                "live bridge tutorial-ready",
                "native BMP stage target csd bound",
                "raw CHudSonicStage owner hook",
                "CCsdProject::Make traversal",
            },
            {
                "out/ui_lab_runtime_evidence/20260428_083009/tutorial",
            },
        },
        {
            "Use this as a SGFX contextual control-guide overlay.",
            "Keep tutorial text/content separate from HUD shell timing.",
            "Require real host ownership before guide panels can claim readiness.",
        },
    },
};
} // namespace

const std::vector<SgfxScreenTemplate>& sgfxScreenTemplates()
{
    return kSgfxScreenTemplates;
}

const SgfxScreenTemplate* findSgfxScreenTemplate(std::string_view id)
{
    const auto& templates = sgfxScreenTemplates();
    const auto found = std::find_if(
        templates.begin(),
        templates.end(),
        [id](const SgfxScreenTemplate& screenTemplate)
        {
            return screenTemplate.id == id || screenTemplate.primaryRuntimeTarget == id;
        });

    return found == templates.end() ? nullptr : &*found;
}

std::string formatSgfxTemplateCatalog()
{
    std::ostringstream out;
    out << "SGFX template catalog\n";
    out << "templates=" << sgfxScreenTemplates().size() << '\n';

    for (const auto& screenTemplate : sgfxScreenTemplates())
    {
        out << screenTemplate.id
            << ':' << screenTemplate.contractFileName
            << ':' << firstEvent(screenTemplate)
            << ":target=" << screenTemplate.primaryRuntimeTarget
            << ":evidence=" << screenTemplate.evidence.lane
            << '\n';
    }

    return out.str();
}

std::string formatSgfxTemplateDetail(const SgfxScreenTemplate& screenTemplate)
{
    std::ostringstream out;
    out << "template=" << screenTemplate.id << '\n';
    out << "display=" << screenTemplate.displayName << '\n';
    out << "contract=" << screenTemplate.contractFileName << '\n';
    out << "runtime_target=" << screenTemplate.primaryRuntimeTarget << '\n';
    out << "source_family=" << screenTemplate.sourceFamily << '\n';
    out << "source_paths=" << joinStrings(screenTemplate.recoveredSourcePaths, ",") << '\n';
    out << "render_intent=" << screenTemplate.assetPolicy.renderIntent << '\n';
    out << "placeholder_assets=" << screenTemplate.assetPolicy.placeholderAssetFamily << '\n';
    out << "final_assets=" << screenTemplate.assetPolicy.finalAssetFamily << '\n';
    out << "asset_slots=" << joinStrings(screenTemplate.assetPolicy.swappableSlots, ",") << '\n';
    out << "asset_constraints=" << joinStrings(screenTemplate.assetPolicy.constraints, " | ") << '\n';
    out << "events=" << joinStrings(screenTemplate.evidence.requiredEvents, ",") << '\n';
    out << "proof=" << joinStrings(screenTemplate.evidence.proofArtifacts, ",") << '\n';
    out << "state_flow=" << joinStrings(screenTemplate.stateFlow, " | ") << '\n';
    out << "adaptation=" << joinStrings(screenTemplate.sgfxAdaptationNotes, " | ") << '\n';
    return out.str();
}
} // namespace sward::ui_runtime
