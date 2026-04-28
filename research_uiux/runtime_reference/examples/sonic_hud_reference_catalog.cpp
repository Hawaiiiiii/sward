#include <sward/ui_runtime/sonic_hud_reference.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

using namespace sward::ui_runtime;

namespace
{
constexpr std::string_view kPhase137Header = "Phase 137 Sonic HUD reference";

bool hasArg(int argc, char** argv, std::string_view expected)
{
    for (int index = 1; index < argc; ++index)
    {
        if (argv[index] == expected)
            return true;
    }
    return false;
}

std::string valueAfterArg(int argc, char** argv, std::string_view expected)
{
    for (int index = 1; index + 1 < argc; ++index)
    {
        if (argv[index] == expected)
            return argv[index + 1];
    }
    return {};
}

int intValueAfterArg(int argc, char** argv, std::string_view expected, int fallback)
{
    const std::string text = valueAfterArg(argc, argv, expected);
    if (text.empty())
        return fallback;
    try
    {
        return std::stoi(text);
    }
    catch (...)
    {
        return fallback;
    }
}

int runPhase137Smoke()
{
    std::cout << "sward_sonic_hud_reference_catalog phase137 smoke ok\n";
    std::cout << formatSonicHudReferenceCatalog();
    if (const auto* scene = findSonicHudScenePolicy("so_speed_gauge"))
    {
        const auto sample = sampleSonicHudTimeline(*scene, 99);
        std::cout << "timeline_sample="
                  << sample.sceneName
                  << ':' << sample.animationName
                  << ":frame=" << sample.frame
                  << '/' << sample.frameCount
                  << ":progress=" << std::fixed << std::setprecision(3) << sample.progress
                  << '\n';
        if (!scene->materialSlots.empty())
        {
            const auto& slot = scene->materialSlots.front();
            std::cout << "material_slot="
                      << scene->sceneName
                      << ':' << slot.slotId
                      << ':' << slot.textureName
                      << ":placeholder=" << slot.placeholderFamily
                      << '\n';
        }
    }
    return 0;
}
} // namespace

int main(int argc, char** argv)
{
    if (hasArg(argc, argv, "--phase137-smoke"))
        return runPhase137Smoke();

    const std::string sceneName = valueAfterArg(argc, argv, "--scene");
    if (!sceneName.empty())
    {
        const auto* scene = findSonicHudScenePolicy(sceneName);
        if (!scene)
        {
            std::cerr << "Unknown Sonic HUD scene: " << sceneName << '\n';
            return 2;
        }

        std::cout << kPhase137Header << '\n';
        std::cout << formatSonicHudSceneDetail(*scene);
        if (hasArg(argc, argv, "--sample"))
        {
            const int frame = intValueAfterArg(argc, argv, "--sample", scene->timeline.sampleFrame);
            const auto sample = sampleSonicHudTimeline(*scene, frame);
            std::cout << "sample="
                      << sample.sceneName
                      << ':' << sample.animationName
                      << ":frame=" << sample.frame
                      << '/' << sample.frameCount
                      << ":progress=" << std::fixed << std::setprecision(3) << sample.progress
                      << ":clamped=" << (sample.clamped ? 1 : 0)
                      << '\n';
        }
        return 0;
    }

    if (hasArg(argc, argv, "--catalog") || argc == 1)
    {
        std::cout << kPhase137Header << '\n';
        std::cout << formatSonicHudReferenceCatalog();
        return 0;
    }

    std::cerr << "Usage: sward_sonic_hud_reference_catalog [--catalog] [--scene <name>] [--sample <frame>] [--phase137-smoke]\n";
    return 2;
}
