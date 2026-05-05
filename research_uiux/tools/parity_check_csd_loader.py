"""Phase 281: side-by-side parity validator for the C++ CSD project loader.

This is the answer to "how do we know the human-readable port reads the
exact same scenes Sonic Unleashed displays". For every retail CSD
project the YNCP native component map already knows about, the
validator:

* Spawns `sgfx_hud_csd_project_loader_smoke_test.exe --json <path>` (the
  Phase 275 / 279 / 280 C++ loader compiled via the Phase 278 wrapper).
* Parses the JSON the loader emits — root scene names, recursive scene
  references, project name, CPAF / NCPJ status.
* Compares the resulting `(node_path, scene_name)` set against the
  YNCP native component map's ground-truth set for the same project.

If every project shows zero diff, every screen the human-readable port
will display has been independently confirmed to load the same scenes
the recompiled SWA executable loads — proving 1:1 fidelity at the asset
level, not "inspired" or "reconstructed".

The C++ loader binary is built on demand by invoking the existing
PowerShell wrapper if it is not already present, so the validator runs
end-to-end from a clean checkout.
"""
from __future__ import annotations

import argparse
import datetime as _dt
import json
import subprocess
from dataclasses import dataclass, asdict
from pathlib import Path


@dataclass
class ParityEntry:
    project_name: str
    relative_path: str
    yncp_map_scene_count: int
    cpp_loader_scene_count: int
    only_in_yncp_map: list[str]
    only_in_cpp_loader: list[str]
    status: str


def _yncp_map_scene_set(project: dict) -> set[str]:
    """Compose the YNCP native component map's `(node_path, scene_name)`
    set into the same `<sub>/<scene>` shape the C++ loader emits.
    """
    out: set[str] = set()
    for scene in project.get("scenes", []) or []:
        scene_name = scene.get("scene_name") or scene.get("name")
        if not scene_name:
            continue
        node_path = scene.get("node_path") or scene.get("path") or "Root"
        if node_path == "Root":
            sub = ""
        elif node_path.startswith("Root/"):
            sub = node_path[len("Root/"):]
        elif node_path.startswith("Root"):
            sub = node_path[len("Root"):].lstrip("/")
        else:
            sub = node_path.lstrip("/")
        composed = f"{sub}/{scene_name}" if sub else scene_name
        out.add(composed)
    return out


def _cpp_loader_scene_set(loader_json: dict) -> set[str]:
    out: set[str] = set()
    for ref in loader_json.get("allSceneRefs", []) or []:
        name = ref.get("name", "")
        if not name:
            continue
        node_path = ref.get("nodePath", "") or ""
        composed = f"{node_path}/{name}" if node_path else name
        out.add(composed)
    return out


def ensure_smoke_binary(repo_root: Path, build_script: Path, smoke_exe: Path) -> None:
    if smoke_exe.is_file():
        return
    if not build_script.is_file():
        raise RuntimeError(f"Smoke build script not found: {build_script}")
    print(f"parity-check: building C++ smoke binary via {build_script.name}")
    result = subprocess.run(
        [
            "powershell",
            "-ExecutionPolicy", "Bypass",
            "-File", str(build_script),
            "-SkipRun",
        ],
        cwd=str(repo_root),
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"Smoke build script failed (exit {result.returncode}):\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}")
    if not smoke_exe.is_file():
        raise RuntimeError(
            f"Smoke build claimed success but binary still missing: {smoke_exe}")


def run_cpp_loader(smoke_exe: Path, asset_path: Path) -> dict:
    result = subprocess.run(
        [str(smoke_exe), "--json", str(asset_path)],
        capture_output=True,
        text=True,
    )
    # Exit code 0 => recognized magic. Even on non-zero we may still have
    # diagnostic JSON on stdout, so try to parse before deciding.
    try:
        return json.loads(result.stdout)
    except json.JSONDecodeError as exc:
        raise RuntimeError(
            f"Loader returned non-JSON output for {asset_path}:\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}") from exc


