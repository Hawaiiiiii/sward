#pragma once

// SGFX HUD layout: human-readable port of `class CGeneralWindow`.
//
// Phase 268: generated directly from the UnleashedRecomp SWA API header
// `local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/GeneralWindow/GeneralWindow.h` — that header already names every
// member of this class with the authoritative SWA template-argument
// types and pins each member offset via `SWA_ASSERT_OFFSETOF`. The
// generator copies those offsets verbatim and pads between members so
// `static_assert(offsetof(...))` continues to validate the layout at
// compile time. Method bodies are intentionally out of scope; they
// will be ported in subsequent phases as the recomp flow is decoded.
//
// Generated at: 2026-05-05T06:56:26+00:00

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
    class CProject;
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
        using CProject = ::sward::ui_runtime::generated::sgfx_hud::CProject;
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

    enum class EWindowStatus : std::uint32_t
    {
        eWindowStatus_Closed,
        eWindowStatus_OpeningMessage = 2,
        eWindowStatus_DisplayingMessage,
        eWindowStatus_OpeningControls,
        eWindowStatus_DisplayingControls
    };

    class CGeneralWindow
    {
    public:
        std::array<std::uint8_t, 0xD0> m_padding0000_00D0;  // pre-m_rcGeneral padding (covers SWA base class / SWA_INSERT_PADDING bytes)
        Chao::CSD::RCPtr<Chao::CSD::CProject> m_rcGeneral;  // +0xD0 Chao::CSD::RCPtr<Chao::CSD::CProject> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcBg;  // +0xD8 Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcWindow;  // +0xE0 Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcWindow_2;  // +0xE8 Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcWindowSelect;  // +0xF0 Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcFooter;  // +0xF8 Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        std::array<std::uint8_t, 0x58> m_padding0100_0158;  // pre-m_Status padding (covers SWA base class / SWA_INSERT_PADDING bytes)
        be<EWindowStatus> m_Status;  // +0x158 be<EWindowStatus> (type from SWA API header)
        be<uint32_t> m_CursorIndex;  // +0x15C be<uint32_t> (type from SWA API header)
        std::array<std::uint8_t, 0x4> m_padding0160_0164;  // pre-m_SelectedIndex padding (covers SWA base class / SWA_INSERT_PADDING bytes)
        be<uint32_t> m_SelectedIndex;  // +0x164 be<uint32_t> (type from SWA API header)

        // Phase 270: inline accessors for scalar / enum members.
        // Mechanical translations of the SWA `be<T>` storage layout to
        // the host-side semantic value. Derived purely from the SWA
        // API header; no recomp method bodies are referenced.
        EWindowStatus getStatus() const noexcept { return static_cast<EWindowStatus>(m_Status.m_storage); }
        uint32_t getCursorIndex() const noexcept { return m_CursorIndex.m_storage; }
        uint32_t getSelectedIndex() const noexcept { return m_SelectedIndex.m_storage; }
    };

    // Compile-time guards: every named member must land at the SWA-asserted
    // offset. Drift against the live recomp executable's class layout breaks
    // the build and forces a re-generation.
    static_assert(offsetof(CGeneralWindow, m_rcGeneral) == 0xD0, "CGeneralWindow::m_rcGeneral must remain at +0xD0");
    static_assert(offsetof(CGeneralWindow, m_rcBg) == 0xD8, "CGeneralWindow::m_rcBg must remain at +0xD8");
    static_assert(offsetof(CGeneralWindow, m_rcWindow) == 0xE0, "CGeneralWindow::m_rcWindow must remain at +0xE0");
    static_assert(offsetof(CGeneralWindow, m_rcWindow_2) == 0xE8, "CGeneralWindow::m_rcWindow_2 must remain at +0xE8");
    static_assert(offsetof(CGeneralWindow, m_rcWindowSelect) == 0xF0, "CGeneralWindow::m_rcWindowSelect must remain at +0xF0");
    static_assert(offsetof(CGeneralWindow, m_rcFooter) == 0xF8, "CGeneralWindow::m_rcFooter must remain at +0xF8");
    static_assert(offsetof(CGeneralWindow, m_Status) == 0x158, "CGeneralWindow::m_Status must remain at +0x158");
    static_assert(offsetof(CGeneralWindow, m_CursorIndex) == 0x15C, "CGeneralWindow::m_CursorIndex must remain at +0x15C");
    static_assert(offsetof(CGeneralWindow, m_SelectedIndex) == 0x164, "CGeneralWindow::m_SelectedIndex must remain at +0x164");

    static constexpr std::string_view kGeneratedAt = "2026-05-05T06:56:26+00:00";
    static constexpr std::string_view kSwaApiHeaderRelpath = "local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/GeneralWindow/GeneralWindow.h";

} // namespace sward::ui_runtime::generated::sgfx_hud
