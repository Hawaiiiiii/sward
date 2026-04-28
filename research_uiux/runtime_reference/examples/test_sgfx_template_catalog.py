#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
CMAKE_FILE = REPO_ROOT / "research_uiux" / "runtime_reference" / "CMakeLists.txt"
HEADER = REPO_ROOT / "research_uiux" / "runtime_reference" / "include" / "sward" / "ui_runtime" / "sgfx_templates.hpp"
SOURCE = REPO_ROOT / "research_uiux" / "runtime_reference" / "src" / "sgfx_templates.cpp"
EXAMPLE = REPO_ROOT / "research_uiux" / "runtime_reference" / "examples" / "sgfx_template_catalog.cpp"
SONIC_HUD_HEADER = REPO_ROOT / "research_uiux" / "runtime_reference" / "include" / "sward" / "ui_runtime" / "sonic_hud_reference.hpp"
SONIC_HUD_SOURCE = REPO_ROOT / "research_uiux" / "runtime_reference" / "src" / "sonic_hud_reference.cpp"
SONIC_HUD_EXAMPLE = REPO_ROOT / "research_uiux" / "runtime_reference" / "examples" / "sonic_hud_reference_catalog.cpp"
REPORT = REPO_ROOT / "research_uiux" / "DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md"
README = REPO_ROOT / "research_uiux" / "runtime_reference" / "README.md"
DEFAULT_EXE = REPO_ROOT / "b" / "rr122" / "sward_sgfx_template_catalog.exe"
DEFAULT_SONIC_HUD_EXE = REPO_ROOT / "b" / "rr134" / "Release" / "sward_sonic_hud_reference_catalog.exe"


