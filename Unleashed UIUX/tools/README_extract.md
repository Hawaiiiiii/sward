# Auto-fill pipeline — reconstruct screens from YOUR own copy

The SWARD viewer composes each "Reconstructed" screen from a **manifest**
(`manifests/<screen>.json`) — node rects authored in the game's native
1280×720 reference space — plus **PNG textures** in `assets/<screen>/`.

This folder turns your own legally-owned game files into those PNGs +
manifests. **No game assets are shipped here** — same model as
UnleashedRecomp, which also requires you to supply your own dump.

## How the viewer resolves art

For every node, the viewer renders an `<image-slot src="assets/<screen>/<tex>">`:

- **PNG present** → it loads automatically, placed at the real rect.
- **PNG missing** → you get a placeholder + drag-to-fill target.

So "filling" the screens is a build step (`extract_ui_assets.py`), not
manual dragging. Drag-drop still works as a fallback for one-offs.

## One-time setup

```bash
python -m pip install pillow
# Windows + compressed DDS: put Microsoft 'texconv.exe' on PATH
```

`external_tools/HedgeArcPack` is already in this tree.

## Run

```bash
python tools/extract_ui_assets.py \
  --game  "Unleashed Recomp - Windows (Complete Installation) 1.0.3/game" \
  --out   . \
  --screens title sonic_hud pause world_map result boss loading options
```

This writes `assets/<screen>/*.png` and `manifests/<screen>.json`, then the
viewer's **Reconstructed** screens render 1:1 from your files.

## Generating manifests from REAL parsed layouts — `csd_to_manifest.py`

The per-node **rect** reader is now concrete. `research_uiux/tools/inspect_xncp_yncp.py`
already parses every extracted `.xncp/.yncp` into
`research_uiux/data/layout_deep_analysis.json` (28 layouts, full CSD cast tree
with quad corners, cast names, and texture names). `csd_to_manifest.py` reads
that and emits real-rect manifests — no estimates:

```bash
python "Unleashed UIUX/tools/csd_to_manifest.py"                 # all configured screens
python "Unleashed UIUX/tools/csd_to_manifest.py" --only pause world_map
python "Unleashed UIUX/tools/csd_to_manifest.py" --dump ui_pause # inspect a stem's scenes
```

What it does:

- Resolves the cast hierarchy + `cast_info.translation`/`scale` into absolute
  1280×720 rects (v3 quad corners and translations are normalised screen
  fractions; accumulated down the tree, scaled to px at the leaf).
- Emits **one region per top-level cast per CSD scene** — the container-level
  granularity (header / footer / info / stage panels are already separate
  scenes). 9-slice frame fragments collapse into their parent's bbox; runaway
  stretch pieces and code-positioned cells are filtered as outliers.
- Names each slot after the **real texture** (`mat_pause_en_001.png`), so it
  auto-loads once `extract_ui_assets.py` has written the matching PNG.
- Skips (and reports) scenes authored around a local origin — those are
  positioned by runtime code, not by the layout, so they resolve off-screen
  and need a runtime capture (see below) or manual placement.

Currently configured screens: `pause`, `world_map`, `result`, `boss`, `loading`
(plus the hand-authored `title`, `sonic_hud`). `options` has **no** dedicated
`.yncp` — it's a frontend menu, so it stays as the DOM screen rather than a
guessed-rect manifest.

**Atlas UV cropping (implemented):** the game packs many regions into shared
atlas sheets (`mat_*_common_*`) addressed by per-cast UV sub-rects. Each manifest
node now carries a `uv:[u0,v0,u1,v1]` (the representative sprite's normalised
sub-rect, read straight from the parsed `subimages`), and `<image-slot>` crops the
loaded sheet to that sub-rect — so an atlas-backed slot shows its *one* sprite, not
the whole sheet. A user's own dropped image ignores `uv` and fills the slot normally.
Limitation: a region that fuses several different sprites from one atlas (some menu
rows) crops to its largest sprite; structure/placement stay exact.

## Filling assets from already-extracted textures — `fill_assets.py`

If the textures are already unpacked on disk (the research phase left ~3,900 DDS
under `extracted_assets/`), skip archive unpacking entirely. `fill_assets.py` reads
the texture names the manifests reference, finds each `.dds`, and converts it to
`assets/<screen>/<tex>.png` with Pillow (these de-tiled DDS decode to RGBA without
texconv):

