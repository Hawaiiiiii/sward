// Phase 339: standalone xex header inspector + (limited) string dumper.
//
// ATTEMPTED with full XenonUtils linkage; abandoned because:
//   1. Linking XenonUtils.lib pulls in a giant static-init map
//      (XboxKernelExports) that segfaults under MSVC at process start
//      in this environment.
//   2. Even if that linked clean, retail Sonic Unleashed default.xex
//      uses XEX_COMPRESSION_NORMAL (LZX, value 2). The shipped
//      Xex2LoadImage only handles NONE (0) and BASIC (1). LZX would
//      need libmspack hooked up -- vendored at .../thirdparty/libmspack
//      but not wired into the existing CMake.
//   3. Even with decompression working, the strings I was hunting
//      ("ui_result", "result_num_1", etc.) live in the .yncp asset
//      files, NOT the xex executable. Confirmed by direct grep:
//         grep -aoE "result_num_[0-9]" extracted_assets/.../ui_result.yncp
//         -> result_num_1 .. result_num_6
//      So a successful xex dump wouldn't have answered the original
//      question (where is CHudResult::Update?) anyway.
//
// What remains useful: a self-contained xex header dumper that needs
// no XenonUtils dependency. Reads the raw XEX2 header + opt-headers,
// reports compression / encryption / image base / entry point so the
// next attempt knows exactly what it would have to handle.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Big-endian read helpers (XEX2 is BE).
static std::uint32_t beU32(const std::uint8_t* p) noexcept
{
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16)
         | (std::uint32_t(p[2]) << 8)  |  std::uint32_t(p[3]);
}
static std::uint16_t beU16(const std::uint8_t* p) noexcept
{
    return std::uint16_t((std::uint32_t(p[0]) << 8) | std::uint32_t(p[1]));
}

