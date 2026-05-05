// Phase 331: SGFX-shaped port of SWA::Player::CEvilHudGuide.
//
// Sourced directly from
//   local_build_env/ur103clean/UnleashedRecomp/api/SWA/Player/Character/
//   EvilSonic/Hud/EvilHudGuide.h
//
// CEvilHudGuide is the on-screen button-prompt overlay used during
// Werehog ("Evil Sonic") grab/throw QTE sequences. When the Werehog
// picks up an enemy or starts a chain combo, the HUD shows an "A"
// (or B / X / Y) prompt; the player presses it to commit the action.
// EGuideAction distinguishes a single-press prompt from a multi-press
// chain.
//
// Real retail layout (CEvilHudGuide @ +0xXX from CGameObject):
//   m_pVftable       @ +0x00
//   <pad +0x8D>
//   m_IsShown        @ +0x95   bool
//   m_IsVisible      @ +0x96   bool
//   m_GuideType      @ +0x98   be<EGuideType>
//
// Companion fields live on CEvilSonicContext:
//   m_DarkGaiaEnergy   (float, +0x688)
//   m_AnimationID      (uint32, +0x7C8)
//   m_GuideType        (EGuideType, +0x808 region)
//   m_OutOfControlCount (uint32, +0x8B8 region)

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Mirror of SWA::Player::EGuideAction (retail values: Single=0, Chain=1).
    enum class EvilGuideAction : std::uint32_t
    {
        Single = 0,
        Chain  = 1,
    };

    // Mirror of SWA::Player::EGuideType (retail values: A=0..Y=3).
    enum class EvilGuideType : std::uint32_t
    {
        A = 0,
        B = 1,
        X = 2,
        Y = 3,
    };

    enum class EvilHudGuideEventKind : std::uint8_t
    {
        GuideShown,       // m_IsShown 0->1: prompt appears on screen
        GuideHidden,      // m_IsShown 1->0: prompt fades out
        GuideTypeChanged, // m_GuideType changed (A->B etc)
        ChainAdvanced,    // chain QTE consumed one of multiple presses
        ChainCompleted,   // chain QTE filled its press budget
        ButtonMissed,     // wrong button or timed out
    };

    // CEvilHudGuide field mirror.
    struct EvilHudGuideState
    {
        bool          isShown   = false;            // m_IsShown @ +0x95
        bool          isVisible = false;            // m_IsVisible @ +0x96
        EvilGuideType guideType = EvilGuideType::A; // m_GuideType @ +0x98

        // CEvilSonicContext companions tracked here so the QTE state
        // machine can read them in one place. Values are host-fed.
        float        darkGaiaEnergy = 0.0f;        // m_DarkGaiaEnergy @ +0x688
        std::uint32_t animationId   = 0;           // m_AnimationID @ context+0x7C8
        std::uint32_t outOfControlCount = 0;       // m_OutOfControlCount @ context+0x8B8

        // Chain-QTE bookkeeping (CEvilHudGuide doesn't store these
        // per the api header; the gameplay code drives them through
        // CGuideAction events). SGFX hosts maintain them so the same
        // single-vs-chain distinction flows through to events.
        EvilGuideAction action = EvilGuideAction::Single;
        std::int32_t   chainPressesRemaining = 0;
        std::int32_t   chainPressesTotal     = 0;
    };

    struct EvilHudGuideInput
    {
        bool aTapped = false;
        bool bTapped = false;
        bool xTapped = false;
        bool yTapped = false;
        // Host signals when the QTE timer runs out.
        bool qteTimedOut = false;
    };

    struct EvilHudGuideEvent
    {
        EvilHudGuideEventKind kind = EvilHudGuideEventKind::GuideShown;
        EvilGuideType         atType = EvilGuideType::A;
        std::int32_t          chainRemaining = 0;
        std::string           sfxCueName;
    };

    // Cues for QTE feedback are not directly grep-mineable from
    // UnleashedRecomp's host-side patches (the Werehog grab logic
    // lives entirely in PPC-translated code). Confirmed cue is
    // empty here -- host wires to its audio bank.
    constexpr std::string_view kEvilGuideSfxShow      = "";
    constexpr std::string_view kEvilGuideSfxAdvance   = "";
    constexpr std::string_view kEvilGuideSfxComplete  = "";
    constexpr std::string_view kEvilGuideSfxMiss      = "";

    // Host calls this when the gameplay code has decided a QTE
    // prompt should appear. Sets m_IsShown + m_GuideType to mirror
    // what the retail PPC code does.
    inline std::vector<EvilHudGuideEvent> showEvilHudGuide(
        EvilHudGuideState& s,
        EvilGuideType type,
        EvilGuideAction action,
        std::int32_t chainTotal = 1) noexcept
    {
        s.isShown = true;
        s.isVisible = true;
        s.guideType = type;
        s.action = action;
        s.chainPressesTotal = (action == EvilGuideAction::Chain) ? chainTotal : 1;
        s.chainPressesRemaining = s.chainPressesTotal;
        return {{EvilHudGuideEventKind::GuideShown, type,
                 s.chainPressesRemaining,
                 std::string(kEvilGuideSfxShow)}};
    }

    inline std::vector<EvilHudGuideEvent> hideEvilHudGuide(
        EvilHudGuideState& s) noexcept
    {
        if (!s.isShown && !s.isVisible) return {};
        s.isShown = false;
        s.isVisible = false;
        return {{EvilHudGuideEventKind::GuideHidden, s.guideType, 0, ""}};
    }

    inline std::vector<EvilHudGuideEvent> updateEvilHudGuideOneFrame(
        EvilHudGuideState& s,
        const EvilHudGuideInput& input)
    {
        std::vector<EvilHudGuideEvent> events;
        if (!s.isShown || !s.isVisible) return events;

        const bool pressedCorrect =
              (s.guideType == EvilGuideType::A && input.aTapped)
           || (s.guideType == EvilGuideType::B && input.bTapped)
           || (s.guideType == EvilGuideType::X && input.xTapped)
           || (s.guideType == EvilGuideType::Y && input.yTapped);

        const bool pressedAnyOther =
            (input.aTapped || input.bTapped || input.xTapped || input.yTapped)
            && !pressedCorrect;

        if (pressedCorrect)
        {
            if (s.action == EvilGuideAction::Single)
            {
                events.push_back({EvilHudGuideEventKind::ChainCompleted,
                                  s.guideType, 0,
                                  std::string(kEvilGuideSfxComplete)});
                s.isShown = false; s.isVisible = false;
            }
            else
            {
                s.chainPressesRemaining = std::max(0, s.chainPressesRemaining - 1);
                if (s.chainPressesRemaining == 0)
                {
                    events.push_back({EvilHudGuideEventKind::ChainCompleted,
                                      s.guideType, 0,
                                      std::string(kEvilGuideSfxComplete)});
                    s.isShown = false; s.isVisible = false;
                }
                else
                {
                    events.push_back({EvilHudGuideEventKind::ChainAdvanced,
                                      s.guideType, s.chainPressesRemaining,
                                      std::string(kEvilGuideSfxAdvance)});
                }
            }
        }
        else if (pressedAnyOther || input.qteTimedOut)
        {
            events.push_back({EvilHudGuideEventKind::ButtonMissed,
                              s.guideType, s.chainPressesRemaining,
                              std::string(kEvilGuideSfxMiss)});
            s.isShown = false; s.isVisible = false;
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
