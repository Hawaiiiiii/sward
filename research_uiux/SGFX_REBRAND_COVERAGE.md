# SGFX Rebrand Coverage (Phase 373 inventory)

This is the practical "stop seeing Sonic stuff" checklist for the
SGFX shell. Each row identifies a place where the running shell
still looks/sounds like retail Sonic Unleashed, and classifies HOW
that surface can be replaced today.

**Scope.** This document covers the SGFX shell's render-time and
host-window surfaces only. Gameplay logic, physics, audio mixer
wiring, and 3D viewport composition are out of scope for the
Phase 373 audit -- they are tracked separately under "future Path A
work" in [PHASE_366_PATH_B_RUNTIME_PROOF.md](PHASE_366_PATH_B_RUNTIME_PROOF.md).

**Status legend.**

| Tag | Meaning |
|---|---|
| `text-scoped`   | Replaceable today via `scoped_text_rules` in the BMW QA pack (Phase 369B + 372) |
| `pixel`         | Replaceable today via `picture_overrides` (DDS) consumed by the MakePictureData hook (Phase 369A + 372) |
| `loose`         | Replaceable today via a `loose_files` entry in the pack pointing to a guest-path DDS (Phase 370B) |
| `branding`      | Already covered by `branding.{window_title, build_label, icon}` in the pack (Phase 370A) |
| `csd-route`     | Needs a new CSD project / scene-route override; design known, no implementation yet |
| `audio-hook`    | Needs a new audio override hook (BGM / SFX / voice); design unknown, no implementation yet |
| `unknown-probe` | The override mechanism is unclear from current evidence; needs a runtime probe or Ghidra cross-ref before deciding |

## Surfaces inventory

### Window / shell branding

| Surface | Source location | Replaceable today | Status | Notes |
|---|---|---|---|---|
| Window title bar | `SGBranding::TryGetWindowTitle()` -> SDL_SetWindowTitle | yes | branding | Driven by `pack.branding.window_title` (Phase 370A). The Phase 372 author tool templates `${ticket}` / `${project}`. |
| Window/taskbar icon | `SGBranding::TryGetIconPath()` -> SDL_SetWindowIcon | yes | branding | `pack.branding.icon` (PNG via stb_image; BMP via SDL_LoadBMP_RW). |
| Process / taskbar exe name | Launcher copies `UnleashedRecomp.exe` -> `SgfxShell.exe` | yes | branding | Phase 371A; deletes the branded copy on exit so the install dir stays vanilla. |
| Build label in log banner | `SGBranding::TryGetBuildLabel()` | yes | branding | Logged once at boot; does NOT mutate the git-derived `g_versionString` (intentional). |

### Title-screen text and labels

| Surface | Retail literal (representative) | Replaceable today | Status | Notes |
|---|---|---|---|---|
| Title menu options | `Common_Yes`, `Common_No`, locale strings via `Localise()` | yes | text-scoped | Targeted via `scoped_text_rules` with `csd_project_substring` matching the title CSD project. The Phase 372 author tool rejects unscoped rules to prevent the "BMW999 everywhere" regression. |
| HUD literals (digits / scores) | `99`, `999999`, `16`, `30`, `0` (status HUD) | yes | text-scoped | The Phase 372 BMW QA sample swaps these to `G65`, `G65-IDCevo`, `G65-16`, `G65-30`, `G65-0`. |
| In-game button-guide labels | Locale-driven strings like `Press_Start`, `Pause` | yes | text-scoped | Identical mechanism as title text. Worth confirming each project_substring per CSD layout. |
| Copyright / SEGA strings | "(C) SEGA" panels burned into pictures | partial | text-scoped + pixel | Some surfaces are text (text-scoped); some are baked into a picture (pixel via `picture_overrides`). Coverage TBD per concrete screen. |

### Picture-level Sonic branding

| Surface | Guest picture name | Replaceable today | Status | Notes |
|---|---|---|---|---|
| `logo_sonicteam` (Title) | `logo_sonicteam` | yes | pixel | Phase 369A demonstrated the swap; Phase 372's BMW QA pack uses it. Bytes are LZX-compressed retail-side; the SGFX path injects post-decompression at MakePictureData. |
| `logo_havok` | `logo_havok` | yes | pixel | Same mechanism; not yet exercised by the BMW QA sample (would need the operator to drop in a replacement DDS). |
| Loading-screen pictures | various; need enumeration | partially | pixel + unknown-probe | `MakePictureData` interception covers any picture name once we know it. The set of picture names retail uses on the Loading screen needs a runtime probe (turn on `SG_PREFLIGHT_LOG_LOADS=1` and walk the Loading screen). |
| Title-screen background art | name TBD | yes if name known | pixel | Same as above. |
| World Map Sonic icon | name TBD | maybe | pixel + csd-route | If it's a CTexturePicture: pixel. If it's a 3D model with a fixed Sonic mesh: csd-route + future viewport work. |
| HUD Sonic ring icon | name TBD | yes | pixel | Phase 369A pattern works for any picture name. |

### Loose-file replaceable surfaces

