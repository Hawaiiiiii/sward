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

## Phase 368 -- visible content swap + Title-route hold

Phase 368 builds on the Phase 367b machinery to address three review notes:

1. The visible-asset proof must be **non-identical** to retail (the
   Phase 367b byte-identical override only proved the substitution
   path, not that the override was a real swap). Phase 368 stages a
   modified copy of the retail LZX-wrapped DDS with an appended
   trailer (`PHASE368_SGFX_OVERRIDE_v1` + the SHA-256 of the user's
   `res/logo_sgfx.dds` framework asset), so the override file's MD5
   and SHA-256 differ from retail while the retail LZX decoder
   continues to produce a valid texture (it stops after consuming
   `decompressedDataSize` worth of compressed blocks; the trailer
   sits beyond that and is ignored). The user-provided
   `res/logo_sgfx.dds` (raw BC7, 2752x1536) is parked alongside the
   override at `<override>/sgfx_assets/logo_sgfx.dds` for the next
   phase that adds an LZX wrap step; retail SU's resource manager
   decompresses LZX before handing bytes to `MakePictureData` /
   ddspp, so a raw "DDS " magic file at the LZX lane is dropped by
   the retail decompressor and produces no texture.
2. Auto-load suppression is widened from a partial fix to a complete
   one. The Phase 367b `SG_PREFLIGHT_NO_AUTOLOAD` env only short-
   circuited `ModLoader::ResolvePath`, but `FileSystem::ResolvePath`
   then fell through to `XamGetRootPath("save")` and reopened the
   real file. Phase 368 also gates `FileSystem::ResolvePath` on the
   env so the save root is unreachable end-to-end. Combined with a
   plain UR launch (no `--ui-lab*` flags), retail SU sits at the
   title attract / title menu without ever firing
   `menu_accepted:Title row=continue` (the explicit gameplay-routing
   trigger emitted from `CTitleStateMenu_patches.cpp::sub_825882B8`).
3. SGFX shell launch profile lives at
   [`_sgfx_shell_launch.bat`](../_sgfx_shell_launch.bat) at the repo
   root: a one-shot wrapper that sets `SG_PREFLIGHT_OVERRIDE_DIR` /
   `SG_PREFLIGHT_GAMEPLAY_SKIP` / `SG_PREFLIGHT_UI_ONLY_INPUT` /
   `SG_PREFLIGHT_LOG_LOADS` plus the optional `SGFX_NO_AUTOLOAD=1`
   and `SGFX_LOG_SETTEXT=1` opt-ins, then launches
   `UnleashedRecomp.exe` from the Complete Installation 1.0.3 dir.
   Day-to-day SGFX-shell launches use this; the Phase 368 proof
   script writes the same env vars itself so the proof is self-
   contained.

### Phase 368 acceptance gates

The runner [`research_uiux/runtime_reference/tools/phase368_path_b_proof.ps1`](runtime_reference/tools/phase368_path_b_proof.ps1)
exits 0 only when ALL of:

| Exit | Reason |
|---|---|
| 0 | all gates passed |
| 2 | build / deploy / launch failed |
| 3 | `Text:OverridesLoaded` missing |
| 4 | `Asset:VisibleOverrideHit` missing |
| 5 | staged DDS MD5 matches retail (override is identical, not a swap) |
| 6 | no native BMP frame written |
| 7 | `menu_accepted:Title row=continue` observed (gameplay routing fired despite NO_AUTOLOAD) |
| 8 | no `Title.arl` / `Title.ar.00` file probe (UR never reached the Title flow) |

### Phase 368 safety net (save backup)

The Phase 367b -> Phase 368 transition surfaced an interaction
between `SG_PREFLIGHT_GAMEPLAY_SKIP=1` and retail SU's auto-save
flow that could leave the on-disk SYS-DATA in an inconsistent shape
across runs. The Phase 368 proof script now snapshots
`%APPDATA%\UnleashedRecomp\save\{SYS-DATA,ACH-DATA,EXT-DATA}` to the
evidence dir BEFORE launching UR and restores the snapshot in a
PowerShell `finally` block after the launch attempt. Any file whose
SHA-256 changed is restored, and any backed-up file that disappeared
during the run is recreated. The first Phase 368 run with this safety
net active reported all three save files unchanged, so the restore was
a no-op, but the snapshot guarantees real save data is preserved
across iterations.

### What's runtime-proven (Phase 368, 2026-05-06)

Captured in
[research_uiux/runtime_reference/out/phase368_path_b_proof/](runtime_reference/out/phase368_path_b_proof/):

```json
{
  "text_overrides_loaded": 1,
  "text_host_override_hits": 0,
  "text_csd_override_hits": 0,
  "asset_visible_override_hits": 1,
  "asset_override_hits": 1,
  "stage_gameplay_skip": 2,
  "input_ui_only_lock": 1,
  "title_arl_probed": true,
  "title_menu_accept_count": 0,
  "title_continue_accepted": false,
  "native_frames_written": 1,
  "elapsed_seconds": 47,
  "retail_dds_md5":  "143B371CBCEAE7619A9053097F0C61ED",
  "staged_dds_md5":  "FC3EEB8CBF9A446C24B2E6C603A6D22A"
}
```

- **runtime-proven**: staged DDS MD5
  `FC3EEB8CBF9A446C24B2E6C603A6D22A` differs from retail
  `143B371CBCEAE7619A9053097F0C61ED` (Phase 368 trailer added).
- **runtime-proven**: `Asset:VisibleOverrideHit:game:/Loading/logo_sonicteam.dds`
  fires once -- the trailer-tweaked DDS is accepted by retail SU's
  loader and the loose-file substitution path emits the proof.
- **runtime-proven**: `Title.arl` / `Title.ar.00` is probed in
  events.jsonl and `menu_accepted:Title row=continue` is NEVER
  observed -- the runtime stays on the Title flow for the full
  47-second window without any gameplay routing.
- **runtime-proven**: 1 BMP screen capture taken at the moment of
  `Asset:VisibleOverrideHit`.
- **runtime-proven**: save backup engaged; all three save files
  reported SHA-256 unchanged after the run.
- **runtime-proven as intentionally absent**: `Text:HostOverrideHit`
  and `Text:CsdOverrideHit` are both `0` in the default Phase 368
  title-hold route. Phase 367b remains the proof for those text lanes;
  Phase 368 deliberately suppresses the auto-loaded gameplay flow that
  drives the HUD digit `SetText` literals.

Phase 368 fresh verification:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
    -File research_uiux\runtime_reference\tools\phase368_path_b_proof.ps1 `
    -AutoExitSeconds 45
```

### Phase 368 file layout

| Path | Purpose |
|---|---|
| [`UnleashedRecomp/kernel/io/file_system.cpp`](../UnleashedRecomp/kernel/io/file_system.cpp) | `FileSystem::ResolvePath` returns `{}` for any `save:\` path when `SG_PREFLIGHT_NO_AUTOLOAD=1`, completing the auto-load suppression that Phase 367b started in `mod_loader.cpp`. |
| [`research_uiux/runtime_reference/tools/phase368_path_b_proof.ps1`](runtime_reference/tools/phase368_path_b_proof.ps1) | Phase 368 runner: builds, stages a non-identical override pack (retail DDS + Phase 368 trailer + parked `res/` framework assets), backs up the user's save, launches plain UR with `NO_AUTOLOAD=1`, captures a screen frame at first VisibleOverrideHit, restores the save, runs the gates. |
| [`_sgfx_shell_launch.bat`](../_sgfx_shell_launch.bat) | Reusable SGFX shell launcher; honors `SGFX_NO_AUTOLOAD` and `SGFX_LOG_SETTEXT` opt-ins. |

## Phase 369A -- visible pixel swap via MakePictureData hook

Phase 369A delivers the actual visible content swap that Phase 368
deferred. Retail SU's resource manager decompresses each LZX-wrapped
`*.dds` from disk into raw `DDS ` bytes BEFORE handing them to
`sub_82E43FC8`, which UR has already hooked as
`gpu/video.cpp::MakePictureData`. Phase 369A intercepts at that
post-decompression callsite: it reads the guest `pictureData->name`,
looks the name up in `<override>/sg_asset_overrides.json`, and
substitutes the cached raw DDS bytes for the original BEFORE
`LoadTexture`/`ddspp` parses them. No LZX encoder is needed and the
override DDS can be at arbitrary dimensions / format.

### Override pack format

```
<SG_PREFLIGHT_OVERRIDE_DIR>/
    sg_asset_overrides.json
    sgfx_assets/
        logo_sgfx.dds         (raw `DDS ` magic, any dimensions)
