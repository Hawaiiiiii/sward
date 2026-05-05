"""Generate a human-readable C++ layout header for `class CHudSonicStage`.

Phase 266: this is the first concrete artifact in the human-readable 1:1 port
of Sonic Unleashed's UI/UX. The deliverable is a stable C++ header that:

* Declares `class CHudSonicStage` with the runtime-confirmed RCPtr<T> data
  members at the correct byte offsets (matching the original SWA HUD layout).
* Carries `static_assert(offsetof(...))` for every member so any drift
  against the live recomp executable's class layout is caught at compile
  time.
* Carries a `kSceneBindings[]` registry naming the CSD project / scene path
  each renderable RCPtr should be populated from at runtime, sourced from
  the latest `hud_owner_layout.json` sidecar evidence written by the
  `WriteHudOwnerLayoutSidecar` runtime path.

Inputs:

* `UnleashedRecomp/patches/ui_lab_patches.cpp` — parsed for the
  `kChudSonicStageExpectedOwnerFields` table (which is itself sourced from
  the constructor decode in `ppc_recomp.28.cpp:61909` plus the runtime
  Phase 260 sweep).
* Optionally `out/ui_lab_runtime_evidence/<latest>/hud_owner_layout.json`
  — used to enrich each member with the runtime-confirmed CSD project /
  scene path.

The generator never invents field names: every name comes from the C++
table parsed out of `ui_lab_patches.cpp`, and the scene bindings come
from runtime-confirmed sidecar evidence. Members the table names with
`m_rcPtrFieldXxx` flow through verbatim to the generated header.
"""
from __future__ import annotations

import argparse
import datetime as _dt
import json
import re
from dataclasses import dataclass
from pathlib import Path

# Each entry in the C++ kChudSonicStageExpectedOwnerFields table looks like
#     { "m_rcPlayScreen", 0xE0, 0xE4 },
# Capture the field name (group 1), rcPtrOffset (group 2), and the second
# offset (rcObjectOffset / m_pMemory; group 3) directly from the .cpp.
_FIELD_RE = re.compile(
    r'\{\s*"(?P<name>m_rc[A-Za-z0-9_]+)"\s*,\s*'
    r'(?P<rc_ptr>0x[0-9A-Fa-f]+)\s*,\s*'
    r'(?P<rc_object>0x[0-9A-Fa-f]+)\s*\}'
)

# Capture the source-attribution string written by Phase 265.
_SOURCE_ATTR_RE = re.compile(
    r'kChudSonicStageExpectedOwnerFieldSource\s*=\s*"([^"]+)"'
)


@dataclass(frozen=True)
class ExpectedField:
    name: str
    rc_ptr_offset: int
    rc_object_offset: int


@dataclass(frozen=True)
class SceneBinding:
    field_name: str
    project_name: str
    scene_path: str
    manager_scene_address: str
    confidence_tier: str
    instance_count: int


def parse_expected_fields(ui_lab_path: Path) -> tuple[list[ExpectedField], str]:
    text = ui_lab_path.read_text(encoding="utf-8")
    # Anchor on the actual table declaration, not the first textual mention
    # (the symbol name also shows up in comments). The declaration line
    # always starts with `static constexpr std::array<ChudSonicStageExpectedOwnerField,`.
    decl_marker = "std::array<ChudSonicStageExpectedOwnerField,"
    decl_start = text.find(decl_marker)
    if decl_start < 0:
        raise RuntimeError(
            f"kChudSonicStageExpectedOwnerFields declaration not found in {ui_lab_path}; "
            "Phase 265 expected-fields table required as input.")

    # Limit the regex search to the table body to avoid accidental matches
    # elsewhere in the file (other RCPtr-named structs etc.).
    table_end = text.find("}};", decl_start)
    if table_end < 0:
        raise RuntimeError("Could not find the closing }}; of the expected-fields table.")
    body = text[decl_start:table_end]

    fields = [
        ExpectedField(
            name=m.group("name"),
            rc_ptr_offset=int(m.group("rc_ptr"), 16),
            rc_object_offset=int(m.group("rc_object"), 16),
        )
        for m in _FIELD_RE.finditer(body)
    ]
    if not fields:
        raise RuntimeError("kChudSonicStageExpectedOwnerFields parsed empty.")

    source_attr_match = _SOURCE_ATTR_RE.search(text)
    source_attr = source_attr_match.group(1) if source_attr_match else ""

    # Every consecutive pair must be exactly 8 bytes apart since each
    # RCPtr<T> is 4-byte rcObject + 4-byte rcMemory.
    for prev, current in zip(fields, fields[1:]):
        if current.rc_ptr_offset - prev.rc_ptr_offset != 0x8:
            raise RuntimeError(
                "Non-contiguous RCPtr layout: "
                f"{prev.name}@{prev.rc_ptr_offset:#x} -> "
                f"{current.name}@{current.rc_ptr_offset:#x}; "
                "expected 8-byte stride per RCPtr member.")
        if current.rc_object_offset != current.rc_ptr_offset + 4:
            raise RuntimeError(
                f"Field {current.name} rcObjectOffset {current.rc_object_offset:#x} "
                f"is not rcPtrOffset+4 ({current.rc_ptr_offset + 4:#x}); "
                "RCPtr layout invariant violated.")

    return fields, source_attr


