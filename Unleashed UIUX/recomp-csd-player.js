/* ============================================================
   recomp-csd-player.js — plays the ACTUAL Sonic Unleashed CSD
   animations in the browser, driven by the state-machine DB.

   Two modes:
   • RESTING — shows the settled resting layout (scenes flagged rest),
     each scene playing its settled anim. (screens with no contract)
   • CONTRACT — walks the screen's state machine from window.SWARD_CONTRACTS
     (Intro -> Idle -> Navigate -> Confirm -> Outro ...), spending each
     state's timeline-band duration, playing that state's animation across
     the scenes, and bringing alt-state scenes (submenus/reveals/popups)
     in during the states that own them. i.e. ALL the states the game has.

   Per frame it evaluates each cast's CSD keyframes (Const/Linear/Hermite),
   composes the cast hierarchy, and draws each cast's UV-cropped sprite.
   Port of sgfx_ui_cpp/src/main.cpp + tools/diag_casts.py (verified).
   ============================================================ */
(function () {
  "use strict";
  const REF_W = 1280, REF_H = 720;

  function evalTrack(kf, frame) {
    if (!kf.length) return 0;
    if (frame <= kf[0].f) return kf[0].v;
    if (frame >= kf[kf.length - 1].f) return kf[kf.length - 1].v;
    for (let i = 0; i < kf.length - 1; i++) {
      const a = kf[i], b = kf[i + 1];
      if (frame >= a.f && frame <= b.f) {
        const dt = b.f - a.f; if (dt <= 0) return a.v;
        const t = (frame - a.f) / dt;
        if (a.t === "Const") return a.v;
        if (a.t === "Linear") return a.v + (b.v - a.v) * t;
        const t2 = t * t, t3 = t2 * t;
        return (2 * t3 - 3 * t2 + 1) * a.v + (t3 - 2 * t2 + t) * (a.ot * dt)
             + (-2 * t3 + 3 * t2) * b.v + (t3 - t2) * (b.it * dt);
      }
    }
    return kf[kf.length - 1].v;
  }

  // CSD cast colors are packed 0xRRGGBBAA (alpha = LOW byte). The base color is a
  // "0x........" string; Color/Gradient animation tracks store the same packed value
  // as an integer per keyframe.
  function decodeRGBA(n) {
    n = n >>> 0;
    return { r: (n >>> 24) & 255, g: (n >>> 16) & 255, b: (n >>> 8) & 255, a: n & 255 };
  }
  function parseBaseColor(s) {
    if (typeof s === "string" && s.length === 10) return decodeRGBA(parseInt(s.slice(2), 16));
    if (typeof s === "number") return decodeRGBA(s);
    return { r: 255, g: 255, b: 255, a: 255 };
  }
  // interpolate a color track (packed-int keyframes) per channel -> {r,g,b,a} in 0..255
  function evalColorTrack(kf, frame) {
    if (!kf || !kf.length) return { r: 255, g: 255, b: 255, a: 255 };
    if (frame <= kf[0].f) return decodeRGBA(kf[0].v);
    if (frame >= kf[kf.length - 1].f) return decodeRGBA(kf[kf.length - 1].v);
    for (let i = 0; i < kf.length - 1; i++) {
      const a = kf[i], b = kf[i + 1];
      if (frame >= a.f && frame <= b.f) {
        const dt = b.f - a.f; if (dt <= 0) return decodeRGBA(a.v);
        if (a.t === "Const") return decodeRGBA(a.v);
        const t = (frame - a.f) / dt, A = decodeRGBA(a.v), B = decodeRGBA(b.v);
        return { r: A.r + (B.r - A.r) * t, g: A.g + (B.g - A.g) * t,
                 b: A.b + (B.b - A.b) * t, a: A.a + (B.a - A.a) * t };
      }
    }
    return decodeRGBA(kf[kf.length - 1].v);
  }
  function avgCorner(tracks, frame) {
    const present = ["GradientTL", "GradientTR", "GradientBL", "GradientBR"]
      .map((k) => tracks[k]).filter(Boolean).map((t) => evalColorTrack(t, frame));
    if (!present.length) return null;
    const s = present.reduce((o, c) => ({ r: o.r + c.r, g: o.g + c.g, b: o.b + c.b, a: o.a + c.a }),
      { r: 0, g: 0, b: 0, a: 0 });
    const n = present.length;
    return { r: s.r / n, g: s.g / n, b: s.b / n, a: s.a / n };
  }

  function evalCast(c, anim, frame) {
    const b = c.base;
    const bc = parseBaseColor(b.color);
    const x = { tx: b.tx, ty: b.ty, sx: b.sx, sy: b.sy, rot: b.rot,
                alpha: bc.a / 255, tr: bc.r, tg: bc.g, tb: bc.b, vis: (b.hide || 0) === 0 };
    const ca = c.anims && c.anims[anim];
    if (ca) {
      const tr = ca.tracks;
      if (tr.XPosition) x.tx = evalTrack(tr.XPosition, frame);
      if (tr.YPosition) x.ty = evalTrack(tr.YPosition, frame);
      if (tr.XScale)    x.sx = evalTrack(tr.XScale, frame);
      if (tr.YScale)    x.sy = evalTrack(tr.YScale, frame);
      if (tr.Rotation)  x.rot = evalTrack(tr.Rotation, frame);
      if (tr.HideFlag)  x.vis = evalTrack(tr.HideFlag, frame) < 0.5;
      const col = tr.Color ? evalColorTrack(tr.Color, frame) : avgCorner(tr, frame);
      if (col) { x.alpha = Math.max(0, Math.min(1, col.a / 255)); x.tr = col.r; x.tg = col.g; x.tb = col.b; }
    }
    return x;
  }

  function worldOf(casts, cache, i, anim, frame) {
    if (cache[i]) return cache[i];
    const x = evalCast(casts[i], anim, frame);
    const p = casts[i].parent;
    let w;
    if (p < 0) {
      w = { ox: x.tx, oy: x.ty, sx: x.sx, sy: x.sy, rot: x.rot, alpha: x.alpha, vis: x.vis,
            tr: x.tr, tg: x.tg, tb: x.tb };
    } else {
      const pw = worldOf(casts, cache, p, anim, frame);
      w = { ox: pw.ox + x.tx * pw.sx, oy: pw.oy + x.ty * pw.sy,
            sx: pw.sx * x.sx, sy: pw.sy * x.sy, rot: pw.rot + x.rot,
            alpha: pw.alpha * x.alpha, vis: pw.vis && x.vis,
            tr: pw.tr * x.tr / 255, tg: pw.tg * x.tg / 255, tb: pw.tb * x.tb / 255 };
    }
    cache[i] = w; return w;
  }

  // settled/resting anim for a scene: prefer Usual/idle loop, else Intro, else first
  function pickRestAnim(scene) {
    const a = scene.anims || [];
    const f = (re) => a.find((x) => re.test(x.name));
    const loop = f(/usual|idle|loop/i);
    if (loop) return loop.name;
    const intro = f(/intro/i) || a[0];
    return intro ? intro.name : "";
  }

  // which anim a scene should play for a given state (regex over its anim names)
  function matchAnim(scene, re) {
    const hit = (scene.anims || []).find((x) => re.test(x.name));
    return hit ? hit.name : null;
  }

  // state name -> the anim-name pattern the game plays in that state
  const STATE_PATTERN = {
    Boot: /intro/i, Intro: /intro|reveal|appear|_in\b|enter/i, MoviePrep: /intro|in\b/i,
    Idle: /usual|idle|loop|hold|sustain|default|battle|play|browse/i,
    Live: /usual|idle|loop|play|live|battle/i, Loop: /loop|usual|info|card/i,
    Saving: /spin|usual|loop/i, Hidden: /usual/i,
    Navigate: /move|switch|select|browse|scroll|focus|pulse|travel/i,
    Confirm: /select|decide|confirm|reveal|press|flash|get|combo|break/i,
    Cancel: /cancel|back|close|out/i,
    Outro: /outro|out\b|close|disappear|cleanup|fade/i, Closed: /outro|out/i,
  };
  function statePattern(name) { return STATE_PATTERN[name] || /usual|idle/i; }

  class CsdPlayer {
    constructor(canvas, screen, assetBase, opts) {
      opts = opts || {};
      this.canvas = canvas;
      this.ctx = canvas.getContext("2d");
      this.screen = screen;
      this.assetBase = assetBase;
      this.contract = opts.contract || null;
      this.framerate = screen.framerate || 60;
      this.textures = {};
      this._tintCache = new Map();
      this.raf = 0;
      this.startMs = performance.now();
      this._prepare();
      this._loadTextures();
      this._tick = this._tick.bind(this);
      this.raf = requestAnimationFrame(this._tick);
    }

    _prepare() {
      const all = (this.screen.scenes || []).map((sc) => ({
        casts: sc.casts || [], anims: sc.anims || [], rest: sc.rest !== false,
        restAnim: pickRestAnim(sc),
      }));
      this.scenes = all;
      if (this.contract) this._setupTimeline();
    }

    _setupTimeline() {
      const c = this.contract;
      const bands = {}; for (const b of (c.timeline_bands || [])) bands[b.id] = b.seconds;
      const stateByName = {}; for (const s of (c.states || [])) stateByName[s.state] = s;
      const walk = (c.walk && c.walk.length) ? c.walk : (c.states || []).map((s) => s.state);
      this.timeline = walk.map((name) => {
        const st = stateByName[name] || {};
        let dur = st.timeline_band_id ? (bands[st.timeline_band_id] || 1.0) : 0;
        if (dur <= 0) dur = (name === "Idle" || name === "Live" || name === "Loop" || name === "Await") ? 2.6 : 0.45;
        const roles = (c.visible_overlay_roles && c.visible_overlay_roles[name]) || null;
        return { name, dur, pat: statePattern(name), roles, input: !!st.input_enabled };
      }).filter((e) => e.name !== "Boot" && e.name !== "Closed" && e.name !== "Hidden");
      this.totalDur = Math.max(0.5, this.timeline.reduce((a, e) => a + e.dur, 0));
    }

    _loadTextures() {
      const names = new Set();
      for (const t of (this.screen.textures || [])) names.add(t);
      for (const sc of this.scenes) for (const c of sc.casts) if (c.tex) names.add(c.tex);
      for (const name of names) { const img = new Image(); img.src = this.assetBase + name + ".png"; this.textures[name] = img; }
    }

    _tick() {
      const c = this.canvas, w = c.clientWidth | 0, h = c.clientHeight | 0;
      if (w > 0 && h > 0 && (c.width !== w || c.height !== h)) { c.width = w; c.height = h; }
      if (this.contract) this._drawContract(performance.now() - this.startMs);
      else this._drawResting(performance.now() - this.startMs);
      this.raf = requestAnimationFrame(this._tick);
    }

    replay() { this.startMs = performance.now(); }
    curStateName() { return this._curState ? this._curState.name : ""; }

    // ---- resting mode: settled layout, each rest scene plays its settled anim ----
    _drawResting(ms) {
      this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
      const f = ms / 1000 * this.framerate;
      for (const sc of this.scenes) {
        if (!sc.rest) continue;
        const maxF = (sc.anims.find((a) => a.name === sc.restAnim) || {}).frames || 0;
        const frame = maxF > 0 ? Math.min(f, maxF) : f;
        this._drawScene(sc, sc.restAnim, frame);
      }
    }

    // ---- contract mode: walk the state machine, play each state's animation ----
    _drawContract(ms) {
      this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
      let t = (ms / 1000) % this.totalDur, acc = 0, cur = this.timeline[0], stateT = 0;
      for (const e of this.timeline) { if (t < acc + e.dur) { cur = e; stateT = t - acc; break; } acc += e.dur; }
      this._curState = cur;
      const looping = (cur.name === "Idle" || cur.name === "Live" || cur.name === "Loop");
      for (const sc of this.scenes) {
        const stateAnim = matchAnim(sc, cur.pat);
        // a scene shows when: it's part of the resting layout, OR it has the active
        // state's animation (so submenus/reveals/popups appear in their own state).
        if (!sc.rest && !stateAnim) continue;
        const anim = stateAnim || sc.restAnim;
        const maxF = (sc.anims.find((a) => a.name === anim) || {}).frames || 0;
        let frame = stateT * this.framerate;
        if (maxF > 0) frame = looping ? (frame % maxF) : Math.min(frame, maxF);
        this._drawScene(sc, anim, frame);
      }
    }

    _drawScene(sc, anim, frame) {
      const ctx = this.ctx, W = this.canvas.width, H = this.canvas.height;
      const sX = W / REF_W, sY = H / REF_H;
      const casts = sc.casts, cache = new Array(casts.length);
      for (let i = 0; i < casts.length; i++) {
        const c = casts[i];
        if (!c.quad || !c.tex || !c.uv) continue;
        const w = worldOf(casts, cache, i, anim, frame);
        if (!w.vis || w.alpha <= 0.004) continue;
        let qx = (w.ox + c.quad[0] * w.sx), qy = (w.oy + c.quad[1] * w.sy);
        let qw = (c.quad[2] - c.quad[0]) * w.sx, qh = (c.quad[3] - c.quad[1]) * w.sy;
        let X = qx * REF_W, Y = qy * REF_H, BW = qw * REF_W, BH = qh * REF_H;
        if (BW < 0) { X += BW; BW = -BW; }
        if (BH < 0) { Y += BH; BH = -BH; }
        if (BW < 0.5 || BH < 0.5 || BW > 2.5 * REF_W || BH > 2.5 * REF_H) continue;
        const img = this.textures[c.tex];
        if (!img || !img.complete || !img.naturalWidth) continue;
        const u0 = c.uv[0] * img.naturalWidth, v0 = c.uv[1] * img.naturalHeight;
        const uw = (c.uv[2] - c.uv[0]) * img.naturalWidth, uh = (c.uv[3] - c.uv[1]) * img.naturalHeight;
        if (uw < 0.5 || uh < 0.5) continue;
        const tr = w.tr | 0, tg = w.tg | 0, tb = w.tb | 0;
        const tinted = (tr < 250 || tg < 250 || tb < 250)
          ? this._tinted(c.tex, img, u0, v0, uw, uh, tr, tg, tb) : null;
        ctx.save();
        ctx.globalAlpha = Math.max(0, Math.min(1, w.alpha));
        if (c.add) ctx.globalCompositeOperation = "lighter";   // additive glow/shine
        const cx = (X + BW / 2) * sX, cy = (Y + BH / 2) * sY;
        ctx.translate(cx, cy);
        if (w.rot) ctx.rotate(w.rot * Math.PI / 180);
        const dx = -BW / 2 * sX, dy = -BH / 2 * sY, dw = BW * sX, dh = BH * sY;
        if (tinted) ctx.drawImage(tinted, 0, 0, tinted.width, tinted.height, dx, dy, dw, dh);
        else ctx.drawImage(img, u0, v0, uw, uh, dx, dy, dw, dh);
        ctx.restore();
      }
    }

    // Cropped sprite multiplied by a (non-white) vertex tint, cached. CSD multiplies the
    // cast color into the texture; canvas can't do that inline, so bake it once per
    // (texture, uv-rect, tint) into an offscreen and reuse it every frame.
    _tinted(tex, img, u0, v0, uw, uh, r, g, b) {
      const key = tex + "|" + Math.round(u0) + "," + Math.round(v0) + "," + Math.round(uw) + "," + Math.round(uh) + "|" + r + "," + g + "," + b;
      let oc = this._tintCache.get(key);
      if (oc) return oc;
      const W = Math.max(1, Math.round(uw)), H = Math.max(1, Math.round(uh));
      oc = document.createElement("canvas"); oc.width = W; oc.height = H;
      const o = oc.getContext("2d");
      o.drawImage(img, u0, v0, uw, uh, 0, 0, W, H);            // cropped sprite
      o.globalCompositeOperation = "multiply";                 // RGB *= tint
      o.fillStyle = "rgb(" + r + "," + g + "," + b + ")";
      o.fillRect(0, 0, W, H);
      o.globalCompositeOperation = "destination-in";           // re-clip to sprite alpha
      o.drawImage(img, u0, v0, uw, uh, 0, 0, W, H);
      this._tintCache.set(key, oc);
      return oc;
    }

    stop() { if (this.raf) cancelAnimationFrame(this.raf); this.raf = 0; }
  }

  // viewer screen id -> contract key in window.SWARD_CONTRACTS
  window.CSD_CONTRACT_FOR = {
    title: "TitleMenu", pause: "PauseMenu", options: "OptionsMenu", world_map: "WorldMap",
    loading: "LoadingTransition", result: "MissionResult", result_ex: "MissionResult",
    sonic_hud: "SonicStageHUD", boss: "BossHUD",
  };
  window.CsdPlayer = CsdPlayer;
})();
