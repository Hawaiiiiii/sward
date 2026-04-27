#include "CTitleStateIntro_patches.h"
#include <api/SWA.h>
#include <install/update_checker.h>
#include <locale/locale.h>
#include <os/logger.h>
#include <ui/fader.h>
#include <ui/message_window.h>
#include <user/achievement_manager.h>
#include <user/paths.h>
#include <app.h>
#include <patches/ui_lab_patches.h>

static std::atomic<bool> g_faderBegun = false;

bool g_quitMessageOpen = false;
static int g_quitMessageResult = -1;

static std::atomic<bool> g_corruptSaveMessageOpen = false;
static int g_corruptSaveMessageResult = -1;

static std::atomic<bool> g_corruptAchievementsMessageOpen = false;
static int g_corruptAchievementsMessageResult = -1;

static std::atomic<bool> g_updateAvailableMessageOpen = false;
static int g_updateAvailableMessageResult = -1;

static bool ProcessQuitMessage()
{
    if (!g_quitMessageOpen)
        return false;

    std::array<std::string, 2> options = { Localise("Common_Yes"), Localise("Common_No") };

    if (MessageWindow::Open(Localise("Title_Message_Quit"), &g_quitMessageResult, options, 1) == MSG_CLOSED)
    {
        if (!g_quitMessageResult)
        {
            Fader::FadeOut(1, []() { App::Exit(); });
            g_faderBegun = true;
        }

        g_quitMessageOpen = false;
        g_quitMessageResult = -1;
    }

    return true;
}

static bool ProcessCorruptSaveMessage()
{
    if (!g_corruptSaveMessageOpen)
        return false;

    if (MessageWindow::Open(Localise("Title_Message_SaveDataCorrupt"), &g_corruptSaveMessageResult) == MSG_CLOSED)
    {
        g_corruptSaveMessageOpen = false;
        g_corruptSaveMessageOpen.notify_one();
        g_corruptSaveMessageResult = -1;
    }

    return true;
}

static bool ProcessCorruptAchievementsMessage()
{
    if (!g_corruptAchievementsMessageOpen)
        return false;

    auto message = AchievementManager::BinStatus == EAchBinStatus::IOError
        ? Localise("Title_Message_AchievementDataIOError")
        : Localise("Title_Message_AchievementDataCorrupt");

    if (MessageWindow::Open(message, &g_corruptAchievementsMessageResult) == MSG_CLOSED)
    {
        // Create a new save file if the file was successfully loaded and failed validation.
        // If the file couldn't be opened, restarting may fix this error, so it isn't worth clearing the data for.
        if (AchievementManager::BinStatus != EAchBinStatus::IOError)
            AchievementManager::SaveBinary(true);

        g_corruptAchievementsMessageOpen = false;
        g_corruptAchievementsMessageOpen.notify_one();
        g_corruptAchievementsMessageResult = -1;
    }

    return true;
}

static bool ProcessUpdateAvailableMessage()
{
    if (!g_updateAvailableMessageOpen)
        return false;

    std::array<std::string, 2> options = { Localise("Common_Yes"), Localise("Common_No") };

    if (MessageWindow::Open(Localise("Title_Message_UpdateAvailable"), &g_updateAvailableMessageResult, options) == MSG_CLOSED)
    {
        if (!g_updateAvailableMessageResult)
        {
            Fader::FadeOut(1,
            //
                []()
                {
                    UpdateChecker::visitWebsite();
                    App::Exit();
                }
            );

            g_faderBegun = true;
        }

        g_updateAvailableMessageOpen = false;
        g_updateAvailableMessageOpen.notify_one();
        g_updateAvailableMessageResult = -1;
    }

    return true;
}

static void InjectTitleAccept()
{
    auto pInputState = SWA::CInputState::GetInstance();
    if (!pInputState)
        return;

    constexpr uint32_t acceptMask = SWA::eKeyState_A | SWA::eKeyState_Start;
    auto& padState = pInputState->m_PadStates[(uint32_t)pInputState->m_CurrentPadStateIndex];
    padState.DownState = (uint32_t)padState.DownState | acceptMask;
    padState.TappedState = (uint32_t)padState.TappedState | acceptMask;
}

static uint32_t ReadGuestU32(const void* address)
{
    return reinterpret_cast<const be<uint32_t>*>(address)->get();
}

PPC_FUNC_IMPL(__imp__sub_825811C8);

static uint8_t ArmTitleIntroCsdCompletion(uint8_t* base, uint32_t titleStateGuestAddress)
{
    if (!UiLab::ShouldArmTitleIntroCsdCompletion() || !titleStateGuestAddress)
        return 0;

    const auto titleContextGuestAddress = PPC_LOAD_U32(titleStateGuestAddress + 8);
    if (!titleContextGuestAddress)
        return 0;

    const auto csdSceneGuestAddress = PPC_LOAD_U32(titleContextGuestAddress + 0x1E8);
    if (!csdSceneGuestAddress)
        return 0;

    auto csdCompleteArmed = PPC_LOAD_U8(csdSceneGuestAddress + 84);
    if (csdCompleteArmed == 0)
    {
        PPC_STORE_U8(csdSceneGuestAddress + 84, 1);
        csdCompleteArmed = 1;
    }

    return csdCompleteArmed;
}

