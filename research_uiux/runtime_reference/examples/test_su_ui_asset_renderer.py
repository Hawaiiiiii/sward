#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
RENDERER_SOURCE = REPO_ROOT / "research_uiux" / "runtime_reference" / "examples" / "su_ui_asset_renderer.cpp"
CMAKE_FILE = REPO_ROOT / "research_uiux" / "runtime_reference" / "CMakeLists.txt"
DEFAULT_EXE = REPO_ROOT / "b" / "rr88" / "sward_su_ui_asset_renderer.exe"


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
        self.assertIn("LoadingTransition", source_text)
        self.assertIn("SonicStageHud", source_text)
        self.assertNotIn("kDebugWorkbenchHostEntries", source_text)

    def test_renderer_source_decodes_local_dds_textures_for_real_asset_blits(self) -> None:
        source_text = RENDERER_SOURCE.read_text(encoding="utf-8")
        self.assertIn("DdsTextureImage", source_text)
        self.assertIn("decodeDxt5Block", source_text)
        self.assertIn("loadDdsTextureImage", source_text)
        self.assertIn("mat_load_comon_001.dds", source_text)
        self.assertIn("ui_mm_parts1.dds", source_text)
        self.assertIn("ui_ps1_gauge1.dds", source_text)
        self.assertIn("--renderer-smoke", source_text)

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
        self.assertIn("screens=3", completed.stdout)
        self.assertIn("casts=3", completed.stdout)
        self.assertIn("textures=3", completed.stdout)
        self.assertIn("LoadingTransition:bg_1/arrow:mat_load_comon_001.dds:DXT5:1280x720", completed.stdout)
        self.assertIn("SonicTitleMenu:mm_bg_usual/black3:ui_mm_parts1.dds:DXT5:1280x640", completed.stdout)
        self.assertIn("SonicStageHud:so_speed_gauge/position_hd:ui_ps1_gauge1.dds:DXT5:256x128", completed.stdout)


if __name__ == "__main__":
    unittest.main()
