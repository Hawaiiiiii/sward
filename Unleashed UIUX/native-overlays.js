/* ============================================================
   native-overlays.js — 1:1 ports of the recomp's OWN native UI
   These mirror, function-for-function, the Hedge Dev source:
     Fader            <- ui/fader.cpp
     ButtonGuide      <- ui/button_guide.cpp
     AchievementOverlay <- ui/achievement_overlay.cpp
     MessageWindow    <- ui/message_window.cpp
     PauseDemo        <- ui/imgui_utils.cpp (DrawPauseContainer shell)
   Same static fields, same motion constants, same Draw/Open/Close
   shape — just rendered through window.IM instead of ImGui.
   Each exposes: meta (for inspector), reset(), tick(t,playing),
   Draw(family), phase(), and input hooks where relevant.
   ============================================================ */
(function () {
  const IM = window.IM;

  /* ---------------- Fader  (fader.cpp) ---------------- */
  const Fader = {
    key: "Fader",
    meta: {
      label: "Fader", source: "ui/fader.cpp",
      blurb: "Single serialized owner. A new fade is rejected while one is active (g_isFading). On completion fires g_endCallback — 'finish the transition, THEN do the disruptive action'.",
      constants: [["FadeIn", "Lerp(1,0,t)"], ["FadeOut", "Lerp(0,1,t)"], ["serialized", "g_isFading guard"]],
    },
    sIsVisible: false, isFading: false, isFadeIn: false, startTime: 0, duration: 1.0, colour: "#000",
    DoFade(isFadeIn, duration) {
      if (this.isFading) return;
      this.isFading = true; this.isFadeIn = isFadeIn; this.startTime = IM.time;
      this.duration = duration; this.sIsVisible = true;
    },
    FadeIn(d) { this.DoFade(true, d); }, FadeOut(d) { this.DoFade(false, d); },
    reset() { this.sIsVisible = false; this.isFading = false; this._dir = 0; },
    tick() {
      if (!this.isFading) {
        // director: alternate fade out / in for the demo
        this._dir = this._dir ? 0 : 1;
        if (this._dir) this.FadeOut(1.2); else this.FadeIn(1.2);
      }
    },
    phase() { return this.isFading ? (this.isFadeIn ? "FadeIn" : "FadeOut") : "idle"; },
    Draw() {
      // a scene placeholder so the fade reads against something
      IM.image(IM.P(0, 0), IM.P(1280, 720), "scene  ·  fade target");
      if (!this.sIsVisible) return;
      let t = (IM.time - this.startTime) / this.duration;
      let alpha = 1.0;
      if (t >= 1.0) { this.isFading = false; if (this.isFadeIn) this.sIsVisible = false; }
      else alpha = this.isFadeIn ? IM.Lerp(1, 0, t) : IM.Lerp(0, 1, t);
      if (this.isFadeIn && !this.isFading) return;
      IM.rectFilled(IM.P(0, 0), IM.P(1280, 720), IM.col(this.colour, alpha));
      IM.text(IM.Scale(13), IM.P(20, 24), IM.theme.inkFaint, `Fader::${this.isFadeIn ? "FadeIn" : "FadeOut"}  alpha=${alpha.toFixed(2)}`, "left", IM.theme.mono);
    },
  };

  /* ---------------- ButtonGuide  (button_guide.cpp) ---------------- */
  const ButtonGuide = {
    key: "ButtonGuide",
    meta: {
      label: "Button Guide", source: "ui/button_guide.cpp",
      blurb: "Bottom prompt row. Left & right groups accumulate an offset as they lay out; each icon is a UV sub-rect of controller.dds (PS vs Xbox = a Y-offset into the atlas). Text drawn with outline via DrawTextWithOutline.",
      constants: [["DEFAULT_SIDE_MARGINS", "379"], ["fontSize", "Scale(21.8)"], ["icon", "Scale(40) (LB/RB 70)"]],
    },
    sIsVisible: true,
    buttons: [
      { name: "Select", icon: "A", align: "left" },
      { name: "Back", icon: "B", align: "left" },
      { name: "Details", icon: "X", align: "right" },
      { name: "Help", icon: "RB", align: "right" },
    ],
    reset() { this.sIsVisible = true; },
    tick() {},
    phase() { return "visible"; },
    Draw(family) {
      IM.image(IM.P(0, 0), IM.P(1280, 612), "host screen");
      // a footer band so the guide reads
      IM.rectFilled(IM.P(0, 618), IM.P(1280, 720), IM.col("#0d1420", 0.85));
      if (!this.sIsVisible) return;
      IM.drawButtonGuide(this.buttons, family);
    },
  };

  /* ------------- AchievementOverlay (achievement_overlay.cpp) ------------- */
  const AchievementOverlay = {
    key: "AchievementOverlay",
    meta: {
      label: "Achievement Toast", source: "ui/achievement_overlay.cpp",
      blurb: "Queue-driven. Dequeues only when the sound subsystem is ready (CanDequeueAchievement, main thread). Container Hermite-expands from centre + alpha fades; OVERLAY_DURATION=3 then auto-Close. Non-reentrant via g_isClosing.",
      constants: [["OVERLAY_DURATION", "3"], ["common motion", "0–11f"], ["intro fade", "5–9f"], ["outro fade", "0–4f"]],
    },
    COMMON_END: 11, INTRO_S: 5, INTRO_E: 9, OUTRO_S: 0, OUTRO_E: 4, DURATION: 3,
    sIsVisible: false, isClosing: false, appearTime: 0, _next: 0,
    Open() { this.sIsVisible = true; this.isClosing = false; this.appearTime = IM.time; },
    Close() { if (!this.isClosing) { this.appearTime = IM.time; this.isClosing = true; } },
    reset() { this.Open(); this.appearTime = IM.time - 0.6; this.isClosing = false; this._next = IM.time + 100; },
    tick() {
      if (!this.sIsVisible && IM.time >= this._next) this.Open();
      if (this.sIsVisible && !this.isClosing && (IM.time - this.appearTime >= this.DURATION)) this.Close();
      if (this.sIsVisible && this.isClosing) {
        const m = IM.ComputeMotion(this.appearTime, this.OUTRO_S, this.OUTRO_E);
        if (m >= 1) { this.sIsVisible = false; this._next = IM.time + 1.2; }
      }
    },
    phase() {
      if (!this.sIsVisible) return "queue idle";
      if (this.isClosing) return "outro";
      return (IM.time - this.appearTime) < 0.2 ? "intro" : "hold";
    },
    drawContainer(min, max) {
      const cm = IM.ComputeMotion(this.appearTime, 0, this.COMMON_END);
      const cx = (min.x + max.x) / 2, cy = (min.y + max.y) / 2;
      let a = { x: min.x, y: min.y }, b = { x: max.x, y: max.y };
      if (this.isClosing) {
        a.x = IM.Hermite(min.x, cx, cm); b.x = IM.Hermite(max.x, cx, cm);
        a.y = IM.Hermite(min.y, cy, cm); b.y = IM.Hermite(max.y, cy, cm);
      } else {
        a.x = IM.Hermite(cx, min.x, cm); b.x = IM.Hermite(cx, max.x, cm);
        a.y = IM.Hermite(cy, min.y, cm); b.y = IM.Hermite(cy, max.y, cm);
      }
      const colourMotion = this.isClosing
        ? IM.ComputeMotion(this.appearTime, this.OUTRO_S, this.OUTRO_E)
        : IM.ComputeMotion(this.appearTime, this.INTRO_S, this.INTRO_E);
      const alpha = this.isClosing ? IM.Hermite(1, 0, colourMotion) : IM.Hermite(0, 1, colourMotion);
      IM.drawPauseContainer(a, b, alpha, "general_window.dds");
      return { a, b, cm, alpha };
    },
    Draw() {
      IM.image(IM.P(0, 0), IM.P(1280, 720), "gameplay / host screen");
      if (!this.sIsVisible) return;
      const fontSize = IM.Scale(24);
      const header = "ACHIEVEMENT UNLOCKED";
      const name = "Lightspeed Attack";
      const maxW = Math.max(IM.measure(fontSize, header).x, IM.measure(fontSize, name).x) + IM.Scale(5);
      const imgSize = IM.Scale(60), imgMX = IM.Scale(25), imgMY = IM.Scale(22.5);
      const textMX = imgMX * 2 + imgSize - IM.Scale(5);
      const containerW = imgMX + textMX + maxW;
      const min = { x: IM.res.x / 2 - containerW / 2, y: IM.offY + IM.Scale(55) };
      const max = { x: min.x + containerW, y: min.y + IM.Scale(105) };
      const { a, b, cm } = this.drawContainer(min, max);
      if (cm >= 1 && !this.isClosing) {
        // achievement icon slot
        IM.image({ x: min.x + imgMX, y: min.y + imgMY }, { x: min.x + imgMX + imgSize, y: min.y + imgMY + imgSize }, "icon");
        IM.textShadow(fontSize, { x: min.x + textMX, y: min.y + IM.Scale(24) }, "#fcf305", header, 2, "left", IM.theme.cond);
        IM.textShadow(fontSize, { x: min.x + textMX, y: min.y + IM.Scale(58) }, "#fff", name, 2, "left", IM.theme.font);
      }
    },
  };

  /* ---------------- MessageWindow (message_window.cpp) ---------------- */
  const MessageWindow = {
    key: "MessageWindow",
    meta: {
      label: "Message Window", source: "ui/message_window.cpp",
      blurb: "STAGED reveal: the first Accept does NOT confirm — it reveals a second container of buttons (g_isControlsVisible). Only then does Accept commit g_result. Row change eases via pow(1-t,3). Backdrop dims to 190 alpha.",
      constants: [["common motion", "0–11f"], ["intro fade", "5–9f"], ["staged", "controls on 1st accept"], ["backdrop", "alpha 190"]],
    },
    COMMON_END: 11, INTRO_S: 5, INTRO_E: 9,
    sIsVisible: false, isClosing: false, isControlsVisible: false,
    appearTime: 0, controlsAppearTime: 0, selectedRow: 0, prevRow: 0, rowSelTime: 0,
    text: "Return to the World Map?\nUnsaved progress in this stage will be lost.",
    buttons: ["Yes", "No"], result: -1, _next: 0, _stage: 0,
    Open() {
      this.sIsVisible = true; this.isClosing = false; this.isControlsVisible = false;
      this.appearTime = IM.time; this.controlsAppearTime = IM.time; this.selectedRow = 0; this.result = -1;
    },
    Close() { if (!this.isClosing) { this.appearTime = IM.time; this.controlsAppearTime = IM.time; this.isClosing = true; this.isControlsVisible = false; } },
    Accept() {
      if (!this.sIsVisible) return;
      if (!this.isControlsVisible) { // first accept reveals controls
        this.isControlsVisible = true; this.controlsAppearTime = IM.time; this.selectedRow = 0;
      } else { this.result = this.selectedRow; this.Close(); }
    },
    Decline() { if (this.sIsVisible && this.isControlsVisible) { this.result = 1; this.Close(); } },
    Up() { if (this.isControlsVisible) { this.prevRow = this.selectedRow; this.selectedRow = (this.selectedRow + this.buttons.length - 1) % this.buttons.length; this.rowSelTime = IM.time; } },
    Down() { if (this.isControlsVisible) { this.prevRow = this.selectedRow; this.selectedRow = (this.selectedRow + 1) % this.buttons.length; this.rowSelTime = IM.time; } },
    reset() { this.sIsVisible = true; this.isClosing = false; this.isControlsVisible = false; this.appearTime = IM.time - 1; this.controlsAppearTime = IM.time; this.selectedRow = 0; this.result = -1; this._next = IM.time + 100; this._stage = IM.time; },
    tick(playing, interactive) {
      if (interactive) { // user drives it via keyboard
        if (!this.sIsVisible && !this.isClosing) this.Open();
        if (this.isClosing && IM.ComputeMotion(this.appearTime, 0, this.COMMON_END) >= 1) { this.sIsVisible = false; this.isClosing = false; this._next = IM.time + 0.6; }
        return;
      }
      // auto director: open -> (reveal controls) -> pick -> close -> loop
      if (!this.sIsVisible && IM.time >= this._next) { this.Open(); this._stage = IM.time; }
      if (this.sIsVisible && !this.isClosing) {
        const el = IM.time - this._stage;
        if (!this.isControlsVisible && el > 1.4) { this.isControlsVisible = true; this.controlsAppearTime = IM.time; }
        else if (this.isControlsVisible && el > 3.2) { this.result = 0; this.Close(); }
      }
      if (this.isClosing && IM.ComputeMotion(this.appearTime, 0, this.COMMON_END) >= 1) { this.sIsVisible = false; this._next = IM.time + 1.0; }
    },
    phase() {
      if (!this.sIsVisible) return "hidden";
      if (this.isClosing) return "closing";
      return this.isControlsVisible ? "awaiting selection" : "prompt (locked)";
    },
    drawContainer(startTime, centre, half, isForeground) {
      const cm = IM.ComputeMotion(startTime, 0, this.COMMON_END);
      let mn = { x: centre.x - half.x, y: centre.y - half.y };
      let mx = { x: centre.x + half.x, y: centre.y + half.y };
      if (this.isClosing) {
        mn.x = IM.Hermite(mn.x, centre.x, cm); mx.x = IM.Hermite(mx.x, centre.x, cm);
        mn.y = IM.Hermite(mn.y, centre.y, cm); mx.y = IM.Hermite(mx.y, centre.y, cm);
      } else {
        mn.x = IM.Hermite(centre.x, mn.x, cm); mx.x = IM.Hermite(centre.x, mx.x, cm);
        mn.y = IM.Hermite(centre.y, mn.y, cm); mx.y = IM.Hermite(centre.y, mx.y, cm);
      }
      const colourMotion = this.isClosing
        ? IM.ComputeMotion(startTime, 0, 4)
        : IM.ComputeMotion(startTime, this.INTRO_S, this.INTRO_E);
      const alpha = this.isClosing ? IM.Lerp(1, 0, colourMotion) : IM.Lerp(0, 1, colourMotion);
      if (isForeground) IM.rectFilled(IM.P(0, 0), IM.P(1280, 720), IM.col("#000", 0.745 * (this.isControlsVisible ? 1 : alpha)));
      IM.drawPauseContainer(mn, mx, alpha, "general_window.dds");
      return { mn, mx, cm };
    },
    Draw() {
      IM.image(IM.P(0, 0), IM.P(1280, 720), "host screen");
      if (!this.sIsVisible) return;
      const centre = { x: IM.res.x / 2, y: IM.res.y / 2 };
      const fontSize = IM.Scale(28);
      const lines = this.text.split("\n");
      let tw = 0; lines.forEach((l) => { tw = Math.max(tw, IM.measure(fontSize, l).x); });
      const th = lines.length * (fontSize + IM.Scale(5));
      const half = { x: tw / 2 + IM.Scale(37), y: th / 2 + IM.Scale(45) };
      const { mn, mx, cm } = this.drawContainer(this.appearTime, centre, half, !this.isControlsVisible);
      if (cm >= 1 && !this.isClosing) {
        lines.forEach((l, i) => {
          IM.textShadow(fontSize, { x: centre.x, y: centre.y - th / 2 + i * (fontSize + IM.Scale(5)) }, "#fff", l, 1.5, "center");
        });
        if (this.isControlsVisible) {
          const itemW = Math.max(IM.Scale(162), IM.measure(fontSize, "Yes").x + IM.Scale(60));
          const itemH = IM.Scale(57), wMX = IM.Scale(23), wMY = IM.Scale(30);
          const cHalf = { x: itemW / 2 + wMX, y: itemH / 2 * this.buttons.length + wMY };
          const r = this.drawContainer(this.controlsAppearTime, centre, cHalf, false);
          const c2 = IM.ComputeMotion(this.controlsAppearTime, 0, this.COMMON_END);
          if (c2 >= 1) {
            const listTop = centre.y - (itemH * this.buttons.length) / 2;
            this.buttons.forEach((b, i) => {
              const bmin = { x: centre.x - itemW / 2, y: listTop + i * itemH };
              const bmax = { x: centre.x + itemW / 2, y: bmin.y + itemH };
              if (i === this.selectedRow) {
                // ease the selection box from the previous row (pow(1-t,3))
                const animRatio = Math.min(1, (IM.time - this.rowSelTime) * 60 / 8);
                const prevOff = (this.prevRow - this.selectedRow) * itemH * Math.pow(1 - animRatio, 3);
                IM.drawSelectionContainer({ x: bmin.x, y: bmin.y + prevOff }, { x: bmax.x, y: bmax.y + prevOff });
              }
              IM.textShadow(fontSize, { x: centre.x, y: bmin.y + (itemH - fontSize) / 2 }, i === this.selectedRow ? IM.theme.warn : "#fff", b, 2, "center");
            });
          }
        }
      } else if (this.isClosing && cm >= 1) {
        this.sIsVisible = false;
      }
      // staged-reveal hint
      if (!this.isControlsVisible && !this.isClosing) {
        IM.text(IM.Scale(13), { x: centre.x, y: mx.y + IM.Scale(12) }, IM.theme.warn, "first Accept reveals controls — no instant confirm", "center", IM.theme.mono);
      }
    },
  };

  /* ---------------- PauseDemo (DrawPauseContainer shell) ---------------- */
  const PauseDemo = {
    key: "PauseDemo",
    meta: {
      label: "Pause Shell (9-slice)", source: "ui/imgui_utils.cpp · DrawPauseContainer",
      blurb: "The reusable framed-window grammar: one general_window.dds 9-sliced into corners + stretched edges, a header bar, a breathing selection container, and a footer button guide. Intro Hermite-expands from centre.",
      constants: [["9-slice", "corners Scale(35)"], ["header", "DrawPauseHeaderContainer"], ["selection", "BREATHE_MOTION"], ["intro", "0–11f expand"]],
    },
    items: ["Resume", "Restart Stage", "Skill Shop", "Options", "Return to Map"],
    sIsVisible: false, appearTime: 0, sel: 1,
    reset() { this.sIsVisible = true; this.appearTime = IM.time - 3; this.sel = 1; },
    tick() {},
    phase() { return IM.ComputeMotion(this.appearTime, 0, 11) >= 1 ? "idle" : "intro"; },
    Draw(family) {
      IM.image(IM.P(0, 0), IM.P(1280, 720), "frozen gameplay");
      IM.rectFilled(IM.P(0, 0), IM.P(1280, 720), IM.col("#000", 0.55));
      const cm = IM.ComputeMotion(this.appearTime, 0, 11);
      const c = IM.center;
      const halfX = IM.Scale(460), halfY = IM.Scale(300);
      let mn = { x: IM.Hermite(c.x, c.x - halfX, cm), y: IM.Hermite(c.y, c.y - halfY, cm) };
      let mx = { x: IM.Hermite(c.x, c.x + halfX, cm), y: IM.Hermite(c.y, c.y + halfY, cm) };
      const alpha = IM.ComputeMotion(this.appearTime, 5, 9);
      IM.drawPauseContainer(mn, mx, alpha, "ui_general/window");
      if (cm < 1) return;
      // header
      IM.drawPauseHeaderContainer({ x: mn.x + IM.Scale(30), y: mn.y - IM.Scale(8) }, { x: mn.x + IM.Scale(260), y: mn.y + IM.Scale(34) }, 1);
      IM.text(IM.Scale(26), { x: mn.x + IM.Scale(48), y: mn.y + IM.Scale(2) }, "#06210f", "PAUSE", "left", IM.theme.cond);
      // menu rows + selection
      const rowH = IM.Scale(64), x0 = mn.x + IM.Scale(45), rowW = IM.Scale(420);
      this.items.forEach((it, i) => {
        const ry = mn.y + IM.Scale(80) + i * rowH;
        if (i === this.sel) IM.drawSelectionContainer({ x: x0 - IM.Scale(8), y: ry - IM.Scale(4) }, { x: x0 + rowW, y: ry + IM.Scale(40) });
        IM.text(IM.Scale(28), { x: x0 + IM.Scale(10), y: ry }, i === this.sel ? "#fff" : IM.theme.inkDim, it, "left", IM.theme.font);
      });
      // right preview slot
      IM.image({ x: mx.x - IM.Scale(300), y: mn.y + IM.Scale(80) }, { x: mx.x - IM.Scale(40), y: mx.y - IM.Scale(120) }, "ui_pause/text_area");
      // footer guide
      IM.drawButtonGuide([
        { name: "Select", icon: "A", align: "left" },
        { name: "Back", icon: "B", align: "left" },
        { name: "Prev", icon: "LB", align: "right" },
        { name: "Next", icon: "RB", align: "right" },
      ], family);
    },
  };

  window.SWARD_NATIVE = { Fader, ButtonGuide, AchievementOverlay, MessageWindow, PauseDemo };
  window.SWARD_NATIVE_ORDER = ["PauseDemo", "MessageWindow", "AchievementOverlay", "ButtonGuide", "Fader"];
})();
