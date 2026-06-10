#!/usr/bin/env python3
"""
runtime_capture_to_manifest.py — build a manifest from a RUNTIME UI-draw capture.

Static .yncp extraction (csd_to_manifest.py) recovers the authored layout, but it
cannot recover things the game computes at RUNTIME:
  - elements re-anchored by code off their authored origin  (pause title bar,
    boss gauge frame -> ALIGN_TOP_RIGHT etc.)
  - text/number sprites whose subimage is swapped at runtime (the real row labels
    TIME/RING/SCORE, score values, menu item names)

This tool ingests a per-frame dump of the casts the recomp actually DREW
(each with its final on-screen rect + bound texture) and emits a manifest with the
TRUE positions. It reuses the viewer's manifest format, so captured screens render
exactly like the static ones — just more complete.

Capture dump schema (one JSON file per screen/frame; produced by the recomp hook,
see tools/README_extract.md "Live-from-runtime option"):

  {
    "screen": "pause",
    "ref": [1280, 720],
    "elements": [
      { "path": "Root/window_1/bg_1",     // cast path (EmplacePath naming)
        "tex":  "mat_pause_en_001",        // bound texture name (ext optional)
        "rect": [x, y, w, h],              // FINAL on-screen rect, 1280x720 ref space
        "uv":   [u0, v0, u1, v1],          // optional atlas sub-rect (normalised)
        "color":"0xAARRGGBB",              // optional; alpha byte -> opacity
        "visible": true },                 // optional; false -> skipped
      ...
    ]
  }

Element array order is treated as draw order (earlier = behind).

Usage:
  python "Unleashed UIUX/tools/runtime_capture_to_manifest.py" capture/pause.json
  python "Unleashed UIUX/tools/runtime_capture_to_manifest.py" capture/pause.json --augment
  python "Unleashed UIUX/tools/runtime_capture_to_manifest.py" --demo   # synthesize+ingest a sample
"""
from __future__ import annotations
import argparse, json, os, re

HERE = os.path.dirname(os.path.abspath(__file__))
VIEWER_DIR = os.path.abspath(os.path.join(HERE, ".."))
MANIFEST_DIR = os.path.join(VIEWER_DIR, "manifests")
CAPTURE_DIR = os.path.join(VIEWER_DIR, "capture")
REF_W, REF_H = 1280, 720


def sanitize(s):
    return re.sub(r"[^a-z0-9]+", "_", s.lower()).strip("_") or "node"


def png_for(tex):
    return (os.path.splitext(tex)[0] if tex else "node") + ".png"


def alpha_of(el):
    """opacity 0..1 from an element's color/alpha, or 1.0."""
    c = el.get("color")
    if isinstance(c, str) and c.lower().startswith("0x") and len(c) == 10:
        return int(c[2:4], 16) / 255.0
    if isinstance(el.get("alpha"), (int, float)):
        a = el["alpha"]
        return a / 255.0 if a > 1 else float(a)
    return 1.0


def onscreen_frac(r):
    x, y, w, h = r
    ix = max(0, min(REF_W, x + w)) - max(0, min(REF_W, x))
    iy = max(0, min(REF_H, y + h)) - max(0, min(REF_H, y))
    return max(0, ix) * max(0, iy) / max(1.0, abs(w * h))


def element_to_node(el, z, used_ids, ref):
    rect = el.get("rect")
    if not rect or len(rect) != 4:
        return None
    rw, rh = ref
    # scale capture rect into 1280x720 ref space if the dump used a different ref
    sx, sy = REF_W / rw, REF_H / rh
    x, y, w, h = rect[0] * sx, rect[1] * sy, rect[2] * sx, rect[3] * sy
    if w < 0:
        x, w = x + w, -w
    if h < 0:
        y, h = y + h, -h
    if onscreen_frac([x, y, w, h]) < 0.4 or w * h < 12:
        return None
    cx = max(0, min(REF_W, x)); cy = max(0, min(REF_H, y))
    cw = max(1, min(REF_W - cx, w)); ch = max(1, min(REF_H - cy, h))
    base = sanitize(el.get("path", "").replace("/", "_") or el.get("tex", "node"))
    nid = base; k = 2
    while nid in used_ids:
        nid = f"{base}_{k}"; k += 1
    used_ids.add(nid)
    node = {"id": nid, "tex": png_for(el.get("tex")),
            "rect": [round(cx), round(cy), round(cw), round(ch)],
            "z": z, "fit": "fill"}
    uv = el.get("uv")
    if uv and len(uv) == 4 and not (uv[2] - uv[0] > 0.999 and uv[3] - uv[1] > 0.999):
        node["uv"] = [round(float(v), 5) for v in uv]
    a = alpha_of(el)
    if a < 0.999:
        node["alpha"] = round(a, 3)
    return node


