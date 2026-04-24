<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Gameplay HUD Proxy Preview Binding

Phase 53 tightens the first visual gameplay-HUD pass in the native GUI workbench:

```text
b/rr53/sward_ui_runtime_debug_gui.exe
```

The important distinction is evidence quality. The exact loose Sonic/Werehog gameplay HUD layout packages are still not recovered as atlas-backed local sheets, but `ExStageTails_Common/ui_prov_playscreen.yncp` is recovered and shares the counter/gauge vocabulary needed for useful stage-HUD preview work.

## What Changed

- added explicit gameplay-HUD atlas candidates for:
  - `sonic_stage_hud_reference.json`
  - `werehog_stage_hud_reference.json`
- bound both of those families to `exstagetails_common__ui_prov_playscreen.png` as a marked `proxy`, not as exact layout parity
- moved `extra_stage_hud_reference.json` onto the same recovered `ui_prov_playscreen` play-screen sheet
- updated the GUI details/footer so proxy atlas usage is visible to the operator
- replaced the simple diagonal layer stack with role-aware bounded layer placement for counters, gauges, sidecars, transient effects, and prompt strips
- increased the preview panel height budget so local atlas sheets stay readable in a normal desktop-sized workbench window
- extended `--preview-smoke` to report total atlas candidates and proxy candidates

Verified preview smoke:

```text
sward_ui_runtime_debug_gui preview smoke ok atlas_candidates=10 proxy_candidates=2 existing_local_atlas=10 title=mainmenu__ui_mainmenu.png pause=systemcommoncore__ui_pause.png sonic_stage=exstagetails_common__ui_prov_playscreen.png
```

## Phase 60/61 Follow-On

Phase 60 keeps the same proxy boundary, but the GUI no longer treats the gameplay HUD atlas as only a backdrop. Phase 61 audits the scene ownership labels against parsed `ui_prov_playscreen` facts. Sonic, Werehog, and Extra Stage HUD previews now draw six recovered scene primitives each:

- `Root/so_speed_gauge`
- `Root/so_ringenagy_gauge`
- `Root/info_1`
- `Root/info_2`
- `Root/ring_get_effect`
- `Root/bg`

The primitive pass represents `680` parsed keyframes per gameplay HUD contract and is verified by:

```text
sward_ui_runtime_debug_gui layout primitive smoke ok title_primitives=6 keyframes=806 pause_primitives=6 keyframes=806 loading_primitives=6 keyframes=2775 sonic_stage_primitives=6 keyframes=680 werehog_stage_primitives=6 keyframes=680 extra_stage_primitives=6 keyframes=680 sonic_speed_gauge_kf=360 sonic_ring_energy_gauge_kf=240 sonic_ring_get_effect_kf=14 sonic_bg_kf=0
```

## Evidence Boundary

Direct supporting evidence:

- `research_uiux/GAMEPLAY_HUD_CORE_RECOVERY.md` identifies `ui_playscreen`, `ui_playscreen_ev`, `ui_playscreen_ev_hit`, and `ui_playscreen_su` as real authored gameplay-HUD layout families.
- That same report records that the loose Sonic/Werehog `ui_playscreen*` layout packages have not yet been recovered.
- `research_uiux/CODE_TO_LAYOUT_CORRELATION.md` ties `ui_prov_playscreen` to concrete counter/gauge nodes such as `so_speed_gauge`, `so_ringenagy_gauge`, and `bg`.
- The local visual atlas layer already has `exstagetails_common__ui_prov_playscreen.png`.

So the GUI now has useful gameplay-HUD visual context, but the label remains intentionally conservative: `proxy`.

## Practical Runway

For SGFX/template reuse, the first usable framework slice is close: title/pause/HUD-style screen state, prompt-row, overlay-role, atlas-preview, and transition-band behavior can be shaped into a reusable operator shell over the next few focused beats.

For broad Sonic Unleashed UI/UX reconstruction, the remaining work is longer:

- decode layout node transforms beyond atlas-sheet preview
- add CSD/timeline playback instead of only contract timing bands
- continue PPC-backed host ownership by family
- replace more local-only scaffolds with translated-behavior-backed ownership
- correlate missing `ui_playscreen*` gameplay HUD payloads when recovered
- run parity checks family by family

The current `.exe` is now a real visual debug shell, not just a CLI probe. It is still a diagnostic/reference renderer, not a full 1:1 Sonic Unleashed UI playback engine.
