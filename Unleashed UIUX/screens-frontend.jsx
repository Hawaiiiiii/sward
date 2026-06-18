/* ============================================================
   screens-frontend.jsx — Front-end + transition/result screens
   Neutral, re-skinnable structural reproductions. Every art
   region is an asset SLOT labelled with the real recovered
   node/asset name (e.g. ui_pause/footer/footer_A).
   Overlay layers show/hide per contract.visible_overlay_roles.
   ============================================================ */
const { useState: useStateF, useEffect: useEffectF } = React;

/* ---------- shared primitives (exported to window) ---------- */
function Slot({ label, lit, className = "", style }) {
  return (
    <div className={"slot " + (lit ? "lit " : "") + className} style={style}>
      <span className="slab">{label}</span>
    </div>
  );
}

/* A composited overlay layer. The RESTING (visible) state is the base
   style; entrance motion plays FROM hidden via a CSS @keyframes anim.
   This way a stalled/throttled animation rests at VISIBLE, never hidden. */
function Layer({ show, band = 0.33, motion = "fade", className = "", style, children, z = 1 }) {
  const base = {
    position: "absolute",
    opacity: show ? 1 : 0,
    pointerEvents: show ? "auto" : "none",
    zIndex: z,
    ...(show ? { animation: `sw-${motion} ${band}s cubic-bezier(.2,.8,.2,1) forwards` } : null),
    ...style,
  };
  return <div className={className} style={base}>{children}</div>;
}

/* prompt glyphs across switchable families */
function glyphInfo(button, family) {
  const F = {
    glyph: { A: ["✓", ""], B: ["✕", "b"], X: ["◇", "x"], Y: ["◻", "y"], LB: ["«", "shoulder"], RB: ["»", "shoulder"], START: ["≡", "shoulder"] },
    letter: { A: ["A", ""], B: ["B", "b"], X: ["X", "x"], Y: ["Y", "y"], LB: ["LB", "shoulder"], RB: ["RB", "shoulder"], START: ["START", "shoulder"] },
    kbd: { A: ["Enter", "kbd"], B: ["Esc", "kbd"], X: ["Space", "kbd"], Y: ["Tab", "kbd"], LB: ["Q", "kbd"], RB: ["E", "kbd"], START: ["Esc", "kbd"] },
  };
  return (F[family] || F.letter)[button] || [button, "shoulder"];
}
function Prompt({ button, label, family }) {
  const [txt, cls] = glyphInfo(button, family);
  return (
    <span className="prompt">
      <span className={"glyph " + cls}>{txt}</span>
      <span className="plabel">{label}</span>
    </span>
  );
}
function PromptRow({ prompts, family, state, style }) {
  const vis = prompts.filter((p) => p.visible_states.includes(state));
  if (!vis.length) return null;
  return (
    <div className="promptrow" style={style}>
      {vis.map((p) => <Prompt key={p.slot_id} button={p.button} label={p.label} family={family} />)}
    </div>
  );
}

/* helper: is role visible in this state */
const makeVis = (contract, state) => {
  const set = new Set(contract.visible_overlay_roles[state] || []);
  return (role) => set.has(role);
};
/* current band seconds for a state */
const bandOf = (contract, state) => {
  const st = contract.states.find((s) => s.state === state);
  if (!st || !st.timeline_band_id) return 0.33;
  const b = contract.timeline_bands.find((b) => b.id === st.timeline_band_id);
  return b ? b.seconds : 0.33;
};

/* ============================================================
   TITLE / MAIN MENU
   ============================================================ */
