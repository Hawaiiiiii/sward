#include <sward/ui_runtime/contract_loader.hpp>
#include <sward/ui_runtime/runtime.hpp>
#include <sward/ui_runtime/source_family_selector_data.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace sward::ui_runtime;

namespace
{
struct ContractEntry
{
    std::filesystem::path path;
    ScreenContract contract;
};

struct ResolvedFamilyEntry
{
    const SourceFamilySelectorEntry* metadata = nullptr;
    std::size_t contractIndex = 0;
};

[[nodiscard]] std::string normalizeToken(std::string_view value)
{
    std::string normalized;
    normalized.reserve(value.size());

    for (const char ch : value)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)))
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    return normalized;
}

[[nodiscard]] std::vector<ContractEntry> loadBundledEntries()
{
    std::vector<ContractEntry> result;
    for (const auto& path : bundledContractPaths())
        result.push_back(ContractEntry{ path, loadContractFromJsonFile(path) });
    return result;
}

[[nodiscard]] std::vector<std::string_view> splitBlob(std::string_view blob)
{
    std::vector<std::string_view> tokens;
    std::size_t start = 0;
    while (start <= blob.size())
    {
        const std::size_t end = blob.find('|', start);
        const std::size_t stop = end == std::string_view::npos ? blob.size() : end;
        if (stop > start)
            tokens.push_back(blob.substr(start, stop - start));
        if (end == std::string_view::npos)
            break;
        start = end + 1;
    }
    return tokens;
}

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

void printEntryList(const std::vector<ContractEntry>& entries)
{
    std::cout << "Bundled runtime contracts:\n";
    for (std::size_t index = 0; index < entries.size(); ++index)
    {
        const auto& entry = entries[index];
        std::cout
            << "  " << (index + 1)
            << ". " << entry.contract.screenId
            << " [" << entry.path.filename().string() << ']'
            << " states=" << entry.contract.states.size()
            << " overlays=" << entry.contract.overlayLayers.size()
            << " prompts=" << entry.contract.promptSlots.size()
            << '\n';
    }
}

[[nodiscard]] std::vector<ResolvedFamilyEntry> resolveFamilyEntries(const std::vector<ContractEntry>& bundledEntries)
{
    std::vector<ResolvedFamilyEntry> result;
    for (const auto& family : kSourceFamilySelectorEntries)
    {
        const auto found = std::find_if(
            bundledEntries.begin(),
            bundledEntries.end(),
            [&family](const ContractEntry& entry)
            {
                return entry.path.filename() == family.contractFileName;
            });
        if (found == bundledEntries.end())
            continue;

        result.push_back(
            ResolvedFamilyEntry{
                .metadata = &family,
                .contractIndex = static_cast<std::size_t>(std::distance(bundledEntries.begin(), found)),
            });
    }
    return result;
}

void printFamilyList(const std::vector<ResolvedFamilyEntry>& families)
{
    std::cout << "Source-path launch families:\n";
    for (std::size_t index = 0; index < families.size(); ++index)
    {
        const auto& family = *families[index].metadata;
        const auto sourcePaths = splitBlob(family.sourcePathBlob);
        std::cout
            << "  " << (index + 1)
            << ". " << family.displayName
            << " [" << family.contractFileName << ']'
            << " source_system=" << family.sourceSystemId;
        if (!sourcePaths.empty())
            std::cout << " anchor=" << sourcePaths.front();
        std::cout << '\n';
    }
}

[[nodiscard]] double timelineDuration(const ScreenContract& contract, ScreenState state)
{
    const auto stateIt = contract.states.find(state);
    if (stateIt == contract.states.end() || !stateIt->second.timelineBandId.has_value())
        return 0.0;

    const auto bandIt = contract.timelineBands.find(*stateIt->second.timelineBandId);
    return bandIt == contract.timelineBands.end() ? 0.0 : bandIt->second.seconds;
}

void enableAllPromptPredicates(ScreenRuntime& runtime, const ScreenContract& contract)
{
    std::vector<std::string> predicates;
    for (const auto& prompt : contract.promptSlots)
    {
        for (const auto& predicate : prompt.requiredPredicates)
        {
            if (std::find(predicates.begin(), predicates.end(), predicate) == predicates.end())
                predicates.push_back(predicate);
        }
    }

    for (const auto& predicate : predicates)
        runtime.setPredicate(predicate, true);
}

