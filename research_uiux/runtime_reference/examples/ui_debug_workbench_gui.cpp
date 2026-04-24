#include <sward/ui_runtime/contract_loader.hpp>
#include <sward/ui_runtime/debug_workbench_data.hpp>
#include <sward/ui_runtime/runtime.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
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

enum ControlId
{
    kGroupListId = 1001,
    kHostListId = 1002,
    kRunButtonId = 1003,
    kMoveNextButtonId = 1004,
    kConfirmButtonId = 1005,
    kCancelButtonId = 1006,
    kResetButtonId = 1007,
};

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

template <typename T>
void appendUnique(std::vector<T>& values, T value)
{
    if (std::find(values.begin(), values.end(), value) == values.end())
        values.push_back(value);
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
            appendUnique(predicates, predicate);
    }

    for (const auto& predicate : predicates)
        runtime.setPredicate(predicate, true);
}

[[nodiscard]] std::string snapshotText(const ScreenRuntime& runtime, const ContractEntry& entry, const DebugWorkbenchHostEntry& host)
{
    std::ostringstream text;
    text
        << "Host group: " << host.groupDisplayName << " [" << host.priority << "]\r\n"
        << "Host path: " << host.relativeSourcePath << "\r\n"
        << "Primary contract: " << host.primaryContractFileName << "\r\n"
        << "Contract source: " << entry.path.string() << "\r\n"
        << "Screen id: " << entry.contract.screenId << "\r\n"
        << "State: " << toString(runtime.state()) << "\r\n"
        << "Input locked: " << (runtime.isInputLocked() ? "yes" : "no") << "\r\n\r\n"
        << "Visible layers:\r\n";

    const auto layers = runtime.visibleLayers();
    if (layers.empty())
        text << "  none\r\n";
    for (const auto& layer : layers)
        text << "  " << layer.id << " : " << layer.role << "\r\n";

    text << "\r\nVisible prompts:\r\n";
    const auto prompts = runtime.visiblePrompts();
    if (prompts.empty())
        text << "  none\r\n";
    for (const auto& prompt : prompts)
        text << "  [" << toString(prompt.button) << "] " << prompt.label << "\r\n";

    text << "\r\nNotes:\r\n" << host.notes << "\r\n";
    return text.str();
}

[[nodiscard]] int runSmoke()
{
    const auto contracts = loadBundledEntries();
    const auto hosts = resolveHostEntries(contracts);
    const auto groups = buildGroups(hosts);
    const auto supportHosts = std::count_if(
        hosts.begin(),
        hosts.end(),
        [](const ResolvedHostEntry& host)
        {
            return host.metadata && host.metadata->groupId == std::string_view("support_substrate_hosts");
        });

    std::cout
        << "sward_ui_runtime_debug_gui smoke ok "
        << "contracts=" << contracts.size()
        << " hosts=" << hosts.size()
        << " groups=" << groups.size()
        << " support_hosts=" << supportHosts
        << '\n';
    return 0;
}

class WorkbenchGui
{
public:
    explicit WorkbenchGui(HINSTANCE instance)
        : m_instance(instance)
        , m_contracts(loadBundledEntries())
        , m_hosts(resolveHostEntries(m_contracts))
        , m_groups(buildGroups(m_hosts))
    {
    }

