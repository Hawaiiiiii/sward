#include <sward/ui_runtime/contract_loader.hpp>
#include <sward/ui_runtime/profiles.hpp>

#include <stdexcept>

namespace sward::ui_runtime
{
ScreenContract makePauseMenuContract()
{
    return loadBundledContract(ReferenceProfile::PauseMenu);
}

ScreenContract makeTitleMenuContract()
{
    return loadBundledContract(ReferenceProfile::TitleMenu);
}

ScreenContract makeAutosaveToastContract()
{
    return loadBundledContract(ReferenceProfile::AutosaveToast);
}

ScreenContract makeLoadingTransitionContract()
{
    return loadBundledContract(ReferenceProfile::LoadingTransition);
}

ScreenContract makeMissionResultContract()
{
    return loadBundledContract(ReferenceProfile::MissionResult);
}

ScreenContract makeSubtitleCutsceneContract()
{
    return loadBundledContract(ReferenceProfile::SubtitleCutscene);
}

ScreenContract makeWorldMapContract()
{
    return loadBundledContract(ReferenceProfile::WorldMap);
}

ScreenContract makeSonicStageHudContract()
{
    return loadBundledContract(ReferenceProfile::SonicStageHud);
}

ScreenContract makeWerehogStageHudContract()
{
    return loadBundledContract(ReferenceProfile::WerehogStageHud);
}

ScreenContract makeExtraStageHudContract()
{
    return loadBundledContract(ReferenceProfile::ExtraStageHud);
}

ScreenContract makeSuperSonicHudContract()
{
    return loadBundledContract(ReferenceProfile::SuperSonicHud);
}

ScreenContract makeBossHudContract()
{
    return loadBundledContract(ReferenceProfile::BossHud);
}

ScreenContract makeTownUiContract()
{
    return loadBundledContract(ReferenceProfile::TownUi);
}

ScreenContract makeCameraShellContract()
{
    return loadBundledContract(ReferenceProfile::CameraShell);
}

ScreenContract makeApplicationWorldShellContract()
{
    return loadBundledContract(ReferenceProfile::ApplicationWorldShell);
}

ScreenContract makeFrontendSequenceShellContract()
{
    return loadBundledContract(ReferenceProfile::FrontendSequenceShell);
}

ScreenContract makeAchievementUnlockSupportContract()
{
    return loadBundledContract(ReferenceProfile::AchievementUnlockSupport);
}

ScreenContract makeAudioCueSupportContract()
{
    return loadBundledContract(ReferenceProfile::AudioCueSupport);
}

ScreenContract makeXmlDataLoadingSupportContract()
{
    return loadBundledContract(ReferenceProfile::XmlDataLoadingSupport);
}

ScreenContract makeContract(ReferenceProfile profile)
{
    return loadBundledContract(profile);
}

std::string_view toString(ReferenceProfile profile)
{
    switch (profile)
    {
    case ReferenceProfile::PauseMenu:
        return "PauseMenu";
    case ReferenceProfile::TitleMenu:
        return "TitleMenu";
    case ReferenceProfile::AutosaveToast:
        return "AutosaveToast";
    case ReferenceProfile::LoadingTransition:
        return "LoadingTransition";
    case ReferenceProfile::MissionResult:
        return "MissionResult";
    case ReferenceProfile::SubtitleCutscene:
        return "SubtitleCutscene";
    case ReferenceProfile::WorldMap:
        return "WorldMap";
    case ReferenceProfile::SonicStageHud:
        return "SonicStageHud";
    case ReferenceProfile::WerehogStageHud:
        return "WerehogStageHud";
    case ReferenceProfile::ExtraStageHud:
        return "ExtraStageHud";
    case ReferenceProfile::SuperSonicHud:
        return "SuperSonicHud";
    case ReferenceProfile::BossHud:
        return "BossHud";
    case ReferenceProfile::TownUi:
        return "TownUi";
    case ReferenceProfile::CameraShell:
        return "CameraShell";
    case ReferenceProfile::ApplicationWorldShell:
        return "ApplicationWorldShell";
    case ReferenceProfile::FrontendSequenceShell:
        return "FrontendSequenceShell";
    case ReferenceProfile::AchievementUnlockSupport:
        return "AchievementUnlockSupport";
    case ReferenceProfile::AudioCueSupport:
        return "AudioCueSupport";
    case ReferenceProfile::XmlDataLoadingSupport:
        return "XmlDataLoadingSupport";
    }

    return "Unknown";
}
} // namespace sward::ui_runtime
