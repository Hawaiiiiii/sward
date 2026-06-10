/* ============================================================
   screens-ingame.jsx — In-game HUD, cinematic, overlay screens
   Uses shared primitives from window (Slot, Layer, PromptRow,
   makeVis, bandOf) defined in screens-frontend.jsx.
   ============================================================ */
const { Slot: SlotG, Layer: LayerG, PromptRow: PromptRowG, makeVis: makeVisG, bandOf: bandOfG } = window;

/* small HUD counter */
function Counter({ label, value, big, align = "left", accent }) {
  return (
    <div className="counter" style={{ alignItems: align === "right" ? "flex-end" : "flex-start" }}>
      <span className="clab" style={{ fontSize: "1.5cqh" }}>{label}</span>
      <span className="cval" style={{ fontSize: big ? "4.6cqh" : "3.2cqh", color: accent ? "var(--accent)" : "#fff" }}>{value}</span>
    </div>
  );
}

/* ============================================================
   SONIC STAGE HUD (day)
   ============================================================ */
function SonicStageHUDScreen({ contract, state, family }) {
  const vis = makeVisG(contract, state);
  const band = bandOfG(contract, state);
  const boost = state === "Navigate";
  const medal = state === "Confirm";
  return (
    <div className="scr">
      <SlotG label="day stage gameplay" style={{ position: "absolute", inset: 0, borderRadius: 0, opacity: .3, background: "linear-gradient(180deg,#173a5e,#0b1f33)" }} />

      {/* top-left counters */}
      <LayerG show={vis("counters_tl")} band={band} motion="slideL" style={{ left: "3cqw", top: "4cqh", display: "flex", gap: "3cqw" }} z={4}>
        <Counter label="Time" value="01:24:60" />
        <Counter label="Score" value="128,400" />
        <Counter label="Rings" value="86" accent />
        <Counter label="EXP" value="4,210" />
      </LayerG>

      {/* bottom-left: speed + ring-energy/boost gauge + ring counter */}
      <LayerG show={vis("gauge_bl")} band={band} motion="rise" style={{ left: "3cqw", bottom: "5cqh", width: "30cqw" }} z={4}>
        <div style={{ display: "flex", alignItems: "flex-end", gap: "1.4cqw", marginBottom: "1.4cqh" }}>
          <Counter label="Speed" value="312" big />
          <span style={{ fontFamily: "var(--font-cond)", fontWeight: 700, fontSize: "1.8cqh", color: "var(--ink-dim)", marginBottom: ".6cqh" }}>MPH</span>
        </div>
        <div className="gauge" style={{ position: "relative", height: "3cqh", width: "100%" }}>
          <div className="gfill" style={{ width: boost ? "100%" : "62%", background: boost ? "linear-gradient(90deg,var(--warn),var(--accent))" : "linear-gradient(90deg,var(--accent-2),var(--accent))", transition: `width ${band}s ease` }} />
          <span style={{ position: "absolute", left: "1cqh", top: "50%", transform: "translateY(-50%)", fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "#06210f", fontWeight: 700 }}>RING ENERGY · boost gauge</span>
        </div>
      </LayerG>

      <LayerG show={vis("ring_bl")} band={band} style={{ left: "3cqw", bottom: "13.5cqh", fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "var(--ink-faint)" }} z={4}>ui_playscreen · gauge_frame</LayerG>

      {/* bottom-right sidecars */}
      <LayerG show={vis("sidecar_br")} band={band} motion="rise" style={{ right: "3cqw", bottom: "5cqh", display: "flex", flexDirection: "column", alignItems: "flex-end", gap: "1cqh" }} z={4}>
        <div style={{ transform: medal ? "scale(1.12)" : "scale(1)", transition: `transform ${band}s cubic-bezier(.2,.9,.2,1)`, display: "flex", alignItems: "center", gap: "1cqh" }}>
          <div style={{ width: "4cqh", height: "4cqh", borderRadius: "50%", background: medal ? "var(--warn)" : "var(--chrome-2)", border: "1px solid var(--stroke)" }} />
          <Counter label="Medals" value="14 / 50" align="right" accent={medal} />
        </div>
        <span style={{ fontFamily: "var(--font-mono)", fontSize: "1.2cqh", color: "var(--ink-faint)" }}>u_info · speed_bonus · medal_get</span>
      </LayerG>
    </div>
  );
}

/* ============================================================
   WEREHOG STAGE HUD (night)
   ============================================================ */
