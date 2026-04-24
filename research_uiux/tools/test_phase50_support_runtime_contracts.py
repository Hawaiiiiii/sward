#!/usr/bin/env python3
from __future__ import annotations

import unittest
from collections import Counter
from pathlib import Path

from build_debug_workbench_data import build_host_entries
from build_source_family_selector_data import build_entries
from map_ui_source_paths import RUNTIME_CONTRACTS, build_payload, read_json


REPO_ROOT = Path(__file__).resolve().parents[2]
CONTRACTS_DIR = REPO_ROOT / "research_uiux" / "runtime_reference" / "contracts"
SOURCE_SEED = REPO_ROOT / "research_uiux" / "source_path_seeds" / "UI_ADJACENT_SOURCE_PATHS_FROM_MATCH_DUMP.txt"
ARCHAEOLOGY_JSON = REPO_ROOT / "research_uiux" / "data" / "ui_archaeology_database.json"
FRONTEND_RECOVERY_JSON = REPO_ROOT / "research_uiux" / "data" / "frontend_shell_recovery.json"
GAMEPLAY_HUD_JSON = REPO_ROOT / "research_uiux" / "data" / "gameplay_hud_core_map.json"


EXPECTED_SUPPORT_CONTRACTS = {
    "achievement_unlock_support": ("achievement_unlock_support_reference.json", "AchievementUnlockSupportReference"),
    "audio_cue_support": ("audio_cue_support_reference.json", "AudioCueSupportReference"),
    "xml_data_loading_support": ("xml_data_loading_support_reference.json", "XmlDataLoadingSupportReference"),
}


class Phase50SupportRuntimeContractsTests(unittest.TestCase):
    def test_support_contracts_are_bundled_and_schema_complete(self) -> None:
        for source_system, (contract_name, screen_id) in EXPECTED_SUPPORT_CONTRACTS.items():
            with self.subTest(contract_name=contract_name):
                payload = read_json(CONTRACTS_DIR / contract_name)
                self.assertEqual(payload["screen_id"], screen_id)
                self.assertEqual(payload["source_system"], source_system)
                self.assertGreaterEqual(len(payload["timeline_bands"]), 4)
                self.assertEqual(
                    {state["state"] for state in payload["states"]},
                    {"Boot", "Intro", "Idle", "Navigate", "Confirm", "Cancel", "Outro", "Closed"},
                )
                self.assertGreaterEqual(len(payload["overlay_layers"]), 4)
                self.assertGreaterEqual(len(payload["prompt_slots"]), 3)

    def test_manifest_promotes_support_substrate_paths_to_contract_backed(self) -> None:
        for source_system, (contract_name, _) in EXPECTED_SUPPORT_CONTRACTS.items():
            self.assertEqual(RUNTIME_CONTRACTS[source_system], contract_name)

        payload = build_payload(REPO_ROOT, SOURCE_SEED, ARCHAEOLOGY_JSON, CONTRACTS_DIR)
        self.assertEqual(payload["summary"]["input_path_count"], 269)
        self.assertEqual(payload["summary"]["runtime_contract_backed_count"], 203)
        self.assertEqual(payload["summary"]["runtime_contract_backed_pct"], 75.5)
        self.assertEqual(payload["summary"]["named_seed_only_count"], 0)
        self.assertEqual(payload["summary"]["status_counts"]["archaeology_mapped"], 9)

        expected_paths = {
            "Achievement/AchievementManager.cpp": "achievement_unlock_support_reference.json",
            "Sound/SoundController.cpp": "audio_cue_support_reference.json",
            "Sound/SoundPlayer.cpp": "audio_cue_support_reference.json",
            "XML/XMLManager.cpp": "xml_data_loading_support_reference.json",
            "XML/XMLDocument.cpp": "xml_data_loading_support_reference.json",
        }
        entries_by_path = {
            entry["relative_source_path"]: entry
            for entry in payload["entries"]
        }
        for relative_path, contract_name in expected_paths.items():
            with self.subTest(relative_path=relative_path):
                entry = entries_by_path[relative_path]
                self.assertEqual(entry["status"], "contract_backed")
                self.assertIn(contract_name, entry["runtime_contracts"])

    def test_selector_and_workbench_expose_support_contract_hosts(self) -> None:
        manifest_payload = build_payload(REPO_ROOT, SOURCE_SEED, ARCHAEOLOGY_JSON, CONTRACTS_DIR)

        selector_entries = build_entries(manifest_payload)
        selector_by_family = {
            entry["family_id"]: entry
            for entry in selector_entries
        }
        self.assertEqual(len(selector_entries), 19)
        for source_system, (contract_name, _) in EXPECTED_SUPPORT_CONTRACTS.items():
            with self.subTest(selector_family=source_system):
                self.assertIn(source_system, selector_by_family)
                self.assertEqual(selector_by_family[source_system]["contract_file_name"], contract_name)

        workbench_entries = build_host_entries(
            read_json(FRONTEND_RECOVERY_JSON),
            read_json(GAMEPLAY_HUD_JSON),
            manifest_payload,
        )
        group_counts = Counter(entry["group_display_name"] for entry in workbench_entries)
        self.assertEqual(len(workbench_entries), 176)
        self.assertEqual(group_counts["Support Substrate Hosts"], 17)

        support_contracts = {
            entry["primary_contract_file_name"]
            for entry in workbench_entries
            if entry["group_display_name"] == "Support Substrate Hosts"
        }
        self.assertEqual(
            support_contracts,
            {
                "achievement_unlock_support_reference.json",
                "audio_cue_support_reference.json",
                "xml_data_loading_support_reference.json",
            },
        )


if __name__ == "__main__":
    unittest.main()
