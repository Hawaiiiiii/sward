<p align="right">
    <img src="../../../docs/assets/branding/icon_sward.png" width="116" alt="SWARD icon"/>
</p>

# <img src="../../../docs/assets/branding/icon_sward.png" width="34" alt="SWARD icon"/> C# Reference Port

This directory contains the managed Phase 24/27/37/39 reference port for the SWARD runtime concepts.

It is intentionally self-contained:

- no extracted assets
- no P/Invoke boundary
- no dependence on translated PPC output
- shared JSON contract loading with the native runtime layer

Build locally with the staged repo-local `.NET 8` runtime:

```powershell
external_tools/dotnet8/dotnet.exe build research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release
external_tools/dotnet8/dotnet.exe run --project research_uiux/runtime_reference/csharp_reference/Sward.UiRuntime.Reference.csproj -c Release
```

Included profiles:

- `PauseMenuReference`
- `TitleMenuReference`
- `AutosaveToastReference`
- `LoadingTransitionReference`
- `MissionResultReference`
- `SonicStageHudReference`
- `WerehogStageHudReference`
- `ExtraStageHudReference`
- `SuperSonicHudReference`
- `BossHudReference`
- `SubtitleCutsceneReference`
- `WorldMapReference`

Portable contract notes:

- bundled contract JSON files are copied to `bin/Release/net8.0/contracts/`
- `ReferenceProfiles.Load(...)` uses those copied JSON files
- `ContractLoader.LoadFromFile(...)` can still load any explicit contract path directly when you do not want to go through the bundled profile enum
