<p align="center">
<img src="./docs/assets/branding/logo_sward.png" width="900"/>
</p>

# Project Sonic World Adventure R&D

Also referred to in this workspace as **SWARD**.

Research and Development workspace around Sonic Unleashed HD / Unleashed Recompiled, focused on UI/UX behavior, state machines, menu architecture, overlays, transitions, animation timing, and reusable design-engineering patterns.

> [!NOTE]
> This repository is the publishable R&D layer only. It contains open-source code already present in the workspace, research tooling, readable indexes, and transferable UI/UX notes.

> [!IMPORTANT]
> This repository does **not** publish extracted game assets, private game inputs, generated translated PowerPC C++, shader cache output, or leaked/proprietary source. Those remain local-only.

> [!WARNING]
> Sonic Unleashed and related assets belong to SEGA / Sonic Team. This repository is an unaffiliated research workspace and contribution layer. Use your own legally acquired files for any local asset-backed workflow.

> [!TIP]
> Project history now lives in [`CHANGELOG.md`](./CHANGELOG.md). The fastest way into the current R&D layer is the report stack under [`research_uiux/`](./research_uiux/).

> [!TIP]
> For the direct answer to "how much of the game is actually recovered here?", start with [`research_uiux/WHOLE_GAME_COVERAGE_AND_GAPS.md`](./research_uiux/WHOLE_GAME_COVERAGE_AND_GAPS.md).

## <img src="./docs/assets/branding/icon_extra.png" width="30" alt="SWARD icon"/> What This Repository Is

This project keeps a documented, versioned R&D environment for studying:

- title screens and intro flow
- pause, options, achievement, and message UI
- HUD activation and suppression behavior
- fades, black bars, static overlays, and transitions
- button guides and input lockouts
- timing/state relationships between readable patch code and local-only generated outputs
- reusable UI/UX engineering patterns for original projects

The codebase includes a snapshot of the open-source Unleashed Recompiled integration layer plus research automation that helps index readable UI code, patch hooks, and local research outputs.

## <img src="./docs/assets/branding/icon_debug.png" width="30" alt="SWARD icon"/> What Lives Here

- Open-source handwritten runtime, UI, and patch code from the Unleashed Recompiled layer
- Build scripts, presets, and project metadata
- Local repository branding assets under [`docs/assets/branding/`](./docs/assets/branding)
- Research scripts under [`research_uiux/tools/`](./research_uiux/tools)
- Reusable template pack assets under [`research_uiux/templates/`](./research_uiux/templates)
- Reusable runtime reference code under [`research_uiux/runtime_reference/`](./research_uiux/runtime_reference)
- Publishable research notes such as:
- [`research_uiux/UI_CODE_INDEX.md`](./research_uiux/UI_CODE_INDEX.md)
- [`research_uiux/PATCH_HOOK_INDEX.md`](./research_uiux/PATCH_HOOK_INDEX.md)
- [`research_uiux/CODE_TO_LAYOUT_CORRELATION.md`](./research_uiux/CODE_TO_LAYOUT_CORRELATION.md)
- [`research_uiux/PAUSE_STATUS_WORLDMAP_DEEP_DIVE.md`](./research_uiux/PAUSE_STATUS_WORLDMAP_DEEP_DIVE.md)
- [`research_uiux/BOSS_RESULT_SAVE_LOAD_DEEP_DIVE.md`](./research_uiux/BOSS_RESULT_SAVE_LOAD_DEEP_DIVE.md)
- [`research_uiux/BOSS_RESULT_SAVE_VISUAL_TAXONOMY.md`](./research_uiux/BOSS_RESULT_SAVE_VISUAL_TAXONOMY.md)
- [`research_uiux/VISUAL_ATLAS_DOCS.md`](./research_uiux/VISUAL_ATLAS_DOCS.md)
- [`research_uiux/SUBTITLE_CUTSCENE_PRESENTATION_DEEP_DIVE.md`](./research_uiux/SUBTITLE_CUTSCENE_PRESENTATION_DEEP_DIVE.md)
- [`research_uiux/CODE_BACKED_RUNTIME_IMPLEMENTATION.md`](./research_uiux/CODE_BACKED_RUNTIME_IMPLEMENTATION.md)
- [`research_uiux/TEMPLATE_PACK_FOR_ORIGINAL_PROJECTS.md`](./research_uiux/TEMPLATE_PACK_FOR_ORIGINAL_PROJECTS.md)
- [`research_uiux/UI_UX_INSPIRATION_NOTES.md`](./research_uiux/UI_UX_INSPIRATION_NOTES.md)
- [`research_uiux/WHOLE_GAME_COVERAGE_AND_GAPS.md`](./research_uiux/WHOLE_GAME_COVERAGE_AND_GAPS.md)
- [`CHANGELOG.md`](./CHANGELOG.md) for the repo timeline from initial publication to the latest research beat
- Repo policy under [`REPO_PUBLISHING_SCOPE.md`](./REPO_PUBLISHING_SCOPE.md)

