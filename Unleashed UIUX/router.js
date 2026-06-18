/* ============================================================
   router.js — evaluates Sonic Unleashed's OWN navigation database
   (window.SWARD_GAMEDB.nav, the GoTo* flag-conditioned routers) against
   a live flag store, exactly like the game does, to resolve which screen
   you land on. Turns the reconstruction's navigation into the game's
   real rules instead of a hand-drawn walk.
   ============================================================ */
(function () {
  "use strict";
  const DB = window.SWARD_GAMEDB || { flags: [], stages: [], nav: [] };
  const stageById = {}; for (const s of DB.stages) stageById[s.id] = s;
  const navById = {}; for (const n of DB.nav) navById[n.id] = n;

  function defaultFlags() {
    const f = {};
    for (const fl of DB.flags)
      f[fl.name] = fl.type === "Bool" ? false : fl.type === "List" ? ((fl.items && fl.items[0]) || "") : 0;
    return f;
  }

  function coerce(v) {
    if (v === "true") return true; if (v === "false") return false;
    return isNaN(+v) ? v : +v;
  }
  function evalGuard(g, flags) {
    const cur = flags[g.flag], val = coerce(g.value);
    switch (g.op) {
      case "E":  return cur == val;
      case "NE": return cur != val;
      case "L":  return +cur < +val;
      case "G":  return +cur > +val;
      case "LE": return +cur <= +val;
      case "GE": return +cur >= +val;
    }
    return false;
  }

  // resolve a sequence id (GoTo* or SR_Enter*) to a final stage, recording the path
  function resolve(seqId, flags, trace, depth) {
    trace = trace || []; depth = depth || 0;
    if (depth > 40) return { stage: null, trace };
    seqId = String(seqId || "").replace(/^ChangeStage:/, "");
    if (stageById[seqId]) { trace.push(seqId + "  [stage: " + (stageById[seqId].stageType) + "]"); return { stage: stageById[seqId], trace }; }
    const router = navById[seqId];
    if (!router) { trace.push(seqId + "  [unresolved]"); return { stage: null, trace, unresolved: seqId }; }
    for (const c of router.cases) {
      if (c.guards.every((g) => evalGuard(g, flags))) {
        const why = c.guards.map((g) => g.flag + " " + g.op + " " + g.value).join(" & ") || "(default)";
        trace.push(seqId + ":  " + why + "  →  " + c.target);
        if (!c.target) return { stage: null, trace };
        return resolve(c.target, flags, trace, depth + 1);
      }
    }
    return { stage: null, trace };
  }

  function screenKey(stage) { return stage && stage.screen ? "mf:" + stage.screen : null; }

  // the GoTo routers that are interesting for a UI flow demo (frontend navigation)
  function frontendRouters() {
    return DB.nav.map((n) => n.id).filter((id) =>
      /^GoTo(Title|WorldMap|SelectStage|EntryPoint)$/.test(id) ||
      /^GoTo\w+(Town|Boss)$/.test(id) || id === "StartPlay" || id === "Opening");
  }

  window.SWARD_ROUTER = { defaultFlags, evalGuard, resolve, screenKey, stageById, navById, frontendRouters };
})();