function TitleMenuScreen({ contract, state, family }) {
  const vis = makeVis(contract, state);
  const items = ["New Game", "Continue", "Extras", "Options", "Quit"];
  const sel = state === "Navigate" ? 1 : state === "Confirm" ? 3 : 0;
  return (
    <div className="scr" style={{ background: "radial-gradient(120% 120% at 70% 20%, #16243a, #070b12 70%)" }}>
      <Layer show={vis("backdrop")} band={bandOf(contract, state)} className="" style={{ inset: 0 }} z={1}>
        <image-slot id="title_bg" placeholder="drop your ui_mainmenu / mm_bg_intro background" shape="rect" fit="cover"
          style={{ position: "absolute", inset: 0, width: "100%", height: "100%", opacity: 0.96 }}></image-slot>
      </Layer>

      <Layer show={vis("chrome")} band={bandOf(contract, state)} motion="rise" style={{ left: "8cqw", top: "12cqh", width: "44cqw", height: "26cqh" }} z={3}>
        <image-slot id="title_logo" placeholder="drop your title logo (PNG, transparent)" shape="rect" fit="contain"
          style={{ position: "absolute", inset: 0, width: "100%", height: "100%" }}></image-slot>
      </Layer>

      <Layer show={vis("content")} band={bandOf(contract, state)} motion="slideL" style={{ left: "9cqw", top: "48cqh", width: "40cqw" }} z={4}>
        <div style={{ display: "flex", flexDirection: "column", gap: "1.4cqh", fontFamily: "var(--font-cond)" }}>
          {items.map((it, i) => (
            <div key={it} style={{
              position: "relative", padding: ".9cqh 2cqh", borderRadius: "var(--r-m)",
              fontSize: i === sel ? "3.6cqh" : "3cqh", fontWeight: i === sel ? 800 : 600,
              textTransform: "uppercase", letterSpacing: ".1cqh",
              color: i === sel ? "#06210f" : "var(--ink-dim)",
              background: i === sel ? "var(--accent)" : "transparent",
              boxShadow: i === sel ? "0 0 3cqh color-mix(in oklab,var(--accent) 50%, transparent)" : "none",
              transition: "all .2s", width: i === sel ? "100%" : "70%",
            }}>{it}</div>
          ))}
        </div>
      </Layer>

      <Layer show={vis("transient_fx")} band={0.2} style={{ left: "9cqw", top: "48cqh" }} z={5}>
        <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.4cqh", color: "var(--warn)" }}>mm_contentsitem_move ▸ select</div>
      </Layer>

      <PromptRow prompts={contract.prompt_slots} family={family} state={state} style={{ right: "4cqw", bottom: "4cqh" }} />
    </div>
  );
}

/* ============================================================
   PAUSE MENU  (ui_general shell + ui_pause specialization)
   ============================================================ */