function WerehogStageHUDScreen({ contract, state, family }) {
  const vis = makeVisG(contract, state);
  const band = bandOfG(contract, state);
  const charge = state === "Navigate";
  const hit = state === "Confirm";
  const Gauge = ({ label, pct, color }) => (
    <div style={{ marginBottom: "1.2cqh" }}>
      <div style={{ fontFamily: "var(--font-cond)", fontWeight: 700, fontSize: "1.4cqh", textTransform: "uppercase", color: "var(--ink-dim)", marginBottom: ".4cqh" }}>{label}</div>
      <div className="gauge" style={{ position: "relative", height: "2.2cqh", width: "26cqw" }}>
        <div className="gfill" style={{ width: pct, background: color, transition: `width ${band}s ease` }} />
      </div>
    </div>
  );
  return (
    <div className="scr">
      <SlotG label="night stage gameplay" style={{ position: "absolute", inset: 0, borderRadius: 0, opacity: .32, background: "linear-gradient(180deg,#2a1840,#100a1c)" }} />

      {/* left gauge stack: unleash / life / shield */}
      <LayerG show={vis("gauge_stack_l")} band={band} motion="slideL" style={{ left: "3cqw", top: "5cqh" }} z={4}>
        <Gauge label="Unleash" pct={charge ? "100%" : "48%"} color={charge ? "linear-gradient(90deg,var(--warn),#fff)" : "linear-gradient(90deg,#a64bff,#d9a3ff)"} />
        <Gauge label="Life" pct="74%" color="linear-gradient(90deg,var(--danger),#ff9a9a)" />
        <Gauge label="Shield" pct="55%" color="linear-gradient(90deg,var(--accent-2),#9bd0ff)" />
        <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.2cqh", color: "var(--ink-faint)", marginTop: ".4cqh" }}>ui_playscreen_ev · CEvilHudGuide</div>
      </LayerG>

      <LayerG show={vis("counters_tl")} band={band} style={{ right: "3cqw", top: "5cqh", display: "flex", gap: "2.6cqw" }} z={4}>
        <Counter label="Time" value="03:58:12" align="right" />
        <Counter label="Score" value="44,900" align="right" />
      </LayerG>

      {/* right hit counter */}
      <LayerG show={vis("hit_counter_r")} band={band} motion="grow" style={{ right: "6cqw", top: "34cqh", display: "flex", flexDirection: "column", alignItems: "flex-end" }} z={5}>
        <div style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: hit ? "9cqh" : "6cqh", lineHeight: 1, color: hit ? "var(--warn)" : "#fff", transition: `all ${band}s cubic-bezier(.2,.9,.2,1)`, textShadow: "0 .4cqh 1.2cqh #000b" }}>
            ×24
        </div>
        <div style={{ fontFamily: "var(--font-cond)", fontWeight: 700, fontSize: "2cqh", textTransform: "uppercase", color: "var(--ink-dim)" }}>Combo</div>
        <span style={{ fontFamily: "var(--font-mono)", fontSize: "1.2cqh", color: "var(--ink-faint)", marginTop: ".4cqh" }}>ui_playscreen_ev_hit</span>
      </LayerG>

      <LayerG show={vis("guide_sidecar")} band={band} style={{ left: "3cqw", bottom: "5cqh", fontFamily: "var(--font-mono)", fontSize: "1.4cqh", color: "var(--ink-faint)" }} z={4}>
        EvilHudGuide ▸ combat action hints
      </LayerG>
    </div>
  );
}

/* ============================================================
   BOSS / FINAL HUD  (name reveal ladder → segmented gauge)
   ============================================================ */
