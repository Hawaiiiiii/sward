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

        self.assertIn("const std::array<RuntimeTarget, 10>& GetRuntimeTargets()", header)
        self.assertIn("static constexpr std::array<RuntimeTarget, 10> kRuntimeTargets", ui_lab)
        self.assertIn('{ ScreenId::SonicHud, "sonic-hud", "Sonic Stage HUD", "ui_playscreen"', ui_lab)
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
        self.assertIn("BuildTitleOwnerDetail", stage_title)
        self.assertIn("BuildTitleOwnerDetail(pGameModeStageTitle, base)", stage_title)
        self.assertIn("owner_title_context", stage_title)
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
        self.assertIn("resident_patches.cpp", script)
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


if __name__ == "__main__":
    unittest.main()
