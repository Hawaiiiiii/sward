#include <kernel/function.h>
#include <kernel/memory.h>
#include <patches/sg_text_overrides.h>
#include <patches/ui_lab_patches.h>

#include <cctype>
#include <string>

namespace
{
static constexpr uint32_t kGuestTextProbeMaxBytes = 96;

bool IsPlausibleGuestAddress(uint32_t address)
{
    return address >= 0x10000 && address < (PPC_MEMORY_SIZE - kGuestTextProbeMaxBytes);
}

std::string TryReadGuestAsciiString(uint32_t textAddress)
{
    if (g_memory.base == nullptr || !IsPlausibleGuestAddress(textAddress))
        return {};

    const auto* chars = reinterpret_cast<const char*>(g_memory.Translate(textAddress));
    std::string text;
    text.reserve(16);

    for (uint32_t index = 0; index < kGuestTextProbeMaxBytes; ++index)
    {
        const unsigned char c = static_cast<unsigned char>(chars[index]);
        if (c == 0)
            break;

        if (c != '\t' && c != '\n' && c != '\r' && !std::isprint(c))
            return {};

        text.push_back(static_cast<char>(c));
    }

    return text;
}

std::string TryReadGuestUtf16String(uint32_t textAddress)
{
    if (g_memory.base == nullptr || !IsPlausibleGuestAddress(textAddress))
        return {};

    const auto* bytes = reinterpret_cast<const uint8_t*>(g_memory.Translate(textAddress));
    std::string text;
    text.reserve(16);

    for (uint32_t index = 0; index + 1 < kGuestTextProbeMaxBytes; index += 2)
    {
        const uint8_t high = bytes[index];
        const uint8_t low = bytes[index + 1];
        if (high == 0 && low == 0)
            break;

        unsigned char c = 0;
        if (high == 0)
            c = low;
        else if (low == 0)
            c = high;
        else
            return {};

        if (c != '\t' && c != '\n' && c != '\r' && !std::isprint(c))
            return {};

        text.push_back(static_cast<char>(c));
    }

    return text;
}
} // namespace

PPC_FUNC_IMPL(__imp__sub_830BF640);
PPC_FUNC(sub_830BF640)
{
    const uint32_t nodeAddress = ctx.r3.u32;
    const uint32_t originalTextAddress = ctx.r4.u32;
    std::string textUtf8 = TryReadGuestAsciiString(originalTextAddress);
    if (textUtf8.empty())
        textUtf8 = TryReadGuestUtf16String(originalTextAddress);

    // Phase 364: per-string text override. If the override map has an
    // entry for the original literal, point r4 at a guest-heap-resident
    // UTF-8 copy of the override before SetText runs. Only swap when the
    // original parsed as ASCII -- the existing CSD glyph layout for
    // Sonic Unleashed treats single-byte UTF-8 in this slot the same as
    // ASCII, which matches the retail string layout for menu labels.
    uint32_t effectiveTextAddress = originalTextAddress;
    bool overrideApplied = false;
    if (!textUtf8.empty())
    {
        const uint32_t overrideGuestPtr = SGTextOverrides::TryGetOverrideGuestPtr(textUtf8);
        if (overrideGuestPtr != 0)
        {
            ctx.r4.u32 = overrideGuestPtr;
            effectiveTextAddress = overrideGuestPtr;
            overrideApplied = true;
        }
    }

    __imp__sub_830BF640(ctx, base);

    if (overrideApplied)
    {
        const std::string* overrideText = SGTextOverrides::TryGetOverride(textUtf8);
        UiLab::OnCsdNodeSetText(
            nodeAddress,
            effectiveTextAddress,
            overrideText != nullptr ? *overrideText : textUtf8,
            "CSD::CNode::SetText/sub_830BF640+override");
    }
    else
    {
        UiLab::OnCsdNodeSetText(
            nodeAddress,
            originalTextAddress,
            textUtf8,
            "CSD::CNode::SetText/sub_830BF640");
    }
}
