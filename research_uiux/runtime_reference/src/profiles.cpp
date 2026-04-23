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
    }

    return "Unknown";
}
} // namespace sward::ui_runtime
