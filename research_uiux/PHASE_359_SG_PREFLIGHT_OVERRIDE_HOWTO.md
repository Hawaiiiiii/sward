# Phase 359 -- SG-Preflight asset override mount (operator how-to)

> Current note: Phase 366 tracks the source implementation directly under
> `UnleashedRecomp/` and proves the mount through
> `Asset:OverrideHit:<guest-path>` bridge events. The override dir can mirror
> normal root-relative paths such as `work/Application/...`; the runtime also
> keeps the existing `win32/game/...` layout for retail asset packs.

## TL;DR

You can now run plain UnleashedRecomp with retail Sonic Unleashed,
and swap any retail file with a custom one by dropping it in an
override directory. No `cpkredir.ini`, no `Mods/<id>/mod.ini`, no
HMM/UMM manifest. Just a directory with the right path layout.

```powershell
# Activate the override dir
$env:SG_PREFLIGHT_OVERRIDE_DIR = "C:\path\to\sg_overrides"

# (Optional) silence the per-load console log if it's too noisy
# $env:SG_PREFLIGHT_LOG_LOADS = "0"

# Run UnleashedRecomp normally
.\UnleashedRecomp.exe
```

Drop replacement files at the matching game-relative path under
`<override-dir>/win32/...`. Your file overrides retail.

## Why this exists

UnleashedRecomp already supports a Hedge Mod Manager / Unleashed
Mod Manager mod stack via `cpkredir.ini` + a mods database. Great
for end-user mods. Heavy for sg-preflight's incremental "swap one
asset, see what happens, move on" workflow during alpha
development.

Phase 359 adds a *minimal* override mount that piggybacks on the
existing mod loader so:

- you don't author an `.ini` to swap a single texture;
- the override always wins (registered at the top of `g_mods` so
  it has priority over any cpkredir mod);
- console logging is auto-enabled when the override mount is active
  so you see substitutions land in real time.

## Activation precedence

The override directory is resolved in this order (first match wins):

1. `SG_PREFLIGHT_OVERRIDE_DIR` environment variable -- absolute
   path to a directory.
2. `<userPath>/sg_preflight_overrides/` -- auto-detected if it
   exists. (`userPath` = `%LOCALAPPDATA%\UnleashedRecomp` on
   Windows by default.)

If neither is set / present, the override mount stays inactive and
UnleashedRecomp behaves identically to a stock build.

## Path layout

Inside the override dir, mirror the in-game path. UnleashedRecomp's
file system uses `game:\` and `update:\` roots that resolve to
`win32/<retail_dir>/...`. So the override layout is:

```
<override-dir>/
└── win32/
    ├── game/
    │   ├── Title/
    │   │   └── ui_title.yncp                  # overrides retail title CSD
    │   ├── MainMenu/
    │   │   └── ui_mm_contentstext.dds         # overrides menu row text strip
    │   ├── ActionCommon/
    │   │   └── ui_result.yncp
    │   └── Languages/English/MainMenu/        # if you author EN strips here
    │       └── mat_mainmenu_text_en.dds
    └── update/
        └── ...
```

Refer to `extracted_assets/full_install_archives/<...>` in the
SWARD repo for the canonical file paths -- they mirror what
UnleashedRecomp expects under `<override-dir>/win32/`.

## Workflow: swap a single asset

Goal: replace the SU title-screen logo with a placeholder.

1. Make the override dir:
   ```powershell
   mkdir C:\sg_overrides\win32\game\Title -Force
   ```
2. Drop your replacement (any tool that produces a Sonic-style CSD
   `.yncp` -- HedgeArcPack, custom asset pipeline, etc.):
   ```powershell
   copy my_logo.yncp C:\sg_overrides\win32\game\Title\ui_title.yncp
   ```
3. Activate the mount:
   ```powershell
   $env:SG_PREFLIGHT_OVERRIDE_DIR = "C:\sg_overrides"
   ```
4. Run UnleashedRecomp normally. The console will log:
   ```
   [SG-Preflight] override mount active: "C:\sg_overrides" (priority=top)
   [Mod Loader] Loading file: "C:\sg_overrides\win32\game\Title\ui_title.yncp"
   ```
   The second line confirms your file landed instead of retail.

## Workflow: incremental conversion of SU UI to sg-preflight

Each of the in-scope screens has a primary CSD project; the
SWARD validation pack documents which.

| sg-preflight screen | SU analog | Override target |
|---|---|---|
| Launch / project picker | Title menu | `win32/game/MainMenu/ui_mainmenu.yncp` |
| Bundle picker (3D BMW car instead of Earth globe) | World Map | `win32/game/WorldMap/ui_worldmap.yncp` |
| Preflight loading display | Loading | `win32/game/Loading/ui_loading.yncp` |
| Validation runner HUD | Day Sonic stage HUD | `win32/game/Sonic/ui_playscreen.yncp` |
| Settings / blocker overlay | Pause menu | `win32/game/SystemCommonCore/ui_pause.yncp` |
| QA evidence summary | Results screen | `win32/game/ActionCommon/ui_result.yncp` |
| Review Board | Hub town screen | `win32/game/Town_Common/ui_townscreen.yncp` |

Swap one at a time, run, get feedback, iterate. Console log shows
exactly which retail file each override replaced.

## What this does now

- **Asset override mount.** `SG_PREFLIGHT_OVERRIDE_DIR` wins over retail loads
  and emits `Asset:OverrideHit:<guest-path>` for proven substitutions.
- **Text override manifest load.** `sg_text_overrides.json` loads from the same
  override dir and emits `Text:OverridesLoaded:<count>`.
- **Gameplay skip + UI-only input.** `SG_PREFLIGHT_GAMEPLAY_SKIP=1` and
  `SG_PREFLIGHT_UI_ONLY_INPUT=1` keep the player in UI/HUD space and emit
  `Stage:GameplaySkip` plus `Input:UiOnlyLock`.
- **Bridge events.** The C++ host writes events to
  `%APPDATA%\UnleashedRecomp\sgfx_bridge\events.jsonl`; the optional Python
  daemon can tail the same path and publish `state.json`.

## Mirroring this change to sg-preflight's UnleashedRecomp copy

The Phase 359 change lives in:

```
local_build_env/ur103clean/UnleashedRecomp/mod/mod_loader.cpp
```

To get it into sg-preflight (`sg-preflight/UnleashedRecomp-1.0.3/
UnleashedRecomp/mod/mod_loader.cpp`), copy the
`registerSgPreflightOverrideMod()` function and the
`ModLoader::Init()` call site (one line addition near the top of
`Init`). The change is self-contained -- one new static function
+ one call site, no other files touched.

Once sg-preflight's UR copy has the same change, both build paths
honor `SG_PREFLIGHT_OVERRIDE_DIR`.
