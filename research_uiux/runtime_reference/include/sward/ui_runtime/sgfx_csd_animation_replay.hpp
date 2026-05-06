// Phase 357: SGFX CSD animation replay loader.
//
// Consumes a `ui_lab_csd_setposition.jsonl` file produced by
// UnleashedRecomp's UI Lab harness (see
// local_build_env/ur103clean/UnleashedRecomp/patches/CsdNodeValue_patches.cpp
// + ui_lab_patches.cpp::EmitCsdSetterEvent for the source format)
// and replays the captured per-frame setter events as runtime
// overrides for SGFX's native CSD compositor.
//
// Why this exists: UnleashedRecomp's CSD animation interpreter
// (SetMotion + per-frame Update + per-cast SetPosition cascades)
// lives inside the auto-translated PPC retail code, NOT in
// human-readable C++. SGFX's renderer paints static frame-0
// composites because we don't reimplement that interpreter. The
// pragmatic alternative is to OBSERVE the running retail
// interpreter via the existing harvester hooks (Phase 294), persist
// its per-frame output to JSONL, and replay that JSONL in SGFX so
// the composite animates exactly as retail did at capture time.
//
// JSONL line format (one per setter event), as written by
// EmitCsdSetterEvent at ui_lab_patches.cpp:12999:
//
//   {"frame":<u32>,"time":<f64>,"kind":"position|scale|uniformScaleOrAlpha|...",
//    "node":"0xHHHHHHHH","x":<f32>[,"y":<f32>],"hits":<u32>,"hook":"...",
//    "target":"<screen-token>"}
//
// Phase 357 limitations (= what the next harvester pass needs to
// add upstream):
//
//   * Each event is keyed by `node` (a runtime memory address) +
//     `target` (the screen token like "title-menu" / "world-map").
//     We do NOT yet have (project, scene, cast) names per event;
//     a follow-up patch in CsdNodeValue_patches.cpp would resolve
//     the node back to its CCastNode metadata at hook time and
//     emit those names. Until then, replay coalesces all events
//     for a given target into a single per-target anchor stream
//     (best-effort) -- enough to drive whole-scene "did the
//     window slide in / out" animations but not per-cast joint
//     curves.
//
//   * `kind=position` (SetPosition / sub_830BB3D0) is captured per
//     CCastNode in pixel-space relative to a 1280x720 logical
//     canvas. Replay maps that into SGFX's normalized canvas
//     (anchorXPx / fb.width).
//
//   * `kind=scale` (sub_830BB650) is per-cast (X,Y).
//
//   * `kind=uniformScaleOrAlpha` (sub_830BB5F8) is one float; the
//     harvester comment notes "uniform scale stays in [0..2];
//     alpha in [0..1]". Replay treats values <= 1.05 as alpha
//     and values > 1.05 as scale. Tag: pending-Ghidra (the exact
//     setter intent isn't observable from the PPC stub alone).
//
// For Phase 358 (next pass), the harvester gains scene+cast name
// resolution and an explicit `kind=color` setter; the loader below
// is structured so that future enrichment is additive (extra
// fields on EventKey + the corresponding Sample variants).

#pragma once

