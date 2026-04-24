#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_EXE = REPO_ROOT / "b" / "rr50" / "sward_ui_runtime_debug_workbench.exe"


class UiDebugWorkbenchCatalogTests(unittest.TestCase):
    def test_catalog_prints_group_contract_and_sample_host_summary(self) -> None:
        exe = Path(os.environ.get("SWARD_UI_DEBUG_WORKBENCH_EXE", DEFAULT_EXE))
        self.assertTrue(exe.exists(), f"missing workbench executable: {exe}")

        completed = subprocess.run(
            [str(exe), "--catalog"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn("Debug workbench catalog", completed.stdout)
        self.assertIn("Total hosts: 176", completed.stdout)
        self.assertIn("Groups: 11", completed.stdout)
        self.assertIn("Application / World Shell Hosts [high] hosts=64", completed.stdout)
        self.assertIn("Camera / Replay Hosts [medium] hosts=30", completed.stdout)
        self.assertIn("Support Substrate Hosts [medium] hosts=17", completed.stdout)
        self.assertIn("contract: camera_shell_reference.json", completed.stdout)
        self.assertIn("contract: audio_cue_support_reference.json", completed.stdout)
        self.assertIn("sample: Player3DBossCamera.cpp", completed.stdout)
        self.assertIn("sample: SoundController.cpp", completed.stdout)


if __name__ == "__main__":
    unittest.main()
