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
FRONTEND_SCREEN_HEADER = REPO_ROOT / "research_uiux" / "runtime_reference" / "include" / "sward" / "ui_runtime" / "frontend_screen_reference.hpp"
FRONTEND_SCREEN_SOURCE = REPO_ROOT / "research_uiux" / "runtime_reference" / "src" / "frontend_screen_reference.cpp"
FRONTEND_SCREEN_EXAMPLE = REPO_ROOT / "research_uiux" / "runtime_reference" / "examples" / "frontend_screen_reference_catalog.cpp"
FRONTEND_CONTROLLERS_HEADER = REPO_ROOT / "research_uiux" / "runtime_reference" / "include" / "sward" / "ui_runtime" / "frontend_screen_controllers.hpp"
FRONTEND_CONTROLLERS_SOURCE = REPO_ROOT / "research_uiux" / "runtime_reference" / "src" / "frontend_screen_controllers.cpp"
FRONTEND_CONTROLLERS_EXAMPLE = REPO_ROOT / "research_uiux" / "runtime_reference" / "examples" / "frontend_screen_controller_catalog.cpp"
REPORT = REPO_ROOT / "research_uiux" / "DEBUG_MENU_FORK_HARVEST_AND_LIVE_BRIDGE.md"
README = REPO_ROOT / "research_uiux" / "runtime_reference" / "README.md"
DEFAULT_EXE = REPO_ROOT / "b" / "rr122" / "sward_sgfx_template_catalog.exe"
DEFAULT_SONIC_HUD_EXE = REPO_ROOT / "b" / "rr134" / "Release" / "sward_sonic_hud_reference_catalog.exe"
DEFAULT_FRONTEND_SCREEN_EXE = REPO_ROOT / "b" / "rr134" / "Release" / "sward_frontend_screen_reference_catalog.exe"
DEFAULT_FRONTEND_CONTROLLER_EXE = REPO_ROOT / "b" / "rr134" / "Release" / "sward_frontend_screen_controller_catalog.exe"


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

    def test_phase141_declares_reusable_frontend_screen_reference_source(self) -> None:
        cmake = self.read(CMAKE_FILE)
        self.assertTrue(FRONTEND_SCREEN_HEADER.exists(), "Phase 141 frontend screen reference header is missing")
        self.assertTrue(FRONTEND_SCREEN_SOURCE.exists(), "Phase 141 frontend screen reference source is missing")
        self.assertTrue(FRONTEND_SCREEN_EXAMPLE.exists(), "Phase 141 frontend screen reference example is missing")
        header = self.read(FRONTEND_SCREEN_HEADER)
        source = self.read(FRONTEND_SCREEN_SOURCE)
        example = self.read(FRONTEND_SCREEN_EXAMPLE)

        self.assertIn("src/frontend_screen_reference.cpp", cmake)
        self.assertIn("add_executable(sward_frontend_screen_reference_catalog", cmake)
        self.assertIn("examples/frontend_screen_reference_catalog.cpp", cmake)

        for token in [
            "struct FrontendScreenMaterialSlot",
            "struct FrontendScreenMaterialSemantics",
            "struct FrontendScreenTimelineChannel",
            "struct FrontendScreenScenePolicy",
            "struct FrontendScreenPolicy",
            "struct FrontendRuntimeAlignment",
            "struct FrontendScreenTimelineSample",
            "frontendScreenPolicies",
            "findFrontendScreenPolicy",
            "findFrontendScreenScenePolicy",
            "defaultFrontendRuntimeAlignment",
            "sampleFrontendScreenTimeline",
            "formatFrontendScreenReferenceCatalog",
            "formatFrontendScreenReferenceDetail",
            "formatFrontendRuntimeAlignment",
        ]:
            self.assertIn(token, header)

        for token in [
            "MainMenuComposite",
            "LoadingComposite",
            "TitleOptionsReference",
            "PauseMenuReference",
            "ui_mainmenu.yncp",
            "ui_loading.yncp",
            "ui_pause.yncp",
            "title-menu-visible",
            "loading-display-active",
            "title-options-ready",
            "pause-ready",
            "select_travel",
            "pda_intro",
            "intro_medium",
            "input lock",
            "source-free structural",
            "runtime CSD/UI layer preferred",
            "blend=source-over/additive",
            "sampler=csd-point-seam",
            "transitionBand",
            "inputLockState",
            "SGFX slot",
            "Sonic assets are local placeholders",
        ]:
            self.assertIn(token, source)

        for token in [
            "--catalog",
            "--screen",
            "--scene",
            "--sample",
            "--phase141-smoke",
            "Phase 141 frontend screen reference",
        ]:
            self.assertIn(token, example)

    def test_phase141_frontend_screen_reference_smoke_reports_policy_and_timeline(self) -> None:
        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_REFERENCE_CATALOG_EXE", DEFAULT_FRONTEND_SCREEN_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen reference catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase141-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_reference_catalog phase141 smoke ok", completed.stdout)
        self.assertIn("screens=4", completed.stdout)
        self.assertIn("screen=title-menu:name=MainMenuComposite:layout=ui_mainmenu.yncp:activation=title-menu-visible:transition=select_travel->title menu visual ready:input_lock=until:title-menu-visible:scenes=3", completed.stdout)
        self.assertIn("screen=loading:name=LoadingComposite:layout=ui_loading.yncp:activation=loading-display-active:transition=pda_intro->loading display active:input_lock=until:loading-display-active:scenes=2", completed.stdout)
        self.assertIn("screen=title-options:name=TitleOptionsReference:layout=ui_mainmenu.yncp:activation=title-options-ready:transition=select_travel->title options visual ready:input_lock=until:title-options-ready:scenes=2", completed.stdout)
        self.assertIn("screen=pause:name=PauseMenuReference:layout=ui_pause.yncp:activation=pause-ready:transition=intro_medium->pause menu visual ready:input_lock=until:pause-ready:scenes=8", completed.stdout)
        self.assertIn("scene=pause/bg:slot=pause_backdrop:activation=pause-ready:order=10:timeline=Intro_Anim:commands=1:structural=1:source_free=1", completed.stdout)
        self.assertIn("scene=pause/text_area:slot=pause_text_area:activation=pause-ready:order=50:timeline=Usual_Anim:commands=3:structural=3:source_free=3", completed.stdout)
        self.assertIn("material_semantics=pause:blend=source-over/additive:alpha=straight-alpha:color=packed-rgba-gradient:filter=csd-point-seam:offset=half-pixel:oracle=runtime CSD/UI layer preferred", completed.stdout)
        self.assertIn("runtime_alignment=pause:active_screen=pause:active_scenes=bg,bg_1,bg_1_select,bg_2,text_area,skill_select,arrow,skill_scroll_bar_bg:motion=intro_medium:frame=15:cursor_owner=CHudPause:transition=intro_medium->pause menu visual ready:input_lock=until:pause-ready", completed.stdout)
        self.assertIn("timeline_sample=pause/text_area:Usual_Anim:frame=50/200:progress=0.250", completed.stdout)
        self.assertIn("material_slot=title-menu:backdrop:ui_mm_base.dds:placeholder=frontend-title-menu", completed.stdout)

    def test_phase161_declares_title_loading_media_timing_reference(self) -> None:
        header = self.read(FRONTEND_SCREEN_HEADER)
        source = self.read(FRONTEND_SCREEN_SOURCE)
        example = self.read(FRONTEND_SCREEN_EXAMPLE)

        for token in [
            "struct FrontendScreenMediaCue",
            "mediaCues",
            "formatFrontendScreenMediaTimingCatalog",
            "formatFrontendScreenMediaCueDetail",
            "frontendScreenMediaCueCounts",
        ]:
            self.assertIn(token, header)

        for token in [
            "title_loop_movie",
            "evmo_title_loop.sfd",
            "title_press_start_to_menu_fade",
            "title_prompt_glyphs",
            "title_press_start_accept_sfx",
            "loading_pda_intro_fade",
            "loading_now_loading_copy",
            "loading_controller_glyphs",
            "loading_display_open_sfx",
            "runtime-ui-layer-capture + CSD timeline",
            "audio-id-pending",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase161-media-smoke", example)

    def test_phase161_frontend_media_timing_smoke_reports_title_loading_cues(self) -> None:
        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_REFERENCE_CATALOG_EXE", DEFAULT_FRONTEND_SCREEN_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen reference catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase161-media-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_reference_catalog phase161 media timing smoke ok", completed.stdout)
        self.assertIn("media_timing_status=title-menu:movie=1:text=1:glyph=1:fade=1:sfx=2:visual_proven=4:audio_pending=2", completed.stdout)
        self.assertIn("media_timing_status=loading:movie=0:text=1:glyph=1:fade=2:sfx=2:visual_proven=4:audio_pending=2", completed.stdout)
        self.assertIn("media_cue=title-menu:title_loop_movie:kind=movie:slot=title_backdrop_movie:asset=game/movie/evmo_title_loop.sfd:start=0.000:duration=0.333:source=runtime-ui-layer-capture + CSD timeline:status=visual-policy-proven", completed.stdout)
        self.assertIn("media_cue=title-menu:title_press_start_accept_sfx:kind=sfx:slot=confirm_audio:asset=host-title-confirm-sfx:start=0.333:duration=0.000:source=title-menu-visible latch:status=audio-id-pending", completed.stdout)
        self.assertIn("media_cue=loading:loading_now_loading_copy:kind=text:slot=loading_copy:asset=mat_load_en_001.dds:start=1.250:duration=3.000:source=runtime-ui-layer-capture + CSD timeline:status=visual-policy-proven", completed.stdout)
        self.assertIn("media_cue=loading:loading_display_open_sfx:kind=sfx:slot=loading_open_audio:asset=host-loading-open-sfx:start=0.000:duration=0.000:source=loading-display-active latch:status=audio-id-pending", completed.stdout)

    def test_phase162_declares_media_asset_resolution_for_title_loading(self) -> None:
        header = self.read(FRONTEND_SCREEN_HEADER)
        source = self.read(FRONTEND_SCREEN_SOURCE)
        example = self.read(FRONTEND_SCREEN_EXAMPLE)

        for token in [
            "struct FrontendScreenMediaAssetProbe",
            "FrontendScreenMediaAssetProbeCounts",
            "frontendScreenMediaAssetProbes",
            "formatFrontendScreenMediaAssetProbeCatalog",
            "formatFrontendScreenMediaAssetProbeDetail",
        ]:
            self.assertIn(token, header)

        for token in [
            "Unleashed Recomp - Windows (Complete Installation) 1.0.3",
            "extracted_assets/runtime_previews/title/evmo_title_loop_0001.png",
            "extracted_assets/ui_frontend_archives/MainMenu/ui_mm_contentstext.dds",
            "extracted_assets/ui_frontend_archives/Loading_English/mat_load_en_001.dds",
            "sfd-found-preview-frame-ready-movie-decode-pending",
            "dds-material-ready",
            "audio-id-pending",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase162-media-asset-smoke", example)

    def test_phase162_media_asset_smoke_resolves_local_visual_placeholders(self) -> None:
        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_REFERENCE_CATALOG_EXE", DEFAULT_FRONTEND_SCREEN_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen reference catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase162-media-asset-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_reference_catalog phase162 media asset smoke ok", completed.stdout)
        self.assertIn("media_asset_status=title-menu:resolved=4:preview=1:playback_ready=3:decode_pending=1:audio_pending=2", completed.stdout)
        self.assertIn("media_asset_status=loading:resolved=4:preview=0:playback_ready=4:decode_pending=0:audio_pending=2", completed.stdout)
        self.assertIn("media_asset_probe=title-menu:title_loop_movie:kind=movie:asset=game/movie/evmo_title_loop.sfd:resolved=1:resolved_path=Unleashed Recomp - Windows (Complete Installation) 1.0.3/game/movie/evmo_title_loop.sfd:preview=extracted_assets/runtime_previews/title/evmo_title_loop_0001.png:playback=sfd-found-preview-frame-ready-movie-decode-pending", completed.stdout)
        self.assertIn("media_asset_probe=title-menu:title_menu_copy:kind=text:asset=ui_mm_contentstext.dds:resolved=1:resolved_path=extracted_assets/ui_frontend_archives/MainMenu/ui_mm_contentstext.dds:preview=missing:playback=dds-material-ready", completed.stdout)
        self.assertIn("media_asset_probe=loading:loading_now_loading_copy:kind=text:asset=mat_load_en_001.dds:resolved=1:resolved_path=extracted_assets/ui_frontend_archives/Loading_English/mat_load_en_001.dds:preview=missing:playback=dds-material-ready", completed.stdout)
        self.assertIn("media_asset_probe=title-menu:title_press_start_accept_sfx:kind=sfx:asset=host-title-confirm-sfx:resolved=0:resolved_path=missing:preview=missing:playback=audio-id-pending", completed.stdout)

    def test_phase163_declares_native_frontend_screen_controllers(self) -> None:
        cmake = self.read(CMAKE_FILE)
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)

        self.assertIn("src/frontend_screen_controllers.cpp", cmake)
        self.assertIn("add_executable(sward_frontend_screen_controller_catalog", cmake)
        self.assertIn("examples/frontend_screen_controller_catalog.cpp", cmake)

        for token in [
            "class TitleMenuController",
            "class LoadingScreenController",
            "class OptionsMenuController",
            "class PauseMenuController",
            "struct FrontendControllerFrame",
            "enum class FrontendControllerInput",
            "runFrontendControllerSmokeSequence",
            "formatFrontendControllerCatalog",
            "formatFrontendControllerFrame",
        ]:
            self.assertIn(token, header)

        for token in [
            "NEW_FILE",
            "CONTINUE",
            "SETTINGS",
            "DLC",
            "EXIT",
            "title_press_start_accept_sfx",
            "title_cursor_move_sfx",
            "loading_display_open_sfx",
            "loading_display_close_sfx",
            "pause_display_open_sfx",
            "sonic-day-hud-next",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase163-smoke", example)

    def test_phase163_frontend_controller_smoke_reports_frame_by_frame_state(self) -> None:
        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase163-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase163 smoke ok", completed.stdout)
        self.assertIn("controller_catalog=frontend-native-screen-controllers:count=4:policy_source=frontend_screen_reference:sonic_day_hud_next=1", completed.stdout)
        self.assertIn("controller_frame=TitleMenuController:screen=title-menu:frame=0:state=press-start-wait:motion=title_loop:input_locked=1:cursor=1/CONTINUE:sfx=none:next=none:scenes=mm_bg_usual", completed.stdout)
        self.assertIn("controller_frame=TitleMenuController:screen=title-menu:frame=20:state=menu-ready:motion=select_travel:input_locked=0:cursor=1/CONTINUE:sfx=title_press_start_accept_sfx:next=none:scenes=mm_bg_usual,mm_donut_move,mm_contentsitem_select", completed.stdout)
        self.assertIn("controller_frame=TitleMenuController:screen=title-menu:frame=21:state=menu-ready:motion=cursor_move:input_locked=0:cursor=2/SETTINGS:sfx=title_cursor_move_sfx:next=title-options:scenes=mm_bg_usual,mm_donut_move,mm_contentsitem_select", completed.stdout)
        self.assertIn("controller_frame=LoadingScreenController:screen=loading:frame=75:state=loading-visible:motion=pda_intro:input_locked=1:cursor=0/none:sfx=loading_display_open_sfx:next=sonic-day-hud-next:scenes=pda,pda_txt", completed.stdout)
        self.assertIn("controller_frame=OptionsMenuController:screen=title-options:frame=15:state=options-ready:motion=select_travel:input_locked=0:cursor=0/SOUND:sfx=title_cursor_move_sfx:next=title-menu:scenes=mm_bg_usual,mm_contentsitem_select", completed.stdout)
        self.assertIn("controller_frame=PauseMenuController:screen=pause:frame=15:state=pause-ready:motion=intro_medium:input_locked=0:cursor=0/RESUME:sfx=pause_display_open_sfx:next=sonic-day-hud-next:scenes=bg,bg_1,bg_1_select,bg_2,text_area,skill_select,arrow,skill_scroll_bar_bg", completed.stdout)


if __name__ == "__main__":
    unittest.main()
