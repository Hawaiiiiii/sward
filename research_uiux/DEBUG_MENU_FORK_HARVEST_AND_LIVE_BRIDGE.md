<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# Debug Menu Fork Harvest And Live Bridge

Phase 116 harvests the local `UnleashedRecomp-debug-menu/` fork as a reference-only source of runtime structure and operator-shell patterns. The fork is not vendored into SWARD, and the local tree remains ignored. The useful material is converted into SWARD-owned UI Lab evidence, typed inspector labels, and a direct live bridge.

Machine-readable harvest: `research_uiux/data/debug_menu_fork_harvest.json`

## Harvested API Surfaces

### CSD Manager

Useful paths:

- `api/CSD/Manager/csdmProject.h`
- `api/CSD/Manager/csdmScene.h`
- `api/CSD/Manager/csdmNode.h`
- `api/CSD/Manager/csdmRCPtr.h`
- `api/CSD/Platform/csdTexList.h`

Key typed fields now carried into the UI Lab live-state vocabulary:

- `CSD.Manager.CScene.m_MotionFrame`
- `CSD.Manager.CScene.m_MotionRepeatType`
- `CSD.Manager.CNode.m_pMotionPattern`

These are best treated as CSD readiness and motion-state anchors. They should drive future CSD project/scene/node/layer inspection, not a standalone renderer.

### SWA CSD Wrappers

Useful paths:

- `api/SWA/CSD/CsdProject.h`
- `api/SWA/CSD/CsdDatabaseWrapper.h`
- `api/SWA/CSD/CsdTexListMirage.h`
- `api/SWA/CSD/GameObjectCSD.h`

Key typed fields:

- `SWA.CSD.CCsdProject.m_rcProject`
- `SWA.CSD.CGameObjectCSD.m_rcProject`

These map runtime CSD owner objects back to the UI Lab target CSD readiness events such as `target-csd-project-made` and `stage-target-csd-bound`.

### SWA HUD Owners

Useful paths:

- `api/SWA/HUD/Sonic/HudSonicStage.h`
- `api/SWA/HUD/Loading/Loading.h`
- `api/SWA/HUD/Pause/HudPause.h`
- `api/SWA/HUD/GeneralWindow/GeneralWindow.h`
- `api/SWA/HUD/SaveIcon/SaveIcon.h`

Key typed fields:

- `SWA.HUD.CHudSonicStage.m_rcPlayScreen`
- `SWA.HUD.CHudSonicStage.m_rcSpeedGauge`
- `SWA.HUD.CLoading.m_LoadingDisplayType`
- `SWA.HUD.CHudPause.m_Action`
- `SWA.HUD.CGeneralWindow.m_rcGeneral`
- `SWA.HUD.CSaveIcon.m_IsVisible`

These are the next typed HUD-owner/status inspector anchors for Sonic HUD, loading, pause/general windows, and save icon visibility.

### SWA System And GameMode

Useful paths:

- `api/SWA/System/ApplicationDocument.h`
- `api/SWA/System/GameDocument.h`
- `api/SWA/System/GameMode/GameModeStage.h`
- `api/SWA/System/GameMode/Title/TitleMenu.h`
- `api/SWA/System/GameMode/Title/TitleStateBase.h`

Key typed fields:

- `SWA.System.GameMode.CGameModeStage`
- `SWA.System.GameMode.Title.CTitleMenu.m_CursorIndex`
- `SWA.System.GameMode.Title.CTitleStateBase.CMember.m_pTitleMenu`
- `SWA.System.GameDocument.CMember.m_StageName`

These remain high-value for title-menu readiness, deterministic stage boot, game-mode ownership, loading transitions, and active-stage context.

### Reddog Operator Shell

Useful paths:

- `ui/reddog/reddog_manager.h`
- `ui/reddog/reddog_window.h`
- `ui/reddog/debug_draw.h`
- `ui/reddog/windows/window_list.h`
- `ui/reddog/windows/counter_window.h`
- `ui/reddog/windows/view_window.h`
- `ui/reddog/windows/exports_window.h`
- `ui/reddog/windows/welcome_window.h`

Useful concepts:

- `Reddog.Manager`
- `Reddog.DebugDraw`

SWARD already ports this as a default-open operator shell with window list, counters, exports, view/debug toggles, and a foreground debug-draw layer.

## Live Bridge

The Phase 116 bridge keeps native BMP capture as visual proof and JSONL as durable evidence, but adds a direct tool-facing live bridge so Codex/tools can ask the running UI Lab what it knows.

Transport:

- Windows named pipe: `\\.\pipe\sward_ui_lab_live`
- Enabled by default for UI Lab sessions.
- Can be disabled with `--ui-lab-live-bridge=off`.
- Pipe name can be changed with `--ui-lab-live-bridge-name <name>`.

Supported commands:

- `state`
- `events`
- `route <target>`
- `reset`
- `set-global <name> <0|1>`
- `capture`
- `help`

The bridge exposes:

- current target
- route/event latch
- title/menu/loading/stage/HUD readiness
- CSD project and stage pointers already known to the UI Lab
- SGlobals values and addresses
- debug-menu fork-derived typed fields
- recent JSONL events
- command capabilities

This does not replace evidence. It makes the real runtime easier to interrogate while native BMPs and JSONL remain the oracle.

## Verification

Local-only evidence, not committed:

- `out/ui_lab_runtime_evidence/phase116_bridge_20260427_213626/`
  - launched the real UI Lab runtime
  - connected to `\\.\pipe\sward_ui_lab_live`
  - sent `state`
  - saved `bridge_state_response.json`
  - response reported `target=title-loop`, bridge started, `7` bridge commands, `15` debug-menu fork-derived typed fields, and recent JSONL events

- `out/ui_lab_runtime_evidence/20260427_213715/`
  - full early-game RGB-gated native capture passed after the bridge landed
  - `title-loop`, `title-menu`, `title-options`, `loading`, and `sonic-hud` all produced evidence-ready manifests
  - all targets produced `liveBridgeName=sward_ui_lab_live`
  - all targets produced `ui_lab_live_state.json`
  - selected native routes stayed target-aware: `title menu visual ready`, `loading display active`, and `stage target ready`

- `out/ui_lab_runtime_evidence/20260427_214440/`
  - post-rebuild title-loop native capture passed after the Live API window also displayed the bridge command list and fork-derived field count
  - `3 / 3` native BMP captures were RGB-nonblack
  - manifest reported `liveBridgeName=sward_ui_lab_live` and `bestRoute=loading display ended`
