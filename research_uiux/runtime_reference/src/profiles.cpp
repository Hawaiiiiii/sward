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
    }

    return "Unknown";
}
} // namespace sward::ui_runtime
