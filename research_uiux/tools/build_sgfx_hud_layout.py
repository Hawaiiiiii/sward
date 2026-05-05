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

# Phase 267: capture `RCPtr<T> m_xxx;` member declarations from the
# UnleashedRecomp team's existing SWA API headers (e.g.
# `api/SWA/HUD/Sonic/HudSonicStage.h`). The headers may write either the
# fully-qualified `Chao::CSD::RCPtr<Chao::CSD::CScene>` form or the brief
# `RCPtr<CScene>` form (the Pause header opens with `using namespace
# Chao::CSD;`); both must round-trip to the same template-argument type
# name so we can render `Chao::CSD::RCPtr<Chao::CSD::CScene>` consistently
# in the generated port header.
_SWA_RCPTR_DECL_RE = re.compile(
    r'(?:Chao::CSD::)?RCPtr<\s*(?:Chao::CSD::)?(?P<inner>[A-Za-z0-9_:]+)\s*>\s+'
    r'(?P<name>m_rc[A-Za-z0-9_]+)\s*;'
)

# Phase 268: capture every `SWA_ASSERT_OFFSETOF(Class, m_member, 0xN);`
# entry — the UnleashedRecomp team's authoritative offsets for a given SWA
# class. The asserts live just below the `class` body in the SWA API
# header so we use them as the ground truth for member offsets when laying
# out the corresponding generated port class.
_SWA_OFFSETOF_RE = re.compile(
    r'SWA_ASSERT_OFFSETOF\(\s*(?P<class>[A-Za-z0-9_]+)\s*,\s*'
    r'(?P<member>m_[A-Za-z0-9_]+)\s*,\s*(?P<offset>0x[0-9A-Fa-f]+)\s*\)'
)

# Phase 268: capture the `class CXxx [: [public] [Namespace::]CYyy]` line.
# CHudPause inherits `: public CGameObject`; CHudSonicStage has no base;
# CSaveIcon inherits `: Hedgehog::Universe::CUpdateUnit` (multi-namespace
# base, default access). All three must parse cleanly.
_SWA_CLASS_DECL_RE = re.compile(
    r'class\s+(?P<class>C[A-Za-z0-9_]+)\s*'
    r'(?:\:\s*(?:public\s+)?(?P<base>[A-Za-z0-9_]+(?:::[A-Za-z0-9_]+)*))?\s*\{'
)

# Phase 268 / 269: capture `enum EXxx [: uint32_t] { ... };` definitions.
# The underlying type is optional because some SWA enums (e.g.
# `ELoadingDisplayType`) omit it and rely on the C++ default `int`.
_SWA_ENUM_DEF_RE = re.compile(
    r'enum\s+(?P<name>E[A-Za-z0-9_]+)\s*'
    r'(?:\:\s*(?P<underlying>[A-Za-z0-9_:]+)\s*)?'
    r'\{(?P<body>[^}]*)\}\s*;',
    re.DOTALL,
)

# Phase 268: capture non-RCPtr scalar members inside the class body so
# the generated port class can emit them with their authoritative type.
# Examples that must match: `bool m_IsVisible;`,
# `be<EActionType> m_Action;`, `be<uint32_t> m_Submenu;`.
_SWA_SCALAR_MEMBER_RE = re.compile(
    r'^[ \t]+(?P<type>(?:be<[A-Za-z0-9_:]+>|bool|float|double|std::uint8_t|'
    r'std::uint16_t|std::uint32_t|std::uint64_t|std::int8_t|std::int16_t|'
    r'std::int32_t|std::int64_t|uint8_t|uint16_t|uint32_t|uint64_t|'
    r'int8_t|int16_t|int32_t|int64_t))\s+(?P<name>m_[A-Za-z0-9_]+)\s*;',
    re.MULTILINE,
)


@dataclass(frozen=True)
class SwaApiMember:
    name: str
    decl_type: str          # full C++ type as written in the SWA API header
    rcptr_inner_type: str | None  # "CProject" / "CScene" / "CNode" if RCPtr, else None
    offset: int             # absolute offset within the class, from SWA_ASSERT_OFFSETOF


@dataclass(frozen=True)
class SwaApiEnum:
    name: str
    underlying_type: str
    values: tuple[tuple[str, int | None], ...]


@dataclass(frozen=True)
class SwaApiClass:
    namespace: str
    class_name: str
    base_class: str | None
    members: tuple[SwaApiMember, ...]   # ordered by offset
    enums: tuple[SwaApiEnum, ...]
    swa_api_header_relpath: str


def _strip_block_comments(text: str) -> str:
    return re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)