void printRuntimeSnapshot(const ScreenRuntime& runtime)
{
    std::cout
        << "State: " << toString(runtime.state())
        << " | elapsed=" << runtime.stateElapsedSeconds()
        << " | input_locked=" << (runtime.isInputLocked() ? "yes" : "no")
        << '\n';
    printVisibleLayers(runtime);
    printVisiblePrompts(runtime);
}

void settleRuntime(ScreenRuntime& runtime, const ScreenContract& contract, std::size_t maxSteps = 12)
{
    for (std::size_t step = 0; step < maxSteps; ++step)
    {
        printRuntimeSnapshot(runtime);
        if (runtime.state() == ScreenState::Closed)
            return;

        const auto stateIt = contract.states.find(runtime.state());
        if (stateIt == contract.states.end())
            return;

        const double duration = timelineDuration(contract, runtime.state());
        if (duration > 0.0)
        {
            runtime.tick(duration + 0.05);
            continue;
        }

        if (!stateIt->second.inputEnabled)
        {
            if (runtime.dispatch(RuntimeEventType::Timeout))
                continue;
            if (runtime.dispatch(RuntimeEventType::AnimationFinished))
                continue;
        }

        return;
    }
}

void driveRuntime(const ContractEntry& entry)
{
    ScreenRuntime runtime(
        entry.contract,
        RuntimeCallbacks{
            .onStateEntered = [](ScreenState state) { std::cout << "Enter " << toString(state) << '\n'; },
            .onStateChanged = [](ScreenState from, ScreenState to) { std::cout << "Transition " << toString(from) << " -> " << toString(to) << '\n'; },
            .onInputBlocked = [](InputAction action) { std::cout << "Blocked input: " << toString(action) << '\n'; },
            .onSceneRequested = [](std::string_view scene) { std::cout << "Request scene: " << scene << '\n'; },
        });

    std::cout << "Contract source: " << entry.path.string() << '\n';
    std::cout << "Screen id: " << entry.contract.screenId << '\n';
    enableAllPromptPredicates(runtime, entry.contract);

    if (!runtime.dispatch(RuntimeEventType::ResourcesReady))
        std::cout << "ResourcesReady did not change state.\n";
    settleRuntime(runtime, entry.contract);

    if (runtime.state() == ScreenState::Idle && runtime.requestAction(InputAction::MoveNext))
    {
        std::cout << "Action: MoveNext\n";
        settleRuntime(runtime, entry.contract);
    }

    if (runtime.state() == ScreenState::Idle && runtime.requestAction(InputAction::Confirm))
    {
        std::cout << "Action: Confirm\n";
        settleRuntime(runtime, entry.contract);
    }

    if (runtime.state() == ScreenState::Idle && runtime.dispatch(RuntimeEventType::HostForceClose))
    {
        std::cout << "Event: HostForceClose\n";
        settleRuntime(runtime, entry.contract);
    }

    if (runtime.state() == ScreenState::Idle && runtime.requestAction(InputAction::Cancel))
    {
        std::cout << "Action: Cancel\n";
        settleRuntime(runtime, entry.contract);
    }

    std::cout << "Final state: " << toString(runtime.state()) << '\n';
}

[[nodiscard]] std::optional<std::size_t> selectInteractiveIndex(const std::vector<ContractEntry>& entries)
{
    printEntryList(entries);
    std::cout << "\nSelect contract number: ";

    std::string line;
    if (!std::getline(std::cin, line))
        return std::nullopt;

    std::istringstream parser(line);
    std::size_t value = 0;
    if (!(parser >> value) || value == 0 || value > entries.size())
        return std::nullopt;
    return value - 1;
}

[[nodiscard]] std::optional<std::size_t> selectInteractiveFamilyIndex(const std::vector<ResolvedFamilyEntry>& families)
{
    printFamilyList(families);
    std::cout << "\nSelect family number: ";

    std::string line;
    if (!std::getline(std::cin, line))
        return std::nullopt;

    std::istringstream parser(line);
    std::size_t value = 0;
    if (!(parser >> value) || value == 0 || value > families.size())
        return std::nullopt;
    return value - 1;
}

