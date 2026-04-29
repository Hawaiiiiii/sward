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
        self.assertIn("ui_playscreen.yncp", source_text)
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

    def test_renderer_source_exposes_phase135_runtime_material_export(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("CsdHudRuntimeMaterialEntry", source_text)
        self.assertIn("writeSonicHudRuntimeMaterialExport", source_text)
        self.assertIn("runRuntimeCsdMaterialExportSmoke", source_text)
        self.assertIn("--export-runtime-csd-materials", source_text)
        self.assertIn("runtime_csd_material_export=", source_text)
        self.assertIn("runtime_csd_material_scene=", source_text)
        self.assertIn("runtime_csd_material_sample=", source_text)
        self.assertIn("runtime-material-exact-local-layout", source_text)
        self.assertIn("runtime-tree+exact-local-layout", source_text)
        self.assertIn("materialSourceStatus", source_text)
        self.assertIn("out\" / \"csd_runtime_exports\" / \"phase135", source_text)

    def test_renderer_source_exposes_phase136_sonic_hud_compositor_reference_export(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("SonicHudCompositorScene", source_text)
        self.assertIn("SonicHudCompositorModel", source_text)
        self.assertIn("buildSonicHudCompositorModel", source_text)
        self.assertIn("writeSonicHudCompositorManifest", source_text)
        self.assertIn("writeSonicHudReferenceCode", source_text)
        self.assertIn("runSonicHudCompositorExportSmoke", source_text)
        self.assertIn("--export-sonic-hud-compositor", source_text)
        self.assertIn("sonic_hud_compositor=", source_text)
        self.assertIn("sonic_hud_compositor_scene=", source_text)
        self.assertIn("sonic_hud_reference_owner=", source_text)
        self.assertIn("clean-readable-reference-exported", source_text)
        self.assertIn("out\" / \"csd_runtime_exports\" / \"phase136", source_text)

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
        self.assertIn("screens=10", completed.stdout)
        self.assertIn("casts=20", completed.stdout)
        self.assertIn("textures=20", completed.stdout)
        self.assertIn("full_screen_casts=1", completed.stdout)
        self.assertIn("TitleLoopReconstruction:ui_title/bg/bg/title_movie_frame:ui_mm_base.dds:DXT5:1280x720", completed.stdout)
        self.assertIn("TitleLoopReconstruction:ui_title/logo/opmovie_titlelogo_en:OPmovie_titlelogo_EN.decompressed.dds:DXT5:1280x720", completed.stdout)
        self.assertIn("TitleLoopReconstruction:mm_title_intro/press_start_text:mat_title_en_001.dds:DXT5:256x512", completed.stdout)
        self.assertIn("TitleLoopReconstruction:CTitleStateIntro::Update/alternate_title_gate:mat_title_en_001.dds:DXT5:256x512", completed.stdout)
        self.assertIn("SonicHudReconstruction:ui_prov_playscreen.yncp/so_speed_gauge_body:ui_ps1_gauge1.dds:BGRA8:256x128", completed.stdout)
        self.assertIn("SonicHudReconstruction:ui_prov_playscreen.yncp/ring_energy_label:mat_playscreen_en_001.dds:DXT5:128x128", completed.stdout)
        self.assertIn("SonicHudReconstruction:ui_prov_playscreen.yncp/ring_digits_zeroes:mat_comon_num_001.dds:DXT5:512x64", completed.stdout)
        self.assertIn("LoadingComposite:load_composite/full_screen:mat_load_comon_001.dds:DXT5:1280x720:dst=0,0,1280x720", completed.stdout)
        self.assertNotIn("LoadingTransition:bg_1/arrow", completed.stdout)
        self.assertIn("MainMenuComposite:mm_bg/base_sheet:ui_mm_base.dds:DXT5:1280x720:dst=0,0,1280x720", completed.stdout)
        self.assertIn("SonicTitleMenu:mm_bg_usual/black3:ui_mm_parts1.dds:DXT5:1280x640", completed.stdout)
        self.assertIn("TitleLogoSheet:title/logo_en_001:mat_title_en_001.dds:DXT5:256x512", completed.stdout)
        self.assertIn("SonicStageHud:so_speed_gauge/position_hd:ui_ps1_gauge1.dds:BGRA8:256x128", completed.stdout)

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
        self.assertIn("csd_pipeline=sonic-hud:layout=ui_playscreen.yncp:scene=so_speed_gauge:casts=47:subimages=202", completed.stdout)
        self.assertIn("csd_pipeline=tutorial:layout=ui_playscreen.yncp:scene=u_info:casts=16:subimages=202", completed.stdout)
        self.assertIn("textures=ui_mm_base.dds,ui_mm_parts1.dds,ui_mm_contentstext.dds", completed.stdout)
        self.assertIn("textures=mat_comon_txt_001.dds,mat_load_comon_001.dds", completed.stdout)
        self.assertIn("textures=mat_comon_001.dds,mat_comon_002.dds,mat_comon_003.dds,mat_comon_004.dds,mat_comon_num_001.dds", completed.stdout)
        self.assertIn("timeline=mm_donut_move/DefaultAnim/220/3.666667", completed.stdout)
        self.assertIn("timeline=pda_txt/Usual_Anim_3/240/4", completed.stdout)
        self.assertIn("timeline=so_speed_gauge/DefaultAnim/100/1.666667", completed.stdout)
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
        self.assertIn("csd_pipeline=sonic-hud:layout=ui_playscreen.yncp:scene=so_speed_gauge", completed.stdout)
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
        self.assertIn("csd_drawable=title-menu:layout=ui_mainmenu.yncp:scene=mm_bg_usual:commands=47:casts=47:subimages=46", completed.stdout)
        self.assertIn("csd_drawable=loading:layout=ui_loading.yncp:scene=pda:commands=54:casts=57:subimages=320", completed.stdout)
        self.assertIn("csd_drawable=sonic-hud:layout=ui_playscreen.yncp:scene=so_speed_gauge:commands=47:casts=47:subimages=202", completed.stdout)
        self.assertIn("csd_drawable=tutorial:layout=ui_playscreen.yncp:scene=u_info:commands=16:casts=16:subimages=202", completed.stdout)
        self.assertIn("csd_draw_command=title-menu:mm_bg_usual/black3->black3:texture=ui_mm_parts1.dds:subimage=14:src=896,336,16x16:dst=655,435,368x464", completed.stdout)
        self.assertIn("csd_draw_command=loading:pda/bg->bg:texture=mat_load_comon_001.dds:subimage=201:src=595,463,10x60:dst=180,146,920x60", completed.stdout)
        self.assertIn("csd_draw_command=sonic-hud:so_speed_gauge/Cast_0506_bg->Cast_0506_bg:texture=ui_ps1_gauge1.dds:subimage=154:src=4,64,16x20:dst=752,357,16x20", completed.stdout)
        self.assertIn("csd_draw_command=tutorial:u_info/bar->bar:texture=mat_hit_001.dds:subimage=115:src=246,248,1x7:dst=646,352,243x7", completed.stdout)
        self.assertIn(":source-free-structural", completed.stdout)
        self.assertIn("texture_binding=title-menu:ui_mm_parts1.dds:resolved=1:size=1280x640", completed.stdout)
        self.assertIn("texture_binding=loading:mat_load_comon_001.dds:resolved=1:size=1280x720", completed.stdout)
        self.assertIn("texture_binding=sonic-hud:ui_ps1_gauge1.dds:resolved=1:size=256x128", completed.stdout)
        self.assertIn("texture_binding=tutorial:mat_hit_001.dds:resolved=1:size=256x256", completed.stdout)
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
        self.assertIn("csd_drawable=loading:layout=ui_loading.yncp:scene=pda:commands=54", completed.stdout)
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
        self.assertIn("csd_timeline=sonic-hud:layout=ui_playscreen.yncp:scene=so_speed_gauge:animation=DefaultAnim:frame=99/100:tracks=174:numeric=14:keyframes=360", completed.stdout)
        self.assertIn("csd_timeline=tutorial:layout=ui_playscreen.yncp:scene=u_info:animation=Intro_Anim:frame=20/20:tracks=30:numeric=28:keyframes=34", completed.stdout)
        self.assertIn("timeline_sample=title-menu:mm_donut_move/compass:track=Rotation:frame=10:value=-16", completed.stdout)
        self.assertIn("timeline_sample=loading:pda_txt/txt_cursor:track=XScale:frame=75:value=0.75", completed.stdout)
        self.assertIn("timeline_sample=sonic-hud:so_speed_gauge/Cast_0641:track=YScale:frame=99:value=2", completed.stdout)
        self.assertIn("timeline_sample=tutorial:u_info/bar:track=GradientTL:frame=20:value=-6.031250", completed.stdout)
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
        self.assertIn("csd_timeline=sonic-hud:layout=ui_playscreen.yncp:scene=so_speed_gauge:animation=DefaultAnim", completed.stdout)
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
        self.assertIn(":local_layout=ui_playscreen.yncp:local_project=ui_playscreen", completed.stdout)
        self.assertIn(":layout_status=exact-runtime-layout", completed.stdout)
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
        self.assertIn(":layout_status=exact-runtime-layout", completed.stdout)
        self.assertIn(":drawable_status=runtime-scene-layer-tree-exported-no-material-rects", completed.stdout)
        self.assertIn("runtime_csd_scene=sonic-hud:ui_playscreen/so_speed_gauge:casts=47", completed.stdout)
        self.assertIn("runtime_csd_layer_sample=sonic-hud:ui_playscreen/so_speed_gauge", completed.stdout)
        self.assertIn("runtime_csd_export_path=out/csd_runtime_exports/phase134/ui_playscreen_runtime_tree.json", completed.stdout)

    def test_renderer_runtime_csd_material_export_smoke_recovers_exact_materials(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--export-runtime-csd-materials", "--template", "sonic-hud"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=240,
        )

        self.assertIn("sward_su_ui_asset_renderer runtime csd material export ok", completed.stdout)
        self.assertIn("runtime_csd_material_export=sonic-hud:source=runtime-tree+exact-local-layout:project=ui_playscreen", completed.stdout)
        self.assertIn(":local_layout=ui_playscreen.yncp:local_project=ui_playscreen", completed.stdout)
        self.assertIn(":runtime_layers=209:exported_layers=203:material_resolved=167:subimage_resolved=167:timeline_resolved=167:material_unresolved=36", completed.stdout)
        self.assertIn(":drawable_status=runtime-material-exact-local-layout", completed.stdout)
        self.assertIn("runtime_csd_material_scene=sonic-hud:ui_playscreen/so_speed_gauge:layers=47:material_resolved=43:subimage_resolved=43:timeline=DefaultAnim@99/100", completed.stdout)
        self.assertIn("runtime_csd_material_scene=sonic-hud:ui_playscreen/add/u_info:layers=10:material_resolved=5:subimage_resolved=5:timeline=Intro_Anim@20/20", completed.stdout)
        self.assertIn("runtime_csd_material_sample=sonic-hud:ui_playscreen/so_speed_gauge/position/speed_bg/Cast_0506_bg:cast=Cast_0506_bg:texture=ui_ps1_gauge1.dds:subimage=154:src=4,64,16x20:dst=752,357,16x20:timeline=DefaultAnim@99", completed.stdout)
        self.assertIn("runtime_csd_material_export_path=out/csd_runtime_exports/phase135/ui_playscreen_runtime_materials.json", completed.stdout)

    def test_renderer_sonic_hud_compositor_export_smoke_writes_reference_code(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--export-sonic-hud-compositor", "--template", "sonic-hud"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer sonic hud compositor export ok", completed.stdout)
        self.assertIn("sonic_hud_compositor=sonic-hud:source=runtime-tree+exact-local-layout:project=ui_playscreen", completed.stdout)
        self.assertIn(":scenes=13:runtime_layers=209:exported_layers=203:drawable_layers=167:structural_layers=36", completed.stdout)
        self.assertIn(":owner=CHudSonicStage:state=sonic-hud-ready:reference_status=clean-readable-reference-exported", completed.stdout)
        self.assertIn("sonic_hud_compositor_scene=sonic-hud:ui_playscreen/so_speed_gauge:activation=stage-hud-ready:slot=speed_gauge:runtime_layers=47:drawable_layers=43:timeline=DefaultAnim@99/100:textures=1", completed.stdout)
        self.assertIn("sonic_hud_compositor_scene=sonic-hud:ui_playscreen/so_ringenagy_gauge:activation=stage-hud-ready:slot=energy_gauge:runtime_layers=43:drawable_layers=40:timeline=total_quantity@99/100:textures=1", completed.stdout)
        self.assertIn("sonic_hud_compositor_scene=sonic-hud:ui_playscreen/add/u_info:activation=tutorial-hud-owner-path-ready:slot=prompt_strip:runtime_layers=10:drawable_layers=5:timeline=Intro_Anim@20/20:textures=3", completed.stdout)
        self.assertIn("sonic_hud_reference_owner=CHudSonicStage:ownerHook=sub_824D9308:project=ui_playscreen:sceneCount=13", completed.stdout)
        self.assertIn("sonic_hud_reference_code_path=out/csd_runtime_exports/phase136/ui_playscreen_hud_reference.hpp", completed.stdout)
        self.assertIn("sonic_hud_compositor_manifest_path=out/csd_runtime_exports/phase136/ui_playscreen_hud_compositor.json", completed.stdout)

    def test_renderer_source_wires_phase137_sonic_hud_reference_viewer(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("sonic_hud_reference.hpp", source_text)
        self.assertIn("RendererScreenKind::SonicHudReferencePipeline", source_text)
        self.assertIn("renderSonicHudReferencePolicyScene", source_text)
        self.assertIn("renderSonicHudReferenceViewerOverlay", source_text)
        self.assertIn("runRendererSonicHudReferencePolicySmoke", source_text)
        self.assertIn("--renderer-sonic-hud-reference-smoke", source_text)
        self.assertIn("phase137-ui_playscreen-policy", source_text)
        self.assertIn("compact-reference-status", source_text)

    def test_renderer_source_wires_phase139_reference_viewer_lanes_and_compare(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("RendererScreenKind::CsdReferencePipeline", source_text)
        self.assertIn("CsdReferenceViewerLane", source_text)
        self.assertIn("renderCsdReferenceViewerLane", source_text)
        self.assertIn("renderCsdReferenceViewerOverlay", source_text)
        self.assertIn("runRendererReferenceLanesSmoke", source_text)
        self.assertIn("runViewerRenderCompareSmoke", source_text)
        self.assertIn("--renderer-reference-lanes-smoke", source_text)
        self.assertIn("--viewer-render-compare-smoke", source_text)
        self.assertIn("phase139-reference-viewer", source_text)
        self.assertIn("viewer_render_compare", source_text)
        self.assertIn("TitleOptionsReference", source_text)
        self.assertIn("PauseMenuReference", source_text)
        self.assertIn("frontend_screen_reference.hpp", source_text)
        self.assertIn("buildReferenceViewerLanesFromTrackedPolicy", source_text)
        self.assertIn("title-options", source_text)
        self.assertIn("compact-reference-status:no-template-card=1", source_text)

    def test_renderer_source_wires_phase140_reference_playback_and_policy_export(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("phase142-tracked-policy-playback", source_text)
        self.assertIn("sourceFreeStructural", source_text)
        self.assertIn("renderCsdReferenceViewerCommands", source_text)
        self.assertIn("CsdReusableReferenceScreenModel", source_text)
        self.assertIn("writeReusableScreenReferenceCode", source_text)
        self.assertIn("runReferencePolicyExportSmoke", source_text)
        self.assertIn("--renderer-reference-policy-export-smoke", source_text)

    def test_renderer_source_wires_phase142_reference_lanes_to_tracked_policy(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("frontend_screen_reference.hpp", source_text)
        self.assertIn("frontendScreenPolicies", source_text)
        self.assertIn("buildReferenceViewerLanesFromTrackedPolicy", source_text)
        self.assertIn("defaultFrontendRuntimeAlignment", source_text)
        self.assertIn("phase142-tracked-policy-playback", source_text)
        self.assertNotIn("kTitleMenuReferenceViewerScenes", source_text)
        self.assertNotIn("kLoadingReferenceViewerScenes", source_text)
        self.assertNotIn("kTitleOptionsReferenceViewerScenes", source_text)
        self.assertNotIn("kPauseReferenceViewerScenes", source_text)
        self.assertNotIn("kCsdReferenceViewerLanes{{", source_text)

    def test_renderer_source_wires_phase143_live_state_runtime_alignment(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendLiveStateAlignmentEvidence", source_text)
        self.assertIn("findLatestFrontendLiveStatePath", source_text)
        self.assertIn("loadFrontendRuntimeAlignmentFromLiveState", source_text)
        self.assertIn("formatFrontendRuntimeAlignmentEvidence", source_text)
        self.assertIn("runRendererRuntimeAlignmentSmoke", source_text)
        self.assertIn("--renderer-runtime-alignment-smoke", source_text)
        self.assertIn("phase143-live-state-alignment", source_text)
        self.assertIn("runtime_alignment_source=ui_lab_live_state", source_text)

    def test_renderer_source_wires_phase144_direct_live_bridge_alignment_probe(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendLiveBridgeProbeResult", source_text)
        self.assertIn("discoverFrontendLiveBridgeName", source_text)
        self.assertIn("queryUiLabLiveBridgeState", source_text)
        self.assertIn("loadFrontendRuntimeAlignmentFromLiveBridge", source_text)
        self.assertIn("runRendererLiveBridgeAlignmentSmoke", source_text)
        self.assertIn("--renderer-live-bridge-alignment-smoke", source_text)
        self.assertIn("phase144-live-bridge-alignment", source_text)
        self.assertIn("runtime_alignment_probe=direct-live-bridge", source_text)

    def test_renderer_source_wires_phase145_runtime_ui_oracle_smoke(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendUiOracleEvidence", source_text)
        self.assertIn("queryUiLabLiveBridgeUiOracle", source_text)
        self.assertIn("loadFrontendUiOracleEvidence", source_text)
        self.assertIn("runRendererUiOracleSmoke", source_text)
        self.assertIn("--renderer-ui-oracle-smoke", source_text)
        self.assertIn("phase145-ui-only-oracle", source_text)
        self.assertIn("ui_oracle_probe=direct-ui-oracle", source_text)

    def test_renderer_source_wires_phase146_ui_oracle_playback_clock(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendUiOraclePlaybackClock", source_text)
        self.assertIn("loadFrontendUiOraclePlaybackClock", source_text)
        self.assertIn("applyUiOraclePlaybackFrameToLane", source_text)
        self.assertIn("runRendererUiOraclePlaybackSmoke", source_text)
        self.assertIn("--renderer-ui-oracle-playback-smoke", source_text)
        self.assertIn("phase146-ui-oracle-playback", source_text)
        self.assertIn("playback_clock=ui-oracle-runtime-frame", source_text)
        self.assertIn("timeline_frame_source=ui-oracle-mod-frame", source_text)

    def test_renderer_source_wires_phase147_runtime_drawable_oracle(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendRuntimeDrawableOracle", source_text)
        self.assertIn("buildFrontendRuntimeDrawableOracle", source_text)
        self.assertIn("runRendererUiDrawableOracleSmoke", source_text)
        self.assertIn("--renderer-ui-drawable-oracle-smoke", source_text)
        self.assertIn("phase147-ui-drawable-oracle", source_text)
        self.assertIn("runtime_drawable_oracle_status=runtime-csd-tree-local-material", source_text)
        self.assertIn("gpu_draw_list_status=pending", source_text)
        self.assertIn("drawable_scene_source=ui-oracle-active-scenes", source_text)

    def test_renderer_source_wires_phase149_runtime_draw_list_consumption(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendRuntimeDrawListEvidence", source_text)
        self.assertIn("FrontendRuntimeDrawCall", source_text)
        self.assertIn("queryUiLabLiveBridgeUiDrawList", source_text)
        self.assertIn("loadFrontendRuntimeDrawListEvidence", source_text)
        self.assertIn("buildFrontendRuntimeDrawListTriage", source_text)
        self.assertIn("runRendererUiDrawListTriageSmoke", source_text)
        self.assertIn("--renderer-ui-draw-list-triage-smoke", source_text)
        self.assertIn("phase149-ui-draw-list-triage", source_text)
        self.assertIn("runtime_draw_list_source=ui-draw-list", source_text)
        self.assertIn("material_triage=runtime-rectangles-vs-local-csd", source_text)
        self.assertIn("backend_submit_status=pending", source_text)

    def test_renderer_source_wires_phase150_backend_submit_material_triage(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendGpuSubmitEvidence", source_text)
        self.assertIn("FrontendGpuSubmitCall", source_text)
        self.assertIn("queryUiLabLiveBridgeGpuSubmit", source_text)
        self.assertIn("loadFrontendGpuSubmitEvidence", source_text)
        self.assertIn("buildFrontendGpuSubmitMaterialTriage", source_text)
        self.assertIn("runRendererGpuSubmitTriageSmoke", source_text)
        self.assertIn("--renderer-gpu-submit-triage-smoke", source_text)
        self.assertIn("phase150-backend-submit-material-triage", source_text)
        self.assertIn("gpu_submit_source=ui-gpu-submit", source_text)
        self.assertIn("material_triage=backend-submit-vs-runtime-rectangles", source_text)
        self.assertIn("backend_submit_status=render-thread-material-submit", source_text)

    def test_renderer_source_wires_phase151_material_correlation_oracle(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendMaterialCorrelationEvidence", source_text)
        self.assertIn("FrontendMaterialCorrelationPair", source_text)
        self.assertIn("queryUiLabLiveBridgeMaterialCorrelation", source_text)
        self.assertIn("loadFrontendMaterialCorrelationEvidence", source_text)
        self.assertIn("buildFrontendMaterialCorrelationTriage", source_text)
        self.assertIn("runRendererMaterialCorrelationSmoke", source_text)
        self.assertIn("--renderer-material-correlation-smoke", source_text)
        self.assertIn("phase151-material-correlation", source_text)
        self.assertIn("material_correlation_source=ui-material-correlation", source_text)
        self.assertIn("blend_semantics=runtime-submit-named", source_text)
        self.assertIn("sampler_semantics=runtime-submit-named", source_text)
        self.assertIn("raw_backend_command_status=", source_text)

    def test_renderer_source_wires_phase152_backend_resolved_submit_oracle(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendBackendResolvedEvidence", source_text)
        self.assertIn("FrontendBackendResolvedSubmit", source_text)
        self.assertIn("queryUiLabLiveBridgeBackendResolved", source_text)
        self.assertIn("loadFrontendBackendResolvedEvidence", source_text)
        self.assertIn("buildFrontendBackendResolvedTriage", source_text)
        self.assertIn("runRendererBackendResolvedTriageSmoke", source_text)
        self.assertIn("--renderer-backend-resolved-triage-smoke", source_text)
        self.assertIn("phase152-backend-resolved-submit", source_text)
        self.assertIn("backend_resolved_source=ui-backend-resolved", source_text)
        self.assertIn("material_correlation_backend_resolved=joined", source_text)
        self.assertIn("resolved_pso_blend_framebuffer=runtime-backend", source_text)

    def test_renderer_source_wires_phase153_backend_material_parity_hints(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendBackendMaterialParityTriage", source_text)
        self.assertIn("buildFrontendBackendMaterialParityTriage", source_text)
        self.assertIn("runRendererMaterialParityHintsSmoke", source_text)
        self.assertIn("--renderer-material-parity-hints-smoke", source_text)
        self.assertIn("phase153-backend-material-parity-hints", source_text)
        self.assertIn("material_parity_policy=backend-resolved-pso-blend-framebuffer", source_text)
        self.assertIn("texture_view_sampler_gap=pending", source_text)
        self.assertIn("text_movie_sfx_gap=pending", source_text)

    def test_renderer_source_wires_phase154_descriptor_semantics(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendDescriptorSemanticsTriage", source_text)
        self.assertIn("buildFrontendDescriptorSemanticsTriage", source_text)
        self.assertIn("runRendererDescriptorSemanticsSmoke", source_text)
        self.assertIn("--renderer-descriptor-semantics-smoke", source_text)
        self.assertIn("phase154-texture-sampler-descriptor-semantics", source_text)
        self.assertIn("texture_sampler_policy=runtime-descriptor-state", source_text)
        self.assertIn("vendor_descriptor_gap=pending-native-descriptor-dump", source_text)
        self.assertIn("textureDescriptorSemantic", source_text)
        self.assertIn("samplerDescriptorSemantic", source_text)

    def test_renderer_source_wires_phase155_vendor_resource_capture(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendVendorResourceCaptureTriage", source_text)
        self.assertIn("buildFrontendVendorResourceCaptureTriage", source_text)
        self.assertIn("runRendererVendorResourceCaptureSmoke", source_text)
        self.assertIn("--renderer-vendor-resource-capture-smoke", source_text)
        self.assertIn("phase155-vendor-resource-capture", source_text)
        self.assertIn("vendor_resource_policy=native-rhi-resource-view-sampler", source_text)
        self.assertIn("ui_only_layer_status=pending-runtime-ui-render-target-copy", source_text)
        self.assertIn("native_command_gap=pending-full-vendor-command-buffer-dump", source_text)
        self.assertIn("vendorResourceCaptureStatus", source_text)
        self.assertIn("textureResourceViewKnownCount", source_text)

    def test_renderer_source_wires_phase156_material_resource_view_parity(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendMaterialResourceViewParityTriage", source_text)
        self.assertIn("buildFrontendMaterialResourceViewParityTriage", source_text)
        self.assertIn("runRendererMaterialResourceViewParitySmoke", source_text)
        self.assertIn("--renderer-material-resource-view-parity-smoke", source_text)
        self.assertIn("phase156-material-resource-view-parity", source_text)
        self.assertIn("material_parity_policy=vendor-resource-view-alpha-gamma-srgb", source_text)
        self.assertIn("ui_only_capture_policy=copy-ui-render-target-before-present", source_text)
        self.assertIn("resource_view_exactness=", source_text)
        self.assertIn("premultiplied_alpha_status=", source_text)
        self.assertIn("gamma_srgb_status=", source_text)
        self.assertIn("materialResourceViewParityStatus", source_text)
        self.assertIn("resourceViewExactPairCount", source_text)

    def test_renderer_source_wires_phase157_vendor_command_resource_dump(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendVendorCommandResourceDumpTriage", source_text)
        self.assertIn("buildFrontendVendorCommandResourceDumpTriage", source_text)
        self.assertIn("runRendererVendorCommandResourceDumpSmoke", source_text)
        self.assertIn("--renderer-vendor-command-resource-dump-smoke", source_text)
        self.assertIn("phase157-vendor-command-resource-dump", source_text)
        self.assertIn("vendor_command_resource_dump_policy=raw-backend-command-plus-resource-view-dump", source_text)
        self.assertIn("ui_only_layer_status=pending-runtime-ui-render-target-copy", source_text)
        self.assertIn("vendor_command_replay_gap=pending-full-vendor-command-buffer-replay", source_text)
        self.assertIn("vendorCommandResourceDumpStatus", source_text)
        self.assertIn("rawBackendCommandCount", source_text)

    def test_renderer_source_wires_phase158_ui_layer_capture(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendUiOnlyLayerCaptureTriage", source_text)
        self.assertIn("buildFrontendUiOnlyLayerCaptureTriage", source_text)
        self.assertIn("runRendererUiOnlyLayerCaptureSmoke", source_text)
        self.assertIn("--renderer-ui-layer-capture-smoke", source_text)
        self.assertIn("phase158-ui-render-target-capture", source_text)
        self.assertIn("ui_layer_capture_policy=copy-active-ui-render-target-before-imgui-present", source_text)
        self.assertIn("ui_layer_capture_status=", source_text)
        self.assertIn("ui_layer_isolation_status=", source_text)
        self.assertIn("uiOnlyRenderTargetCaptureStatus", source_text)
        self.assertIn("uiOnlyRenderTargetCapturePath", source_text)

    def test_renderer_source_wires_phase159_ui_layer_pixel_compare(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("FrontendUiLayerPixelCompareRecord", source_text)
        self.assertIn("findLatestUiLayerCaptureBmpPathForTarget", source_text)
        self.assertIn("renderFrontendPolicyUiLayerPixelCompare", source_text)
        self.assertIn("writeFrontendUiLayerPixelCompareManifest", source_text)
        self.assertIn("runRendererUiLayerPixelCompareSmoke", source_text)
        self.assertIn("--renderer-ui-layer-pixel-compare-smoke", source_text)
        self.assertIn("phase159-ui-layer-pixel-compare", source_text)
        self.assertIn("ui_layer_pixel_compare_manifest=", source_text)
        self.assertIn("ui_layer_pixel_delta=", source_text)
        self.assertIn("ui_layer_capture_isolation=", source_text)
        self.assertIn("ui_layer_oracle_upgrade=dedicated-ui-target-or-vendor-replay-needed", source_text)
        self.assertIn("text_movie_sfx_status=title-loading-media-timing-reference-ready-audio-id-pending", source_text)

    def test_renderer_source_wires_phase162_media_asset_readiness(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("runRendererMediaAssetReadinessSmoke", source_text)
        self.assertIn("--renderer-media-asset-readiness-smoke", source_text)
        self.assertIn("phase162-media-asset-resolution", source_text)
        self.assertIn("formatFrontendScreenMediaAssetProbeCatalog", source_text)

    def test_renderer_sonic_hud_reference_smoke_reports_exact_policy_viewer(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-sonic-hud-reference-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=120,
        )

        self.assertIn("sward_su_ui_asset_renderer sonic hud reference viewer smoke ok", completed.stdout)
        self.assertIn("screen=SonicHudReconstruction:mode=phase137-ui_playscreen-policy", completed.stdout)
        self.assertIn("owner=CHudSonicStage:hook=sub_824D9308:project=ui_playscreen", completed.stdout)
        self.assertIn("scenes=13:drawable_layers=167", completed.stdout)
        self.assertIn("render_scene=so_ringenagy_gauge:local_scene=so_ringenagy_gauge:slot=energy_gauge:order=60:commands=40", completed.stdout)
        self.assertIn("render_scene=so_speed_gauge:local_scene=so_speed_gauge:slot=speed_gauge:order=70:commands=43", completed.stdout)
        self.assertIn("render_scene=add/u_info:local_scene=u_info:slot=prompt_strip:order=120:commands=5", completed.stdout)
        self.assertIn("viewer_overlay=compact-reference-status:no-template-card=1", completed.stdout)

    def test_renderer_reference_lanes_smoke_reports_no_proxy_card_viewers(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-reference-lanes-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=120,
        )

        self.assertIn("sward_su_ui_asset_renderer reference lanes smoke ok", completed.stdout)
        self.assertIn("mode=phase142-tracked-policy-playback", completed.stdout)
        self.assertIn("reference_policy_source=frontend_screen_reference", completed.stdout)
        self.assertIn("reference_lane=title-menu:screen=MainMenuComposite:layout=ui_mainmenu.yncp:scenes=3", completed.stdout)
        self.assertIn("reference_lane=loading:screen=LoadingComposite:layout=ui_loading.yncp:scenes=2", completed.stdout)
        self.assertIn("reference_lane=title-options:screen=TitleOptionsReference:layout=ui_mainmenu.yncp:scenes=2", completed.stdout)
        self.assertIn("reference_lane=pause:screen=PauseMenuReference:layout=ui_pause.yncp:scenes=8", completed.stdout)
        self.assertRegex(completed.stdout, r"runtime_alignment=pause:active_screen=pause:active_scenes=bg,bg_1,bg_1_select,bg_2,text_area,skill_select,arrow,skill_scroll_bar_bg:motion=pause target ready:frame=[1-9]\d*:cursor_owner=CHudPause/menu=.*status=.*transition=.*:transition=intro_medium->pause menu visual ready:input_lock=released:pause-ready")
        self.assertIn("alignment_lane=pause:source=ui_lab_live_state", completed.stdout)
        self.assertIn("material_semantics=loading:blend=source-over/additive:alpha=straight-alpha:color=packed-rgba-gradient:filter=csd-point-seam:offset=half-pixel:oracle=runtime CSD/UI layer preferred", completed.stdout)
        self.assertRegex(completed.stdout, r"reference_lane=title-menu:.*timeline_resolved=[1-9]\d*:.*sampled_tracks=[1-9]\d*")
        self.assertRegex(completed.stdout, r"reference_lane=pause:.*structural=[1-9]\d*:source_free=[1-9]\d*")
        self.assertRegex(completed.stdout, r"reference_scene=title-menu:mm_bg_usual:commands=[1-9]\d*")
        self.assertRegex(completed.stdout, r"reference_scene=loading:pda:commands=[1-9]\d*")
        self.assertIn("reference_scene=title-options:mm_contentsitem_select", completed.stdout)
        self.assertRegex(completed.stdout, r"reference_scene=pause:bg:commands=[1-9]\d*:.*structural=[1-9]\d*")
        self.assertRegex(completed.stdout, r"reference_scene=pause:text_area:commands=[1-9]\d*:.*structural=[1-9]\d*")
        self.assertRegex(completed.stdout, r"reference_scene=pause:bg_1:commands=[1-9]\d*")
        self.assertRegex(completed.stdout, r"reference_scene=pause:bg_1:.*timeline=Intro_Anim@[0-9]+/[0-9]+:sampled_tracks=[1-9]\d*")
        self.assertIn("reference_overlay=compact-reference-status:no-template-card=1", completed.stdout)

    def test_renderer_runtime_alignment_smoke_reads_live_state_when_available(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-runtime-alignment-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=120,
        )

        self.assertIn("sward_su_ui_asset_renderer runtime alignment smoke ok", completed.stdout)
        self.assertIn("mode=phase143-live-state-alignment", completed.stdout)
        self.assertIn("runtime_alignment_source=ui_lab_live_state", completed.stdout)
        self.assertIn("alignment_lane=title-menu:source=ui_lab_live_state", completed.stdout)
        self.assertIn("alignment_lane=loading:source=ui_lab_live_state", completed.stdout)
        self.assertIn("alignment_lane=title-options:source=ui_lab_live_state", completed.stdout)
        self.assertIn("alignment_lane=pause:source=ui_lab_live_state", completed.stdout)
        self.assertRegex(completed.stdout, r"runtime_alignment=title-menu:active_screen=title-menu:.*motion=title menu visual ready:frame=[1-9]\d*:cursor_owner=CTitleStateMenu/menu_cursor=[0-9]+")
        self.assertRegex(completed.stdout, r"runtime_alignment=loading:active_screen=loading:.*motion=.*loading.*:frame=[1-9]\d*:cursor_owner=LoadingDisplay/display_type=[0-9]+")
        self.assertRegex(completed.stdout, r"runtime_alignment=pause:active_screen=pause:.*motion=pause target ready:frame=[1-9]\d*:cursor_owner=CHudPause/menu=.*status=.*transition=.*")
        self.assertRegex(completed.stdout, r"live_state_path=title-menu:out/ui_lab_runtime_evidence/.*/title-menu/ui_lab_live_state.json")
        self.assertIn("alignment_field_status=title-menu:active_screen=live:active_scenes=policy:motion=live-route:frame=live-frame:cursor_owner=live-title-menu:transition=policy:input_lock=live-readiness", completed.stdout)

    def test_renderer_live_bridge_alignment_smoke_reports_direct_probe_or_snapshot_fallback(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-live-bridge-alignment-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=120,
        )

        self.assertIn("sward_su_ui_asset_renderer live bridge alignment smoke ok", completed.stdout)
        self.assertIn("mode=phase144-live-bridge-alignment", completed.stdout)
        self.assertRegex(completed.stdout, r"runtime_alignment_probe=(direct-live-bridge|snapshot-fallback)")
        self.assertRegex(completed.stdout, r"bridge_probe=title-menu:pipe=[^:]+:connected=[01]:fallback=(ui_lab_live_state|none)")
        self.assertIn("alignment_lane=title-menu:", completed.stdout)
        self.assertIn("runtime_alignment=title-menu:active_screen=title-menu", completed.stdout)

    def test_renderer_ui_oracle_smoke_reports_runtime_csd_oracle_or_snapshot_fallback(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-ui-oracle-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=120,
        )

        self.assertIn("sward_su_ui_asset_renderer ui oracle smoke ok", completed.stdout)
        self.assertIn("mode=phase145-ui-only-oracle", completed.stdout)
        self.assertRegex(completed.stdout, r"ui_oracle_probe=(direct-ui-oracle|state-fallback|snapshot-fallback)")
        self.assertRegex(completed.stdout, r"ui_oracle=title-menu:source=(ui_lab_live_bridge_ui_oracle|ui_lab_live_bridge_state|ui_lab_live_state):project=ui_title:scenes=[0-9]+:layers=[0-9]+:draw_list_status=")
        self.assertIn("active_scenes=", completed.stdout)

    def test_renderer_ui_oracle_playback_smoke_reports_runtime_driven_timeline_frames(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-ui-oracle-playback-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=120,
        )

        self.assertIn("sward_su_ui_asset_renderer ui oracle playback smoke ok", completed.stdout)
        self.assertIn("mode=phase146-ui-oracle-playback", completed.stdout)
        self.assertRegex(completed.stdout, r"ui_oracle_playback_probe=(direct-ui-oracle|state-fallback|snapshot-fallback)")
        self.assertRegex(completed.stdout, r"oracle_playback=title-menu:source=(ui_lab_live_bridge_ui_oracle|ui_lab_live_bridge_state|ui_lab_live_state):runtime_frame=[1-9]\d*:playback_clock=ui-oracle-runtime-frame:playback_frame=[0-9]+:active_motion=")
        self.assertRegex(completed.stdout, r"oracle_scene_playback=title-menu:mm_bg_usual:animation=DefaultAnim:frame=[0-9]+/120:timeline_frame_source=ui-oracle-mod-frame")
        self.assertRegex(completed.stdout, r"oracle_scene_playback=loading:pda:animation=Usual_Anim_3:frame=[0-9]+/240:timeline_frame_source=ui-oracle-mod-frame")

    def test_renderer_ui_drawable_oracle_smoke_reports_scene_draw_commands(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-ui-drawable-oracle-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer ui drawable oracle smoke ok", completed.stdout)
        self.assertIn("mode=phase147-ui-drawable-oracle", completed.stdout)
        self.assertIn("runtime_drawable_oracle_status=runtime-csd-tree-local-material", completed.stdout)
        self.assertIn("gpu_draw_list_status=pending", completed.stdout)
        self.assertIn("drawable_scene_source=ui-oracle-active-scenes", completed.stdout)
        self.assertRegex(completed.stdout, r"drawable_oracle=title-menu:source=(ui_lab_live_bridge_ui_oracle|ui_lab_live_bridge_state|ui_lab_live_state):active_project=ui_title:active_scenes=[0-9]+:drawable_scenes=[1-9]\d*:commands=[1-9]\d*")
        self.assertRegex(completed.stdout, r"drawable_oracle=loading:source=(ui_lab_live_bridge_ui_oracle|ui_lab_live_bridge_state|ui_lab_live_state):active_project=ui_loading:active_scenes=[0-9]+:drawable_scenes=[1-9]\d*:commands=[1-9]\d*")
        self.assertRegex(completed.stdout, r"drawable_oracle_scene=title-menu:mm_bg_usual:runtime_path=[^:]+:animation=DefaultAnim:frame=[0-9]+/120:commands=[1-9]\d*:sampled_tracks=[0-9]+:textures=[1-9]\d*:drawable_scene_source=ui-oracle-active-scenes")
        self.assertRegex(completed.stdout, r"drawable_oracle_scene=loading:pda:runtime_path=[^:]+:animation=Usual_Anim_3:frame=[0-9]+/240:commands=[1-9]\d*:sampled_tracks=[0-9]+:textures=[1-9]\d*:drawable_scene_source=ui-oracle-active-scenes")

    def test_renderer_ui_draw_list_triage_smoke_reports_runtime_rectangles(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-ui-draw-list-triage-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer ui draw-list triage smoke ok", completed.stdout)
        self.assertIn("mode=phase149-ui-draw-list-triage", completed.stdout)
        self.assertRegex(completed.stdout, r"ui_draw_list_probe=(direct-ui-draw-list|ui-oracle-fallback|missing)")
        self.assertIn("runtime_draw_list_source=ui-draw-list", completed.stdout)
        self.assertIn("material_triage=runtime-rectangles-vs-local-csd", completed.stdout)
        self.assertIn("backend_submit_status=pending", completed.stdout)
        self.assertRegex(completed.stdout, r"draw_list_triage=title-menu:source=(ui_lab_live_bridge_ui_draw_list|ui_lab_live_bridge_ui_oracle|missing):runtime_calls=[0-9]+:runtime_rects=[0-9]+:local_commands=[1-9]\d*:local_textures=[1-9]\d*:rect_match_candidates=[0-9]+:backend_submit_status=pending")
        self.assertRegex(completed.stdout, r"draw_list_triage=loading:source=(ui_lab_live_bridge_ui_draw_list|ui_lab_live_bridge_ui_oracle|missing):runtime_calls=[0-9]+:runtime_rects=[0-9]+:local_commands=[1-9]\d*:local_textures=[1-9]\d*:rect_match_candidates=[0-9]+:backend_submit_status=pending")
        self.assertRegex(completed.stdout, r"draw_list_scene=title-menu:mm_bg_usual:local_commands=[1-9]\d*:runtime_rects=[0-9]+:material_triage=runtime-rectangles-vs-local-csd")
        self.assertRegex(completed.stdout, r"draw_list_scene=loading:pda:local_commands=[1-9]\d*:runtime_rects=[0-9]+:material_triage=runtime-rectangles-vs-local-csd")

    def test_renderer_gpu_submit_triage_smoke_reports_backend_material_state(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-gpu-submit-triage-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer gpu submit triage smoke ok", completed.stdout)
        self.assertIn("mode=phase150-backend-submit-material-triage", completed.stdout)
        self.assertRegex(completed.stdout, r"gpu_submit_probe=(direct-ui-gpu-submit|ui-draw-list-fallback|missing)")
        self.assertIn("gpu_submit_source=ui-gpu-submit", completed.stdout)
        self.assertIn("material_triage=backend-submit-vs-runtime-rectangles", completed.stdout)
        self.assertIn("backend_submit_status=render-thread-material-submit", completed.stdout)
        self.assertRegex(completed.stdout, r"gpu_submit_triage=title-menu:source=(ui_lab_live_bridge_gpu_submit|ui_lab_live_bridge_ui_draw_list|missing):backend_submits=[0-9]+:textured_submits=[0-9]+:alpha_blend_submits=[0-9]+:draw_rects=[0-9]+:local_commands=[1-9]\d*:material_triage=backend-submit-vs-runtime-rectangles")
        self.assertRegex(completed.stdout, r"gpu_submit_triage=loading:source=(ui_lab_live_bridge_gpu_submit|ui_lab_live_bridge_ui_draw_list|missing):backend_submits=[0-9]+:textured_submits=[0-9]+:alpha_blend_submits=[0-9]+:draw_rects=[0-9]+:local_commands=[1-9]\d*:material_triage=backend-submit-vs-runtime-rectangles")

    def test_renderer_material_correlation_smoke_reports_named_backend_semantics(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-material-correlation-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer material correlation smoke ok", completed.stdout)
        self.assertIn("mode=phase151-material-correlation", completed.stdout)
        self.assertRegex(completed.stdout, r"material_correlation_probe=(direct-ui-material-correlation|gpu-submit-fallback|missing)")
        self.assertIn("material_correlation_source=ui-material-correlation", completed.stdout)
        self.assertIn("blend_semantics=runtime-submit-named", completed.stdout)
        self.assertIn("sampler_semantics=runtime-submit-named", completed.stdout)
        self.assertRegex(completed.stdout, r"material_correlation=title-menu:source=(ui_lab_live_bridge_material_correlation|ui_lab_live_bridge_gpu_submit|missing):pairs=[0-9]+:draw_calls=[0-9]+:backend_submits=[0-9]+:alpha_blend_pairs=[0-9]+:additive_pairs=[0-9]+:filter_linear_pairs=[0-9]+:filter_point_pairs=[0-9]+:local_commands=[1-9]\d*:raw_backend_command_status=[^\r\n]+")
        self.assertRegex(completed.stdout, r"material_correlation=loading:source=(ui_lab_live_bridge_material_correlation|ui_lab_live_bridge_gpu_submit|missing):pairs=[0-9]+:draw_calls=[0-9]+:backend_submits=[0-9]+:alpha_blend_pairs=[0-9]+:additive_pairs=[0-9]+:filter_linear_pairs=[0-9]+:filter_point_pairs=[0-9]+:local_commands=[1-9]\d*:raw_backend_command_status=[^\r\n]+")

    def test_renderer_backend_resolved_triage_smoke_reports_resolved_submit_state(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-backend-resolved-triage-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer backend resolved triage smoke ok", completed.stdout)
        self.assertIn("mode=phase152-backend-resolved-submit", completed.stdout)
        self.assertRegex(completed.stdout, r"backend_resolved_probe=(direct-ui-backend-resolved|material-correlation-fallback|missing)")
        self.assertIn("backend_resolved_source=ui-backend-resolved", completed.stdout)
        self.assertIn("material_correlation_backend_resolved=joined", completed.stdout)
        self.assertIn("resolved_pso_blend_framebuffer=runtime-backend", completed.stdout)
        self.assertRegex(completed.stdout, r"backend_resolved=title-menu:source=(ui_lab_live_bridge_backend_resolved|ui_lab_live_bridge_material_correlation|missing):backend_resolved_submits=[0-9]+:resolved_pipeline_submits=[0-9]+:blend_enabled_submits=[0-9]+:rt0_format_known=[0-9]+:framebuffer_known=[0-9]+:material_pairs=[0-9]+:local_commands=[1-9]\d*:resolved_backend_status=[^\r\n]+")
        self.assertRegex(completed.stdout, r"backend_resolved=loading:source=(ui_lab_live_bridge_backend_resolved|ui_lab_live_bridge_material_correlation|missing):backend_resolved_submits=[0-9]+:resolved_pipeline_submits=[0-9]+:blend_enabled_submits=[0-9]+:rt0_format_known=[0-9]+:framebuffer_known=[0-9]+:material_pairs=[0-9]+:local_commands=[1-9]\d*:resolved_backend_status=[^\r\n]+")

    def test_renderer_material_parity_hints_smoke_reports_backend_policy(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-material-parity-hints-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer material parity hints smoke ok", completed.stdout)
        self.assertIn("mode=phase153-backend-material-parity-hints", completed.stdout)
        self.assertRegex(completed.stdout, r"material_parity_probe=(direct-ui-backend-resolved|material-correlation-fallback|missing)")
        self.assertIn("material_parity_policy=backend-resolved-pso-blend-framebuffer", completed.stdout)
        self.assertIn("texture_view_sampler_gap=pending", completed.stdout)
        self.assertIn("text_movie_sfx_gap=pending", completed.stdout)
        self.assertRegex(completed.stdout, r"material_parity=title-menu:source=(ui_lab_live_bridge_backend_resolved|ui_lab_live_bridge_material_correlation|missing):backend_resolved_submits=[0-9]+:source_over=[0-9]+:additive=[0-9]+:opaque=[0-9]+:framebuffer_registered=[0-9]+:local_commands=[1-9]\d*:material_parity_status=[^\r\n]+")
        self.assertRegex(completed.stdout, r"material_parity=loading:source=(ui_lab_live_bridge_backend_resolved|ui_lab_live_bridge_material_correlation|missing):backend_resolved_submits=[0-9]+:source_over=[0-9]+:additive=[0-9]+:opaque=[0-9]+:framebuffer_registered=[0-9]+:local_commands=[1-9]\d*:material_parity_status=[^\r\n]+")

    def test_renderer_descriptor_semantics_smoke_reports_runtime_descriptor_policy(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-descriptor-semantics-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer descriptor semantics smoke ok", completed.stdout)
        self.assertIn("mode=phase154-texture-sampler-descriptor-semantics", completed.stdout)
        self.assertRegex(completed.stdout, r"descriptor_semantics_probe=(direct-ui-backend-resolved|material-correlation-fallback|missing)")
        self.assertIn("texture_sampler_policy=runtime-descriptor-state", completed.stdout)
        self.assertIn("vendor_descriptor_gap=pending-native-descriptor-dump", completed.stdout)
        self.assertIn("text_movie_sfx_gap=pending", completed.stdout)
        self.assertRegex(completed.stdout, r"descriptor_semantics=title-menu:source=(ui_lab_live_bridge_backend_resolved|ui_lab_live_bridge_material_correlation|missing):texture_descriptor_known=[0-9]+:sampler_descriptor_known=[0-9]+:linear=[0-9]+:point=[0-9]+:wrap=[0-9]+:clamp=[0-9]+:local_commands=[1-9]\d*:texture_view_sampler_status=[^\r\n]+")
        self.assertRegex(completed.stdout, r"descriptor_semantics=loading:source=(ui_lab_live_bridge_backend_resolved|ui_lab_live_bridge_material_correlation|missing):texture_descriptor_known=[0-9]+:sampler_descriptor_known=[0-9]+:linear=[0-9]+:point=[0-9]+:wrap=[0-9]+:clamp=[0-9]+:local_commands=[1-9]\d*:texture_view_sampler_status=[^\r\n]+")

    def test_renderer_vendor_resource_capture_smoke_reports_native_resource_policy(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-vendor-resource-capture-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer vendor resource capture smoke ok", completed.stdout)
        self.assertIn("mode=phase155-vendor-resource-capture", completed.stdout)
        self.assertRegex(completed.stdout, r"vendor_resource_probe=(direct-ui-backend-resolved|material-correlation-fallback|missing)")
        self.assertIn("vendor_resource_policy=native-rhi-resource-view-sampler", completed.stdout)
        self.assertIn("ui_only_layer_status=pending-runtime-ui-render-target-copy", completed.stdout)
        self.assertIn("native_command_gap=pending-full-vendor-command-buffer-dump", completed.stdout)
        self.assertRegex(completed.stdout, r"vendor_resource=title-menu:source=(ui_lab_live_bridge_backend_resolved|ui_lab_live_bridge_material_correlation|missing):texture_views=[0-9]+:sampler_views=[0-9]+:resource_pairs=[0-9]+:local_commands=[1-9]\d*:vendor_resource_status=[^\r\n]+")
        self.assertRegex(completed.stdout, r"vendor_resource=loading:source=(ui_lab_live_bridge_backend_resolved|ui_lab_live_bridge_material_correlation|missing):texture_views=[0-9]+:sampler_views=[0-9]+:resource_pairs=[0-9]+:local_commands=[1-9]\d*:vendor_resource_status=[^\r\n]+")

    def test_renderer_material_resource_view_parity_smoke_reports_resource_view_exactness(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-material-resource-view-parity-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer material resource-view parity smoke ok", completed.stdout)
        self.assertIn("mode=phase156-material-resource-view-parity", completed.stdout)
        self.assertRegex(completed.stdout, r"material_resource_view_probe=(direct-ui-backend-resolved|material-correlation-fallback|missing)")
        self.assertIn("material_parity_policy=vendor-resource-view-alpha-gamma-srgb", completed.stdout)
        self.assertIn("ui_only_capture_policy=copy-ui-render-target-before-present", completed.stdout)
        self.assertIn("ui_only_layer_status=pending-runtime-ui-render-target-copy", completed.stdout)
        self.assertRegex(completed.stdout, r"material_resource_view=title-menu:source=(ui_lab_live_bridge_backend_resolved|ui_lab_live_bridge_material_correlation|missing):resource_pairs=[0-9]+:srgb_candidates=[0-9]+:local_commands=[1-9]\d*:resource_view_exactness=[^:]+:premultiplied_alpha_status=[^:]+:gamma_srgb_status=[^\r\n]+")
        self.assertRegex(completed.stdout, r"material_resource_view=loading:source=(ui_lab_live_bridge_backend_resolved|ui_lab_live_bridge_material_correlation|missing):resource_pairs=[0-9]+:srgb_candidates=[0-9]+:local_commands=[1-9]\d*:resource_view_exactness=[^:]+:premultiplied_alpha_status=[^:]+:gamma_srgb_status=[^\r\n]+")

    def test_renderer_vendor_command_resource_dump_smoke_reports_raw_backend_dump(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-vendor-command-resource-dump-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer vendor command/resource dump smoke ok", completed.stdout)
        self.assertIn("mode=phase157-vendor-command-resource-dump", completed.stdout)
        self.assertRegex(completed.stdout, r"vendor_command_resource_probe=(direct-ui-vendor-command-capture|backend-resolved-fallback|missing)")
        self.assertIn("vendor_command_resource_dump_policy=raw-backend-command-plus-resource-view-dump", completed.stdout)
        self.assertIn("ui_only_layer_status=pending-runtime-ui-render-target-copy", completed.stdout)
        self.assertIn("vendor_command_replay_gap=pending-full-vendor-command-buffer-replay", completed.stdout)
        self.assertRegex(completed.stdout, r"vendor_command_resource=title-menu:source=(ui_lab_live_bridge_vendor_command_capture|ui_lab_live_bridge_backend_resolved|missing):raw_commands=[0-9]+:backend_submits=[0-9]+:resource_pairs=[0-9]+:texture_views=[0-9]+:sampler_views=[0-9]+:local_commands=[1-9]\d*:dump_status=[^\r\n]+")
        self.assertRegex(completed.stdout, r"vendor_command_resource=loading:source=(ui_lab_live_bridge_vendor_command_capture|ui_lab_live_bridge_backend_resolved|missing):raw_commands=[0-9]+:backend_submits=[0-9]+:resource_pairs=[0-9]+:texture_views=[0-9]+:sampler_views=[0-9]+:local_commands=[1-9]\d*:dump_status=[^\r\n]+")

    def test_renderer_ui_layer_capture_smoke_reports_render_target_readback(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-ui-layer-capture-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer UI layer capture smoke ok", completed.stdout)
        self.assertIn("mode=phase158-ui-render-target-capture", completed.stdout)
        self.assertRegex(completed.stdout, r"ui_layer_capture_probe=(direct-ui-layer-status|vendor-command-fallback|missing)")
        self.assertIn("ui_layer_capture_policy=copy-active-ui-render-target-before-imgui-present", completed.stdout)
        self.assertRegex(completed.stdout, r"ui_layer_capture=title-menu:source=(ui_lab_live_bridge_ui_layer_status|ui_lab_live_bridge_vendor_command_capture|missing):capture_status=[^:]+:isolation=[^:]+:width=[0-9]+:height=[0-9]+:local_commands=[1-9]\d*:ui_layer_capture_path=[^\r\n]*")
        self.assertRegex(completed.stdout, r"ui_layer_capture=loading:source=(ui_lab_live_bridge_ui_layer_status|ui_lab_live_bridge_vendor_command_capture|missing):capture_status=[^:]+:isolation=[^:]+:width=[0-9]+:height=[0-9]+:local_commands=[1-9]\d*:ui_layer_capture_path=[^\r\n]*")

    def test_renderer_ui_layer_pixel_compare_smoke_reports_visual_delta_or_missing_capture(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-ui-layer-pixel-compare-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer UI layer pixel compare smoke ok", completed.stdout)
        self.assertIn("mode=phase159-ui-layer-pixel-compare", completed.stdout)
        self.assertIn("ui_layer_pixel_compare_manifest=out/ui_layer_pixel_compare/phase159/ui_layer_pixel_compare_manifest.json", completed.stdout)
        self.assertIn("ui_layer_oracle_upgrade=dedicated-ui-target-or-vendor-replay-needed", completed.stdout)
        self.assertIn("text_movie_sfx_status=title-loading-media-timing-reference-ready-audio-id-pending", completed.stdout)
        self.assertRegex(completed.stdout, r"ui_layer_pixel_delta=title-menu:source=(ui-layer-capture-bmp|missing):native=(found|missing):mean_abs_rgb=[0-9.]+:max_abs_rgb=[0-9]+:local_commands=[1-9]\d*:ui_layer_capture_isolation=[^\r\n]+")
        self.assertRegex(completed.stdout, r"ui_layer_pixel_delta=loading:source=(ui-layer-capture-bmp|missing):native=(found|missing):mean_abs_rgb=[0-9.]+:max_abs_rgb=[0-9]+:local_commands=[1-9]\d*:ui_layer_capture_isolation=[^\r\n]+")

    def test_renderer_media_asset_readiness_smoke_reports_resolved_visual_media(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-media-asset-readiness-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=120,
        )

        self.assertIn("sward_su_ui_asset_renderer media asset readiness smoke ok", completed.stdout)
        self.assertIn("mode=phase162-media-asset-resolution", completed.stdout)
        self.assertIn("media_asset_status=title-menu:resolved=4:preview=1:playback_ready=3:decode_pending=1:audio_pending=2", completed.stdout)
        self.assertIn("media_asset_status=loading:resolved=4:preview=0:playback_ready=4:decode_pending=0:audio_pending=2", completed.stdout)
        self.assertIn("media_asset_probe=title-menu:title_loop_movie:kind=movie:asset=game/movie/evmo_title_loop.sfd:resolved=1", completed.stdout)
        self.assertIn("media_asset_probe=loading:loading_now_loading_copy:kind=text:asset=mat_load_en_001.dds:resolved=1", completed.stdout)

    def test_renderer_reference_policy_export_smoke_writes_clean_reusable_source(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--renderer-reference-policy-export-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer reference policy export smoke ok", completed.stdout)
        self.assertIn("mode=phase140-reusable-screen-policy", completed.stdout)
        self.assertIn("reference_policy_export_path=out/csd_runtime_exports/phase140/title_loading_options_pause_reference.hpp", completed.stdout)
        self.assertIn("reference_policy=title-menu:screen=MainMenuComposite:layout=ui_mainmenu.yncp:activation=title-menu-visible:transition=select_travel->title menu visual ready:input_lock=until:title-menu-visible:render_order=scene-stack:material_slots=4:sgfx_slots=4", completed.stdout)
        self.assertIn("reference_policy=loading:screen=LoadingComposite:layout=ui_loading.yncp:activation=loading-display-active:transition=pda_intro->loading display active:input_lock=until:loading-display-active:render_order=scene-stack:material_slots=4:sgfx_slots=4", completed.stdout)
        self.assertIn("reference_policy=title-options:screen=TitleOptionsReference:layout=ui_mainmenu.yncp:activation=title-options-ready:transition=select_travel->title options visual ready:input_lock=until:title-options-ready:render_order=scene-stack:material_slots=4:sgfx_slots=4", completed.stdout)
        self.assertIn("reference_policy=pause:screen=PauseMenuReference:layout=ui_pause.yncp:activation=pause-ready:transition=intro_medium->pause menu visual ready:input_lock=until:pause-ready:render_order=scene-stack:material_slots=4:sgfx_slots=4", completed.stdout)
        self.assertRegex(completed.stdout, r"reference_policy_scene=pause:bg:timeline=Intro_Anim@[0-9]+/[0-9]+:commands=[1-9]\d*:structural=[1-9]\d*:sampled_tracks=[0-9]+")
        self.assertRegex(completed.stdout, r"reference_policy_scene=pause:text_area:timeline=Usual_Anim@[0-9]+/[0-9]+:commands=[1-9]\d*:structural=[1-9]\d*:sampled_tracks=[0-9]+")
        self.assertIn("reference_policy_source_status=clean-readable-title-loading-options-pause-exported", completed.stdout)

    def test_renderer_viewer_compare_smoke_writes_offscreen_viewer_frames(self) -> None:
        exe = Path(os.environ.get("SWARD_SU_UI_RENDERER_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"renderer executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--viewer-render-compare-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
            timeout=180,
        )

        self.assertIn("sward_su_ui_asset_renderer viewer render compare smoke ok", completed.stdout)
        self.assertIn("viewer_compare_manifest=out/viewer_render_compare/phase139/viewer_render_compare_manifest.json", completed.stdout)
        self.assertIn("viewer_frame_path=title-menu:out/viewer_render_compare/phase139/title-menu_viewer.bmp", completed.stdout)
        self.assertIn("viewer_frame_path=loading:out/viewer_render_compare/phase139/loading_viewer.bmp", completed.stdout)
        self.assertIn("viewer_frame_path=title-options:out/viewer_render_compare/phase139/title-options_viewer.bmp", completed.stdout)
        self.assertIn("viewer_frame_path=pause:out/viewer_render_compare/phase139/pause_viewer.bmp", completed.stdout)
        self.assertIn("viewer_visual_delta=title-menu:native=found:sample_grid=64x36", completed.stdout)
        self.assertIn("viewer_visual_delta=loading:native=found:sample_grid=64x36", completed.stdout)
        self.assertRegex(completed.stdout, r"viewer_visual_delta=title-options:native=(found|missing):sample_grid=64x36")
        self.assertRegex(completed.stdout, r"viewer_visual_delta=pause:native=(found|missing):sample_grid=64x36")
        self.assertIn("viewer_render_source=title-menu:screen=MainMenuComposite:scenes=3", completed.stdout)
        self.assertIn("viewer_render_source=pause:screen=PauseMenuReference:scenes=8", completed.stdout)
        self.assertIn("operator_overlay=compact-reference-status:excluded_from_native_compare=1", completed.stdout)

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
        self.assertIn("screens=10", completed.stdout)
        self.assertIn("controls=5", completed.stdout)
        self.assertIn("first=TitleLoopReconstruction", completed.stdout)
        self.assertIn("last=SonicStageHud", completed.stdout)
        self.assertIn("label=1/10 TitleLoopReconstruction - Title Loop Reconstructed", completed.stdout)
        self.assertIn("screen=TitleLoopReconstruction:casts=4:contract=title_menu_reference.json", completed.stdout)
        self.assertIn("screen=MainMenuComposite:casts=3:contract=title_menu_reference.json", completed.stdout)
        self.assertIn("screen=TitleOptionsReference:casts=0:contract=title_menu_reference.json", completed.stdout)
        self.assertIn("screen=PauseMenuReference:casts=0:contract=pause_menu_reference.json", completed.stdout)
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
