#!/usr/bin/env python3
"""Render a flat composite PNG of a reconstructed screen (manifest rects + filled
assets) so you can eyeball the layout without the live viewer. Output: _preview/<id>.png."""
import json, sys
from pathlib import Path
from PIL import Image

HERE = Path(__file__).resolve().parent
VIEWER = HERE.parent
MAN = VIEWER / "manifests"
ASSETS = VIEWER / "assets"
OUT = VIEWER / "_preview"
SCALE = 0.75  # 1280x720 -> 960x540


def contain(img, bw, bh):
    iw, ih = img.size
    if iw == 0 or ih == 0:
        return img, 0, 0
    r = min(bw / iw, bh / ih)
    nw, nh = max(1, int(iw * r)), max(1, int(ih * r))
    return img.resize((nw, nh)), (bw - nw) // 2, (bh - nh) // 2


def render(mid):
    m = json.loads((MAN / f"{mid}.json").read_text(encoding="utf-8"))
    rw, rh = m.get("ref", [1280, 720])
    W, H = int(rw * SCALE), int(rh * SCALE)
    canvas = Image.new("RGBA", (W, H), (14, 18, 26, 255))
    for n in m["nodes"]:
        if n.get("type") == "text" or not n.get("tex"):
            continue
        x, y, w, h = n["rect"]
        bx, by, bw, bh = int(x * SCALE), int(y * SCALE), max(1, int(w * SCALE)), max(1, int(h * SCALE))
        p = ASSETS / mid / n["tex"]
        if not p.exists():
            continue
        try:
            img = Image.open(p).convert("RGBA")
        except Exception:
            continue
        uv = n.get("uv")
        if uv:
            iw, ih = img.size
            box = (int(uv[0] * iw), int(uv[1] * ih),
                   max(int(uv[0] * iw) + 1, int(uv[2] * iw)),
                   max(int(uv[1] * ih) + 1, int(uv[3] * ih)))
            img = img.crop(box)
            img = img.resize((bw, bh)); ox = oy = 0   # uv path stretches to fill (matches image-slot)
        elif n.get("fit", "contain") in ("cover", "fill"):
            img = img.resize((bw, bh)); ox = oy = 0
        else:
            img, ox, oy = contain(img, bw, bh)
        a = n.get("alpha")
        if a is not None and a < 1:
            alpha = img.split()[3].point(lambda v: int(v * a))
            img.putalpha(alpha)
        canvas.alpha_composite(img, (bx + ox, by + oy))
    OUT.mkdir(exist_ok=True)
    out = OUT / f"{mid}.png"
    canvas.convert("RGB").save(out)
    print("wrote", out, canvas.size)


if __name__ == "__main__":
    ids = sys.argv[1:] or ["pause", "world_map", "result", "boss", "loading"]
    for i in ids:
        render(i)