    int run(int showCommand)
    {
        WNDCLASSA windowClass{};
        windowClass.lpfnWndProc = &WorkbenchGui::windowProc;
        windowClass.hInstance = m_instance;
        windowClass.lpszClassName = "SwardUiRuntimeDebugGui";
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

        RegisterClassA(&windowClass);

        m_window = CreateWindowExA(
            0,
            windowClass.lpszClassName,
            "SWARD UI Runtime Debug Workbench",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            1240,
            760,
            nullptr,
            nullptr,
            m_instance,
            this);

        if (!m_window)
            return 1;

        applyDefaultFont();
        ShowWindow(m_window, showCommand);
        UpdateWindow(m_window);

        MSG message{};
        while (GetMessageA(&message, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        return static_cast<int>(message.wParam);
    }

private:
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* app = reinterpret_cast<WorkbenchGui*>(GetWindowLongPtrA(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            const auto* create = reinterpret_cast<CREATESTRUCTA*>(lParam);
            app = reinterpret_cast<WorkbenchGui*>(create->lpCreateParams);
            SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
            app->m_window = window;
        }

        if (!app)
            return DefWindowProcA(window, message, wParam, lParam);
        return app->handleMessage(message, wParam, lParam);
    }

    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
            createControls();
            populateGroups();
            return 0;
        case WM_SIZE:
            layoutControls(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_COMMAND:
            handleCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcA(m_window, message, wParam, lParam);
        }
    }

    void createControls()
    {
        m_groupLabel = createStatic("Groups");
        m_groupList = createListBox(kGroupListId);
        m_hostLabel = createStatic("Hosts");
        m_hostList = createListBox(kHostListId);
        m_runButton = createButton("Run Host", kRunButtonId);
        m_moveNextButton = createButton("Move Next", kMoveNextButtonId);
        m_confirmButton = createButton("Confirm", kConfirmButtonId);
        m_cancelButton = createButton("Cancel", kCancelButtonId);
        m_resetButton = createButton("Reset", kResetButtonId);
        m_detailText = createEdit();
        m_logText = createEdit();
    }

    void applyDefaultFont()
    {
        const auto font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        const HWND controls[] = {
            m_groupLabel,
            m_groupList,
            m_hostLabel,
            m_hostList,
            m_runButton,
            m_moveNextButton,
            m_confirmButton,
            m_cancelButton,
            m_resetButton,
            m_detailText,
            m_logText,
        };

        for (const HWND control : controls)
        {
            if (control)
                SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
    }

    HWND createStatic(const char* text)
    {
        return CreateWindowExA(0, "STATIC", text, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_window, nullptr, m_instance, nullptr);
    }

    HWND createButton(const char* text, int id)
    {
        return CreateWindowExA(0, "BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, m_window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), m_instance, nullptr);
    }

    HWND createListBox(int id)
    {
        return CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "LISTBOX",
            "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
            0,
            0,
            0,
            0,
            m_window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            m_instance,
            nullptr);
    }

    HWND createEdit()
    {
        return CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "EDIT",
            "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0,
            0,
            0,
            0,
            m_window,
            nullptr,
            m_instance,
            nullptr);
    }

    void layoutControls(int width, int height)
    {
        const int margin = 12;
        const int labelHeight = 20;
        const int buttonHeight = 30;
        const int groupWidth = 270;
        const int hostWidth = 300;
        const int rightX = margin + groupWidth + margin + hostWidth + margin;
        const int rightWidth = std::max(260, width - rightX - margin);
        const int listTop = margin + labelHeight;
        const int listHeight = std::max(160, height - listTop - margin);

        MoveWindow(m_groupLabel, margin, margin, groupWidth, labelHeight, TRUE);
        MoveWindow(m_groupList, margin, listTop, groupWidth, listHeight, TRUE);
        MoveWindow(m_hostLabel, margin + groupWidth + margin, margin, hostWidth, labelHeight, TRUE);
        MoveWindow(m_hostList, margin + groupWidth + margin, listTop, hostWidth, listHeight, TRUE);

        const int buttonY = margin;
        const int buttonWidth = 92;
        MoveWindow(m_runButton, rightX, buttonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_moveNextButton, rightX + 100, buttonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_confirmButton, rightX + 200, buttonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_cancelButton, rightX + 300, buttonY, buttonWidth, buttonHeight, TRUE);
        MoveWindow(m_resetButton, rightX + 400, buttonY, buttonWidth, buttonHeight, TRUE);

        const int detailsTop = buttonY + buttonHeight + margin;
        const int detailsHeight = std::max(220, (height - detailsTop - (margin * 3)) / 2);
        MoveWindow(m_detailText, rightX, detailsTop, rightWidth, detailsHeight, TRUE);
        MoveWindow(m_logText, rightX, detailsTop + detailsHeight + margin, rightWidth, height - detailsTop - detailsHeight - (margin * 2), TRUE);
    }

    void populateGroups()
    {
        SendMessageA(m_groupList, LB_RESETCONTENT, 0, 0);
        for (const auto& group : m_groups)
        {
            std::ostringstream label;
            label << group.groupDisplayName << " [" << group.priority << "] (" << group.hostIndices.size() << ")";
            const std::string labelText = label.str();
            SendMessageA(m_groupList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(labelText.c_str()));
        }

        if (!m_groups.empty())
        {
            SendMessageA(m_groupList, LB_SETCURSEL, 0, 0);
            populateHosts(0);
        }
    }

    void populateHosts(std::size_t groupIndex)
    {
        m_visibleHostIndices = m_groups[groupIndex].hostIndices;
        SendMessageA(m_hostList, LB_RESETCONTENT, 0, 0);
        for (const auto hostIndex : m_visibleHostIndices)
        {
            const std::string label = std::string(m_hosts[hostIndex].metadata->hostDisplayName);
            SendMessageA(m_hostList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        }

        if (!m_visibleHostIndices.empty())
        {
            SendMessageA(m_hostList, LB_SETCURSEL, 0, 0);
            m_selectedHostIndex = m_visibleHostIndices.front();
            showHostSummary();
        }
    }

    void handleCommand(int id, int notification)
    {
        if (id == kGroupListId && notification == LBN_SELCHANGE)
        {
            const auto selected = SendMessageA(m_groupList, LB_GETCURSEL, 0, 0);
            if (selected >= 0)
                populateHosts(static_cast<std::size_t>(selected));
            return;
        }

        if (id == kHostListId && notification == LBN_SELCHANGE)
        {
            const auto selected = SendMessageA(m_hostList, LB_GETCURSEL, 0, 0);
            if (selected >= 0 && static_cast<std::size_t>(selected) < m_visibleHostIndices.size())
            {
                m_selectedHostIndex = m_visibleHostIndices[static_cast<std::size_t>(selected)];
                showHostSummary();
            }
            return;
        }

        if (notification != BN_CLICKED)
            return;

        if (id == kRunButtonId || id == kResetButtonId)
            runSelectedHost();
        else if (id == kMoveNextButtonId)
            requestAction(InputAction::MoveNext);
        else if (id == kConfirmButtonId)
            requestAction(InputAction::Confirm);
        else if (id == kCancelButtonId)
            requestAction(InputAction::Cancel);
    }

    void showHostSummary()
    {
        if (!m_selectedHostIndex.has_value())
            return;

        const auto& host = *m_hosts[*m_selectedHostIndex].metadata;
        std::ostringstream text;
        text
            << "Selected host:\r\n"
            << host.hostDisplayName << "\r\n\r\n"
            << "Group: " << host.groupDisplayName << "\r\n"
            << "Path: " << host.relativeSourcePath << "\r\n"
            << "Contract: " << host.primaryContractFileName << "\r\n\r\n"
            << host.notes << "\r\n\r\n"
            << "Click Run Host to drive this contract through the runtime.";
        SetWindowTextA(m_detailText, text.str().c_str());
    }

    void runSelectedHost()
    {
        if (!m_selectedHostIndex.has_value())
            return;

        const auto& resolved = m_hosts[*m_selectedHostIndex];
        const auto& entry = m_contracts[resolved.contractIndex];
        const auto* host = resolved.metadata;

        m_log.clear();
        appendLogLine("Run Host: " + std::string(host->hostDisplayName));

        m_runtime = std::make_unique<ScreenRuntime>(
            entry.contract,
            RuntimeCallbacks{
                .onStateEntered = [this](ScreenState state) { appendLogLine("Enter " + std::string(toString(state))); },
                .onStateChanged = [this](ScreenState from, ScreenState to)
                {
                    appendLogLine("Transition " + std::string(toString(from)) + " -> " + std::string(toString(to)));
                },
                .onInputBlocked = [this](InputAction action) { appendLogLine("Blocked input: " + std::string(toString(action))); },
                .onSceneRequested = [this](std::string_view scene) { appendLogLine("Request scene: " + std::string(scene)); },
            });

        m_runningContractIndex = resolved.contractIndex;
        enableAllPromptPredicates(*m_runtime, entry.contract);
        m_runtime->dispatch(RuntimeEventType::ResourcesReady);
        settleRuntime();
        updateRuntimeDetails();
    }

    void requestAction(InputAction action)
    {
        if (!m_runtime || !m_runningContractIndex.has_value() || !m_selectedHostIndex.has_value())
            return;

        appendLogLine("Action: " + std::string(toString(action)));
        if (m_runtime->requestAction(action))
            settleRuntime();
        updateRuntimeDetails();
    }

    void settleRuntime(std::size_t maxSteps = 12)
    {
        if (!m_runtime || !m_runningContractIndex.has_value())
            return;

        const auto& contract = m_contracts[*m_runningContractIndex].contract;
        for (std::size_t step = 0; step < maxSteps; ++step)
        {
            if (m_runtime->state() == ScreenState::Closed)
                return;

            const auto stateIt = contract.states.find(m_runtime->state());
            if (stateIt == contract.states.end())
                return;

            const double duration = timelineDuration(contract, m_runtime->state());
            if (duration > 0.0)
            {
                m_runtime->tick(duration + 0.05);
                continue;
            }

            if (!stateIt->second.inputEnabled)
            {
                if (m_runtime->dispatch(RuntimeEventType::Timeout))
                    continue;
                if (m_runtime->dispatch(RuntimeEventType::AnimationFinished))
                    continue;
            }

            return;
        }
    }

    void updateRuntimeDetails()
    {
        if (!m_runtime || !m_runningContractIndex.has_value() || !m_selectedHostIndex.has_value())
            return;

        const auto& entry = m_contracts[*m_runningContractIndex];
        const auto& host = *m_hosts[*m_selectedHostIndex].metadata;
        SetWindowTextA(m_detailText, snapshotText(*m_runtime, entry, host).c_str());
        SetWindowTextA(m_logText, m_log.c_str());
    }

    void appendLogLine(const std::string& line)
    {
        m_log += line;
        m_log += "\r\n";
        if (m_logText)
            SetWindowTextA(m_logText, m_log.c_str());
    }

    HINSTANCE m_instance = nullptr;
    HWND m_window = nullptr;
    HWND m_groupLabel = nullptr;
    HWND m_groupList = nullptr;
    HWND m_hostLabel = nullptr;
    HWND m_hostList = nullptr;
    HWND m_runButton = nullptr;
    HWND m_moveNextButton = nullptr;
    HWND m_confirmButton = nullptr;
    HWND m_cancelButton = nullptr;
    HWND m_resetButton = nullptr;
    HWND m_detailText = nullptr;
    HWND m_logText = nullptr;

    std::vector<ContractEntry> m_contracts;
    std::vector<ResolvedHostEntry> m_hosts;
    std::vector<GroupEntry> m_groups;
    std::vector<std::size_t> m_visibleHostIndices;
    std::optional<std::size_t> m_selectedHostIndex;
    std::optional<std::size_t> m_runningContractIndex;
    std::unique_ptr<ScreenRuntime> m_runtime;
    std::string m_log;
};
} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR commandLine, int showCommand)
{
    try
    {
        const std::string command = commandLine ? commandLine : "";
        if (command.find("--smoke") != std::string::npos)
            return runSmoke();

        WorkbenchGui app(instance);
        return app.run(showCommand);
    }
    catch (const std::exception& exception)
    {
        MessageBoxA(nullptr, exception.what(), "SWARD UI Runtime Debug GUI failed", MB_ICONERROR | MB_OK);
        return 1;
    }
}
