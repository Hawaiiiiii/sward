#!/usr/bin/env python3
"""
resolve_nodes.py — flatten a screen's rich CSD runtime data (sgfx_ui_cpp/data/<id>.json)
into a clean, correct node list the hand-written C++ screens can transcribe:
    [{ tex, rect:[x,y,w,h] (1280x720 px), uv:[u0,v0,u1,v1], z, alpha, add }]

It reuses the same settled-pose + hierarchy + RRGGBBAA-color logic the (fixed) CSD
player uses, so texture names are the REAL on-disk atlases (mat_*/ui_*), not the
renamed placeholders some manifests carry (c_1/so/playscreen_* etc.).

    python resolve_nodes.py status sonic_hud shop
-> writes nodelists/<id>.json next to the manifests.
"""
from __future__ import annotations
import json, os, sys, math

HERE = os.path.dirname(os.path.abspath(__file__))
CPP  = os.path.abspath(os.path.join(HERE, ".."))          # sgfx_ui_cpp
VIEW = os.path.abspath(os.path.join(CPP, ".."))           # Unleashed UIUX
DATA = os.path.join(CPP, "data")
OUT  = os.path.join(VIEW, "nodelists")
REF_W, REF_H = 1280.0, 720.0


def eval_track(kf, frame):
    if not kf: return 0.0
    if frame <= kf[0]["f"]: return kf[0]["v"]
    if frame >= kf[-1]["f"]: return kf[-1]["v"]
    for i in range(len(kf) - 1):
        a, b = kf[i], kf[i + 1]
        if a["f"] <= frame <= b["f"]:
            dt = b["f"] - a["f"]
            if dt <= 0: return a["v"]
            t = (frame - a["f"]) / dt
            if a["t"] == "Const": return a["v"]
            if a["t"] == "Linear": return a["v"] + (b["v"] - a["v"]) * t
            t2, t3 = t * t, t * t * t
            return ((2*t3-3*t2+1)*a["v"] + (t3-2*t2+t)*(a.get("ot",0)*dt)
                    + (-2*t3+3*t2)*b["v"] + (t3-t2)*(b.get("it",0)*dt))
    return kf[-1]["v"]


def decode(n):
    n = int(n) & 0xFFFFFFFF
    return ((n >> 24) & 255, (n >> 16) & 255, (n >> 8) & 255, n & 255)   # RRGGBBAA


def eval_color_track(kf, frame):
    if not kf: return (255, 255, 255, 255)
    if frame <= kf[0]["f"]: return decode(kf[0]["v"])
    if frame >= kf[-1]["f"]: return decode(kf[-1]["v"])
    for i in range(len(kf) - 1):
        a, b = kf[i], kf[i + 1]
        if a["f"] <= frame <= b["f"]:
            dt = b["f"] - a["f"]
            if dt <= 0 or a["t"] == "Const": return decode(a["v"])
            t = (frame - a["f"]) / dt
            A, B = decode(a["v"]), decode(b["v"])
            return tuple(A[j] + (B[j] - A[j]) * t for j in range(4))
    return decode(kf[-1]["v"])


def base_rgba(s):
    if isinstance(s, str) and len(s) == 10:
        return decode(int(s[2:], 16))
    return (255, 255, 255, 255)


def eval_cast(c, anim, frame):
    b = c["base"]
    x = {"tx": b["tx"], "ty": b["ty"], "sx": b["sx"], "sy": b["sy"], "rot": b.get("rot", 0)}
    bc = base_rgba(b.get("color", "0xFFFFFFFF"))
    x["alpha"] = bc[3] / 255.0
    x["vis"] = b.get("hide", 0) == 0
    ca = (c.get("anims") or {}).get(anim)
    if ca:
        tr = ca["tracks"]
        for key, fld in (("XPosition","tx"),("YPosition","ty"),("XScale","sx"),("YScale","sy"),("Rotation","rot")):
            if key in tr: x[fld] = eval_track(tr[key], frame)
        if "HideFlag" in tr: x["vis"] = eval_track(tr["HideFlag"], frame) < 0.5
        col = None
        if "Color" in tr:
            col = eval_color_track(tr["Color"], frame)
        else:
            grads = [tr[g] for g in ("GradientTL","GradientTR","GradientBL","GradientBR") if g in tr]
            if grads:
                cs = [eval_color_track(g, frame) for g in grads]
                col = tuple(sum(c[j] for c in cs)/len(cs) for j in range(4))
        if col: x["alpha"] = max(0.0, min(1.0, col[3] / 255.0))
    return x