# Phase 274: scan a class body for the cumulative byte offset of each
# named member by walking `SWA_INSERT_PADDING(N);`, RCPtr<T> declarations,
# and scalar declarations in source order. Used as a fallback when the
# SWA API header carries no `SWA_ASSERT_OFFSETOF` lines.
_SWA_PADDING_DIRECTIVE_RE = re.compile(r'SWA_INSERT_PADDING\s*\(\s*(0x[0-9A-Fa-f]+|\d+)\s*\)\s*;')


def _infer_offsets_from_swa_padding(class_body: str) -> dict[str, int]:
    cursor = 0
    offsets: dict[str, int] = {}
    for line in class_body.splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        pad_match = _SWA_PADDING_DIRECTIVE_RE.search(stripped)
        if pad_match is not None:
            cursor += int(pad_match.group(1), 0)
            continue
        rcptr_match = _SWA_RCPTR_DECL_RE.search(stripped)
        if rcptr_match is not None:
            offsets[rcptr_match.group("name")] = cursor
            cursor += 8
            continue
        scalar_match = _SWA_SCALAR_MEMBER_RE.match(line)
        if scalar_match is not None:
            offsets[scalar_match.group("name")] = cursor
            decl_type = scalar_match.group("type")
            if decl_type == "bool" or decl_type.endswith("8_t"):
                cursor += 1
            elif decl_type.endswith("16_t"):
                cursor += 2
            elif decl_type.endswith("64_t") or decl_type == "double":
                cursor += 8
            else:
                cursor += 4
            continue
        # Anything else (access specifiers, comments, blank lines) does
        # not advance the offset cursor.
    return offsets


def _strip_line_comments(text: str) -> str:
    return re.sub(r'//[^\n]*', '', text)


def _enumerator_values(body: str) -> tuple[tuple[str, int | None], ...]:
    cleaned = _strip_block_comments(_strip_line_comments(body))
    out: list[tuple[str, int | None]] = []
    for raw in cleaned.split(","):
        token = raw.strip()
        if not token:
            continue
        if "=" in token:
            name_part, value_part = token.split("=", 1)
            name = name_part.strip()
            try:
                value: int | None = int(value_part.strip(), 0)
            except ValueError:
                value = None
        else:
            name = token
            value = None
        if name:
            out.append((name, value))
    return tuple(out)


def parse_swa_api_class(api_header_path: Path, repo_root: Path) -> SwaApiClass | None:
    """Parse a single-class SWA API HUD header into a structured SwaApiClass.

    The parser is deliberately scoped to the SWA HUD header pattern (one
    class per file, RCPtr/scalar members + SWA_INSERT_PADDING + a trailing
    block of SWA_ASSERT_OFFSETOF lines). Returns None if the header does
    not match that pattern; the caller decides whether that's an error.
    """
    if not api_header_path.is_file():
        return None
    raw_text = api_header_path.read_text(encoding="utf-8")
    text = _strip_line_comments(_strip_block_comments(raw_text))

    class_match = _SWA_CLASS_DECL_RE.search(text)
    if class_match is None:
        return None
    class_name = class_match.group("class")
    base_class = class_match.group("base")

    # Class body runs from the opening `{` to the matching `}` immediately
    # before the closing `};`. The SWA API headers have no nested types so
    # a depth-1 brace counter from the class opening is adequate.
    body_start = class_match.end() - 1  # back to the `{`
    depth = 0
    body_end = body_start
    for i in range(body_start, len(text)):
        ch = text[i]
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                body_end = i
                break
    if body_end <= body_start:
        return None
    class_body = text[body_start:body_end + 1]

    # Offset table — read every SWA_ASSERT_OFFSETOF entry that targets this class.
    offsets: dict[str, int] = {}
    for m in _SWA_OFFSETOF_RE.finditer(text):
        if m.group("class") != class_name:
            continue
        offsets[m.group("member")] = int(m.group("offset"), 16)
    # Phase 274: a few SWA API headers (notably SaveIcon.h) declare members
    # but no `SWA_ASSERT_OFFSETOF` entries — the SWA team relied on the
    # `SWA_INSERT_PADDING(N)` declarations alone. When that happens, fall
    # back to a sequential layout walk through the class body that uses
    # the SWA_INSERT_PADDING values verbatim and assigns each member its
    # cumulative offset starting at 0. This matches the SWA team's
    # implicit convention that `SWA_INSERT_PADDING` is measured from the
    # start of the class itself (the recomp source confirms this for
    # CSaveIcon: `lwz r3, 216(r31)` reads `m_IsVisible` at +0xD8).
    if not offsets:
        offsets = _infer_offsets_from_swa_padding(class_body)
        if not offsets:
            return None

    # Members: collect RCPtr<T> declarations and scalar declarations from
    # the class body and pair each with its authoritative offset.
    members: list[SwaApiMember] = []
    for m in _SWA_RCPTR_DECL_RE.finditer(class_body):
        name = m.group("name")
        if name not in offsets:
            continue
        inner = m.group("inner")
        members.append(SwaApiMember(
            name=name,
            decl_type=f"Chao::CSD::RCPtr<Chao::CSD::{inner}>",
            rcptr_inner_type=inner,
            offset=offsets[name],
        ))
    for m in _SWA_SCALAR_MEMBER_RE.finditer(class_body):
        name = m.group("name")
        if name not in offsets:
            continue
        members.append(SwaApiMember(
            name=name,
            decl_type=m.group("type"),
            rcptr_inner_type=None,
            offset=offsets[name],
        ))

    members.sort(key=lambda m: m.offset)
    if not members:
        return None

    enums: list[SwaApiEnum] = []
    for m in _SWA_ENUM_DEF_RE.finditer(text):
        # Phase 269: when the SWA API header writes `enum EXxx { ... }`
        # without an explicit underlying type the C++ default is `int`.
        # Preserve that intent in the generated header so the port matches
        # the SWA enum width exactly.
        underlying = m.group("underlying") or "int32_t"
        enums.append(SwaApiEnum(
            name=m.group("name"),
            underlying_type=underlying,
            values=_enumerator_values(m.group("body")),
        ))

    try:
        relpath = str(api_header_path.relative_to(repo_root).as_posix())
    except ValueError:
        relpath = str(api_header_path)

    return SwaApiClass(
        namespace="SWA",
        class_name=class_name,
        base_class=base_class,
        members=tuple(members),
        enums=tuple(enums),
        swa_api_header_relpath=relpath,
    )


