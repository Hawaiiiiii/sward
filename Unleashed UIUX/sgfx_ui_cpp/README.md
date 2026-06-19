# Operator UI (native C++)

A native C++ screen viewer — the cinematic operator surface for the Project
Quality-Hero QA tool. It renders the tool's workflow as full-screen views (launcher,
QA hub, profile select, run metrics, verdict, evidence review, settings, …),
navigable end to end, and reads a real run's data through a small status file. With
no status file present it shows representative defaults, so it runs standalone.

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

## Ship a runtime drop
    python tools/package.py --build build --out drop --zip

Produces a clean drop — exe + the D3D12 runtime + the fonts + the host logo-slot
folder — leaving out the game art atlases, layout data and music the screens don't
load. Drop your own logo at `assets/gameart/boot_logo.png` and a real
`sgfx_status.json` beside the exe.

## Layout
- `src/` — the screens (`screen_<id>.cpp`), the `sgfxui` draw layer, the D3D12
  backend, and the data bridge (`sgfx_data.*`).
- `tools/` — the status exporter, the launcher, the packager, the tests.
- `../specs/` — a layout/interaction spec per screen + the data-bridge contract.

## Known
The on-screen fonts are still the reconstruction's MSDF fonts; swapping them for an
own/permissive face is the one remaining asset task. The host logo, the
baseline/candidate review thumbnails and the globe texture are content slots
(absent = clean fallback).
