#define _CRT_SECURE_NO_WARNINGS 1 // std::getenv (below) is flagged deprecated by MSVC under /WX

#include "csd_capture.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

// Defined in gpu/video.cpp — the texture bound to sampler slot 0 at draw time
// (g_textures[0]). CSD draws bind their picture's texture immediately before the
// DrawPrimitiveUP, so this is the current cast's texture when RecordDraw runs.
extern const void* SWA_CsdBoundTexture0();

namespace
{
    std::mutex g_mutex;
    std::map<const void*, std::string> g_pathStrings;          // cast/node host ptr -> path
    std::unordered_map<const void*, std::string> g_texNames;   // GuestTexture* -> name
    const void* g_currentCast = nullptr;
    const void* g_currentNode = nullptr;
    std::ofstream g_out;
    bool g_outOpen = false;

    // Read a big-endian 32-bit float (the guest vertex format).
    float ReadBeFloat(const uint8_t* p)
    {
        uint32_t u;
        std::memcpy(&u, p, 4);
        u = ((u & 0x000000FFu) << 24) | ((u & 0x0000FF00u) << 8) |
            ((u & 0x00FF0000u) >> 8) | ((u & 0xFF000000u) >> 24);
        float f;
        std::memcpy(&f, &u, 4);
        return f;
    }

    std::string JsonEscape(const std::string& s)
    {
        std::string o;
        o.reserve(s.size());
        for (char c : s)
        {
            if (c == '"' || c == '\\') o += '\\';
            o += c;
        }
        return o;
    }
}

bool CsdCapture::Enabled()
{
    static int enabled = -1;
    if (enabled < 0)
    {
        const char* v = std::getenv("SWA_CSD_CAPTURE");
        enabled = (v != nullptr && v[0] != '\0' && v[0] != '0') ? 1 : 0;
    }
    return enabled == 1;
}

void CsdCapture::SetPath(const void* key, std::string_view path)
{
    if (!Enabled()) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    g_pathStrings.emplace(key, std::string(path));
}

void CsdCapture::ErasePathRange(const void* lo, const void* hi)
{
    if (!Enabled()) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    g_pathStrings.erase(g_pathStrings.lower_bound(lo), g_pathStrings.lower_bound(hi));
}

void CsdCapture::NoteCurrentCast(const void* hostPtr) { if (Enabled()) g_currentCast = hostPtr; }
void CsdCapture::NoteCurrentNode(const void* hostPtr) { if (Enabled()) g_currentNode = hostPtr; }

void CsdCapture::RegisterTexture(const void* hostTexture, const char* name)
{
    if (!Enabled() || hostTexture == nullptr || name == nullptr) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    g_texNames[hostTexture] = name;
}

void CsdCapture::RecordDraw(const uint8_t* verts, uint32_t count, uint32_t stride, bool textured)
{
    if (!Enabled() || verts == nullptr || count == 0) return;

    float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
    for (uint32_t i = 0; i < count; i++)
    {
        const uint8_t* v = verts + i * stride;
        float x = ReadBeFloat(v);
        float y = ReadBeFloat(v + 4);
        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    std::string path;
    auto it = g_pathStrings.find(g_currentCast);
    if (it == g_pathStrings.end())
        it = g_pathStrings.find(g_currentNode);
    if (it != g_pathStrings.end())
        path = it->second;

    std::string tex;
    if (textured)
    {
        auto t = g_texNames.find(SWA_CsdBoundTexture0());
        if (t != g_texNames.end())
            tex = t->second;
    }

    if (!g_outOpen)
    {
        g_out.open("csd_capture.jsonl", std::ios::out | std::ios::trunc);
        g_outOpen = true;
    }
    // The stream can silently go bad under sustained load (observed live: the
    // log froze mid-session while the game kept running, dropping every record
    // afterwards). Recover by clearing the error state and reopening in append
    // mode rather than failing silently forever.
    if (!g_out.good())
    {
        g_out.clear();
        g_out.close();
        g_out.open("csd_capture.jsonl", std::ios::out | std::ios::app);
    }
    if (!g_out.is_open())
        return;

    g_out << "{\"path\":\"" << JsonEscape(path) << "\",\"rect\":["
          << (long)std::lround(minX) << "," << (long)std::lround(minY) << ","
          << (long)std::lround(maxX - minX) << "," << (long)std::lround(maxY - minY) << "]"
          << ",\"tex\":\"" << JsonEscape(tex) << "\",\"textured\":" << (textured ? "true" : "false")
          << "}\n";
    g_out.flush();
}