def capture_to_nodes(cap):
    ref = cap.get("ref", [REF_W, REF_H])
    used = set()
    nodes = []
    for i, el in enumerate(cap.get("elements", [])):
        if el.get("visible") is False:
            continue
        n = element_to_node(el, i, used, ref)
        if n:
            nodes.append(n)
    # drop exact-overlap duplicates (same sprite at same rect)
    out = []
    for n in nodes:
        if any(d["tex"] == n["tex"] and d.get("uv") == n.get("uv") and
               all(abs(d["rect"][j] - n["rect"][j]) < 2 for j in range(4)) for d in out):
            continue
        out.append(n)
    return out


# first path token (the .yncp stem the recomp emits) -> viewer screen id.
# Only stems whose UVs we can recover from the parsed CSD (so atlas sprites crop
# correctly). ui_playscreen/ui_title have no matching parsed .yncp, so they're left to
# their static manifests rather than rendered as whole-atlas-sheet noise.
STEM_TO_SCREEN = {
    "ui_pause": "pause", "ui_worldmap": "world_map", "ui_result": "result",
    "ui_boss_gauge": "boss", "ui_boss_name": "boss", "ui_loading": "loading",
    "ui_general": "options", "ui_status": "status",
    "ui_townscreen": "town", "ui_shop": "shop",
}

# parsed CSD with per-cast UVs, to enrich runtime rects (which lack UVs).
LAYOUT_JSON = os.path.join(os.path.dirname(VIEWER_DIR), "research_uiux", "data", "layout_deep_analysis.json")


def build_uv_maps(stems):
    """{recomp_path: [u0,v0,u1,v1]} for the given stems, replicating the recomp's
    EmplacePath naming (stem / node.. / scene / cast..), so runtime paths match."""
    if not os.path.exists(LAYOUT_JSON):
        return {}
    da = json.load(open(LAYOUT_JSON, encoding="utf-8"))
    pf = {p["stem"]: p for p in da.get("parsed_files", [])}
    out = {}

    def cast_uv(cast, scene):
        cm = cast.get("cast_material", {})
        for si in cm.get("used_subimage_indices", []):
            subs = scene.get("subimages", [])
            if 0 <= si < len(subs):
                s = subs[si]; tl, br = s.get("top_left"), s.get("bottom_right")
                if tl and br:
                    return [round(tl[0], 5), round(tl[1], 5), round(br[0], 5), round(br[1], 5)]
        return None

    def walk_scene(scene, spath):
        name_by = {(d["group_index"], d["cast_index"]): d["name"] for d in scene.get("cast_dictionaries", [])}
        for gi, g in enumerate(scene.get("cast_groups", [])):
            casts = g.get("casts", []); hier = g.get("hierarchy", []); n = len(casts)
            def visit(idx, parent, branch):
                if not (0 <= idx < n) or idx in branch:
                    return
                branch = branch | {idx}
                cpath = parent + "/" + name_by.get((gi, idx), f"g{gi}c{idx}")
                uv = cast_uv(casts[idx], scene)
                if uv and not (uv[2] - uv[0] > 0.999 and uv[3] - uv[1] > 0.999):
                    out[cpath] = uv
                kid = hier[idx]["child_index"] if idx < len(hier) else -1
                cur, seen = kid, set()
                while 0 <= cur < n and cur not in seen:
                    seen.add(cur); visit(cur, cpath, branch)
                    cur = hier[cur]["next_index"] if cur < len(hier) else -1
            cur, rs = g.get("root_cast_index", 0), set()
            while 0 <= cur < n and cur not in rs:
                rs.add(cur); visit(cur, spath, set())
                cur = hier[cur]["next_index"] if cur < len(hier) else -1

    def walk_node(node, path):
        sids = sorted(node.get("scene_ids", []), key=lambda x: x["index"])
        for i, sc in enumerate(node.get("scenes", [])):
            walk_scene(sc, path + "/" + (sids[i]["name"] if i < len(sids) else f"s{i}"))
        nds = sorted(node.get("node_dictionaries", []), key=lambda x: x["index"])
        for i, ch in enumerate(node.get("children", [])):
            walk_node(ch, path + "/" + (nds[i]["name"] if i < len(nds) else f"n{i}"))

    for stem in stems:
        p = pf.get(stem)
        if p:
            walk_node(p["resources"][0]["content"]["csdm_project"]["root"], stem)
    return out


