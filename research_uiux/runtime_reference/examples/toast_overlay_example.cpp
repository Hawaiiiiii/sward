#include <sward/ui_runtime/contract_loader.hpp>
#include <sward/ui_runtime/runtime.hpp>

#include <iostream>
#include <utility>

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

int main(int argc, char** argv)
{
    ScreenContract contract = argc > 1 ? loadContractFromJsonFile(argv[1]) : loadBundledContract(ReferenceProfile::AutosaveToast);
    ScreenRuntime runtime(
        std::move(contract),
        RuntimeCallbacks{
            .onStateEntered = [](ScreenState state) { std::cout << "Enter " << toString(state) << '\n'; },
            .onStateChanged = [](ScreenState from, ScreenState to) { std::cout << "Transition " << toString(from) << " -> " << toString(to) << '\n'; },
            .onSceneRequested = [](std::string_view scene) { std::cout << "Request scene: " << scene << '\n'; },
        });

    std::cout << "Contract source: " << (argc > 1 ? argv[1] : bundledContractPath(ReferenceProfile::AutosaveToast).string()) << '\n';

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
