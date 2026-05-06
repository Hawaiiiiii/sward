# Path B runtime proof (Phases 366 -> 367b)

Path B is the fast SGFX shell lane: run Sonic Unleashed UI through
UnleashedRecomp itself, then override assets/text and lock gameplay so
the runtime behaves like a reusable UI prototype engine.

The tracked `UnleashedRecomp/` source contains the runtime hooks
directly. The CMake/ninja build resolves sources from the parallel
build tree at `C:\ur103clean\UnleashedRecomp` (per
`UnleashedRecomp_SOURCE_DIR` in
`C:\ur103clean\b\ui_lab_runtime\CMakeCache.txt`); the Phase 367b build
wrapper [`_phase367_build.bat`](../_phase367_build.bat) runs a
`robocopy /E` repo -> build-tree sync BEFORE invoking ninja so edits
in this repo always reach the build, with `/XD` exclusions for the
SWA `api/` git submodule and the cmake-generated `version.cpp` /
`version.h` pair so they survive the sync untouched.

## Event taxonomy (Phase 367b)

The bridge events are split across distinct, non-overlapping names so
consumers can tell which override path actually fired. Boot-time
markers, runtime application markers, and load-time markers are
disjoint -- earlier phases used a shared `Text:OverrideHit:<key>`
event for both the host-side and guest-side paths and a synthetic
boot-time probe; that conflation is gone in Phase 367b.

| Event | Source | Hook site | Cardinality |
|---|---|---|---|
| `Text:OverridesLoaded:<count>` | text override manifest load | `sg_text_overrides.cpp::DoLoad()` | once per process |
| `Text:HostOverrideHit:<key>` | host-side `Localise()` returned an overridden string | `locale.cpp::Localise()` -> `NoteHostHitForKey()` | once per unique key per process |
| `Text:CsdOverrideHit:<original>` | guest-side CSD `SetText` substituted an overridden literal | `CsdNodeText_patches.cpp::sub_830BF640` -> `TryGetOverrideGuestPtr()` | once per unique original literal per process |
| `Text:CsdSetTextSample:<literal>` | env-gated probe; logs the first 64 unique literals retail SU passes through `sub_830BF640` | `CsdNodeText_patches.cpp::EmitSetTextSampleIfFirst()` | env `SG_PREFLIGHT_LOG_SETTEXT=1`; up to 64 events per process |
| `Asset:VisibleOverrideHit:<guestPath>` | loose-file substitution path; UR's `ResolvePath` returned an SG-Preflight override file for an asset retail UR opens directly | `mod_loader.cpp::ResolvePath()` | once per unique guest path per process |
| `Asset:OverrideHit:<assetName>` | archive-entry substitution path; the `CCreateFromArchive::CreateCallback` resolved a loose override for a `*.ar`-resident entry | `mod_loader.cpp::sub_82E0B500` | once per unique asset name per process |
| `Stage:GameplaySkip` | `CGameModeStage::Update` first tick of a stage instance under `SG_PREFLIGHT_GAMEPLAY_SKIP=1` | `CGameModeStage_patches.cpp::sub_8253B7C0` -> `OnGameplaySkipStageEntered()` | once per stage instance |
| `Input:UiOnlyLock` | gameplay pad mask applied under `SG_PREFLIGHT_UI_ONLY_INPUT=1` | same site -> `OnUiOnlyInputLockApplied()` | once per process |

Bridge files live in `%APPDATA%\UnleashedRecomp\sgfx_bridge`. Override
files live in the directory selected by `SG_PREFLIGHT_OVERRIDE_DIR`.

## What's runtime-proven (Phase 367b, 2026-05-06)

Captured in
[research_uiux/runtime_reference/out/phase367_path_b_proof/events_post_phase367.jsonl](runtime_reference/out/phase367_path_b_proof/events_post_phase367.jsonl)
with summary at
[summary.json](runtime_reference/out/phase367_path_b_proof/summary.json):

```json
{
  "text_overrides_loaded": 1,
  "text_host_override_hits": 2,
  "text_csd_override_hits": 7,
  "asset_visible_override_hits": 1,
  "asset_override_hits": 1,
  "stage_gameplay_skip": 2,
  "input_ui_only_lock": 1,
  "native_frames_written": 1,
  "elapsed_seconds": 61
}
```

- **runtime-proven**: `Text:OverridesLoaded:20` fires once at boot
  from `DoLoad()` after the 20 keys in `sg_text_overrides.json` are
  merged into `g_locale`.
- **runtime-proven**: `Text:HostOverrideHit:Common_Back` and
  `Text:HostOverrideHit:Common_Select` fire when UR's button-guide
  Localise call (`button_guide.cpp:254` and `:281`,
  `Localise(btn.Name)`) resolves the overridden keys for the
  Settings sub-menu the runtime walks through during boot. These are
  REAL UI hits -- there is no boot-time synthetic probe in Phase
  367b.
