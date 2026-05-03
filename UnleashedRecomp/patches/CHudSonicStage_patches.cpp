#include <kernel/function.h>
#include <kernel/memory.h>
#include <api/SWA.h>
#include <patches/ui_lab_patches.h>
#include <climits>
#include <cstring>
#include <string_view>

static bool IsPlausibleGuestAddress(uint32_t address)
{
    return address >= 0x10000;
}

static thread_local uint32_t g_sonicHudSpeedReadoutOwnerAddress = 0;
static thread_local uint32_t g_sonicHudSpeedReadoutCaptureDepth = 0;

static uint32_t ReadGuestU32ForCtWriter(uint8_t* base, uint32_t address)
{
    if (!IsPlausibleGuestAddress(address))
        return 0;

    return PPC_LOAD_U32(address);
}

static float FloatFromGuestU32(uint32_t value)
{
    float result = 0.0f;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

static int32_t SaturatingSignedDelta(uint32_t previousValue, uint32_t value)
{
    const int64_t delta =
        static_cast<int64_t>(value) - static_cast<int64_t>(previousValue);
    if (delta > INT32_MAX)
        return INT32_MAX;
    if (delta < INT32_MIN)
        return INT32_MIN;
    return static_cast<int32_t>(delta);
}

static void RecordHudSonicStageInspector(
    uint32_t ownerAddress,
    const SWA::CHudSonicStage* pHudSonicStage,
    const char* hookSource)
{
    if (pHudSonicStage == nullptr)
        return;

    // The raw owner hook can run while the embedded RCPtr slots are still
    // transient. Do not call RCPtr::Get() here; the live bridge samples the raw
    // slot bytes separately and CSD tree/draw-list hooks resolve real nodes.
    UiLab::OnHudSonicStageUpdate(
        ownerAddress,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        hookSource);

    const std::string_view source(hookSource);
    if (source.find("value update hook") == std::string_view::npos)
        UiLab::OnHudSonicStageOwnerFieldSample(ownerAddress, hookSource);
}

static void RecordHudSonicStageInspector(uint32_t ownerAddress, const char* hookSource)
{
    if (!IsPlausibleGuestAddress(ownerAddress))
        return;

    auto* pHudSonicStage = reinterpret_cast<SWA::CHudSonicStage*>(g_memory.Translate(ownerAddress));
    RecordHudSonicStageInspector(ownerAddress, pHudSonicStage, hookSource);
}

static void RecordHudSonicStageCallsiteSample(
    uint32_t ownerAddress,
    const char* hookName,
    const char* samplePhase,
    const PPCContext& ctx)
{
    if (!IsPlausibleGuestAddress(ownerAddress))
        return;

    UiLab::OnSonicHudUpdateCallsiteSample(
        ownerAddress,
        hookName,
        samplePhase,
        ctx.f1.f64,
        ctx.r4.u32);
}

static void RecordCtGameplayWriterProbe(
    const char* valueName,
    const char* callsite,
    const char* phase,
    uint32_t ownerAddress,
    const PPCContext& ctx,
    const char* hookSource)
{
    UiLab::OnSonicHudCtGameplayWriterProbe(
        valueName,
        callsite,
        phase,
        ownerAddress,
        ctx.r3.u32,
        ctx.r4.u32,
        ctx.r5.u32,
        ctx.r6.u32,
        ctx.r7.u32,
        ctx.r8.u32,
        ctx.r9.u32,
        ctx.r10.u32,
        hookSource);
}

// Phase 211/217: CT-anchored gameplay writer seams. These are not UI nodes by
// themselves; they anchor the gameplay values that the Sonic HUD later mirrors.
PPC_FUNC_IMPL(__imp__sub_82519FE8);
PPC_FUNC(sub_82519FE8)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    RecordCtGameplayWriterProbe(
        "ringCount",
        "sub_82519FE8",
        "entry",
        ownerAddress,
        ctx,
        "ct-anchored-gameplay-writer-probe:rings");

    uint32_t storageAddress = 0;
    uint32_t previousValue = 0;

    if (IsPlausibleGuestAddress(ownerAddress))
    {
        const uint32_t storageBase = ReadGuestU32ForCtWriter(base, ownerAddress + 148);
        storageAddress = storageBase + 34216;
        previousValue = ReadGuestU32ForCtWriter(base, storageAddress);
    }

    __imp__sub_82519FE8(ctx, base);

    if (!IsPlausibleGuestAddress(storageAddress))
        return;

    const uint32_t value = ReadGuestU32ForCtWriter(base, storageAddress);
    UiLab::OnSonicHudCtGameplayWriter(
        "ringCount",
        "sub_82519FE8",
        ownerAddress,
        storageAddress,
        previousValue,
        value,
        SaturatingSignedDelta(previousValue, value),
        0.0f,
        false,
        "ct-anchored-gameplay-writer:rings");
}

