/* ============================================================
   viewer.jsx — the debug-viewer shell + state-walk engine
   Sidebar (jump to any screen) · 16:9 stage · live debug panel
   (state machine, timeline bands, overlay roles, prompts,
   telemetry) · transport that drives each contract's walk
   using the recovered timing bands.
   ============================================================ */
const { useState, useEffect, useRef, useCallback } = React;

const SCREEN_COMPONENTS = {
  TitleMenu: () => window.TitleMenuScreen, PauseMenu: () => window.PauseMenuScreen,
  OptionsMenu: () => window.OptionsMenuScreen, WorldMap: () => window.WorldMapScreen,
  LoadingTransition: () => window.LoadingTransitionScreen, MissionResult: () => window.MissionResultScreen,
  AutosaveToast: () => window.AutosaveToastScreen, SonicStageHUD: () => window.SonicStageHUDScreen,
  WerehogStageHUD: () => window.WerehogStageHUDScreen, BossHUD: () => window.BossHUDScreen,
  SubtitleCutscene: () => window.SubtitleCutsceneScreen, Overlays: () => window.OverlaysScreen,
};

/* dwell time (contract seconds at 1x) a state holds before the walk advances */
function dwellOf(contract, stateName) {
  const st = contract.states.find((s) => s.state === stateName);
  if (!st) return 0.6;
  if (st.input_enabled) return 1.6;                 // idle/interactive — held for readability
  const b = st.timeline_band_id && contract.timeline_bands.find((x) => x.id === st.timeline_band_id);
  if (b) return Math.max(b.seconds, 0.35);
  return 0.5;
}
const fmt = (s) => s.toFixed(2).padStart(6, "0");

function Icon({ d, paths }) {
  return <svg className="gi" viewBox="0 0 24 24">{paths ? paths : <path d={d} />}</svg>;
}
const IcPlay = <svg className="gi" viewBox="0 0 24 24"><path d="M6 4l14 8-14 8z" fill="currentColor" stroke="none" /></svg>;
const IcPause = <svg className="gi" viewBox="0 0 24 24"><path d="M7 5v14M17 5v14" /></svg>;
const IcStep = <svg className="gi" viewBox="0 0 24 24"><path d="M6 5l9 7-9 7zM18 5v14" /></svg>;
const IcReset = <svg className="gi" viewBox="0 0 24 24"><path d="M4 12a8 8 0 1 0 2.3-5.6M4 4v3h3" /></svg>;

/* ---------------- TOP BAR ---------------- */
function TopBar({ family, setFamily, sidebarOpen, setSidebarOpen, debugOpen, setDebugOpen, playing, setPlaying, gameType, setGameType, qaShell, setQaShell }) {
  const fams = [["glyph", "Glyph"], ["letter", "A B X Y"], ["kbd", "Keys"]];
  return (
    <div className="topbar">
      <button className={"topbtn" + (sidebarOpen ? " on" : "")} onClick={() => setSidebarOpen(!sidebarOpen)} title="Toggle screen list">
        <svg className="gi" viewBox="0 0 24 24"><path d="M4 6h16M4 12h16M4 18h16" /></svg>
      </button>
      <div className="brand"><span className="mark">SWARD</span><span className="sub">UI Screen Viewer</span></div>
      <div className="spacer" />
      <div className="seg">
        <span className="lbl">Type</span>
        <button className={gameType ? "" : "on"} onClick={() => setGameType(false)}>Default</button>
        <button className={gameType ? "on" : ""} onClick={() => setGameType(true)}>DFHei</button>
      </div>
      <div className="seg">
        <span className="lbl">Prompts</span>
        {fams.map(([id, lab]) => <button key={id} className={family === id ? "on" : ""} onClick={() => setFamily(id)}>{lab}</button>)}
      </div>
      <button className={"topbtn" + (playing ? " on" : "")} onClick={() => setPlaying(!playing)} title="Play / pause state walk">
        {playing ? IcPause : IcPlay}<span>{playing ? "Pause" : "Play walk"}</span>
      </button>
      <button className={"topbtn" + (qaShell ? " on" : "")} onClick={() => setQaShell(!qaShell)} title="Toggle SGFX QA Shell overlay">
        <svg className="gi" viewBox="0 0 24 24"><path d="M4 5h16v14H4zM4 9h16M8 13h2M8 16h6" /></svg>
        <span>QA Shell</span>
      </button>
      <button className={"topbtn" + (debugOpen ? " on" : "")} onClick={() => setDebugOpen(!debugOpen)} title="Toggle debug panel">
        <svg className="gi" viewBox="0 0 24 24"><path d="M8 3v4M16 3v4M5 8h14v11a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2zM9 12h6M9 16h4" /></svg>
        <span>Inspector</span>
      </button>
    </div>
  );
}