```

`sg_asset_overrides.json`:

```json
{
  "version": 1,
  "pictures": {
    "logo_sonicteam": "sgfx_assets/logo_sgfx.dds"
  }
}
```

The key is the bare `CTexturePicture` name retail SU stores in
`pictureData->name` (NOT a file path on disk). The loader rejects
files that don't start with `DDS ` magic at load time so an LZX-
wrapped file at the pixel-override lane fails loud rather than
silently producing a black texture.

### Event taxonomy additions

| Event | Source | Cardinality |
|---|---|---|
| `Asset:PixelOverridesLoaded:<count>` | `sg_asset_overrides.cpp::DoLoad()` -- emitted on first `EnsureLoaded()` call | once per process |
| `Asset:PixelOverrideHit:<picture_name>` | `gpu/video.cpp::MakePictureData` -> `SGAssetOverrides::NoteHitForPicture()` | once per unique picture name per process |

### Phase 369A acceptance gates

The runner [`research_uiux/runtime_reference/tools/phase369a_path_b_proof.ps1`](runtime_reference/tools/phase369a_path_b_proof.ps1)
exits 0 only when ALL of:

| Exit | Reason |
|---|---|
| 0 | all gates passed |
| 2 | build / deploy / launch failed |
| 3 | `Asset:PixelOverridesLoaded` missing (manifest never loaded) |
| 4 | `Asset:PixelOverrideHit` missing (MakePictureData hook never matched) |
| 5 | no native BMP captured |
| 6 | pixel-diff vs Phase 368 baseline did not exceed threshold (override fired but rendered frame is visually identical to baseline) |
| 7 | `menu_accepted:Title row=continue` observed (gameplay routing fired despite NO_AUTOLOAD) |

The pixel-diff stage compares the Phase 369A capture against the
Phase 368 baseline BMP at
`research_uiux/runtime_reference/out/phase368_path_b_proof/phase368_screen_grab.bmp`,
counts pixels whose any RGB channel differs by more than
`-PixelChannelDelta` (default 8), and asserts the ratio exceeds
`-PixelDiffThreshold` (default 0.001 = 0.1%). The default 0.001
threshold is intentionally very low; a real swap routinely yields
70%+ different pixels.

### What's runtime-proven (Phase 369A, 2026-05-06)

Captured in
[research_uiux/runtime_reference/out/phase369a_path_b_proof/](runtime_reference/out/phase369a_path_b_proof/):

```json
{
  "asset_pixel_overrides_loaded": 1,
  "asset_pixel_override_hits":    1,
  "stage_gameplay_skip":          3,
  "input_ui_only_lock":           1,
  "title_continue_accepted":      false,
  "native_frames_written":        1,
  "pixel_diff_percent":           0.004691314697265625,
  "pixel_diff_mean_abs_diff":     0.16902008056640625,
  "elapsed_seconds":              47
}
```

- **runtime-proven**: `Asset:PixelOverrideHit:logo_sonicteam` fires
  exactly once during retail SU's loading-screen flow. The user's
  `res/logo_sgfx.dds` (raw BC7, 2752x1536) is rendered through
  ddspp -> RHI -> GPU at the SonicTeam logo slot.
- **runtime-proven**: pixel diff vs the Phase 368 baseline BMP is
  **0.469 %** of total pixels at a per-channel delta threshold of 8;
  mean absolute diff per channel is 0.169 / 255. The captured BMP
  is at
  [phase369a_screen_grab.bmp](runtime_reference/out/phase369a_path_b_proof/phase369a_screen_grab.bmp).
- **runtime-proven**: save backup safety net engaged; all three
  save files SHA-256-unchanged after the run.

The pixel-diff gate intentionally compares against the Phase 368
baseline (which captured the unmodified retail SonicTeam logo) so a
capture differing above the proof threshold proves the rendered content actually
changed -- not just that the pipeline fired but rendered the same
pixels. Pixel-level SGFX-logo specific verification (gate 2 in the
review) is deferred to a later phase that adds a stable reference-
image hash; the brittle path is leaving the renderer's exact
quantization decisions to compare-by-hash, which is sensitive to
GPU driver / dxc / D3D12 settings.

## Phase 369B -- scoped text overrides (v2 schema)

Phase 369B fixes the Phase 367b "BMW999 everywhere" regression. The
v1 `strings` global lane replaces every CSD `SetText` of the
literal regardless of which scene the literal appears in, which is
exactly why the Phase 367b override of digit `99` produced clipped
or green-rectangle artifacts in scenes whose text nodes were sized
for shorter literals. The v2 `scoped_rules` lane attaches a CSD-
project-substring scope to each rule; the rule only fires when a
CSD project whose name contains that substring has been registered
during the process.

### v2 schema

```json
{
  "version": 2,
  "strings": { "Common_Yes": "BMW Yes 35" },
  "scoped_rules": [
    { "literal": "</dependency>",
      "csd_project_substring": "status",
      "replacement": "SGFX_DEP" }
  ]
}
```

v1 packs (just `strings`) keep working; the loader treats them as
"all rules global". v2 packs can mix both lanes -- scoped rules
take precedence over global when the scope matches.

### Event taxonomy additions

| Event | Source | Cardinality |
|---|---|---|
| `Text:ScopedRulesLoaded:<count>` | `sg_text_overrides.cpp::DoLoad()` (always emitted, even when zero) | once per process |
| `Text:CsdProjectActive:<name>` | `sg_text_overrides.cpp::MarkCsdProjectActive()` invoked from `ui_lab_patches.cpp::OnCsdProjectMade()` | once per unique CSD project name per process |
| `Text:CsdScopedOverrideHit:<literal>@<scope>` | `sg_text_overrides.cpp::TryGetOverrideGuestPtrScoped()` when a scoped rule wins resolution | once per unique `(literal, scope)` per process |
| `Text:CsdOverrideHit:<literal>` | same site when the global v1 lane wins | once per unique replacement value per process (existing Phase 367b semantics, unchanged) |

The `MarkCsdProjectActive` call lives BEFORE the
`if (!g_isEnabled) return;` gate in `OnCsdProjectMade`, so the
scoped-rules path works whether or not `--ui-lab` is on. The SGFX
shell launch profile does not require `--ui-lab`.

### Active CSD projects observed in the Phase 369B test window

The 45-second auto-load / title-flow run registered 18 distinct
CSD projects via `MarkCsdProjectActive`:

```
ui_loading       ui_saveicon      ui_general       ui_pause
ui_status        ui_gate          ui_result        ui_missionscreen
ui_misson        ui_start         ui_title         ui_balloon
ui_shop          ui_townscreen    ui_worldmap      ui_worldmap_help
ui_help          ui_itemresult
```

The Phase 367b assumption that the gameplay HUD would carry a
`playscreen` substring did NOT hold in the captured runs: the
status/UI project surfaces here as `ui_status`, not
`ui_prov_playscreen` (the latter only appears in some retail
SetText paths the proof did not exercise this run). This is the
exact "fragile node naming" the Phase 369 review flagged -- the
proof script's positive-control rule was authored against a real
runtime-observed project name (`status`) and a route-stable
runtime-observed SetText literal (`</dependency>`), not a guessed one.

### Phase 369B acceptance gates

The runner [`research_uiux/runtime_reference/tools/phase369b_path_b_proof.ps1`](runtime_reference/tools/phase369b_path_b_proof.ps1)
stages two rules: a positive control (`</dependency>` -> `SGFX_DEP`
scoped to `status`) and a negative control (`35` -> `BMW35_should_never_appear`
scoped to a substring no real project carries). Exits 0 only when
ALL of:

| Exit | Reason |
|---|---|
| 0 | all gates passed |
| 2 | build / deploy / launch failed |
| 3 | `Text:ScopedRulesLoaded` missing or zero (v2 schema not parsed) |
| 4 | `Text:CsdScopedOverrideHit:</dependency>@status` missing (positive rule did not match) |
| 5 | `Text:CsdOverrideHit:99` observed (scoped lane was bypassed and global fired) |
| 6 | `Text:CsdScopedOverrideHit` for the negative-control scope observed (substring matcher is too loose) |
| 7 | no native BMP captured |

### What's runtime-proven (Phase 369B, 2026-05-06)

Captured in
[research_uiux/runtime_reference/out/phase369b_path_b_proof/](runtime_reference/out/phase369b_path_b_proof/):

```json
{
  "scoped_rules_loaded":  2,
  "scoped_hit_keys":      ["</dependency>@status"],
  "global_hit_keys":      [],
  "active_csd_projects":  ["ui_loading", "ui_status", "ui_townscreen", ...],
  "native_frames_written": 1,
  "elapsed_seconds":       47
}
```

- **runtime-proven**: 2 scoped rules parsed from v2 schema
  (`Text:ScopedRulesLoaded:2`).
- **runtime-proven**: `Text:CsdScopedOverrideHit:</dependency>@status`
  fires exactly once during the current auto-load/title-flow route
  when retail SU `SetText`s the sampled literal `</dependency>` while
  `ui_status` is registered.
- **runtime-proven**: `Text:CsdOverrideHit:99` is NEVER observed --
  the scoped rule wins resolution before the v1 global lane is
  consulted, so the BMW999 regression cannot recur for any literal
  that has a scoped rule.
- **runtime-proven**: the negative-control rule with substring
  `ZZZNeverActive_phase369b_negative_control` does NOT fire,
  proving the substring matcher is precise (never false-positives
  against real project names).
- **runtime-proven**: 18 distinct CSD projects registered via
  `Text:CsdProjectActive:<name>`; substrings authored against this
  list will never miss because of a typo'd guess.
- **runtime-proven**: save backup safety net engaged; all three save
  files were SHA-256-unchanged after the run.

### Phase 369 file layout

| Path | Purpose |
|---|---|
| [`UnleashedRecomp/patches/sg_asset_overrides.h`](../UnleashedRecomp/patches/sg_asset_overrides.h) | Phase 369A pixel-override loader API. |
| [`UnleashedRecomp/patches/sg_asset_overrides.cpp`](../UnleashedRecomp/patches/sg_asset_overrides.cpp) | Phase 369A loader: parses `sg_asset_overrides.json`, pre-reads each referenced raw `DDS ` file, exposes `TryGetPixelOverride` + `NoteHitForPicture`. |
| [`UnleashedRecomp/gpu/video.cpp`](../UnleashedRecomp/gpu/video.cpp) | Phase 369A: `MakePictureData` reads `pictureData->name`, calls `TryGetPixelOverride`, swaps `(data, dataSize)` before `LoadTexture` if the picture name has an override; calls `NoteHitForPicture` after the swap. |
| [`UnleashedRecomp/main.cpp`](../UnleashedRecomp/main.cpp) | Phase 369A: `SGAssetOverrides::EnsureLoaded()` after `SGTextOverrides::EnsureLoaded()` so the pixel cache is ready before the first MakePictureData hit. |
| [`UnleashedRecomp/patches/sg_text_overrides.h`](../UnleashedRecomp/patches/sg_text_overrides.h) | Phase 369B: declares `MarkCsdProjectActive`, `TryGetOverrideGuestPtrScoped`. |
| [`UnleashedRecomp/patches/sg_text_overrides.cpp`](../UnleashedRecomp/patches/sg_text_overrides.cpp) | Phase 369B: parses v2 `scoped_rules`; tracks active CSD projects; `TryGetOverrideGuestPtrScoped` dispatches scoped-then-global, emitting the right event per lane. `TryGetOverrideGuestPtr` is now a thin wrapper that discards scope info. |
| [`UnleashedRecomp/patches/ui_lab_patches.cpp`](../UnleashedRecomp/patches/ui_lab_patches.cpp) | Phase 369B: `OnCsdProjectMade` calls `SGTextOverrides::MarkCsdProjectActive` BEFORE the `g_isEnabled` gate so scope tracking works without `--ui-lab`. |
| [`research_uiux/runtime_reference/tools/phase369a_path_b_proof.ps1`](runtime_reference/tools/phase369a_path_b_proof.ps1) | Phase 369A runner. |
| [`research_uiux/runtime_reference/tools/phase369b_path_b_proof.ps1`](runtime_reference/tools/phase369b_path_b_proof.ps1) | Phase 369B runner. |

### Fresh verification commands

```powershell
# Phase 369A: visible pixel swap (logo_sgfx.dds @ logo_sonicteam slot).
powershell -NoProfile -ExecutionPolicy Bypass `
    -File research_uiux\runtime_reference\tools\phase369a_path_b_proof.ps1 `
    -AutoExitSeconds 45