- **runtime-proven**: `Text:CsdOverrideHit:99`, `Text:CsdOverrideHit:999999`,
  `Text:CsdOverrideHit:[200]`, `Text:CsdOverrideHit:7`,
  `Text:CsdOverrideHit:16`, `Text:CsdOverrideHit:35`, and
  `Text:CsdOverrideHit:30` fire when retail SU's gameplay HUD CSD
  scenes call `sub_830BF640::SetText` with each of those baked
  defaults during the auto-loaded Empire City stage. The acceptance
  gate is intentionally `>= 1` because the exact set can vary with how
  far the plain retail route advances during the timed proof window,
  but the default 60-second run captured all seven.
- **runtime-proven**: `Asset:VisibleOverrideHit:game:/Loading/logo_sonicteam.dds`
  fires once when the loading screen requests the SonicTeam logo DDS
  via `ModLoader::ResolvePath`; the SG-Preflight override mod's
  `<override>/Loading/logo_sonicteam.dds` (byte-identical to retail,
  md5 = `143b371cbceae7619a9053097f0c61ed`) wins resolution. This is
  the canonical visible-asset lane (the older `Asset:OverrideHit`
  also fires for the same file via the archive-entry callback in
  `sub_82E0B500`, both events are recorded; the visible-asset
  variant is the new Phase 367b emit and is what the proof's
  acceptance test gates on).
- **runtime-proven**: `Stage:GameplaySkip` fires twice -- once at the
  title intro stage entry and once at the Empire City hub stage
  entry; `Input:UiOnlyLock` fires once when the gameplay pad mask is
  first applied. Both prove the Phase 360/363 hooks are wired.
- **runtime-proven**: a 10 MB BMP screen capture at
  [native_frame_phase367_screen_grab.bmp](runtime_reference/out/phase367_path_b_proof/native_frame_phase367_screen_grab.bmp)
  was written immediately after the first `Asset:VisibleOverrideHit`
  by the proof script's `System.Drawing.Graphics.CopyFromScreen`
  call. The script chose the screen-grab path over UR's built-in
  `--ui-lab-native-capture-*` because every observed `--ui-lab*`
  invocation in this environment triggers UR to exit at 6-13 s
  (well under the configured 60 s auto-exit) before the runtime has
  rendered anything substantial; running plain UR + driving capture
  from the script side avoids that. The script fails with exit 6 if
  zero BMPs land in the evidence dir.

## Acceptance gating

The proof script
[`research_uiux/runtime_reference/tools/phase367_path_b_proof.ps1`](runtime_reference/tools/phase367_path_b_proof.ps1)
fails explicitly with a distinct exit code per missing-event reason:

| Exit | Reason |
|---|---|
| 0 | all gates passed |
| 2 | build / deploy / launch failed |
| 3 | `Text:OverridesLoaded` missing (text override manifest never loaded) |
| 4 | `Text:CsdOverrideHit` missing (guest CSD SetText override never fired) |
| 5 | `Asset:VisibleOverrideHit` missing (no loose-file override applied via `ResolvePath`) |
| 6 | no native frame BMP written to the evidence dir |

`Text:HostOverrideHit` is reported as informational and is not gating
because the host-side path requires UR's installer-wizard / options
menu / message windows to fire, which is timing-dependent on what
sub-menus the runtime walks through during the test window.

## Fresh verification command

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
    -File research_uiux\runtime_reference\tools\phase367_path_b_proof.ps1 `
    -AutoExitSeconds 60 `
    -NativeCaptureCount 1
