#pragma once

// SGFX HUD layout: human-readable port of `class CSaveIcon`.
//
// Phase 268: generated directly from the UnleashedRecomp SWA API header
// `local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/SaveIcon/SaveIcon.h` — that header already names every
// member of this class with the authoritative SWA template-argument
// types and pins each member offset via `SWA_ASSERT_OFFSETOF`. The
// generator copies those offsets verbatim and pads between members so
// `static_assert(offsetof(...))` continues to validate the layout at
// compile time. Method bodies are intentionally out of scope; they
// will be ported in subsequent phases as the recomp flow is decoded.
//
// Generated at: 2026-05-05T01:41:56+00:00
// SWA base class: Hedgehog::Universe::CUpdateUnit (modeled here as leading byte padding rather than a real C++ base class to keep the layout self-contained).

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Forward declarations of the SWA CSD types referenced by this HUD
    // class. The retail SWA executable holds the corresponding
    // `Chao::CSD::*` types; the human-readable port keeps them opaque at
    // this layer because the CSD runtime is ported separately.
    class CScene;

    // SWA `RCPtr<T>` matches an Xbox 360 32-bit pointer pair:
    // `m_pRCObject` (the reference-counted wrapper) at offset 0 and
    // `m_pMemory` (the wrapped object) at offset 4. Total size: 8 bytes.
    template <class T>
    struct RCPtr
    {
        std::uint32_t m_pRCObject;  // guest-relative pointer to the RCObject wrapper
        std::uint32_t m_pMemory;    // guest-relative pointer to the wrapped T
    };
    static_assert(sizeof(RCPtr<CScene>) == 8, "RCPtr<T> must match the SWA 8-byte layout");

    // The SWA executable references the wrapper as `Chao::CSD::RCPtr<T>`
    // throughout the existing API headers; the `Chao::CSD::` alias here
    // matches that convention so the human-readable port's member
    // declarations read identically to the SWA originals.
    namespace Chao { namespace CSD
    {
        template <class T> using RCPtr = ::sward::ui_runtime::generated::sgfx_hud::RCPtr<T>;
        using CScene = ::sward::ui_runtime::generated::sgfx_hud::CScene;
    }} // namespace Chao::CSD

    class CSaveIcon
    {
    public:
    private: std::array<std::uint8_t, 0xD8> m_padding0000_00D8;  // pre-m_IsVisible padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        bool m_IsVisible;  // +0xD8 bool (type from SWA API header)

        // Phase 270: inline accessors for scalar / enum members.
        // Mechanical translations of the SWA `be<T>` storage layout to
        // the host-side semantic value. Derived purely from the SWA
        // API header; no recomp method bodies are referenced.
        bool isVisible() const noexcept { return m_IsVisible; }
    };

    // Compile-time guards: every named member must land at the SWA-asserted
    // offset. Drift against the live recomp executable's class layout breaks
    // the build and forces a re-generation.
    static_assert(offsetof(CSaveIcon, m_IsVisible) == 0xD8, "CSaveIcon::m_IsVisible must remain at +0xD8");

    static constexpr std::string_view kGeneratedAt = "2026-05-05T01:41:56+00:00";
    static constexpr std::string_view kSwaApiHeaderRelpath = "local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/SaveIcon/SaveIcon.h";

} // namespace sward::ui_runtime::generated::sgfx_hud