/* ---------------- SIDEBAR ---------------- */
function Sidebar({ activeKey, onPick }) {
  const groups = window.SWARD_GROUPS, labels = window.SWARD_LABELS, C = window.SWARD_CONTRACTS;
  let n = 0;
  // --- game-DB router: navigate by the game's real flag-conditioned flow ---
  const R = window.SWARD_ROUTER;
  const [flags, setFlags] = useState(() => (R ? R.defaultFlags() : {}));
  const [route, setRoute] = useState(null);
  const KEYFLAGS = ["bFlag_StgClear_Africa_B1", "bFlag_Event_S2_04", "bFlag_StgClear_EuropeanCity_N1", "iFlag_TownSecNo", "eFlag_TownState"];
  const flagDef = (nm) => (window.SWARD_GAMEDB ? window.SWARD_GAMEDB.flags.find((f) => f.name === nm) : null);
  const runGoTo = (goTo) => {
    if (!R) return;
    const res = R.resolve(goTo, flags);
    const key = R.screenKey(res.stage);
    setRoute({ goTo, stage: res.stage ? res.stage.id : "(unresolved)", screen: res.stage && res.stage.screen, key, trace: res.trace });
    if (key) onPick(key);
  };
  return (
    <div className="sidebar">
      {groups.map((g) => (
        <div className="sb-group" key={g.title}>
          <div className="sb-group-h">{g.title}</div>
          {g.ids.map((id) => {
            n += 1; const idx = String(n).padStart(2, "0");
            const c = C[id];
            return (
              <button key={id} className={"sb-item" + (activeKey === id ? " on" : "")} onClick={() => onPick(id)}>
                <span className="sb-idx">{idx}</span>
                <span className="sb-meta">
                  <span className="sb-name">{labels[id]}</span>
                  <span className="sb-sys">{c.source_system}</span>
                </span>
              </button>
            );
          })}
        </div>
      ))}
      <div style={{ padding: "16px 18px", fontFamily: "var(--font-mono)", fontSize: "10px", lineHeight: 1.7, color: "var(--tool-dim)", borderTop: "1px solid var(--panel-line)" }}>
        12 contract-backed families.<br />Architecture + timing only — drop your own art into the labelled slots.
      </div>

      {/* native immediate-mode group (canvas) — 1:1 ports of the recomp's OWN UI */}
      <div className="sb-group">
        <div className="sb-group-h">Native — recomp 1:1</div>
        {window.SWARD_NATIVE_ORDER.map((nk) => {
          const ov = window.SWARD_NATIVE[nk];
          const key = "native:" + nk;
          return (
            <button key={key} className={"sb-item" + (activeKey === key ? " on" : "")} onClick={() => onPick(key)}>
              <span className="sb-idx" style={{ color: "var(--accent-2)" }}>◈</span>
              <span className="sb-meta">
                <span className="sb-name">{ov.meta.label}</span>
                <span className="sb-sys">{ov.meta.source}</span>
              </span>
            </button>
          );
        })}
      </div>
      <div style={{ padding: "14px 18px 22px", fontFamily: "var(--font-mono)", fontSize: "10px", lineHeight: 1.7, color: "var(--tool-dim)" }}>
        Canvas immediate-mode. These are the overlays the recomp actually wrote in C++ — rendered through a JS port of imgui_utils (Scale · ComputeMotion · Hermite · 9-slice).
      </div>

      {/* reconstructed group — manifest-driven, auto-filled from your extracted assets */}
      <div className="sb-group">
        <div className="sb-group-h">Reconstructed — your assets</div>
        {window.SWARD_MANIFESTS.map((mf) => {
          const key = "mf:" + mf.id;
          return (
            <button key={key} className={"sb-item" + (activeKey === key ? " on" : "")} onClick={() => onPick(key)}>
              <span className="sb-idx" style={{ color: "var(--warn)" }}>▦</span>
              <span className="sb-meta">
                <span className="sb-name">{mf.label}</span>
                <span className="sb-sys">{mf.stem}</span>
              </span>
            </button>
          );
        })}
      </div>
      <div style={{ padding: "14px 18px 22px", fontFamily: "var(--font-mono)", fontSize: "10px", lineHeight: 1.7, color: "var(--tool-dim)" }}>
        Composed from manifests/&lt;id&gt;.json at real 1280×720 rects. Run tools/extract_ui_assets.py to auto-fill from your own copy; empty nodes are drop targets.
      </div>

      {/* GAME FLOW — navigate by the game's OWN flag-conditioned router (db/nav.json) */}
      {R && (
        <div className="sb-group">
          <div className="sb-group-h">Game Flow — real DB router</div>
          <div style={{ padding: "2px 18px 8px", display: "flex", flexDirection: "column", gap: "5px" }}>
            {KEYFLAGS.map((nm) => {
              const d = flagDef(nm); if (!d) return null;
              const v = flags[nm], short = nm.replace(/^[biei]*Flag_/, "");
              const lbl = { display: "flex", alignItems: "center", gap: "6px", fontFamily: "var(--font-mono)", fontSize: "9.5px", color: "var(--ink-dim)" };
              if (d.type === "Bool") return (
                <label key={nm} style={lbl}><input type="checkbox" checked={!!v} onChange={(e) => setFlags({ ...flags, [nm]: e.target.checked })} /><span style={{ overflowWrap: "anywhere" }}>{short}</span></label>
              );
              if (d.type === "List") return (
                <label key={nm} style={{ ...lbl, flexDirection: "column", alignItems: "stretch" }}><span>{short}</span><select value={v} onChange={(e) => setFlags({ ...flags, [nm]: e.target.value })}>{(d.items || []).map((it) => <option key={it} value={it}>{it}</option>)}</select></label>
              );
              return (
                <label key={nm} style={lbl}><span style={{ flex: 1, overflowWrap: "anywhere" }}>{short}</span><input type="number" value={v} onChange={(e) => setFlags({ ...flags, [nm]: +e.target.value })} style={{ width: "62px" }} /></label>
              );
            })}
          </div>
          {["GoToTitle", "GoToWorldMap", "GoToSelectStage"].map((gt) => (
            <button key={gt} className="sb-item" onClick={() => runGoTo(gt)}>
              <span className="sb-idx" style={{ color: "var(--accent)" }}>▸</span>
              <span className="sb-meta"><span className="sb-name">{gt}</span><span className="sb-sys">run the game's router</span></span>
            </button>
          ))}
          {route && (
            <div style={{ padding: "6px 18px 14px", fontFamily: "var(--font-mono)", fontSize: "9px", lineHeight: 1.6, color: "var(--tool-dim)" }}>
              <div style={{ color: "var(--accent)", marginBottom: "3px" }}>→ {route.stage}{route.screen ? "  (" + route.screen + ")" : ""}</div>
              {route.trace.map((t, i) => <div key={i} style={{ overflowWrap: "anywhere" }}>{t}</div>)}
            </div>
          )}
        </div>
      )}
    </div>
  );
}