## <img src="./docs/assets/branding/icon_extra.png" width="30" alt="SWARD icon"/> What Stays Local

- `UnleashedRecompLib/private/`
- `UnleashedRecompLib/ppc/`
- `UnleashedRecompLib/shader/shader_cache.*`
- `extracted_assets/`
- `external_tools/`
- `local_build_env/`
- machine-specific build trees and caches
- research reports or JSON catalogs that directly enumerate proprietary extracted content or generated translated code

> [!TIP]
> The ignore boundary for those local-only materials is enforced in [`.gitignore`](./.gitignore). If you expand the local research workspace, keep that boundary intact.

## <img src="./docs/assets/branding/icon_debug.png" width="30" alt="SWARD icon"/> Relationship to Upstream

This repository builds on the open-source work of:

- [UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp)
- [XenonRecomp](https://github.com/hedge-dev/XenonRecomp)
- [XenosRecomp](https://github.com/hedge-dev/XenosRecomp)

The goal here is different from the upstream end-user distribution goal. This repo is structured as an R&D sandbox for:

- UI/UX reverse-engineering notes
- local-only asset-backed archaeology
- publishable tooling and documentation
- transferable templates and architecture patterns

> [!TIP]
> The codebase itself consistently uses the `SWA` shorthand, which aligns with `Sonic World Adventure`. For the repository-facing identity, this workspace uses the shorter **SWARD** label.

## <img src="./docs/assets/branding/icon_extra.png" width="30" alt="SWARD icon"/> Repository Layout

```text
.
|-- UnleashedRecomp/                 # open-source runtime, UI, and patch layer
|-- UnleashedRecompLib/              # config plus local-only private/generated dirs
|-- docs/                            # build and local acquisition guidance
|-- research_uiux/
|   |-- tools/                       # research automation scripts
|   |-- UI_CODE_INDEX.md
|   |-- PATCH_HOOK_INDEX.md
|   `-- UI_UX_INSPIRATION_NOTES.md
|-- REPO_PUBLISHING_SCOPE.md
`-- .github/workflows/               # repo validation workflows
```

## <img src="./docs/assets/branding/icon_debug.png" width="30" alt="SWARD icon"/> Local Workflow

1. Clone the repository with submodules.
2. Keep private game inputs local under `UnleashedRecompLib/private/`.
3. Generate translated code and extracted assets locally only when needed.
4. Commit publishable tooling, notes, and safe code changes.
5. Keep extracted proprietary content and generated translation outputs out of git history.

Supporting docs:

- [`docs/BUILDING.md`](./docs/BUILDING.md)
- [`docs/DUMPING-en.md`](./docs/DUMPING-en.md)
- [`CHANGELOG.md`](./CHANGELOG.md)
- [`REPO_PUBLISHING_SCOPE.md`](./REPO_PUBLISHING_SCOPE.md)

## <img src="./docs/assets/branding/icon_extra.png" width="30" alt="SWARD icon"/> CI Modes

> [!NOTE]
> This repository supports two validation modes.
>
> - If private asset secrets are configured, GitHub Actions can run full asset-backed build validation.
> - If private asset secrets are not configured, CI falls back to publishable-scope validation for docs, boundaries, research tooling, and preset integrity.

That split is intentional. Public-safe automation should pass without requiring proprietary inputs, while full runtime builds remain possible in controlled environments.

## <img src="./docs/assets/branding/icon_debug.png" width="30" alt="SWARD icon"/> Contributing

Contributions are welcome for:

- research tooling
- documentation quality
- UI code and patch indexing
- CI/workflow hardening
- generic state-machine and UI/UX pattern extraction
- improvements to the open-source integration layer already present here

Do not contribute:

- extracted game assets
- private Xbox 360 content
- generated translated PPC output
- generated shader cache output
- leaked or proprietary source material

> [!IMPORTANT]
> By contributing, keep the repo publishable. If a change depends on owned game data, structure it so the code and notes can be committed while the proprietary inputs remain local.

## <img src="./docs/assets/branding/icon_debug.png" width="30" alt="SWARD icon"/> Rights and Attribution

- Sonic Unleashed, its code, art, audio, and shipped game assets belong to SEGA / Sonic Team.
- Unleashed Recompiled and its upstream open-source code remain subject to their original authorship and license notices.
- This repository preserves those upstream notices and keeps local-only proprietary materials outside published history.

See [`COPYING`](./COPYING) for the repository license text already present in this codebase.

<p align="center">
<img src="./docs/assets/branding/icon_sward.png" alt="SWARD icon"/>
</p>
