#include <api/SWA.h>
#include <cpu/guest_stack_var.h>
#include <locale/locale.h>
#include <os/logger.h>
#include <ui/button_guide.h>
#include <ui/fader.h>
#include <ui/message_window.h>
#include <ui/options_menu.h>
#include <user/achievement_manager.h>
#include <user/paths.h>
#include <app.h>
#include <exports.h>
#include <patches/ui_lab_patches.h>

static bool g_installMessageOpen = false;
static bool g_installMessageFaderBegun = false;
static int g_installMessageResult = -1;

static bool ProcessInstallMessage()
{
    if (!g_installMessageOpen)
        return false;

    if (g_installMessageFaderBegun)
        return true;

    auto& str = App::s_isMissingDLC
        ? Localise("Installer_Message_TitleMissingDLC")
        : Localise("Installer_Message_Title");

    std::array<std::string, 2> options = { Localise("Common_Yes"), Localise("Common_No") };

    if (MessageWindow::Open(str, &g_installMessageResult, options, 1) == MSG_CLOSED)
    {
        switch (g_installMessageResult)
        {
            case 0:
                Fader::FadeOut(1, []() { App::Restart({ "--install-dlc" }); });
                g_installMessageFaderBegun = true;
                break;

            case 1:
                g_installMessageOpen = false;
                g_installMessageResult = -1;
                break;
        }
    }

    return true;
}

static void InjectTitleAccept(SWA::SPadState& padState)
{
    constexpr uint32_t acceptMask = SWA::eKeyState_A | SWA::eKeyState_Start;
    padState.DownState = (uint32_t)padState.DownState | acceptMask;
    padState.TappedState = (uint32_t)padState.TappedState | acceptMask;
}

static void SuppressTitleAccept(SWA::SPadState& padState)
{
    constexpr uint32_t acceptMask = SWA::eKeyState_A | SWA::eKeyState_Start;
    padState.DownState = (uint32_t)padState.DownState & ~acceptMask;
    padState.TappedState = (uint32_t)padState.TappedState & ~acceptMask;
}

static uint32_t ReadGuestU32(const void* address)
{
    return reinterpret_cast<const be<uint32_t>*>(address)->get();
}

static uint32_t GuestAddressOf(const void* host)
{
    return host != nullptr ? g_memory.MapVirtual(host) : 0;
}

static void RecordGeneralWindowInspector(const SWA::CGameDocument* pGameDocument)
{
    if (pGameDocument == nullptr || !pGameDocument->m_pMember || !pGameDocument->m_pMember->m_pGeneralWindow)
        return;

    const auto* pGeneralWindow = pGameDocument->m_pMember->m_pGeneralWindow.get();
    UiLab::OnGeneralWindowUpdate(
        GuestAddressOf(pGeneralWindow),
        GuestAddressOf(pGeneralWindow->m_rcGeneral.Get()),
        GuestAddressOf(pGeneralWindow->m_rcBg.Get()),
        static_cast<uint32_t>(pGeneralWindow->m_Status),
        static_cast<uint32_t>(pGeneralWindow->m_CursorIndex),
        static_cast<uint32_t>(pGeneralWindow->m_SelectedIndex));
}

static void ApplyDirectTitleMenuContext(SWA::CTitleMenu& titleMenu)
{
    titleMenu.m_CursorIndex = 0;
    titleMenu.m_Field3C = true;
    titleMenu.m_Field54 = true;
    titleMenu.m_Field9A = true;
}