function BossHUDScreen({ contract, state, family }) {
  const vis = makeVisG(contract, state);
  const band = bandOfG(contract, state);
  const breaking = state === "Navigate";
  return (
    <div className="scr">
      <SlotG label="boss arena · HideLayer suppresses other UI" style={{ position: "absolute", inset: 0, borderRadius: 0, opacity: .3, background: "radial-gradient(120% 100% at 50% 30%,#3a1020,#0a0510)" }} />

      {/* name reveal band */}
      <LayerG show={vis("name_band")} band={band} motion="grow" style={{ left: 0, right: 0, top: "36cqh", display: "flex", flexDirection: "column", alignItems: "center" }} z={6}>
        <div style={{ position: "absolute", left: 0, right: 0, top: "-2cqh", height: "16cqh", background: "linear-gradient(90deg,transparent,#000a 30% 70%,transparent)" }} />
        <div style={{ position: "relative", fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "9cqh", lineHeight: 1, color: "#fff", letterSpacing: ".3cqh", textTransform: "uppercase", textShadow: "0 .6cqh 2cqh #000c" }}>Boss Name</div>
        <div style={{ position: "relative", fontFamily: "var(--font-cond)", fontWeight: 600, fontSize: "2.6cqh", color: "var(--danger)", letterSpacing: ".8cqh", textTransform: "uppercase", marginTop: ".6cqh" }}>name_so · 01–05_Anim</div>
      </LayerG>

      {/* segmented gauge */}
      <LayerG show={vis("gauge")} band={band} motion="fade" style={{ left: "50%", top: "8cqh", width: "60cqw", transform: "translateX(-50%)" }} z={4}>
        <div style={{ fontFamily: "var(--font-cond)", fontWeight: 700, fontSize: "1.8cqh", textTransform: "uppercase", color: "var(--ink-dim)", textAlign: "center", marginBottom: ".8cqh", letterSpacing: ".2cqh" }}>Boss Name</div>
        <div style={{ position: "relative", height: "3.4cqh", display: "flex", gap: ".4cqh" }}>
          {/* gauge_1 / gauge_2 segments */}
          <div className="gauge" style={{ position: "relative", flex: 2, height: "100%" }}>
            <div className="gfill" style={{ width: breaking ? "40%" : "82%", background: "linear-gradient(90deg,var(--danger),#ff8a5b)", transition: `width ${band}s cubic-bezier(.3,.7,.2,1)` }} />
          </div>
          <div className="gauge" style={{ position: "relative", flex: 1, height: "100%" }}>
            <div className="gfill" style={{ width: "100%", background: "linear-gradient(90deg,#ff8a5b,var(--warn))" }} />
          </div>
          {/* breakpoint marker */}
          <div style={{ position: "absolute", left: "62%", top: "-.8cqh", bottom: "-.8cqh", width: ".4cqh", background: "var(--warn)", boxShadow: "0 0 1.4cqh var(--warn)" }} />
        </div>
        <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.2cqh", color: "var(--ink-faint)", textAlign: "center", marginTop: ".6cqh" }}>ui_boss_gauge · gauge_1 / gauge_2 / gauge_breakpoint</div>
      </LayerG>

      <LayerG show={vis("transient_fx")} band={0.3} style={{ left: "50%", top: "58cqh", transform: "translateX(-50%)", fontFamily: "var(--font-mono)", fontSize: "1.4cqh", color: "var(--warn)" }} z={7}>
        {breaking ? "Size_Anim · 100f gauge break" : "GoToBoss ▸ sequence-owned"}
      </LayerG>
    </div>
  );
}

/* ============================================================
   SUBTITLE / CUTSCENE  (movie ownership state + bottom caption)
   ============================================================ */
function SubtitleCutsceneScreen({ contract, state, family }) {
  const vis = makeVisG(contract, state);
  const band = bandOfG(contract, state);
  return (
    <div className="scr" style={{ background: "#000" }}>
      <LayerG show={vis("movie")} band={band} style={{ inset: 0 }} z={2}>
        <SlotG label="PlayMovie · evrt_*  ·  KeepMovieUntilStageChange" style={{ position: "absolute", inset: 0, borderRadius: 0, opacity: .55 }} />
      </LayerG>

      {/* letterbox */}
      <LayerG show={vis("letterbox")} band={band} style={{ inset: 0 }} z={5}>
        <div className="letterbox" style={{ top: 0, height: "12cqh", transition: `height ${band}s ease` }} />
        <div className="letterbox" style={{ bottom: 0, height: "12cqh", transition: `height ${band}s ease` }} />
        <div style={{ position: "absolute", left: "2cqw", top: "4cqh", fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "var(--ink-faint)" }}>Config::CutsceneAspectRatio</div>
      </LayerG>

      {/* subtitle window (bottom-anchored ConverseData cue) */}
      <LayerG show={vis("subtitle")} band={band} style={{ left: 0, right: 0, bottom: "15cqh", display: "flex", justifyContent: "center" }} z={6}>
        <div style={{ maxWidth: "70cqw", textAlign: "center" }}>
          <div style={{ fontSize: "3cqh", color: "#fff", textShadow: "0 .3cqh 1cqh #000", lineHeight: 1.35, fontWeight: 500 }}>Authored caption window — a frame range on a ConverseData cell, anchored BOTTOM.</div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "var(--ink-faint)", marginTop: "1cqh" }}>inspire_resource.xml · cue [142–298]</div>
        </div>
      </LayerG>

      <PromptRowG prompts={contract.prompt_slots} family={family} state={state} style={{ right: "4cqw", bottom: "16cqh" }} />
    </div>
  );
}