static std::vector<std::uint8_t> readAll(const char* path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    f.seekg(0, std::ios::end);
    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(size);
    f.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <default.xex>\n";
        std::cerr << "  Reports XEX2 header layout + compression / encryption type.\n";
        return 2;
    }

    const auto bytes = readAll(argv[1]);
    if (bytes.empty()) { std::cerr << "could not read " << argv[1] << "\n"; return 1; }
    std::cout << "loaded " << bytes.size() << " bytes from " << argv[1] << "\n";

    if (bytes.size() < 24
        || bytes[0] != 'X' || bytes[1] != 'E' || bytes[2] != 'X' || bytes[3] != '2')
    {
        std::cerr << "not an XEX2 file\n";
        return 1;
    }

    // Xex2Header layout (be<u32> fields):
    //   +0x00 magic           ('XEX2')
    //   +0x04 moduleFlags
    //   +0x08 headerSize
    //   +0x0C reserved
    //   +0x10 securityOffset
    //   +0x14 headerCount
    const std::uint32_t moduleFlags    = beU32(bytes.data() + 0x04);
    const std::uint32_t headerSize     = beU32(bytes.data() + 0x08);
    const std::uint32_t securityOffset = beU32(bytes.data() + 0x10);
    const std::uint32_t headerCount    = beU32(bytes.data() + 0x14);
    std::cout << std::hex
              << "  moduleFlags=0x" << moduleFlags
              << "  headerSize=0x" << headerSize
              << "  securityOffset=0x" << securityOffset
              << std::dec
              << "  headerCount=" << headerCount << "\n";

    // Walk opt headers. Each is 8 bytes: { be<u32> key, be<u32> value/offset }.
    constexpr std::uint32_t XEX_HEADER_FILE_FORMAT_INFO   = 0x000003FF;
    constexpr std::uint32_t XEX_HEADER_IMAGE_BASE_ADDRESS = 0x00010201;
    constexpr std::uint32_t XEX_HEADER_ENTRY_POINT        = 0x00010100;
    const std::uint8_t* optBase = bytes.data() + 0x18;
    std::uint32_t fmtOffset = 0, imageBase = 0, entryPoint = 0;
    for (std::uint32_t i = 0; i < headerCount && (0x18 + i * 8 + 8) <= bytes.size(); ++i)
    {
        const std::uint32_t key   = beU32(optBase + i * 8);
        const std::uint32_t value = beU32(optBase + i * 8 + 4);
        if (key == XEX_HEADER_FILE_FORMAT_INFO)   fmtOffset  = value;
        if (key == XEX_HEADER_IMAGE_BASE_ADDRESS) imageBase  = value;
        if (key == XEX_HEADER_ENTRY_POINT)        entryPoint = value;
    }
    std::cout << std::hex
              << "  imageBase=0x" << imageBase
              << "  entryPoint=0x" << entryPoint
              << "  fileFormatOffset=0x" << fmtOffset << std::dec << "\n";

    // Xex2OptFileFormatInfo layout:
    //   +0x00 infoSize        (be u32)
    //   +0x04 encryptionType  (be u16)
    //   +0x06 compressionType (be u16)
    if (fmtOffset != 0 && fmtOffset + 8 <= bytes.size())
    {
        const std::uint32_t infoSize        = beU32(bytes.data() + fmtOffset + 0);
        const std::uint16_t encryption      = beU16(bytes.data() + fmtOffset + 4);
        const std::uint16_t compression     = beU16(bytes.data() + fmtOffset + 6);
        const char* compName =
            compression == 0 ? "NONE" :
            compression == 1 ? "BASIC" :
            compression == 2 ? "NORMAL (LZX)" :
            compression == 3 ? "DELTA" : "?";
        const char* encName  = encryption == 0 ? "NONE"
                            : encryption == 1 ? "NORMAL (AES)"
                            : "?";
        std::cout << "  fileFormatInfo: infoSize=" << infoSize
                  << " encryption=" << encryption << " (" << encName << ")"
                  << " compression=" << compression << " (" << compName << ")\n";

        if (compression == 0 || compression == 1)
        {
            std::cout << "  >>> NONE/BASIC compression: in-place data scan IS feasible\n";
            std::cout << "  >>> (this dumper does not yet implement post-decryption "
                         "BASIC reassembly; future work)\n";
        }
        else if (compression == 2)
        {
            std::cout << "  >>> NORMAL (LZX) compression: needs libmspack to inflate\n";
            std::cout << "  >>> XenonUtils' shipped Xex2LoadImage does NOT handle this\n";
        }
    }

    // String scan, when the format is NONE/NONE we can just walk
    // the bytes after the xex header.
    bool scanStrings = false;
    std::uint32_t scanStart = headerSize;
    std::uint32_t scanEnd = static_cast<std::uint32_t>(bytes.size());
    if (fmtOffset != 0 && fmtOffset + 8 <= bytes.size())
    {
        const std::uint16_t enc = beU16(bytes.data() + fmtOffset + 4);
        const std::uint16_t cmp = beU16(bytes.data() + fmtOffset + 6);
        scanStrings = (enc == 0 && cmp == 0);
    }
    if (scanStrings)
    {
        std::vector<std::string> keywords;
        if (argc > 2)
            for (int i = 2; i < argc; ++i) keywords.emplace_back(argv[i]);
        else
        {
            keywords = {
                "ui_result", "ui_result_ex", "ui_itemresult",
                "result_num_", "cts_result", "result_window",
                "bgm_sys_result",
                "HudResult", "CHudResult", "CResult",
                "ui_balloon", "ui_townscreen", "ui_shop", "ui_mediaroom",
                "CTownScreen", "CBalloon",
                // Sanity: things we already know are in the image
                "ui_pause", "ui_general", "ui_title", "ui_loading",
                "ui_playscreen", "ui_worldmap",
            };
        }
        std::cout << "\nscanning bytes [0x" << std::hex << scanStart
                  << "..0x" << scanEnd << std::dec
                  << "] for printable ASCII strings (>=6 chars) matching "
                  << keywords.size() << " keywords...\n";

        const auto isPrintable = [](unsigned char c) noexcept {
            return c >= 0x20 && c < 0x7F;
        };
        std::string current;
        std::size_t startOffset = 0;
        int total = 0;
        for (std::size_t i = scanStart; i < scanEnd; ++i)
        {
            const auto c = bytes[i];
            if (isPrintable(c))
            {
                if (current.empty()) startOffset = i;
                current.push_back(static_cast<char>(c));
            }
            else
            {
                if (current.size() >= 6)
                {
                    for (const auto& kw : keywords)
                    {
                        if (current.find(kw) != std::string::npos)
                        {
                            // Approximate VA: file offset - headerSize + imageBase.
                            // Real layout is more complex (PE-style sections after
                            // headerSize) but this gets us close enough to grep
                            // ppc_recomp.*.cpp for `lis r* + addi r*` immediates.
                            const std::size_t fileOff = startOffset;
                            const std::uint32_t va =
                                imageBase + static_cast<std::uint32_t>(fileOff - scanStart);
                            std::cout << "  [match] file=0x" << std::hex << fileOff
                                      << " va~=0x" << va << std::dec
                                      << " str=\"" << current << "\"\n";
                            ++total;
                            break;
                        }
                    }
                }
                current.clear();
            }
        }
        std::cout << "\ntotal matches: " << total << "\n";
    }
    else
    {
        std::cout << "skipping string scan (compression/encryption present)\n";
    }
    return 0;
}