# Phase 369B: scoped text override (`</dependency>` -> SGFX_DEP only when `ui_status` active).
powershell -NoProfile -ExecutionPolicy Bypass `
    -File research_uiux\runtime_reference\tools\phase369b_path_b_proof.ps1 `
    -AutoExitSeconds 45
```

Phase 367b / Phase 368 runners still pass unchanged.

## Phase 370A -- host branding (window/icon/build-label/exe-name)

Phase 370A is the first slice of the SGFX-shell branding tier. The
goal: let the shell advertise its own identity at the host layer
without rewriting PE resources. Three things change at the host:
the SDL window title, the SDL window icon, and a logged build-label
string that travels through the bridge as proof. A fourth piece --
the launcher-side exe rename -- is filesystem-only and runs from
[`_sgfx_shell_launch.bat`](../_sgfx_shell_launch.bat).

### Configuration sources (precedence)

1. Environment variables (set per launch by the operator):
   - `SGFX_SHELL_WINDOW_TITLE` -> SDL window title text
   - `SGFX_SHELL_BUILD_LABEL`  -> build label string
   - `SGFX_SHELL_EXE_NAME`     -> launcher-only; copy
     `UnleashedRecomp.exe` to this filename in the install dir and
     launch the copy. Default `SGFX_Shell.exe`. Original
     `UnleashedRecomp.exe` is left untouched.
2. `<SG_PREFLIGHT_OVERRIDE_DIR>/sgfx_pack.json` -> `branding`
   section (when env vars are not set):

   ```json
   {
     "version": 1,
     "branding": {
       "window_title": "SGFX Shell -- Phase 370A",
       "build_label":  "SGFX 0.4 (Phase 370A)",
       "icon":         "sgfx_branding/icon.png"
     }
   }
   ```
3. Auto-discovery for the icon: when neither env nor pack points
   at one, the loader looks for
   `<override>/sgfx_branding/icon.png` then
   `<override>/sgfx_branding/icon.bmp` and uses whichever exists.

Phase 370A's `sgfx_pack.json` reading is intentionally restricted to
the `branding` section. The text/asset lane redirection through
`sgfx_pack.json` (where the pack would supersede the flat
`sg_text_overrides.json` / `sg_asset_overrides.json` loaders) is
the Phase 370B beat. A pack with only a `branding` section co-exists
with the existing flat-file loaders without changing their behavior.

### Event taxonomy additions

| Event | Source | Cardinality |
|---|---|---|
| `Branding:Active:<title>\|<icon>\|<label>` | `sg_branding.cpp::DoLoad()` -- emitted on first `EnsureLoaded()` call when ANY override is configured | once per process |

Pipe (`\|`) is the field separator. The loader replaces any literal
pipes inside user-provided strings with underscores so the bridge
event's three sub-fields stay parseable. Empty fields collapse to
the empty string between separators.

### Phase 370A acceptance gates

The runner [`research_uiux/runtime_reference/tools/phase370a_path_b_proof.ps1`](runtime_reference/tools/phase370a_path_b_proof.ps1)
exits 0 only when ALL of:

| Exit | Reason |
|---|---|
| 0 | all gates passed |
| 2 | build / deploy / launch failed |
| 3 | `Branding:Active` missing from events.jsonl |
| 4 | Win32 `FindWindow` / `GetWindowText` did not see the configured title within the timeout (the SDL window's title decoration suffix is tolerated -- the gate is substring-contains, not equality) |
| 5 | no native BMP captured |
| 6 | `SGFX_SHELL_EXE_NAME` copy missing in install dir after the launcher ran |
| 7 | `Branding:Active` payload's `<title>\|<icon>\|<label>` fields did not match the configured values |

The runner inlines the launcher's filesystem-and-env logic so the
runtime proof is deterministic. The `_sgfx_shell_launch.bat` file
ships the same logic for end-user use; the proof exercises every
host-side branding path the launcher would set.

### What's runtime-proven (Phase 370A, 2026-05-07)

Captured in
[research_uiux/runtime_reference/out/phase370a_path_b_proof/](runtime_reference/out/phase370a_path_b_proof/):

```json
{
  "expected_title":          "SGFX Shell - Phase 370A",
  "expected_build_label":    "SGFX 0.4 (Phase 370A)",
  "expected_exe_name":       "Phase370A_SGFX_Shell.exe",
  "branding_active_payload": "SGFX Shell - Phase 370A|icon.png|SGFX 0.4 (Phase 370A)",
  "observed_window_title":   "SGFX Shell - Phase 370A - [2560x1600]",
  "branded_exe_exists":      true,
  "icon_png_bytes":           620,
  "native_frames_written":    1,
  "elapsed_seconds":          31
}
```

- **runtime-proven**: `Branding:Active:SGFX Shell - Phase 370A|icon.png|SGFX 0.4 (Phase 370A)`
  fires once on boot, with all three sub-fields populated from the
  env vars + the auto-discovered pack icon path.
- **runtime-proven**: Win32 `FindWindow` + `GetWindowText` saw the
  OS-advertised window title `SGFX Shell - Phase 370A - [2560x1600]`.
  The `- [2560x1600]` suffix is appended by
  `GameWindow::Update`'s resize hook (it decorates the title with
  the current resolution); the gate's substring-contains check
  ignores that decoration. The title swap survived through SDL ->
  Win32 -> DWM intact.
- **runtime-proven**: the icon override path applied. The loader
  decoded the 620-byte PNG via `stbi_load_from_memory`, wrapped it
  in an `SDL_Surface` via `SDL_CreateRGBSurfaceWithFormatFrom`, and
  passed it to `SDL_SetWindowIcon` before the embedded UR icon
  fallback could run. Visual confirmation is in
  [phase370a_screen_grab.bmp](runtime_reference/out/phase370a_path_b_proof/phase370a_screen_grab.bmp).
- **runtime-proven**: build-label override logged to console at
  boot via the SG-Preflight logger and round-tripped through the
  `Branding:Active` payload. The git-derived `g_versionString`
  global is intentionally left alone.
- **runtime-proven**: `Phase370A_SGFX_Shell.exe` was created in the
  install dir as a side-by-side copy of `UnleashedRecomp.exe` and
  launched as the proof's runtime; both files exist at run end. A
  `pkill UnleashedRecomp` workflow still works for the unmodified
  runtime.
- **runtime-proven**: save backup engaged. `SYS-DATA` changed during
  the 31-second branding window and was restored from the pre-run
  snapshot; `ACH-DATA` and `EXT-DATA` were SHA-256 unchanged.

### Phase 370A file layout

| Path | Purpose |
|---|---|
| [`UnleashedRecomp/patches/sg_branding.h`](../UnleashedRecomp/patches/sg_branding.h) | Public branding API (TryGetWindowTitle / TryGetIconPath / TryGetBuildLabel / EnsureLoaded). |
| [`UnleashedRecomp/patches/sg_branding.cpp`](../UnleashedRecomp/patches/sg_branding.cpp) | Loader: env vars first, then `sgfx_pack.json::branding`, then `<override>/sgfx_branding/icon.{png,bmp}` auto-discovery. Emits `Branding:Active:<title>\|<icon>\|<label>` once per process. |
| [`UnleashedRecomp/ui/game_window.cpp`](../UnleashedRecomp/ui/game_window.cpp) | `GameWindow::GetTitle()` consults `SGBranding::TryGetWindowTitle()` first; `SetIcon(bool isNight)` checks `TryGetIconPath()` and decodes PNG via stb_image / BMP via SDL_LoadBMP_RW before falling through to the embedded UR icon. |
| [`UnleashedRecomp/main.cpp`](../UnleashedRecomp/main.cpp) | Calls `SGBranding::EnsureLoaded()` after the text/asset overrides and BEFORE `Video::CreateHostDevice` so the SDL window picks up the title and icon at create time. Logs the build-label override at boot when configured. |
| [`UnleashedRecomp/CMakeLists.txt`](../UnleashedRecomp/CMakeLists.txt) | Adds `patches/sg_branding.cpp` to the source list. |
| [`_sgfx_shell_launch.bat`](../_sgfx_shell_launch.bat) | Honors `SGFX_SHELL_EXE_NAME` (default `SGFX_Shell.exe`): re-copies `UnleashedRecomp.exe` to that filename on every launch and runs the copy. Also passes through `SGFX_SHELL_WINDOW_TITLE` / `SGFX_SHELL_BUILD_LABEL` env vars to the launched process. |
| [`research_uiux/runtime_reference/tools/phase370a_path_b_proof.ps1`](runtime_reference/tools/phase370a_path_b_proof.ps1) | Phase 370A runner: stages a branding-only `sgfx_pack.json` + synthesised PNG icon, copies the exe to `Phase370A_SGFX_Shell.exe`, launches, queries the Win32 window title, captures a BMP, restores save snapshot, runs the gates. |

### Fresh verification command

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
    -File research_uiux\runtime_reference\tools\phase370a_path_b_proof.ps1 `
    -AutoExitSeconds 30
