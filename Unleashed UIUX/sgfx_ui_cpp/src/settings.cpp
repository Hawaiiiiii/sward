// =============================================================================
// settings.cpp — see settings.h.
// =============================================================================
#include "settings.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

namespace settings {
namespace {
std::map<std::string, int> g_kv;
bool g_loaded = false;
const char* INI = "sgfx_settings.ini";

void write() {
    FILE* f = fopen(INI, "w");
    if (!f) return;
    for (const auto& kv : g_kv) fprintf(f, "%s=%d\n", kv.first.c_str(), kv.second);
    fclose(f);
}
} // namespace

void Load() {
    g_loaded = true;
    FILE* f = fopen(INI, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        g_kv[line] = atoi(eq + 1);
    }
    fclose(f);
}

int GetInt(const char* key, int defaultValue) {
    if (!g_loaded) Load();
    auto it = g_kv.find(key);
    return (it == g_kv.end()) ? defaultValue : it->second;
}

void SetInt(const char* key, int value) {
    if (!g_loaded) Load();
    auto it = g_kv.find(key);
    if (it != g_kv.end() && it->second == value) return;
    g_kv[key] = value;
    write();
}

} // namespace settings
