# Project Sonic World Adventure R&D Publishing Scope

This repository is the publishable layer of Project Sonic World Adventure R&D, a local Sonic Unleashed UI/UX research workspace.

## Included

- Handwritten open-source project source already present in this workspace
- Build scripts, presets, and project metadata that are part of the open-source codebase
- Research tooling under `research_uiux/tools/`
- Generic UI/UX engineering notes that do not embed proprietary assets or translated game code
- Readable code and patch indexes derived from the handwritten open-source layer

## Excluded

- Extracted game assets
- Private game inputs
- Generated translated PowerPC C++ output
- Generated shader cache output
- Locally downloaded reverse-engineering tools and SDK/runtime bundles
- Machine-specific build trees, caches, and recovery environments
- Reports or JSON data that catalog proprietary extracted content or generated code

## Working Rule

Use this repository for reusable tooling, notes, and open-source code only.

Keep all proprietary extracted content and generated translation output local-only, outside the published git history, even when the files were produced from owned copies on this machine.