/* ============================================================
   OVERLAYS  (achievement toast + staged message window + guide)
   ============================================================ */
function OverlaysScreen({ contract, state, family }) {
  const vis = makeVisG(contract, state);
  const band = bandOfG(contract, state);
  const toastExpanded = ["ToastIn", "ToastHold", "MsgAppear", "MsgControls"].includes(state);
  const controlsVisible = state === "Await";
  return (
    <div className="scr">
      <SlotG label="host screen underneath" style={{ position: "absolute", inset: 0, borderRadius: 0, opacity: .22 }} />

      {/* achievement toast (queue-driven, OVERLAY_DURATION 3) */}
      <LayerG show={vis("toast")} band={band} style={{ left: "50%", top: "5cqh", transform: "translateX(-50%)" }} z={6}>
        <div style={{ display: "flex", alignItems: "center", gap: "1.6cqh", padding: "1.4cqh 2cqh", borderRadius: "var(--r-l)", background: "var(--chrome-2)", border: "1px solid var(--stroke)",
          width: toastExpanded ? "44cqw" : "8cqh", overflow: "hidden", transition: `width ${band}s cubic-bezier(.2,.8,.2,1)`, boxShadow: "0 1cqh 3cqh #000a" }}>
          <div style={{ width: "5cqh", height: "5cqh", flexShrink: 0, borderRadius: "var(--r-s)", background: "var(--accent)", display: "flex", alignItems: "center", justifyContent: "center", color: "#06210f", fontSize: "2.6cqh", fontWeight: 800 }}>★</div>
          <div style={{ minWidth: 0, opacity: toastExpanded ? 1 : 0, transition: "opacity .2s" }}>
            <div style={{ fontFamily: "var(--font-cond)", fontWeight: 800, fontSize: "2.2cqh", color: "#fff", textTransform: "uppercase", whiteSpace: "nowrap" }}>Achievement Unlocked</div>
            <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "var(--ink-faint)", whiteSpace: "nowrap" }}>achievement_overlay.cpp · queue dequeue</div>
          </div>
        </div>
      </LayerG>

      {/* modal message window (staged reveal) */}
      <LayerG show={vis("modal_backdrop")} band={band} style={{ inset: 0 }} z={7}><div className="gbackdrop" /></LayerG>
      <LayerG show={vis("modal")} band={band} motion="fade" style={{ left: "50%", top: "50%", width: "50cqw", transform: "translate(-50%,-50%)" }} z={8}>
        <div className="gwin" style={{ position: "relative", padding: "4cqh 4cqh 3cqh" }}>
          <div style={{ fontSize: "2.8cqh", color: "#fff", textAlign: "center", fontWeight: 500, lineHeight: 1.4 }}>Return to the World Map? Unsaved progress in this stage will be lost.</div>
          <div style={{ fontFamily: "var(--font-mono)", fontSize: "1.3cqh", color: "var(--ink-faint)", textAlign: "center", marginTop: "1.4cqh" }}>message_window.cpp · staged reveal</div>
          {/* controls revealed only after first accept */}
          <div style={{ display: "flex", justifyContent: "center", gap: "2cqw", marginTop: "3cqh", opacity: controlsVisible ? 1 : 0.25, transition: `opacity ${band}s ease` }}>
            <div style={{ padding: "1cqh 3cqw", borderRadius: "var(--r-m)", background: controlsVisible ? "var(--accent)" : "var(--chrome)", color: controlsVisible ? "#06210f" : "var(--ink-faint)", fontWeight: 700, fontSize: "2.2cqh" }}>Yes</div>
            <div style={{ padding: "1cqh 3cqw", borderRadius: "var(--r-m)", background: "var(--chrome)", color: "var(--ink-dim)", fontWeight: 700, fontSize: "2.2cqh", border: "1px solid var(--stroke)" }}>No</div>
          </div>
          {!controlsVisible && (state === "MsgAppear" || state === "MsgControls") &&
            <div style={{ textAlign: "center", marginTop: "1cqh", fontFamily: "var(--font-mono)", fontSize: "1.4cqh", color: "var(--warn)" }}>first accept reveals controls ▸ no instant confirm</div>}
        </div>
      </LayerG>

      <PromptRowG prompts={contract.prompt_slots} family={family} state={state} style={{ left: "50%", bottom: "8cqh", transform: "translateX(-50%)" }} />
    </div>
  );
}

Object.assign(window, {
  SonicStageHUDScreen, WerehogStageHUDScreen, BossHUDScreen,
  SubtitleCutsceneScreen, OverlaysScreen,
});
