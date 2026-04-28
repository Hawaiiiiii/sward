#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
RENDERER_SOURCE = REPO_ROOT / "research_uiux" / "runtime_reference" / "examples" / "su_ui_asset_renderer.cpp"
CMAKE_FILE = REPO_ROOT / "research_uiux" / "runtime_reference" / "CMakeLists.txt"
DEFAULT_EXE = REPO_ROOT / "b" / "rr123" / "sward_su_ui_asset_renderer.exe"


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
