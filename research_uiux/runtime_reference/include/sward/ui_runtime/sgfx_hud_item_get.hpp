// Phase 355: SGFX-shaped port of SWA::CHudItemGet.
//
// The small in-stage popup that appears when the player picks up a
// key item -- a Sun Medal, Moon Medal, continent-map fragment, or a
// sub-collectible. Visible during a brief intro + dwell + outro
// before auto-hiding. NOT the same thing as Results' final medal
// tally; this is the per-pickup acknowledge.
//
// Retail evidence (mined from default_patched.xex via the strings
// dumper, paths recovered via the embedded source_file_paths blob):
//
//   RTTI mangled names:
//     .?AVCHudItemGet@SWA@@                       (the class)
//     .?AV?$sp_counted_impl_p@VCHudItemGet@SWA@@  (boost::shared_ptr instance)
//
//   Source path:
//     D:\SonicWorldAdventure\SWA\source\HUD\Item\HudItemGet.cpp
//
//   Inbound message types:
//     MsgRequestItemGet     -- host -> CHudItemGet : "show pickup popup
//                              for item <id>, ratio <num>/<deno>".
//     MsgRequestItemResult  -- host -> CHudItemGet : "request the
//                              cumulative pickup tally for item <id>"
//                              (used by Results screen to decorate
//                              its rank fanfare with collected medals).
//     MsgRequestItemReturn  -- CHudItemGet -> host : the response to
//                              MsgRequestItemResult, or the
//                              acknowledgement that the popup
//                              finished its outro and is hidden.
//
//   CSD project:           game/SystemCommon/ui_itemresult.yncp
//   Scene names:           contents, iresult_title, result_footer, window
//   Animation track names: Intro_so_etf_Anim, Intro_ev_etf_Anim,
//                          usual_etf_Anim, usual_Anim_2,
//                          select_so_Anim, select_ev_Anim,
//                          Outro_Anim, Outro_so_..., Outro_ev_...
//   Cast names with retail-meaningful counter roles:
//     num_m_nume / num_m_deno -- medal counter numerator / denominator
//                                ("3" / "5" rendered as "3/5")
//     num_s_nume / num_s_deno -- sub-counter (e.g. ring-shield ratio)
//     num_m_pale / num_m_shade / num_s_pale / num_shade -- color
//                                tint + drop-shadow overlays
//
// SGFX port keeps it small: state struct + input + event list. The
// state machine is a 4-phase sequence (Hidden -> Intro -> Usual ->
// Outro -> Hidden) clocked by the host's deltaSeconds. The intro
// track switches between Sonic and Werehog variants the same way
// CHudResult does (so_ vs ev_); SGFX exposes that via a "mode" flag
// the host sets when requesting the show.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Retail collectible categories the popup can display. The
    // image cast inside the `contents` scene swaps based on this;
    // the counter pair (m_nume/m_deno) shows the cumulative tally.
    enum class ItemGetKind : std::uint8_t
    {
        SunMedal      = 0, // gold day-stage medal
        MoonMedal     = 1, // silver night-stage medal
        ContinentPart = 2, // continent map fragment (story progression)
        SubItem       = 3, // sub-collectible (1up, ring shield, etc.)
        Generic       = 4, // anything else (host caption fallback)
    };

    enum class ItemGetMode : std::uint8_t
    {
        DaySonic = 0, // Intro_so_etf_Anim track
        Werehog  = 1, // Intro_ev_etf_Anim track
    };

    // Lifecycle phases. Mirrors retail CHudItemGet::TStateMachine
    // (intro animation -> dwell -> outro animation -> hidden) without
    // claiming RTTI parity (the retail TStateMachine sub-state RTTI
    // for HudItemGet wasn't recovered from the xex; only the wrapping
    // class name is). Tag: pending-Ghidra for sub-state names.
    enum class ItemGetPhase : std::uint8_t
    {
        Hidden = 0,
        Intro  = 1,
        Usual  = 2,
        Outro  = 3,
    };

    enum class ItemGetEventKind : std::uint8_t
    {
        Shown,             // Hidden -> Intro
        UsualReached,      // Intro  -> Usual
        OutroStarted,      // Usual  -> Outro
        Hidden_,           // Outro  -> Hidden (reply to host: ItemReturn)
        Suppressed,        // host called show while a popup was
                           // already animating; new show ignored
    };

    struct ItemGetState
    {
        ItemGetPhase phase = ItemGetPhase::Hidden;
        ItemGetMode  mode  = ItemGetMode::DaySonic;
        ItemGetKind  kind  = ItemGetKind::Generic;

        // Counter pair displayed by num_m_nume / num_m_deno casts.
        // Retail shows "<numerator>/<denominator>" with the m_pale
        // and m_shade casts overlaid for color + drop shadow.
        std::int32_t numerator   = 0;
        std::int32_t denominator = 0;

        // Sub-counter (num_s_nume / num_s_deno). Optional; -1 means
        // "don't render the sub-counter".
        std::int32_t subNumerator   = -1;
        std::int32_t subDenominator = -1;

        // Phase clocks. Retail track durations live in the .yncp
        // animation segments (Intro_*_etf_Anim, usual_etf_Anim, etc.)
        // SGFX uses host-tunable defaults that match the captured
        // pickup-popup duration in UnleashedRecomp's title -> stage
        // playthrough trace (~0.4 s intro, ~1.6 s dwell, ~0.4 s outro).
        // Tag: pending-Ghidra for exact retail per-track lengths.
        float secondsInPhase     = 0.0f;
        float introDurationSec   = 0.40f;
        float usualDurationSec   = 1.60f;
        float outroDurationSec   = 0.40f;
    };

    struct ItemGetInput
    {
        float deltaSeconds = 0.0f;
    };

    struct ItemGetEvent
    {
        ItemGetEventKind kind = ItemGetEventKind::Shown;
        std::string      sfxCueName;
    };

    // Phase 311 audit recovered the retail item-get cue name from
    // PlaySound call sites adjacent to the HudItemGet vtable.
    // (sys_actstg_itemget is the canonical name SGFX emits; if a
    // future PlaySound mining pass uncovers a different cue, swap
    // here without changing call sites.)
    constexpr std::string_view kItemGetSfxShow = "sys_actstg_itemget";
    constexpr std::string_view kItemGetSfxHide = ""; // silent outro

    // Helper: kick off the popup with the host's pickup payload.
    // Matches the shape of CHudItemGet::HandleMessage(MsgRequestItemGet).
    // Returns Suppressed if a popup is already animating; otherwise
    // returns Shown + the SFX cue.
    inline std::vector<ItemGetEvent> showItemGet(
        ItemGetState& s,
        ItemGetKind kind,
        ItemGetMode mode,
        std::int32_t numerator,
        std::int32_t denominator,
        std::int32_t subNumerator   = -1,
        std::int32_t subDenominator = -1) noexcept
    {
        if (s.phase != ItemGetPhase::Hidden)
        {
            return {{ItemGetEventKind::Suppressed, ""}};
        }
        s.phase = ItemGetPhase::Intro;
        s.mode  = mode;
        s.kind  = kind;
        s.numerator      = numerator;
        s.denominator    = denominator;
        s.subNumerator   = subNumerator;
        s.subDenominator = subDenominator;
        s.secondsInPhase = 0.0f;
        return {{ItemGetEventKind::Shown, std::string(kItemGetSfxShow)}};
    }

    // Per-frame tick. Drives the 4-phase clock; emits events at
    // each transition. The host walks the events to play SFX, swap
    // CSD animation tracks (Intro_*_etf_Anim -> usual_etf_Anim ->
    // Outro_Anim), and reply with MsgRequestItemReturn when Hidden_
    // fires.
    inline std::vector<ItemGetEvent> updateItemGetOneFrame(
        ItemGetState& s,
        const ItemGetInput& input) noexcept
    {
        std::vector<ItemGetEvent> events;
        if (s.phase == ItemGetPhase::Hidden) return events;

        s.secondsInPhase += input.deltaSeconds;
        switch (s.phase)
        {
        case ItemGetPhase::Intro:
            if (s.secondsInPhase >= s.introDurationSec)
            {
                s.phase = ItemGetPhase::Usual;
                s.secondsInPhase = 0.0f;
                events.push_back({ItemGetEventKind::UsualReached, ""});
            }
            break;
        case ItemGetPhase::Usual:
            if (s.secondsInPhase >= s.usualDurationSec)
            {
                s.phase = ItemGetPhase::Outro;
                s.secondsInPhase = 0.0f;
                events.push_back({ItemGetEventKind::OutroStarted, ""});
            }
            break;
        case ItemGetPhase::Outro:
            if (s.secondsInPhase >= s.outroDurationSec)
            {
                s.phase = ItemGetPhase::Hidden;
                s.secondsInPhase = 0.0f;
                events.push_back({ItemGetEventKind::Hidden_,
                                  std::string(kItemGetSfxHide)});
            }
            break;
        case ItemGetPhase::Hidden:
            break;
        }
        return events;
    }

    // Convenience predicate hosts use to decide whether to mount the
    // ui_itemresult.yncp project this frame.
    inline bool isItemGetVisible(const ItemGetState& s) noexcept
    {
        return s.phase != ItemGetPhase::Hidden;
    }

    // Returns the active animation track name for the current phase,
    // matching the retail .yncp animation segments. Hosts pass this
    // straight to their CSD scene player.
    inline std::string_view itemGetAnimationTrack(const ItemGetState& s) noexcept
    {
        switch (s.phase)
        {
        case ItemGetPhase::Intro:
            return s.mode == ItemGetMode::Werehog
                   ? "Intro_ev_etf_Anim" : "Intro_so_etf_Anim";
        case ItemGetPhase::Usual:
            return "usual_etf_Anim";
        case ItemGetPhase::Outro:
            return s.mode == ItemGetMode::Werehog
                   ? "Outro_ev_Anim" : "Outro_so_Anim";
        case ItemGetPhase::Hidden:
        default:
            return "";
        }
    }

    // CSD project + scene names retail uses, exposed as constants so
    // hosts can mount the right asset without re-mining.
    constexpr std::string_view kItemGetCsdProject = "game/SystemCommon/ui_itemresult.yncp";
    constexpr std::string_view kItemGetSceneContents = "contents";
    constexpr std::string_view kItemGetSceneTitle    = "iresult_title";
    constexpr std::string_view kItemGetSceneFooter   = "result_footer";
    constexpr std::string_view kItemGetSceneWindow   = "window";

} // namespace sward::ui_runtime::generated::sgfx_hud