function PauseMenuScreen({ contract, state, family }) {
  const vis = makeVis(contract, state);
  const band = bandOf(contract, state);
  const tabs = ["Status", "Skills", "Options"];
  const items = ["Resume", "Restart Stage", "Skill Shop", "Options", "Return to Map"];
  const sel = state === "Navigate" ? 2 : state === "Confirm" ? 0 : 1;
  return (
    <div className="scr">
      <Slot label="frozen gameplay" style={{ position: "absolute", inset: 0, borderRadius: 0, opacity: .25 }} />
      <Layer show={vis("backdrop")} band={band} style={{ inset: 0 }} z={2}><div className="gbackdrop" /></Layer>

      <Layer show={vis("chrome")} band={band} motion="grow" style={{ left: "14cqw", top: "12cqh", width: "72cqw", height: "76cqh" }} z={3}>
        <div className="gwin" style={{ inset: 0 }} />
        {/* header: title-extend + status_title */}
        <div className="title-extend" style={{ left: "3cqw", top: "3cqh", fontSize: "3.4cqh", color: "var(--ink)" }}>
          <span className="bar" style={{ width: "5cqw" }} />PAUSE
        </div>
        <div style={{ position: "absolute", right: "3cqw", top: "3.4cqh", fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "var(--ink-faint)" }}>ui_pause/header/status_title</div>
        {/* dual footer footer_A / footer_B */}
        <div style={{ position: "absolute", left: "3cqw", right: "3cqw", bottom: "2.4cqh", display: "flex", justifyContent: "space-between", fontFamily: "var(--font-mono)", fontSize: "1.2cqh", color: "var(--ink-faint)", borderTop: "1px solid var(--stroke)", paddingTop: "1.4cqh" }}>
          <span>footer_A</span><span>footer_B</span>
        </div>
      </Layer>

      <Layer show={vis("content")} band={band} style={{ left: "14cqw", top: "12cqh", width: "72cqw", height: "76cqh" }} z={4}>
        {/* tab row */}
        <div style={{ position: "absolute", left: "3cqw", top: "10cqh", display: "flex", gap: "1.4cqh" }}>
          {tabs.map((t, i) => (
            <div key={t} style={{ padding: ".7cqh 2cqh", borderRadius: "var(--r-s)", fontFamily: "var(--font-cond)", fontWeight: 700, fontSize: "2.2cqh", textTransform: "uppercase",
              color: i === 0 ? "var(--ink)" : "var(--ink-faint)", background: i === 0 ? "var(--chrome-2)" : "transparent", borderBottom: i === 0 ? "2px solid var(--accent)" : "2px solid transparent" }}>{t}</div>
          ))}
        </div>
        {/* menu list with selection bar */}
        <div style={{ position: "absolute", left: "3cqw", top: "20cqh", width: "40cqw", display: "flex", flexDirection: "column", gap: ".4cqh" }}>
          {items.map((it, i) => (
            <div key={it} style={{ position: "relative", padding: "1.3cqh 2cqh", fontFamily: "var(--font-ui)", fontSize: "2.4cqh", fontWeight: i === sel ? 700 : 500, color: i === sel ? "#06210f" : "var(--ink-dim)", borderRadius: "var(--r-s)" }}>
              {i === sel && <div className="selbar" style={{ inset: 0, transition: `all ${band}s cubic-bezier(.2,.8,.2,1)` }} />}
              <span style={{ position: "relative" }}>{it}</span>
            </div>
          ))}
        </div>
        {/* right preview */}
        <Slot label="ui_pause / text_area · skill_select" style={{ position: "absolute", right: "3cqw", top: "20cqh", width: "22cqw", height: "44cqh" }} />
      </Layer>

      <Layer show={vis("transient_fx")} band={0.2} style={{ left: "16cqw", top: "32cqh" }} z={6}>
        <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "var(--warn)" }}>Scroll_Anim · Size_Anim</div>
      </Layer>

      <PromptRow prompts={contract.prompt_slots} family={family} state={state} style={{ left: "16cqw", bottom: "5.5cqh" }} />
    </div>
  );
}

/* ============================================================
   OPTIONS MENU (rail + list + marquee info + TV-static thumb)
   ============================================================ */
