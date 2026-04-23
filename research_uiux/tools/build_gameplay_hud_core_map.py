#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


REPO_ROOT = Path(__file__).resolve().parents[2]
ASPECT_RATIO_PATCHES = REPO_ROOT / "UnleashedRecomp" / "patches" / "aspect_ratio_patches.cpp"
SWA_TOML = REPO_ROOT / "UnleashedRecompLib" / "config" / "SWA.toml"
SOURCE_PATH_DUMP = REPO_ROOT / "Match SU OG source code folders and locations.txt"
UI_SOURCE_SEED = REPO_ROOT / "research_uiux" / "source_path_seeds" / "UI_SOURCE_PATHS_FROM_EXECUTABLE.txt"
EXTRACTED_ASSETS = REPO_ROOT / "extracted_assets"


@dataclass(frozen=True)
class SystemDefinition:
    system_id: str
    screen_name: str
    host_paths: tuple[str, ...]
    layout_families: tuple[str, ...]
    texture_tokens: tuple[str, ...]
    ownership_summary: str
    confidence: str


SYSTEMS: tuple[SystemDefinition, ...] = (
    SystemDefinition(
        system_id="sonic_stage_hud",
        screen_name="Sonic Stage HUD",
        host_paths=(
            "HUD/Sonic/HudSonicStage.cpp",
            "HUD/Sonic/SonicMainDisplay.cpp",
            "Player/Character/Sonic/Hud/SonicHudGuide.cpp",
            "Player/Character/Sonic/Hud/SonicHudHomingAttack.cpp",
        ),
        layout_families=("ui_playscreen",),
        texture_tokens=("mat_playscreen", "mat_playscreen_en"),
        ownership_summary="Day-stage HUD shell with score/time/exp/ring/boost ownership plus guide-side prompt helpers.",
        confidence="medium_high",
    ),
    SystemDefinition(
        system_id="werehog_stage_hud",
        screen_name="Werehog Stage HUD",
        host_paths=(
            "HUD/Evil/HudEvilStage.cpp",
            "HUD/Evil/EvilMainDisplay.cpp",
            "Player/Character/EvilSonic/Hud/EvilHudGuide.cpp",
            "Player/Character/EvilSonic/Hud/EvilHudTarget.cpp",
        ),
        layout_families=("ui_playscreen_ev", "ui_playscreen_ev_hit"),
        texture_tokens=("mat_playscreen", "mat_playscreen_en", "ui_ps1_e_gauge1"),
        ownership_summary="Night-stage HUD shell with unleash/life/shield gauges, hit-counter overlay, and helper-guide ownership.",
        confidence="high",
    ),
    SystemDefinition(
        system_id="extra_stage_hud",
        screen_name="Extra Stage / Tornado Defense HUD",
        host_paths=("ExtraStage/Tails/Hud/HudExQte.cpp",),
        layout_families=("ui_prov_playscreen", "ui_qte"),
        texture_tokens=("mat_playscreen", "mat_playscreen_en", "mat_qte"),
        ownership_summary="ExStage battle HUD with play-screen counters, controller-specific QTE prompts, and dedicated extra-stage host ownership.",
        confidence="high",
    ),
    SystemDefinition(
        system_id="super_sonic_hud",
        screen_name="Super Sonic / Final HUD Bridge",
        host_paths=(
            "Boss/BossHudSuperSonic.cpp",
            "Boss/BossHudVitality.cpp",
            "Boss/BossNamePlate.cpp",
        ),
        layout_families=("ui_playscreen_su",),
        texture_tokens=("mat_playscreen", "mat_playscreen_en"),
        ownership_summary="Super Sonic / boss-facing HUD bridge with dual gauges and footer ownership hints.",
        confidence="medium",
    ),
)


HASH_RE = re.compile(r'HashStr\("([^"]+)"\)')
FUNC_IMPL_RE = re.compile(r"PPC_FUNC_IMPL\(__imp__sub_([0-9A-Fa-f]+)\);")
HOOK_NAME_RE = re.compile(r'name = "(EvilHudGuideAllocMidAsmHook|EvilHudGuideUpdateMidAsmHook)"')


def read_text_lines(path: Path) -> list[str]:
    if not path.is_file():
        return []
    return path.read_text(encoding="utf-8", errors="ignore").splitlines()


def collect_source_path_hits(relative_paths: Iterable[str]) -> dict[str, list[str]]:
    dump_lines = read_text_lines(SOURCE_PATH_DUMP)
    seed_lines = read_text_lines(UI_SOURCE_SEED)
    haystack = dump_lines + seed_lines
    result: dict[str, list[str]] = {}
    for relative in relative_paths:
        hits = [line.strip() for line in haystack if relative.replace("/", "\\") in line]
        if hits:
            result[relative] = sorted(dict.fromkeys(hits))
    return result


def collect_layout_nodes() -> dict[str, list[str]]:
    lines = read_text_lines(ASPECT_RATIO_PATCHES)
    results = {family: [] for system in SYSTEMS for family in system.layout_families}
    for line in lines:
        match = HASH_RE.search(line)
        if not match:
            continue
        hash_value = match.group(1)
        for family in results:
            if hash_value == family or hash_value.startswith(family + "/"):
                results[family].append(hash_value)

    return {family: sorted(dict.fromkeys(nodes)) for family, nodes in results.items()}


