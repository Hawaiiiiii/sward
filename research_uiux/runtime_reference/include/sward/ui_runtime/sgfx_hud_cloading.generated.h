#pragma once

// SGFX HUD layout: human-readable port of `class CLoading`.
//
// Phase 268: generated directly from the UnleashedRecomp SWA API header
// `local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/Loading/Loading.h` — that header already names every
// member of this class with the authoritative SWA template-argument
// types and pins each member offset via `SWA_ASSERT_OFFSETOF`. The
// generator copies those offsets verbatim and pads between members so
// `static_assert(offsetof(...))` continues to validate the layout at
// compile time. Method bodies are intentionally out of scope; they
// will be ported in subsequent phases as the recomp flow is decoded.
//
// Generated at: 2026-05-05T01:12:27+00:00

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

    // SWA `be<T>` is a thin big-endian wrapper around T; for layout
    // purposes it is equivalent to T itself (same size and alignment).
    // Endian decoding is the responsibility of a separate runtime
    // layer ported alongside the rest of the SWA executable.
    template <class T>
    struct be
    {
        T m_storage;
    };

    enum class ELoadingDisplayType : std::int32_t
    {
        eLoadingDisplayType_MilesElectric,
        eLoadingDisplayType_None,
        eLoadingDisplayType_WerehogMovie,
        eLoadingDisplayType_MilesElectricContext,
        eLoadingDisplayType_Arrows,
        eLoadingDisplayType_NowLoading,
        eLoadingDisplayType_EventGallery,
        eLoadingDisplayType_ChangeTimeOfDay,
        eLoadingDisplayType_Blank
    };

    class CLoading
    {
    public:
    private: std::array<std::uint8_t, 0xD8> m_padding0000_00D8;  // pre-m_FieldD8 padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        be<uint32_t> m_FieldD8;  // +0xD8 be<uint32_t> (type from SWA API header)
    private: std::array<std::uint8_t, 0x3C> m_padding00DC_0118;  // pre-m_rcNightToDay padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcNightToDay;  // +0x118 Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
    private: std::array<std::uint8_t, 0xC> m_padding0120_012C;  // pre-m_IsVisible padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        be<uint32_t> m_IsVisible;  // +0x12C be<uint32_t> (type from SWA API header)
    private: std::array<std::uint8_t, 0xC> m_padding0130_013C;  // pre-m_LoadingDisplayType padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        be<ELoadingDisplayType> m_LoadingDisplayType;  // +0x13C be<ELoadingDisplayType> (type from SWA API header)
    private: std::array<std::uint8_t, 0x61> m_padding0140_01A1;  // pre-m_IsNightToDay padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        bool m_IsNightToDay;  // +0x1A1 bool (type from SWA API header)

        // Phase 270: inline accessors for scalar / enum members.
        // Mechanical translations of the SWA `be<T>` storage layout to
        // the host-side semantic value. Derived purely from the SWA
        // API header; no recomp method bodies are referenced.
        uint32_t getFieldD8() const noexcept { return m_FieldD8.m_storage; }
        uint32_t getIsVisible() const noexcept { return m_IsVisible.m_storage; }
        ELoadingDisplayType getLoadingDisplayType() const noexcept { return static_cast<ELoadingDisplayType>(m_LoadingDisplayType.m_storage); }
        bool isNightToDay() const noexcept { return m_IsNightToDay; }
    };

    // Compile-time guards: every named member must land at the SWA-asserted
    // offset. Drift against the live recomp executable's class layout breaks
    // the build and forces a re-generation.
    static_assert(offsetof(CLoading, m_FieldD8) == 0xD8, "CLoading::m_FieldD8 must remain at +0xD8");
    static_assert(offsetof(CLoading, m_rcNightToDay) == 0x118, "CLoading::m_rcNightToDay must remain at +0x118");
    static_assert(offsetof(CLoading, m_IsVisible) == 0x12C, "CLoading::m_IsVisible must remain at +0x12C");
    static_assert(offsetof(CLoading, m_LoadingDisplayType) == 0x13C, "CLoading::m_LoadingDisplayType must remain at +0x13C");
    static_assert(offsetof(CLoading, m_IsNightToDay) == 0x1A1, "CLoading::m_IsNightToDay must remain at +0x1A1");

    static constexpr std::string_view kGeneratedAt = "2026-05-05T01:12:27+00:00";
    static constexpr std::string_view kSwaApiHeaderRelpath = "local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/Loading/Loading.h";

} // namespace sward::ui_runtime::generated::sgfx_hud
