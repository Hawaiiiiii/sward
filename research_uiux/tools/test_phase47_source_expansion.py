#!/usr/bin/env python3
from __future__ import annotations

import unittest
from pathlib import Path

from build_broader_ui_adjacent_source_seed import build_seed_lines, extract_relative_source_path
from map_ui_source_paths import classify_source_path


REPO_ROOT = Path(__file__).resolve().parents[2]
MATCH_DUMP = REPO_ROOT / "Match SU OG source code folders and locations.txt"


class Phase47SourceExpansionTests(unittest.TestCase):
    def test_seed_keeps_curated_ui_support_substrate_paths(self) -> None:
        seed_lines = build_seed_lines(MATCH_DUMP)
        relatives = {
            extract_relative_source_path(line)
            for line in seed_lines
        }

        required_paths = {
            "Achievement/AchievementManager.cpp",
            "Animation/EventTrigger/Event/AnimationEventTriggerAudio.cpp",
            "Camera/Controller/Player3DBossCamera.cpp",
            "Player/Parameter/PlayerParameter.cpp",
            "Player/Switch/PlayerSwitchManager.cpp",
            "Sound/SoundController.cpp",
            "XML/XMLManager.cpp",
        }
        for relative_path in required_paths:
            self.assertIn(relative_path, relatives)

        self.assertNotIn("NPC/Enemy/SonicEnemy/EnemyList/EAirChaser/EnemyEAirChaserAIManager.cpp", relatives)
        self.assertNotIn("Object/Common/ObjSpring.cpp", relatives)

    def test_new_support_paths_get_specific_families(self) -> None:
        cases = {
            "Achievement/AchievementManager.cpp": (
                "achievement_unlock_support",
                ["achievement_unlock_support"],
            ),
            "Animation/EventTrigger/Event/AnimationEventTriggerAudio.cpp": (
                "timeline_event_trigger_support",
                ["subtitle_cutscene_presentation"],
            ),
            "Camera/Controller/Player3DBossCamera.cpp": (
                "frontend_camera_shell",
                ["camera_shell", "boss_hud"],
            ),
            "Player/Parameter/PlayerParameter.cpp": (
                "player_status_support",
                ["sonic_stage_hud", "werehog_stage_hud", "super_sonic_hud"],
            ),
            "Sound/SoundController.cpp": (
                "audio_cue_support",
                ["audio_cue_support"],
            ),
            "XML/XMLManager.cpp": (
                "xml_data_loading_support",
                ["xml_data_loading_support"],
            ),
        }

        for relative_path, (family_id, candidate_system_ids) in cases.items():
            with self.subTest(relative_path=relative_path):
                classification = classify_source_path(relative_path)
                self.assertEqual(classification["family_id"], family_id)
                self.assertEqual(classification["candidate_system_ids"], candidate_system_ids)


if __name__ == "__main__":
    unittest.main()
