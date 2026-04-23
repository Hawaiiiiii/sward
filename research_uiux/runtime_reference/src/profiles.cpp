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
    }

    return "Unknown";
}
} // namespace sward::ui_runtime
