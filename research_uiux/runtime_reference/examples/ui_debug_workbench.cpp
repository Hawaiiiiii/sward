#include <sward/ui_runtime/contract_loader.hpp>
#include <sward/ui_runtime/debug_workbench_data.hpp>
#include <sward/ui_runtime/runtime.hpp>

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

struct ResolvedHostEntry
{
    const DebugWorkbenchHostEntry* metadata = nullptr;
    std::size_t contractIndex = 0;
};

struct GroupEntry
{
    std::string_view groupId;
    std::string_view groupDisplayName;
    std::string_view priority;
    std::vector<std::size_t> hostIndices;
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

[[nodiscard]] std::vector<ContractEntry> loadBundledEntries()
{
    std::vector<ContractEntry> result;
    for (const auto& path : bundledContractPaths())
        result.push_back(ContractEntry{ path, loadContractFromJsonFile(path) });
    return result;
}

[[nodiscard]] std::vector<ResolvedHostEntry> resolveHostEntries(const std::vector<ContractEntry>& bundledEntries)
{
    std::vector<ResolvedHostEntry> result;
    for (const auto& host : kDebugWorkbenchHostEntries)
    {
        const auto found = std::find_if(
            bundledEntries.begin(),
            bundledEntries.end(),
            [&host](const ContractEntry& entry)
            {
                return entry.path.filename() == host.primaryContractFileName;
            });
        if (found == bundledEntries.end())
            continue;

        result.push_back(
            ResolvedHostEntry{
                .metadata = &host,
                .contractIndex = static_cast<std::size_t>(std::distance(bundledEntries.begin(), found)),
            });
    }
    return result;
}

[[nodiscard]] std::vector<GroupEntry> buildGroups(const std::vector<ResolvedHostEntry>& hosts)
{
    std::vector<GroupEntry> groups;
    for (std::size_t index = 0; index < hosts.size(); ++index)
    {
        const auto& host = *hosts[index].metadata;
        auto found = std::find_if(
            groups.begin(),
            groups.end(),
            [&host](const GroupEntry& group)
            {
                return group.groupId == host.groupId;
            });
        if (found == groups.end())
        {
            groups.push_back(
                GroupEntry{
                    .groupId = host.groupId,
                    .groupDisplayName = host.groupDisplayName,
                    .priority = host.priority,
                    .hostIndices = { index },
                });
            continue;
        }
        found->hostIndices.push_back(index);
    }
    return groups;
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

void driveRuntime(const ContractEntry& entry, const DebugWorkbenchHostEntry* host)
{
    if (host)
    {
        std::cout << "Host group: " << host->groupDisplayName << " [" << host->priority << "]\n";
        std::cout << "Host path: " << host->relativeSourcePath << '\n';
        std::cout << "Host note: " << host->notes << '\n';
        std::cout << "Primary contract: " << host->primaryContractFileName << "\n\n";
    }

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

    if (runtime.state() == ScreenState::Idle && runtime.requestAction(InputAction::Cancel))
    {
        std::cout << "Action: Cancel\n";
        settleRuntime(runtime, entry.contract);
    }

    std::cout << "Final state: " << toString(runtime.state()) << '\n';
}

void waitForEnter(std::string_view prompt)
{
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
}

[[nodiscard]] bool promptReturnToMenu()
{
    std::cout << "\nPress Enter to return to the menu, or type q to quit: ";
    std::string line;
    if (!std::getline(std::cin, line))
        return false;

    const std::string normalized = normalizeToken(line);
    return normalized != "q" && normalized != "quit" && normalized != "exit";
}

[[nodiscard]] std::optional<std::size_t> findHostIndex(const std::vector<ResolvedHostEntry>& hosts, std::string_view token)
{
    const std::string needle = normalizeToken(token);
    if (needle.empty())
        return std::nullopt;

    for (std::size_t index = 0; index < hosts.size(); ++index)
    {
        const auto& host = *hosts[index].metadata;
        if (normalizeToken(host.relativeSourcePath) == needle)
            return index;
        if (normalizeToken(host.hostDisplayName) == needle)
            return index;
    }

    for (std::size_t index = 0; index < hosts.size(); ++index)
    {
        const auto& host = *hosts[index].metadata;
        if (normalizeToken(host.groupId).find(needle) != std::string::npos)
            return index;
        if (normalizeToken(host.groupDisplayName).find(needle) != std::string::npos)
            return index;
        if (normalizeToken(host.relativeSourcePath).find(needle) != std::string::npos)
            return index;
        if (normalizeToken(host.hostDisplayName).find(needle) != std::string::npos)
            return index;
        if (normalizeToken(host.primaryContractFileName).find(needle) != std::string::npos)
            return index;
    }

    for (std::size_t index = 0; index < hosts.size(); ++index)
    {
        const auto& host = *hosts[index].metadata;
        for (const auto alias : splitBlob(host.aliasBlob))
        {
            if (normalizeToken(alias).find(needle) != std::string::npos)
                return index;
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> findGroupIndex(const std::vector<GroupEntry>& groups, std::string_view token)
{
    const std::string needle = normalizeToken(token);
    if (needle.empty())
        return std::nullopt;

    for (std::size_t index = 0; index < groups.size(); ++index)
    {
        if (normalizeToken(groups[index].groupId).find(needle) != std::string::npos)
            return index;
        if (normalizeToken(groups[index].groupDisplayName).find(needle) != std::string::npos)
            return index;
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

void printGroupList(const std::vector<GroupEntry>& groups)
{
    std::cout << "Debug workbench groups:\n";
    for (std::size_t index = 0; index < groups.size(); ++index)
    {
        std::cout
            << "  " << (index + 1)
            << ". " << groups[index].groupDisplayName
            << " [" << groups[index].priority << ']'
            << " hosts=" << groups[index].hostIndices.size()
            << '\n';
    }
}

void printHostList(const std::vector<ResolvedHostEntry>& hosts, const std::optional<GroupEntry>& group = std::nullopt)
{
    std::cout << "Debug workbench hosts:\n";
    std::vector<std::size_t> indices;
    if (group.has_value())
        indices = group->hostIndices;
    else
    {
        indices.resize(hosts.size());
        for (std::size_t index = 0; index < hosts.size(); ++index)
            indices[index] = index;
    }

    for (std::size_t localIndex = 0; localIndex < indices.size(); ++localIndex)
    {
        const auto& host = *hosts[indices[localIndex]].metadata;
        std::cout
            << "  " << (localIndex + 1)
            << ". " << host.hostDisplayName
            << " -> " << host.primaryContractFileName
            << " [" << host.groupDisplayName << "]"
            << '\n';
    }
}

[[nodiscard]] std::optional<std::size_t> selectInteractiveIndex(std::size_t maxValue, std::string_view prompt)
{
    while (true)
    {
        std::cout << prompt;

        std::string line;
        if (!std::getline(std::cin, line))
            return std::nullopt;

        const std::string normalized = normalizeToken(line);
        if (normalized == "q" || normalized == "quit" || normalized == "exit")
            return std::nullopt;

        const auto parsed = parseIndexArgument(line, maxValue);
        if (parsed.has_value())
            return parsed;

        std::cout << "Invalid selection.\n\n";
    }
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
        const std::vector<ResolvedHostEntry> hosts = resolveHostEntries(bundledEntries);
        const std::vector<GroupEntry> groups = buildGroups(hosts);

        std::vector<std::string> arguments(argv + 1, argv + argc);
        const auto stayOpenIt = std::find(arguments.begin(), arguments.end(), "--stay-open");
        const bool stayOpen = stayOpenIt != arguments.end();
        if (stayOpen)
            arguments.erase(stayOpenIt);

        if (!arguments.empty())
        {
            const std::string& command = arguments[0];
            if (command == "--list-groups")
            {
                printGroupList(groups);
                if (stayOpen)
                    waitForEnter("\nPress Enter to exit...");
                return 0;
            }

            if (command == "--list-hosts")
            {
                printHostList(hosts);
                if (stayOpen)
                    waitForEnter("\nPress Enter to exit...");
                return 0;
            }

            if (command == "--group")
            {
                if (arguments.size() < 2)
                {
                    std::cerr << "--group requires a group token.\n";
                    return 1;
                }

                const auto groupIndex = findGroupIndex(groups, arguments[1]);
                if (!groupIndex.has_value())
                {
                    std::cerr << "Unknown host group. Use --list-groups to inspect the available groups.\n";
                    return 1;
                }

                printHostList(hosts, groups[*groupIndex]);
                if (stayOpen)
                    waitForEnter("\nPress Enter to exit...");
                return 0;
            }

            if (command == "--host")
            {
                if (arguments.size() < 2)
                {
                    std::cerr << "--host requires a host token.\n";
                    return 1;
                }

                const auto hostIndex = findHostIndex(hosts, arguments[1]);
                if (!hostIndex.has_value())
                {
                    std::cerr << "Unknown host token. Use --list-hosts to inspect the available hosts.\n";
                    return 1;
                }

                driveRuntime(bundledEntries[hosts[*hostIndex].contractIndex], hosts[*hostIndex].metadata);
                if (stayOpen)
                    waitForEnter("\nPress Enter to exit...");
                return 0;
            }

            if (command == "--path")
            {
                if (arguments.size() < 2)
                {
                    std::cerr << "--path requires a contract file path.\n";
                    return 1;
                }

                driveRuntime(loadExplicitEntry(arguments[1]), nullptr);
                if (stayOpen)
                    waitForEnter("\nPress Enter to exit...");
                return 0;
            }

            const auto hostIndex = findHostIndex(hosts, command);
            if (!hostIndex.has_value())
            {
                std::cerr << "Unknown host token. Use --list-groups or --list-hosts to inspect the available workbench entries.\n";
                return 1;
            }

            driveRuntime(bundledEntries[hosts[*hostIndex].contractIndex], hosts[*hostIndex].metadata);
            if (stayOpen)
                waitForEnter("\nPress Enter to exit...");
            return 0;
        }

        while (true)
        {
            printGroupList(groups);
            const auto groupIndex = selectInteractiveIndex(groups.size(), "\nSelect group number (or q to quit): ");
            if (!groupIndex.has_value())
                return 0;

            const auto& group = groups[*groupIndex];
            printHostList(hosts, group);
            const auto hostLocalIndex = selectInteractiveIndex(group.hostIndices.size(), "\nSelect host number (or q to quit): ");
            if (!hostLocalIndex.has_value())
                return 0;

            const std::size_t hostIndex = group.hostIndices[*hostLocalIndex];
            driveRuntime(bundledEntries[hosts[hostIndex].contractIndex], hosts[hostIndex].metadata);

            if (!promptReturnToMenu())
                return 0;
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "ui_debug_workbench failed: " << exception.what() << '\n';
        return 1;
    }
}
