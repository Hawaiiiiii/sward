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

static void ForwardTitleOwnerContext(
    const SWA::CGameModeStageTitle* pGameModeStageTitle,
    uint8_t* base,
    bool isTitleStateMenu)
{
    const auto gameModeBase = reinterpret_cast<const uint8_t*>(pGameModeStageTitle);
    const auto titleContextGuestAddress = ReadGuestU32(gameModeBase + 0x220);
    const bool ownerGate568 = *(gameModeBase + 0x238) != 0;
    const bool ownerGate570 = *(gameModeBase + 0x23A) != 0;

    uint32_t csdSceneGuestAddress = 0;
    uint8_t titleRequest = 0;
    uint8_t titleDirty = 0;
    uint8_t titleTransition = 0;
    uint8_t titleFlag580 = 0;
    uint8_t csdByte62 = 0;
    uint8_t csdByte84 = 0;
    uint8_t csdByte152 = 0;
    uint8_t csdByte160 = 0;

    if (titleContextGuestAddress)
    {
        csdSceneGuestAddress = PPC_LOAD_U32(titleContextGuestAddress + 0x1E8);
        titleRequest = PPC_LOAD_U8(titleContextGuestAddress + 0x180);
        titleDirty = PPC_LOAD_U8(titleContextGuestAddress + 0x181);
        titleTransition = PPC_LOAD_U8(titleContextGuestAddress + 0x238);
        titleFlag580 = PPC_LOAD_U8(titleContextGuestAddress + 0x244);

        if (csdSceneGuestAddress)
        {
            csdByte62 = PPC_LOAD_U8(csdSceneGuestAddress + 62);
            csdByte84 = PPC_LOAD_U8(csdSceneGuestAddress + 84);
            csdByte152 = PPC_LOAD_U8(csdSceneGuestAddress + 152);
            csdByte160 = PPC_LOAD_U8(csdSceneGuestAddress + 160);
        }
    }

    UiLab::OnTitleOwnerContext(
        isTitleStateMenu,
        titleContextGuestAddress,
        csdSceneGuestAddress,
        ownerGate568,
        ownerGate570,
        titleRequest,
        titleDirty,
        titleTransition,
        titleFlag580,
        csdByte62,
        csdByte84,
        csdByte152,
        csdByte160);
}

static void ArmStageTitleOwnerDirectState(
    uint8_t* base,
    uint32_t gameModeGuestAddress,
    const SWA::CGameModeStageTitle* pGameModeStageTitle)
{
    const auto gameModeBase = reinterpret_cast<const uint8_t*>(pGameModeStageTitle);
    const auto titleContextGuestAddress = ReadGuestU32(gameModeBase + 0x220);

    if (!titleContextGuestAddress || !UiLab::ShouldRefreshStageTitleOwnerDirectState())
        return;

    const auto titleCsdAddress = PPC_LOAD_U32(titleContextGuestAddress + 0x1E8);

    PPC_STORE_U8(titleContextGuestAddress + 0x180, 1);
    PPC_STORE_U8(titleContextGuestAddress + 0x181, 1);
    PPC_STORE_U8(titleContextGuestAddress + 0x1D1, 1);
    PPC_STORE_U8(titleContextGuestAddress + 0x238, 1);
    PPC_STORE_U8(gameModeGuestAddress + 0x238, 1);

    if (titleCsdAddress)
        PPC_STORE_U8(titleCsdAddress + 84, 1);

    UiLab::OnStageTitleOwnerDirectStateApplied(
        titleContextGuestAddress,
        titleCsdAddress,
        PPC_LOAD_U8(titleContextGuestAddress + 0x180),
        PPC_LOAD_U8(titleContextGuestAddress + 0x181),
        PPC_LOAD_U8(titleContextGuestAddress + 0x238),
        PPC_LOAD_U8(titleContextGuestAddress + 0x1D1),
        PPC_LOAD_U8(gameModeGuestAddress + 0x238),
        titleCsdAddress ? PPC_LOAD_U8(titleCsdAddress + 84) : 0);
}

static bool IsPlausibleTitleGuestAddress(uint32_t address)
{
    return address >= 0x10000;
}

static uint32_t ReadTitleContextScenePointer(uint8_t* base, uint32_t titleContextGuestAddress)
{
    if (!IsPlausibleTitleGuestAddress(titleContextGuestAddress))
        return 0;

    return PPC_LOAD_U32(titleContextGuestAddress + 0x1E8);
}