```

The script:

1. Calls [`_phase367_build.bat`](../_phase367_build.bat) which
   `robocopy /E`'s the repo's `UnleashedRecomp/` tree into
   `C:\ur103clean\UnleashedRecomp\` (skipping the SWA `api/` submodule,
   the `res/` sub-tree where the cmake-generated version files live,
   and any `.git` / build intermediates) THEN runs
   `ninja UnleashedRecomp` against the build tree at
   `C:\ur103clean\b\ui_lab_runtime`. This guarantees that any edit
   under the tracked repo's `UnleashedRecomp/` is picked up by the
   build, addressing the original Phase 367 review note about source
   sync.
2. Stages a clean override pack at
   `%LOCALAPPDATA%\UnleashedRecomp\sg_overrides_phase367\`:
   - `sg_text_overrides.json` -- 20 keys spanning g_locale-known
     keys (`Common_Select`, `Common_Cancel`, `Common_Back`,
     `Common_Yes`, `Common_No`), Title menu CSD literal
     candidates, and the seven runtime-proven HUD digit literals
     (`99`, `999999`, `[200]`, `7`, `16`, `35`, `30`).
   - `Loading/logo_sonicteam.dds` -- byte-copied from the retail
     install dir (`game/Loading/logo_sonicteam.dds`); the script
     verifies MD5 parity before launching so the override is
     guaranteed safe.
3. Deploys the freshly-built UnleashedRecomp.exe + dxcompiler.dll +
   dxil.dll trio to the Complete Installation 1.0.3 dir.
4. Resets `events.jsonl` (after backing up the pre-run copy as
   `events_pre_phase367.jsonl`).
5. Launches `UnleashedRecomp.exe` PLAIN (no `--ui-lab*` flags) under:
   - `SG_PREFLIGHT_OVERRIDE_DIR` = staged pack
   - `SG_PREFLIGHT_GAMEPLAY_SKIP=1`
   - `SG_PREFLIGHT_UI_ONLY_INPUT=1`
   - `SG_PREFLIGHT_LOG_LOADS=1`
   - `SG_PREFLIGHT_LOG_SETTEXT=1` (probe; bounded to 64 unique
     literals per process via the dedup set in
     `EmitSetTextSampleIfFirst`)
6. Polls `events.jsonl` every 2 s; the moment
   `Asset:VisibleOverrideHit` appears it grabs a `System.Drawing`
   screen-shot of the primary monitor into the evidence dir as
   `native_frame_phase367_screen_grab.bmp`.
7. After `AutoExitSeconds` (default 60), terminates UR via
   `Process.Kill()`.
8. Tallies all event prefixes, writes `summary.json`, then runs the
   acceptance gates and exits with the appropriate code.

The optional `SG_PREFLIGHT_NO_AUTOLOAD=1` env (also Phase 367b;
implemented in `mod_loader.cpp::ResolvePath` for the `save:` root)
forces retail SU's title menu to show the New Save row instead of an
auto-resumable Continue, which can be useful when iterating on
Title-menu CSD literals. The default proof run leaves it OFF because
the auto-loaded Empire City flow reaches the gameplay HUD CSDs that
fire `Text:CsdOverrideHit` for the staged number literals.

## File layout (Phase 367b)

| Path | Phase | Purpose |
|---|---|---|
| [`UnleashedRecomp/locale/locale.cpp`](../UnleashedRecomp/locale/locale.cpp) | 367 | `Localise()` calls `NoteHostHitForKey(key)` once per key the first time it returns an overridden string -> emits `Text:HostOverrideHit:<key>`. |
| [`UnleashedRecomp/patches/sg_text_overrides.h`](../UnleashedRecomp/patches/sg_text_overrides.h) | 367b | Public `NoteHostHitForKey` declaration (renamed/clarified from Phase 367's confused dual-purpose name). |
| [`UnleashedRecomp/patches/sg_text_overrides.cpp`](../UnleashedRecomp/patches/sg_text_overrides.cpp) | 367b | `g_hostHitSet` dedup; `TryGetOverrideGuestPtr` emits `Text:CsdOverrideHit:<original>` (renamed from the conflated `Text:OverrideHit`); `NoteHostHitForKey` emits `Text:HostOverrideHit:<key>`. The Phase 367 boot-time synthetic probe inside `DoLoad()` is GONE. |
| [`UnleashedRecomp/patches/CsdNodeText_patches.cpp`](../UnleashedRecomp/patches/CsdNodeText_patches.cpp) | 367b | `EmitSetTextSampleIfFirst` env-gated probe captures up to 64 unique literals as `Text:CsdSetTextSample:<literal>` per process. |
| [`UnleashedRecomp/mod/mod_loader.cpp`](../UnleashedRecomp/mod/mod_loader.cpp) | 367b | `ResolvePath` emits `Asset:VisibleOverrideHit:<guestPath>` once per unique path when an SG-Preflight override file wins resolution. Also implements `SG_PREFLIGHT_NO_AUTOLOAD` (returns `{}` for `save:` roots). The pre-existing archive-entry path in `sub_82E0B500` still emits the older `Asset:OverrideHit:<assetName>`. |
| [`research_uiux/runtime_reference/tools/phase367_path_b_proof.ps1`](runtime_reference/tools/phase367_path_b_proof.ps1) | 367b | End-to-end build-sync + deploy + launch + polling-screen-capture + summarise + acceptance-gate runner. |
| [`_phase367_build.bat`](../_phase367_build.bat) | 367b | Source-sync wrapper (robocopy `/E` repo -> build tree, exclude `api/`/`res/`/version files), then `vcvars64` + `ninja UnleashedRecomp`. |
| [`UnleashedRecomp/patches/CGameModeStage_patches.cpp`](../UnleashedRecomp/patches/CGameModeStage_patches.cpp) | 360 / 363 | `Stage:GameplaySkip` and `Input:UiOnlyLock` emits unchanged. |
