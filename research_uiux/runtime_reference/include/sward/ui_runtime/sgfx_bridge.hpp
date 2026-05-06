// Phase 361: SGFX <-> sg-preflight Python bridge.
//
// Lets retail Sonic Unleashed (running under UnleashedRecomp) flow
// real BMW QA data into its UI screens by talking to sg-preflight's
// Python backend through two filesystem-watched JSON files. No
// subprocess management on either side -- the C++ host reads
// `state.json` when its mtime changes, and appends to
// `events.jsonl` on UI accept events. The Python daemon
// (`python -m sg_preflight bridge-daemon`) tail-reads
// `events.jsonl`, dispatches the events to the existing
// sg_preflight commands, and republishes `state.json` whenever
// QA state changes.
//
// Bridge directory (default: <userPath>/sgfx_bridge/):
//
//     sgfx_bridge/
//         state.json       (Python -> SU)   current QA state
//         events.jsonl     (SU -> Python)   user actions on the UI
//
// The bridge is intentionally pull-based on the C++ side (mtime
// poll once per frame) so it stays cooperative with retail
// Sonic Unleashed's main loop and adds no thread of its own.
//
// Schema versioning: every JSON message carries
// `"schema": "sgfx_bridge_state"|"sgfx_bridge_event"` and an
// integer `"version"`. Phase 361 ships version 1 of both. Future
// breakages bump the version; the C++ side falls back to a
// "no-state" shape on mismatch and logs a warning.