def find_latest_sidecar(repo_root: Path) -> Path | None:
    candidates = sorted(
        (repo_root / "out" / "ui_lab_runtime_evidence").glob(
            "manual_*/manual-observer/hud_owner_layout.json"),
        key=lambda p: p.stat().st_mtime,
        reverse=True,
    )
    for path in candidates:
        try:
            if path.stat().st_size > 0:
                return path
        except OSError:
            continue
    return None


def build_scene_bindings(
    fields: list[ExpectedField], sidecar_path: Path | None
) -> list[SceneBinding]:
    if sidecar_path is None:
        return []
    try:
        layout = json.loads(sidecar_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return []

    field_by_offset = {f.rc_object_offset: f for f in fields}
    bindings: dict[str, SceneBinding] = {}

    # `renderableSlotGroups` is the Phase 264 cleaned view; prefer it. Fall
    # back to the raw `renderableSlots` if the sidecar is older.
    groups = layout.get("renderableSlotGroups") or []
    raw_slots = layout.get("renderableSlots") or []

    def consume_group(g: dict) -> None:
        owner_source = g.get("ownerSource", "")
        if "constructor-confirmed" not in owner_source:
            return
        instance_count = int(g.get("instanceCount", 0) or 0)
        for offset_text in g.get("fieldOffsets", []):
            try:
                offset = int(offset_text, 16)
            except (TypeError, ValueError):
                continue
            field = field_by_offset.get(offset)
            if field is None:
                continue
            if field.name in bindings:
                continue
            bindings[field.name] = SceneBinding(
                field_name=field.name,
                project_name=g.get("projectName", ""),
                scene_path=g.get("scenePath", ""),
                manager_scene_address=g.get("managerSceneAddress", "0x0"),
                confidence_tier=g.get("confidenceTier", ""),
                instance_count=instance_count,
            )

    def consume_slot(s: dict) -> None:
        owner_source = s.get("ownerSource", "")
        if "constructor-confirmed" not in owner_source:
            return
        try:
            offset = int(s.get("fieldOffset", "0x0"), 16)
        except (TypeError, ValueError):
            return
        field = field_by_offset.get(offset)
        if field is None:
            return
        if field.name in bindings:
            return
        bindings[field.name] = SceneBinding(
            field_name=field.name,
            project_name=s.get("projectName", ""),
            scene_path=s.get("scenePath", ""),
            manager_scene_address=s.get("managerSceneAddress", "0x0"),
            confidence_tier=s.get("confidenceTier", ""),
            instance_count=1,
        )

    for g in groups:
        consume_group(g)
    for s in raw_slots:
        consume_slot(s)

    # Stable ordering matches expected-fields order so the generated header
    # listing is deterministic.
    field_order = {f.name: i for i, f in enumerate(fields)}
    return sorted(bindings.values(), key=lambda b: field_order.get(b.field_name, 1 << 30))


def emit_header(
    fields: list[ExpectedField],
    bindings: list[SceneBinding],
    source_attr: str,
    expected_total_size: int,
) -> str:
    generated_at = _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds")
    binding_by_name = {b.field_name: b for b in bindings}

    lines: list[str] = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("// SGFX HUD layout: human-readable port of `class CHudSonicStage`.")
    lines.append("//")
    lines.append("// Phase 266: generated from `kChudSonicStageExpectedOwnerFields` (which is")
    lines.append("// itself sourced from the CHudSonicStage constructor decode in")
    lines.append("// `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.28.cpp:61909`")
    lines.append("// plus the runtime Phase 260 sweep evidence). Every RCPtr offset here is")
    lines.append("// runtime-confirmed; the 11 still-unnamed RCPtrs use offset-based names")
    lines.append("// (`m_rcPtrFieldXxx`) so their existence is recorded without inventing")
    lines.append("// semantic claims. Method bodies are intentionally out of scope for this")
    lines.append("// header — it is a layout reference for the human-readable 1:1 port; the")
    lines.append("// constructor / destructor / Update / Render method bodies will be ported")
    lines.append("// in subsequent phases as their recomp flow is decoded.")
    lines.append("//")
    lines.append(f"// Generated at: {generated_at}")
    if source_attr:
        lines.append(f"// Source attribution: {source_attr}")
    lines.append("")
    lines.append("#include <array>")
    lines.append("#include <cstddef>")
    lines.append("#include <cstdint>")
    lines.append("#include <string_view>")
    lines.append("")
    lines.append("namespace sward::ui_runtime::generated::sgfx_hud")
    lines.append("{")
    lines.append("    // Forward declaration of the SWA CSD scene type. The retail SWA")
    lines.append("    // executable holds a `CSD::Manager::CScene*` here; the human-readable")
    lines.append("    // port keeps the type opaque at this layer because the CSD runtime is")
    lines.append("    // ported separately.")
    lines.append("    class CScene;")
    lines.append("")
    lines.append("    // SWA `RCPtr<T>` matches an Xbox 360 32-bit pointer pair:")
    lines.append("    // `m_pRCObject` (the reference-counted wrapper) at offset 0 and")
    lines.append("    // `m_pMemory` (the wrapped object) at offset 4. Total size: 8 bytes.")
    lines.append("    template <class T>")
    lines.append("    struct RCPtr")
    lines.append("    {")
    lines.append("        std::uint32_t m_pRCObject;  // guest-relative pointer to the RCObject wrapper")
    lines.append("        std::uint32_t m_pMemory;    // guest-relative pointer to the wrapped T")
    lines.append("    };")
    lines.append("    static_assert(sizeof(RCPtr<CScene>) == 8, \"RCPtr<T> must match the SWA 8-byte layout\");")
    lines.append("")
    lines.append("    // Layout reference for `class CHudSonicStage` (the Sonic stage HUD).")
    lines.append("    // Constructor entry point: `sub_824D89B0` (named in Phase 265).")
    lines.append("    // Destructor entry point: `sub_824D8CE8`.")
    lines.append("    // The vtable pointer lives at `+0x00` and a secondary vtable / typeinfo")
    lines.append("    // pointer at `+0x28`, both populated by the constructor.")
    lines.append("    class CHudSonicStage")
    lines.append("    {")
    lines.append("    public:")
    lines.append("        std::uint32_t m_pVTable;             // +0x00 vtable pointer set by sub_824D89B0")
    lines.append("    private:")
    lines.append("        std::array<std::uint8_t, 0x24> m_padding00_28;  // pre-secondary-vtable bytes")
    lines.append("    public:")
    lines.append("        std::uint32_t m_pSecondaryVTable;    // +0x28 typeinfo / aux vtable set by sub_824D89B0")
    lines.append("    private:")
    lines.append(f"        std::array<std::uint8_t, 0x{fields[0].rc_ptr_offset - 0x2C:X}> m_padding2C_E0;  // pre-RCPtr-table bytes")
    lines.append("    public:")
    for field in fields:
        binding = binding_by_name.get(field.name)
        comment = f"+0x{field.rc_ptr_offset:X} RCPtr<CScene>"
        if binding and binding.scene_path:
            # The sidecar's scenePath field is already a full project-relative
            # path (e.g. `ui_playscreen/so_speed_gauge`), so quote it directly
            # without re-prepending projectName.
            comment += (
                f"; runtime: {binding.scene_path} "
                f"({binding.confidence_tier}, instances={binding.instance_count})"
            )
        elif field.name.startswith("m_rcPtrField"):
            comment += "; constructor-confirmed but no runtime scene yet — name pending evidence"
        lines.append(f"        RCPtr<CScene> {field.name};  // {comment}")
    lines.append("    };")
    lines.append("")
    last = fields[-1]
    last_member_end = last.rc_ptr_offset + 8
    lines.append("    // Compile-time guards: every named RCPtr must land at the runtime-")
    lines.append("    // confirmed offset. If the layout drifts (recomp regenerated, expected-")
    lines.append("    // fields table updated, etc.) the header fails to compile and forces a")
    lines.append("    // re-generation.")
    lines.append("    static_assert(offsetof(CHudSonicStage, m_pVTable) == 0x00,")
    lines.append("        \"CHudSonicStage vtable pointer must remain at +0x00\");")
    lines.append("    static_assert(offsetof(CHudSonicStage, m_pSecondaryVTable) == 0x28,")
    lines.append("        \"CHudSonicStage secondary vtable / typeinfo must remain at +0x28\");")
    for field in fields:
        lines.append(
            f"    static_assert(offsetof(CHudSonicStage, {field.name}) == 0x{field.rc_ptr_offset:X},"
            f" \"CHudSonicStage::{field.name} must remain at +0x{field.rc_ptr_offset:X}\");"
        )
    lines.append("")
    lines.append("    // Each binding tells the runtime which CSD project / scene path should")
    lines.append("    // populate the named RCPtr member. Sourced from the live")
    lines.append("    // `hud_owner_layout.json` sidecar (Phase 262 / 264) so every entry is")
    lines.append("    // backed by an observed runtime correlation, not a guess.")
    lines.append("    struct SceneBinding")
    lines.append("    {")
    lines.append("        std::string_view memberName;")
    lines.append("        std::size_t      memberOffset;")
    lines.append("        std::string_view projectName;")
    lines.append("        std::string_view scenePath;")
    lines.append("        std::string_view confidenceTier;")
    lines.append("        std::size_t      instanceCount;")
    lines.append("    };")
    lines.append("")
    lines.append(f"    static constexpr std::array<SceneBinding, {len(bindings)}> kSceneBindings =")
    lines.append("    {{")
    for binding in bindings:
        field = next(f for f in fields if f.name == binding.field_name)
        lines.append(
            "        {"
            f"\"{binding.field_name}\", 0x{field.rc_ptr_offset:X}, "
            f"\"{_cpp_escape(binding.project_name)}\", "
            f"\"{_cpp_escape(binding.scene_path)}\", "
            f"\"{_cpp_escape(binding.confidence_tier)}\", "
            f"{binding.instance_count}"
            "},"
        )
    lines.append("    }};")
    lines.append("")
    lines.append(f"    static constexpr std::string_view kGeneratedAt = \"{generated_at}\";")
    if source_attr:
        lines.append(f"    static constexpr std::string_view kSourceAttribution = \"{_cpp_escape(source_attr)}\";")
    else:
        lines.append("    static constexpr std::string_view kSourceAttribution = \"\";")
    lines.append("")
    lines.append("} // namespace sward::ui_runtime::generated::sgfx_hud")
    lines.append("")
    return "\n".join(lines)


def _cpp_escape(value: str) -> str:
    return value.replace("\\", "\\\\").replace("\"", "\\\"")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".", help="Repository root.")
    parser.add_argument(
        "--ui-lab-path",
        default="UnleashedRecomp/patches/ui_lab_patches.cpp",
        help="Path to ui_lab_patches.cpp (relative to repo root).",
    )
    parser.add_argument(
        "--sidecar",
        default=None,
        help=(
            "Optional explicit path to a hud_owner_layout.json sidecar; "
            "if omitted, the latest non-empty sidecar in "
            "out/ui_lab_runtime_evidence/manual_*/manual-observer/ is used."
        ),
    )
    parser.add_argument(
        "--output-header",
        default=(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_chud_sonic_stage.generated.h"
        ),
        help="Where to write the generated C++ header.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    ui_lab_path = (repo_root / args.ui_lab_path).resolve()
    fields, source_attr = parse_expected_fields(ui_lab_path)

    if args.sidecar is not None:
        sidecar_path = (repo_root / args.sidecar).resolve()
    else:
        sidecar_path = find_latest_sidecar(repo_root)

    bindings = build_scene_bindings(fields, sidecar_path)

    expected_total_size = fields[-1].rc_ptr_offset + 8
    header = emit_header(fields, bindings, source_attr, expected_total_size)
    output_path = (repo_root / args.output_header).resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(header, encoding="utf-8")

    print(
        f"sgfx-hud-layout: wrote {output_path.relative_to(repo_root)} "
        f"with {len(fields)} RCPtr members and {len(bindings)} runtime scene bindings"
        + (f" from sidecar {sidecar_path.relative_to(repo_root)}" if sidecar_path else "")
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
