// =============================================================================
// viewer3d.cpp — launches the external Ramses 3D-car viewer for a profile (see
// viewer3d.h). Reads operator-local paths from viewer3d.json beside the exe, maps
// the profile id to its exported.ramses under the car-models repo, and starts the
// viewer with ShellExecute (its DLLs sit beside it, so the working dir is its own
// folder). A QA perspective set is applied by reading the car's perspectives_*.json
// and passing the chosen view's camera as --qa-perspective-* args. No window or
// render work happens here — this only spawns the viewer.
// =============================================================================
#include "viewer3d.h"

#include <windows.h>
#include <shellapi.h>

#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <system_error>
#include <algorithm>

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
            c.extraArgs = j.value("extra_args", "--frames 360000");
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

// the car directory (.../<id>/) that holds export/ and perspectives_*.json
fs::path CarDir(const Config& c, const std::string& id) {
    std::string scene = ResolveScene(c.repoRoot, id);
    if (scene.empty()) return {};
    return fs::path(scene).parent_path().parent_path();
}

float Num(const json& o, const char* k, float def) {
    return (o.contains(k) && o[k].is_number()) ? o[k].get<float>() : def;
}

// build the --qa-perspective-* args from a set entry (entryOverride, else the
// representative "all good" entry, else the first).
std::string PerspectiveArgs(const fs::path& carDir, const std::string& set, const std::string& entryOverride) {
    std::ifstream f(carDir / ("perspectives_" + set + ".json"));
    if (!f) return "";
    json j; try { f >> j; } catch (const std::exception&) { return ""; }
    if (!j.is_object() || j.empty()) return "";

    std::string id = (!entryOverride.empty() && j.contains(entryOverride)) ? entryOverride
                   : j.contains("CID_CARHUB_ALL_GOOD") ? "CID_CARHUB_ALL_GOOD"
                   : j.begin().key();
    const json& e = j[id];
    std::ostringstream a;
    a << " --qa-perspective-name " << id;
    if (e.contains("CraneGimbal")) {
        const json& g = e["CraneGimbal"];
        a << " --qa-perspective-distance " << Num(g, "Distance", 10.0f)
          << " --qa-perspective-yaw "      << Num(g, "Yaw", 0.0f)
          << " --qa-perspective-pitch "    << Num(g, "Pitch", 0.0f)
          << " --qa-perspective-roll "     << Num(g, "Roll", 0.0f);
    }
    if (e.contains("Frustum")) {
        const json& fr = e["Frustum"];
        a << " --qa-perspective-horizontal-fov " << Num(fr, "HorizontalFOV", 35.0f)
          << " --qa-perspective-aspect-ratio "   << Num(fr, "AspectRatio", 1.0f)
          << " --qa-perspective-near-plane "     << Num(fr, "NearPlane", 0.5f)
          << " --qa-perspective-far-plane "      << Num(fr, "FarPlane", 100.0f);
    }
    if (e.contains("Scale") && e["Scale"].is_number())
        a << " --qa-perspective-scale " << e["Scale"].get<float>();
    if (e.contains("Origin") && e["Origin"].is_array() && e["Origin"].size() == 3)
        a << " --qa-perspective-origin-x " << e["Origin"][0].get<float>()
          << " --qa-perspective-origin-y " << e["Origin"][1].get<float>()
          << " --qa-perspective-origin-z " << e["Origin"][2].get<float>();
    a << " --qa-perspective-aspect-from-resolution "
      << (e.value("AspectFromResolution_isEnabled", true) ? "true" : "false");
    return a.str();
}

bool Spawn(const std::string& exe, const std::string& args) {
    std::string workdir = fs::path(exe).parent_path().string();
    HINSTANCE r = ShellExecuteA(nullptr, "open", exe.c_str(), args.c_str(), workdir.c_str(), SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(r) > 32;
}

} // namespace

bool Available() {
    Config c = LoadConfig();
    if (!c.loaded) return false;
    std::error_code ec;
    return fs::exists(c.viewerExe, ec);
}

std::vector<std::string> ListPerspectiveSets(const std::string& profileId) {
    std::vector<std::string> sets;
    Config c = LoadConfig();
    if (!c.loaded) return sets;
    fs::path carDir = CarDir(c, profileId);
    if (carDir.empty()) return sets;
    std::error_code ec;
    for (const auto& de : fs::directory_iterator(carDir, ec)) {
        if (de.path().extension() != ".json") continue;
        std::string fn = de.path().filename().string();
        const std::string pre = "perspectives_";
        if (fn.rfind(pre, 0) != 0) continue;
        sets.push_back(fn.substr(pre.size(), fn.size() - pre.size() - 5));   // strip prefix + ".json"
    }
    std::sort(sets.begin(), sets.end());
    return sets;
}

std::vector<std::string> ListPerspectiveEntries(const std::string& profileId, const std::string& set) {
    std::vector<std::string> entries;
    Config c = LoadConfig();
    if (!c.loaded) return entries;
    fs::path carDir = CarDir(c, profileId);
    if (carDir.empty()) return entries;
    std::ifstream f(carDir / ("perspectives_" + set + ".json"));
    if (!f) return entries;
    json j; try { f >> j; } catch (const std::exception&) { return entries; }
    if (!j.is_object()) return entries;
    for (auto it = j.begin(); it != j.end(); ++it) entries.push_back(it.key());
    return entries;
}

std::string Launch(const std::string& profileId, const std::string& view, const std::string& entry) {
    Config c = LoadConfig();
    if (!c.loaded) return "3D viewer not configured (see viewer3d.json)";
    std::error_code ec;
    if (!fs::exists(c.viewerExe, ec)) return "3D viewer not found at the configured path";

    std::string scene = ResolveScene(c.repoRoot, profileId);
    if (scene.empty()) return "No 3D export found for " + profileId;

    std::string args = "--scene-file \"" + scene + "\" " + c.extraArgs;
    std::string label = profileId;
    if (view == "orbit") { args += " --orbit"; label += " (orbit)"; }
    else if (!view.empty() && view != "authored") {
        std::string pa = PerspectiveArgs(CarDir(c, profileId), view, entry);
        if (pa.empty()) return "Could not read perspective " + view;
        args += pa;
        label += " (" + view + (entry.empty() ? "" : " / " + entry) + ")";
    }

    if (!Spawn(c.viewerExe, args)) return "Could not start the 3D viewer";
    return "Launching 3D preview - " + label;
}

std::string SnapshotPath() {
    std::error_code ec;
    return fs::absolute("carview.png", ec).string();
}

std::string RenderSnapshot(const std::string& profileId, const std::string& view, const std::string& entry) {
    Config c = LoadConfig();
    if (!c.loaded) return "3D viewer not configured (see viewer3d.json)";
    std::error_code ec;
    if (!fs::exists(c.viewerExe, ec)) return "3D viewer not found at the configured path";
    std::string scene = ResolveScene(c.repoRoot, profileId);
    if (scene.empty()) return "No 3D export found for " + profileId;

    std::string out = SnapshotPath();
    std::string args = "--scene-file \"" + scene + "\" --readback --screenshot \"" + out
                     + "\" --frames 80 --width 1280 --height 720";
    if (!view.empty() && view != "authored" && view != "orbit") {
        std::string pa = PerspectiveArgs(CarDir(c, profileId), view, entry);
        if (!pa.empty()) args += pa;
    }
    // default ("" / "authored"): the export's authored QA camera (the documented framing)
    if (!Spawn(c.viewerExe, args)) return "Could not start the 3D render";
    return "Rendering " + profileId + " ...";
}

} // namespace viewer3d
