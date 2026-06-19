// =============================================================================
// al_coverage.cpp — scans the Ambient Layer assets repo for screenshot-test state
// (see al_coverage.h). Per the AL test workflow, each exported scene keeps
// tests/{expected,actuals,diff}: 'expected' is the baseline, 'actuals' the latest run,
// and a non-empty 'diff' means a visual regression. This reports that state per
// brand/screen — no tests are run here. Path: viewer3d.json "al_assets_root".
// =============================================================================
#include "al_coverage.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;
using nlohmann::json;

namespace al {
namespace {

std::string LoadRoot() {
    for (const char* p : { "viewer3d.json", "data/viewer3d.json" }) {
        std::ifstream f(p);
        if (!f) continue;
        try { json j; f >> j; return j.value("al_assets_root", std::string()); }
        catch (const std::exception&) { return ""; }
    }
    return "";
}

int CountPng(const fs::path& dir) {
    std::error_code ec; int n = 0;
    if (!fs::is_directory(dir, ec)) return 0;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        std::string ext = e.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        if (ext == ".png") ++n;
    }
    return n;
}

std::string ScreenOf(const std::string& dirname) {   // "export_CID" -> "CID"
    const std::string pre = "export_";
    return (dirname.rfind(pre, 0) == 0) ? dirname.substr(pre.size()) : dirname;
}

} // namespace

Coverage Scan() {
    Coverage cov;
    cov.root = LoadRoot();
    cov.configured = !cov.root.empty();
    if (!cov.configured) return cov;

    std::error_code ec;
    const fs::path assets = fs::path(cov.root) / "assets";
    cov.rootExists = fs::is_directory(assets, ec);
    if (!cov.rootExists) return cov;

    // assets/<group>/<brand>/export_<screen>/tests/{expected,actuals,diff}
    for (const auto& grp : fs::directory_iterator(assets, ec)) {
        if (!grp.is_directory()) continue;
        for (const auto& brand : fs::directory_iterator(grp.path(), ec)) {
            if (!brand.is_directory()) continue;
            for (const auto& exp : fs::directory_iterator(brand.path(), ec)) {
                if (!exp.is_directory()) continue;
                const std::string dn = exp.path().filename().string();
                if (dn.rfind("export_", 0) != 0) continue;
                const fs::path tests = exp.path() / "tests";
                if (!fs::is_directory(tests, ec)) continue;

                Cell c;
                c.brand    = brand.path().filename().string();
                c.screen   = ScreenOf(dn);
                c.expected = CountPng(tests / "expected");
                c.diffs    = CountPng(tests / "diff");
                c.actuals  = CountPng(tests / "actuals") > 0;
                if      (c.expected == 0) { c.status = "nobaseline"; ++cov.nobaseline; }
                else if (c.diffs > 0)     { c.status = "diff";       ++cov.diff; }
                else if (c.actuals)       { c.status = "pass";       ++cov.pass; }
                else                      { c.status = "notrun";     ++cov.notrun; }
                cov.cells.push_back(std::move(c));
            }
        }
    }
    std::sort(cov.cells.begin(), cov.cells.end(), [](const Cell& a, const Cell& b) {
        return a.brand != b.brand ? a.brand < b.brand : a.screen < b.screen;
    });
    return cov;
}

} // namespace al
