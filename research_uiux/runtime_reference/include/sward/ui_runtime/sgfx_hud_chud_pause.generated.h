#pragma once

// SGFX HUD layout: human-readable port of `class CHudPause`.
//
// Phase 268: generated directly from the UnleashedRecomp SWA API header
// `local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/Pause/HudPause.h` — that header already names every
// member of this class with the authoritative SWA template-argument
// types and pins each member offset via `SWA_ASSERT_OFFSETOF`. The
// generator copies those offsets verbatim and pads between members so
// `static_assert(offsetof(...))` continues to validate the layout at
// compile time. Method bodies are intentionally out of scope; they
// will be ported in subsequent phases as the recomp flow is decoded.
//
// Generated at: 2026-05-05T00:53:18+00:00
// SWA base class: CGameObject (modeled here as leading byte padding rather than a real C++ base class to keep the layout self-contained).

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

    enum class EActionType : std::uint32_t
    {
        eActionType_Undefined,
        eActionType_Status,
        eActionType_Return,
        eActionType_Inventory,
        eActionType_Skills,
        eActionType_Lab,
        eActionType_Wait,
        eActionType_Restart = 8,
        eActionType_Continue
    };

    enum class EMenuType : std::uint32_t
    {
        eMenuType_WorldMap,
        eMenuType_Village,
        eMenuType_Stage,
        eMenuType_Hub,
        eMenuType_Misc
    };

    enum class EStatusType : std::uint32_t
    {
        eStatusType_Idle,
        eStatusType_Accept,
        eStatusType_Decline
    };

    enum class ETransitionType : std::uint32_t
    {
        eTransitionType_Undefined,
        eTransitionType_Quit = 2,
        eTransitionType_Dialog = 5,
        eTransitionType_Hide,
        eTransitionType_Abort,
        eTransitionType_SubMenu
    };

    class CHudPause
    {
    public:
    private: std::array<std::uint8_t, 0xEC> m_padding0000_00EC;  // pre-m_rcPause padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        Chao::CSD::RCPtr<Chao::CSD::CProject> m_rcPause;  // +0xEC Chao::CSD::RCPtr<Chao::CSD::CProject> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcBg;  // +0xF4 Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcBg1;  // +0xFC Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcBg1_2;  // +0x104 Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcBg1Select;  // +0x10C Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcBg1Select_2;  // +0x114 Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcStatusTitle;  // +0x11C Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
        Chao::CSD::RCPtr<Chao::CSD::CScene> m_rcFooterA;  // +0x124 Chao::CSD::RCPtr<Chao::CSD::CScene> (type from SWA API header)
    private: std::array<std::uint8_t, 0x59> m_padding012C_0185;  // pre-m_IsVisible padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        bool m_IsVisible;  // +0x185 bool (type from SWA API header)
    private: std::array<std::uint8_t, 0x2> m_padding0186_0188;  // pre-m_Action padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        be<EActionType> m_Action;  // +0x188 be<EActionType> (type from SWA API header)
        be<EMenuType> m_Menu;  // +0x18C be<EMenuType> (type from SWA API header)
        be<EStatusType> m_Status;  // +0x190 be<EStatusType> (type from SWA API header)
        be<ETransitionType> m_Transition;  // +0x194 be<ETransitionType> (type from SWA API header)
    private: std::array<std::uint8_t, 0x4> m_padding0198_019C;  // pre-m_Submenu padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        be<uint32_t> m_Submenu;  // +0x19C be<uint32_t> (type from SWA API header)
    private: std::array<std::uint8_t, 0x18> m_padding01A0_01B8;  // pre-m_IsShown padding (covers SWA base class / SWA_INSERT_PADDING bytes)
    public:
        bool m_IsShown;  // +0x1B8 bool (type from SWA API header)
    };

    // Compile-time guards: every named member must land at the SWA-asserted
    // offset. Drift against the live recomp executable's class layout breaks
    // the build and forces a re-generation.
    static_assert(offsetof(CHudPause, m_rcPause) == 0xEC, "CHudPause::m_rcPause must remain at +0xEC");
    static_assert(offsetof(CHudPause, m_rcBg) == 0xF4, "CHudPause::m_rcBg must remain at +0xF4");
    static_assert(offsetof(CHudPause, m_rcBg1) == 0xFC, "CHudPause::m_rcBg1 must remain at +0xFC");
    static_assert(offsetof(CHudPause, m_rcBg1_2) == 0x104, "CHudPause::m_rcBg1_2 must remain at +0x104");
    static_assert(offsetof(CHudPause, m_rcBg1Select) == 0x10C, "CHudPause::m_rcBg1Select must remain at +0x10C");
    static_assert(offsetof(CHudPause, m_rcBg1Select_2) == 0x114, "CHudPause::m_rcBg1Select_2 must remain at +0x114");
    static_assert(offsetof(CHudPause, m_rcStatusTitle) == 0x11C, "CHudPause::m_rcStatusTitle must remain at +0x11C");
    static_assert(offsetof(CHudPause, m_rcFooterA) == 0x124, "CHudPause::m_rcFooterA must remain at +0x124");
    static_assert(offsetof(CHudPause, m_IsVisible) == 0x185, "CHudPause::m_IsVisible must remain at +0x185");
    static_assert(offsetof(CHudPause, m_Action) == 0x188, "CHudPause::m_Action must remain at +0x188");
    static_assert(offsetof(CHudPause, m_Menu) == 0x18C, "CHudPause::m_Menu must remain at +0x18C");
    static_assert(offsetof(CHudPause, m_Status) == 0x190, "CHudPause::m_Status must remain at +0x190");
    static_assert(offsetof(CHudPause, m_Transition) == 0x194, "CHudPause::m_Transition must remain at +0x194");
    static_assert(offsetof(CHudPause, m_Submenu) == 0x19C, "CHudPause::m_Submenu must remain at +0x19C");
    static_assert(offsetof(CHudPause, m_IsShown) == 0x1B8, "CHudPause::m_IsShown must remain at +0x1B8");

    static constexpr std::string_view kGeneratedAt = "2026-05-05T00:53:18+00:00";
    static constexpr std::string_view kSwaApiHeaderRelpath = "local_build_env/ur103clean/UnleashedRecomp/api/SWA/HUD/Pause/HudPause.h";

} // namespace sward::ui_runtime::generated::sgfx_hud
