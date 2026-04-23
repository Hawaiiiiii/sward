<p align="right">
    <img src="../../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Template Pack

This directory contains generic, reusable UI/UX templates derived from the local SWARD research workspace.

These templates are for original projects only. They are intended to preserve structure, timing, and host/overlay contracts without copying proprietary assets or authored scene packages.

Included files:

- `screen_state_machine_template.yaml`
- `overlay_stack_template.yaml`
- `transition_timeline_template.yaml`
- `button_prompt_template.json`
- `screen_contract_template.yaml`

Suggested order:

1. Fill out `screen_contract_template.yaml`.
2. Wire state flow with `screen_state_machine_template.yaml`.
3. Assign timing bands from `transition_timeline_template.yaml`.
4. Bind prompts with `button_prompt_template.json`.
5. Build your own visuals and strings on top of `overlay_stack_template.yaml`.
