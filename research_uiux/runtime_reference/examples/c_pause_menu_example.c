#include <sward/ui_runtime/runtime_c.h>

#include <stdio.h>

static void print_prompts(sward_ui_runtime* runtime)
{
    size_t count = sward_ui_runtime_visible_prompt_count(runtime);
    printf("Visible prompts:");
    if (count == 0)
    {
        printf(" none\n");
        return;
    }

    for (size_t i = 0; i < count; ++i)
    {
        sward_ui_prompt_view prompt;
        if (sward_ui_runtime_visible_prompt_at(runtime, i, &prompt))
            printf(" [%s: %s]", sward_ui_to_string_button(prompt.button), prompt.label);
    }

    printf("\n");
}

static void print_layers(sward_ui_runtime* runtime)
{
    size_t count = sward_ui_runtime_visible_overlay_count(runtime);
    printf("Visible layers:");
    if (count == 0)
    {
        printf(" none\n");
        return;
    }

    for (size_t i = 0; i < count; ++i)
    {
        sward_ui_overlay_view layer;
        if (sward_ui_runtime_visible_overlay_at(runtime, i, &layer))
            printf(" [%s:%s]", layer.id, layer.role);
    }

    printf("\n");
}

int main(void)
{
    sward_ui_runtime* runtime = sward_ui_runtime_create_profile(SWARD_UI_PROFILE_PAUSE_MENU);
    if (!runtime)
        return 1;

    sward_ui_runtime_set_predicate(runtime, "can_confirm", 1);
    sward_ui_runtime_set_predicate(runtime, "can_cancel", 1);
    sward_ui_runtime_set_predicate(runtime, "has_previous_tab", 1);
    sward_ui_runtime_set_predicate(runtime, "has_next_tab", 1);

    printf("Profile: %s\n", sward_ui_to_string_profile(SWARD_UI_PROFILE_PAUSE_MENU));
    sward_ui_runtime_dispatch(runtime, SWARD_UI_EVENT_RESOURCES_READY);
    printf("Scene: %s\n", sward_ui_runtime_last_scene_request(runtime));
    sward_ui_runtime_tick(runtime, 0.4);
    print_prompts(runtime);
    print_layers(runtime);

    sward_ui_runtime_request_action(runtime, SWARD_UI_ACTION_MOVE_NEXT);
    printf("State after move: %s\n", sward_ui_to_string_state(sward_ui_runtime_state(runtime)));
    sward_ui_runtime_tick(runtime, 0.4);
    print_prompts(runtime);

    sward_ui_runtime_request_action(runtime, SWARD_UI_ACTION_CANCEL);
    sward_ui_runtime_tick(runtime, 0.3);
    sward_ui_runtime_tick(runtime, 0.4);
    printf("Final state: %s\n", sward_ui_to_string_state(sward_ui_runtime_state(runtime)));

    sward_ui_runtime_destroy(runtime);
    return 0;
}
