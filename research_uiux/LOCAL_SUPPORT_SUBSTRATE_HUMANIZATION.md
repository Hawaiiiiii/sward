<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Local Support-Substrate Humanization

Phase 49 turns the Phase 47 support-substrate seed from classified source paths into local-only readable `.cpp` scaffolds under `SONIC UNLEASHED/`.

> [!IMPORTANT]
> The generated mirror files remain local-only. This tracked report records the support group, counts, and generator behavior without publishing `SONIC UNLEASHED/`.

## Snapshot

- Added support group: `support_substrate_sources`
- Group display name: `Support Substrate Sources`
- Local-only support scaffolds added: `23`
- Materialized local-only readable `.cpp` files in the generator pass: `116`
- Total local-only readable `.cpp` files under `SONIC UNLEASHED/`: `125`
- Expansion groups in the source-tree materializer: `7`

## Support Families

| Family | Paths | Runtime adjacency |
|---|---:|---|
| Achievement / Unlock Support | `1` | Frontend sequence unlock dispatch |
| Timeline Event Trigger Support | `4` | Subtitle/cutscene and Inspire preview playback |
| Player Status / Switch Support | `2` | Sonic, Werehog, and Super Sonic HUD contracts |
| Audio Cue / BGM Support | `10` | Stage, town, timeline, and HUD presentation routes |
| XML / Data Loading Support | `6` | Stage loader and future UI/resource binding probes |

## Local-Only Paths Added

- `Achievement/AchievementManager.cpp`
- `Animation/EventTrigger/AnimationEventTriggerContainer.cpp`
- `Animation/EventTrigger/Event/AnimationEventTriggerAudio.cpp`
- `Animation/EventTrigger/Event/AnimationEventTriggerSparkle.cpp`
- `Animation/EventTrigger/Event/AnimationEventTriggerVibration.cpp`
- `Player/Parameter/PlayerParameter.cpp`
- `Player/Switch/PlayerSwitchManager.cpp`
- `Sound/Sound.cpp`
- `Sound/SoundBGMActEggman.cpp`
- `Sound/SoundBGMActEvil.cpp`
- `Sound/SoundBGMActSonic.cpp`
- `Sound/SoundBGMDispel.cpp`
- `Sound/SoundBGMExtra.cpp`
- `Sound/SoundBGMStandard.cpp`
- `Sound/SoundBGMTown.cpp`
- `Sound/SoundController.cpp`
- `Sound/SoundPlayer.cpp`
- `XML/XMLBinData.cpp`
- `XML/XMLDocument.cpp`
- `XML/XMLManager.cpp`
- `XML/XMLNode.cpp`
- `XML/XMLTypeSLBin.cpp`
- `XML/XMLTypeSLTxt.cpp`

## Why This Matters

The support-substrate paths are not screens by themselves, but they feed the visible runtime: achievement unlock dispatch, timeline-triggered feedback, HUD status inputs, audio/BGM timing, and XML-backed stage/resource loading. Giving them local-only readable scaffolds makes the mirror a better debug-oriented source base instead of leaving these paths as classified names only.
