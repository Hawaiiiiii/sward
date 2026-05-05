// Phase 349: SGFX-shaped port of the Boss HUD family.
//
// Phase 340 catalog recovery (xex RTTI scan):
//   D:\SonicWorldAdventure\SWA\source\Boss\BossHudVitality.cpp
//   D:\SonicWorldAdventure\SWA\source\Boss\BossHudSuperSonic.cpp
//   D:\SonicWorldAdventure\SWA\source\Boss\FinalDarkGaia\Object\FinalDarkGaiaHud.cpp
//   D:\SonicWorldAdventure\SWA\source\Boss\Phoenix\Hud\PhoenixHudVitality.cpp
// Plus RTTI: .?AVCBossNamePlate@Boss@SWA@@
//
// Boss HUD family handles boss-fight-specific overlays:
//   * BossHudVitality: HP gauge for ordinary bosses
//   * BossHudSuperSonic: Super Sonic ring drain timer + boss HP
//   * FinalDarkGaiaHud: final boss multi-phase HP + QTE prompts
//   * PhoenixHudVitality: Phoenix sub-boss HP variant
//   * CBossNamePlate: animated boss-name title card
//
// SGFX models them as a single unified BossHudState with a kind
// flag; field set is similar enough that one struct + a switch
// captures all four. CBossNamePlate is a separate small overlay.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class BossKind : std::uint8_t
    {
        Standard       = 0, // BossHudVitality
        SuperSonic     = 1, // BossHudSuperSonic (Super Sonic ring drain)
        FinalDarkGaia  = 2, // FinalDarkGaiaHud
        Phoenix        = 3, // PhoenixHudVitality
    };

    enum class BossHudEventKind : std::uint8_t
    {
        VitalityChanged,
        BossDefeated,
        SuperSonicRingsExhausted,
        PhaseAdvanced,            // FinalDarkGaia multi-phase
    };

    struct BossHudState
    {
        BossKind kind = BossKind::Standard;
        // Vitality gauge: 0..maxVitality.
        std::int32_t maxVitality = 100;
        std::int32_t currentVitality = 100;
        // SuperSonic-only: ring drain starts at maxRings, drains to 0.
        std::int32_t superSonicRingsRemaining = 0;
        float        superSonicDrainPerSecond = 1.0f;
        // FinalDarkGaia-only: phase index (0..3 in retail).
        std::uint8_t darkGaiaPhase = 0;
        // BossNamePlate-only flag: card visible.
        bool         namePlateVisible = false;
    };

    struct BossHudEvent
    {
        BossHudEventKind kind = BossHudEventKind::VitalityChanged;
        std::int32_t     value = 0;
        std::string      sfxCueName;
    };

    inline std::vector<BossHudEvent> applyBossDamage(
        BossHudState& s, std::int32_t damage) noexcept
    {
        std::vector<BossHudEvent> events;
        if (damage <= 0) return events;
        s.currentVitality -= damage;
        if (s.currentVitality < 0) s.currentVitality = 0;
        events.push_back({BossHudEventKind::VitalityChanged,
                          s.currentVitality, ""});
        if (s.currentVitality == 0)
        {
            events.push_back({BossHudEventKind::BossDefeated, 0, ""});
        }
        return events;
    }

    inline std::vector<BossHudEvent> tickSuperSonicDrain(
        BossHudState& s, float deltaSeconds) noexcept
    {
        std::vector<BossHudEvent> events;
        if (s.kind != BossKind::SuperSonic) return events;
        const float drain = s.superSonicDrainPerSecond * deltaSeconds;
        const auto drainInt = static_cast<std::int32_t>(drain);
        if (drainInt <= 0) return events;
        s.superSonicRingsRemaining -= drainInt;
        if (s.superSonicRingsRemaining <= 0)
        {
            s.superSonicRingsRemaining = 0;
            events.push_back({BossHudEventKind::SuperSonicRingsExhausted, 0, ""});
        }
        return events;
    }

    inline std::vector<BossHudEvent> advanceDarkGaiaPhase(
        BossHudState& s) noexcept
    {
        std::vector<BossHudEvent> events;
        if (s.kind != BossKind::FinalDarkGaia) return events;
        if (s.darkGaiaPhase < 3) ++s.darkGaiaPhase;
        s.currentVitality = s.maxVitality; // refresh gauge for new phase
        events.push_back({BossHudEventKind::PhaseAdvanced,
                          s.darkGaiaPhase, ""});
        return events;
    }

    // CBossNamePlate animation: simple show/hide with auto-fade.
    struct BossNamePlateState
    {
        bool  isVisible = false;
        float secondsRemaining = 0.0f;
        float autoFadeSeconds = 4.0f;
    };

    inline void showBossNamePlate(BossNamePlateState& s) noexcept
    {
        s.isVisible = true;
        s.secondsRemaining = s.autoFadeSeconds;
    }

    inline void tickBossNamePlate(BossNamePlateState& s, float deltaSeconds) noexcept
    {
        if (!s.isVisible) return;
        s.secondsRemaining -= deltaSeconds;
        if (s.secondsRemaining <= 0.0f)
        {
            s.isVisible = false;
            s.secondsRemaining = 0.0f;
        }
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
