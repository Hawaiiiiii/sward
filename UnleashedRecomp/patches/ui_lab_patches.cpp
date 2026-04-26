#include <patches/ui_lab_patches.h>
#include <gpu/imgui/imgui_common.h>
#include <os/logger.h>
#include <user/config.h>
#include <string>

namespace UiLab
{
    static constexpr std::array<RuntimeTarget, 8> kRuntimeTargets =
    {{
        { ScreenId::TitleLoop, "title-loop", "Title Loop", "ui_title", "System/GameMode/Title/TitleStateIntro.cpp", false },
        { ScreenId::TitleMenu, "title-menu", "Title Menu", "ui_title", "System/GameMode/Title/TitleMenu.cpp", false },
        { ScreenId::Loading, "loading", "Loading / Miles Electric", "ui_loading", "System/Loading.cpp", false },
        { ScreenId::SonicHud, "sonic-hud", "Sonic Stage HUD", "ui_prov_playscreen", "Player/Character/Sonic/Hud/SonicMainDisplay.cpp", true },
        { ScreenId::Result, "result", "Stage Result", "ui_result", "HUD/Result/Result.cpp", true },
        { ScreenId::Status, "status", "Status / Skill Upgrade", "ui_status", "HUD/Status/Status.cpp", false },
        { ScreenId::Tutorial, "tutorial", "Tutorial / Control Guide", "ui_loading", "Player/Character/Sonic/Hud/SonicHudGuide.cpp", true },
        { ScreenId::WorldMap, "world-map", "World Map", "ui_worldmap", "System/GameMode/WorldMap/WorldMapSelect.cpp", false },
    }};

    static bool g_isEnabled = false;
    static ScreenId g_target = ScreenId::TitleLoop;
    static bool g_routePending = false;
    static bool g_titleIntroAcceptInjected = false;
    static bool g_titleMenuAcceptInjected = false;
    static bool g_stageContextObserved = false;
    static std::string_view g_routeStatus = "idle";
    static std::string g_requestedStageHarness = "auto";
    static bool g_loggedIntroHook = false;
    static bool g_loggedMenuHook = false;
    static bool g_loggedStageHarness = false;

    static const RuntimeTarget& TargetFor(ScreenId id)
    {
        for (const auto& target : kRuntimeTargets)
        {
            if (target.id == id)
                return target;
        }

        return kRuntimeTargets.front();
    }

    static size_t TargetIndexFor(ScreenId id)
    {
        for (size_t i = 0; i < kRuntimeTargets.size(); ++i)
        {
            if (kRuntimeTargets[i].id == id)
                return i;
        }

        return 0;
    }

    static void SelectTargetIndex(size_t index)
    {
        if (index >= kRuntimeTargets.size())
            index = 0;

        g_target = kRuntimeTargets[index].id;
        RequestRouteToCurrentTarget();
        const auto& target = TargetFor(g_target);
        LOGFN("SWARD UI Lab target selected: {} ({})", target.token, target.label);
    }

    static bool TargetCanRouteFromTitleIntro(ScreenId id)
    {
        return id == ScreenId::TitleMenu || id == ScreenId::Loading;
    }

    static bool TargetNeedsStageHarness(ScreenId id)
    {
        return TargetFor(id).requiresStageContext;
    }

    static bool TrySetTarget(std::string_view token)
    {
        if (token == "title")
            token = "title-loop";
        else if (token == "menu")
            token = "title-menu";
        else if (token == "hud")
            token = "sonic-hud";
        else if (token == "worldmap")
            token = "world-map";

        for (const auto& target : kRuntimeTargets)
        {
            if (target.token == token)
            {
                g_target = target.id;
                return true;
            }
        }

        return false;
    }

