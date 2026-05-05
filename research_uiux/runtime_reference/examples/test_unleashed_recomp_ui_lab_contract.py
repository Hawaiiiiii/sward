import json
import subprocess
import tempfile
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
        self.assertIn("ImGui::BeginTabBar(\"sward-operator-profiler-tabs\")", ui_lab)
        self.assertIn("ImGui::BeginTabItem(\"Overview\")", ui_lab)
        self.assertIn("ImGui::BeginTabItem(\"Runtime\")", ui_lab)
        self.assertIn("ImGui::BeginTabItem(\"Title/Menu\")", ui_lab)
        self.assertIn("ImGui::BeginTabItem(\"HUD\")", ui_lab)
        self.assertIn("ImGui::BeginTabItem(\"Capture\")", ui_lab)
        self.assertIn("ImGui::BeginTabItem(\"Panels\")", ui_lab)
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
        self.assertIn("void UpdateOperatorShellToggle(bool toggleDown)", header)
        self.assertIn("struct OperatorWindowEntry", ui_lab)
        self.assertIn("g_operatorShellVisible", ui_lab)
        self.assertIn("g_operatorWindowListVisible", ui_lab)
        self.assertIn("DrawOperatorDebugIcon", ui_lab)
        self.assertIn("DrawOperatorWindowList", ui_lab)
        self.assertIn("DrawOperatorProfilerPanel", ui_lab)
        self.assertIn("SWARD Operator Profiler", ui_lab)
        self.assertIn("ImGui::PlotLines(\"Frame Time\"", ui_lab)
        self.assertIn("DrawOperatorCounterWindow", ui_lab)
        self.assertIn("DrawOperatorViewWindow", ui_lab)
        self.assertIn("DrawOperatorExportsWindow", ui_lab)
        self.assertIn("DrawOperatorDebugDrawWindow", ui_lab)
        self.assertIn("DrawOperatorDebugDrawLayer", ui_lab)
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
        self.assertIn("uiLabDockedProfiler", video)
        self.assertIn("g_uiLabProfilerWasEnabled", video)
        self.assertIn("uiLabDockedProfiler && !g_uiLabProfilerWasEnabled", video)
        self.assertIn("g_profilerVisible = !g_profilerVisible;", video)
        self.assertIn("ImGuiCond_FirstUseEver", video)
        self.assertIn("Full profiler details", video)
        self.assertIn("UiLab::DrawProfilerAddon();", video)
        self.assertNotIn("ImGuiWindowFlags_NoMove", video)
        self.assertNotIn("SDL_SCANCODE_F2", video)
        self.assertNotIn("UiLab::UpdateOperatorShellToggle(toggleOperator)", video)
        self.assertNotIn("UiLab::UpdateOperatorShellToggle(toggleProfiler)", video)
        self.assertIn("return false; // Leave F1 to DrawProfiler().", ui_lab)
        self.assertIn("This native Profiler + SWARD UI Lab workspace stays visible", ui_lab)
        self.assertIn("AnyOperatorFloatingPaneVisible", ui_lab)
        self.assertIn("ImGui::Checkbox(\"Window List\"", ui_lab)
        self.assertIn("F1 remains reserved for the native Recomp Profiler", report)
        self.assertIn("profiler-style SWARD operator panel", report)
        self.assertIn("counter/view/export/debug-draw windows", report)

    def test_ui_lab_operator_shell_defaults_to_compact_runtime_views(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        self.assertIn("Compact-on-demand operator windows", ui_lab)
        self.assertIn("g_operatorWindowListVisible = false", ui_lab)
        self.assertIn("g_operatorCounterVisible = false", ui_lab)
        self.assertIn("g_operatorViewVisible = false", ui_lab)
        self.assertIn("g_operatorExportsVisible = false", ui_lab)
        self.assertIn("g_operatorDebugDrawVisible = false", ui_lab)
        self.assertIn("g_operatorWelcomeVisible = false", ui_lab)
        self.assertIn("g_operatorStageHudVisible = false", ui_lab)
        self.assertIn("g_operatorLiveApiVisible = false", ui_lab)
        self.assertIn("g_operatorDebugDrawLayerVisible = false", ui_lab)
        self.assertIn("DrawOperatorWelcomeWindow", ui_lab)
        self.assertIn("DrawOperatorStageHudWindow", ui_lab)
        self.assertIn("DrawOperatorLiveApiWindow", ui_lab)
        self.assertIn("SWARD Welcome", ui_lab)
        self.assertIn("SWARD Stage / HUD", ui_lab)
        self.assertIn("SWARD Live API", ui_lab)
        self.assertIn("profiler-style SWARD operator panel", report)

    def test_ui_lab_embeds_yncp_native_component_map_browser(self):
        header_path = ROOT / "UnleashedRecomp/patches/ui_lab_yncp_native_component_map.generated.h"
        self.assertTrue(header_path.is_file())
        header = header_path.read_text(encoding="utf-8")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        build_script = self.read("research_uiux/runtime_reference/tools/build_unleashed_recomp_ui_lab.ps1")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")

        for token in [
            "namespace UiLab::GeneratedYncPNativeComponentMap",
            "struct Project",
            "struct Scene",
            "kProjects",
            "kScenes",
            "kGeneratedAt",
            "ui_title",
            "game/Title/ui_title.yncp",
            "ui_playscreen",
            "game/Sonic/ui_playscreen.yncp",
            "so_speed_gauge",
            "ui_worldmap",
            "game/WorldMap/ui_worldmap.yncp",
            "sfx-correlation-pending",
            "runtime-derived-native-reconstruction",
        ]:
            self.assertIn(token, header)

        for token in [
            '#include "ui_lab_yncp_native_component_map.generated.h"',
            "DrawOperatorUiProjectsBrowserTab",
            'ImGui::BeginTabItem("UI Projects")',
            "UI project browser",
            "Timeline scrub",
            "Animation playback",
            "SFX correlation",
            "SGFX export provenance",
            "GeneratedYncPNativeComponentMap::kProjects",
            "GeneratedYncPNativeComponentMap::kScenes",
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("build_yncp_native_component_map.py", build_script)
        self.assertIn("ui_lab_yncp_native_component_map.generated.h", build_script)
        self.assertIn("--output-header", generator)
        self.assertIn("Phase 236", report)

    def test_ui_lab_draws_independent_yncp_preview_lane_with_sfx_correlation(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")

        for token in [
            "g_uiProjectBrowserPreviewPinned = true",
            "DrawYncScenePreviewCanvas",
            "DrawYncTimelineLane",
            "DrawYncSfxCorrelationLane",
            "Force selected scene preview",
            "Independent preview",
            "selected YNCP scene, not current runtime route",
            "keyframe tick",
            "audio-bank correlation",
            "runtime hooks",
            "Ghidra xrefs",
            "ui-project-preview-forced",
            "ui-project-sfx-correlation-marker",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 237",
            "independent drawable/keyframe preview lane",
            "audio-bank correlation lane",
            "not tied to the current gameplay/title/loading route",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_binds_yncp_subimages_to_texture_backed_preview_commands(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_yncp_native_component_map.generated.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")

        for token in [
            "struct PreviewDrawCommand",
            "struct SceneDrawCommand",
            "kPreviewDrawCommands",
            "kSceneDrawCommands",
            "textureName",
            "textureRelativePath",
            "subimageIndex",
            "sourceTextureWidth",
            "sourceTextureHeight",
            "sourceX",
            "sourceY",
            "sceneLeft",
            "sceneTop",
            "sceneWidth",
            "sceneHeight",
            "baseTranslationX",
            "baseTranslationY",
            "normalizedCastLeft",
            "normalizedCastTop",
            "real-yncp-subimage-dds-rect",
            "real-yncp-cast-tree-subimage-scene-rect",
            "ui_playscreen",
            ".dds",
        ]:
            self.assertIn(token, header)

        for token in [
            "extract_preview_draw_commands",
            "extract_scene_draw_commands",
            "composed_cast_rect",
            "build_group_global_transforms",
            "read_dds_dimensions",
            "used_subimage_indices",
            "texture_relative_path",
            "real-yncp-subimage-dds-rect",
            "real-yncp-cast-tree-subimage-scene-rect",
        ]:
            self.assertIn(token, generator)

        for token in [
            "FindYncPreviewDrawCommands",
            "FindYncSceneDrawCommands",
            "DrawYncComposedScenePreview",
            "LoadYncPreviewTexture",
            "DrawYncTextureBackedSubimagePreview",
            "AddYncPreviewSubimageRect",
            "GeneratedYncPNativeComponentMap::kInputRoot",
            "LoadTexture",
            "AddImage",
            "composed YNCP scene",
            "cast-tree scene placement",
            "texture-backed cast/subimage",
            "src=",
            "dst=",
            "real DDS/subimage rectangles",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 238",
            "texture-backed cast/subimage preview",
            "real DDS/subimage rectangles",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_starts_exact_sfx_cue_candidate_join_for_ui_projects(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_yncp_native_component_map.generated.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")

        for token in [
            "struct SfxCueCandidate",
            "kSfxCueCandidates",
            "bankName",
            "cueName",
            "runtimeHook",
            "ghidraXref",
            "se_system_worldmap",
            "sys_worldmap_window",
            "sys_worldmap_decide",
            "sys_worldmap_cansel",
            "CTitleStateMenu::Update",
            "Game_PlaySound",
        ]:
            self.assertIn(token, header)

        for token in [
            "FindYncSfxCueCandidates",
            "DrawYncSfxCueCandidates",
            "runtime hook hits + Ghidra xrefs",
            "exact cue candidate",
            "se_system_worldmap",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "runtime-hook + Ghidra xref SFX candidates",
            "se_system_worldmap",
            "sys_worldmap_decide",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_samples_yncp_animation_keyframes_into_composed_scene_preview(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_yncp_native_component_map.generated.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")

        for token in [
            "struct AnimationTrackKeyframe",
            "kAnimationTrackKeyframes",
            "animationName",
            "trackType",
            "keyframeIndex",
            "frame",
            "value",
            "inTangent",
            "outTangent",
            "interpolationType",
            "real-yncp-animation-keyframe",
            "Usual_Anim",
            "XPosition",
        ]:
            self.assertIn(token, header)

        for token in [
            "extract_animation_track_keyframes",
            "is_animation_track_supported_for_preview",
            "real-yncp-animation-keyframe",
            "animation_keyframe_data_list",
            "animation_frame_data_list",
            "keyframes",
        ]:
            self.assertIn(token, generator)

        for token in [
            "FindYncAnimationTrackKeyframes",
            "SampleYncAnimationTrack",
            "SampleYncSceneAnimationState",
            "ApplyYncAnimationToSceneDraw",
            "animated keyframe sample",
            "YNCP animation scrub",
            "real authored animation tracks",
            "Hermite",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 240",
            "keyframe interpolation",
            "real authored animation tracks",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_invokes_selected_yncp_scene_as_foreground_projection(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")

        for token in [
            "g_yncForegroundInvokeVisible",
            "g_yncForegroundProjectIndex",
            "g_yncForegroundSceneIndex",
            "DrawYncForegroundInvokedScene",
            "Invoke Selected Scene",
            "Close Invoked Scene",
            "ui-project-foreground-invoke",
            "foreground invoked YNCP scene",
            "pause-menu-style foreground projection",
            "summoned UI surface",
            "FindYncAnimationTrackKeyframes(project, scene)",
            "SampleYncSceneAnimationState",
            "ApplyYncAnimationToSceneDraw",
            "FindYncSfxCueCandidates(project, scene)",
            "DrawYncForegroundInvokedScene();",
        ]:
            self.assertIn(token, ui_lab)

        overlay_order = ui_lab.index("DrawYncForegroundInvokedScene();")
        shell_gate = ui_lab.index("if (!g_operatorShellVisible && !AnyOperatorFloatingPaneVisible())")
        self.assertLess(overlay_order, shell_gate)

        for token in [
            "Phase 241",
            "foreground invoke mode",
            "summoned UI surface",
        ]:
            self.assertIn(token, report)

        self.assertIn("foreground invoke mode", generator)

    def test_ui_lab_can_probe_native_csd_make_for_selected_yncp_project(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        ui_lab_header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        aspect = self.read("UnleashedRecomp/patches/aspect_ratio_patches.cpp")
        resident = self.read("UnleashedRecomp/patches/resident_patches.cpp")
        generated = self.read("UnleashedRecomp/patches/ui_lab_yncp_native_component_map.generated.h")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")

        for token in [
            "struct NativeCsdMakeProbeState",
            "g_nativeCsdMakeProbe",
            "RequestNativeCsdMakeProbe",
            "RunNativeCsdMakeProbe",
            "ResolveNativeCsdSceneForSelectedProject",
            "WriteNativeCsdSceneMotionFrame",
            "Copy selected YNCP bytes into guest heap",
            "ValidateNativeCsdPackageBytes",
            "GuestToHostCsdMakeWithProbeContext",
            "PPCContext newCtx = currentCtx",
            "NativeCsdMakeProbeProjectNameForTraversal",
            "OnNativeCsdMakeCallContext",
            "OnCsdProjectTreeTraversalFinished",
            "TryResolveQueuedNativeCsdMakeProbeFromObservedTree",
            "g_nativeCsdMakeLastContextAddress",
            "g_nativeCsdMakeProbeExecuteExperimentalMake",
            "cloned real-call r6 context",
            "danger: execute Make",
            "native-csd-observe-succeeded",
            "CPAF",
            "experimental any CSD",
            "sub_825E4068",
            "g_userHeap.Alloc",
            "g_userHeap.Free",
            "CCsdProject::Make",
            "MakeCsdProjectMidAsmHook",
            "Native CSD Make Probe",
            "Probe Native CSD Make",
            "native-csd-make-probe-requested",
            "native-csd-make-probe-succeeded",
            "native project pointer",
            "root node",
            "scene count",
            "Native scene motion scrub",
            "m_MotionFrame",
            "0x64",
            "m_MotionRepeatType",
            "0x94",
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("NativeCsdMakeProbeProjectNameForTraversal", ui_lab_header)
        self.assertIn("OnNativeCsdMakeCallContext", ui_lab_header)
        self.assertIn("OnCsdProjectTreeTraversalFinished", ui_lab_header)
        self.assertIn("NativeCsdMakeProbeProjectNameForTraversal", aspect)
        self.assertIn("OnCsdProjectTreeTraversalFinished", aspect)

        for token in [
            "SWA::CCsdProject::Make",
            "sub_825E4068",
            "UiLab::RunNativeCsdMakeProbe",
            "UiLab::OnNativeCsdMakeCallContext",
        ]:
            self.assertIn(token, resident)

        for token in [
            "ui_loading.yncp",
            "ui_worldmap_help.yncp",
            "relativePath",
            "kInputRoot",
        ]:
            self.assertIn(token, generated)

        for token in [
            "Phase 242",
            "Phase 243",
            "Phase 244",
            "Native CSD Make Probe",
            "CCsdProject::Make",
            "r6",
            "danger: execute Make",
        ]:
            self.assertIn(token, report)

        self.assertIn("Native CSD Make Probe", generator)

    def test_ui_lab_can_attach_observed_native_csd_scene_as_foreground_render_probe(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        ui_lab_header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        aspect = self.read("UnleashedRecomp/patches/aspect_ratio_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")

        for token in [
            "struct NativeCsdForegroundRenderProbeState",
            "g_nativeCsdForegroundRenderProbe",
            "RequestNativeForegroundRenderProbe",
            "DetachNativeForegroundRenderProbe",
            "ConsumeNativeForegroundSceneRenderAddress",
            "OnNativeForegroundSceneRendered",
            "UpdateNativeCsdSceneMotionPlayback",
            "TryFindNativeCsdMakeProbeScene",
            "piggyback on active CSD render pass",
            "no owner pointer hijack",
            "native foreground render host",
            "CsdManagerSceneCorrelation",
            "OnCsdManagerSceneRender",
            "TryCorrelateCsdManagerSceneToResourceScene",
            "nativeManagerScenePointer",
            "nativeResourceScenePointer",
            "managerSceneResourceOffset",
            "native-csd-manager-scene-correlated",
            "Attach Native Scene",
            "Spawn Native Foreground",
            "Detach Native Foreground",
            "native-csd-foreground-render-probe-requested",
            "native-csd-foreground-rendered",
            "nativeSceneMotionLastUpdateFrame",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "ConsumeNativeForegroundSceneRenderAddress",
            "OnNativeForegroundSceneRendered",
            "OnCsdManagerSceneRender",
        ]:
            self.assertIn(token, ui_lab_header)

        for token in [
            "ConsumeNativeForegroundSceneRenderAddress",
            "OnNativeForegroundSceneRendered",
            "OnCsdManagerSceneRender(hostCtx.r3.u32)",
            "PPCContext foregroundCtx",
            "__imp__sub_830BC640(foregroundCtx, base)",
            "native foreground CScene::Render piggyback",
        ]:
            self.assertIn(token, aspect)

        for token in [
            "Phase 245",
            "Native Foreground Render Probe",
            "piggyback",
            "owner pointer hijack",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_bridge_reports_native_foreground_probe_status(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        query_script = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")

        for token in [
            "BuildNativeForegroundStatusJson",
            "AppendNativeCsdMakeProbeStatusJson",
            "AppendNativeCsdForegroundRenderProbeStatusJson",
            "HandleNativeForegroundBridgeControlCommand",
            "HandleNativeMakeProbeBridgeCommand",
            "FindCsdProjectTreeRecordBySceneAddress",
            "host CSD project mismatch",
            "same CSD project render host required",
            "no correlated live manager CScene instance",
            "\\\"nativeManagerScenePointer\\\"",
            "\\\"nativeResourceScenePointer\\\"",
            "\\\"managerSceneResourceOffset\\\"",
            "\\\"nativeCsdMakeProbe\\\"",
            "\\\"nativeCsdForegroundRenderProbe\\\"",
            "\\\"nativeSceneMotion\\\"",
            "\\\"ownerHijackUsed\\\"",
            "\\\"renderPassSeen\\\"",
            "\\\"lastRenderedScenePointer\\\"",
            "\"native-foreground-status\"",
            "\"native-make-observe\"",
            "\"native-foreground-attach\"",
            "\"native-foreground-detach\"",
            "\"native-motion-play\"",
            "\"native-motion-stop\"",
            "\"native-motion-scrub\"",
            "native-csd-make-probe-requested-by-live-bridge",
            "native-csd-make-probe-live-bridge-immediate-resolve-check",
            "native-csd-foreground-control",
            "native-csd-motion-control",
            "native foreground render probe status",
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("native-foreground-status", query_script)
        self.assertIn("native-make-observe", query_script)
        self.assertIn("native-foreground-attach", query_script)
        self.assertIn("native-motion-scrub", query_script)
        self.assertIn("[string]$Project", query_script)
        self.assertIn("[string]$Scene", query_script)
        self.assertIn("[string]$Frame", query_script)
        self.assertIn("[string]$MotionFrame", query_script)

    def test_ui_lab_discovers_native_csd_foreground_owner_hosts_before_hijack(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        query_script = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            "struct CsdManagerSceneOwnerCandidate",
            "g_csdManagerSceneOwnerCandidates",
            "TryDiscoverCsdManagerSceneOwnerCandidates",
            "ScanCsdOwnerCandidateRange",
            "AppendNativeCsdOwnerDiscoveryJson",
            "\\\"nativeCsdOwnerDiscovery\\\"",
            "\\\"ownerCandidates\\\"",
            "\\\"ownerAddress\\\"",
            "\\\"fieldOffset\\\"",
            "\\\"fieldAddress\\\"",
            "\\\"matchKind\\\"",
            "\\\"confidence\\\"",
            "native-csd-owner-candidate",
            "native-owner-discovery",
            "owner/host attach discovery",
            "direct-manager-scene-pointer",
            "indirect-manager-scene-pointer",
            "known UI owner scan range",
            "CHudPause owner",
            "CHudSonicStage owner",
            "title owner context",
            "foreground owner attach discovery",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "native-owner-discovery",
            "native-owner-scan",
        ]:
            self.assertIn(token, query_script)

        for token in [
            "Phase 252",
            "Foreground Owner/Host Attach Discovery",
            "owner/host attach",
            "manager CScene",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 252", generator)

    def test_ui_lab_bounds_checks_guest_reads_used_by_owner_discovery(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")

        for token in [
            "IsReadableGuestRange",
            "PPC_MEMORY_SIZE",
            "guestEnd",
            "TryReadGuestU32",
            "TryWriteGuestU32",
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("if (!IsReadableGuestRange(guestAddress, sizeof(uint32_t)))", ui_lab)

    def test_ui_lab_maps_native_csd_owner_layout_sibling_fields(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        query_script = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            "struct CsdOwnerLayoutFieldCandidate",
            "g_csdOwnerLayoutFieldCandidates",
            "TryMapCsdOwnerLayoutForCandidate",
            "MapOwnerLayoutsForResolvedNativeProbe",
            "ScanCsdOwnerLayoutFieldRange",
            "AppendNativeCsdOwnerLayoutJson",
            "\\\"nativeCsdOwnerLayout\\\"",
            "\\\"layoutFields\\\"",
            "\\\"sourceCandidateFieldOffset\\\"",
            "\\\"resolvedManagerScene\\\"",
            "\\\"resolvedResourceScene\\\"",
            "\\\"layoutFieldCount\\\"",
            "native-csd-owner-layout-field",
            "native-owner-layout",
            "owner layout map",
            "sibling owner CSD fields",
            "title owner layout",
            "HUD owner layout",
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("native-owner-layout", query_script)
        self.assertIn("Phase 253", report)
        self.assertIn("Owner Layout Mapping", report)
        self.assertIn("0x1E4", report)
        self.assertIn("Phase 253", generator)

    def test_ui_lab_names_owner_layout_fields_with_ghidra_xref_targets(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        wrapper = self.read("research_uiux/runtime_reference/tools/export_unleashed_recomp_ghidra_context.ps1")
        java_script = self.read("research_uiux/runtime_reference/ghidra_scripts/SwardExportFunctionContext.java")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            "struct CsdOwnerLayoutSemantic",
            "ResolveCsdOwnerLayoutSemantic",
            "ApplyCsdOwnerLayoutSemantic",
            "native-csd-owner-layout-semantic",
            "\\\"semanticName\\\"",
            "\\\"semanticRole\\\"",
            "\\\"ownerLifecycle\\\"",
            "\\\"attachSetterCandidate\\\"",
            "\\\"ghidraXrefStatus\\\"",
            "\\\"ghidraXref\\\"",
            "titleContext.m_rcTitleManager",
            "titleContext.m_rcTitleResource",
            "title-owner-context+0x1E4",
            "CGameModeStageTitle::Update",
            "HUD owner layout pending runtime gameplay evidence",
            "Ghidra xref oracle",
            "owner attach setter path",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "sub_825518B8",
            "sub_824D89B0",
            "sub_824D9308",
            "sub_824D95F8",
        ]:
            self.assertIn(token, wrapper)
            self.assertIn(token, java_script)

        for token in [
            "Phase 254",
            "Owner Layout Semantic Naming",
            "Ghidra xref oracle",
            "titleContext.m_rcTitleManager",
            "HUD owner layout pending runtime gameplay evidence",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 254", generator)

    def test_ui_lab_probes_native_owner_attach_setter_candidates(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        title_patch = self.read("UnleashedRecomp/patches/CGameModeStageTitle_patches.cpp")
        hud_patch = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        query_script = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            "void OnTitleOwnerSetterProbe(",
            "void OnHudOwnerSetterProbe(",
        ]:
            self.assertIn(token, header)

        for token in [
            "struct NativeOwnerSetterProbeSample",
            "g_nativeOwnerSetterProbeSamples",
            "RecordNativeOwnerSetterProbeSample",
            "AppendNativeOwnerSetterProbeJson",
            "\\\"nativeOwnerSetterProbes\\\"",
            "\\\"helperName\\\"",
            "\\\"ownerAddress\\\"",
            "\\\"fieldOffset\\\"",
            "\\\"fieldValue\\\"",
            "\\\"argR3\\\"",
            "\\\"argR4\\\"",
            "\\\"argR5\\\"",
            "\\\"argR6\\\"",
            "\\\"resultR3\\\"",
            "\\\"routeEvidence\\\"",
            "native-owner-setter-probe",
            "native-owner-setter-probe-status",
            "titleContext+0x1E8",
            "titleContext+0x1D1",
            "db-xml-route-candidate",
            "asset-db-route-evidence",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "PPC_FUNC_IMPL(__imp__sub_8250F2B8);",
            "PPC_FUNC(sub_8250F2B8)",
            "PPC_FUNC_IMPL(__imp__sub_82581288);",
            "PPC_FUNC(sub_82581288)",
            "PPC_FUNC_IMPL(__imp__sub_825812A8);",
            "PPC_FUNC(sub_825812A8)",
            "PPC_FUNC_IMPL(__imp__sub_825812E8);",
            "PPC_FUNC(sub_825812E8)",
            "UiLab::OnTitleOwnerSetterProbe",
        ]:
            self.assertIn(token, title_patch)

        for token in [
            "PPC_FUNC_IMPL(__imp__sub_82E5FCD0);",
            "PPC_FUNC(sub_82E5FCD0)",
            "PPC_FUNC_IMPL(__imp__sub_82E61A78);",
            "PPC_FUNC(sub_82E61A78)",
            "UiLab::OnHudOwnerSetterProbe",
        ]:
            self.assertIn(token, hud_patch)

        for token in [
            "native-owner-setter-probe",
            "native-owner-setter-status",
        ]:
            self.assertIn(token, query_script)

        for token in [
            "Phase 255",
            "Native Owner Setter Probe",
            "sub_8250F2B8",
            "sub_82581288/A8/E8",
            "sub_82E5FCD0 / sub_82E61A78",
            "DB/XML route evidence",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 255", generator)

    def test_ui_lab_maps_hud_owner_setter_helpers_back_to_owner_fields(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            "struct NativeOwnerSetterProbeOwnerMap",
            "ResolveNativeOwnerSetterProbeOwnerMap",
            "helperObjectOffset",
            "probableOwnerAddress",
            "probableOwnerSource",
            "sourceFieldOffset",
            "targetFieldOffset",
            "generatedCallsiteDiscriminator",
            "native-owner-setter-hud-owner-field-map",
            "CHudSonicStage::sub_824D9308",
            "argR3 == owner+0x28",
            "argR5=110",
            "argR5=121",
            "argR3 - 0x28 inferred owner",
            "infer CHudSonicStage owner from helper argR3",
            "owner+0xD8",
            "owner+0xE0",
            "owner+0xF0",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 256",
            "HUD Owner Setter Field Map",
            "argR3 == owner+0x28",
            "owner+0xD8",
            "owner+0xE0",
            "owner+0xF0",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 256", generator)

    def test_ui_lab_seeds_inferred_chudsonicstage_owner_for_layout_scanning(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            # Phase 257 inferred owner globals.
            "g_inferredChudSonicStageOwnerAddress",
            "g_inferredChudSonicStageOwnerSource",
            "g_inferredChudSonicStageOwnerHelper",
            "g_inferredChudSonicStageOwnerLastUpdatedFrame",
            "g_inferredChudSonicStageOwnerSampleCount",
            # Phase 257 evidence event for the seeded inferred owner.
            "native-owner-setter-hud-inferred-owner-seeded",
            "read-only inferred CHudSonicStage owner seeded for owner-layout scanning",
            # Phase 257 plumbing into the bounded owner-layout scan ranges.
            "inferred CHudSonicStage owner",
            "inferred CHudSonicStage owner; argR3 == owner+0x28 helper object",
            # Phase 257 + 261 HUD owner field semantic names. Helper-related
            # fields keep their argR5 role labels; renderable scenes use the
            # SWA HUD class field naming convention.
            "hudOwner.helperAttachR6Field",
            "hudOwner.helperAttachReturnField",
            "hudOwner.helperUpdateScenePrimaryField",
            "hudOwner.m_rcSpeedGauge",
            "hudOwner.m_rcRingEnergyGauge",
            "hudOwner.m_rcGaugeFrame",
            "hudOwnerInferred.helperAttachR6Field",
            "hudOwnerInferred.helperAttachReturnField",
            "hudOwnerInferred.helperUpdateScenePrimaryField",
            "hudOwnerInferred.m_rcSpeedGauge",
            "hudOwnerInferred.m_rcRingEnergyGauge",
            "hudOwnerInferred.m_rcGaugeFrame",
            # Phase 257 + 261 HUD owner field semantic roles.
            "hud-owner-helper-attach-r6-field",
            "hud-owner-helper-attach-return-field",
            "hud-owner-helper-update-scene-primary-field",
            "hud-owner-speed-gauge-rcobject-memory-field",
            "hud-owner-ring-energy-gauge-rcobject-memory-field",
            "hud-owner-gauge-frame-rcobject-memory-field",
            # Phase 257 + 261 attach-setter narratives still surface the
            # argR5 helper roles even when the slot turns out to hold a
            # static-module pointer or non-scene field at sample time.
            "owner+0xD8 source field read into r6 by sub_82E5FCD0",
            "owner+0xE0 written by sub_82E5FCD0",
            "owner+0xF0 snapshotted by sub_82E61A78",
            "owner+0xF4 m_rcRingEnergyGauge.m_pMemory",
            # Phase 257 live-state JSON snapshot exposes the inferred owner.
            "inferredChudSonicStageOwnerAddress",
            "inferredChudSonicStageOwnerSource",
            "inferredChudSonicStageOwnerHelper",
            "inferredChudSonicStageOwnerLastUpdatedFrame",
            "inferredChudSonicStageOwnerSampleCount",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 257",
            "HUD Owner Layout Field Map",
            "inferred CHudSonicStage owner",
            "owner+0xD8",
            "owner+0xE0",
            "owner+0xF0",
            "owner+0xF4",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 257", generator)

    def test_ui_lab_samples_hud_owner_slot_readouts(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            # Phase 258 struct + globals.
            "struct HudOwnerSlotReadout",
            "g_lastHudOwnerSlotReadouts",
            "g_lastHudOwnerSlotReadoutStableSignatures",
            "g_lastHudOwnerSlotReadoutEvidenceFrames",
            "kHudOwnerSlotReadoutMinEvidenceIntervalFrames",
            # Phase 258 sampler entry points.
            "BuildHudOwnerSlotReadout",
            "SampleHudOwnerSlotReadouts",
            # Phase 258 + 261 slot specs cover all 13 named CHudSonicStage
            # owner offsets we have runtime evidence for.
            "owner+0xD8 helperAttachR6Field",
            "owner+0xE0 helperAttachReturnField",
            "owner+0xEC m_rcSpeedGauge",
            "owner+0xF0 helperUpdateScenePrimaryField",
            "owner+0xF4 m_rcRingEnergyGauge",
            "owner+0xFC m_rcGaugeFrame",
            "owner+0x1958 medalGetSceneRef0",
            "owner+0x1964 medalGetSceneRef1",
            "owner+0x19C4 medalGetSceneRef2",
            "owner+0x1B58 speedCountSceneRef0",
            "owner+0x1B64 speedCountSceneRef1",
            "owner+0x1BC4 speedCountSceneRef2",
            "owner+0x1E40 scoreCountManagerScenePointer",
            # Phase 258 classification labels.
            "live-manager-scene: HUD owner slot directly references a live manager CScene",
            "resource-scene: HUD owner slot directly references a resource Scene",
            "indirect-manager-scene: HUD owner slot points at object containing a live manager CScene",
            "indirect-resource-scene: HUD owner slot points at object containing a resource Scene",
            "uncorrelated-pointer: HUD owner slot value is a plausible pointer",
            "non-pointer: HUD owner slot value is not a plausible guest pointer",
            "null: HUD owner slot is unset",
            # Phase 258 evidence event.
            "native-owner-setter-hud-owner-slot-readout",
            # Phase 258 owner source labels (constructor vs inferred).
            "CHudSonicStage owner (constructor-confirmed)",
            "inferred CHudSonicStage owner",
            # Phase 258 live-state JSON mirror.
            "ownerSlotReadoutCount",
            "ownerSlotReadouts",
            "correlatedManagerSceneAddress",
            "correlatedResourceSceneAddress",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 258",
            "HUD Owner Slot Readout",
            "live-manager-scene",
            "indirect-manager-scene",
            "uncorrelated-pointer",
            "native-owner-setter-hud-owner-slot-readout",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 258", generator)

    def test_ui_lab_filters_hud_owner_setter_probe_hot_path(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            # Phase 259 hot-path filter constants and globals.
            "kProvenArgR5Sub82E5FCD0",
            "kProvenArgR5Sub82E61A78",
            "isProvenStageBindCallsite",
            "g_loggedHudOwnerSetterProbeFirstSeenKeys",
            "g_skippedHudOwnerSetterProbeCallCount",
            "g_recordedHudOwnerSetterProbeCallCount",
            # Phase 259 breadcrumb evidence event for non-proven helpers.
            "native-owner-setter-hud-helper-first-seen",
            "non-proven HUD setter helper callsite; sample recording skipped to preserve gameplay framerate",
            # Phase 259 perf counters surfaced in live-state JSON.
            "skippedHudOwnerSetterProbeCallCount",
            "recordedHudOwnerSetterProbeCallCount",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 259",
            "HUD Setter Probe Hot-Path Filter",
            "argR5=110",
            "argR5=121",
            "native-owner-setter-hud-helper-first-seen",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 259", generator)

    def test_ui_lab_sweeps_hud_owner_for_renderable_slots(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            # Phase 260 struct + globals (Phase 263 replaced the set with a
            # correlation-aware map, see test_ui_lab_resweeps_when_csd_correlations_grow).
            "struct HudOwnerRenderableSlot",
            "g_lastHudOwnerSweepStates",
            "g_hudOwnerRenderableSlots",
            "g_loggedHudOwnerRenderableSlotKeys",
            # Phase 260 sweep entry point.
            "TryRunOpportunisticHudOwnerLayoutSweep",
            "kHudOwnerSweepBytes",
            "kHudOwnerSweepIndirectBytes",
            # Phase 260 evidence events.
            "native-hud-owner-renderable-slot",
            "native-hud-owner-layout-sweep-complete",
            "read-only renderable HUD owner slot discovered; inspect before any guarded native foreground attach",
            "read-only HUD owner layout sweep complete",
            # Phase 260 match kinds emitted by the sweep.
            "direct-manager-scene",
            "indirect-manager-scene",
            # Phase 260 live-state JSON fields.
            "renderableSlotCount",
            "renderableSlots",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 260",
            "HUD Owner Renderable-Slot Sweep",
            "native-hud-owner-renderable-slot",
            "indirect-manager-scene",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 260", generator)

    def test_ui_lab_cross_validates_hud_owner_field_names(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            # Phase 261 cross-validation lookup helper + event.
            "FindChudSonicStageExpectedOwnerFieldByRcObjectOffset",
            "native-hud-owner-field-cross-validated",
            "runtime-confirmed: Phase 260 sweep agrees with kChudSonicStageExpectedOwnerFields entry for this offset",
            # Phase 261 sweep summary surfaces cross-validated count.
            "crossValidatedHits",
            "expectedFieldTableSize",
            # Phase 261 deep-cluster semantic names from sweep evidence.
            "hudOwner.medalGetSceneRef0",
            "hudOwner.medalGetSceneRef1",
            "hudOwner.medalGetSceneRef2",
            "hudOwner.speedCountSceneRef0",
            "hudOwner.speedCountSceneRef1",
            "hudOwner.speedCountSceneRef2",
            "hudOwner.scoreCountManagerScenePointer",
            "hudOwnerInferred.medalGetSceneRef0",
            "hudOwnerInferred.scoreCountManagerScenePointer",
            # Phase 261 deep-cluster semantic roles.
            "hud-owner-medal-get-scene-ref",
            "hud-owner-speed-count-scene-ref",
            "hud-owner-score-count-direct-manager-scene-pointer",
            # Phase 261 attach-setter narratives reference the sweep
            # evidence backing each runtime-discovered field.
            "Phase 260 sweep saw three RCPtr-style slots at 0x1958/0x1964/0x19C4 dereferencing into a shared backing block",
            "Phase 260 sweep saw three RCPtr-style slots at 0x1B58/0x1B64/0x1BC4 dereferencing into a shared backing block",
            "Phase 260 sweep + constructor expected-fields agree this is the live SpeedGauge scene wrapper",
            "Phase 260 sweep + constructor expected-fields agree this is the live RingEnergyGauge scene wrapper",
            "Phase 260 sweep + constructor expected-fields agree this is the live GaugeFrame scene wrapper",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 261",
            "HUD Owner Field Naming + Cross-Validation",
            "native-hud-owner-field-cross-validated",
            "kChudSonicStageExpectedOwnerFields",
            "medal_get_m",
            "speed_count",
            "score_count",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 261", generator)

    def test_ui_lab_writes_hud_owner_layout_sidecar(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            # Phase 262 globals + path helper.
            "g_lastHudOwnerLayoutSidecarPath",
            "g_lastHudOwnerLayoutSidecarFrame",
            "HudOwnerLayoutSidecarPath",
            "hud_owner_layout.json",
            # Phase 262 writer entry point + schema.
            "WriteHudOwnerLayoutSidecar",
            "sward-hud-owner-layout-v1",
            "constructorHookSource",
            "expectedFieldHeaderSource",
            "expectedFieldTable",
            "namedSlots",
            "renderableSlots",
            "crossValidatedExpectedField",
            # Phase 262 evidence event.
            "native-hud-owner-layout-sidecar-written",
            "runtime-confirmed HUD owner layout written to sward-hud-owner-layout-v1 sidecar for SGFX HUD code generation",
            # Phase 262 live-state JSON exposes the sidecar path + frame.
            "hudOwnerLayoutSidecarPath",
            "hudOwnerLayoutSidecarFrame",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 262",
            "HUD Owner Layout JSON Sidecar",
            "sward-hud-owner-layout-v1",
            "hud_owner_layout.json",
            "native-hud-owner-layout-sidecar-written",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 262", generator)

    def test_ui_lab_resweeps_when_csd_correlations_grow(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            # Phase 263 sweep state + thresholds.
            "struct HudOwnerSweepState",
            "g_lastHudOwnerSweepStates",
            "kHudOwnerSweepMinReSweepIntervalFrames",
            "kHudOwnerSweepMinReSweepCorrelationGrowth",
            # Phase 263 gating predicates.
            "firstSweepForOwner",
            "correlationsGrewEnough",
            "intervalElapsed",
            # Phase 263 sweep-complete summary now reports correlation table size + first-sweep flag.
            "correlationTableSize",
            "firstSweepForOwner=",
            # Phase 263 reuses g_csdManagerSceneCorrelations to drive re-sweep.
            "g_csdManagerSceneCorrelations.size()",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 263",
            "HUD Owner Sweep Re-run on Correlation Growth",
            "kHudOwnerSweepMinReSweepIntervalFrames",
            "kHudOwnerSweepMinReSweepCorrelationGrowth",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 263", generator)

    def test_ui_lab_groups_renderable_slots_and_tags_confidence(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            # Phase 264 confidence tier helper + struct field.
            "ResolveHudOwnerRenderableSlotConfidenceTier",
            "std::string confidenceTier",
            # Phase 264 the three tiers must all appear as literals.
            "\"cross-validated\"",
            "\"constructor-confirmed\"",
            "\"inferred-owner\"",
            # Phase 264 the renderable-slot evidence event carries the tier.
            "|confidenceTier=",
            # Phase 264 grouping in the sidecar.
            "renderableSlotGroups",
            "instanceCount",
            "matchKinds",
            # Phase 264 live-state JSON exposes per-tier counts.
            "renderableSlotCrossValidatedCount",
            "renderableSlotConstructorConfirmedCount",
            "renderableSlotInferredOwnerCount",
            # Phase 264 sidecar-written event surfaces group count.
            "renderableSlotGroupCount",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 264",
            "HUD Renderable-Slot Grouping + Confidence Tiers",
            "ring_get",
            "renderableSlotGroups",
            "cross-validated",
            "inferred-owner",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 264", generator)

    def test_ui_lab_extends_chudsonic_stage_expected_fields_from_constructor_decode(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/YNCP_NATIVE_COMPONENT_MAP.md")
        generator = self.read("research_uiux/tools/build_yncp_native_component_map.py")

        for token in [
            # Phase 265 expanded table size (21 entries).
            "std::array<ChudSonicStageExpectedOwnerField, 21> kChudSonicStageExpectedOwnerFields",
            # Phase 265 named constructor-decoded fields beyond the original 9.
            '{ "m_rcExpCount", 0x100, 0x104 },',
            '{ "m_rcPtrField108", 0x108, 0x10C },',
            '{ "m_rcPtrField110", 0x110, 0x114 },',
            # Phase 266: +0x118 promoted to m_rcSpeedCount after the runtime
            # sweep cross-validated it against ui_playscreen/add/speed_count
            # on the constructor-confirmed CHudSonicStage owner.
            '{ "m_rcSpeedCount", 0x118, 0x11C },',
            '{ "m_rcPtrField120", 0x120, 0x124 },',
            '{ "m_rcPtrField150", 0x150, 0x154 },',
            '{ "m_rcPtrField158", 0x158, 0x15C },',
            '{ "m_rcPtrField160", 0x160, 0x164 },',
            '{ "m_rcPtrField168", 0x168, 0x16C },',
            '{ "m_rcPtrField170", 0x170, 0x174 },',
            '{ "m_rcPtrField178", 0x178, 0x17C },',
            '{ "m_rcPtrField180", 0x180, 0x184 },',
            # Phase 265 source attribution string cites the recomp file.
            "Phase 265 expanded from CHudSonicStage::CHudSonicStage (sub_824D89B0) decoded from local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.28.cpp:61909",
            # Phase 265 semantic resolver case for m_rcExpCount.
            "hudOwner.m_rcExpCount",
            "hudOwnerInferred.m_rcExpCount",
            "hud-owner-exp-count-rcobject-memory-field",
            "Phase 265 ppc_recomp.28.cpp constructor decode places an RCPtr at 0x100/0x104",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 265",
            "CHudSonicStage Constructor Decode",
            "21 RCPtrs",
            "m_rcExpCount",
            "ppc_recomp.28.cpp",
            "sub_824D89B0",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 265", generator)

    def test_sgfx_hud_layout_generator_produces_human_readable_class(self):
        # Phase 266: end-to-end test of the generator and the emitted header.
        # Drive the generator with a self-contained synthetic ui_lab_patches
        # snippet + a synthetic sidecar so the test does not depend on
        # whatever live evidence happens to exist.
        import importlib.util
        import sys
        import tempfile

        generator_path = ROOT / "research_uiux/tools/build_sgfx_hud_layout.py"
        self.assertTrue(generator_path.is_file(),
            "Phase 266 generator script must exist at the expected path")

        spec = importlib.util.spec_from_file_location(
            "sgfx_hud_layout_under_test", generator_path)
        module = importlib.util.module_from_spec(spec)
        sys.modules["sgfx_hud_layout_under_test"] = module
        spec.loader.exec_module(module)

        synthetic_ui_lab = (
            "// header preamble\n"
            "namespace UiLab {\n"
            "    static constexpr std::string_view kChudSonicStageExpectedOwnerFieldSource =\n"
            "        \"synthetic source attribution for test\";\n"
            "    static constexpr std::array<ChudSonicStageExpectedOwnerField, 3> kChudSonicStageExpectedOwnerFields =\n"
            "    {{\n"
            "        { \"m_rcAlpha\", 0xE0, 0xE4 },\n"
            "        { \"m_rcBeta\", 0xE8, 0xEC },\n"
            "        { \"m_rcGamma\", 0xF0, 0xF4 },\n"
            "    }};\n"
            "}\n"
        )
        synthetic_sidecar = {
            "schema": "sward-hud-owner-layout-v1",
            "renderableSlotGroups": [
                {
                    "ownerSource": "CHudSonicStage owner (constructor-confirmed)",
                    "projectName": "ui_synthetic",
                    "scenePath": "ui_synthetic/beta_scene",
                    "managerSceneAddress": "0xDEADBEEF",
                    "confidenceTier": "cross-validated",
                    "instanceCount": 1,
                    "fieldOffsets": ["0xEC"],
                },
                {
                    # Inferred-owner groups must be ignored by the generator.
                    "ownerSource": "inferred CHudSonicStage owner",
                    "projectName": "ui_inferred_noise",
                    "scenePath": "ui_inferred_noise/should_be_dropped",
                    "managerSceneAddress": "0xCAFEBABE",
                    "confidenceTier": "inferred-owner",
                    "instanceCount": 1,
                    "fieldOffsets": ["0xE4"],
                },
            ],
            "renderableSlots": [],
        }

        # Phase 267: synthetic SWA API header that names m_rcBeta as
        # CProject so the generator picks up an authoritative type for at
        # least one member. The other two synthetic members fall through
        # to the conservative default.
        synthetic_swa_api = (
            "#pragma once\n"
            "#include <SWA.inl>\n"
            "namespace SWA {\n"
            "    class CHudSonicStage {\n"
            "    public:\n"
            "        SWA_INSERT_PADDING(0xE0);\n"
            "        Chao::CSD::RCPtr<Chao::CSD::CProject> m_rcBeta;\n"
            "    };\n"
            "}\n"
        )

        with tempfile.TemporaryDirectory() as tmp:
            tmp_root = Path(tmp)
            patches_dir = tmp_root / "UnleashedRecomp/patches"
            patches_dir.mkdir(parents=True)
            (patches_dir / "ui_lab_patches.cpp").write_text(synthetic_ui_lab, encoding="utf-8")

            api_dir = tmp_root / "local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/Sonic"
            api_dir.mkdir(parents=True)
            (api_dir / "HudSonicStage.h").write_text(synthetic_swa_api, encoding="utf-8")

            sidecar_dir = tmp_root / "out/ui_lab_runtime_evidence/manual_test/manual-observer"
            sidecar_dir.mkdir(parents=True)
            sidecar_path = sidecar_dir / "hud_owner_layout.json"
            sidecar_path.write_text(json.dumps(synthetic_sidecar), encoding="utf-8")

            output_header = tmp_root / "research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_hud_chud_sonic_stage.generated.h"

            argv_backup = sys.argv
            sys.argv = [
                "build_sgfx_hud_layout.py",
                "--repo-root", str(tmp_root),
                "--output-header", str(output_header.relative_to(tmp_root).as_posix()),
            ]
            try:
                rc = module.main()
            finally:
                sys.argv = argv_backup
            self.assertEqual(rc, 0, "Generator should exit 0 on a valid synthetic input")

            self.assertTrue(output_header.is_file(), "Generator must write the header path it was told to use")
            emitted = output_header.read_text(encoding="utf-8")

            for token in [
                # Layout-only header is honest about what it is.
                "// SGFX HUD layout: human-readable port of `class CHudSonicStage`.",
                "Phase 266",
                "Phase 267",
                "ppc_recomp.28.cpp:61909",
                "namespace sward::ui_runtime::generated::sgfx_hud",
                "class CScene;",
                "template <class T>\n    struct RCPtr",
                "static_assert(sizeof(RCPtr<CScene>) == 8",
                "class CHudSonicStage",
                # Phase 267: members declared with Chao::CSD::RCPtr<...>
                # template form matching the SWA convention.
                "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcAlpha;",
                # m_rcBeta is named in the synthetic SWA API header as
                # RCPtr<CProject> so the generator must use that type.
                "Chao::CSD::RCPtr<Chao::CSD::CProject> m_rcBeta;",
                "type from SWA API header",
                "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcGamma;",
                "type defaulted to CScene; SWA API header has not yet named this RCPtr",
                # Forward declarations include the union of named types.
                "class CProject;",
                # Citation in preamble
                "SWA API header (authoritative for SWA-named field types)",
                "HudSonicStage.h",
                # The cross-validated synthetic binding flows through.
                "ui_synthetic/beta_scene",
                "cross-validated",
                # Static-asserts cover every named member and the vtables.
                "static_assert(offsetof(CHudSonicStage, m_pVTable) == 0x00,",
                "static_assert(offsetof(CHudSonicStage, m_pSecondaryVTable) == 0x28,",
                "static_assert(offsetof(CHudSonicStage, m_rcAlpha) == 0xE0,",
                "static_assert(offsetof(CHudSonicStage, m_rcBeta) == 0xE8,",
                "static_assert(offsetof(CHudSonicStage, m_rcGamma) == 0xF0,",
                # SceneBinding registry contains the cross-validated entry only.
                "struct SceneBinding",
                "kSceneBindings",
                "{\"m_rcBeta\", 0xE8,",
                "synthetic source attribution for test",
            ]:
                self.assertIn(token, emitted)

            # The inferred-owner group must NOT leak into the binding registry.
            self.assertNotIn("ui_inferred_noise", emitted)
            self.assertNotIn("should_be_dropped", emitted)

        # The committed live-generated header must also exist and have the
        # expected runtime-confirmed members + sidecar bindings, now using
        # the Phase 267 type-aware Chao::CSD::RCPtr<T> declarations.
        live_header = self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_chud_sonic_stage.generated.h")
        for token in [
            "Phase 266",
            "Phase 267",
            "namespace sward::ui_runtime::generated::sgfx_hud",
            "class CHudSonicStage",
            # Phase 267: SWA-API-typed members carry their authoritative
            # template type and the gauge cluster is RCPtr<CScene>.
            "Chao::CSD::RCPtr<Chao::CSD::CProject> m_rcPlayScreen;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcSpeedGauge;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcRingEnergyGauge;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcGaugeFrame;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcExpCount;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcSpeedCount;",
            "Chao::CSD::RCPtr<Chao::CSD::CNode> m_rcScoreCount;",
            "Chao::CSD::RCPtr<Chao::CSD::CNode> m_rcTimeCount;",
            "static_assert(offsetof(CHudSonicStage, m_rcRingEnergyGauge) == 0xF0",
            # Phase 267: the SWA API header path is cited authoritatively.
            "api/SWA/HUD/Sonic/HudSonicStage.h",
        ]:
            self.assertIn(token, live_header)

    def test_sgfx_hud_layout_generates_chud_pause_from_swa_api_header(self):
        # Phase 268: end-to-end test that the generator can produce a full
        # SWA-API-driven port header for a HUD class with mixed member
        # types (RCPtrs + scalars + be<enum> + multiple enum definitions)
        # and that it correctly captures every SWA_ASSERT_OFFSETOF entry.
        import importlib.util
        import sys
        import tempfile

        generator_path = ROOT / "research_uiux/tools/build_sgfx_hud_layout.py"
        spec_loader = importlib.util.spec_from_file_location(
            "sgfx_hud_layout_chudpause_under_test", generator_path)
        module = importlib.util.module_from_spec(spec_loader)
        sys.modules["sgfx_hud_layout_chudpause_under_test"] = module
        spec_loader.loader.exec_module(module)

        synthetic_pause_api = (
            "#pragma once\n"
            "#include <SWA.inl>\n"
            "using namespace Chao::CSD;\n"
            "namespace SWA {\n"
            "    enum EThing : uint32_t { eThing_A, eThing_B = 4, eThing_C };\n"
            "    class CFakePause : public CGameObject {\n"
            "    public:\n"
            "        SWA_INSERT_PADDING(0x10);\n"
            "        RCPtr<CProject> m_rcProject;\n"
            "        RCPtr<CScene> m_rcScene;\n"
            "        SWA_INSERT_PADDING(0x4);\n"
            "        bool m_IsActive;\n"
            "        SWA_INSERT_PADDING(0x3);\n"
            "        be<EThing> m_Thing;\n"
            "    };\n"
            "    SWA_ASSERT_OFFSETOF(CFakePause, m_rcProject, 0x40);\n"
            "    SWA_ASSERT_OFFSETOF(CFakePause, m_rcScene, 0x48);\n"
            "    SWA_ASSERT_OFFSETOF(CFakePause, m_IsActive, 0x54);\n"
            "    SWA_ASSERT_OFFSETOF(CFakePause, m_Thing, 0x58);\n"
            "}\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            api_path = Path(tmp) / "FakePause.h"
            api_path.write_text(synthetic_pause_api, encoding="utf-8")
            spec = module.parse_swa_api_class(api_path, Path(tmp))

        self.assertIsNotNone(spec)
        self.assertEqual(spec.class_name, "CFakePause")
        self.assertEqual(spec.base_class, "CGameObject")
        self.assertEqual([m.name for m in spec.members],
            ["m_rcProject", "m_rcScene", "m_IsActive", "m_Thing"])
        self.assertEqual([m.offset for m in spec.members], [0x40, 0x48, 0x54, 0x58])
        self.assertEqual(spec.members[0].rcptr_inner_type, "CProject")
        self.assertEqual(spec.members[1].rcptr_inner_type, "CScene")
        self.assertIsNone(spec.members[2].rcptr_inner_type)
        self.assertEqual(spec.members[2].decl_type, "bool")
        self.assertEqual(spec.members[3].decl_type, "be<EThing>")
        self.assertEqual(len(spec.enums), 1)
        self.assertEqual(spec.enums[0].name, "EThing")
        self.assertEqual(spec.enums[0].underlying_type, "uint32_t")
        self.assertEqual(spec.enums[0].values,
            (("eThing_A", None), ("eThing_B", 4), ("eThing_C", None)))

        emitted = module.emit_swa_api_class_header(spec)
        for token in [
            "Phase 268",
            "class CFakePause",
            "Chao::CSD::RCPtr<Chao::CSD::CProject> m_rcProject;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcScene;",
            "bool m_IsActive;",
            "be<EThing> m_Thing;",
            "enum class EThing : std::uint32_t",
            "eThing_A,",
            "eThing_B = 4,",
            "eThing_C",
            # Padding fills the gaps between members.
            "m_padding0000_0040",
            "m_padding0050_0054",
            "m_padding0055_0058",
            # Static asserts cover every SWA_ASSERT_OFFSETOF entry.
            "static_assert(offsetof(CFakePause, m_rcProject) == 0x40,",
            "static_assert(offsetof(CFakePause, m_rcScene) == 0x48,",
            "static_assert(offsetof(CFakePause, m_IsActive) == 0x54,",
            "static_assert(offsetof(CFakePause, m_Thing) == 0x58,",
            # be<T> wrapper is emitted because at least one member uses it.
            "struct be",
            # Base class noted in preamble.
            "SWA base class: CGameObject",
        ]:
            self.assertIn(token, emitted)

        # The committed live CHudPause header must also exist with the
        # expected SWA-API-driven members.
        live_pause = self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_chud_pause.generated.h")
        for token in [
            "Phase 268",
            "class CHudPause",
            "Chao::CSD::RCPtr<Chao::CSD::CProject> m_rcPause;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcBg;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcFooterA;",
            "bool m_IsVisible;",
            "be<EActionType> m_Action;",
            "be<EMenuType> m_Menu;",
            "be<uint32_t> m_Submenu;",
            "enum class EActionType : std::uint32_t",
            "enum class EMenuType : std::uint32_t",
            "enum class EStatusType : std::uint32_t",
            "enum class ETransitionType : std::uint32_t",
            "static_assert(offsetof(CHudPause, m_rcPause) == 0xEC,",
            "static_assert(offsetof(CHudPause, m_IsShown) == 0x1B8,",
            "api/SWA/HUD/Pause/HudPause.h",
        ]:
            self.assertIn(token, live_pause)

    def test_sgfx_hud_layout_sweep_emits_per_class_headers_and_manifest(self):
        # Phase 269: the sweep walks every SWA API HUD header and emits
        # one port header per parseable class plus a manifest JSON. Verify
        # the live emitted artifacts cover the new classes (CGeneralWindow,
        # CLoading) and the manifest faithfully records what was emitted /
        # skipped.
        general_window = self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_cgeneral_window.generated.h")
        for token in [
            "Phase 268",
            "class CGeneralWindow",
            "Chao::CSD::RCPtr<Chao::CSD::CProject> m_rcGeneral;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcBg;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcWindow;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcWindowSelect;",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcFooter;",
            "be<EWindowStatus> m_Status;",
            "be<uint32_t> m_CursorIndex;",
            "be<uint32_t> m_SelectedIndex;",
            "enum class EWindowStatus : std::uint32_t",
            "static_assert(offsetof(CGeneralWindow, m_rcGeneral) == 0xD0,",
            "static_assert(offsetof(CGeneralWindow, m_SelectedIndex) == 0x164,",
            "api/SWA/HUD/GeneralWindow/GeneralWindow.h",
        ]:
            self.assertIn(token, general_window)

        loading = self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_cloading.generated.h")
        for token in [
            "class CLoading",
            "Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcNightToDay;",
            "be<ELoadingDisplayType> m_LoadingDisplayType;",
            "bool m_IsNightToDay;",
            # Phase 269: ELoadingDisplayType is declared without explicit
            # underlying type in the SWA header so the parser falls back
            # to the C++ default (modeled here as int32_t).
            "enum class ELoadingDisplayType : std::int32_t",
            "static_assert(offsetof(CLoading, m_FieldD8) == 0xD8,",
            "static_assert(offsetof(CLoading, m_IsNightToDay) == 0x1A1,",
        ]:
            self.assertIn(token, loading)

        manifest = json.loads(self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_layout_manifest.generated.json"))
        self.assertEqual(manifest["schema"], "sward-sgfx-hud-layout-manifest-v1")
        manifest_class_names = {entry["className"] for entry in manifest["manifestEntries"]}
        self.assertIn("CGeneralWindow", manifest_class_names)
        self.assertIn("CLoading", manifest_class_names)
        # Phase 274: SaveIcon now generates via the SWA_INSERT_PADDING
        # fallback path, so it appears in manifestEntries. CHudSonicStage
        # still has its own runtime-extended path (Phase 266 / 267) and
        # is deduped from the SWA-API-only sweep silently, so the
        # generated CHudSonicStage header is the extended-table version
        # rather than a SWA-API-only re-emission.
        self.assertIn("CSaveIcon", manifest_class_names)
        self.assertNotIn("CHudSonicStage", manifest_class_names)

    def test_sgfx_hud_layout_emits_phase270_inline_accessors(self):
        # Phase 270: every scalar / enum member should get a const noexcept
        # inline accessor. RCPtr members do not get accessors yet because
        # reading through them requires the SWA RCObject runtime to be
        # linked in.
        pause = self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_chud_pause.generated.h")
        for token in [
            "// Phase 270: inline accessors for scalar / enum members.",
            "bool isVisible() const noexcept { return m_IsVisible; }",
            "EActionType getAction() const noexcept { return static_cast<EActionType>(m_Action.m_storage); }",
            "EMenuType getMenu() const noexcept { return static_cast<EMenuType>(m_Menu.m_storage); }",
            "EStatusType getStatus() const noexcept { return static_cast<EStatusType>(m_Status.m_storage); }",
            "ETransitionType getTransition() const noexcept { return static_cast<ETransitionType>(m_Transition.m_storage); }",
            "uint32_t getSubmenu() const noexcept { return m_Submenu.m_storage; }",
            "bool isShown() const noexcept { return m_IsShown; }",
        ]:
            self.assertIn(token, pause)

        general_window = self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_cgeneral_window.generated.h")
        for token in [
            "EWindowStatus getStatus() const noexcept { return static_cast<EWindowStatus>(m_Status.m_storage); }",
            "uint32_t getCursorIndex() const noexcept { return m_CursorIndex.m_storage; }",
            "uint32_t getSelectedIndex() const noexcept { return m_SelectedIndex.m_storage; }",
        ]:
            self.assertIn(token, general_window)

    def test_sgfx_hud_asset_binding_validator_resolves_scene_bindings_against_extracted_assets(self):
        # Phase 271: the validator reads every kSceneBindings row from the
        # generated headers, resolves each `projectName` to the extracted
        # `.yncp` file path via the YNCP native component map, and emits a
        # manifest that pairs each binding with its on-disk asset status.
        # The committed validation manifest must show every gauge-cluster
        # binding resolving to an existing YNCP file.
        validation = json.loads(self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_asset_binding_validation.generated.json"))
        self.assertEqual(
            validation["schema"], "sward-sgfx-hud-asset-binding-validation-v1")
        self.assertGreater(validation["bindingCount"], 0,
            "validator must find at least one SceneBinding to validate")
        # Every validated binding for the gauge cluster must at least have
        # the asset present on disk; "ok" or "asset-ok-scene-list-empty"
        # are both acceptable terminal states (the YNCP map does not
        # currently carry per-scene paths so cross-check is best-effort).
        for entry in validation["validations"]:
            status = entry["status"]
            self.assertTrue(
                status.startswith("ok")
                or status.startswith("asset-ok-scene-list-empty"),
                f"Binding {entry['member_name']} unexpectedly failed validation: {status}")
            self.assertTrue(entry["asset_exists"],
                f"Binding {entry['member_name']} resolved to a non-existent asset: {entry}")
            self.assertTrue(entry["asset_magic_ok"],
                f"Binding {entry['member_name']} asset failed magic-byte check: {entry}")
        # Specifically the SpeedGauge / RingEnergyGauge / GaugeFrame /
        # SpeedCount bindings must all be present (they correspond to the
        # cross-validated runtime sweep finds).
        member_names = {v["member_name"] for v in validation["validations"]}
        self.assertIn("m_rcSpeedGauge", member_names)
        self.assertIn("m_rcRingEnergyGauge", member_names)
        self.assertIn("m_rcGaugeFrame", member_names)

    def test_sgfx_hud_asset_binding_validator_handles_unresolved_project(self):
        # Phase 271: the validator must produce a useful status even when
        # the SceneBinding references a project the YNCP native map does
        # not know about. Build a synthetic generated header + map and
        # assert the resolution behavior end-to-end.
        import importlib.util
        import sys
        import tempfile

        validator_path = ROOT / "research_uiux/tools/validate_sgfx_hud_asset_bindings.py"
        spec = importlib.util.spec_from_file_location(
            "sgfx_hud_validator_under_test", validator_path)
        module = importlib.util.module_from_spec(spec)
        sys.modules["sgfx_hud_validator_under_test"] = module
        spec.loader.exec_module(module)

        synthetic_header = (
            "#pragma once\n"
            "namespace sward::ui_runtime::generated::sgfx_hud {\n"
            "    static constexpr std::array<SceneBinding, 2> kSceneBindings =\n"
            "    {{\n"
            "        {\"m_rcKnown\", 0xE0, \"ui_known\", \"ui_known/scene_a\", \"cross-validated\", 1},\n"
            "        {\"m_rcMissing\", 0xE8, \"ui_does_not_exist\", \"ui_does_not_exist/x\", \"inferred-owner\", 1},\n"
            "    }};\n"
            "}\n"
        )

        with tempfile.TemporaryDirectory() as tmp:
            tmp_root = Path(tmp)
            headers_dir = tmp_root / "research_uiux/runtime_reference/include/sward/ui_runtime"
            headers_dir.mkdir(parents=True)
            (headers_dir / "fake_class.generated.h").write_text(
                synthetic_header, encoding="utf-8")

            extracted_root = tmp_root / "extracted_assets/full_install_archives/game/Known"
            extracted_root.mkdir(parents=True)
            (extracted_root / "ui_known.yncp").write_bytes(
                b"YNCP" + b"\x00" * 60)

            yncp_map = tmp_root / "research_uiux/data/yncp_native_component_map.json"
            yncp_map.parent.mkdir(parents=True)
            yncp_map.write_text(json.dumps({
                "screen_groups": {
                    "test_group": [
                        {
                            "project": "ui_known",
                            "relative_path": "game/Known/ui_known.yncp",
                            # Phase 272 composes scene paths from
                            # `(node_path, scene_name)` pairs.
                            "scenes": [
                                {"node_path": "Root", "scene_name": "scene_a"},
                            ],
                        },
                    ],
                },
            }), encoding="utf-8")

            output = tmp_root / "validation.json"
            argv_backup = sys.argv
            sys.argv = [
                "validate_sgfx_hud_asset_bindings.py",
                "--repo-root", str(tmp_root),
                "--headers-dir",
                str(headers_dir.relative_to(tmp_root).as_posix()),
                "--yncp-native-map",
                str(yncp_map.relative_to(tmp_root).as_posix()),
                "--extracted-root",
                str((tmp_root / "extracted_assets/full_install_archives").relative_to(tmp_root).as_posix()),
                "--output", str(output.relative_to(tmp_root).as_posix()),
            ]
            try:
                rc = module.main()
            finally:
                sys.argv = argv_backup
            self.assertEqual(rc, 0)

            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(payload["bindingCount"], 2)
            statuses = {v["member_name"]: v["status"] for v in payload["validations"]}
            self.assertTrue(statuses["m_rcKnown"].startswith("ok"),
                f"Known project should resolve and find the scene: {statuses['m_rcKnown']}")
            self.assertTrue(statuses["m_rcMissing"].startswith("unresolved-project"),
                f"Missing project should be flagged unresolved-project: {statuses['m_rcMissing']}")

    def test_sgfx_hud_validator_composes_scene_paths_from_node_path_and_scene_name(self):
        # Phase 272: the validator must produce `<project>[/<sub>]/<scene>`
        # paths from each YNCP scene's (node_path, scene_name) pair so it
        # can match the runtime sweep's SceneBindings format.
        import importlib.util
        import sys

        validator_path = ROOT / "research_uiux/tools/validate_sgfx_hud_asset_bindings.py"
        spec_loader = importlib.util.spec_from_file_location(
            "sgfx_hud_validator_phase272_under_test", validator_path)
        module = importlib.util.module_from_spec(spec_loader)
        sys.modules["sgfx_hud_validator_phase272_under_test"] = module
        spec_loader.loader.exec_module(module)

        composed = module._compose_project_scene_paths(
            "ui_playscreen",
            {"scenes": [
                {"node_path": "Root", "scene_name": "so_speed_gauge"},
                {"node_path": "Root", "scene_name": "gauge_frame"},
                {"node_path": "Root/add", "scene_name": "speed_count"},
                {"node_path": "Root/add/sub", "scene_name": "deeper"},
                {"node_path": "Root", "scene_name": ""},  # skipped — no name
                {"scene_name": "no_node_path_scene"},     # treated as Root
            ]},
        )
        self.assertEqual(composed, (
            "ui_playscreen/so_speed_gauge",
            "ui_playscreen/gauge_frame",
            "ui_playscreen/add/speed_count",
            "ui_playscreen/add/sub/deeper",
            "ui_playscreen/no_node_path_scene",
        ))

        # The committed live validation manifest must show every gauge-
        # cluster binding resolving to `ok` (Phase 272 closed the
        # asset-ok-scene-list-empty path).
        validation = json.loads(self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_asset_binding_validation.generated.json"))
        ok_count = validation["summary"].get("ok", 0)
        self.assertGreaterEqual(ok_count, 4,
            "Phase 272 must have at least 4 'ok' validations after the "
            "scene-path composition fix; live summary was: "
            f"{validation['summary']}")

    def test_sgfx_hud_chud_pause_methods_real_method_bodies(self):
        # Phase 273 + 276: hand-written real method bodies on top of the
        # generated CHudPause layout header. The .cpp file is included in
        # the repo and must reference the generated header + the four
        # state-machine helper functions.
        path = ROOT / "research_uiux/runtime_reference/src/sgfx_hud_chud_pause_methods.cpp"
        self.assertTrue(path.is_file(),
            "Phase 273 method-body .cpp must exist at the documented path")
        text = path.read_text(encoding="utf-8")
        for token in [
            "Phase 273 / 276: hand-written method bodies for `class CHudPause`.",
            '#include "sward/ui_runtime/sgfx_hud_chud_pause.generated.h"',
            "namespace sward::ui_runtime::generated::sgfx_hud",
            "bool isPauseQuitDialogArmed(const CHudPause& pause)",
            "ETransitionType::eTransitionType_Quit",
            "bool isPauseInteractive(const CHudPause& pause)",
            "pause.isVisible()",
            "pause.isShown()",
            "ETransitionType::eTransitionType_Undefined",
            "bool isPauseShowingSubmenu(const CHudPause& pause)",
            "pause.getSubmenu()",
            "bool isPauseMiscMenuActionAccepted(const CHudPause& pause)",
            "EMenuType::eMenuType_Misc",
            "EStatusType::eStatusType_Accept",
            "static_assert(\n        sizeof(CHudPause) >= 0x1B9,",
        ]:
            self.assertIn(token, text)

    def test_sgfx_hud_csave_icon_emitted_via_swa_padding_fallback(self):
        # Phase 274: SaveIcon's SWA API header has no SWA_ASSERT_OFFSETOF
        # entries, so the parser falls back to walking the SWA_INSERT_PADDING
        # directives. The recomp's `lwz r3, 216(r31)` in `sub_824E5170`
        # confirms `m_IsVisible` lives at offset 0xD8.
        save_icon = self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_csave_icon.generated.h")
        for token in [
            "class CSaveIcon",
            "bool m_IsVisible;",
            "static_assert(offsetof(CSaveIcon, m_IsVisible) == 0xD8,",
            "bool isVisible() const noexcept { return m_IsVisible; }",
            "Hedgehog::Universe::CUpdateUnit",
            "api/SWA/HUD/SaveIcon/SaveIcon.h",
        ]:
            self.assertIn(token, save_icon)

    def test_sgfx_hud_csd_project_loader_header_and_smoke_test_present(self):
        # Phase 275: header-only C++ YNCP/CPAF loader with a paired
        # smoke-test translation unit. The header must declare the
        # CsdProjectFile struct, the magic enum, and the entry-point
        # function; the smoke test must include the header and call into
        # `loadCsdProjectFile()`.
        loader_path = ROOT / "research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_hud_csd_project_loader.hpp"
        self.assertTrue(loader_path.is_file())
        loader = loader_path.read_text(encoding="utf-8")
        for token in [
            "Phase 275: C++ loader for Sonic Unleashed CSD project files",
            "namespace sward::ui_runtime::generated::sgfx_hud",
            "enum class CsdProjectMagic : std::uint8_t",
            "struct CsdProjectFile",
            "std::filesystem::path        sourcePath;",
            "CsdProjectMagic              outerMagic = CsdProjectMagic::Unknown;",
            "constexpr bool hasRecognizedMagic() const noexcept",
            "constexpr bool hasYncpPayload() const noexcept",
            "inline CsdProjectFile loadCsdProjectFile(const std::filesystem::path& path)",
            "inline bool isLoadableCsdProject(const std::filesystem::path& path)",
            'classifyOuterMagic',
            'CPAF',
            'YNCP',
            'XNCP',
        ]:
            self.assertIn(token, loader)

        smoke_path = ROOT / "research_uiux/runtime_reference/src/sgfx_hud_csd_project_loader_smoke_test.cpp"
        self.assertTrue(smoke_path.is_file())
        smoke = smoke_path.read_text(encoding="utf-8")
        for token in [
            "#include \"sward/ui_runtime/sgfx_hud_csd_project_loader.hpp\"",
            # Phase 281: argv handling moved into a small loop that sets
            # `path` after stripping the `--json` flag, so the loader
            # call is now `loadCsdProjectFile(path)` rather than the
            # original `loadCsdProjectFile(argv[1])`.
            "loadCsdProjectFile(path)",
            "loadStatus",
            "outerMagicChars",
            # Phase 281 dropped the verbose innerYncpMagicOffset trace
            # line in favor of the structured JSON dump (`emitJson`); the
            # field is still declared on the loader struct, just no
            # longer printed by the smoke driver.
            "rootSceneIds",
        ]:
            self.assertIn(token, smoke)

        # The Python validator already proves end-to-end on the same set
        # of real assets; the C++ loader's surface mirrors the validator's
        # acceptance set so a follow-up build pipeline can swap the
        # validator for the loader without behavioral drift.
        validation = json.loads(self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_asset_binding_validation.generated.json"))
        for entry in validation["validations"]:
            self.assertTrue(entry["asset_magic_ok"],
                f"Validator marked {entry['member_name']} as bad-magic — the "
                "C++ loader's accept set (CPAF / YNCP / XNCP) should have "
                "matched too: " + str(entry))

    def test_sgfx_hud_chud_sonic_stage_methods_real_method_bodies(self):
        # Phase 277 + 280: real method bodies on top of the runtime-
        # extended CHudSonicStage layout, including a destructor-port
        # helper that mirrors the SWA `sub_824D8CE8` cleanup order and
        # an asset-side readiness predicate that consults the C++ CSD
        # project loader.
        path = ROOT / "research_uiux/runtime_reference/src/sgfx_hud_chud_sonic_stage_methods.cpp"
        self.assertTrue(path.is_file(),
            "Phase 277 / 280 method-body .cpp must exist at the documented path")
        text = path.read_text(encoding="utf-8")
        for token in [
            'Phase 277: hand-written method bodies for `class CHudSonicStage`',
            '#include "sward/ui_runtime/sgfx_hud_chud_sonic_stage.generated.h"',
            '#include "sward/ui_runtime/sgfx_hud_csd_project_loader.hpp"',
            "namespace sward::ui_runtime::generated::sgfx_hud",
            "const SceneBinding* findSceneBindingByMember(std::string_view memberName)",
            "bool hasFullGaugeClusterBindings()",
            "std::size_t countCrossValidatedBindings()",
            "std::string_view ownedCsdProjectName()",
            "bool isAdditiveClusterBinding(const SceneBinding& binding)",
            # Phase 280 ports.
            "Phase 280: ported method bodies that mirror the SWA recomp flow",
            "bool isCHudSonicStageInPostConstructorState(const CHudSonicStage& hud)",
            "void releaseAllOwnedScenes(CHudSonicStage& hud)",
            "bool isPlayScreenAssetSideReadyForBinding",
            "release(hud.m_rcPtrField180);",
            "release(hud.m_rcPlayScreen);",
            "static_assert(\n        sizeof(CHudSonicStage) >= 0x188,",
            "static_assert(\n        kSceneBindings.size() >= 4,",
        ]:
            self.assertIn(token, text)

    def test_sgfx_hud_smoke_test_pipeline_present_and_buildable(self):
        # Phase 278 + 279 + 280: a PowerShell wrapper drives clang-cl
        # against the SGFX HUD smoke tests using the same VsDevCmd /
        # LLVM toolchain the main UI Lab build wraps. Verify the script
        # references both smoke targets and the key compile + run flow.
        ps1 = self.read("research_uiux/runtime_reference/tools/build_sgfx_hud_smoke_tests.ps1")
        for token in [
            "Phase 278",
            "VsDevCmd.bat",
            "C:\\Program Files\\LLVM\\bin",
            "vswhere.exe",
            "sgfx_hud_csd_project_loader_smoke_test.cpp",
            "sgfx_hud_chud_sonic_stage_methods_smoke_test.cpp",
            "clang-cl /nologo /std:c++17",
            "smoke target(s) failed",
        ]:
            self.assertIn(token, ps1)

        loader_smoke = self.read(
            "research_uiux/runtime_reference/src/sgfx_hud_csd_project_loader_smoke_test.cpp")
        for token in [
            "loadCsdProjectFile",
            "outerMagicChars",
            "rootSceneIds",
            "projectName",
            "ncpjSignature",
        ]:
            self.assertIn(token, loader_smoke)

        methods_smoke = self.read(
            "research_uiux/runtime_reference/src/sgfx_hud_chud_sonic_stage_methods_smoke_test.cpp")
        for token in [
            "isCHudSonicStageInPostConstructorState",
            "releaseAllOwnedScenes",
            "isPlayScreenAssetSideReadyForBinding",
            "findSceneBindingByMember",
            "hasFullGaugeClusterBindings",
            "ownedCsdProjectName",
        ]:
            self.assertIn(token, methods_smoke)

    def test_sgfx_hud_csd_project_loader_extracts_scene_ids_and_walks_children(self):
        # Phase 279 + 280: the C++ loader extracts root scene IDs AND
        # recursively walks child nodes to populate `allSceneRefs`. The
        # helpers + test fields the runtime-extended SceneBinding
        # validator needs are all present in the header.
        loader = self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_csd_project_loader.hpp")
        for token in [
            "Phase 279: parsed CSD project metadata",
            "struct CsdSceneId",
            "struct CsdSceneRef",
            "std::vector<CsdSceneId>      rootSceneIds;",
            "std::vector<CsdSceneRef>     allSceneRefs;",
            "inline std::string parseCsdProjectInPlace(CsdProjectFile& out) noexcept",
            "inline void walkCsdNodeRecursive(",
            "constexpr int kMaxDepth = 6",
            "isBigEndianContainer(",
            'CsdProjectMagic::Cpaf',
            'CsdProjectMagic::Fapc',
        ]:
            self.assertIn(token, loader)

    def test_sgfx_hud_loader_parity_report_shows_full_csd_project_coverage(self):
        # Phase 281 / 283: the parity report is the proof that every
        # retail CSD project the C++ loader reads produces the SAME scene
        # set the YNCP native component map (canonical Python parser)
        # produces. A 41/41 match across every entry in
        # `extracted_assets/full_install_archives/` is concrete evidence
        # the human-readable port loads what Sonic Unleashed actually
        # displays — not an inspired or reconstructed approximation.
        report = json.loads(self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_loader_parity_report.generated.json"))
        self.assertEqual(report["schema"], "sward-sgfx-hud-loader-parity-report-v1")
        self.assertGreaterEqual(report["projectCount"], 25,
            "parity sweep should cover at least 25 retail CSD projects; "
            f"actual: {report['projectCount']}")
        self.assertEqual(report["summary"]["parity_ok"], report["projectCount"],
            "every project in the parity report must show parity-ok; "
            f"summary: {report['summary']}")
        self.assertEqual(report["summary"]["parity_mismatch"], 0)
        self.assertEqual(report["summary"]["loader_error"], 0)
        # Spot-check a representative entry: ui_playscreen must be in the
        # report and must show parity-ok with a non-empty scene set.
        playscreen = next(
            (e for e in report["entries"]
             if e["project_name"] == "ui_playscreen"
             and "Sonic" in e["relative_path"]),
            None)
        self.assertIsNotNone(playscreen,
            "parity report must include ui_playscreen from game/Sonic/")
        self.assertTrue(playscreen["status"].startswith("parity-ok"),
            f"ui_playscreen parity status: {playscreen['status']}")
        self.assertGreater(playscreen["yncp_map_scene_count"], 0)
        self.assertEqual(
            playscreen["yncp_map_scene_count"],
            playscreen["cpp_loader_scene_count"])

    def test_sgfx_hud_csd_loader_extracts_texture_names(self):
        # Phase 284: the C++ loader walks both FAPC resources and pulls
        # the NXTL chunk's per-index DDS texture names. The header must
        # declare the textureNames field and a parseTextureList helper.
        loader = self.read(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_csd_project_loader.hpp")
        for token in [
            "Phase 284: every texture name referenced by the project",
            "std::vector<std::string>     textureNames;",
            "inline void parseTextureList(",
            'NXTL',
            "out.textureNames",
        ]:
            self.assertIn(token, loader)

        # The smoke test must also surface texture names so a downstream
        # driver / parity tool can consume them. The JSON dump has the
        # field as an escaped C++ string literal `\"textureNames\":`;
        # the human-readable trace uses `textureNames:`.
        smoke = self.read(
            "research_uiux/runtime_reference/src/sgfx_hud_csd_project_loader_smoke_test.cpp")
        for token in [
            "loaded.textureNames",
            "\\\"textureNames\\\":",
            "textureNames:",
        ]:
            self.assertIn(token, smoke)

    def test_sgfx_hud_remaining_class_method_bodies_present(self):
        # Phase 285: every previously layout-only HUD class now has its
        # own hand-written method-body translation unit. The PowerShell
        # smoke wrapper compiles them alongside the existing CHudPause /
        # CHudSonicStage methods to validate the layouts and includes
        # are well-formed.
        for header_relpath, expected_helper in [
            (
                "research_uiux/runtime_reference/src/sgfx_hud_cgeneral_window_methods.cpp",
                "isGeneralWindowVisible",
            ),
            (
                "research_uiux/runtime_reference/src/sgfx_hud_cloading_methods.cpp",
                "isLoadingScreenVisible",
            ),
            (
                "research_uiux/runtime_reference/src/sgfx_hud_csave_icon_methods.cpp",
                "isSaveInProgress",
            ),
        ]:
            text = self.read(header_relpath)
            self.assertIn("Phase 285", text)
            self.assertIn(expected_helper, text)
            self.assertIn("namespace sward::ui_runtime::generated::sgfx_hud", text)
            self.assertIn("static_assert(", text)

        # The smoke-test wrapper must include all three new methods .cpp
        # files in the link list so a future regression in any of them
        # breaks the build immediately.
        wrapper = self.read("research_uiux/runtime_reference/tools/build_sgfx_hud_smoke_tests.ps1")
        for fname in [
            "sgfx_hud_chud_pause_methods.cpp",
            "sgfx_hud_cgeneral_window_methods.cpp",
            "sgfx_hud_cloading_methods.cpp",
            "sgfx_hud_csave_icon_methods.cpp",
        ]:
            self.assertIn(fname, wrapper)

    def test_sgfx_hud_layout_paddings_are_public_for_standard_layout(self):
        # Phase 282: every committed generated header must declare its
        # paddings in the same access section as its members so the
        # class qualifies as standard-layout per [class.prop] and
        # `offsetof` is well-defined for the static_asserts that follow.
        # `private:` access blocks for paddings are forbidden because
        # they trigger -Winvalid-offsetof under clang/MSVC.
        for header_relpath in [
            "research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_hud_chud_sonic_stage.generated.h",
            "research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_hud_chud_pause.generated.h",
            "research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_hud_cgeneral_window.generated.h",
            "research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_hud_cloading.generated.h",
            "research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_hud_csave_icon.generated.h",
        ]:
            text = self.read(header_relpath)
            # The padding declarations are now plain `std::array<...>
            # m_padding...;` lines without a leading `private:` access
            # specifier in the same line.
            self.assertNotIn("private: std::array<std::uint8_t,", text,
                f"{header_relpath} still has private-access paddings; the class "
                "is not standard-layout and offsetof asserts will warn")

    def test_sgfx_hud_layout_parses_swa_api_header_rcptr_declarations(self):
        # Phase 267: focused unit test for the SWA API header parser. The
        # parser must accept both fully-qualified and brief RCPtr<T>
        # declarations because real SWA API headers mix the two forms.
        import importlib.util
        import sys
        import tempfile

        generator_path = ROOT / "research_uiux/tools/build_sgfx_hud_layout.py"
        spec = importlib.util.spec_from_file_location(
            "sgfx_hud_layout_parser_under_test", generator_path)
        module = importlib.util.module_from_spec(spec)
        sys.modules["sgfx_hud_layout_parser_under_test"] = module
        spec.loader.exec_module(module)

        with tempfile.TemporaryDirectory() as tmp:
            api_path = Path(tmp) / "FakeHud.h"
            api_path.write_text(
                "#pragma once\n"
                "namespace SWA {\n"
                "    class CFakeHud {\n"
                "    public:\n"
                "        Chao::CSD::RCPtr<Chao::CSD::CProject> m_rcProject;\n"
                "        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcMain;\n"
                "        RCPtr<CScene> m_rcAlt;\n"
                "        RCPtr<CNode> m_rcCounter;\n"
                "    };\n"
                "}\n",
                encoding="utf-8",
            )
            result = module.parse_swa_api_header(api_path)
        self.assertEqual(result, {
            "m_rcProject": "CProject",
            "m_rcMain": "CScene",
            "m_rcAlt": "CScene",
            "m_rcCounter": "CNode",
        })
        # Missing file returns empty mapping rather than raising.
        self.assertEqual(module.parse_swa_api_header(Path(tmp) / "missing.h"), {})

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

    def test_ui_lab_runtime_build_helper_uses_transient_short_drive_only(self):
        build_script = self.read("research_uiux/runtime_reference/tools/build_unleashed_recomp_ui_lab.ps1")
        capture_script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")
        manual_script = self.read("research_uiux/runtime_reference/tools/launch_unleashed_recomp_ui_lab_manual.ps1")

        self.assertIn("[switch]$KeepSubstDrive", build_script)
        self.assertIn("$createdSubstDrive = $false", build_script)
        self.assertIn("Remove-TransientSubstDrive", build_script)
        self.assertIn("if ($createdSubstDrive -and -not $KeepSubstDrive)", build_script)
        self.assertIn('cmd /c "subst $drive /D"', build_script)
        self.assertIn("$hostBuildPath = Join-Path $root $BuildDir", build_script)
        self.assertIn("Built UI Lab runtime: $hostExe", build_script)
        self.assertIn(
            '[string]$ExePath = "local_build_env\\ur103clean\\b\\ui_lab_runtime\\UnleashedRecomp\\UnleashedRecomp.exe"',
            capture_script,
        )
        self.assertIn(
            '[string]$BuildExePath = "local_build_env\\ur103clean\\b\\ui_lab_runtime\\UnleashedRecomp\\UnleashedRecomp.exe"',
            manual_script,
        )
        self.assertNotIn('[string]$ExePath = "W:\\', capture_script)
        self.assertNotIn('[string]$BuildExePath = "W:\\', manual_script)

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

    def test_ui_lab_live_bridge_reports_dynamic_observed_screen_apart_from_requested_target(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")

        for token in [
            "ObservedRuntimeScreen",
            "BuildObservedRuntimeScreen",
            "IsPassiveObservedCsdProject",
            "WasObservedFrameRecent",
            '"observedScreen"',
            '"observedScreenLabel"',
            '"observedCsdProject"',
            '"observedSourceFamily"',
            '"observedScreenSource"',
            '"targetObservedMismatch"',
            'project == "ui_itemresult"',
            '"ui_saveicon"',
            "stage-target-live-inspector",
            "requested-target-fallback",
            'observed.token != target.token',
        ]:
            self.assertIn(token, ui_lab)

        for passive_overlay in [
            '"ui_itembox"',
            '"ui_qte"',
            '"ui_lcursor"',
            '"ui_lcursor_enemy"',
        ]:
            self.assertIn(passive_overlay, ui_lab)

        for stage_ready_guard in [
            "g_loggedStageTargetReady",
            "project == target.primaryCsdScene",
        ]:
            self.assertIn(stage_ready_guard, ui_lab)

        for mapping in [
            '{ "ui_title", "title-runtime"',
            '{ "ui_loading", "loading"',
            '{ "ui_playscreen", "sonic-hud"',
            '{ "ui_prov_playscreen", "extra-stage-hud"',
            '{ "ui_pause", "pause"',
            '{ "ui_status", "status"',
            '{ "ui_result", "result"',
            '{ "ui_itemresult", "item-result"',
            '{ "ui_worldmap", "world-map"',
            '{ "ui_worldmap_help", "world-map-help"',
        ]:
            self.assertIn(mapping, ui_lab)

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

        self.assertIn('[ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]', client)
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
        self.assertIn('[ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]', client)
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
        self.assertIn('[ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]', client)
        self.assertIn("Phase 150", harvest)
        self.assertIn("render-thread material submit hook", harvest)
        self.assertIn("raw D3D12/Vulkan backend capture pending", harvest)

    def test_ui_lab_phase151_correlates_draw_list_and_backend_submit_materials(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        ui_lab_header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        video = self.read("UnleashedRecomp/gpu/video.cpp")
        client = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "RuntimeMaterialCorrelation",
            "BuildRuntimeMaterialCorrelationJson",
            "BuildRuntimeMaterialCorrelationPairs",
            '"materialCorrelationOracle"',
            '"uiDrawSequence"',
            '"gpuSubmitSequence"',
            '"correlationMethod": "same-frame-order-window"',
            '"blendSemantic"',
            '"blendOperationSemantic"',
            '"samplerSemantic"',
            '"addressSemantic"',
            '"halfPixelOffset"',
            '"rawBackendCommandStatus"',
            '"ui-material-correlation"',
            "RenderBlendName",
            "RenderTextureFilterName",
            "RenderTextureAddressName",
            "D3DBLEND_SRCALPHA",
            "D3DBLEND_INVSRCALPHA",
            "D3DBLEND_ONE",
            "D3DBLEND_ZERO",
            "D3DTEXF_POINT",
            "D3DTEXF_LINEAR",
            "D3DTADDRESS_CLAMP",
            "D3DTADDRESS_WRAP",
            "src-alpha/inv-src-alpha",
            "src-alpha/one additive",
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("OnBackendMaterialSubmit", ui_lab_header)
        self.assertIn("halfPixelOffsetX", ui_lab_header)
        self.assertIn("halfPixelOffsetY", ui_lab_header)
        self.assertIn("OnRawBackendCommand", ui_lab_header)
        self.assertIn("RecordUiLabBackendMaterialSubmit", video)
        self.assertIn("g_sharedConstants.halfPixelOffsetX", video)
        self.assertIn("g_sharedConstants.halfPixelOffsetY", video)
        self.assertIn("UiLab::OnRawBackendCommand", video)
        self.assertIn('"RHI command-list boundary"', video)
        self.assertIn(
            '[ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]',
            client,
        )
        self.assertIn("Phase 151", harvest)
        self.assertIn("same-frame-order-window", harvest)
        self.assertIn("named Xenos/D3D-ish material semantics", harvest)
        self.assertIn("raw D3D12/Vulkan command capture pending", harvest)

    def test_ui_lab_phase152_exposes_backend_resolved_submit_oracle(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        ui_lab_header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        d3d12_header = self.read("UnleashedRecomp/gpu/rhi/plume_d3d12.h")
        d3d12 = self.read("UnleashedRecomp/gpu/rhi/plume_d3d12.cpp")
        vulkan_header = self.read("UnleashedRecomp/gpu/rhi/plume_vulkan.h")
        vulkan = self.read("UnleashedRecomp/gpu/rhi/plume_vulkan.cpp")
        client = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "RuntimeBackendResolvedSubmit",
            "g_runtimeBackendResolvedSubmits",
            "OnResolvedBackendSubmit",
            "BuildRuntimeBackendResolvedJson",
            '"backendResolvedSubmitOracle"',
            '"resolvedBackendStatus"',
            '"nativeCommand"',
            '"nativePipelineHandle"',
            '"nativePipelineLayoutHandle"',
            '"pipelineBlendState"',
            '"renderTargetFormat0"',
            '"depthTargetFormat"',
            '"framebufferSize"',
            '"resolvedPipelineKnown"',
            '"backendResolvedJoinMethod": "same-frame-order-window"',
            '"ui-backend-resolved"',
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("OnResolvedBackendSubmit", ui_lab_header)
        self.assertIn("UiLab::OnResolvedBackendSubmit", d3d12)
        self.assertIn("D3D12.DrawInstanced", d3d12)
        self.assertIn("D3D12.DrawIndexedInstanced", d3d12)
        self.assertIn("uiLabBlend0", d3d12_header)
        self.assertIn("uiLabRenderTargetFormat0", d3d12_header)
        self.assertIn("activeGraphicsPipeline", d3d12_header)
        self.assertIn("UiLab::OnResolvedBackendSubmit", vulkan)
        self.assertIn("vkCmdDraw", vulkan)
        self.assertIn("vkCmdDrawIndexed", vulkan)
        self.assertIn("uiLabBlend0", vulkan_header)
        self.assertIn("uiLabRenderTargetFormat0", vulkan_header)
        self.assertIn("activeGraphicsPipeline", vulkan_header)
        self.assertIn(
            '[ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]',
            client,
        )
        self.assertIn("Phase 152", harvest)
        self.assertIn("backend-resolved D3D12/Vulkan submit details", harvest)
        self.assertIn("resolved PSO/blend/framebuffer state", harvest)

    def test_ui_lab_phase153_exposes_backend_material_parity_hints(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        renderer = self.read("research_uiux/runtime_reference/examples/su_ui_asset_renderer.cpp")
        tests = self.read("research_uiux/runtime_reference/examples/test_su_ui_asset_renderer.py")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "BackendMaterialParityHint",
            "BuildBackendMaterialParityHintsJson",
            "RuntimeBackendMaterialParityHint",
            '"backendMaterialParityHints"',
            '"materialParityHint"',
            '"blendParityPolicy": "backend-resolved-pso-blend"',
            '"framebufferParityPolicy": "backend-resolved-framebuffer-registration"',
            '"textureViewSamplerGap": "pending-descriptor-view-decode"',
            '"textMovieSfxGap": "pending-title-loading-media-timing"',
            '"materialParityStatus"',
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "FrontendBackendMaterialParityTriage",
            "buildFrontendBackendMaterialParityTriage",
            "runRendererMaterialParityHintsSmoke",
            "--renderer-material-parity-hints-smoke",
            "phase153-backend-material-parity-hints",
            "material_parity_policy=backend-resolved-pso-blend-framebuffer",
            "texture_view_sampler_gap=pending",
            "text_movie_sfx_gap=pending",
        ]:
            self.assertIn(token, renderer)

        self.assertIn("test_renderer_material_parity_hints_smoke_reports_backend_policy", tests)
        self.assertIn("Phase 153", harvest)
        self.assertIn("backend-resolved PSO/blend/framebuffer material parity hints", harvest)
        self.assertIn("texture-view/sampler descriptor internals remain pending", harvest)

    def test_ui_lab_phase154_exposes_texture_sampler_descriptor_semantics(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        ui_lab_header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        video = self.read("UnleashedRecomp/gpu/video.cpp")
        renderer = self.read("research_uiux/runtime_reference/examples/su_ui_asset_renderer.cpp")
        tests = self.read("research_uiux/runtime_reference/examples/test_su_ui_asset_renderer.py")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "RuntimeTextureDescriptorSemantic",
            "RuntimeSamplerDescriptorSemantic",
            "OnBackendTextureDescriptorResolved",
            "OnBackendSamplerDescriptorResolved",
            "BuildBackendDescriptorSemanticsJson",
            '"backendDescriptorSemantics"',
            '"textureViewSamplerStatus"',
            '"textureDescriptorSemantic"',
            '"samplerDescriptorSemantic"',
            '"textureDescriptorPolicy": "runtime-texture-view-descriptor-state"',
            '"samplerDescriptorPolicy": "runtime-sampler-descriptor-state"',
            '"vendorDescriptorCaptureGap": "pending-native-descriptor-dump"',
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("OnBackendTextureDescriptorResolved", ui_lab_header)
        self.assertIn("OnBackendSamplerDescriptorResolved", ui_lab_header)
        self.assertIn("UiLab::OnBackendTextureDescriptorResolved", video)
        self.assertIn("UiLab::OnBackendSamplerDescriptorResolved", video)
        self.assertIn("texture->descriptorIndex", video)
        self.assertIn("g_samplerDescriptorSet->setSampler", video)

        for token in [
            "FrontendDescriptorSemanticsTriage",
            "buildFrontendDescriptorSemanticsTriage",
            "runRendererDescriptorSemanticsSmoke",
            "--renderer-descriptor-semantics-smoke",
            "phase154-texture-sampler-descriptor-semantics",
            "texture_sampler_policy=runtime-descriptor-state",
            "vendor_descriptor_gap=pending-native-descriptor-dump",
        ]:
            self.assertIn(token, renderer)

        self.assertIn("test_renderer_descriptor_semantics_smoke_reports_runtime_descriptor_policy", tests)
        self.assertIn("Phase 154", harvest)
        self.assertIn("runtime texture-view/sampler descriptor semantics", harvest)
        self.assertIn("native descriptor dump remains pending", harvest)

    def test_ui_lab_phase155_exposes_vendor_resource_capture_oracle(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        ui_lab_header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        d3d12 = self.read("UnleashedRecomp/gpu/rhi/plume_d3d12.cpp")
        vulkan = self.read("UnleashedRecomp/gpu/rhi/plume_vulkan.cpp")
        renderer = self.read("research_uiux/runtime_reference/examples/su_ui_asset_renderer.cpp")
        tests = self.read("research_uiux/runtime_reference/examples/test_su_ui_asset_renderer.py")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "RuntimeVendorTextureResourceView",
            "RuntimeVendorSamplerResourceView",
            "OnVendorTextureResourceViewResolved",
            "OnVendorSamplerResourceViewResolved",
            "BuildBackendVendorResourceCaptureJson",
            '"backendVendorResourceCapture"',
            '"vendorResourceCaptureStatus"',
            '"vendorResourceCapturePolicy": "native-rhi-resource-view-and-sampler-handles"',
            '"uiOnlyLayerCaptureStatus": "pending-runtime-ui-render-target-copy"',
            '"nativeCommandCaptureGap": "pending-full-vendor-command-buffer-dump"',
            '"nativeTextureResourceHandle"',
            '"nativeSamplerHandle"',
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("OnVendorTextureResourceViewResolved", ui_lab_header)
        self.assertIn("OnVendorSamplerResourceViewResolved", ui_lab_header)
        self.assertIn("UiLab::OnVendorTextureResourceViewResolved", d3d12)
        self.assertIn("UiLab::OnVendorSamplerResourceViewResolved", d3d12)
        self.assertIn("UiLab::OnVendorTextureResourceViewResolved", vulkan)
        self.assertIn("UiLab::OnVendorSamplerResourceViewResolved", vulkan)
        self.assertIn("NativeVkHandleToU64", vulkan)

        for token in [
            "FrontendVendorResourceCaptureTriage",
            "buildFrontendVendorResourceCaptureTriage",
            "runRendererVendorResourceCaptureSmoke",
            "--renderer-vendor-resource-capture-smoke",
            "phase155-vendor-resource-capture",
            "vendor_resource_policy=native-rhi-resource-view-sampler",
            "ui_only_layer_status=pending-runtime-ui-render-target-copy",
            "native_command_gap=pending-full-vendor-command-buffer-dump",
        ]:
            self.assertIn(token, renderer)

        self.assertIn("test_renderer_vendor_resource_capture_smoke_reports_native_resource_policy", tests)
        self.assertIn("Phase 155", harvest)
        self.assertIn("native RHI resource-view/sampler handle capture", harvest)
        self.assertIn("true UI-only rendered layer remains pending", harvest)

    def test_ui_lab_phase156_exposes_material_resource_view_parity_oracle(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        renderer = self.read("research_uiux/runtime_reference/examples/su_ui_asset_renderer.cpp")
        tests = self.read("research_uiux/runtime_reference/examples/test_su_ui_asset_renderer.py")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "RuntimeMaterialResourceViewParityStatus",
            "RuntimeNativeFormatLooksSrgb",
            "BuildBackendMaterialResourceViewParityJson",
            '"backendMaterialResourceViewParity"',
            '"materialResourceViewParityPolicy": "vendor-resource-view-alpha-gamma-srgb"',
            '"premultipliedAlphaPolicy": "runtime-blend-state-plus-vendor-resource-view"',
            '"gammaSrgbPolicy": "native-resource-view-format-classification"',
            '"premultipliedAlphaStatus"',
            '"gammaSrgbStatus"',
            '"resourceViewExactnessStatus"',
            '"resourceViewExactPairCount"',
            '"srgbTextureResourceViewCount"',
            '"uiOnlyRenderTargetCaptureProbe"',
            '"uiOnlyRenderTargetCapturePolicy": "copy-ui-render-target-before-present"',
            '"uiOnlyLayerCaptureStatus": "pending-runtime-ui-render-target-copy"',
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "FrontendMaterialResourceViewParityTriage",
            "buildFrontendMaterialResourceViewParityTriage",
            "runRendererMaterialResourceViewParitySmoke",
            "--renderer-material-resource-view-parity-smoke",
            "phase156-material-resource-view-parity",
            "material_parity_policy=vendor-resource-view-alpha-gamma-srgb",
            "ui_only_capture_policy=copy-ui-render-target-before-present",
            "resource_view_exactness=",
            "premultiplied_alpha_status=",
            "gamma_srgb_status=",
        ]:
            self.assertIn(token, renderer)

        self.assertIn("test_renderer_material_resource_view_parity_smoke_reports_resource_view_exactness", tests)
        self.assertIn("Phase 156", harvest)
        self.assertIn("premultiplied alpha/gamma/sRGB resource-view parity", harvest)
        self.assertIn("UI-only render-target capture remains pending", harvest)

    def test_ui_lab_phase157_exposes_vendor_command_resource_dump_oracle(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        renderer = self.read("research_uiux/runtime_reference/examples/su_ui_asset_renderer.cpp")
        tests = self.read("research_uiux/runtime_reference/examples/test_su_ui_asset_renderer.py")
        client = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "BuildRuntimeVendorCommandResourceDumpJson",
            "RuntimeVendorCommandResourceDumpStatus",
            '"vendorCommandResourceDump"',
            '"vendorCommandResourceDumpPolicy": "raw-backend-command-plus-resource-view-dump"',
            '"vendorCommandResourceDumpStatus"',
            '"rawBackendCommandCount"',
            '"backendResolvedSubmitCount"',
            '"textureResourceViewDumpCount"',
            '"samplerResourceViewDumpCount"',
            '"resourcePairDumpCount"',
            '"uiOnlyRenderedLayerStatus": "pending-runtime-ui-render-target-copy"',
            '"vendorCommandReplayGap": "pending-full-vendor-command-buffer-replay"',
            "ui-vendor-command-capture",
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn(
            '[ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]',
            client,
        )

        for token in [
            "FrontendVendorCommandResourceDumpTriage",
            "buildFrontendVendorCommandResourceDumpTriage",
            "runRendererVendorCommandResourceDumpSmoke",
            "--renderer-vendor-command-resource-dump-smoke",
            "phase157-vendor-command-resource-dump",
            "vendor_command_resource_dump_policy=raw-backend-command-plus-resource-view-dump",
            "ui_only_layer_status=pending-runtime-ui-render-target-copy",
            "vendor_command_replay_gap=pending-full-vendor-command-buffer-replay",
            "vendor_command_resource=",
        ]:
            self.assertIn(token, renderer)

        self.assertIn("test_renderer_vendor_command_resource_dump_smoke_reports_raw_backend_dump", tests)
        self.assertIn("Phase 157", harvest)
        self.assertIn("vendor command/resource dump", harvest)
        self.assertIn("UI-only render-target copy remains pending", harvest)

    def test_ui_lab_phase158_exposes_ui_render_target_capture_oracle(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        video = self.read("UnleashedRecomp/gpu/video.cpp")
        renderer = self.read("research_uiux/runtime_reference/examples/su_ui_asset_renderer.cpp")
        tests = self.read("research_uiux/runtime_reference/examples/test_su_ui_asset_renderer.py")
        client = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "BuildRuntimeUiOnlyRenderTargetCaptureJson",
            "ConsumeUiOnlyRenderTargetCapturePath",
            "OnUiOnlyRenderTargetCaptured",
            '"uiOnlyRenderTargetCapture"',
            '"uiOnlyRenderTargetCapturePolicy": "copy-active-ui-render-target-before-imgui-present"',
            '"uiOnlyRenderTargetCaptureStatus"',
            '"uiOnlyLayerCaptureStatus"',
            '"uiOnlyLayerIsolationStatus"',
            '"uiOnlyRenderTargetCapturePath"',
            '"uiOnlyRenderTargetCaptureSource"',
            "ui-layer-capture",
            "ui-layer-status",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "RenderCommandType::QueueUiLayerCapture",
            "QueueUiLabUiOnlyRenderTargetCapture",
            "active-render-target-before-imgui-present",
        ]:
            self.assertIn(token, video)

        self.assertIn(
            '[ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]',
            client,
        )

        for token in [
            "FrontendUiOnlyLayerCaptureTriage",
            "buildFrontendUiOnlyLayerCaptureTriage",
            "runRendererUiOnlyLayerCaptureSmoke",
            "--renderer-ui-layer-capture-smoke",
            "phase158-ui-render-target-capture",
            "ui_layer_capture_policy=copy-active-ui-render-target-before-imgui-present",
            "ui_layer_capture_status=",
            "ui_layer_isolation_status=",
            "ui_layer_capture_path=",
            "ui_layer_capture=",
        ]:
            self.assertIn(token, renderer)

        self.assertIn("test_renderer_ui_layer_capture_smoke_reports_render_target_readback", tests)
        self.assertIn("Phase 158", harvest)
        self.assertIn("UI render-target capture", harvest)
        self.assertIn("active render target may still include scene/background pixels", harvest)

    def test_ui_lab_phase159_wires_ui_layer_pixel_compare_sidecar_oracle(self):
        renderer = self.read("research_uiux/runtime_reference/examples/su_ui_asset_renderer.cpp")
        tests = self.read("research_uiux/runtime_reference/examples/test_su_ui_asset_renderer.py")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "FrontendUiLayerPixelCompareRecord",
            "findLatestUiLayerCaptureBmpPathForTarget",
            "renderFrontendPolicyUiLayerPixelCompare",
            "writeFrontendUiLayerPixelCompareManifest",
            "runRendererUiLayerPixelCompareSmoke",
            "--renderer-ui-layer-pixel-compare-smoke",
            "phase159-ui-layer-pixel-compare",
            "ui_layer_pixel_compare_manifest=",
            "ui_layer_pixel_delta=",
            "ui_layer_capture_isolation=",
            "ui_layer_oracle_upgrade=dedicated-ui-target-or-vendor-replay-needed",
            "text_movie_sfx_status=title-loading-media-timing-reference-ready-audio-id-pending",
        ]:
            self.assertIn(token, renderer)

        self.assertIn("test_renderer_ui_layer_pixel_compare_smoke_reports_visual_delta_or_missing_capture", tests)
        self.assertIn("Phase 159", harvest)
        self.assertIn("UI-layer pixel comparison", harvest)
        self.assertIn("dedicated UI target or vendor replay", harvest)

    def test_ui_lab_capture_helper_can_arm_ui_layer_capture_per_target(self):
        script = self.read("research_uiux/runtime_reference/tools/capture_unleashed_recomp_ui_lab.ps1")
        harvest = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "[switch]$UiLayerCapture",
            "[switch]$RequireUiLayerCapture",
            "Wait-UiLabUiLayerCapture",
            "Invoke-UiLabBridgeJsonCommand",
            "ui-layer-capture",
            "ui-layer-status",
            "uiLayerCaptureRequest =",
            "uiLayerCaptureStatus =",
            "uiLayerCaptureAttempted =",
            "uiLayerCapturePassed =",
            "uiLayerCaptureRequired =",
            "ui-layer-capture-timeout",
            "ui-layer-capture-observed",
        ]:
            self.assertIn(token, script)

        self.assertIn("Phase 160", harvest)
        self.assertIn("-UiLayerCapture", harvest)
        self.assertIn("ui-layer-capture-observed", harvest)

    def test_ui_lab_has_repo_safe_live_bridge_client_tool(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1"
        self.assertTrue(script_path.is_file())

        script = self.read("research_uiux/runtime_reference/tools/query_unleashed_recomp_ui_lab_bridge.ps1")
        for token in [
            "NamedPipeClientStream",
            "sward_ui_lab_live",
            "Invoke-UiLabBridgeCommand",
            "Read-UiLabBridgeResponse",
            '[ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]',
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
            '[ValidateSet("state", "events", "route-status", "native-foreground-status", "native-make-observe", "native-owner-discovery", "native-owner-scan", "native-owner-layout", "native-foreground-attach", "native-foreground-detach", "native-motion-play", "native-motion-stop", "native-motion-scrub", "ui-oracle", "ui-draw-list", "ui-gpu-submit", "ui-material-correlation", "ui-backend-resolved", "ui-vendor-command-capture", "ui-layer-capture", "ui-layer-status", "route", "reset", "set-global", "capture", "help")]',
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
            "Do not call RCPtr::Get() here",
            "OnHudSonicStageOwnerFieldSample",
            "PPC_FUNC_IMPL(__imp__sub_824D89B0)",
            "PPC_FUNC_IMPL(__imp__sub_824D9308)",
            "PPC_FUNC_IMPL(__imp__sub_824D95F8)",
            "raw CHudSonicStage owner hook",
        ]:
            self.assertIn(token, sonic)
        self.assertNotIn("GuestAddressOf(pHudSonicStage->m_rcPlayScreen.Get())", sonic)
        self.assertNotIn("GuestAddressOf(pHudSonicStage->m_rcSpeedGauge.Get())", sonic)
        self.assertNotIn("GuestAddressOf(pHudSonicStage->m_rcRingEnergyGauge.Get())", sonic)
        self.assertNotIn("GuestAddressOf(pHudSonicStage->m_rcGaugeFrame.Get())", sonic)

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
            # Phase 265: range expanded to 0xE0..0x184 from CHudSonicStage
            # constructor decode (sub_824D89B0); the older 0xE0..0x14C range
            # only covered the 9 originally-named SWA HUD fields.
            "api/SWA/HUD/Sonic/HudSonicStage.h offsets 0xE0..0x184",
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
            '"UP" { return 0x26 }',
            '"DOWN" { return 0x28 }',
            '"LEFT" { return 0x25 }',
            '"RIGHT" { return 0x27 }',
            '@("ENTER", "W", "A", "S", "D", "Q", "E", "UP", "DOWN", "LEFT", "RIGHT")',
            '"gameplay-sweep" {',
            '@("ENTER", "RIGHT", "RIGHT", "UP", "LEFT", "DOWN", "Q", "E")',
            'if ($ControlAutomationPlan -ne "gameplay-sweep")',
            'Wait-UiLabControlAutomationAwareSleep $settleSeconds $process "post-evidence-settle"',
            "$controlAutomationEnabled = -not $Observer -and -not $DisableControlAutomation -and (($controlAutomationTargets -contains $target) -or $UseControlAutomation)",
            "Wait-UiLabControlAutomationAwareSleep",
            "controlAutomation = $controlAutomationRecord",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 121 control automation",
            "ENTER/W/A/S/D/Q/E",
            "Phase 181 arrow-key automation",
            "ENTER/W/A/S/D/Q/E/UP/DOWN/LEFT/RIGHT",
            "stage targets default to real keyboard input automation",
            "input automation is the route driver",
            "live bridge plus native BMP remain the oracle",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase182_summarizes_manual_sonic_hud_value_observer_runs(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        self.assertTrue(script_path.is_file())
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "sonic-hud-value-text-write",
            "sonic-hud-gauge-pattern-write",
            "sonic-hud-gauge-hide-write",
            "sonic-hud-gauge-scale-write",
            "sonic-hud-value-write-update",
            "sonic-hud-callsite-value-classified",
            "ui_playscreen/so_speed_gauge",
            "ui_playscreen/so_ringenagy_gauge",
            "ui_playscreen/add/u_info",
            "manual gameplay observer",
        ]:
            self.assertIn(token, script)

        self.assertIn("Phase 182", report)
        self.assertIn("manual Sonic HUD value observer summarizer", report)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"event":"sonic-hud-value-text-write","detail":"value=ringCount path=ui_playscreen/ring_count/num_ring node=0x1 text=\\"005\\" pathResolutionSource=raw-chud-sonic-stage-owner-field source=CSD::CNode::SetText/sub_830BF640"}',
                        '{"event":"sonic-hud-gauge-scale-write","detail":"value=boostGauge path=ui_playscreen/so_speed_gauge node=0x2 scale=0.650,1 source=CSD::CNode::SetScale/sub_830BF090"}',
                        '{"event":"sonic-hud-gauge-pattern-write","detail":"value=tutorialPrompt path=ui_playscreen/add/u_info node=0x3 pattern=3 source=CSD::CNode::SetPatternIndex/sub_830BF300"}',
                        '{"event":"sonic-hud-gauge-hide-write","detail":"value=tutorialPrompt path=ui_playscreen/add/u_info node=0x3 hide=0 source=CSD::CNode::SetHideFlag/sub_830BF080"}',
                        '{"event":"sonic-hud-value-write-update","detail":"path=ui_playscreen/so_ringenagy_gauge kind=scale value=0.720000 source=CSD::CNode::SetScale/sub_830BF090@ui_playscreen/so_ringenagy_gauge"}',
                        '{"event":"sonic-hud-callsite-value-classified","detail":"value=speedKmh status=runtime-proven-via-sub_8251A568-return source=generated-PPC:sub_824D6418"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("sward_ui_lab_hud_value_summary", completed.stdout)
        self.assertIn("text_writes=1", completed.stdout)
        self.assertIn("gauge_writes=3", completed.stdout)
        self.assertIn("gauge_scale=1", completed.stdout)
        self.assertIn("gauge_pattern=1", completed.stdout)
        self.assertIn("gauge_hide=1", completed.stdout)
        self.assertIn("gameplay_updates=1", completed.stdout)
        self.assertIn("callsite_classifications=1", completed.stdout)
        self.assertIn("paths=ui_playscreen/add/u_info,ui_playscreen/ring_count/num_ring,ui_playscreen/so_ringenagy_gauge,ui_playscreen/so_speed_gauge", completed.stdout)
        self.assertIn("status=sonic-hud-value-events-found", completed.stdout)

    def test_ui_lab_phase183_groups_unresolved_sonic_hud_node_candidates(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        self.assertTrue(script_path.is_file())
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "sonic-hud-node-write-unresolved",
            "unresolvedNodeCandidates",
            "Get-UnresolvedNodeCandidateLabel",
            "node_candidate node=",
            "numeric-text-counter-candidate",
            "gauge-or-prompt-candidate",
            "manual unresolved node resolver",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 183",
            "unresolved Sonic HUD node candidates",
            "manual unresolved node resolver",
        ]:
            self.assertIn(token, report)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-node-write-unresolved","detail":"kind=text node=0x1111 value=\\"000\\" source=CSD::CNode::SetText/sub_830BF640 reason=ui_playscreen-active-path-unresolved"}',
                        '{"time":1.1,"frame":11,"event":"sonic-hud-node-write-unresolved","detail":"kind=text node=0x1111 value=\\"001\\" source=CSD::CNode::SetText/sub_830BF640 reason=ui_playscreen-active-path-unresolved"}',
                        '{"time":2.0,"frame":20,"event":"sonic-hud-node-write-unresolved","detail":"kind=scale node=0x2222 value=\\"0.650\\" source=CSD::CNode::SetScale/sub_830BF090 reason=ui_playscreen-active-path-unresolved"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("unresolved_node_writes=3:node_candidates=2", completed.stdout)
        self.assertIn(
            "node_candidate node=0x1111 writes=2 kinds=text values=000,001 frames=10-11",
            completed.stdout,
        )
        self.assertIn("likely=numeric-text-counter-candidate", completed.stdout)
        self.assertIn(
            "node_candidate node=0x2222 writes=1 kinds=scale values=0.650 frames=20-20",
            completed.stdout,
        )
        self.assertIn("likely=gauge-or-prompt-candidate", completed.stdout)

    def test_ui_lab_phase184_embeds_sward_operator_into_native_profiler(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        pivot = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        for token in [
            "void DrawProfilerAddon()",
            "DrawProfilerAddon",
        ]:
            self.assertIn(token, header)

        for token in [
            "g_operatorShellVisible = false",
            "DrawProfilerAddon",
            "SWARD UI Lab",
            "sward-profiler-addon-tabs",
            "HUD Switches",
            "Legacy floating panes",
            "SGlobals HUD/render switches",
            "ms_IsRenderHud",
            "ms_IsRenderGameMainHud",
            "ms_IsRenderHudPause",
            "HUD node writes: resolved=",
            "Sonic HUD binding:",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 184",
            "embeds SWARD operator readouts into the native Recomp Profiler",
            "Legacy floating panes",
            "SGlobals HUD/render switches",
        ]:
            self.assertIn(token, report)

        self.assertIn("native Recomp Profiler is the primary operator surface", pivot)

    def test_ui_lab_phase233_merges_sward_operator_into_single_native_workspace(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        video = self.read("UnleashedRecomp/gpu/video.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        pivot = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        self.assertIn("void UpdateOperatorShellToggle(bool toggleDown)", header)
        self.assertNotIn("SDL_SCANCODE_F2", video)
        self.assertNotIn("toggleOperator", video)
        self.assertNotIn("UiLab::UpdateOperatorShellToggle(toggleOperator)", video)
        self.assertIn("SDL_SCANCODE_F1", video)
        self.assertIn("DrawProfiler()", video)
        self.assertIn("uiLabDockedProfiler", video)
        self.assertIn("uiLabDockedProfiler && !g_uiLabProfilerWasEnabled", video)
        self.assertIn("profilerPlotSize", video)
        self.assertIn("ImGuiCond_FirstUseEver", video)
        self.assertIn("Full profiler details", video)
        self.assertNotIn("ImGuiWindowFlags_NoMove", video)
        self.assertIn("UiLab::DrawProfilerAddon();", video)

        for token in [
            "This native Profiler + SWARD UI Lab workspace stays visible",
            "F2 is no longer used by UI Lab",
            "DrawOperatorInGameConsoleTab",
            "ImGui::BeginTabItem(\"Console\")",
            "AnyOperatorFloatingPaneVisible",
            "ImGui::Checkbox(\"Window List\"",
            "SGlobals HUD/render switches",
            "DrawOperatorHudSwitchesPanel",
            "ImGui::BeginTabItem(\"HUD Switches\")",
            "ms_IsRenderHud is the whole UI render gate",
            "Legacy floating panes",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 233",
            "single native in-game workspace",
            "F2 is no longer used",
            "ms_IsRenderHud is the whole UI/UX render gate",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 233 merges the profiler and SWARD UI Lab", pivot)

    def test_ui_lab_phase186_restores_f2_embedded_style_and_hud_gate_correlation(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        pivot = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        for token in [
            "DrawProfilerAddonContent",
            "DrawDetachedProfilerAddonTab",
            "ImGui::BeginTabItem(\"SWARD UI Lab\")",
            "sward-profiler-addon-tabs",
            "This native Profiler + SWARD UI Lab workspace stays visible",
            "F2 is no longer used by UI Lab",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "struct HudRenderGateCorrelationSnapshot",
            "BuildHudRenderGateCorrelationSnapshot",
            "hudRenderGateCorrelation",
            "ms_IsRenderHudCallers",
            "frontend_listener.cpp",
            "options_menu.cpp::SetOptionsMenuVisible",
            "CHudPause_patches.cpp",
            "unresolvedUiPlayScreenNodeWrites",
            "sonic-hud-render-gate-correlated",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 186",
            "ms_IsRenderHud / ms_IsRenderGameMainHud / ms_IsRenderHudPause",
            "unresolved ui_playscreen node writes",
            "sonic-hud-render-gate-correlated",
        ]:
            self.assertIn(token, report)

        self.assertIn("F2 panel now contains the old embedded-profiler SWARD UI Lab tab", pivot)

    def test_ui_lab_phase187_native_profiler_style_and_node_callsite_correlation(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        pivot = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        for token in [
            "PushSwardNativeProfilerFont",
            "PopSwardNativeProfilerFont",
            "ImFontAtlasSnapshot::GetFont(\"FOT-SeuratPro-M.otf\")",
            "DrawSwardNativeProfilerFrameTimePlot",
            "ImPlot::BeginPlot(\"Frame Time\")",
            "ImPlot::PlotLine<float>(\"Application\"",
            "SWARD UI Lab###SWARD Operator Profiler",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "struct SonicHudNodeWriteCallsiteCorrelation",
            "CorrelateUnresolvedSonicHudNodeWriteWithCallsite",
            "callsiteCorrelationKnown",
            "callsiteValueCandidate",
            "same-frame-hud-update-context",
            "nearest-generated-PPC-callsite-sample",
            "timer/speed/boost-ring-energy/tutorial",
            "sonic-hud-node-write-callsite-correlated",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 187",
            "native Profiler font and ImPlot frame-time style",
            "unresolved Sonic HUD node writes",
            "timer/speed/boost-ring-energy/tutorial",
            "sonic-hud-node-write-callsite-correlated",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 187 moves F2 closer to the OG Profiler style", pivot)

    def test_ui_lab_phase188_promotes_correlated_hud_nodes_to_semantic_path_candidates(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        pivot = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        for token in [
            "SonicHudSemanticPathCandidate",
            "ResolveSonicHudSemanticPathCandidateFromCallsiteCorrelation",
            "semanticPathCandidate",
            "semanticValueName",
            "generated-PPC-callsite-semantic-candidate",
            "sonic-hud-node-write-semantic-path-candidate",
            "ui_playscreen/add/speed_count/position/num_speed",
            "ui_playscreen/so_speed_gauge",
            "ui_playscreen/so_ringenagy_gauge",
            "ui_playscreen/add/u_info",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "semanticPathCandidates",
            "sonic-hud-node-write-semantic-path-candidate",
            "semantic_candidate_paths=",
            "semantic_path_candidates=",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 188",
            "semantic path candidates",
            "ui_playscreen/add/speed_count/position/num_speed",
            "ui_playscreen/so_speed_gauge",
            "ui_playscreen/so_ringenagy_gauge",
            "ui_playscreen/add/u_info",
        ]:
            self.assertIn(token, report)

        self.assertIn("Phase 188", pivot)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=text node=0x1111 value=\\"042\\" valueCandidate=speed semanticValueName=speedKmh semanticPathCandidate=ui_playscreen/add/speed_count/position/num_speed source=same-frame-hud-update-context:sub_824D6418 pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                        '{"time":2.0,"frame":20,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=scale node=0x2222 value=\\"0.500\\" valueCandidate=boost-ring-energy semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge source=same-frame-hud-update-context:sub_824D6C18 pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                        '{"time":3.0,"frame":30,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=scale node=0x3333 value=\\"0.750\\" valueCandidate=boost-ring-energy semanticValueName=ringEnergyGauge semanticPathCandidate=ui_playscreen/so_ringenagy_gauge source=same-frame-hud-update-context:sub_824D6C18 pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                        '{"time":4.0,"frame":40,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=pattern-index node=0x4444 value=\\"1\\" valueCandidate=tutorial semanticValueName=tutorialPrompt semanticPathCandidate=ui_playscreen/add/u_info source=same-frame-hud-update-context:sub_824D7100 pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("semantic_path_candidates=4", completed.stdout)
        self.assertIn(
            "semantic_candidate_paths=ui_playscreen/add/speed_count/position/num_speed,ui_playscreen/add/u_info,ui_playscreen/so_ringenagy_gauge,ui_playscreen/so_speed_gauge",
            completed.stdout,
        )

    def test_ui_lab_manual_launcher_uses_complete_install_root_and_cwd_mode(self):
        script = self.read("research_uiux/runtime_reference/tools/launch_unleashed_recomp_ui_lab_manual.ps1")

        for token in [
            "Unleashed Recomp - Windows (Complete Installation) 1.0.3",
            "sward_ui_lab_runtime_manual",
            "--use-cwd",
            "--ui-lab-observer",
            "--ui-lab-live-bridge",
            "Start-Process",
            "$installRootResolved",
            "$sidecarRoot",
            "Copy-Item",
        ]:
            self.assertIn(token, script)

        self.assertNotIn("UnleashedRecomp_sward_ui_lab", script)

    def test_ui_lab_phase190_groups_semantic_hud_path_candidate_stability(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "semanticPathCandidateGroups",
            "Add-SemanticPathCandidateGroup",
            "semantic_candidate_groups=",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 190",
            "semantic candidate stability groups",
            "semantic_candidate_groups",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=text node=0x1111 value=\\"042\\" valueCandidate=speed semanticValueName=speedKmh semanticPathCandidate=ui_playscreen/add/speed_count/position/num_speed source=same-frame-hud-update-context:sub_824D6418 pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                        '{"time":2.0,"frame":20,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=text node=0x2222 value=\\"043\\" valueCandidate=speed semanticValueName=speedKmh semanticPathCandidate=ui_playscreen/add/speed_count/position/num_speed source=same-frame-hud-update-context:sub_824D6418 pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                        '{"time":3.0,"frame":30,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=pattern-index node=0x3333 value=\\"1\\" valueCandidate=tutorial semanticValueName=tutorialPrompt semanticPathCandidate=ui_playscreen/add/u_info source=same-frame-hud-update-context:sub_824D7100 pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("semantic_candidate_groups=ui_playscreen/add/speed_count/position/num_speed:speedKmh=2,ui_playscreen/add/u_info:tutorialPrompt=1", completed.stdout)

    def test_ui_lab_phase191_binds_semantic_candidates_without_claiming_exact_node_resolution(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "ApplySonicHudSemanticPathCandidateToGameplayValues",
            "sonic-hud-node-write-semantic-bound",
            "semanticBindingStatus=stable-candidate-bound-pending-exact-child-node-resolution",
            "generated-PPC-callsite-semantic-candidate",
            "pathResolved=false",
            "ui_playscreen/add/speed_count/position/num_speed",
            "ui_playscreen/so_speed_gauge",
            "ui_playscreen/so_ringenagy_gauge",
            "ui_playscreen/add/u_info",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 191",
            "semantic-bound",
            "semantic-candidate-bound-pending-exact-child-node-resolution",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

    def test_ui_lab_phase192_summarizes_semantic_bound_hud_evidence_separately(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "semanticBoundWrites",
            "semanticBoundGroups",
            "Add-SemanticBoundGroup",
            "sonic-hud-node-write-semantic-bound",
            "semantic_bound_groups=",
            "semantic_bound_paths=",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 192",
            "semantic_bound_groups",
            "semantic-bound tutorial",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=pattern-index node=0x1111 value=\\"1\\" valueCandidate=tutorial semanticValueName=tutorialPrompt semanticPathCandidate=ui_playscreen/add/u_info source=same-frame-hud-update-context:sub_824D7100 pathResolutionSource=generated-PPC-callsite-semantic-candidate pathResolved=false"}',
                        '{"time":2.0,"frame":20,"event":"sonic-hud-node-write-semantic-bound","detail":"kind=hide-flag node=0x2222 value=\\"0\\" semanticValueName=tutorialPrompt semanticPathCandidate=ui_playscreen/add/u_info source=nearest-generated-PPC-callsite-sample:generated-PPC:sub_824D7100 status=classified-via-generated-PPC-callsite-candidate pathResolutionSource=generated-PPC-callsite-semantic-candidate pathResolved=false semanticBindingStatus=stable-candidate-bound-pending-exact-child-node-resolution"}',
                        '{"time":3.0,"frame":30,"event":"sonic-hud-node-write-semantic-bound","detail":"kind=pattern-index node=0x3333 value=\\"1\\" semanticValueName=tutorialPrompt semanticPathCandidate=ui_playscreen/add/u_info source=nearest-generated-PPC-callsite-sample:generated-PPC:sub_824D7100 status=classified-via-generated-PPC-callsite-candidate pathResolutionSource=generated-PPC-callsite-semantic-candidate pathResolved=false semanticBindingStatus=stable-candidate-bound-pending-exact-child-node-resolution"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("semantic_path_candidates=1", completed.stdout)
        self.assertIn("semantic_bound=2", completed.stdout)
        self.assertIn("semantic_candidate_groups=ui_playscreen/add/u_info:tutorialPrompt=1", completed.stdout)
        self.assertIn("semantic_bound_groups=ui_playscreen/add/u_info:tutorialPrompt=2", completed.stdout)
        self.assertIn("semantic_bound_paths=ui_playscreen/add/u_info", completed.stdout)
        self.assertIn("paths=", completed.stdout)

    def test_ui_lab_phase194_summarizes_gauge_draw_child_paths_without_claiming_setter_resolution(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "DrawListPath",
            "gaugeDrawPathGroups",
            "gaugeSetterNodeCandidates",
            "gauge_draw_path_groups=",
            "gauge_setter_node_candidates=",
            "setter-node-address-join-pending",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 194",
            "gauge draw child paths",
            "setter-node-address-join-pending",
            "ui_playscreen/so_speed_gauge/position/speed_gauge_color",
            "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            events = tmp_path / "ui_lab_events.jsonl"
            draw_list = tmp_path / "ui_draw_list.json"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-node-write-unresolved","detail":"kind=scale node=0x2222 value=\\"0.650\\" source=CSD::CNode::SetScale/sub_830BF090 reason=ui_playscreen-active-path-unresolved callsiteCandidate=boost-ring-energy semanticPathCandidate=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge semanticValueName=boostGauge|ringEnergyGauge pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                        '{"time":1.1,"frame":10,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=text node=0x9999 value=\\"357\\" valueCandidate=boost-ring-energy semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge source=same-frame-hud-update-context:sub_824D6C18 pathResolutionSource=generated-PPC-callsite-semantic-candidate pathResolved=false"}',
                        '{"time":1.2,"frame":10,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=text node=0x9999 value=\\"357\\" valueCandidate=boost-ring-energy semanticValueName=ringEnergyGauge semanticPathCandidate=ui_playscreen/so_ringenagy_gauge source=same-frame-hud-update-context:sub_824D6C18 pathResolutionSource=generated-PPC-callsite-semantic-candidate pathResolved=false"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
            draw_list.write_text(
                json.dumps(
                    {
                        "uiDrawListOracle": {
                            "drawCalls": [
                                {
                                    "layerPath": "ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506",
                                    "layerAddress": "0xAAAA",
                                    "castNodeAddress": "0xBBBB",
                                },
                                {
                                    "layerPath": "ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0507",
                                    "layerAddress": "0xAAAB",
                                    "castNodeAddress": "0xBBBB",
                                },
                                {
                                    "layerPath": "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color/Cast_0483",
                                    "layerAddress": "0xCCCC",
                                    "castNodeAddress": "0xDDDD",
                                },
                            ]
                        }
                    }
                ),
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                    "-DrawListPath",
                    str(draw_list),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn(
            "gauge_draw_path_groups=ui_playscreen/so_speed_gauge/position/speed_gauge_color:boostGauge=2,ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color:ringEnergyGauge=1",
            completed.stdout,
        )
        self.assertIn(
            "gauge_setter_node_candidates=0x2222:boostGauge|ringEnergyGauge:scale:ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge=1",
            completed.stdout,
        )
        self.assertIn(
            "gauge_child_path_status=runtime-draw-list-exact-child-paths;setter-node-address-join-pending",
            completed.stdout,
        )

    def test_ui_lab_phase195_joins_gauge_setter_nodes_to_exact_draw_child_paths(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "gaugeSetterChildPathJoins",
            "gauge_setter_child_path_joins=",
            "runtime-draw-list-setter-node-joined",
            "setter-node-address-join-runtime-proven",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 195",
            "setter-node address join",
            "runtime-draw-list-setter-node-joined",
            "ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506",
            "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color/Cast_0483",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            events = tmp_path / "ui_lab_events.jsonl"
            draw_list = tmp_path / "ui_draw_list.json"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-node-write-unresolved","detail":"kind=scale node=0xBBBB value=\\"0.650\\" source=CSD::CNode::SetScale/sub_830BF090 reason=ui_playscreen-active-path-unresolved callsiteCandidate=boost-ring-energy semanticPathCandidate=ui_playscreen/so_speed_gauge semanticValueName=boostGauge pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                        '{"time":1.1,"frame":10,"event":"sonic-hud-node-write-unresolved","detail":"kind=scale node=0xDDDD value=\\"0.425\\" source=CSD::CNode::SetScale/sub_830BF090 reason=ui_playscreen-active-path-unresolved callsiteCandidate=boost-ring-energy semanticPathCandidate=ui_playscreen/so_ringenagy_gauge semanticValueName=ringEnergyGauge pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                        '{"time":1.2,"frame":10,"event":"sonic-hud-node-write-unresolved","detail":"kind=pattern-index node=0x9999 value=\\"1\\" source=CSD::CNode::SetPatternIndex/sub_830BF300 reason=ui_playscreen-active-path-unresolved callsiteCandidate=boost-ring-energy semanticPathCandidate=ui_playscreen/so_speed_gauge semanticValueName=boostGauge pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
            draw_list.write_text(
                json.dumps(
                    {
                        "uiDrawListOracle": {
                            "drawCalls": [
                                {
                                    "layerPath": "ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506",
                                    "layerAddress": "0xAAAA",
                                    "castNodeAddress": "0xBBBB",
                                },
                                {
                                    "layerPath": "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color/Cast_0483",
                                    "layerAddress": "0xCCCC",
                                    "castNodeAddress": "0xDDDD",
                                },
                            ]
                        }
                    }
                ),
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                    "-DrawListPath",
                    str(draw_list),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn(
            "gauge_setter_child_path_joins=0xBBBB:boostGauge:scale:cast-node:ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506=1,0xDDDD:ringEnergyGauge:scale:cast-node:ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color/Cast_0483=1",
            completed.stdout,
        )
        self.assertIn(
            "gauge_setter_node_candidates=0xBBBB:boostGauge:scale:ui_playscreen/so_speed_gauge=1,0xDDDD:ringEnergyGauge:scale:ui_playscreen/so_ringenagy_gauge=1,0x9999:boostGauge:pattern-index:ui_playscreen/so_speed_gauge=1",
            completed.stdout,
        )
        self.assertIn(
            "gauge_child_path_status=runtime-draw-list-setter-node-joined",
            completed.stdout,
        )

    def test_ui_lab_phase196_groups_sub_824d6c18_rolling_counter_text_candidates(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "rollingCounterSemanticGroups",
            "rolling_counter_semantic_groups=",
            "boost_ring_energy_status=",
            "rolling-counter-text-candidate-pending-gauge-state-normalization",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 196",
            "sub_824D6C18",
            "rolling counter",
            "pending gauge-state normalization",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-node-write-unresolved","detail":"kind=text node=0x82914B0 value=\\"530\\" source=CSD::CNode::SetText/sub_830BF640 reason=ui_playscreen-active-path-unresolved callsiteCandidate=boost-ring-energy callsiteSource=same-frame-hud-update-context:sub_824D6C18 semanticPathCandidate=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge semanticValueName=boostGauge|ringEnergyGauge pathResolutionSource=generated-PPC-callsite-semantic-candidate"}',
                        '{"time":1.1,"frame":10,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=text node=0x82914B0 value=\\"530\\" valueCandidate=boost-ring-energy semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge source=same-frame-hud-update-context:sub_824D6C18 status=timer/speed/boost-ring-energy/tutorial pathResolutionSource=generated-PPC-callsite-semantic-candidate pathResolved=false"}',
                        '{"time":1.2,"frame":10,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=text node=0x82914B0 value=\\"530\\" valueCandidate=boost-ring-energy semanticValueName=ringEnergyGauge semanticPathCandidate=ui_playscreen/so_ringenagy_gauge source=same-frame-hud-update-context:sub_824D6C18 status=timer/speed/boost-ring-energy/tutorial pathResolutionSource=generated-PPC-callsite-semantic-candidate pathResolved=false"}',
                        '{"time":2.1,"frame":11,"event":"sonic-hud-node-write-semantic-path-candidate","detail":"kind=text node=0x96C6E70 value=\\"531\\" valueCandidate=boost-ring-energy semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge source=same-frame-hud-update-context:sub_824D6C18 status=timer/speed/boost-ring-energy/tutorial pathResolutionSource=generated-PPC-callsite-semantic-candidate pathResolved=false"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn(
            "rolling_counter_semantic_groups=ui_playscreen/so_speed_gauge:boostGauge:text:sub_824D6C18=2,ui_playscreen/so_ringenagy_gauge:ringEnergyGauge:text:sub_824D6C18=1",
            completed.stdout,
        )
        self.assertIn(
            "boost_ring_energy_status=rolling-counter-text-candidate-pending-gauge-state-normalization",
            completed.stdout,
        )
        self.assertIn(
            "gauge_child_path_status=pending-runtime-ui-draw-list",
            completed.stdout,
        )

    def test_ui_lab_phase197_groups_sub_824d6c18_owner_field_rolling_counter(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "ownerFieldRollingCounterGroups",
            "New-OwnerFieldRollingCounterGroup",
            "Add-OwnerFieldRollingCounterGroup",
            "owner_field_rolling_counter_groups=",
            "owner_field_rolling_counter_status=",
            "owner-field-rolling-counter-pending-exact-offset-normalization",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 197",
            "sub_824D6C18",
            "owner-field rolling counter",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=100,1,200,0,5000 fieldValueHexes=0x64,0x1,0xC8,0x0,0x1388 candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":2.0,"frame":50,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=110,1,205,0,5005 fieldValueHexes=0x6E,0x1,0xCD,0x0,0x138D candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn(
            "owner_field_rolling_counter_groups=owner=0xCE2D6B0:field+460=boostGauge:samples=2,owner=0xCE2D6B0:field+460=ringEnergyGauge:samples=2",
            completed.stdout,
        )
        self.assertIn(
            "owner=0xCE2D6B0:field+480=boostGauge:samples=2,owner=0xCE2D6B0:field+480=ringEnergyGauge:samples=2",
            completed.stdout,
        )
        self.assertIn(
            "owner_field_rolling_counter_status=owner-field-rolling-counter-pending-exact-offset-normalization",
            completed.stdout,
        )
        self.assertNotIn("boost_ring_energy_status=resolved", completed.stdout)
        self.assertNotIn("boost_ring_energy_status=runtime-final", completed.stdout)

    def test_ui_lab_phase198_joins_owner_field_snapshot_with_setter_scale(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        ui_lab_h = self.read("UnleashedRecomp/patches/ui_lab_patches.h")

        for token in [
            "ownerFieldGaugeScaleCorrelationGroups",
            "New-OwnerFieldGaugeScaleCorrelationGroup",
            "Add-OwnerFieldGaugeScaleCorrelationGroup",
            "owner_field_gauge_scale_correlation_groups=",
            "owner_field_gauge_scale_correlation_status=",
            "owner-field-gauge-scale-correlation-pending-formula-proof",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 198",
            "owner-field-to-setter-scale",
            "Cast_0506",
            "Cast_0483",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        for token in [
            "OwnerFieldGaugeSnapshotCache",
            "g_lastOwnerFieldGaugeSnapshot",
            "sonic-hud-gauge-scale-owner-correlated",
        ]:
            self.assertIn(token, ui_lab)
        self.assertIn(
            "OnHudSonicStageOwnerFieldGaugeSnapshot",
            ui_lab_h,
        )

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-gauge-scale-owner-correlated","detail":"path=ui_playscreen/so_speed_gauge/position/speed_gauge_color node=0xEA09708 scale=0.650 ownerAddress=0xDE94C30 ownerField460=4 ownerField464=2 ownerField468=3 ownerField472=3 ownerField480=895 frameDelta=2 source=runtime-csd-node-set-scale-owner-field-join:sub_830BF090"}',
                        '{"time":1.1,"frame":11,"event":"sonic-hud-gauge-scale-owner-correlated","detail":"path=ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color node=0xEA0A990 scale=0.425 ownerAddress=0xDE94C30 ownerField460=4 ownerField464=7 ownerField468=7 ownerField472=6 ownerField480=684 frameDelta=1 source=runtime-csd-node-set-scale-owner-field-join:sub_830BF090"}',
                        '{"time":2.0,"frame":70,"event":"sonic-hud-gauge-scale-owner-correlated","detail":"path=ui_playscreen/so_speed_gauge/position/speed_gauge_color node=0xEA09708 scale=0.700 ownerAddress=0xDE94C30 ownerField460=4 ownerField464=3 ownerField468=4 ownerField472=4 ownerField480=910 frameDelta=2 source=runtime-csd-node-set-scale-owner-field-join:sub_830BF090"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn(
            "owner_field_gauge_scale_correlation_groups=",
            completed.stdout,
        )
        self.assertIn(
            "path=ui_playscreen/so_speed_gauge/position/speed_gauge_color:owner=0xDE94C30:field+460:joins=2",
            completed.stdout,
        )
        self.assertIn(
            "path=ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color:owner=0xDE94C30:field+480:joins=1",
            completed.stdout,
        )
        self.assertIn(
            "owner_field_gauge_scale_correlation_status=owner-field-gauge-scale-correlation-pending-formula-proof",
            completed.stdout,
        )
        self.assertNotIn("boost_ring_energy_status=resolved", completed.stdout)

    def test_ui_lab_phase199_classifies_owner_field_offset_shapes(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "ownerFieldOffsetClassifications",
            "New-OwnerFieldOffsetClassification",
            "Add-OwnerFieldOffsetClassification",
            "Resolve-OwnerFieldOffsetCandidateLabel",
            "owner_field_offset_classifications=",
            "owner_field_offset_classification_status=",
            "owner-field-offset-classification-pending-formula-proof",
            "low-cardinality-narrow-range-candidate",
            "moderate-cardinality-narrow-range-candidate",
            "high-cardinality-narrow-range-candidate",
            "high-cardinality-wide-range-candidate",
            "unclassified-pending-more-evidence",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 199",
            "owner-field offset classification",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=64,0,0,100,100 fieldValueHexes=0x40,0x0,0x0,0x64,0x64 candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":1.5,"frame":70,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=128,1,1,100,500 fieldValueHexes=0x80,0x1,0x1,0x64,0x1F4 candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":2.0,"frame":130,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=192,0,2,100,800 fieldValueHexes=0xC0,0x0,0x2,0x64,0x320 candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":2.5,"frame":190,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=255,1,15,100,200 fieldValueHexes=0xFF,0x1,0xF,0x64,0xC8 candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":3.0,"frame":250,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=128,0,2,100,999 fieldValueHexes=0x80,0x0,0x2,0x64,0x3E7 candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":3.1,"frame":260,"event":"sonic-hud-gauge-scale-owner-correlated","detail":"path=ui_playscreen/so_speed_gauge/position/speed_gauge_color node=0xEA09708 scale=0.250 ownerAddress=0xCE2D6B0 ownerField460=64 ownerField464=0 ownerField468=0 ownerField472=100 ownerField480=100 frameDelta=2 source=runtime-csd-node-set-scale-owner-field-join:sub_830BF090"}',
                        '{"time":3.2,"frame":270,"event":"sonic-hud-gauge-scale-owner-correlated","detail":"path=ui_playscreen/so_speed_gauge/position/speed_gauge_color node=0xEA09708 scale=0.500 ownerAddress=0xCE2D6B0 ownerField460=128 ownerField464=1 ownerField468=1 ownerField472=100 ownerField480=500 frameDelta=2 source=runtime-csd-node-set-scale-owner-field-join:sub_830BF090"}',
                        '{"time":3.3,"frame":280,"event":"sonic-hud-gauge-scale-owner-correlated","detail":"path=ui_playscreen/so_speed_gauge/position/speed_gauge_color node=0xEA09708 scale=0.750 ownerAddress=0xCE2D6B0 ownerField460=192 ownerField464=0 ownerField468=2 ownerField472=100 ownerField480=800 frameDelta=2 source=runtime-csd-node-set-scale-owner-field-join:sub_830BF090"}',
                        '{"time":3.4,"frame":290,"event":"sonic-hud-gauge-scale-owner-correlated","detail":"path=ui_playscreen/so_speed_gauge/position/speed_gauge_color node=0xEA09708 scale=1.000 ownerAddress=0xCE2D6B0 ownerField460=255 ownerField464=1 ownerField468=15 ownerField472=100 ownerField480=200 frameDelta=2 source=runtime-csd-node-set-scale-owner-field-join:sub_830BF090"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn(
            "owner_field_offset_classifications=owner=0xCE2D6B0:field+460:cardinality=4:min=64:max=255:joins=4:candidate=high-cardinality-narrow-range-candidate",
            completed.stdout,
        )
        self.assertIn(
            "owner=0xCE2D6B0:field+464:cardinality=2:min=0:max=1:joins=4:candidate=low-cardinality-narrow-range-candidate",
            completed.stdout,
        )
        self.assertIn(
            "owner=0xCE2D6B0:field+468:cardinality=4:min=0:max=15:joins=4:candidate=moderate-cardinality-narrow-range-candidate",
            completed.stdout,
        )
        self.assertIn(
            "owner=0xCE2D6B0:field+472:cardinality=1:min=100:max=100:joins=4:candidate=unclassified-pending-more-evidence",
            completed.stdout,
        )
        self.assertIn(
            "owner=0xCE2D6B0:field+480:cardinality=5:min=100:max=999:joins=4:candidate=high-cardinality-wide-range-candidate",
            completed.stdout,
        )
        self.assertIn(
            "owner_field_offset_classification_status=owner-field-offset-classification-pending-formula-proof",
            completed.stdout,
        )
        self.assertNotIn("boost_ring_energy_status=resolved", completed.stdout)
        self.assertNotIn("boost_ring_energy_status=runtime-final", completed.stdout)

    def test_ui_lab_phase200_emits_owner_field_offset_transition_diagnostics(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "ownerFieldOffsetTransitionDiagnostics",
            "New-OwnerFieldOffsetTransitionTrack",
            "Add-OwnerFieldOffsetTransitionSample",
            "Resolve-OwnerFieldOffsetMonotonicTrendLabel",
            "owner_field_offset_transition_diagnostics=",
            "owner_field_offset_transition_diagnostics_status=",
            "owner-field-offset-transition-diagnostics-pending-formula-proof",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 200",
            "owner-field offset transition diagnostics",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":100,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=0,10,5,4,100 fieldValueHexes=0x0,0xA,0x5,0x4,0x64 candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":1.5,"frame":700,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=4,10,5,4,200 fieldValueHexes=0x4,0xA,0x5,0x4,0xC8 candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":2.0,"frame":1300,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=0,10,5,4,900 fieldValueHexes=0x0,0xA,0x5,0x4,0x384 candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":2.5,"frame":1900,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=0,10,5,4,300 fieldValueHexes=0x0,0xA,0x5,0x4,0x12C candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":3.0,"frame":2500,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=4,10,5,4,300 fieldValueHexes=0x4,0xA,0x5,0x4,0x12C candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        # +460 went 0,4,0,0,4: 2 inc, 1 dec, 1 eq, magnitudes all 4 (or 0 for the eq).
        self.assertIn(
            "owner=0xCE2D6B0:field+460:trend=non-monotonic:wrap=true:transitions=4:inc=2:dec=1:eq=1:min_delta=0:max_delta=4",
            completed.stdout,
        )
        # +464,+468,+472 stayed flat throughout -> stable, no wrap.
        self.assertIn(
            "owner=0xCE2D6B0:field+464:trend=stable:wrap=false:transitions=4:inc=0:dec=0:eq=4:min_delta=0:max_delta=0",
            completed.stdout,
        )
        self.assertIn(
            "owner=0xCE2D6B0:field+468:trend=stable:wrap=false:transitions=4:inc=0:dec=0:eq=4:min_delta=0:max_delta=0",
            completed.stdout,
        )
        self.assertIn(
            "owner=0xCE2D6B0:field+472:trend=stable:wrap=false:transitions=4:inc=0:dec=0:eq=4:min_delta=0:max_delta=0",
            completed.stdout,
        )
        # +480 went 100,200,900,300,300: 2 inc, 1 dec, 1 eq, magnitudes 100,700,600,0.
        self.assertIn(
            "owner=0xCE2D6B0:field+480:trend=non-monotonic:wrap=true:transitions=4:inc=2:dec=1:eq=1:min_delta=0:max_delta=700",
            completed.stdout,
        )
        self.assertIn(
            "owner_field_offset_transition_diagnostics_status=owner-field-offset-transition-diagnostics-pending-formula-proof",
            completed.stdout,
        )
        # Hard guard: Phase 200 must NEVER promote the gauge values to runtime-final.
        self.assertNotIn("boost_ring_energy_status=resolved", completed.stdout)
        self.assertNotIn("boost_ring_energy_status=runtime-final", completed.stdout)
        self.assertNotIn("owner_field_offset_transition_diagnostics_status=resolved", completed.stdout)

    def test_ui_lab_phase201_bridges_owner_fields_to_exact_visible_gauge_paths(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "ownerFieldDrawPathBridgeGroups",
            "New-OwnerFieldDrawPathBridgeGroup",
            "Add-OwnerFieldDrawPathBridgeGroup",
            "owner_field_draw_path_bridge_groups=",
            "owner_field_draw_path_bridge_status=",
            "owner-field-draw-path-bridge-pending-formula-proof",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 201",
            "owner-field-to-visible-gauge-path bridge",
            "owner-field-draw-path-bridge-pending-formula-proof",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            events = tmp_path / "ui_lab_events.jsonl"
            draw_list = tmp_path / "ui_draw_list.json"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":10,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=4,2,3,3,895 fieldValueHexes=0x4,0x2,0x3,0x3,0x37F candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":2.0,"frame":70,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=4,3,4,4,910 fieldValueHexes=0x4,0x3,0x4,0x4,0x38E candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
            draw_list.write_text(
                json.dumps(
                    {
                        "uiDrawListOracle": {
                            "drawCalls": [
                                {
                                    "layerPath": "ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506",
                                    "layerAddress": "0xAAAA",
                                    "castNodeAddress": "0xBBBB",
                                },
                                {
                                    "layerPath": "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color/Cast_0483",
                                    "layerAddress": "0xCCCC",
                                    "castNodeAddress": "0xDDDD",
                                },
                            ]
                        }
                    }
                ),
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                    "-DrawListPath",
                    str(draw_list),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn(
            "owner_field_draw_path_bridge_groups=path=ui_playscreen/so_speed_gauge/position/speed_gauge_color:value=boostGauge:owner=0xCE2D6B0:field+460:samples=2:draws=1",
            completed.stdout,
        )
        self.assertIn(
            "path=ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color:value=ringEnergyGauge:owner=0xCE2D6B0:field+480:samples=2:draws=1",
            completed.stdout,
        )
        self.assertIn(
            "owner_field_draw_path_bridge_status=owner-field-draw-path-bridge-pending-formula-proof",
            completed.stdout,
        )
        self.assertIn(
            "boost_ring_energy_status=pending-runtime-rolling-counter-evidence",
            completed.stdout,
        )
        self.assertNotIn("boost_ring_energy_status=resolved", completed.stdout)
        self.assertNotIn("boost_ring_energy_status=runtime-final", completed.stdout)

    def test_ui_lab_phase206_reports_retail_sonic_hud_gauge_audio_proof_matrix(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "sonicHudRuntimeProofMatrix",
            "New-SonicHudRuntimeProofLane",
            "Resolve-SonicHudRuntimeProofMatrixStatus",
            "sonic_hud_runtime_proof_matrix=",
            "sonic_hud_runtime_proof_matrix_status=",
            "sonic_hud_audio_callsite_status=",
            "retail-runtime-gauge-proof-partial-audio-pending",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 206",
            "retail-runtime Sonic Day HUD proof matrix",
            "retail-runtime-gauge-proof-partial-audio-pending",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            events = tmp_path / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":100,"event":"sonic-hud-gauge-scale-write","detail":"value=boostGauge path=ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506 node=0xBBBB scale=0.650,1 source=CSD::CNode::SetScale/sub_830BF090"}',
                        '{"time":1.0,"frame":100,"event":"sonic-hud-gauge-pattern-write","detail":"value=boostGauge path=ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506 node=0xBBBB pattern=3 source=CSD::CNode::SetPatternIndex/sub_830BF300"}',
                        '{"time":1.0,"frame":100,"event":"sonic-hud-gauge-hide-write","detail":"value=boostGauge path=ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506 node=0xBBBB hidden=0 source=CSD::CNode::SetHideFlag/sub_830BF080"}',
                        '{"time":1.0,"frame":100,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=4,2,3,3,895 fieldValueHexes=0x4,0x2,0x3,0x3,0x37F candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":1.0,"frame":100,"event":"sonic-hud-gauge-scale-owner-correlated","detail":"value=boostGauge path=ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506 node=0xBBBB ownerAddress=0xCE2D6B0 ownerField460=4 ownerField464=2 ownerField468=3 ownerField472=3 ownerField480=895 scale=0.650 frameDelta=0 source=runtime-csd-node-set-scale-owner-field-join:sub_830BF090"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("sonic_hud_runtime_proof_matrix=", completed.stdout)
        self.assertIn("gauge-scale-write:present:1", completed.stdout)
        self.assertIn("gauge-pattern-write:present:1", completed.stdout)
        self.assertIn("gauge-hide-write:present:1", completed.stdout)
        self.assertIn("owner-field-snapshot:present:1", completed.stdout)
        self.assertIn("owner-scale-correlation:present:1", completed.stdout)
        self.assertIn("audio-callsite:pending:0", completed.stdout)
        self.assertIn(
            "sonic_hud_runtime_proof_matrix_status=retail-runtime-gauge-proof-partial-audio-pending",
            completed.stdout,
        )
        self.assertIn("sonic_hud_audio_callsite_status=audio-callsite-pending", completed.stdout)
        self.assertNotIn("sonic_hud_runtime_proof_matrix_status=retail-runtime-gauge-and-audio-proof-ready", completed.stdout)
        self.assertNotIn("boost_ring_energy_status=runtime-final", completed.stdout)

    def test_ui_lab_phase207_reports_unresolved_setter_owner_candidate_correlation(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "sonic-hud-gauge-setter-owner-candidate-correlated",
            "EmitSonicHudGaugeSetterOwnerCandidateCorrelation",
            "g_loggedOwnerFieldGaugeSetterCandidateCorrelationKeys",
            'writeKind == "text" ||',
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "sonic-hud-gauge-setter-owner-candidate-correlated",
            "ownerSetterCandidateCorrelationEvents",
            "ownerSetterCandidateCorrelationGroups",
            "owner_setter_candidate_correlation_groups=",
            "owner-setter-candidate-correlation",
            "sonic_hud_owner_setter_candidate_correlation_status=",
            "retail-runtime-setter-owner-candidate-correlation-pending-exact-child-path",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 207",
            "unresolved setter owner-candidate correlation",
            "retail-runtime-setter-owner-candidate-correlation-pending-exact-child-path",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        for token in [
            "Phase 208",
            "owner-setter candidate correlation groups",
            "owner_setter_candidate_correlation_groups=",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":100,"event":"sonic-hud-owner-gauge-snapshot","detail":"ownerAddress=0xCE2D6B0 callsite=sub_824D6C18 fieldOffsets=460,464,468,472,480 fieldValues=4,2,3,3,895 fieldValueHexes=0x4,0x2,0x3,0x3,0x37F candidatePaths=ui_playscreen/so_speed_gauge|ui_playscreen/so_ringenagy_gauge candidateValueNames=boostGauge|ringEnergyGauge source=runtime-owner-field-snapshot:sub_824D6C18"}',
                        '{"time":1.0,"frame":100,"event":"sonic-hud-gauge-setter-owner-candidate-correlated","detail":"kind=scale node=0xBBBB value=\\"0.650,1\\" semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge ownerAddress=0xCE2D6B0 ownerField460=4 ownerField464=2 ownerField468=3 ownerField472=3 ownerField480=895 frameDelta=0 source=runtime-csd-node-setter-owner-field-candidate-join:CSD::CNode::SetScale/sub_830BF090 pathResolved=false"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("owner-setter-candidate-correlation:present:1", completed.stdout)
        self.assertIn(
            "owner_setter_candidate_correlation_groups=node=0xBBBB:boostGauge:scale:ui_playscreen/so_speed_gauge:owner=0xCE2D6B0:joins=1",
            completed.stdout,
        )
        self.assertIn("fields=460=4|464=2|468=3|472=3|480=895", completed.stdout)
        self.assertIn(
            "sonic_hud_owner_setter_candidate_correlation_status=retail-runtime-setter-owner-candidate-correlation-pending-exact-child-path",
            completed.stdout,
        )
        self.assertIn(
            "sonic_hud_runtime_proof_matrix_status=retail-runtime-gauge-proof-incomplete-audio-pending",
            completed.stdout,
        )
        self.assertNotIn("sonic_hud_runtime_proof_matrix_status=retail-runtime-gauge-and-audio-proof-ready", completed.stdout)
        self.assertNotIn("boost_ring_energy_status=runtime-final", completed.stdout)

    def test_ui_lab_phase209_reports_owner_setter_candidate_numeric_relations(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "ownerSetterCandidateNumericRelationGroups",
            "owner_setter_candidate_numeric_relation_groups=",
            "New-OwnerSetterCandidateNumericRelationGroup",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 209",
            "owner-setter candidate numeric relation groups",
            "owner_setter_candidate_numeric_relation_groups=",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":100,"event":"sonic-hud-gauge-setter-owner-candidate-correlated","detail":"kind=text node=0xCCCC value=\\"895\\" semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge ownerAddress=0xCE2D6B0 ownerField460=4 ownerField464=2 ownerField468=3 ownerField472=5 ownerField480=895 frameDelta=0 callsiteSource=same-frame-hud-update-context:sub_824D6C18 source=runtime-csd-node-setter-owner-field-candidate-join:CSD::CNode::SetText/sub_830BF640 pathResolved=false"}',
                        '{"time":1.1,"frame":101,"event":"sonic-hud-gauge-setter-owner-candidate-correlated","detail":"kind=text node=0xCCCC value=\\"551\\" semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge ownerAddress=0xCE2D6B0 ownerField460=4 ownerField464=2 ownerField468=3 ownerField472=5 ownerField480=895 frameDelta=1 callsiteSource=same-frame-hud-update-context:sub_824D6C18 source=runtime-csd-node-setter-owner-field-candidate-join:CSD::CNode::SetText/sub_830BF640 pathResolved=false"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn(
            "owner_setter_candidate_numeric_relation_groups=node=0xCCCC:boostGauge:text:ui_playscreen/so_speed_gauge:owner=0xCE2D6B0:field+480:pairs=2:matches=1:min_delta=0:max_delta=344:setter=551-895:owner_field=895-895:frames=100-101",
            completed.stdout,
        )
        self.assertNotIn("boost_ring_energy_status=runtime-final", completed.stdout)

    def test_ui_lab_phase210_summarizes_local_cheat_table_anchors_without_committing_ct(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_sonic_unleashed_cheat_table.ps1"
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")
        self.assertTrue(script_path.is_file())

        script = script_path.read_text(encoding="utf-8")
        for token in [
            "CheatTablePath",
            "RuntimeExePath",
            "CheatEngineTableVersion",
            "aobscanmodule",
            "runtime_hits=",
            "ct_entry id=",
            "local CT evidence lane",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 210",
            "local CT evidence lane",
            "summarize_sonic_unleashed_cheat_table.ps1",
            "CT file stays local-only",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            cheat_table = Path(tmp) / "local_test.ct"
            runtime = Path(tmp) / "UnleashedRecomp.exe"
            output = Path(tmp) / "ct_summary.json"

            cheat_table.write_text(
                """<?xml version="1.0" encoding="utf-8"?>
<CheatTable CheatEngineTableVersion="42">
  <CheatEntries>
    <CheatEntry>
      <ID>337</ID>
      <Description>&quot;Collect A Ring For Infinite Rings&quot;</Description>
      <CheatEntries>
        <CheatEntry>
          <ID>369</ID>
          <Description>&quot;Infinite Boost&quot;</Description>
          <AssemblerScript>[ENABLE]
aobscanmodule(INJECT,UnleashedRecomp.exe,46 89 04 0A 44 8B 01)
// UnleashedRecomp.exe+51FE7A
mov r8d,(int)999999
[DISABLE]
</AssemblerScript>
        </CheatEntry>
      </CheatEntries>
    </CheatEntry>
  </CheatEntries>
</CheatTable>
""",
                encoding="utf-8",
            )
            runtime.write_bytes(bytes.fromhex("00 11 22 46 89 04 0A 44 8B 01 33 44"))

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-CheatTablePath",
                    str(cheat_table),
                    "-RuntimeExePath",
                    str(runtime),
                    "-OutputPath",
                    str(output),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

            summary = json.loads(output.read_text(encoding="utf-8"))

        self.assertIn("cheat_table_version=42", completed.stdout)
        self.assertIn("ct_entries_with_aob=1", completed.stdout)
        self.assertIn("ct_entry id=369", completed.stdout)
        self.assertIn("path=Collect A Ring For Infinite Rings/Infinite Boost", completed.stdout)
        self.assertIn("runtime_hits=1", completed.stdout)
        self.assertIn("file_offsets=0x3", completed.stdout)
        self.assertIn("old_injection_points=UnleashedRecomp.exe+51FE7A", completed.stdout)
        self.assertEqual(summary["entries"][0]["runtimeHits"][0]["fileOffset"], 3)
        self.assertEqual(summary["entries"][0]["scriptConstants"], ["999999"])
        self.assertNotIn("boost_ring_energy_status=runtime-final", completed.stdout)

    def test_ui_lab_phase211_hooks_ct_anchored_day_gameplay_writers(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        hud_hook = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "PPC_FUNC_IMPL(__imp__sub_82519FE8)",
            "PPC_FUNC_IMPL(__imp__sub_82A50838)",
            "PPC_FUNC_IMPL(__imp__sub_82BDBA20)",
            "PPC_FUNC_IMPL(__imp__sub_82BDBA60)",
            "OnSonicHudCtGameplayWriter",
            "ct-anchored-gameplay-writer",
        ]:
            self.assertIn(token, hud_hook)

        self.assertIn("OnSonicHudCtGameplayWriter", header)
        self.assertIn("sonic-hud-ct-gameplay-writer", ui_lab)

        for token in [
            "ctGameplayWriterEvents",
            "ct_gameplay_writer_groups=",
            "ct_gameplay_writer_owner_setter_candidate_correlation_groups=",
            "sonic_hud_ct_gameplay_writer_status=",
            "ct-gameplay-writer",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 211",
            "ct-anchored gameplay writer",
            "sonic-hud-ct-gameplay-writer",
            "ct_gameplay_writer_owner_setter_candidate_correlation_groups=",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":100,"event":"sonic-hud-ct-gameplay-writer","detail":"valueName=boostGauge callsite=sub_82A50838 ownerAddress=0xAAAA storageAddress=0xAB12 previousValue=1065353216 value=1073741824 delta=8388608 valueFloat=2 source=ct-anchored-gameplay-writer:day-boost"}',
                        '{"time":1.1,"frame":101,"event":"sonic-hud-gauge-setter-owner-candidate-correlated","detail":"kind=text node=0xCCCC value=\\"895\\" semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge ownerAddress=0xCE2D6B0 ownerField460=4 ownerField464=2 ownerField468=3 ownerField472=5 ownerField480=895 frameDelta=0 callsiteSource=same-frame-hud-update-context:sub_824D6C18 source=runtime-csd-node-setter-owner-field-candidate-join:CSD::CNode::SetText/sub_830BF640 pathResolved=false"}',
                        '{"time":1.2,"frame":110,"event":"sonic-hud-ct-gameplay-writer","detail":"valueName=ringCount callsite=sub_82519FE8 ownerAddress=0xBBBB storageAddress=0xCDEF previousValue=4 value=5 delta=1 source=ct-anchored-gameplay-writer:rings"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("ct_gameplay_writer_events=2", completed.stdout)
        self.assertIn(
            "ct_gameplay_writer_groups=boostGauge:sub_82A50838:writes=1:value=1073741824-1073741824:float=2-2:delta=8388608-8388608:frames=100-100",
            completed.stdout,
        )
        self.assertIn(
            "ringCount:sub_82519FE8:writes=1:value=5-5:float=<none>:delta=1-1:frames=110-110",
            completed.stdout,
        )
        self.assertIn(
            "ct_gameplay_writer_owner_setter_candidate_correlation_groups=value=boostGauge:writer=sub_82A50838:setterNode=0xCCCC:setterKind=text:path=ui_playscreen/so_speed_gauge:joins=1:frame_delta=1-1",
            completed.stdout,
        )
        self.assertIn("ct-gameplay-writer:present:2", completed.stdout)
        self.assertIn(
            "sonic_hud_ct_gameplay_writer_status=ct-anchored-gameplay-writer-evidence-present-pending-final-hud-formula",
            completed.stdout,
        )
        self.assertNotIn("boost_ring_energy_status=runtime-final", completed.stdout)

    def test_ui_lab_phase217_reports_ct_writer_entry_probes_separately_from_writer_proof(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        hud_hook = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "OnSonicHudCtGameplayWriterProbe",
            "ct-anchored-gameplay-writer-probe",
            "sub_82FE41C0",
        ]:
            self.assertIn(token, hud_hook)

        self.assertIn("OnSonicHudCtGameplayWriterProbe", header)
        self.assertIn("sonic-hud-ct-gameplay-writer-probe", ui_lab)

        for token in [
            "ctGameplayWriterProbeEvents",
            "ct_gameplay_writer_probe_events=",
            "ct_gameplay_writer_probe_groups=",
            "ct-anchored-gameplay-writer-probe-evidence-present-writer-mutation-pending",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 217",
            "ct_gameplay_writer_probe_events=",
            "gameplay writer probes",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":100,"event":"sonic-hud-ct-gameplay-writer-probe","detail":"valueName=boostGauge callsite=sub_82FE41C0 phase=entry ownerAddress=0xAAAA r3=0xAAAA r4=0xBBBB r5=0xCCCC source=ct-anchored-gameplay-writer-probe:day-boost-aob"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("ct_gameplay_writer_probe_events=1", completed.stdout)
        self.assertIn(
            "ct_gameplay_writer_probe_groups=boostGauge:sub_82FE41C0:phase=entry:probes=1:frames=100-100",
            completed.stdout,
        )
        self.assertIn("ct_gameplay_writer_events=0", completed.stdout)
        self.assertIn("ct-gameplay-writer:pending:0", completed.stdout)
        self.assertIn(
            "sonic_hud_ct_gameplay_writer_status=ct-anchored-gameplay-writer-probe-evidence-present-writer-mutation-pending",
            completed.stdout,
        )
        self.assertNotIn("ct-gameplay-writer:present:1", completed.stdout)

    def test_ui_lab_phase219_reports_ct_code_entry_gauge_candidates_without_writer_proof(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        hud_hook = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "OnSonicHudCtCodeEntryGaugeTransitionCandidate",
            "ct-code-entry-gauge-transition-candidate",
            "sub_8231C590",
            "sub_8231C5F0",
            "sub_8231C628",
        ]:
            self.assertIn(token, hud_hook)

        self.assertIn("OnSonicHudCtCodeEntryGaugeTransitionCandidate", header)
        self.assertIn("sonic-hud-ct-code-entry-gauge-transition-candidate", ui_lab)

        for token in [
            "ctCodeEntryGaugeTransitionCandidateEvents",
            "ct_code_entry_gauge_transition_candidate_events=",
            "ct_code_entry_gauge_transition_candidate_groups=",
            "sonic_hud_ct_code_entry_gauge_transition_candidate_status=",
            "ct-code-entry-gauge-transition-candidate-present-pending-exact-value-identity",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 219",
            "ct_code_entry_gauge_transition_candidate_events=",
            "CodeEntry float-gauge candidates",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":100,"event":"sonic-hud-ct-code-entry-gauge-transition-candidate","detail":"valueName=boostGaugeCandidate callsite=sub_8231C628 phase=add-clamp ownerAddress=0xAAAA storageAddress=0xAFE6 previousRawValue=1065353216 rawValue=1073741824 previousFloatValue=1 floatValue=2 inputFloatValue=1 source=ct-code-entry-gauge-transition-candidate:day-boost-a74bd6"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("ct_code_entry_gauge_transition_candidate_events=1", completed.stdout)
        self.assertIn(
            "ct_code_entry_gauge_transition_candidate_groups=boostGaugeCandidate:sub_8231C628:phase=add-clamp:events=1:owners=1:storages=1:raw=1073741824-1073741824:float=2-2:input=1-1:frames=100-100",
            completed.stdout,
        )
        self.assertIn("ct_gameplay_writer_events=0", completed.stdout)
        self.assertIn("ct-gameplay-writer:pending:0", completed.stdout)
        self.assertIn(
            "sonic_hud_ct_code_entry_gauge_transition_candidate_status=ct-code-entry-gauge-transition-candidate-present-pending-exact-value-identity",
            completed.stdout,
        )
        self.assertNotIn("ct-gameplay-writer:present:1", completed.stdout)

    def test_ui_lab_phase221_correlates_ct_code_entry_gauge_candidates_to_owner_setters(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "ctCodeEntryGaugeTransitionOwnerSetterCandidateCorrelationGroups",
            "ct_code_entry_gauge_transition_owner_setter_candidate_correlation_groups=",
            "sonic_hud_ct_code_entry_gauge_transition_correlation_status=",
            "ct-code-entry-gauge-transition-owner-setter-candidate-correlation-present-pending-formula-proof",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 221",
            "ct_code_entry_gauge_transition_owner_setter_candidate_correlation_groups=",
            "field+480",
            "rolling-counter",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            events = Path(tmp) / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":100,"event":"sonic-hud-ct-code-entry-gauge-transition-candidate","detail":"valueName=boostGaugeCandidate callsite=sub_8231C5F0 phase=ratio-read ownerAddress=0xAAAA storageAddress=0x153C previousRawValue=0 rawValue=1120403456 previousFloatValue=0 floatValue=100 inputFloatValue=1 source=ct-code-entry-gauge-transition-candidate:day-boost-a74bd6"}',
                        '{"time":1.1,"frame":101,"event":"sonic-hud-gauge-setter-owner-candidate-correlated","detail":"kind=text node=0xCCCC value=\\"573\\" semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge ownerAddress=0x759FE30 ownerField460=0 ownerField464=7 ownerField468=3 ownerField472=5 ownerField480=573 frameDelta=0 callsiteSource=same-frame-hud-update-context:sub_824D6C18 source=runtime-csd-node-setter-owner-field-candidate-join:CSD::CNode::SetText/sub_830BF640 pathResolved=false"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("ct_code_entry_gauge_transition_candidate_events=1", completed.stdout)
        self.assertIn("owner_setter_candidate_correlations=1", completed.stdout)
        self.assertIn(
            "ct_code_entry_gauge_transition_owner_setter_candidate_correlation_groups=ctValue=boostGaugeCandidate:ctCallsite=sub_8231C5F0:ctPhase=ratio-read:setterValue=boostGauge:setterNode=0xCCCC:setterKind=text:path=ui_playscreen/so_speed_gauge:field+480:joins=1:frame_delta=1-1:ct_float=100-100:input=1-1:setter=573-573:owner_field=573-573:frames=100-101",
            completed.stdout,
        )
        self.assertIn(
            "sonic_hud_ct_code_entry_gauge_transition_correlation_status=ct-code-entry-gauge-transition-owner-setter-candidate-correlation-present-pending-formula-proof",
            completed.stdout,
        )
        self.assertIn("ct_gameplay_writer_events=0", completed.stdout)
        self.assertIn("ct-gameplay-writer:pending:0", completed.stdout)
        self.assertNotIn("ct-gameplay-writer:present:1", completed.stdout)

    def test_ui_lab_phase222_exports_ghidra_context_for_ct_and_hud_candidates(self):
        java_script = self.read("research_uiux/runtime_reference/ghidra_scripts/SwardExportFunctionContext.java")
        wrapper = self.read("research_uiux/runtime_reference/tools/export_unleashed_recomp_ghidra_context.ps1")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "SwardExportFunctionContext",
            "schema",
            "sward-ghidra-function-context-v1",
            "callers",
            "callees",
            "referencesTo",
            "referencesFrom",
            "sub_824D6C18",
            "sub_8231C590",
            "sub_8231C5F0",
            "sub_8231C628",
            "sub_82519FE8",
            "sub_82BDBA20",
            "sub_82BDBA60",
            "sub_830BF640",
            "sub_830BF090",
            "sub_830BF300",
            "sub_830BF080",
        ]:
            self.assertIn(token, java_script)

        for token in [
            "SWARD_GHIDRA_HOME",
            "manifest.json",
            "javaHome",
            "$env:JAVA_HOME",
            "analyzeHeadless",
            "SwardExportFunctionContext.java",
            "out\\ui_lab_static_re\\ghidra",
            "local_build_env\\ur103clean\\b\\ui_lab_runtime\\UnleashedRecomp\\UnleashedRecomp.exe",
            "New-Item -ItemType HardLink",
            "New-Item -ItemType Directory -Force -Path $resolvedProjectRoot",
            "-postScript",
        ]:
            self.assertIn(token, wrapper)

        for token in [
            "Phase 222",
            "Ghidra headless",
            "static_function_context_targets=",
            "static_runtime_correlation_groups=",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

    def test_ui_lab_phase222_correlates_static_ghidra_context_to_runtime_hud_rows(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")

        for token in [
            "StaticContextPath",
            "staticFunctionContextTargets",
            "staticRuntimeCorrelationGroups",
            "static_function_context_targets=",
            "static_runtime_correlation_groups=",
            "sonic_hud_static_context_correlation_status=",
            "static-ghidra-context-correlated-with-runtime-hud-evidence",
        ]:
            self.assertIn(token, script)

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            events = tmp_path / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":100,"event":"sonic-hud-ct-code-entry-gauge-transition-candidate","detail":"valueName=boostGaugeCandidate callsite=sub_8231C5F0 phase=ratio-read ownerAddress=0xAAAA storageAddress=0x153C previousRawValue=0 rawValue=1120403456 previousFloatValue=0 floatValue=100 inputFloatValue=1 source=ct-code-entry-gauge-transition-candidate:day-boost-a74bd6"}',
                        '{"time":1.1,"frame":101,"event":"sonic-hud-gauge-setter-owner-candidate-correlated","detail":"kind=text node=0xCCCC value=\\"573\\" semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge ownerAddress=0x759FE30 ownerField460=0 ownerField464=7 ownerField468=3 ownerField472=5 ownerField480=573 frameDelta=0 callsiteSource=same-frame-hud-update-context:sub_824D6C18 source=runtime-csd-node-setter-owner-field-candidate-join:CSD::CNode::SetText/sub_830BF640 pathResolved=false"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
            static_context = tmp_path / "function_context.json"
            static_context.write_text(
                json.dumps(
                    {
                        "schema": "sward-ghidra-function-context-v1",
                        "targets": [
                            {
                                "query": "sub_8231C5F0",
                                "name": "sub_8231C5F0",
                                "entry": "0x8231C5F0",
                                "status": "resolved",
                                "callers": [{"name": "sub_8231C628", "entry": "0x8231C628"}],
                                "callees": [{"name": "sub_830BF640", "entry": "0x830BF640"}],
                            },
                            {
                                "query": "sub_824D6C18",
                                "name": "sub_824D6C18",
                                "entry": "0x824D6C18",
                                "status": "resolved",
                                "callers": [{"name": "sub_824D69B0", "entry": "0x824D69B0"}],
                                "callees": [{"name": "sub_830BF640", "entry": "0x830BF640"}],
                            },
                        ],
                    }
                ),
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                    "-StaticContextPath",
                    str(static_context),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("static_function_context_targets=2:resolved=2:unresolved=0", completed.stdout)
        self.assertIn(
            "static_runtime_correlation_groups=callsite=sub_8231C5F0:static=resolved:runtime=ct-code-entry-gauge-transition,ct-code-entry-owner-setter-correlation:callers=1:callees=1",
            completed.stdout,
        )
        self.assertIn(
            "callsite=sub_824D6C18:static=resolved:runtime=owner-setter-candidate-correlation:callers=1:callees=1",
            completed.stdout,
        )
        self.assertIn(
            "sonic_hud_static_context_correlation_status=static-ghidra-context-correlated-with-runtime-hud-evidence",
            completed.stdout,
        )

    def test_ui_lab_phase223_correlates_native_ct_ghidra_targets_to_runtime_sources(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")

        for token in [
            "Get-StaticRuntimeSourceAlias",
            "Get-StaticContextTargetCorrelationKeys",
            "day-boost-a74bd6",
            "0x140A74BD6",
        ]:
            self.assertIn(token, script)

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            events = tmp_path / "ui_lab_events.jsonl"
            events.write_text(
                '{"time":1.0,"frame":100,"event":"sonic-hud-ct-code-entry-gauge-transition-candidate","detail":"valueName=boostGaugeCandidate callsite=sub_8231C5F0 phase=ratio-read ownerAddress=0xAAAA storageAddress=0x153C previousRawValue=0 rawValue=1120403456 previousFloatValue=0 floatValue=100 inputFloatValue=1 source=ct-code-entry-gauge-transition-candidate:day-boost-a74bd6"}\n',
                encoding="utf-8",
            )
            static_context = tmp_path / "function_context.json"
            static_context.write_text(
                json.dumps(
                    {
                        "schema": "sward-ghidra-function-context-v1",
                        "targets": [
                            {
                                "query": "0x140A74BD6",
                                "name": "FUN_140a73d90",
                                "entry": "0x140A73D90",
                                "rva": "0xA73D90",
                                "status": "resolved",
                                "callers": [{"name": "FUN_1416c0450", "entry": "0x1416C0450"}],
                                "callees": [{"name": "FUN_140a695c0", "entry": "0x140A695C0"}],
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                    "-StaticContextPath",
                    str(static_context),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("static_function_context_targets=1:resolved=1:unresolved=0", completed.stdout)
        self.assertIn(
            "static_runtime_correlation_groups=callsite=ct-source:day-boost-a74bd6:static=resolved:runtime=ct-code-entry-gauge-transition:callers=1:callees=1",
            completed.stdout,
        )
        self.assertIn("target=0x140A74BD6:function=FUN_140a73d90", completed.stdout)
        self.assertIn(
            "sonic_hud_static_context_correlation_status=static-ghidra-context-correlated-with-runtime-hud-evidence",
            completed.stdout,
        )

    def test_ui_lab_phase224_correlates_native_day_boost_site_to_hud_settext_chain(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_unleashed_recomp_ui_lab_hud_values.ps1"
        script = script_path.read_text(encoding="utf-8")
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        hud_hook = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "OnSonicHudNativeCtDayBoostSite",
            "sonic-hud-native-ct-day-boost-site",
            "FUN_140a73d90",
            "0x140A74BD6",
            "0x140A74C79",
            "0x140A74E4A",
            "sub_8231C590",
            "sub_8231C5F0",
            "sub_8231C628",
            "native-ct-day-boost:day-boost-a74bd6",
        ]:
            self.assertIn(token, hud_hook)

        self.assertIn("OnSonicHudNativeCtDayBoostSite", header)
        self.assertIn("sonic-hud-native-ct-day-boost-site", ui_lab)

        for token in [
            "nativeCtDayBoostSiteEvents",
            "native_ct_day_boost_site_events=",
            "native_ct_day_boost_site_groups=",
            "native_ct_day_boost_hud_chain_groups=",
            "sonic_hud_native_ct_day_boost_chain_status=",
            "native-ct-day-boost-chain-present-pending-formula-proof",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 224",
            "native_ct_day_boost_site_events=",
            "native_ct_day_boost_hud_chain_groups=",
            "FUN_140a73d90",
            "field +480",
            "rolling-counter SetText",
            "not final boost/ring-energy formula proof",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            events = tmp_path / "ui_lab_events.jsonl"
            events.write_text(
                "\n".join(
                    [
                        '{"time":1.0,"frame":100,"event":"sonic-hud-native-ct-day-boost-site","detail":"valueName=boostGaugeCandidate nativeFunction=FUN_140a73d90 nativeTarget=0x140A74BD6 nativeCaller=FUN_1416c0450 generatedCallsite=sub_8231C5F0 phase=ratio-read ownerAddress=0xAAAA storageAddress=0x153C previousRawValue=0 rawValue=1120403456 previousFloatValue=0 floatValue=100 inputFloatValue=1 source=native-ct-day-boost:day-boost-a74bd6"}',
                        '{"time":1.0,"frame":100,"event":"sonic-hud-ct-code-entry-gauge-transition-candidate","detail":"valueName=boostGaugeCandidate callsite=sub_8231C5F0 phase=ratio-read ownerAddress=0xAAAA storageAddress=0x153C previousRawValue=0 rawValue=1120403456 previousFloatValue=0 floatValue=100 inputFloatValue=1 source=ct-code-entry-gauge-transition-candidate:day-boost-a74bd6"}',
                        '{"time":1.1,"frame":101,"event":"sonic-hud-gauge-setter-owner-candidate-correlated","detail":"kind=text node=0xCCCC value=\\"573\\" semanticValueName=boostGauge semanticPathCandidate=ui_playscreen/so_speed_gauge ownerAddress=0x759FE30 ownerField460=0 ownerField464=7 ownerField468=3 ownerField472=5 ownerField480=573 frameDelta=0 callsiteSource=same-frame-hud-update-context:sub_824D6C18 source=runtime-csd-node-setter-owner-field-candidate-join:CSD::CNode::SetText/sub_830BF640 pathResolved=false"}',
                    ]
                )
                + "\n",
                encoding="utf-8",
            )
            static_context = tmp_path / "function_context.json"
            static_context.write_text(
                json.dumps(
                    {
                        "schema": "sward-ghidra-function-context-v1",
                        "targets": [
                            {
                                "query": "0x140A74BD6",
                                "name": "FUN_140a73d90",
                                "entry": "0x140A73D90",
                                "rva": "0xA73D90",
                                "status": "resolved",
                                "callers": [{"name": "FUN_1416c0450", "entry": "0x1416C0450"}],
                                "callees": [{"name": "FUN_140a695c0", "entry": "0x140A695C0"}],
                            },
                            {
                                "query": "sub_8231C5F0",
                                "name": "sub_8231C5F0",
                                "entry": "0x8231C5F0",
                                "status": "resolved",
                                "callers": [{"name": "sub_8231C628", "entry": "0x8231C628"}],
                                "callees": [{"name": "sub_830BF640", "entry": "0x830BF640"}],
                            },
                        ],
                    }
                ),
                encoding="utf-8",
            )

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-EventsPath",
                    str(events),
                    "-StaticContextPath",
                    str(static_context),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

        self.assertIn("native_ct_day_boost_site_events=1", completed.stdout)
        self.assertIn(
            "native_ct_day_boost_site_groups=FUN_140a73d90:0x140A74BD6:sub_8231C5F0:phase=ratio-read:events=1:owners=1:storages=1:raw=1120403456-1120403456:float=100-100:input=1-1:frames=100-100",
            completed.stdout,
        )
        self.assertIn(
            "native_ct_day_boost_hud_chain_groups=native=FUN_140a73d90:target=0x140A74BD6:generated=sub_8231C5F0:phase=ratio-read:setterValue=boostGauge:setterNode=0xCCCC:setterKind=text:path=ui_playscreen/so_speed_gauge:field+480:joins=1:frame_delta=1-1:native_float=100-100:input=1-1:setter=573-573:owner_field=573-573:frames=100-101",
            completed.stdout,
        )
        self.assertIn(
            "callsite=ct-source:day-boost-a74bd6:static=resolved:runtime=ct-code-entry-gauge-transition,native-ct-day-boost-site:callers=1:callees=1:target=0x140A74BD6:function=FUN_140a73d90",
            completed.stdout,
        )
        self.assertIn(
            "sonic_hud_native_ct_day_boost_chain_status=native-ct-day-boost-chain-present-pending-formula-proof",
            completed.stdout,
        )
        self.assertNotIn("final-formula", completed.stdout)

    def test_ui_lab_phase212_summarizes_cheat_table_code_entries_as_host_sites(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_sonic_unleashed_cheat_table.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "CheatCodes/CodeEntry",
            "ce-code-entry",
            "ct_code_entries=",
            "ct_host_native_sites=",
            "codeEntries",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 212",
            "CheatCodes",
            "CodeEntry",
            "ct_host_native_sites",
            "ce-code-entry",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        with tempfile.TemporaryDirectory() as tmp:
            cheat_table = Path(tmp) / "local_code_entries.ct"
            runtime = Path(tmp) / "UnleashedRecomp.exe"
            output = Path(tmp) / "ct_code_summary.json"

            cheat_table.write_text(
                """<?xml version="1.0" encoding="utf-8"?>
<CheatTable CheatEngineTableVersion="42">
  <CheatEntries>
    <CheatEntry>
      <ID>369</ID>
      <Description>&quot;Infinite Boost&quot;</Description>
      <AssemblerScript>[ENABLE]
aobscanmodule(INJECT,UnleashedRecomp.exe,89 04 1E 48 89 F9)
// ORIGINAL CODE - INJECTION POINT: UnleashedRecomp.exe+A74E4A
[DISABLE]
</AssemblerScript>
    </CheatEntry>
  </CheatEntries>
  <CheatCodes>
    <CodeEntry>
      <Description>Change of mov [rsi+rbx],eax</Description>
      <AddressString>UnleashedRecomp.exe+A6974D</AddressString>
      <Before>
        <Byte>F9</Byte>
        <Byte>7E</Byte>
        <Byte>C0</Byte>
        <Byte>0F</Byte>
        <Byte>C8</Byte>
      </Before>
      <Actual>
        <Byte>89</Byte>
        <Byte>04</Byte>
        <Byte>1E</Byte>
      </Actual>
      <After>
        <Byte>48</Byte>
        <Byte>8B</Byte>
        <Byte>47</Byte>
        <Byte>08</Byte>
        <Byte>48</Byte>
      </After>
    </CodeEntry>
  </CheatCodes>
</CheatTable>
""",
                encoding="utf-8",
            )
            runtime.write_bytes(bytes.fromhex("AA F9 7E C0 0F C8 89 04 1E 48 8B 47 08 48 BB"))

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-CheatTablePath",
                    str(cheat_table),
                    "-RuntimeExePath",
                    str(runtime),
                    "-OutputPath",
                    str(output),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

            summary = json.loads(output.read_text(encoding="utf-8"))

        self.assertIn("ct_code_entries=1", completed.stdout)
        self.assertIn("ct_host_native_sites=2", completed.stdout)
        self.assertIn("ct_code_entry description=Change of mov [rsi+rbx],eax", completed.stdout)
        self.assertIn("old_injection_points=UnleashedRecomp.exe+A6974D", completed.stdout)
        self.assertIn("runtime_hits=1", completed.stdout)
        self.assertIn("file_offsets=0x6", completed.stdout)
        self.assertEqual(summary["codeEntries"][0]["entryKind"], "ce-code-entry")
        self.assertEqual(summary["codeEntries"][0]["runtimeHits"][0]["fileOffset"], 6)
        self.assertEqual(summary["codeEntries"][0]["codeEntryBeforeByteCount"], 5)
        self.assertEqual(summary["codeEntries"][0]["codeEntryActualByteCount"], 3)
        self.assertEqual(summary["codeEntries"][0]["oldInjectionPoints"], ["UnleashedRecomp.exe+A6974D"])

    def test_ui_lab_phase213_reports_ct_old_rva_alignment_against_runtime_binary(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_sonic_unleashed_cheat_table.ps1"
        script = script_path.read_text(encoding="utf-8")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        for token in [
            "old_ct_injection_point_rva_matches=",
            "oldInjectionPointRuntimeRvaStatus",
            "Resolve-OldInjectionPointRuntimeRvaAlignment",
        ]:
            self.assertIn(token, script)

        for token in [
            "Phase 213",
            "old CT RVA",
            "retail binary layout",
            "patched UI Lab binary layout",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

        def minimal_pe_with_pattern(pattern: bytes, pattern_file_offset: int = 0x210) -> bytes:
            data = bytearray(b"\x00" * 0x400)
            data[0:2] = b"MZ"
            data[0x3C:0x40] = (0x80).to_bytes(4, "little")
            data[0x80:0x84] = b"PE\x00\x00"
            data[0x84:0x86] = (0x8664).to_bytes(2, "little")
            data[0x86:0x88] = (1).to_bytes(2, "little")
            data[0x94:0x96] = (0xF0).to_bytes(2, "little")
            data[0x98:0x9A] = (0x20B).to_bytes(2, "little")
            data[0x98 + 24:0x98 + 32] = (0x140000000).to_bytes(8, "little")
            section = 0x80 + 24 + 0xF0
            data[section:section + 8] = b".text\x00\x00\x00"
            data[section + 8:section + 12] = (0x200).to_bytes(4, "little")
            data[section + 12:section + 16] = (0x1000).to_bytes(4, "little")
            data[section + 16:section + 20] = (0x200).to_bytes(4, "little")
            data[section + 20:section + 24] = (0x200).to_bytes(4, "little")
            data[pattern_file_offset:pattern_file_offset + len(pattern)] = pattern
            return bytes(data)

        with tempfile.TemporaryDirectory() as tmp:
            cheat_table = Path(tmp) / "old_rva_alignment.ct"
            runtime = Path(tmp) / "UnleashedRecomp.exe"
            output = Path(tmp) / "ct_alignment_summary.json"

            cheat_table.write_text(
                """<?xml version="1.0" encoding="utf-8"?>
<CheatTable CheatEngineTableVersion="42">
  <CheatEntries>
    <CheatEntry>
      <ID>369</ID>
      <Description>&quot;Infinite Boost&quot;</Description>
      <AssemblerScript>[ENABLE]
aobscanmodule(INJECT,UnleashedRecomp.exe,89 04 1E 48 89 F9)
// ORIGINAL CODE - INJECTION POINT: UnleashedRecomp.exe+1010
[DISABLE]
</AssemblerScript>
    </CheatEntry>
  </CheatEntries>
  <CheatCodes>
    <CodeEntry>
      <Description>Change of mov [rsi+rbx],eax</Description>
      <AddressString>UnleashedRecomp.exe+1012</AddressString>
      <Before>
        <Byte>F9</Byte>
        <Byte>7E</Byte>
      </Before>
      <Actual>
        <Byte>89</Byte>
        <Byte>04</Byte>
        <Byte>1E</Byte>
      </Actual>
      <After>
        <Byte>48</Byte>
        <Byte>89</Byte>
      </After>
    </CodeEntry>
  </CheatCodes>
</CheatTable>
""",
                encoding="utf-8",
            )
            runtime.write_bytes(minimal_pe_with_pattern(bytes.fromhex("F9 7E 89 04 1E 48 89")))

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-CheatTablePath",
                    str(cheat_table),
                    "-RuntimeExePath",
                    str(runtime),
                    "-OutputPath",
                    str(output),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

            summary = json.loads(output.read_text(encoding="utf-8"))

        self.assertIn("old_ct_injection_point_rva_matches=1", completed.stdout)
        self.assertIn("old_point_rva_status=shifted-or-no-exact-rva-match", completed.stdout)
        self.assertIn("old_point_rva_matches=UnleashedRecomp.exe+1012", completed.stdout)
        self.assertEqual(summary["oldInjectionPointRuntimeRvaMatchCount"], 1)
        self.assertEqual(summary["entries"][0]["oldInjectionPointRuntimeRvaStatus"], "shifted-or-no-exact-rva-match")
        self.assertEqual(summary["codeEntries"][0]["oldInjectionPointRuntimeRvaStatus"], "exact-rva-match")
        self.assertEqual(
            summary["codeEntries"][0]["oldInjectionPointRuntimeRvaMatches"],
            ["UnleashedRecomp.exe+1012"],
        )

    def test_ui_lab_phase202_preview_build_inventory_reports_repo_safe_metadata(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/inventory_sonic_unleashed_preview_build.ps1"
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")
        self.assertTrue(script_path.is_file())

        with tempfile.TemporaryDirectory() as tmp:
            preview_root = Path(tmp) / "preview"
            preview_root.mkdir()
            for relative_path in [
                "default.xex",
                "shader.ar",
                "shader.arl",
                "Title.ar.00",
                "Title.arl",
                "Loading.ar.00",
                "Loading.arl",
                "ActionCommon.ar.00",
                "ActionCommon.arl",
                "WorldMap.ar.00",
                "WorldMap.arl",
                "reddog/debug_icon.dds",
                "reddog/title_bar.dds",
                "Sound/bgm_sys_title.csb",
                "Languages/English/Title.ar.00",
                "Languages/English/Title.arl",
            ]:
                path = preview_root / relative_path
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"fixture")

            output_path = Path(tmp) / "preview_inventory.json"
            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-PreviewRoot",
                    str(preview_root),
                    "-OutputPath",
                    str(output_path),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

            self.assertIn("preview_inventory_status=ok", completed.stdout)
            summary = json.loads(output_path.read_text(encoding="utf-8"))

        self.assertEqual(summary["buildKind"], "sonic-unleashed-preview-build-disc1")
        self.assertTrue(summary["xex"]["present"])
        self.assertEqual(summary["xex"]["relativePath"], "default.xex")
        self.assertIn("Title", summary["uiArchiveFamilies"])
        self.assertIn("Loading", summary["uiArchiveFamilies"])
        self.assertIn("ActionCommon", summary["uiArchiveFamilies"])
        self.assertIn("WorldMap", summary["uiArchiveFamilies"])
        self.assertIn("debug_icon.dds", summary["reddogDebugUiAssets"])
        self.assertIn("bgm_sys_title.csb", summary["soundBanks"])
        self.assertIn("Languages/English/Title", summary["localizedArchiveFamilies"])
        self.assertIn("ui_playscreen", summary["expectedUiPackageNames"])
        for token in [
            "Phase 202",
            "preview build secondary oracle",
            "metadata-only",
            "Reddog debug UI assets",
            "do not replace the retail runtime oracle",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

    def test_ui_lab_phase203_preview_select_stage_taxonomy_maps_recovery_targets(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_sonic_unleashed_preview_select_stage.ps1"
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")
        self.assertTrue(script_path.is_file())

        select_xml = """<?xml version="1.0" encoding="utf-8"?>
<StageSelect>
  <Category>
    <Name>Rom</Name>
    <Category>
      <Name>Mykonos</Name>
      <Stage>
        <Type>LoadXML</Type>
        <Name>ActD_MykonosAct1</Name>
        <Archive>ActD_MykonosAct1</Archive>
        <AppendArchive>SonicActionCommon_Mykonos</AppendArchive>
        <IsEvil>false</IsEvil>
      </Stage>
    </Category>
  </Category>
  <Category>
    <Name>STAGE_Evil</Name>
    <Category>
      <Name>Stage1_Mykonos</Name>
      <Stage>
        <Type>LoadXML</Type>
        <Name>MykonosEvil_focus20080423</Name>
        <Archive>ActN_MykonosEvil</Archive>
        <AppendArchive>EvilActionCommon_Mykonos</AppendArchive>
        <IsEvil>true</IsEvil>
      </Stage>
    </Category>
  </Category>
  <Category>
    <Name>Other</Name>
    <Stage>
      <Type>OldMainMenu</Type>
      <Name>OldMainMenu</Name>
    </Stage>
  </Category>
  <Category>
    <Name>Sound Test</Name>
    <Stage>
      <Type>SoundTest</Type>
      <Name>Sonic SE Test</Name>
    </Stage>
  </Category>
  <Stage>
    <Type>Title</Type>
    <Name>Title</Name>
  </Stage>
  <Stage>
    <Type>WorldMap</Type>
    <Name>WorldMap</Name>
  </Stage>
  <Stage>
    <Type>SequenceEntryPoint</Type>
    <Name>SequenceEntryPoint</Name>
  </Stage>
</StageSelect>
"""

        with tempfile.TemporaryDirectory() as tmp:
            xml_path = Path(tmp) / "Select.xml"
            output_path = Path(tmp) / "preview_select_stage_taxonomy.json"
            xml_path.write_text(select_xml, encoding="utf-8")

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-SelectXmlPath",
                    str(xml_path),
                    "-OutputPath",
                    str(output_path),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )

            summary = json.loads(output_path.read_text(encoding="utf-8"))

        self.assertIn("preview_select_stage_status=ok", completed.stdout)
        self.assertIn("preview_select_stage_target_mappings=", completed.stdout)
        self.assertEqual(summary["buildKind"], "sonic-unleashed-preview-select-stage")
        self.assertEqual(summary["stageCount"], 7)
        self.assertEqual(summary["typeCounts"]["LoadXML"], 2)
        self.assertIn("Rom", summary["rootCategories"])
        self.assertIn("STAGE_Evil", summary["rootCategories"])

        mappings = {entry["targetId"]: entry for entry in summary["recoveryTargetMappings"]}
        self.assertEqual(mappings["title"]["controller"], "TitleMenuController")
        self.assertEqual(mappings["old-main-menu"]["controller"], "TitleMenuController")
        self.assertEqual(mappings["world-map"]["controller"], "WorldMapController")
        self.assertEqual(mappings["sonic-day-stage-hud"]["controller"], "SonicDayHudController")
        self.assertEqual(mappings["werehog-stage-hud"]["controller"], "WerehogHudController")
        self.assertEqual(mappings["sound-test"]["controller"], "AudioCueCatalog")
        self.assertEqual(mappings["sequence-entry-point"]["controller"], "SequenceRouteController")
        self.assertEqual(mappings["loading"]["status"], "not-present-in-select-xml")
        self.assertEqual(mappings["pause"]["status"], "not-present-in-select-xml")
        self.assertIn("ActD_MykonosAct1", mappings["sonic-day-stage-hud"]["sampleRoutes"][0]["name"])
        self.assertIn("MykonosEvil_focus20080423", mappings["werehog-stage-hud"]["sampleRoutes"][0]["name"])
        self.assertIn("metadata-only", summary["publishBoundary"])

        for token in [
            "Phase 203",
            "#SelectStage/Select.xml",
            "route taxonomy",
            "prototype-only secondary oracle",
            "SonicDayHudController",
            "WerehogHudController",
            "Reddog",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

    def test_ui_lab_phase204_preview_route_taxonomy_feeds_starter_coverage_matrix(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_sonic_unleashed_preview_select_stage.ps1"
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        select_xml = """<?xml version="1.0" encoding="utf-8"?>
<StageSelect>
  <Category>
    <Name>Rom</Name>
    <Category>
      <Name>Mykonos</Name>
      <Stage>
        <Type>LoadXML</Type>
        <Name>ActD_MykonosAct1</Name>
        <Archive>ActD_MykonosAct1</Archive>
        <AppendArchive>SonicActionCommon_Mykonos</AppendArchive>
        <IsEvil>false</IsEvil>
      </Stage>
    </Category>
  </Category>
  <Category>
    <Name>STAGE_Evil</Name>
    <Stage>
      <Type>LoadXML</Type>
      <Name>MykonosEvil_focus20080423</Name>
      <Archive>ActN_MykonosEvil</Archive>
      <AppendArchive>EvilActionCommon_Mykonos</AppendArchive>
      <IsEvil>true</IsEvil>
    </Stage>
  </Category>
  <Stage>
    <Type>Title</Type>
    <Name>Title</Name>
  </Stage>
  <Stage>
    <Type>OldMainMenu</Type>
    <Name>OldMainMenu</Name>
  </Stage>
  <Stage>
    <Type>WorldMap</Type>
    <Name>WorldMap</Name>
  </Stage>
  <Stage>
    <Type>Ending</Type>
    <Name>Ending</Name>
  </Stage>
</StageSelect>
"""

        with tempfile.TemporaryDirectory() as tmp:
            xml_path = Path(tmp) / "Select.xml"
            output_path = Path(tmp) / "coverage.json"
            xml_path.write_text(select_xml, encoding="utf-8")

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-SelectXmlPath",
                    str(xml_path),
                    "-OutputPath",
                    str(output_path),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )
            summary = json.loads(output_path.read_text(encoding="utf-8"))

        self.assertIn(
            "preview_select_stage_coverage_status=starter-uiux-route-coverage-matrix-ready",
            completed.stdout,
        )
        coverage = {entry["screenId"]: entry for entry in summary["starterScreenCoverageMatrix"]}
        self.assertEqual(coverage["title-menu"]["routeCount"], 2)
        self.assertEqual(coverage["title-menu"]["routeEvidenceStatus"], "prototype-route-taxonomy-proven")
        self.assertEqual(coverage["loading"]["routeEvidenceStatus"], "package/runtime-lane-not-select-route")
        self.assertEqual(coverage["pause"]["routeEvidenceStatus"], "retail-runtime-lane-not-select-route")
        self.assertEqual(coverage["sonic-day-hud"]["controller"], "SonicDayHudController")
        self.assertEqual(coverage["sonic-day-hud"]["routeCount"], 1)
        self.assertEqual(coverage["werehog-hud"]["controller"], "WerehogHudController")
        self.assertEqual(coverage["results"]["controller"], "ResultScreenController")
        self.assertEqual(coverage["world-map"]["routeEvidenceStatus"], "prototype-route-taxonomy-proven")
        self.assertIn("runtime evidence remains primary", coverage["sonic-day-hud"]["oracleBoundary"])
        self.assertIn("Reddog debug/profiler assets", summary["f2PanelStyleReference"]["localAssetFamily"])
        self.assertIn("not committed", summary["f2PanelStyleReference"]["publishBoundary"])

        for token in [
            "struct StarterUiCoverageRow",
            "kStarterUiCoverageRows",
            "DrawOperatorCoverageMatrixTab",
            "DrawSwardReddogStyleSectionHeader",
            "local-only Reddog style reference",
            "starter UI/UX coverage matrix",
            "ImGui::BeginTabItem(\"Coverage\")",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 204",
            "starter UI/UX coverage matrix",
            "Reddog-style F2 panel",
            "local-only Reddog assets",
            "prototype route taxonomy",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

    def test_ui_lab_phase205_coverage_matrix_selects_next_source_recovery_lane(self):
        script_path = ROOT / "research_uiux/runtime_reference/tools/summarize_sonic_unleashed_preview_select_stage.ps1"
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        checklist = self.read("research_uiux/TODO_CHECKLIST.md")

        select_xml = """<?xml version="1.0" encoding="utf-8"?>
<StageSelect>
  <Category>
    <Name>Rom</Name>
    <Category>
      <Name>Mykonos</Name>
      <Stage>
        <Type>LoadXML</Type>
        <Name>ActD_MykonosAct1</Name>
        <Archive>ActD_MykonosAct1</Archive>
        <AppendArchive>SonicActionCommon_Mykonos</AppendArchive>
        <IsEvil>false</IsEvil>
      </Stage>
    </Category>
  </Category>
  <Category>
    <Name>STAGE_Evil</Name>
    <Stage>
      <Type>LoadXML</Type>
      <Name>MykonosEvil_focus20080423</Name>
      <Archive>ActN_MykonosEvil</Archive>
      <AppendArchive>EvilActionCommon_Mykonos</AppendArchive>
      <IsEvil>true</IsEvil>
    </Stage>
  </Category>
  <Category>
    <Name>Sound Test</Name>
    <Stage>
      <Type>SoundTest</Type>
      <Name>Sonic SE Test</Name>
    </Stage>
  </Category>
  <Stage>
    <Type>Title</Type>
    <Name>Title</Name>
  </Stage>
  <Stage>
    <Type>OldMainMenu</Type>
    <Name>OldMainMenu</Name>
  </Stage>
  <Stage>
    <Type>WorldMap</Type>
    <Name>WorldMap</Name>
  </Stage>
  <Stage>
    <Type>Ending</Type>
    <Name>Ending</Name>
  </Stage>
</StageSelect>
"""

        with tempfile.TemporaryDirectory() as tmp:
            xml_path = Path(tmp) / "Select.xml"
            output_path = Path(tmp) / "phase205.json"
            xml_path.write_text(select_xml, encoding="utf-8")

            completed = subprocess.run(
                [
                    "powershell",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    str(script_path),
                    "-SelectXmlPath",
                    str(xml_path),
                    "-OutputPath",
                    str(output_path),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )
            summary = json.loads(output_path.read_text(encoding="utf-8"))

        self.assertIn(
            "preview_select_stage_next_lane_status=coverage-matrix-selected-retail-sonic-day-hud",
            completed.stdout,
        )
        self.assertIn(
            "preview_select_stage_next_lane=sonic-day-hud-retail-runtime",
            completed.stdout,
        )

        next_lane = summary["nextSourceRecoveryLane"]
        self.assertEqual(next_lane["laneId"], "sonic-day-hud-retail-runtime")
        self.assertEqual(next_lane["screenId"], "sonic-day-hud")
        self.assertEqual(next_lane["controller"], "SonicDayHudController")
        self.assertEqual(next_lane["primaryOracle"], "retail-runtime-ui-lab")
        self.assertEqual(next_lane["secondaryOracle"], "prototype-route-taxonomy")
        self.assertEqual(next_lane["decision"], "continue-retail-runtime-hud-value-recovery")
        self.assertIn("boost/ring-energy", next_lane["why"])
        self.assertIn("exact SFX/audio IDs", next_lane["blockedBy"])
        self.assertIn("SetPatternIndex/SetHideFlag gauge state joins", next_lane["blockedBy"])

        queued_lanes = {entry["laneId"]: entry for entry in summary["sourceRecoveryLaneQueue"]}
        self.assertEqual(queued_lanes["world-map-prototype-route"]["controller"], "WorldMapController")
        self.assertEqual(queued_lanes["results-prototype-route"]["controller"], "ResultScreenController")
        self.assertEqual(queued_lanes["audio-sfx-retail-runtime"]["controller"], "AudioCueCatalog")

        for token in [
            "struct SourceRecoveryLaneRow",
            "kSourceRecoveryLaneRows",
            "DrawOperatorNextSourceRecoveryLane",
            "sonic-day-hud-retail-runtime",
            "coverage-matrix-selected-retail-sonic-day-hud",
            "world-map-prototype-route",
            "results-prototype-route",
            "audio-sfx-retail-runtime",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 205",
            "coverage matrix selected Sonic Day HUD retail runtime",
            "world-map/results prototype lanes queued",
            "boost/ring-energy",
            "exact SFX/audio IDs",
        ]:
            self.assertIn(token, report)
            self.assertIn(token, checklist)

    def test_ui_lab_phase184_promotes_score_csd_text_path_resolution(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "ui_playscreen/score_count/score",
            "ui_playscreen/score_count/num_score",
            "return \"score\"",
            "snapshot.scoreKnown = true",
            "snapshot.score = parsedValue",
            "snapshot.scoreSource = source",
            "score:known-via-csd-text-or-game-document",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 184",
            "score_count/score",
            "score_count/num_score",
            "anonymous Sonic HUD text writes",
            "named score value",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase166_exposes_sonic_hud_gameplay_value_bridge_contract(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        player = self.read("UnleashedRecomp/patches/player_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "OnSonicHudGameplayValues",
            "ringCountKnown",
            "scoreKnown",
            "elapsedFramesKnown",
            "speedKmhKnown",
            "boostGaugeKnown",
            "ringEnergyGaugeKnown",
            "lifeCountKnown",
            "tutorialPromptKnown",
            "tutorialVisible",
        ]:
            self.assertIn(token, header)

        for token in [
            "struct SonicHudGameplayValueSnapshot",
            "BuildSonicHudGameplayValueSnapshot",
            "gameplayValues",
            "ringCountKnown",
            "scoreKnown",
            "elapsedFramesKnown",
            "speedKmhKnown",
            "boostGaugeKnown",
            "ringEnergyGaugeKnown",
            "lifeCountKnown",
            "tutorialPromptKnown",
            "scoreSource",
            "SWA::CGameDocument::GetInstance",
            "m_ScoreInfo.EnemyScore",
            "m_ScoreInfo.TrickScore",
            "SWA::CGameDocument::m_pMember->m_ScoreInfo.EnemyScore+TrickScore",
            "pending-runtime-field",
            "audioIds",
            "sys_actstg_pausewinopen",
            "sys_actstg_pausecursor",
            "audio-id-pending",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "m_ScoreInfo.EnemyScore",
            "m_ScoreInfo.TrickScore",
        ]:
            self.assertIn(token, player)

        for token in [
            "Phase 166",
            "typedInspectors.sonicHud.gameplayValues",
            "Score is the first runtime-bound gameplay value",
            "ring/speed/boost/energy/life/tutorial IDs remain pending-runtime-field until exact owner/player offsets are proven",
            "Sonic HUD SFX IDs remain audio-id-pending unless a runtime callsite proves the exact cue",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase167_promotes_exact_sonic_hud_display_and_scoreinfo_paths(self):
        hud_hook = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "m_rcScoreCount",
            "m_rcTimeCount",
            "m_rcTimeCount2",
            "m_rcTimeCount3",
            "m_rcPlayerCount",
        ]:
            self.assertIn(token, ui_lab)
        self.assertNotIn("GuestAddressOf(pHudSonicStage->m_rcScoreCount.Get())", hud_hook)
        self.assertNotIn("GuestAddressOf(pHudSonicStage->m_rcPlayerCount.Get())", hud_hook)

        for token in [
            "scoreInfoPointMarkerRecordSpeedKnown",
            "scoreInfoPointMarkerRecordSpeed",
            "scoreInfoPointMarkerRecordSpeedSource",
            "scoreInfoPointMarkerCountKnown",
            "scoreInfoPointMarkerCount",
            "scoreInfoPointMarkerCountSource",
            "CGameDocument::m_pMember->m_ScoreInfo.PointMarkerRecordSpeed",
            "CGameDocument::m_pMember->m_ScoreInfo.PointMarkerCount",
            "rcScoreCountNodeAddress",
            "rcTimeCountNodeAddress",
            "rcTimeCount2NodeAddress",
            "rcTimeCount3NodeAddress",
            "rcPlayerCountNodeAddress",
            "rcRingCountSceneAddress",
            "rcTutorialInfoSceneAddress",
            "ui_playscreen/ring_count",
            "ui_playscreen/add/u_info",
            "displayOwnerPaths",
            "gameplayNumericBindingStatus",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 167",
            "ScoreInfo.PointMarkerRecordSpeed",
            "ScoreInfo.PointMarkerCount",
            "m_rcScoreCount/m_rcTimeCount/m_rcPlayerCount",
            "ring/timer/speed/boost/energy/lives/tutorial gameplay numerics remain pending",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase168_hooks_sonic_hud_csd_text_value_write_paths(self):
        cmake = self.read("UnleashedRecomp/CMakeLists.txt")
        hook = self.read("UnleashedRecomp/patches/CsdNodeText_patches.cpp")
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        self.assertIn("patches/CsdNodeText_patches.cpp", cmake)

        for token in [
            "PPC_FUNC_IMPL(__imp__sub_830BF640)",
            "PPC_FUNC(sub_830BF640)",
            "TryReadGuestAsciiString",
            "UiLab::OnCsdNodeSetText",
            "CSD::CNode::SetText/sub_830BF640",
        ]:
            self.assertIn(token, hook)

        for token in [
            "OnCsdNodeSetText",
            "nodeAddress",
            "textAddress",
            "textUtf8",
            "hookSource",
        ]:
            self.assertIn(token, header)

        for token in [
            "SonicHudValueWriteObservation",
            "g_sonicHudValueWriteObservations",
            "ResolveSonicHudValuePathFromCsdNode",
            "ApplySonicHudTextWriteToGameplayValues",
            "ui_playscreen/ring_count/num_ring",
            "ui_playscreen/time_count/time001",
            "ui_playscreen/time_count/time010",
            "ui_playscreen/time_count/time100",
            "ui_playscreen/add/speed_count/position/num_speed",
            "ui_playscreen/player_count/player",
            "sonic-hud-value-text-write",
            "sonic-hud-value-write-update",
            "ring/timer/speed/lives:known-via-csd-text-write",
            "boost/energy/tutorial:csd-node-pattern-hide-scale-hooks-installed-with-unresolved-write-probe-pending-runtime-normalization",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 168",
            "CNode::SetText/sub_830BF640",
            "ring_count/num_ring",
            "time_count/time001",
            "add/speed_count/position/num_speed",
            "player_count/player",
            "boost/energy/tutorial remain pending until gauge/prompt setter callsites are proven",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase169_hooks_sonic_hud_csd_gauge_and_prompt_node_writes(self):
        cmake = self.read("UnleashedRecomp/CMakeLists.txt")
        build_script = self.read("research_uiux/runtime_reference/tools/build_unleashed_recomp_ui_lab.ps1")
        hook = self.read("UnleashedRecomp/patches/CsdNodeValue_patches.cpp")
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        self.assertIn("patches/CsdNodeValue_patches.cpp", cmake)
        self.assertIn("UnleashedRecomp\\patches\\CsdNodeValue_patches.cpp", build_script)

        for token in [
            "PPC_FUNC_IMPL(__imp__sub_830BF300)",
            "PPC_FUNC(sub_830BF300)",
            "PPC_FUNC_IMPL(__imp__sub_830BF080)",
            "PPC_FUNC(sub_830BF080)",
            "PPC_FUNC_IMPL(__imp__sub_830BF090)",
            "PPC_FUNC(sub_830BF090)",
            "UiLab::OnCsdNodeSetPatternIndex",
            "UiLab::OnCsdNodeSetHideFlag",
            "UiLab::OnCsdNodeSetScale",
            "CSD::CNode::SetPatternIndex/sub_830BF300",
            "CSD::CNode::SetHideFlag/sub_830BF080",
            "CSD::CNode::SetScale/sub_830BF090",
        ]:
            self.assertIn(token, hook)

        for token in [
            "OnCsdNodeSetPatternIndex",
            "OnCsdNodeSetHideFlag",
            "OnCsdNodeSetScale",
        ]:
            self.assertIn(token, header)

        for token in [
            "IsSonicHudGaugeOrPromptPath",
            "ResolveSonicHudGaugeOrPromptPathFromCsdNode",
            "ui_playscreen/so_speed_gauge",
            "ui_playscreen/gauge_frame",
            "ui_playscreen/so_ringenagy_gauge",
            "ui_playscreen/add/u_info",
            "sonic-hud-gauge-pattern-write",
            "sonic-hud-gauge-hide-write",
            "sonic-hud-gauge-scale-write",
            "sonic-hud-node-write-unresolved",
            "HasRecentUiPlayScreenDrawActivity",
            "IsLikelySonicHudUnresolvedValue",
            "RecordUnresolvedSonicHudNodeWrite",
            "ResolveSonicHudPathFromRecentDrawCalls",
            "TryLateResolveSonicHudNodeWriteObservations",
            "sonic-hud-node-write-late-resolved",
            "pathResolutionSource",
            "pathResolved",
            "writeKind",
            "numericValueKnown",
            "boost/energy/tutorial:csd-node-pattern-hide-scale-hooks-installed-with-unresolved-write-probe-pending-runtime-normalization",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 169",
            "ui-draw-list manual observer",
            "CNode::SetPatternIndex/sub_830BF300",
            "CNode::SetHideFlag/sub_830BF080",
            "CNode::SetScale/sub_830BF090",
            "Phase 170",
            "sonic-hud-node-write-unresolved",
            "Phase 171",
            "late-resolve unresolved CSD node writes",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase172_hooks_csd_child_lookup_and_sonic_hud_update_contexts(self):
        cmake = self.read("UnleashedRecomp/CMakeLists.txt")
        build_script = self.read("research_uiux/runtime_reference/tools/build_unleashed_recomp_ui_lab.ps1")
        lookup_hook = self.read("UnleashedRecomp/patches/CsdNodeLookup_patches.cpp")
        hud_hook = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        self.assertIn("patches/CsdNodeLookup_patches.cpp", cmake)
        self.assertIn("UnleashedRecomp\\patches\\CsdNodeLookup_patches.cpp", build_script)

        for token in [
            "PPC_FUNC_IMPL(__imp__sub_830BCCA8)",
            "PPC_FUNC(sub_830BCCA8)",
            "PPC_FUNC_IMPL(__imp__sub_830BA228)",
            "PPC_FUNC(sub_830BA228)",
            "TryReadGuestLookupName",
            "UiLab::OnCsdChildNodeLookupResolved",
            "UiLab::OnCsdNodePointerResolved",
            "CSD::CNode::GetChild/sub_830BCCA8",
            "CSD::RCPtr::Get/sub_830BA228",
        ]:
            self.assertIn(token, lookup_hook)

        for token in [
            "PushSonicHudUpdateContext",
            "PopSonicHudUpdateContext",
            "sub_824D6048",
            "sub_824D6418",
            "sub_824D69B0",
            "sub_824D6C18",
            "sub_824D7100",
        ]:
            self.assertIn(token, hud_hook)

        for token in [
            "OnCsdChildNodeLookupResolved",
            "OnCsdNodePointerResolved",
            "PushSonicHudUpdateContext",
            "PopSonicHudUpdateContext",
        ]:
            self.assertIn(token, header)

        for token in [
            "CsdChildNodeLookupObservation",
            "CsdNodeSourceOwnerObservation",
            "g_csdChildNodeLookupObservations",
            "g_csdNodeSourceOwnerObservations",
            "ResolveSonicHudPathFromRawOwnerFieldsLocked",
            "ResolveCsdNodePathFromLookupChainLocked",
            "ResolveSonicHudPathFromNodeSourceOwnerLocked",
            "sonic-hud-node-source-owner-resolved",
            "sonic-hud-update-context",
            "pathResolutionSource=raw-chud-sonic-stage-owner-field",
            "pathResolutionSource=csd-child-lookup-chain",
            "sourceOwnerAddress",
            "sourceOwnerOffsetFromUpdateOwner",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 172",
            "sub_830BCCA8",
            "sub_830BA228",
            "child lookup chain",
            "CHudSonicStage update context",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase174_samples_sonic_hud_update_callsite_fields(self):
        hud_hook = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "RecordHudSonicStageCallsiteSample",
            "UiLab::OnSonicHudUpdateCallsiteSample",
            "samplePhase=pre-original",
            "samplePhase=post-original",
            "sub_824D6048",
            "sub_824D6418",
            "sub_824D6C18",
        ]:
            self.assertIn(token, hud_hook)

        for token in [
            "OnSonicHudUpdateCallsiteSample",
            "ownerAddress",
            "hookName",
            "samplePhase",
            "deltaTime",
        ]:
            self.assertIn(token, header)

        for token in [
            "SonicHudUpdateCallsiteSample",
            "g_sonicHudUpdateCallsiteSamples",
            "BuildSonicHudUpdateCallsiteSamples",
            "AppendSonicHudUpdateCallsiteSamples",
            "sonic-hud-update-callsite-sample",
            "ownerField452",
            "ownerField456",
            "ownerField460",
            "ownerField480",
            "timer/counter/speed/gauge candidates:sampled-via-chud-update-callsites",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 174",
            "sonic-hud-update-callsite-sample",
            "owner +452/+456",
            "owner +460/+480",
            "manual observer windows where CNode::SetText does not fire",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase176_classifies_sonic_hud_callsite_samples_into_live_values(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "ApplySonicHudUpdateCallsiteSampleToGameplayValues",
            "ClassifySonicHudUpdateCallsiteSample",
            "sonic-hud-callsite-value-classified",
            "generated-PPC:sub_824D6048 owner+456/+452 -> CSD::CNode::SetText",
            "runtime-proven-via-chud-update-callsite-sample",
            "classified-via-generated-PPC-callsite-candidate",
            "elapsedFramesKnown = true",
            "elapsedFramesSource = source",
            "ownerField456 * 60 + std::min<uint32_t>(sample.ownerField452, 59)",
            "timer:runtime-proven-via-chud-update-callsite-sample",
            "boost/energy/tutorial:classified-callsite-candidates-pending-normalization",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 176",
            "sonic-hud-callsite-value-classified",
            "timer:runtime-proven-via-chud-update-callsite-sample",
            "boost/energy/tutorial remain classified candidates pending normalization",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase177_keeps_last_classified_sonic_hud_callsite_value_readable(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "SonicHudLastClassifiedCallsiteValue",
            "g_lastSonicHudClassifiedCallsiteValue",
            "BuildSonicHudLastClassifiedCallsiteValue",
            "lastClassifiedCallsiteValue",
            "lastClassificationKnown",
            "normalizedValueKnown",
            "lastClassifiedCallsiteValueSource",
            "lastClassifiedCallsiteValueFrame",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Phase 177",
            "lastClassifiedCallsiteValue",
            "durable JSONL evidence",
            "latest live-state snapshot",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase178_compacts_overlay_and_throttles_hud_callsite_spam(self):
        header = self.read("UnleashedRecomp/patches/ui_lab_patches.h")
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        hud_hook = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")

        for token in [
            "BuildSonicHudUpdateCallsiteStableSignature",
            "g_lastSonicHudUpdateCallsiteStableSignatures",
            "g_lastSonicHudUpdateCallsiteEvidenceFrames",
            "kSonicHudUpdateCallsiteMinEvidenceIntervalFrames",
            "ShouldSampleSonicHudUpdateCallsiteFrame",
            "stableSignature",
            "intervalElapsed",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "OnSonicHudSpeedReadoutValue",
            "sonic-hud-speed-readout-value",
            "runtime-proven-via-sub_8251A568-return",
            "generated-PPC:sub_824D6418 -> sub_8251A568 return",
        ]:
            self.assertIn(token, ui_lab)

        self.assertIn("void OnSonicHudSpeedReadoutValue", header)
        self.assertIn("PPC_FUNC_IMPL(__imp__sub_8251A568)", hud_hook)
        self.assertIn("PPC_FUNC(sub_8251A568)", hud_hook)
        self.assertIn("g_sonicHudSpeedReadoutCaptureDepth", hud_hook)
        self.assertIn("OnSonicHudSpeedReadoutValue", hud_hook)
        self.assertIn("value update hook", hud_hook)
        self.assertIn("RCPtr::Get()", hud_hook)

        for token in [
            "Phase 178",
            "compact-on-demand operator overlay",
            "stable HUD callsite signature",
            "sub_8251A568 return",
            "speed:runtime-proven-via-sub_8251A568-return",
        ]:
            self.assertIn(token, report)

    def test_ui_lab_phase179_profiler_style_operator_panel_and_perf_safe_hud_sampling(self):
        ui_lab = self.read("UnleashedRecomp/patches/ui_lab_patches.cpp")
        hud_hook = self.read("UnleashedRecomp/patches/CHudSonicStage_patches.cpp")
        report = self.read("research_uiux/DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md")
        pivot = self.read("research_uiux/UNLEASHED_RECOMP_UI_LAB_PIVOT.md")

        for token in [
            "SWARD Operator Profiler",
            "DrawOperatorProfilerPanel",
            "DrawOperatorProfilerSummary",
            "DrawOperatorProfilerPanelsTab",
            "ImGui::PlotLines(\"Frame Time\"",
            "ImGui::BeginTabBar(\"sward-operator-profiler-tabs\")",
            "ImGui::BeginTabItem(\"Runtime\")",
            "ImGui::BeginTabItem(\"HUD\")",
            "ImGui::BeginTabItem(\"Capture\")",
            "ImGui::BeginTabItem(\"Panels\")",
            "return false; // Leave F1 to DrawProfiler().",
            "ShouldSampleSonicHudUpdateCallsiteFrame",
            "g_lastSonicHudUpdateCallsiteSampleFrame",
        ]:
            self.assertIn(token, ui_lab)

        for token in [
            "Do not call RCPtr::Get() here",
            "source.find(\"value update hook\")",
            "OnHudSonicStageOwnerFieldSample",
        ]:
            self.assertIn(token, hud_hook)

        for token in [
            "Phase 179",
            "profiler-style SWARD operator panel",
            "F1 remains the native Recomp Profiler toggle",
            "low-overhead Sonic HUD callsite sampling",
        ]:
            self.assertIn(token, report)

        self.assertIn("profiler-style SWARD operator panel", pivot)
        self.assertIn("F1 remains reserved for the native Recomp Profiler", pivot)


if __name__ == "__main__":
    unittest.main()

