// =============================================================================
// gltf_loader.cpp — see gltf_loader.h.
// =============================================================================
#include "gltf_loader.h"
#include "gfx_d3d12.h"

#include <nlohmann/json.hpp>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <algorithm>

using nlohmann::json;

namespace gltf {
namespace {

struct Mat4 { float m[16]; };   // row-major, row-vector convention (pos * M)

Mat4 identity() { Mat4 r{}; for (int i = 0; i < 16; ++i) r.m[i] = (i % 5 == 0) ? 1.0f : 0.0f; return r; }
Mat4 mul(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            r.m[i * 4 + j] = a.m[i * 4 + 0] * b.m[0 * 4 + j] + a.m[i * 4 + 1] * b.m[1 * 4 + j]
                           + a.m[i * 4 + 2] * b.m[2 * 4 + j] + a.m[i * 4 + 3] * b.m[3 * 4 + j];
    return r;
}

bool readFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    out.resize((size_t)f.tellg());
    f.seekg(0);
    f.read((char*)out.data(), (std::streamsize)out.size());
    return true;
}

std::string dirOf(const std::string& p) {
    size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? std::string() : p.substr(0, s + 1);
}

struct Ctx {
    json j;
    std::vector<std::vector<uint8_t>> buffers;
    std::string dir;
    Model* model = nullptr;
};

// raw pointer + stride for an accessor's data (nullptr if unsupported)
const uint8_t* accessorData(const Ctx& c, int accIdx, int& count, int& compType, int& comps, int& stride) {
    const auto& acc = c.j["accessors"][accIdx];
    count = acc.value("count", 0);
    compType = acc.value("componentType", 0);
    const std::string type = acc.value("type", "SCALAR");
    comps = (type == "SCALAR") ? 1 : (type == "VEC2") ? 2 : (type == "VEC3") ? 3 : (type == "VEC4") ? 4 : 0;
    if (!comps || !acc.contains("bufferView")) return nullptr;
    const auto& bv = c.j["bufferViews"][acc["bufferView"].get<int>()];
    int buf = bv.value("buffer", 0);
    if (buf >= (int)c.buffers.size()) return nullptr;
    size_t off = (size_t)bv.value("byteOffset", 0) + (size_t)acc.value("byteOffset", 0);
    int compSize = (compType == 5126 || compType == 5125) ? 4 : (compType == 5123 || compType == 5122) ? 2 : 1;
    stride = bv.value("byteStride", 0);
    if (stride == 0) stride = compSize * comps;
    if (off >= c.buffers[buf].size()) return nullptr;
    return c.buffers[buf].data() + off;
}

void loadPrimitive(Ctx& c, const json& prim, const Mat4& world) {
    if (!prim.contains("attributes") || !prim["attributes"].contains("POSITION")) return;
    int nPos = 0, ct = 0, comps = 0, stP = 0;
    const uint8_t* pos = accessorData(c, prim["attributes"]["POSITION"].get<int>(), nPos, ct, comps, stP);
    if (!pos || ct != 5126 || comps != 3 || nPos <= 0) return;
    if (nPos > 65535) { fprintf(stderr, "[gltf] primitive >65535 verts skipped\n"); return; }

    const uint8_t* nrm = nullptr; int stN = 0;
    if (prim["attributes"].contains("NORMAL")) {
        int n2, ct2, co2;
        nrm = accessorData(c, prim["attributes"]["NORMAL"].get<int>(), n2, ct2, co2, stN);
        if (ct2 != 5126 || co2 != 3 || n2 < nPos) nrm = nullptr;
    }
    const uint8_t* uv = nullptr; int stT = 0;
    if (prim["attributes"].contains("TEXCOORD_0")) {
        int n2, ct2, co2;
        uv = accessorData(c, prim["attributes"]["TEXCOORD_0"].get<int>(), n2, ct2, co2, stT);
        if (ct2 != 5126 || co2 != 2 || n2 < nPos) uv = nullptr;
    }

    std::vector<gfx::MeshVertex> verts((size_t)nPos);
    for (int i = 0; i < nPos; ++i) {
        const float* p = (const float*)(pos + (size_t)i * stP);
        float x = p[0], y = p[1], z = p[2];
        gfx::MeshVertex& v = verts[i];
        v.px = x * world.m[0] + y * world.m[4] + z * world.m[8] + world.m[12];
        v.py = x * world.m[1] + y * world.m[5] + z * world.m[9] + world.m[13];
        v.pz = x * world.m[2] + y * world.m[6] + z * world.m[10] + world.m[14];
        if (nrm) {
            const float* n = (const float*)(nrm + (size_t)i * stN);
            v.nx = n[0] * world.m[0] + n[1] * world.m[4] + n[2] * world.m[8];
            v.ny = n[0] * world.m[1] + n[1] * world.m[5] + n[2] * world.m[9];
            v.nz = n[0] * world.m[2] + n[1] * world.m[6] + n[2] * world.m[10];
            float l = std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
            if (l > 1e-6f) { v.nx /= l; v.ny /= l; v.nz /= l; }
        } else { v.nx = 0; v.ny = 1; v.nz = 0; }
        if (uv) {
            const float* t = (const float*)(uv + (size_t)i * stT);
            v.u = t[0]; v.v = t[1];
        } else { v.u = v.v = 0; }
        Model& M = *c.model;
        if (M.prims.empty() && i == 0) {
            for (int k = 0; k < 3; ++k) { M.bboxMin[k] = (&v.px)[k]; M.bboxMax[k] = (&v.px)[k]; }
        }
        M.bboxMin[0] = std::min(M.bboxMin[0], v.px); M.bboxMax[0] = std::max(M.bboxMax[0], v.px);
        M.bboxMin[1] = std::min(M.bboxMin[1], v.py); M.bboxMax[1] = std::max(M.bboxMax[1], v.py);
        M.bboxMin[2] = std::min(M.bboxMin[2], v.pz); M.bboxMax[2] = std::max(M.bboxMax[2], v.pz);
    }

    std::vector<uint16_t> idx;
    if (prim.contains("indices")) {
        int nIdx, ctI, coI, stI;
        const uint8_t* id = accessorData(c, prim["indices"].get<int>(), nIdx, ctI, coI, stI);
        if (!id || nIdx <= 0) return;
        idx.resize((size_t)nIdx);
        for (int i = 0; i < nIdx; ++i) {
            uint32_t v = (ctI == 5125) ? *(const uint32_t*)(id + (size_t)i * stI)
                       : (ctI == 5123) ? *(const uint16_t*)(id + (size_t)i * stI)
                                       : *(const uint8_t*)(id + (size_t)i * stI);
            if (v > 65535) { fprintf(stderr, "[gltf] index out of u16 range\n"); return; }
            idx[i] = (uint16_t)v;
        }
    } else {
        idx.resize((size_t)nPos);
        for (int i = 0; i < nPos; ++i) idx[i] = (uint16_t)i;
    }

    Primitive out;
    out.mesh = gfx::createMesh(verts.data(), (int)verts.size(), idx.data(), (int)idx.size());
    if (out.mesh < 0) return;

    if (prim.contains("material")) {
        const auto& mat = c.j["materials"][prim["material"].get<int>()];
        if (mat.contains("pbrMetallicRoughness")) {
            const auto& pbr = mat["pbrMetallicRoughness"];
            if (pbr.contains("baseColorFactor")) {
                const auto& f = pbr["baseColorFactor"];
                for (int k = 0; k < 4 && k < (int)f.size(); ++k) out.baseColor[k] = f[k].get<float>();
            }
            if (pbr.contains("baseColorTexture")) {
                int texI = pbr["baseColorTexture"].value("index", -1);
                if (texI >= 0 && c.j.contains("textures")) {
                    int src = c.j["textures"][texI].value("source", -1);
                    if (src >= 0 && c.j.contains("images")) {
                        std::string uri = c.j["images"][src].value("uri", "");
                        if (!uri.empty() && uri.rfind("data:", 0) != 0)
                            out.texIndex = gfx::loadTexture(c.dir + uri);
                    }
                }
            }
        }
    }
    c.model->prims.push_back(out);
}

void loadNode(Ctx& c, int nodeIdx, const Mat4& parent) {
    const auto& node = c.j["nodes"][nodeIdx];
    Mat4 local = identity();
    if (node.contains("matrix")) {
        // glTF matrices are COLUMN-major; transpose into our row-vector layout
        const auto& m = node["matrix"];
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                local.m[col * 4 + row] = m[col * 4 + row].get<float>();
        // (column-major storage read sequentially lands exactly in row-vector form)
    } else {
        float t[3] = { 0, 0, 0 }, s[3] = { 1, 1, 1 }, q[4] = { 0, 0, 0, 1 };
        if (node.contains("translation")) for (int k = 0; k < 3; ++k) t[k] = node["translation"][k].get<float>();
        if (node.contains("scale"))       for (int k = 0; k < 3; ++k) s[k] = node["scale"][k].get<float>();
        if (node.contains("rotation"))    for (int k = 0; k < 4; ++k) q[k] = node["rotation"][k].get<float>();
        // compose S * R * T in row-vector convention
        const float x = q[0], y = q[1], z = q[2], w = q[3];
        Mat4 R = identity();
        R.m[0] = 1 - 2 * (y * y + z * z); R.m[1] = 2 * (x * y + z * w);     R.m[2] = 2 * (x * z - y * w);
        R.m[4] = 2 * (x * y - z * w);     R.m[5] = 1 - 2 * (x * x + z * z); R.m[6] = 2 * (y * z + x * w);
        R.m[8] = 2 * (x * z + y * w);     R.m[9] = 2 * (y * z - x * w);     R.m[10] = 1 - 2 * (x * x + y * y);
        Mat4 S = identity(); S.m[0] = s[0]; S.m[5] = s[1]; S.m[10] = s[2];
        Mat4 T = identity(); T.m[12] = t[0]; T.m[13] = t[1]; T.m[14] = t[2];
        local = mul(mul(S, R), T);
    }
    Mat4 world = mul(local, parent);
    if (node.contains("mesh")) {
        const auto& mesh = c.j["meshes"][node["mesh"].get<int>()];
        for (const auto& prim : mesh.value("primitives", json::array()))
            loadPrimitive(c, prim, world);
    }
    for (const auto& ch : node.value("children", json::array()))
        loadNode(c, ch.get<int>(), world);
}

} // namespace

