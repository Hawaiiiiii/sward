#!/usr/bin/env python3
"""
emit_runtime_data.py — export a screen's full CSD runtime data (geometry + ANIMATIONS)
for the sgfx_ui C++ runtime, from the already-parsed layout_deep_analysis.json.

Unlike the static manifests (rects only), this exports everything needed to PLAY the
screen 1:1 like the game: per-cast base transform, UV, texture, hierarchy, and every
animation's keyframe tracks (XPosition/YPosition/Rotation/XScale/YScale/Color/SubImage/
HideFlag with Const/Linear/Hermite keyframes + tangents) at the scene's framerate.

Output: sgfx_ui_cpp/data/<screen>.json   (schema consumed by the C++ loader)

Usage:
  python emit_runtime_data.py            # all configured screens
  python emit_runtime_data.py pause title
"""
from __future__ import annotations
import json, math, os, sys


def clean(o):
    """Replace NaN/Inf (invalid JSON, rejected by nlohmann) with 0.0, recursively."""
    if isinstance(o, float):
        return o if math.isfinite(o) else 0.0
    if isinstance(o, dict):
        return {k: clean(v) for k, v in o.items()}
    if isinstance(o, list):
        return [clean(v) for v in o]
    return o

HERE = os.path.dirname(os.path.abspath(__file__))
CPP_DIR = os.path.abspath(os.path.join(HERE, ".."))
VIEWER_DIR = os.path.abspath(os.path.join(CPP_DIR, ".."))
REPO_ROOT = os.path.abspath(os.path.join(VIEWER_DIR, ".."))
LAYOUT = os.path.join(REPO_ROOT, "research_uiux", "data", "layout_deep_analysis.json")
OUT_DIR = os.path.join(CPP_DIR, "data")

# screen id -> .yncp stems (a screen may span several CSD files). IDs match the
# viewer's SWARD_MANIFESTS so each screen auto-upgrades to the real CSD player.
SCREENS = {
    "title":          ["ui_mainmenu"],
    "pause":          ["ui_pause"],
    "world_map":      ["ui_worldmap"],
    "world_map_help": ["ui_worldmap_help"],
    "result":         ["ui_result"],
    "result_ex":      ["ui_result_ex"],
    "boss":           ["ui_boss_gauge", "ui_boss_name"],
    "loading":        ["ui_loading"],
    "options":        ["ui_general"],
    "status":         ["ui_status"],
    "shop":           ["ui_shop"],
    "town":           ["ui_townscreen"],
    "sonic_hud":      ["ui_prov_playscreen"],
    "mission_screen": ["ui_missionscreen"],
    "mission":        ["ui_misson"],
    "item_result":    ["ui_itemresult"],
    "exstage":        ["ui_exstage"],
    "qte":            ["ui_qte"],
    "gate":           ["ui_gate"],
    "balloon":        ["ui_balloon"],
    "mediaroom":      ["ui_mediaroom"],
    "help":           ["ui_help"],
    "start":          ["ui_start"],
}

# Resting-state scene excludes (mirrors the static csd_to_manifest pipeline): drop
# alternate-state / submenu / popup scenes whose node path contains any substring,
# so a screen shows its settled resting layout, not every overlaid sub-state at once.
# (from the 6-screen fidelity audit, wf csd-fidelity-audit)
EXCLUDE = {
    "title": [  # keep mm_base + the _usual/_idle resting variant of each widget family;
                # drop the _intro entry anims (double-draw the same art) + interactive alternates
        "mm_bg_intro", "mm_donut_select", "mm_donut_move", "mm_donut_intro",
        "mm_contentsitem_select", "mm_contentsitem_move", "mm_contentsitem_intro",
        "mm_contentsitem_text", "mm_title_intro", "mm_front_intro",
    ],
    "pause":     ["/window_2", "/window_3", "/explanatory", "footer_B"],  # submenus, popups, dup prompt bar
    "world_map": ["cts_choices",                                          # stage-select popup + FULL-SCREEN DIMMER (covers everything)
                  "cts_cursor_effect", "cts_stage_scroll_bg", "cts_stage_scroll_bar",
                  "cts_guide_txt", "cts_guide_icon"],                     # transient FX + empty scroll/label slots
    "result":    ["newRecode", "result_rank_E", "result_rank_D",
                  "result_rank_C", "result_rank_B", "result_rank_A"],     # new-record reveal + alt ranks (keep S)
    "boss":      ["name_so", "name_ev"],  # both transient boss-name entry cards (plain + flame/event):
                                          # at rest ALL boss names (egg/ray/fish/lancer/dragoon) overlap.
                                          # Resting HUD = the gauge. (One clean name needs a per-scene anim override.)
    "loading":   ["Root/bg_1", "Root/bg_2"],                              # intro/outro backdrop tile sweeps (alpha 0 at rest)
    "result_ex": ["newRecode", "blaze", "rank_a", "rank_b", "rank_c", "rank_d", "rank_e"],
    "qte":       ["qte_multi", "qte_single", "Root/ex", "qte_txt_2", "qte_txt_3", "qte_txt_4"],
    "start":     ["Failed", "Game_over"],                                 # keep MISSION CLEAR! (mutually-exclusive end states)
}

