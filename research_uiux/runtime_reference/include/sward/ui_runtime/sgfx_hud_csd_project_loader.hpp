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
        Cpaf,   // outer Hedgehog Engine resource container ('C','P','A','F')
        Yncp,   // raw Chao::CSD project ('Y','N','C','P')
        Xncp,   // raw Chao::CSD project, alternate variant ('X','N','C','P')
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
            if (magic[0] == 'Y' && magic[1] == 'N' && magic[2] == 'C' && magic[3] == 'P')
                return CsdProjectMagic::Yncp;
            if (magic[0] == 'X' && magic[1] == 'N' && magic[2] == 'C' && magic[3] == 'P')
                return CsdProjectMagic::Xncp;
            return CsdProjectMagic::Unknown;
        }

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

        if (out.outerMagic == CsdProjectMagic::Cpaf
            && !out.hasYncpPayload()
            && !out.hasXncpPayload())
        {
            out.loadStatus =
                "wrapped-no-payload: CPAF container present but no inner YNCP / XNCP magic was found within the first 1 KiB";
            return out;
        }

        out.loadStatus = "ok: CSD project asset loaded with recognizable magic";
        return out;
    }

    // Convenience: returns true iff the file at `path` is a loadable CSD
    // project asset. Discards the loaded buffer; for a streaming check
    // that does not allocate the full file, see the future Phase 277+
    // version that will read just the header bytes.
    inline bool isLoadableCsdProject(const std::filesystem::path& path)
    {
        const CsdProjectFile loaded = loadCsdProjectFile(path);
        return loaded.hasRecognizedMagic()
            && (loaded.hasYncpPayload() || loaded.hasXncpPayload());
    }

} // namespace sward::ui_runtime::generated::sgfx_hud