def collect_evil_hud_guide_hooks() -> dict[str, object]:
    lines = read_text_lines(ASPECT_RATIO_PATCHES)
    constructors: list[str] = []
    updates: list[str] = []
    current_marker: str | None = None

    for line in lines:
        if "CEvilHudGuide::CEvilHudGuide" in line:
            current_marker = "constructor"
            continue
        if "CEvilHudGuide::Update" in line:
            current_marker = "update"
            continue

        match = FUNC_IMPL_RE.search(line)
        if not match or current_marker is None:
            continue

        value = f"0x{match.group(1).upper()}"
        if current_marker == "constructor":
            constructors.append(value)
        elif current_marker == "update":
            updates.append(value)
        current_marker = None

    toml_hooks = sorted({match.group(1) for match in HOOK_NAME_RE.finditer("\n".join(read_text_lines(SWA_TOML)))})
    return {
        "config_hooks": toml_hooks,
        "constructor_seams": constructors,
        "update_seams": updates,
    }


def collect_extracted_asset_hits(texture_tokens: Iterable[str], layout_families: Iterable[str]) -> dict[str, list[str]]:
    if not EXTRACTED_ASSETS.is_dir():
        return {"loose_layout_files": [], "texture_hits": []}

    layout_names = {f"{family}.yncp" for family in layout_families} | {f"{family}.xncp" for family in layout_families}
    texture_tokens_normalized = [token.lower() for token in texture_tokens]
    loose_layout_files: list[str] = []
    texture_hits: list[str] = []

    for path in EXTRACTED_ASSETS.rglob("*"):
        if not path.is_file():
            continue

        relative = path.relative_to(REPO_ROOT).as_posix()
        name_lower = path.name.lower()

        if path.name in layout_names:
            loose_layout_files.append(relative)

        if any(token in name_lower for token in texture_tokens_normalized):
            texture_hits.append(relative)

    texture_hits = sorted(dict.fromkeys(texture_hits))[:24]
    return {
        "loose_layout_files": sorted(dict.fromkeys(loose_layout_files)),
        "texture_hits": texture_hits,
    }


def build_payload() -> dict[str, object]:
    layout_nodes = collect_layout_nodes()
    evil_hooks = collect_evil_hud_guide_hooks()
    systems_payload: list[dict[str, object]] = []

    for system in SYSTEMS:
        source_hits = collect_source_path_hits(system.host_paths)
        asset_hits = collect_extracted_asset_hits(system.texture_tokens, system.layout_families)
        family_summaries = []
        for family in system.layout_families:
            family_summaries.append(
                {
                    "layout_id": family,
                    "hash_node_count": len(layout_nodes.get(family, [])),
                    "hash_nodes": layout_nodes.get(family, [])[:32],
                    "loose_layout_present": any(Path(path).name.startswith(family) for path in asset_hits["loose_layout_files"]),
                }
            )

        system_payload: dict[str, object] = {
            "system_id": system.system_id,
            "screen_name": system.screen_name,
            "confidence": system.confidence,
            "ownership_summary": system.ownership_summary,
            "host_paths": list(system.host_paths),
            "source_path_hits": source_hits,
            "layout_families": family_summaries,
            "loose_layout_files": asset_hits["loose_layout_files"],
            "texture_hits": asset_hits["texture_hits"],
        }

        if system.system_id == "werehog_stage_hud":
            system_payload["readable_hooks"] = evil_hooks

        systems_payload.append(system_payload)

    return {
        "generated_at_repo_root": str(REPO_ROOT),
        "source_inputs": {
            "source_path_dump": str(SOURCE_PATH_DUMP),
            "ui_source_seed": str(UI_SOURCE_SEED),
            "aspect_ratio_patches": str(ASPECT_RATIO_PATCHES),
            "swa_toml": str(SWA_TOML),
            "extracted_assets_root": str(EXTRACTED_ASSETS),
        },
        "summary": {
            "system_count": len(SYSTEMS),
            "layout_family_count": sum(len(system.layout_families) for system in SYSTEMS),
            "loose_layout_family_count": sum(
                1
                for system in systems_payload
                for family in system["layout_families"]
                if family["loose_layout_present"]
            ),
            "hash_only_family_count": sum(
                1
                for system in systems_payload
                for family in system["layout_families"]
                if not family["loose_layout_present"]
            ),
            "evil_hud_guide_hook_count": len(evil_hooks["config_hooks"]) + len(evil_hooks["constructor_seams"]) + len(evil_hooks["update_seams"]),
        },
        "systems": systems_payload,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a machine-readable gameplay HUD core recovery map.")
    parser.add_argument(
        "--output",
        type=Path,
        default=REPO_ROOT / "research_uiux" / "data" / "gameplay_hud_core_map.json",
        help="Output JSON path.",
    )
    args = parser.parse_args()

    payload = build_payload()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    summary = payload["summary"]
    print(
        "gameplay_hud_core_map:",
        f"systems={summary['system_count']}",
        f"layout_families={summary['layout_family_count']}",
        f"loose_layout_families={summary['loose_layout_family_count']}",
        f"hash_only_families={summary['hash_only_family_count']}",
        f"evil_hooks={summary['evil_hud_guide_hook_count']}",
    )
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
