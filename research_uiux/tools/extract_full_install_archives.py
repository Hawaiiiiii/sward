#!/usr/bin/env python3
"""Extract every Hedgehog Engine archive from the current Unleashed install."""

from __future__ import annotations

import argparse
import json
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


def resolve_path(repo_root: Path, value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else repo_root / path


def archive_source_for(arl_path: Path) -> Path | None:
    first_segment = arl_path.with_suffix(".ar.00")
    if first_segment.exists():
        return first_segment

    single_archive = arl_path.with_suffix(".ar")
    if single_archive.exists():
        return single_archive

    return None


def output_dir_for(output_root: Path, install_root: Path, source_path: Path) -> Path:
    relative = source_path.relative_to(install_root)
    name = source_path.name
    if name.lower().endswith(".ar.00"):
        stem = name[:-6]
    else:
        stem = source_path.stem
    return output_root.joinpath(*relative.parts[:-1], stem)


def count_files(path: Path) -> int:
    if not path.exists():
        return 0
    return sum(1 for child in path.rglob("*") if child.is_file())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".")
    parser.add_argument(
        "--install-root",
        default="Unleashed Recomp - Windows (Complete Installation) 1.0.3",
    )
    parser.add_argument("--tool", default="external_tools/HedgeArcPack/HedgeArcPack.exe")
    parser.add_argument("--output-root", default="extracted_assets/full_install_archives")
    parser.add_argument("--manifest", default="research_uiux/data/full_install_archive_extraction_manifest.json")
    parser.add_argument("--log", default="research_uiux/data/full_install_archive_extraction_log.jsonl")
    parser.add_argument("--limit", type=int, default=0, help="Debug limit; 0 extracts all archives.")
    parser.add_argument("--force", action="store_true", help="Re-run extraction even if output has files.")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    install_root = resolve_path(repo_root, args.install_root).resolve()
    tool = resolve_path(repo_root, args.tool).resolve()
    output_root = resolve_path(repo_root, args.output_root).resolve()
    manifest_path = resolve_path(repo_root, args.manifest).resolve()
    log_path = resolve_path(repo_root, args.log).resolve()

    if not install_root.exists():
        raise FileNotFoundError(f"Install root not found: {install_root}")
    if not tool.exists():
        raise FileNotFoundError(f"HedgeArcPack not found: {tool}")

    output_root.mkdir(parents=True, exist_ok=True)
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    log_path.parent.mkdir(parents=True, exist_ok=True)

    arl_paths = sorted(install_root.rglob("*.arl"))
    if args.limit > 0:
        arl_paths = arl_paths[: args.limit]

    records: list[dict[str, object]] = []
    started_at = datetime.now(timezone.utc).isoformat()

    with log_path.open("a", encoding="utf-8") as log_file:
        for index, arl_path in enumerate(arl_paths, start=1):
            source_path = archive_source_for(arl_path)
            relative_arl = arl_path.relative_to(install_root).as_posix()
            record: dict[str, object] = {
                "index": index,
                "archive_count": len(arl_paths),
                "arl": relative_arl,
                "source": source_path.relative_to(install_root).as_posix() if source_path else None,
                "status": "pending",
                "output": None,
                "file_count": 0,
                "elapsed_seconds": 0.0,
            }

            if source_path is None:
                record["status"] = "missing_ar_segment"
                records.append(record)
                log_file.write(json.dumps(record, sort_keys=True) + "\n")
                log_file.flush()
                print(f"[{index}/{len(arl_paths)}] missing segment: {relative_arl}", flush=True)
                continue

            output_dir = output_dir_for(output_root, install_root, source_path)
            record["output"] = output_dir.relative_to(repo_root).as_posix()

            existing_count = count_files(output_dir)
            if existing_count > 0 and not args.force:
                record["status"] = "skipped_existing"
                record["file_count"] = existing_count
                records.append(record)
                log_file.write(json.dumps(record, sort_keys=True) + "\n")
                log_file.flush()
                print(f"[{index}/{len(arl_paths)}] skip existing: {relative_arl} ({existing_count} files)", flush=True)
                continue

            output_dir.mkdir(parents=True, exist_ok=True)
            command = [str(tool), str(source_path), str(output_dir), "-E"]
            start = time.monotonic()
            print(f"[{index}/{len(arl_paths)}] extract: {relative_arl}", flush=True)
            try:
                completed = subprocess.run(
                    command,
                    check=False,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    encoding="utf-8",
                    errors="replace",
                )
                record["elapsed_seconds"] = round(time.monotonic() - start, 3)
                record["file_count"] = count_files(output_dir)
                record["exit_code"] = completed.returncode
                record["tool_output_tail"] = completed.stdout[-4000:]
                record["status"] = "extracted" if completed.returncode == 0 else "failed"
            except Exception as exc:  # noqa: BLE001 - preserve extraction manifest detail.
                record["elapsed_seconds"] = round(time.monotonic() - start, 3)
                record["status"] = "exception"
                record["error"] = str(exc)

            records.append(record)
            log_file.write(json.dumps(record, sort_keys=True) + "\n")
            log_file.flush()

    summary_counts: dict[str, int] = {}
    total_files = 0
    for record in records:
        status = str(record["status"])
        summary_counts[status] = summary_counts.get(status, 0) + 1
        total_files += int(record.get("file_count", 0) or 0)

    manifest = {
        "started_at": started_at,
        "finished_at": datetime.now(timezone.utc).isoformat(),
        "install_root": install_root.as_posix(),
        "tool": tool.as_posix(),
        "output_root": output_root.relative_to(repo_root).as_posix(),
        "archive_count": len(arl_paths),
        "status_counts": dict(sorted(summary_counts.items())),
        "total_output_file_count_seen": total_files,
        "records": records,
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True), encoding="utf-8")

    print(manifest_path)
    print(f"archives={len(arl_paths)} statuses={manifest['status_counts']}")
    return 1 if summary_counts.get("failed") or summary_counts.get("exception") else 0


if __name__ == "__main__":
    raise SystemExit(main())
