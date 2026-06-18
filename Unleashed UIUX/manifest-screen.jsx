/* ============================================================
   manifest-screen.jsx — layout-driven reconstruction renderer
   Consumes a SWARD screen manifest (manifests/<id>.json) — node
   rects authored in 1280x720 reference space — and auto-composes
   the screen. Each image node renders as an <image-slot> whose
   `src` points at the extracted texture (assetBase + tex):
     • if that PNG exists  -> it loads automatically
     • if it doesn't       -> placeholder + drag-to-fill target
   So once you run the extractor (tools/extract_ui_assets.py) the
   screens populate themselves with NO manual placement — real
   rects, real textures, from your own game files.
   ============================================================ */
const { useState: useStateM, useEffect: useEffectM } = React;

window.SWARD_MANIFESTS = [
  // ---- frontend / menus ----
  { id: "title", label: "Main Menu", stem: "ui_mainmenu" },
  { id: "world_map", label: "World Map", stem: "ui_worldmap" },
  { id: "world_map_help", label: "World Map — Help", stem: "ui_worldmap_help" },
  { id: "options", label: "Options / General", stem: "ui_general" },
  { id: "status", label: "Status / Skills", stem: "ui_status" },
  { id: "shop", label: "Shop", stem: "ui_shop" },
  { id: "town", label: "Town / Hub", stem: "ui_townscreen" },
  { id: "mediaroom", label: "Media Room", stem: "ui_mediaroom" },
  { id: "help", label: "Help", stem: "ui_help" },
  // ---- in-game ----
  { id: "sonic_hud", label: "In-Game HUD", stem: "ui_prov_playscreen" },
  { id: "pause", label: "Pause", stem: "ui_pause" },
  { id: "loading", label: "Loading", stem: "ui_loading" },
  { id: "start", label: "Stage Start", stem: "ui_start" },
  { id: "gate", label: "Stage Gate", stem: "ui_gate" },
  { id: "mission_screen", label: "Mission Screen", stem: "ui_missionscreen" },
  { id: "mission", label: "Mission Objective", stem: "ui_misson" },
  { id: "qte", label: "QTE Prompts", stem: "ui_qte" },
  { id: "balloon", label: "Town Balloon", stem: "ui_balloon" },
  { id: "exstage", label: "EX Stage (Tails)", stem: "ui_exstage" },
  // ---- boss / results ----
  { id: "boss", label: "Boss HUD", stem: "ui_boss_gauge" },
  { id: "result", label: "Mission Result", stem: "ui_result" },
  { id: "result_ex", label: "Result (EX / Tails)", stem: "ui_result_ex" },
  { id: "item_result", label: "Item Result", stem: "ui_itemresult" },
];

// Render a screen with the REAL CSD runtime (full per-cast keyframe animation) when
// that rich data exists at sgfx_ui_cpp/data/<id>.json — the actual game motion, not a
// fade — else fall back to the static manifest layout.
function ManifestScreen({ id }) {
  const [rich, setRich] = useStateM(undefined);   // undefined=checking, null=none, obj=rich data
  useEffectM(() => {
    setRich(undefined);
    fetch("sgfx_ui_cpp/data/" + id + ".json?t=" + Date.now())
      .then((r) => (r.ok ? r.json() : null))
      .then((d) => setRich(d && d.scenes && d.scenes.length ? d : null))
      .catch(() => setRich(null));
  }, [id]);
  if (rich === undefined) return <div className="scr" style={{ display: "flex", alignItems: "center", justifyContent: "center", color: "var(--ink-faint)", fontFamily: "var(--font-mono)", fontSize: "1.6cqh" }}>loading…</div>;
  if (rich && window.CsdPlayer) return <CsdPlayerScreen id={id} data={rich} />;
  return <ManifestStatic id={id} />;
}

// Canvas that plays the real CSD animation each frame via window.CsdPlayer.
// .scr is already position:absolute/inset:0 (fills the frame) — do NOT override it;
// the canvas fills .scr and the player syncs its buffer to the displayed size.
function CsdPlayerScreen({ id, data }) {
  const ref = React.useRef(null);
  useEffectM(() => {
    const cv = ref.current; if (!cv || !window.CsdPlayer) return;
    const ckey = window.CSD_CONTRACT_FOR && window.CSD_CONTRACT_FOR[id];
    const contract = (ckey && window.SWARD_CONTRACTS) ? window.SWARD_CONTRACTS[ckey] : null;
    const player = new window.CsdPlayer(cv, data, "assets/" + id + "/", { contract });
    return () => player.stop();
  }, [id, data]);
  return <div className="scr"><canvas ref={ref} style={{ position: "absolute", inset: 0, width: "100%", height: "100%", display: "block" }} /></div>;
}

