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
            "Loading",
            "SonicHud",
            "Result",
            "Status",
            "Tutorial",
            "WorldMap",
        ]:
            self.assertIn(screen_id, header)

    def test_ui_lab_hooks_existing_title_runtime_states(self):
        intro = self.read("UnleashedRecomp/patches/CTitleStateIntro_patches.cpp")
        menu = self.read("UnleashedRecomp/patches/CTitleStateMenu_patches.cpp")

        self.assertIn("#include <patches/ui_lab_patches.h>", intro)
        self.assertIn("UiLab::OnTitleStateIntroUpdate", intro)
        self.assertIn("#include <patches/ui_lab_patches.h>", menu)
        self.assertIn("UiLab::OnTitleStateMenuUpdate", menu)

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

    def test_ui_lab_stage_harness_observes_real_stage_loading_exit(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        video = self.read("UnleashedRecomp/gpu/video.cpp")

        self.assertIn("void OnStageExitLoading()", header)
        self.assertIn("GetStageHarnessLabel", header)
        self.assertIn("--ui-lab-stage", ui_lab)
        self.assertIn("stage harness armed", ui_lab)
        self.assertIn("CGameModeStage::ExitLoading", ui_lab)
        self.assertIn("UiLab::OnStageExitLoading()", video)

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

    def test_ui_lab_capture_helper_collects_screenshots_and_events(self):
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")

        self.assertIn("PrintWindow", script)
        self.assertIn("CopyFromScreen", script)
        self.assertIn("GetWindowRect", script)
        self.assertIn("Test-BitmapHasSignal", script)
        self.assertIn("ProcessStartInfo", script)
        self.assertIn("ArgumentList.Add", script)
        self.assertIn("--ui-lab-evidence-dir", script)
        self.assertIn("--ui-lab-screen", script)
        self.assertIn("-split \",\"", script)
        self.assertIn("ui_lab_events.jsonl", script)

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
