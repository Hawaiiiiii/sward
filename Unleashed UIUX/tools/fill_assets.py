#!/usr/bin/env python3
"""
fill_assets.py — populate assets/<screen>/*.png from textures already extracted
from YOUR own copy (research_uiux extracted_assets/), so the reconstructed
screens render with real art instead of drop-target placeholders.

Reads the texture names the manifests already reference (manifests/<id>.json),
finds the matching .dds under the local extracted-asset roots, and converts it
to PNG with Pillow (no texconv needed for these de-tiled DDS). Ships no assets;
operates only on files already on this machine.

Usage:
  python "Unleashed UIUX/tools/fill_assets.py"                  # all manifest screens
  python "Unleashed UIUX/tools/fill_assets.py" --only world_map loading
  python "Unleashed UIUX/tools/fill_assets.py" --force          # re-convert existing
"""
from __future__ import annotations
import argparse, json, os, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
VIEWER_DIR = HERE.parent
REPO_ROOT = VIEWER_DIR.parent
MANIFEST_DIR = VIEWER_DIR / "manifests"
ASSET_DIR = VIEWER_DIR / "assets"

SEARCH_ROOTS = [
    REPO_ROOT / "extracted_assets",
    REPO_ROOT / "Unleashed Recomp - Windows (Complete Installation) 1.0.3",
]

# when a texture name occurs in several archives, prefer a path hinting at the
# screen's home archive / English localisation
ARCHIVE_HINT = {
    "title": "SystemData", "pause": "SystemCommonCore", "options": "SystemCommonCore",
    "world_map": "WorldMap", "result": "ActionCommon", "boss": "BossCommon",
    "sonic_hud": "SonicActionCommon", "loading": "Loading",
}


def build_index():
    """stem(lower) -> list[Path] for every .dds under the search roots."""
    idx = {}
    for root in SEARCH_ROOTS:
        if not root.exists():
            continue
        for p in root.rglob("*.dds"):
            idx.setdefault(p.stem.lower(), []).append(p)
    return idx


def pick(paths, screen):
    """Choose the best of several same-named DDS for a screen."""
    if len(paths) == 1:
        return paths[0]
    hint = ARCHIVE_HINT.get(screen, "")
    # 1) home archive + English; 2) home archive; 3) English; 4) first
    def score(p):
        s = str(p).lower()
        return (hint.lower() in s, "english" in s, "languages" in s)
    return sorted(paths, key=score, reverse=True)[0]


def convert(dds_path, png_path):
    from PIL import Image
    png_path.parent.mkdir(parents=True, exist_ok=True)
    im = Image.open(dds_path)
    im.load()
    if im.mode not in ("RGBA", "RGB"):
        im = im.convert("RGBA")
    im.save(png_path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", nargs="*", help="screen ids to fill")
    ap.add_argument("--force", action="store_true", help="re-convert even if PNG exists")
    args = ap.parse_args()

    manifests = sorted(MANIFEST_DIR.glob("*.json"))
    if args.only:
        manifests = [m for m in manifests if m.stem in args.only]

    print("indexing extracted DDS ...")
    idx = build_index()
    print(f"  {sum(len(v) for v in idx.values())} dds files, {len(idx)} unique names\n")

    grand = {"filled": 0, "skipped": 0, "missing": 0}
    for mpath in manifests:
        m = json.loads(mpath.read_text(encoding="utf-8"))
        screen = m.get("screen", mpath.stem)
        texset = {n["tex"] for n in m.get("nodes", []) if n.get("tex")}
        # also fill every texture the rich CSD-player data references (the player draws
        # ALL casts, not just the manifest's collapsed regions)
        rich = VIEWER_DIR / "sgfx_ui_cpp" / "data" / (screen + ".json")
        if rich.exists():
            rd = json.loads(rich.read_text(encoding="utf-8"))
            for t in rd.get("textures", []):
                texset.add(t if t.lower().endswith(".png") else t + ".png")
        texs = sorted(texset)
        filled = missing = skipped = 0
        miss_names = []
        for tex in texs:
            stem = Path(tex).stem.lower()
            out = ASSET_DIR / screen / tex
            if out.exists() and not args.force:
                skipped += 1
                continue
            cands = idx.get(stem)
            if not cands:
                missing += 1
                miss_names.append(tex)
                continue
            try:
                convert(pick(cands, screen), out)
                filled += 1
            except Exception as e:
                missing += 1
                miss_names.append(f"{tex} (convert err: {type(e).__name__})")
        grand["filled"] += filled; grand["skipped"] += skipped; grand["missing"] += missing
        print(f"[{screen:<10}] {len(texs):>2} textures -> filled {filled}, "
              f"skipped {skipped}, missing {missing}")
        if miss_names:
            print("              missing: " + ", ".join(miss_names[:8])
                  + (" ..." if len(miss_names) > 8 else ""))
    print(f"\nTotal: filled {grand['filled']}, skipped {grand['skipped']}, missing {grand['missing']}")
    print("Reload the viewer; the 'Reconstructed' screens now show your textures.")


if __name__ == "__main__":
    main()
