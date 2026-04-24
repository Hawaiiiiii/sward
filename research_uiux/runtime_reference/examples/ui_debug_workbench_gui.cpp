#include <sward/ui_runtime/contract_loader.hpp>
#include <sward/ui_runtime/debug_workbench_data.hpp>
#include <sward/ui_runtime/runtime.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <propidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <array>
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

struct AtlasCandidate
{
    std::string_view contractFileName;
    std::string_view shortName;
    std::string_view atlasFileName;
    std::string_view matchKind;
};

inline constexpr const char* kPreviewPanelClassName = "SwardUiRuntimePreviewPanel";

inline constexpr std::array<AtlasCandidate, 10> kPreviewAtlasCandidates{{
    { "title_menu_reference.json", "title", "mainmenu__ui_mainmenu.png", "exact" },
    { "pause_menu_reference.json", "pause", "systemcommoncore__ui_pause.png", "exact" },
    { "autosave_toast_reference.json", "autosave", "autosave__ui_saveicon.png", "exact" },
    { "loading_transition_reference.json", "loading", "loading__ui_loading.png", "exact" },
    { "mission_result_reference.json", "mission_result", "actioncommon__ui_result.png", "exact" },
    { "world_map_reference.json", "world_map", "worldmap__ui_worldmap.png", "exact" },
    { "boss_hud_reference.json", "boss_hud", "bosscommon__ui_boss_gauge.png", "exact" },
    { "extra_stage_hud_reference.json", "extra_stage", "exstagetails_common__ui_prov_playscreen.png", "exact" },
    { "sonic_stage_hud_reference.json", "sonic_stage", "exstagetails_common__ui_prov_playscreen.png", "proxy" },
    { "werehog_stage_hud_reference.json", "werehog_stage", "exstagetails_common__ui_prov_playscreen.png", "proxy" },
}};

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

[[nodiscard]] const AtlasCandidate* atlasCandidateForContract(std::string_view contractFileName)
{
    const auto found = std::find_if(
        kPreviewAtlasCandidates.begin(),
        kPreviewAtlasCandidates.end(),
        [contractFileName](const AtlasCandidate& candidate)
        {
            return candidate.contractFileName == contractFileName;
        });
    return found == kPreviewAtlasCandidates.end() ? nullptr : &*found;
}

[[nodiscard]] const AtlasCandidate* atlasCandidateByShortName(std::string_view shortName)
{
    const auto found = std::find_if(
        kPreviewAtlasCandidates.begin(),
        kPreviewAtlasCandidates.end(),
        [shortName](const AtlasCandidate& candidate)
        {
            return candidate.shortName == shortName;
        });
    return found == kPreviewAtlasCandidates.end() ? nullptr : &*found;
}

[[nodiscard]] bool isProxyAtlasCandidate(const AtlasCandidate& candidate)
{
    return candidate.matchKind == std::string_view("proxy");
}

[[nodiscard]] std::string atlasCandidateDisplayName(const AtlasCandidate& candidate)
{
    std::string label(candidate.atlasFileName);
    if (isProxyAtlasCandidate(candidate))
        label += " (proxy)";
    return label;
}

[[nodiscard]] std::filesystem::path visualAtlasSheetRoot()
{
    return std::filesystem::current_path() / "extracted_assets/visual_atlas/sheets";
}