static void RequestTitleIntroDirectState(PPCContext& ctx, uint8_t* base, uint32_t titleStateGuestAddress, uint8_t csdCompleteArmed)
{
    if (!titleStateGuestAddress)
        return;

    const auto titleContextGuestAddress = PPC_LOAD_U32(titleStateGuestAddress + 8);
    if (!titleContextGuestAddress)
        return;

    const auto savedR3 = ctx.r3;
    const auto savedR4 = ctx.r4;

    ctx.r3.u32 = titleContextGuestAddress;
    ctx.r4.u32 = 1;
    __imp__sub_825811C8(ctx, base);

    const auto requestedState = PPC_LOAD_U8(titleContextGuestAddress + 0x180);
    auto dirtyFlag = PPC_LOAD_U8(titleContextGuestAddress + 0x181);
    if (requestedState == 1 && dirtyFlag == 0)
    {
        PPC_STORE_U8(titleContextGuestAddress + 0x181, 1);
        dirtyFlag = 1;
    }

    auto transitionArmed = PPC_LOAD_U8(titleContextGuestAddress + 0x238);
    if (transitionArmed == 0)
    {
        PPC_STORE_U8(titleContextGuestAddress + 0x238, 1);
        transitionArmed = 1;
    }

    auto outputArmed = PPC_LOAD_U8(titleContextGuestAddress + 0x1D1);
    if (UiLab::ShouldArmTitleIntroOwnerOutput() && outputArmed == 0)
    {
        PPC_STORE_U8(titleContextGuestAddress + 0x1D1, 1);
        outputArmed = 1;
    }

    ctx.r3 = savedR3;
    ctx.r4 = savedR4;

    UiLab::OnTitleIntroDirectStateApplied(requestedState, dirtyFlag, transitionArmed, outputArmed, csdCompleteArmed);
}

void StorageDevicePromptMidAsmHook() {}

// Save data validation hook.
PPC_FUNC_IMPL(__imp__sub_822C55B0);
PPC_FUNC(sub_822C55B0)
{
    App::s_isSaveDataCorrupt = true;
    g_corruptSaveMessageOpen = true;
    g_corruptSaveMessageOpen.wait(true);
    ctx.r3.u32 = 0;
}

void PressStartSaveLoadThreadMidAsmHook()
{
    if (UiLab::ShouldBypassStartupPromptBlockers())
        return;

    if (UpdateChecker::check() == UpdateChecker::Result::UpdateAvailable)
    {
        g_updateAvailableMessageOpen = true;
        g_updateAvailableMessageOpen.wait(true);
        g_faderBegun.wait(true);
    }

    if (!AchievementManager::LoadBinary())
        LOGFN_ERROR("Failed to load achievement data... (status code {})", (int)AchievementManager::BinStatus);

    if (AchievementManager::BinStatus != EAchBinStatus::Success)
    {
        g_corruptAchievementsMessageOpen = true;
        g_corruptAchievementsMessageOpen.wait(true);
    }
}

// SWA::CTitleStateIntro::Update
PPC_FUNC_IMPL(__imp__sub_82587E50);
PPC_FUNC(sub_82587E50)
{
    const auto titleStateGuestAddress = ctx.r3.u32;
    auto pTitleStateIntro = (SWA::CTitleStateIntro*)g_memory.Translate(ctx.r3.u32);
    auto pContext = pTitleStateIntro->GetContextBase();
    auto pTime = (be<float>*)((uint8_t*)pContext + 0x10C);
    UiLab::OnTitleStateIntroUpdate(*pTime);

    const auto contextBase = reinterpret_cast<const uint8_t*>(pContext);
    UiLab::OnTitleIntroContext(
        pTitleStateIntro->m_pContext.ptr.get(),
        pTitleStateIntro->m_pStateMachine.ptr.get(),
        *pTime,
        *(contextBase + 0x180),
        *(contextBase + 0x181),
        *(contextBase + 0x238),
        *(contextBase + 0x244),
        ReadGuestU32(contextBase + 0x1D8),
        ReadGuestU32(contextBase + 0x1E0),
        ReadGuestU32(contextBase + 0x1E8));

    bool directState = false;
    if (UiLab::ApplyTitleIntroStateForcing(*pTime, directState))
        InjectTitleAccept();

    const auto csdCompleteArmed = directState ? ArmTitleIntroCsdCompletion(base, titleStateGuestAddress) : 0;

    if (*SWA::SGlobals::ms_IsAutoSaveWarningShown)
    {
        __imp__sub_82587E50(ctx, base);
    }
    else if (!ProcessUpdateAvailableMessage() && !ProcessCorruptSaveMessage() && !ProcessCorruptAchievementsMessage() && !g_faderBegun)
    {
        if (auto pInputState = SWA::CInputState::GetInstance())
        {
            if (pInputState->GetPadState().IsTapped(SWA::eKeyState_B) && *pTime > 0.5f)
                g_quitMessageOpen = true;
        }

        if (!ProcessQuitMessage())
            __imp__sub_82587E50(ctx, base);
    }

    if (directState)
        RequestTitleIntroDirectState(ctx, base, titleStateGuestAddress, csdCompleteArmed);
}
