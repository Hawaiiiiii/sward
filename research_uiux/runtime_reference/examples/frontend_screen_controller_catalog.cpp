#include <sward/ui_runtime/frontend_screen_controllers.hpp>

#include <iostream>
#include <string_view>

using namespace sward::ui_runtime;

namespace
{
bool hasArg(int argc, char** argv, std::string_view expected)
{
    for (int index = 1; index < argc; ++index)
    {
        if (argv[index] == expected)
            return true;
    }
    return false;
}
} // namespace

int main(int argc, char** argv)
{
    if (hasArg(argc, argv, "--phase163-smoke"))
    {
        std::cout << "sward_frontend_screen_controller_catalog phase163 smoke ok\n";
        std::cout << formatFrontendControllerSmokeSequence();
        return 0;
    }
    if (hasArg(argc, argv, "--phase164-sonic-hud-smoke"))
    {
        std::cout << "sward_frontend_screen_controller_catalog phase164 sonic hud smoke ok\n";
        std::cout << formatSonicDayHudControllerSmokeSequence();
        return 0;
    }
    if (hasArg(argc, argv, "--phase165-sonic-hud-state-smoke"))
    {
        std::cout << "sward_frontend_screen_controller_catalog phase165 sonic hud state smoke ok\n";
        std::cout << formatSonicDayHudGameplayStateSmokeSequence();
        return 0;
    }
    if (hasArg(argc, argv, "--phase166-sonic-hud-runtime-binding-smoke"))
    {
        std::cout << "sward_frontend_screen_controller_catalog phase166 sonic hud runtime binding smoke ok\n";
        std::cout << formatSonicDayHudRuntimeBindingSmokeSequence();
        return 0;
    }

    if (hasArg(argc, argv, "--catalog") || argc == 1)
    {
        std::cout << "Frontend native screen controller catalog\n";
        std::cout << formatFrontendControllerCatalog();
        return 0;
    }

    std::cerr << "Usage: sward_frontend_screen_controller_catalog [--catalog] [--phase163-smoke] [--phase164-sonic-hud-smoke] [--phase165-sonic-hud-state-smoke] [--phase166-sonic-hud-runtime-binding-smoke]\n";
    return 2;
}
