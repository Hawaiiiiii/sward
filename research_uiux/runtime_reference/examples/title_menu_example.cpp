#include <sward/ui_runtime/profiles.hpp>
#include <sward/ui_runtime/runtime.hpp>

#include <iostream>

using namespace sward::ui_runtime;

namespace
{
void printVisiblePrompts(const ScreenRuntime& runtime)
{
    std::cout << "Visible prompts:";
    auto prompts = runtime.visiblePrompts();
    if (prompts.empty())
    {
        std::cout << " none\n";
        return;
    }

    for (const auto& prompt : prompts)
        std::cout << " [" << toString(prompt.button) << ": " << prompt.label << "]";
    std::cout << '\n';
}

void printVisibleLayers(const ScreenRuntime& runtime)
{
    std::cout << "Visible layers:";
    auto layers = runtime.visibleLayers();
    if (layers.empty())
    {
        std::cout << " none\n";
        return;
    }

    for (const auto& layer : layers)
        std::cout << " [" << layer.id << ":" << layer.role << "]";
    std::cout << '\n';
}
} // namespace

int main()
{
    ScreenRuntime runtime(
        makeTitleMenuContract(),
        RuntimeCallbacks{
            .onStateEntered = [](ScreenState state) { std::cout << "Enter " << toString(state) << '\n'; },
            .onStateChanged = [](ScreenState from, ScreenState to) { std::cout << "Transition " << toString(from) << " -> " << toString(to) << '\n'; },
            .onInputBlocked = [](InputAction action) { std::cout << "Blocked input: " << toString(action) << '\n'; },
            .onSceneRequested = [](std::string_view scene) { std::cout << "Request scene: " << scene << '\n'; },
        });

    runtime.setPredicate("can_confirm", true);
    runtime.setPredicate("can_cancel", true);
    runtime.setPredicate("has_previous_tab", true);
    runtime.setPredicate("has_next_tab", true);
    runtime.setPredicate("show_start_prompt", true);

    runtime.dispatch(RuntimeEventType::ResourcesReady);
    runtime.tick(1.3);
    printVisiblePrompts(runtime);
    printVisibleLayers(runtime);

    runtime.requestAction(InputAction::MoveNext);
    runtime.tick(0.4);
    printVisiblePrompts(runtime);

    runtime.requestAction(InputAction::Confirm);
    runtime.tick(0.5);
    runtime.tick(0.4);

    std::cout << "Final state: " << toString(runtime.state()) << '\n';
    return 0;
}
