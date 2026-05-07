#include <kernel/function.h>
#include <kernel/memory.h>
#include <patches/sg_text_overrides.h>
#include <patches/ui_lab_patches.h>

#include <cctype>
#include <cstdlib>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>

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

// Phase 367b: SG_PREFLIGHT_LOG_SETTEXT=1 turns on a bounded probe that
// emits one `Text:CsdSetTextSample:<literal>` bridge event per unique
// ASCII literal SetText'd by retail SU. Bounded to the first
// kSetTextSampleLimit unique literals per process boot so the bridge
// events.jsonl never grows unbounded. Used only to discover which
// literals retail SU passes through `sub_830BF640::SetText` so the
// proof's override pack can target keys that actually fire (the Title
// menu rows are pre-baked into ui_title.yncp and never go through
// SetText, so this probe is the canonical way to find live keys).
namespace
{
constexpr std::size_t kSetTextSampleLimit = 64;
std::unordered_set<std::string> g_setTextSampleSet;
std::mutex g_setTextSampleMutex;

bool IsSetTextSampleEnabled()
{
    static const bool enabled = []
    {
        const char* env = std::getenv("SG_PREFLIGHT_LOG_SETTEXT");
        return env != nullptr && std::string_view(env) == "1";
    }();
    return enabled;
}

void EmitSetTextSampleIfFirst(std::string_view literal)
{
    if (!IsSetTextSampleEnabled() || literal.empty()) return;

    std::scoped_lock lock(g_setTextSampleMutex);
    if (g_setTextSampleSet.size() >= kSetTextSampleLimit) return;
    if (!g_setTextSampleSet.emplace(literal).second) return;

    UiLab::EmitBridgeScreenEntered(
        "Text:CsdSetTextSample:" + std::string(literal));
}
} // namespace

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

    // Phase 367b: probe (env-gated) emits one bridge sample per unique
    // SetText literal so the operator can populate the override pack
    // with literals that actually fire at runtime.
    EmitSetTextSampleIfFirst(textUtf8);

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
        std::string overrideText;
        UiLab::OnCsdNodeSetText(
            nodeAddress,
            effectiveTextAddress,
            SGTextOverrides::TryGetOverride(textUtf8, &overrideText) ? overrideText : textUtf8,
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