Model Load(const std::string& path) {
    Model model;
    Ctx c; c.model = &model; c.dir = dirOf(path);

    std::vector<uint8_t> file;
    if (!readFile(path, file)) { fprintf(stderr, "[gltf] cannot read '%s'\n", path.c_str()); return model; }

    if (file.size() >= 12 && memcmp(file.data(), "glTF", 4) == 0) {
        // GLB container: 12-byte header, then chunks (JSON first, then BIN)
        size_t off = 12;
        std::string jsonText;
        while (off + 8 <= file.size()) {
            uint32_t len, type;
            memcpy(&len, file.data() + off, 4);
            memcpy(&type, file.data() + off + 4, 4);
            off += 8;
            if (off + len > file.size()) break;
            if (type == 0x4E4F534A)      jsonText.assign((const char*)file.data() + off, len);   // 'JSON'
            else if (type == 0x004E4942) c.buffers.emplace_back(file.begin() + off, file.begin() + off + len); // 'BIN'
            off += len;
        }
        if (jsonText.empty()) { fprintf(stderr, "[gltf] GLB without JSON chunk\n"); return model; }
        try { c.j = json::parse(jsonText); } catch (...) { fprintf(stderr, "[gltf] GLB JSON parse failed\n"); return model; }
    } else {
        try { c.j = json::parse(file.begin(), file.end()); } catch (...) { fprintf(stderr, "[gltf] JSON parse failed\n"); return model; }
        for (const auto& buf : c.j.value("buffers", json::array())) {
            std::string uri = buf.value("uri", "");
            std::vector<uint8_t> data;
            if (!uri.empty() && uri.rfind("data:", 0) != 0 && !readFile(c.dir + uri, data))
                fprintf(stderr, "[gltf] buffer '%s' missing\n", uri.c_str());
            c.buffers.push_back(std::move(data));
        }
    }

    if (!c.j.contains("nodes") || !c.j.contains("meshes")) { fprintf(stderr, "[gltf] no nodes/meshes\n"); return model; }
    int sceneIdx = c.j.value("scene", 0);
    const auto scenes = c.j.value("scenes", json::array());
    Mat4 root = identity();
    if (sceneIdx < (int)scenes.size()) {
        for (const auto& n : scenes[sceneIdx].value("nodes", json::array()))
            loadNode(c, n.get<int>(), root);
    } else {
        for (int i = 0; i < (int)c.j["nodes"].size(); ++i) loadNode(c, i, root);
    }

    model.ok = !model.prims.empty();
    printf("[gltf] '%s': %zu primitive(s), bbox (%.2f %.2f %.2f)-(%.2f %.2f %.2f)\n",
           path.c_str(), model.prims.size(),
           model.bboxMin[0], model.bboxMin[1], model.bboxMin[2],
           model.bboxMax[0], model.bboxMax[1], model.bboxMax[2]);
    return model;
}

} // namespace gltf
