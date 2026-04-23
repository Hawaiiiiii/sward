#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def cpp_string(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def normalize_token(value: str) -> str:
    return value.replace("\\", "/")


def unique_ordered(values: list[str]) -> list[str]:
    seen = set()
    result = []
    for value in values:
        if value in seen:
            continue
        seen.add(value)
        result.append(value)
    return result


def primary_contract_for_entry(group: dict, entry: dict) -> str | None:
    runtime_contracts = entry.get("runtime_contracts", [])
    if runtime_contracts:
        return runtime_contracts[0]

    relative_source_path = entry["relative_source_path"]
    lowered = relative_source_path.lower()

    if "inspirepreview" in lowered or "stagemovie" in lowered or "movie/" in lowered or "playmovie" in lowered:
        return "subtitle_cutscene_reference.json"
    if "gamemodemenuselectdebug" in lowered or "gamemodemainmenu_test" in lowered:
        return "title_menu_reference.json"
    if "gamemodestageselectdebug" in lowered:
        return "loading_transition_reference.json"

    likely_runtime_contracts = group.get("likely_runtime_contracts", [])
    if likely_runtime_contracts:
        return likely_runtime_contracts[0]
    return None


def build_host_entries(frontend_payload: dict) -> list[dict]:
    entries: list[dict] = []

    for group in frontend_payload.get("priority_groups", []):
        priority = group.get("priority", "medium")
        if priority not in {"immediate", "high"}:
            continue

        for entry in group.get("entries", []):
            primary_contract = primary_contract_for_entry(group, entry)
            if not primary_contract:
                continue

            relative_source_path = entry["relative_source_path"]
            alias_tokens = unique_ordered(
                [
                    group["display_name"],
                    group["group_id"],
                    entry["family_name"],
                    entry["family_id"],
                    relative_source_path,
                    Path(relative_source_path).name,
                    primary_contract,
                    Path(primary_contract).stem,
                ]
            )

            entries.append(
                {
                    "group_id": group["group_id"],
                    "group_display_name": group["display_name"],
                    "priority": priority,
                    "relative_source_path": relative_source_path,
                    "host_display_name": Path(relative_source_path).name,
                    "primary_contract_file_name": primary_contract,
                    "alias_tokens": alias_tokens,
                    "notes": entry.get("humanization_priority", group.get("rationale", "")),
                }
            )

    entries.sort(key=lambda item: (item["group_display_name"], item["host_display_name"]))
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
        "struct DebugWorkbenchHostEntry",
        "{",
        "    std::string_view groupId;",
        "    std::string_view groupDisplayName;",
        "    std::string_view priority;",
        "    std::string_view relativeSourcePath;",
        "    std::string_view hostDisplayName;",
        "    std::string_view primaryContractFileName;",
        "    std::string_view aliasBlob;",
        "    std::string_view notes;",
        "};",
        "",
        f"inline constexpr std::array<DebugWorkbenchHostEntry, {len(entries)}> kDebugWorkbenchHostEntries{{{{",
    ]

    for entry in entries:
        alias_blob = "|".join(entry["alias_tokens"])
        lines.extend(
            [
                "    {",
                f'        "{cpp_string(entry["group_id"])}",',
                f'        "{cpp_string(entry["group_display_name"])}",',
                f'        "{cpp_string(entry["priority"])}",',
                f'        "{cpp_string(entry["relative_source_path"])}",',
                f'        "{cpp_string(entry["host_display_name"])}",',
                f'        "{cpp_string(entry["primary_contract_file_name"])}",',
                f'        "{cpp_string(alias_blob)}",',
                f'        "{cpp_string(entry["notes"])}",',
                "    },",
            ]
        )

    lines.extend(
        [
            "}};",
            "} // namespace sward::ui_runtime",
        ]
    )
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate richer debug workbench host metadata from the frontend shell recovery layer.")
    parser.add_argument("--input", default="research_uiux/data/frontend_shell_recovery.json", help="Frontend shell recovery JSON.")
    parser.add_argument("--output-header", default="research_uiux/runtime_reference/include/sward/ui_runtime/debug_workbench_data.hpp", help="Generated workbench metadata header.")
    parser.add_argument("--output-json", default="research_uiux/data/debug_workbench_host_map.json", help="Tracked debug workbench host JSON.")
    args = parser.parse_args()

    input_path = Path(args.input).resolve()
    output_header = Path(args.output_header).resolve()
    output_json = Path(args.output_json).resolve()

    frontend_payload = read_json(input_path)
    entries = build_host_entries(frontend_payload)

    output_header.parent.mkdir(parents=True, exist_ok=True)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    write_header(entries, output_header)
    output_json.write_text(json.dumps({"hosts": entries}, indent=2) + "\n", encoding="utf-8")
    print("built_debug_workbench_data", f"hosts={len(entries)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
