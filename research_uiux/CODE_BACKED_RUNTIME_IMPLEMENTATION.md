<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Code-Backed Runtime Implementation

## Summary

Phase 21 turns the generic template pack into a buildable, reusable C++ reference runtime.

Implementation root:

- `research_uiux/runtime_reference/`

Key files:

- `runtime_reference/include/sward/ui_runtime/runtime.hpp`
- `runtime_reference/src/runtime.cpp`
- `runtime_reference/examples/pause_menu_example.cpp`
- `runtime_reference/CMakeLists.txt`

> [!IMPORTANT]
> This runtime is intentionally generic. It does not link against extracted assets, translated PPC output, or Sonic-specific content. It is a publishable reference implementation of the structural patterns recovered by the earlier phases.

## What It Implements

The runtime currently provides:

- explicit screen states:
  - `Boot`
  - `Intro`
  - `Idle`
  - `Navigate`
  - `Confirm`
  - `Cancel`
  - `Outro`
  - `Closed`
- timer-banded state timeouts
- input-lock behavior derived from current state
- overlay-role visibility by state
- predicate-driven button prompt visibility
- host/runtime callbacks for:
  - state entry
  - state change
  - blocked input
  - requested scene tags

## Relationship To The Template Pack

This runtime is the code-side mirror of:

- `templates/screen_state_machine_template.yaml`
- `templates/overlay_stack_template.yaml`
- `templates/transition_timeline_template.yaml`
- `templates/button_prompt_template.json`
- `templates/screen_contract_template.yaml`

The mapping is direct:

- `ScreenContract` mirrors the screen contract template
- `StateDefinition` mirrors the state machine template
- `TimelineBand` mirrors the transition timeline template
- `PromptSlot` mirrors the button prompt template
- `OverlayLayer` plus state visibility rules mirror the overlay stack template

## Example Included

`examples/pause_menu_example.cpp` builds a generic pause-menu contract and demonstrates:

- boot-to-intro startup
- intro timeout to idle
- navigation pulse
- confirm path returning to idle
- cancel path leading to outro and closed
- prompt-row visibility only when input is unlocked

This is not meant to copy Sonic Unleashed's exact code path. It is a reusable skeleton that preserves the architectural lessons:

- choreography over single-state widgets
- explicit lock windows
- scene-tag requests as orchestration seams
- prompt rows as stateful output rather than ad hoc per-button checks

## Build Verification

Local build command:

```powershell
cmake -S research_uiux/runtime_reference -B b/rr
cmake --build b/rr --config Release
b/rr/Release/sward_ui_runtime_example.exe
```

Verified locally in this workspace:

- configure: succeeded
- build: succeeded
- example executable: ran successfully

## Adaptation Guidance

For original projects:

- replace scene tags such as `reveal`, `focus_pulse`, `confirm`, and `outro` with your own animation identifiers
- replace button/icon policy with your own platform family
- keep the state machine explicit even if your UI toolkit is declarative
- keep prompt visibility and overlay visibility derived from state, not scattered through unrelated widget code

## Why This Matters

Earlier phases recovered:

- timing bands
- overlay layering
- prompt-row abstraction
- ceremony versus telemetry screen families
- loading/cutscene/transition ownership rules

This phase makes those findings executable. The workspace now contains not just notes and templates, but a concrete runtime reference that can be used as a starting point for original UI systems.
