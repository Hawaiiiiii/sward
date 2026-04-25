#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
RENDERER_SOURCE = REPO_ROOT / "research_uiux" / "runtime_reference" / "examples" / "su_ui_asset_renderer.cpp"
CMAKE_FILE = REPO_ROOT / "research_uiux" / "runtime_reference" / "CMakeLists.txt"
DEFAULT_EXE = REPO_ROOT / "b" / "rr91" / "sward_su_ui_asset_renderer.exe"


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
        self.assertIn("screens=6", completed.stdout)
        self.assertIn("casts=8", completed.stdout)
        self.assertIn("textures=8", completed.stdout)
        self.assertIn("full_screen_casts=1", completed.stdout)
        self.assertIn("LoadingComposite:load_composite/full_screen:mat_load_comon_001.dds:DXT5:1280x720:dst=0,0,1280x720", completed.stdout)
        self.assertNotIn("LoadingTransition:bg_1/arrow", completed.stdout)
        self.assertIn("MainMenuComposite:mm_bg/base_sheet:ui_mm_base.dds:DXT5:1280x720:dst=0,0,1280x720", completed.stdout)
        self.assertIn("SonicTitleMenu:mm_bg_usual/black3:ui_mm_parts1.dds:DXT5:1280x640", completed.stdout)
        self.assertIn("TitleLogoSheet:title/logo_en_001:mat_title_en_001.dds:DXT5:256x512", completed.stdout)
        self.assertIn("SonicStageHud:so_speed_gauge/position_hd:ui_ps1_gauge1.dds:DXT5:256x128", completed.stdout)

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
        self.assertIn("screens=6", completed.stdout)
        self.assertIn("controls=5", completed.stdout)
        self.assertIn("first=VisualAtlasGallery", completed.stdout)
        self.assertIn("last=SonicStageHud", completed.stdout)
        self.assertIn("label=1/6 VisualAtlasGallery - Visual Atlas Gallery", completed.stdout)
        self.assertIn("atlas 1/22 actioncommon__ui_gate.png", completed.stdout)
        self.assertIn("screen=MainMenuComposite:casts=3:contract=title_menu_reference.json", completed.stdout)

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
