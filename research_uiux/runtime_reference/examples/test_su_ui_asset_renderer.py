#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
RENDERER_SOURCE = REPO_ROOT / "research_uiux" / "runtime_reference" / "examples" / "su_ui_asset_renderer.cpp"
CMAKE_FILE = REPO_ROOT / "research_uiux" / "runtime_reference" / "CMakeLists.txt"
DEFAULT_EXE = REPO_ROOT / "b" / "rr134" / "Release" / "sward_su_ui_asset_renderer.exe"


class SuUiAssetRendererTests(unittest.TestCase):
    def test_renderer_target_is_declared_as_clean_native_windows_executable(self) -> None:
        cmake_text = CMAKE_FILE.read_text(encoding="utf-8")
        self.assertIn("add_executable(sward_su_ui_asset_renderer WIN32", cmake_text)
        self.assertIn("examples/su_ui_asset_renderer.cpp", cmake_text)
        self.assertIn("user32", cmake_text)
        self.assertIn("gdi32", cmake_text)
        self.assertIn("gdiplus", cmake_text)

    def test_renderer_source_owns_clean_asset_backed_screen_catalog(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("SwardSuUiAssetRenderer", source_text)
        self.assertIn("SuUiRendererScreen", source_text)
        self.assertIn("SuUiRenderCast", source_text)
        self.assertIn("renderCleanScreen", source_text)
        self.assertIn("SonicTitleMenu", source_text)
        self.assertIn("LoadingTransition Composite", source_text)
        self.assertIn("LoadingComposite", source_text)
        self.assertIn("SonicStageHud", source_text)
        self.assertNotIn("kDebugWorkbenchHostEntries", source_text)

    def test_renderer_source_decodes_local_dds_textures_for_real_asset_blits(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("DdsTextureImage", source_text)
        self.assertIn("decodeDxt5Block", source_text)
        self.assertIn("loadDdsTextureImage", source_text)
        self.assertIn("mat_load_comon_001.dds", source_text)
        self.assertIn("ui_mm_base.dds", source_text)
        self.assertIn("ui_mm_parts1.dds", source_text)
        self.assertIn("ui_mm_contentstext.dds", source_text)
        self.assertIn("mat_title_en_001.dds", source_text)
        self.assertIn("OPmovie_titlelogo_EN.decompressed.dds", source_text)
        self.assertIn("mat_start_en_001.dds", source_text)
        self.assertIn("ui_ps1_gauge1.dds", source_text)
        self.assertIn("--renderer-smoke", source_text)

    def test_renderer_source_exposes_visible_viewer_navigation(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("kPrevButtonId", source_text)
        self.assertIn("kNextButtonId", source_text)
        self.assertIn("kAtlasPrevButtonId", source_text)
        self.assertIn("kAtlasNextButtonId", source_text)
        self.assertIn("kScreenLabelId", source_text)
        self.assertIn("createRendererControls", source_text)
        self.assertIn("layoutRendererControls", source_text)
        self.assertIn("updateRendererStatus", source_text)
        self.assertIn("selectedScreenIndexText", source_text)
        self.assertIn('CreateWindowExW(0, L"BUTTON", L"Prev"', source_text)
        self.assertIn('CreateWindowExW(0, L"BUTTON", L"Next"', source_text)
        self.assertIn('CreateWindowExW(0, L"BUTTON", L"Atlas Prev"', source_text)
        self.assertIn('CreateWindowExW(0, L"BUTTON", L"Atlas Next"', source_text)
        self.assertIn("--renderer-navigation-smoke", source_text)

    def test_renderer_source_exposes_local_visual_atlas_gallery_mode(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("RendererScreenKind::AtlasGallery", source_text)
        self.assertIn("VisualAtlasGallery", source_text)
        self.assertIn("discoverAtlasSheetPaths", source_text)
        self.assertIn("visual_atlas/sheets", source_text)
        self.assertIn("currentAtlasBitmap", source_text)
        self.assertIn("renderAtlasGalleryScreen", source_text)
        self.assertIn("--renderer-atlas-gallery-smoke", source_text)

    def test_renderer_source_exposes_reconstructed_sonic_hud_screen(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("SonicHudReconstruction", source_text)
        self.assertIn("Sonic HUD Reconstructed", source_text)
        self.assertIn("RendererScreenKind::SonicHudReconstruction", source_text)
        self.assertIn("kSonicHudReconstructionCasts", source_text)
        self.assertIn("renderSonicHudReconstructionScreen", source_text)
        self.assertIn("drawSlantedHudPanel", source_text)
        self.assertIn("ui_prov_playscreen.yncp", source_text)
        self.assertIn("so_speed_gauge_body", source_text)
        self.assertIn("so_ring_energy_body", source_text)
        self.assertIn("so_head_life_icon", source_text)
        self.assertIn("ring_digits_zeroes", source_text)
        self.assertIn("ring_energy_label", source_text)
        self.assertIn("mat_playscreen_001.dds", source_text)
        self.assertIn("mat_playscreen_en_001.dds", source_text)
        self.assertIn("mat_comon_num_001.dds", source_text)
        self.assertIn("--renderer-reconstructed-screen-smoke", source_text)

    def test_renderer_source_exposes_sgfx_template_placeholder_driver(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("sgfx_templates.hpp", source_text)
        self.assertIn("SgfxTemplateRenderBinding", source_text)
        self.assertIn("findSgfxTemplateRenderBinding", source_text)
        self.assertIn("renderSgfxTemplatePlaceholderScreen", source_text)
        self.assertIn("selectSgfxTemplate", source_text)
        self.assertIn("--template", source_text)
        self.assertIn("--sgfx-template-smoke", source_text)
        self.assertIn("title-menu", source_text)
        self.assertIn("loading", source_text)
        self.assertIn("sonic-hud", source_text)
        self.assertIn("tutorial", source_text)
        self.assertIn("title-menu-visible", source_text)
        self.assertIn("loading-display-active", source_text)
        self.assertIn("sonic-hud-ready", source_text)
        self.assertIn("tutorial-hud-owner-path-ready", source_text)
        self.assertIn("placeholder_slot=", source_text)
        self.assertIn("timeline_hook=", source_text)

    def test_renderer_source_exposes_csd_driven_local_pipeline_viewer(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdPipelineEvidence", source_text)
        self.assertIn("CsdPipelineSceneSummary", source_text)
        self.assertIn("loadCsdPipelineEvidence", source_text)
        self.assertIn("layout_deep_analysis.json", source_text)
        self.assertIn("findRuntimeEvidenceManifestForTarget", source_text)
        self.assertIn("renderCsdPipelineEvidenceOverlay", source_text)
        self.assertIn("--csd-pipeline-smoke", source_text)
        self.assertIn("csd_pipeline=", source_text)
        self.assertIn("sgfx_element_map=", source_text)
        self.assertIn("runtime_evidence_compare=", source_text)
        self.assertIn("ui_mainmenu.yncp", source_text)
        self.assertIn("ui_loading.yncp", source_text)
        self.assertIn("ui_prov_playscreen.yncp", source_text)
        self.assertIn("mm_bg_usual", source_text)
        self.assertIn("pda", source_text)
        self.assertIn("so_speed_gauge", source_text)

    def test_renderer_source_exposes_csd_drawable_traversal(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdDrawableCommand", source_text)
        self.assertIn("CsdDrawableScene", source_text)
        self.assertIn("loadCsdDrawableScene", source_text)
        self.assertIn("buildCsdDrawableCommands", source_text)
        self.assertIn("renderCsdDrawableScene", source_text)
        self.assertIn("--csd-drawable-smoke", source_text)
        self.assertIn("csd_drawable=", source_text)
        self.assertIn("csd_draw_command=", source_text)
        self.assertIn("texture_binding=", source_text)
        self.assertIn("sampled_transform=", source_text)
        self.assertIn("native_bmp_compare=", source_text)

    def test_renderer_source_exposes_csd_timeline_playback(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdTimelineKeyframe", source_text)
        self.assertIn("CsdTimelineTrackSample", source_text)
        self.assertIn("loadCsdTimelinePlayback", source_text)
        self.assertIn("sampleCsdTimelineTrack", source_text)
        self.assertIn("applyCsdTimelineToDrawableCommand", source_text)
        self.assertIn("--csd-timeline-smoke", source_text)
        self.assertIn("csd_timeline=", source_text)
        self.assertIn("timeline_sample=", source_text)
        self.assertIn("timeline_draw_command=", source_text)
        self.assertIn("rendered_frame_compare=", source_text)

    def test_renderer_source_exposes_phase127_offscreen_render_compare(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdRenderedFrameComparison", source_text)
        self.assertIn("renderCsdOffscreenFrame", source_text)
        self.assertIn("saveBitmapAsBmp", source_text)
        self.assertIn("computeBitmapComparisonStats", source_text)
        self.assertIn("writeCsdRenderCompareManifest", source_text)
        self.assertIn("--csd-render-compare-smoke", source_text)
        self.assertIn("rendered_frame_path=", source_text)
        self.assertIn("render_compare_manifest=", source_text)
        self.assertIn("visual_delta=", source_text)
        self.assertIn("material_semantics=", source_text)
        self.assertIn("blend=src-alpha/inv-src-alpha", source_text)
        self.assertIn("native_best_path=", source_text)

    def test_renderer_source_exposes_phase128_material_channel_alignment_semantics(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdColorRgba", source_text)
        self.assertIn("decodeCsdPackedRgba", source_text)
        self.assertIn("gradientTopLeftRgba", source_text)
        self.assertIn("additiveBlend", source_text)
        self.assertIn("linearFiltering", source_text)
        self.assertIn("packedColorTrackCount", source_text)
        self.assertIn("packedGradientTrackCount", source_text)
        self.assertIn("nativeAlignmentCrop", source_text)
        self.assertIn("center-crop-16x9", source_text)
        self.assertIn("color_order=rgba", source_text)
        self.assertIn("blend=src-alpha/inv-src-alpha", source_text)
        self.assertIn("blend=src-alpha/one", source_text)
        self.assertIn("channel_semantics=", source_text)
        self.assertIn("native_alignment=", source_text)

    def test_renderer_source_exposes_phase129_packed_rgba_and_software_quad_semantics(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdTimelinePackedRgbaKeyframe", source_text)
        self.assertIn("CsdTimelinePackedRgbaTrackSample", source_text)
        self.assertIn("parseCsdTimelinePackedRgbaKeyframes", source_text)
        self.assertIn("sampleCsdPackedRgbaTimelineTrack", source_text)
        self.assertIn("applyCsdPackedRgbaTimelineToDrawableCommand", source_text)
        self.assertIn("drawCsdDrawableCommandSoftware", source_text)
        self.assertIn("blendCsdPixelSrcAlphaOne", source_text)
        self.assertIn("sampleTextureArgbBilinear", source_text)
        self.assertIn("gradient_vertex_color", source_text)
        self.assertIn("quad_renderer=software-argb", source_text)
        self.assertIn("decoded_packed_keyframes=", source_text)
        self.assertIn("additive_software=", source_text)
        self.assertIn("writeCsdRenderCompareManifest", source_text)

    def test_renderer_source_exposes_phase130_sampler_and_registration_parity(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("sampleTextureArgbCsdFilter", source_text)
        self.assertIn("CsdSamplerStats", source_text)
        self.assertIn("csdPointFilterSampleCount", source_text)
        self.assertIn("CsdNativeFrameRegistration", source_text)
        self.assertIn("findBestNativeFrameRegistration", source_text)
        self.assertIn("registrationCandidateCount", source_text)
        self.assertIn("search-center-crop-16x9", source_text)
        self.assertIn("sampler_filter=csd-point-seam", source_text)
        self.assertIn("native_frame_registration=", source_text)
        self.assertIn("registration_candidates=", source_text)
        self.assertIn("registration_offset=", source_text)
        self.assertIn("writeCsdRenderCompareManifest", source_text)

    def test_renderer_source_exposes_phase131_full_frame_material_triage(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdFullFrameDeltaStats", source_text)
        self.assertIn("CsdMaterialParityTriage", source_text)
        self.assertIn("computeFullFrameDeltaStats", source_text)
        self.assertIn("materialParityTriageForComparison", source_text)
        self.assertIn("diffFramePath", source_text)
        self.assertIn("full_frame_delta=", source_text)
        self.assertIn("material_parity_triage=", source_text)
        self.assertIn("stage-background-not-rendered", source_text)
        self.assertIn("materialParityTriage", source_text)

    def test_renderer_source_exposes_phase132_ui_layer_coverage_delta(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdUiLayerMaskStats", source_text)
        self.assertIn("uiLayerDiffFramePath", source_text)
        self.assertIn("computeUiLayerDeltaStats", source_text)
        self.assertIn("renderCsdOffscreenFrameWithCoverageMask", source_text)
        self.assertIn("rendered-csd-coverage-mask", source_text)
        self.assertIn("ui_layer_delta=", source_text)
        self.assertIn("ui_layer_diff_frame_path=", source_text)
        self.assertIn("uiLayerDelta", source_text)
        self.assertIn("out\" / \"csd_render_compare\"", source_text)

    def test_renderer_source_exposes_phase133_sonic_hud_runtime_scene_archaeology(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdHudRuntimeSceneEvidence", source_text)
        self.assertIn("CsdHudSceneCoverageDiagnostic", source_text)
        self.assertIn("CsdHudCastCoverageDiagnostic", source_text)
        self.assertIn("loadLatestSonicHudLiveStateEvidence", source_text)
        self.assertIn("buildSonicHudSceneCoverageDiagnostics", source_text)
        self.assertIn("buildSonicHudCastCoverageDiagnostics", source_text)
        self.assertIn("sonic_hud_runtime_scene=", source_text)
        self.assertIn("sonic_hud_scene_coverage=", source_text)
        self.assertIn("sonic_hud_cast_coverage=", source_text)
        self.assertIn("owner_path_status=", source_text)
        self.assertIn("runtime_scene_matched=", source_text)
        self.assertIn("exact-ui-playscreen-layout-unrecovered", source_text)
        self.assertIn("local-proxy-layout-ui_prov_playscreen", source_text)
        self.assertIn("out\" / \"csd_render_compare\" / \"phase133", source_text)

    def test_renderer_source_exposes_phase134_runtime_csd_tree_export(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdHudRuntimeNodeEntry", source_text)
        self.assertIn("CsdHudRuntimeLayerEntry", source_text)
        self.assertIn("writeSonicHudRuntimeCsdTreeExport", source_text)
        self.assertIn("runRuntimeCsdTreeExportSmoke", source_text)
        self.assertIn("--export-runtime-csd-tree", source_text)
        self.assertIn("runtime_csd_export=", source_text)
        self.assertIn("runtime_csd_scene=", source_text)
        self.assertIn("runtime_csd_layer_sample=", source_text)
        self.assertIn("runtime-scene-layer-tree-exported-no-material-rects", source_text)
        self.assertIn("exact-ui-playscreen-layout-unrecovered", source_text)
        self.assertIn("out\" / \"csd_runtime_exports\" / \"phase134", source_text)

    def test_renderer_source_exposes_title_loop_reconstruction_screen(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("TitleLoopReconstruction", source_text)
        self.assertIn("Title Loop Reconstructed", source_text)
        self.assertIn("RendererScreenKind::TitleLoopReconstruction", source_text)
        self.assertIn("kTitleLoopReconstructionCasts", source_text)
        self.assertIn("renderTitleLoopReconstructionScreen", source_text)
        self.assertIn("titleMovieFrameBitmap", source_text)
        self.assertIn("titleLogoBitmap", source_text)
        self.assertIn("evmo_title_loop.sfd", source_text)
        self.assertIn("evmo_title_loop_00_00_35_000.png", source_text)
        self.assertIn("OPmovie_titlelogo_EN.decompressed.dds", source_text)
        self.assertIn("OPmovie_titlelogo_EN.decompressed.png", source_text)
        self.assertIn("opmovie_titlelogo_en", source_text)
        self.assertIn("ui_title/bg/bg", source_text)
        self.assertIn("ui_title/logo", source_text)
        self.assertIn("mm_title_intro", source_text)
        self.assertIn("CTitleStateIntro::Update", source_text)
        self.assertIn("UseAlternateTitleMidAsmHook", source_text)
        self.assertIn("--renderer-title-screen-smoke", source_text)

    def test_renderer_catalog_starts_with_full_screen_composition_not_single_arrow(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn('screen.id == "LoadingComposite"', source_text)
        self.assertIn('full_screen_casts=', source_text)
        self.assertIn('{ "load_composite", "full_screen", "mat_load_comon_001.dds", 0, 0, 1280, 720, 0, 0, 1280, 720 }', source_text)
        self.assertIn('kMainMenuCompositeCasts', source_text)
        self.assertIn('kTitleLogoSheetCasts', source_text)

    def test_renderer_smoke_reports_clean_screen_texture_inventory(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_su_ui_asset_renderer smoke ok", completed.stdout)
        self.assertIn("screens=8", completed.stdout)
        self.assertIn("casts=20", completed.stdout)
        self.assertIn("textures=20", completed.stdout)
        self.assertIn("full_screen_casts=1", completed.stdout)
        self.assertIn("TitleLoopReconstruction:ui_title/bg/bg/title_movie_frame:ui_mm_base.dds:DXT5:1280x720", completed.stdout)
        self.assertIn("TitleLoopReconstruction:ui_title/logo/opmovie_titlelogo_en:OPmovie_titlelogo_EN.decompressed.dds:DXT5:1280x720", completed.stdout)
        self.assertIn("TitleLoopReconstruction:mm_title_intro/press_start_text:mat_title_en_001.dds:DXT5:256x512", completed.stdout)
        self.assertIn("TitleLoopReconstruction:CTitleStateIntro::Update/alternate_title_gate:mat_title_en_001.dds:DXT5:256x512", completed.stdout)
        self.assertIn("SonicHudReconstruction:ui_prov_playscreen.yncp/so_speed_gauge_body:ui_ps1_gauge1.dds:DXT5:256x128", completed.stdout)
        self.assertIn("SonicHudReconstruction:ui_prov_playscreen.yncp/ring_energy_label:mat_playscreen_en_001.dds:DXT5:128x128", completed.stdout)
        self.assertIn("SonicHudReconstruction:ui_prov_playscreen.yncp/ring_digits_zeroes:mat_comon_num_001.dds:DXT5:512x64", completed.stdout)
        self.assertIn("LoadingComposite:load_composite/full_screen:mat_load_comon_001.dds:DXT5:1280x720:dst=0,0,1280x720", completed.stdout)
        self.assertNotIn("LoadingTransition:bg_1/arrow", completed.stdout)
        self.assertIn("MainMenuComposite:mm_bg/base_sheet:ui_mm_base.dds:DXT5:1280x720:dst=0,0,1280x720", completed.stdout)
        self.assertIn("SonicTitleMenu:mm_bg_usual/black3:ui_mm_parts1.dds:DXT5:1280x640", completed.stdout)
        self.assertIn("TitleLogoSheet:title/logo_en_001:mat_title_en_001.dds:DXT5:256x512", completed.stdout)
        self.assertIn("SonicStageHud:so_speed_gauge/position_hd:ui_ps1_gauge1.dds:DXT5:256x128", completed.stdout)

    def test_renderer_title_screen_smoke_reports_movie_backed_composition(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-title-screen-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_su_ui_asset_renderer title screen smoke ok", completed.stdout)
        self.assertIn("first=TitleLoopReconstruction", completed.stdout)
        self.assertIn("source=evmo_title_loop.sfd", completed.stdout)
        self.assertIn("contract=title_menu_reference.json", completed.stdout)
        self.assertIn("movie_frame=exists", completed.stdout)
        self.assertIn("title_logo_preview=exists", completed.stdout)
        self.assertIn("title_logo_preview_bitmap=loads", completed.stdout)
        self.assertIn("title_logo=exists", completed.stdout)
        self.assertIn("casts=4", completed.stdout)
        self.assertIn("resolved=4", completed.stdout)
        self.assertIn("in_bounds=4", completed.stdout)
        self.assertIn("scenes=ui_title/bg/bg,mm_title_intro,CTitleStateIntro::Update", completed.stdout)
        self.assertIn("title_movie_frame:ui_mm_base.dds:src=0,0,1280x720:dst=0,0,1280x720", completed.stdout)
        self.assertIn("opmovie_titlelogo_en:OPmovie_titlelogo_EN.decompressed.dds:src=0,0,1280x720:dst=0,0,1280x720", completed.stdout)
        self.assertIn("press_start_text:mat_title_en_001.dds:src=32,0,192x24:dst=550,540,180x24", completed.stdout)

    def test_renderer_reconstructed_screen_smoke_reports_screen_composition(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-reconstructed-screen-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_su_ui_asset_renderer reconstructed screen smoke ok", completed.stdout)
        self.assertIn("screen=SonicHudReconstruction", completed.stdout)
        self.assertIn("source=ui_prov_playscreen.yncp", completed.stdout)
        self.assertIn("contract=sonic_stage_hud_reference.json", completed.stdout)
        self.assertIn("casts=8", completed.stdout)
        self.assertIn("so_speed_gauge_body:ui_ps1_gauge1.dds:src=0,0,256x128:dst=18,512,430x215", completed.stdout)
        self.assertIn("so_ring_energy_body:ui_ps1_gauge1.dds:src=0,64,192x48:dst=40,636,320x80", completed.stdout)
        self.assertIn("ring_energy_label:mat_playscreen_en_001.dds", completed.stdout)
        self.assertIn("ring_digits_zeroes:mat_comon_num_001.dds", completed.stdout)

    def test_renderer_sgfx_template_smoke_reports_placeholder_screen_recipes(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--sgfx-template-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=20,
        )

        self.assertIn("sward_su_ui_asset_renderer sgfx template smoke ok", completed.stdout)
        self.assertIn("templates=4", completed.stdout)
        self.assertIn("bindings=4", completed.stdout)
        self.assertIn("template=title-menu:screen=MainMenuComposite:contract=title_menu_reference.json:event=title-menu-visible", completed.stdout)
        self.assertIn("template=loading:screen=LoadingComposite:contract=loading_transition_reference.json:event=loading-display-active", completed.stdout)
        self.assertIn("template=sonic-hud:screen=SonicHudReconstruction:contract=sonic_stage_hud_reference.json:event=sonic-hud-ready", completed.stdout)
        self.assertIn("template=tutorial:screen=SonicHudReconstruction:contract=sonic_stage_hud_reference.json:event=tutorial-hud-owner-path-ready", completed.stdout)
        self.assertIn("placeholder_slot=title-menu:logo->OPmovie_titlelogo_EN.decompressed.dds", completed.stdout)
        self.assertIn("placeholder_slot=loading:device_frame->mat_load_comon_001.dds", completed.stdout)
        self.assertIn("placeholder_slot=sonic-hud:speed_gauge->ui_ps1_gauge1.dds", completed.stdout)
        self.assertIn("placeholder_slot=tutorial:prompt_row->mat_start_en_001.dds", completed.stdout)
        self.assertIn("timeline_hook=title-menu:select_travel=0.333333:title menu visual ready", completed.stdout)
        self.assertIn("timeline_hook=loading:pda_intro=4.5:loading display active", completed.stdout)
        self.assertIn("timeline_hook=sonic-hud:hud_in=0.35:sonic-hud-ready", completed.stdout)
        self.assertIn("timeline_hook=tutorial:hud_in=0.35:tutorial-ready", completed.stdout)

    def test_renderer_template_argument_launches_specific_placeholder_recipe(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--template", "loading", "--sgfx-template-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=20,
        )

        self.assertIn("sward_su_ui_asset_renderer sgfx template smoke ok", completed.stdout)
        self.assertIn("templates=1", completed.stdout)
        self.assertIn("bindings=1", completed.stdout)
        self.assertIn("template=loading:screen=LoadingComposite:contract=loading_transition_reference.json:event=loading-display-active", completed.stdout)
        self.assertNotIn("template=title-menu:", completed.stdout)
        self.assertNotIn("template=sonic-hud:", completed.stdout)

    def test_renderer_csd_pipeline_smoke_reports_layout_driven_screen_recipes(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--csd-pipeline-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=20,
        )

        self.assertIn("sward_su_ui_asset_renderer csd pipeline smoke ok", completed.stdout)
        self.assertIn("layout_source=research_uiux/data/layout_deep_analysis.json", completed.stdout)
        self.assertIn("templates=4", completed.stdout)
        self.assertIn("packages=3", completed.stdout)
        self.assertIn("csd_pipeline=title-menu:layout=ui_mainmenu.yncp:scene=mm_bg_usual:casts=47:subimages=46", completed.stdout)
        self.assertIn("csd_pipeline=loading:layout=ui_loading.yncp:scene=pda:casts=57:subimages=320", completed.stdout)
        self.assertIn("csd_pipeline=sonic-hud:layout=ui_prov_playscreen.yncp:scene=so_speed_gauge:casts=47:subimages=109", completed.stdout)
        self.assertIn("csd_pipeline=tutorial:layout=ui_prov_playscreen.yncp:scene=info_1:casts=24:subimages=109", completed.stdout)
        self.assertIn("textures=ui_mm_base.dds,ui_mm_parts1.dds,ui_mm_contentstext.dds", completed.stdout)
        self.assertIn("textures=mat_comon_txt_001.dds,mat_load_comon_001.dds", completed.stdout)
        self.assertIn("textures=ui_ps1_gauge1.dds,mat_comon_001.dds,mat_comon_num_001.dds", completed.stdout)
        self.assertIn("timeline=mm_donut_move/DefaultAnim/220/3.666667", completed.stdout)
        self.assertIn("timeline=pda_txt/Usual_Anim_3/240/4", completed.stdout)
        self.assertIn("timeline=so_speed_gauge/Size_Anim/100/1.666667", completed.stdout)
        self.assertIn("sgfx_element_map=title-menu:scene=mm_bg_usual:slot=backdrop:texture=ui_mm_base.dds", completed.stdout)
        self.assertIn("sgfx_element_map=loading:scene=pda:slot=device_frame:texture=mat_load_comon_001.dds", completed.stdout)
        self.assertIn("sgfx_element_map=sonic-hud:scene=so_speed_gauge:slot=speed_gauge:texture=ui_ps1_gauge1.dds", completed.stdout)
        self.assertIn("runtime_evidence_compare=title-menu:target=title-menu:event=title-menu-visible", completed.stdout)
        self.assertIn("runtime_evidence_compare=loading:target=loading:event=loading-display-active", completed.stdout)
        self.assertIn("runtime_evidence_compare=sonic-hud:target=sonic-hud:event=sonic-hud-ready", completed.stdout)

    def test_renderer_csd_pipeline_template_filter_reports_one_layout_recipe(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--template", "sonic-hud", "--csd-pipeline-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=20,
        )

        self.assertIn("sward_su_ui_asset_renderer csd pipeline smoke ok", completed.stdout)
        self.assertIn("templates=1", completed.stdout)
        self.assertIn("packages=1", completed.stdout)
        self.assertIn("csd_pipeline=sonic-hud:layout=ui_prov_playscreen.yncp:scene=so_speed_gauge", completed.stdout)
        self.assertNotIn("csd_pipeline=title-menu:", completed.stdout)
        self.assertNotIn("csd_pipeline=loading:", completed.stdout)

    def test_renderer_csd_drawable_smoke_reports_scene_draw_commands(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--csd-drawable-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=60,
        )

        self.assertIn("sward_su_ui_asset_renderer csd drawable smoke ok", completed.stdout)
        self.assertIn("layout_source=research_uiux/data/layout_deep_analysis.json", completed.stdout)
        self.assertIn("templates=4", completed.stdout)
        self.assertIn("packages=3", completed.stdout)
        self.assertIn("csd_drawable=title-menu:layout=ui_mainmenu.yncp:scene=mm_bg_usual:commands=36:casts=47:subimages=46", completed.stdout)
        self.assertIn("csd_drawable=loading:layout=ui_loading.yncp:scene=pda:commands=28:casts=57:subimages=320", completed.stdout)
        self.assertIn("csd_drawable=sonic-hud:layout=ui_prov_playscreen.yncp:scene=so_speed_gauge:commands=43:casts=47:subimages=109", completed.stdout)
        self.assertIn("csd_drawable=tutorial:layout=ui_prov_playscreen.yncp:scene=info_1:commands=14:casts=24:subimages=109", completed.stdout)
        self.assertIn("csd_draw_command=title-menu:mm_bg_usual/black3->black3:texture=ui_mm_parts1.dds:subimage=14:src=896,336,16x16:dst=655,435,368x464", completed.stdout)
        self.assertIn("csd_draw_command=loading:pda/bg->bg:texture=mat_load_comon_001.dds:subimage=201:src=595,463,10x60:dst=180,146,920x60", completed.stdout)
        self.assertIn("csd_draw_command=sonic-hud:so_speed_gauge/Cast_0506_bg->Cast_0506_bg:texture=ui_ps1_gauge1.dds:subimage=1:src=4,64,16x20:dst=752,357,16x20", completed.stdout)
        self.assertIn("csd_draw_command=tutorial:info_1/bg_1->bg_1:texture=mat_playscreen_001.dds:subimage=97:src=56,2,1x30:dst=385,320,250x30", completed.stdout)
        self.assertIn("texture_binding=title-menu:ui_mm_parts1.dds:resolved=1:size=1280x640", completed.stdout)
        self.assertIn("texture_binding=loading:mat_load_comon_001.dds:resolved=1:size=1280x720", completed.stdout)
        self.assertIn("texture_binding=sonic-hud:ui_ps1_gauge1.dds:resolved=1:size=256x128", completed.stdout)
        self.assertIn("texture_binding=tutorial:mat_playscreen_001.dds:resolved=1:size=128x256", completed.stdout)
        self.assertIn("sampled_transform=title-menu:mm_bg_usual/black3:translation=0.155469,0.426389:scale=23,29", completed.stdout)
        self.assertIn("sampled_transform=loading:pda/bg:translation=0,-0.255556:scale=92,1", completed.stdout)
        self.assertIn("native_bmp_compare=title-menu:target=title-menu:event=title-menu-visible", completed.stdout)
        self.assertIn("native_bmp_compare=loading:target=loading:event=loading-display-active", completed.stdout)
        self.assertIn("native_bmp_compare=sonic-hud:target=sonic-hud:event=sonic-hud-ready", completed.stdout)
        self.assertIn("sgfx_element_map=title-menu:scene=mm_bg_usual:cast=black3:slot=backdrop", completed.stdout)

    def test_renderer_csd_drawable_template_filter_reports_one_scene(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--template", "loading", "--csd-drawable-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=60,
        )

        self.assertIn("sward_su_ui_asset_renderer csd drawable smoke ok", completed.stdout)
        self.assertIn("templates=1", completed.stdout)
        self.assertIn("packages=1", completed.stdout)
        self.assertIn("csd_drawable=loading:layout=ui_loading.yncp:scene=pda:commands=28", completed.stdout)
        self.assertIn("csd_draw_command=loading:pda/bg->bg:texture=mat_load_comon_001.dds", completed.stdout)
        self.assertNotIn("csd_drawable=title-menu:", completed.stdout)
        self.assertNotIn("csd_drawable=sonic-hud:", completed.stdout)

    def test_renderer_csd_timeline_smoke_reports_sampled_motion_and_native_compare(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--csd-timeline-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=60,
        )

        self.assertIn("sward_su_ui_asset_renderer csd timeline smoke ok", completed.stdout)
        self.assertIn("layout_source=research_uiux/data/layout_deep_analysis.json", completed.stdout)
        self.assertIn("templates=4", completed.stdout)
        self.assertIn("packages=3", completed.stdout)
        self.assertIn("csd_timeline=title-menu:layout=ui_mainmenu.yncp:scene=mm_donut_move:animation=DefaultAnim:frame=10/220:tracks=4:numeric=4:keyframes=12", completed.stdout)
        self.assertIn("csd_timeline=loading:layout=ui_loading.yncp:scene=pda_txt:animation=Usual_Anim_3:frame=75/240:tracks=18:numeric=6:keyframes=92", completed.stdout)
        self.assertIn("csd_timeline=sonic-hud:layout=ui_prov_playscreen.yncp:scene=so_speed_gauge:animation=Size_Anim:frame=99/100:tracks=174:numeric=14:keyframes=360", completed.stdout)
        self.assertIn("csd_timeline=tutorial:layout=ui_prov_playscreen.yncp:scene=info_1:animation=Count_Anim:frame=50/100:tracks=19:numeric=19:keyframes=19", completed.stdout)
        self.assertIn("timeline_sample=title-menu:mm_donut_move/compass:track=Rotation:frame=10:value=-16", completed.stdout)
        self.assertIn("timeline_sample=loading:pda_txt/txt_cursor:track=XScale:frame=75:value=0.75", completed.stdout)
        self.assertIn("timeline_sample=sonic-hud:so_speed_gauge/Cast_0641:track=YScale:frame=99:value=2", completed.stdout)
        self.assertIn("timeline_sample=tutorial:info_1/bg_1:track=GradientTL:frame=50:value=0", completed.stdout)
        self.assertIn("timeline_draw_command=sonic-hud:so_speed_gauge/Cast_0641:frame=99:track=YScale:value=2:dst=1000,317,17x22", completed.stdout)
        self.assertIn("rendered_frame_compare=title-menu:target=title-menu:event=title-menu-visible:frame=10:native=found", completed.stdout)
        self.assertIn("rendered_frame_compare=loading:target=loading:event=loading-display-active:frame=75:native=found", completed.stdout)
        self.assertIn("rendered_frame_compare=sonic-hud:target=sonic-hud:event=sonic-hud-ready:frame=99:native=found", completed.stdout)

    def test_renderer_csd_timeline_template_filter_reports_one_timeline(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--template", "sonic-hud", "--csd-timeline-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=60,
        )

        self.assertIn("sward_su_ui_asset_renderer csd timeline smoke ok", completed.stdout)
        self.assertIn("templates=1", completed.stdout)
        self.assertIn("packages=1", completed.stdout)
        self.assertIn("csd_timeline=sonic-hud:layout=ui_prov_playscreen.yncp:scene=so_speed_gauge:animation=Size_Anim", completed.stdout)
        self.assertIn("timeline_draw_command=sonic-hud:so_speed_gauge/Cast_0641:frame=99", completed.stdout)
        self.assertNotIn("csd_timeline=title-menu:", completed.stdout)
        self.assertNotIn("csd_timeline=loading:", completed.stdout)

    def test_renderer_csd_render_compare_smoke_writes_local_outputs_and_deltas(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--csd-render-compare-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=120,
        )

        self.assertIn("sward_su_ui_asset_renderer csd render compare smoke ok", completed.stdout)
        self.assertIn("templates=4", completed.stdout)
        self.assertIn("rendered_frame_path=title-menu:out/csd_render_compare/phase133/title-menu_frame10.bmp", completed.stdout)
        self.assertIn("rendered_frame_path=loading:out/csd_render_compare/phase133/loading_frame75.bmp", completed.stdout)
        self.assertIn("rendered_frame_path=sonic-hud:out/csd_render_compare/phase133/sonic-hud_frame99.bmp", completed.stdout)
        self.assertIn("rendered_frame_path=tutorial:out/csd_render_compare/phase133/tutorial_frame50.bmp", completed.stdout)
        self.assertIn("render_compare_manifest=out/csd_render_compare/phase133/csd_render_compare_manifest.json", completed.stdout)
        self.assertIn("visual_delta=title-menu:native=found:sample_grid=64x36:alignment=search-center-crop-16x9", completed.stdout)
        self.assertIn("visual_delta=loading:native=found:sample_grid=64x36:alignment=search-center-crop-16x9", completed.stdout)
        self.assertIn("visual_delta=sonic-hud:native=found:sample_grid=64x36:alignment=search-center-crop-16x9", completed.stdout)
        self.assertIn("material_semantics=title-menu:quad_renderer=software-argb:sampler_filter=csd-point-seam:color_order=rgba:blend=src-alpha/inv-src-alpha", completed.stdout)
        self.assertIn("material_semantics=loading:quad_renderer=software-argb:sampler_filter=csd-point-seam:color_order=rgba:blend=src-alpha/inv-src-alpha", completed.stdout)
        self.assertIn(":gradient_vertex_color=", completed.stdout)
        self.assertIn(":additive_software=", completed.stdout)
        self.assertIn(":csd_point_samples=", completed.stdout)
        self.assertIn("channel_semantics=sonic-hud:packed_color_tracks=", completed.stdout)
        self.assertIn("channel_semantics=tutorial:packed_color_tracks=", completed.stdout)
        self.assertIn(":decoded_packed_keyframes=", completed.stdout)
        self.assertIn("native_alignment=title-menu:mode=search-center-crop-16x9:crop=", completed.stdout)
        self.assertIn("native_alignment=sonic-hud:mode=search-center-crop-16x9:crop=", completed.stdout)
        self.assertIn("native_frame_registration=title-menu:mode=search-center-crop-16x9:registration_offset=", completed.stdout)
        self.assertIn(":registration_candidates=", completed.stdout)
        self.assertIn("diff_frame_path=sonic-hud:out/csd_render_compare/phase133/sonic-hud_frame99_diff.bmp", completed.stdout)
        self.assertIn("full_frame_delta=sonic-hud:mode=registered-full-frame-nearest", completed.stdout)
        self.assertIn("ui_layer_diff_frame_path=sonic-hud:out/csd_render_compare/phase133/sonic-hud_frame99_ui_layer_diff.bmp", completed.stdout)
        self.assertIn("ui_layer_delta=sonic-hud:mode=rendered-csd-coverage-mask", completed.stdout)
        self.assertIn("ui_layer_delta=tutorial:mode=rendered-csd-coverage-mask", completed.stdout)
        self.assertIn(":masked_pixels=", completed.stdout)
        self.assertIn("material_parity_triage=sonic-hud:primary=stage-background-not-rendered", completed.stdout)
        self.assertIn("material_parity_triage=tutorial:primary=stage-background-not-rendered", completed.stdout)
        self.assertIn("native_best_path=sonic-hud:", completed.stdout)
        self.assertIn("sonic_hud_runtime_scene=sonic-hud:runtime_project=ui_playscreen", completed.stdout)
        self.assertIn(":local_layout=ui_prov_playscreen.yncp:local_project=ui_prov_playscreen", completed.stdout)
        self.assertIn(":layout_status=exact-ui-playscreen-layout-unrecovered", completed.stdout)
        self.assertIn(":owner_path_status=raw CHudSonicStage owner hook live; CSD owner fields pending", completed.stdout)
        self.assertIn("sonic_hud_scene_coverage=sonic-hud:scene=so_speed_gauge", completed.stdout)
        self.assertIn(":runtime_scene_matched=1", completed.stdout)
        self.assertIn("sonic_hud_scene_coverage=sonic-hud:scene=so_ringenagy_gauge", completed.stdout)
        self.assertIn(":locally_rendered=0", completed.stdout)
        self.assertIn("sonic_hud_cast_coverage=sonic-hud:scene=so_speed_gauge", completed.stdout)
        self.assertIn(":native_nonblack_pixels=", completed.stdout)

    def test_renderer_runtime_csd_tree_export_smoke_writes_ui_playscreen_tree(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--export-runtime-csd-tree", "--template", "sonic-hud"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=120,
        )

        self.assertIn("sward_su_ui_asset_renderer runtime csd export ok", completed.stdout)
        self.assertIn("runtime_csd_export=sonic-hud:source=live-bridge:project=ui_playscreen", completed.stdout)
        self.assertIn(":scenes=13:nodes=2:layers=209", completed.stdout)
        self.assertIn(":layout_status=exact-ui-playscreen-layout-unrecovered", completed.stdout)
        self.assertIn(":drawable_status=runtime-scene-layer-tree-exported-no-material-rects", completed.stdout)
        self.assertIn("runtime_csd_scene=sonic-hud:ui_playscreen/so_speed_gauge:casts=47", completed.stdout)
        self.assertIn("runtime_csd_layer_sample=sonic-hud:ui_playscreen/so_speed_gauge", completed.stdout)
        self.assertIn("runtime_csd_export_path=out/csd_runtime_exports/phase134/ui_playscreen_runtime_tree.json", completed.stdout)

    def test_renderer_navigation_smoke_reports_interactive_catalog(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-navigation-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_su_ui_asset_renderer navigation smoke ok", completed.stdout)
        self.assertIn("screens=8", completed.stdout)
        self.assertIn("controls=5", completed.stdout)
        self.assertIn("first=TitleLoopReconstruction", completed.stdout)
        self.assertIn("last=SonicStageHud", completed.stdout)
        self.assertIn("label=1/8 TitleLoopReconstruction - Title Loop Reconstructed", completed.stdout)
        self.assertIn("screen=TitleLoopReconstruction:casts=4:contract=title_menu_reference.json", completed.stdout)
        self.assertIn("screen=MainMenuComposite:casts=3:contract=title_menu_reference.json", completed.stdout)
        self.assertIn("screen=VisualAtlasGallery:casts=0:contract=visual_atlas/atlas_index.json", completed.stdout)

    def test_renderer_atlas_gallery_smoke_reports_local_sheet_inventory(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-atlas-gallery-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_su_ui_asset_renderer atlas gallery smoke ok", completed.stdout)
        self.assertIn("sheets=22", completed.stdout)
        self.assertIn("first=actioncommon__ui_gate.png", completed.stdout)
        self.assertIn("loading=loading__ui_loading.png", completed.stdout)
        self.assertIn("mainmenu=mainmenu__ui_mainmenu.png", completed.stdout)
        self.assertIn("status=systemcommoncore__ui_status.png", completed.stdout)


if __name__ == "__main__":
    unittest.main()
