// =============================================================================
// changelog.cpp — reads each car's CHANGELOG.md for its latest delivery (see
// changelog.h). Changelogs are "Keep a Changelog" style: version sections head with
// `## [<version>] - <date>` and list `* ` bullets under ### Fixed/Changed/Added. This
// parses the first (newest) section per car. Read-only. Path: viewer3d.json
// "bmw_git_root" (the same root the 3D viewer renders from).
// =============================================================================
#include "changelog.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <algorithm>

namespace fs = std::filesystem;
using nlohmann::json;

namespace changelog {
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

std::string Trim(std::string s) {
    const char* ws = " \t\r\n";
    s.erase(0, s.find_first_not_of(ws));
    const size_t e = s.find_last_not_of(ws);
    if (e != std::string::npos) s.erase(e + 1);
    return s;
}

// parse the first `## [version] - date` section: version, date, bullet count
void ParseLatest(const fs::path& md, Entry& e) {
    std::ifstream f(md);
    if (!f) return;
    std::string line;
    bool inFirst = false;
    while (std::getline(f, line)) {
        const std::string s = Trim(line);
        if (s.rfind("## ", 0) == 0) {
            if (inFirst) break;            // reached the next version -> done
            const size_t lb = s.find('['), rb = s.find(']');
            if (lb != std::string::npos && rb != std::string::npos && rb > lb)
                e.version = s.substr(lb + 1, rb - lb - 1);
            const size_t dash = s.find(" - ", rb == std::string::npos ? 0 : rb);
            if (dash != std::string::npos) e.date = Trim(s.substr(dash + 3));
            inFirst = true;
            continue;
        }
        if (inFirst && s.rfind("* ", 0) == 0) ++e.changes;
    }
}

} // namespace

Log Scan() {
    Log log;
    log.root = LoadRoot();
    log.configured = !log.root.empty();
    if (!log.configured) return log;

    std::error_code ec;
    const fs::path cars = fs::path(log.root) / "cars";
    log.rootExists = fs::is_directory(cars, ec);
    if (!log.rootExists) return log;

    for (const auto& brand : fs::directory_iterator(cars, ec)) {
        if (!brand.is_directory()) continue;
        const std::string bn = brand.path().filename().string();
        if (bn.empty() || bn[0] == '_') continue;
        for (const auto& car : fs::directory_iterator(brand.path(), ec)) {
            if (!car.is_directory()) continue;
            const std::string cn = car.path().filename().string();
            if (cn.empty() || cn[0] == '_') continue;
            const fs::path md = car.path() / "CHANGELOG.md";
            if (!fs::is_regular_file(md, ec)) continue;
            Entry e; e.brand = bn; e.id = cn;
            ParseLatest(md, e);
            log.entries.push_back(std::move(e));
        }
    }
    std::sort(log.entries.begin(), log.entries.end(), [](const Entry& a, const Entry& b) {
        if (a.date != b.date) return a.date > b.date;   // newest delivery first
        return a.brand != b.brand ? a.brand < b.brand : a.id < b.id;
    });
    return log;
}

} // namespace changelog
