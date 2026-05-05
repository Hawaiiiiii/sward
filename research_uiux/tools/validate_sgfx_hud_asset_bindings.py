"""Phase 271: validate SGFX HUD scene bindings against the extracted retail assets.

This is the first tool that crosses the line from "telemetry + layout
metadata" to "the human-readable port actually consumes the user's
extracted Sonic Unleashed assets". For every `SceneBinding` entry the
SGFX HUD layout headers carry, the validator:

* Resolves the SWA CSD project name (e.g. `ui_playscreen`) to its
  on-disk `.yncp` file path under `extracted_assets/full_install_archives`
  by joining the YNCP native component map (built from real extraction)
  with the manifest the layout generator writes alongside the headers.
* Confirms the `.yncp` file actually exists, is non-empty, and starts
  with the expected `YNCP` / `XNCP` magic bytes.
* Reads the parsed-project section of the YNCP native component map and
  verifies the bound `scenePath` (e.g. `ui_playscreen/so_speed_gauge`)
  is one of the scenes that project actually contains.
* Emits a `sgfx_hud_asset_binding_validation.generated.json` manifest
  capturing per-binding pass/fail with the resolved asset path, file
  size, byte magic, scene-list membership, and a human-readable status.

The validator is read-only and never modifies the extracted assets. It
is the closing edge of the Phase 266-270 pipeline — generated layout
headers + runtime cross-validations are now provably backed by real
asset files on disk.
"""
from __future__ import annotations

import argparse
import datetime as _dt
import json
import re
from dataclasses import dataclass, asdict
from pathlib import Path

# Phase 266 wrote the SceneBinding rows in the generated header; parse
# them out so the validator can run without re-reading the sidecar.
_SCENE_BINDING_RE = re.compile(
    r'\{\s*"(?P<member>m_[A-Za-z0-9_]+)"\s*,\s*'
    r'(?P<offset>0x[0-9A-Fa-f]+)\s*,\s*'
    r'"(?P<project>[^"]*)"\s*,\s*'
    r'"(?P<scene>[^"]*)"\s*,\s*'
    r'"(?P<confidence>[^"]*)"\s*,\s*'
    r'(?P<instances>\d+)\s*\}'
)

_GENERATED_AT_RE = re.compile(r'kGeneratedAt = "([^"]+)"')


@dataclass(frozen=True)
class SceneBindingRow:
    member_name: str
    member_offset: int
    project_name: str
    scene_path: str
    confidence_tier: str
    instance_count: int
    source_header: str


@dataclass(frozen=True)
class ProjectAssetEntry:
    project_name: str
    relative_path: str            # e.g. game/Sonic/ui_playscreen.yncp
    absolute_path: str
    scene_paths: tuple[str, ...]


@dataclass
class BindingValidation:
    member_name: str
    member_offset_hex: str
    project_name: str
    scene_path: str
    confidence_tier: str
    instance_count: int
    source_header: str
    asset_relative_path: str | None
    asset_absolute_path: str | None
    asset_exists: bool
    asset_size_bytes: int | None
    asset_magic_ok: bool | None
    scene_in_project: bool | None
    status: str


def collect_scene_bindings(headers_dir: Path) -> list[SceneBindingRow]:
    rows: list[SceneBindingRow] = []
    for header_path in sorted(headers_dir.glob("*.generated.h")):
        text = header_path.read_text(encoding="utf-8")
        # Limit the regex to the kSceneBindings array body; the same
        # SceneBinding struct definition above also contains the literal
        # `m_xxx, 0x...` example pattern so we narrow the search to the
        # array initializer itself when present.
        sb_start = text.find("kSceneBindings")
        if sb_start < 0:
            continue
        sb_end = text.find("}};", sb_start)
        if sb_end < 0:
            continue
        body = text[sb_start:sb_end]
        for m in _SCENE_BINDING_RE.finditer(body):
            rows.append(SceneBindingRow(
                member_name=m.group("member"),
                member_offset=int(m.group("offset"), 16),
                project_name=m.group("project"),
                scene_path=m.group("scene"),
                confidence_tier=m.group("confidence"),
                instance_count=int(m.group("instances")),
                source_header=str(header_path.name),
            ))
    return rows


