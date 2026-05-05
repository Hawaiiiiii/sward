// Phase 297: native C++ extractor that parses each scene's cast tree
// directly out of the .yncp / .xncp file bytes (no Python JSON in the
// loop). Ports `inspect_xncp_yncp.parse_cast_group` + `parse_cast` +
// `parse_cast_info` plus `build_yncp_native_component_map`'s
// `build_group_global_transforms` and `composed_cast_rect`. Output
// shape matches `CsdNativeDrawCommand` from the native renderer so the
// existing compositor consumes it unchanged.
//
// The extractor relies on the loader (Phase 286) having already parsed
// the project + scene-table + per-scene metadata, and uses
// `CsdSceneMetadata::sceneHeaderFileOffset` / `ncpjChunkFileOffset` /
// `bigEndian` to re-walk down to the cast tables.
//
// On-disk layout reference (matches inspect_xncp_yncp.py and is
// runtime-proven against the live UnleashedRecomp loader because the
// existing JSON-driven renderer was Phase 281 parity-validated):
//
//   Scene header (76 bytes, partially read in Phase 286):
//     +0x24 group_count                u32
//     +0x28 cast_group_table_offset    u32 (relative to ncpjOrigin)
//     +0x2C cast_count                 u32 (total across groups)
//     +0x30 cast_dictionary_offset     u32 (relative to ncpjOrigin)
//
//   Cast group (16 bytes header):
//     +0x00 cast_count                 u32
//     +0x04 cast_table_offset          u32 (rel ncpjOrigin -> u32[])
//     +0x08 root_cast_index            u32
//     +0x0C cast_hierarchy_tree_offset u32 (rel ncpjOrigin -> {i32 child, i32 next}[])
//
//   Cast (>=80 bytes, version >= 3 = SU retail = 104 bytes used):
//     +0x08 is_enabled                 u32 (skip if 0)
//     +0x0C top_left.x, +0x10 top_left.y          (2 floats)
//     +0x14 bottom_left.x, +0x18 bottom_left.y    (2 floats)
//     +0x1C top_right.x, +0x20 top_right.y        (2 floats)
//     +0x24 bottom_right.x, +0x28 bottom_right.y  (2 floats)
//     +0x30 cast_info_offset           u32 (rel ncpjOrigin)
//     +0x3C subimage_count             u32
//     +0x40 cast_material_offset       u32 (rel ncpjOrigin)
//
//   Cast info (60 bytes):
//     +0x00 hide_flag                  i32
//     +0x04 translation.x, +0x08 translation.y
//     +0x0C rotation
//     +0x10 scale.x, +0x14 scale.y
//     +0x18 subimage                   f32 (used as i32 index into cast_material)
//
//   Cast material (128 bytes): 32 x i32 subimage_indices.
//
//   Cast dictionary (12 bytes per entry):
//     +0x00 name_offset (rel ncpjOrigin -> ASCIIZ)
//     +0x04 group_index                u32
//     +0x08 cast_index                 u32

#pragma once

