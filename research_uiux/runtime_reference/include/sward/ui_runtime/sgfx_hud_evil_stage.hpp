// Phase 341: SGFX-shaped port of SWA::CHudEvilStage and the EvilEnemy
// combat overlay family (Werehog gameplay HUD).
//
// Phase 340 retail mining (xex RTTI scan) confirmed the Werehog HUD
// is a SEPARATE class from CHudSonicStage. Source files mined from
// default_patched.xex string table:
//
//   D:\SonicWorldAdventure\SWA\source\HUD\Evil\HudEvilStage.cpp
//   D:\SonicWorldAdventure\SWA\source\HUD\Evil\EvilMainDisplay.cpp
//   D:\SonicWorldAdventure\SWA\source\NPC\Enemy\EvilEnemy\Common\
//     ChanceAttack\EvilEnemyChanceAttackHud.cpp
//     ChanceAttack\EvilEnemyChanceAttackHudSuccess.cpp
//     Combo\EvilEnemyComboHud.cpp
//     EvilEnemyHudStage.cpp
//
// And the related state-machine / object classes:
//   CEvilHudGuide@SWA      -- TStateMachine<CEvilHudGuide>; ported
//                             separately in sgfx_evil_hud_guide.hpp
//   CEvilHudTarget@SWA     -- targeting reticle (NEW here)
//
// The Werehog stage HUD shares several fields with the Day Sonic HUD
// (rings, score, time, lives, pause flag) so this header reuses the
// generic StageHudState core and ADDS Werehog-only fields:
//   * Dark Gaia energy bar
//   * Combo counter (per EvilEnemyComboHud)
//   * Chance Attack overlay (per EvilEnemyChanceAttackHud)
//   * Targeting reticle visibility (per CEvilHudTarget)
//   * Out-of-control gauge (CEvilSonicContext::m_OutOfControlCount,
//     mined api/SWA/Player/Character/EvilSonic/EvilSonicContext.h)
//
// The Werehog QTE prompt (CEvilHudGuide -> A/B/X/Y) lives in
// sgfx_evil_hud_guide.hpp as a sibling state machine; this header
// just references it via state composition.

#pragma once

