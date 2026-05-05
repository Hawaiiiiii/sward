// Phase 275 / 281 smoke test + JSON dumper for CSD project assets.
//
// Stand-alone translation unit; compile via the wrapping PowerShell
// script `build_sgfx_hud_smoke_tests.ps1`. Two output modes:
//
//   * Default: human-readable trace of the loader's findings.
//   * `--json`: machine-readable JSON document on stdout, used by the
//     Phase 281 parity validator (`parity_check_csd_loader.py`) to
//     compare the C++ loader's parsed scene set against the YNCP
//     native component map's ground truth across every retail asset.

#include "sward/ui_runtime/sgfx_hud_csd_project_loader.hpp"

#include <iostream>
#include <string>

static std::string jsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s)
    {
        switch (c)
        {
        case '\\': out += "\\\\"; break;
        case '"':  out += "\\\""; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
                out += '?';
            else
                out += c;
        }
    }
    return out;
}

static void emitJson(const sward::ui_runtime::generated::sgfx_hud::CsdProjectFile& loaded)
{
    std::cout << "{\n";
    std::cout << "  \"sourcePath\": \"" << jsonEscape(loaded.sourcePath.string()) << "\",\n";
    std::cout << "  \"fileSizeBytes\": " << loaded.fileSizeBytes << ",\n";
    std::cout << "  \"outerMagic\": \"";
    for (char c : loaded.outerMagicChars) std::cout << (c >= 0x20 && c < 0x7F ? c : '.');
    std::cout << "\",\n";
    std::cout << "  \"hasRecognizedMagic\": " << (loaded.hasRecognizedMagic() ? "true" : "false") << ",\n";
    std::cout << "  \"loadStatus\": \"" << jsonEscape(loaded.loadStatus) << "\",\n";
    std::cout << "  \"parseStatus\": \"" << jsonEscape(loaded.parseStatus) << "\",\n";
    std::cout << "  \"projectName\": \"" << jsonEscape(loaded.projectName) << "\",\n";
    std::cout << "  \"ncpjSignature\": \"" << jsonEscape(loaded.ncpjSignature) << "\",\n";
    std::cout << "  \"rootSceneIds\": [";
    for (std::size_t i = 0; i < loaded.rootSceneIds.size(); ++i)
    {
        const auto& sid = loaded.rootSceneIds[i];
        std::cout << (i ? "," : "") << "\n    {\"index\": " << sid.index
                  << ", \"name\": \"" << jsonEscape(sid.name) << "\"}";
    }
    std::cout << (loaded.rootSceneIds.empty() ? "]" : "\n  ]") << ",\n";
    std::cout << "  \"allSceneRefs\": [";
    for (std::size_t i = 0; i < loaded.allSceneRefs.size(); ++i)
    {
        const auto& ref = loaded.allSceneRefs[i];
        std::cout << (i ? "," : "") << "\n    {\"nodePath\": \"" << jsonEscape(ref.nodePath)
                  << "\", \"name\": \"" << jsonEscape(ref.name)
                  << "\", \"index\": " << ref.index
                  << ", \"castCount\": " << ref.metadata.castCount
                  << ", \"castGroupCount\": " << ref.metadata.castGroupCount
                  << ", \"animationCount\": " << ref.metadata.animationCount
                  << ", \"animationFramerate\": " << ref.metadata.animationFramerate
                  << ", \"aspectRatio\": " << ref.metadata.aspectRatio
                  << ", \"subimageCount\": " << ref.metadata.subimages.size()
                  << ", \"subimages\": [";
        for (std::size_t j = 0; j < ref.metadata.subimages.size(); ++j)
        {
            const auto& sub = ref.metadata.subimages[j];
            std::cout << (j ? "," : "")
                      << "\n      {\"texIdx\": " << sub.textureIndex
                      << ", \"u0\": " << sub.topLeftU
                      << ", \"v0\": " << sub.topLeftV
                      << ", \"u1\": " << sub.bottomRightU
                      << ", \"v1\": " << sub.bottomRightV << "}";
        }
        std::cout << (ref.metadata.subimages.empty() ? "]" : "\n    ]") << "}";
    }
    std::cout << (loaded.allSceneRefs.empty() ? "]" : "\n  ]") << ",\n";
    std::cout << "  \"textureNames\": [";
    for (std::size_t i = 0; i < loaded.textureNames.size(); ++i)
    {
        std::cout << (i ? "," : "") << "\n    \""
                  << jsonEscape(loaded.textureNames[i]) << "\"";
    }
    std::cout << (loaded.textureNames.empty() ? "]" : "\n  ]") << "\n";
    std::cout << "}\n";
}

int main(int argc, char** argv)
{
    using namespace sward::ui_runtime::generated::sgfx_hud;

    bool jsonOutput = false;
    const char* path = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--json")
            jsonOutput = true;
        else
            path = argv[i];
    }
    if (path == nullptr)
    {
        std::cerr << "usage: " << argv[0] << " [--json] <path-to-yncp-or-xncp-file>\n";
        return 2;
    }

    const CsdProjectFile loaded = loadCsdProjectFile(path);

    if (jsonOutput)
    {
        emitJson(loaded);
        return loaded.hasRecognizedMagic() ? 0 : 1;
    }

    std::cout << "sourcePath:       " << loaded.sourcePath.string() << "\n";
    std::cout << "fileSizeBytes:    " << loaded.fileSizeBytes << "\n";
    std::cout << "outerMagicChars:  ";
    for (char c : loaded.outerMagicChars)
        std::cout << (c >= 0x20 && c < 0x7F ? c : '.');
    std::cout << "\n";
    std::cout << "outerMagic enum:  " << static_cast<int>(loaded.outerMagic)
              << " (0=Unknown, 1=Cpaf, 2=Fapc, 3=Yncp, 4=Xncp)\n";
    std::cout << "hasRecognizedMagic: " << (loaded.hasRecognizedMagic() ? "true" : "false") << "\n";
    std::cout << "loadStatus:       " << loaded.loadStatus << "\n";
    std::cout << "parseStatus:      " << loaded.parseStatus << "\n";
    std::cout << "projectName:      " << loaded.projectName << "\n";
    std::cout << "ncpjSignature:    " << loaded.ncpjSignature << "\n";
    std::cout << "rootSceneIds:     count=" << loaded.rootSceneIds.size() << "\n";
    for (const auto& sid : loaded.rootSceneIds)
        std::cout << "    [" << sid.index << "] " << sid.name << "\n";
    std::cout << "allSceneRefs:     count=" << loaded.allSceneRefs.size() << "\n";
    for (const auto& ref : loaded.allSceneRefs)
    {
        std::cout << "    [" << ref.index << "] "
                  << (ref.nodePath.empty() ? "" : ref.nodePath + "/") << ref.name
                  << " (casts=" << ref.metadata.castCount
                  << ", groups=" << ref.metadata.castGroupCount
                  << ", anims=" << ref.metadata.animationCount
                  << ", subimages=" << ref.metadata.subimages.size()
                  << ", aspect=" << ref.metadata.aspectRatio
                  << ")\n";
    }
    std::cout << "textureNames:     count=" << loaded.textureNames.size() << "\n";
    for (std::size_t i = 0; i < loaded.textureNames.size(); ++i)
        std::cout << "    [" << i << "] " << loaded.textureNames[i] << "\n";

    return loaded.hasRecognizedMagic() ? 0 : 1;
}
