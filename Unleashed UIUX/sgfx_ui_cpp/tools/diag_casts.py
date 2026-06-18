#!/usr/bin/env python3
"""diag_casts.py — replicate the C++ CSD transform and dump the largest-area casts
for a screen at a given anim frame, to identify what's flooding the screen."""
import json, os, sys, math

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, "..", "data")
REF_W, REF_H = 1280.0, 720.0

def eval_track(kf, frame):
    if not kf: return 0.0
    if frame <= kf[0]["f"]: return kf[0]["v"]
    if frame >= kf[-1]["f"]: return kf[-1]["v"]
    for i in range(len(kf)-1):
        a, b = kf[i], kf[i+1]
        if a["f"] <= frame <= b["f"]:
            dt = float(b["f"]-a["f"])
            if dt <= 0: return a["v"]
            t = (frame-a["f"])/dt
            if a["t"] == "Const": return a["v"]
            if a["t"] == "Linear": return a["v"]+(b["v"]-a["v"])*t
            t2,t3 = t*t, t*t*t
            return ((2*t3-3*t2+1)*a["v"] + (t3-2*t2+t)*(a["ot"]*dt)
                    + (-2*t3+3*t2)*b["v"] + (t3-t2)*(b["it"]*dt))
    return kf[-1]["v"]

def eval_cast(c, anim, frame):
    b = c["base"]
    color = b.get("color","0xFFFFFFFF")
    a = (int(color,16)>>24)&0xFF if isinstance(color,str) else 255
    x = dict(tx=b["tx"],ty=b["ty"],sx=b["sx"],sy=b["sy"],rot=b["rot"],
             alpha=a/255.0, vis=(b.get("hide",0)==0))
    an = c["anims"].get(anim)
    if an:
        tr = an["tracks"]
        if "XPosition" in tr: x["tx"]=eval_track(tr["XPosition"],frame)
        if "YPosition" in tr: x["ty"]=eval_track(tr["YPosition"],frame)
        if "XScale" in tr: x["sx"]=eval_track(tr["XScale"],frame)
        if "YScale" in tr: x["sy"]=eval_track(tr["YScale"],frame)
        if "Rotation" in tr: x["rot"]=eval_track(tr["Rotation"],frame)
        if "Color" in tr: x["alpha"]=max(0.0,min(1.0,eval_track(tr["Color"],frame)))
        if "HideFlag" in tr: x["vis"]=eval_track(tr["HideFlag"],frame)<0.5
    return x

def world_of(casts, cache, i, anim, frame):
    if i in cache: return cache[i]
    x = eval_cast(casts[i], anim, frame)
    p = casts[i]["parent"]
    if p < 0:
        w = dict(ox=x["tx"],oy=x["ty"],sx=x["sx"],sy=x["sy"],rot=x["rot"],alpha=x["alpha"],vis=x["vis"])
    else:
        pw = world_of(casts, cache, p, anim, frame)
        w = dict(ox=pw["ox"]+x["tx"]*pw["sx"], oy=pw["oy"]+x["ty"]*pw["sy"],
                 sx=pw["sx"]*x["sx"], sy=pw["sy"]*x["sy"], rot=pw["rot"]+x["rot"],
                 alpha=pw["alpha"]*x["alpha"], vis=pw["vis"] and x["vis"])
    cache[i]=w; return w

def pick_intro(sc):
    """Choose the Intro anim for THIS scene. Scenes author different Intro names
    (rank scenes use Intro_Anim; num/title scenes use Intro_ev_Anim/Intro_so_Anim),
    so the pick must be per-scene — a single global name leaves the scenes that
    don't use it falling back to BASE transforms and reporting bogus giant sizes."""
    names = [a["name"] for a in sc["anims"]]
    for pref in ("Intro_so_Anim", "Intro_ev_Anim", "Intro_Anim"):
        if pref in names: return pref
    for n in names:
        if "Intro" in n: return n
    return names[0] if names else ""

def main():
    screen = sys.argv[1] if len(sys.argv)>1 else "pause"
    frame = float(sys.argv[2]) if len(sys.argv)>2 else 40.0
    S = json.load(open(os.path.join(DATA, screen+".json"), encoding="utf-8"))
    print(f"screen={screen} anim=<per-scene Intro> frame={frame}")
    rows=[]
    for sc in S["scenes"]:
        anim = pick_intro(sc)
        maxf=max([a["frames"] for a in sc["anims"] if a["name"]==anim] or [0])
        f=min(frame,maxf) if maxf>0 else frame
        cache={}
        for i,c in enumerate(sc["casts"]):
            if not c.get("quad") or not c.get("tex") or not c.get("uv"): continue
            w=world_of(sc["casts"],cache,i,anim,f)
            if not w["vis"] or w["alpha"]<=0.003: continue
            q=c["quad"]
            x0=(w["ox"]+q[0]*w["sx"])*REF_W; y0=(w["oy"]+q[1]*w["sy"])*REF_H
            x1=(w["ox"]+q[2]*w["sx"])*REF_W; y1=(w["oy"]+q[3]*w["sy"])*REF_H
            if x1<x0: x0,x1=x1,x0
            if y1<y0: y0,y1=y1,y0
            bw,bh=x1-x0,y1-y0
            rows.append((bw*bh, sc["name"].split("/")[-1], c["id"], c["tex"],
                         bw,bh, x0,y0, w["alpha"], c["uv"], anim))
    rows.sort(reverse=True)
    print(f"{'area':>11} {'scene':<14} {'cast':<22} {'tex':<22} {'w x h':>13} {'pos':>13} {'a':>4} {'anim':<14} uv")
    for r in rows[:20]:
        area,scene,cid,tex,bw,bh,x0,y0,al,uv,anim = r
        print(f"{area:11.0f} {scene:<14.14} {cid:<22.22} {tex:<22.22} {bw:6.0f}x{bh:<6.0f} {x0:5.0f},{y0:<5.0f} {al:4.2f} {anim:<14.14} [{uv[0]:.2f},{uv[1]:.2f},{uv[2]:.2f},{uv[3]:.2f}]")

if __name__=="__main__":
    main()
