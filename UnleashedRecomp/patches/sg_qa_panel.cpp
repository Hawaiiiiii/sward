#include "sg_qa_panel.h"

#include <patches/sg_asset_overrides.h>
#include <patches/sg_pack.h>
#include <patches/sg_preflight_state.h>
#include <patches/sg_text_overrides.h>
#include <patches/ui_lab_patches.h>

#include <imgui.h>

#include <atomic>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>

namespace
{
    enum class Mode { Auto, ForcedOff, ForcedOn };

    static Mode ResolveMode()
    {
        if (const char* env = std::getenv("SG_PREFLIGHT_QA_PANEL");
            env != nullptr && env[0] != '\0')
        {
            std::string sv(env);
            for (char& c : sv)
            {
                if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
            }
            if (sv == "0" || sv == "false" || sv == "off")
                return Mode::ForcedOff;
            return Mode::ForcedOn;
        }
        return Mode::Auto;
    }

    static bool ShouldDraw()
    {
        const auto mode = ResolveMode();
        if (mode == Mode::ForcedOff) return false;
        if (mode == Mode::ForcedOn)  return true;
        // Auto: panel only appears when the operator staged a
        // pack_meta.json with a ticket. Vanilla UR boots and
        // pack-meta-less SGFX runs stay free of the overlay.
        return SGPack::TryGetTicket() != nullptr;
    }

    static std::atomic<bool> g_activeEmitted{false};
    // Phase 373: separate one-shot gate for the sg-preflight state
    // panel emit. Fires the FIRST time the panel renders AFTER the
    // SGPreflightState loader has populated its fields, so a
    // deferred load (env not set at boot but state file appearing
    // later) still emits a clean panel-side proof event when the
    // panel finally surfaces it.
    static std::atomic<bool> g_preflightPanelEmitted{false};
    static uint64_t g_lastTextReloads = std::numeric_limits<uint64_t>::max();
    static uint64_t g_lastAssetReloads = std::numeric_limits<uint64_t>::max();
    static uint64_t g_lastPackReloads = std::numeric_limits<uint64_t>::max();

    static const std::string& OrPlaceholder(const std::string* p,
                                            const std::string& fallback)
    {
        return p != nullptr ? *p : fallback;
    }
}

namespace SGQAPanel
{
    void Draw()
    {
        if (!ShouldDraw()) return;

        // Pin the panel to the top-right corner. UI Lab's runtime
        // bridge status overlay lives in the top-LEFT region, so
        // this placement avoids visual collision in the operator
        // shell mode.
        const ImGuiIO& io = ImGui::GetIO();
        const float pad = 8.0f;
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x - pad, pad),
            ImGuiCond_Always,
            ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.55f);

        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration   |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings  |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoNav;

        if (!ImGui::Begin("SGFX QA Shell", nullptr, flags))
        {
            ImGui::End();
            return;
        }

        static const std::string kPlaceholder = "(none)";
        static const std::string kNoPack      = "(no pack)";

        const std::string& ticket  = OrPlaceholder(SGPack::TryGetTicket(),  kNoPack);
        const std::string& project = OrPlaceholder(SGPack::TryGetProject(), kNoPack);
        const std::string& phase   = OrPlaceholder(SGPack::TryGetPhase(),   kPlaceholder);
        const std::string& route   = OrPlaceholder(SGPack::TryGetRoute(),   kPlaceholder);

        ImGui::TextUnformatted("SGFX QA Shell");
        ImGui::Separator();
        ImGui::Text("ticket:  %s", ticket.c_str());
        ImGui::Text("project: %s", project.c_str());
        ImGui::Text("phase:   %s", phase.c_str());
        ImGui::Text("route:   %s", route.c_str());

        ImGui::Separator();
        const char* packStatus = SGPack::IsActive() ? "loaded" : "absent";
        ImGui::Text("pack:    %s", packStatus);

        const auto textReloads  = SGTextOverrides::GetReloadCount();
        const auto assetReloads = SGAssetOverrides::GetReloadCount();
        const auto packReloads  = SGPack::GetReloadCount();
        ImGui::Text("reload:  text=%llu  asset=%llu  pack=%llu",
                    static_cast<unsigned long long>(textReloads),
                    static_cast<unsigned long long>(assetReloads),
                    static_cast<unsigned long long>(packReloads));

        // Phase 373: sg-preflight state surface. Read accessors
        // each frame -- they're cheap atomic loads + pointer
        // returns. Render placeholders when the loader did not
        // pick up a state file so the operator can tell the panel
        // is alive but the bridge is missing.
        ImGui::Separator();
        if (SGPreflightState::IsLoaded())
        {
            const std::string* preflightProfile = SGPreflightState::TryGetSelectedProfile();
            const std::string* preflightSource  = SGPreflightState::TryGetSourceRoot();
            const std::string* preflightLatest  = SGPreflightState::TryGetLatestRunStatus();
            const std::string* preflightWarning = SGPreflightState::TryGetFirstWarning();
            const std::size_t  preflightActions = SGPreflightState::GetActionCount();

            ImGui::Text("preflight profile: %s",
                        preflightProfile != nullptr ? preflightProfile->c_str() : "(none)");
            ImGui::Text("preflight source:  %s",
                        preflightSource != nullptr ? preflightSource->c_str() : "(unknown)");
            ImGui::Text("actions:           %zu", preflightActions);
            ImGui::Text("latest run:        %s",
                        preflightLatest != nullptr ? preflightLatest->c_str() : "(none)");
            ImGui::Text("first warning:     %s",
                        preflightWarning != nullptr ? preflightWarning->c_str() : "(none)");
        }
        else
        {
            ImGui::TextUnformatted("preflight state:   (no state file)");
        }

        ImGui::End();

        // Phase 373: panel-side proof event for the sg-preflight
        // state surface. Fires once on the first frame where the
        // loader has populated something; if the loader never sees
        // a state file, this never fires (and the QAPanel:Active
        // event still proves the panel itself rendered).
        if (SGPreflightState::IsLoaded() &&
            !g_preflightPanelEmitted.exchange(true, std::memory_order_acq_rel))
        {
            const std::string* preflightProfile = SGPreflightState::TryGetSelectedProfile();
            const std::string  profileTag = (preflightProfile != nullptr && !preflightProfile->empty())
                ? *preflightProfile
                : std::string("_");
            const std::size_t  preflightActions = SGPreflightState::GetActionCount();
            UiLab::EmitBridgeScreenEntered(
                "QAPanel:SgPreflightState:" + profileTag +
                ":" + std::to_string(preflightActions));
        }

        if (textReloads != g_lastTextReloads ||
            assetReloads != g_lastAssetReloads ||
            packReloads != g_lastPackReloads)
        {
            g_lastTextReloads = textReloads;
            g_lastAssetReloads = assetReloads;
            g_lastPackReloads = packReloads;
            UiLab::EmitBridgeScreenEntered(
                "QAPanel:ReloadCounts:" +
                std::to_string(textReloads) + ":" +
                std::to_string(assetReloads) + ":" +
                std::to_string(packReloads));
        }

        // Emit the runtime gate exactly once -- after the first
        // successful Begin/End that actually got drawn. Capturing
        // it post-End ensures we are not racing the ShouldDraw()
        // check above (which only depends on env + pack metadata,
        // both of which are stable post-boot).
        if (!g_activeEmitted.exchange(true, std::memory_order_acq_rel))
        {
            UiLab::EmitBridgeScreenEntered(
                "QAPanel:Active:" + ticket + ":" + route);
        }
    }
}