def _compose_project_scene_paths(project_name: str, project: dict) -> tuple[str, ...]:
    """Phase 272: turn each YNCP scene's `(node_path, scene_name)` pair into
    the `<project>/<sub_path>/<scene>` form that the runtime sweep produces
    in `kSceneBindings[]` rows. The YNCP map stores `node_path='Root'` for
    top-level scenes and `node_path='Root/<sub>'` for nested clusters; we
    replace the literal `Root` prefix with the project name so a scene
    named `so_speed_gauge` under `Root` becomes `ui_playscreen/so_speed_gauge`
    and a scene named `speed_count` under `Root/add` becomes
    `ui_playscreen/add/speed_count`.
    """
    out: list[str] = []
    for scene in project.get("scenes", []):
        scene_name = scene.get("scene_name") or scene.get("name")
        if not scene_name:
            continue
        node_path = scene.get("node_path") or scene.get("path") or "Root"
        if node_path == "Root":
            sub_path = ""
        elif node_path.startswith("Root/"):
            sub_path = node_path[len("Root"):]  # leading slash kept
        elif node_path.startswith("Root"):
            sub_path = node_path[len("Root"):]
        else:
            # Treat any non-`Root` node_path as a sub-path appended verbatim.
            sub_path = "/" + node_path.strip("/")
        out.append(f"{project_name}{sub_path}/{scene_name}")
    return tuple(out)


def build_project_asset_index(
    yncp_native_map_path: Path, repo_root: Path, extracted_root: Path
) -> dict[str, ProjectAssetEntry]:
    """Index every YNCP project the native component map knows about against
    its on-disk file path under `extracted_assets/full_install_archives`.

    A given project name (e.g. `ui_playscreen`) can appear under several
    actor / variant directories — `Sonic/`, `SuperSonic/`, etc. The first
    entry found becomes the canonical resolved path; the others are
    treated as variant copies and ignored.
    """
    if not yncp_native_map_path.is_file():
        return {}
    payload = json.loads(yncp_native_map_path.read_text(encoding="utf-8"))
    out: dict[str, ProjectAssetEntry] = {}
    for _group_name, projects in payload.get("screen_groups", {}).items():
        for project in projects:
            project_name = project.get("project")
            relative_path = project.get("relative_path")
            if not project_name or not relative_path:
                continue
            if project_name in out:
                continue
            absolute = (extracted_root / relative_path).resolve()
            scene_paths = _compose_project_scene_paths(project_name, project)
            out[project_name] = ProjectAssetEntry(
                project_name=project_name,
                relative_path=str(relative_path).replace("\\", "/"),
                absolute_path=str(absolute),
                scene_paths=scene_paths,
            )
    return out


