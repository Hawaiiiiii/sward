#include <sward/ui_runtime/contract_loader.hpp>
#include <sward/ui_runtime/profiles.hpp>
#include <sward/ui_runtime/runtime_c.h>

#include <string>
#include <utility>
#include <vector>

using namespace sward::ui_runtime;

struct sward_ui_runtime
{
    explicit sward_ui_runtime(ScreenContract contract, ReferenceProfile profile)
        : runtime(
            std::move(contract),
            RuntimeCallbacks{
                .onSceneRequested = [this](std::string_view scene) { lastScene = std::string(scene); },
            })
        , profile(profile)
    {
    }

    ScreenRuntime runtime;
    ReferenceProfile profile;
    std::string lastScene;
    std::vector<PromptSlotView> promptScratch;
    std::vector<OverlayLayer> overlayScratch;
};

namespace
{
ReferenceProfile fromCProfile(sward_ui_profile_id profileId)
{
    switch (profileId)
    {
    case SWARD_UI_PROFILE_PAUSE_MENU:
        return ReferenceProfile::PauseMenu;
    case SWARD_UI_PROFILE_TITLE_MENU:
        return ReferenceProfile::TitleMenu;
    case SWARD_UI_PROFILE_AUTOSAVE_TOAST:
        return ReferenceProfile::AutosaveToast;
    case SWARD_UI_PROFILE_LOADING_TRANSITION:
        return ReferenceProfile::LoadingTransition;
    case SWARD_UI_PROFILE_MISSION_RESULT:
        return ReferenceProfile::MissionResult;
    case SWARD_UI_PROFILE_SUBTITLE_CUTSCENE:
        return ReferenceProfile::SubtitleCutscene;
    case SWARD_UI_PROFILE_WORLD_MAP:
        return ReferenceProfile::WorldMap;
    case SWARD_UI_PROFILE_SONIC_STAGE_HUD:
        return ReferenceProfile::SonicStageHud;
    case SWARD_UI_PROFILE_WEREHOG_STAGE_HUD:
        return ReferenceProfile::WerehogStageHud;
    case SWARD_UI_PROFILE_EXTRA_STAGE_HUD:
        return ReferenceProfile::ExtraStageHud;
    case SWARD_UI_PROFILE_SUPER_SONIC_HUD:
        return ReferenceProfile::SuperSonicHud;
    case SWARD_UI_PROFILE_BOSS_HUD:
        return ReferenceProfile::BossHud;
    case SWARD_UI_PROFILE_TOWN_UI:
        return ReferenceProfile::TownUi;
    case SWARD_UI_PROFILE_CAMERA_SHELL:
        return ReferenceProfile::CameraShell;
    case SWARD_UI_PROFILE_APPLICATION_WORLD_SHELL:
        return ReferenceProfile::ApplicationWorldShell;
    case SWARD_UI_PROFILE_FRONTEND_SEQUENCE_SHELL:
        return ReferenceProfile::FrontendSequenceShell;
    }

    return ReferenceProfile::PauseMenu;
}

sward_ui_screen_state toCState(ScreenState state)
{
    switch (state)
    {
    case ScreenState::Boot:
        return SWARD_UI_STATE_BOOT;
    case ScreenState::Intro:
        return SWARD_UI_STATE_INTRO;
    case ScreenState::Idle:
        return SWARD_UI_STATE_IDLE;
    case ScreenState::Navigate:
        return SWARD_UI_STATE_NAVIGATE;
    case ScreenState::Confirm:
        return SWARD_UI_STATE_CONFIRM;
    case ScreenState::Cancel:
        return SWARD_UI_STATE_CANCEL;
    case ScreenState::Outro:
        return SWARD_UI_STATE_OUTRO;
    case ScreenState::Closed:
        return SWARD_UI_STATE_CLOSED;
    }

    return SWARD_UI_STATE_BOOT;
}

InputAction fromCAction(sward_ui_input_action action)
{
    switch (action)
    {
    case SWARD_UI_ACTION_MOVE_PREVIOUS:
        return InputAction::MovePrevious;
    case SWARD_UI_ACTION_MOVE_NEXT:
        return InputAction::MoveNext;
    case SWARD_UI_ACTION_PAGE_LEFT:
        return InputAction::PageLeft;
    case SWARD_UI_ACTION_PAGE_RIGHT:
        return InputAction::PageRight;
    case SWARD_UI_ACTION_CONFIRM:
        return InputAction::Confirm;
    case SWARD_UI_ACTION_CANCEL:
        return InputAction::Cancel;
    }

    return InputAction::Cancel;
}

RuntimeEventType fromCEvent(sward_ui_runtime_event event)
{
    switch (event)
    {
    case SWARD_UI_EVENT_RESOURCES_READY:
        return RuntimeEventType::ResourcesReady;
    case SWARD_UI_EVENT_ANIMATION_FINISHED:
        return RuntimeEventType::AnimationFinished;
    case SWARD_UI_EVENT_ACTION_COMPLETED_KEEP_OPEN:
        return RuntimeEventType::ActionCompletedKeepOpen;
    case SWARD_UI_EVENT_ACTION_COMPLETED_CLOSE:
        return RuntimeEventType::ActionCompletedClose;
    case SWARD_UI_EVENT_HOST_FORCE_CLOSE:
        return RuntimeEventType::HostForceClose;
    case SWARD_UI_EVENT_TIMEOUT:
        return RuntimeEventType::Timeout;
    }

    return RuntimeEventType::Timeout;
}

sward_ui_prompt_button toCButton(PromptButton button)
{
    switch (button)
    {
    case PromptButton::A:
        return SWARD_UI_BUTTON_A;
    case PromptButton::B:
        return SWARD_UI_BUTTON_B;
    case PromptButton::X:
        return SWARD_UI_BUTTON_X;
    case PromptButton::Y:
        return SWARD_UI_BUTTON_Y;
    case PromptButton::LB:
        return SWARD_UI_BUTTON_LB;
    case PromptButton::RB:
        return SWARD_UI_BUTTON_RB;
    case PromptButton::Start:
        return SWARD_UI_BUTTON_START;
    case PromptButton::Unknown:
        return SWARD_UI_BUTTON_UNKNOWN;
    }

    return SWARD_UI_BUTTON_UNKNOWN;
}

void refreshPrompts(sward_ui_runtime* runtime)
{
    runtime->promptScratch = runtime->runtime.visiblePrompts();
}

void refreshOverlays(sward_ui_runtime* runtime)
{
    runtime->overlayScratch = runtime->runtime.visibleLayers();
}
} // namespace