class SgfxTemplateCatalogTests(unittest.TestCase):
    def read(self, path: Path) -> str:
        return path.read_text(encoding="utf-8")

    def test_phase122_declares_repo_safe_sgfx_template_catalog(self) -> None:
        cmake = self.read(CMAKE_FILE)
        header = self.read(HEADER)
        source = self.read(SOURCE)
        example = self.read(EXAMPLE)

        self.assertIn("src/sgfx_templates.cpp", cmake)
        self.assertIn("add_executable(sward_sgfx_template_catalog", cmake)
        self.assertIn("examples/sgfx_template_catalog.cpp", cmake)

        for token in [
            "struct SgfxScreenTemplate",
            "struct SgfxEvidenceSource",
            "struct SgfxTimelineBand",
            "struct SgfxLayerRole",
            "struct SgfxAssetPolicy",
            "placeholderAssetFamily",
            "swappableSlots",
            "sgfxScreenTemplates",
            "findSgfxScreenTemplate",
            "formatSgfxTemplateCatalog",
            "formatSgfxTemplateDetail",
        ]:
            self.assertIn(token, header)

        for token in [
            "title-menu",
            "loading",
            "sonic-hud",
            "tutorial",
            "title_menu_reference.json",
            "loading_transition_reference.json",
            "sonic_stage_hud_reference.json",
            "SonicHudGuide.cpp",
            "native BMP",
            "live bridge",
            "JSONL",
            "SendInput",
            "Render Sonic",
            "local placeholders",
            "custom SGFX",
        ]:
            self.assertIn(token, source)

        for token in [
            "--catalog",
            "--template",
            "--phase122-smoke",
            "SGFX template catalog",
            "Phase 122 SGFX reusable templates",
        ]:
            self.assertIn(token, example)

    def test_phase122_templates_preserve_real_runtime_proven_events(self) -> None:
        source = self.read(SOURCE)

        for token in [
            "title-menu-visible",
            "title menu visual ready",
            "loading-display-active",
            "sonic-hud-ready",
            "tutorial-hud-owner-path-ready",
            "tutorial-target-ready",
            "tutorial-ready",
            "stage-target-csd-bound",
            "target-csd-project-made",
            "ui_playscreen",
            "ui_loading",
            "raw CHudSonicStage owner hook",
            "CCsdProject::Make traversal",
            "ui_title/ui_mainmenu Sonic title/menu CSD",
            "ui_loading Sonic loading CSD",
            "ui_playscreen Sonic HUD CSD tree",
            "SonicHudGuide/ui_playscreen prompt CSD",
        ]:
            self.assertIn(token, source)

    def test_phase122_docs_explain_debug_fork_remainder_and_sgfx_reuse_boundary(self) -> None:
        report = self.read(REPORT)
        readme = self.read(README)

        for token in [
            "Phase 122 SGFX Reusable Templates",
            "mostly harvested",
            "remaining debug-menu fork deltas",
            "Inspire/Movie",
            "renderer/shader metadata",
            "not a fake Sonic renderer",
            "SGFX template catalog",
            "render Sonic assets as local placeholders",
            "custom SGFX assets",
        ]:
            self.assertIn(token, report)

        for token in [
            "sward_sgfx_template_catalog.exe",
            "--phase122-smoke",
            "SGFX template catalog",
            "real-runtime evidence",
            "Sonic placeholder assets",
            "custom SGFX art",
        ]:
            self.assertIn(token, readme)

    def test_phase122_template_catalog_smoke_reports_reusable_screen_recipes(self) -> None:
        exe = Path(os.environ.get("SWARD_SGFX_TEMPLATE_CATALOG_EXE", DEFAULT_EXE))
        if not exe.exists():
            self.skipTest(f"SGFX template catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase122-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_sgfx_template_catalog phase122 smoke ok", completed.stdout)
        self.assertIn("templates=4", completed.stdout)
        self.assertIn("real_runtime_templates=4", completed.stdout)
        self.assertIn("title-menu:title_menu_reference.json:title-menu-visible", completed.stdout)
        self.assertIn("loading:loading_transition_reference.json:loading-display-active", completed.stdout)
        self.assertIn("sonic-hud:sonic_stage_hud_reference.json:sonic-hud-ready", completed.stdout)
        self.assertIn("tutorial:sonic_stage_hud_reference.json:tutorial-hud-owner-path-ready", completed.stdout)

    def test_phase137_declares_reusable_sonic_hud_reference_source(self) -> None:
        cmake = self.read(CMAKE_FILE)
        header = self.read(SONIC_HUD_HEADER)
        source = self.read(SONIC_HUD_SOURCE)
        example = self.read(SONIC_HUD_EXAMPLE)

        self.assertIn("src/sonic_hud_reference.cpp", cmake)
        self.assertIn("add_executable(sward_sonic_hud_reference_catalog", cmake)
        self.assertIn("examples/sonic_hud_reference_catalog.cpp", cmake)

        for token in [
            "struct SonicHudMaterialSlot",
            "struct SonicHudTimelineChannel",
            "struct SonicHudScenePolicy",
            "struct SonicHudTimelineSample",
            "sonicHudScenePolicies",
            "findSonicHudScenePolicy",
            "sampleSonicHudTimeline",
            "formatSonicHudReferenceCatalog",
            "formatSonicHudSceneDetail",
        ]:
            self.assertIn(token, header)

        for token in [
            "CHudSonicStage",
            "sub_824D9308",
            "ui_playscreen",
            "stage-hud-ready",
            "tutorial-hud-owner-path-ready",
            "so_speed_gauge",
            "so_ringenagy_gauge",
            "add/u_info",
            "DefaultAnim",
            "total_quantity",
            "Intro_Anim",
            "SGFX slot",
            "Sonic assets are local placeholders",
        ]:
            self.assertIn(token, source)

        for token in [
            "--catalog",
            "--scene",
            "--sample",
            "--phase137-smoke",
            "Phase 137 Sonic HUD reference",
        ]:
            self.assertIn(token, example)

    def test_phase137_sonic_hud_reference_smoke_reports_scene_policy_and_timeline(self) -> None:
        exe = Path(os.environ.get("SWARD_SONIC_HUD_REFERENCE_CATALOG_EXE", DEFAULT_SONIC_HUD_EXE))
        if not exe.exists():
            self.skipTest(f"Sonic HUD reference catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase137-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_sonic_hud_reference_catalog phase137 smoke ok", completed.stdout)
        self.assertIn("owner=CHudSonicStage:hook=sub_824D9308:project=ui_playscreen", completed.stdout)
        self.assertIn("scenes=13", completed.stdout)
        self.assertIn("drawable_layers=167", completed.stdout)
        self.assertIn("scene=so_speed_gauge:slot=speed_gauge:activation=stage-hud-ready:order=70:timeline=DefaultAnim", completed.stdout)
        self.assertIn("scene=so_ringenagy_gauge:slot=energy_gauge:activation=stage-hud-ready:order=60:timeline=total_quantity", completed.stdout)
        self.assertIn("scene=add/u_info:slot=prompt_strip:activation=tutorial-hud-owner-path-ready:order=120:timeline=Intro_Anim", completed.stdout)
        self.assertIn("timeline_sample=so_speed_gauge:DefaultAnim:frame=99/100:progress=0.990", completed.stdout)
        self.assertIn("material_slot=so_speed_gauge:speed_gauge:ui_ps1_gauge1.dds:placeholder=normal-sonic-hud", completed.stdout)


if __name__ == "__main__":
    unittest.main()