#include "sgfx_stage_hud.hpp"
#include "sgfx_evil_hud_guide.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // EvilEnemyComboHud: visible combo counter shown above the
    // enemy currently being hit. Cleared after a fixed grace
    // window once the player stops hitting; visible while >0.
    struct EvilComboHud
    {
        std::int32_t  comboCount = 0;
        float         timeSinceLastHitSeconds = 0.0f;
        float         comboGraceWindowSeconds = 1.5f; // mined as ~1.5s default
        bool          isVisible = false;
    };

    enum class EvilChanceAttackPhase : std::uint8_t
    {
        Idle = 0,
        // Chance Attack: when an enemy reaches a vulnerability state,
        // a button-prompt + trigger window appears. Player presses
        // the prompt -> Active. Successful timing -> Success
        // (separate retail class EvilEnemyChanceAttackHudSuccess).
        Prompt = 1,
        Active = 2,
        Success = 3,
        Failed = 4,
    };

    struct EvilChanceAttack
    {
        EvilChanceAttackPhase phase = EvilChanceAttackPhase::Idle;
        // Visible button on the prompt; reuses EvilGuideType so
        // hosts can render a single sprite atlas for both QTE and
        // Chance Attack prompts.
        EvilGuideType         buttonType = EvilGuideType::A;
        float                 secondsInPhase = 0.0f;
        // Window during which a press is accepted as "success".
        float                 promptWindowSeconds = 0.8f;
    };

    // CEvilHudTarget: the reticle shown over the enemy currently
    // locked-on for a Werehog grab. Position is host-supplied
    // (typically projected from the enemy's world matrix into
    // screen space).
    struct EvilHudTarget
    {
        bool   isVisible = false;
        float  screenX = 0.0f;
        float  screenY = 0.0f;
        // 0..1, fades from 0 at acquisition to 1 at full lock.
        float  lockProgress = 0.0f;
    };

    // The Werehog Stage HUD state. The base StageHudState carries
    // shared fields (rings, score, time, lives, pause); Werehog-only
    // overlays are aggregated here.
    struct EvilStageHudState
    {
        StageHudState     base;            // shared core fields (mode = Werehog)
        EvilHudGuideState qtePrompt;       // CEvilHudGuide instance
        EvilComboHud      combo;           // EvilEnemyComboHud
        EvilChanceAttack  chanceAttack;    // EvilEnemyChanceAttackHud(Success)
        EvilHudTarget     target;          // CEvilHudTarget
        // CEvilSonicContext companions are also tracked on
        // EvilHudGuideState for the QTE machine; we mirror them here
        // so the Werehog HUD root has them in one place too.
        float        darkGaiaEnergy   = 0.0f;
        std::uint32_t outOfControlCount = 0;
    };

    enum class EvilStageHudEventKind : std::uint8_t
    {
        ComboIncremented,
        ComboReset,
        ChanceAttackPromptShown,
        ChanceAttackSuccess,
        ChanceAttackFailed,
        TargetAcquired,
        TargetLost,
    };

    struct EvilStageHudEvent
    {
        EvilStageHudEventKind kind = EvilStageHudEventKind::ComboIncremented;
        std::int32_t          value = 0;
        std::string           sfxCueName;
    };

    // SFX cues for Werehog overlays are not directly grep-mined from
    // UnleashedRecomp's host-side patches (Werehog combat lives in
    // PPC-translated code). Empty strings = host-wired.
    constexpr std::string_view kEvilSfxComboHit       = "";
    constexpr std::string_view kEvilSfxComboReset     = "";
    constexpr std::string_view kEvilSfxChancePrompt   = "";
    constexpr std::string_view kEvilSfxChanceSuccess  = "";
    constexpr std::string_view kEvilSfxChanceFail     = "";
    constexpr std::string_view kEvilSfxTargetAcquire  = "";

    inline std::vector<EvilStageHudEvent> evilStageHudIncrementCombo(
        EvilStageHudState& s) noexcept
    {
        std::vector<EvilStageHudEvent> events;
        s.combo.comboCount += 1;
        s.combo.timeSinceLastHitSeconds = 0.0f;
        s.combo.isVisible = true;
        events.push_back({EvilStageHudEventKind::ComboIncremented,
                          s.combo.comboCount, std::string(kEvilSfxComboHit)});
        return events;
    }

    inline std::vector<EvilStageHudEvent> evilStageHudShowChancePrompt(
        EvilStageHudState& s, EvilGuideType button) noexcept
    {
        std::vector<EvilStageHudEvent> events;
        s.chanceAttack.phase = EvilChanceAttackPhase::Prompt;
        s.chanceAttack.buttonType = button;
        s.chanceAttack.secondsInPhase = 0.0f;
        events.push_back({EvilStageHudEventKind::ChanceAttackPromptShown,
                          static_cast<std::int32_t>(button),
                          std::string(kEvilSfxChancePrompt)});
        return events;
    }

    inline std::vector<EvilStageHudEvent> evilStageHudAcquireTarget(
        EvilStageHudState& s, float screenX, float screenY) noexcept
    {
        std::vector<EvilStageHudEvent> events;
        const bool wasInvisible = !s.target.isVisible;
        s.target.isVisible = true;
        s.target.screenX = screenX;
        s.target.screenY = screenY;
        s.target.lockProgress = 0.0f;
        if (wasInvisible)
            events.push_back({EvilStageHudEventKind::TargetAcquired, 0,
                              std::string(kEvilSfxTargetAcquire)});
        return events;
    }

    inline std::vector<EvilStageHudEvent> evilStageHudReleaseTarget(
        EvilStageHudState& s) noexcept
    {
        std::vector<EvilStageHudEvent> events;
        if (s.target.isVisible)
        {
            s.target.isVisible = false;
            s.target.lockProgress = 0.0f;
            events.push_back({EvilStageHudEventKind::TargetLost, 0, ""});
        }
        return events;
    }

    inline std::vector<EvilStageHudEvent> updateEvilStageHudOneFrame(
        EvilStageHudState& s,
        const StageHudInput& shared,
        bool chanceButtonTappedThisFrame = false)
    {
        std::vector<EvilStageHudEvent> events;
        // Combo grace window expiration.
        if (s.combo.isVisible && s.combo.comboCount > 0)
        {
            s.combo.timeSinceLastHitSeconds += shared.deltaSeconds;
            if (s.combo.timeSinceLastHitSeconds >= s.combo.comboGraceWindowSeconds)
            {
                events.push_back({EvilStageHudEventKind::ComboReset,
                                  s.combo.comboCount,
                                  std::string(kEvilSfxComboReset)});
                s.combo.comboCount = 0;
                s.combo.isVisible = false;
                s.combo.timeSinceLastHitSeconds = 0.0f;
            }
        }
        // Chance Attack window timing.
        if (s.chanceAttack.phase == EvilChanceAttackPhase::Prompt)
        {
            s.chanceAttack.secondsInPhase += shared.deltaSeconds;
            if (chanceButtonTappedThisFrame)
            {
                s.chanceAttack.phase = EvilChanceAttackPhase::Success;
                s.chanceAttack.secondsInPhase = 0.0f;
                events.push_back({EvilStageHudEventKind::ChanceAttackSuccess,
                                  static_cast<std::int32_t>(s.chanceAttack.buttonType),
                                  std::string(kEvilSfxChanceSuccess)});
            }
            else if (s.chanceAttack.secondsInPhase >= s.chanceAttack.promptWindowSeconds)
            {
                s.chanceAttack.phase = EvilChanceAttackPhase::Failed;
                s.chanceAttack.secondsInPhase = 0.0f;
                events.push_back({EvilStageHudEventKind::ChanceAttackFailed,
                                  static_cast<std::int32_t>(s.chanceAttack.buttonType),
                                  std::string(kEvilSfxChanceFail)});
            }
        }
        // Update target lock progress (host-driven; SGFX just clamps).
        if (s.target.isVisible && s.target.lockProgress < 1.0f)
        {
            s.target.lockProgress += shared.deltaSeconds * 4.0f; // ~250ms acquire
            if (s.target.lockProgress > 1.0f) s.target.lockProgress = 1.0f;
        }
        // Mirror the Werehog companion fields onto the QTE state so
        // a single place owns the "current Werehog gameplay state".
        s.qtePrompt.darkGaiaEnergy = s.darkGaiaEnergy;
        s.qtePrompt.outOfControlCount = s.outOfControlCount;
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
