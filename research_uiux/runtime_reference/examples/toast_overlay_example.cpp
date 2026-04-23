#include <sward/ui_runtime/profiles.hpp>
#include <sward/ui_runtime/runtime.hpp>

#include <iostream>

using namespace sward::ui_runtime;

namespace
{
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
        makeAutosaveToastContract(),
        RuntimeCallbacks{
            .onStateEntered = [](ScreenState state) { std::cout << "Enter " << toString(state) << '\n'; },
            .onStateChanged = [](ScreenState from, ScreenState to) { std::cout << "Transition " << toString(from) << " -> " << toString(to) << '\n'; },
            .onSceneRequested = [](std::string_view scene) { std::cout << "Request scene: " << scene << '\n'; },
        });

    runtime.dispatch(RuntimeEventType::ResourcesReady);
    printVisibleLayers(runtime);
    runtime.tick(0.3);
    printVisibleLayers(runtime);
    runtime.tick(3.1);
    printVisibleLayers(runtime);
    runtime.tick(0.3);

    std::cout << "Final state: " << toString(runtime.state()) << '\n';
    return 0;
}
