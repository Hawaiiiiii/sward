// =============================================================================
// viewer3d.cpp — launches the external Ramses 3D-car viewer for a profile (see
// viewer3d.h). Reads operator-local paths from viewer3d.json beside the exe, maps
// the profile id to its exported.ramses under the car-models repo, and starts the
// viewer with ShellExecute (its DLLs sit beside it, so the working dir is its own
// folder). No window or render work happens here — this only spawns the viewer.
// =============================================================================
#include "viewer3d.h"

#include <windows.h>
#include <shellapi.h>

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;
using nlohmann::json;

namespace viewer3d {
namespace {

struct Config { std::string viewerExe, repoRoot, extraArgs; bool loaded = false; };

Config LoadConfig() {
    Config c;
    for (const char* path : { "viewer3d.json", "data/viewer3d.json" }) {
        std::ifstream f(path);
        if (!f) continue;
        try {
            json j; f >> j;
            c.viewerExe = j.value("viewer_exe", "");
            c.repoRoot  = j.value("bmw_git_root", "");
            c.extraArgs = j.value("extra_args", "--orbit --frames 360000");
            c.loaded    = !c.viewerExe.empty();
        } catch (const std::exception&) { /* malformed config -> treated as absent */ }
        break;
    }
    return c;
}

// <repo>/cars/BMW/<id>/export/exported.ramses, trying the id and common suffixes.
std::string ResolveScene(const std::string& root, const std::string& id) {
    if (root.empty() || id.empty()) return "";
    const std::string bases[] = { id, id + "_EVO", id + "_evo" };
    std::error_code ec;
    for (const auto& b : bases) {
        fs::path p = fs::path(root) / "cars" / "BMW" / b / "export" / "exported.ramses";
        if (fs::exists(p, ec)) return p.string();
    }
    return "";
}

} // namespace

bool Available() {
    Config c = LoadConfig();
    if (!c.loaded) return false;
    std::error_code ec;
    return fs::exists(c.viewerExe, ec);
}

std::string Launch(const std::string& profileId, const std::string& perspective) {
    Config c = LoadConfig();
    if (!c.loaded) return "3D viewer not configured (see viewer3d.json)";
    std::error_code ec;
    if (!fs::exists(c.viewerExe, ec)) return "3D viewer not found at the configured path";

    std::string scene = ResolveScene(c.repoRoot, profileId);
    if (scene.empty()) return "No 3D export found for " + profileId;

    std::string args = "--scene \"" + scene + "\" " + c.extraArgs;
    if (!perspective.empty()) args += " --qa-perspective-name " + perspective;

    std::string workdir = fs::path(c.viewerExe).parent_path().string();
    HINSTANCE r = ShellExecuteA(nullptr, "open", c.viewerExe.c_str(), args.c_str(),
                                workdir.c_str(), SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(r) <= 32) return "Could not start the 3D viewer";
    return "Launching 3D preview - " + profileId;
}

} // namespace viewer3d
