#!/usr/bin/env python3
from __future__ import annotations

import unittest
from pathlib import Path

from materialize_local_debug_source_tree import build_target_groups, normalize_key, read_json


REPO_ROOT = Path(__file__).resolve().parents[2]
MANIFEST = REPO_ROOT / "research_uiux" / "data" / "ui_source_path_manifest.json"
GAMEPLAY_HUD_MAP = REPO_ROOT / "research_uiux" / "data" / "gameplay_hud_core_map.json"


EXPECTED_SUPPORT_PATHS = {
    "Achievement/AchievementManager.cpp",
    "Animation/EventTrigger/AnimationEventTriggerContainer.cpp",
    "Animation/EventTrigger/Event/AnimationEventTriggerAudio.cpp",
    "Animation/EventTrigger/Event/AnimationEventTriggerSparkle.cpp",
    "Animation/EventTrigger/Event/AnimationEventTriggerVibration.cpp",
    "Player/Parameter/PlayerParameter.cpp",
    "Player/Switch/PlayerSwitchManager.cpp",
    "Sound/Sound.cpp",
    "Sound/SoundBGMActEggman.cpp",
    "Sound/SoundBGMActEvil.cpp",
    "Sound/SoundBGMActSonic.cpp",
    "Sound/SoundBGMDispel.cpp",
    "Sound/SoundBGMExtra.cpp",
    "Sound/SoundBGMStandard.cpp",
    "Sound/SoundBGMTown.cpp",
    "Sound/SoundController.cpp",
    "Sound/SoundPlayer.cpp",
    "XML/XMLBinData.cpp",
    "XML/XMLDocument.cpp",
    "XML/XMLManager.cpp",
    "XML/XMLNode.cpp",
    "XML/XMLTypeSLBin.cpp",
    "XML/XMLTypeSLTxt.cpp",
}


class LocalDebugSourceTreeSupportSubstrateTests(unittest.TestCase):
    def test_phase47_support_substrate_has_a_dedicated_local_source_group(self) -> None:
        manifest_payload = read_json(MANIFEST)
        gameplay_payload = read_json(GAMEPLAY_HUD_MAP)
        entries_by_path = {
            normalize_key(entry["relative_source_path"]): entry
            for entry in manifest_payload.get("entries", [])
        }

        groups = build_target_groups(entries_by_path, gameplay_payload)
        support_group = next(
            (group for group in groups if group["group_id"] == "support_substrate_sources"),
            None,
        )

        self.assertIsNotNone(support_group)
        self.assertEqual(support_group["display_name"], "Support Substrate Sources")
        self.assertEqual(support_group["purpose"], "Readable local-only support-substrate scaffolds for achievement unlocks, animation event triggers, player-status feeds, audio routing, and XML/data loading.")

        targets = {
            target["relative_source_path"]: target
            for target in support_group["targets"]
        }
        self.assertEqual(set(targets), EXPECTED_SUPPORT_PATHS)
        self.assertEqual(len(targets), 23)
        self.assertTrue(all(target["renderer"] == "support_substrate" for target in targets.values()))

        for relative_path, target in targets.items():
            with self.subTest(relative_path=relative_path):
                self.assertNotEqual(target["entry"]["status"], "named_seed_only")


if __name__ == "__main__":
    unittest.main()