def emit_swa_api_class_header(spec: SwaApiClass) -> str:
    """Emit the human-readable port C++ header for a class fully sourced from
    the UnleashedRecomp SWA API header (no ui_lab table extension needed).

    Produces: namespace, forward decls, RCPtr template defn, namespace
    alias, enum declarations, the class itself with byte-padded layout,
    and one static_assert per SWA_ASSERT_OFFSETOF entry. Output structure
    mirrors `emit_header` so downstream consumers see a consistent layout
    regardless of which generator path produced the file.
    """
    generated_at = _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds")
    rcptr_inner_types = sorted({
        m.rcptr_inner_type for m in spec.members if m.rcptr_inner_type
    })
    forward_types = list(rcptr_inner_types)
    if "CScene" not in forward_types:
        forward_types.append("CScene")
    forward_types = sorted(set(forward_types))

    lines: list[str] = []
    lines.append("#pragma once")
    lines.append("")
    lines.append(f"// SGFX HUD layout: human-readable port of `class {spec.class_name}`.")
    lines.append("//")
    lines.append("// Phase 268: generated directly from the UnleashedRecomp SWA API header")
    lines.append(f"// `{spec.swa_api_header_relpath}` — that header already names every")
    lines.append("// member of this class with the authoritative SWA template-argument")
    lines.append("// types and pins each member offset via `SWA_ASSERT_OFFSETOF`. The")
    lines.append("// generator copies those offsets verbatim and pads between members so")
    lines.append("// `static_assert(offsetof(...))` continues to validate the layout at")
    lines.append("// compile time. Method bodies are intentionally out of scope; they")
    lines.append("// will be ported in subsequent phases as the recomp flow is decoded.")
    lines.append("//")
    lines.append(f"// Generated at: {generated_at}")
    if spec.base_class:
        lines.append(
            f"// SWA base class: {spec.base_class} (modeled here as leading byte padding "
            "rather than a real C++ base class to keep the layout self-contained).")
    lines.append("")
    lines.append("#include <array>")
    lines.append("#include <cstddef>")
    lines.append("#include <cstdint>")
    lines.append("#include <string_view>")
    lines.append("")
    lines.append("namespace sward::ui_runtime::generated::sgfx_hud")
    lines.append("{")
    lines.append("    // Forward declarations of the SWA CSD types referenced by this HUD")
    lines.append("    // class. The retail SWA executable holds the corresponding")
    lines.append("    // `Chao::CSD::*` types; the human-readable port keeps them opaque at")
    lines.append("    // this layer because the CSD runtime is ported separately.")
    for fwd in forward_types:
        lines.append(f"    class {fwd};")
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
    lines.append("    // The SWA executable references the wrapper as `Chao::CSD::RCPtr<T>`")
    lines.append("    // throughout the existing API headers; the `Chao::CSD::` alias here")
    lines.append("    // matches that convention so the human-readable port's member")
    lines.append("    // declarations read identically to the SWA originals.")
    lines.append("    namespace Chao { namespace CSD")
    lines.append("    {")
    lines.append("        template <class T> using RCPtr = ::sward::ui_runtime::generated::sgfx_hud::RCPtr<T>;")
    for fwd in forward_types:
        lines.append(f"        using {fwd} = ::sward::ui_runtime::generated::sgfx_hud::{fwd};")
    lines.append("    }} // namespace Chao::CSD")
    lines.append("")

    # SWA `be<T>` is a big-endian wrapper. Re-emit it here as a thin struct
    # that holds the same byte width as T so static_assert(sizeof(...))
    # continues to round-trip the SWA layout. The semantic decoding (host
    # endian conversion) belongs to a runtime layer ported separately.
    if any(m.decl_type.startswith("be<") for m in spec.members):
        lines.append("    // SWA `be<T>` is a thin big-endian wrapper around T; for layout")
        lines.append("    // purposes it is equivalent to T itself (same size and alignment).")
        lines.append("    // Endian decoding is the responsibility of a separate runtime")
        lines.append("    // layer ported alongside the rest of the SWA executable.")
        lines.append("    template <class T>")
        lines.append("    struct be")
        lines.append("    {")
        lines.append("        T m_storage;")
        lines.append("    };")
        lines.append("")

    # Enum declarations.
    for enum in spec.enums:
        lines.append(f"    enum class {enum.name} : std::{enum.underlying_type}")
        lines.append("    {")
        for i, (name, value) in enumerate(enum.values):
            comma = "," if i + 1 < len(enum.values) else ""
            if value is None:
                lines.append(f"        {name}{comma}")
            else:
                lines.append(f"        {name} = {value}{comma}")
        lines.append("    };")
        lines.append("")

    # The class itself: emit a flat layout with explicit byte padding
    # between members so each member lands at its SWA_ASSERT_OFFSETOF
    # offset.
    lines.append(f"    class {spec.class_name}")
    lines.append("    {")
    lines.append("    public:")

    # Phase 282: every member is `public:` so the class qualifies as
    # standard-layout per [class.prop]. That makes `offsetof` on the
    # paddings + members below well-defined under clang/MSVC and silences
    # the `-Winvalid-offsetof` warning the alternating-access version
    # triggered for every SWA_INSERT_PADDING / member alternation.
    cursor = 0
    for i, member in enumerate(spec.members):
        if member.offset > cursor:
            gap = member.offset - cursor
            lines.append(
                f"        std::array<std::uint8_t, 0x{gap:X}> m_padding{cursor:04X}_{member.offset:04X};"
                f"  // pre-{member.name} padding (covers SWA base class / SWA_INSERT_PADDING bytes)"
            )
        size_bytes = _swa_member_size_bytes(member)
        lines.append(
            f"        {_render_swa_member_decl(member)}  "
            f"// +0x{member.offset:X} {_swa_member_provenance_comment(member)}"
        )
        cursor = member.offset + size_bytes

    # Phase 270: emit inline accessor methods for the scalar / enum
    # members. These are the first method bodies in the human-readable
    # port — small, mechanical, and fully derivable from the SWA API
    # header. RCPtr members do not get accessors yet because reading
    # through `m_pMemory` requires the SWA RCObject runtime to be linked
    # in; that arrives in a later phase.
    accessor_lines = _render_scalar_accessor_methods(spec)
    if accessor_lines:
        lines.append("")
        lines.append("        // Phase 270: inline accessors for scalar / enum members.")
        lines.append("        // Mechanical translations of the SWA `be<T>` storage layout to")
        lines.append("        // the host-side semantic value. Derived purely from the SWA")
        lines.append("        // API header; no recomp method bodies are referenced.")
        for line in accessor_lines:
            lines.append(f"        {line}")
    lines.append("    };")
    lines.append("")

    # Compile-time guards.
    lines.append("    // Compile-time guards: every named member must land at the SWA-asserted")
    lines.append("    // offset. Drift against the live recomp executable's class layout breaks")
    lines.append("    // the build and forces a re-generation.")
    for member in spec.members:
        lines.append(
            f"    static_assert(offsetof({spec.class_name}, {member.name}) == 0x{member.offset:X},"
            f" \"{spec.class_name}::{member.name} must remain at +0x{member.offset:X}\");"
        )
    lines.append("")
    lines.append(f"    static constexpr std::string_view kGeneratedAt = \"{generated_at}\";")
    lines.append(
        f"    static constexpr std::string_view kSwaApiHeaderRelpath = "
        f"\"{_cpp_escape(spec.swa_api_header_relpath)}\";"
    )
    lines.append("")
    lines.append("} // namespace sward::ui_runtime::generated::sgfx_hud")
    lines.append("")
    return "\n".join(lines)