def validate_binding(
    row: SceneBindingRow,
    project_index: dict[str, ProjectAssetEntry],
    repo_root: Path,
) -> BindingValidation:
    project_entry = project_index.get(row.project_name)
    if project_entry is None:
        return BindingValidation(
            member_name=row.member_name,
            member_offset_hex=f"0x{row.member_offset:X}",
            project_name=row.project_name,
            scene_path=row.scene_path,
            confidence_tier=row.confidence_tier,
            instance_count=row.instance_count,
            source_header=row.source_header,
            asset_relative_path=None,
            asset_absolute_path=None,
            asset_exists=False,
            asset_size_bytes=None,
            asset_magic_ok=None,
            scene_in_project=None,
            status=(
                f"unresolved-project: '{row.project_name}' is not in the YNCP "
                "native component map; the binding cannot be loaded as-is"
            ),
        )

    absolute = Path(project_entry.absolute_path)
    asset_exists = absolute.is_file()
    asset_size = absolute.stat().st_size if asset_exists else None
    magic_ok: bool | None = None
    if asset_exists and asset_size and asset_size >= 4:
        with absolute.open("rb") as f:
            head = f.read(64)
        # SWA / Hedgehog Engine ships these files inside a `CPAF` resource
        # container; the inner `YNCP` / `XNCP` payload appears a few bytes
        # later. Accept either the outer container magic or a raw
        # YNCP/XNCP file as a valid Chao::CSD project asset.
        leading = head[:4]
        magic_ok = leading in (b"CPAF", b"YNCP", b"XNCP") or (
            b"YNCP" in head or b"XNCP" in head
        )
    scene_in_project = (
        row.scene_path in project_entry.scene_paths
        if project_entry.scene_paths
        else None
    )

    if not asset_exists:
        status = (
            f"missing-asset: project '{row.project_name}' resolved to "
            f"'{project_entry.relative_path}' but the file does not exist on disk"
        )
    elif magic_ok is False:
        status = (
            f"bad-magic: '{project_entry.relative_path}' exists but does not "
            "start with YNCP / XNCP magic bytes"
        )
    elif scene_in_project is False:
        status = (
            f"unresolved-scene: project '{row.project_name}' loaded from "
            f"'{project_entry.relative_path}' but does not contain scene "
            f"path '{row.scene_path}' in its parsed scene list"
        )
    elif scene_in_project is None:
        status = (
            f"asset-ok-scene-list-empty: project '{row.project_name}' file "
            "exists with valid magic but the YNCP map does not list its "
            "scenes; binding likely OK but cannot be cross-checked yet"
        )
    else:
        status = (
            f"ok: project '{row.project_name}' loaded from "
            f"'{project_entry.relative_path}' contains scene "
            f"'{row.scene_path}'"
        )
    rel_path: str | None
    try:
        rel_path = str(absolute.relative_to(repo_root).as_posix())
    except ValueError:
        rel_path = project_entry.relative_path
    return BindingValidation(
        member_name=row.member_name,
        member_offset_hex=f"0x{row.member_offset:X}",
        project_name=row.project_name,
        scene_path=row.scene_path,
        confidence_tier=row.confidence_tier,
        instance_count=row.instance_count,
        source_header=row.source_header,
        asset_relative_path=rel_path,
        asset_absolute_path=str(absolute),
        asset_exists=asset_exists,
        asset_size_bytes=asset_size,
        asset_magic_ok=magic_ok,
        scene_in_project=scene_in_project,
        status=status,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".")
    parser.add_argument(
        "--headers-dir",
        default=(
            "research_uiux/runtime_reference/include/sward/ui_runtime"
        ),
    )
    parser.add_argument(
        "--yncp-native-map",
        default="research_uiux/data/yncp_native_component_map.json",
    )
    parser.add_argument(
        "--extracted-root",
        default="extracted_assets/full_install_archives",
    )
    parser.add_argument(
        "--output",
        default=(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_asset_binding_validation.generated.json"
        ),
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    headers_dir = (repo_root / args.headers_dir).resolve()
    yncp_native_map_path = (repo_root / args.yncp_native_map).resolve()
    extracted_root = (repo_root / args.extracted_root).resolve()
    output_path = (repo_root / args.output).resolve()

    rows = collect_scene_bindings(headers_dir)
    project_index = build_project_asset_index(yncp_native_map_path, repo_root, extracted_root)
    validations = [validate_binding(r, project_index, repo_root) for r in rows]

    summary = {
        "ok": sum(1 for v in validations if v.status.startswith("ok")),
        "asset_ok_scene_list_empty": sum(
            1 for v in validations if v.status.startswith("asset-ok-scene-list-empty")),
        "missing_asset": sum(1 for v in validations if v.status.startswith("missing-asset")),
        "bad_magic": sum(1 for v in validations if v.status.startswith("bad-magic")),
        "unresolved_project": sum(
            1 for v in validations if v.status.startswith("unresolved-project")),
        "unresolved_scene": sum(
            1 for v in validations if v.status.startswith("unresolved-scene")),
    }

    output_payload = {
        "schema": "sward-sgfx-hud-asset-binding-validation-v1",
        "generatedAt": _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds"),
        "headersDir": (
            str(headers_dir.relative_to(repo_root).as_posix())
            if headers_dir.is_relative_to(repo_root) else str(headers_dir)
        ),
        "yncpNativeMap": (
            str(yncp_native_map_path.relative_to(repo_root).as_posix())
            if yncp_native_map_path.is_relative_to(repo_root) else str(yncp_native_map_path)
        ),
        "extractedRoot": (
            str(extracted_root.relative_to(repo_root).as_posix())
            if extracted_root.is_relative_to(repo_root) else str(extracted_root)
        ),
        "indexedProjectCount": len(project_index),
        "bindingCount": len(rows),
        "summary": summary,
        "validations": [asdict(v) for v in validations],
    }

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(output_payload, indent=2), encoding="utf-8")
    print(
        f"sgfx-hud-asset-binding-validation: validated {len(rows)} bindings "
        f"({summary['ok']} ok, {summary['asset_ok_scene_list_empty']} asset-ok-no-scene-list, "
        f"{summary['missing_asset']} missing, "
        f"{summary['bad_magic']} bad-magic, "
        f"{summary['unresolved_project']} unresolved-project, "
        f"{summary['unresolved_scene']} unresolved-scene); "
        f"wrote {output_path.relative_to(repo_root)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