    void ConfigureFromCommandLine(int argc, char* argv[])
    {
        for (int i = 1; i < argc; ++i)
        {
            std::string_view arg(argv[i]);

            if (arg == "--ui-lab")
            {
                g_isEnabled = true;
                continue;
            }

            if (arg == "--ui-lab-screen")
            {
                g_isEnabled = true;

                if ((i + 1) < argc && TrySetTarget(argv[i + 1]))
                    ++i;
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-screen was provided without a known target.");

                continue;
            }

            constexpr std::string_view screenPrefix = "--ui-lab-screen=";
            if (arg.starts_with(screenPrefix))
            {
                g_isEnabled = true;
                auto token = arg.substr(screenPrefix.size());

                if (!TrySetTarget(token))
                    LOGFN_WARNING("SWARD UI Lab: unknown screen target '{}'.", std::string(token));

                continue;
            }

            constexpr std::string_view labPrefix = "--ui-lab=";
            if (arg.starts_with(labPrefix))
            {
                g_isEnabled = true;
                auto token = arg.substr(labPrefix.size());

                if (!TrySetTarget(token))
                    LOGFN_WARNING("SWARD UI Lab: unknown screen target '{}'.", std::string(token));

                continue;
            }

            if (arg == "--ui-lab-stage")
            {
                g_isEnabled = true;

                if ((i + 1) < argc)
                    g_requestedStageHarness = argv[++i];
                else
                    LOGN_WARNING("SWARD UI Lab: --ui-lab-stage was provided without a stage token.");

                continue;
            }

            constexpr std::string_view stagePrefix = "--ui-lab-stage=";
            if (arg.starts_with(stagePrefix))
            {
                g_isEnabled = true;
                g_requestedStageHarness = std::string(arg.substr(stagePrefix.size()));
            }
        }

        if (g_isEnabled)
        {
            RequestRouteToCurrentTarget();
            const auto& target = TargetFor(g_target);
            LOGFN(
                "SWARD UI Lab enabled: target={} label={} csd={} family={} stage_context={}",
                target.token,
                target.label,
                target.primaryCsdScene,
                target.sourceFamily,
                target.requiresStageContext ? "yes" : "no");

            if (target.requiresStageContext)
                LOGFN("SWARD UI Lab stage harness armed: requested_stage={}", g_requestedStageHarness);
        }
    }

    void ApplyConfigOverrides()
    {
        if (!g_isEnabled)
            return;

        Config::ShowConsole = true;
        Config::SkipIntroLogos = true;
        Config::DisableAutoSaveWarning = true;
    }

    bool IsEnabled()
    {
        return g_isEnabled;
    }

    bool ShouldBypassStartupPromptBlockers()
    {
        return g_isEnabled;
    }

    ScreenId GetTarget()
    {
        return g_target;
    }

    std::string_view GetTargetToken()
    {
        return TargetFor(g_target).token;
    }

    std::string_view GetTargetLabel()
    {
        return TargetFor(g_target).label;
    }

    std::string_view GetRouteStatusLabel()
    {
        return g_routeStatus;
    }

    std::string_view GetStageHarnessLabel()
    {
        if (!TargetNeedsStageHarness(g_target))
            return "not required";

        if (g_stageContextObserved)
            return "stage context observed";

        return g_requestedStageHarness;
    }

    const std::array<RuntimeTarget, 8>& GetRuntimeTargets()
    {
        return kRuntimeTargets;
    }

    void RequestRouteToCurrentTarget()
    {
        g_titleIntroAcceptInjected = false;
        g_titleMenuAcceptInjected = false;
        g_stageContextObserved = false;
        g_loggedStageHarness = false;

        if (g_target == ScreenId::TitleLoop)
        {
            g_routePending = false;
            g_routeStatus = "holding title loop";
            return;
        }

        g_routePending = true;
        g_routeStatus = TargetNeedsStageHarness(g_target)
            ? "stage harness armed"
            : "route pending";
    }

    void SelectPreviousTarget()
    {
        auto index = TargetIndexFor(g_target);
        SelectTargetIndex(index == 0 ? kRuntimeTargets.size() - 1 : index - 1);
    }

    void SelectNextTarget()
    {
        SelectTargetIndex((TargetIndexFor(g_target) + 1) % kRuntimeTargets.size());
    }

    void OnTitleStateIntroUpdate(float elapsedSeconds)
    {
        if (!g_isEnabled || g_loggedIntroHook)
            return;

        const auto& target = TargetFor(g_target);
        LOGFN(
            "SWARD UI Lab attached to CTitleStateIntro::Update: target={} elapsed={:.3f} csd={}",
            target.token,
            elapsedSeconds,
            target.primaryCsdScene);
        g_loggedIntroHook = true;
    }

    void OnTitleStateMenuUpdate(int32_t cursorIndex)
    {
        if (!g_isEnabled || g_loggedMenuHook)
            return;

        const auto& target = TargetFor(g_target);
        LOGFN(
            "SWARD UI Lab attached to CTitleStateMenu::Update: target={} cursor={} csd={}",
            target.token,
            cursorIndex,
            target.primaryCsdScene);
        g_loggedMenuHook = true;
    }