extern "C"
{
sward_ui_runtime* sward_ui_runtime_create_profile(sward_ui_profile_id profile_id)
{
    try
    {
        ReferenceProfile profile = fromCProfile(profile_id);
        return new sward_ui_runtime(loadBundledContract(profile), profile);
    }
    catch (...)
    {
        return nullptr;
    }
}

sward_ui_runtime* sward_ui_runtime_create_contract_path(const char* contract_path)
{
    if (!contract_path)
        return nullptr;

    try
    {
        return new sward_ui_runtime(loadContractFromJsonFile(contract_path), ReferenceProfile::PauseMenu);
    }
    catch (...)
    {
        return nullptr;
    }
}

void sward_ui_runtime_destroy(sward_ui_runtime* runtime)
{
    delete runtime;
}

const char* sward_ui_runtime_screen_id(const sward_ui_runtime* runtime)
{
    return runtime ? runtime->runtime.screenId().c_str() : "";
}

const char* sward_ui_runtime_last_scene_request(const sward_ui_runtime* runtime)
{
    return runtime ? runtime->lastScene.c_str() : "";
}

void sward_ui_runtime_clear_last_scene_request(sward_ui_runtime* runtime)
{
    if (runtime)
        runtime->lastScene.clear();
}

sward_ui_screen_state sward_ui_runtime_state(const sward_ui_runtime* runtime)
{
    return runtime ? toCState(runtime->runtime.state()) : SWARD_UI_STATE_BOOT;
}

double sward_ui_runtime_state_elapsed_seconds(const sward_ui_runtime* runtime)
{
    return runtime ? runtime->runtime.stateElapsedSeconds() : 0.0;
}

int sward_ui_runtime_is_input_locked(const sward_ui_runtime* runtime)
{
    return runtime ? (runtime->runtime.isInputLocked() ? 1 : 0) : 1;
}

int sward_ui_runtime_request_action(sward_ui_runtime* runtime, sward_ui_input_action action)
{
    return runtime && runtime->runtime.requestAction(fromCAction(action)) ? 1 : 0;
}

int sward_ui_runtime_dispatch(sward_ui_runtime* runtime, sward_ui_runtime_event event)
{
    return runtime && runtime->runtime.dispatch(fromCEvent(event)) ? 1 : 0;
}

void sward_ui_runtime_tick(sward_ui_runtime* runtime, double delta_seconds)
{
    if (runtime)
        runtime->runtime.tick(delta_seconds);
}

void sward_ui_runtime_set_predicate(sward_ui_runtime* runtime, const char* predicate_id, int value)
{
    if (runtime && predicate_id)
        runtime->runtime.setPredicate(predicate_id, value != 0);
}

size_t sward_ui_runtime_visible_prompt_count(sward_ui_runtime* runtime)
{
    if (!runtime)
        return 0;

    refreshPrompts(runtime);
    return runtime->promptScratch.size();
}

int sward_ui_runtime_visible_prompt_at(sward_ui_runtime* runtime, size_t index, sward_ui_prompt_view* out_prompt)
{
    if (!runtime || !out_prompt)
        return 0;

    refreshPrompts(runtime);
    if (index >= runtime->promptScratch.size())
        return 0;

    const auto& prompt = runtime->promptScratch[index];
    out_prompt->slot_id = prompt.slotId.c_str();
    out_prompt->button = toCButton(prompt.button);
    out_prompt->label = prompt.label.c_str();
    return 1;
}

size_t sward_ui_runtime_visible_overlay_count(sward_ui_runtime* runtime)
{
    if (!runtime)
        return 0;

    refreshOverlays(runtime);
    return runtime->overlayScratch.size();
}

int sward_ui_runtime_visible_overlay_at(sward_ui_runtime* runtime, size_t index, sward_ui_overlay_view* out_overlay)
{
    if (!runtime || !out_overlay)
        return 0;

    refreshOverlays(runtime);
    if (index >= runtime->overlayScratch.size())
        return 0;

    const auto& layer = runtime->overlayScratch[index];
    out_overlay->id = layer.id.c_str();
    out_overlay->role = layer.role.c_str();
    out_overlay->interactive = layer.interactive ? 1 : 0;
    return 1;
}

const char* sward_ui_to_string_state(sward_ui_screen_state state)
{
    switch (state)
    {
    case SWARD_UI_STATE_BOOT:
        return "Boot";
    case SWARD_UI_STATE_INTRO:
        return "Intro";
    case SWARD_UI_STATE_IDLE:
        return "Idle";
    case SWARD_UI_STATE_NAVIGATE:
        return "Navigate";
    case SWARD_UI_STATE_CONFIRM:
        return "Confirm";
    case SWARD_UI_STATE_CANCEL:
        return "Cancel";
    case SWARD_UI_STATE_OUTRO:
        return "Outro";
    case SWARD_UI_STATE_CLOSED:
        return "Closed";
    }

    return "Unknown";
}

const char* sward_ui_to_string_action(sward_ui_input_action action)
{
    switch (action)
    {
    case SWARD_UI_ACTION_MOVE_PREVIOUS:
        return "MovePrevious";
    case SWARD_UI_ACTION_MOVE_NEXT:
        return "MoveNext";
    case SWARD_UI_ACTION_PAGE_LEFT:
        return "PageLeft";
    case SWARD_UI_ACTION_PAGE_RIGHT:
        return "PageRight";
    case SWARD_UI_ACTION_CONFIRM:
        return "Confirm";
    case SWARD_UI_ACTION_CANCEL:
        return "Cancel";
    }

    return "Unknown";
}

const char* sward_ui_to_string_button(sward_ui_prompt_button button)
{
    switch (button)
    {
    case SWARD_UI_BUTTON_A:
        return "A";
    case SWARD_UI_BUTTON_B:
        return "B";
    case SWARD_UI_BUTTON_X:
        return "X";
    case SWARD_UI_BUTTON_Y:
        return "Y";
    case SWARD_UI_BUTTON_LB:
        return "LB";
    case SWARD_UI_BUTTON_RB:
        return "RB";
    case SWARD_UI_BUTTON_START:
        return "Start";
    case SWARD_UI_BUTTON_UNKNOWN:
        return "Unknown";
    }

    return "Unknown";
}

const char* sward_ui_to_string_profile(sward_ui_profile_id profile_id)
{
    switch (profile_id)
    {
    case SWARD_UI_PROFILE_PAUSE_MENU:
        return "PauseMenu";
    case SWARD_UI_PROFILE_TITLE_MENU:
        return "TitleMenu";
    case SWARD_UI_PROFILE_AUTOSAVE_TOAST:
        return "AutosaveToast";
    case SWARD_UI_PROFILE_LOADING_TRANSITION:
        return "LoadingTransition";
    case SWARD_UI_PROFILE_MISSION_RESULT:
        return "MissionResult";
    case SWARD_UI_PROFILE_SUBTITLE_CUTSCENE:
        return "SubtitleCutscene";
    case SWARD_UI_PROFILE_WORLD_MAP:
        return "WorldMap";
    case SWARD_UI_PROFILE_SONIC_STAGE_HUD:
        return "SonicStageHud";
    case SWARD_UI_PROFILE_WEREHOG_STAGE_HUD:
        return "WerehogStageHud";
    case SWARD_UI_PROFILE_EXTRA_STAGE_HUD:
        return "ExtraStageHud";
    case SWARD_UI_PROFILE_SUPER_SONIC_HUD:
        return "SuperSonicHud";
    case SWARD_UI_PROFILE_BOSS_HUD:
        return "BossHud";
    case SWARD_UI_PROFILE_TOWN_UI:
        return "TownUi";
    case SWARD_UI_PROFILE_CAMERA_SHELL:
        return "CameraShell";
    case SWARD_UI_PROFILE_APPLICATION_WORLD_SHELL:
        return "ApplicationWorldShell";
    case SWARD_UI_PROFILE_FRONTEND_SEQUENCE_SHELL:
        return "FrontendSequenceShell";
    }

    return "Unknown";
}
}
