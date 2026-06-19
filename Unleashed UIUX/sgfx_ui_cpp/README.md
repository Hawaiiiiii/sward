# Operator UI (native C++)

A native C++ operator surface for the Project Quality-Hero QA tool. It renders the
workflow as full-screen views — launcher, QA hub, profile select, run metrics,
verdict, evidence review, a tabbed green settings family (settings / setup / the
action hub), and a 3D view of the actual car — navigable end to end, reading a real
run's data through a small status file. With no status file it shows representative,
profile-agnostic defaults, so it runs standalone.

## Build
Needs a checkout of the upstream recomp (for the plume RHI submodule + the DirectX
deps) at `RECOMP_ROOT`. From a VS x64 developer shell:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRECOMP_ROOT=<recomp> \
          -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl
    cmake --build build --target sgfx_screens

(The full one-time environment recipe — submodules, the vcpkg DirectX deps, the CSD
shader headers — is in the build notes.)

## Run
    sgfx_screens.exe                                    # interactive: boot -> launcher -> hub -> ...
    sgfx_screens.exe <screen-id>                        # open one screen (world_map, status, gate, ...)
    sgfx_screens.exe --shot <id> <sec> <out.png> [keys] # headless render; keys drive the nav
    sgfx_screens.exe --soak [sec]                       # headless fleet soak (crash/leak gate)

Controls: arrows / WASD move, Enter / Z = accept, Esc / Backspace = back.

## Real data
The viewer reads `sgfx_status.json` (beside the exe) for the active run, the hub
totals, and per-profile verdicts; absent, it falls back to defaults. Produce it from
the tool's report output:

    python tools/export_status.py --report <run>/logs/run-profile-G65/g65-report.json \
                                  --reports-dir <run>/logs --out sgfx_status.json
    # or one command that also launches the viewer:
    python tools/launch.py <run-folder> --profile G65 --exe path/to/sgfx_screens.exe

Schema + details: `../specs/data_bridge.md`. The exporter mapping is unit-tested
(`cd tools && python -m unittest test_export_status`).

## 3D car view
The "3D Car" screen (action hub → Live 3D car) renders the actual production car
inside the shell via the external Ramses/RaCo viewer: it asks the viewer to render a
frame of the car to a snapshot and displays it (composited as a D3D12 texture).
Left/Right pick the documented QA perspective (read from the car's
`perspectives_*.json`); RB shows the car live *inside* the pane (the viewer orbits and
writes a readback from a Ramses offscreen buffer, which the screen reloads), LB opens
it in a separate window. The gate also offers "Preview in 3D" with a set→view picker.

Paths are operator-local in `viewer3d.json` beside the exe (copy `viewer3d.example.json`):

    { "viewer_exe":   ".../sgfx_cine_ramses_real_scene.exe",
      "bmw_git_root": ".../digital-3d-car-models" }

The shell resolves `<profile>` → `<repo>/cars/BMW/<id>/export/exported.ramses` and
spawns the viewer with `--scene-file`. With no config the 3D actions show a clear
"not configured" status; everything else runs.

## Ambient Layer coverage
The "Ambient" screen (action hub → Ambient tests) shows a brand × screen readiness
matrix for the Ambient Layer screenshot tests. It reads the AL assets repo's per-scene
`tests/{expected,actuals,diff}` folders (the test workflow's own outputs) and reports
each cell as PASS, DIFF (a visual change since the baseline), not run, or no baseline —
read-only, it runs nothing. Point it at your checkout with `al_assets_root` in
`viewer3d.json`; unset shows a hint. Structure read:
`<root>/assets/<group>/<brand>/export_<screen>/tests/{expected,actuals,diff}`.

## Fleet readiness
The "Fleet" screen (action hub → Fleet readiness) lists every car in the 3D-car models
repo with its export state (`export/exported.ramses` present) and how many QA
camera-perspective sets it has (`perspectives_*.json`) — at a glance, which cars can be
rendered / screenshot-tested and how completely their QA cameras are set up. Read-only,
straight from `bmw_git_root` (the same root the 3D viewer renders from). Up/Down scroll.

## Ship a runtime drop
    python tools/package.py --build build --out drop --zip

Produces a clean drop — exe + the D3D12 runtime + the fonts + the host logo-slot
folder — leaving out the game art atlases, layout data and music the screens don't
load. Drop your own logo at `assets/gameart/boot_logo.png` and a real
`sgfx_status.json` beside the exe; for the 3D view, copy `viewer3d.example.json` to
`viewer3d.json` and set the paths.

## Layout
- `src/` — the screens (`screen_<id>.cpp`), the `sgfxui` draw layer, the D3D12
  backend, the data bridge (`sgfx_data.*`), the shared green chrome (`green_chrome.*`),
  and the 3D-viewer launcher (`viewer3d.*`).
- `tools/` — the status exporter, the launcher, the packager, the tests.
- `../specs/` — a layout/interaction spec per screen + the data-bridge contract.

## Known
The on-screen fonts are the game's licensable third-party faces (DFHei / NewRodin /
Seurat / dfsogei — not SEGA), kept by choice. The host logo, the baseline/candidate
review thumbnails and the globe texture are content slots (absent = clean fallback).
For the 3D view, a *live* car rendered inside the pane (vs the still snapshot) needs
Ramses rendered to a texture in-process — a re-parented window does not compose over
the shell's flip-model D3D12 swapchain. The in-shell snapshot and the live external
window are the working paths today.