def _swa_member_size_bytes(member: SwaApiMember) -> int:
    if member.rcptr_inner_type is not None:
        return 8
    if member.decl_type == "bool":
        return 1
    if member.decl_type.startswith("be<"):
        # SWA `be<T>` is the size of T. The underlying T inside `be<>` is
        # the SWA enum or scalar; for the SWA HUD class enums we observe
        # `EActionType` etc. all use `uint32_t` underlying, and `be<uint32_t>`
        # is also 4 bytes. Default to 4 bytes which covers every observed
        # case in the existing SWA API headers.
        return 4
    if member.decl_type in {
        "std::uint8_t", "std::int8_t", "uint8_t", "int8_t",
    }:
        return 1
    if member.decl_type in {
        "std::uint16_t", "std::int16_t", "uint16_t", "int16_t",
    }:
        return 2
    if member.decl_type in {
        "std::uint32_t", "std::int32_t", "uint32_t", "int32_t", "float",
    }:
        return 4
    if member.decl_type in {
        "std::uint64_t", "std::int64_t", "uint64_t", "int64_t", "double",
    }:
        return 8
    # Conservative default for an unknown SWA scalar — assume 4 bytes
    # which matches the most common case (be<EnumType> / be<uint32_t>).
    return 4


def _render_swa_member_decl(member: SwaApiMember) -> str:
    return f"{member.decl_type} {member.name};"


