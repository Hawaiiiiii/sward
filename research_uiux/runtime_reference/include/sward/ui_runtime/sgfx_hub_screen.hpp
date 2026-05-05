// Phase 309 / 340: SGFX-shaped port of the Hub world UI.
//
// Phase 340 retail-fidelity update: xex-mined RTTI (sgfx_xex_strings_dumper
// against default_patched.xex) revealed the actual retail class
// hierarchy is much richer than my earlier inferred port:
//
//   CHudTown@SWA           -- main hub HUD (was: HubState root inferred)
//   CObjBalloon@SWA        -- talk balloon overlay (lives in Object/, not HUD/)
//   CTownManBase@SWA       -- base class for hub NPCs ("town men")
//   CTownManParameterPool  -- NPC config pool, reads XML
//   CTownManStateDamage    -- NPC sub-state (taking damage)
//   CTownManStateDispel    -- NPC sub-state (being dispelled)
//   TownShopCamera         -- camera-controller for shop interactions
//   TownTalkCamera         -- camera-controller for NPC talk
//
//   CMediaRoom@SWA         -- media room main screen
//   CMediaRoomDetail       -- media room detail panel
//   CMediaRoomSelectCountry / SelectItem / SelectBook / ItemList / ScrollBar
//
// Plus a retail message-passing protocol (40+ Msg* types) that
// drives talkability, flash highlight, animation triggers, etc.
// See sgfx_retail_class_hierarchy.generated.json for the full
// catalog. The SGFX HubState below is intentionally simpler than
// the retail multi-class graph -- it preserves behavior at the
// orchestrator boundary while flagging where the inferred logic
// would need to expand to mirror retail in full.
//
// Day Sonic and Werehog (codename "Evil Sonic") share the hub
// state but with different time-of-day display and different
// available NPC interactions. SGFX exposes the time-of-day mode
// as a host parameter so the standalone build can switch freely.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class HubMode : std::uint8_t
    {
        DaySonic = 0,
        Werehog  = 1, // a.k.a. Evil Sonic / Night
    };

    enum class HubOverlay : std::uint8_t
    {
        None         = 0,
        BalloonText  = 1,  // ui_balloon.yncp
        ShopMenu     = 2,  // ui_shop.yncp
        StageGate    = 3,  // ui_gate.yncp
        MissionBrief = 4,  // ui_missionscreen.yncp / ui_misson.yncp
        TownMap      = 5,  // ui_townscreen.yncp
    };

    enum class HubEventKind : std::uint8_t
    {
        OverlayOpened,
        OverlayClosed,
        StageEntryConfirmed,    // gate -> stage launch
        ShopPurchaseConfirmed,  // shop dialogue
        MissionAccepted,        // mission brief
        TimeOfDayChanged,       // Day -> Werehog or vice versa
    };

    struct HubInput
    {
        bool acceptTapped = false;
        bool cancelTapped = false;
        bool talkTapped = false;       // X / Y to interact with NPC
        bool openMapTapped = false;    // Back / Select for town map
    };

    // Phase 340 retail-mined: the hub NPC ("town man") tracks
    // talkability + highlight + screaming separately. Mirrors the
    // boolean queries the retail Msg* protocol exposes:
    //   MsgGetIsTownManTalkable  -> isTalkable
    //   MsgGetIsTownManFlashable -> isFlashable
    //   MsgSetTownManFlash       -> sets isFlashing
    //   MsgSetTownManSpeakActive -> sets isSpeaking
    // Real retail also stores country, height, anim ID, facial-anim,
    // talk matrix, and a parameter-pool pointer; SGFX captures the
    // ones the orchestrator + renderer actually consume.
    struct HubTownMan
    {
        std::int32_t  npcId = -1;        // MsgGetTownManID
        std::uint8_t  countryId = 0;     // MsgGetTownManCountry (Apotos=0..)
        bool          isTalkable = false;
        bool          isFlashable = false;
        bool          isFlashing = false;
        bool          isSpeaking = false;
        bool          isActive = true;   // MsgSetTownManActive
        // Cached talk position (MsgGetTownManTalkMatrix returns the
        // world matrix; SGFX collapses it to an X/Y/Z anchor).
        float         talkAnchorX = 0.0f;
        float         talkAnchorY = 0.0f;
        float         talkAnchorZ = 0.0f;
    };

    struct HubState
    {
        HubMode mode = HubMode::DaySonic;
        HubOverlay overlay = HubOverlay::None;

        // Phase 340: which NPC the player is currently interacting
        // with (-1 = none). Retail keeps the actual CTownManBase*
        // pointer; SGFX uses an index into a host-provided NPC list.
        std::int32_t activeTownManIndex = -1;

        // Phase 340: time-of-day timer (MsgSetTownTime in retail).
        // SGFX hosts that don't model day/night cycles can leave
        // this at 0; orchestrator does not consume it.
        float timeOfDaySeconds = 0.0f;
    };

    struct HubEvent
    {
        HubEventKind kind = HubEventKind::OverlayOpened;
        HubOverlay   overlay = HubOverlay::None;
        std::string  sfxCueName;
    };

    // Phase 318 retail-fidelity update from captured logcat
    // (phase315_take2 session, frames 7000..16950 in the WorldMap +
    // Hub navigation window). The user opened in-hub menus 94+ times
    // and the cursor cues fired as sys_actstg_pausecursor (NOT
    // sys_worldmap_cursor). The pause-bank cues are the real
    // sound-effect bank shared by hub gate/shop/mission menus
    // because those are stage-context overlays. Balloon dialogue
    // ("twn" = town) keeps its dedicated cue. Tutorial popup
    // (3 captured fires) uses obj_navi_appear.
    constexpr std::string_view kHubSfxOpenMenu        = "sys_actstg_pausewinopen";  // 3 captured fires
    constexpr std::string_view kHubSfxCloseMenu       = "sys_actstg_pausewinclose"; // 2 captured fires
    constexpr std::string_view kHubSfxConfirm         = "sys_actstg_pausedecide";   // 1 captured fire
    constexpr std::string_view kHubSfxCancel          = "sys_actstg_pausecansel";
    constexpr std::string_view kHubSfxCursor          = "sys_actstg_pausecursor";   // 94 captured fires
    constexpr std::string_view kHubSfxBalloonAdvance  = "sys_actstg_twn_speechbutton"; // 31 captured fires
    constexpr std::string_view kHubSfxTutorialPopup   = "obj_navi_appear";           // 3 captured fires
    // Some flows still go through sys_worldmap_decide (16 captured fires
    // in the WorldMap range, attributed to stage-pick confirms going
    // through the world-map-side menu rather than the hub-side).
    constexpr std::string_view kHubSfxWorldMapConfirm = "sys_worldmap_decide";

    inline std::vector<HubEvent> openHubOverlay(HubState& s, HubOverlay o)
    {
        if (s.overlay == o) return {};
        s.overlay = o;
        return {{HubEventKind::OverlayOpened, o, std::string(kHubSfxOpenMenu)}};
    }

    inline std::vector<HubEvent> closeHubOverlay(HubState& s)
    {
        if (s.overlay == HubOverlay::None) return {};
        const auto prev = s.overlay;
        s.overlay = HubOverlay::None;
        return {{HubEventKind::OverlayClosed, prev, std::string(kHubSfxCloseMenu)}};
    }

    // Phase 318: tutorial popup. The retail Sonic Unleashed hub
    // pops a "navi" tutorial bubble on first-encounter NPCs and
    // certain dialog triggers; cue is obj_navi_appear (3 captured
    // fires in the trace). Host calls this when their tutorial
    // trigger fires; the SGFX layer just emits the event + cue.
    inline std::vector<HubEvent> openHubTutorialPopup(HubState& s)
    {
        return {{HubEventKind::OverlayOpened, HubOverlay::BalloonText,
                 std::string(kHubSfxTutorialPopup)}};
    }

    inline std::vector<HubEvent> setHubTimeOfDay(HubState& s, HubMode m)
    {
        if (s.mode == m) return {};
        s.mode = m;
        return {{HubEventKind::TimeOfDayChanged, s.overlay, ""}};
    }

    inline std::vector<HubEvent> updateHubScreenOneFrame(
        HubState& state,
        const HubInput& input)
    {
        std::vector<HubEvent> events;
        if (state.overlay != HubOverlay::None)
        {
            if (input.cancelTapped)
            {
                const auto closed = closeHubOverlay(state);
                events.insert(events.end(), closed.begin(), closed.end());
            }
            else if (input.acceptTapped)
            {
                switch (state.overlay)
                {
                case HubOverlay::StageGate:
                    events.push_back({HubEventKind::StageEntryConfirmed,
                                      HubOverlay::StageGate,
                                      std::string(kHubSfxConfirm)});
                    state.overlay = HubOverlay::None;
                    break;
                case HubOverlay::ShopMenu:
                    events.push_back({HubEventKind::ShopPurchaseConfirmed,
                                      HubOverlay::ShopMenu,
                                      std::string(kHubSfxConfirm)});
                    break;
                case HubOverlay::MissionBrief:
                    events.push_back({HubEventKind::MissionAccepted,
                                      HubOverlay::MissionBrief,
                                      std::string(kHubSfxConfirm)});
                    state.overlay = HubOverlay::None;
                    break;
                default:
                    break;
                }
            }
            return events;
        }
        // Free-roam in the hub: open overlays based on input.
        if (input.openMapTapped)
        {
            const auto opened = openHubOverlay(state, HubOverlay::TownMap);
            events.insert(events.end(), opened.begin(), opened.end());
        }
        else if (input.talkTapped)
        {
            const auto opened = openHubOverlay(state, HubOverlay::BalloonText);
            events.insert(events.end(), opened.begin(), opened.end());
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
