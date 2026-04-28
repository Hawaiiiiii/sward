#include <sward/ui_runtime/sgfx_templates.hpp>

#include <algorithm>
#include <iostream>
#include <string>

using namespace sward::ui_runtime;

namespace
{
constexpr std::string_view kPhase122Header = "Phase 122 SGFX reusable templates";
constexpr std::string_view kCatalogHeader = "SGFX template catalog";

bool hasArg(int argc, char** argv, std::string_view expected)
{
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == expected)
            return true;
    }

    return false;
}

std::string valueAfterArg(int argc, char** argv, std::string_view expected)
{
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (argv[i] == expected)
            return argv[i + 1];
    }

    return {};
}

int runPhase122Smoke()
{
    const auto& templates = sgfxScreenTemplates();
    const auto realRuntimeTemplates = std::count_if(
        templates.begin(),
        templates.end(),
        [](const SgfxScreenTemplate& screenTemplate)
        {
            return screenTemplate.evidence.lane.find("real-runtime") != std::string::npos;
        });

    std::cout << "sward_sgfx_template_catalog phase122 smoke ok\n";
    std::cout << "templates=" << templates.size() << '\n';
    std::cout << "real_runtime_templates=" << realRuntimeTemplates << '\n';

    for (const auto& screenTemplate : templates)
    {
        const std::string event =
            screenTemplate.evidence.requiredEvents.empty()
                ? ""
                : screenTemplate.evidence.requiredEvents.front();

        std::cout << screenTemplate.id
                  << ':' << screenTemplate.contractFileName
                  << ':' << event
                  << '\n';
    }

    return 0;
}
} // namespace

int main(int argc, char** argv)
{
    if (hasArg(argc, argv, "--phase122-smoke"))
        return runPhase122Smoke();

    const std::string templateId = valueAfterArg(argc, argv, "--template");
    if (!templateId.empty())
    {
        const auto* screenTemplate = findSgfxScreenTemplate(templateId);
        if (!screenTemplate)
        {
            std::cerr << "Unknown SGFX template: " << templateId << '\n';
            return 2;
        }

        std::cout << kPhase122Header << '\n';
        std::cout << formatSgfxTemplateDetail(*screenTemplate);
        return 0;
    }

    if (hasArg(argc, argv, "--catalog") || argc == 1)
    {
        std::cout << kPhase122Header << '\n';
        std::cout << "view=" << kCatalogHeader << '\n';
        std::cout << formatSgfxTemplateCatalog();
        return 0;
    }

    std::cerr << "Usage: sward_sgfx_template_catalog [--catalog] [--template <id>] [--phase122-smoke]\n";
    return 2;
}
