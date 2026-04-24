<p align="right">
    <img src="../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> Support-Substrate Runtime Contracts

Phase 50 promotes the Phase 49 support-substrate scaffolds into runtime-backed diagnostic surfaces. The goal is to keep moving from source-path organization toward an `.exe` workbench that can exercise recovered UI/UX behavior.

> [!IMPORTANT]
> These are portable diagnostic contracts. They encode state, timing, overlay-role, prompt, and launch behavior. They do not include extracted assets or original/proprietary source.

## New Contracts

| Contract | Source system | Workbench anchors |
|---|---|---|
| `achievement_unlock_support_reference.json` | `achievement_unlock_support` | `AchievementManager.cpp` |
| `audio_cue_support_reference.json` | `audio_cue_support` | `Sound.cpp`, `SoundController.cpp`, `SoundPlayer.cpp`, BGM route files |
| `xml_data_loading_support_reference.json` | `xml_data_loading_support` | `XMLManager.cpp`, `XMLDocument.cpp`, XML type loaders |

## Runtime Surface

- Bundled native/C++ profiles now include `AchievementUnlockSupport`, `AudioCueSupport`, and `XmlDataLoadingSupport`.
- C ABI profile IDs now expose the same three support profiles.
- The C# reference profile enum and bundled contract path mapper now expose the same three contracts.
- The source-family selector now has `19` launch families, adding achievement, audio/BGM, and XML/data-loading support entries.
- The debug workbench now has `176` hosts across `11` groups, including `17` support-substrate hosts.

## Coverage Shift

The current `269`-path curated seed keeps `0` `named_seed_only` entries. Runtime-contract-backed paths rise from `186 / 269` to `203 / 269` (`75.5%`) because achievement, sound/BGM, and XML/data-loading support paths are no longer only archaeology-mapped.

## Why This Matters

Achievement unlocks, audio cues, BGM route changes, and XML-backed data loading are not primary UI screens, but they feed visible UI behavior and transition timing. Making them workbench-launchable gives the executable a better substrate for future 1:1-style UI playback instead of leaving those paths as disconnected support notes.