| Surface | Guest path | Replaceable today | Status | Notes |
|---|---|---|---|---|
| Loading logo DDS at fixed guest path | `Loading\logo_sonicteam.dds` | yes | loose | Pack's `loose_files` array; Phase 370B proved a DDS at a canonical guest path is served by `ResolvePath`. |
| Other DDS assets at known guest paths | various | yes | loose | Same mechanism. The set of guest paths the Loading screen requests is enumerable via `SG_PREFLIGHT_LOG_LOADS=1`. |
| Non-DDS assets (XEX, XMA, etc.) | various | partial | loose + unknown-probe | `loose_files` is mechanism-agnostic on the override side, but the consumer (`ResolvePath`) is read-only -- it does not transcode. Replacement formats must match what retail expects for that path. |

### CSD-level / project-route surfaces

| Surface | Hook target | Replaceable today | Status | Notes |
|---|---|---|---|---|
| Active CSD project gating | `SGTextOverrides::MarkCsdProjectActive` | partial | csd-route | Phase 369B uses `csd_project_substring` matching to scope text rules. Project ROUTING (e.g. "open the World Map shell" without launching gameplay) is not yet implemented. |
| Boot route preset | Launcher env routing (`SG_PREFLIGHT_NO_AUTOLOAD`) | yes | csd-route (mechanism only) | Phase 371B exposes `route` in pack_meta and surfaces it via `Pack:Route:` + the QA panel. The `auto/worldmap/hud/results` routes need captured per-route save sandboxes; only `title` is operationally complete today. |
| Stage select / mission flow | Sg-preflight `qa_actions` | partial | csd-route | sg-preflight already declares per-profile actions (`qa_stack__G65`, etc.); Phase 373 surfaces them via the bridge JSON, but UR does not yet act on the actions list -- the panel only displays counts. |

### Audio surfaces

| Surface | Hook needed | Replaceable today | Status | Notes |
|---|---|---|---|---|
| Title BGM (Endless Possibility) | `SDL_mixer Mix_PlayMusic` interception | no | audio-hook | The BGM stream goes through `Mix_PlayMusic` (Phase 347 wired the bridge). A picture/text override pattern does not apply -- a music-override hook needs to intercept the path before the music data is fed to SDL_mixer. |
| In-stage BGM | same | no | audio-hook | Same hook would cover this. |
| SFX (jump, dash, ring pickup) | XACT/SoundCue route | no | audio-hook + unknown-probe | XACT-based; the right hook point is unknown today. Listed for completeness. |
| Voice clips ("Sonic!") | same as SFX | no | audio-hook + unknown-probe | Same. |
| UI menu click SFX | same | no | audio-hook | Likely same XACT path. |

### Surfaces still classified `unknown-probe`

These need a runtime probe (turn on `SG_PREFLIGHT_LOG_LOADS=1`,
walk the screen in question, capture the asset names retail
requests) OR a Ghidra cross-reference before we can pick the right
override mechanism. Listed here so Phase 374+ has a starting list.

| Surface | Notes |
|---|---|
| Stage Title fly-in animation | Mix of CSD text + picture + maybe a 3D banner. Each component category needs to be split before mechanism choice. |
| Story cutscene captions | Locale strings -- text-scoped likely; needs probe. |
| Achievement unlock toast | Mix of picture + text + audio; partial coverage today via pixel + text-scoped, but the ding is audio-hook. |
| Pause menu Sonic portrait | Picture, but the path inside the CSD asset bundle is not yet known. |
| World map cursor Sonic head | Probably a model + picture, not a flat picture; csd-route territory. |

## Coverage summary

| Mechanism | Surfaces in this inventory | Implemented today |
|---|---:|---|
| Branding (window/icon/exe/label) | 4 | 4 |
| Text-scoped overrides | 4 | 4 |
| Pixel overrides (MakePictureData) | 6 | 2 (logo_sonicteam, logo_havok pattern); the rest need picture-name enumeration |
| Loose-file overrides | 3 | 1 (Loading/logo_sonicteam.dds proven); the rest depend on guest-path enumeration |
| CSD route overrides | 3 | 1 partial (route metadata + NO_AUTOLOAD only) |
| Audio overrides | 5 | 0 (no audio hook yet) |
| Unknown-probe | 5 | n/a -- requires probe or Ghidra |

**Net:** Of the headline branded surfaces an operator sees in the
first ~30 seconds of the SGFX shell (window, title menu, HUD
digits, Loading logo), roughly 4 of 7 are fully replaceable today
and one more is partially replaceable. The remaining surfaces
needing audio replacement or new hooks are bounded by **one new
hook per category** -- they are not a long tail.

## Operator-facing next steps

If you want the shell to "stop seeing Sonic stuff" as fast as
possible WITHOUT new hook work:

1. Stage a BMW QA pack via Phase 372 (`sgfx_author_from_preflight.ps1`).
2. Add scoped text rules for the title-menu literals you observe;
   the QA panel's reload counter ticks each time you re-author.
3. Add picture overrides for `logo_sonicteam` (Title) and any
   loading-screen pictures you have BMW assets for. Walk the
   Loading screen with `SG_PREFLIGHT_LOG_LOADS=1` to harvest the
   missing guest names.
4. Use the QA panel to confirm pack/route/profile match the ticket
   you opened the shell for.

If you also want the audio rebranded, that is a Phase 374+ beat:
the audio-hook category lists five surfaces that one new hook
(`Mix_PlayMusic` interception with a path-keyed override map)
should cover the BGM portion of.

---

*Generated by hand from runtime evidence in
[PHASE_366_PATH_B_RUNTIME_PROOF.md](PHASE_366_PATH_B_RUNTIME_PROOF.md)
and the Phase 367b through 372 proof outputs. Last updated as part
of Phase 373.*
