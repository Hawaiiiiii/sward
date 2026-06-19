// =============================================================================
// fleet.cpp — scans the 3D-car models repo for per-car readiness (see fleet.h).
// A car directory counts when it has at least one perspectives_*.json (QA cameras)
// or an export/exported.ramses; dirs that start with "_" (shared/test) and non-car
// folders fall away. Read-only. Path: viewer3d.json "bmw_git_root" (the same root the
// 3D viewer resolves scenes under).
// =============================================================================
#include "fleet.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <algorithm>

namespace fs = std::filesystem;
using nlohmann::json;

namespace fleet {
namespace {

std::string LoadRoot() {
    for (const char* p : { "viewer3d.json", "data/viewer3d.json" }) {
        std::ifstream f(p);
        if (!f) continue;
        try { json j; f >> j; return j.value("bmw_git_root", std::string()); }
        catch (const std::exception&) { return ""; }
    }
    return "";
}

int CountPng(const fs::path& dir) {
    std::error_code ec; int n = 0;
    if (!fs::is_directory(dir, ec)) return 0;
    for (const auto& f : fs::directory_iterator(dir, ec)) {
        std::string ext = f.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return (char)std::tolower(c); });
        if (ext == ".png") ++n;
    }
    return n;
}

// distinct tests that differ from baseline: diffs are split per channel
// (<test>_alpha.png + <test>_color.png), so collapse the channel suffix.
int CountUniqueDiffs(const fs::path& dir) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return 0;
    std::vector<std::string> stems;
    for (const auto& f : fs::directory_iterator(dir, ec)) {
        if (f.path().extension() != ".png") continue;
        std::string s = f.path().stem().string();
        for (const char* suf : { "_alpha", "_color" }) {
            const size_t n = std::char_traits<char>::length(suf);
            if (s.size() > n && s.compare(s.size() - n, n, suf) == 0) { s.erase(s.size() - n); break; }
        }
        if (std::find(stems.begin(), stems.end(), s) == stems.end()) stems.push_back(s);
    }
    return (int)stems.size();
}

} // namespace

Roster Scan() {
    Roster r;
    r.root = LoadRoot();
    r.configured = !r.root.empty();
    if (!r.configured) return r;

    std::error_code ec;
    const fs::path cars = fs::path(r.root) / "cars";
    r.rootExists = fs::is_directory(cars, ec);
    if (!r.rootExists) return r;

    for (const auto& brand : fs::directory_iterator(cars, ec)) {
        if (!brand.is_directory()) continue;
        const std::string bn = brand.path().filename().string();
        if (bn.empty() || bn[0] == '_') continue;            // _TestTracefiles, etc.
        for (const auto& car : fs::directory_iterator(brand.path(), ec)) {
            if (!car.is_directory()) continue;
            const std::string cn = car.path().filename().string();
            if (cn.empty() || cn[0] == '_') continue;        // _Shared, _Shared_IDCevo

            int persp = 0;
            for (const auto& f : fs::directory_iterator(car.path(), ec)) {
                const std::string fn = f.path().filename().string();
                if (fn.rfind("perspectives_", 0) == 0 && f.path().extension() == ".json") ++persp;
            }
            const bool exp = fs::exists(car.path() / "export" / "exported.ramses", ec);
            if (persp == 0 && !exp) continue;                // licenses/meta/noise

            Car c; c.brand = bn; c.id = cn; c.exported = exp; c.perspSets = persp;
            const fs::path tdir = car.path() / "export" / "tests";
            c.testExpected = CountPng(tdir / "expected");
            c.testDiff     = CountUniqueDiffs(tdir / "diff");
            c.testActuals  = CountPng(tdir / "actuals") > 0;
            if (c.testExpected == 0) c.testStatus = "none";
            else { ++r.tested;
                   if (c.testDiff > 0)     { c.testStatus = "diff";   ++r.testDiff; }
                   else if (c.testActuals) { c.testStatus = "pass";   ++r.testPass; }
                   else                    { c.testStatus = "notrun"; ++r.testNotrun; } }
            if (exp) ++r.exportedCount;
            if (persp > 0) ++r.withPersp;
            r.cars.push_back(std::move(c));
        }
    }

    std::sort(r.cars.begin(), r.cars.end(), [](const Car& a, const Car& b) {
        return a.brand != b.brand ? a.brand < b.brand : a.id < b.id;
    });
    for (const auto& c : r.cars)
        if (std::find(r.brands.begin(), r.brands.end(), c.brand) == r.brands.end()) r.brands.push_back(c.brand);
    return r;
}

} // namespace fleet