# Cast-subtree excludes within a kept scene (id of a subtree root -> drop it + descendants).
# loadinfo double-draws both controller variants and both tip-text sets at once; this is a
# 360 English build, so drop the PS3 controller subtree and the night/"evil" tip copy.
EXCLUDE_CASTS = {
    "loading": ["ps3", "pos_text_evil"],
}


def tex_names(parsed):
    try:
        return [t["name"] for t in parsed["resources"][1]["content"]["texture_list"]["textures"]]
    except Exception:
        return []


def cast_tex_uv(cast, scene, texs):
    """Return (tex, uv, cells). A cast's material lists used_subimage_indices — the
    sprite-sheet 'cells' this cast can show (CNode::SetPatternIndex picks one). The
    base 'sub' index and a 'SubImage' anim track both select WHICH cell. We emit the
    FULL cell list (all on one texture) so the runtime can pick the right one; uv is
    cells[0] for back-compat / single-cell casts. (Previously only cells[0] was kept,
    so any cast with base sub>0 or a non-zero SubImage track showed the wrong cell.)"""
    cm = cast.get("cast_material", {})
    subs = scene.get("subimages", [])
    tex, cells = "", []
    for si in cm.get("used_subimage_indices", []):
        if 0 <= si < len(subs):
            s = subs[si]
            ti = s.get("texture_index", -1)
            if 0 <= ti < len(texs):
                tl, br = s.get("top_left") or [0, 0], s.get("bottom_right") or [1, 1]
                if not tex:
                    tex = os.path.splitext(texs[ti])[0]   # strip .dds -> matches .png assets
                cells.append([tl[0], tl[1], br[0], br[1]])
    uv = cells[0] if cells else None
    return tex, uv, cells


# CSD blend mode is a runtime/guest decision not stored in the .yncp, so additive
# FX (glows/shines/flashes/sparkles) are inferred by SEGA's naming convention
# (confirmed by the fidelity audit). These layers should add light, not occlude.
ADDITIVE_NAME_BITS = (
    "brilliance", "blliriance", "_bri", "brilli",      # title/rank/header shine beams
    "light", "glow", "shine", "flash", "flare",        # highlights / flashes
    "blaze", "spark", "beam", "lens", "fog",           # flares / rank fog
    "noise", "dots", "led", "lamp", "pale",            # screenshot shimmer / sparkles / lamps / flash twin
)

def is_additive(cast_id):
    cid = (cast_id or "").lower()
    return any(b in cid for b in ADDITIVE_NAME_BITS)


def cast_base(cast):
    ci = cast.get("cast_info", {})
    tr = ci.get("translation", [0, 0]) or [0, 0]
    sc = ci.get("scale", [1, 1]) or [1, 1]
    return {
        "tx": tr[0], "ty": tr[1],
        "sx": sc[0], "sy": sc[1],
        "rot": ci.get("rotation", 0.0) or 0.0,
        "color": ci.get("color", "0xFFFFFFFF"),
        "sub": ci.get("subimage", 0.0) or 0.0,
        "hide": ci.get("hide_flag", 0),
    }