def _swa_member_provenance_comment(member: SwaApiMember) -> str:
    if member.rcptr_inner_type is not None:
        return f"{member.decl_type} (type from SWA API header)"
    return f"{member.decl_type} (type from SWA API header)"


# Phase 270: derive a getter name from a member name. `m_IsVisible` →
# `isVisible`, `m_Action` → `getAction`, `m_CursorIndex` → `getCursorIndex`,
# `m_Submenu` → `getSubmenu`. Bool-returning getters use the `is*` /
# `has*` form when the member already starts with `Is`/`Has`.
def _accessor_name_from_member(member_name: str, returns_bool: bool) -> str:
    base = member_name[len("m_"):] if member_name.startswith("m_") else member_name
    if not base:
        return "value"
    if returns_bool:
        if base.startswith("Is"):
            # `m_IsVisible` → `isVisible`
            return "is" + base[2:]
        if base.startswith("Has"):
            return "has" + base[3:]
        # bool with neutral name → use the `is` prefix anyway so the
        # accessor reads naturally.
        return "is" + base
    return "get" + base


def _render_scalar_accessor_methods(spec: SwaApiClass) -> list[str]:
    """Return the inline-accessor method lines for every scalar / enum member
    in the class. Each accessor is a `const`-qualified getter that returns
    the host-side semantic value with the appropriate cast for SWA's
    `be<T>` big-endian wrapper.
    """
    enum_names = {e.name for e in spec.enums}
    out: list[str] = []
    for member in spec.members:
        if member.rcptr_inner_type is not None:
            continue
        decl_type = member.decl_type
        if decl_type == "bool":
            accessor = _accessor_name_from_member(member.name, returns_bool=True)
            out.append(f"bool {accessor}() const noexcept {{ return {member.name}; }}")
            continue
        if decl_type.startswith("be<") and decl_type.endswith(">"):
            inner = decl_type[3:-1].strip()
            if inner in enum_names:
                accessor = _accessor_name_from_member(member.name, returns_bool=False)
                out.append(
                    f"{inner} {accessor}() const noexcept "
                    f"{{ return static_cast<{inner}>({member.name}.m_storage); }}"
                )
            else:
                # Plain integer be<T>; return the wrapped value as-is. The
                # endian conversion still belongs to a separate runtime
                # layer; this accessor is layout-correct but not yet
                # endian-correct.
                accessor = _accessor_name_from_member(member.name, returns_bool=False)
                out.append(
                    f"{inner} {accessor}() const noexcept "
                    f"{{ return {member.name}.m_storage; }}"
                )
            continue
        # Plain non-be<T> scalar: bool-typed already handled above; for
        # ints / floats just return the raw value.
        accessor = _accessor_name_from_member(member.name, returns_bool=False)
        out.append(
            f"{decl_type} {accessor}() const noexcept "
            f"{{ return {member.name}; }}"
        )
    return out


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