function OptionsMenuScreen({ contract, state, family }) {
  const vis = makeVis(contract, state);
  const band = bandOf(contract, state);
  const cats = ["System", "Input", "Audio", "Video"];
  const opts = [
    ["Language", "English"], ["Voice Language", "Japanese"], ["Subtitles", "On"],
    ["Hints", "On"], ["Control Tutorial", "On"], ["Achievement Notifications", "On"],
    ["Time of Day Transition", "Auto"],
  ];
  const sel = state === "Navigate" || state === "Confirm" ? 3 : 1;
  const editing = state === "Confirm";
  return (
    <div className="scr" style={{ background: "linear-gradient(180deg,#0a0f17,#05080d)" }}>
      <Layer show={vis("backdrop")} band={band} style={{ inset: 0 }} z={1}><div style={{ position: "absolute", inset: 0, background: "#000b" }} /></Layer>

      {/* title + top category tabs (role: rail) */}
      <Layer show={vis("rail")} band={band} motion="rise" style={{ left: "5cqw", top: "5cqh", right: "5cqw" }} z={4}>
        <div style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "5cqh", textTransform: "uppercase", color: "var(--accent)", letterSpacing: ".2cqh" }}>Options</div>
        <div style={{ display: "flex", gap: "3cqw", marginTop: "1.4cqh", borderBottom: "1px solid var(--stroke)", paddingBottom: "1cqh" }}>
          {cats.map((c, i) => (
            <div key={c} style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "3cqh", textTransform: "uppercase", letterSpacing: ".1cqh",
              color: i === 0 ? "var(--ink)" : "var(--ink-faint)", borderBottom: i === 0 ? "3px solid var(--accent)" : "3px solid transparent", paddingBottom: ".8cqh" }}>{c}</div>
          ))}
        </div>
      </Layer>

      {/* option list panel (label left, value pill right) */}
      <Layer show={vis("content")} band={band} style={{ left: "5cqw", top: "25cqh", width: "57cqw", bottom: "11cqh" }} z={4}>
        <div className="gwin" style={{ inset: 0, opacity: .5 }} />
        <div style={{ position: "absolute", inset: "2cqh", display: "flex", flexDirection: "column", gap: ".4cqh" }}>
          {opts.map(([k, v], i) => (
            <div key={k} style={{ position: "relative", display: "flex", justifyContent: "space-between", alignItems: "center", padding: "1.25cqh 1.8cqh", borderRadius: "var(--r-s)", fontSize: "2.5cqh",
              color: i === sel ? "#1a1407" : "var(--ink-dim)" }}>
              {i === sel && <div className="selbar" style={{ inset: 0, borderColor: editing ? "var(--warn)" : "var(--accent)", background: editing ? "color-mix(in oklab,var(--warn) 30%,transparent)" : "color-mix(in oklab,var(--accent) 26%,transparent)" }} />}
              <span style={{ position: "relative", fontWeight: i === sel ? 700 : 500 }}>{k}</span>
              <span style={{ position: "relative", fontFamily: "var(--font-mono)", fontSize: "2cqh", fontWeight: 700, padding: ".3cqh 1.2cqh", borderRadius: "var(--r-s)",
                background: i === sel ? "#0003" : "var(--chrome-2)", color: i === sel ? "#1a1407" : "var(--ink)" }}>{editing && i === sel ? "‹ " + v + " ›" : v}</span>
            </div>
          ))}
        </div>
      </Layer>

      {/* language-preview thumbnail w/ TV static (role: thumbnail) */}
      <Layer show={vis("thumbnail")} band={band} motion="grow" style={{ right: "5cqw", top: "25cqh", width: "26cqw", height: "26cqh" }} z={4}>
        <Slot lit label="options_menu_thumbnails + tv_static.cpp&#10;[ あ / A  language preview ]" style={{ position: "absolute", inset: 0 }} />
        <div style={{ position: "absolute", inset: 0, mixBlendMode: "screen", opacity: .22, background: "repeating-linear-gradient(0deg,#fff2 0 1px,transparent 1px 3px)" }} />
      </Layer>

      {/* info panel with marquee (role: info) */}
      <Layer show={vis("info")} band={band} style={{ right: "5cqw", top: "54cqh", width: "26cqw", bottom: "11cqh" }} z={4}>
        <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.4cqh", color: "var(--accent-2)", marginBottom: "1cqh" }}>INFO · marquee_delay 1.2s</div>
        <div style={{ fontSize: "2.2cqh", lineHeight: 1.4, color: "var(--ink-dim)" }}>Changes the language used for character voices.</div>
      </Layer>

      <PromptRow prompts={contract.prompt_slots} family={family} state={state} style={{ left: "50%", bottom: "3.5cqh", transform: "translateX(-50%)" }} />
    </div>
  );
}

/* ============================================================
   WORLD MAP (header/footer bands + info pane + help sidecar)
   ============================================================ */
