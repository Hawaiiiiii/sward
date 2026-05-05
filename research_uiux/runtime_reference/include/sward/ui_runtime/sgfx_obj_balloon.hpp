// Phase 343: SGFX-shaped port of SWA::CObjBalloon and the hub
// town message-passing protocol.
//
// Phase 340 retail mining recovered every Msg* type the hub uses
// to talk to its NPCs. The xex .data section also carries the
// bind/functor RTTI for CObjBalloon, confirming it's a heap-
// allocated game object (not a HUD class).
//
// CObjBalloon is the on-screen talk-balloon overlay that pops up
// above an NPC ("town man") when the player engages them. Its
// state machine is driven through the Msg* protocol -- the player's
// avatar sends MsgRequestNowTalkBalloon to the talkable NPC, the
// NPC's CTownManBase responds with MsgReceiveNowTalkBalloon to
// summon the balloon, and the balloon listens for further
// MsgSetTownManTalk(End) and MsgReceiveSelectBalloon to advance
// dialogue / select a branch.
//
// SGFX models the messages as plain enum + payload structs so a
// host can dispatch them without needing the Hedgehog Engine's
// MessageActor machinery. The state machine here is the one a
// CObjBalloon would run on its own; a CTownManBase would run a
// parallel state machine that's not modeled here yet.

#pragma once