    void OnStageExitLoading()
    {
        if (!g_isEnabled || !TargetNeedsStageHarness(g_target))
            return;

        g_stageContextObserved = true;
        g_routeStatus = "stage context live";

        if (!g_loggedStageHarness)
        {
            const auto& target = TargetFor(g_target);
            LOGFN(
                "SWARD UI Lab stage harness observed CGameModeStage::ExitLoading: target={} requested_stage={} csd={}",
                target.token,
                g_requestedStageHarness,
                target.primaryCsdScene);
            g_loggedStageHarness = true;
        }
    }

    bool ApplyTitleIntroStateForcing(float elapsedSeconds)
    {
        if (!g_isEnabled || !g_routePending || g_titleIntroAcceptInjected)
            return false;

        if (!TargetCanRouteFromTitleIntro(g_target))
            return false;

        if (elapsedSeconds < 0.75f)
            return false;

        g_titleIntroAcceptInjected = true;
        g_routeStatus = "title accept injected";
        LOGFN("SWARD UI Lab route: injected title accept for target={}", TargetFor(g_target).token);
        return true;
    }

    bool ApplyTitleMenuStateForcing(int32_t& cursorIndex, bool& injectAccept)
    {
        injectAccept = false;

        if (!g_isEnabled)
            return false;

        if (g_target == ScreenId::TitleMenu)
        {
            if (g_routePending)
            {
                g_routePending = false;
                g_routeStatus = "title menu reached";
                LOGN("SWARD UI Lab route: title menu reached.");
            }

            return false;
        }

        if (g_target != ScreenId::Loading || !g_routePending)
            return false;

        cursorIndex = 0;
        g_routeStatus = "loading route via new game";

        if (!g_titleMenuAcceptInjected)
        {
            injectAccept = true;
            g_titleMenuAcceptInjected = true;
            g_routePending = false;
            g_routeStatus = "loading accept injected";
            LOGN("SWARD UI Lab route: forced title menu New Game accept for loading target.");
        }

        return true;
    }

    void DrawOverlay()
    {
        if (!g_isEnabled)
            return;

        const auto& target = TargetFor(g_target);
        const std::string targetLabel(target.label);
        const std::string targetToken(target.token);
        const std::string targetScene(target.primaryCsdScene);
        const std::string targetFamily(target.sourceFamily);

        ImGui::SetNextWindowPos({ 18.0f, 18.0f }, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.82f);

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing;

        if (ImGui::Begin("SWARD UI Lab", nullptr, flags))
        {
            ImGui::TextUnformatted("Runtime-backed UI Lab");
            ImGui::Separator();
            ImGui::Text("Target: %s [%s]", targetLabel.c_str(), targetToken.c_str());
            ImGui::Text("CSD scene: %s", targetScene.c_str());
            ImGui::Text("Source family: %s", targetFamily.c_str());
            ImGui::Text("Stage context: %s", target.requiresStageContext ? "required" : "not required");
            ImGui::Text("Stage harness: %s", std::string(GetStageHarnessLabel()).c_str());
            ImGui::Text("Title intro hook: %s", g_loggedIntroHook ? "attached" : "waiting");
            ImGui::Text("Title menu hook: %s", g_loggedMenuHook ? "attached" : "waiting");
            ImGui::Text("Route: %s", std::string(GetRouteStatusLabel()).c_str());
            ImGui::Text("Startup prompt blockers: %s", ShouldBypassStartupPromptBlockers() ? "bypassed" : "normal");
            ImGui::Separator();

            if (ImGui::Button("Previous Target"))
                SelectPreviousTarget();

            ImGui::SameLine();

            if (ImGui::Button("Next Target"))
                SelectNextTarget();

            ImGui::SameLine();

            if (ImGui::Button("Route Selected Target"))
                RequestRouteToCurrentTarget();

            ImGui::Separator();
            ImGui::TextUnformatted("Targets:");

            for (const auto& runtimeTarget : kRuntimeTargets)
            {
                const std::string label(runtimeTarget.label);
                const std::string token(runtimeTarget.token);
                ImGui::Text("%s %s (%s)",
                    runtimeTarget.id == g_target ? ">" : " ",
                    label.c_str(),
                    token.c_str());
            }
        }

        ImGui::End();
    }
}
