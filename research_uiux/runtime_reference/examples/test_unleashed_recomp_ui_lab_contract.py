import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]


class UnleashedRecompUiLabContractTests(unittest.TestCase):
    def read(self, relative_path: str) -> str:
        return (ROOT / relative_path).read_text(encoding="utf-8")

    def test_ui_lab_patch_module_is_registered_with_unleashed_recomp(self):
        self.assertTrue((ROOT / "UnleashedRecomp/patches/ui_lab_patches.h").is_file())
        self.assertTrue((ROOT / "UnleashedRecomp/patches/ui_lab_patches.cpp").is_file())

        cmake = self.read("UnleashedRecomp/CMakeLists.txt")
        self.assertIn('"patches/ui_lab_patches.cpp"', cmake)

    def test_ui_lab_command_line_is_parsed_before_runtime_boot(self):
        main = self.read("UnleashedRecomp/main.cpp")
        self.assertIn("#include <patches/ui_lab_patches.h>", main)
        self.assertIn("UiLab::ConfigureFromCommandLine(argc, argv)", main)

    def test_ui_lab_declares_real_runtime_screen_targets(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        for screen_id in [
            "TitleLoop",
            "TitleMenu",
            "TitleOptions",
            "Loading",
            "SonicHud",
            "Pause",
            "ExtraStageHud",
            "Result",
            "Status",
            "Tutorial",
            "WorldMap",
        ]:
            self.assertIn(screen_id, header)

    def test_ui_lab_separates_sonic_and_extra_stage_hud_targets(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        workbench = self.read("research_uiux/runtime_reference/include/sward/ui_runtime/debug_workbench_data.hpp")
        aspect = self.read("UnleashedRecomp/patches/aspect_ratio_patches.cpp")
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        self.assertIn("const std::array<RuntimeTarget, 11>& GetRuntimeTargets()", header)
        self.assertIn("static constexpr std::array<RuntimeTarget, 11> kRuntimeTargets", ui_lab)
        self.assertIn('{ ScreenId::SonicHud, "sonic-hud", "Sonic Stage HUD", "ui_playscreen"', ui_lab)
        self.assertIn('{ ScreenId::Pause, "pause", "Pause Menu", "ui_pause"', ui_lab)
        self.assertIn('{ ScreenId::ExtraStageHud, "extra-stage-hud", "Extra Stage / Tornado HUD", "ui_prov_playscreen"', ui_lab)
        self.assertIn('token == "prov-hud"', ui_lab)
        self.assertIn('token == "tornado-hud"', ui_lab)
        self.assertIn("Sonic Stage HUD|sonic_stage_hud", workbench)
        self.assertIn("sonic_stage_hud_reference|ui_playscreen|GameModeStageForwardTest.cpp", workbench)
        self.assertIn("Extra Stage / Tornado Defense HUD|extra_stage_hud", workbench)
        self.assertIn("extra_stage_hud_reference|ui_prov_playscreen|ui_qte|GameModeStageMotionTest.cpp", workbench)
        self.assertIn('HashStr("ui_playscreen/so_speed_gauge")', aspect)
        self.assertIn('HashStr("ui_prov_playscreen/so_speed_gauge")', aspect)
        self.assertIn('"extra-stage-hud"', script)

    def test_ui_lab_promotes_early_game_visible_targets(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        menu = self.read("UnleashedRecomp/patches/CTitleStateMenu_patches.cpp")
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        self.assertIn("TitleOptions", header)
        self.assertIn('{ ScreenId::TitleOptions, "title-options", "Title Options"', ui_lab)
        self.assertIn('token == "options"', ui_lab)
        self.assertIn("TargetRoutesThroughTitleMenu", ui_lab)
        self.assertIn("g_target == ScreenId::TitleOptions", ui_lab)
        self.assertIn("title-options-accept-injected", ui_lab)
        self.assertIn("cursorIndex = 2", ui_lab)
        self.assertIn("OptionsMenu::Open", menu)
        self.assertIn("TargetSet", script)
        self.assertIn('"early-game"', script)
        self.assertIn('@("title-loop", "title-menu", "title-options", "loading", "sonic-hud")', script)
        self.assertIn('"all"', script)

    def test_ui_lab_capture_helper_validates_early_game_alpha_routes(self):
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        self.assertIn('[string]$RoutePolicy = "direct-context"', script)
        self.assertIn("Get-UiLabRequiredEvents", script)
        self.assertIn("Test-UiLabEvidenceEvents", script)
        self.assertIn("Wait-UiLabEvidenceEvents", script)
        self.assertIn("StagePostEvidenceDelaySeconds", script)
        self.assertIn('"title-options-accept-injected"', script)
        self.assertIn('"stage-target-csd-bound"', script)
        self.assertIn('"target-csd-project-made"', script)
        self.assertIn("evidenceChecks =", script)
        self.assertIn("evidenceReady =", script)
        self.assertIn("lateCaptureReason =", script)
        self.assertIn("missingEvents =", script)
        self.assertIn("passed =", script)

    def test_ui_lab_hooks_existing_title_runtime_states(self):
        intro = self.read("UnleashedRecomp/patches/CTitleStateIntro_patches.cpp")
        menu = self.read("UnleashedRecomp/patches/CTitleStateMenu_patches.cpp")
        stage_title = self.read("UnleashedRecomp/patches/CGameModeStageTitle_patches.cpp")

        self.assertIn("#include <patches/ui_lab_patches.h>", intro)
        self.assertIn("UiLab::OnTitleStateIntroUpdate", intro)
        self.assertIn("UiLab::OnTitleIntroContext", intro)
        self.assertIn("#include <patches/ui_lab_patches.h>", menu)
        self.assertIn("UiLab::OnTitleStateMenuUpdate", menu)
        self.assertIn("#include <patches/ui_lab_patches.h>", stage_title)
        self.assertIn("UiLab::OnGameModeStageTitleContext", stage_title)
        self.assertIn("UiLab::OnTitleOwnerContext", stage_title)
        self.assertIn("ForwardTitleOwnerContext", stage_title)
        self.assertIn("BuildTitleOwnerDetail", stage_title)
        self.assertIn("BuildTitleOwnerDetail(pGameModeStageTitle, base)", stage_title)
        self.assertIn("owner_title_context", stage_title)
        self.assertIn("owner_gate568", stage_title)
        self.assertIn("title_request", stage_title)
        self.assertIn("title_transition", stage_title)
        self.assertIn("title_ctx467", stage_title)
        self.assertIn("csd_byte84", stage_title)

    def test_ui_lab_can_force_real_title_and_loading_routes(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        intro = self.read("UnleashedRecomp/patches/CTitleStateIntro_patches.cpp")
        menu = self.read("UnleashedRecomp/patches/CTitleStateMenu_patches.cpp")

        self.assertIn("void RequestRouteToCurrentTarget()", header)
        self.assertIn("bool ApplyTitleIntroStateForcing", header)
        self.assertIn("bool ApplyTitleMenuStateForcing", header)
        self.assertIn("GetRouteStatusLabel", header)
        self.assertIn("Route Selected Target", ui_lab)
        self.assertIn("UiLab::ApplyTitleIntroStateForcing", intro)
        self.assertIn("eKeyState_Start", intro)
        self.assertIn("TappedState", intro)
        self.assertIn("UiLab::ApplyTitleMenuStateForcing", menu)
        self.assertIn("suppressAccept", menu)
        self.assertIn("SuppressTitleAccept(pPadState)", menu)
        self.assertIn("m_CursorIndex = (uint32_t)forcedCursorIndex", menu)
        self.assertIn("InjectTitleAccept(pPadState)", menu)

    def test_ui_lab_records_title_menu_context_for_direct_forcing(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        menu = self.read("UnleashedRecomp/patches/CTitleStateMenu_patches.cpp")

        self.assertIn("void OnTitleMenuContext", header)
        self.assertIn("title-menu-context", ui_lab)
        self.assertIn("context_phase", ui_lab)
        self.assertIn("menu_field3c", ui_lab)
        self.assertIn("menu_field54", ui_lab)
        self.assertIn("menu_field9a", ui_lab)
        self.assertIn("UiLab::OnTitleMenuContext", menu)
        self.assertIn("ReadGuestU32", menu)
        self.assertIn("contextBase + 0x240", menu)
        self.assertIn("contextBase + 0x244", menu)

    def test_ui_lab_gates_title_menu_native_capture_on_visual_readiness(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        menu = self.read("UnleashedRecomp/patches/CTitleStateMenu_patches.cpp")
        stage_title = self.read("UnleashedRecomp/patches/CGameModeStageTitle_patches.cpp")
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")
        report = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        self.assertIn("void OnTitleOwnerContext", header)
        self.assertIn("bool ShouldHoldTitleMenuRuntime()", header)
        self.assertIn("g_titleMenuVisualReady", ui_lab)
        self.assertIn("g_titleMenuPressStartAccepted", ui_lab)
        self.assertIn("g_titleMenuTransitionPulseObserved", ui_lab)
        self.assertIn("g_titleMenuPostPressStartHeld", ui_lab)
        self.assertIn("title-press-start-accept-injected", ui_lab)
        self.assertIn("title-menu-post-press-start-held", ui_lab)
        self.assertIn("title-menu-post-press-start-ready", ui_lab)
        self.assertIn("title-menu-visible", ui_lab)
        self.assertIn("title menu visual ready", ui_lab)
        self.assertIn("source=title-menu-context", ui_lab)
        self.assertIn("ownerGate568", ui_lab)
        self.assertIn("titleRequest != 0", ui_lab)
        self.assertIn("titleTransition != 0", ui_lab)
        self.assertIn("csdByte84 != 0", ui_lab)
        self.assertIn("postPressStartMenuReady", ui_lab)
        self.assertIn("context472 == 0", ui_lab)
        self.assertIn("contextPhase == 0", ui_lab)
        self.assertIn("menuCursor != 0", ui_lab)
        self.assertIn("kTitleMenuContextVisualSettleFrames = 40", ui_lab)
        self.assertIn("stable_frames=", ui_lab)
        self.assertIn("return g_titleMenuVisualReady", ui_lab)
        self.assertIn("UiLab::OnTitleOwnerContext", stage_title)
        self.assertIn("suppressAccept = true", ui_lab)
        self.assertIn("cursorIndex = 1", ui_lab)
        self.assertIn("ShouldHoldTitleMenuRuntime", ui_lab)
        self.assertNotIn("title-menu-post-press-start-accept-injected", ui_lab)
        self.assertIn("SuppressTitleAccept(pPadState)", menu)
        self.assertIn("UiLab::ShouldHoldTitleMenuRuntime()", menu)
        self.assertIn('"title-press-start-accept-injected"', script)
        self.assertIn('"title-menu-post-press-start-held"', script)
        self.assertNotIn('"title-menu-post-press-start-accept-injected"', script)
        self.assertIn('"title-menu-post-press-start-ready"', script)
        self.assertIn('"title-menu-visible"', script)
        self.assertIn('$Route -eq "title menu visual ready"', script)
        self.assertIn('[int]$_.index', script)
        self.assertNotIn('1000 - [int]$_.index', script)
        self.assertIn("post-Press-Start/menu-ready latch", report)

    def test_ui_lab_has_always_available_imgui_runtime_inspector(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")
        ignore = self.read(".gitignore")

        self.assertIn("struct TitleIntroInspectorSnapshot", ui_lab)
        self.assertIn("struct TitleOwnerInspectorSnapshot", ui_lab)
        self.assertIn("struct TitleMenuInspectorSnapshot", ui_lab)
        self.assertIn("g_lastCsdProjectName", ui_lab)
        self.assertIn("g_lastNativeFrameCapturePath", ui_lab)
        self.assertIn("DrawRuntimeInspectorOverview", ui_lab)
        self.assertIn("DrawTitleMenuLatchInspector", ui_lab)
        self.assertIn("DrawCaptureInspector", ui_lab)
        self.assertIn("DrawTargetRouterInspector", ui_lab)
        self.assertIn("ImGui::BeginTabBar(\"ui-lab-inspector-tabs\")", ui_lab)
        self.assertIn("ImGui::BeginTabItem(\"Overview\")", ui_lab)
        self.assertIn("ImGui::BeginTabItem(\"Title/Menu\")", ui_lab)
        self.assertIn("ImGui::BeginTabItem(\"Capture\")", ui_lab)
        self.assertIn("ImGui::BeginTabItem(\"Targets\")", ui_lab)
        self.assertIn("Title menu latch predicates", ui_lab)
        self.assertIn("Reddog-style window-list pattern", report)
        self.assertIn("/UnleashedRecomp-debug-menu/", ignore)

    def test_ui_lab_ports_reddog_operator_shell_reference_features(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        video = self.read("UnleashedRecomp/gpu/video.cpp")
        report = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        self.assertIn("bool ShouldReserveF1DebugToggle()", header)
        self.assertIn("void UpdateOperatorShellToggle(bool f1Down)", header)
        self.assertIn("struct OperatorWindowEntry", ui_lab)
        self.assertIn("g_operatorShellVisible", ui_lab)
        self.assertIn("g_operatorWindowListVisible", ui_lab)
        self.assertIn("DrawOperatorDebugIcon", ui_lab)
        self.assertIn("DrawOperatorWindowList", ui_lab)
        self.assertIn("DrawOperatorCounterWindow", ui_lab)
        self.assertIn("DrawOperatorViewWindow", ui_lab)
        self.assertIn("DrawOperatorExportsWindow", ui_lab)
        self.assertIn("DrawOperatorDebugDrawWindow", ui_lab)
        self.assertIn("DrawOperatorDebugDrawLayer", ui_lab)
        self.assertIn("operator-shell-f1-toggle", ui_lab)
        self.assertIn("Config::ShowFPS", ui_lab)
        self.assertIn("Config::EnableEventCollisionDebugView", ui_lab)
        self.assertIn("Config::AllowCancellingUnleash", ui_lab)
        self.assertIn("ImGui::GetForegroundDrawList", ui_lab)
        self.assertIn("SWARD Operator Window List", ui_lab)
        self.assertIn("SWARD Counter", ui_lab)
        self.assertIn("SWARD View", ui_lab)
        self.assertIn("SWARD Exports", ui_lab)
        self.assertIn("SWARD Debug Draw", ui_lab)
        self.assertIn("UiLab::ShouldReserveF1DebugToggle()", video)
        self.assertIn("UiLab::UpdateOperatorShellToggle(toggleProfiler)", video)
        self.assertIn("toggleProfiler = false", video)
        self.assertIn("F1-toggled operator shell", report)
        self.assertIn("draggable debug icon", report)
        self.assertIn("counter/view/export/debug-draw windows", report)

    def test_ui_lab_operator_shell_defaults_to_internal_runtime_views(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        self.assertIn("g_operatorWindowListVisible = true", ui_lab)
        self.assertIn("g_operatorCounterVisible = true", ui_lab)
        self.assertIn("g_operatorViewVisible = true", ui_lab)
        self.assertIn("g_operatorExportsVisible = true", ui_lab)
        self.assertIn("g_operatorDebugDrawVisible = true", ui_lab)
        self.assertIn("g_operatorWelcomeVisible = true", ui_lab)
        self.assertIn("g_operatorStageHudVisible = true", ui_lab)
        self.assertIn("g_operatorLiveApiVisible = true", ui_lab)
        self.assertIn("DrawOperatorWelcomeWindow", ui_lab)
        self.assertIn("DrawOperatorStageHudWindow", ui_lab)
        self.assertIn("DrawOperatorLiveApiWindow", ui_lab)
        self.assertIn("SWARD Welcome", ui_lab)
        self.assertIn("SWARD Stage / HUD", ui_lab)
        self.assertIn("SWARD Live API", ui_lab)
        self.assertIn("Default-open operator windows", report)

    def test_ui_lab_operator_reads_debug_menu_guest_globals(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        self.assertIn("#include <kernel/memory.h>", ui_lab)
        self.assertIn("struct GuestBoolRef", ui_lab)
        self.assertIn("DrawGuestBoolCheckbox", ui_lab)
        self.assertIn("ReadGuestBool", ui_lab)
        self.assertIn("WriteGuestBool", ui_lab)
        self.assertIn("ms_IsRenderHud", ui_lab)
        self.assertIn("0x8328BB26", ui_lab)
        self.assertIn("ms_IsRenderGameMainHud", ui_lab)
        self.assertIn("0x8328BB27", ui_lab)
        self.assertIn("ms_IsRenderDebugDraw", ui_lab)
        self.assertIn("0x8328BB23", ui_lab)
        self.assertIn("ms_IsRenderDebugDrawText", ui_lab)
        self.assertIn("0x8328BB25", ui_lab)
        self.assertIn("ms_IsCollisionRender", ui_lab)
        self.assertIn("0x833678A6", ui_lab)
        self.assertIn("ms_IsObjectCollisionRender", ui_lab)
        self.assertIn("0x83367905", ui_lab)
        self.assertIn("ms_IsTriggerRender", ui_lab)
        self.assertIn("0x83367904", ui_lab)
        self.assertIn("direct guest-memory debug globals", report)

    def test_ui_lab_writes_live_state_snapshot_for_direct_operator_reads(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")
        report = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        self.assertIn("void WriteLiveStateSnapshot()", header)
        self.assertIn("WriteLiveStateSnapshot", ui_lab)
        self.assertIn("ui_lab_live_state.json", ui_lab)
        self.assertIn("liveStatePath", script)
        self.assertIn("ui_lab_live_state.json", script)
        self.assertIn('"target"', ui_lab)
        self.assertIn('"route"', ui_lab)
        self.assertIn('"stageGameModeAddress"', ui_lab)
        self.assertIn('"nativeCaptureStatus"', ui_lab)
        self.assertIn("live-state-json", report)

    def test_ui_lab_stage_hud_operator_and_ready_events(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")
        report = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        self.assertIn("void OnStageTargetReady", header)
        self.assertIn("g_lastStageGameModeAddress", ui_lab)
        self.assertIn("g_lastStageContextFrame", ui_lab)
        self.assertIn("g_lastStageReadyEventName", ui_lab)
        self.assertIn("StageReadyEventName", ui_lab)
        self.assertIn("EmitStageTargetReadyIfNeeded", ui_lab)
        self.assertIn("stage-harness-selected", ui_lab)
        self.assertIn("sonic-hud-ready", ui_lab)
        self.assertIn("tutorial-ready", ui_lab)
        self.assertIn("result-ready", ui_lab)
        self.assertIn("stage-target-ready", ui_lab)
        self.assertIn('"sonic-hud-ready"', script)
        self.assertIn('"tutorial-ready"', script)
        self.assertIn('"result-ready"', script)
        self.assertIn("Stage/HUD operator", report)

    def test_ui_lab_has_experimental_direct_context_route_policy(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        menu = self.read("UnleashedRecomp/patches/CTitleStateMenu_patches.cpp")
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        self.assertIn("bool& directContext", header)
        self.assertIn("RoutePolicy::DirectContext", ui_lab)
        self.assertIn("--ui-lab-route-policy", ui_lab)
        self.assertIn("title-intro-direct-state-requested", ui_lab)
        self.assertIn("title-intro-context", ui_lab)
        self.assertIn("stage-title-context", ui_lab)
        self.assertIn("ownerDetail", ui_lab)
        self.assertIn("title-menu-direct-context-requested", ui_lab)
        self.assertIn("direct context requested", ui_lab)
        self.assertIn("RequestTitleIntroDirectState", intro := self.read("UnleashedRecomp/patches/CTitleStateIntro_patches.cpp"))
        self.assertIn("__imp__sub_825811C8(ctx, base)", intro)
        self.assertIn("titleStateGuestAddress", intro)
        self.assertIn("PPC_STORE_U8(titleContextGuestAddress + 0x181, 1)", intro)
        self.assertIn("PPC_STORE_U8(titleContextGuestAddress + 0x238, 1)", intro)
        self.assertIn("PPC_STORE_U8(titleContextGuestAddress + 0x1D1, 1)", intro)
        self.assertIn("ShouldArmTitleIntroOwnerOutput", header)
        self.assertIn("ShouldArmTitleIntroCsdCompletion", header)
        self.assertIn("TargetShouldRouteThroughLoading(g_target)", ui_lab)
        self.assertIn("g_target == ScreenId::TitleMenu", ui_lab)
        self.assertIn("ArmTitleIntroCsdCompletion", intro)
        self.assertIn("PPC_LOAD_U32(titleContextGuestAddress + 0x1E8)", intro)
        self.assertIn("PPC_STORE_U8(csdSceneGuestAddress + 84, 1)", intro)
        self.assertIn("OnTitleIntroDirectStateApplied", header)
        self.assertIn("transition_armed", ui_lab)
        self.assertIn("output_armed", ui_lab)
        self.assertIn("csd_complete_armed", ui_lab)
        self.assertIn("OnTitleIntroContext", header)
        self.assertIn("OnGameModeStageTitleContext", header)
        self.assertIn("title-intro-direct-state-applied", ui_lab)
        self.assertIn("directContext", menu)
        self.assertIn("ApplyDirectTitleMenuContext", menu)
        self.assertIn("m_Field9A = true", menu)
        self.assertIn("RoutePolicy", script)
        self.assertIn("--ui-lab-route-policy", script)

    def test_ui_lab_stage_harness_observes_real_stage_loading_exit(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        video = self.read("UnleashedRecomp/gpu/video.cpp")

        self.assertIn("void OnStageExitLoading(uint32_t gameModeStageAddress = 0)", header)
        self.assertIn("GetStageHarnessLabel", header)
        self.assertIn("--ui-lab-stage", ui_lab)
        self.assertIn("stage harness armed", ui_lab)
        self.assertIn("CGameModeStage::ExitLoading", ui_lab)
        self.assertIn("const uint32_t stageGameModeAddress = ctx.r3.u32", video)
        self.assertIn("UiLab::OnStageExitLoading(stageGameModeAddress)", video)

    def test_ui_lab_binds_stage_targets_to_observed_real_csd_project(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")

        self.assertIn("GetTargetCsdStatusLabel", header)
        self.assertIn("g_targetCsdObserved", ui_lab)
        self.assertIn("g_loggedStageTargetCsdBound", ui_lab)
        self.assertIn("RefreshTargetCsdProjectStatus", ui_lab)
        self.assertIn("stage_address=", ui_lab)
        self.assertIn("target_csd_observed=", ui_lab)
        self.assertIn("stage-target-csd-bound", ui_lab)
        self.assertIn("target-csd-project-made", ui_lab)
        self.assertIn("Target CSD:", ui_lab)

    def test_ui_lab_stage_targets_route_through_real_loading_path(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")

        self.assertIn("TargetShouldRouteThroughLoading", ui_lab)
        self.assertIn("TargetNeedsStageHarness(id)", ui_lab)
        self.assertIn("stage route via new game", ui_lab)
        self.assertIn("stage accept injected", ui_lab)

    def test_ui_lab_writes_runtime_evidence_log(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        video = self.read("UnleashedRecomp/gpu/video.cpp")

        self.assertIn("void OnPresentedFrame()", header)
        self.assertIn("--ui-lab-evidence-dir", ui_lab)
        self.assertIn("--ui-lab-auto-exit", ui_lab)
        self.assertIn("WriteEvidenceEvent", ui_lab)
        self.assertIn("ui_lab_events.jsonl", ui_lab)
        self.assertIn("UiLab::OnPresentedFrame()", video)

    def test_ui_lab_can_write_native_backbuffer_captures(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        video = self.read("UnleashedRecomp/gpu/video.cpp")
        d3d12 = self.read("UnleashedRecomp/gpu/rhi/plume_d3d12.cpp")
        vulkan = self.read("UnleashedRecomp/gpu/rhi/plume_vulkan.cpp")
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")
        build_script = self.read("research_uiux/runtime_reference/tools/build_unleashed_recomp_ui_lab.ps1")

        self.assertIn("ConsumeNativeFrameCapturePath", header)
        self.assertIn("IsNativeFrameCaptureEnabled", header)
        self.assertIn("OnNativeFrameCaptured", header)
        self.assertIn("--ui-lab-native-capture", ui_lab)
        self.assertIn("--ui-lab-native-capture-dir", ui_lab)
        self.assertIn("bool IsNativeFrameCaptureEnabled()", ui_lab)
        self.assertIn("native-frame-captured", ui_lab)
        self.assertIn("native-frame-capture-failed", ui_lab)
        self.assertIn("QueueUiLabNativeFrameCapture", video)
        self.assertIn("WriteUiLabNativeFrameBmp", video)
        self.assertIn("RenderBufferDesc::ReadbackBuffer", video)
        self.assertIn("RenderTextureLayout::COPY_SOURCE", video)
        self.assertIn("UiLab::IsNativeFrameCaptureEnabled()", video)
        self.assertIn("g_intermediaryBackBufferTexture.get(),", video)
        self.assertIn("RenderTextureLayout::SHADER_READ", video)
        self.assertNotIn("QueueUiLabNativeFrameCapture(\n                commandList.get(),\n                swapChainTexture", video)
        self.assertIn("commandFenceAlreadyWaited", video)
        self.assertIn("g_commandListStates[g_frame] = !commandFenceAlreadyWaited", video)
        self.assertIn("UiLab::ConsumeNativeFrameCapturePath", video)
        self.assertIn("UiLab::OnNativeFrameCaptured", video)
        self.assertIn("samplePositionTexture", d3d12)
        self.assertIn("dstLocation.type == RenderTextureCopyType::PLACED_FOOTPRINT", vulkan)
        self.assertIn("srcLocation.type == RenderTextureCopyType::SUBRESOURCE", vulkan)
        self.assertIn("vkCmdCopyImageToBuffer", vulkan)
        self.assertIn("NativeCapture", script)
        self.assertIn("--ui-lab-native-capture", script)
        self.assertIn("Wait-UiLabNativeFrameCapture", script)
        self.assertIn('lateCaptureReason = "native-frame-captured"', script)
        self.assertIn("nativeFrameCapture =", script)
        self.assertIn("UnleashedRecomp\\gpu\\rhi\\plume_d3d12.cpp", build_script)
        self.assertIn("UnleashedRecomp\\gpu\\rhi\\plume_vulkan.cpp", build_script)

    def test_ui_lab_can_write_native_backbuffer_capture_series(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        self.assertIn("--ui-lab-native-capture-count", ui_lab)
        self.assertIn("--ui-lab-native-capture-interval-frames", ui_lab)
        self.assertIn("g_nativeFrameCaptureMaxCount", ui_lab)
        self.assertIn("g_nativeFrameCaptureWrittenCount", ui_lab)
        self.assertIn("g_lastNativeFrameCaptureFrame", ui_lab)
        self.assertIn("if (g_observerMode && !g_routeTargetExplicit)", ui_lab)
        self.assertIn("NativeCaptureCount", script)
        self.assertIn("NativeCaptureIntervalFrames", script)
        self.assertIn('[bool]$NativeCapture = $false', script)
        self.assertIn("Get-UiLabNativeFrameCaptures", script)
        self.assertIn("Get-BmpSignalStats", script)
        self.assertIn("Get-UiLabNativeFrameSignalSummary", script)
        self.assertIn("Get-UiLabNativeFramePreferenceScore", script)
        self.assertIn("Get-UiLabNativeCapturePlan", script)
        self.assertIn("effectiveNativeCaptureIntervalFrames", script)
        self.assertIn("effectiveNativeCaptureCount", script)
        self.assertIn("preferredScore", script)
        self.assertIn("bestRoute", script)
        self.assertIn("rgbNonBlack", script)
        self.assertIn("rgbSum", script)
        self.assertIn("alphaSum", script)
        self.assertIn("nativeFrameSignalSummary", script)
        self.assertIn("bestRgbSum", script)
        self.assertIn("RequireNativeRgbSignal", script)
        self.assertIn("nativeSignalPassed", script)
        self.assertIn("native BMP RGB signal missing", script)
        self.assertIn("$process.Refresh()", script)
        self.assertIn("SkipWindowScreenshots", script)
        self.assertIn("Prepare-UiLabWindow", script)
        self.assertIn("window-screenshots-skipped", script)
        self.assertIn("nativeFrameCaptures =", script)

    def test_ui_lab_records_loading_and_csd_context_evidence(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        resident = self.read("UnleashedRecomp/patches/resident_patches.cpp")
        aspect = self.read("UnleashedRecomp/patches/aspect_ratio_patches.cpp")

        self.assertIn("void OnLoadingRequest(uint32_t displayType)", header)
        self.assertIn("void OnLoadingUpdate(uint32_t displayType)", header)
        self.assertIn("void OnCsdProjectMade(std::string_view projectName)", header)
        self.assertIn("loading-requested", ui_lab)
        self.assertIn("loading-display-active", ui_lab)
        self.assertIn("csd-project-made", ui_lab)
        self.assertIn("g_loadingDisplayWasActive", ui_lab)
        self.assertIn("UiLab::OnLoadingRequest(ctx.r4.u32)", resident)
        self.assertIn("UiLab::OnLoadingUpdate(pLoading->m_LoadingDisplayType)", resident)
        self.assertIn("UiLab::OnCsdProjectMade(name)", aspect)

    def test_ui_lab_capture_helper_collects_screenshots_and_events(self):
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        self.assertIn("PrintWindow", script)
        self.assertIn("CopyFromScreen", script)
        self.assertIn("GetForegroundWindow", script)
        self.assertIn("GetWindowThreadProcessId", script)
        self.assertIn("Test-ForegroundBelongsToProcess", script)
        self.assertIn("GetWindowRect", script)
        self.assertIn("Test-BitmapHasSignal", script)
        self.assertIn("ProcessStartInfo", script)
        self.assertIn("ArgumentList.Add", script)
        self.assertIn("--ui-lab-evidence-dir", script)
        self.assertIn("--ui-lab-screen", script)
        self.assertIn("-split \",\"", script)
        self.assertIn("ui_lab_events.jsonl", script)

    def test_ui_lab_capture_helper_supports_long_manual_observation(self):
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        self.assertIn("ObserveSeconds", script)
        self.assertIn("SnapshotIntervalSeconds", script)
        self.assertIn("KeepRunning", script)
        self.assertIn("snapshots", script)
        self.assertIn("stillRunning", script)
        self.assertIn("processId", script)
        self.assertIn("if (-not $KeepRunning -and -not $process.HasExited)", script)

    def test_ui_lab_observer_mode_keeps_runtime_manual(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")

        self.assertIn("bool IsObserverMode()", header)
        self.assertIn("--ui-lab-observer", ui_lab)
        self.assertIn("g_observerMode", ui_lab)
        self.assertIn("g_routeTargetExplicit", ui_lab)
        self.assertIn("capture/evidence observer mode", ui_lab)
        self.assertIn("observer mode", ui_lab)
        self.assertIn("if (!g_observerMode)", ui_lab)
        self.assertIn("return g_isEnabled && !g_observerMode", ui_lab)
        self.assertIn("if (!g_hideOverlay)", ui_lab)

    def test_ui_lab_capture_helper_launches_observer_without_screen_route(self):
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        self.assertIn("Observer", script)
        self.assertIn("HideOverlay", script)
        self.assertIn("--ui-lab-observer", script)
        self.assertIn("--ui-lab-overlay", script)
        self.assertIn("manual-observer", script)
        self.assertIn("if (-not $Observer)", script)

    def test_ui_lab_bypasses_startup_prompt_blockers_for_lab_runs(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        intro = self.read("UnleashedRecomp/patches/CTitleStateIntro_patches.cpp")

        self.assertIn("bool ShouldBypassStartupPromptBlockers()", header)
        self.assertIn("UiLab::ShouldBypassStartupPromptBlockers()", intro)
        self.assertIn("return;", intro)

    def test_ui_lab_draws_inside_the_real_imgui_runtime_frame(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        video = self.read("UnleashedRecomp/gpu/video.cpp")

        self.assertIn("void DrawOverlay()", header)
        self.assertIn("void SelectPreviousTarget()", header)
        self.assertIn("void SelectNextTarget()", header)
        self.assertIn("#include <patches/ui_lab_patches.h>", video)
        self.assertIn("UiLab::DrawOverlay()", video)

    def test_ui_lab_runtime_build_helper_captures_windows_environment_fix(self):
        script = self.read("research_uiux/runtime_reference/tools/build_unleashed_recomp_ui_lab.ps1")
        self.assertIn("subst $drive", script)
        self.assertIn("Sync-TrackedRuntimeFile", script)
        self.assertIn("CMakeLists.txt", script)
        self.assertIn("resident_patches.cpp", script)
        self.assertIn("CHudPause_patches.cpp", script)
        self.assertIn("CHudSonicStage_patches.cpp", script)
        self.assertIn("CGameModeStage_patches.cpp", script)
        self.assertIn("aspect_ratio_patches.cpp", script)
        self.assertIn("CGameModeStageTitle_patches.cpp", script)
        self.assertIn("CTitleStateIntro_patches.cpp", script)
        self.assertIn("CTitleStateMenu_patches.cpp", script)
        self.assertIn("VsDevCmd.bat", script)
        self.assertIn("C:\\Program Files\\LLVM\\bin", script)
        self.assertIn("DIRECTX_DXIL_LIBRARY", script)
        self.assertIn("Patch-SdlPrefetchShim", script)

    def test_ui_lab_is_documented_as_primary_parity_lane(self):
        report = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")
        self.assertIn("UnleashedRecomp UI Lab", report)
        self.assertIn("real CSD/material/movie/render stack", report)
        self.assertIn("diagnostic sidecar", report)

    def test_ui_lab_harvests_debug_menu_fork_typed_api_surfaces(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        harvest_data = self.read("research_uiux/data/debug_menu_fork_harvest.json")
        report = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        for token in [
            "DebugMenuForkField",
            "CSD.Manager.CScene.m_MotionFrame",
            "CSD.Manager.CScene.m_MotionRepeatType",
            "SWA.CSD.CCsdProject.m_rcProject",
            "SWA.HUD.CHudSonicStage.m_rcPlayScreen",
            "SWA.HUD.CLoading.m_LoadingDisplayType",
            "SWA.HUD.CHudPause.m_Action",
            "SWA.HUD.CGeneralWindow.m_rcGeneral",
            "SWA.HUD.CSaveIcon.m_IsVisible",
            "SWA.System.GameMode.CGameModeStage",
            "SWA.System.GameMode.Title.CTitleMenu.m_CursorIndex",
            "Reddog.Manager",
            "Reddog.DebugDraw",
            "debugForkTypedFields",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "api/CSD/Manager/csdmScene.h",
            "api/SWA/CSD/CsdProject.h",
            "api/SWA/HUD/Sonic/HudSonicStage.h",
            "api/SWA/HUD/Loading/Loading.h",
            "api/SWA/HUD/Pause/HudPause.h",
            "api/SWA/HUD/GeneralWindow/GeneralWindow.h",
            "api/SWA/HUD/SaveIcon/SaveIcon.h",
            "api/SWA/System/GameMode/GameModeStage.h",
            "api/SWA/System/GameMode/Title/TitleMenu.h",
            "api/SWA/System/GameMode/Title/TitleStateBase.h",
            "ui/reddog/reddog_manager.h",
            "ui/reddog/debug_draw.h",
            "live bridge",
        ]:
            self.assertIn(token, harvest)
            self.assertIn(token, harvest_data)

        self.assertIn("Phase 116", report)
        self.assertIn("debug-menu fork-derived typed fields", report)

    def test_ui_lab_live_bridge_exposes_state_events_and_commands(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        self.assertIn("GetLiveBridgeName", header)
        self.assertIn("IsLiveBridgeEnabled", header)
        self.assertIn("BuildLiveStateJson", header)
        self.assertIn("--ui-lab-live-bridge", ui_lab)
        self.assertIn("--ui-lab-live-bridge-name", ui_lab)
        self.assertIn("StartLiveBridge", ui_lab)
        self.assertIn("UiLabLiveBridgeThread", ui_lab)
        self.assertIn("\\\\\\\\.\\\\pipe\\\\sward_ui_lab_live", ui_lab)
        self.assertIn("HandleLiveBridgeCommand", ui_lab)
        self.assertIn("capabilities", ui_lab)
        self.assertIn("recentEvents", ui_lab)
        self.assertIn("sglobals", ui_lab)
        self.assertIn("debugForkTypedFields", ui_lab)
        self.assertIn("commands", ui_lab)
        self.assertIn("route <target>", ui_lab)
        self.assertIn("set-global <name> <0|1>", ui_lab)
        self.assertIn("capture", ui_lab)
        self.assertIn("state", ui_lab)
        self.assertIn("events", ui_lab)
        self.assertIn("live-bridge-started", ui_lab)
        self.assertIn("live-bridge-command", ui_lab)
        self.assertIn("live-bridge-capture-requested", ui_lab)
        self.assertIn("LiveBridge", script)
        self.assertIn("LiveBridgeName", script)
        self.assertIn("--ui-lab-live-bridge", script)
        self.assertIn("--ui-lab-live-bridge-name", script)
        self.assertIn("liveBridgeName =", script)

    def test_ui_lab_phase145_exposes_runtime_ui_oracle_bridge_command(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        client = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "BuildUiOracleJson",
            '"uiLayerOracle"',
            '"runtimeDrawListStatus"',
            '"runtime CSD tree; GPU draw-list pending"',
            '"activeScreen"',
            '"activeScenes"',
            '"activeMotionName"',
            '"cursorOwner"',
            '"transitionBand"',
            '"inputLockState"',
            '"ui-oracle"',
            "ui-only oracle",
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn('[ValidateSet("state", "events", "route-status", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "route", "reset", "set-global", "capture", "help")]', client)
        self.assertIn("ui-oracle", harvest)

    def test_ui_lab_phase148_exposes_runtime_csd_platform_draw_list_bridge_command(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        ui_lab_header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        aspect = self.read("UnleashedRecomp/patches/aspect_ratio_patches.cpp")
        client = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "RuntimeUiDrawCall",
            "g_runtimeUiDrawCalls",
            "OnCsdPlatformDraw",
            "BuildRuntimeUiDrawListJson",
            '"uiDrawListOracle"',
            '"runtime CSD platform draw hook; GPU backend submit pending"',
            '"gpuDrawListStatus"',
            '"drawCalls"',
            '"primitive": "quad"',
            '"screenRect"',
            '"layerPath"',
            '"ui-draw-list"',
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("OnCsdPlatformDraw", ui_lab_header)
        self.assertIn("RecordUiLabCsdPlatformDraw", aspect)
        self.assertIn("UiLab::OnCsdPlatformDraw", aspect)
        self.assertIn("SWA::CCsdPlatformMirage::Draw", aspect)
        self.assertIn("SWA::CCsdPlatformMirage::DrawNoTex", aspect)
        self.assertIn('[ValidateSet("state", "events", "route-status", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "route", "reset", "set-global", "capture", "help")]', client)
        self.assertIn("Phase 148", harvest)
        self.assertIn("runtime CSD platform draw hook", harvest)
        self.assertIn("GPU backend submit pending", harvest)

    def test_ui_lab_phase150_exposes_backend_material_submit_bridge_command(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        ui_lab_header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        video = self.read("UnleashedRecomp/gpu/video.cpp")
        client = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "RuntimeGpuSubmitCall",
            "g_runtimeGpuSubmitCalls",
            "OnBackendMaterialSubmit",
            "BuildRuntimeGpuSubmitJson",
            '"gpuSubmitOracle"',
            '"render-thread material submit hook"',
            '"backendSubmitStatus"',
            '"pipelineState"',
            '"alphaBlendEnable"',
            '"texture2DDescriptorIndex"',
            '"samplerDescriptorIndex"',
            '"samplerState"',
            '"ui-gpu-submit"',
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("OnBackendMaterialSubmit", ui_lab_header)
        self.assertIn("RecordUiLabBackendMaterialSubmit", video)
        self.assertIn("UiLab::OnBackendMaterialSubmit", video)
        self.assertIn("ProcDrawPrimitive", video)
        self.assertIn("ProcDrawIndexedPrimitive", video)
        self.assertIn("ProcDrawPrimitiveUP", video)
        self.assertIn('[ValidateSet("state", "events", "route-status", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "route", "reset", "set-global", "capture", "help")]', client)
        self.assertIn("Phase 150", harvest)
        self.assertIn("render-thread material submit hook", harvest)
        self.assertIn("raw D3D12/Vulkan backend capture pending", harvest)

    def test_ui_lab_has_repo_safe_live_bridge_client_tool(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1"
        self.assertTrue(script_path.is_file())

        script = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        for token in [
            "NamedPipeClientStream",
            "sward_ui_lab_live",
            "Invoke-UiLabBridgeCommand",
            "Read-UiLabBridgeResponse",
            '[ValidateSet("state", "events", "route-status", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "route", "reset", "set-global", "capture", "help")]',
            "route <target>",
            "set-global <name> <0|1>",
            "Connect($TimeoutMilliseconds)",
            "PipeOptions.None",
            "ConvertFrom-Json",
            "AsJson",
            "Raw",
        ]:
            self.assertIn(token, script)

    def test_ui_lab_capture_helper_can_wait_on_live_bridge_readiness(self):
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        for token in [
            "[switch]$UseLiveBridgeReadiness",
            "Get-UiLabLiveBridgeState",
            "Test-UiLabLiveBridgeReadiness",
            "Wait-UiLabLiveBridgeReadiness",
            "Invoke-UiLabBridgeCommand",
            "required-events-observed-via-live-bridge",
            "required-events-timeout-via-live-bridge",
            "readinessSource =",
            "liveBridgeReadiness =",
            "liveBridgeState =",
            '"titleMenuVisible"',
            '"loadingActive"',
            '"stageTargetReady"',
            '"stageTargetReadyEvent"',
            "Wait-UiLabEvidenceEvents $target $eventsPath",
            "Test-UiLabEvidenceEvents $target $eventsPath",
        ]:
            self.assertIn(token, script)

    def test_ui_lab_phase118_exposes_route_status_and_resets_latches(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        client = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")

        for token in [
            "void ResetRouteLatchState()",
            "g_routeGeneration",
            "g_routeResetCount",
            "++g_routeGeneration",
            "++g_routeResetCount",
            "g_loggedIntroHook = false",
            "g_loggedMenuHook = false",
            "g_lastLoadingRequestType = UINT32_MAX",
            "g_lastLoadingDisplayType = UINT32_MAX",
            "g_loadingDisplayWasActive = false",
            "BuildRouteStatusJson",
            "routePending",
            "routeGeneration",
            "routeResetCount",
            "titleIntroHookObserved",
            "titleMenuHookObserved",
            "lastTitleIntroContext",
            "lastTitleMenuContext",
            "lastStageTitleContext",
            "route-status",
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn(
            '[ValidateSet("state", "events", "route-status", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "route", "reset", "set-global", "capture", "help")]',
            client,
        )

    def test_ui_lab_phase118_capture_helper_uses_unique_bridge_and_durable_title_menu_event(self):
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "[switch]$UseUniqueLiveBridgeName",
            "Get-UiLabEffectiveLiveBridgeName",
            "effectiveLiveBridgeName =",
            "sward_ui_lab_live_",
            "Wait-UiLabLiveBridgeReadiness $target $effectiveLiveBridgeName $maxEvidenceWaitSeconds $process $eventsPath",
            "Test-UiLabDurableEvidenceEvent",
            '"title-menu-visible"',
            "durableEvidenceEvent",
            "durableEvidencePassed",
            "required-events-observed-via-live-bridge-and-jsonl",
            "required-events-timeout-via-live-bridge-jsonl",
            "--ui-lab-live-bridge-name\", $effectiveLiveBridgeName",
            "liveBridgeName = if ($LiveBridge) { $effectiveLiveBridgeName } else { $null }",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 118",
            "unique live bridge pipe",
            "title-menu-visible",
            "route-status",
            "full early-game live-bridge sweep",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_live_state_promotes_debug_fork_fields_into_typed_inspectors(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "struct CsdLiveInspectorSnapshot",
            "struct LoadingLiveInspectorSnapshot",
            "struct SonicHudLiveInspectorSnapshot",
            "LoadingDisplayTypeLabel",
            "MotionRepeatTypeLabel",
            "AppendTypedInspectors",
            '"typedInspectors"',
            '"csd"',
            '"titleMenu"',
            '"loading"',
            '"sonicHud"',
            '"sceneMotionFrame"',
            '"sceneMotionRepeatType"',
            '"loadingDisplayTypeLabel"',
            '"titleMenuOwnerContextAddress"',
            '"titleMenuCursor"',
            '"hudOwnerAddress"',
            '"playScreenProject"',
            '"speedGaugeScene"',
            "CSD.Manager.CScene.m_MotionFrame",
            "SWA.HUD.CHudSonicStage.m_rcPlayScreen",
            "SWA.System.GameMode.Title.CTitleMenu.m_CursorIndex",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 117",
            "live-bridge client",
            "typedInspectors",
            "CSD scene motion frame",
            "loading display type",
            "title cursor/menu owner",
            "Sonic HUD owner fields",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase119_promotes_full_csd_tree_traversal_into_live_bridge(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        aspect = self.read("UnleashedRecomp/patches/aspect_ratio_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "struct CsdProjectTreeInspectorSnapshot",
            "struct CsdTreeEntry",
            "BuildCsdProjectTreeInspectorSnapshot",
            "AppendCsdTreeEntries",
            "OnCsdProjectTreeMade",
            "OnCsdSceneNodeTraversed",
            "OnCsdSceneTraversed",
            "OnCsdLayerTraversed",
            '"csdProjectTree"',
            '"observedProjects"',
            '"projectAddress"',
            '"rootNodeAddress"',
            '"sceneCount"',
            '"nodeCount"',
            '"layerCount"',
            '"scenes"',
            '"nodes"',
            '"layers"',
            '"runtimeSceneMotionFrame"',
            '"runtimeSceneMotionRepeatTypeLabel"',
            "CCsdProject::Make resource traversal",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "void OnCsdProjectTreeMade",
            "void OnCsdSceneNodeTraversed",
            "void OnCsdSceneTraversed",
            "void OnCsdLayerTraversed",
        ]:
            self.assertIn(token, header)

        for token in [
            "UiLab::OnCsdProjectTreeMade",
            "UiLab::OnCsdSceneNodeTraversed",
            "UiLab::OnCsdSceneTraversed",
            "UiLab::OnCsdLayerTraversed",
            "GuestAddressOf",
        ]:
            self.assertIn(token, aspect)

        for token in [
            "Phase 119",
            "full CSD project/scene/node/layer traversal",
            "CCsdProject::Make",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase134_widens_csd_tree_layer_export_samples(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")

        for token in [
            "kMaxCsdTreeEntrySamples = 512",
            "Phase 134",
            "ui_playscreen runtime tree export",
            "so_speed_gauge layer samples",
        ]:
            self.assertIn(token, ui_lab)

    def test_ui_lab_phase119_promotes_pause_general_save_inspectors(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        pause = self.read("UnleashedRecomp/patches/CHudPause_patches.cpp")
        resident = self.read("UnleashedRecomp/patches/resident_patches.cpp")
        title_menu = self.read("UnleashedRecomp/patches/CTitleStateMenu_patches.cpp")

        for token in [
            "struct PauseGeneralSaveLiveInspectorSnapshot",
            "BuildPauseGeneralSaveLiveInspectorSnapshot",
            "OnHudPauseUpdate",
            "OnGeneralWindowUpdate",
            "OnSaveIconUpdate",
            '"pauseGeneralSave"',
            '"pause"',
            '"generalWindow"',
            '"saveIcon"',
            '"pauseAddress"',
            '"pauseProjectAddress"',
            '"pauseAction"',
            '"pauseActionLabel"',
            '"generalWindowAddress"',
            '"generalProjectAddress"',
            '"generalWindowStatusLabel"',
            '"saveIconAddress"',
            '"saveIconVisible"',
            "PauseActionTypeLabel",
            "GeneralWindowStatusLabel",
            "SWA.HUD.CHudPause.m_Action",
            "SWA.HUD.CGeneralWindow.m_rcGeneral",
            "SWA.HUD.CSaveIcon.m_IsVisible",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "void OnHudPauseUpdate",
            "void OnGeneralWindowUpdate",
            "void OnSaveIconUpdate",
        ]:
            self.assertIn(token, header)

        self.assertIn("#include <patches/ui_lab_patches.h>", pause)
        self.assertIn("UiLab::OnHudPauseUpdate", pause)
        self.assertIn("UiLab::OnSaveIconUpdate", resident)
        self.assertIn("UiLab::OnGeneralWindowUpdate", title_menu)

    def test_ui_lab_phase119_exposes_sonic_hud_owner_pointer_paths(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "struct SonicHudOwnerPathInspectorSnapshot",
            "BuildSonicHudOwnerPathInspectorSnapshot",
            '"ownerPath"',
            '"chudSonicStageOwnerAddress"',
            '"ownerPointerStatus"',
            '"stageGameModeAddress"',
            '"rcPlayScreenProjectAddress"',
            '"rcSpeedGaugeSceneAddress"',
            '"rcRingEnergyGaugeSceneAddress"',
            '"rcGaugeFrameSceneAddress"',
            '"resolvedFromCsdProjectTree"',
            '"expectedOwnerFieldSource"',
            "SWA.HUD.CHudSonicStage.m_rcPlayScreen",
            "SWA.HUD.CHudSonicStage.m_rcSpeedGauge",
            "api/SWA/HUD/Sonic/HudSonicStage.h",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "CHudSonicStage owner pointer paths",
            "resolved CSD ownership",
            "raw owner pointer",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase120_hooks_raw_chud_sonic_stage_owner(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        cmake = self.read("UnleashedRecomp/CMakeLists.txt")
        sonic = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "void OnHudSonicStageUpdate",
            "uint32_t ownerAddress",
            "uint32_t playScreenProjectAddress",
            "uint32_t speedGaugeSceneAddress",
            "uint32_t ringEnergyGaugeSceneAddress",
            "uint32_t gaugeFrameSceneAddress",
            "std::string_view hookSource",
        ]:
            self.assertIn(token, header)

        for token in [
            '"patches/CHudSonicStage_patches.cpp"',
            "CHudSonicStage_patches.cpp",
        ]:
            self.assertIn(token, cmake)

        for token in [
            "#include <api/SWA.h>",
            "#include <patches/ui_lab_patches.h>",
            "SWA::CHudSonicStage",
            "IsPlausibleGuestAddress",
            "RecordHudSonicStageInspector",
            "UiLab::OnHudSonicStageUpdate",
            "GuestAddressOf(pHudSonicStage->m_rcPlayScreen.Get())",
            "GuestAddressOf(pHudSonicStage->m_rcSpeedGauge.Get())",
            "GuestAddressOf(pHudSonicStage->m_rcRingEnergyGauge.Get())",
            "GuestAddressOf(pHudSonicStage->m_rcGaugeFrame.Get())",
            "PPC_FUNC_IMPL(__imp__sub_824D89B0)",
            "PPC_FUNC_IMPL(__imp__sub_824D9308)",
            "PPC_FUNC_IMPL(__imp__sub_824D95F8)",
            "raw CHudSonicStage owner hook",
        ]:
            self.assertIn(token, sonic)

        for token in [
            "g_chudSonicStageOwnerAddress",
            "g_chudSonicStageRawHookFrame",
            "g_chudSonicStageRawHookSource",
            "sonic-hud-owner-hooked",
            '"rawOwnerKnown"',
            '"rawOwnerFieldsReady"',
            '"rawOwnerFrame"',
            '"rawHookSource"',
            "owner_fields_ready=",
            "raw CHudSonicStage owner hook live",
            "raw CHudSonicStage owner hook live; CSD owner fields pending",
            "raw CHudSonicStage owner hook pending runtime observation",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 120",
            "raw CHudSonicStage owner object hook",
            "sonic-hud-owner-hooked",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase120_adds_deterministic_pause_route_target(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        cmake = self.read("UnleashedRecomp/CMakeLists.txt")
        stage = self.read("UnleashedRecomp/patches/CGameModeStage_patches.cpp")
        pause = self.read("UnleashedRecomp/patches/CHudPause_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "Pause",
            "const std::array<RuntimeTarget, 11>& GetRuntimeTargets()",
            "bool ApplyPauseRouteInput",
        ]:
            self.assertIn(token, header)

        self.assertIn('"patches/CGameModeStage_patches.cpp"', cmake)

        for token in [
            "#include <patches/ui_lab_patches.h>",
            "PPC_FUNC_IMPL(__imp__sub_8253B7C0)",
            'UiLab::ApplyPauseRouteInput("CGameModeStage::Update pause gate sub_8253B7C0")',
            "__imp__sub_8253B7C0(ctx, base)",
        ]:
            self.assertIn(token, stage)

        for token in [
            "PPC_FUNC_IMPL(__imp__sub_824B1810)",
            "RecordHudPauseInspector(pauseAddress, pHudPause)",
            "pause transition helper sub_824B1810",
            "IsPlausibleGuestAddress",
        ]:
            self.assertIn(token, pause)

        for token in [
            "static constexpr std::array<RuntimeTarget, 11> kRuntimeTargets",
            '{ ScreenId::Pause, "pause", "Pause Menu", "ui_pause", "HUD/Pause/HudPause.cpp", true }',
            'case ScreenId::Pause:',
            'return "pause-ready"',
            "ApplyPauseRouteInput",
            "g_pauseRouteStartInjected",
            "g_pauseRouteInputHoldStartFrame",
            "pause-route-start-injected",
            "pause-route-start-retried",
            "pause-owner-observed",
            "g_loggedStageTargetReady",
            "preservePauseReadyRoute",
            "pause-route-input-source",
            "pause target ready",
            "pause-target-ready",
            "IsPauseTargetRuntimeReady",
            "native-frame-capture-complete-auto-exit",
            "RequestUiLabExit",
            "if (g_target == ScreenId::Pause)",
            "return IsPauseTargetRuntimeReady();",
            "eKeyState_Start",
            "TappedState",
            "SWA::CInputState::GetInstance()",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "deterministic pause route",
            "pause-ready",
            "pause-target-ready",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase120_capture_helper_supports_pause_live_bridge_readiness(self):
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        for token in [
            '"pause"',
            '"pause-ready"',
            '"pause-target-ready"',
            '"pause-owner-observed"',
            '"pause-route-start-injected"',
            '@("stage-context-observed", "target-csd-project-made", "stage-target-csd-bound", "pause-owner-observed", "pause-route-start-injected", "pause-target-ready", "pause-ready")',
            '@("sonic-hud", "extra-stage-hud", "tutorial", "result", "pause")',
            '$stageTargetReadyEvent -eq "pause-ready"',
            '$route -eq "pause target ready"',
            '$Csd -eq "ui_pause"',
            '$_.target -eq "pause" -and $_.route -eq "pause target ready"',
            '$effectiveNativeCaptureCount = [Math]::Max($effectiveNativeCaptureCount, 4)',
            'Get-UiLabEffectiveAutoExitSeconds',
            'return [Math]::Max($RequestedAutoExitSeconds, 95)',
            'requestedAutoExitSeconds',
            'effectiveAutoExitSeconds',
            '$pauseIndex -ge 2 -and $pauseIndex -le 4',
        ]:
            self.assertIn(token, script)

    def test_ui_lab_phase121_samples_sonic_hud_owner_maturation(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        sonic = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "void OnHudSonicStageOwnerFieldSample",
            "uint32_t ownerAddress",
            "std::string_view hookSource",
            "bool ShouldRefreshStageTitleOwnerDirectState",
            "void OnStageTitleOwnerDirectStateApplied",
        ]:
            self.assertIn(token, header)

        for token in [
            "struct SonicHudOwnerFieldSample",
            "kChudSonicStageExpectedOwnerFields",
            "sampleOffset",
            "rcObjectAddress",
            "resolvedMemoryAddress",
            "rawOwnerFieldSamples",
            "rawOwnerFieldSampleCount",
            "rawOwnerResolvedMemoryCount",
            "ownerFieldMaturationStatus",
            "fork API CHudSonicStage RCPtr slots stayed null",
            "sonic-hud-owner-field-sample",
            "stage-title-owner-direct-state-requested",
            "stage-title-owner-direct-state-applied",
            "kStageTitleOwnerDirectStateFallbackFrames",
            "g_titleIntroDirectStateApplied",
            "if (g_titleIntroDirectStateApplied)",
            "WriteEvidenceEvent(\"title-intro-direct-state-requested\"",
            "g_titleIntroDirectStateLastRequestFrame == 0",
            "g_presentedFrameCount < g_titleIntroDirectStateLastRequestFrame + kStageTitleOwnerDirectStateFallbackFrames",
            "g_stageTitleOwnerDirectStateFallbackEnabled",
            "if (!g_stageTitleOwnerDirectStateFallbackEnabled)",
            "g_titleMenuDirectContextAcceptInjected",
            "title-menu-direct-context-accept-injected",
            "shouldHoldDirectContext",
            "!g_targetCsdObserved",
            "api/SWA/HUD/Sonic/HudSonicStage.h offsets 0xE0..0xFC",
        ]:
            self.assertIn(token, ui_lab)
        self.assertNotIn("title-intro-direct-state-refreshed", ui_lab)

        stage_title = self.read("UnleashedRecomp/patches/CGameModeStageTitle_patches.cpp")
        for token in [
            "ArmStageTitleOwnerDirectState",
            "UiLab::ShouldRefreshStageTitleOwnerDirectState",
            "PPC_STORE_U8(titleContextGuestAddress + 0x181, 1)",
            "PPC_STORE_U8(titleContextGuestAddress + 0x238, 1)",
            "PPC_STORE_U8(titleContextGuestAddress + 0x1D1, 1)",
            "PPC_STORE_U8(titleCsdAddress + 84, 1)",
            "UiLab::OnStageTitleOwnerDirectStateApplied",
        ]:
            self.assertIn(token, stage_title)

        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")
        for token in [
            "[switch]$EnableStageTitleOwnerDirectFallback",
            "--ui-lab-stage-title-owner-direct-fallback",
        ]:
            self.assertIn(token, script)

        for token in [
            "UiLab::OnHudSonicStageOwnerFieldSample",
            "raw CHudSonicStage owner hook sub_824D89B0",
            "raw CHudSonicStage owner hook sub_824D9308",
            "raw CHudSonicStage owner hook sub_824D95F8",
        ]:
            self.assertIn(token, sonic)

        for token in [
            "Phase 121",
            "owner maturation",
            "raw owner field samples",
            "m_rcPlayScreen/m_rcSpeedGauge/m_rcRingEnergyGauge/m_rcGaugeFrame stayed zero",
            "stage title owner direct-state fallback waits",
            "late fallback",
            "one-shot title intro direct-state request",
            "direct-context menu handoff injects one accept pulse",
            "owner direct-state fallback is diagnostic opt-in",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase121_routes_tutorial_from_sonic_hud_owner_path(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            '{ ScreenId::Tutorial, "tutorial", "Tutorial / Control Guide", "ui_playscreen", "Player/Character/Sonic/Hud/SonicHudGuide.cpp", true }',
            "IsTutorialTargetRuntimeReady",
            "tutorial-hud-owner-path-ready",
            "tutorial-target-ready",
            "tutorial ready from SonicHudGuide owner path",
            'case ScreenId::Tutorial:',
            'return "tutorial-ready"',
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            '"tutorial" {',
            '@("stage-context-observed", "target-csd-project-made", "stage-target-csd-bound", "sonic-hud-owner-hooked", "tutorial-hud-owner-path-ready", "tutorial-target-ready", "tutorial-ready")',
            '$observedEvents.Contains("tutorial-hud-owner-path-ready")',
            '$observedEvents.Contains("tutorial-target-ready")',
            '$stageTargetReadyEvent -eq "tutorial-ready"',
            '@("sonic-hud", "tutorial")',
            'return [Math]::Max($RequestedAutoExitSeconds, 220)',
            '[int]$StageTargetRetries = 2',
            'retrying fresh runtime session after incomplete live/native evidence',
        ]:
            self.assertIn(token, script)

        for token in [
            "tutorial/HUD guide route",
            "SonicHudGuide.cpp",
            "live bridge plus native BMP",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase121_capture_helper_drives_real_mapped_controls(self):
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "[switch]$UseControlAutomation",
            "[switch]$DisableControlAutomation",
            "[string]$ControlAutomationPlan = \"early-stage-route\"",
            "public struct UiLabMouseInput",
            "SendKeyboardInput",
            "KEYEVENTF_SCANCODE",
            "function Get-UiLabVirtualKey",
            "function Send-UiLabKey",
            "function Invoke-UiLabControlAutomationTick",
            "function Start-UiLabControlAutomation",
            "VK_RETURN",
            "foregroundBefore",
            "foregroundAfter",
            "sendInputDown",
            "sendInputUp",
            '$controlAutomationTargets = @("title-menu", "title-options") + $stageTargets',
            "titleMenuVisible",
            "titleMenuVisualReady",
            '"ENTER" { return 0x0D }',
            '"W" { return 0x57 }',
            '"A" { return 0x41 }',
            '"S" { return 0x53 }',
            '"D" { return 0x44 }',
            '"Q" { return 0x51 }',
            '"E" { return 0x45 }',
            "$controlAutomationEnabled = -not $Observer -and -not $DisableControlAutomation -and (($controlAutomationTargets -contains $target) -or $UseControlAutomation)",
            "Wait-UiLabControlAutomationAwareSleep",
            "controlAutomation = $controlAutomationRecord",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 121 control automation",
            "ENTER/W/A/S/D/Q/E",
            "stage targets default to real keyboard input automation",
            "input automation is the route driver",
            "live bridge plus native BMP remain the oracle",
        ]:
            self.assertIn(token, report)


if __name__ == "__main__":
    unittest.main()
