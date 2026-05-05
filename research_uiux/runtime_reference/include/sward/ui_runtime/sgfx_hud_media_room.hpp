// Phase 351: SGFX-shaped port of the MediaRoom 7-class hierarchy.
//
// Phase 340 catalog recovery (xex RTTI scan):
//   D:\SonicWorldAdventure\SWA\source\HUD\MediaRoom\MediaRoom.cpp
//                                                  MediaRoomDetail.cpp
//                                                  MediaRoomItemList.cpp
//                                                  MediaRoomSelectCountry.cpp
//                                                  MediaRoomSelectItem.cpp
// RTTI symbols recovered:
//   .?AVCMediaRoom@SWA@@
//   .?AVCMediaRoomDetail@SWA@@
//   .?AVCMediaRoomScrollBar@SWA@@
//   .?AVCMediaRoomSelectBook@SWA@@
//   .?AVCMediaRoomSelectCountry@SWA@@
//   .?AVCMediaRoomSelectItem@SWA@@
//   .?AVCMediaRoomItemList@SWA@@
//   USItemInfo (struct)
//
// MediaRoom is the in-hub gallery / collection menu. It nests:
//   CMediaRoom (root) -- shell + book browser
//     CMediaRoomSelectBook -- choose a book (book = item category)
//     CMediaRoomSelectCountry -- filter by country (Apotos, Mazuri, ...)
//     CMediaRoomItemList -- scrollable item list (CMediaRoomScrollBar)
//       CMediaRoomSelectItem -- highlighted item entry
//     CMediaRoomDetail -- detail panel for the highlighted item
//
// SGFX models the navigation as a 4-phase top-level state plus
// child cursors / scroll positions.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Phase 340 USItemInfo struct mirror. SGFX simplifies to the
    // fields a host actually needs (id / name / description /
    // thumbnail-asset key).
    struct MediaRoomItemInfo
    {
        std::int32_t  itemId = -1;
        std::int32_t  countryId = 0;            // 0=Apotos, 1=Spagonia, ...
        std::int32_t  bookId = 0;               // collection category
        std::string   nameKey;                  // localization key for display name
        std::string   descriptionKey;           // localization key for description
        std::string   thumbnailAssetName;       // .dds filename
        bool          unlocked = false;
    };

    enum class MediaRoomPhase : std::uint8_t
    {
        Closed         = 0,
        SelectBook     = 1,
        SelectCountry  = 2,
        ItemList       = 3,
        Detail         = 4,
    };

    enum class MediaRoomEventKind : std::uint8_t
    {
        Opened,
        Closed,
        BookSelected,
        CountrySelected,
        ItemHovered,
        ItemDetailOpened,
        ItemDetailClosed,
        ScrollChanged,
    };

    struct MediaRoomState
    {
        MediaRoomPhase phase = MediaRoomPhase::Closed;
        std::int32_t   selectedBookId = 0;
        std::int32_t   selectedCountryId = 0;
        std::int32_t   hoveredItemIndex = 0;
        std::int32_t   itemCount = 0;
        // CMediaRoomScrollBar position 0..1.
        float          scrollPosition01 = 0.0f;
        std::int32_t   visibleRowCount = 8; // typical retail item-list height
    };

    struct MediaRoomInput
    {
        bool acceptTapped = false;
        bool cancelTapped = false;
        bool upTapped = false, downTapped = false;
        bool leftTapped = false, rightTapped = false;
    };

    struct MediaRoomEvent
    {
        MediaRoomEventKind kind = MediaRoomEventKind::Opened;
        std::int32_t       value = 0;
        std::string        sfxCueName;
    };

    constexpr std::string_view kMediaRoomSfxOpen    = "sys_actstg_pausewinopen";
    constexpr std::string_view kMediaRoomSfxClose   = "sys_actstg_pausewinclose";
    constexpr std::string_view kMediaRoomSfxConfirm = "sys_actstg_pausedecide";
    constexpr std::string_view kMediaRoomSfxCursor  = "sys_actstg_pausecursor";
    constexpr std::string_view kMediaRoomSfxBack    = "sys_actstg_pausecansel";

    inline std::vector<MediaRoomEvent> openMediaRoom(MediaRoomState& s) noexcept
    {
        s.phase = MediaRoomPhase::SelectBook;
        s.selectedBookId = 0;
        s.selectedCountryId = 0;
        s.hoveredItemIndex = 0;
        s.scrollPosition01 = 0.0f;
        return {{MediaRoomEventKind::Opened, 0, std::string(kMediaRoomSfxOpen)}};
    }

    // Walk the navigation flow. Caller is expected to populate
    // itemCount before transitioning into ItemList.
    inline std::vector<MediaRoomEvent> updateMediaRoomOneFrame(
        MediaRoomState& s, const MediaRoomInput& input)
    {
        std::vector<MediaRoomEvent> events;
        if (s.phase == MediaRoomPhase::Closed) return events;

        if (input.cancelTapped)
        {
            switch (s.phase)
            {
                case MediaRoomPhase::SelectBook:
                    s.phase = MediaRoomPhase::Closed;
                    events.push_back({MediaRoomEventKind::Closed, 0,
                                      std::string(kMediaRoomSfxClose)});
                    break;
                case MediaRoomPhase::SelectCountry:
                    s.phase = MediaRoomPhase::SelectBook;
                    events.push_back({MediaRoomEventKind::ScrollChanged, 0,
                                      std::string(kMediaRoomSfxBack)});
                    break;
                case MediaRoomPhase::ItemList:
                    s.phase = MediaRoomPhase::SelectCountry;
                    events.push_back({MediaRoomEventKind::ScrollChanged, 0,
                                      std::string(kMediaRoomSfxBack)});
                    break;
                case MediaRoomPhase::Detail:
                    s.phase = MediaRoomPhase::ItemList;
                    events.push_back({MediaRoomEventKind::ItemDetailClosed, 0,
                                      std::string(kMediaRoomSfxBack)});
                    break;
                default:
                    break;
            }
            return events;
        }

        if (input.acceptTapped)
        {
            switch (s.phase)
            {
                case MediaRoomPhase::SelectBook:
                    s.phase = MediaRoomPhase::SelectCountry;
                    events.push_back({MediaRoomEventKind::BookSelected,
                                      s.selectedBookId,
                                      std::string(kMediaRoomSfxConfirm)});
                    break;
                case MediaRoomPhase::SelectCountry:
                    s.phase = MediaRoomPhase::ItemList;
                    s.hoveredItemIndex = 0;
                    s.scrollPosition01 = 0.0f;
                    events.push_back({MediaRoomEventKind::CountrySelected,
                                      s.selectedCountryId,
                                      std::string(kMediaRoomSfxConfirm)});
                    break;
                case MediaRoomPhase::ItemList:
                    s.phase = MediaRoomPhase::Detail;
                    events.push_back({MediaRoomEventKind::ItemDetailOpened,
                                      s.hoveredItemIndex,
                                      std::string(kMediaRoomSfxConfirm)});
                    break;
                default:
                    break;
            }
            return events;
        }

        // Cursor movement per phase.
        if (input.upTapped || input.downTapped)
        {
            const std::int32_t step = input.upTapped ? -1 : +1;
            if (s.phase == MediaRoomPhase::ItemList && s.itemCount > 0)
            {
                std::int32_t next = s.hoveredItemIndex + step;
                if (next < 0)              next = s.itemCount - 1;
                if (next >= s.itemCount)   next = 0;
                if (next != s.hoveredItemIndex)
                {
                    s.hoveredItemIndex = next;
                    s.scrollPosition01 = (s.itemCount > 1)
                        ? static_cast<float>(s.hoveredItemIndex) /
                              static_cast<float>(s.itemCount - 1)
                        : 0.0f;
                    events.push_back({MediaRoomEventKind::ItemHovered, next,
                                      std::string(kMediaRoomSfxCursor)});
                }
            }
        }
        if (input.leftTapped || input.rightTapped)
        {
            const std::int32_t step = input.leftTapped ? -1 : +1;
            if (s.phase == MediaRoomPhase::SelectBook)
                s.selectedBookId += step;
            else if (s.phase == MediaRoomPhase::SelectCountry)
                s.selectedCountryId += step;
        }
        return events;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
