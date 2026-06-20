// =============================================================================
// variants.cpp — reads cars/model_variants.json (see variants.h). The file maps each
// model to its powertrains, and each powertrain to [ [trim names...], [flags...] ]; this
// surfaces the trim count per model/powertrain. Read-only. Path: viewer3d.json
// "bmw_git_root" (the same root the 3D viewer renders from).
// =============================================================================
#include "variants.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <algorithm>

namespace fs = std::filesystem;
using nlohmann::json;

namespace variants {
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

// canonical column order; anything else lands after these
int Rank(const std::string& pt) {
    static const char* order[] = { "ICE", "PHEV", "HEV", "FCEV", "BEV" };
    for (int i = 0; i < 5; ++i) if (pt == order[i]) return i;
    return 99;
}

} // namespace

Catalog Scan() {
    Catalog cat;
    cat.root = LoadRoot();
    cat.configured = !cat.root.empty();
    if (!cat.configured) return cat;

    const fs::path file = fs::path(cat.root) / "cars" / "model_variants.json";
    std::error_code ec;
    if (!fs::is_regular_file(file, ec)) return cat;
    cat.rootExists = true;

    std::ifstream f(file);
    json j;
    try { f >> j; } catch (const std::exception&) { return cat; }
    if (!j.is_object()) return cat;

    for (auto it = j.begin(); it != j.end(); ++it) {
        if (!it.value().is_object()) continue;
        Model m; m.name = it.key();
        for (auto pt = it.value().begin(); pt != it.value().end(); ++pt) {
            int trims = 0;
            if (pt.value().is_array() && !pt.value().empty() && pt.value()[0].is_array())
                trims = (int)pt.value()[0].size();
            m.powertrains[pt.key()] = trims;
            m.totalTrims += trims;
            cat.totalVariants += trims;
            if (std::find(cat.powertrains.begin(), cat.powertrains.end(), pt.key()) == cat.powertrains.end())
                cat.powertrains.push_back(pt.key());
        }
        cat.models.push_back(std::move(m));
    }
    std::sort(cat.powertrains.begin(), cat.powertrains.end(),
              [](const std::string& a, const std::string& b) { return Rank(a) < Rank(b); });
    return cat;
}

} // namespace variants
