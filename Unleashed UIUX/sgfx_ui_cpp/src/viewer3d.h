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

// Resolve <profile>'s exported.ramses under the configured repo and spawn the viewer.
//   view = "" / "authored" -> the export's authored camera
//   view = "orbit"         -> free orbit
//   view = <set name>      -> that QA perspective's camera (extracted from its json)
// Returns a short status line for the UI — success or the reason it could not launch.
std::string Launch(const std::string& profileId, const std::string& view = "");

} // namespace viewer3d