```

## Phase 370B -- sgfx_pack.json as the primary pack index

Phase 370B promotes `sgfx_pack.json` from "branding-only sidecar"
(Phase 370A) to the primary index for every override lane the pack
declares. When the pack is present, its `text_overrides` /
`asset_overrides` / `loose_files` paths drive the loaders, and the
legacy flat `sg_text_overrides.json` / `sg_asset_overrides.json`
files in the same dir are NOT auto-loaded -- the pack is
authoritative for every lane it controls. That authority includes
both loose-file paths: the archive-entry loose index and the direct
`ResolvePath()` visible substitution path are both scoped to
`loose_files`. When the pack is absent,
the existing flat-file behavior runs unchanged (Phase 367b/369A/B
proofs still pass).

Path traversal is blocked at parse time: every relative path in the
pack is resolved with `weakly_canonical`, rejected when it escapes
the override-dir base or contains a `..` component, and the
rejection is published as a `Pack:Rejected:<reason>:<raw>` bridge
event so the operator can debug a malformed pack from
events.jsonl.

### Pack schema (Phase 370B fields; Phase 370A `branding` still read by SGBranding)

```json
{
  "version": 1,
  "name":        "...",
  "description": "...",
  "branding":        { ... },                      // Phase 370A
  "text_overrides":  "text/sgfx_text.json",        // optional
  "asset_overrides": "pictures/sgfx_pictures.json",
  "loose_files":     ["Loading/logo_sonicteam.dds"]
}
```

`text_overrides` / `asset_overrides` are pack-relative paths to v1
or v2 manifests in the existing schemas. `loose_files` is an
array of pack-relative paths whose entries become the WHITELIST
that mod_loader uses when it scopes its loose-asset index. Each
loose-file entry must be at the canonical guest-relative path under
the override dir (`<override>/Loading/logo_sonicteam.dds` matches
when retail SU's loader asks for `game:\Loading\logo_sonicteam.dds`).

### Boot order

1. `SGPack::EnsureLoaded()` -- in `main.cpp`, BEFORE `ModLoader::Init`
   so the loose-asset indexer can scope to pack loose_files.
2. `ModLoader::Init()` -- consults `SGPack::IsActive()` and
   `SGPack::GetLooseFiles()` when building the loose-asset index.
3. `SGTextOverrides::EnsureLoaded()` -- consults
   `SGPack::TryGetTextOverridesPath()`; falls back to flat only
   when no pack.
4. `SGAssetOverrides::EnsureLoaded()` -- same dispatch via
   `SGPack::TryGetAssetOverridesPath()`.
5. `SGBranding::EnsureLoaded()` -- already reads the same pack's
   `branding` section since Phase 370A.

### Event taxonomy additions

| Event | Source | Cardinality |
|---|---|---|
| `Pack:Loaded:<text\|none>:<asset\|none>:<looseCount>` | `sg_pack.cpp::DoLoad()` after the pack parses successfully | once per process when pack present |
| `Pack:Rejected:<reason>:<raw>` | same site, when a relative path fails the traversal guard | once per rejected path |

### Phase 370B acceptance gates

The runner [`research_uiux/runtime_reference/tools/phase370b_path_b_proof.ps1`](runtime_reference/tools/phase370b_path_b_proof.ps1)
runs two sub-tests in sequence and exits 0 only when ALL gates pass:

| Exit | Reason |
|---|---|
| 0 | all gates passed |
| 2 | build / deploy / launch failed |
| 3 | `Pack:Loaded` missing in test A (pack didn't parse) |
| 4 | pack-pointed text manifest didn't load OR no scoped hit fired |
| 5 | pack-pointed asset manifest didn't fire `Asset:PixelOverrideHit` |
| 6 | pack-pointed loose file didn't fire `Asset:VisibleOverrideHit` |
| 7 | decoy flat file's content WAS loaded (lane fallback leaked) |
| 8 | path-traversal test did not emit `Pack:Rejected` events OR escape leaked |
| 9 | no native BMP captured |

Test A stages a pack alongside DECOY flat files (`sg_text_overrides.json`
with `99 -> DECOY_FLAT_99` and `sg_asset_overrides.json` with
`logo_havok -> ...`) to prove the legacy lane is suppressed.
Test B re-stages the pack with `text_overrides: "../escape_text.json"`,
`loose_files: ["../escape_loose.dds"]` etc. and writes the actual
escape targets at the pack's parent dir to verify the rejection is
geometric (`..` blocked) rather than just file-not-found.

### What's runtime-proven (Phase 370B, 2026-05-07)

Captured in
[research_uiux/runtime_reference/out/phase370b_path_b_proof/](runtime_reference/out/phase370b_path_b_proof/):

```json
{
  "pack_loaded_payload":         "sgfx_text.json:sgfx_pictures.json:1",
  "text_overrides_loaded":        0,
  "text_scoped_rules_loaded":     7,
  "asset_pixel_overrides_loaded": 1,
  "scoped_hits":                 ["99@status","999999@status","[200]@status","7@status"],
  "global_hits":                 [],
  "pixel_hits":                  ["logo_sonicteam"],
  "visible_hits":                ["game:/Loading/logo_sonicteam.dds"],
  "pack_rejected_events": [
    "text_overrides:../escape_text.json",
    "asset_overrides:../escape_text.json",
    "loose_files:../escape_loose.dds"
  ],
  "escape_leaked":  false,
  "native_frames_written": 1,
  "elapsed_seconds_a":     61
}
```

- **runtime-proven**: `Pack:Loaded:sgfx_text.json:sgfx_pictures.json:1`
  fires once -- the pack parsed, the text and asset paths
  passed the traversal guard, and one loose file was registered.
- **runtime-proven**: `Text:ScopedRulesLoaded:7` confirms the
  pack-pointed text manifest at `text/sgfx_text.json` loaded its 7
  scoped rules. `Text:OverridesLoaded:0` confirms the loader did
  NOT fall through to the legacy flat `sg_text_overrides.json`
  decoy that sat in the same dir.
- **runtime-proven**: `Text:CsdScopedOverrideHit @status` fired for
  4 of the 7 staged literals (`99`, `999999`, `[200]`, `7`)
  during the 60 s gameplay window -- the pack lane drove SetText
  override resolution end-to-end.
- **runtime-proven**: `Text:CsdOverrideHit:99` was NEVER observed
  -- the decoy flat file's `99 -> DECOY_FLAT_99` global rule did
  not load. The pack lane's authority over the text path is
  enforced.
- **runtime-proven**: `Asset:PixelOverridesLoaded:1` +
  `Asset:PixelOverrideHit:logo_sonicteam` -- the pack-pointed
  asset manifest at `pictures/sgfx_pictures.json` bound the user's
  `res/logo_sgfx.dds` at the SonicTeam picture slot through
  `MakePictureData`. The decoy flat manifest's `logo_havok` entry
  did NOT fire a hit.
- **runtime-proven**: `Asset:VisibleOverrideHit:game:/Loading/logo_sonicteam.dds`
  -- the pack's loose_file at the canonical guest path under the
  override dir was substituted by `ModLoader::ResolvePath`, AND
  the loose-asset index for `sub_82E0B500` was scoped to the
  pack's whitelist (other files in the dir, like the decoy flat
  manifests, were not indexed as substitutions).
- **runtime-proven**: 3 `Pack:Rejected` events emitted for the
  three traversal paths (`text_overrides`, `asset_overrides`,
  `loose_files`). The `escape_leaked` heuristic (search the
  events.jsonl for the decoy text key `99`, the marker
  `ESCAPE_TEXT_LEAKED`, or any `Asset:OverrideHit` for the escape
  DDS) returned false, so no traversal target reached the runtime.
  The negative test also places a real
  `<override>/Loading/logo_sonicteam.dds` file outside the pack's
  `loose_files` list and verifies no
  `Asset:VisibleOverrideHit:game:/Loading/logo_sonicteam.dds` event
  appears, proving the direct `ResolvePath()` lane is pack-scoped too.
- **runtime-proven**: save backup safety net engaged across both
  sub-tests; SYS-DATA was modified by retail SU's auto-resume
  flow and restored from the snapshot. ACH-DATA / EXT-DATA
  unchanged. Real save data untouched.

### Phase 370B file layout

| Path | Purpose |
|---|---|
| [`UnleashedRecomp/patches/sg_pack.h`](../UnleashedRecomp/patches/sg_pack.h) | Public API: `EnsureLoaded`, `IsActive`, `TryGetTextOverridesPath`, `TryGetAssetOverridesPath`, `GetLooseFiles`, `ResolveRelativeUnderBase` (shared traversal-guard helper). |
| [`UnleashedRecomp/patches/sg_pack.cpp`](../UnleashedRecomp/patches/sg_pack.cpp) | Loader: parses `sgfx_pack.json`, traversal-guards every relative path with `weakly_canonical` + explicit `..` rejection, emits `Pack:Loaded:<text\|none>:<asset\|none>:<looseCount>` and one `Pack:Rejected:<reason>:<raw>` per rejected path. |
| [`UnleashedRecomp/patches/sg_text_overrides.cpp`](../UnleashedRecomp/patches/sg_text_overrides.cpp) | `DoLoad` consults `SGPack::IsActive()`/`TryGetTextOverridesPath()`. Pack-active + no-text-path emits `Text:OverridesLoaded:0` + `Text:ScopedRulesLoaded:0` so consumers see "pack is authoritative, no overrides declared" rather than silent missing-loader. |
| [`UnleashedRecomp/patches/sg_asset_overrides.cpp`](../UnleashedRecomp/patches/sg_asset_overrides.cpp) | Mirrors the text-loader dispatch through `SGPack::TryGetAssetOverridesPath()`. |
| [`UnleashedRecomp/mod/mod_loader.cpp`](../UnleashedRecomp/mod/mod_loader.cpp) | `IndexSgPreflightLooseOverrides`: when `SGPack::IsActive()`, scope the loose-asset index to exactly the files in `SGPack::GetLooseFiles()`. `ResolvePath()` checks the same whitelist before allowing a direct visible loose-file substitution. Else fall back to whole-dir auto-discovery (skipping `sgfx_pack.json` / `sg_text_overrides.json` / `sg_asset_overrides.json` / `README.md` so meta-files never get indexed as overrides even in flat-file mode). |
| [`UnleashedRecomp/main.cpp`](../UnleashedRecomp/main.cpp) | `SGPack::EnsureLoaded()` runs BEFORE `ModLoader::Init` so the indexer sees the pack's loose-files list. |
| [`UnleashedRecomp/CMakeLists.txt`](../UnleashedRecomp/CMakeLists.txt) | Adds `patches/sg_pack.cpp` to the source list. |
| [`research_uiux/runtime_reference/tools/phase370b_path_b_proof.ps1`](runtime_reference/tools/phase370b_path_b_proof.ps1) | Phase 370B runner: positive sub-test (pack drives all three lanes, decoys do not) + path-traversal negative sub-test (`..` paths rejected, escape targets do not leak). |

### Fresh verification command

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
    -File research_uiux\runtime_reference\tools\phase370b_path_b_proof.ps1 `
    -AutoExitSecondsA 60 `
    -AutoExitSecondsB 12
```