function WorldMapScreen({ contract, state, family }) {
  const vis = makeVis(contract, state);
  const band = bandOf(contract, state);
  return (
    <div className="scr">
      <Layer show={vis("backdrop")} band={band} style={{ inset: 0 }} z={1}>
        <Slot label="ui_worldmap / worldmap_background" style={{ position: "absolute", inset: 0, borderRadius: 0, opacity: .6 }} />
        {/* cursor */}
        <Layer show={vis("transient_fx")} band={0.3} style={{ left: state === "Navigate" ? "58cqw" : "40cqw", top: state === "Navigate" ? "40cqh" : "52cqh", transition: `left ${band}s, top ${band}s` }} z={3}>
          <div style={{ width: "4cqh", height: "4cqh", borderRadius: "50%", border: "0.4cqh solid var(--accent)", boxShadow: "0 0 2cqh var(--accent)" }} />
          <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.2cqh", color: "var(--accent)", marginTop: ".4cqh" }}>sys_worldmap_cursor</div>
        </Layer>
      </Layer>

      {/* header + footer bands */}
      <Layer show={vis("header")} band={band} style={{ inset: 0 }} z={4}>
        <div style={{ position: "absolute", left: 0, right: 0, top: 0, height: "9cqh", background: "linear-gradient(180deg,#0d1420ee,transparent)", display: "flex", alignItems: "center", padding: "0 3cqw", justifyContent: "space-between" }}>
          <span style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "3cqh", textTransform: "uppercase", color: "#fff" }}>World Map</span>
          <span style={{ fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "var(--ink-faint)" }}>worldmap_header_bg</span>
        </div>
        <div style={{ position: "absolute", left: 0, right: 0, bottom: 0, height: "7cqh", background: "linear-gradient(0deg,#0d1420ee,transparent)" }} />
      </Layer>

      {/* info pane + choices pane */}
      <Layer show={vis("content")} band={band} motion="rise" style={{ left: "5cqw", bottom: "11cqh", width: "34cqw", height: "32cqh" }} z={5}>
        <div className="gwin" style={{ inset: 0 }} />
        <Slot label="cts_info_bg / info_img_1" style={{ position: "absolute", left: "1.6cqh", top: "1.6cqh", width: "13cqw", bottom: "1.6cqh" }} />
        <div style={{ position: "absolute", left: "16cqw", top: "2.4cqh", right: "1.8cqh" }}>
          <div style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "2.8cqh", color: "#fff", textTransform: "uppercase" }}>Stage Name</div>
          <div style={{ fontSize: "1.8cqh", color: "var(--ink-dim)", marginTop: "1cqh", lineHeight: 1.4 }}>info_bg_1 · stage description and mission summary lane.</div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.5cqh", color: "var(--accent)", marginTop: "1.4cqh" }}>RANK A · 02:41:80</div>
        </div>
      </Layer>

      {/* help sidecar */}
      <Layer show={vis("help_sidecar")} band={band} motion="grow" style={{ right: "5cqw", bottom: "11cqh", width: "24cqw", height: "20cqh" }} z={5}>
        <div className="gwin" style={{ inset: 0, borderColor: "var(--accent-2)" }} />
        <div style={{ position: "absolute", inset: "2cqh" }}>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "var(--accent-2)" }}>ui_worldmap_help · msg_bg_l/r</div>
          <div style={{ fontSize: "1.9cqh", color: "var(--ink-dim)", marginTop: "1cqh", lineHeight: 1.4 }}>Hold a direction to pan the map. Press Help to toggle this balloon.</div>
        </div>
      </Layer>

      <PromptRow prompts={contract.prompt_slots} family={family} state={state} style={{ left: "50%", bottom: "1.8cqh", transform: "translateX(-50%)" }} />
    </div>
  );
}

/* ============================================================
   LOADING TRANSITION  (host-driven, non-interactive)
   ============================================================ */
