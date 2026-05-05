// Phase 275 smoke test: drive `loadCsdProjectFile()` against a real
// extracted Sonic Unleashed asset and print the result. Stand-alone
// translation unit; compile and run from the repo root with:
//
//   g++ -std=c++17 -I research_uiux/runtime_reference/include \
//       research_uiux/runtime_reference/src/sgfx_hud_csd_project_loader_smoke_test.cpp \
//       -o /tmp/sgfx_hud_csd_project_loader_smoke_test
//   ./tmp/sgfx_hud_csd_project_loader_smoke_test \
//       extracted_assets/full_install_archives/game/Sonic/ui_playscreen.yncp
//
// Expected output: `loadStatus = ok: CSD project asset loaded with
// recognizable magic` and the inner YNCP magic offset equal to 11
// (matches the user's extraction).

#include "sward/ui_runtime/sgfx_hud_csd_project_loader.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    using namespace sward::ui_runtime::generated::sgfx_hud;

    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <path-to-yncp-or-xncp-file>\n";
        return 2;
    }

    const CsdProjectFile loaded = loadCsdProjectFile(argv[1]);

    std::cout << "sourcePath:       " << loaded.sourcePath.string() << "\n";
    std::cout << "fileSizeBytes:    " << loaded.fileSizeBytes << "\n";
    std::cout << "outerMagicChars:  ";
    for (char c : loaded.outerMagicChars)
        std::cout << (c >= 0x20 && c < 0x7F ? c : '.');
    std::cout << "\n";
    std::cout << "outerMagic enum:  " << static_cast<int>(loaded.outerMagic)
              << " (0=Unknown, 1=Cpaf, 2=Yncp, 3=Xncp)\n";
    std::cout << "hasRecognizedMagic: " << (loaded.hasRecognizedMagic() ? "true" : "false") << "\n";
    std::cout << "hasYncpPayload:   " << (loaded.hasYncpPayload() ? "true" : "false") << "\n";
    std::cout << "hasXncpPayload:   " << (loaded.hasXncpPayload() ? "true" : "false") << "\n";
    if (loaded.innerYncpMagicOffset)
        std::cout << "innerYncpOffset:  " << *loaded.innerYncpMagicOffset << "\n";
    if (loaded.innerXncpMagicOffset)
        std::cout << "innerXncpOffset:  " << *loaded.innerXncpMagicOffset << "\n";
    std::cout << "loadStatus:       " << loaded.loadStatus << "\n";

    const bool ok = loaded.hasRecognizedMagic()
        && (loaded.hasYncpPayload() || loaded.hasXncpPayload());
    return ok ? 0 : 1;
}