```bash
python "Unleashed UIUX/tools/fill_assets.py"                 # all manifest screens
python "Unleashed UIUX/tools/fill_assets.py" --only world_map loading
python "Unleashed UIUX/tools/fill_assets.py" --force         # re-convert existing
```

Ships no assets — operates only on files already on this machine, from your own copy.

## Eyeballing a screen offline — `preview_render.py`

Flattens a manifest + its filled assets (UV crops applied) into
`_preview/<id>.png`, so you can review the reconstruction without the live viewer
(handy where the in-browser preview won't composite):

```bash
python "Unleashed UIUX/tools/preview_render.py" world_map pause
```

### Legacy `--parser` hook (live extraction path)

`extract_ui_assets.py` still accepts `--parser your_module` where
`your_module.parse_layout(stem, dir)` returns
`[ { "id", "tex", "rect": [x,y,w,h] }, ... ]` in 1280×720 space, for parsing
freshly-unpacked archives inline. Without it (or `csd_to_manifest.py`), the
script emits a **skeleton** manifest (one full-frame slot) so you still get a
working, fillable screen.

## Live-from-runtime capture — the genuinely-missing bits

Static `.yncp` extraction can't recover what the game computes at **runtime**:
elements re-anchored by code off their authored origin (the pause **title bar**,
the boss **gauge frame** → `ALIGN_TOP_RIGHT`), and **text/number sprites swapped at
runtime** (the real row labels TIME/RING/SCORE, score values, menu item names — the
layout only holds a placeholder). To get those, capture the casts the recomp actually
**draws** and feed them to `runtime_capture_to_manifest.py`.

### 1 · Build the recomp — the capture hook is already integrated

The capture is wired into the recomp source (**inert** unless enabled, so normal play is
unaffected). What's in the tree:

- `UnleashedRecomp/patches/csd_capture.{h,cpp}` — the dumper (new file, added to
  `CMakeLists.txt`).
- `aspect_ratio_patches.cpp` — at the per-cast draw choke point `Draw()` it records each
  drawn cast; `EmplacePath` keeps the readable path string (the engine's `g_paths` stores
  only an XXH64 hash); the `RenderCsdCast*` hooks note the current cast.
- `gpu/video.cpp` — `MakePictureData` builds a `GuestTexture* → name` map and exposes the
  slot-0 bound texture, so each dumped cast carries its real texture name.

It reads each cast's 4 vertices while they're still in **1280×720 reference space** (before
the aspect transform), so the rects match the manifest space directly. It captures
rect + path + texture + textured-flag only — `uv`/`alpha` come from the static manifest via
`--augment`, so the ambiguous packed-vertex fields are never relied on. Hidden/SKIP and
corner-extract measurement passes are excluded automatically.

Just build the recomp as usual — no code changes needed.

### 2 · Capture

```
set SWA_CSD_CAPTURE=1   &&   run the recomp        (Windows)
SWA_CSD_CAPTURE=1 ./UnleashedRecomp                (Linux/macOS)
```

Navigate through the screens you want (pause, world map, a boss, a result, a loading
screen). Every drawn UI cast is appended to **`csd_capture.jsonl`** in the working
directory (the screen redraws each frame, so the *last* line per element is its final
on-screen state). Quit when done and copy that file next to the tools.

### 3 · Ingest → manifests

```bash
python "Unleashed UIUX/tools/runtime_capture_to_manifest.py" csd_capture.jsonl --stream
python "Unleashed UIUX/tools/runtime_capture_to_manifest.py" csd_capture.jsonl --stream --augment
```

`--stream` splits the `.jsonl` into one manifest per screen (keying the screen off each
path's `.yncp` stem, keeping the last-drawn rect per element). Add `--augment` to **merge**
the runtime elements into the existing static manifests instead of replacing them — this
adds the off-screen-anchored / runtime-text elements the static pass dropped (pause title
bar, boss gauge frame, real row labels) while keeping the rest. Then run `fill_assets.py`
for any newly-referenced textures.

*(Single-frame JSON `{screen, ref, elements:[…]}` is also accepted without `--stream`;
validate the ingester end-to-end with `runtime_capture_to_manifest.py --demo`. RenderDoc
alternative: export each CSD draw's bound texture + post-VS vertex positions into the same
schema.)*