#include "sgfx_hub_screen.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Phase 343: retail Msg* set for hub NPC interaction. Names
    // mirror the RTTI strings recovered in Phase 340:
    //   .?AUMsgSetTownManTalk@Message@SWA@@   etc.
    enum class HubMessageKind : std::uint16_t
    {
        // Talk lifecycle.
        SetTownManTalk          = 0,
        SetTownManTalkEnd       = 1,
        SetTownManTalkTwo       = 2,
        SetTownManTalkTarget    = 3,
        SetTownManTalkableMark  = 4,
        SetTownManSpeakActive   = 5,
        // Balloon protocol.
        RequestNowTalkBalloon   = 6,
        ReceiveNowTalkBalloon   = 7,
        ReceiveSelectBalloon    = 8,
        // Animation control.
        ChangeTownManAnimation        = 10,
        ChangeTownManFacialAnimation  = 11,
        ChangeTownManFaceForce        = 12,
        // Visual highlight.
        SetTownManFlash         = 20,
        SetTownManFlashEnd      = 21,
        SetTownManFlashTarget   = 22,
        SetTownManVibration     = 23,
        // Activity / direction.
        SetTownManActive        = 30,
        SetTownManDirection     = 31,
        SetTownManRetryTimeTable = 32,
        SetTownManGrobalWindow  = 33, // sic -- retail typo preserved
        // Mission notification.
        LetTownManKnowMissionEnd = 40,
        // Time of day.
        SetTownTime             = 50,
        // Queries (return data via reply pointer in retail).
        GetIsTownManTalkable    = 60,
        GetIsTownManFlashable   = 61,
        GetTownManCamOffset     = 62,
        GetTownManCountry       = 63,
        GetTownManHeight        = 64,
        GetTownManID            = 65,
        GetTownManPkg           = 66,
        GetTownManScream        = 67,
        GetTownManTalkMatrix    = 68,
        GetTownManTalkPreInfo   = 69,
    };

    // Compact payload union -- small enough to pass by value. SGFX
    // hosts that need richer data (e.g. talk-matrix worldspace) can
    // use the talkAnchor fields on HubTownMan or extend this struct.
    struct HubMessage
    {
        HubMessageKind kind = HubMessageKind::SetTownManTalkEnd;
        std::int32_t   targetTownManId = -1; // which NPC the message is for
        // Generic payload slots; meaning depends on kind.
        std::int32_t   intParam0 = 0;
        std::int32_t   intParam1 = 0;
        float          floatParam0 = 0.0f;
        bool           boolParam0 = false;
    };

    // CObjBalloon visual state. Retail's CObjBalloon runs its own
    // animation timeline; SGFX collapses that to a phase + dwell.
    enum class BalloonPhase : std::uint8_t
    {
        Hidden       = 0,
        Appearing    = 1, // pop-in animation
        Showing      = 2, // dialog text being displayed
        Choosing     = 3, // player picking a dialog branch
        Disappearing = 4, // pop-out animation
    };

    struct ObjBalloonState
    {
        BalloonPhase phase = BalloonPhase::Hidden;
        // Which NPC is "speaking" this balloon (-1 = none).
        std::int32_t boundTownManId = -1;
        // World-space anchor (typically the NPC's head). The host
        // projects to screen space; SGFX stores both.
        float        worldAnchorX = 0.0f;
        float        worldAnchorY = 0.0f;
        float        worldAnchorZ = 0.0f;
        // Dialog choice index when in Choosing phase.
        std::int32_t choiceIndex = 0;
        std::int32_t choiceCount = 0;
        // Animation timers.
        float        secondsInPhase = 0.0f;
        float        appearDurationSeconds = 0.15f;
        float        disappearDurationSeconds = 0.15f;
    };

    enum class BalloonEventKind : std::uint8_t
    {
        Appeared,
        ShowingNow,
        ChoiceSelected,
        Disappeared,
    };

    struct BalloonEvent
    {
        BalloonEventKind kind = BalloonEventKind::Appeared;
        std::int32_t     value = 0; // choice index for ChoiceSelected
        std::string      sfxCueName;
    };

    constexpr std::string_view kBalloonSfxAppear = "obj_navi_appear";

    // Phase 343: dispatch a single HubMessage. Returns the (possibly
    // empty) event list the message would produce. This is the
    // SGFX-side handler -- not all messages drive the balloon state
    // machine (e.g. SetTownManVibration is a pure NPC effect with no
    // balloon-side reaction).
    inline std::vector<BalloonEvent> dispatchHubMessage(
        ObjBalloonState& balloon,
        HubTownMan& npc,
        const HubMessage& msg) noexcept
    {
        std::vector<BalloonEvent> events;
        switch (msg.kind)
        {
            case HubMessageKind::ReceiveNowTalkBalloon:
                if (balloon.phase == BalloonPhase::Hidden)
                {
                    balloon.phase = BalloonPhase::Appearing;
                    balloon.boundTownManId = msg.targetTownManId;
                    balloon.worldAnchorX = npc.talkAnchorX;
                    balloon.worldAnchorY = npc.talkAnchorY;
                    balloon.worldAnchorZ = npc.talkAnchorZ;
                    balloon.secondsInPhase = 0.0f;
                    balloon.choiceIndex = msg.intParam0;
                    balloon.choiceCount = msg.intParam1;
                    events.push_back({BalloonEventKind::Appeared,
                                      msg.targetTownManId,
                                      std::string(kBalloonSfxAppear)});
                }
                break;
            case HubMessageKind::SetTownManTalk:
                npc.isSpeaking = true;
                break;
            case HubMessageKind::SetTownManTalkEnd:
                npc.isSpeaking = false;
                if (balloon.phase == BalloonPhase::Showing
                    || balloon.phase == BalloonPhase::Choosing)
                {
                    balloon.phase = BalloonPhase::Disappearing;
                    balloon.secondsInPhase = 0.0f;
                }
                break;
            case HubMessageKind::ReceiveSelectBalloon:
                if (balloon.phase == BalloonPhase::Choosing)
                {
                    balloon.choiceIndex = msg.intParam0;
                    events.push_back({BalloonEventKind::ChoiceSelected,
                                      msg.intParam0, ""});
                    balloon.phase = BalloonPhase::Disappearing;
                    balloon.secondsInPhase = 0.0f;
                }
                break;
            case HubMessageKind::SetTownManFlash:
                npc.isFlashing = true;
                break;
            case HubMessageKind::SetTownManFlashEnd:
                npc.isFlashing = false;
                break;
            case HubMessageKind::SetTownManSpeakActive:
                npc.isSpeaking = msg.boolParam0;
                break;
            case HubMessageKind::SetTownManActive:
                npc.isActive = msg.boolParam0;
                break;
            case HubMessageKind::SetTownManTalkableMark:
                npc.isTalkable = msg.boolParam0;
                break;
            // Query messages: SGFX answers in-place by writing to
            // the message's float/int param slots (caller passes
            // mutable HubMessage). Host adapters can rewrite this if
            // they want to use Hedgehog's reply-via-pointer pattern.
            default:
                break;
        }
        return events;
    }

    // Reply-shape helpers for the Get* messages. SGFX fills the
    // payload in-place so callers don't have to pattern-match the
    // dispatch return.
    inline void replyGetIsTownManTalkable(HubMessage& msg, const HubTownMan& npc) noexcept
    { msg.boolParam0 = npc.isTalkable; }
    inline void replyGetIsTownManFlashable(HubMessage& msg, const HubTownMan& npc) noexcept
    { msg.boolParam0 = npc.isFlashable; }
    inline void replyGetTownManCountry(HubMessage& msg, const HubTownMan& npc) noexcept
    { msg.intParam0 = static_cast<std::int32_t>(npc.countryId); }
    inline void replyGetTownManID(HubMessage& msg, const HubTownMan& npc) noexcept
    { msg.intParam0 = npc.npcId; }

    // Per-frame tick that advances the balloon's animation phases.
    // Hosts call this every frame on whatever ObjBalloonState is
    // active; messages drive entry into Appearing / Choosing /
    // Disappearing, this routine drives those phases to completion.
    inline std::vector<BalloonEvent> updateObjBalloonOneFrame(
        ObjBalloonState& balloon, float deltaSeconds)
    {
        std::vector<BalloonEvent> events;
        balloon.secondsInPhase += deltaSeconds;
        if (balloon.phase == BalloonPhase::Appearing
            && balloon.secondsInPhase >= balloon.appearDurationSeconds)
        {
            balloon.phase = (balloon.choiceCount > 0)
                ? BalloonPhase::Choosing
                : BalloonPhase::Showing;
            balloon.secondsInPhase = 0.0f;
            events.push_back({BalloonEventKind::ShowingNow,
                              balloon.boundTownManId, ""});
        }
        else if (balloon.phase == BalloonPhase::Disappearing
                 && balloon.secondsInPhase >= balloon.disappearDurationSeconds)
        {
            balloon.phase = BalloonPhase::Hidden;
            balloon.secondsInPhase = 0.0f;
            const auto fired = balloon.boundTownManId;
            balloon.boundTownManId = -1;
            balloon.choiceCount = 0;
            balloon.choiceIndex = 0;
            events.push_back({BalloonEventKind::Disappeared, fired, ""});
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