def world_of(casts, cache, i, anim, frame):
    if i in cache: return cache[i]
    x = eval_cast(casts[i], anim, frame)
    p = casts[i]["parent"]
    if p < 0:
        w = {"ox": x["tx"], "oy": x["ty"], "sx": x["sx"], "sy": x["sy"],
             "rot": x["rot"], "alpha": x["alpha"], "vis": x["vis"]}
    else:
        pw = world_of(casts, cache, p, anim, frame)
        w = {"ox": pw["ox"] + x["tx"] * pw["sx"], "oy": pw["oy"] + x["ty"] * pw["sy"],
             "sx": pw["sx"] * x["sx"], "sy": pw["sy"] * x["sy"], "rot": pw["rot"] + x["rot"],
             "alpha": pw["alpha"] * x["alpha"], "vis": pw["vis"] and x["vis"]}
    cache[i] = w
    return w


def screen_rect(c, w):
    qx = w["ox"] + c["quad"][0] * w["sx"]; qy = w["oy"] + c["quad"][1] * w["sy"]
    qw = (c["quad"][2] - c["quad"][0]) * w["sx"]; qh = (c["quad"][3] - c["quad"][1]) * w["sy"]
    x0, y0, x1, y1 = qx*REF_W, qy*REF_H, (qx+qw)*REF_W, (qy+qh)*REF_H
    if x1 < x0: x0, x1 = x1, x0
    if y1 < y0: y0, y1 = y1, y0
    return x0, y0, x1 - x0, y1 - y0


def pose_score(sc, anim, frame):
    casts = sc["casts"]; cache = {}; score = 0
    for i, c in enumerate(casts):
        if not (c.get("quad") and c.get("tex") and c.get("uv")): continue
        w = world_of(casts, cache, i, anim, frame)
        if not w["vis"] or w["alpha"] <= 0.003: continue
        x, y, bw, bh = screen_rect(c, w)
        if bw < 0.5 or bh < 0.5 or bw > 1.5*REF_W or bh > 1.5*REF_H: continue
        if x+bw <= 0 or y+bh <= 0 or x >= REF_W or y >= REF_H: continue
        score += 1
    return score


def resolve_rest(sc):
    best_anim, best_frame, best = "", 0.0, pose_score(sc, "", 0)
    for a in sc.get("anims", []):
        maxf = a.get("frames", 0) or 0
        for k in range(9):
            f = maxf * k / 8 if maxf > 0 else 0.0
            s = pose_score(sc, a["name"], f)
            if s > best or (s == best and s > 0 and f > best_frame):
                best, best_anim, best_frame = s, a["name"], f
            if maxf <= 0: break
    return best_anim, best_frame


def resolve_screen(sid):
    d = json.load(open(os.path.join(DATA, sid + ".json"), encoding="utf-8"))
    nodes, z = [], 0
    for sc in d.get("scenes", []):
        if sc.get("rest") is False: continue
        anim, frame = resolve_rest(sc)
        casts = sc["casts"]; cache = {}
        for i, c in enumerate(casts):
            if not (c.get("quad") and c.get("tex") and c.get("uv")): continue
            w = world_of(casts, cache, i, anim, frame)
            if not w["vis"] or w["alpha"] <= 0.004: continue
            x, y, bw, bh = screen_rect(c, w)
            if bw < 0.5 or bh < 0.5 or bw > 2.5*REF_W or bh > 2.5*REF_H: continue
            if x+bw <= 0 or y+bh <= 0 or x >= REF_W or y >= REF_H: continue
            nodes.append({
                "tex": c["tex"], "rect": [round(x,1), round(y,1), round(bw,1), round(bh,1)],
                "uv": [round(v,5) for v in c["uv"]], "z": z,
                "alpha": round(w["alpha"], 3), "add": bool(c.get("add", 0)),
            })
            z += 1
    return {"screen": sid, "ref": [1280, 720], "nodes": nodes,
            "textures": sorted({n["tex"] for n in nodes})}


def main():
    os.makedirs(OUT, exist_ok=True)
    for sid in (sys.argv[1:] or ["status", "sonic_hud", "shop"]):
        out = resolve_screen(sid)
        with open(os.path.join(OUT, sid + ".json"), "w", encoding="utf-8") as f:
            json.dump(out, f, separators=(",", ":"))
        print(f"{sid:<10} {len(out['nodes'])} nodes, {len(out['textures'])} textures -> nodelists/{sid}.json")


if __name__ == "__main__":
    raise SystemExit(main())
