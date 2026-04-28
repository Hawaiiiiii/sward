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
