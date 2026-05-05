// Phase 280 smoke test: drive the ported `class CHudSonicStage` method
// bodies against a default-constructed instance and against a real
// extracted Sonic `ui_playscreen.yncp` asset.
//
// Compile + run via `research_uiux/runtime_reference/tools/build_sgfx_hud_smoke_tests.ps1`.

#include "sward/ui_runtime/sgfx_hud_chud_sonic_stage.generated.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

// Method-body declarations live alongside the implementations in
// `sgfx_hud_chud_sonic_stage_methods.cpp`; declare them here so this
// translation unit can link against them without pulling in a separate
// header. (A future phase will move them to a dedicated `_methods.hpp`.)
namespace sward::ui_runtime::generated::sgfx_hud
{
    bool isCHudSonicStageInPostConstructorState(const CHudSonicStage& hud) noexcept;
    void releaseAllOwnedScenes(CHudSonicStage& hud) noexcept;
    bool isPlayScreenAssetSideReadyForBinding(const std::filesystem::path& playScreenYncpPath) noexcept;
    const SceneBinding* findSceneBindingByMember(std::string_view memberName) noexcept;
    bool hasFullGaugeClusterBindings() noexcept;
    std::size_t countCrossValidatedBindings() noexcept;
    std::string_view ownedCsdProjectName() noexcept;
}

int main(int argc, char** argv)
{
    using namespace sward::ui_runtime::generated::sgfx_hud;

    int failures = 0;
    auto check = [&failures](const char* label, bool condition) {
        std::cout << (condition ? "OK   " : "FAIL ") << label << "\n";
        if (!condition) ++failures;
    };

    // Default-constructed `CHudSonicStage` should match the recomp
    // post-constructor state: every RCPtr in the empty (0, 0) form.
    CHudSonicStage hud{};
    check("default-construct -> isCHudSonicStageInPostConstructorState",
        isCHudSonicStageInPostConstructorState(hud));

    // Set every RCPtr to non-zero, run the destructor-port helper, and
    // confirm the slots return to the empty state. Mirrors the recomp
    // destructor's `sub_830BA1D8` walk (Phase 280).
    hud.m_rcPlayScreen.m_pMemory      = 0xDEAD0001;
    hud.m_rcSpeedGauge.m_pMemory      = 0xDEAD0002;
    hud.m_rcRingEnergyGauge.m_pMemory = 0xDEAD0003;
    hud.m_rcPtrField180.m_pMemory     = 0xDEAD0180;
    check("pre-release -> not in post-constructor state",
        !isCHudSonicStageInPostConstructorState(hud));
    releaseAllOwnedScenes(hud);
    check("post-release -> back in post-constructor state",
        isCHudSonicStageInPostConstructorState(hud));

    // Static binding registry queries (Phase 277).
    check("findSceneBindingByMember(\"m_rcSpeedGauge\") != nullptr",
        findSceneBindingByMember("m_rcSpeedGauge") != nullptr);
    check("findSceneBindingByMember(\"m_rcDoesNotExist\") == nullptr",
        findSceneBindingByMember("m_rcDoesNotExist") == nullptr);
    check("hasFullGaugeClusterBindings()",
        hasFullGaugeClusterBindings());
    check("countCrossValidatedBindings() >= 4",
        countCrossValidatedBindings() >= 4);
    check("ownedCsdProjectName() == \"ui_playscreen\"",
        ownedCsdProjectName() == "ui_playscreen");

    // Asset-side readiness (Phases 275 + 279 + 280). Skipped if no
    // path argument is provided; otherwise the smoke test verifies that
    // every cross-validated binding's scene name is present in the
    // CPAF-parsed root scene list.
    if (argc >= 2)
    {
        const std::filesystem::path playScreen{argv[1]};
        check("isPlayScreenAssetSideReadyForBinding(<real ui_playscreen.yncp>)",
            isPlayScreenAssetSideReadyForBinding(playScreen));
    }
    else
    {
        std::cout << "SKIP no path argument; pass extracted ui_playscreen.yncp to test asset readiness\n";
    }

    std::cout << "failures: " << failures << "\n";
    return failures == 0 ? 0 : 1;
}