// SWA::CTitleStateMenu::Update
PPC_FUNC_IMPL(__imp__sub_825882B8);
PPC_FUNC(sub_825882B8)
{
    auto pTitleStateMenu = (SWA::CTitleStateMenu*)g_memory.Translate(ctx.r3.u32);
    auto pGameDocument = SWA::CGameDocument::GetInstance();
    RecordGeneralWindowInspector(pGameDocument);

    auto pInputState = SWA::CInputState::GetInstance();
    auto& pPadState = pInputState->m_PadStates[(uint32_t)pInputState->m_CurrentPadStateIndex];

    auto pContext = pTitleStateMenu->GetContextBase<SWA::CTitleStateMenu::CTitleStateMenuContext>();
    auto pTitleMenu = pContext->m_pTitleMenu.get();
    UiLab::OnTitleStateMenuUpdate(pTitleMenu->m_CursorIndex);

    const auto contextBase = reinterpret_cast<const uint8_t*>(pContext);
    UiLab::OnTitleMenuContext(
        ReadGuestU32(contextBase + 0x1D8),
        ReadGuestU32(contextBase + 0x1E0),
        ReadGuestU32(contextBase + 0x1E8),
        ReadGuestU32(contextBase + 0x240),
        *(contextBase + 0x244),
        pTitleMenu->m_CursorIndex,
        pTitleMenu->m_Field3C,
        pTitleMenu->m_Field54,
        pTitleMenu->m_Field9A);

    auto forcedCursorIndex = (int32_t)pTitleMenu->m_CursorIndex;
    bool injectAccept = false;
    bool suppressAccept = false;
    bool directContext = false;

    if (UiLab::ApplyTitleMenuStateForcing(forcedCursorIndex, injectAccept, suppressAccept, directContext))
        pTitleMenu->m_CursorIndex = (uint32_t)forcedCursorIndex;

    if (suppressAccept)
        SuppressTitleAccept(pPadState);

    if (directContext)
        ApplyDirectTitleMenuContext(*pTitleMenu);

    if (injectAccept)
        InjectTitleAccept(pPadState);

    auto isAccepted = pPadState.IsTapped(SWA::eKeyState_A) || pPadState.IsTapped(SWA::eKeyState_Start);

    auto isNewGameIndex = pTitleMenu->m_CursorIndex == 0;
    auto isOptionsIndex = pTitleMenu->m_CursorIndex == 2;
    auto isInstallIndex = pTitleMenu->m_CursorIndex == 3;

    // Always default to New Game with corrupted save data.
    if (App::s_isSaveDataCorrupt && pTitleMenu->m_CursorIndex == 1)
        pTitleMenu->m_CursorIndex = 0;

    if (isNewGameIndex && isAccepted)
    {
        if (pTitleMenu->m_IsDeleteCheckMessageOpen &&
            pGameDocument->m_pMember->m_pGeneralWindow->m_SelectedIndex == 1)
        {
            LOGN("Resetting achievements...");

            AchievementManager::Reset();
        }
    }
    else if (!OptionsMenu::s_isVisible && isOptionsIndex)
    {
        if (OptionsMenu::s_isRestartRequired)
        {
            static int result = -1;

            if (MessageWindow::Open(Localise("Options_Message_Restart"), &result) == MSG_CLOSED)
                Fader::FadeOut(1, []() { App::Restart(); });
        }
        else if (isAccepted)
        {
            Game_PlaySound("sys_worldmap_window");
            Game_PlaySound("sys_worldmap_decide");
            OptionsMenu::Open();
        }
    }
    else if (isInstallIndex && isAccepted)
    {
        g_installMessageOpen = true;
    }

    if (!OptionsMenu::s_isVisible &&
        !OptionsMenu::s_isRestartRequired &&
        !UiLab::ShouldHoldTitleMenuRuntime() &&
        !ProcessInstallMessage())
    {
        __imp__sub_825882B8(ctx, base);
    }

    if (isOptionsIndex)
    {
        if (OptionsMenu::CanClose() && pPadState.IsTapped(SWA::eKeyState_B))
        {
            Game_PlaySound("sys_worldmap_cansel");
            OptionsMenu::Close();
        }
    }

    RecordGeneralWindowInspector(pGameDocument);
}

void TitleMenuRemoveContinueOnCorruptSaveMidAsmHook(PPCRegister& r3)
{
    if (!App::s_isSaveDataCorrupt)
        return;

    r3.u64 = 0;
}

void TitleMenuRemoveStorageDeviceOptionMidAsmHook(PPCRegister& r11)
{
    r11.u32 = 0;
}

void TitleMenuAddInstallOptionMidAsmHook(PPCRegister& r3)
{
    r3.u32 = 1;
}