def cast_anims(scene, gi, ci):
    """Per-anim track keyframes for cast (gi,ci): {animName: {frames, tracks:{type:[kf]}}}."""
    anims = {}
    dicts = scene.get("animation_dictionaries", [])
    kdl = scene.get("animation_keyframe_data_list", [])
    fdl = scene.get("animation_frame_data_list", [])
    for ai, ad in enumerate(dicts):
        name = ad.get("name", f"anim_{ai}")
        frames = fdl[ai]["frame_count"] if ai < len(fdl) else 0.0
        tracks = {}
        if ai < len(kdl):
            groups = kdl[ai].get("groups", [])
            if gi < len(groups):
                gcasts = groups[gi].get("casts", [])
                if ci < len(gcasts):
                    for tr in gcasts[ci].get("sub_data", []):
                        tt = tr.get("track_type")
                        kfs = [{"f": k["frame"], "v": k["value"], "t": k["type"],
                                "it": k.get("in_tangent", 0.0), "ot": k.get("out_tangent", 0.0)}
                               for k in tr.get("keyframes", [])]
                        if tt and kfs:
                            tracks[tt] = kfs
        if tracks:
            anims[name] = {"frames": frames, "tracks": tracks}
    return anims


def emit_scene(scene, scene_path, texs, cast_excludes=()):
    """Flatten a scene's cast tree into ordered casts with parent indices + anims.
    cast_excludes: cast ids whose whole subtree is dropped (e.g. the PS3 controller)."""
    name_by = {(d["group_index"], d["cast_index"]): d["name"] for d in scene.get("cast_dictionaries", [])}
    casts = []           # flat list
    idx_of = {}          # (gi,ci) -> flat index
    for gi, g in enumerate(scene.get("cast_groups", [])):
        gcasts = g.get("casts", [])
        hier = g.get("hierarchy", [])
        n = len(gcasts)

        def visit(ci, parent_flat, branch):
            if not (0 <= ci < n) or ci in branch:
                return
            if name_by.get((gi, ci)) in cast_excludes:   # drop this cast + its whole subtree
                return
            branch = branch | {ci}
            c = gcasts[ci]
            tl, br = c.get("top_left"), c.get("bottom_right")
            quad = [tl[0], tl[1], br[0], br[1]] if tl and br else None
            tex, uv, cells = cast_tex_uv(c, scene, texs)
            base = cast_base(c)
            flat = len(casts)
            idx_of[(gi, ci)] = flat
            cid = name_by.get((gi, ci), f"g{gi}c{ci}")
            entry = {
                "id": cid,
                "gi": gi, "ci": ci, "parent": parent_flat,
                "quad": quad, "tex": tex, "uv": uv,
                "base": base,
                "anims": cast_anims(scene, gi, ci),
            }
            # Emit the full sprite-sheet cell list (+ base cell index) only when a cast
            # actually has multiple cells — single-cell casts are fully covered by uv.
            if len(cells) > 1:
                entry["cells"] = cells
                si = int(round(base["sub"])) if (base["sub"] and base["sub"] >= 0) else 0
                if si:
                    entry["subIdx"] = si
            # 9-slice anchor mask (field34 low bits): bit10 (0x400) set = stretch X, clear = anchor X
            # (fixed width); bit11 (0x800) set = stretch Y, clear = anchor Y. Anchored slices keep their
            # intrinsic size while still being positioned by the parent's stretch, so window frames /
            # gauge end-caps stay crisp instead of inheriting the stretch and smearing across the screen.
            # Emit a compact 'anc' (1=anchorX, 2=anchorY) only when an axis is actually anchored.
            f34 = c.get("field34", 0) or 0
            anc = (1 if not (f34 & 0x400) else 0) | (2 if not (f34 & 0x800) else 0)
            if anc:
                entry["anc"] = anc
            if is_additive(cid):
                entry["add"] = 1
            casts.append(entry)
            kid = hier[ci]["child_index"] if ci < len(hier) else -1
            cur, seen = kid, set()
            while 0 <= cur < n and cur not in seen:
                seen.add(cur); visit(cur, flat, branch)
                cur = hier[cur]["next_index"] if cur < len(hier) else -1

        cur, rs = g.get("root_cast_index", 0), set()
        while 0 <= cur < n and cur not in rs:
            rs.add(cur); visit(cur, -1, set())
            cur = hier[cur]["next_index"] if cur < len(hier) else -1

    fdl = scene.get("animation_frame_data_list", [])
    anim_list = [{"name": d.get("name", f"anim_{i}"),
                  "frames": fdl[i]["frame_count"] if i < len(fdl) else 0.0}
                 for i, d in enumerate(scene.get("animation_dictionaries", []))]
    return {"name": scene_path, "framerate": scene.get("animation_framerate", 60.0),
            "anims": anim_list, "casts": casts}


