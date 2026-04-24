<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Local Debug-Oriented Source Tree Expansion

This pass widens the local-only readable source layer from the first menu/cutscene hosts into gameplay HUD, stage-test, town, camera, sequence, application/world, and support-substrate shell paths.

> [!IMPORTANT]
> These files stay local-only under `SONIC UNLEASHED/`. The tracked repo carries the materializer plus the summary, not the mirrored files themselves.

## Snapshot

- Materialized local-only readable `.cpp` files in this pass: `116`
- Total local-only readable `.cpp` files under `SONIC UNLEASHED/`: `125`
- Expansion groups added: `7`

## Group Breakdown

| Group | Files | Purpose |
|---|---:|---|
| Gameplay HUD Sources | `13` | Readable local-only HUD ownership scaffolds for Sonic, Werehog, EX-stage, and boss/final HUD families. |
| Stage Test Sources | `9` | Readable local-only stage-test probes that point the debug game modes at current runtime contracts. |
| Town / Media Room Sources | `19` | Readable local-only town, talk, shop, and media-room scaffolds tied back to extracted town layouts. |
| Camera Shell Sources | `8` | Readable local-only camera scaffolds for replay, goal, and free-camera ownership around the UI stack. |
| Frontend Sequence Sources | `12` | Readable local-only sequence-core and sequence-unit scaffolds tied back to the new frontend sequence shell and its downstream runtime families. |
| Application / World Shell Sources | `32` | Readable local-only system-shell scaffolds linking application/world/stage hosts back to current runtime families. |
| Support Substrate Sources | `23` | Readable local-only support-substrate scaffolds for achievement unlocks, animation event triggers, player-status feeds, audio routing, and XML/data loading. |

## What Changed

- Gameplay HUD hosts now have local-only readable ownership scaffolds tied to the new runtime contracts.
- Stage-test game modes now read like probe hosts instead of only path-dump notes.
- Town/media-room, camera, sequence, and application/world shells now carry readable ownership bindings instead of remaining note-only placeholders.
- Phase 47 support-substrate paths now have local-only readable bindings for achievement unlocks, animation event triggers, player-status feeds, audio routing, and XML/data loading.
- The mirrored local source tree is materially closer to a debug-oriented source layout instead of a note staging area.