def evaluate_project(
    project: dict,
    extracted_root: Path,
    smoke_exe: Path,
) -> ParityEntry:
    project_name = project.get("project", "")
    relative_path = project.get("relative_path", "")
    yncp_set = _yncp_map_scene_set(project)
    asset_path = extracted_root / relative_path
    if not asset_path.is_file():
        return ParityEntry(
            project_name=project_name,
            relative_path=relative_path,
            yncp_map_scene_count=len(yncp_set),
            cpp_loader_scene_count=0,
            only_in_yncp_map=sorted(yncp_set),
            only_in_cpp_loader=[],
            status=(
                f"missing-asset: {relative_path} not present under "
                f"extracted_root={extracted_root}"
            ),
        )
    try:
        loader_json = run_cpp_loader(smoke_exe, asset_path)
    except RuntimeError as exc:
        return ParityEntry(
            project_name=project_name,
            relative_path=relative_path,
            yncp_map_scene_count=len(yncp_set),
            cpp_loader_scene_count=0,
            only_in_yncp_map=sorted(yncp_set),
            only_in_cpp_loader=[],
            status=f"loader-error: {exc}",
        )

    cpp_set = _cpp_loader_scene_set(loader_json)
    only_in_yncp = sorted(yncp_set - cpp_set)
    only_in_cpp = sorted(cpp_set - yncp_set)
    if not only_in_yncp and not only_in_cpp:
        status = (
            f"parity-ok: both paths agree on {len(yncp_set)} scenes for "
            f"project '{project_name}'"
        )
    else:
        status = (
            f"parity-mismatch: project '{project_name}' has "
            f"{len(only_in_yncp)} YNCP-map-only and "
            f"{len(only_in_cpp)} C++-loader-only scene(s)"
        )
    return ParityEntry(
        project_name=project_name,
        relative_path=relative_path,
        yncp_map_scene_count=len(yncp_set),
        cpp_loader_scene_count=len(cpp_set),
        only_in_yncp_map=only_in_yncp,
        only_in_cpp_loader=only_in_cpp,
        status=status,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".")
    parser.add_argument(
        "--yncp-native-map",
        default="research_uiux/data/yncp_native_component_map.json",
    )
    parser.add_argument(
        "--extracted-root",
        default="extracted_assets/full_install_archives",
    )
    parser.add_argument(
        "--build-script",
        default="research_uiux/runtime_reference/tools/build_sgfx_hud_smoke_tests.ps1",
    )
    parser.add_argument(
        "--smoke-exe",
        default="out/sgfx_hud_smoke_tests/sgfx_hud_csd_project_loader_smoke_test.exe",
    )
    parser.add_argument(
        "--output",
        default="research_uiux/runtime_reference/include/sward/ui_runtime/sgfx_hud_loader_parity_report.generated.json",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Do not invoke the build script even if the smoke binary is missing.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    yncp_map_path = (repo_root / args.yncp_native_map).resolve()
    extracted_root = (repo_root / args.extracted_root).resolve()
    build_script = (repo_root / args.build_script).resolve()
    smoke_exe = (repo_root / args.smoke_exe).resolve()
    output_path = (repo_root / args.output).resolve()

    if not yncp_map_path.is_file():
        raise SystemExit(f"YNCP native component map missing: {yncp_map_path}")

    if not args.skip_build:
        ensure_smoke_binary(repo_root, build_script, smoke_exe)
    if not smoke_exe.is_file():
        raise SystemExit(
            f"Smoke binary not present and --skip-build was set: {smoke_exe}")

    payload = json.loads(yncp_map_path.read_text(encoding="utf-8"))
    seen_projects: set[tuple[str, str]] = set()
    entries: list[ParityEntry] = []
    for _group_name, projects in payload.get("screen_groups", {}).items():
        for project in projects:
            key = (project.get("project", ""), project.get("relative_path", ""))
            if key in seen_projects:
                continue
            seen_projects.add(key)
            entries.append(evaluate_project(project, extracted_root, smoke_exe))

    summary = {
        "parity_ok": sum(1 for e in entries if e.status.startswith("parity-ok")),
        "parity_mismatch": sum(1 for e in entries if e.status.startswith("parity-mismatch")),
        "missing_asset": sum(1 for e in entries if e.status.startswith("missing-asset")),
        "loader_error": sum(1 for e in entries if e.status.startswith("loader-error")),
    }

    report = {
        "schema": "sward-sgfx-hud-loader-parity-report-v1",
        "generatedAt": _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds"),
        "repoRoot": str(repo_root.as_posix()),
        "yncpNativeMap": str(yncp_map_path.relative_to(repo_root).as_posix()),
        "extractedRoot": str(extracted_root.relative_to(repo_root).as_posix())
            if extracted_root.is_relative_to(repo_root) else str(extracted_root),
        "smokeExe": str(smoke_exe.relative_to(repo_root).as_posix())
            if smoke_exe.is_relative_to(repo_root) else str(smoke_exe),
        "projectCount": len(entries),
        "summary": summary,
        "entries": [asdict(e) for e in entries],
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(
        f"parity-check: {len(entries)} projects evaluated; "
        f"ok={summary['parity_ok']} "
        f"mismatch={summary['parity_mismatch']} "
        f"missing={summary['missing_asset']} "
        f"loader-error={summary['loader_error']}; "
        f"wrote {output_path.relative_to(repo_root)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
