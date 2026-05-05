// Phase 275: C++ loader for Sonic Unleashed CSD project files (`.yncp`).
//
// This is the first piece of the human-readable port that actually reads
// retail asset bytes. It pairs with the `kSceneBindings[]` registry the
// SGFX HUD layout headers carry (Phase 266 / 268) and the Python asset
// validator (Phase 271) to give the port a complete chain from member
// declaration all the way to bytes-on-disk.
//
// Scope:
//
// * Open a `.yncp` file from disk, validate it is a real Sonic Unleashed
//   CSD project asset by checking for the SWA / Hedgehog Engine `CPAF`
//   resource container magic at offset 0 and locating the inner `YNCP`
//   (or `XNCP`) magic the Chao::CSD parser keys on.
// * Surface basic file metadata — total size, container magic, inner
//   magic offset, byte ordering — sufficient for a downstream C++ port
//   to decide whether the file is loadable.
// * Stay header-only and standard-library only (`<cstdint>`,
//   `<cstring>`, `<filesystem>`, `<fstream>`, `<optional>`, `<string>`,
//   `<vector>`); no third-party dependencies, no allocations beyond the
//   single read buffer, no exceptions thrown out of the public API.
//
// Out of scope (deferred to future phases as the real CSD runtime ports
// land):
//
// * Decoding individual scenes / casts / animations from the YNCP
//   payload — the binary spec needs the existing Python parser ported in
//   detail and is too big for one phase.
// * Reference-counted allocation hooks; the loader returns plain owning
//   `std::vector<std::byte>` buffers.
// * Endianness-correct field access for the YNCP payload (the file is
//   always big-endian on Xbox 360; the loader records the magic offset
//   and lets a follow-up phase do the swapping).

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime::generated::sgfx_hud
{
    enum class CsdProjectMagic : std::uint8_t
    {
        Unknown,
        Cpaf,   // outer Hedgehog Engine resource container ('C','P','A','F') — big-endian payload, Xbox 360 retail
        Fapc,   // outer Hedgehog Engine resource container ('F','A','P','C') — little-endian payload, PC variant
        Yncp,   // raw Chao::CSD project ('Y','N','C','P')
        Xncp,   // raw Chao::CSD project, alternate variant ('X','N','C','P')
    };

    // Phase 279: a single CSD scene id pulled out of the YNCP / XNCP
    // root CSD node. The Sonic Unleashed runtime sweep references scenes
    // by `(project, scene)` pairs; the `name` field here is the bare
    // `scene_name` half of that pair (e.g. `so_speed_gauge`,
    // `gauge_frame`). Index is the SWA-side ordinal used to address
    // scenes within the project's render plan.
    struct CsdSceneId
    {
        std::string  name;
        std::uint32_t index = 0;
    };

    // Phase 280: a CSD scene id reached through the recursive walk of
    // the project's CSD node tree. `nodePath` carries the slash-joined
    // chain of node names from the root down to the scene's parent
    // (e.g. `add` for scenes that live inside `Root/add`); `name` is
    // the bare scene name. The `nodePath` is empty for scenes that hang
    // directly off the root node, matching the YNCP native component
    // map's `Root` node_path.
    struct CsdSceneRef
    {
        std::string nodePath;
        std::string name;
        std::uint32_t index = 0;
    };

    struct CsdProjectFile
    {
        std::filesystem::path        sourcePath;
        std::vector<std::byte>       fileBytes;
        std::uint64_t                fileSizeBytes = 0;
        CsdProjectMagic              outerMagic = CsdProjectMagic::Unknown;
        std::array<char, 4>          outerMagicChars{};
        std::optional<std::uint64_t> innerYncpMagicOffset;
        std::optional<std::uint64_t> innerXncpMagicOffset;
        std::string                  loadStatus;
        // Phase 279: parsed CSD project metadata. Populated when the
        // outer CPAF / FAPC container is structurally walkable; left
        // empty (with a populated `parseStatus`) on any failure.
        std::string                  projectName;
        std::string                  ncpjSignature;       // e.g. "JPCN" / "NCPJ" depending on endian
        std::vector<CsdSceneId>      rootSceneIds;
        std::string                  parseStatus;
        // Phase 280: every scene reachable from the project's root CSD
        // node, including scenes that live inside nested child nodes
        // (e.g. the `Root/add/speed_count` cluster). Populated as a
        // recursive walk over the CSD node tree's child / scene tables;
        // empty when only the root scenes are present.
        std::vector<CsdSceneRef>     allSceneRefs;
        // Phase 284: every texture name referenced by the project, in
        // SWA texture-table order. The retail Sonic Unleashed CSD format
        // ships a parallel `NXTL` chunk alongside `NCPJ`; the SWA HUD
        // bindings use these names to resolve subimage UVs back to DDS
        // files on disk. Empty when the project file has no NXTL chunk.
        std::vector<std::string>     textureNames;

        // True iff `outerMagic` is one of the known CSD project magics
        // (CPAF wraps a YNCP/XNCP payload; Yncp / Xncp are raw payloads).
        // The validator (Phase 271) uses the same accept set.
        constexpr bool hasRecognizedMagic() const noexcept
        {
            return outerMagic != CsdProjectMagic::Unknown;
        }

        // True iff the file actually contains the inner `YNCP` magic,
        // either at offset 0 (raw form) or somewhere inside the CPAF
        // container payload.
        constexpr bool hasYncpPayload() const noexcept
        {
            return innerYncpMagicOffset.has_value()
                || outerMagic == CsdProjectMagic::Yncp;
        }

        constexpr bool hasXncpPayload() const noexcept
        {
            return innerXncpMagicOffset.has_value()
                || outerMagic == CsdProjectMagic::Xncp;
        }
    };

    namespace detail
    {
        inline CsdProjectMagic classifyOuterMagic(const std::array<char, 4>& magic) noexcept
        {
            if (magic[0] == 'C' && magic[1] == 'P' && magic[2] == 'A' && magic[3] == 'F')
                return CsdProjectMagic::Cpaf;
            if (magic[0] == 'F' && magic[1] == 'A' && magic[2] == 'P' && magic[3] == 'C')
                return CsdProjectMagic::Fapc;
            if (magic[0] == 'Y' && magic[1] == 'N' && magic[2] == 'C' && magic[3] == 'P')
                return CsdProjectMagic::Yncp;
            if (magic[0] == 'X' && magic[1] == 'N' && magic[2] == 'C' && magic[3] == 'P')
                return CsdProjectMagic::Xncp;
            return CsdProjectMagic::Unknown;
        }

        inline bool isBigEndianContainer(CsdProjectMagic magic) noexcept
        {
            // CPAF (Xbox 360 retail) is big-endian; FAPC (PC) is little.
            // Raw YNCP / XNCP payloads we observe in the user's archive
            // also follow the CPAF / big-endian convention.
            return magic == CsdProjectMagic::Cpaf
                || magic == CsdProjectMagic::Yncp
                || magic == CsdProjectMagic::Xncp;
        }

        inline std::uint32_t readU32(
            const std::vector<std::byte>& bytes,
            std::uint64_t offset,
            bool bigEndian) noexcept
        {
            if (offset + 4 > bytes.size())
                return 0;
            const auto b0 = static_cast<std::uint32_t>(static_cast<std::uint8_t>(bytes[offset + 0]));
            const auto b1 = static_cast<std::uint32_t>(static_cast<std::uint8_t>(bytes[offset + 1]));
            const auto b2 = static_cast<std::uint32_t>(static_cast<std::uint8_t>(bytes[offset + 2]));
            const auto b3 = static_cast<std::uint32_t>(static_cast<std::uint8_t>(bytes[offset + 3]));
            return bigEndian
                ? (b0 << 24) | (b1 << 16) | (b2 << 8) | b3
                : (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
        }

        inline std::array<char, 4> readMagic(
            const std::vector<std::byte>& bytes,
            std::uint64_t offset) noexcept
        {
            std::array<char, 4> out{};
            if (offset + 4 > bytes.size())
                return out;
            for (std::size_t i = 0; i < 4; ++i)
                out[i] = static_cast<char>(bytes[offset + i]);
            return out;
        }

        inline std::string readNullTerminatedString(
            const std::vector<std::byte>& bytes,
            std::uint64_t offset) noexcept
        {
            if (offset == 0 || offset >= bytes.size())
                return {};
            std::string out;
            for (std::uint64_t i = offset; i < bytes.size(); ++i)
            {
                const auto byte = static_cast<std::uint8_t>(bytes[i]);
                if (byte == 0)
                    break;
                out.push_back(static_cast<char>(byte));
            }
            return out;
        }

        // Forward declarations for the in-place CPAF / FAPC parser; the
        // implementation lives below `loadCsdProjectFile` so the latter
        // is grouped with its public-facing companions.
        inline std::string parseCsdProjectInPlace(CsdProjectFile& out) noexcept;
        inline void walkCsdNodeRecursive(
            CsdProjectFile& out,
            const std::vector<std::byte>& bytes,
            bool bigEndian,
            std::uint64_t ncpjOrigin,
            std::uint64_t nodeOrigin,
            const std::string& currentNodePath,
            int depth,
            bool isRootNode) noexcept;
        // Phase 284: walk an `NXTL` (texture list) chunk and populate
        // `out.textureNames` with the per-index DDS filenames the SWA
        // CSD runtime would resolve when loading the project's casts.
        inline void parseTextureList(
            CsdProjectFile& out,
            const std::vector<std::byte>& bytes,
            bool bigEndian,
            std::uint64_t nxtlOrigin) noexcept;

        inline std::optional<std::uint64_t> findFourByteMagic(
            const std::vector<std::byte>& bytes,
            const char (&needle)[5]) noexcept
        {
            // Linear scan for a four-byte ASCII tag inside the file's
            // header chunk. Bounded to the first 1 KiB because every
            // real CSD project file places its YNCP / XNCP payload
            // header within the first few hundred bytes (the actual
            // observed offset for `ui_playscreen.yncp` in the user's
            // extraction is 11). Capping the search keeps the loader
            // O(1) on file size.
            const std::uint64_t scanLimit =
                bytes.size() < 1024 ? bytes.size() : 1024;
            if (scanLimit < 4)
                return std::nullopt;
            for (std::uint64_t i = 0; i + 4 <= scanLimit; ++i)
            {
                if (static_cast<char>(bytes[i + 0]) == needle[0]
                    && static_cast<char>(bytes[i + 1]) == needle[1]
                    && static_cast<char>(bytes[i + 2]) == needle[2]
                    && static_cast<char>(bytes[i + 3]) == needle[3])
                {
                    return i;
                }
            }
            return std::nullopt;
        }
    } // namespace detail

    // Load a `.yncp` (or `.xncp`) file from disk and validate it is a
    // real Sonic Unleashed CSD project asset. Always returns a populated
    // `CsdProjectFile` — failures are surfaced via `loadStatus` rather
    // than exceptions so callers can decide policy.
    inline CsdProjectFile loadCsdProjectFile(const std::filesystem::path& path)
    {
        CsdProjectFile out;
        out.sourcePath = path;

        std::error_code ec;
        if (!std::filesystem::exists(path, ec) || ec)
        {
            out.loadStatus = "missing-asset: file does not exist on disk";
            return out;
        }
        const auto fileSize = std::filesystem::file_size(path, ec);
        if (ec)
        {
            out.loadStatus = "io-error: filesystem failed to report size";
            return out;
        }
        out.fileSizeBytes = fileSize;
        if (fileSize == 0)
        {
            out.loadStatus = "empty-asset: file is zero bytes";
            return out;
        }

        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open())
        {
            out.loadStatus = "io-error: failed to open asset for reading";
            return out;
        }
        out.fileBytes.resize(static_cast<std::size_t>(fileSize));
        stream.read(
            reinterpret_cast<char*>(out.fileBytes.data()),
            static_cast<std::streamsize>(fileSize));
        if (!stream)
        {
            out.loadStatus = "io-error: short read while loading asset";
            return out;
        }

        if (out.fileBytes.size() < 4)
        {
            out.loadStatus = "truncated-asset: fewer than 4 bytes (no magic)";
            return out;
        }
        for (std::size_t i = 0; i < 4; ++i)
            out.outerMagicChars[i] = static_cast<char>(out.fileBytes[i]);
        out.outerMagic = detail::classifyOuterMagic(out.outerMagicChars);

        if (out.outerMagic == CsdProjectMagic::Unknown)
        {
            out.loadStatus =
                "bad-magic: file does not start with CPAF / YNCP / XNCP magic";
            return out;
        }

        out.innerYncpMagicOffset = detail::findFourByteMagic(out.fileBytes, "YNCP");
        out.innerXncpMagicOffset = detail::findFourByteMagic(out.fileBytes, "XNCP");

        // CPAF is sufficient on its own: real Sonic Unleashed `.yncp`
        // files ship as a Hedgehog Engine CPAF container whose inner
        // payload format is identified by a tag that is not stored as
        // the literal ASCII string "YNCP" / "XNCP" anywhere in the file.
        // The inner-magic search above remains as informational metadata
        // for the rare raw payload case (some test / probe files in the
        // repo are stored unwrapped) but is not required for `ok` status.
        out.loadStatus = "ok: CSD project asset loaded with recognizable magic";

        // Phase 279: walk the CPAF / FAPC container to extract the
        // project name and the root CSD node's scene IDs. Failure here
        // populates `parseStatus` and leaves the rest of the parsed
        // fields empty; the loader's `loadStatus` stays `ok` because
        // magic-only loading is still a useful baseline outcome.
        if (out.outerMagic == CsdProjectMagic::Cpaf
            || out.outerMagic == CsdProjectMagic::Fapc)
        {
            const std::string err = detail::parseCsdProjectInPlace(out);
            out.parseStatus = err.empty()
                ? std::string{"ok: CPAF / FAPC payload parsed; "
                              "project name and root scene IDs populated"}
                : err;
        }
        else
        {
            out.parseStatus =
                "deferred-raw-payload: outer magic is YNCP / XNCP "
                "(unwrapped raw payload); detailed scene parsing for raw "
                "payloads is deferred to a future phase";
        }
        return out;
    }

    // Convenience: returns true iff the file at `path` is a loadable CSD
    // project asset. Discards the loaded buffer; for a streaming check
    // that does not allocate the full file, see the future Phase 277+
    // version that will read just the header bytes.
    inline bool isLoadableCsdProject(const std::filesystem::path& path)
    {
        const CsdProjectFile loaded = loadCsdProjectFile(path);
        // CPAF outer magic alone is sufficient; the inner YNCP / XNCP
        // payload tags are an optional signal that only appears in
        // unwrapped raw payloads (rare in retail data).
        return loaded.hasRecognizedMagic();
    }

    namespace detail
    {
        // Phase 279: walk the CPAF / FAPC container payload to extract
        // the CSD project name and the root CSD node's scene IDs. Ports
        // a focused subset of the Python `inspect_xncp_yncp.py` parser:
        //
        //   1. Skip the 4-byte outer magic.
        //   2. Read content_size_0 (4 bytes), then a chunk_file at the
        //      next 8-byte-aligned offset.
        //   3. Inside the chunk_file, read the 32-byte chunk header.
        //      The `header_size` field is little-endian regardless of
        //      file endianness (matches the Python parser).
        //   4. Follow `next_chunk_offset` to the NCPJ chunk — the CSD
        //      project root.
        //   5. Read the project_name string and the root_csd_node.
        //   6. Walk the root_csd_node to collect every (scene_name,
        //      index) pair.
        //
        // Cast / animation / texture extraction is deferred to future
        // phases — this is the minimum needed to surface scene names so
        // the SGFX HUD asset binding validator can cross-check
        // `kSceneBindings[]` rows against actual asset content.
        inline std::string parseCsdProjectInPlace(CsdProjectFile& out) noexcept
        {
            const auto& bytes = out.fileBytes;
            const bool bigEndian = isBigEndianContainer(out.outerMagic);

            // Outer header: 4-byte magic + 4-byte content_size_0,
            // followed by chunk_file_0 starting at offset 8. The Sonic
            // Unleashed CPAF / FAPC container ships TWO chunk_file
            // resources back-to-back; one carries the NCPJ project tree,
            // the other the NXTL texture list. Phase 284 walks both.
            constexpr std::uint64_t kFirstResourceSize = 4;
            std::uint64_t cursor = 4;  // skip 4-byte outer magic
            std::uint64_t ncpjOrigin = 0;
            bool foundNcpj = false;
            for (int resourceIndex = 0; resourceIndex < 2; ++resourceIndex)
            {
                if (cursor + kFirstResourceSize > bytes.size())
                    break;
                const std::uint32_t resourceContentSize =
                    readU32(bytes, cursor, bigEndian);
                const std::uint64_t chunkFileStart = cursor + 4;
                if (chunkFileStart + 32 > bytes.size())
                    break;

                const std::uint32_t nextChunkOff =
                    readU32(bytes, chunkFileStart + 12, bigEndian);
                if (nextChunkOff != 0
                    && chunkFileStart + nextChunkOff + 4 <= bytes.size())
                {
                    const std::uint64_t innerOrigin = chunkFileStart + nextChunkOff;
                    const auto innerMagic = readMagic(bytes, innerOrigin);
                    if (innerMagic[0] == 'N' && innerMagic[1] == 'X'
                        && innerMagic[2] == 'T' && innerMagic[3] == 'L')
                    {
                        parseTextureList(out, bytes, bigEndian, innerOrigin);
                    }
                    else if (!foundNcpj)
                    {
                        ncpjOrigin = innerOrigin;
                        out.ncpjSignature.assign(innerMagic.begin(), innerMagic.end());
                        foundNcpj = true;
                    }
                }

                cursor = chunkFileStart + resourceContentSize;
            }

            if (!foundNcpj)
                return "no-ncpj-resource: container has no NCPJ-style project chunk";
            if (ncpjOrigin + 32 > bytes.size())
                return "bad-ncpj-origin: NCPJ chunk position is past end of file";

            const auto ncpjMagic = readMagic(bytes, ncpjOrigin);
            out.ncpjSignature.assign(ncpjMagic.begin(), ncpjMagic.end());
            // The NCPJ signature is the project-format tag; we do not
            // hard-require a specific value because PC / Xbox 360 builds
            // store the four bytes in different orders. The signature is
            // surfaced as-is so callers can inspect it if needed.

            const std::uint32_t rootNodeOffset = readU32(bytes, ncpjOrigin + 16, bigEndian);
            const std::uint32_t projectNameOffset = readU32(bytes, ncpjOrigin + 20, bigEndian);

            // Strings are null-terminated and addressed via offsets that
            // are relative to the chunk origin.
            if (projectNameOffset != 0)
                out.projectName = readNullTerminatedString(bytes, ncpjOrigin + projectNameOffset);

            if (rootNodeOffset == 0
                || ncpjOrigin + rootNodeOffset + 24 > bytes.size())
            {
                return "bad-ncpj-header: root_node_offset points outside the file";
            }

            const std::uint64_t rootNodeOrigin = ncpjOrigin + rootNodeOffset;
            const std::uint32_t sceneCount       = readU32(bytes, rootNodeOrigin + 0, bigEndian);
            const std::uint32_t sceneIdTableOff  = readU32(bytes, rootNodeOrigin + 8, bigEndian);
            // Sanity bound the scene count so a corrupt header cannot
            // make the loop run away. The largest observed root scene
            // count in retail Sonic Unleashed UI projects is well under
            // 256; cap at 1024 for safety.
            if (sceneCount > 1024)
                return "implausible-scene-count: root CSD node lists more than 1024 scenes";
            const std::uint64_t sceneIdTableOrigin = ncpjOrigin + sceneIdTableOff;
            if (sceneCount > 0
                && sceneIdTableOrigin + (8 * static_cast<std::uint64_t>(sceneCount)) > bytes.size())
            {
                return "bad-scene-id-table: table extends past end of file";
            }

            // Recursive walk of the CSD node tree. Every scene the tree
            // exposes is captured in `allSceneRefs` with its node path;
            // only the root node's scenes also feed `rootSceneIds` for
            // backwards compatibility with Phase 279 callers.
            out.rootSceneIds.reserve(sceneCount);
            walkCsdNodeRecursive(
                out, bytes, bigEndian,
                ncpjOrigin, rootNodeOrigin,
                /*currentNodePath=*/std::string{},
                /*depth=*/0,
                /*isRootNode=*/true);

            // Returning empty status means "ok"; non-empty is a failure
            // diagnostic. The NCPJ signature tag stays in
            // `out.ncpjSignature` for callers; the actual wire-format
            // value depends on platform endian and is not hard-asserted.
            (void)sceneCount;  // sceneCount was for the bound check; the
                               // recursive walker re-reads it per-node.
            return std::string{};
        }

        // Helper used by `parseCsdProjectInPlace` to traverse the CSD
        // node tree. Bounded by an explicit max depth so a corrupt
        // child / scene offset cannot cause unbounded recursion.
        inline void walkCsdNodeRecursive(
            CsdProjectFile& out,
            const std::vector<std::byte>& bytes,
            bool bigEndian,
            std::uint64_t ncpjOrigin,
            std::uint64_t nodeOrigin,
            const std::string& currentNodePath,
            int depth,
            bool isRootNode) noexcept
        {
            constexpr int kMaxDepth = 6;
            if (depth > kMaxDepth)
                return;
            if (nodeOrigin + 24 > bytes.size())
                return;

            const std::uint32_t sceneCount         = readU32(bytes, nodeOrigin + 0, bigEndian);
            // sceneTableOff (offset +4) is the table of full scene
            // descriptors; we only need the scene IDs at offset +8 for
            // name lookup, so the descriptor table is not consumed here.
            const std::uint32_t sceneIdTableOff    = readU32(bytes, nodeOrigin + 8, bigEndian);
            const std::uint32_t childCount         = readU32(bytes, nodeOrigin + 12, bigEndian);
            const std::uint32_t childListOffset    = readU32(bytes, nodeOrigin + 16, bigEndian);
            const std::uint32_t childDictionaryOff = readU32(bytes, nodeOrigin + 20, bigEndian);

            if (sceneCount > 1024 || childCount > 1024)
                return;  // implausible — corrupt header.

            // Pull the scene IDs at this node level into `allSceneRefs`
            // and (for the root node) also into `rootSceneIds`.
            //
            // Phase 281 fix: each scene-id entry's `index` field points
            // at which scene in the node's `scenes[]` table receives
            // that name; the entries on disk are NOT necessarily in
            // index order. Build a name array sized to `sceneCount` and
            // place each name at its target index so the output matches
            // the canonical scene order the Python ground-truth parser
            // produces (`sorted(scene_ids, key=lambda x: x["index"])`).
            const std::uint64_t sceneIdTableOrigin = ncpjOrigin + sceneIdTableOff;
            if (sceneCount > 0
                && sceneIdTableOrigin + (8 * static_cast<std::uint64_t>(sceneCount)) <= bytes.size())
            {
                std::vector<std::string> sceneNamesByIndex(sceneCount);
                for (std::uint32_t i = 0; i < sceneCount; ++i)
                {
                    const std::uint64_t entryOrigin = sceneIdTableOrigin + (8u * i);
                    const std::uint32_t nameOffset = readU32(bytes, entryOrigin + 0, bigEndian);
                    const std::uint32_t sceneIndex = readU32(bytes, entryOrigin + 4, bigEndian);
                    if (sceneIndex < sceneCount)
                    {
                        sceneNamesByIndex[sceneIndex] = nameOffset != 0
                            ? readNullTerminatedString(bytes, ncpjOrigin + nameOffset)
                            : std::string{};
                    }
                }
                for (std::uint32_t i = 0; i < sceneCount; ++i)
                {
                    CsdSceneRef ref;
                    ref.nodePath = currentNodePath;
                    ref.name = sceneNamesByIndex[i];
                    ref.index = i;
                    out.allSceneRefs.push_back(ref);

                    if (isRootNode)
                    {
                        CsdSceneId sid;
                        sid.name = sceneNamesByIndex[i];
                        sid.index = i;
                        out.rootSceneIds.push_back(std::move(sid));
                    }
                }
            }

            // Walk children. Each child entry in the node-list is 24
            // bytes wide (matches the recursive `parse_csd_node` stride
            // in `inspect_xncp_yncp.py`); the dictionary table uses
            // 8-byte entries holding (name_offset, index) where the
            // index points at which entry in the child-list receives
            // that name. Phase 281 fix: same canonical-index re-ordering
            // as the scene-id loop above so child names line up with the
            // file's child[i] declaration order.
            const std::uint64_t childListOrigin = ncpjOrigin + childListOffset;
            const std::uint64_t childDictionaryOrigin = ncpjOrigin + childDictionaryOff;
            if (childCount > 0
                && childListOrigin + (24 * static_cast<std::uint64_t>(childCount)) <= bytes.size()
                && childDictionaryOrigin + (8 * static_cast<std::uint64_t>(childCount)) <= bytes.size())
            {
                std::vector<std::string> childNamesByIndex(childCount);
                for (std::uint32_t i = 0; i < childCount; ++i)
                {
                    const std::uint64_t dictEntryOrigin = childDictionaryOrigin + (8u * i);
                    const std::uint32_t childNameOffset = readU32(bytes, dictEntryOrigin + 0, bigEndian);
                    const std::uint32_t childIndex = readU32(bytes, dictEntryOrigin + 4, bigEndian);
                    if (childIndex < childCount)
                    {
                        childNamesByIndex[childIndex] = childNameOffset != 0
                            ? readNullTerminatedString(bytes, ncpjOrigin + childNameOffset)
                            : std::string{};
                    }
                }
                for (std::uint32_t i = 0; i < childCount; ++i)
                {
                    const std::uint64_t childOrigin = childListOrigin + (24u * i);
                    const std::string& childName = childNamesByIndex[i];
                    const std::string nextPath = currentNodePath.empty()
                        ? childName
                        : currentNodePath + "/" + childName;
                    walkCsdNodeRecursive(
                        out, bytes, bigEndian,
                        ncpjOrigin, childOrigin,
                        nextPath,
                        depth + 1,
                        /*isRootNode=*/false);
                }
            }
        }

        // Phase 284: extract every DDS texture name referenced by the
        // CSD project. The NXTL chunk header layout (matches the Python
        // `parse_texture_list`):
        //   +0x00: 4-byte "NXTL" signature
        //   +0x04: chunk size (little-endian, ignored here)
        //   +0x08: list_offset (endian)
        //   +0x0C: field0c (endian, ignored)
        //   +0x10: texture_count (endian)
        //   +0x14: textures_offset (endian; pointer relative to NXTL origin)
        // Each texture entry is 8 bytes: name_offset (endian) + field04
        // (endian, ignored). Names are null-terminated strings whose
        // offsets are relative to the NXTL origin.
        inline void parseTextureList(
            CsdProjectFile& out,
            const std::vector<std::byte>& bytes,
            bool bigEndian,
            std::uint64_t nxtlOrigin) noexcept
        {
            if (nxtlOrigin + 0x18 > bytes.size())
                return;
            const std::uint32_t textureCount   = readU32(bytes, nxtlOrigin + 0x10, bigEndian);
            const std::uint32_t texturesOffset = readU32(bytes, nxtlOrigin + 0x14, bigEndian);
            if (textureCount > 4096)
                return;  // implausible — corrupt header.
            const std::uint64_t entriesOrigin = nxtlOrigin + texturesOffset;
            if (entriesOrigin + (8 * static_cast<std::uint64_t>(textureCount)) > bytes.size())
                return;

            out.textureNames.reserve(textureCount);
            for (std::uint32_t i = 0; i < textureCount; ++i)
            {
                const std::uint64_t entryOrigin = entriesOrigin + (8u * i);
                const std::uint32_t nameOffset = readU32(bytes, entryOrigin + 0, bigEndian);
                std::string name = nameOffset != 0
                    ? readNullTerminatedString(bytes, nxtlOrigin + nameOffset)
                    : std::string{};
                out.textureNames.push_back(std::move(name));
            }
        }
    } // namespace detail

} // namespace sward::ui_runtime::generated::sgfx_hud