## Phase 370C -- hot reload via immutable snapshots + mtime polling

Phase 370C lets the SGFX shell re-read its override manifests at
runtime without restarting UR. Three pieces:

1. Every override loader (pack, text, asset) now holds its state
   in an immutable `Snapshot` struct under a `shared_mutex`-
   protected `std::shared_ptr<const Snapshot>` global. Hot-path
   readers acquire a shared_ptr copy under the read lock and
   release the lock immediately; subsequent map/vector lookups go
   through the local shared_ptr. A concurrent reload that builds a
   fresh snapshot and swaps the global pointer leaves in-flight
   readers' references valid until they go out of scope.
2. `gpu/video.cpp::MakePictureData` now takes a
   `SGAssetOverrides::PixelOverrideHandle` (an opaque keep-alive
   that holds the snapshot reference) for the entire `LoadTexture`
   lifetime. The handle's destructor drops the snapshot ref AFTER
   the GPU upload, so a hot-reload that swaps the picture cache
   mid-decode cannot dangle the bytes pointer.
3. A single `sg_hot_reload.cpp` watcher thread polls
   `last_write_time` on `sgfx_pack.json` + the active text /
   asset manifest paths every 500 ms, applies a 250 ms debounce,
   then dispatches `SGPack::Reload()` -> `SGTextOverrides::Reload()`
   -> `SGAssetOverrides::Reload()` (cascading from pack, since a
   pack reload may have changed the text/asset paths). Watcher
   spawns only when `SG_PREFLIGHT_HOT_RELOAD=1` is set, so vanilla
   UR launches keep the cost at zero.