#include "sgfx_hud_native_csd_renderer.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // One captured setter event from the harvester JSONL.
    // The node address is opaque (runtime memory) but stable for the
    // duration of the capture, so events with the same node form a
    // time series for the same cast.
    struct CsdReplayEvent
    {
        std::uint32_t frame = 0;
        double        timeSeconds = 0.0;
        std::string   kind;        // "position" | "scale" | "uniformScaleOrAlpha"
        std::uint32_t nodeAddress = 0;
        float         x = 0.0f;
        float         y = 0.0f;
        bool          hasY = false;
        std::string   target;      // screen token: "title-menu", "world-map", etc.
    };

    struct CsdReplayLog
    {
        std::vector<CsdReplayEvent> events;
        std::uint32_t lastFrame = 0;
        double        lastTimeSeconds = 0.0;
        std::size_t   parseErrors = 0;
    };

    namespace detail::csd_replay
    {
        // Tiny single-line JSON parser sufficient for the
        // EmitCsdSetterEvent format. Returns std::nullopt for
        // malformed lines instead of throwing.
        inline std::string extractString(std::string_view line,
                                         std::string_view key) noexcept
        {
            std::string needle;
            needle.reserve(key.size() + 4);
            needle.push_back('"');
            needle.append(key);
            needle.append("\":\"");
            const auto pos = line.find(needle);
            if (pos == std::string_view::npos) return {};
            const auto valStart = pos + needle.size();
            std::string out;
            for (auto i = valStart; i < line.size(); ++i)
            {
                const char c = line[i];
                if (c == '\\' && i + 1 < line.size())
                {
                    out.push_back(line[i + 1]);
                    ++i;
                    continue;
                }
                if (c == '"') break;
                out.push_back(c);
            }
            return out;
        }

        inline std::optional<double> extractNumber(std::string_view line,
                                                   std::string_view key) noexcept
        {
            std::string needle;
            needle.reserve(key.size() + 3);
            needle.push_back('"');
            needle.append(key);
            needle.append("\":");
            const auto pos = line.find(needle);
            if (pos == std::string_view::npos) return std::nullopt;
            const auto valStart = pos + needle.size();
            // Read until comma / closing brace / whitespace.
            std::string num;
            for (auto i = valStart; i < line.size(); ++i)
            {
                const char c = line[i];
                if (c == ',' || c == '}' || c == ' ' || c == '\n' || c == '\r')
                    break;
                num.push_back(c);
            }
            if (num.empty()) return std::nullopt;
            try
            {
                return std::stod(num);
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        inline std::optional<std::uint32_t> parseHexNode(std::string_view s) noexcept
        {
            if (s.size() < 3 || s[0] != '0' || (s[1] != 'x' && s[1] != 'X'))
                return std::nullopt;
            std::uint32_t v = 0;
            for (std::size_t i = 2; i < s.size(); ++i)
            {
                const char c = s[i];
                std::uint32_t d = 0;
                if (c >= '0' && c <= '9') d = static_cast<std::uint32_t>(c - '0');
                else if (c >= 'a' && c <= 'f') d = 10 + static_cast<std::uint32_t>(c - 'a');
                else if (c >= 'A' && c <= 'F') d = 10 + static_cast<std::uint32_t>(c - 'A');
                else return std::nullopt;
                v = (v << 4) | d;
            }
            return v;
        }

        inline std::optional<CsdReplayEvent> parseLine(std::string_view line) noexcept
        {
            if (line.empty() || line.front() != '{') return std::nullopt;
            CsdReplayEvent ev;
            const auto frame = extractNumber(line, "frame");
            const auto time  = extractNumber(line, "time");
            if (!frame || !time) return std::nullopt;
            ev.frame = static_cast<std::uint32_t>(*frame);
            ev.timeSeconds = *time;
            ev.kind = extractString(line, "kind");
            if (ev.kind.empty()) return std::nullopt;
            const auto nodeStr = extractString(line, "node");
            if (auto n = parseHexNode(nodeStr)) ev.nodeAddress = *n;
            const auto x = extractNumber(line, "x");
            if (!x) return std::nullopt;
            ev.x = static_cast<float>(*x);
            if (auto y = extractNumber(line, "y"))
            {
                ev.y = static_cast<float>(*y);
                ev.hasY = true;
            }
            ev.target = extractString(line, "target");
            return ev;
        }
    } // namespace detail::csd_replay

    // Load the entire JSONL into memory. The file is expected to be
    // bounded by a single capture session (typically a few MB). Lines
    // that fail to parse increment parseErrors but do not abort.
    inline CsdReplayLog loadCsdReplayLog(const std::filesystem::path& path) noexcept
    {
        CsdReplayLog log;
        std::ifstream file(path);
        if (!file) return log;
        std::string line;
        while (std::getline(file, line))
        {
            // Strip trailing CR if file was written on Windows but
            // read in binary stream.
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            auto ev = detail::csd_replay::parseLine(line);
            if (!ev)
            {
                ++log.parseErrors;
                continue;
            }
            log.lastFrame = std::max(log.lastFrame, ev->frame);
            log.lastTimeSeconds = std::max(log.lastTimeSeconds, ev->timeSeconds);
            log.events.push_back(std::move(*ev));
        }
        // Sort by frame so range queries are O(log n) via lower_bound.
        std::sort(log.events.begin(), log.events.end(),
                  [](const CsdReplayEvent& a, const CsdReplayEvent& b) noexcept
                  {
                      if (a.frame != b.frame) return a.frame < b.frame;
                      return a.timeSeconds < b.timeSeconds;
                  });
        return log;
    }

    // Build a runtime override list from the replay log at a given
    // playback time. Coalesces by (target, kind=position) -- the
    // latest "position" event for each unique target/node pair at or
    // before the query time becomes that scene's anchor override.
    //
    // Without scene/cast name enrichment in the harvester (Phase 358
    // todo), we can't yet map node addresses to specific scenes by
    // name. As a pragmatic stand-in, this function honors the
    // `targetFilter` argument: pass the active screen's target token
    // (e.g. "title-menu") and replay only events from that target's
    // capture window. Pass an empty string to replay every event.
    //
    // Output is a list of (sceneName, anchorPx, scale) ready for
    // the renderer's runtimeOverrides parameter. Until enrichment
    // lands, sceneName is the synthesized string "node_<HEX>" so
    // overrides only apply when the renderer is also configured to
    // index casts by node address (a future renderer pass).
    inline std::vector<CsdNativeRuntimeOverride>
    buildRuntimeOverridesAt(const CsdReplayLog& log,
                            double queryTimeSeconds,
                            std::string_view targetFilter = {}) noexcept
    {
        // Latest position + scale per node address.
        struct Acc
        {
            float anchorXPx = 0.0f;
            float anchorYPx = 0.0f;
            bool  hasAnchor = false;
            float scaleX = 1.0f;
            float scaleY = 1.0f;
            bool  hasScale = false;
            std::string target;
        };
        // Walk in chronological order; later events overwrite earlier.
        std::vector<std::pair<std::uint32_t, Acc>> nodes;
        auto find = [&](std::uint32_t addr) -> Acc&
        {
            for (auto& kv : nodes)
                if (kv.first == addr) return kv.second;
            nodes.emplace_back(addr, Acc{});
            return nodes.back().second;
        };
        for (const auto& ev : log.events)
        {
            if (ev.timeSeconds > queryTimeSeconds) break;
            if (!targetFilter.empty() && ev.target != targetFilter) continue;
            auto& a = find(ev.nodeAddress);
            a.target = ev.target;
            if (ev.kind == "position")
            {
                a.anchorXPx = ev.x;
                a.anchorYPx = ev.hasY ? ev.y : 0.0f;
                a.hasAnchor = true;
            }
            else if (ev.kind == "scale")
            {
                a.scaleX = ev.x;
                a.scaleY = ev.hasY ? ev.y : ev.x;
                a.hasScale = true;
            }
            else if (ev.kind == "uniformScaleOrAlpha")
            {
                if (ev.x > 1.05f)
                {
                    a.scaleX = ev.x;
                    a.scaleY = ev.x;
                    a.hasScale = true;
                }
                // alpha is dropped here -- a future renderer pass
                // wires alpha into a per-cast tint channel.
            }
        }
        std::vector<CsdNativeRuntimeOverride> out;
        out.reserve(nodes.size());
        for (const auto& kv : nodes)
        {
            const auto& a = kv.second;
            if (!a.hasAnchor && !a.hasScale) continue;
            CsdNativeRuntimeOverride ov;
            ov.sceneName = "node_" + [&]
            {
                char buf[16];
                std::snprintf(buf, sizeof(buf), "0x%08X", kv.first);
                return std::string(buf);
            }();
            ov.anchorXPx = a.anchorXPx;
            ov.anchorYPx = a.anchorYPx;
            ov.scaleX = a.scaleX;
            ov.scaleY = a.scaleY;
            out.push_back(std::move(ov));
        }
        return out;
    }

    // Inspection helpers.
    inline std::size_t csdReplayEventCount(const CsdReplayLog& log) noexcept
    {
        return log.events.size();
    }

    inline std::size_t csdReplayCountForTarget(const CsdReplayLog& log,
                                               std::string_view target) noexcept
    {
        std::size_t n = 0;
        for (const auto& ev : log.events)
            if (ev.target == target) ++n;
        return n;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
