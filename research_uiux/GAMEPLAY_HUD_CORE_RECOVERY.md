<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Gameplay HUD Core Recovery

Phase 30 closes the largest remaining UI-family gap after the runtime/debug-selector beat: the in-stage HUD core.

> [!IMPORTANT]
> The gameplay HUD core is no longer a blind spot, but it is not uniformly recovered. Right now the evidence splits into two extracted loose-layout families and four hash-driven/source-path-driven families.

## Snapshot

- Systems grouped: `4`
- Layout families grouped: `6`
- Loose extracted layout families: `2`
- Hash-only layout families: `4`
- Direct `CEvilHudGuide` hook/seam anchors: `4`

Machine-readable output:

- `research_uiux/data/gameplay_hud_core_map.json`

Primary evidence roots:

- `UnleashedRecomp/patches/aspect_ratio_patches.cpp`
- `UnleashedRecompLib/config/SWA.toml`
- `research_uiux/source_path_seeds/UI_SOURCE_PATHS_FROM_EXECUTABLE.txt`
- the supplied local root path dump `Match SU OG source code folders and locations.txt`
- extracted loose HUD assets under `extracted_assets/`

## Recovered Systems

### Sonic Stage HUD

Source-path host group:

- `HUD/Sonic/HudSonicStage.cpp`
- `HUD/Sonic/SonicMainDisplay.cpp`
- `Player/Character/Sonic/Hud/SonicHudGuide.cpp`
- `Player/Character/Sonic/Hud/SonicHudHomingAttack.cpp`

Recovered layout family:

- `ui_playscreen`

What is now clear:

- the day-stage HUD owns top-left `player/time/score/exp` counters
- the bottom-left cluster owns the speed/ring-energy/gauge frame/ring counter shell
- the bottom-right cluster owns speed bonus, `u_info`, and medal-get sidecars

Current limitation:

- no loose `ui_playscreen.yncp` has been recovered yet
- this family is currently anchored by `35` readable hash-node entries plus `mat_playscreen*` texture hits rather than a direct layout package

### Werehog Stage HUD

Source-path host group:

- `HUD/Evil/HudEvilStage.cpp`
- `HUD/Evil/EvilMainDisplay.cpp`
- `Player/Character/EvilSonic/Hud/EvilHudGuide.cpp`
- `Player/Character/EvilSonic/Hud/EvilHudTarget.cpp`

Recovered layout families:

- `ui_playscreen_ev`
- `ui_playscreen_ev_hit`

What is now clear:

- the night-stage HUD adds the full unleash/life/shield gauge stack
- the hit-counter family is separate and right-anchored
- chance-attack feedback is part of the same `ui_playscreen_ev_hit` overlay shell

Direct readable hook anchors:

- `EvilHudGuideAllocMidAsmHook`
- `EvilHudGuideUpdateMidAsmHook`
- `0x82448CF0` for `SWA::Player::CEvilHudGuide::CEvilHudGuide`
- `0x82449088` for `SWA::Player::CEvilHudGuide::Update`

Current limitation:

- neither `ui_playscreen_ev` nor `ui_playscreen_ev_hit` has been recovered as a loose `.yncp` yet
- this family is therefore strong on host ownership and node naming, but still weaker on scene/timeline reconstruction than the extracted families

### Extra Stage / Tornado Defense HUD

Source-path host group:

- `ExtraStage/Tails/Hud/HudExQte.cpp`

Recovered layout families:

- `ui_prov_playscreen`
- `ui_qte`

Loose extracted layout files:

- `extracted_assets/phase16_support_archives/ExStageTails_Common/ui_prov_playscreen.yncp`
- `extracted_assets/phase16_support_archives/ExStageTails_Common/ui_qte.yncp`

What is now clear:

- this is the highest-confidence gameplay HUD family in the current set
- `ui_prov_playscreen` owns the extra-stage counter shell and gauge family
- `ui_qte` owns the controller-prompt stream
- the already-existing archaeology/correlation layer for Tornado Defense was correct and is now explicitly part of the gameplay HUD core rather than a side pocket

### Super Sonic / Final HUD Bridge

Source-path host group:

- `Boss/BossHudSuperSonic.cpp`
- `Boss/BossHudVitality.cpp`
- `Boss/BossNamePlate.cpp`

Recovered layout family:

- `ui_playscreen_su`

What is now clear:

- the Super Sonic/final-family HUD exposes dual left-anchored gauges and a footer strip
- the supplied path dump ties that layout family back to explicit boss HUD hosts

Current limitation:

- no loose `ui_playscreen_su` package has been recovered yet
- confidence is therefore lower than the Tornado Defense family and lower than the Werehog helper-hook case

## Cross-Family Findings

1. The gameplay HUD core is now best understood as `4` host-owned systems rather than one monolithic `playscreen` blob.
2. The `ui_playscreen*` families are real authored layout groups, not inferred labels, because the readable hash rules enumerate concrete node hierarchies for counters, gauges, medals, and sidecars.
3. The current extraction gap is asymmetrical:
   - extra-stage HUD is layout-backed
   - main day/night/super families are still mostly source-path-plus-hash-backed
4. The strongest direct bridge between readable code and translated seams in this pass is still `CEvilHudGuide`, not the main stage HUD shell.
5. `mat_playscreen*` is a shared texture family and by itself is not enough to disambiguate day/night/super ownership; host paths and hash groups are what separate those systems.

## What This Unlocks

This pass is enough to start treating the gameplay HUD as a reusable contract family for `1:1`-style template work:

- counter clusters
- dual-corner HUD composition
- helper-guide sidecars
- ring/boost/unleash/life gauge ownership
- right-edge hit-counter overlays
- extra-stage QTE prompt segregation

It is not yet enough to claim a full human-readable source-level reconstruction of every in-stage HUD class.

## Remaining Gap

What still blocks the HUD core from being fully humanized:

- loose extraction of `ui_playscreen`, `ui_playscreen_ev`, `ui_playscreen_ev_hit`, and `ui_playscreen_su`
- broader translated seam naming for the main stage HUD owners beyond `CEvilHudGuide`
- deeper `CSD/*` foundation mapping so the layout/widget layer can be described in reusable scene/widget terms instead of only node hashes

That is the handoff into Phase 31.