/* ---------------- DEBUG PANEL ---------------- */
function DebugPanel({ contract, state, family, telemetry, speed, setSpeed, clock }) {
  const st = contract.states.find((s) => s.state === state) || {};
  const visRoles = new Set(contract.visible_overlay_roles[state] || []);
  const curBand = st.timeline_band_id && contract.timeline_bands.find((b) => b.id === st.timeline_band_id);
  const maxBand = Math.max(...contract.timeline_bands.map((b) => b.seconds), 0.001);
  return (
    <div className="debug">
      <div className="dbg-sec">
        <div className="dbg-h">Screen <span className="pill">{contract.source_system}</span></div>
        <div style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "18px", color: "var(--ink)", textTransform: "uppercase", letterSpacing: ".5px", overflowWrap: "anywhere" }}>{contract.screen_id}</div>
        <p className="notes" style={{ marginTop: "8px" }}>{contract.notes}</p>
        <div style={{ marginTop: "10px" }}>
          {contract.source_files.map((f) => (
            <div key={f} className="kv"><span className="k">src</span><span className="v">{f}</span></div>
          ))}
        </div>
      </div>

      <div className="dbg-sec">
        <div className="dbg-h">State Machine <span className="pill">{state}</span></div>
        <div className="states">
          {contract.states.map((s) => (
            <span key={s.state} className={"state-chip " + (s.state === state ? "cur " : "") + (s.input_enabled ? "input-dot" : (s.state === state ? "locked-dot" : ""))}
              title={(s.input_enabled ? "input enabled" : "input locked") + (s.timeout_target ? " → " + s.timeout_target : "")}>
              {s.debug_name}
            </span>
          ))}
        </div>
        <div style={{ marginTop: "11px" }}>
          <div className="kv"><span className="k">enter_scene</span><span className="v">{st.enter_scene || "—"}</span></div>
          <div className="kv"><span className="k">timeout_target</span><span className="v">{st.timeout_target || "—"}</span></div>
          <div className="kv"><span className="k">input_enabled</span><span className="v" style={{ color: st.input_enabled ? "var(--accent)" : "var(--danger)" }}>{st.input_enabled ? "true" : "false (locked)"}</span></div>
          {contract.host_driven && <div className="kv"><span className="k">host_driven</span><span className="v" style={{ color: "var(--warn)" }}>true</span></div>}
        </div>
      </div>

      <div className="dbg-sec">
        <div className="dbg-h">Timeline Bands</div>
        {contract.timeline_bands.map((b) => (
          <div className="band" key={b.id}>
            <span className="bid" style={{ color: curBand && curBand.id === b.id ? "var(--accent)" : undefined }}>{b.id}</span>
            <span className="track"><span className="fill" style={{ width: (b.seconds / maxBand * 100) + "%", opacity: curBand && curBand.id === b.id ? 1 : .4 }} /></span>
            <span className="secs">{b.seconds.toFixed(3)}s</span>
          </div>
        ))}
      </div>

      <div className="dbg-sec">
        <div className="dbg-h">Overlay Layers</div>
        <div className="roles">
          {contract.overlay_layers.map((l) => {
            const on = visRoles.has(l.role);
            return (
              <div key={l.id} className={"role-row " + (on ? "vis" : "off")}>
                <span className="role-dot" /><span className="rname">{l.role}</span>
                <span className="rint">{l.interactive ? "interactive" : ""}</span>
              </div>
            );
          })}
        </div>
      </div>

      {contract.prompt_slots.length > 0 && (
        <div className="dbg-sec">
          <div className="dbg-h">Prompt Slots</div>
          {contract.prompt_slots.map((p) => {
            const shown = p.visible_states.includes(state);
            return (
              <div key={p.slot_id} className={"pslot " + (shown ? "" : "hidden")}>
                <Prompt button={p.button} label={p.label} family={family} />
                <span className="pred">{p.required_predicates.join(" & ")}</span>
              </div>
            );
          })}
        </div>
      )}

      <div className="dbg-sec">
        <div className="dbg-h">Telemetry <span className="pill">{fmt(clock)}s</span></div>
        <div className="telem">
          {telemetry.length === 0 && <div style={{ color: "var(--tool-dim)" }}>press play to walk the state machine…</div>}
          {telemetry.map((t, i) => (
            <div className="tline" key={i}>
              <span className="tt">{fmt(t.t)}</span>
              <span className={"te " + t.type}>{t.type}</span>
              <span className="td">{t.detail}</span>
            </div>
          ))}
        </div>
        <div className="speed">
          <span>speed</span>
          <input type="range" min="0.25" max="3" step="0.25" value={speed} onChange={(e) => setSpeed(parseFloat(e.target.value))} />
          <span style={{ width: "34px", textAlign: "right", color: "var(--ink-dim)" }}>{speed.toFixed(2)}×</span>
        </div>
      </div>
    </div>
  );
}

