#!/usr/bin/env python3
"""Probe: can we recover UVs for runtime-captured paths from the parsed CSD?
Builds recomp-format paths (stem/node.../scene/cast...) -> uv from layout_deep_analysis
and measures overlap with the runtime capture's ui_pause paths."""
import json, os, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DA = json.load(open(os.path.join(ROOT, "research_uiux", "data", "layout_deep_analysis.json"), encoding="utf-8"))
PF = {p["stem"]: p for p in DA["parsed_files"]}

def cast_uv(cast, scene, texs):
    cm = cast.get("cast_material", {})
    for si in cm.get("used_subimage_indices", []):
        subs = scene.get("subimages", [])
        if 0 <= si < len(subs):
            s = subs[si]
            tl, br = s.get("top_left"), s.get("bottom_right")
            if tl and br:
                return [round(tl[0],5),round(tl[1],5),round(br[0],5),round(br[1],5)]
    return None

def build_paths(stem):
    """Return {recomp_path: uv} replicating the recomp's EmplacePath naming."""
    p = PF.get(stem)
    if not p: return {}
    texs = []  # not needed for uv
    out = {}
    def walk_scene(scene, spath):
        name_by = {(d["group_index"], d["cast_index"]): d["name"] for d in scene.get("cast_dictionaries", [])}
        for gi, g in enumerate(scene.get("cast_groups", [])):
            casts = g.get("casts", []); hier = g.get("hierarchy", []); n = len(casts)
            def visit(idx, parentpath, branch):
                if not (0 <= idx < n) or idx in branch: return
                branch = branch | {idx}
                nm = name_by.get((gi, idx), f"g{gi}c{idx}")
                cpath = parentpath + "/" + nm
                uv = cast_uv(casts[idx], scene, texs)
                if uv: out[cpath] = uv
                kid = hier[idx]["child_index"] if idx < len(hier) else -1
                cur, seen = kid, set()
                while 0 <= cur < n and cur not in seen:
                    seen.add(cur); visit(cur, cpath, branch)
                    cur = hier[cur]["next_index"] if cur < len(hier) else -1
            root = g.get("root_cast_index", 0); cur, rs = root, set()
            while 0 <= cur < n and cur not in rs:
                rs.add(cur); visit(cur, spath, set())
                cur = hier[cur]["next_index"] if cur < len(hier) else -1
    def walk_node(node, path):
        sids = sorted(node.get("scene_ids", []), key=lambda x: x["index"])
        for i, sc in enumerate(node.get("scenes", [])):
            nm = sids[i]["name"] if i < len(sids) else f"s{i}"
            walk_scene(sc, path + "/" + nm)
        nds = sorted(node.get("node_dictionaries", []), key=lambda x: x["index"])
        for i, ch in enumerate(node.get("children", [])):
            cn = nds[i]["name"] if i < len(nds) else f"n{i}"
            walk_node(ch, path + "/" + cn)
    proj = p["resources"][0]["content"]["csdm_project"]
    walk_node(proj["root"], stem)
    return out

stem = sys.argv[1] if len(sys.argv) > 1 else "ui_pause"
uvmap = build_paths(stem)
print(f"{stem}: built {len(uvmap)} path->uv entries")
# runtime paths
F = "/c/swardbuild/out/build/x64-Clang-RelWithDebInfo/UnleashedRecomp/csd_capture.jsonl".replace("/c/", "C:/")
rt = set()
with open(F, encoding="utf-8") as f:
    for line in f:
        if '"path":"'+stem+'/' in line:
            pth = line.split('"path":"',1)[1].split('"',1)[0]
            rt.add(pth)
print(f"runtime unique {stem} paths: {len(rt)}")
matched = sum(1 for r in rt if r in uvmap)
print(f"matched (exact): {matched}/{len(rt)} = {matched*100//max(1,len(rt))}%")
print("sample UNMATCHED runtime paths:")
for r in sorted(rt - set(uvmap))[:10]: print("   ", r)
print("sample matched:")
for r in sorted(rt & set(uvmap))[:5]: print("   ", r, "->", uvmap[r])
