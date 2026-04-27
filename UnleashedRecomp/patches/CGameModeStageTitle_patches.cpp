#include <api/SWA.h>
#include <patches/CTitleStateIntro_patches.h>
#include <patches/ui_lab_patches.h>
#include <string>

static uint32_t ReadGuestU32(const void* address)
{
    return reinterpret_cast<const be<uint32_t>*>(address)->get();
}

static float ReadGuestFloat(const void* address)
{
    return reinterpret_cast<const be<float>*>(address)->get();
}

static std::string BuildTitleOwnerDetail(const SWA::CGameModeStageTitle* pGameModeStageTitle, uint8_t* base)
{
    const auto gameModeBase = reinterpret_cast<const uint8_t*>(pGameModeStageTitle);
    const auto titleContextGuestAddress = ReadGuestU32(gameModeBase + 0x220);

    auto detail =
        "owner_title_context=" + std::to_string(titleContextGuestAddress) +
        " owner_flag556=" + std::to_string(*(gameModeBase + 0x22C)) +
        " owner_flag558=" + std::to_string(*(gameModeBase + 0x22E)) +
        " owner_gate568=" + std::to_string(*(gameModeBase + 0x238)) +
        " owner_gate570=" + std::to_string(*(gameModeBase + 0x23A)) +
        " owner_timer560_ms=" + std::to_string(static_cast<uint32_t>(ReadGuestFloat(gameModeBase + 0x230) * 1000.0f)) +
        " owner_timer564_ms=" + std::to_string(static_cast<uint32_t>(ReadGuestFloat(gameModeBase + 0x234) * 1000.0f));

    if (!titleContextGuestAddress)
        return detail;

    const auto csdSceneGuestAddress = PPC_LOAD_U32(titleContextGuestAddress + 0x1E8);
    detail +=
        " title_ctx464=" + std::to_string(PPC_LOAD_U8(titleContextGuestAddress + 0x1D0)) +
        " title_ctx465=" + std::to_string(PPC_LOAD_U8(titleContextGuestAddress + 0x1D1)) +
        " title_ctx466=" + std::to_string(PPC_LOAD_U8(titleContextGuestAddress + 0x1D2)) +
        " title_ctx467=" + std::to_string(PPC_LOAD_U8(titleContextGuestAddress + 0x1D3)) +
        " title_ctx468=" + std::to_string(PPC_LOAD_U8(titleContextGuestAddress + 0x1D4)) +
        " title_request=" + std::to_string(PPC_LOAD_U8(titleContextGuestAddress + 0x180)) +
        " title_dirty=" + std::to_string(PPC_LOAD_U8(titleContextGuestAddress + 0x181)) +
        " title_transition=" + std::to_string(PPC_LOAD_U8(titleContextGuestAddress + 0x238)) +
        " title_flag580=" + std::to_string(PPC_LOAD_U8(titleContextGuestAddress + 0x244)) +
        " title_csd488=" + std::to_string(csdSceneGuestAddress);

    if (!csdSceneGuestAddress)
        return detail;

    detail +=
        " csd_byte62=" + std::to_string(PPC_LOAD_U8(csdSceneGuestAddress + 62)) +
        " csd_byte84=" + std::to_string(PPC_LOAD_U8(csdSceneGuestAddress + 84)) +
        " csd_byte152=" + std::to_string(PPC_LOAD_U8(csdSceneGuestAddress + 152)) +
        " csd_byte160=" + std::to_string(PPC_LOAD_U8(csdSceneGuestAddress + 160));

    return detail;
}

// SWA::CGameModeStageTitle::Update
PPC_FUNC_IMPL(__imp__sub_825518B8);
PPC_FUNC(sub_825518B8)
{
    const auto gameModeGuestAddress = ctx.r3.u32;
    auto pGameModeStageTitle = (SWA::CGameModeStageTitle*)g_memory.Translate(ctx.r3.u32);

    __imp__sub_825518B8(ctx, base);

    UiLab::OnGameModeStageTitleContext(
        gameModeGuestAddress,
        pGameModeStageTitle->m_pContext.ptr.get(),
        pGameModeStageTitle->m_pStateMachine.ptr.get(),
        pGameModeStageTitle->m_Time,
        *SWA::SGlobals::ms_IsTitleStateMenu,
        *SWA::SGlobals::ms_IsAutoSaveWarningShown,
        BuildTitleOwnerDetail(pGameModeStageTitle, base));

    if (g_quitMessageOpen)
        pGameModeStageTitle->m_AdvertiseMovieWaitTime = 0;
}