[[nodiscard]] std::optional<std::size_t> findEntryIndex(const std::vector<ContractEntry>& entries, std::string_view token)
{
    const std::string needle = normalizeToken(token);
    if (needle.empty())
        return std::nullopt;

    for (std::size_t index = 0; index < entries.size(); ++index)
    {
        const auto& entry = entries[index];
        if (normalizeToken(entry.contract.screenId).find(needle) != std::string::npos)
            return index;
        if (normalizeToken(entry.path.stem().string()).find(needle) != std::string::npos)
            return index;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> findFamilyIndex(const std::vector<ResolvedFamilyEntry>& families, std::string_view token)
{
    const std::string needle = normalizeToken(token);
    if (needle.empty())
        return std::nullopt;

    for (std::size_t index = 0; index < families.size(); ++index)
    {
        const auto& family = *families[index].metadata;
        if (normalizeToken(family.displayName).find(needle) != std::string::npos)
            return index;
        if (normalizeToken(family.familyId).find(needle) != std::string::npos)
            return index;
        if (normalizeToken(family.sourceSystemId).find(needle) != std::string::npos)
            return index;
        if (normalizeToken(family.contractFileName).find(needle) != std::string::npos)
            return index;

        for (const auto alias : splitBlob(family.aliasBlob))
        {
            if (normalizeToken(alias).find(needle) != std::string::npos)
                return index;
        }
        for (const auto sourcePath : splitBlob(family.sourcePathBlob))
        {
            if (normalizeToken(sourcePath).find(needle) != std::string::npos)
                return index;
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> parseIndexArgument(const std::string& value, std::size_t maxValue)
{
    std::istringstream parser(value);
    std::size_t parsed = 0;
    if (!(parser >> parsed) || parsed == 0 || parsed > maxValue)
        return std::nullopt;
    return parsed - 1;
}

[[nodiscard]] ContractEntry loadExplicitEntry(const std::filesystem::path& path)
{
    return ContractEntry{ path, loadContractFromJsonFile(path) };
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        const std::vector<ContractEntry> bundledEntries = loadBundledEntries();
        const std::vector<ResolvedFamilyEntry> families = resolveFamilyEntries(bundledEntries);
        if (argc > 1)
        {
            const std::string command = argv[1];
            if (command == "--list")
            {
                printEntryList(bundledEntries);
                return 0;
            }

            if (command == "--list-families")
            {
                printFamilyList(families);
                return 0;
            }

            if (command == "--path")
            {
                if (argc < 3)
                {
                    std::cerr << "--path requires a contract file path.\n";
                    return 1;
                }

                driveRuntime(loadExplicitEntry(argv[2]));
                return 0;
            }

            if (command == "--family")
            {
                if (argc < 3)
                {
                    std::cerr << "--family requires a family token.\n";
                    return 1;
                }

                const auto familyIndex = findFamilyIndex(families, argv[2]);
                if (!familyIndex.has_value())
                {
                    std::cerr << "Unknown family token. Use --list-families to inspect the available launch families.\n";
                    return 1;
                }

                driveRuntime(bundledEntries[families[*familyIndex].contractIndex]);
                return 0;
            }

            if (command == "--index")
            {
                if (argc < 3)
                {
                    std::cerr << "--index requires a 1-based contract number.\n";
                    return 1;
                }

                const auto index = parseIndexArgument(argv[2], bundledEntries.size());
                if (!index.has_value())
                {
                    std::cerr << "Invalid contract index.\n";
                    return 1;
                }

                driveRuntime(bundledEntries[*index]);
                return 0;
            }

            const auto familyIndex = findFamilyIndex(families, command);
            if (familyIndex.has_value())
            {
                driveRuntime(bundledEntries[families[*familyIndex].contractIndex]);
                return 0;
            }

            const auto entryIndex = findEntryIndex(bundledEntries, command);
            if (!entryIndex.has_value())
            {
                std::cerr << "Unknown screen or family token. Use --list or --list-families to inspect the available launches.\n";
                return 1;
            }

            driveRuntime(bundledEntries[*entryIndex]);
            return 0;
        }

        const auto familyIndex = selectInteractiveFamilyIndex(families);
        if (!familyIndex.has_value())
        {
            std::cerr << "No valid family selection was provided.\n";
            return 1;
        }

        driveRuntime(bundledEntries[families[*familyIndex].contractIndex]);
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "ui_debug_selector failed: " << exception.what() << '\n';
        return 1;
    }
}
