#!/usr/bin/env python3
"""
csd_to_manifest.py — generate SWARD viewer manifests from already-parsed CSD layout data.

Reads research_uiux/data/layout_deep_analysis.json (the output of
research_uiux/tools/inspect_xncp_yncp.py) and, for each configured screen, resolves
the .yncp cast hierarchy into absolute 1280x720 rects, collapses each top-level
container (and its 9-slice / icon-cluster children) into a single region node, names
the slot after the real texture (so it auto-fills once you run the DDS->PNG extractor),
and writes Unleashed UIUX/manifests/<id>.json in the viewer's manifest format.

This is the concrete "per-node rect reader" the extractor README references: the rects
are REAL (from your own parsed game files), not estimates.

Coordinate model (verified against the parser):
  - v3 cast quad corners (top_left/bottom_right) and cast_info.translation are BOTH
    normalised screen fractions (0..1). We accumulate translation*scale down the cast
    hierarchy in normalised space, then multiply by [1280,720] at the leaf.
  - Some scenes author content around a local origin and are positioned by runtime code
    (not by translation). Those resolve off-screen; we skip them and report why -- they
    need manual placement or a runtime capture, and are never silently dropped.

Usage:
  python "Unleashed UIUX/tools/csd_to_manifest.py"            # generate all configured screens
  python "Unleashed UIUX/tools/csd_to_manifest.py" --only pause world_map
  python "Unleashed UIUX/tools/csd_to_manifest.py" --dump ui_pause   # inspect a stem's scenes
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
VIEWER_DIR = os.path.abspath(os.path.join(HERE, ".."))
REPO_ROOT = os.path.abspath(os.path.join(VIEWER_DIR, ".."))
LAYOUT_JSON = os.path.join(REPO_ROOT, "research_uiux", "data", "layout_deep_analysis.json")
MANIFEST_DIR = os.path.join(VIEWER_DIR, "manifests")

REF_W, REF_H = 1280, 720

# ---- per-screen configuration -------------------------------------------------
# id        : manifest id (-> manifests/<id>.json, assets/<id>/)
# stems     : ordered .yncp stems to pull scenes from
# label     : sidebar label
# include   : if set, only scenes whose path/name contains one of these substrings
# exclude   : skip scenes whose path/name contains one of these substrings
# A scene contributes its top-level container regions (depth<=max_depth).
# GLOBAL resting-state excludes: scene-name fragments that are almost always an
# alternate/transient sub-state (popup, dimmer, reveal card, confirm dialog) — dropped
# on every screen so a screen shows its settled resting layout, not stacked sub-states.
GLOBAL_EXCLUDE = ["cts_choices", "name_ev", "newrecode", "_dimmer", "dimmer",
                  "blackout", "_cover", "_confirm", "confirm_", "_popup", "popup_"]

SCREENS = [
    # ---- audited / tuned resting states (the main screens) ----
    {"id": "title", "stems": ["ui_mainmenu"], "label": "Main Menu",
     # keep mm_base + each widget family's idle/usual; drop the _intro/_select/_move/_text variants
     "exclude": ["_intro", "donut_select", "donut_move",
                 "contentsitem_select", "contentsitem_move", "contentsitem_text"]},
    {"id": "pause", "stems": ["ui_pause"], "label": "Pause",
     "exclude": ["explanatory", "skill", "window_2", "window_3"]},
    {"id": "world_map", "stems": ["ui_worldmap"], "label": "World Map",
     "exclude": ["cts_choices"]},
    {"id": "result", "stems": ["ui_result"], "label": "Mission Result",
     "exclude": ["newRecode", "blaze", "rank_a", "rank_b", "rank_c", "rank_d", "rank_e"]},
    {"id": "boss", "stems": ["ui_boss_gauge", "ui_boss_name"], "label": "Boss HUD",
     "exclude": ["name_ev"]},
    {"id": "loading", "stems": ["ui_loading"], "label": "Loading",
     "exclude": ["event_viewer", "loadinfo"]},
    # ---- the rest of the game's UI (parsed-data manifests) ----
    {"id": "options", "stems": ["ui_general"], "label": "Options / General"},
    {"id": "status", "stems": ["ui_status"], "label": "Status / Skills"},
    {"id": "shop", "stems": ["ui_shop"], "label": "Shop"},
    {"id": "town", "stems": ["ui_townscreen"], "label": "Town Screen"},
    {"id": "hud", "stems": ["ui_prov_playscreen"], "label": "In-Game HUD"},
    {"id": "world_map_help", "stems": ["ui_worldmap_help"], "label": "World Map Help"},
    {"id": "result_ex", "stems": ["ui_result_ex"], "label": "Result (EX / Tails)",
     "exclude": ["newRecode", "blaze", "rank_a", "rank_b", "rank_c", "rank_d", "rank_e"]},
    {"id": "mission_screen", "stems": ["ui_missionscreen"], "label": "Mission Screen"},
    {"id": "mission", "stems": ["ui_misson"], "label": "Mission Objective"},
    {"id": "item_result", "stems": ["ui_itemresult"], "label": "Item Result"},
    {"id": "exstage", "stems": ["ui_exstage"], "label": "EX Stage (Tails)"},
    {"id": "qte", "stems": ["ui_qte"], "label": "QTE Prompts",
     # drop the backdrop variants, the ex-stage set, and the alternate feedback-text
     # sprites (GO/NICE/GREAT...) that all stack at one spot — keep one (qte_txt_1)
     "exclude": ["qte_multi", "qte_single", "root/ex", "qte_txt_2", "qte_txt_3", "qte_txt_4"]},
    {"id": "gate", "stems": ["ui_gate"], "label": "Stage Gate"},
    {"id": "balloon", "stems": ["ui_balloon"], "label": "Town Balloon"},
    {"id": "mediaroom", "stems": ["ui_mediaroom"], "label": "Media Room"},
    {"id": "help", "stems": ["ui_help"], "label": "Help"},
    {"id": "start", "stems": ["ui_start"], "label": "Stage Start",
     # Clear / Failed / Game_over are mutually-exclusive end states at the same spot;
     # keep MISSION CLEAR! as the hero default
     "exclude": ["Failed", "Game_over"]},
    {"id": "saveicon", "stems": ["ui_saveicon"], "label": "Save Icon"},
    {"id": "end", "stems": ["ui_end"], "label": "Staff Roll / End"},
]

# casts whose name marks them as a non-visual pivot/anchor (16x16 marker, no texture)
PIVOT_NAMES = re.compile(r"^(position|center|display|contents?|contens|box|line_|window_?\d*|root)$", re.I)


def load_layout():
    with open(LAYOUT_JSON, encoding="utf-8") as f:
        da = json.load(f)
    return {p["stem"]: p for p in da["parsed_files"]}


def tex_names(parsed):
    try:
        return [t["name"] for t in parsed["resources"][1]["content"]["texture_list"]["textures"]]
    except Exception:
        return []


def cast_tex_uv(cast, scene, texs):
    """Return (texture_name, uv) for a cast, where uv = [u0,v0,u1,v1] normalised
    sub-rect of the sheet (the atlas crop), or None."""
    cm = cast.get("cast_material", {})
    subs = scene.get("subimages", [])
    for si in cm.get("used_subimage_indices", []):
        if 0 <= si < len(subs):
            sub = subs[si]
            ti = sub.get("texture_index", -1)
            if 0 <= ti < len(texs):
                tl = sub.get("top_left") or [0.0, 0.0]
                br = sub.get("bottom_right") or [1.0, 1.0]
                return texs[ti], [tl[0], tl[1], br[0], br[1]]
    return "", None


def resolve_scene(scene, texs):
    """Resolve every cast to an absolute px rect. Returns a flat list of node dicts
    plus, for each cast, its subtree members (by (gi, idx)) for bbox unions."""
    groups = scene.get("cast_groups", [])
    name_by = {(d["group_index"], d["cast_index"]): d["name"] for d in scene.get("cast_dictionaries", [])}
    nodes = []          # one dict per cast
    children = {}       # (gi, idx) -> list of (gi, childidx)
    for gi, g in enumerate(groups):
        casts = g.get("casts", [])
        hier = g.get("hierarchy", [])
        root = g.get("root_cast_index", 0)
        n = len(casts)

        def visit(idx, depth, parent, ox, oy, sx, sy, branch):
            if not (0 <= idx < n) or idx in branch:
                return
            branch = branch | {idx}
            c = casts[idx]
            ci = c.get("cast_info", {})
            tr = ci.get("translation", [0, 0]) or [0, 0]
            scl = ci.get("scale", [1, 1]) or [1, 1]
            ax = ox + tr[0] * sx
            ay = oy + tr[1] * sy
            nsx = sx * (scl[0] if scl[0] else 1.0)
            nsy = sy * (scl[1] if scl[1] else 1.0)
            tl, br = c.get("top_left"), c.get("bottom_right")
            rect = None
            if tl and br:
                rx = (ax + tl[0] * nsx) * REF_W
                ry = (ay + tl[1] * nsy) * REF_H
                rw = (br[0] - tl[0]) * nsx * REF_W
                rh = (br[1] - tl[1]) * nsy * REF_H
                if rw < 0:
                    rx, rw = rx + rw, -rw
                if rh < 0:
                    ry, rh = ry + rh, -rh
                rect = [rx, ry, rw, rh]
            key = (gi, idx)
            tx, uv = cast_tex_uv(c, scene, texs)
            col = ci.get("color", "")
            alpha = 255
            if isinstance(col, str) and col.startswith("0x") and len(col) == 10:
                alpha = int(col[2:4], 16)
            nodes.append({"key": key, "depth": depth, "parent": parent, "order": len(nodes),
                          "name": name_by.get(key, f"g{gi}c{idx}"),
                          "rect": rect, "tex": tx, "uv": uv, "alpha": alpha,
                          "en": c.get("is_enabled")})
            children.setdefault(parent, []).append(key) if parent else None
            kid = hier[idx]["child_index"] if idx < len(hier) else -1
            cur, seen = kid, set()
            while 0 <= cur < n and cur not in seen:
                seen.add(cur)
                visit(cur, depth + 1, key, ax, ay, nsx, nsy, branch)
                cur = hier[cur]["next_index"] if cur < len(hier) else -1

        cur, rseen = root, set()
        while 0 <= cur < n and cur not in rseen:
            rseen.add(cur)
            visit(cur, 0, None, 0.0, 0.0, 1.0, 1.0, set())
            cur = hier[cur]["next_index"] if cur < len(hier) else -1
    return nodes


def walk_scenes(parsed):
    """Yield (scene_path, scene_name, scene) for every scene in the CSD tree."""
    proj = parsed["resources"][0]["content"].get("csdm_project", {})
    out = []

    def rec(node, path):
        sids = sorted(node.get("scene_ids", []), key=lambda x: x["index"])
        for i, sc in enumerate(node.get("scenes", [])):
            nm = sids[i]["name"] if i < len(sids) else f"scene_{i}"
            out.append((path, nm, sc))
        nds = sorted(node.get("node_dictionaries", []), key=lambda x: x["index"])
        for i, ch in enumerate(node.get("children", [])):
            cn = nds[i]["name"] if i < len(nds) else f"node_{i}"
            rec(ch, f"{path}/{cn}")

    rec(proj.get("root", {}), "Root")
    return out


def _is_outlier(r):
    """A single 9-slice stretch piece or code-positioned cell can resolve to a runaway
    rect that would blow up a union bbox. Reject members that are implausibly large or
    sit well off the reference frame."""
    x, y, w, h = r
    if w > 1.5 * REF_W or h > 1.5 * REF_H:
        return True
    if x + w < -0.5 * REF_W or x > 1.5 * REF_W:
        return True
    if y + h < -0.5 * REF_H or y > 1.5 * REF_H:
        return True
    return False


def subtree_bbox(node_list, root_key):
    """Union bbox + dominant texture + representative UV crop + alpha for root_key's
    subtree. UV/alpha come from the largest-area cast using the dominant texture, so a
    single sprite crops exactly and a 9-slice region crops to its body piece. Casts that
    are base-invisible (alpha 0) are excluded so hidden/alt-state content can't bloat the
    box or win the representative pick."""
    by_key = {nd["key"]: nd for nd in node_list}
    kids = {}
    for nd in node_list:
        kids.setdefault(nd["parent"], []).append(nd["key"])
    items = []  # (rect, tex, uv, area, alpha)
    stack = [root_key]
    seen = set()
    while stack:
        k = stack.pop()
        if k in seen:
            continue
        seen.add(k)
        nd = by_key.get(k)
        if nd:
            if nd["tex"] and nd["rect"] and nd["alpha"] > 0 and not _is_outlier(nd["rect"]):
                r = nd["rect"]
                items.append((r, nd["tex"], nd.get("uv"), abs(r[2] * r[3]), nd["alpha"]))
            stack.extend(kids.get(k, []))
    if not items:
        return None, None, None, 255
    rects = [it[0] for it in items]
    texs = [it[1] for it in items]
    x0 = min(r[0] for r in rects); y0 = min(r[1] for r in rects)
    x1 = max(r[0] + r[2] for r in rects); y1 = max(r[1] + r[3] for r in rects)
    dom = max(set(texs), key=texs.count)
    dom_items = [it for it in items if it[1] == dom and it[2]]
    rep = max(dom_items, key=lambda it: it[3]) if dom_items else None
    uv = rep[2] if rep else None
    alpha = rep[4] if rep else 255
    if uv and (uv[2] - uv[0] > 0.999 and uv[3] - uv[1] > 0.999):
        uv = None
    return [x0, y0, x1 - x0, y1 - y0], dom, uv, alpha


def onscreen_frac(rect):
    x, y, w, h = rect
    ix0, iy0 = max(0, x), max(0, y)
    ix1, iy1 = min(REF_W, x + w), min(REF_H, y + h)
    inter = max(0, ix1 - ix0) * max(0, iy1 - iy0)
    area = max(1.0, w * h)
    return inter / area


def png_for(tex):
    stem = os.path.splitext(tex)[0]
    return stem + ".png"


def sanitize(s):
    return re.sub(r"[^a-z0-9]+", "_", s.lower()).strip("_")


def build_screen(cfg, parsed_by_stem, max_regions=60, min_area=400):
    """Container granularity: one region per top-level (depth-0) cast per CSD scene.
    Each scene is already a logical component (header / footer / info / stage panel / a
    menu window), so its subtree bbox is the right unit -- it keeps content-heavy panels
    whole AND gives crisp chrome (each chrome element is its own scene). A region's slot
    is UV-cropped to its representative sprite and dimmed by its base alpha."""
    regions = []
    skipped = {"offscreen": 0, "no_texture": 0, "tiny": 0, "duplicate": 0}
    used_ids = set()
    for stem in cfg["stems"]:
        parsed = parsed_by_stem.get(stem)
        if not parsed:
            continue
        texs = tex_names(parsed)
        for path, nm, sc in walk_scenes(parsed):
            tag = f"{path}::{nm}"
            inc = cfg.get("include"); exc = cfg.get("exclude")
            if any(g in tag.lower() for g in GLOBAL_EXCLUDE):
                continue
            if inc and not any(s.lower() in tag.lower() for s in inc):
                continue
            if exc and any(s.lower() in tag.lower() for s in exc):
                continue
            node_list = resolve_scene(sc, texs)
            for nd in node_list:
                if nd["depth"] != 0:
                    continue
                bbox, dom, uv, alpha = subtree_bbox(node_list, nd["key"])
                if not bbox or not dom:
                    skipped["no_texture"] += 1; continue
                if bbox[2] * bbox[3] < min_area:
                    skipped["tiny"] += 1; continue
                if onscreen_frac(bbox) < 0.5:
                    skipped["offscreen"] += 1; continue
                x = max(0, min(REF_W, bbox[0])); y = max(0, min(REF_H, bbox[1]))
                w = max(1, min(REF_W - x, bbox[2])); h = max(1, min(REF_H - y, bbox[3]))
                cname = nd["name"]
                base = sanitize(nm if PIVOT_NAMES.match(cname) else f"{nm}_{cname}") or "node"
                nid = base; k = 2
                while nid in used_ids:
                    nid = f"{base}_{k}"; k += 1
                used_ids.add(nid)
                node = {
                    "id": nid, "tex": png_for(dom),
                    "rect": [round(x), round(y), round(w), round(h)],
                    "z": 2, "fit": "fill", "_area": w * h, "_scene": tag,
                }
                if uv:
                    node["uv"] = [round(v, 5) for v in uv]
                if alpha < 255:
                    node["alpha"] = round(alpha / 255, 3)
                regions.append(node)

    # collapse near-identical regions (mutually-exclusive state variants at the same rect)
    def iou(a, b):
        ax0, ay0, aw, ah = a; bx0, by0, bw, bh = b
        ix0, iy0 = max(ax0, bx0), max(ay0, by0)
        ix1, iy1 = min(ax0 + aw, bx0 + bw), min(ay0 + ah, by0 + bh)
        inter = max(0, ix1 - ix0) * max(0, iy1 - iy0)
        union = aw * ah + bw * bh - inter
        return inter / union if union > 0 else 0
    regions.sort(key=lambda r: -r["_area"])
    deduped = []
    for r in regions:
        if any(iou(r["rect"], d["rect"]) > 0.9 for d in deduped):
            skipped["duplicate"] += 1; continue
        deduped.append(r)
    capped = deduped[:max_regions]
    dropped = len(deduped) - len(capped)
    for r in capped:
        r.pop("_area", None); r.pop("_scene", None)
    capped.sort(key=lambda r: (r["z"], r["rect"][1], r["rect"][0]))
    return capped, skipped, dropped, len(deduped)


def write_manifest(cfg, nodes):
    out = {
        "screen": cfg["id"],
        "stem": cfg["stems"][0],
        "ref": [REF_W, REF_H],
        "assetBase": f"assets/{cfg['id']}/",
        "generated_by": "tools/csd_to_manifest.py",
        "source_stems": cfg["stems"],
        "notes": ("Region rects resolved from the parsed CSD layout (layout_deep_analysis.json) "
                  "in real 1280x720 space. Each node is a top-level container; its slot is named "
                  "after the real texture so it auto-loads once you run the DDS->PNG extractor."),
        "nodes": nodes,
    }
    path = os.path.join(MANIFEST_DIR, cfg["id"] + ".json")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2)
    return path


def dump_stem(stem, parsed_by_stem):
    parsed = parsed_by_stem.get(stem)
    if not parsed:
        print("no such stem:", stem); return
    texs = tex_names(parsed)
    print(f"STEM {stem}: {len(texs)} textures")
    for path, nm, sc in walk_scenes(parsed):
        nl = resolve_scene(sc, texs)
        tx = [n for n in nl if n["tex"] and n["rect"]]
        if not tx:
            continue
        x0 = min(n["rect"][0] for n in tx); y0 = min(n["rect"][1] for n in tx)
        x1 = max(n["rect"][0] + n["rect"][2] for n in tx); y1 = max(n["rect"][1] + n["rect"][3] for n in tx)
        on = onscreen_frac([x0, y0, x1 - x0, y1 - y0])
        flag = "" if on >= 0.5 else "  <OFF-SCREEN/local>"
        print(f"  {path}::{nm:<22} casts={len(nl):>3} textured={len(tx):>3} "
              f"bbox=[{x0:6.0f},{y0:6.0f},{x1-x0:6.0f},{y1-y0:6.0f}] on={on:.0%}{flag}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", nargs="*", help="generate only these screen ids")
    ap.add_argument("--dump", help="dump a stem's scenes (diagnostic) and exit")
    ap.add_argument("--max-regions", type=int, default=60)
    args = ap.parse_args()

    parsed_by_stem = load_layout()
    if args.dump:
        dump_stem(args.dump, parsed_by_stem)
        return 0

    os.makedirs(MANIFEST_DIR, exist_ok=True)
    targets = [c for c in SCREENS if (not args.only or c["id"] in args.only)]
    summary = []
    for cfg in targets:
        nodes, skipped, dropped, total = build_screen(cfg, parsed_by_stem, args.max_regions)
        path = write_manifest(cfg, nodes)
        summary.append((cfg["id"], len(nodes), total, dropped, skipped, path))
        print(f"[{cfg['id']:<10}] {len(nodes):>3} region nodes "
              f"(of {total}; {dropped} over cap; skipped "
              f"offscreen={skipped['offscreen']} notex={skipped['no_texture']} "
              f"tiny={skipped['tiny']} dup={skipped['duplicate']})")
        print(f"             -> {os.path.relpath(path, REPO_ROOT)}")
    print("\nNext: register these ids in manifest-screen.jsx -> window.SWARD_MANIFESTS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