def parse_swa_api_header(api_header_path: Path) -> dict[str, str]:
    """Return a `{member_name: rcptr_inner_type}` map for every `RCPtr<T> m_xxx;`
    declared in the UnleashedRecomp team's SWA API header for this class.

    The map is the authoritative source for template-argument types
    (`CProject` vs `CScene` vs `CNode`) for the originally-named SWA HUD
    fields. Phase-265 runtime extensions that don't appear in this map fall
    back to the conservative default `CScene` in the generator and the
    generated header annotates the fallback so a reader knows the type is
    runtime-evidence only.
    """
    if not api_header_path.is_file():
        return {}
    text = api_header_path.read_text(encoding="utf-8")
    return {
        m.group("name"): m.group("inner")
        for m in _SWA_RCPTR_DECL_RE.finditer(text)
    }


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
    rcptr_type_by_name: dict[str, str] | None = None,
    swa_api_header_relpath: str | None = None,
) -> str:
    generated_at = _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds")
    binding_by_name = {b.field_name: b for b in bindings}
    type_map = dict(rcptr_type_by_name or {})

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
    lines.append("// Phase 267: per-member template-argument types come from the")
    lines.append("// UnleashedRecomp team's existing SWA API header (when available). Members")
    lines.append("// the SWA API header has not yet named fall back to the conservative")
    lines.append("// default `Chao::CSD::CScene`; those fall-backs are flagged inline so")
    lines.append("// readers know the type is runtime-evidence-only and may be refined later.")
    lines.append("//")
    lines.append(f"// Generated at: {generated_at}")
    if source_attr:
        lines.append(f"// Source attribution: {source_attr}")
    if swa_api_header_relpath:
        lines.append(f"// SWA API header (authoritative for SWA-named field types): {swa_api_header_relpath}")
    lines.append("")
    lines.append("#include <array>")
    lines.append("#include <cstddef>")
    lines.append("#include <cstdint>")
    lines.append("#include <string_view>")
    lines.append("")
    lines.append("namespace sward::ui_runtime::generated::sgfx_hud")
    lines.append("{")
    lines.append("    // Forward declarations of the SWA CSD types referenced by the SWA")
    lines.append("    // HUD class. The retail SWA executable holds the corresponding")
    lines.append("    // `Chao::CSD::*` types; the human-readable port keeps them opaque at")
    lines.append("    // this layer because the CSD runtime is ported separately. The")
    lines.append("    // forward-decl set is the union of every template-argument type the")
    lines.append("    // SWA API header uses for the SWA-named members.")
    forward_types = sorted({type_map[name] for name in type_map if type_map.get(name)})
    forward_types_with_default = forward_types + (["CScene"] if "CScene" not in forward_types else [])
    forward_types_sorted = sorted(set(forward_types_with_default))
    for fwd in forward_types_sorted:
        lines.append(f"    class {fwd};")
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
    lines.append("    // The SWA executable references the wrapper as `Chao::CSD::RCPtr<T>`")
    lines.append("    // throughout the existing API headers; the `Chao::CSD::` alias here")
    lines.append("    // matches that convention so the human-readable port's member")
    lines.append("    // declarations read identically to the SWA originals.")
    lines.append("    namespace Chao { namespace CSD")
    lines.append("    {")
    lines.append("        template <class T> using RCPtr = ::sward::ui_runtime::generated::sgfx_hud::RCPtr<T>;")
    for fwd in forward_types_sorted:
        lines.append(f"        using {fwd} = ::sward::ui_runtime::generated::sgfx_hud::{fwd};")
    lines.append("    }} // namespace Chao::CSD")
    lines.append("")
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
    lines.append("        // Phase 282: every member is `public:` so the class qualifies as")
    lines.append("        // standard-layout per [class.prop]. That makes `offsetof` on the")
    lines.append("        // members below well-defined under clang/MSVC and silences the")
    lines.append("        // `-Winvalid-offsetof` warning the alternating-access version triggered.")
    lines.append("        std::uint32_t m_pVTable;             // +0x00 vtable pointer set by sub_824D89B0")
    lines.append("        std::array<std::uint8_t, 0x24> m_padding00_28;  // pre-secondary-vtable bytes")
    lines.append("        std::uint32_t m_pSecondaryVTable;    // +0x28 typeinfo / aux vtable set by sub_824D89B0")
    lines.append(f"        std::array<std::uint8_t, 0x{fields[0].rc_ptr_offset - 0x2C:X}> m_padding2C_E0;  // pre-RCPtr-table bytes")
    for field in fields:
        binding = binding_by_name.get(field.name)
        rcptr_inner = type_map.get(field.name)
        type_provenance: str
        if rcptr_inner:
            type_provenance = "type from SWA API header"
        else:
            rcptr_inner = "CScene"
            type_provenance = "type defaulted to CScene; SWA API header has not yet named this RCPtr"
        comment = f"+0x{field.rc_ptr_offset:X} Chao::CSD::RCPtr<{rcptr_inner}>; {type_provenance}"
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
        lines.append(f"        Chao::CSD::RCPtr<Chao::CSD::{rcptr_inner}> {field.name};  // {comment}")
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
        "--swa-api-header",
        default=(
            "local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/Sonic/HudSonicStage.h"
        ),
        help=(
            "Path to the UnleashedRecomp SWA API header for the HUD class "
            "being ported (relative to repo root). Used for authoritative "
            "RCPtr<T> template-argument types."
        ),
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
    parser.add_argument(
        "--also-generate-chud-pause",
        action="store_true",
        default=True,
        help=(
            "Also generate sgfx_hud_chud_pause.generated.h from the "
            "UnleashedRecomp SWA API header for CHudPause; on by default."
        ),
    )
    parser.add_argument(
        "--no-also-generate-chud-pause",
        action="store_false",
        dest="also_generate_chud_pause",
        help="Skip the parallel CHudPause generation pass.",
    )
    parser.add_argument(
        "--chud-pause-swa-api-header",
        default=(
            "local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/Pause/HudPause.h"
        ),
        help="Path to the UnleashedRecomp SWA API header for CHudPause.",
    )
    parser.add_argument(
        "--chud-pause-output-header",
        default=(
            "research_uiux/runtime_reference/include/sward/ui_runtime/"
            "sgfx_hud_chud_pause.generated.h"
        ),
        help="Where to write the generated C++ header for CHudPause.",
    )
    parser.add_argument(
        "--sweep-swa-hud-headers",
        action="store_true",
        default=True,
        help=(
            "Phase 269: also walk every SWA API HUD header under "
            "local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/ and "
            "emit a port header for each parseable class."
        ),
    )
    parser.add_argument(
        "--no-sweep-swa-hud-headers",
        action="store_false",
        dest="sweep_swa_hud_headers",
        help="Skip the Phase 269 SWA-HUD-headers sweep.",
    )
    parser.add_argument(
        "--swa-hud-headers-root",
        default="local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD",
        help="Root directory under which to sweep for SWA HUD API headers.",
    )
    parser.add_argument(
        "--port-header-output-dir",
        default=(
            "research_uiux/runtime_reference/include/sward/ui_runtime"
        ),
        help="Directory where Phase 269 sweep emits one port header per class.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    ui_lab_path = (repo_root / args.ui_lab_path).resolve()
    fields, source_attr = parse_expected_fields(ui_lab_path)

    swa_api_header_path = (repo_root / args.swa_api_header).resolve()
    rcptr_type_by_name = parse_swa_api_header(swa_api_header_path)
    swa_api_header_relpath: str | None = None
    if rcptr_type_by_name and swa_api_header_path.exists():
        try:
            swa_api_header_relpath = str(
                swa_api_header_path.relative_to(repo_root).as_posix())
        except ValueError:
            swa_api_header_relpath = str(swa_api_header_path)

    if args.sidecar is not None:
        sidecar_path = (repo_root / args.sidecar).resolve()
    else:
        sidecar_path = find_latest_sidecar(repo_root)

    bindings = build_scene_bindings(fields, sidecar_path)

    expected_total_size = fields[-1].rc_ptr_offset + 8
    header = emit_header(
        fields,
        bindings,
        source_attr,
        expected_total_size,
        rcptr_type_by_name=rcptr_type_by_name,
        swa_api_header_relpath=swa_api_header_relpath,
    )
    output_path = (repo_root / args.output_header).resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(header, encoding="utf-8")

    swa_api_summary = (
        f"; SWA API types from {swa_api_header_relpath} ({len(rcptr_type_by_name)} typed members)"
        if swa_api_header_relpath
        else "; no SWA API header found"
    )
    print(
        f"sgfx-hud-layout: wrote {output_path.relative_to(repo_root)} "
        f"with {len(fields)} RCPtr members and {len(bindings)} runtime scene bindings"
        + (f" from sidecar {sidecar_path.relative_to(repo_root)}" if sidecar_path else "")
        + swa_api_summary
    )

    if args.also_generate_chud_pause:
        chud_pause_api_path = (repo_root / args.chud_pause_swa_api_header).resolve()
        chud_pause_spec = parse_swa_api_class(chud_pause_api_path, repo_root)
        if chud_pause_spec is None:
            print(
                f"sgfx-hud-layout: skipped CHudPause generation; could not parse "
                f"{chud_pause_api_path.relative_to(repo_root) if chud_pause_api_path.is_relative_to(repo_root) else chud_pause_api_path}"
            )
        else:
            chud_pause_header_text = emit_swa_api_class_header(chud_pause_spec)
            chud_pause_output_path = (repo_root / args.chud_pause_output_header).resolve()
            chud_pause_output_path.parent.mkdir(parents=True, exist_ok=True)
            chud_pause_output_path.write_text(chud_pause_header_text, encoding="utf-8")
            print(
                f"sgfx-hud-layout: wrote {chud_pause_output_path.relative_to(repo_root)} "
                f"with {len(chud_pause_spec.members)} members "
                f"({sum(1 for m in chud_pause_spec.members if m.rcptr_inner_type)} RCPtrs, "
                f"{sum(1 for m in chud_pause_spec.members if not m.rcptr_inner_type)} scalars) "
                f"and {len(chud_pause_spec.enums)} enum definitions; "
                f"sourced from {chud_pause_spec.swa_api_header_relpath}"
            )

    if args.sweep_swa_hud_headers:
        sweep_root = (repo_root / args.swa_hud_headers_root).resolve()
        port_dir = (repo_root / args.port_header_output_dir).resolve()
        manifest_entries: list[dict[str, object]] = []
        skipped_entries: list[dict[str, object]] = []
        # Dedup: the explicit CHudPause path above already covered Pause;
        # walk every other .h under the sweep root and emit per-class
        # headers for the parseable ones.
        already_emitted_class_names: set[str] = set()
        # CHudSonicStage uses the runtime-extended path, not the SWA-API-
        # only path, so its header is already authoritative; do not let
        # the sweep overwrite it.
        already_emitted_class_names.add("CHudSonicStage")
        if args.also_generate_chud_pause:
            already_emitted_class_names.add("CHudPause")
        for header_path in sorted(sweep_root.rglob("*.h")):
            spec = parse_swa_api_class(header_path, repo_root)
            relpath = (
                str(header_path.relative_to(repo_root).as_posix())
                if header_path.is_relative_to(repo_root)
                else str(header_path)
            )
            if spec is None:
                skipped_entries.append({
                    "header": relpath,
                    "reason": (
                        "no SWA_ASSERT_OFFSETOF entries or no recognizable "
                        "single-class layout — generator left the file alone "
                        "to avoid emitting an unverified port"
                    ),
                })
                print(f"sgfx-hud-layout: sweep skipped {relpath}")
                continue
            if spec.class_name in already_emitted_class_names:
                continue
            output_filename = f"sgfx_hud_{_camel_to_snake(spec.class_name)}.generated.h"
            output_path = port_dir / output_filename
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_text(emit_swa_api_class_header(spec), encoding="utf-8")
            already_emitted_class_names.add(spec.class_name)
            rcptr_count = sum(1 for m in spec.members if m.rcptr_inner_type)
            scalar_count = sum(1 for m in spec.members if not m.rcptr_inner_type)
            manifest_entries.append({
                "className": spec.class_name,
                "swaApiHeader": spec.swa_api_header_relpath,
                "outputHeader": str(output_path.relative_to(repo_root).as_posix()),
                "rcptrCount": rcptr_count,
                "scalarCount": scalar_count,
                "enumCount": len(spec.enums),
                "memberCount": len(spec.members),
            })
            print(
                f"sgfx-hud-layout: sweep wrote {output_path.relative_to(repo_root)} "
                f"({len(spec.members)} members: {rcptr_count} RCPtrs + {scalar_count} scalars, "
                f"{len(spec.enums)} enums) from {spec.swa_api_header_relpath}"
            )
        manifest_path = port_dir / "sgfx_hud_layout_manifest.generated.json"
        manifest_path.parent.mkdir(parents=True, exist_ok=True)
        manifest_path.write_text(
            json.dumps({
                "schema": "sward-sgfx-hud-layout-manifest-v1",
                "sweptRoot": (
                    str(sweep_root.relative_to(repo_root).as_posix())
                    if sweep_root.is_relative_to(repo_root)
                    else str(sweep_root)
                ),
                "generatedAt": _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="seconds"),
                "manifestEntries": manifest_entries,
                "skippedEntries": skipped_entries,
            }, indent=2),
            encoding="utf-8",
        )
        print(
            f"sgfx-hud-layout: wrote {manifest_path.relative_to(repo_root)} "
            f"with {len(manifest_entries)} class entries and {len(skipped_entries)} skipped"
        )

    return 0


def _camel_to_snake(name: str) -> str:
    # Phase 269: turn `CHudSonicStage` → `c_hud_sonic_stage` for filename use.
    out = []
    for i, ch in enumerate(name):
        if ch.isupper() and i > 0 and not name[i - 1].isupper():
            out.append("_")
        out.append(ch.lower())
    return "".join(out)


if __name__ == "__main__":
    raise SystemExit(main())
