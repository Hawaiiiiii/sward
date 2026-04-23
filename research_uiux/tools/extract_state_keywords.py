#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from collections import Counter
from pathlib import Path


SCAN_DIRS = ["UnleashedRecomp/ui", "UnleashedRecomp/patches"]
SOURCE_SUFFIXES = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".inl"}

ENUM_BLOCK_RE = re.compile(
    r"enum(?:\s+class)?\s+([A-Za-z_]\w*)?\s*\{(?P<body>.*?)\}",
    re.DOTALL,
)
STATE_TOKEN_RE = re.compile(
    r"\b([A-Za-z_]\w*(?:State|Status|Mode|Phase|Transition|Fade|Overlay|Window|Guide|Menu|Intro|Pause|Title))\b"
)
CASE_RE = re.compile(r"^\s*case\s+([^:]+)\s*:", re.MULTILINE)


def unique(items):
    seen = set()
    output = []
    for item in items:
        if not item or item in seen:
            continue
        seen.add(item)
        output.append(item)
    return output


def iter_files(repo_root: Path):
    for subdir in SCAN_DIRS:
        root = repo_root / subdir
        if not root.exists():
            continue
        for path in sorted(root.rglob("*")):
            if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES:
                yield path


def scan_file(repo_root: Path, path: Path) -> dict:
    text = path.read_text(encoding="utf-8", errors="ignore")
    enums = []
    for match in ENUM_BLOCK_RE.finditer(text):
        name = match.group(1) or "<anonymous>"
        body = match.group("body")
        values = unique(
            token.strip().split("=")[0].strip()
            for token in body.replace("\n", " ").split(",")
            if token.strip()
        )
        enums.append({"name": name, "values": values})

    state_tokens = unique(STATE_TOKEN_RE.findall(text))
    switch_cases = unique(case.strip() for case in CASE_RE.findall(text))

    return {
        "path": path.relative_to(repo_root).as_posix(),
        "enums": enums,
        "state_tokens": state_tokens,
        "switch_cases": switch_cases,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract state-machine keywords from UI and patch source.")
    parser.add_argument("--repo-root", default=".", help="Path to the UnleashedRecomp source root.")
    parser.add_argument(
        "--output",
        default="research_uiux/data/state_keywords.json",
        help="Output JSON path.",
    )
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    output = Path(args.output)
    if not output.is_absolute():
        output = repo_root / output
    output.parent.mkdir(parents=True, exist_ok=True)

    files = [scan_file(repo_root, path) for path in iter_files(repo_root)]
    token_counter: Counter[str] = Counter()
    case_counter: Counter[str] = Counter()
    for file_entry in files:
        token_counter.update(file_entry["state_tokens"])
        case_counter.update(file_entry["switch_cases"])

    payload = {
        "repo_root": repo_root.as_posix(),
        "scan_dirs": SCAN_DIRS,
        "file_count": len(files),
        "top_state_tokens": dict(sorted(token_counter.items(), key=lambda item: (-item[1], item[0]))[:200]),
        "top_switch_cases": dict(sorted(case_counter.items(), key=lambda item: (-item[1], item[0]))[:200]),
        "files": files,
    }
    output.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
