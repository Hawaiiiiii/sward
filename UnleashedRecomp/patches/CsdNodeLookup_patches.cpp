#include <kernel/function.h>
#include <kernel/memory.h>
#include <patches/ui_lab_patches.h>

#include <cctype>
#include <string>

namespace
{
static constexpr uint32_t kGuestLookupNameMaxBytes = 96;

bool IsPlausibleGuestLookupAddress(uint32_t address)
{
    return address >= 0x10000 && address < (PPC_MEMORY_SIZE - kGuestLookupNameMaxBytes);
}

std::string TryReadGuestLookupName(uint32_t textAddress)
{
    if (g_memory.base == nullptr || !IsPlausibleGuestLookupAddress(textAddress))
        return {};

    const auto* chars = reinterpret_cast<const char*>(g_memory.Translate(textAddress));
    std::string text;
    text.reserve(24);

    for (uint32_t index = 0; index < kGuestLookupNameMaxBytes; ++index)
    {
        const unsigned char c = static_cast<unsigned char>(chars[index]);
        if (c == 0)
            break;

        if (c != '_' && c != '-' && c != '/' && c != '.' && !std::isalnum(c))
            return {};

        text.push_back(static_cast<char>(c));
    }

    return text;
}
} // namespace

PPC_FUNC_IMPL(__imp__sub_830BCCA8);
PPC_FUNC(sub_830BCCA8)
{
    const uint32_t resultOwnerAddress = ctx.r3.u32;
    const uint32_t parentNodeAddress = ctx.r4.u32;
    const std::string childName = TryReadGuestLookupName(ctx.r5.u32);

    __imp__sub_830BCCA8(ctx, base);

    UiLab::OnCsdChildNodeLookupResolved(
        resultOwnerAddress,
        parentNodeAddress,
        childName,
        "CSD::CNode::GetChild/sub_830BCCA8");
}

PPC_FUNC_IMPL(__imp__sub_830BA228);
PPC_FUNC(sub_830BA228)
{
    const uint32_t sourceOwnerAddress = ctx.r3.u32;

    __imp__sub_830BA228(ctx, base);

    UiLab::OnCsdNodePointerResolved(
        sourceOwnerAddress,
        ctx.r3.u32,
        "CSD::RCPtr::Get/sub_830BA228");
}