def read_stream(jsonl_path, uvmap=None, only=None):
    """Read csd_capture.jsonl (one drawn cast per line). For each (screen, path) we take the
    MOST-FREQUENT rect (mode), not the last: a screen redraws every frame while you sit on it,
    so the settled layout dominates -- whereas the *last* frame of a screen you transition
    THROUGH (e.g. the world map when you enter a stage) is the fade-out, with elements sliding
    off. Each element is UV-enriched from the parsed CSD (by path) so atlas sprites crop."""
    from collections import Counter
    uvmap = uvmap or {}
    only = set(only) if only else None
    rectcount = {}  # (screen, path) -> Counter(rect_tuple)
    rep = {}        # (screen, path) -> representative element (tex/uv)
    order = {}      # (screen, path) -> first-seen seq (for draw order)
    seq = 0
    with open(jsonl_path, encoding="utf-8") as f:
        for line in f:
            line = line.strip().rstrip(",")
            if not line or line[0] != "{":
                continue
            try:
                el = json.loads(line)
            except Exception:
                continue
            path = el.get("path") or ""
            screen = STEM_TO_SCREEN.get(path.split("/", 1)[0])
            if not screen or (only and screen not in only):
                continue
            r = el.get("rect")
            if not r or len(r) != 4:
                continue
            key = (screen, path)
            rectcount.setdefault(key, Counter())[tuple(r)] += 1
            if key not in rep:
                if not el.get("tex"):
                    el["tex"] = path.rsplit("/", 1)[-1]  # untextured -> name fallback
                uv = uvmap.get(path)
                if uv:
                    el["uv"] = uv
                rep[key] = el
                order[key] = seq
            seq += 1
    matched = [sum(1 for k in rep if rep[k].get("uv")), len(rep)]
    if matched[1]:
        print(f"  UV enrichment: {matched[0]}/{matched[1]} elements cropped "
              f"({matched[0]*100//matched[1]}%)")
    screens = {}
    for key, ctr in rectcount.items():
        screen, path = key
        el = dict(rep[key])
        el["rect"] = list(ctr.most_common(1)[0][0])   # settled (modal) rect
        screens.setdefault(screen, []).append((order[key], el))
    out = {}
    for screen, items in screens.items():
        items.sort(key=lambda t: t[0])
        out[screen] = {"screen": screen, "ref": [REF_W, REF_H],
                       "elements": [el for _, el in items]}
    return out


def iou(a, b):
    ix = max(0, min(a[0]+a[2], b[0]+b[2]) - max(a[0], b[0]))
    iy = max(0, min(a[1]+a[3], b[1]+b[3]) - max(a[1], b[1]))
    inter = ix * iy
    u = a[2]*a[3] + b[2]*b[3] - inter
    return inter / u if u > 0 else 0


def write_manifest(screen, nodes, stem="runtime", augmented=False):
    out = {
        "screen": screen, "stem": stem, "ref": [REF_W, REF_H],
        "assetBase": f"assets/{screen}/",
        "generated_by": "tools/runtime_capture_to_manifest.py" + (" (augmented)" if augmented else ""),
        "notes": ("Built from a RUNTIME UI-draw capture: rects are the elements' true "
                  "on-screen positions (incl. runtime-anchored / runtime-text elements that "
                  "static .yncp extraction cannot place). Slots are named after the bound "
                  "textures; UV-cropped and alpha-dimmed to match the drawn frame."),
        "nodes": nodes,
    }
    path = os.path.join(MANIFEST_DIR, screen + ".json")
    os.makedirs(MANIFEST_DIR, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2)
    return path


def augment(screen, cap_nodes):
    """Merge capture nodes into the existing static manifest: keep static nodes, append
    capture nodes that don't overlap an existing one, so runtime-only/off-screen-anchored
    elements (boss gauge frame, pause title) get added without losing the static layout."""
    path = os.path.join(MANIFEST_DIR, screen + ".json")
    if not os.path.exists(path):
        return None
    base = json.loads(open(path, encoding="utf-8").read())
    existing = base.get("nodes", [])
    ids = {n["id"] for n in existing}
    added = 0
    zmax = max([n.get("z", 0) for n in existing], default=0)
    for n in cap_nodes:
        if any(iou(n["rect"], e["rect"]) > 0.7 and n["tex"] == e.get("tex") for e in existing):
            continue  # already represented
        nid = n["id"]; k = 2
        while nid in ids:
            nid = f"{nid}_{k}"; k += 1
        n["id"] = nid; ids.add(nid)
        n["z"] = zmax + 1 + added
        existing.append(n); added += 1
    base["nodes"] = existing
    base["generated_by"] = base.get("generated_by", "") + " + runtime augment"
    with open(path, "w", encoding="utf-8") as f:
        json.dump(base, f, indent=2)
    return path, added