PPC_FUNC_IMPL(__imp__sub_82A50838);
PPC_FUNC(sub_82A50838)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    RecordCtGameplayWriterProbe(
        "boostGauge",
        "sub_82A50838",
        "entry",
        ownerAddress,
        ctx,
        "ct-anchored-gameplay-writer-probe:day-boost-code-entry");

    const uint32_t storageAddress = IsPlausibleGuestAddress(ownerAddress)
        ? ownerAddress + 104
        : 0;
    const uint32_t previousValue = ReadGuestU32ForCtWriter(base, storageAddress);

    __imp__sub_82A50838(ctx, base);

    if (!IsPlausibleGuestAddress(storageAddress))
        return;

    const uint32_t value = ReadGuestU32ForCtWriter(base, storageAddress);
    UiLab::OnSonicHudCtGameplayWriter(
        "boostGauge",
        "sub_82A50838",
        ownerAddress,
        storageAddress,
        previousValue,
        value,
        SaturatingSignedDelta(previousValue, value),
        FloatFromGuestU32(value),
        true,
        "ct-anchored-gameplay-writer:day-boost");
}

PPC_FUNC_IMPL(__imp__sub_82FE41C0);
PPC_FUNC(sub_82FE41C0)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    RecordCtGameplayWriterProbe(
        "boostGauge",
        "sub_82FE41C0",
        "entry",
        ownerAddress,
        ctx,
        "ct-anchored-gameplay-writer-probe:day-boost-aob");
    __imp__sub_82FE41C0(ctx, base);
}

PPC_FUNC_IMPL(__imp__sub_82BDBA20);
PPC_FUNC(sub_82BDBA20)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    RecordCtGameplayWriterProbe(
        "lifeCount",
        "sub_82BDBA20",
        "entry",
        ownerAddress,
        ctx,
        "ct-anchored-gameplay-writer-probe:lives-primary");

    const uint32_t storageAddress = IsPlausibleGuestAddress(ownerAddress)
        ? ownerAddress + 11868
        : 0;
    const uint32_t previousValue = ReadGuestU32ForCtWriter(base, storageAddress);

    __imp__sub_82BDBA20(ctx, base);

    if (!IsPlausibleGuestAddress(storageAddress))
        return;

    const uint32_t value = ReadGuestU32ForCtWriter(base, storageAddress);
    UiLab::OnSonicHudCtGameplayWriter(
        "lifeCount",
        "sub_82BDBA20",
        ownerAddress,
        storageAddress,
        previousValue,
        value,
        SaturatingSignedDelta(previousValue, value),
        0.0f,
        false,
        "ct-anchored-gameplay-writer:lives-primary");
}

PPC_FUNC_IMPL(__imp__sub_82BDBA60);
PPC_FUNC(sub_82BDBA60)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    RecordCtGameplayWriterProbe(
        "lifeCount",
        "sub_82BDBA60",
        "entry",
        ownerAddress,
        ctx,
        "ct-anchored-gameplay-writer-probe:lives-secondary");

    const uint32_t storageAddress = IsPlausibleGuestAddress(ownerAddress)
        ? ownerAddress + 11872
        : 0;
    const uint32_t previousValue = ReadGuestU32ForCtWriter(base, storageAddress);

    __imp__sub_82BDBA60(ctx, base);

    if (!IsPlausibleGuestAddress(storageAddress))
        return;

    const uint32_t value = ReadGuestU32ForCtWriter(base, storageAddress);
    UiLab::OnSonicHudCtGameplayWriter(
        "lifeCount",
        "sub_82BDBA60",
        ownerAddress,
        storageAddress,
        previousValue,
        value,
        SaturatingSignedDelta(previousValue, value),
        0.0f,
        false,
        "ct-anchored-gameplay-writer:lives-secondary");
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
    // samplePhase=pre-original: capture input delta/registers before the PPC body mutates owner fields.
    RecordHudSonicStageCallsiteSample(ownerAddress, "sub_824D6048", "pre-original", ctx);
    UiLab::PushSonicHudUpdateContext(ownerAddress, "sonic-hud-update-context CHudSonicStage/sub_824D6048");
    __imp__sub_824D6048(ctx, base);
    UiLab::PopSonicHudUpdateContext("sonic-hud-update-context CHudSonicStage/sub_824D6048");
    // samplePhase=post-original: capture owner +452/+456 display counters after text writes.
    RecordHudSonicStageCallsiteSample(ownerAddress, "sub_824D6048", "post-original", ctx);
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage value update hook sub_824D6048");
}