#include "sgfx_hud_csd_project_loader.hpp"
#include "sgfx_hud_native_csd_renderer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    struct CsdCastInfo
    {
        std::int32_t  hideFlag = 0;
        float         translationX = 0.0f, translationY = 0.0f;
        float         rotation = 0.0f;
        float         scaleX = 1.0f, scaleY = 1.0f;
        float         subimageF = 0.0f;
        bool          present = false;
    };

    struct CsdCastPoints
    {
        float topLeftX = 0.0f, topLeftY = 0.0f;
        float bottomLeftX = 0.0f, bottomLeftY = 0.0f;
        float topRightX = 0.0f, topRightY = 0.0f;
        float bottomRightX = 0.0f, bottomRightY = 0.0f;
    };

    struct CsdCast
    {
        bool          isEnabled = false;
        CsdCastPoints points;
        CsdCastInfo   info;
        std::vector<std::int32_t> subimageIndices;  // From cast_material; 32 slots, -1 = unused.
    };

    struct CsdCastHierarchyEntry
    {
        std::int32_t childIndex = -1;
        std::int32_t nextIndex = -1;
    };

    struct CsdCastGroup
    {
        std::uint32_t rootCastIndex = 0;
        std::vector<CsdCast>                casts;
        std::vector<CsdCastHierarchyEntry>  hierarchy;
    };

    // Phase 297: a per-cast group-global transform (after walking the
    // cast hierarchy). Mirrors the Python `build_group_global_transforms`
    // result: each cast index -> {x, y, scale_x, scale_y}.
    struct CsdCastGlobalTransform
    {
        float x = 0.0f, y = 0.0f;
        float scaleX = 1.0f, scaleY = 1.0f;
    };

    namespace detail::cast_extractor
    {
        using detail::readU32;
        using detail::readF32;
        using detail::readNullTerminatedString;

        inline std::int32_t readI32(
            const std::vector<std::byte>& bytes,
            std::uint64_t offset,
            bool bigEndian) noexcept
        {
            const std::uint32_t u = readU32(bytes, offset, bigEndian);
            std::int32_t i = 0;
            std::memcpy(&i, &u, sizeof(i));
            return i;
        }

        inline CsdCastInfo parseCastInfo(
            const std::vector<std::byte>& bytes,
            std::uint64_t origin,
            bool bigEndian) noexcept
        {
            CsdCastInfo info;
            if (origin + 0x18 > bytes.size())
                return info;
            info.hideFlag      = readI32(bytes, origin + 0x00, bigEndian);
            info.translationX  = readF32(bytes, origin + 0x04, bigEndian);
            info.translationY  = readF32(bytes, origin + 0x08, bigEndian);
            info.rotation      = readF32(bytes, origin + 0x0C, bigEndian);
            info.scaleX        = readF32(bytes, origin + 0x10, bigEndian);
            info.scaleY        = readF32(bytes, origin + 0x14, bigEndian);
            info.subimageF     = readF32(bytes, origin + 0x18, bigEndian);
            info.present       = true;
            return info;
        }

        inline std::vector<std::int32_t> parseCastMaterial(
            const std::vector<std::byte>& bytes,
            std::uint64_t origin,
            bool bigEndian) noexcept
        {
            std::vector<std::int32_t> indices(32, -1);
            if (origin == 0 || origin + 4 * 32 > bytes.size())
                return indices;
            for (int i = 0; i < 32; ++i)
                indices[i] = readI32(bytes, origin + 4 * i, bigEndian);
            return indices;
        }

        inline CsdCast parseCast(
            const std::vector<std::byte>& bytes,
            std::uint64_t origin,
            std::uint64_t ncpjOrigin,
            bool bigEndian,
            std::uint32_t /*sceneVersion*/) noexcept
        {
            CsdCast c;
            if (origin + 0x44 > bytes.size())
                return c;
            c.isEnabled = readU32(bytes, origin + 0x08, bigEndian) != 0;

            c.points.topLeftX     = readF32(bytes, origin + 0x0C, bigEndian);
            c.points.topLeftY     = readF32(bytes, origin + 0x10, bigEndian);
            c.points.bottomLeftX  = readF32(bytes, origin + 0x14, bigEndian);
            c.points.bottomLeftY  = readF32(bytes, origin + 0x18, bigEndian);
            c.points.topRightX    = readF32(bytes, origin + 0x1C, bigEndian);
            c.points.topRightY    = readF32(bytes, origin + 0x20, bigEndian);
            c.points.bottomRightX = readF32(bytes, origin + 0x24, bigEndian);
            c.points.bottomRightY = readF32(bytes, origin + 0x28, bigEndian);

            const std::uint32_t castInfoOffset = readU32(bytes, origin + 0x30, bigEndian);
            const std::uint32_t castMaterialOffset = readU32(bytes, origin + 0x40, bigEndian);

            if (castInfoOffset != 0)
                c.info = parseCastInfo(bytes, ncpjOrigin + castInfoOffset, bigEndian);
            if (castMaterialOffset != 0)
                c.subimageIndices = parseCastMaterial(bytes, ncpjOrigin + castMaterialOffset, bigEndian);
            else
                c.subimageIndices.assign(32, -1);
            return c;
        }

        inline CsdCastGroup parseCastGroup(
            const std::vector<std::byte>& bytes,
            std::uint64_t origin,
            std::uint64_t ncpjOrigin,
            bool bigEndian,
            std::uint32_t sceneVersion) noexcept
        {
            CsdCastGroup g;
            if (origin + 16 > bytes.size())
                return g;

            const std::uint32_t castCount = readU32(bytes, origin + 0x00, bigEndian);
            const std::uint32_t castTableOff = readU32(bytes, origin + 0x04, bigEndian);
            g.rootCastIndex = readU32(bytes, origin + 0x08, bigEndian);
            const std::uint32_t hierarchyOff = readU32(bytes, origin + 0x0C, bigEndian);

            // Sanity caps to defend against malformed asset data.
            if (castCount > 4096) return g;

            const std::uint64_t castTableOrigin = ncpjOrigin + castTableOff;
            if (castTableOrigin + 4 * static_cast<std::uint64_t>(castCount) > bytes.size())
                return g;

            g.casts.reserve(castCount);
            for (std::uint32_t i = 0; i < castCount; ++i)
            {
                const std::uint32_t castOff = readU32(bytes, castTableOrigin + 4 * i, bigEndian);
                if (castOff == 0)
                {
                    g.casts.push_back({});
                    continue;
                }
                g.casts.push_back(parseCast(bytes, ncpjOrigin + castOff, ncpjOrigin, bigEndian, sceneVersion));
            }

            const std::uint64_t hierarchyOrigin = ncpjOrigin + hierarchyOff;
            if (hierarchyOff != 0
                && hierarchyOrigin + 8 * static_cast<std::uint64_t>(castCount) <= bytes.size())
            {
                g.hierarchy.reserve(castCount);
                for (std::uint32_t i = 0; i < castCount; ++i)
                {
                    CsdCastHierarchyEntry entry;
                    entry.childIndex = readI32(bytes, hierarchyOrigin + 8 * i + 0, bigEndian);
                    entry.nextIndex  = readI32(bytes, hierarchyOrigin + 8 * i + 4, bigEndian);
                    g.hierarchy.push_back(entry);
                }
            }
            else
            {
                g.hierarchy.assign(castCount, CsdCastHierarchyEntry{});
            }
            return g;
        }

        // Build a parent_index -> not-the-relation; map child -> parent
        // by walking the hierarchy linked-list from each parent index.
        // Mirrors `build_cast_parent_map` in the Python extractor.
        inline std::vector<std::int32_t> buildParentMap(const CsdCastGroup& g)
        {
            std::vector<std::int32_t> parent(g.casts.size(), -1);
            for (std::int32_t parentIdx = 0; parentIdx < static_cast<std::int32_t>(g.hierarchy.size()); ++parentIdx)
            {
                std::int32_t cursor = g.hierarchy[parentIdx].childIndex;
                std::vector<bool> seen(g.casts.size(), false);
                while (cursor >= 0
                    && cursor < static_cast<std::int32_t>(g.hierarchy.size())
                    && !seen[cursor])
                {
                    seen[cursor] = true;
                    parent[cursor] = parentIdx;
                    cursor = g.hierarchy[cursor].nextIndex;
                }
            }
            return parent;
        }

        inline std::vector<CsdCastGlobalTransform> buildGlobalTransforms(const CsdCastGroup& g)
        {
            std::vector<CsdCastGlobalTransform> transforms(g.casts.size());
            std::vector<bool> resolved(g.casts.size(), false);
            const auto parent = buildParentMap(g);

            // Iterative resolve via DFS from each unresolved cast.
            for (std::size_t i = 0; i < g.casts.size(); ++i)
            {
                if (resolved[i]) continue;
                std::vector<std::int32_t> chain;
                std::int32_t cursor = static_cast<std::int32_t>(i);
                while (cursor >= 0 && !resolved[cursor])
                {
                    chain.push_back(cursor);
                    cursor = parent[cursor];
                }
                CsdCastGlobalTransform pt;
                if (cursor >= 0 && resolved[cursor])
                    pt = transforms[cursor];

                for (auto it = chain.rbegin(); it != chain.rend(); ++it)
                {
                    const std::int32_t idx = *it;
                    const auto& info = g.casts[idx].info;
                    const float lx = info.present ? info.translationX : 0.0f;
                    const float ly = info.present ? info.translationY : 0.0f;
                    const float lsx = info.present ? info.scaleX : 1.0f;
                    const float lsy = info.present ? info.scaleY : 1.0f;
                    if (parent[idx] < 0)
                    {
                        transforms[idx].x = lx;
                        transforms[idx].y = ly;
                        transforms[idx].scaleX = lsx;
                        transforms[idx].scaleY = lsy;
                    }
                    else
                    {
                        transforms[idx].x      = pt.x + lx * pt.scaleX;
                        transforms[idx].y      = pt.y + ly * pt.scaleY;
                        transforms[idx].scaleX = pt.scaleX * lsx;
                        transforms[idx].scaleY = pt.scaleY * lsy;
                    }
                    pt = transforms[idx];
                    resolved[idx] = true;
                }
            }
            return transforms;
        }

        // Compose cast bounding rect in scene coords given the cast's
        // four corner points and its global group transform. Mirrors
        // `composed_cast_rect`.
        inline std::array<float, 4> composedCastRect(
            const CsdCastPoints& pts,
            const CsdCastGlobalTransform& xform) noexcept
        {
            const float xs[4] = {
                xform.x + pts.topLeftX     * xform.scaleX,
                xform.x + pts.bottomLeftX  * xform.scaleX,
                xform.x + pts.topRightX    * xform.scaleX,
                xform.x + pts.bottomRightX * xform.scaleX,
            };
            const float ys[4] = {
                xform.y + pts.topLeftY     * xform.scaleY,
                xform.y + pts.bottomLeftY  * xform.scaleY,
                xform.y + pts.topRightY    * xform.scaleY,
                xform.y + pts.bottomRightY * xform.scaleY,
            };
            const float left   = std::min({xs[0], xs[1], xs[2], xs[3]});
            const float right  = std::max({xs[0], xs[1], xs[2], xs[3]});
            const float top    = std::min({ys[0], ys[1], ys[2], ys[3]});
            const float bottom = std::max({ys[0], ys[1], ys[2], ys[3]});
            const float w = std::max(0.0005f, right - left);
            const float h = std::max(0.0005f, bottom - top);
            return { left, top, w, h };
        }

        // Pick the subimage index for a cast: prefer the slot picked by
        // cast_info.subimage; otherwise the first non-negative entry.
        inline std::int32_t chooseCastSubimageIndex(const CsdCast& c) noexcept
        {
            const std::int32_t slot = static_cast<std::int32_t>(c.info.subimageF + 0.5f);
            if (slot >= 0 && slot < static_cast<std::int32_t>(c.subimageIndices.size())
                && c.subimageIndices[slot] >= 0)
                return c.subimageIndices[slot];
            for (auto v : c.subimageIndices)
                if (v >= 0) return v;
            return -1;
        }

        // Parse a scene's full cast group list (using the scene header
        // offsets stashed by the loader in Phase 297).
        inline std::vector<CsdCastGroup> parseSceneCastGroups(
            const std::vector<std::byte>& bytes,
            const CsdSceneMetadata& meta) noexcept
        {
            std::vector<CsdCastGroup> groups;
            if (meta.sceneHeaderFileOffset == 0 || meta.ncpjChunkFileOffset == 0)
                return groups;

            const std::uint64_t sceneOrigin = meta.sceneHeaderFileOffset;
            if (sceneOrigin + 0x44 > bytes.size())
                return groups;

            const bool be = meta.bigEndian;
            const std::uint32_t groupCount    = readU32(bytes, sceneOrigin + 0x24, be);
            const std::uint32_t groupTableOff = readU32(bytes, sceneOrigin + 0x28, be);
            if (groupCount == 0 || groupTableOff == 0 || groupCount > 256) return groups;

            const std::uint64_t groupTableOrigin = meta.ncpjChunkFileOffset + groupTableOff;
            if (groupTableOrigin + 16 * static_cast<std::uint64_t>(groupCount) > bytes.size())
                return groups;

            groups.reserve(groupCount);
            for (std::uint32_t i = 0; i < groupCount; ++i)
            {
                const std::uint64_t entryOrigin = groupTableOrigin + 16u * i;
                groups.push_back(parseCastGroup(bytes, entryOrigin, meta.ncpjChunkFileOffset, be, meta.version));
            }
            return groups;
        }

        // Parse a scene's cast dictionary (cast names indexed by
        // (group_index, cast_index)). Returns a flat key->name map.
        inline std::vector<std::tuple<std::uint32_t, std::uint32_t, std::string>> parseSceneCastNames(
            const std::vector<std::byte>& bytes,
            const CsdSceneMetadata& meta) noexcept
        {
            std::vector<std::tuple<std::uint32_t, std::uint32_t, std::string>> out;
            if (meta.sceneHeaderFileOffset == 0 || meta.ncpjChunkFileOffset == 0) return out;
            const std::uint64_t sceneOrigin = meta.sceneHeaderFileOffset;
            if (sceneOrigin + 0x44 > bytes.size()) return out;
            const bool be = meta.bigEndian;
            const std::uint32_t castCount = readU32(bytes, sceneOrigin + 0x2C, be);
            const std::uint32_t dictOff   = readU32(bytes, sceneOrigin + 0x30, be);
            if (castCount == 0 || dictOff == 0 || castCount > 4096) return out;
            const std::uint64_t dictOrigin = meta.ncpjChunkFileOffset + dictOff;
            if (dictOrigin + 12 * static_cast<std::uint64_t>(castCount) > bytes.size()) return out;
            out.reserve(castCount);
            for (std::uint32_t i = 0; i < castCount; ++i)
            {
                const std::uint64_t entry = dictOrigin + 12u * i;
                const std::uint32_t nameOffset = readU32(bytes, entry + 0x00, be);
                const std::uint32_t groupIndex = readU32(bytes, entry + 0x04, be);
                const std::uint32_t castIndex  = readU32(bytes, entry + 0x08, be);
                std::string name;
                if (nameOffset != 0)
                    name = readNullTerminatedString(bytes, meta.ncpjChunkFileOffset + nameOffset);
                out.emplace_back(groupIndex, castIndex, std::move(name));
            }
            return out;
        }
    } // namespace detail::cast_extractor

    // Phase 297: extract every renderable draw command from a parsed
    // CSD project (no JSON in the loop). Iterates each scene ref, walks
    // its cast groups, composes per-cast group-global transforms, picks
    // each cast's subimage, and assembles a CsdNativeDrawCommand. This
    // is the C++ equivalent of `extract_scene_draw_commands` in
    // research_uiux/tools/build_yncp_native_component_map.py.
    inline std::vector<CsdNativeDrawCommand> extractDrawCommandsFromProject(
        const CsdProjectFile& project)
    {
        using namespace detail::cast_extractor;
        std::vector<CsdNativeDrawCommand> out;

        // Texture name resolution: scene's subimage[i].textureIndex ->
        // project.textureNames[textureIndex].
        const auto& textureNames = project.textureNames;

        const std::filesystem::path projectPath = project.sourcePath;
        std::string projectRelative;
        // Source path is absolute on disk; the renderer wants the path
        // RELATIVE to the asset extraction root. We can't infer the
        // root here, so leave projectRelativePath as the source-path
        // string and let the renderer prepend the root via texture
        // relative paths instead. The downstream textureRelativePath
        // is resolved via `${assetRoot}/<texture_name>` for retail
        // assets that ship next to the .yncp.

        for (const auto& sceneRef : project.allSceneRefs)
        {
            const auto& meta = sceneRef.metadata;
            const auto groups = parseSceneCastGroups(project.fileBytes, meta);
            const auto castNames = parseSceneCastNames(project.fileBytes, meta);

            // Build (group_index, cast_index) -> name lookup.
            auto findName = [&](std::uint32_t g, std::uint32_t c) -> std::string
            {
                for (const auto& tup : castNames)
                    if (std::get<0>(tup) == g && std::get<1>(tup) == c)
                        return std::get<2>(tup);
                return {};
            };

            std::uint32_t drawOrder = 0;
            for (std::uint32_t gi = 0; gi < groups.size(); ++gi)
            {
                const auto& grp = groups[gi];
                const auto transforms = buildGlobalTransforms(grp);
                for (std::uint32_t ci = 0; ci < grp.casts.size(); ++ci)
                {
                    const auto& cast = grp.casts[ci];
                    if (!cast.isEnabled) continue;

                    const std::int32_t subImgIdx = chooseCastSubimageIndex(cast);
                    if (subImgIdx < 0
                        || subImgIdx >= static_cast<std::int32_t>(meta.subimages.size()))
                        continue;
                    const auto& sub = meta.subimages[static_cast<std::size_t>(subImgIdx)];
                    if (sub.textureIndex >= textureNames.size()) continue;

                    const auto rect = composedCastRect(cast.points, transforms[ci]);

                    CsdNativeDrawCommand cmd;
                    cmd.projectRelativePath = projectPath.filename().string();
                    cmd.sceneName           = sceneRef.name;
                    cmd.castName            = findName(gi, ci);
                    cmd.nodePath            = sceneRef.nodePath;
                    cmd.groupIndex          = gi;
                    cmd.castIndex           = ci;
                    cmd.drawOrder           = drawOrder++;
                    cmd.hideFlag            = static_cast<std::uint32_t>(cast.info.hideFlag);
                    cmd.hasTexture          = true;
                    cmd.textureName         = textureNames[sub.textureIndex];
                    // textureRelativePath: caller must prepend asset
                    // search dirs (e.g. the directory next to the .yncp)
                    // since we don't have the global asset root here.
                    cmd.textureRelativePath = textureNames[sub.textureIndex];
                    cmd.uvLeft   = sub.topLeftU;
                    cmd.uvTop    = sub.topLeftV;
                    cmd.uvRight  = sub.bottomRightU;
                    cmd.uvBottom = sub.bottomRightV;
                    cmd.sceneLeft   = rect[0];
                    cmd.sceneTop    = rect[1];
                    cmd.sceneWidth  = rect[2];
                    cmd.sceneHeight = rect[3];
                    // Match the Python extractor's contract: baseTranslation
                    // is the cast's LOCAL cast_info translation (not the
                    // accumulated group-global transform), and baseScale is
                    // the cast's LOCAL scale. The renderer adds sceneLeft +
                    // baseTranslation to compute the world position; sceneLeft
                    // already contains the group-global offset.
                    cmd.baseTranslationX = cast.info.present ? cast.info.translationX : 0.0f;
                    cmd.baseTranslationY = cast.info.present ? cast.info.translationY : 0.0f;
                    cmd.baseScaleX = cast.info.present ? cast.info.scaleX : 1.0f;
                    cmd.baseScaleY = cast.info.present ? cast.info.scaleY : 1.0f;
                    cmd.baseRotation = cast.info.present ? cast.info.rotation : 0.0f;
                    out.push_back(std::move(cmd));
                }
            }
        }
        return out;
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