/* ---------------- MAIN VIEWER ---------------- */
function Viewer() {
  const C = window.SWARD_CONTRACTS, labels = window.SWARD_LABELS;
  const keys = Object.keys(C);
  const [activeKey, setActiveKey] = useState("TitleMenu");
  const [family, setFamily] = useState("letter");
  const [sidebarOpen, setSidebarOpen] = useState(true);
  const [debugOpen, setDebugOpen] = useState(true);
  const [playing, setPlaying] = useState(true);
  const [speed, setSpeed] = useState(1);
  const [walkIdx, setWalkIdx] = useState(0);
  const [telemetry, setTelemetry] = useState([]);
  const clockRef = useRef(0);
  const [clock, setClock] = useState(0);
  const [stepSig, setStepSig] = useState(0);
  const [resetSig, setResetSig] = useState(0);
  const [nativePhase, setNativePhase] = useState("");
  const [gameType, setGameType] = useState(false);
  const [qaShell, setQaShell] = useState(true);

  // when type changes, let the canvas IM layer re-read the font vars
  useEffect(() => {
    const id = setTimeout(() => { if (window.IM && window.IM.refreshTheme) window.IM.refreshTheme(); }, 60);
    return () => clearTimeout(id);
  }, [gameType]);

  const isNative = activeKey.startsWith("native:");
  const nativeKey = isNative ? activeKey.slice(7) : null;
  const isManifest = activeKey.startsWith("mf:");
  const manifestId = isManifest ? activeKey.slice(3) : null;
  const contract = (isNative || isManifest) ? null : C[activeKey];
  const walk = contract ? contract.walk : [];
  const state = walk[walkIdx] || "";

  const pushTelem = useCallback((entries) => {
    setTelemetry((prev) => [...entries, ...prev].slice(0, 60));
  }, []);

  // reset when screen changes
  useEffect(() => {
    if (isNative || isManifest) { return; }
    setWalkIdx(0); setTelemetry([]); clockRef.current = 0; setClock(0);
    const first = C[activeKey].walk[0];
    pushTelem([{ t: 0, type: "enter", detail: first }]);
  }, [activeKey]);

  // advance helper
  const advance = useCallback((dir = 1) => {
    setWalkIdx((idx) => {
      const w = C[activeKey].walk;
      const cur = w[idx];
      const next = (idx + dir + w.length) % w.length;
      const nextState = w[next];
      const curSt = C[activeKey].states.find((s) => s.state === cur);
      const dwell = dwellOf(C[activeKey], cur);
      clockRef.current += dwell;
      const t = clockRef.current;
      const entries = [];
      if (curSt && curSt.timeline_band_id) entries.push({ t, type: "anim", detail: (curSt.enter_scene || cur) + " complete" });
      entries.push({ t, type: "exit", detail: cur });
      entries.push({ t, type: "enter", detail: nextState });
      const nextSt = C[activeKey].states.find((s) => s.state === nextState);
      if (nextSt && !nextSt.input_enabled && nextState !== "Closed" && nextState !== "Hidden")
        entries.push({ t, type: "blocked", detail: "input locked during " + nextState });
      pushTelem(entries);
      setClock(t);
      return next;
    });
  }, [activeKey, pushTelem]);

  // playback loop
  useEffect(() => {
    if (!playing || isNative || isManifest) return;
    const dwell = dwellOf(contract, state) / speed;
    const id = setTimeout(() => advance(1), dwell * 1000);
    return () => clearTimeout(id);
  }, [playing, walkIdx, speed, activeKey, advance, contract, state]);

  // keyboard
  useEffect(() => {
    const onKey = (e) => {
      if (e.key === " ") { e.preventDefault(); setPlaying((p) => !p); return; }
      if (isNative || isManifest) return; // those modes handle their own keys
      if (e.key === "ArrowRight") { setPlaying(false); advance(1); }
      else if (e.key === "ArrowLeft") { setPlaying(false); advance(-1); }
      else if (e.key === "ArrowDown") { setActiveKey((k) => keys[(keys.indexOf(k) + 1) % keys.length]); }
      else if (e.key === "ArrowUp") { setActiveKey((k) => keys[(keys.indexOf(k) - 1 + keys.length) % keys.length]); }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [advance, isNative, isManifest]);

  const reset = () => { setPlaying(false); setWalkIdx(0); setTelemetry([{ t: 0, type: "enter", detail: walk[0] }]); clockRef.current = 0; setClock(0); };

  const ScreenComp = (isNative || isManifest) ? null : SCREEN_COMPONENTS[activeKey]();
  const st = contract ? (contract.states.find((s) => s.state === state) || {}) : {};

  const appCls = "app" + (debugOpen ? "" : " debug-collapsed") + (sidebarOpen ? "" : " sidebar-collapsed") + (gameType ? " gametype" : "");

  return (
    <div className={appCls}>
      <TopBar {...{ family, setFamily, sidebarOpen, setSidebarOpen, debugOpen, setDebugOpen, playing, setPlaying, gameType, setGameType, qaShell, setQaShell }} />
      <Sidebar activeKey={activeKey} onPick={setActiveKey} />

      <div className="stage">
        <div className="stage-grid" />
        <div className="screen-frame">
          {isNative ? (
            <NativeStage overlayKey={nativeKey} family={family} playing={playing} speed={speed} stepSig={stepSig} resetSig={resetSig} onPhase={setNativePhase} />
          ) : isManifest ? (
            <React.Fragment>
              <div className="frame-tag">manifests/{manifestId}.json</div>
              <div className="frame-state" style={{ color: "var(--warn)" }}>reconstructed · 1280×720 rects</div>
              <window.ManifestScreen id={manifestId} />
            </React.Fragment>
          ) : (
            <React.Fragment>
              <div className="frame-tag">{contract.screen_id}</div>
              <div className="frame-state">{state}{!st.input_enabled && state !== "Closed" && state !== "Hidden" && <span className="lock"> · 🔒 locked</span>}{st.input_enabled && " · ◉ input"}</div>
              {ScreenComp ? <ScreenComp contract={contract} state={state} family={family} /> :
                <div style={{ color: "#fff", padding: 40 }}>missing: {activeKey}</div>}
            </React.Fragment>
          )}
          {qaShell && <window.QAShell />}
        </div>

        {/* transport overlay under the frame */}
        <div style={{ position: "absolute", bottom: "10px", left: "50%", transform: "translateX(-50%)", display: "flex", gap: "8px", background: "var(--panel)", border: "1px solid var(--panel-line)", borderRadius: "10px", padding: "6px", boxShadow: "0 8px 24px #0008" }}>
          {isNative ? (
            <React.Fragment>
              <button className="tbtn primary" style={{ minWidth: "130px" }} onClick={() => setPlaying((p) => !p)}>{playing ? IcPause : IcPlay}{playing ? "Pause" : "Play"}</button>
              <button className="tbtn" onClick={() => { setPlaying(false); setStepSig((s) => s + 1); }} title="Advance clock 0.12s">{IcStep}Step +0.12s</button>
              <button className="tbtn ghost" onClick={() => setResetSig((s) => s + 1)} title="Re-open / reset clock">⟲</button>
            </React.Fragment>
          ) : isManifest ? (
            <div style={{ display: "flex", alignItems: "center", gap: "8px", padding: "0 6px", fontFamily: "var(--font-mono)", fontSize: "11px", color: "var(--tool-dim)" }}>
              static composite — drop textures or run the extractor to fill
            </div>
          ) : (
            <React.Fragment>
              <button className="tbtn ghost" onClick={() => { setPlaying(false); advance(-1); }} title="Prev state">{IcReset}</button>
              <button className="tbtn primary" style={{ minWidth: "130px" }} onClick={() => setPlaying((p) => !p)}>{playing ? IcPause : IcPlay}{playing ? "Pause" : "Play walk"}</button>
              <button className="tbtn" onClick={() => { setPlaying(false); advance(1); }} title="Next state">{IcStep}Step</button>
              <button className="tbtn ghost" onClick={reset} title="Reset">⟲</button>
            </React.Fragment>
          )}
        </div>
      </div>

      {isNative
        ? <NativeInspector overlayKey={nativeKey} family={family} phase={nativePhase} />
        : isManifest
        ? <window.ManifestInspector id={manifestId} />
        : <DebugPanel contract={contract} state={state} family={family} telemetry={telemetry} speed={speed} setSpeed={setSpeed} clock={clock} />}
    </div>
  );
}

/* ---------------- NATIVE (canvas immediate-mode) ---------------- */
function NativeStage({ overlayKey, family, playing, speed, stepSig, resetSig, onPhase }) {
  const canvasRef = useRef(null);
  const clockRef = useRef(0);
  const baseRef = useRef(0);     // clock value when the current play segment began
  const wallRef = useRef(0);     // performance.now() when the current play segment began
  const phaseRef = useRef("");
  const overlay = window.SWARD_NATIVE[overlayKey];
  const interactive = overlayKey === "MessageWindow";

  // re-base wall clock whenever play/speed changes so the clock stays correct
  useEffect(() => { baseRef.current = clockRef.current; wallRef.current = performance.now(); }, [playing, speed, overlayKey, resetSig]);

  const renderOnce = useCallback(() => {
    const cv = canvasRef.current; if (!cv) return;
    // wall-clock derived time: any single drawn frame reflects TRUE elapsed
    // time, so animation is correct even if rAF is throttled.
    if (playing) clockRef.current = baseRef.current + (performance.now() - wallRef.current) / 1000 * speed;
    const ctx = cv.getContext("2d");
    const rect = cv.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    if (cv.width !== Math.round(rect.width * dpr)) { cv.width = Math.round(rect.width * dpr); cv.height = Math.round(rect.height * dpr); }
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    window.IM.beginFrame(ctx, rect.width, rect.height, clockRef.current);
    if (overlay.tick) overlay.tick(playing, interactive);
    overlay.Draw(family);
    const p = overlay.phase ? overlay.phase() : "";
    if (p !== phaseRef.current) { phaseRef.current = p; onPhase && onPhase(p); }
  }, [overlay, family, playing, speed, interactive, onPhase]);

  // reset / re-open
  useEffect(() => {
    window.IM.refreshTheme();
    clockRef.current = 0; baseRef.current = 0; wallRef.current = performance.now();
    window.IM.time = 0; // so reset()'s IM.time-N resting offsets compute correctly
    overlay.reset && overlay.reset();
    renderOnce();
  }, [overlayKey, resetSig]);

  // manual step (when paused)
  useEffect(() => {
    if (stepSig > 0) { clockRef.current += 0.12; baseRef.current = clockRef.current; wallRef.current = performance.now(); renderOnce(); }
  }, [stepSig]);

  // rAF loop (drives continuous redraw; clock itself is wall-derived)
  useEffect(() => {
    let raf;
    const loop = () => { renderOnce(); raf = requestAnimationFrame(loop); };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [renderOnce]);

  // keyboard for interactive overlays (MessageWindow)
  useEffect(() => {
    if (!interactive) return;
    const onKey = (e) => {
      if (e.key === "Enter") { overlay.Accept && overlay.Accept(); }
      else if (e.key === "Escape") { overlay.Decline && overlay.Decline(); }
      else if (e.key === "ArrowUp") { e.preventDefault(); overlay.Up && overlay.Up(); }
      else if (e.key === "ArrowDown") { e.preventDefault(); overlay.Down && overlay.Down(); }
      renderOnce();
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [interactive, overlay, renderOnce]);

  return (
    <React.Fragment>
      <div className="frame-tag">{overlay.meta.source}</div>
      <div className="frame-state" style={{ color: "var(--accent-2)" }}>{phaseRef.current}{interactive ? " · ⌨ Enter/Esc/↑↓" : ""}</div>
      <canvas ref={canvasRef} style={{ position: "absolute", inset: 0, width: "100%", height: "100%", display: "block" }} />
    </React.Fragment>
  );
}

function NativeInspector({ overlayKey, family, phase }) {
  const overlay = window.SWARD_NATIVE[overlayKey];
  const m = overlay.meta;
  return (
    <div className="debug">
      <div className="dbg-sec">
        <div className="dbg-h">Native overlay <span className="pill">canvas</span></div>
        <div style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "18px", color: "var(--ink)", textTransform: "uppercase", letterSpacing: ".5px" }}>{m.label}</div>
        <div className="kv" style={{ marginTop: "6px" }}><span className="k">source</span><span className="v">{m.source}</span></div>
        <p className="notes" style={{ marginTop: "10px" }}>{m.blurb}</p>
      </div>

      <div className="dbg-sec">
        <div className="dbg-h">Live phase <span className="pill" style={{ color: "var(--accent-2)", borderColor: "var(--accent-2)" }}>{phase || "—"}</span></div>
        <p className="notes">Driven in real time by <span className="mono">ImGui::GetTime()</span> → here a canvas clock. Play advances the clock; Step nudges it 0.12s; ⟲ re-opens.</p>
      </div>

      <div className="dbg-sec">
        <div className="dbg-h">Recovered constants</div>
        {m.constants.map(([k, v]) => (
          <div className="kv" key={k}><span className="k">{k}</span><span className="v">{v}</span></div>
        ))}
      </div>

      <div className="dbg-sec">
        <div className="dbg-h">Primitive layer</div>
        <div className="notes" style={{ lineHeight: 1.7 }}>
          <div className="mono" style={{ fontSize: "10.5px", color: "var(--ink-dim)" }}>Scale(v) = v · g_aspectRatioScale</div>
          <div className="mono" style={{ fontSize: "10.5px", color: "var(--ink-dim)" }}>ComputeMotion = √(elapsedFrames/total)</div>
          <div className="mono" style={{ fontSize: "10.5px", color: "var(--ink-dim)" }}>Hermite(a,b,t) = a+(b−a)·t²(3−2t)</div>
          <div className="mono" style={{ fontSize: "10.5px", color: "var(--ink-dim)" }}>DrawPauseContainer → 9-slice</div>
          <div style={{ marginTop: "8px", color: "var(--tool-dim)" }}>Asset slots name the atlas they'd sample (general_window.dds, controller.dds). Drop art there to re-skin.</div>
        </div>
      </div>

      <div className="dbg-sec">
        <div className="dbg-h">Prompt family <span className="pill">{family}</span></div>
        <p className="notes">In the recomp this is a UV Y-offset into <span className="mono">controller.dds</span> (PlayStation vs Xbox). Switch it in the top bar.</p>
      </div>
    </div>
  );
}

window.Viewer = Viewer;