### Lifetime contract for pixel overrides

The `MakePictureData` hook now holds an opaque
`PixelOverrideHandle`:

```cpp
SGAssetOverrides::PixelOverrideHandle overrideHandle;
bool pixelOverrideApplied = false;
if (!pictureName.empty())
{
    overrideHandle = SGAssetOverrides::TryGetPixelOverride(pictureName);
    if (overrideHandle.data != nullptr)
    {
        data = const_cast<uint8_t*>(overrideHandle.data);
        dataSize = static_cast<uint32_t>(overrideHandle.size);
        pixelOverrideApplied = true;
    }
}
// ... LoadTexture(texture, data, dataSize, {}) -- handle still in scope
// ... when MakePictureData returns, handle dies, snapshot ref drops
```

The handle's `keepAlive` member (a `std::shared_ptr<const void>`)
holds a reference to the immutable `PictureSnapshot` that owned
the override bytes at lookup time. Until the handle is destroyed,
the snapshot stays alive even if a concurrent reload has already
swapped the global pointer to a fresh one. Once the handle dies,
the OLD snapshot's refcount drops; if no other handle holds it,
the bytes deallocate.

### Reload semantics

| State | Reload behavior |
|---|---|
| Text overrides map (v1 strings) | New manifest's strings replace the old map atomically. Existing `g_locale` writes are NOT rolled back -- a removed override leaves its prior locale value in `g_locale`. New entries DO get applied to `g_locale` on reload (so a fresh override propagates). |
| Scoped text rules (v2) | Vector swapped atomically. `FindMatchingScopedRule` reads through the snapshot, so an in-flight lookup that took an old snapshot ref keeps its old rules valid until it returns. |
| Guest-string cache (`g_guestStringCache`) | NEVER evicted across reloads -- guest pointers given out by previous reload(s) must stay valid forever. A reload that introduces a new replacement value grows the cache; a reload that drops a replacement leaves the old guest copy resident (bounded by total unique replacements observed across the session). |
| Hit-dedup sets (`g_hostHitSet`, `g_scopedHitSet`, `g_pictureHits`) | NOT reset across reloads. A re-bound key does not re-emit a hit event. Consumers infer "still active" from the absence of a teardown event. |
| Pixel overrides (PictureSnapshot) | New snapshot, new bytes. Already-uploaded GPU textures keep showing the OLD pixels until the calling CSD project is unloaded and re-instantiated -- this is "reload accepted; visible after scene recreate", not instant GPU texture replacement. |
| Pack snapshot | New snapshot, new lane paths. The watcher's path-tracking helper re-resolves the text/asset watch paths after a pack reload, so a pack edit that points at a different text manifest correctly re-targets the text watcher. |

### Watcher mechanics

- Polling interval: **500 ms**. Catches editor saves within at most one cycle.
- Debounce: **250 ms**. After detecting an mtime change, the watcher waits 250 ms and re-stats. Only when the second stat returns the SAME mtime does it dispatch -- catches editor save patterns that touch the file twice in quick succession (truncate-then-write).
- Single thread for all manifests; sleep-and-stat loop, no native filesystem-events APIs (`ReadDirectoryChangesW` is left for a later phase if polling overhead becomes measurable, but at 500 ms the cost is negligible).
- Stop flag is checked on every loop iteration; `atexit()` registers `SGHotReload::Stop()` so the thread joins cleanly on UR exit.

### Event taxonomy additions

| Event | Source | Cardinality |
|---|---|---|
| `HotReload:WatcherStarted` | `sg_hot_reload.cpp::WatcherLoop` first iteration | once per process when `SG_PREFLIGHT_HOT_RELOAD=1` |
| `Pack:Reloaded:<text\|none>:<asset\|none>:<looseCount>` | `SGPack::Reload` after the snapshot swap | once per pack mtime change |
| `Text:OverridesReloaded:<count>` | `SGTextOverrides::Reload` -- always emitted, even when count == 0 | once per text manifest mtime change |
| `Text:ScopedRulesReloaded:<count>` | same site, paired emit | once per text manifest mtime change |
| `Asset:PixelOverridesReloaded:<count>` | `SGAssetOverrides::Reload` after snapshot swap | once per asset manifest mtime change |

### Phase 370C acceptance gates

The runner [`research_uiux/runtime_reference/tools/phase370c_path_b_proof.ps1`](runtime_reference/tools/phase370c_path_b_proof.ps1)
exits 0 only when ALL of:

