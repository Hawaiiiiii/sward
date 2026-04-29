#include <sward/ui_runtime/frontend_screen_reference.hpp>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

using namespace sward::ui_runtime;

namespace
{
constexpr std::string_view kPhase141Header = "Phase 141 frontend screen reference";

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

const FrontendScreenPolicy* selectedScreen(int argc, char** argv)
{
    const std::string screenId = valueAfterArg(argc, argv, "--screen");
    if (!screenId.empty())
        return findFrontendScreenPolicy(screenId);
    return nullptr;
}

int runPhase141Smoke()
{
    std::cout << "sward_frontend_screen_reference_catalog phase141 smoke ok\n";
    std::cout << formatFrontendScreenReferenceCatalog();

    if (const auto* title = findFrontendScreenPolicy("title-menu"))
    {
        if (!title->materialSlots.empty())
        {
            const auto& slot = title->materialSlots.front();
            std::cout << "material_slot="
                      << title->screenId
                      << ':' << slot.slotId
                      << ':' << slot.textureName
                      << ":placeholder=" << slot.placeholderFamily
                      << '\n';
        }
    }

    if (const auto* pause = findFrontendScreenPolicy("pause"))
    {
        if (const auto* textArea = findFrontendScreenScenePolicy(*pause, "text_area"))
        {
            const auto sample = sampleFrontendScreenTimeline(*pause, *textArea, 50);
            std::cout << "timeline_sample="
                      << sample.screenId << '/' << sample.sceneName
                      << ':' << sample.animationName
                      << ":frame=" << sample.frame
                      << '/' << sample.frameCount
                      << ":progress=" << std::fixed << std::setprecision(3) << sample.progress
                      << '\n';
        }
    }
    return 0;
}

int runPhase161MediaSmoke()
{
    std::cout << "sward_frontend_screen_reference_catalog phase161 media timing smoke ok\n";
    std::cout << formatFrontendScreenMediaTimingCatalog();
    return 0;
}

int runPhase162MediaAssetSmoke()
{
    std::cout << "sward_frontend_screen_reference_catalog phase162 media asset smoke ok\n";
    std::cout << formatFrontendScreenMediaAssetProbeCatalog(std::filesystem::current_path().string());
    return 0;
}
} // namespace

int main(int argc, char** argv)
{
    if (hasArg(argc, argv, "--phase141-smoke"))
        return runPhase141Smoke();
    if (hasArg(argc, argv, "--phase161-media-smoke"))
        return runPhase161MediaSmoke();
    if (hasArg(argc, argv, "--phase162-media-asset-smoke"))
        return runPhase162MediaAssetSmoke();

    const auto* screen = selectedScreen(argc, argv);
    const std::string screenArg = valueAfterArg(argc, argv, "--screen");
    if (!screenArg.empty() && !screen)
    {
        std::cerr << "Unknown frontend screen: " << screenArg << '\n';
        return 2;
    }

    const std::string sceneName = valueAfterArg(argc, argv, "--scene");
    if (!sceneName.empty())
    {
        if (!screen)
        {
            std::cerr << "--scene requires --screen\n";
            return 2;
        }

        const auto* scene = findFrontendScreenScenePolicy(*screen, sceneName);
        if (!scene)
        {
            std::cerr << "Unknown scene for " << screen->screenId << ": " << sceneName << '\n';
            return 2;
        }

        std::cout << kPhase141Header << '\n';
        std::cout << formatFrontendScreenSceneDetail(*screen, *scene);
        if (hasArg(argc, argv, "--sample"))
        {
            const int frame = intValueAfterArg(argc, argv, "--sample", scene->timeline.sampleFrame);
            const auto sample = sampleFrontendScreenTimeline(*screen, *scene, frame);
            std::cout << "sample="
                      << sample.screenId << '/' << sample.sceneName
                      << ':' << sample.animationName
                      << ":frame=" << sample.frame
                      << '/' << sample.frameCount
                      << ":progress=" << std::fixed << std::setprecision(3) << sample.progress
                      << ":clamped=" << (sample.clamped ? 1 : 0)
                      << '\n';
        }
        return 0;
    }

    if (screen)
    {
        std::cout << kPhase141Header << '\n';
        std::cout << formatFrontendScreenReferenceDetail(*screen);
        return 0;
    }

    if (hasArg(argc, argv, "--catalog") || argc == 1)
    {
        std::cout << kPhase141Header << '\n';
        std::cout << formatFrontendScreenReferenceCatalog();
        return 0;
    }

    std::cerr << "Usage: sward_frontend_screen_reference_catalog [--catalog] [--screen <id>] [--scene <name>] [--sample <frame>] [--phase141-smoke] [--phase161-media-smoke] [--phase162-media-asset-smoke]\n";
    return 2;
}
