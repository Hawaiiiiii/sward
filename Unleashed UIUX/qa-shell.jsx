/* ============================================================
   qa-shell.jsx — reusable "SGFX QA Shell" debug-HUD overlay
   This is the user's OWN debug overlay (Project Quality-Hero),
   rebuilt as a clean, DATA-DRIVEN, re-skinnable template you can
   drop on top of any screen and repoint at any project.

   Edit window.SGFX_QA_DATA (or pass a `data` prop) to repurpose:
   key/value meta rows, sectioned status, and three tagged task
   lists (top actions / workflow / qa workflows). Status tags
   colour-code: ready·covered = ok, partial·in_progress = mid,
   blocked = no, not_started = dim.
   ============================================================ */

window.SGFX_QA_DATA = {
  title: "SGFX QA Shell",
  meta: [
    ["ticket", "SGFX-QUALITY-HERO"],
    ["project", "SGFX Project Quality Hero"],
    ["phase", "378"],
    ["route", "worldmap"],
  ],
  status: [
    ["pack", "loaded"],
    ["reload", "text=0  asset=0  pack=0"],
    ["audio", "music slots=1"],
    ["worldmap", "mappings=9  focused=(none)"],
  ],
  preflight: [
    ["preflight profile", "G65"],
    ["preflight source", "C:/Users/…/Downloads/sg-preflight"],
    ["actions", "10"],
    ["latest run", "(none)"],
    ["first warning", "(none)"],
  ],
  topActions: [
    ["blocked", "repo_checker_all — Run full repo checkers"],
    ["ready", "daily_live_matrix — Run daily SG check"],
    ["blocked", "repo_checker_idcevo — Run IDCevo repo checkers"],
    ["blocked", "repo_checker_classic — Run classic repo checkers"],
    ["ready", "qa_stack__g65 — Run recommended QA stack for G65"],
  ],
  workflow: [
    ["covered", "deterministic_preflight"],
    ["blocked", "repo_scene_checks"],
    ["blocked", "delivery_checklist"],
    ["partial", "bmw_screenshot_smoke"],
    ["blocked", "rack_review"],
    ["covered", "handoff_evidence"],
  ],
  qaWorkflows: [
    ["not_started", "ambient_layer_preflight"],
    ["not_started", "carpaints_review"],
    ["in_progress", "idcevo_screenshot_changelog"],
    ["not_started", "raco_manual_review"],
    ["not_started", "widget_preflight"],
  ],
};

const QA_TAG_CLASS = {
  ready: "ok", covered: "ok",
  partial: "mid", in_progress: "mid",
  blocked: "no", not_started: "dim",
};

function QATask({ status, label }) {
  return (
    <div className="qa-task">
      <span className={"qa-tag " + (QA_TAG_CLASS[status] || "dim")}>[{status}]</span>
      <span className="qa-task-label">{label}</span>
    </div>
  );
}

function QAShell({ data, fps = 59.9 }) {
  const d = data || window.SGFX_QA_DATA;
  return (
    <React.Fragment>
      <div className="qa-fps">FPS: {fps.toFixed(2)}</div>
      <div className="qa-shell">
        <div className="qa-title">{d.title}</div>

        <div className="qa-sec">
          {d.meta.map(([k, v]) => (
            <div className="qa-kv" key={k}><span className="qa-k">{k}:</span><span className="qa-v">{v}</span></div>
          ))}
        </div>
        <div className="qa-sec">
          {d.status.map(([k, v]) => (
            <div className="qa-kv" key={k}><span className="qa-k">{k}:</span><span className="qa-v">{v}</span></div>
          ))}
        </div>
        <div className="qa-sec">
          {d.preflight.map(([k, v]) => (
            <div className="qa-kv" key={k}><span className="qa-k">{k}:</span><span className="qa-v">{v}</span></div>
          ))}
        </div>

        <div className="qa-sec">
          <div className="qa-h">top actions:</div>
          {d.topActions.map((t, i) => <QATask key={i} status={t[0]} label={t[1]} />)}
        </div>
        <div className="qa-sec">
          <div className="qa-h">workflow:</div>
          {d.workflow.map((t, i) => <QATask key={i} status={t[0]} label={t[1]} />)}
        </div>
        <div className="qa-sec qa-sec-last">
          <div className="qa-h">qa workflows: {d.qaWorkflows.length}</div>
          {d.qaWorkflows.map((t, i) => <QATask key={i} status={t[0]} label={t[1]} />)}
        </div>
      </div>
    </React.Fragment>
  );
}

window.QAShell = QAShell;