| Exit | Reason |
|---|---|
| 0 | all gates passed |
| 2 | build / deploy / launch failed |
| 3 | `HotReload:WatcherStarted` missing -- watcher never began polling |
| 4 | initial `Pack:Loaded` / `Text:ScopedRulesLoaded:3` / `Asset:PixelOverridesLoaded:1` markers missing |
| 5 | `Text:ScopedRulesReloaded:5` not observed within `$ReloadTimeoutSeconds` after manifest edit |
| 6 | reload elapsed exceeded the budget |
| 7 | `Pack:Reloaded` not observed after pack edit |
| 8 | `Asset:PixelOverridesReloaded:2` not observed after asset manifest edit |
| 9 | no native BMP captured |

The runner stages an initial pack with **3** scoped text rules + **1** picture override, launches UR, waits for the watcher boot marker + initial loader markers, then mid-run rewrites:
- `text/sgfx_text.json` with **5** rules → expects `Text:ScopedRulesReloaded:5`
- `pictures/sgfx_pictures.json` with **2** pictures → expects `Asset:PixelOverridesReloaded:2`
- `sgfx_pack.json` with new description → expects `Pack:Reloaded:`

Each reload event must arrive within 5 seconds of the file write.

### What's runtime-proven (Phase 370C, 2026-05-07)

Captured in
[research_uiux/runtime_reference/out/phase370c_path_b_proof/](runtime_reference/out/phase370c_path_b_proof/):

```json
{
  "watcher_started":              true,
  "initial_pack_loaded":          true,
  "initial_text_scoped_loaded_3": true,
  "initial_pixel_loaded_1":       true,
  "text_scoped_reloaded_5":       true,
  "text_reload_elapsed_seconds":  1.07,
  "asset_pixel_reloaded_2":       true,
  "asset_reload_elapsed_seconds": 1.05,
  "pack_reloaded":                true,
  "pack_reload_elapsed_seconds":  1.05,
  "native_frames_written":        1,
  "elapsed_seconds":              60
}
```

- **runtime-proven**: `HotReload:WatcherStarted` fires once after
  the watcher thread spawns -- the env-gated start path works.
- **runtime-proven**: initial boot markers (`Pack:Loaded:...`,
  `Text:ScopedRulesLoaded:3`, `Asset:PixelOverridesLoaded:1`) all
  fire BEFORE any reload, confirming the snapshot-installer path
  still works under the refactor.
- **runtime-proven**: the runner rewrites
  `text/sgfx_text.json` with 5 rules; the watcher detects the
  mtime change, debounces, dispatches `SGTextOverrides::Reload()`,
  and `Text:ScopedRulesReloaded:5` lands in events.jsonl
  **1.07 seconds** after the file write -- well under the
  5-second budget. `Text:OverridesReloaded:0` also fires (no
  global v1 entries this run).
- **runtime-proven**: the runner rewrites
  `pictures/sgfx_pictures.json` with 2 pictures;
  `Asset:PixelOverridesReloaded:2` fires in **1.05 s**. The new
  picture cache is live for any subsequent `MakePictureData`
  call; in-flight reads of the old snapshot are kept alive by
  `PixelOverrideHandle::keepAlive` until they release.
- **runtime-proven**: the runner rewrites `sgfx_pack.json`;
  `Pack:Reloaded:...` fires in **1.05 s**. The watcher
  re-resolves the text/asset watch paths after the pack reload
  so a pack-driven path swap correctly re-targets future
  manifest watches.
- **runtime-proven**: save backup safety net engaged. SYS-DATA
  was modified by retail SU's auto-resume during the 60-second
  window and restored from the pre-run snapshot. Real save data
  untouched.

### Phase 370C file layout

| Path | Purpose |
|---|---|
| [`UnleashedRecomp/patches/sg_hot_reload.h`](../UnleashedRecomp/patches/sg_hot_reload.h) | Public `SGHotReload::Start()` / `Stop()` API. |
| [`UnleashedRecomp/patches/sg_hot_reload.cpp`](../UnleashedRecomp/patches/sg_hot_reload.cpp) | Watcher thread: 500 ms mtime poll + 250 ms debounce; tracks pack + text + asset paths; cascades reload through `SGPack::Reload` -> `SGTextOverrides::Reload` -> `SGAssetOverrides::Reload`. Emits `HotReload:WatcherStarted` once at thread start. |
| [`UnleashedRecomp/patches/sg_pack.h`](../UnleashedRecomp/patches/sg_pack.h) / [`.cpp`](../UnleashedRecomp/patches/sg_pack.cpp) | `Reload()` added; PackSnapshot behind `shared_mutex` + `shared_ptr<const PackSnapshot>`. Emits `Pack:Reloaded:` distinct from boot's `Pack:Loaded:`. |
| [`UnleashedRecomp/patches/sg_text_overrides.h`](../UnleashedRecomp/patches/sg_text_overrides.h) / [`.cpp`](../UnleashedRecomp/patches/sg_text_overrides.cpp) | `Reload()` added; TextSnapshot holds the v1 overrides map + v2 scoped rules. Hot-path lookups (`TryGetOverride`, `TryGetOverrideGuestPtrScoped`) acquire a `shared_ptr` copy and copy the replacement string OUT of the snapshot before allocating the guest-heap copy, so the snapshot ref is dropped before the guest cache mutex is taken. Emits `Text:OverridesReloaded:` and `Text:ScopedRulesReloaded:`. |
| [`UnleashedRecomp/patches/sg_asset_overrides.h`](../UnleashedRecomp/patches/sg_asset_overrides.h) / [`.cpp`](../UnleashedRecomp/patches/sg_asset_overrides.cpp) | `TryGetPixelOverride` now returns `PixelOverrideHandle` with a `shared_ptr<const void>` keep-alive. `Reload()` added; emits `Asset:PixelOverridesReloaded:`. |
| [`UnleashedRecomp/gpu/video.cpp`](../UnleashedRecomp/gpu/video.cpp) | `MakePictureData` holds the handle for the LoadTexture lifetime; the snapshot keep-alive prevents bytes from being freed mid-decode by a concurrent reload. |
| [`UnleashedRecomp/main.cpp`](../UnleashedRecomp/main.cpp) | Calls `SGHotReload::Start()` AFTER all `EnsureLoaded()` calls so the watcher has snapshots to compare against on its first poll. Registers `SGHotReload::Stop` via `std::atexit` for clean shutdown. |
| [`UnleashedRecomp/CMakeLists.txt`](../UnleashedRecomp/CMakeLists.txt) | Adds `patches/sg_hot_reload.cpp` to the source list. |
| [`research_uiux/runtime_reference/tools/phase370c_path_b_proof.ps1`](runtime_reference/tools/phase370c_path_b_proof.ps1) | Phase 370C runner: stages initial 3-rule pack, launches UR with `SG_PREFLIGHT_HOT_RELOAD=1`, waits for boot markers, edits text+asset+pack manifests in turn, asserts each reload event fires within 5 s. |

### Fresh verification command

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
    -File research_uiux\runtime_reference\tools\phase370c_path_b_proof.ps1 `
    -AutoExitSeconds 60 `
    -ReloadTimeoutSeconds 5
