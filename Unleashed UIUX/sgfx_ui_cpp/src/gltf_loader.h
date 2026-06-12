// =============================================================================
// gltf_loader.h — a minimal, dependency-free glTF 2.0 loader for the 3D
// viewport (the RaCo / RAMSES-Composer pipeline speaks glTF). Supports:
//   * .gltf with an external .bin buffer, and binary .glb containers;
//   * POSITION / NORMAL / TEXCOORD_0 float attributes, u8/u16/u32 indices
//     (primitives over 65535 vertices are skipped with a warning);
//   * node hierarchies (matrix or TRS) baked into the vertices at load;
//   * pbrMetallicRoughness baseColorFactor + baseColorTexture (PNG via the
//     bindless texture heap).
// Parsing uses the project's existing nlohmann/json.
// =============================================================================
#pragma once
#include <string>
#include <vector>

namespace gltf {

struct Primitive {
    int   mesh = -1;            // gfx::createMesh handle
    float baseColor[4] = { 1, 1, 1, 1 };
    int   texIndex = -1;        // bindless texture slot (-1 = untextured)
};

struct Model {
    bool ok = false;
    std::vector<Primitive> prims;
    float bboxMin[3] = { 0, 0, 0 }, bboxMax[3] = { 0, 0, 0 };
};

// Load a .gltf or .glb; texture/bin paths resolve relative to the model file.
Model Load(const std::string& path);

} // namespace gltf
