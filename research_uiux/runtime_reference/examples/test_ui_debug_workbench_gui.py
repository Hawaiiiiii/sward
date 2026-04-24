#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
GUI_SOURCE = REPO_ROOT / "research_uiux" / "runtime_reference" / "examples" / "ui_debug_workbench_gui.cpp"
CMAKE_FILE = REPO_ROOT / "research_uiux" / "runtime_reference" / "CMakeLists.txt"
DEFAULT_EXE = REPO_ROOT / "b" / "rr63" / "sward_ui_runtime_debug_gui.exe"


class UiDebugWorkbenchGuiTests(unittest.TestCase):
    def test_gui_target_is_declared_as_native_windows_executable(self) -> None:
        cmake_text = CMAKE_FILE.read_text(encoding="utf-8")
        self.assertIn("add_executable(sward_ui_runtime_debug_gui WIN32", cmake_text)
        self.assertIn("examples/ui_debug_workbench_gui.cpp", cmake_text)
        self.assertIn("user32", cmake_text)
        self.assertIn("gdi32", cmake_text)
        self.assertIn("gdiplus", cmake_text)

    def test_gui_source_uses_workbench_data_and_runtime_contracts(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("int WINAPI WinMain", source_text)
        self.assertIn("kDebugWorkbenchHostEntries", source_text)
        self.assertIn("ScreenRuntime", source_text)
        self.assertIn("CreateWindowExA", source_text)
        self.assertIn("LISTBOX", source_text)
        self.assertIn("Run Host", source_text)
        self.assertIn("--smoke", source_text)

    def test_gui_source_declares_visual_preview_surface(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("SwardUiRuntimePreviewPanel", source_text)
        self.assertIn("WM_PAINT", source_text)
        self.assertIn("paintPreview", source_text)
        self.assertIn("Gdiplus::Image", source_text)
        self.assertIn("extracted_assets/visual_atlas/sheets", source_text)
        self.assertIn("visibleLayers", source_text)
        self.assertIn("visiblePrompts", source_text)
        self.assertIn("--preview-smoke", source_text)

    def test_gui_preview_binds_gameplay_hud_proxy_atlas_and_bounded_layout(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("sonic_stage_hud_reference.json", source_text)
        self.assertIn("werehog_stage_hud_reference.json", source_text)
        self.assertIn("exstagetails_common__ui_prov_playscreen.png", source_text)
        self.assertIn("proxy", source_text)
        self.assertIn("layoutLayerRect", source_text)
        self.assertIn("clampRectToCanvas", source_text)
        self.assertIn("previewMaxHeight", source_text)

    def test_gui_source_exposes_timeline_playback_controls(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("kPlayPauseButtonId", source_text)
        self.assertIn("kStepButtonId", source_text)
        self.assertIn("kPlaybackTimerId", source_text)
        self.assertIn("kPlaybackTickSeconds", source_text)
        self.assertIn("WM_TIMER", source_text)
        self.assertIn("SetTimer", source_text)
        self.assertIn("KillTimer", source_text)
        self.assertIn("startPlayback", source_text)
        self.assertIn("stopPlayback", source_text)
        self.assertIn("tickPlaybackFrame", source_text)
        self.assertIn("--playback-smoke", source_text)

    def test_gui_source_exposes_state_aware_preview_motion(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("PreviewMotion", source_text)
        self.assertIn("easedTimelineProgress", source_text)
        self.assertIn("previewMotionForState", source_text)
        self.assertIn("applyPreviewMotion", source_text)
        self.assertIn("motionAlphaByte", source_text)
        self.assertIn("atlasBackingBrush", source_text)
        self.assertIn("--motion-smoke", source_text)

    def test_gui_source_exposes_family_specific_preview_layouts(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("PreviewFamily", source_text)
        self.assertIn("previewFamilyForContract", source_text)
        self.assertIn("layoutTitleMenuLayerRect", source_text)
        self.assertIn("layoutPauseMenuLayerRect", source_text)
        self.assertIn("layoutLoadingLayerRect", source_text)
        self.assertIn("layoutFamilyLayerRect", source_text)
        self.assertIn("--family-preview-smoke", source_text)

    def test_gui_source_exposes_layout_evidence_overlay(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("LayoutEvidence", source_text)
        self.assertIn("layoutEvidenceForContract", source_text)
        self.assertIn("drawLayoutEvidenceOverlay", source_text)
        self.assertIn("ui_mainmenu", source_text)
        self.assertIn("ui_pause", source_text)
        self.assertIn("ui_loading", source_text)
        self.assertIn("--layout-evidence-smoke", source_text)

    def test_gui_source_exposes_layout_timeline_frame_overlay(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("longestTimelineFrames", source_text)
        self.assertIn("layoutTimelineFrame", source_text)
        self.assertIn("drawLayoutTimelineBar", source_text)
        self.assertIn("--layout-timeline-smoke", source_text)

    def test_gui_source_exposes_layout_scene_primitive_preview(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("LayoutScenePrimitive", source_text)
        self.assertIn("layoutScenePrimitivesForContract", source_text)
        self.assertIn("drawLayoutScenePrimitives", source_text)
        self.assertIn("mm_donut_move", source_text)
        self.assertIn("Root/window_1/item/stick", source_text)
        self.assertIn("Root/bg_2", source_text)
        self.assertIn("--layout-primitive-smoke", source_text)

    def test_gui_source_exposes_layout_scene_primitive_playback_cues(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("animationName", source_text)
        self.assertIn("layoutScenePrimitiveFrame", source_text)
        self.assertIn("--layout-primitive-playback-smoke", source_text)
        self.assertIn("WM_PRINTCLIENT", source_text)

    def test_gui_source_exposes_layout_primitive_detail_cues(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("layoutPrimitiveCueSummary", source_text)
        self.assertIn("Layout primitive cues:", source_text)
        self.assertIn("--layout-primitive-detail-smoke", source_text)

    def test_gui_source_exposes_gameplay_hud_proxy_primitives(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("ui_prov_playscreen", source_text)
        self.assertIn("Root/ring_get_effect", source_text)
        self.assertIn("Root/so_speed_gauge", source_text)
        self.assertIn("sonic_stage_hud_reference.json", source_text)
        self.assertIn("werehog_stage_hud_reference.json", source_text)
        self.assertIn("extra_stage_hud_reference.json", source_text)

    def test_gui_source_maps_gameplay_hud_primitives_to_deep_analysis_scene_ownership(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")

        expected_fragments = [
            '"Root/so_speed_gauge", "so_speed_gauge", "Size_Anim", "Gradient, X/Y scale", 100, 1, 47, 47, 360',
            '"Root/so_ringenagy_gauge", "so_ringenagy_gauge", "Size_Anim", "Gradient, X scale", 100, 1, 43, 43, 240',
            '"Root/info_1", "info_1", "Count_Anim", "Gradient, HideFlag", 100, 3, 72, 24, 57',
            '"Root/info_2", "info_2", "Count_Anim", "HideFlag", 100, 3, 72, 24, 9',
            '"Root/ring_get_effect", "ring_get_effect", "Intro_Anim", "Gradient, Rotation", 5, 1, 2, 2, 14',
            '"Root/bg", "bg", "DefaultAnim", "DefaultAnim", 100, 6, 29, 21, 0',
        ]

        for fragment in expected_fragments:
            self.assertIn(fragment, source_text)

    def test_gui_source_maps_gameplay_hud_primitives_to_animation_banks(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")

        expected_fragments = [
            '"Root/so_speed_gauge", "so_speed_gauge", "Size_Anim", "Gradient, X/Y scale"',
            '"Root/so_ringenagy_gauge", "so_ringenagy_gauge", "Size_Anim", "Gradient, X scale"',
            '"Root/info_1", "info_1", "Count_Anim", "Gradient, HideFlag"',
            '"Root/info_2", "info_2", "Count_Anim", "HideFlag"',
            '"Root/ring_get_effect", "ring_get_effect", "Intro_Anim", "Gradient, Rotation"',
            '"Root/bg", "bg", "DefaultAnim", "DefaultAnim"',
        ]

        for fragment in expected_fragments:
            self.assertIn(fragment, source_text)

    def test_gui_source_preserves_atlas_under_structural_backdrop_layers(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("previewLayerFillAlpha", source_text)
        self.assertIn('role == "backdrop"', source_text)
        self.assertIn('role == "cinematic_frame"', source_text)
        self.assertIn("--layer-fill-smoke", source_text)

    def test_gui_initial_window_uses_desktop_work_area(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("SPI_GETWORKAREA", source_text)
        self.assertIn("SystemParametersInfoA", source_text)
        self.assertIn("windowWidth", source_text)
        self.assertIn("windowHeight", source_text)

    def test_gui_smoke_command_resolves_hosts_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui smoke ok", completed.stdout)
        self.assertIn("hosts=176", completed.stdout)
        self.assertIn("groups=11", completed.stdout)
        self.assertIn("support_hosts=17", completed.stdout)

    def test_preview_smoke_command_reports_atlas_bindings_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--preview-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui preview smoke ok", completed.stdout)
        self.assertIn("atlas_candidates=10", completed.stdout)
        self.assertIn("proxy_candidates=2", completed.stdout)
        self.assertIn("title=mainmenu__ui_mainmenu.png", completed.stdout)
        self.assertIn("pause=systemcommoncore__ui_pause.png", completed.stdout)
        self.assertIn("sonic_stage=exstagetails_common__ui_prov_playscreen.png", completed.stdout)

    def test_playback_smoke_command_advances_timeline_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--playback-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui playback smoke ok", completed.stdout)
        self.assertIn("intro=Intro", completed.stdout)
        self.assertIn("after_intro=Idle", completed.stdout)
        self.assertIn("action=Navigate", completed.stdout)
        self.assertIn("after_action=Idle", completed.stdout)

    def test_motion_smoke_command_reports_eased_preview_motion_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--motion-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui motion smoke ok", completed.stdout)
        self.assertIn("intro_alpha=", completed.stdout)
        self.assertIn("intro_offset_x=", completed.stdout)
        self.assertIn("idle_alpha=1", completed.stdout)
        self.assertIn("outro_alpha=", completed.stdout)

    def test_family_preview_smoke_command_reports_exact_family_layouts(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--family-preview-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui family preview smoke ok", completed.stdout)
        self.assertIn("title=title_menu", completed.stdout)
        self.assertIn("pause=pause_menu", completed.stdout)
        self.assertIn("loading=loading_transition", completed.stdout)
        self.assertIn("title_logo_y=", completed.stdout)

    def test_layout_evidence_smoke_reports_decoded_layout_facts_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--layout-evidence-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui layout evidence smoke ok", completed.stdout)
        self.assertIn("title=ui_mainmenu scenes=16 animations=6", completed.stdout)
        self.assertIn("pause=ui_pause scenes=29 animations=41", completed.stdout)
        self.assertIn("loading=ui_loading scenes=7 animations=37", completed.stdout)

    def test_layer_fill_smoke_preserves_atlas_under_backdrops_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--layer-fill-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui layer fill smoke ok", completed.stdout)
        self.assertIn("backdrop_alpha=0", completed.stdout)
        self.assertIn("cinematic_alpha=0", completed.stdout)
        self.assertIn("content_alpha=0.58", completed.stdout)

    def test_layout_timeline_smoke_reports_frame_domain_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--layout-timeline-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui layout timeline smoke ok", completed.stdout)
        self.assertIn("title_frame=110/220", completed.stdout)
        self.assertIn("pause_frame=120/240", completed.stdout)
        self.assertIn("loading_frame=180/240", completed.stdout)
        self.assertIn("fps=60", completed.stdout)

    def test_layout_primitive_smoke_reports_scene_graph_metrics_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--layout-primitive-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui layout primitive smoke ok", completed.stdout)
        self.assertIn("title_primitives=6 keyframes=806", completed.stdout)
        self.assertIn("pause_primitives=6 keyframes=806", completed.stdout)
        self.assertIn("loading_primitives=6 keyframes=2775", completed.stdout)

    def test_layout_primitive_smoke_reports_gameplay_hud_proxy_metrics_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--layout-primitive-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sonic_stage_primitives=6 keyframes=680", completed.stdout)
        self.assertIn("werehog_stage_primitives=6 keyframes=680", completed.stdout)
        self.assertIn("extra_stage_primitives=6 keyframes=680", completed.stdout)

    def test_layout_primitive_smoke_reports_gameplay_hud_scene_ownership_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--layout-primitive-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sonic_speed_gauge_kf=360", completed.stdout)
        self.assertIn("sonic_ring_energy_gauge_kf=240", completed.stdout)
        self.assertIn("sonic_ring_get_effect_kf=14", completed.stdout)
        self.assertIn("sonic_bg_kf=0", completed.stdout)

    def test_layout_primitive_playback_smoke_reports_gameplay_hud_animation_cursors_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--layout-primitive-playback-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui layout primitive playback smoke ok", completed.stdout)
        self.assertIn("speed_anim=Size_Anim speed_frame=50/100", completed.stdout)
        self.assertIn("energy_anim=Size_Anim energy_frame=50/100", completed.stdout)
        self.assertIn("info_anim=Count_Anim info_frame=50/100", completed.stdout)
        self.assertIn("ring_fx_anim=Intro_Anim ring_fx_frame=3/5", completed.stdout)
        self.assertIn("bg_anim=DefaultAnim bg_frame=50/100", completed.stdout)

    def test_layout_primitive_detail_smoke_reports_gameplay_hud_parity_cues_without_opening_window(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_GUI_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing GUI executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--layout-primitive-detail-smoke"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("sward_ui_runtime_debug_gui layout primitive detail smoke ok", completed.stdout)
        self.assertIn("primitives=6 keyframes=680", completed.stdout)
        self.assertIn("speed=so_speed_gauge/Size_Anim frame=50/100", completed.stdout)
        self.assertIn("ring_fx=ring_get_effect/Intro_Anim frame=3/5", completed.stdout)


if __name__ == "__main__":
    unittest.main()