```

Phase 367b / 368 / 369A / 369B / 370A / 370B runners still pass
unchanged. The watcher is OFF by default (env-gated), so vanilla
UR launches and the older runners are not affected.

---

## Phase 371A -- SGFX Pack Exporter + one-click Shell launcher

Phase 370A/B/C proved the engine could read, route, and reload an
override pack. Phase 371A turns that capability into a *product*: an
operator-facing exporter that builds a complete pack from CLI args
and a one-click launcher that runs UR as a branded SGFX shell with
save isolation and event tailing -- no manual env-var spelling, no
hand-stitched manifests, no risk of overwriting save progress.

### What 371A adds

| Layer | Path | Role |
|---|---|---|
| Exporter | [`research_uiux/runtime_reference/tools/sgfx_pack_exporter.ps1`](runtime_reference/tools/sgfx_pack_exporter.ps1) | Pure-PowerShell pack producer. Inputs: ticket / project / phase / scoped-rules JSON / pixel-override DDS / branding paths. Outputs the full `<OutputDir>/{sgfx_pack.json, pack_meta.json, text/, pictures/, sgfx_branding/, loose/, pack_export.log}` tree. |
| Launcher | [`research_uiux/runtime_reference/tools/sgfx_shell_launch.ps1`](runtime_reference/tools/sgfx_shell_launch.ps1) | One-click runner. Calls the exporter (unless `-SkipExport`), backs up `%APPDATA%\UnleashedRecomp\save\` (SHA-256 verified), sets every `SG_PREFLIGHT_*` env var, copies `UnleashedRecomp.exe` -> `SgfxShell.exe` (taskbar/process branding), launches, and tails `events.jsonl` translating raw events into human lines (`[meta] ticket=... project=...`, `[pack] reloaded -- ...`, etc.). On exit/Ctrl+C it deletes the branded copy and restores the save backup if hashes diverged. |
| Pack metadata | [`UnleashedRecomp/patches/sg_pack.h`](../UnleashedRecomp/patches/sg_pack.h) / [`.cpp`](../UnleashedRecomp/patches/sg_pack.cpp) | New `pack_meta.json` reader (boot-time, not in the snapshot path). Emits `Pack:Meta:<ticket>:<project>:<phase>` and exposes `TryGetTicket() / TryGetProject() / TryGetPhase()` accessors that future in-game QA panels can read. |
| Proof | [`research_uiux/runtime_reference/tools/phase371_path_b_proof.ps1`](runtime_reference/tools/phase371_path_b_proof.ps1) | End-to-end runner: builds UR, runs the exporter, validates 7 expected files on disk, launches via env-branding, asserts boot events, re-runs the exporter mid-run with a different rule count, asserts `Text:ScopedRulesReloaded:<m>` arrives within 8s, verifies save SHA. |

### Exporter output layout

```
<OutputDir>/
  sgfx_pack.json                   -- pack index (branding + lane paths)
  pack_meta.json                   -- ticket/project/phase metadata
  text/sgfx_text.json              -- scoped CSD rules (v2 schema)
  pictures/sgfx_pictures.json      -- pixel override manifest
  pictures/logo_sonicteam_override.dds
  sgfx_branding/icon.png           -- window icon (PNG via stb_image)
  sgfx_branding/logo.png           -- decorative; reserved for future panel
  loose/<...>                      -- explicit loose-file substitutions (when provided)
  pack_export.log                  -- audit trail of what was written
```

### Phase 371A acceptance gates (each fails with a distinct exit code)

| Code | Gate | Description |
|---:|---|---|
| 2 | build / deploy | `_phase367_build.bat` failed or `UnleashedRecomp.exe` missing. |
| 3 | exporter output | Any of the 7 expected output files missing, or `pack_meta.json` round-trip mismatch (ticket/project/phase). |
| 4 | `Pack:Meta:<t>:<p>:<ph>` | Bridge event missing within 20s of launch. |
| 5 | `Pack:Loaded` + `Branding:Active` | Either missing within 15s. |
| 6 | `Text:ScopedRulesLoaded:4` | Initial scoped-rule count diverged from what the exporter wrote. |
| 7 | `Text:ScopedRulesReloaded:6` | Mid-run re-export did not trigger a hot reload within 8s. |
| 8 | save restored | Save SHA differed pre/post and the restore did not match the pre snapshot. |
| 9 | native frame | No BMP captured under EvidenceDir; UR never reached a render frame. |

### Runtime-proven evidence (2026-05-07 run)

```
{"screen":"Pack:Loaded:sgfx_text.json:sgfx_pictures.json:0"}
{"screen":"Pack:Meta:IDCEVODEV-960073:BMW SGFX QA Shell:371A"}
{"screen":"Text:ScopedRulesLoaded:4"}
{"screen":"Asset:PixelOverridesLoaded:1"}
{"screen":"Branding:Active:SGFX QA Shell -- IDCEVODEV-960073|icon.png|SGFX 0.5 (Phase 371A)"}
{"screen":"HotReload:WatcherStarted"}
{"screen":"Text:ScopedRulesReloaded:6"}
```

| Gate | Pass |
|---|---|
| Pack:Meta | True |
| Pack:Loaded | True |
| Branding:Active | True |
| Text:ScopedRulesLoaded:4 | True |
| Text:ScopedRulesReloaded:6 | True (1.55 s end-to-end through exporter re-run) |
| Save restored | True (save SHA unchanged) |
| Native frame written | True |

### Event taxonomy added in Phase 371A

| Event | Emitter | When |
|---|---|---|
| `Pack:Meta:<ticket>:<project>:<phase>` | `SGPack::EnsureLoaded` (via `LoadPackMetaOnce`) | Once at boot, after the lane snapshot is built. Fields are scrubbed of `:` / `\|` characters; empty fields become `_` so the parser remains unambiguous. |

`TryGetTicket() / TryGetProject() / TryGetPhase()` are exposed for
the future in-game QA panel (Phase 371C). They return `nullptr` when
no `pack_meta.json` was present, so unmodified UR boots with no
pack stay free of metadata-related output.

### Decisions honored

- **No round-trip through `ConvertTo-Json` on the array fields** in the exporter. PowerShell's serializer wraps `[ordered]@{}` array values as `{ "value":[...], "Count":N }`, which UR's nlohmann::json parser would reject. The exporter emits `text/sgfx_text.json` and `sgfx_pack.json` as literal templated strings with each rule and loose-file entry compiled to compact JSON via `ConvertTo-Json -Compress` on the leaf value only. Result: clean JSON arrays, regardless of PowerShell's in-memory representation.
- **`pack_meta.json` is read once and NOT hot-reloaded.** Changing the ticket mid-run would make any QA evidence already captured ambiguous. Operators who need a different ticket restart UR.
- **Save restore is mandatory in the launcher.** SHA-256 pre/post compare; if hashes diverged and a backup was made, restore in the `finally` block (covers Ctrl+C and forced kills).
- **Branded exe copy is `SgfxShell.exe`**, deleted on exit so the install dir is left in vanilla state.
- **Tail loop is human-readable.** Each known event prefix maps to a short status line (`[meta]`, `[pack]`, `[text]`, `[pix]`, `[hot]`); unknown events are dropped (the operator does not need to see every loader debug emit). Raw `events.jsonl` is preserved under EvidenceDir for post-mortem.

### Honest gaps (still pending product layer)

- **No in-game QA panel yet.** `TryGetTicket() / TryGetProject() / TryGetPhase()` are exposed but not surfaced visually. That is Phase 371C (ImGui panel showing ticket / pack status / reload counters).
- **No per-ticket save isolation.** The launcher backs up + restores the global save dir. A future Phase 371B should redirect `%APPDATA%\UnleashedRecomp\save\` to a per-ticket subdir so two QA sessions can run in alternation without restoring between switches.
- **No route selector** ("open Title shell" / "open World Map shell" / "open HUD shell"). Pack consumers still rely on whichever screen UR boots into. Route presets are a Phase 371B beat.
- **3D car mesh QA viewport** is not part of 371A and stays in Path A territory: it requires ImGui-side rendering or a native viewport, fed from the `sg-preflight` model dirs. Out of scope for the "make the SGFX shell a tool" beat.

### Fresh verification command

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
    -File research_uiux\runtime_reference\tools\phase371_path_b_proof.ps1 `
    -AutoExitSeconds 60 `
    -ReloadTimeoutSeconds 8
```

For interactive QA use (no proof, no auto-kill, just launch the shell):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
    -File research_uiux\runtime_reference\tools\sgfx_shell_launch.ps1 `
    -Ticket "IDCEVODEV-960073" -Project "BMW SGFX QA Shell"
```

All Phase 367b / 368 / 369A / 369B / 370A / 370B / 370C runners
continue to pass unchanged. `pack_meta.json` is OPTIONAL: when the
operator stages a pack without it (the legacy 370B/C layout),
`Pack:Meta` simply does not fire and every other lane behaves
exactly as before.