[[nodiscard]] std::optional<std::filesystem::path> visualAtlasSheetForContract(std::string_view contractFileName)
{
    const auto* candidate = atlasCandidateForContract(contractFileName);
    if (!candidate)
        return std::nullopt;
    return visualAtlasSheetRoot() / std::string(candidate->atlasFileName);
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

class GdiPlusSession
{
public:
    GdiPlusSession()
    {
        Gdiplus::GdiplusStartupInput input;
        if (Gdiplus::GdiplusStartup(&m_token, &input, nullptr) != Gdiplus::Ok)
            m_token = 0;
    }

    ~GdiPlusSession()
    {
        if (m_token != 0)
            Gdiplus::GdiplusShutdown(m_token);
    }

    GdiPlusSession(const GdiPlusSession&) = delete;
    GdiPlusSession& operator=(const GdiPlusSession&) = delete;

private:
    ULONG_PTR m_token = 0;
};

void drawTextLine(HDC dc, RECT bounds, const std::string& text, COLORREF color, UINT format = DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_VCENTER)
{
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextA(dc, text.c_str(), static_cast<int>(text.size()), &bounds, format);
}

[[nodiscard]] Gdiplus::RectF clampRectToCanvas(Gdiplus::RectF rect, const Gdiplus::RectF& canvas)
{
    const float maxWidth = std::max(1.0F, canvas.Width - 8.0F);
    const float maxHeight = std::max(1.0F, canvas.Height - 8.0F);
    rect.Width = std::min(rect.Width, maxWidth);
    rect.Height = std::min(rect.Height, maxHeight);

    const float minX = canvas.X + 4.0F;
    const float minY = canvas.Y + 4.0F;
    const float maxX = std::max(minX, canvas.X + canvas.Width - rect.Width - 4.0F);
    const float maxY = std::max(minY, canvas.Y + canvas.Height - rect.Height - 4.0F);
    rect.X = std::max(minX, std::min(rect.X, maxX));
    rect.Y = std::max(minY, std::min(rect.Y, maxY));
    return rect;
}

[[nodiscard]] Gdiplus::RectF layoutLayerRect(const OverlayLayer& layer, std::size_t roleIndex, const Gdiplus::RectF& canvas)
{
    if (layer.role == "counter")
    {
        const float width = std::min(250.0F, canvas.Width * 0.38F);
        return clampRectToCanvas(
            Gdiplus::RectF(canvas.X + 18.0F, canvas.Y + 16.0F + (static_cast<float>(roleIndex) * 22.0F), width, 17.0F),
            canvas);
    }

    if (layer.role == "gauge")
    {
        const float width = std::min(300.0F, canvas.Width * 0.48F);
        return clampRectToCanvas(
            Gdiplus::RectF(canvas.X + 18.0F, canvas.Y + canvas.Height - 88.0F + (static_cast<float>(roleIndex) * 22.0F), width, 17.0F),
            canvas);
    }

    if (layer.role == "sidecar")
    {
        const float width = std::min(210.0F, canvas.Width * 0.30F);
        return clampRectToCanvas(
            Gdiplus::RectF(canvas.X + canvas.Width - width - 18.0F, canvas.Y + 22.0F + (static_cast<float>(roleIndex) * 34.0F), width, 26.0F),
            canvas);
    }

    if (layer.role == "transient_fx")
    {
        const float width = std::min(260.0F, canvas.Width * 0.42F);
        return clampRectToCanvas(
            Gdiplus::RectF(canvas.X + ((canvas.Width - width) / 2.0F), canvas.Y + 22.0F + (static_cast<float>(roleIndex) * 30.0F), width, 24.0F),
            canvas);
    }

    if (layer.role == "prompt")
    {
        const float width = std::min(360.0F, canvas.Width - 32.0F);
        return clampRectToCanvas(
            Gdiplus::RectF(canvas.X + 16.0F, canvas.Y + canvas.Height - 38.0F - (static_cast<float>(roleIndex) * 28.0F), width, 22.0F),
            canvas);
    }

    const float width = std::min(280.0F, canvas.Width * 0.44F);
    return clampRectToCanvas(
        Gdiplus::RectF(canvas.X + 20.0F, canvas.Y + 20.0F + (static_cast<float>(roleIndex) * 24.0F), width, 20.0F),
        canvas);
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
        << "Local atlas candidate: ";

    if (const auto* candidate = atlasCandidateForContract(host.primaryContractFileName))
        text << atlasCandidateDisplayName(*candidate);
    else
        text << "none";

    text
        << "\r\n"
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

[[nodiscard]] int runPreviewSmoke()
{
    std::size_t existingAtlasSheets = 0;
    std::size_t proxyCandidates = 0;
    for (const auto& candidate : kPreviewAtlasCandidates)
    {
        if (isProxyAtlasCandidate(candidate))
            ++proxyCandidates;
        if (std::filesystem::exists(visualAtlasSheetRoot() / std::string(candidate.atlasFileName)))
            ++existingAtlasSheets;
    }

    const auto* title = atlasCandidateByShortName("title");
    const auto* pause = atlasCandidateByShortName("pause");
    const auto* sonicStage = atlasCandidateByShortName("sonic_stage");
    std::cout
        << "sward_ui_runtime_debug_gui preview smoke ok "
        << "atlas_candidates=" << kPreviewAtlasCandidates.size()
        << " proxy_candidates=" << proxyCandidates
        << " existing_local_atlas=" << existingAtlasSheets
        << " title=" << (title ? title->atlasFileName : "none")
        << " pause=" << (pause ? pause->atlasFileName : "none")
        << " sonic_stage=" << (sonicStage ? sonicStage->atlasFileName : "none")
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

        registerPreviewClass();
        RegisterClassA(&windowClass);

        RECT workArea{ 0, 0, 1240, 760 };
        SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0);
        const int workAreaWidth = std::max(900, static_cast<int>(workArea.right - workArea.left));
        const int workAreaHeight = std::max(640, static_cast<int>(workArea.bottom - workArea.top));
        const int windowWidth = std::min(1240, std::max(900, workAreaWidth - 40));
        const int windowHeight = std::min(760, std::max(640, workAreaHeight - 40));
        const int windowX = workArea.left + std::max(0, (workAreaWidth - windowWidth) / 2);
        const int windowY = workArea.top + std::max(0, (workAreaHeight - windowHeight) / 2);

        m_window = CreateWindowExA(
            0,
            windowClass.lpszClassName,
            "SWARD UI Runtime Debug Workbench",
            WS_OVERLAPPEDWINDOW,
            windowX,
            windowY,
            windowWidth,
            windowHeight,
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
    void registerPreviewClass()
    {
        WNDCLASSA previewClass{};
        previewClass.lpfnWndProc = &WorkbenchGui::previewWindowProc;
        previewClass.hInstance = m_instance;
        previewClass.lpszClassName = kPreviewPanelClassName;
        previewClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        previewClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        RegisterClassA(&previewClass);
    }

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

    static LRESULT CALLBACK previewWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* app = reinterpret_cast<WorkbenchGui*>(GetWindowLongPtrA(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            const auto* create = reinterpret_cast<CREATESTRUCTA*>(lParam);
            app = reinterpret_cast<WorkbenchGui*>(create->lpCreateParams);
            SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }

        if (message == WM_ERASEBKGND)
            return 1;

        if (message == WM_PAINT)
        {
            PAINTSTRUCT paint{};
            const HDC dc = BeginPaint(window, &paint);
            if (app)
                app->paintPreview(dc);
            EndPaint(window, &paint);
            return 0;
        }

        return DefWindowProcA(window, message, wParam, lParam);
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
        m_previewPanel = createPreviewPanel();
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
            m_previewPanel,
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

    HWND createPreviewPanel()
    {
        return CreateWindowExA(
            WS_EX_CLIENTEDGE,
            kPreviewPanelClassName,
            "",
            WS_CHILD | WS_VISIBLE,
            0,
            0,
            0,
            0,
            m_window,
            nullptr,
            m_instance,
            this);
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

        const int previewTop = buttonY + buttonHeight + margin;
        const int previewWantedHeight = (rightWidth * 9 / 16) + 46;
        const int previewMaxHeight = std::max(220, height * 45 / 100);
        const int previewHeight = std::min(std::max(220, previewWantedHeight), previewMaxHeight);
        MoveWindow(m_previewPanel, rightX, previewTop, rightWidth, previewHeight, TRUE);

        const int detailsTop = previewTop + previewHeight + margin;
        const int remainingHeight = std::max(160, height - detailsTop - margin);
        const int detailsHeight = std::max(110, (remainingHeight - margin) / 2);
        MoveWindow(m_detailText, rightX, detailsTop, rightWidth, detailsHeight, TRUE);
        MoveWindow(m_logText, rightX, detailsTop + detailsHeight + margin, rightWidth, std::max(80, remainingHeight - detailsHeight - margin), TRUE);
    }

    [[nodiscard]] const ResolvedHostEntry* selectedHostEntry() const
    {
        if (!m_selectedHostIndex.has_value() || *m_selectedHostIndex >= m_hosts.size())
            return nullptr;
        return &m_hosts[*m_selectedHostIndex];
    }

    void paintPreview(HDC dc)
    {
        RECT client{};
        GetClientRect(m_previewPanel, &client);
        const int width = client.right - client.left;
        const int height = client.bottom - client.top;
        if (width <= 0 || height <= 0)
            return;

        Gdiplus::Graphics graphics(dc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.Clear(Gdiplus::Color(255, 246, 247, 244));

        const auto* selected = selectedHostEntry();
        const auto* host = selected ? selected->metadata : nullptr;

        RECT titleRect{ 12, 8, width - 12, 30 };
        std::string title = "Visual Preview";
        if (host)
            title += " - " + std::string(host->hostDisplayName);
        drawTextLine(dc, titleRect, title, RGB(34, 40, 46));

        Gdiplus::RectF bounds(14.0F, 38.0F, static_cast<float>(std::max(1, width - 28)), static_cast<float>(std::max(1, height - 78)));
        const float targetAspect = 16.0F / 9.0F;
        float canvasWidth = bounds.Width;
        float canvasHeight = canvasWidth / targetAspect;
        if (canvasHeight > bounds.Height)
        {
            canvasHeight = bounds.Height;
            canvasWidth = canvasHeight * targetAspect;
        }

        Gdiplus::RectF canvas(
            bounds.X + ((bounds.Width - canvasWidth) / 2.0F),
            bounds.Y + ((bounds.Height - canvasHeight) / 2.0F),
            canvasWidth,
            canvasHeight);

        bool drewAtlas = false;
        std::string atlasLabel = "Local atlas: none";
        if (host)
        {
            if (const auto* candidate = atlasCandidateForContract(host->primaryContractFileName))
            {
                const auto atlasPath = visualAtlasSheetRoot() / std::string(candidate->atlasFileName);
                atlasLabel = "Local atlas: " + atlasCandidateDisplayName(*candidate);
                if (std::filesystem::exists(atlasPath))
                {
                    Gdiplus::Image image(atlasPath.wstring().c_str());
                    if (image.GetLastStatus() == Gdiplus::Ok)
                    {
                        graphics.DrawImage(&image, canvas.X, canvas.Y, canvas.Width, canvas.Height);
                        drewAtlas = true;
                    }
                }
            }
        }

        if (!drewAtlas)
        {
            Gdiplus::SolidBrush canvasBrush(Gdiplus::Color(255, 30, 34, 38));
            graphics.FillRectangle(&canvasBrush, canvas);

            RECT fallbackText{
                static_cast<LONG>(canvas.X + 16.0F),
                static_cast<LONG>(canvas.Y + 16.0F),
                static_cast<LONG>(canvas.X + canvas.Width - 16.0F),
                static_cast<LONG>(canvas.Y + canvas.Height - 16.0F),
            };
            drawTextLine(dc, fallbackText, "No local atlas sheet matched. Rendering contract layers only.", RGB(230, 235, 238), DT_LEFT | DT_WORDBREAK);
        }

        Gdiplus::Pen canvasPen(Gdiplus::Color(255, 38, 45, 52), 2.0F);
        graphics.DrawRectangle(&canvasPen, canvas);

        if (m_runtime)
        {
            const auto layers = m_runtime->visibleLayers();
            for (std::size_t index = 0; index < layers.size(); ++index)
            {
                std::size_t roleIndex = 0;
                for (std::size_t previous = 0; previous < index; ++previous)
                {
                    if (layers[previous].role == layers[index].role)
                        ++roleIndex;
                }
                const Gdiplus::RectF layerRect = layoutLayerRect(layers[index], roleIndex, canvas);

                Gdiplus::SolidBrush layerBrush(Gdiplus::Color(96, 57, 166, 218));
                Gdiplus::Pen layerPen(Gdiplus::Color(220, 245, 248, 250), 1.5F);
                graphics.FillRectangle(&layerBrush, layerRect);
                graphics.DrawRectangle(&layerPen, layerRect);

                RECT layerText{
                    static_cast<LONG>(layerRect.X + 8.0F),
                    static_cast<LONG>(layerRect.Y),
                    static_cast<LONG>(layerRect.X + layerRect.Width - 8.0F),
                    static_cast<LONG>(layerRect.Y + layerRect.Height),
                };
                drawTextLine(dc, layerText, layers[index].id + " : " + layers[index].role, RGB(255, 255, 255));
            }

            const auto prompts = m_runtime->visiblePrompts();
            const float promptTop = canvas.Y + canvas.Height - 38.0F;
            const float promptWidth = prompts.empty() ? 0.0F : std::min(132.0F, (canvas.Width - 32.0F) / static_cast<float>(prompts.size()));
            for (std::size_t index = 0; index < prompts.size(); ++index)
            {
                Gdiplus::RectF promptRect(
                    canvas.X + 16.0F + (static_cast<float>(index) * promptWidth),
                    promptTop,
                    std::max(44.0F, promptWidth - 8.0F),
                    26.0F);
                promptRect = clampRectToCanvas(promptRect, canvas);
                Gdiplus::SolidBrush promptBrush(Gdiplus::Color(228, 247, 211, 72));
                Gdiplus::Pen promptPen(Gdiplus::Color(255, 35, 41, 47), 1.2F);
                graphics.FillRectangle(&promptBrush, promptRect);
                graphics.DrawRectangle(&promptPen, promptRect);

                RECT promptText{
                    static_cast<LONG>(promptRect.X + 6.0F),
                    static_cast<LONG>(promptRect.Y),
                    static_cast<LONG>(promptRect.X + promptRect.Width - 6.0F),
                    static_cast<LONG>(promptRect.Y + promptRect.Height),
                };
                drawTextLine(dc, promptText, std::string(toString(prompts[index].button)) + " " + prompts[index].label, RGB(24, 29, 34));
            }
        }

        if (m_runtime && m_runningContractIndex.has_value())
        {
            const auto& contract = m_contracts[*m_runningContractIndex].contract;
            const double duration = timelineDuration(contract, m_runtime->state());
            const double progress = duration > 0.0 ? std::min(1.0, m_runtime->stateElapsedSeconds() / duration) : (m_runtime->state() == ScreenState::Idle ? 1.0 : 0.0);
            Gdiplus::RectF track(canvas.X, canvas.Y + canvas.Height + 10.0F, canvas.Width, 8.0F);
            Gdiplus::SolidBrush trackBrush(Gdiplus::Color(255, 203, 209, 213));
            Gdiplus::SolidBrush fillBrush(Gdiplus::Color(255, 45, 130, 216));
            graphics.FillRectangle(&trackBrush, track);
            graphics.FillRectangle(&fillBrush, track.X, track.Y, static_cast<float>(track.Width * progress), track.Height);
        }

        RECT footerRect{ 12, height - 30, width - 12, height - 8 };
        if (m_runtime)
        {
            std::ostringstream footer;
            footer << atlasLabel << " | State " << toString(m_runtime->state()) << " | prompts=" << m_runtime->visiblePrompts().size();
            drawTextLine(dc, footerRect, footer.str(), RGB(68, 75, 82));
        }
        else
        {
            drawTextLine(dc, footerRect, atlasLabel + " | Run Host to preview runtime layers and prompt rows.", RGB(68, 75, 82));
        }
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
            << "Contract: " << host.primaryContractFileName << "\r\n";

        if (const auto* candidate = atlasCandidateForContract(host.primaryContractFileName))
            text << "Atlas candidate: " << atlasCandidateDisplayName(*candidate) << "\r\n\r\n";
        else
            text << "Atlas candidate: none\r\n\r\n";

        text
            << host.notes << "\r\n\r\n"
            << "Click Run Host to drive this contract through the runtime.";
        SetWindowTextA(m_detailText, text.str().c_str());
        if (m_previewPanel)
            InvalidateRect(m_previewPanel, nullptr, TRUE);
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
        if (m_previewPanel)
            InvalidateRect(m_previewPanel, nullptr, TRUE);
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
    HWND m_previewPanel = nullptr;
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
        if (command.find("--preview-smoke") != std::string::npos)
            return runPreviewSmoke();
        if (command.find("--smoke") != std::string::npos)
            return runSmoke();

        GdiPlusSession gdiPlus;
        WorkbenchGui app(instance);
        return app.run(showCommand);
    }
    catch (const std::exception& exception)
    {
        MessageBoxA(nullptr, exception.what(), "SWARD UI Runtime Debug GUI failed", MB_ICONERROR | MB_OK);
        return 1;
    }
}
