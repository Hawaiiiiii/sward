#!/usr/bin/env python3
"""diag_pose.py — replicate the C++ resolveRestPose per scene and report the chosen
(anim, frame, score) vs the base score and total drawable casts, to see which scenes
the auto-pose picks an empty/wrong pose for."""
import json, os, sys
import diag_casts as d
REF_W, REF_H = 1280.0, 720.0

def pose_score(sc, anim, frame):
    cache={}; score=0
    for i,c in enumerate(sc["casts"]):
        if not c.get("quad") or not c.get("tex") or not c.get("uv"): continue
        w=d.world_of(sc["casts"],cache,i,anim,frame)
        if not w["vis"] or w["alpha"]<=0.003: continue
        q=c["quad"]
        x0=(w["ox"]+q[0]*w["sx"])*REF_W; y0=(w["oy"]+q[1]*w["sy"])*REF_H
        x1=(w["ox"]+q[2]*w["sx"])*REF_W; y1=(w["oy"]+q[3]*w["sy"])*REF_H
        if x1<x0: x0,x1=x1,x0
        if y1<y0: y0,y1=y1,y0
        bw,bh=x1-x0,y1-y0
        if bw<0.5 or bh<0.5: continue
        if bw>1.5*REF_W or bh>1.5*REF_H: continue
        if x1<=0 or y1<=0 or x0>=REF_W or y0>=REF_H: continue
        score+=1
    return score

def resolve(sc):
    best=("",0.0,pose_score(sc,"",0.0))   # base
    for nm,frames in [(a["name"],a["frames"]) for a in sc["anims"]]:
        S=8
        ks = range(S+1) if frames>0 else [0]
        for k in ks:
            f = frames*k/S if frames>0 else 0.0
            s=pose_score(sc,nm,f)
            if s>best[2] or (s==best[2] and s>0 and f>best[1]):
                best=(nm,f,s)
    return best

def main():
    screen=sys.argv[1] if len(sys.argv)>1 else "world_map"
    dump = "--dump" in sys.argv
    S=json.load(open(os.path.join(d.DATA,screen+".json"),encoding="utf-8"))
    if dump:
        rows=[]
        for sc in S["scenes"]:
            anim,frame,_=resolve(sc); cache={}
            for c in sc["casts"]:
                if not c.get("quad") or not c.get("tex") or not c.get("uv"): continue
                w=d.world_of(sc["casts"],cache,sc["casts"].index(c),anim,frame)
                if not w["vis"] or w["alpha"]<=0.003: continue
                q=c["quad"]
                x0=(w["ox"]+q[0]*w["sx"])*REF_W; y0=(w["oy"]+q[1]*w["sy"])*REF_H
                x1=(w["ox"]+q[2]*w["sx"])*REF_W; y1=(w["oy"]+q[3]*w["sy"])*REF_H
                if x1<x0: x0,x1=x1,x0
                if y1<y0: y0,y1=y1,y0
                bw,bh=x1-x0,y1-y0
                if bw<0.5 or bh<0.5 or bw>2.5*REF_W or bh>2.5*REF_H: continue
                rows.append((bw*bh, sc["name"].split("/")[-1], c["id"], bw,bh,x0,y0,w["alpha"]))
        rows.sort(reverse=True)
        print(f"{'area':>11} {'scene':<22} {'cast':<18} {'w x h':>13} {'pos':>13} {'a':>4}")
        for r in rows[:25]:
            ar,sn,ci,bw,bh,x0,y0,al=r
            print(f"{ar:11.0f} {sn:<22.22} {ci:<18.18} {bw:6.0f}x{bh:<6.0f} {x0:5.0f},{y0:<5.0f} {al:4.2f}")
        return
    print(f"{'scene':<40} {'total':>5} {'base':>5} {'chosen anim':<18} {'frame':>7} {'score':>5}")
    for sc in S["scenes"]:
        total=sum(1 for c in sc["casts"] if c.get("quad") and c.get("tex") and c.get("uv"))
        base=pose_score(sc,"",0.0)
        anim,frame,score=resolve(sc)
        nm=sc["name"].split("/",1)[-1]
        print(f"{nm:<40.40} {total:>5} {base:>5} {anim:<18.18} {frame:>7.1f} {score:>5}")

if __name__=="__main__":
    main()
