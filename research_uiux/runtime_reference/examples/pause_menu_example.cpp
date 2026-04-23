#include <sward/ui_runtime/contract_loader.hpp>
#include <sward/ui_runtime/runtime.hpp>

#include <iostream>
#include <utility>

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
} // namespace

int main(int argc, char** argv)
{
    ScreenContract contract = argc > 1 ? loadContractFromJsonFile(argv[1]) : loadBundledContract(ReferenceProfile::PauseMenu);
    ScreenRuntime runtime(
        std::move(contract),
        RuntimeCallbacks{
            .onStateEntered = [](ScreenState state) { std::cout << "Enter " << toString(state) << '\n'; },
            .onStateChanged = [](ScreenState from, ScreenState to) { std::cout << "Transition " << toString(from) << " -> " << toString(to) << '\n'; },
            .onInputBlocked = [](InputAction action) { std::cout << "Blocked input: " << toString(action) << '\n'; },
            .onSceneRequested = [](std::string_view scene) { std::cout << "Request scene: " << scene << '\n'; },
        });

    std::cout << "Contract source: " << (argc > 1 ? argv[1] : bundledContractPath(ReferenceProfile::PauseMenu).string()) << '\n';

    runtime.setPredicate("can_confirm", true);
    runtime.setPredicate("can_cancel", true);
    runtime.setPredicate("has_previous_tab", true);
    runtime.setPredicate("has_next_tab", true);

    runtime.dispatch(RuntimeEventType::ResourcesReady);
    runtime.tick(0.4);
    printVisiblePrompts(runtime);

    runtime.requestAction(InputAction::MoveNext);
    runtime.tick(0.4);
    printVisiblePrompts(runtime);

    runtime.requestAction(InputAction::Confirm);
    runtime.dispatch(RuntimeEventType::ActionCompletedKeepOpen);
    printVisiblePrompts(runtime);

    runtime.requestAction(InputAction::Cancel);
    runtime.tick(0.3);
    runtime.tick(0.4);

    std::cout << "Final state: " << toString(runtime.state()) << '\n';
    return 0;
}
