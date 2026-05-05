#pragma once

// SGFX HUD layout: human-readable port of `class CHudSonicStage`.
//
// Phase 266: generated from `kChudSonicStageExpectedOwnerFields` (which is
// itself sourced from the CHudSonicStage constructor decode in
// `local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.28.cpp:61909`
// plus the runtime Phase 260 sweep evidence). Every RCPtr offset here is
// runtime-confirmed; the 11 still-unnamed RCPtrs use offset-based names
// (`m_rcPtrFieldXxx`) so their existence is recorded without inventing
// semantic claims. Method bodies are intentionally out of scope for this
// header — it is a layout reference for the human-readable 1:1 port; the
// constructor / destructor / Update / Render method bodies will be ported
// in subsequent phases as their recomp flow is decoded.
//
// Generated at: 2026-05-05T00:13:57+00:00
// Source attribution: api/SWA/HUD/Sonic/HudSonicStage.h offsets 0xE0..0x184; Phase 265 expanded from CHudSonicStage::CHudSonicStage (sub_824D89B0) decoded from local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.28.cpp:61909

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Forward declaration of the SWA CSD scene type. The retail SWA
    // executable holds a `CSD::Manager::CScene*` here; the human-readable
    // port keeps the type opaque at this layer because the CSD runtime is
    // ported separately.
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

    // Layout reference for `class CHudSonicStage` (the Sonic stage HUD).
    // Constructor entry point: `sub_824D89B0` (named in Phase 265).
    // Destructor entry point: `sub_824D8CE8`.
    // The vtable pointer lives at `+0x00` and a secondary vtable / typeinfo
    // pointer at `+0x28`, both populated by the constructor.
    class CHudSonicStage
    {
    public:
        std::uint32_t m_pVTable;             // +0x00 vtable pointer set by sub_824D89B0
    private:
        std::array<std::uint8_t, 0x24> m_padding00_28;  // pre-secondary-vtable bytes
    public:
        std::uint32_t m_pSecondaryVTable;    // +0x28 typeinfo / aux vtable set by sub_824D89B0
    private:
        std::array<std::uint8_t, 0xB4> m_padding2C_E0;  // pre-RCPtr-table bytes
    public:
        RCPtr<CScene> m_rcPlayScreen;  // +0xE0 RCPtr<CScene>
        RCPtr<CScene> m_rcSpeedGauge;  // +0xE8 RCPtr<CScene>; runtime: ui_playscreen/so_speed_gauge (cross-validated, instances=1)
        RCPtr<CScene> m_rcRingEnergyGauge;  // +0xF0 RCPtr<CScene>; runtime: ui_playscreen/so_ringenagy_gauge (cross-validated, instances=1)
        RCPtr<CScene> m_rcGaugeFrame;  // +0xF8 RCPtr<CScene>; runtime: ui_playscreen/gauge_frame (cross-validated, instances=1)
        RCPtr<CScene> m_rcExpCount;  // +0x100 RCPtr<CScene>
        RCPtr<CScene> m_rcPtrField108;  // +0x108 RCPtr<CScene>; constructor-confirmed but no runtime scene yet — name pending evidence
        RCPtr<CScene> m_rcPtrField110;  // +0x110 RCPtr<CScene>; constructor-confirmed but no runtime scene yet — name pending evidence
        RCPtr<CScene> m_rcSpeedCount;  // +0x118 RCPtr<CScene>; runtime: ui_playscreen/add/speed_count (cross-validated, instances=1)
        RCPtr<CScene> m_rcPtrField120;  // +0x120 RCPtr<CScene>; constructor-confirmed but no runtime scene yet — name pending evidence
        RCPtr<CScene> m_rcScoreCount;  // +0x128 RCPtr<CScene>
        RCPtr<CScene> m_rcTimeCount;  // +0x130 RCPtr<CScene>
        RCPtr<CScene> m_rcTimeCount2;  // +0x138 RCPtr<CScene>
        RCPtr<CScene> m_rcTimeCount3;  // +0x140 RCPtr<CScene>
        RCPtr<CScene> m_rcPlayerCount;  // +0x148 RCPtr<CScene>
        RCPtr<CScene> m_rcPtrField150;  // +0x150 RCPtr<CScene>; constructor-confirmed but no runtime scene yet — name pending evidence
        RCPtr<CScene> m_rcPtrField158;  // +0x158 RCPtr<CScene>; constructor-confirmed but no runtime scene yet — name pending evidence
        RCPtr<CScene> m_rcPtrField160;  // +0x160 RCPtr<CScene>; constructor-confirmed but no runtime scene yet — name pending evidence
        RCPtr<CScene> m_rcPtrField168;  // +0x168 RCPtr<CScene>; constructor-confirmed but no runtime scene yet — name pending evidence
        RCPtr<CScene> m_rcPtrField170;  // +0x170 RCPtr<CScene>; constructor-confirmed but no runtime scene yet — name pending evidence
        RCPtr<CScene> m_rcPtrField178;  // +0x178 RCPtr<CScene>; constructor-confirmed but no runtime scene yet — name pending evidence
        RCPtr<CScene> m_rcPtrField180;  // +0x180 RCPtr<CScene>; constructor-confirmed but no runtime scene yet — name pending evidence
    };

    // Compile-time guards: every named RCPtr must land at the runtime-
    // confirmed offset. If the layout drifts (recomp regenerated, expected-
    // fields table updated, etc.) the header fails to compile and forces a
    // re-generation.
    static_assert(offsetof(CHudSonicStage, m_pVTable) == 0x00,
        "CHudSonicStage vtable pointer must remain at +0x00");
    static_assert(offsetof(CHudSonicStage, m_pSecondaryVTable) == 0x28,
        "CHudSonicStage secondary vtable / typeinfo must remain at +0x28");
    static_assert(offsetof(CHudSonicStage, m_rcPlayScreen) == 0xE0, "CHudSonicStage::m_rcPlayScreen must remain at +0xE0");
    static_assert(offsetof(CHudSonicStage, m_rcSpeedGauge) == 0xE8, "CHudSonicStage::m_rcSpeedGauge must remain at +0xE8");
    static_assert(offsetof(CHudSonicStage, m_rcRingEnergyGauge) == 0xF0, "CHudSonicStage::m_rcRingEnergyGauge must remain at +0xF0");
    static_assert(offsetof(CHudSonicStage, m_rcGaugeFrame) == 0xF8, "CHudSonicStage::m_rcGaugeFrame must remain at +0xF8");
    static_assert(offsetof(CHudSonicStage, m_rcExpCount) == 0x100, "CHudSonicStage::m_rcExpCount must remain at +0x100");
    static_assert(offsetof(CHudSonicStage, m_rcPtrField108) == 0x108, "CHudSonicStage::m_rcPtrField108 must remain at +0x108");
    static_assert(offsetof(CHudSonicStage, m_rcPtrField110) == 0x110, "CHudSonicStage::m_rcPtrField110 must remain at +0x110");
    static_assert(offsetof(CHudSonicStage, m_rcSpeedCount) == 0x118, "CHudSonicStage::m_rcSpeedCount must remain at +0x118");
    static_assert(offsetof(CHudSonicStage, m_rcPtrField120) == 0x120, "CHudSonicStage::m_rcPtrField120 must remain at +0x120");
    static_assert(offsetof(CHudSonicStage, m_rcScoreCount) == 0x128, "CHudSonicStage::m_rcScoreCount must remain at +0x128");
    static_assert(offsetof(CHudSonicStage, m_rcTimeCount) == 0x130, "CHudSonicStage::m_rcTimeCount must remain at +0x130");
    static_assert(offsetof(CHudSonicStage, m_rcTimeCount2) == 0x138, "CHudSonicStage::m_rcTimeCount2 must remain at +0x138");
    static_assert(offsetof(CHudSonicStage, m_rcTimeCount3) == 0x140, "CHudSonicStage::m_rcTimeCount3 must remain at +0x140");
    static_assert(offsetof(CHudSonicStage, m_rcPlayerCount) == 0x148, "CHudSonicStage::m_rcPlayerCount must remain at +0x148");
    static_assert(offsetof(CHudSonicStage, m_rcPtrField150) == 0x150, "CHudSonicStage::m_rcPtrField150 must remain at +0x150");
    static_assert(offsetof(CHudSonicStage, m_rcPtrField158) == 0x158, "CHudSonicStage::m_rcPtrField158 must remain at +0x158");
    static_assert(offsetof(CHudSonicStage, m_rcPtrField160) == 0x160, "CHudSonicStage::m_rcPtrField160 must remain at +0x160");
    static_assert(offsetof(CHudSonicStage, m_rcPtrField168) == 0x168, "CHudSonicStage::m_rcPtrField168 must remain at +0x168");
    static_assert(offsetof(CHudSonicStage, m_rcPtrField170) == 0x170, "CHudSonicStage::m_rcPtrField170 must remain at +0x170");
    static_assert(offsetof(CHudSonicStage, m_rcPtrField178) == 0x178, "CHudSonicStage::m_rcPtrField178 must remain at +0x178");
    static_assert(offsetof(CHudSonicStage, m_rcPtrField180) == 0x180, "CHudSonicStage::m_rcPtrField180 must remain at +0x180");

    // Each binding tells the runtime which CSD project / scene path should
    // populate the named RCPtr member. Sourced from the live
    // `hud_owner_layout.json` sidecar (Phase 262 / 264) so every entry is
    // backed by an observed runtime correlation, not a guess.
    struct SceneBinding
    {
        std::string_view memberName;
        std::size_t      memberOffset;
        std::string_view projectName;
        std::string_view scenePath;
        std::string_view confidenceTier;
        std::size_t      instanceCount;
    };

    static constexpr std::array<SceneBinding, 4> kSceneBindings =
    {{
        {"m_rcSpeedGauge", 0xE8, "ui_playscreen", "ui_playscreen/so_speed_gauge", "cross-validated", 1},
        {"m_rcRingEnergyGauge", 0xF0, "ui_playscreen", "ui_playscreen/so_ringenagy_gauge", "cross-validated", 1},
        {"m_rcGaugeFrame", 0xF8, "ui_playscreen", "ui_playscreen/gauge_frame", "cross-validated", 1},
        {"m_rcSpeedCount", 0x118, "ui_playscreen", "ui_playscreen/add/speed_count", "cross-validated", 1},
    }};

    static constexpr std::string_view kGeneratedAt = "2026-05-05T00:13:57+00:00";
    static constexpr std::string_view kSourceAttribution = "api/SWA/HUD/Sonic/HudSonicStage.h offsets 0xE0..0x184; Phase 265 expanded from CHudSonicStage::CHudSonicStage (sub_824D89B0) decoded from local_build_env/ur103clean/UnleashedRecompLib/ppc/ppc_recomp.28.cpp:61909";

} // namespace sward::ui_runtime::generated::sgfx_hud
