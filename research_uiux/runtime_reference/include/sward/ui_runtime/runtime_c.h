#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum sward_ui_screen_state
{
    SWARD_UI_STATE_BOOT = 0,
    SWARD_UI_STATE_INTRO = 1,
    SWARD_UI_STATE_IDLE = 2,
    SWARD_UI_STATE_NAVIGATE = 3,
    SWARD_UI_STATE_CONFIRM = 4,
    SWARD_UI_STATE_CANCEL = 5,
    SWARD_UI_STATE_OUTRO = 6,
    SWARD_UI_STATE_CLOSED = 7,
} sward_ui_screen_state;

typedef enum sward_ui_input_action
{
    SWARD_UI_ACTION_MOVE_PREVIOUS = 0,
    SWARD_UI_ACTION_MOVE_NEXT = 1,
    SWARD_UI_ACTION_PAGE_LEFT = 2,
    SWARD_UI_ACTION_PAGE_RIGHT = 3,
    SWARD_UI_ACTION_CONFIRM = 4,
    SWARD_UI_ACTION_CANCEL = 5,
} sward_ui_input_action;

typedef enum sward_ui_runtime_event
{
    SWARD_UI_EVENT_RESOURCES_READY = 0,
    SWARD_UI_EVENT_ANIMATION_FINISHED = 1,
    SWARD_UI_EVENT_ACTION_COMPLETED_KEEP_OPEN = 2,
    SWARD_UI_EVENT_ACTION_COMPLETED_CLOSE = 3,
    SWARD_UI_EVENT_HOST_FORCE_CLOSE = 4,
    SWARD_UI_EVENT_TIMEOUT = 5,
} sward_ui_runtime_event;

typedef enum sward_ui_prompt_button
{
    SWARD_UI_BUTTON_A = 0,
    SWARD_UI_BUTTON_B = 1,
    SWARD_UI_BUTTON_X = 2,
    SWARD_UI_BUTTON_Y = 3,
    SWARD_UI_BUTTON_LB = 4,
    SWARD_UI_BUTTON_RB = 5,
    SWARD_UI_BUTTON_START = 6,
    SWARD_UI_BUTTON_UNKNOWN = 7,
} sward_ui_prompt_button;

typedef enum sward_ui_profile_id
{
    SWARD_UI_PROFILE_PAUSE_MENU = 0,
    SWARD_UI_PROFILE_TITLE_MENU = 1,
    SWARD_UI_PROFILE_AUTOSAVE_TOAST = 2,
    SWARD_UI_PROFILE_LOADING_TRANSITION = 3,
    SWARD_UI_PROFILE_MISSION_RESULT = 4,
    SWARD_UI_PROFILE_SUBTITLE_CUTSCENE = 5,
    SWARD_UI_PROFILE_WORLD_MAP = 6,
    SWARD_UI_PROFILE_SONIC_STAGE_HUD = 7,
    SWARD_UI_PROFILE_WEREHOG_STAGE_HUD = 8,
    SWARD_UI_PROFILE_EXTRA_STAGE_HUD = 9,
    SWARD_UI_PROFILE_SUPER_SONIC_HUD = 10,
    SWARD_UI_PROFILE_BOSS_HUD = 11,
    SWARD_UI_PROFILE_TOWN_UI = 12,
    SWARD_UI_PROFILE_CAMERA_SHELL = 13,
    SWARD_UI_PROFILE_APPLICATION_WORLD_SHELL = 14,
    SWARD_UI_PROFILE_FRONTEND_SEQUENCE_SHELL = 15,
} sward_ui_profile_id;

typedef struct sward_ui_prompt_view
{
    const char* slot_id;
    sward_ui_prompt_button button;
    const char* label;
} sward_ui_prompt_view;

typedef struct sward_ui_overlay_view
{
    const char* id;
    const char* role;
    int interactive;
} sward_ui_overlay_view;

typedef struct sward_ui_runtime sward_ui_runtime;

sward_ui_runtime* sward_ui_runtime_create_profile(sward_ui_profile_id profile_id);
sward_ui_runtime* sward_ui_runtime_create_contract_path(const char* contract_path);
void sward_ui_runtime_destroy(sward_ui_runtime* runtime);

const char* sward_ui_runtime_screen_id(const sward_ui_runtime* runtime);
const char* sward_ui_runtime_last_scene_request(const sward_ui_runtime* runtime);
void sward_ui_runtime_clear_last_scene_request(sward_ui_runtime* runtime);

sward_ui_screen_state sward_ui_runtime_state(const sward_ui_runtime* runtime);
double sward_ui_runtime_state_elapsed_seconds(const sward_ui_runtime* runtime);
int sward_ui_runtime_is_input_locked(const sward_ui_runtime* runtime);

int sward_ui_runtime_request_action(sward_ui_runtime* runtime, sward_ui_input_action action);
int sward_ui_runtime_dispatch(sward_ui_runtime* runtime, sward_ui_runtime_event event);
void sward_ui_runtime_tick(sward_ui_runtime* runtime, double delta_seconds);
void sward_ui_runtime_set_predicate(sward_ui_runtime* runtime, const char* predicate_id, int value);

size_t sward_ui_runtime_visible_prompt_count(sward_ui_runtime* runtime);
int sward_ui_runtime_visible_prompt_at(sward_ui_runtime* runtime, size_t index, sward_ui_prompt_view* out_prompt);
size_t sward_ui_runtime_visible_overlay_count(sward_ui_runtime* runtime);
int sward_ui_runtime_visible_overlay_at(sward_ui_runtime* runtime, size_t index, sward_ui_overlay_view* out_overlay);

const char* sward_ui_to_string_state(sward_ui_screen_state state);
const char* sward_ui_to_string_action(sward_ui_input_action action);
const char* sward_ui_to_string_button(sward_ui_prompt_button button);
const char* sward_ui_to_string_profile(sward_ui_profile_id profile_id);

#ifdef __cplusplus
}
#endif