#pragma once

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Schema versions emitted/consumed by this bridge. Bump when
    // making backwards-incompatible field changes.
    constexpr int kSgfxBridgeStateVersion = 1;
    constexpr int kSgfxBridgeEventVersion = 1;

    // ---- State (Python -> SU) ----
    //
    // Mirrors the JSON written by sg_preflight's bridge-daemon.
    // Field set is the union of what every in-scope SU screen
    // needs to render real BMW QA data.

    enum class SgfxBridgeProfileStatus : std::uint8_t
    {
        Unknown = 0,
        Ready,         // bundle loaded, ready to validate
        Validating,    // validation in progress
        Passed,        // last run all-green
        Blocked,       // last run had blockers
        Warning,       // last run had warnings only
    };

    struct SgfxBridgeProfile
    {
        std::string id;        // e.g. "G05_C01_V07"
        std::string label;     // operator-facing label
        SgfxBridgeProfileStatus status = SgfxBridgeProfileStatus::Unknown;
    };

    struct SgfxBridgeAction
    {
        std::string id;        // e.g. "anchors_check"
        std::string label;     // e.g. "Anchors Check"
        bool        available = true;
    };

    struct SgfxBridgeValidationResult
    {
        bool        passed = false;
        std::int32_t blockers = 0;
        std::int32_t warnings = 0;
        std::string  evidencePath;  // path to evidence HTML/JSON
    };

    struct SgfxBridgeEnvironment
    {
        bool racoReady    = false;
        bool blenderReady = false;
        bool pythonReady  = false;
    };

    struct SgfxBridgeState
    {
        bool        loaded = false;
        std::string updatedAt;             // ISO-8601 from Python
        std::vector<SgfxBridgeProfile> profiles;
        std::optional<std::string> activeBundleId;
        std::string activeBundleName;
        std::string activeBundlePath;
        std::vector<SgfxBridgeAction>  actions;
        SgfxBridgeValidationResult lastValidation;
        SgfxBridgeEnvironment      environment;
    };

    // ---- Event (SU -> Python) ----

    enum class SgfxBridgeEventKind : std::uint8_t
    {
        ScreenEntered = 0,
        ScreenExited  = 1,
        MenuAccepted  = 2,   // user pressed accept on a menu row
        ProfileSelected = 3, // user picked a profile on World Map
        ActionRequested = 4, // user requested an action (validation run)
        ResultsAcknowledged = 5,
        QuitRequested = 6,
    };

    struct SgfxBridgeEvent
    {
        SgfxBridgeEventKind kind = SgfxBridgeEventKind::ScreenEntered;
        std::string screen;
        std::string rowId;       // for MenuAccepted
        std::string profileId;   // for ProfileSelected
        std::string actionId;    // for ActionRequested
        std::int64_t timeUnixMs = 0;
    };

    // ---- Bridge runtime ----
    //
    // SgfxBridge is a tiny class held by the host as a single
    // instance. Tick() once per frame; EmitEvent() from any thread
    // that handles UI input. State() returns a snapshot of the
    // most recently parsed state.json; the snapshot is stable
    // until the next Tick() call returns true (state changed).
    class SgfxBridge
    {
    public:
        // Initialise with the bridge directory. Creates the dir +
        // writes a marker file so the Python daemon can pick it up
        // even on first launch. Idempotent.
        bool Init(const std::filesystem::path& bridgeDir) noexcept;

        // Per-frame poll. Returns true when state.json was re-read
        // (mtime advanced); false otherwise. Cheap to call -- a
        // single stat() syscall per call.
        bool Tick() noexcept;

        // Append one event line to events.jsonl. Thread-safe via
        // an internal mutex; ordering is best-effort across writers.
        void EmitEvent(const SgfxBridgeEvent& ev) noexcept;

        // Snapshot of the most recently parsed state. The reference
        // is valid until the next Tick() that returns true.
        const SgfxBridgeState& State() const noexcept { return m_state; }

        // Convenience: did Init succeed AND has at least one Tick()
        // observed a state.json from Python?
        bool HasPythonHandshake() const noexcept { return m_state.loaded; }

        // Bridge directory accessor (debug / logging).
        const std::filesystem::path& Dir() const noexcept { return m_dir; }

    private:
        std::filesystem::path m_dir;
        std::filesystem::path m_statePath;
        std::filesystem::path m_eventsPath;
        std::filesystem::file_time_type m_lastStateMtime{};
        SgfxBridgeState m_state;
        std::mutex      m_eventsMutex;
        std::atomic<bool> m_initOk{false};

        bool reparseState() noexcept;
    };

    // ---- Helpers (string<->enum) ----

    inline std::string_view sgfxBridgeEventKindToken(SgfxBridgeEventKind k) noexcept
    {
        switch (k)
        {
            case SgfxBridgeEventKind::ScreenEntered:        return "screen_entered";
            case SgfxBridgeEventKind::ScreenExited:         return "screen_exited";
            case SgfxBridgeEventKind::MenuAccepted:         return "menu_accepted";
            case SgfxBridgeEventKind::ProfileSelected:      return "profile_selected";
            case SgfxBridgeEventKind::ActionRequested:      return "action_requested";
            case SgfxBridgeEventKind::ResultsAcknowledged:  return "results_acknowledged";
            case SgfxBridgeEventKind::QuitRequested:        return "quit_requested";
        }
        return "unknown";
    }

    inline SgfxBridgeProfileStatus sgfxBridgeParseProfileStatus(std::string_view s) noexcept
    {
        if (s == "ready")      return SgfxBridgeProfileStatus::Ready;
        if (s == "validating") return SgfxBridgeProfileStatus::Validating;
        if (s == "passed")     return SgfxBridgeProfileStatus::Passed;
        if (s == "blocked")    return SgfxBridgeProfileStatus::Blocked;
        if (s == "warning")    return SgfxBridgeProfileStatus::Warning;
        return SgfxBridgeProfileStatus::Unknown;
    }

    // ---- Implementation ----

    namespace detail::bridge
    {
        // Same minimal JSON helpers as the replay loader; kept
        // local so this header has no extra deps.
        inline std::string_view trim(std::string_view s) noexcept
        {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t'
                || s.front() == '\n' || s.front() == '\r'))
                s.remove_prefix(1);
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t'
                || s.back() == '\n' || s.back() == '\r'))
                s.remove_suffix(1);
            return s;
        }

        inline std::string readWholeFile(const std::filesystem::path& path) noexcept
        {
            std::ifstream file(path);
            if (!file) return {};
            std::string out;
            file.seekg(0, std::ios::end);
            out.reserve(static_cast<std::size_t>(file.tellg()));
            file.seekg(0, std::ios::beg);
            out.assign(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
            return out;
        }

        // Find the value of a top-level "key": ... entry. Returns the
        // raw value substring (without leading whitespace). Naive
        // scan -- works for the bridge's flat schema; if Python
        // emits nested structures matching keys at depth, the first
        // match wins (caller must use unique names).
        inline std::string_view findValue(std::string_view body,
                                          std::string_view key) noexcept
        {
            std::string needle;
            needle.reserve(key.size() + 4);
            needle.push_back('"');
            needle.append(key);
            needle.append("\":");
            const auto pos = body.find(needle);
            if (pos == std::string_view::npos) return {};
            auto v = body.substr(pos + needle.size());
            return trim(v);
        }

        inline std::string extractStringFromValue(std::string_view v) noexcept
        {
            if (v.empty() || v.front() != '"') return {};
            std::string out;
            for (std::size_t i = 1; i < v.size(); ++i)
            {
                const char c = v[i];
                if (c == '\\' && i + 1 < v.size())
                {
                    out.push_back(v[i + 1]);
                    ++i;
                    continue;
                }
                if (c == '"') break;
                out.push_back(c);
            }
            return out;
        }

        inline std::optional<double> extractNumberFromValue(std::string_view v) noexcept
        {
            std::string num;
            for (char c : v)
            {
                if (c == ',' || c == '}' || c == ']'
                    || c == ' ' || c == '\n' || c == '\r' || c == '\t')
                    break;
                num.push_back(c);
            }
            if (num.empty()) return std::nullopt;
            // Phase 362b: avoid std::stod (which throws) so this header
            // compiles cleanly under -fno-exceptions (UnleashedRecomp's
            // build disables exceptions on clang-cl). std::strtod has the
            // same parse rules as std::stod's underlying implementation
            // and reports failure via errno/end-pointer instead.
            const char* begin = num.c_str();
            char* end = nullptr;
            errno = 0;
            const double parsed = std::strtod(begin, &end);
            if (end == begin || errno != 0) return std::nullopt;
            return parsed;
        }

        inline bool extractBoolFromValue(std::string_view v) noexcept
        {
            return v.starts_with("true");
        }

        // Locate the matching brace/bracket at a given index.
        inline std::size_t matchBrace(std::string_view body,
                                      std::size_t openIndex,
                                      char openCh, char closeCh) noexcept
        {
            int depth = 1;
            for (std::size_t i = openIndex + 1; i < body.size(); ++i)
            {
                if (body[i] == '"')
                {
                    // skip string
                    ++i;
                    while (i < body.size() && body[i] != '"')
                    {
                        if (body[i] == '\\' && i + 1 < body.size()) ++i;
                        ++i;
                    }
                }
                else if (body[i] == openCh) ++depth;
                else if (body[i] == closeCh)
                {
                    if (--depth == 0) return i;
                }
            }
            return std::string_view::npos;
        }

        // Walk a JSON array of objects, calling `visit` on each.
        inline void forEachArrayObject(std::string_view body,
                                       std::string_view key,
                                       const std::function<void(std::string_view)>& visit)
        {
            const auto v = findValue(body, key);
            if (v.empty() || v.front() != '[') return;
            std::size_t i = 1;
            while (i < v.size())
            {
                while (i < v.size() && v[i] != '{' && v[i] != ']') ++i;
                if (i >= v.size() || v[i] == ']') break;
                const auto end = matchBrace(v, i, '{', '}');
                if (end == std::string_view::npos) break;
                visit(v.substr(i, end - i + 1));
                i = end + 1;
            }
        }
    } // namespace detail::bridge

    inline bool SgfxBridge::Init(const std::filesystem::path& bridgeDir) noexcept
    {
        std::error_code ec;
        std::filesystem::create_directories(bridgeDir, ec);
        if (ec) return false;
        m_dir = bridgeDir;
        m_statePath  = bridgeDir / "state.json";
        m_eventsPath = bridgeDir / "events.jsonl";
        // Touch the events file so the Python daemon's tail-reader
        // can attach immediately on first launch.
        if (!std::filesystem::exists(m_eventsPath, ec))
        {
            std::ofstream(m_eventsPath).flush();
        }
        m_initOk.store(true, std::memory_order_release);
        // Try one initial state read; not fatal if it isn't there
        // yet (Python may not have started the daemon).
        reparseState();
        return true;
    }

    inline bool SgfxBridge::Tick() noexcept
    {
        if (!m_initOk.load(std::memory_order_acquire)) return false;
        std::error_code ec;
        const auto mtime = std::filesystem::last_write_time(m_statePath, ec);
        if (ec) return false; // state.json missing; Python not up yet
        if (mtime == m_lastStateMtime) return false;
        m_lastStateMtime = mtime;
        return reparseState();
    }

    inline void SgfxBridge::EmitEvent(const SgfxBridgeEvent& ev) noexcept
    {
        if (!m_initOk.load(std::memory_order_acquire)) return;
        std::lock_guard<std::mutex> lk(m_eventsMutex);
        std::ofstream out(m_eventsPath, std::ios::app);
        if (!out) return;
        out << "{\"schema\":\"sgfx_bridge_event\""
            << ",\"version\":" << kSgfxBridgeEventVersion
            << ",\"event\":\"" << sgfxBridgeEventKindToken(ev.kind) << "\""
            << ",\"time_unix_ms\":" << (ev.timeUnixMs ? ev.timeUnixMs
                : std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());
        if (!ev.screen.empty())    out << ",\"screen\":\""    << ev.screen    << "\"";
        if (!ev.rowId.empty())     out << ",\"row_id\":\""    << ev.rowId     << "\"";
        if (!ev.profileId.empty()) out << ",\"profile_id\":\""<< ev.profileId << "\"";
        if (!ev.actionId.empty())  out << ",\"action_id\":\"" << ev.actionId  << "\"";
        out << "}\n";
    }

    inline bool SgfxBridge::reparseState() noexcept
    {
        const auto body = detail::bridge::readWholeFile(m_statePath);
        if (body.empty()) return false;

        const auto schema = detail::bridge::extractStringFromValue(
            detail::bridge::findValue(body, "schema"));
        if (schema != "sgfx_bridge_state") return false;
        const auto version = detail::bridge::extractNumberFromValue(
            detail::bridge::findValue(body, "version"));
        if (!version || static_cast<int>(*version) != kSgfxBridgeStateVersion)
            return false;

        SgfxBridgeState s;
        s.loaded = true;
        s.updatedAt = detail::bridge::extractStringFromValue(
            detail::bridge::findValue(body, "updated_at"));

        // Active bundle (object).
        if (auto v = detail::bridge::findValue(body, "active_bundle");
            !v.empty() && v.front() == '{')
        {
            const auto end = detail::bridge::matchBrace(v, 0, '{', '}');
            if (end != std::string_view::npos)
            {
                const auto obj = v.substr(0, end + 1);
                s.activeBundleId = detail::bridge::extractStringFromValue(
                    detail::bridge::findValue(obj, "id"));
                if (s.activeBundleId && s.activeBundleId->empty())
                    s.activeBundleId.reset();
                s.activeBundleName = detail::bridge::extractStringFromValue(
                    detail::bridge::findValue(obj, "name"));
                s.activeBundlePath = detail::bridge::extractStringFromValue(
                    detail::bridge::findValue(obj, "path"));
            }
        }

        // Profiles array.
        detail::bridge::forEachArrayObject(body, "profiles",
            [&](std::string_view obj)
            {
                SgfxBridgeProfile p;
                p.id = detail::bridge::extractStringFromValue(
                    detail::bridge::findValue(obj, "id"));
                p.label = detail::bridge::extractStringFromValue(
                    detail::bridge::findValue(obj, "label"));
                const auto status = detail::bridge::extractStringFromValue(
                    detail::bridge::findValue(obj, "status"));
                p.status = sgfxBridgeParseProfileStatus(status);
                if (!p.id.empty()) s.profiles.push_back(std::move(p));
            });

        // Actions array.
        detail::bridge::forEachArrayObject(body, "actions",
            [&](std::string_view obj)
            {
                SgfxBridgeAction a;
                a.id = detail::bridge::extractStringFromValue(
                    detail::bridge::findValue(obj, "id"));
                a.label = detail::bridge::extractStringFromValue(
                    detail::bridge::findValue(obj, "label"));
                a.available = detail::bridge::extractBoolFromValue(
                    detail::bridge::findValue(obj, "available"));
                if (!a.id.empty()) s.actions.push_back(std::move(a));
            });

        // last_validation (object).
        if (auto v = detail::bridge::findValue(body, "last_validation");
            !v.empty() && v.front() == '{')
        {
            const auto end = detail::bridge::matchBrace(v, 0, '{', '}');
            if (end != std::string_view::npos)
            {
                const auto obj = v.substr(0, end + 1);
                s.lastValidation.passed = detail::bridge::extractBoolFromValue(
                    detail::bridge::findValue(obj, "passed"));
                if (auto n = detail::bridge::extractNumberFromValue(
                        detail::bridge::findValue(obj, "blockers")))
                    s.lastValidation.blockers = static_cast<std::int32_t>(*n);
                if (auto n = detail::bridge::extractNumberFromValue(
                        detail::bridge::findValue(obj, "warnings")))
                    s.lastValidation.warnings = static_cast<std::int32_t>(*n);
                s.lastValidation.evidencePath = detail::bridge::extractStringFromValue(
                    detail::bridge::findValue(obj, "evidence_path"));
            }
        }

        // environment (object).
        if (auto v = detail::bridge::findValue(body, "environment");
            !v.empty() && v.front() == '{')
        {
            const auto end = detail::bridge::matchBrace(v, 0, '{', '}');
            if (end != std::string_view::npos)
            {
                const auto obj = v.substr(0, end + 1);
                s.environment.racoReady = detail::bridge::extractBoolFromValue(
                    detail::bridge::findValue(obj, "raco_ready"));
                s.environment.blenderReady = detail::bridge::extractBoolFromValue(
                    detail::bridge::findValue(obj, "blender_ready"));
                s.environment.pythonReady = detail::bridge::extractBoolFromValue(
                    detail::bridge::findValue(obj, "python_ready"));
            }
        }

        m_state = std::move(s);
        return true;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