function LoadingTransitionScreen({ contract, state, family, platform }) {
  const vis = makeVis(contract, state);
  const band = bandOf(contract, state);
  const platLabel = { glyph: "generic pad icon", letter: "360_* icon atlas", kbd: "keyboard hints" }[family] || "pad icon";
  return (
    <div className="scr" style={{ background: "#04060a" }}>
      <Layer show={vis("backdrop")} band={band} style={{ inset: 0 }} z={1}><div style={{ position: "absolute", inset: 0, background: "radial-gradient(120% 100% at 80% 90%, #101a2b, #04060a)" }} /></Layer>

      {/* black bars */}
      <Layer show={vis("black_bars")} band={band} style={{ inset: 0 }} z={6}>
        <div className="letterbox" style={{ top: 0, height: "8cqh" }} />
        <div className="letterbox" style={{ bottom: 0, height: "8cqh" }} />
      </Layer>

      {/* loading card */}
      <Layer show={vis("card")} band={band} motion="rise" style={{ right: "6cqw", bottom: "13cqh", width: "40cqw", height: "26cqh" }} z={4}>
        <div className="gwin" style={{ inset: 0 }} />
        <Slot lit label="ui_loading / loadinfo&#10;[ stage art · n_2_d / d_2_n branch ]" style={{ position: "absolute", left: "2cqh", top: "2cqh", width: "16cqw", bottom: "2cqh" }} />
        {/* info text lines */}
        <Layer show={vis("info")} band={0.4} style={{ position: "absolute", left: "19cqw", top: "3cqh", right: "2.5cqh" }} z={2}>
          <div style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "2.6cqh", color: "#fff", textTransform: "uppercase" }}>Now Loading</div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.5cqh", color: "var(--ink-dim)", marginTop: "1.2cqh", lineHeight: 1.6 }}>pda_txt · 240f loop tip lane</div>
          <div style={{ marginTop: "1.6cqh", height: ".8cqh", borderRadius: "4px", background: "#0008", overflow: "hidden" }}>
            <div style={{ height: "100%", width: state === "Loop" ? "72%" : "12%", background: "linear-gradient(90deg,var(--accent-2),var(--accent))", transition: "width 4s linear" }} />
          </div>
        </Layer>
      </Layer>

      {/* platform icon (swap band ~2f) */}
      <Layer show={vis("platform_icon")} band={0.05} style={{ right: "6cqw", bottom: "5cqh", display: "flex", alignItems: "center", gap: "1cqh" }} z={5}>
        <div style={{ width: "3.2cqh", height: "3.2cqh", borderRadius: "50%", border: "0.3cqh solid var(--ink-faint)", display: "flex", alignItems: "center", justifyContent: "center", fontFamily: "var(--font-mono)", fontSize: "1.4cqh", color: "var(--ink-dim)" }}>↻</div>
        <span style={{ fontFamily: "var(--font-mono)", fontSize: "1.4cqh", color: "var(--ink-faint)" }}>{platLabel}</span>
      </Layer>
    </div>
  );
}

/* ============================================================
   MISSION RESULT  (number roll → rank reveal performance)
   ============================================================ */
