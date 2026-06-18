// =============================================================================
// sgfx_data.h — the live data bridge. The tool (Project Quality-Hero) writes a
// small status JSON; the viewer reads it once at startup and the screens draw from
// it. With no file present, Get() returns built-in representative defaults, so the
// viewer still runs standalone and looks identical to the baked-in values.
//
// Schema (sgfx_status.json):
//   { "run": {
//       "activeProfile": "G65",
//       "packs":   [ { "name": "anchors", "err": 0, "warn": 2, "info": 5 }, ... ],
//       "verdict": "NEEDS REVIEW",                       // LIKELY OK | NEEDS REVIEW | BLOCKED
//       "signals": [ { "label": "Errors", "value": "3" }, ... ],   // last row = total
//       "recommendation": "...",
//       "battery": [ { "name": "default", "verdict": "likely_ok", "diff": 0 }, ... ]
//   },
//   "hubTotals": [ { "label": "PROFILES", "value": "19" }, ... ],      // hub left panel rows
//   "profiles":  [ { "id": "G65", "verdict": "needs review" }, ... ]   // gate per-profile verdict
//   }
// Any field may be omitted; the default for that field stands.
// =============================================================================
#pragma once
#include <string>
#include <vector>

namespace sgfx {

struct Pack   { std::string name; int err = 0, warn = 0, info = 0; };
struct Signal { std::string label, value; };
struct Filter { std::string name, verdict; int diff = 0; };
struct ProfileStatus { std::string id, verdict; };   // gate: a profile's last verdict, by id

struct Run {
    std::string         activeProfile = "G65";
    std::vector<Pack>   packs;          // preflight packs: anchors/constants/carpaints/project_sanity
    std::string         verdict;        // "LIKELY OK" | "NEEDS REVIEW" | "BLOCKED"
    std::vector<Signal> signals;        // verdict-card rows (the last row is the total)
    std::string         recommendation;
    std::vector<Filter> filters;        // screenshot battery (result_ex)
};

struct Status {
    Run  run;
    std::vector<Signal>        hubTotals;      // hub left panel rows (label/value)
    std::vector<ProfileStatus> profileStatus;  // gate: per-profile last verdict (overlay by id)
    bool loaded = false;   // true if a status file was parsed (else the built-in defaults)
};

// Load the status JSON (an explicit path, else cwd `sgfx_status.json`, then
// `data/sgfx_status.json`, then the staged data path). Returns true if parsed; on
// absence or parse failure the built-in defaults stand and it returns false.
bool Load(const char* path = nullptr);
const Status& Get();

}
