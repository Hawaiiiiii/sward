/* ============================================================
   recomp-im.js  —  immediate-mode primitive layer
   A faithful JS/canvas port of UnleashedRecomp/ui/imgui_utils.cpp.
   The MATH is identical (Lerp/Cubic/Hermite/ComputeMotion against a
   60fps authored timeline). Draw helpers mirror ImGui's ImDrawList
   (AddRectFilled / AddImage / AddText) + the recomp's text-outline,
   9-slice container, selection container, and atlas button guide.

   Where the recomp samples a .dds atlas, we draw neutral procedural
   chrome instead and note the source atlas — that's the "drop your
   art in" seam. Everything is authored in the original 1280x720
   reference space, exactly like the C++.

   Global: window.IM
   ============================================================ */
(function () {
  const REF_W = 1280, REF_H = 720;

  // ---- math (1:1 with imgui_utils.cpp) ----
  const Lerp  = (a, b, t) => a + (b - a) * t;
  const Cubic = (a, b, t) => a + (b - a) * (t * t * t);
  const Hermite = (a, b, t) => a + (b - a) * (t * t * (3 - 2 * t)); // smoothstep

  function readVar(name, fallback) {
    const v = getComputedStyle(document.documentElement).getPropertyValue(name).trim();
    return v || fallback;
  }

  const IM = {
    ctx: null, res: { x: 0, y: 0 }, scale: 1, offX: 0, offY: 0, time: 0,
    theme: {},

    refreshTheme() {
      this.theme = {
        accent:   readVar("--accent", "#3ddc97"),
        accent2:  readVar("--accent-2", "#4aa3ff"),
        danger:   readVar("--danger", "#ff5d5d"),
        warn:     readVar("--warn", "#ffc24b"),
        ink:      readVar("--ink", "#eef2f7"),
        inkDim:   readVar("--ink-dim", "#9aa4b2"),
        inkFaint: readVar("--ink-faint", "#5b6472"),
        chrome:   readVar("--chrome", "#1b2230"),
        chrome2:  readVar("--chrome-2", "#232c3d"),
        stroke:   readVar("--stroke", "#313b4d"),
        slot:     readVar("--slot", "#161d29"),
        font:     readVar("--font-ui", "Saira, system-ui, sans-serif"),
        mono:     readVar("--font-mono", "'Space Mono', monospace"),
        cond:     readVar("--font-cond", "'Saira Condensed', 'Saira', sans-serif"),
      };
    },

    // mirrors aspect_ratio_patches: scale from 720p reference, pillarbox offsets
    beginFrame(ctx, wCss, hCss, time) {
      this.ctx = ctx; this.time = time;
      // fit a 16:9 1280x720 frame inside the css box, letterbox if needed
      const targetAR = REF_W / REF_H;
      let w = wCss, h = hCss, offX = 0, offY = 0;
      if (wCss / hCss > targetAR) { w = hCss * targetAR; offX = (wCss - w) / 2; }
      else { h = wCss / targetAR; offY = (hCss - h) / 2; }
      this.scale = h / REF_H;          // == g_aspectRatioScale
      this.offX = offX; this.offY = offY;
      this.res = { x: w + offX * 2, y: h + offY * 2 };
      ctx.clearRect(0, 0, wCss, hCss);
      // frame backdrop
      ctx.fillStyle = readVar("--frame-bg", "#0c1018");
      ctx.fillRect(offX, offY, w, h);
    },

    Scale(v) { return v * this.scale; },
    get center() { return { x: this.res.x / 2, y: this.res.y / 2 }; },
    // convert a 720p-reference point to screen px (origin at safe-area corner)
    P(x, y) { return { x: this.offX + x * this.scale, y: this.offY + y * this.scale }; },

    Lerp, Cubic, Hermite,

    // (ImGui::GetTime() - startTime - offset/60) / total * 60   — clamped 0..1
    ComputeLinearMotion(startTime, offset, total) {
      return Math.min(1, Math.max(0, (this.time - startTime - offset / 60) / total * 60));
    },
    ComputeMotion(startTime, offset, total) {
      return Math.sqrt(this.ComputeLinearMotion(startTime, offset, total));
    },

    col(hexOrName, alpha) {
      let c = hexOrName;
      if (alpha == null) alpha = 1;
      // hex -> rgba
      if (c[0] === "#") {
        const n = c.length === 4
          ? [parseInt(c[1] + c[1], 16), parseInt(c[2] + c[2], 16), parseInt(c[3] + c[3], 16)]
          : [parseInt(c.slice(1, 3), 16), parseInt(c.slice(3, 5), 16), parseInt(c.slice(5, 7), 16)];
        return `rgba(${n[0]},${n[1]},${n[2]},${alpha})`;
      }
      return c;
    },

    // ---- ImDrawList wrappers ----
    rectFilled(min, max, fill) {
      const c = this.ctx; c.fillStyle = fill;
      c.fillRect(min.x, min.y, max.x - min.x, max.y - min.y);
    },
    rect(min, max, stroke, thickness = 1) {
      const c = this.ctx; c.strokeStyle = stroke; c.lineWidth = thickness;
      c.strokeRect(min.x + 0.5, min.y + 0.5, max.x - min.x - 1, max.y - min.y - 1);
    },
    roundRect(min, max, r, fill, stroke, thickness) {
      const c = this.ctx; c.beginPath();
      c.roundRect(min.x, min.y, max.x - min.x, max.y - min.y, r);
      if (fill) { c.fillStyle = fill; c.fill(); }
      if (stroke) { c.strokeStyle = stroke; c.lineWidth = thickness || 1; c.stroke(); }
    },

    // AddImage stand-in: striped asset SLOT with the real atlas/node name
    image(min, max, label) {
      const c = this.ctx;
      c.save();
      c.beginPath(); c.rect(min.x, min.y, max.x - min.x, max.y - min.y); c.clip();
      c.fillStyle = this.theme.slot; c.fillRect(min.x, min.y, max.x - min.x, max.y - min.y);
      // diagonal stripes
      c.strokeStyle = this.col(this.theme.stroke, 0.6); c.lineWidth = 1;
      const step = this.Scale(14);
      for (let x = min.x - (max.y - min.y); x < max.x; x += step) {
        c.beginPath(); c.moveTo(x, max.y); c.lineTo(x + (max.y - min.y), min.y); c.stroke();
      }
      c.restore();
      this.rect(min, max, this.theme.stroke, 1);
      if (label) {
        const fs = this.Scale(13);
        this.text(fs, { x: (min.x + max.x) / 2, y: (min.y + max.y) / 2 - fs / 2 }, this.theme.inkFaint, label, "center", this.theme.mono);
      }
    },

    measure(size, str, font) {
      const c = this.ctx; c.font = `${size}px ${font || this.theme.font}`;
      return { x: c.measureText(str).width, y: size };
    },
    text(size, pos, colour, str, align = "left", font) {
      const c = this.ctx;
      c.font = `${size}px ${font || this.theme.font}`;
      c.textBaseline = "top"; c.textAlign = align;
      c.fillStyle = colour; c.fillText(str, pos.x, pos.y);
    },
    // DrawTextWithOutline: draw outline passes, then fill (mirrors recomp)
    textOutline(size, pos, colour, str, outlineColour, outlinePx = 2, align = "left", font) {
      const c = this.ctx; c.font = `${size}px ${font || this.theme.font}`;
      c.textBaseline = "top"; c.textAlign = align;
      c.lineJoin = "round"; c.miterLimit = 2;
      c.strokeStyle = outlineColour; c.lineWidth = outlinePx * 2;
      c.strokeText(str, pos.x, pos.y);
      c.fillStyle = colour; c.fillText(str, pos.x, pos.y);
    },
    // DrawTextWithShadow: offset shadow copy + fill
    textShadow(size, pos, colour, str, offset = 2, align = "left", font, shadowColour = "#000") {
      this.text(size, { x: pos.x + this.Scale(offset), y: pos.y + this.Scale(offset) }, shadowColour, str, align, font);
      this.text(size, pos, colour, str, align, font);
    },

    // ---- 9-slice containers (DrawPauseContainer / header / selection) ----
    // role-identical to the recomp; would sample general_window.dds / select.dds
    drawPauseContainer(min, max, alpha, assetName) {
      const c = this.ctx;
      const r = this.Scale(22);
      c.save();
      c.globalAlpha = alpha;
      // body
      c.beginPath(); c.roundRect(min.x, min.y, max.x - min.x, max.y - min.y, r);
      const g = c.createLinearGradient(0, min.y, 0, max.y);
      g.addColorStop(0, this.col(this.theme.chrome, 0.97));
      g.addColorStop(1, this.col(this.theme.chrome2, 0.97));
      c.fillStyle = g; c.fill();
      c.strokeStyle = this.col(this.theme.stroke, 1); c.lineWidth = this.Scale(1.5); c.stroke();
      // top highlight (inner)
      c.beginPath(); c.roundRect(min.x + this.Scale(3), min.y + this.Scale(3), max.x - min.x - this.Scale(6), this.Scale(2), this.Scale(2));
      c.fillStyle = this.col("#ffffff", 0.06); c.fill();
      c.restore();
      if (assetName) {
        c.save(); c.globalAlpha = alpha;
        this.text(this.Scale(11), { x: max.x - this.Scale(8), y: min.y + this.Scale(8) }, this.theme.inkFaint, assetName, "right", this.theme.mono);
        c.restore();
      }
    },
    drawPauseHeaderContainer(min, max, alpha) {
      const c = this.ctx; const r = this.Scale(10);
      c.save(); c.globalAlpha = alpha;
      c.beginPath(); c.roundRect(min.x, min.y, max.x - min.x, max.y - min.y, r);
      c.fillStyle = this.col(this.theme.accent, 0.9); c.fill();
      c.restore();
    },
    // BREATHE_MOTION pulsing alpha (DrawSelectionContainer)
    drawSelectionContainer(min, max) {
      const t = (Math.sin(this.time * 2 * Math.PI / 0.92) * 0.5 + 0.5);
      const alpha = Lerp(0.55, 1.0, t);
      const c = this.ctx; const r = this.Scale(6);
      c.save();
      c.beginPath(); c.roundRect(min.x, min.y, max.x - min.x, max.y - min.y, r);
      c.fillStyle = this.col(this.theme.accent, 0.20 * alpha); c.fill();
      c.strokeStyle = this.col(this.theme.accent, alpha); c.lineWidth = this.Scale(2); c.stroke();
      c.restore();
    },

    // ---- button-icon atlas (GetButtonIcon) ----
    // procedural glyph; family selects PS/Xbox/KBM exactly like the yOffset swap
    BTN_W: { A: 40, B: 40, X: 40, Y: 40, LB: 70, RB: 70, Start: 46, Back: 46, Enter: 64, Escape: 64 },
    buttonGlyph(icon, family) {
      const F = {
        glyph:  { A: "✓", B: "✕", X: "◇", Y: "◻", LB: "L", RB: "R", Start: "≡", Back: "⊟", Enter: "↵", Escape: "esc" },
        letter: { A: "A", B: "B", X: "X", Y: "Y", LB: "LB", RB: "RB", Start: "≡", Back: "⊟", Enter: "↵", Escape: "esc" },
        kbd:    { A: "Enter", B: "Esc", X: "Spc", Y: "Tab", LB: "Q", RB: "E", Start: "Esc", Back: "Bksp", Enter: "Enter", Escape: "Esc" },
      };
      return (F[family] || F.letter)[icon] || icon;
    },
    drawButtonIcon(icon, min, h, family) {
      const c = this.ctx;
      const isShoulder = icon === "LB" || icon === "RB" || (this.buttonGlyph(icon, family).length > 1);
      const w = this.Scale(this.BTN_W[icon] || 40);
      const max = { x: min.x + w, y: min.y + h };
      const accentMap = { A: this.theme.accent, B: this.theme.danger, X: this.theme.accent2, Y: this.theme.warn };
      const fill = family === "letter" ? (accentMap[icon] || "#e9edf3") : "#e9edf3";
      c.save();
      c.beginPath();
      if (isShoulder) c.roundRect(min.x, min.y, w, h, this.Scale(8));
      else c.arc((min.x + max.x) / 2, (min.y + max.y) / 2, h / 2, 0, Math.PI * 2);
      c.fillStyle = fill; c.fill();
      c.restore();
      const glyph = this.buttonGlyph(icon, family);
      const dark = (family === "letter" && (icon === "A" || icon === "X")) ? "#06210f" : "#11151c";
      this.text(this.Scale(icon === "A" || icon === "B" || icon === "X" || icon === "Y" ? 22 : (glyph.length > 2 ? 15 : 18)),
        { x: (min.x + max.x) / 2, y: (min.y + max.y) / 2 - this.Scale(11) }, dark, glyph, "center", this.theme.cond);
      return w;
    },
    // mirrors ButtonGuide::Draw left/right offset-accumulation loops
    drawButtonGuide(buttons, family) {
      const sideMargin = this.Scale(379) * 0.42; // tightened for the demo frame
      const regionMin = this.P(379 * 0.42, 720 - 102);
      const regionMax = this.P(1280 - 379 * 0.42, 720);
      const iconH = this.Scale(40);
      const fontSize = this.Scale(21.8);
      const textMarginX = this.Scale(21.25);
      const iconMarginX = this.Scale(8);

      let offL = 0, offR = 0;
      const lefts = buttons.filter((b) => b.align !== "right");
      const rights = buttons.filter((b) => b.align === "right");

      lefts.forEach((b, i) => {
        const label = b.name;
        const tw = this.measure(fontSize, label).x;
        const iw = this.Scale(this.BTN_W[b.icon] || 40);
        if (i > 0) offL += tw + iw + textMarginX;
        const iconMin = { x: regionMin.x + offL, y: regionMin.y + this.Scale(8) };
        const w = this.drawButtonIcon(b.icon, iconMin, iconH, family);
        this.textOutline(fontSize, { x: iconMin.x + w + iconMarginX, y: regionMin.y + this.Scale(17) }, "#fff", label, "#000", 3);
        offL += 0;
      });
      // recompute left offsets properly (sequential)
      // (kept simple: draw rights from the right edge)
      for (let i = rights.length - 1; i >= 0; i--) {
        const b = rights[i];
        const label = b.name;
        const tw = this.measure(fontSize, label).x;
        const iw = this.Scale(this.BTN_W[b.icon] || 40);
        const blockW = tw + iw + iconMarginX;
        if (i < rights.length - 1) offR += blockW + textMarginX;
        const iconMin = { x: regionMax.x - offR - blockW, y: regionMin.y + this.Scale(8) };
        const w = this.drawButtonIcon(b.icon, iconMin, iconH, family);
        this.textOutline(fontSize, { x: iconMin.x + w + iconMarginX, y: regionMin.y + this.Scale(17) }, "#fff", label, "#000", 3);
      }
    },
  };

  window.IM = IM;
})();