PPC_FUNC_IMPL(__imp__sub_824D6418);
PPC_FUNC(sub_824D6418)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    RecordHudSonicStageCallsiteSample(ownerAddress, "sub_824D6418", "pre-original", ctx);
    UiLab::PushSonicHudUpdateContext(ownerAddress, "sonic-hud-update-context CHudSonicStage/sub_824D6418");
    const uint32_t previousSpeedReadoutOwnerAddress = g_sonicHudSpeedReadoutOwnerAddress;
    g_sonicHudSpeedReadoutOwnerAddress = ownerAddress;
    ++g_sonicHudSpeedReadoutCaptureDepth;
    __imp__sub_824D6418(ctx, base);
    --g_sonicHudSpeedReadoutCaptureDepth;
    g_sonicHudSpeedReadoutOwnerAddress = previousSpeedReadoutOwnerAddress;
    UiLab::PopSonicHudUpdateContext("sonic-hud-update-context CHudSonicStage/sub_824D6418");
    RecordHudSonicStageCallsiteSample(ownerAddress, "sub_824D6418", "post-original", ctx);
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage value update hook sub_824D6418");
}

PPC_FUNC_IMPL(__imp__sub_8251A568);
PPC_FUNC(sub_8251A568)
{
    __imp__sub_8251A568(ctx, base);

    if (
        g_sonicHudSpeedReadoutCaptureDepth != 0 &&
        IsPlausibleGuestAddress(g_sonicHudSpeedReadoutOwnerAddress))
    {
        UiLab::OnSonicHudSpeedReadoutValue(
            g_sonicHudSpeedReadoutOwnerAddress,
            ctx.r3.u32,
            "generated-PPC:sub_824D6418 -> sub_8251A568 return");
    }
}

PPC_FUNC_IMPL(__imp__sub_824D69B0);
PPC_FUNC(sub_824D69B0)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    RecordHudSonicStageCallsiteSample(ownerAddress, "sub_824D69B0", "pre-original", ctx);
    UiLab::PushSonicHudUpdateContext(ownerAddress, "sonic-hud-update-context CHudSonicStage/sub_824D69B0");
    __imp__sub_824D69B0(ctx, base);
    UiLab::PopSonicHudUpdateContext("sonic-hud-update-context CHudSonicStage/sub_824D69B0");
    RecordHudSonicStageCallsiteSample(ownerAddress, "sub_824D69B0", "post-original", ctx);
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage value update hook sub_824D69B0");
}

PPC_FUNC_IMPL(__imp__sub_824D6C18);
PPC_FUNC(sub_824D6C18)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    RecordHudSonicStageCallsiteSample(ownerAddress, "sub_824D6C18", "pre-original", ctx);
    UiLab::PushSonicHudUpdateContext(ownerAddress, "sonic-hud-update-context CHudSonicStage/sub_824D6C18");
    __imp__sub_824D6C18(ctx, base);
    UiLab::PopSonicHudUpdateContext("sonic-hud-update-context CHudSonicStage/sub_824D6C18");
    // samplePhase=post-original: owner +460/+480 carry staged gauge/counter state in this callsite.
    RecordHudSonicStageCallsiteSample(ownerAddress, "sub_824D6C18", "post-original", ctx);
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage value update hook sub_824D6C18");
    // Phase 197: snapshot owner+460/+464/+468/+472/+480 (the staging block read
    // by this callsite) so the rolling counter / gauge-state lane has its own
    // evidence channel, parallel to the same-frame text-write candidates.
    UiLab::OnHudSonicStageOwnerFieldGaugeSnapshot(ownerAddress, "sub_824D6C18");
}

PPC_FUNC_IMPL(__imp__sub_824D7100);
PPC_FUNC(sub_824D7100)
{
    const uint32_t ownerAddress = ctx.r3.u32;
    RecordHudSonicStageCallsiteSample(ownerAddress, "sub_824D7100", "pre-original", ctx);
    UiLab::PushSonicHudUpdateContext(ownerAddress, "sonic-hud-update-context CHudSonicStage/sub_824D7100");
    __imp__sub_824D7100(ctx, base);
    UiLab::PopSonicHudUpdateContext("sonic-hud-update-context CHudSonicStage/sub_824D7100");
    RecordHudSonicStageCallsiteSample(ownerAddress, "sub_824D7100", "post-original", ctx);
    RecordHudSonicStageInspector(ownerAddress, "raw CHudSonicStage value update hook sub_824D7100");
}