static void RecordTitleOwnerSetterProbe(
    const char* helperName,
    const char* phase,
    uint32_t ownerAddress,
    uint32_t fieldOffset,
    uint32_t fieldValue,
    const PPCContext& ctx,
    uint32_t resultR3)
{
    UiLab::OnTitleOwnerSetterProbe(
        helperName,
        phase,
        ownerAddress,
        fieldOffset,
        fieldValue,
        ctx.r3.u32,
        ctx.r4.u32,
        ctx.r5.u32,
        ctx.r6.u32,
        ctx.r7.u32,
        resultR3);
}

static void RecordTitleContextScenePointerProbe(
    const char* helperName,
    const char* phase,
    const PPCContext& ctx,
    uint32_t resultR3,
    uint8_t* base)
{
    const uint32_t titleContextGuestAddress = ctx.r3.u32;
    const uint32_t titleCsdAddress = ReadTitleContextScenePointer(base, titleContextGuestAddress);

    RecordTitleOwnerSetterProbe(
        helperName,
        phase,
        titleContextGuestAddress,
        0x1E8,
        titleCsdAddress,
        ctx,
        resultR3);
}

// SWA::CGameModeStageTitle::Update
PPC_FUNC_IMPL(__imp__sub_825518B8);
PPC_FUNC(sub_825518B8)
{
    const auto gameModeGuestAddress = ctx.r3.u32;
    auto pGameModeStageTitle = (SWA::CGameModeStageTitle*)g_memory.Translate(ctx.r3.u32);

    __imp__sub_825518B8(ctx, base);

    ArmStageTitleOwnerDirectState(base, gameModeGuestAddress, pGameModeStageTitle);

    const bool isTitleStateMenu = *SWA::SGlobals::ms_IsTitleStateMenu;

    UiLab::OnGameModeStageTitleContext(
        gameModeGuestAddress,
        pGameModeStageTitle->m_pContext.ptr.get(),
        pGameModeStageTitle->m_pStateMachine.ptr.get(),
        pGameModeStageTitle->m_Time,
        isTitleStateMenu,
        *SWA::SGlobals::ms_IsAutoSaveWarningShown,
        BuildTitleOwnerDetail(pGameModeStageTitle, base));
    ForwardTitleOwnerContext(pGameModeStageTitle, base, isTitleStateMenu);

    if (g_quitMessageOpen)
        pGameModeStageTitle->m_AdvertiseMovieWaitTime = 0;
}

// Title/menu transition helper candidate. Read-only probe for the attach path
// feeding title scene changes; no owner pointers are modified here.
PPC_FUNC_IMPL(__imp__sub_8250F2B8);
PPC_FUNC(sub_8250F2B8)
{
    const PPCContext input = ctx;
    RecordTitleOwnerSetterProbe(
        "sub_8250F2B8",
        "pre-original",
        input.r3.u32,
        0xFFFFFFFFu,
        0,
        input,
        0);

    __imp__sub_8250F2B8(ctx, base);

    RecordTitleOwnerSetterProbe(
        "sub_8250F2B8",
        "post-original",
        input.r3.u32,
        0xFFFFFFFFu,
        0,
        input,
        ctx.r3.u32);
}

// Title context resource scene pointer helper: returns titleContext+0x1E8 + 0x40.
PPC_FUNC_IMPL(__imp__sub_82581288);
PPC_FUNC(sub_82581288)
{
    const PPCContext input = ctx;
    RecordTitleContextScenePointerProbe("sub_82581288", "pre-original", input, 0, base);
    __imp__sub_82581288(ctx, base);
    RecordTitleContextScenePointerProbe("sub_82581288", "post-original", input, ctx.r3.u32, base);
}

// Title context resource scene readiness/check helper.
PPC_FUNC_IMPL(__imp__sub_825812A8);
PPC_FUNC(sub_825812A8)
{
    const PPCContext input = ctx;
    RecordTitleContextScenePointerProbe("sub_825812A8", "pre-original", input, 0, base);
    __imp__sub_825812A8(ctx, base);
    RecordTitleContextScenePointerProbe("sub_825812A8", "post-original", input, ctx.r3.u32, base);
}

// Title context visibility/output gate helper: combines titleContext+0x1E8 and
// titleContext+0x1D1. This is still read-only evidence for the real attach path.
PPC_FUNC_IMPL(__imp__sub_825812E8);
PPC_FUNC(sub_825812E8)
{
    const PPCContext input = ctx;
    RecordTitleContextScenePointerProbe("sub_825812E8", "pre-original", input, 0, base);
    __imp__sub_825812E8(ctx, base);
    RecordTitleContextScenePointerProbe("sub_825812E8", "post-original", input, ctx.r3.u32, base);
}
