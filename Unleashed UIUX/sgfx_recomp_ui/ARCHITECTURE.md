# sgfx_recomp_ui — Sonic Unleashed's UI as a clean, reusable C++ library

**Goal.** Take UnleashedRecomp's *authentic* `ui/*.cpp` menu code — clean, human-written
C++ — and refactor it into a **standalone, customizable library** decoupled from the
game runtime, so it drops into SGFX or any other project. NOT the playable game, NOT
the machine-generated recompile blob (`UnleashedRecompLib/ppc/`), NOT our old eyeball
reconstruction (`sgfx_ui_cpp`). The real menu code, liberated.

## Why this exists (the realization)
The previous `sgfx_ui_cpp` was a *from-scratch reimplementation* — a faithful forgery
in a custom engine. The real UI already exists in `C:\swardbuild\UnleashedRecomp\ui\`.
Refactoring the real code is both more authentic (it IS the recomp's logic) and the
point of the exercise (reusable clean C++).

## The two decoupling layers
The recomp's `ui/` has exactly two kinds of dependency. We keep one and shim the other:

1. **KEEP — the render layer.** Dear ImGui (`thirdparty/imgui`) + the recomp's custom
   imgui gradient/outline/skew/additive layer (`gpu/imgui/imgui_common`). This is the
   "secret sauce" that makes imgui look like the game. Portable; behind a swappable
   GPU backend.
2. **SHIM — the game runtime.** Everything game-specific, replaced by
   `platform/sgfx_platform.h` (~6 interfaces). Full coupling surface, measured across
   all 12 ui/ files (8,022 lines):

   | Game symbol | hits | Shim |
   |---|---|---|
   | `Config::` | 138 | `sgfx::Config` (the customization surface) |
   | `GameWindow` | 66 | `sgfx::window::` |
   | `Localise` | 44 | `sgfx::Localise()` |
   | `SWA::` | 34 | abstracted per use |
   | `hid::` | 20 | `sgfx::input::` |
   | `App::` | 16 | `sgfx::app::` |
   | `AspectRatio` | 14 | `sgfx::g_aspectRatio` + constants |

   (`g_` = 1361 hits but the vast majority are UI-local state — kept as-is. `ImGui::`
   = 139 — kept.)

## File order (smallest/foundational first, so each builds on a verified base)
| # | file | lines | stage |
|---|---|---|---|
| 1 | `black_bar` | 58 | ✅ **done** (proof of the pattern) |
| 2 | `fader` | 79 | next |
| 3 | `tv_static` | 392 | |
| 4 | `button_guide` | 322 | |
| 5 | `imgui_utils` | 963 | **keystone** — all menus depend on it |
| 6 | `message_window` | 594 | |
| 7 | `achievement_overlay` / `achievement_menu` | 243 / 788 | |
| 8 | `game_window` | 589 | |
| 9 | `options_menu` (+ thumbnails) | 1854 / 255 | the flagship |
| 10 | `installer_wizard` | 1885 | |

## Build harness (next milestone — required to *verify* each refactor)
A new MSVC target (modeled on `sgfx_ui`, which already links plume + builds here):
Dear ImGui + `gpu/imgui` (custom render layer) + a minimal SDL/D3D12 loop +
`platform/sgfx_platform_default.cpp` (standalone implementations of the shim) +
the refactored `ui/` files. First target: render `black_bar` + a gradient box through
the *real* `imgui_utils` — proving the authentic render layer stands alone. Until this
exists, refactored files are reviewed-but-unbuilt; the harness is priority #1.

## In-game screens (HUD / pause / status / world map / result / …)
The recomp has **no C++** for these — they are the game's own **CSD** data, rendered by
the game's cast system. So "the recomp as the base" only yields C++ for the ~10 menu
overlays above. For the in-game screens the reusable options are: (a) a clean C++ CSD
loader/renderer (the `csd_player` approach, driven by the real game CSD), or (b) keep
the hand-authored `sgfx_ui_cpp` versions as the in-game half. Decision deferred until
the menu refactor is building.

## Status
- **Stage 1 (this pass):** scaffold + `sgfx_platform.h` shim design + `black_bar`
  refactored 1:1 (game coupling → shim, logic untouched). The pattern is proven on
  paper; the build harness is the immediate next step to make it real.
