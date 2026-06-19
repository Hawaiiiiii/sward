#pragma once
#include <string>
#include <vector>

// Launches the external Ramses 3D-car viewer (sg-preflight/cpp/) for a profile, so
// the operator can open a live 3D preview of the actual car from inside the shell.
// Paths are operator-local (viewer3d.json beside the exe) — the viewer exe and the
// car-models repo root — so nothing workstation-specific is baked into the build.
namespace viewer3d {

// is a viewer configured and present? (drives whether the UI offers the action)
bool Available();

// the QA perspective sets documented for a profile (e.g. "CID180_LHD", "PHUD"),
// read from the car's perspectives_*.json. Empty if none / not configured.
std::vector<std::string> ListPerspectiveSets(const std::string& profileId);

// the named views inside one set (e.g. "CID_CARHUB_ALL_GOOD", "CID_CARHUB_LEFT_DOOR").
std::vector<std::string> ListPerspectiveEntries(const std::string& profileId, const std::string& set);

// Resolve <profile>'s exported.ramses under the configured repo and spawn the viewer.
//   view = "" / "authored" -> the export's authored camera
//   view = "orbit"         -> free orbit
//   view = <set name>      -> that QA perspective's camera; entry picks the named view
//                            within the set ("" = the set's representative entry).
// Returns a short status line for the UI — success or the reason it could not launch.
std::string Launch(const std::string& profileId, const std::string& view = "", const std::string& entry = "");

// where the in-shell car snapshot PNG is written / read (absolute, beside the shell exe).
std::string SnapshotPath();

// render ONE car frame for <profile> (view/entry like Launch) to SnapshotPath via the
// viewer's --readback --screenshot, so the shell can display it in a pane. Spawns and
// returns; the shell polls SnapshotPath for the result. Returns a short status line.
std::string RenderSnapshot(const std::string& profileId, const std::string& view = "", const std::string& entry = "");

// continuous LIVE render: a managed viewer process orbits the car and writes a frame
// repeatedly (atomically) to SnapshotPath, so the shell can poll + reload it for a live
// in-pane preview. LiveStop terminates it; LiveActive() is false once it exits.
bool LiveStart(const std::string& profileId);
void LiveStop();
bool LiveActive();


} // namespace viewer3d
