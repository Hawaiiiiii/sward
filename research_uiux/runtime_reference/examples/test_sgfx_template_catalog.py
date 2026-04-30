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

    def test_phase164_declares_sonic_day_hud_controller(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)

        for token in [
            "class SonicDayHudController",
            "runSonicDayHudControllerSmokeSequence",
            "formatSonicDayHudControllerSmokeSequence",
            "FrontendControllerInput::StageReady",
            "FrontendControllerInput::TutorialReady",
            "FrontendControllerInput::RingPickup",
        ]:
            self.assertIn(token, header)

        for token in [
            "sonic_hud_reference.hpp",
            "CHudSonicStage",
            "sub_824D9308",
            "ui_playscreen",
            "sonic-hud-ready",
            "stage-hud-ready",
            "tutorial-hud-owner-path-ready",
            "so_speed_gauge",
            "so_ringenagy_gauge",
            "ring_get",
            "sonic_ring_pickup_sfx",
            "SonicDayHudController",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase164-sonic-hud-smoke", example)

    def test_phase164_sonic_day_hud_controller_smoke_reports_runtime_scene_state(self) -> None:
        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase164-sonic-hud-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase164 sonic hud smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_controller=owner=CHudSonicStage:hook=sub_824D9308:project=ui_playscreen:scenes=13:runtime_layers=209:drawable_layers=167:policy_source=sonic_hud_reference", completed.stdout)
        self.assertIn("controller_frame=SonicDayHudController:screen=sonic-day-hud:frame=0:state=hud-bootstrap:motion=owner-wait:input_locked=1:cursor=0/none:sfx=none:next=none:scenes=", completed.stdout)
        self.assertIn("controller_frame=SonicDayHudController:screen=sonic-day-hud:frame=99:state=hud-ready:motion=DefaultAnim:input_locked=0:cursor=0/none:sfx=none:next=none:scenes=exp_count,gauge_frame,player_count,ring_count,ring_get,score_count,so_ringenagy_gauge,so_speed_gauge,time_count,add/medal_get_m,add/medal_get_s,add/speed_count", completed.stdout)
        self.assertIn("controller_frame=SonicDayHudController:screen=sonic-day-hud:frame=20:state=tutorial-ready:motion=Intro_Anim:input_locked=0:cursor=0/none:sfx=tutorial_prompt_open_sfx:next=none:scenes=exp_count,gauge_frame,player_count,ring_count,ring_get,score_count,so_ringenagy_gauge,so_speed_gauge,time_count,add/medal_get_m,add/medal_get_s,add/speed_count,add/u_info", completed.stdout)
        self.assertIn("controller_frame=SonicDayHudController:screen=sonic-day-hud:frame=60:state=ring-feedback:motion=Egg_Shackle:input_locked=0:cursor=0/none:sfx=sonic_ring_pickup_sfx:next=none:scenes=ring_get", completed.stdout)

    def test_phase165_declares_sonic_day_hud_gameplay_state_model(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)

        for token in [
            "struct SonicDayHudGameplayState",
            "struct SonicDayHudValueProvenance",
            "ringCount",
            "score",
            "elapsedFrames",
            "speedKmh",
            "boostGauge",
            "ringEnergyGauge",
            "lifeCount",
            "tutorialPromptId",
            "tutorialVisible",
            "routeEvent",
            "setGameplayState",
            "applyRingPickup",
            "openTutorialPrompt",
            "dismissTutorialPrompt",
            "formatSonicDayHudGameplayState",
        ]:
            self.assertIn(token, header)

        for token in [
            "ui_playscreen/ring_count",
            "ui_playscreen/score_count",
            "ui_playscreen/time_count",
            "ui_playscreen/so_speed_gauge",
            "ui_playscreen/so_ringenagy_gauge",
            "ui_playscreen/player_count",
            "ui_playscreen/add/u_info",
            "host/live-bridge supplied value",
            "live-bridge-value-port-pending",
            "audio-id-pending",
            "tutorial_prompt_close_sfx",
            "stage-route-hook=CGameModeStage::ExitLoading",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase165-sonic-hud-state-smoke", example)

    def test_phase165_sonic_day_hud_state_smoke_reports_gameplay_value_transitions(self) -> None:
        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase165-sonic-hud-state-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase165 sonic hud state smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_state_model=fields=ring,score,time,speed,boost,energy,lives,tutorial,route:layout=ui_playscreen:controller=SonicDayHudController:value_source=host/live-bridge supplied value:memory_binding=live-bridge-value-port-pending", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=stage-ready:rings=000:score=000000000:time=00:00:00:speed=000:boost=0.000:energy=1.000:lives=3:tutorial=none:hidden:route=stage-hud-ready:sfx=none:sfx_id=audio-id-pending:provenance=ring:ui_playscreen/ring_count,score:ui_playscreen/score_count,time:ui_playscreen/time_count,speed:ui_playscreen/so_speed_gauge,boost:ui_playscreen/so_speed_gauge,energy:ui_playscreen/so_ringenagy_gauge,lives:ui_playscreen/player_count,tutorial:ui_playscreen/add/u_info,route:stage-route-hook=CGameModeStage::ExitLoading,value_source:host/live-bridge supplied value", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=value-tick:rings=000:score=000001250:time=00:05:20:speed=186:boost=0.650:energy=0.720:lives=3:tutorial=none:hidden:route=stage-hud-ready:sfx=none:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=ring-pickup:rings=001:score=000001350:time=00:05:20:speed=186:boost=0.650:energy=0.720:lives=3:tutorial=none:hidden:route=stage-hud-ready:sfx=sonic_ring_pickup_sfx:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=tutorial-open:rings=001:score=000001350:time=00:05:20:speed=186:boost=0.650:energy=0.720:lives=3:tutorial=boost_prompt:visible:route=tutorial-hud-owner-path-ready:sfx=tutorial_prompt_open_sfx:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=tutorial-dismiss:rings=001:score=000001350:time=00:05:20:speed=186:boost=0.650:energy=0.720:lives=3:tutorial=boost_prompt:hidden:route=stage-hud-ready:sfx=tutorial_prompt_close_sfx:sfx_id=audio-id-pending", completed.stdout)

    def test_phase166_declares_sonic_day_hud_runtime_binding_model(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)

        for token in [
            "struct SonicDayHudRuntimeValueBinding",
            "struct SonicDayHudRuntimeBindingSnapshot",
            "ringCountBinding",
            "scoreBinding",
            "elapsedFramesBinding",
            "speedKmhBinding",
            "boostGaugeBinding",
            "ringEnergyGaugeBinding",
            "lifeCountBinding",
            "tutorialPromptBinding",
            "sonicRingPickupSfxId",
            "tutorialPromptOpenSfxId",
            "applyRuntimeBinding",
            "formatSonicDayHudRuntimeBinding",
            "formatSonicDayHudRuntimeBindingSmokeSequence",
        ]:
            self.assertIn(token, header)

        for token in [
            "typedInspectors.sonicHud.gameplayValues",
            "SWA::CGameDocument::m_pMember->m_ScoreInfo.EnemyScore+TrickScore",
            "pending-runtime-field",
            "audio-id-pending",
            "sys_actstg_pausewinopen",
            "sys_actstg_pausecursor",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase166-sonic-hud-runtime-binding-smoke", example)

    def test_phase166_sonic_day_hud_runtime_binding_smoke_reports_bound_and_pending_fields(self) -> None:
        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase166-sonic-hud-runtime-binding-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase166 sonic hud runtime binding smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_binding=source=typedInspectors.sonicHud.gameplayValues:score=known:SWA::CGameDocument::m_pMember->m_ScoreInfo.EnemyScore+TrickScore:ring=pending-runtime-field:timer=pending-runtime-field:speed=pending-runtime-field:boost=pending-runtime-field:energy=pending-runtime-field:lives=pending-runtime-field:tutorial=pending-runtime-field", completed.stdout)
        self.assertIn("sfx=sonic_ring_pickup:audio-id-pending,tutorial_prompt_open:audio-id-pending,pause_open:sys_actstg_pausewinopen,pause_cursor:sys_actstg_pausecursor", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=runtime-bound:rings=000:score=000001250:time=00:00:00:speed=000:boost=0.000:energy=1.000:lives=3:tutorial=none:hidden:route=stage-hud-ready:sfx=none:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("value_source:typedInspectors.sonicHud.gameplayValues", completed.stdout)

    def test_phase167_sonic_day_hud_runtime_binding_smoke_reports_scoreinfo_and_display_owner_paths(self) -> None:
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)

        for token in [
            "SonicDayHudDisplayOwnerPathBinding",
            "scoreInfoPointMarkerRecordSpeedBinding",
            "scoreInfoPointMarkerCountBinding",
            "CGameDocument::m_pMember->m_ScoreInfo.PointMarkerRecordSpeed",
            "ui_playscreen/ring_count",
            "ui_playscreen/add/u_info",
            "formatSonicDayHudRuntimeDisplayOwnerPaths",
            "formatSonicDayHudRuntimeBindingPhase167SmokeSequence",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase167-sonic-hud-runtime-field-path-smoke", example)

        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase167-sonic-hud-runtime-field-path-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase167 sonic hud runtime field path smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_scoreinfo=record_speed=known:SWA::CGameDocument::m_pMember->m_ScoreInfo.PointMarkerRecordSpeed:point_marker_count=known:SWA::CGameDocument::m_pMember->m_ScoreInfo.PointMarkerCount", completed.stdout)
        self.assertIn("sonic_day_hud_display_owner_paths=ring=ui_playscreen/ring_count:score=CHudSonicStage.m_rcScoreCount|ui_playscreen/score_count:timer=CHudSonicStage.m_rcTimeCount|ui_playscreen/time_count:speed=CHudSonicStage.m_rcSpeedGauge|ui_playscreen/so_speed_gauge:boost=CHudSonicStage.m_rcSpeedGauge|ui_playscreen/so_speed_gauge:energy=CHudSonicStage.m_rcRingEnergyGauge|ui_playscreen/so_ringenagy_gauge:lives=CHudSonicStage.m_rcPlayerCount|ui_playscreen/player_count:tutorial=ui_playscreen/add/u_info", completed.stdout)
        self.assertIn("gameplay_numeric_binding=score:known,scoreinfo:known,ring/timer/speed/boost/energy/lives/tutorial:pending-runtime-player-offsets", completed.stdout)

    def test_phase168_sonic_day_hud_runtime_write_paths_promote_csd_text_sinks(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)

        for token in [
            "SonicDayHudRuntimeValueUpdatePath",
            "ringCountWritePath",
            "elapsedFramesWritePath",
            "speedReadoutWritePath",
            "lifeCountWritePath",
            "formatSonicDayHudRuntimeWritePaths",
            "formatSonicDayHudRuntimeBindingPhase168SmokeSequence",
        ]:
            self.assertIn(token, header)

        for token in [
            "CSD::CNode::SetText/sub_830BF640",
            "ui_playscreen/ring_count/num_ring",
            "ui_playscreen/time_count/time001|time010|time100",
            "ui_playscreen/add/speed_count/position/num_speed",
            "ui_playscreen/player_count/player",
            "ring/timer/speed/lives:known-via-csd-text-write",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase168-sonic-hud-runtime-write-path-smoke", example)

    def test_phase168_sonic_day_hud_runtime_write_path_smoke_reports_text_write_bindings(self) -> None:
        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase168-sonic-hud-runtime-write-path-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase168 sonic hud runtime write path smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_write_paths=ring=known:CSD::CNode::SetText/sub_830BF640@ui_playscreen/ring_count/num_ring", completed.stdout)
        self.assertIn("timer=known:CSD::CNode::SetText/sub_830BF640@ui_playscreen/time_count/time001|time010|time100", completed.stdout)
        self.assertIn("speed=known:CSD::CNode::SetText/sub_830BF640@ui_playscreen/add/speed_count/position/num_speed", completed.stdout)
        self.assertIn("lives=known:CSD::CNode::SetText/sub_830BF640@ui_playscreen/player_count/player", completed.stdout)
        self.assertIn("gameplay_numeric_binding=score:known,scoreinfo:known,ring/timer/speed/lives:known-via-csd-text-write,boost/energy/tutorial:pending-gauge-or-prompt-write-hook", completed.stdout)

    def test_phase169_sonic_day_hud_draw_list_coverage_and_gauge_hook_plan(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)

        for token in [
            "struct SonicDayHudRuntimeDrawListCoverage",
            "speedGaugeObserved",
            "ringEnergyGaugeObserved",
            "pauseOverlayObserved",
            "formatSonicDayHudRuntimeDrawListCoverage",
            "formatSonicDayHudRuntimeBindingPhase169SmokeSequence",
        ]:
            self.assertIn(token, header)

        for token in [
            "live-bridge/ui-draw-list manual observer",
            "CSD::CNode::SetPatternIndex/SetHideFlag/SetScale",
            "boost/energy/tutorial:csd-node-pattern-hide-scale-hooks-installed-pending-runtime-normalization",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase169-sonic-hud-draw-list-coverage-smoke", example)

        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase169-sonic-hud-draw-list-coverage-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase169 sonic hud draw-list coverage smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_draw_list_coverage=source=live-bridge/ui-draw-list manual observer:project=ui_playscreen:runtime_calls=96:correlated_pairs=96", completed.stdout)
        self.assertIn("speed_gauge=observed:gauge_frame=observed:energy_gauge=observed:tutorial_prompt=pending:pause_overlay=observed:text_write_observed=0", completed.stdout)
        self.assertIn("next_hook=CSD::CNode::SetPatternIndex/SetHideFlag/SetScale", completed.stdout)
        self.assertIn("gameplay_numeric_binding=score:known,scoreinfo:known,ring/timer/speed/lives:known-via-csd-text-write,boost/energy/tutorial:csd-node-pattern-hide-scale-hooks-installed-pending-runtime-normalization", completed.stdout)

    def test_phase173_sonic_day_hud_controller_consumes_live_text_write_observations(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)
        report = self.read(REPORT)

        for token in [
            "struct SonicDayHudRuntimeTextWriteObservation",
            "applyRuntimeTextWrite",
            "formatSonicDayHudRuntimeTextWriteObservation",
            "formatSonicDayHudRuntimeBindingPhase173SmokeSequence",
        ]:
            self.assertIn(token, header)

        for token in [
            "pathResolutionSource=raw-chud-sonic-stage-owner-field",
            "sonic-hud-value-text-write",
            "runtime-proven-via-raw-owner-field-text-write",
            "ui_playscreen/time_count/time100",
            "ui_playscreen/player_count/player",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase173-sonic-hud-live-text-write-smoke", example)
        self.assertIn("Phase 173", report)

        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase173-sonic-hud-live-text-write-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase173 sonic hud live text write smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_text_write=value=elapsedFrames:path=ui_playscreen/time_count/time100:text=39:resolution=raw-chud-sonic-stage-owner-field", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_text_write=value=lifeCount:path=ui_playscreen/player_count/player:text=03:resolution=raw-chud-sonic-stage-owner-field", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=raw-owner-text-write:rings=000:score=000000000:time=00:00:39:speed=000:boost=0.000:energy=1.000:lives=3:tutorial=none:hidden:route=stage-hud-ready:sfx=none:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("gameplay_numeric_binding=score:known,scoreinfo:known,timer/lives:runtime-proven-via-raw-owner-field-text-write,ring/speed:csd-text-write-ready,boost/energy/tutorial:csd-node-pattern-hide-scale-hooks-installed-pending-runtime-normalization", completed.stdout)

    def test_phase175_sonic_day_hud_controller_classifies_update_callsite_samples(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)
        report = self.read(REPORT)

        for token in [
            "struct SonicDayHudRuntimeCallsiteSample",
            "struct SonicDayHudRuntimeCallsiteClassification",
            "classifySonicDayHudRuntimeCallsiteSample",
            "applyRuntimeCallsiteSample",
            "formatSonicDayHudRuntimeCallsiteSample",
            "formatSonicDayHudRuntimeCallsiteClassification",
            "formatSonicDayHudRuntimeBindingPhase175SmokeSequence",
        ]:
            self.assertIn(token, header)

        for token in [
            "generated-PPC:sub_824D6048 owner+456/+452 -> CSD::CNode::SetText",
            "generated-PPC:sub_824D6418 speed readout via sub_8251A568",
            "generated-PPC:sub_824D6C18 owner+460/+480 rolling counter/gauge state",
            "runtime-proven-via-chud-update-callsite-sample",
            "classified-via-generated-PPC-callsite-candidate",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase175-sonic-hud-callsite-sample-smoke", example)
        self.assertIn("Phase 175", report)

        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase175-sonic-hud-callsite-sample-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase175 sonic hud callsite sample smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_callsite_sample=hook=sub_824D6048:phase=post-original:owner=0xCE2D6B0:delta=0.066667:r4=0x2F3BB30:field452=39:field456=11:field460=0:field480=0", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_callsite_classification=value=elapsedFrames:status=runtime-proven-via-chud-update-callsite-sample:source=generated-PPC:sub_824D6048 owner+456/+452 -> CSD::CNode::SetText", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=callsite-sample:rings=000:score=000000000:time=00:11:39:speed=000:boost=0.000:energy=1.000:lives=3:tutorial=none:hidden:route=stage-hud-ready:sfx=none:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_callsite_classification=value=rollingCounterGaugeState:status=classified-via-generated-PPC-callsite-candidate:source=generated-PPC:sub_824D6C18 owner+460/+480 rolling counter/gauge state", completed.stdout)
        self.assertIn("gameplay_numeric_binding=score:known,scoreinfo:known,timer:runtime-proven-via-chud-update-callsite-sample,ring/speed/lives:csd-text-write-ready,boost/energy/tutorial:classified-callsite-candidates-pending-normalization", completed.stdout)

    def test_phase180_sonic_day_hud_controller_consumes_gauge_prompt_write_observations(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)
        report = self.read(REPORT)

        for token in [
            "struct SonicDayHudRuntimeGaugePromptWriteObservation",
            "applyRuntimeGaugePromptWrite",
            "formatSonicDayHudRuntimeGaugePromptWriteObservation",
            "formatSonicDayHudRuntimeBindingPhase180SmokeSequence",
        ]:
            self.assertIn(token, header)

        for token in [
            "sonic-hud-gauge-scale-write",
            "sonic-hud-gauge-pattern-write",
            "sonic-hud-gauge-hide-write",
            "runtime-proven-via-csd-gauge-prompt-write",
            "ui_playscreen/so_speed_gauge",
            "ui_playscreen/so_ringenagy_gauge",
            "ui_playscreen/add/u_info",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase180-sonic-hud-gauge-prompt-write-smoke", example)
        self.assertIn("Phase 180", report)

        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase180-sonic-hud-gauge-prompt-write-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase180 sonic hud gauge prompt write smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_gauge_prompt_write=value=boostGauge:path=ui_playscreen/so_speed_gauge:kind=scale:value=0.650:resolution=raw-chud-sonic-stage-owner-field", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_gauge_prompt_write=value=ringEnergyGauge:path=ui_playscreen/so_ringenagy_gauge:kind=scale:value=0.720:resolution=raw-chud-sonic-stage-owner-field", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_gauge_prompt_write=value=tutorialPrompt:path=ui_playscreen/add/u_info:kind=pattern-index:value=3.000:resolution=csd-child-lookup-chain", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_gauge_prompt_write=value=tutorialPrompt:path=ui_playscreen/add/u_info:kind=hide-flag:value=0.000:resolution=csd-child-lookup-chain", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=gauge-prompt-write:rings=000:score=000000000:time=00:00:00:speed=000:boost=0.650:energy=0.720:lives=3:tutorial=pattern-3:visible:route=stage-hud-ready:sfx=none:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("gameplay_numeric_binding=score:known,scoreinfo:known,timer:runtime-proven-via-chud-update-callsite-sample,ring/speed/lives:csd-text-write-ready,boost/energy/tutorial:runtime-proven-via-csd-gauge-prompt-write,audio:pending-exact-sfx-id", completed.stdout)

    def test_phase191_sonic_day_hud_controller_consumes_semantic_path_candidates(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)
        report = self.read(REPORT)

        for token in [
            "struct SonicDayHudRuntimeSemanticPathCandidateObservation",
            "applyRuntimeSemanticPathCandidate",
            "formatSonicDayHudRuntimeSemanticPathCandidateObservation",
            "formatSonicDayHudRuntimeBindingPhase191SmokeSequence",
        ]:
            self.assertIn(token, header)

        for token in [
            "generated-PPC-callsite-semantic-candidate",
            "semantic-candidate-bound-pending-exact-child-node-resolution",
            "ui_playscreen/add/speed_count/position/num_speed",
            "ui_playscreen/so_speed_gauge",
            "ui_playscreen/so_ringenagy_gauge",
            "ui_playscreen/add/u_info",
        ]:
            self.assertIn(token, source)

        self.assertIn("--phase191-sonic-hud-semantic-candidate-smoke", example)
        self.assertIn("Phase 191", report)

        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase191-sonic-hud-semantic-candidate-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase191 sonic hud semantic candidate smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_semantic_path_candidate=value=speedKmh:path=ui_playscreen/add/speed_count/position/num_speed:kind=text:text=042:candidate_writes=2:resolution=generated-PPC-callsite-semantic-candidate", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_semantic_path_candidate=value=boostGauge:path=ui_playscreen/so_speed_gauge:kind=scale:value=0.650:candidate_writes=121:resolution=generated-PPC-callsite-semantic-candidate", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_semantic_path_candidate=value=ringEnergyGauge:path=ui_playscreen/so_ringenagy_gauge:kind=scale:value=0.720:candidate_writes=121:resolution=generated-PPC-callsite-semantic-candidate", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_semantic_path_candidate=value=tutorialPrompt:path=ui_playscreen/add/u_info:kind=pattern-index:value=3.000:candidate_writes=80:resolution=generated-PPC-callsite-semantic-candidate", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=semantic-candidate-binding:rings=000:score=000000000:time=00:00:00:speed=042:boost=0.650:energy=0.720:lives=3:tutorial=pattern-3:visible:route=stage-hud-ready:sfx=none:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("gameplay_numeric_binding=score:known,scoreinfo:known,speed/boost/energy/tutorial:semantic-candidate-bound-pending-exact-child-node-resolution,timer/ring/lives:exact-csd-text-write-or-callsite-lanes,audio:pending-exact-sfx-id", completed.stdout)

    def test_phase193_sonic_day_hud_controller_keeps_bound_and_candidate_lanes_separate(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)
        report = self.read(REPORT)

        for token in [
            "bindingStatus",
            "semantic-candidate-only-pending-runtime-bound",
            "formatSonicDayHudRuntimeBindingPhase193SmokeSequence",
        ]:
            self.assertIn(token, header + source)

        self.assertIn("--phase193-sonic-hud-semantic-bound-smoke", example)
        self.assertIn("Phase 193", report)

        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase193-sonic-hud-semantic-bound-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase193 sonic hud semantic bound smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_semantic_path_candidate=value=elapsedFrames:path=ui_playscreen/time_count/time001:kind=text:text=02:candidate_writes=10:resolution=generated-PPC-callsite-semantic-candidate:binding_status=semantic-bound-pending-exact-child-node-resolution", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_semantic_path_candidate=value=speedKmh:path=ui_playscreen/add/speed_count/position/num_speed:kind=text:text=00:candidate_writes=2:resolution=generated-PPC-callsite-semantic-candidate:binding_status=semantic-bound-pending-exact-child-node-resolution", completed.stdout)
        self.assertIn("sonic_day_hud_runtime_semantic_path_candidate=value=boostGauge:path=ui_playscreen/so_speed_gauge:kind=text:text=357:candidate_writes=338:resolution=generated-PPC-callsite-semantic-candidate:binding_status=semantic-candidate-only-pending-runtime-bound", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=phase192-strict-semantic-bound:rings=000:score=000000000:time=00:00:02:speed=000:boost=0.000:energy=1.000:lives=3:tutorial=pattern-1:visible:route=stage-hud-ready:sfx=none:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("gameplay_numeric_binding=elapsedFrames/speed/tutorial:semantic-bound-pending-exact-child-node-resolution,boost/energy:semantic-candidate-only-pending-runtime-bound,exact-child-node-resolution:pending,audio:pending-exact-sfx-id", completed.stdout)

    def test_phase194_sonic_day_hud_controller_reports_exact_gauge_child_paths_separately(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)
        report = self.read(REPORT)

        for token in [
            "SonicDayHudRuntimeGaugeChildPathResolution",
            "formatSonicDayHudRuntimeGaugeChildPathResolution",
            "formatSonicDayHudRuntimeBindingPhase194SmokeSequence",
            "setter-node-address-join-pending",
        ]:
            self.assertIn(token, header + source)

        self.assertIn("--phase194-sonic-hud-gauge-child-path-smoke", example)
        self.assertIn("Phase 194", report)

        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase194-sonic-hud-gauge-child-path-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase194 sonic hud gauge child path smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_gauge_child_path=value=boostGauge:exact_parent=ui_playscreen/so_speed_gauge/position/speed_gauge_color:representative_child=ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506:draw_layers=20:resolution=live-bridge/ui-draw-list:node_join=setter-node-address-join-pending", completed.stdout)
        self.assertIn("sonic_day_hud_gauge_child_path=value=ringEnergyGauge:exact_parent=ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color:representative_child=ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color/Cast_0483:draw_layers=20:resolution=live-bridge/ui-draw-list:node_join=setter-node-address-join-pending", completed.stdout)
        self.assertIn("gameplay_numeric_binding=boost/energy:exact-runtime-draw-child-paths-known,setter-node-address-join:pending,controller-value-update:still-requires-setter-node-match,audio:pending-exact-sfx-id", completed.stdout)

    def test_phase195_sonic_day_hud_controller_binds_gauge_setter_child_path_joins(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)
        report = self.read(REPORT)

        for token in [
            "SonicDayHudRuntimeGaugeSetterChildPathJoin",
            "applyRuntimeGaugeSetterChildPathJoin",
            "formatSonicDayHudRuntimeGaugeSetterChildPathJoin",
            "formatSonicDayHudRuntimeBindingPhase195SmokeSequence",
            "setter-node-address-join-runtime-proven",
        ]:
            self.assertIn(token, header + source)

        self.assertIn("--phase195-sonic-hud-gauge-setter-join-smoke", example)
        self.assertIn("Phase 195", report)

        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase195-sonic-hud-gauge-setter-join-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase195 sonic hud gauge setter join smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_gauge_setter_child_join=value=boostGauge:node=0xEA09708:kind=scale:value=0.650:exact_parent=ui_playscreen/so_speed_gauge/position/speed_gauge_color:exact_child=ui_playscreen/so_speed_gauge/position/speed_gauge_color/Cast_0506:join=runtime-draw-list-cast-node-match:binding_status=setter-node-address-join-runtime-proven", completed.stdout)
        self.assertIn("sonic_day_hud_gauge_setter_child_join=value=ringEnergyGauge:node=0xEA0A990:kind=scale:value=0.425:exact_parent=ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color:exact_child=ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color/Cast_0483:join=runtime-draw-list-cast-node-match:binding_status=setter-node-address-join-runtime-proven", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=phase195-gauge-setter-child-join:rings=000:score=000000000:time=00:00:00:speed=000:boost=0.650:energy=0.425:lives=3:tutorial=none:hidden:route=stage-hud-ready:sfx=none:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("gameplay_numeric_binding=boost/energy:setter-node-address-join-runtime-proven,controller-value-update:runtime-proven-via-exact-gauge-child-path,audio:pending-exact-sfx-id", completed.stdout)

    def test_phase196_sonic_day_hud_controller_reports_rolling_counter_candidates_without_binding_values(self) -> None:
        header = self.read(FRONTEND_CONTROLLERS_HEADER)
        source = self.read(FRONTEND_CONTROLLERS_SOURCE)
        example = self.read(FRONTEND_CONTROLLERS_EXAMPLE)
        report = self.read(REPORT)

        for token in [
            "SonicDayHudRuntimeRollingGaugeCounterObservation",
            "applyRuntimeRollingGaugeCounterObservation",
            "formatSonicDayHudRuntimeRollingGaugeCounterObservation",
            "formatSonicDayHudRuntimeBindingPhase196SmokeSequence",
            "rolling-counter-text-candidate-pending-gauge-state-normalization",
        ]:
            self.assertIn(token, header + source)

        self.assertIn("--phase196-sonic-hud-rolling-counter-smoke", example)
        self.assertIn("Phase 196", report)

        exe = Path(os.environ.get("SWARD_FRONTEND_SCREEN_CONTROLLER_CATALOG_EXE", DEFAULT_FRONTEND_CONTROLLER_EXE))
        if not exe.exists():
            self.skipTest(f"Frontend screen controller catalog executable not built: {exe}")

        completed = subprocess.run(
            [str(exe), "--phase196-sonic-hud-rolling-counter-smoke"],
            cwd=REPO_ROOT,
            check=True,
            capture_output=True,
            text=True,
        )

        self.assertIn("sward_frontend_screen_controller_catalog phase196 sonic hud rolling counter smoke ok", completed.stdout)
        self.assertIn("sonic_day_hud_rolling_gauge_counter=value=boostGauge:node=0x82914B0:path=ui_playscreen/so_speed_gauge:kind=text:text=530:callsite=sub_824D6C18:counter_writes=2:status=rolling-counter-text-candidate-pending-gauge-state-normalization", completed.stdout)
        self.assertIn("sonic_day_hud_rolling_gauge_counter=value=ringEnergyGauge:node=0x82914B0:path=ui_playscreen/so_ringenagy_gauge:kind=text:text=530:callsite=sub_824D6C18:counter_writes=1:status=rolling-counter-text-candidate-pending-gauge-state-normalization", completed.stdout)
        self.assertIn("sonic_day_hud_state=phase=phase196-rolling-counter-candidate:rings=000:score=000000000:time=00:00:00:speed=000:boost=0.000:energy=1.000:lives=3:tutorial=none:hidden:route=stage-hud-ready:sfx=none:sfx_id=audio-id-pending", completed.stdout)
        self.assertIn("gameplay_numeric_binding=boost/energy:rolling-counter-text-candidate-pending-gauge-state-normalization,setter-node-address-join:still-required-for-final-gauge-values,audio:pending-exact-sfx-id", completed.stdout)


if __name__ == "__main__":
    unittest.main()