function MissionResultScreen({ contract, state, family }) {
  const vis = makeVis(contract, state);
  const band = bandOf(contract, state);
  const rolling = state === "Navigate";
  const reveal = state === "Confirm" || state === "Idle";
  const rows = [["Time Bonus", "12,400"], ["Ring Bonus", "3,800"], ["Speed Bonus", "8,600"], ["Total", "24,800"]];
  return (
    <div className="scr" style={{ background: "linear-gradient(135deg,#0c1422,#05080e)" }}>
      <Layer show={vis("backdrop")} band={band} style={{ inset: 0 }} z={1}><div style={{ position: "absolute", inset: 0, background: "#000c" }} /></Layer>

      <Layer show={vis("chrome")} band={band} motion="rise" style={{ inset: 0 }} z={3}>
        <div className="title-extend" style={{ left: "8cqw", top: "12cqh", fontSize: "5cqh", color: "#fff" }}>
          <span className="bar" style={{ width: "7cqw" }} />RESULT
        </div>
        <div style={{ position: "absolute", left: "8cqw", top: "21cqh", fontFamily: "var(--font-mono)", fontSize: "1.4cqh", color: "var(--ink-faint)" }}>ui_result / result_title</div>
      </Layer>

      <Layer show={vis("content")} band={band} style={{ left: "8cqw", top: "30cqh", width: "44cqw" }} z={4}>
        {rows.map(([k, v], i) => (
          <div key={k} style={{ display: "flex", justifyContent: "space-between", padding: "1.4cqh 0", borderBottom: i < rows.length - 1 ? "1px solid var(--stroke)" : "none",
            fontWeight: i === rows.length - 1 ? 800 : 500 }}>
            <span style={{ fontSize: "2.6cqh", color: i === rows.length - 1 ? "#fff" : "var(--ink-dim)", fontFamily: "var(--font-cond)", textTransform: "uppercase", letterSpacing: ".1cqh" }}>{k}</span>
            <span style={{ fontSize: "2.8cqh", fontFamily: "var(--font-mono)", fontWeight: 700, color: i === rows.length - 1 ? "var(--accent)" : "#fff" }}>{rolling ? "—,—" : v}</span>
          </div>
        ))}
      </Layer>

      {/* rank stamp */}
      <Layer show={vis("rank_fx")} band={band} style={{ right: "10cqw", top: "26cqh", width: "30cqw", height: "48cqh", display: "flex", alignItems: "center", justifyContent: "center" }} z={5}>
        <div style={{ position: "absolute", inset: 0, borderRadius: "50%", background: "radial-gradient(circle, color-mix(in oklab,var(--accent) 30%, transparent), transparent 65%)", transform: reveal ? "scale(1)" : "scale(.4)", opacity: reveal ? 1 : 0, transition: `all ${band}s cubic-bezier(.2,.9,.2,1)` }} />
        <div style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "34cqh", lineHeight: 1, color: "var(--accent)", textShadow: "0 1cqh 4cqh #000a",
          transform: reveal ? "scale(1) rotate(-4deg)" : "scale(2.4) rotate(-4deg)", opacity: reveal ? 1 : 0, transition: `all ${band}s cubic-bezier(.15,.85,.25,1)` }}>S</div>
        <div style={{ position: "absolute", bottom: "4cqh", fontFamily: "var(--font-mono)", fontSize: "1.4cqh", color: "var(--ink-faint)" }}>result_rank · 253f reveal</div>
      </Layer>

      <PromptRow prompts={contract.prompt_slots} family={family} state={state} style={{ right: "6cqw", bottom: "5cqh" }} />
    </div>
  );
}

/* ============================================================
   AUTOSAVE TOAST  (single-scene 3.0s presentation node)
   ============================================================ */
function AutosaveToastScreen({ contract, state, family }) {
  const vis = makeVis(contract, state);
  const band = bandOf(contract, state);
  return (
    <div className="scr">
      <Slot label="gameplay continues underneath" style={{ position: "absolute", inset: 0, borderRadius: 0, opacity: .22 }} />
      <Layer show={vis("icon")} band={band} motion="rise" style={{ right: "5cqw", bottom: "7cqh", display: "flex", alignItems: "center", gap: "1.4cqh" }} z={4}>
        <div style={{ width: "5cqh", height: "5cqh", borderRadius: "var(--r-s)", background: "var(--chrome-2)", border: "1px solid var(--stroke)", display: "flex", alignItems: "center", justifyContent: "center" }}>
          <div style={{ width: "3cqh", height: "3cqh", borderRadius: "50%", border: "0.4cqh solid var(--accent)", borderTopColor: "transparent", animation: state === "Saving" ? "swspin 1s linear infinite" : "none" }} />
        </div>
        <div>
          <div style={{ fontFamily: "var(--font-cond)", fontWeight: 700, fontSize: "2.4cqh", color: "#fff", textTransform: "uppercase", letterSpacing: ".1cqh" }}>Saving…</div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "var(--ink-faint)" }}>ui_saveicon · 180f Intro_Anim</div>
        </div>
      </Layer>
      <style>{`@keyframes swspin{to{transform:rotate(360deg)}}`}</style>
    </div>
  );
}

/* export to window for the viewer + ingame screens */
Object.assign(window, {
  Slot, Layer, Prompt, PromptRow, glyphInfo, makeVis, bandOf,
  TitleMenuScreen, PauseMenuScreen, OptionsMenuScreen, WorldMapScreen,
  LoadingTransitionScreen, MissionResultScreen, AutosaveToastScreen,
});
