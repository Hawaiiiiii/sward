// =============================================================================
// sgfx_data.cpp — the live data bridge loader. Parses the status JSON into the
// Run struct; falls back to representative defaults (matching the screens' baked-in
// values) so the viewer runs unchanged with no file present.
// =============================================================================
#include "sgfx_data.h"
#include <nlohmann/json.hpp>
#include <cstdio>

using nlohmann::json;

namespace sgfx {
namespace {

Status g_status;
bool   g_inited = false;

void makeDefaults(Status& s) {
    Run& r = s.run;
    r.activeProfile = "G65";
    r.packs = { {"anchors",0,2,5}, {"constants",3,4,6}, {"carpaints",0,1,2}, {"project_sanity",0,5,5} };
    r.verdict = "NEEDS REVIEW";
    r.signals = { {"Errors","3"}, {"Warnings","12"}, {"Screenshot diffs","5"}, {"Review items","6"}, {"Total findings","44"} };
    r.recommendation = "Resolve the 3 constants errors, then re-run before delivery.";
    r.filters = {
        {"default","likely_ok",0},                {"lights_drl_front","likely_ok",0},
        {"lights_LowBeam","needs_manual_review",142}, {"lights_HighBeam","needs_manual_review",88},
        {"lights_OnlyCones","proxy_candidate_ready",0}, {"openAllDoors_","baseline_missing",0},
        {"automatic_Doors_","baseline_candidate_ready",0}, {"welcome_animation_","needs_manual_review",310},
        {"highlighting_Doors","baseline_missing",0},
    };
    s.loaded = false;
}

void ensureInit() { if (!g_inited) { makeDefaults(g_status); g_inited = true; } }

bool readFile(const char* path, std::string& out) {
    if (!path) return false;
    FILE* fp = fopen(path, "rb");
    if (!fp) return false;
    fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, 0, SEEK_SET);
    if (n <= 0) { fclose(fp); return false; }
    out.assign((size_t)n, '\0');
    size_t got = fread(&out[0], 1, (size_t)n, fp);
    fclose(fp);
    return got == (size_t)n;
}

} // namespace

const Status& Get() { ensureInit(); return g_status; }

bool Load(const char* path) {
    ensureInit();
    std::string buf;
    const char* tries[] = {
        path,
        "sgfx_status.json",
        "data/sgfx_status.json",
        "C:/swardbuild/sgfx_ui/data/sgfx_status.json",
    };
    bool got = false;
    for (const char* p : tries) if (readFile(p, buf)) { got = true; break; }
    if (!got) return false;

    json j;
    try { j = json::parse(buf); } catch (...) { fprintf(stderr, "[sgfx] status parse failed\n"); return false; }
    const json jr = j.value("run", json::object());
    Run& r = g_status.run;

    if (jr.contains("activeProfile")) r.activeProfile = jr.value("activeProfile", r.activeProfile);
    if (jr.contains("packs") && jr["packs"].is_array()) {
        r.packs.clear();
        for (const auto& jp : jr["packs"])
            r.packs.push_back({ jp.value("name", std::string()), jp.value("err",0), jp.value("warn",0), jp.value("info",0) });
    }
    if (jr.contains("verdict")) r.verdict = jr.value("verdict", r.verdict);
    if (jr.contains("signals") && jr["signals"].is_array()) {
        r.signals.clear();
        for (const auto& js : jr["signals"])
            r.signals.push_back({ js.value("label", std::string()), js.value("value", std::string()) });
    }
    if (jr.contains("recommendation")) r.recommendation = jr.value("recommendation", r.recommendation);
    if (jr.contains("battery") && jr["battery"].is_array()) {
        r.filters.clear();
        for (const auto& jf : jr["battery"])
            r.filters.push_back({ jf.value("name", std::string()), jf.value("verdict", std::string("likely_ok")), jf.value("diff",0) });
    }

    g_status.loaded = true;
    fprintf(stderr, "[sgfx] status loaded (profile %s, %zu packs, %zu filters)\n",
            r.activeProfile.c_str(), r.packs.size(), r.filters.size());
    return true;
}

} // namespace sgfx
