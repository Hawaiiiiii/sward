// Phase 350: SGFX-shaped port of CHudExQte (Tornado Defense HUD).
//
// Phase 340 catalog recovery (xex RTTI scan):
//   D:\SonicWorldAdventure\SWA\source\ExtraStage\Tails\Hud\HudExQte.cpp
//   .?AVCHudExQte@SWA@@
//
// CHudExQte is the Tornado Defense / EX Stage HUD. Tails pilots a
// rear-view shooter section with QTE prompts that pop up when an
// enemy fires a projectile -- the player must press the prompted
// button to dodge or counter. Multiple prompts can be active at
// once (multi-button rhythm).

#pragma once

#include "sgfx_evil_hud_guide.hpp" // re-uses EvilGuideType (A/B/X/Y)

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class ExQteSlotPhase : std::uint8_t
    {
        Empty = 0,
        Active = 1,
        Success = 2,
        Failed = 3,
    };

    struct ExQteSlot
    {
        ExQteSlotPhase phase = ExQteSlotPhase::Empty;
        EvilGuideType  buttonType = EvilGuideType::A;
        // Screen position of the prompt sprite.
        float          screenX = 0.0f;
        float          screenY = 0.0f;
        float          secondsInPhase = 0.0f;
        float          activeWindowSeconds = 1.0f;
    };

    enum class ExQteEventKind : std::uint8_t
    {
        SlotShown,
        SlotHit,
        SlotMissed,
    };

    struct ExQteState
    {
        // Up to 4 simultaneous prompts. Retail Tornado Defense
        // typically shows 1-2 at once; 4 is a generous cap.
        std::array<ExQteSlot, 4> slots{};
        std::int32_t score = 0;
        std::int32_t hitsInARow = 0;
    };

    struct ExQteInput
    {
        bool aTapped = false, bTapped = false, xTapped = false, yTapped = false;
        float deltaSeconds = 0.0f;
    };

    struct ExQteEvent
    {
        ExQteEventKind kind = ExQteEventKind::SlotShown;
        std::int32_t   slotIndex = 0;
        std::string    sfxCueName;
    };

    inline std::vector<ExQteEvent> spawnExQteSlot(
        ExQteState& s, EvilGuideType button, float x, float y,
        float windowSeconds = 1.0f) noexcept
    {
        std::vector<ExQteEvent> events;
        for (std::size_t i = 0; i < s.slots.size(); ++i)
        {
            if (s.slots[i].phase == ExQteSlotPhase::Empty)
            {
                s.slots[i].phase = ExQteSlotPhase::Active;
                s.slots[i].buttonType = button;
                s.slots[i].screenX = x;
                s.slots[i].screenY = y;
                s.slots[i].secondsInPhase = 0.0f;
                s.slots[i].activeWindowSeconds = windowSeconds;
                events.push_back({ExQteEventKind::SlotShown,
                                  static_cast<std::int32_t>(i), ""});
                break;
            }
        }
        return events;
    }

    inline std::vector<ExQteEvent> updateExQteOneFrame(
        ExQteState& s, const ExQteInput& input)
    {
        std::vector<ExQteEvent> events;
        for (std::size_t i = 0; i < s.slots.size(); ++i)
        {
            auto& slot = s.slots[i];
            if (slot.phase != ExQteSlotPhase::Active) continue;
            slot.secondsInPhase += input.deltaSeconds;

            const bool pressedCorrect =
                (slot.buttonType == EvilGuideType::A && input.aTapped)
             || (slot.buttonType == EvilGuideType::B && input.bTapped)
             || (slot.buttonType == EvilGuideType::X && input.xTapped)
             || (slot.buttonType == EvilGuideType::Y && input.yTapped);

            if (pressedCorrect)
            {
                slot.phase = ExQteSlotPhase::Success;
                s.score += 100;
                ++s.hitsInARow;
                events.push_back({ExQteEventKind::SlotHit,
                                  static_cast<std::int32_t>(i), ""});
            }
            else if (slot.secondsInPhase >= slot.activeWindowSeconds)
            {
                slot.phase = ExQteSlotPhase::Failed;
                s.hitsInARow = 0;
                events.push_back({ExQteEventKind::SlotMissed,
                                  static_cast<std::int32_t>(i), ""});
            }
        }
        return events;
    }

    inline void clearExQteSlot(ExQteState& s, std::size_t i) noexcept
    {
        if (i < s.slots.size()) s.slots[i] = ExQteSlot{};
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