function ManifestStatic({ id }) {
  const [m, setM] = useStateM(null);
  const [err, setErr] = useStateM(null);
  const [tick, setTick] = useStateM(0);   // ms since intro start (drives the cascade)
  useEffectM(() => {
    setM(null); setErr(null); setTick(0);
    fetch("manifests/" + id + ".json?t=" + Date.now())  // cache-bust: always load the latest regenerated manifest
      .then((r) => { if (!r.ok) throw new Error("HTTP " + r.status); return r.json(); })
      .then(setM).catch((e) => setErr(String(e)));
  }, [id]);

  // recomp-feel intro cascade: each node fades + slides up, staggered, with the
  // installer's ease-out (imgui_utils ComputeMotion = sqrt of a 60fps-frame ramp).
  // It always settles to opacity 1 / offset 0 (= the verified static layout), so the
  // worst case is a sub-second glitch, never a broken screen.
  useEffectM(() => {
    if (!m) return;
    const start = performance.now();
    let raf;
    const loop = () => {
      const el = performance.now() - start;
      setTick(el);
      if (el < 2200) raf = requestAnimationFrame(loop);   // stop once fully settled
    };
    raf = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(raf);
  }, [m, id]);

  if (err) return <div className="scr" style={{ display: "flex", alignItems: "center", justifyContent: "center", color: "var(--danger)", fontFamily: "var(--font-mono)", fontSize: "1.6cqh", padding: "6cqh", textAlign: "center" }}>manifests/{id}.json failed to load<br />{err}</div>;
  if (!m) return <div className="scr" style={{ display: "flex", alignItems: "center", justifyContent: "center", color: "var(--ink-faint)", fontFamily: "var(--font-mono)", fontSize: "1.6cqh" }}>loading manifest…</div>;

  const [rw, rh] = m.ref || [1280, 720];
  const base = m.assetBase || ("assets/" + id + "/");
  const pct = (n) => ({
    position: "absolute",
    left: (n.rect[0] / rw * 100) + "%", top: (n.rect[1] / rh * 100) + "%",
    width: (n.rect[2] / rw * 100) + "%", height: (n.rect[3] / rh * 100) + "%",
    zIndex: n.z || 1,
  });

  // per-node intro: stagger OFF frames each, ramp over DUR frames, ease-out via sqrt
  // (identical curve to UnleashedRecomp ui/imgui_utils.cpp ComputeMotion / window.IM).
  const OFF = 2, DUR = 14;
  const intro = (i) => {
    const f = tick / 1000 * 60;                                  // elapsed in 60fps frames
    const lin = Math.max(0, Math.min(1, (f - i * OFF) / DUR));
    const e = Math.sqrt(lin);
    return { o: e, ty: (1 - e) * 20 };                          // fade + 20px slide-up
  };

  return (
    <div className="scr" style={{ background: id === "title" ? "radial-gradient(120% 120% at 70% 20%, #16243a, #070b12 70%)" : "transparent" }}>
      {m.nodes.map((n, i) => {
        const a = intro(i);
        const xform = a.ty > 0.01 ? `translateY(${a.ty}px)` : undefined;
        if (n.type === "text") {
          return (
            <div key={n.id} style={{ ...pct(n), display: "flex", alignItems: "center", justifyContent: n.align === "center" ? "center" : "flex-start",
              fontFamily: "var(--font-ui)", fontSize: (n.size || 2.2) + "cqh", color: n.color || "var(--ink)", letterSpacing: ".05cqh", opacity: a.o, transform: xform }}>{n.text}</div>
          );
        }
        return (
          <image-slot key={n.id} id={"mf_" + id + "__" + n.id}
            src={base + n.tex} fit={n.fit || "cover"} shape={n.shape || "rect"}
            uv={n.uv ? n.uv.join(",") : undefined}
            placeholder={n.id + "  ·  " + n.tex}
            style={{ ...pct(n), opacity: (n.alpha != null ? n.alpha : 1) * a.o, transform: xform }}></image-slot>
        );
      })}
    </div>
  );
}

function ManifestInspector({ id }) {
  const meta = window.SWARD_MANIFESTS.find((x) => x.id === id) || { label: id };
  const [m, setM] = useStateM(null);
  useEffectM(() => { fetch("manifests/" + id + ".json?t=" + Date.now()).then((r) => r.json()).then(setM).catch(() => {}); }, [id]);
  const base = (m && m.assetBase) || ("assets/" + id + "/");
  return (
    <div className="debug">
      <div className="dbg-sec">
        <div className="dbg-h">Reconstruction <span className="pill">manifest</span></div>
        <div style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "18px", color: "var(--ink)", textTransform: "uppercase", letterSpacing: ".5px" }}>{meta.label}</div>
        <div className="kv" style={{ marginTop: "6px" }}><span className="k">stem</span><span className="v">{meta.stem}</span></div>
        <div className="kv"><span className="k">manifest</span><span className="v">manifests/{id}.json</span></div>
        <div className="kv"><span className="k">assets</span><span className="v">{base}</span></div>
        <p className="notes" style={{ marginTop: "10px" }}>Nodes are placed at their real 1280×720 rects. A node's texture auto-loads from <span className="mono">{base}</span> if present; otherwise it's a drop target. Fill = run the extractor, no manual placement.</p>
      </div>
      <div className="dbg-sec">
        <div className="dbg-h">Nodes {m ? <span className="pill">{m.nodes.length}</span> : null}</div>
        {m ? m.nodes.map((n) => (
          <div className="kv" key={n.id}><span className="k">{n.id}</span><span className="v">{n.type === "text" ? "[text]" : n.tex}</span></div>
        )) : <div className="notes">loading…</div>}
      </div>
      <div className="dbg-sec">
        <div className="dbg-h">Auto-fill pipeline</div>
        <div className="notes" style={{ lineHeight: 1.7 }}>
          <div className="mono" style={{ fontSize: "10.5px", color: "var(--ink-dim)" }}>1 · python tools/extract_ui_assets.py</div>
          <div className="mono" style={{ fontSize: "10.5px", color: "var(--ink-dim)" }}>2 · unpacks .ar/.arl (HedgeArcPack)</div>
          <div className="mono" style={{ fontSize: "10.5px", color: "var(--ink-dim)" }}>3 · DDS → PNG, emits manifest</div>
          <div className="mono" style={{ fontSize: "10.5px", color: "var(--ink-dim)" }}>4 · writes {base}*.png</div>
          <div style={{ marginTop: "8px", color: "var(--tool-dim)" }}>Then this screen renders 1:1 from your own files — no dragging.</div>
        </div>
      </div>
    </div>
  );
}

window.ManifestScreen = ManifestScreen;
window.ManifestInspector = ManifestInspector;