SAMPLE = {  # a tiny synthetic capture exercising the runtime-only cases
    "screen": "_capture_demo", "ref": [1280, 720],
    "elements": [
        {"path": "Root/header/status_title", "tex": "mat_pause_en_002", "rect": [40, 28, 360, 40],
         "uv": [0.0, 0.0, 0.55, 0.18], "color": "0xFFFFFFFF", "visible": True},   # the off-screen-anchored pause title
        {"path": "Root/window_1/bg_1", "tex": "mat_result_comon_001", "rect": [325, 227, 631, 270],
         "uv": [0.2, 0.4, 0.5, 0.6], "color": "0xE6FFFFFF", "visible": True},
        {"path": "Root/window_1/item_continue", "tex": "mat_pause_en_001", "rect": [360, 250, 560, 40],
         "uv": [0.0, 0.30, 0.40, 0.36], "color": "0xFFFFFFFF", "visible": True},   # a real runtime label
        {"path": "Root/window_1/hidden_skill", "tex": "mat_pause_en_001", "rect": [0, 0, 400, 300],
         "color": "0x00FFFFFF", "visible": False},                                 # hidden -> dropped
    ],
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("capture", nargs="?", help="path to a capture JSON (or .jsonl with --stream)")
    ap.add_argument("--stream", action="store_true", help="input is csd_capture.jsonl (one drawn cast per line); split into per-screen manifests")
    ap.add_argument("--only", nargs="*", help="(stream) restrict to these screen ids, so a capture session doesn't overwrite other screens")
    ap.add_argument("--augment", action="store_true", help="merge into the existing static manifest instead of replacing")
    ap.add_argument("--demo", action="store_true", help="synthesize a sample capture, ingest it, and report")
    args = ap.parse_args()

    if args.demo:
        os.makedirs(CAPTURE_DIR, exist_ok=True)
        sample_path = os.path.join(CAPTURE_DIR, "_demo.json")
        with open(sample_path, "w", encoding="utf-8") as f:
            json.dump(SAMPLE, f, indent=2)
        nodes = capture_to_nodes(SAMPLE)
        path = write_manifest(SAMPLE["screen"], nodes)
        print(f"demo capture: {len(SAMPLE['elements'])} elements -> {len(nodes)} nodes "
              f"(1 hidden dropped) -> {os.path.relpath(path, VIEWER_DIR)}")
        for n in nodes:
            print(f"  {n['id']:<28} rect={n['rect']} tex={n['tex']} uv={'uv' in n} alpha={n.get('alpha','-')}")
        return 0

    if not args.capture:
        ap.error("provide a capture JSON/JSONL, or use --demo")

    if args.stream:
        print("building UV map from parsed CSD (layout_deep_analysis.json) ...")
        uvmap = build_uv_maps(set(STEM_TO_SCREEN.keys()))
        print(f"  {len(uvmap)} path->uv entries")
        screens = read_stream(args.capture, uvmap, only=args.only)
        if not screens:
            print("no recognised CSD casts in", args.capture); return 0
        for screen, cap in sorted(screens.items()):
            nodes = capture_to_nodes(cap)
            if args.augment:
                res = augment(screen, nodes)
                if res:
                    path, added = res
                    print(f"[{screen:<10}] augmented +{added} runtime nodes (of {len(nodes)})")
                    continue
            print(f"[{screen:<10}] {len(nodes):>3} nodes -> "
                  f"{os.path.relpath(write_manifest(screen, nodes), VIEWER_DIR)}")
        return 0

    cap = json.loads(open(args.capture, encoding="utf-8").read())
    screen = cap.get("screen") or os.path.splitext(os.path.basename(args.capture))[0]
    nodes = capture_to_nodes(cap)
    if args.augment:
        res = augment(screen, nodes)
        if not res:
            print(f"no existing manifest for '{screen}'; writing fresh instead")
            print("->", os.path.relpath(write_manifest(screen, nodes), VIEWER_DIR))
        else:
            path, added = res
            print(f"augmented {os.path.relpath(path, VIEWER_DIR)}: +{added} runtime nodes "
                  f"(of {len(nodes)} captured; rest already represented)")
    else:
        print(f"{len(nodes)} nodes -> {os.path.relpath(write_manifest(screen, nodes), VIEWER_DIR)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
