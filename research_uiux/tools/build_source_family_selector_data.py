#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def stem(name: str) -> str:
    return Path(name).stem


PRIMARY_FAMILY_IDS = {
    "loading_and_start": "loading_and_boot",
    "mission_result_family": "mission_result",
    "pause_stack": "pause_stack",
    "save_and_ending": "save_and_ending",
    "title_menu": "title_menu",
    "world_map_stack": "world_map_stack",
}


def unique_ordered(values: list[str]) -> list[str]:
    seen = set()
    result = []
    for value in values:
        if value in seen:
            continue
        seen.add(value)
        result.append(value)
    return result


def cpp_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def build_entries(payload: dict) -> list[dict]:
    systems_by_id = {system["system_id"]: system for system in payload.get("systems", [])}
    entries = []

    for system in payload.get("systems", []):
        runtime_contract = system.get("runtime_contract")
        if not runtime_contract:
            continue

        system_id = system["system_id"]
        matched_entries = [
            entry
            for entry in payload.get("entries", [])
            if system_id in entry.get("matched_system_ids", [])
            and entry.get("family_id") == PRIMARY_FAMILY_IDS.get(system_id, system_id)
        ]

        alias_values: list[str] = [
            system["screen_name"],
            system_id,
            runtime_contract,
            stem(runtime_contract),
        ]
        source_paths: list[str] = []

        for entry in matched_entries:
            relative_source_path = entry["relative_source_path"]
            source_path = entry["source_path"]
            source_paths.append(relative_source_path)
            alias_values.extend(
                [
                    entry["family_name"],
                    entry["family_id"],
                    relative_source_path,
                    Path(relative_source_path).name,
                    source_path,
                ]
            )

        entries.append(
            {
                "family_id": system_id,
                "display_name": system["screen_name"],
                "source_system_id": system_id,
                "contract_file_name": runtime_contract,
                "alias_tokens": unique_ordered(alias_values),
                "source_paths": unique_ordered(source_paths),
                "seed_path_count": len(matched_entries),
                "layout_ids": system.get("layout_ids", []),
            }
        )

    entries.sort(key=lambda item: item["display_name"])
    return entries


def write_header(entries: list[dict], output_path: Path) -> None:
    lines = [
        "#pragma once",
        "",
        "#include <array>",
        "#include <string_view>",
        "",
        "namespace sward::ui_runtime",
        "{",
        "struct SourceFamilySelectorEntry",
        "{",
        "    std::string_view familyId;",
        "    std::string_view displayName;",
        "    std::string_view sourceSystemId;",
        "    std::string_view contractFileName;",
        "    std::string_view aliasBlob;",
        "    std::string_view sourcePathBlob;",
        "};",
        "",
        f"inline constexpr std::array<SourceFamilySelectorEntry, {len(entries)}> kSourceFamilySelectorEntries{{{{",
    ]

    for entry in entries:
        alias_blob = "|".join(entry["alias_tokens"])
        source_blob = "|".join(entry["source_paths"])
        lines.append("    {")
        lines.append(f'        "{cpp_string(entry["family_id"])}",')
        lines.append(f'        "{cpp_string(entry["display_name"])}",')
        lines.append(f'        "{cpp_string(entry["source_system_id"])}",')
        lines.append(f'        "{cpp_string(entry["contract_file_name"])}",')
        lines.append(f'        "{cpp_string(alias_blob)}",')
        lines.append(f'        "{cpp_string(source_blob)}",')
        lines.append("    },")

    lines.extend(
        [
            "}};",
            "} // namespace sward::ui_runtime",
        ]
    )

    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_markdown(entries: list[dict], output_path: Path) -> None:
    lines = [
        '<p align="right">',
        '    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>',
        "</p>",
        "",
        '# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Source-Path-Named Debug Selector',
        "",
        "Phase 33 upgrades the standalone selector from contract-stem browsing into source-family browsing.",
        "",
        "> [!IMPORTANT]",
        "> This still runs on the repo-safe runtime contracts. The new part is the naming layer: the selector can now speak in recovered source-family terms such as `TitleMenu.cpp`, `HudPause.cpp`, and `WorldMapSelect.cpp` instead of only `title`, `pause`, or raw contract filenames.",
        "",
        "## Launch Families",
        "",
        f"- Source-path launch families: `{len(entries)}`",
        "",
        "| Family | Contract | Seed paths | Example anchors |",
        "|---|---|---:|---|",
    ]

    for entry in entries:
        sample_paths = ", ".join(f"`{path}`" for path in entry["source_paths"][:3])
        lines.append(
            f"| {entry['display_name']} | `{entry['contract_file_name']}` | `{entry['seed_path_count']}` | {sample_paths} |"
        )

    lines.extend(
        [
            "",
            "## Example Family Tokens",
            "",
        ]
    )

    for entry in entries:
        example_tokens = unique_ordered(
            [Path(path).name for path in entry["source_paths"][:3]] + [entry["display_name"], entry["family_id"]]
        )
        lines.append(f"- `{entry['display_name']}`: {', '.join(f'`{token}`' for token in example_tokens[:6])}")

    lines.extend(
        [
            "",
            "## Selector Direction",
            "",
            "- The selector can now treat source-family aliases as first-class launch tokens.",
            "- The current launch set is still bounded by the six contract-backed systems, but the tokens now line up with the mirrored source-family tree and the executable path dump.",
            "- This is the bridge needed before the richer debug sandbox can be widened with gameplay HUD and subtitle/cutscene families.",
        ]
    )

    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Build source-family selector metadata from the current source-path manifest.")
    parser.add_argument("--input", default="research_uiux/data/ui_source_path_manifest.json", help="Ignored source-path manifest JSON.")
    parser.add_argument(
        "--output-header",
        default="research_uiux/runtime_reference/include/sward/ui_runtime/source_family_selector_data.hpp",
        help="Tracked generated selector metadata header.",
    )
    parser.add_argument(
        "--output-md",
        default="research_uiux/SOURCE_PATH_NAMED_DEBUG_SELECTOR.md",
        help="Tracked markdown summary.",
    )
    args = parser.parse_args()

    input_path = Path(args.input).resolve()
    output_header = Path(args.output_header).resolve()
    output_md = Path(args.output_md).resolve()

    payload = read_json(input_path)
    entries = build_entries(payload)

    output_header.parent.mkdir(parents=True, exist_ok=True)
    output_md.parent.mkdir(parents=True, exist_ok=True)
    write_header(entries, output_header)
    write_markdown(entries, output_md)
    print("built_source_family_selector_data", f"entries={len(entries)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
