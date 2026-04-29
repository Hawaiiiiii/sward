#include <kernel/function.h>
#include <kernel/memory.h>
#include <api/SWA.h>
#include <patches/ui_lab_patches.h>

static uint32_t GuestAddressOf(const void* host)
{
    return host != nullptr ? g_memory.MapVirtual(host) : 0;
}

static bool IsPlausibleGuestAddress(uint32_t address)
{
    return address >= 0x10000;
}

static void RecordHudSonicStageInspector(
    uint32_t ownerAddress,
    const SWA::CHudSonicStage* pHudSonicStage,
    const char* hookSource)
{
    if (pHudSonicStage == nullptr)
        return;

    UiLab::OnHudSonicStageUpdate(
        ownerAddress,
        GuestAddressOf(pHudSonicStage->m_rcPlayScreen.Get()),
        GuestAddressOf(pHudSonicStage->m_rcSpeedGauge.Get()),
        GuestAddressOf(pHudSonicStage->m_rcRingEnergyGauge.Get()),
        GuestAddressOf(pHudSonicStage->m_rcGaugeFrame.Get()),
        GuestAddressOf(pHudSonicStage->m_rcScoreCount.Get()),
        GuestAddressOf(pHudSonicStage->m_rcTimeCount.Get()),
        GuestAddressOf(pHudSonicStage->m_rcTimeCount2.Get()),
        GuestAddressOf(pHudSonicStage->m_rcTimeCount3.Get()),
        GuestAddressOf(pHudSonicStage->m_rcPlayerCount.Get()),
        hookSource);
    UiLab::OnHudSonicStageOwnerFieldSample(ownerAddress, hookSource);
}

static void RecordHudSonicStageInspector(uint32_t ownerAddress, const char* hookSource)
{
    if (!IsPlausibleGuestAddress(ownerAddress))
        return;

    auto* pHudSonicStage = reinterpret_cast<SWA::CHudSonicStage*>(g_memory.Translate(ownerAddress));
    RecordHudSonicStageInspector(ownerAddress, pHudSonicStage, hookSource);
}

// CHudSonicStage constructor-family seam. It anchors the owner address early;
// later hooks usually provide the non-null CSD owner fields.
PPC_FUNC_IMPL(__imp__sub_824D89B0);
PPC_FUNC(sub_824D89B0)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    __imp__sub_824D89B0(ctx, base);
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage owner hook sub_824D89B0");
}

// CHudSonicStage stage/load binding seam.
PPC_FUNC_IMPL(__imp__sub_824D9308);
PPC_FUNC(sub_824D9308)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    __imp__sub_824D9308(ctx, base);
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage owner hook sub_824D9308");
}

// CHudSonicStage runtime scene/control seam.
PPC_FUNC_IMPL(__imp__sub_824D95F8);
PPC_FUNC(sub_824D95F8)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    __imp__sub_824D95F8(ctx, base);
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage owner hook sub_824D95F8");
}

// CHudSonicStage value update hooks. These are deliberately context hooks:
// the nested CSD RCPtr/GetChild/SetText hooks do the exact node/path capture.
PPC_FUNC_IMPL(__imp__sub_824D6048);
PPC_FUNC(sub_824D6048)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    UiLab::PushSonicHudUpdateContext(ownerAddress, "sonic-hud-update-context CHudSonicStage/sub_824D6048");
    __imp__sub_824D6048(ctx, base);
    UiLab::PopSonicHudUpdateContext("sonic-hud-update-context CHudSonicStage/sub_824D6048");
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage value update hook sub_824D6048");
}

PPC_FUNC_IMPL(__imp__sub_824D6418);
PPC_FUNC(sub_824D6418)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    UiLab::PushSonicHudUpdateContext(ownerAddress, "sonic-hud-update-context CHudSonicStage/sub_824D6418");
    __imp__sub_824D6418(ctx, base);
    UiLab::PopSonicHudUpdateContext("sonic-hud-update-context CHudSonicStage/sub_824D6418");
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage value update hook sub_824D6418");
}

PPC_FUNC_IMPL(__imp__sub_824D69B0);
PPC_FUNC(sub_824D69B0)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    UiLab::PushSonicHudUpdateContext(ownerAddress, "sonic-hud-update-context CHudSonicStage/sub_824D69B0");
    __imp__sub_824D69B0(ctx, base);
    UiLab::PopSonicHudUpdateContext("sonic-hud-update-context CHudSonicStage/sub_824D69B0");
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage value update hook sub_824D69B0");
}

PPC_FUNC_IMPL(__imp__sub_824D6C18);
PPC_FUNC(sub_824D6C18)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    UiLab::PushSonicHudUpdateContext(ownerAddress, "sonic-hud-update-context CHudSonicStage/sub_824D6C18");
    __imp__sub_824D6C18(ctx, base);
    UiLab::PopSonicHudUpdateContext("sonic-hud-update-context CHudSonicStage/sub_824D6C18");
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage value update hook sub_824D6C18");
}

PPC_FUNC_IMPL(__imp__sub_824D7100);
PPC_FUNC(sub_824D7100)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    UiLab::PushSonicHudUpdateContext(ownerAddress, "sonic-hud-update-context CHudSonicStage/sub_824D7100");
    __imp__sub_824D7100(ctx, base);
    UiLab::PopSonicHudUpdateContext("sonic-hud-update-context CHudSonicStage/sub_824D7100");
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage value update hook sub_824D7100");
}
