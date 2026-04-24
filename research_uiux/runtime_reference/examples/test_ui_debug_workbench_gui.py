#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
GUI_SOURCE = REPO_ROOT / "research_uiux" / "runtime_reference" / "examples" / "ui_debug_workbench_gui.cpp"
CMAKE_FILE = REPO_ROOT / "research_uiux" / "runtime_reference" / "CMakeLists.txt"
DEFAULT_EXE = REPO_ROOT / "b" / "rr51" / "sward_ui_runtime_debug_gui.exe"


class UiDebugWorkbenchGuiTests(unittest.TestCase):
    def test_gui_target_is_declared_as_native_windows_executable(self) -> None:
        cmake_text = CMAKE_FILE.read_text(encoding="utf-8")
        self.assertIn("add_executable(sward_ui_runtime_debug_gui WIN32", cmake_text)
        self.assertIn("examples/ui_debug_workbench_gui.cpp", cmake_text)
        self.assertIn("user32", cmake_text)
        self.assertIn("gdi32", cmake_text)

    def test_gui_source_uses_workbench_data_and_runtime_contracts(self) -> None:
        source_text = GUI_SOURCE.read_text(encoding="utf-8")
        self.assertIn("int WINAPI WinMain", source_text)
        self.assertIn("kDebugWorkbenchHostEntries", source_text)
        self.assertIn("ScreenRuntime", source_text)
        self.assertIn("CreateWindowExA", source_text)
        self.assertIn("LISTBOX", source_text)
        self.assertIn("Run Host", source_text)
        self.assertIn("--smoke", source_text)

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


if __name__ == "__main__":
    unittest.main()