def walk_scenes(parsed):
    proj = parsed["resources"][0]["content"].get("csdm_project", {})
    out = []

    def rec(node, path):
        sids = sorted(node.get("scene_ids", []), key=lambda x: x["index"])
        for i, sc in enumerate(node.get("scenes", [])):
            nm = sids[i]["name"] if i < len(sids) else f"scene_{i}"
            out.append((f"{path}/{nm}", sc))
        nds = sorted(node.get("node_dictionaries", []), key=lambda x: x["index"])
        for i, ch in enumerate(node.get("children", [])):
            cn = nds[i]["name"] if i < len(nds) else f"node_{i}"
            rec(ch, f"{path}/{cn}")
    rec(proj.get("root", {}), proj.get("project_name", "root"))
    return out


def main():
    da = json.load(open(LAYOUT, encoding="utf-8"))
    # Re-parse each source .yncp/.xncp with the (fixed) XNCP parser so Color/Gradient
    # tracks carry their real packed-RGBA values (0xRRGGBBAA) instead of the
    # NaN-flattened-to-0 garbage baked into layout_deep_analysis.json. We use the exact
    # source path recorded per parsed_file, so file selection is unchanged — only the
    # color-track keyframe values are corrected.
    sys.path.insert(0, os.path.join(REPO_ROOT, "research_uiux", "tools"))
    import inspect_xncp_yncp as _ix
    from pathlib import Path as _Path
    pf, reparsed = {}, 0
    for p in da["parsed_files"]:
        src = p.get("path")
        try:
            if src and os.path.exists(src):
                pf[p["stem"]] = _ix.parse_fapc(_Path(src))
                reparsed += 1
                continue
        except Exception as e:
            print(f"  ! reparse failed for {p.get('stem')}: {e}; using cached copy")
        pf[p["stem"]] = p
    print(f"re-parsed {reparsed}/{len(da['parsed_files'])} source files with fixed color decode")
    os.makedirs(OUT_DIR, exist_ok=True)
    targets = sys.argv[1:] or list(SCREENS.keys())
    for sid in targets:
        stems = SCREENS.get(sid)
        if not stems:
            print("skip unknown screen:", sid); continue
        all_tex = set()
        scenes = []
        framerate = 60.0
        for stem in stems:
            p = pf.get(stem)
            if not p:
                continue
            texs = tex_names(p)
            excludes = EXCLUDE.get(sid, [])
            cast_excludes = EXCLUDE_CASTS.get(sid, [])
            for path, sc in walk_scenes(p):
                # emit EVERY scene (all states) — the contract-driven player needs the
                # alt-state scenes (submenus, popups, reveals) to show them in their
                # state. `rest` marks whether the scene is part of the settled resting
                # layout (passes the resting excludes); the simple player shows only
                # rest scenes, the state-machine player shows all per the active state.
                is_rest = not any(ex in path for ex in excludes)
                s = emit_scene(sc, path, texs, cast_excludes)
                if s["casts"]:
                    s["rest"] = is_rest
                    scenes.append(s)
                    framerate = s["framerate"] or framerate
                    for c in s["casts"]:
                        if c["tex"]:
                            all_tex.add(c["tex"])
        out = {"screen": sid, "stems": stems, "ref": [1280, 720],
               "framerate": framerate, "textures": sorted(all_tex), "scenes": scenes}
        path = os.path.join(OUT_DIR, sid + ".json")
        with open(path, "w", encoding="utf-8") as f:
            json.dump(clean(out), f, separators=(",", ":"), allow_nan=False)
        nc = sum(len(s["casts"]) for s in scenes)
        na = sum(sum(1 for c in s["casts"] if c["anims"]) for s in scenes)
        print(f"[{sid:<10}] {len(scenes)} scenes, {nc} casts, {na} animated casts, "
              f"{len(all_tex)} textures -> {os.path.relpath(path, CPP_DIR)}")


if __name__ == "__main__":
    raise SystemExit(main())
