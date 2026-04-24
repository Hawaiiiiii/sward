<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> GUI Layout Evidence Preview Overlay

Phase 57 moves the native GUI workbench one step closer to decoded layout playback:

```text
b/rr57/sward_ui_runtime_debug_gui.exe
```

The visual preview now surfaces compact parsed-layout evidence for high-confidence atlas-backed families instead of only showing the runtime contract layers.

## What Changed

- added a `LayoutEvidence` table keyed by bundled contract file name
- added `layoutEvidenceForContract(...)` and `drawLayoutEvidenceOverlay(...)` to the native GUI preview path
- draws a compact evidence panel over Title, Pause, and Loading previews with layout ID, correlation verdict, scene count, animation count, cast/subimage counts where known, scene cues, animation cues, and longest parsed timeline
- extended the preview footer so it reports the active recovered layout ID next to atlas, family, state, playback, and prompt counts
- added `--layout-evidence-smoke` so automation can verify the decoded layout facts without opening a window
- added `previewLayerFillAlpha(...)` plus `--layer-fill-smoke` so structural backdrop and cinematic-frame roles preserve the atlas underneath instead of tint-filling over it

## Evidence Bound

| Contract | Layout evidence | Scenes | Animations | Cast dictionaries | Subimages | Longest parsed timeline |
|---|---|---:|---:|---:|---:|---|
| `title_menu_reference.json` | `ui_mainmenu` (`strong`) | `16` | `6` | `0` | `0` | `mm_donut_move/DefaultAnim: 220f @ 60fps` |
| `pause_menu_reference.json` | `ui_pause` (`direct`) | `29` | `41` | `260` | `2871` | `btn_effect/charge_3_Outro: 240f @ 60fps` |
| `loading_transition_reference.json` | `ui_loading` (`direct`) | `7` | `37` | `331` | `2240` | `pda_txt/Usual_Anim_3: 240f @ 60fps` |

The table is derived from the existing extracted-layout reports and machine-readable correlation layer:

- [`research_uiux/XNCP_YNCP_GRAPH_AND_TIMELINE_NOTES.md`](./XNCP_YNCP_GRAPH_AND_TIMELINE_NOTES.md)
- [`research_uiux/CODE_TO_LAYOUT_CORRELATION.md`](./CODE_TO_LAYOUT_CORRELATION.md)
- [`research_uiux/data/layout_code_correlation.json`](./data/layout_code_correlation.json)

## Why This Matters

Phase 56 gave Title, Pause, and Loading their own preview placement adapters. Phase 57 makes those previews evidence-aware by putting parsed `.xncp` / `.yncp` facts in the same surface as atlas drawing, contract state, prompts, and runtime timeline playback.

That is not the same as decoded node rendering yet. It is the bridge needed before the renderer can stop using schematic role rectangles and start consuming recovered scene nodes, CSD-style animation bands, and exact family-specific payloads.

## Verification

```powershell
cmd /c "call ... vcvars64.bat && cmake -S research_uiux/runtime_reference -B b/rr57 -G \"NMake Makefiles\" -DCMAKE_BUILD_TYPE=Release && cmake --build b/rr57 --config Release"

python research_uiux\runtime_reference\examples\test_ui_debug_workbench_gui.py

b\rr57\sward_ui_runtime_debug_gui.exe --layer-fill-smoke
b\rr57\sward_ui_runtime_debug_gui.exe --layout-evidence-smoke
b\rr57\sward_ui_runtime_debug_gui.exe --family-preview-smoke
b\rr57\sward_ui_runtime_debug_gui.exe --motion-smoke
b\rr57\sward_ui_runtime_debug_gui.exe --playback-smoke
b\rr57\sward_ui_runtime_debug_gui.exe --preview-smoke
b\rr57\sward_ui_runtime_debug_gui.exe --smoke
```

Verified smoke output:

```text
sward_ui_runtime_debug_gui layer fill smoke ok backdrop_alpha=0 cinematic_alpha=0 content_alpha=0.58
sward_ui_runtime_debug_gui layout evidence smoke ok title=ui_mainmenu scenes=16 animations=6 pause=ui_pause scenes=29 animations=41 loading=ui_loading scenes=7 animations=37
sward_ui_runtime_debug_gui family preview smoke ok title=title_menu pause=pause_menu loading=loading_transition title_logo_y=64.8 title_content_y=331.2 pause_chrome_w=844.8 loading_frame_y=187.2
sward_ui_runtime_debug_gui motion smoke ok intro_alpha=0.28 intro_offset_x=-204.8 intro_end_alpha=1 idle_alpha=1 outro_alpha=0.32
sward_ui_runtime_debug_gui playback smoke ok intro=Intro after_intro=Idle action=Navigate after_action=Idle
sward_ui_runtime_debug_gui preview smoke ok atlas_candidates=10 proxy_candidates=2 existing_local_atlas=10 title=mainmenu__ui_mainmenu.png pause=systemcommoncore__ui_pause.png sonic_stage=exstagetails_common__ui_prov_playscreen.png
sward_ui_runtime_debug_gui smoke ok contracts=19 hosts=176 groups=11 support_hosts=17
```

The GUI was also launch/capture-checked by selecting `Menu / Stage Debug Hosts -> GameModeMainMenu_Test.cpp`, pressing `Run Host`, pausing playback, and capturing the rr57 window with the title-menu atlas, layer projection, timeline footer, and `ui_mainmenu` evidence overlay visible.
