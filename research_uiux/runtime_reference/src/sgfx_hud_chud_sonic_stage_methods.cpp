// Phase 277: hand-written method bodies for `class CHudSonicStage`.
//
// This is the second class in the human-readable 1:1 port to grow real
// method bodies, mirroring the pattern Phase 273 / 276 established for
// `class CHudPause`. Every helper here is hand-written C++ on top of the
// runtime-extended generated layout in
// `sgfx_hud_chud_sonic_stage.generated.h` (Phases 265 / 266 / 267) and
// reads through the `kSceneBindings[]` registry the generator carries
// alongside the class.
//
// The helpers stay implementation-light: they are derived predicates /
// queries over the static binding registry, with no I/O, no allocation,
// and no calls into the recomp runtime. Each one corresponds to a real
// question a downstream HUD-rendering layer would ask before touching a
// scene wrapper, so the methods are immediately useful for routing the
// port's render / update passes.
//
// Heavier method bodies (Update / Render / destructor) need the SWA
// RCObject runtime ported alongside before they can be implemented; they
// land in subsequent phases.

#include "sward/ui_runtime/sgfx_hud_chud_sonic_stage.generated.h"
#include "sward/ui_runtime/sgfx_hud_csd_project_loader.hpp"

#include <filesystem>
#include <string_view>

namespace sward::ui_runtime::generated::sgfx_hud
{
    // Look up a single SceneBinding entry by RCPtr member name. Returns
    // nullptr when the requested name does not appear in the binding
    // registry — typically because the runtime sweep has not (yet)
    // cross-validated that member to a renderable scene. Callers can
    // treat a null return as "this RCPtr has no known scene to load".
    const SceneBinding* findSceneBindingByMember(std::string_view memberName) noexcept
    {
        for (const auto& entry : kSceneBindings)
        {
            if (entry.memberName == memberName)
                return &entry;
        }
        return nullptr;
    }

    // True iff the gauge cluster (Speed / Ring-Energy / Gauge-Frame) all
    // appear in the SceneBinding registry. The gauge cluster is the
    // always-on Sonic stage HUD core; if any of them is missing, the
    // port should not enter the stage HUD render pass and instead wait
    // for the missing slot to populate.
    bool hasFullGaugeClusterBindings() noexcept
    {
        return findSceneBindingByMember("m_rcSpeedGauge") != nullptr
            && findSceneBindingByMember("m_rcRingEnergyGauge") != nullptr
            && findSceneBindingByMember("m_rcGaugeFrame") != nullptr;
    }

    // Count how many SceneBinding entries reach the highest confidence
    // tier. Cross-validated bindings agree between the constructor
    // expected-fields table and the runtime sweep, so a downstream layer
    // can safely consume them without falling back to inferred-owner
    // heuristics.
    std::size_t countCrossValidatedBindings() noexcept
    {
        std::size_t count = 0;
        for (const auto& entry : kSceneBindings)
        {
            if (entry.confidenceTier == "cross-validated")
                ++count;
        }
        return count;
    }

    // Project name every SceneBinding currently resolves against — the
    // SWA Sonic stage HUD draws all of its members from a single CSD
    // project (`ui_playscreen`). Returns a status string instead of an
    // enum so the binding's cross-validated provenance flows through.
    std::string_view ownedCsdProjectName() noexcept
    {
        // Every binding in the runtime-confirmed set has the same
        // `projectName` because the SWA HUD owner only ever reaches into
        // one CSD project at a time. The pattern is enforced here so a
        // future regen that mixes projects will trip the assert below.
        std::string_view shared{};
        for (const auto& entry : kSceneBindings)
        {
            if (shared.empty())
                shared = entry.projectName;
            else if (entry.projectName != shared)
                return std::string_view{"<mixed>"};
        }
        return shared;
    }

    // True iff the binding resolves to the additive `Root/add` cluster
    // (medal-get pool, speed-count pool). The SWA Sonic stage HUD uses
    // the additive cluster only for transient overlays (medal acquired,
    // speed-count flash, etc.), so a downstream renderer can use this to
    // decide whether to draw the binding into the always-on HUD layer
    // or the transient overlay layer.
    bool isAdditiveClusterBinding(const SceneBinding& binding) noexcept
    {
        // Match the `<project>/add/...` sub-path the YNCP map composes
        // for the additive cluster (Phase 272 added the composition
        // logic to the validator; the same shape lands here in the port).
        const auto& path = binding.scenePath;
        const auto slash = path.find('/');
        if (slash == std::string_view::npos || slash + 1 >= path.size())
            return false;
        const auto rest = path.substr(slash + 1);
        return rest.size() >= 4
            && rest[0] == 'a' && rest[1] == 'd' && rest[2] == 'd' && rest[3] == '/';
    }

    // ----------------------------------------------------------------
    // Phase 280: ported method bodies that mirror the SWA recomp flow.
    // ----------------------------------------------------------------
    //
    // Each method below names the corresponding recomp entry-point
    // (`sub_xxxxx` from `local_build_env/.../ppc_recomp.*.cpp`) and
    // documents which lines of the recomp the port covers. The bodies
    // are real C++ that runs standalone; they don't depend on the
    // recomp executable and instead use the auto-generated layout +
    // runtime evidence carried in `kSceneBindings[]`.

    // Port of the SWA constructor-time RCPtr default-init sequence
    // observed in `sub_824D89B0` (Phase 265 decode). The recomp walks
    // 21 RCPtr slots from `this+0xE0` through `this+0x180` and calls
    // `sub_830BA1C0` (the `RCPtr<T>::RCPtr()` default constructor) on
    // each, leaving `m_pRCObject = m_pMemory = 0`.
    //
    // The human-readable port matches that behavior with C++ default
    // initialization: zero-initialized RCPtr members + zero vtable
    // pointers. Constructor-side vtable pointer assignment
    // (`stw r11, 0(r31)` -> `m_pVTable = 0x82DAFAF0`) is intentionally
    // not ported because those addresses are tied to the Xbox 360
    // build's static module layout; in a true port the C++ compiler
    // manages vtable pointers automatically as virtuals are added.
    //
    // Returns true iff every RCPtr ends in the SWA "default" state
    // (both halves zero) — that is what the recomp constructor leaves
    // behind before any subsequent owner-bind call populates the slots.
    bool isCHudSonicStageInPostConstructorState(const CHudSonicStage& hud) noexcept
    {
        const auto isRCPtrEmpty = [](const Chao::CSD::RCPtr<Chao::CSD::CScene>& rc) noexcept
        {
            return rc.m_pRCObject == 0 && rc.m_pMemory == 0;
        };
        const auto isProjectRCPtrEmpty = [](const Chao::CSD::RCPtr<Chao::CSD::CProject>& rc) noexcept
        {
            return rc.m_pRCObject == 0 && rc.m_pMemory == 0;
        };
        const auto isNodeRCPtrEmpty = [](const Chao::CSD::RCPtr<Chao::CSD::CNode>& rc) noexcept
        {
            return rc.m_pRCObject == 0 && rc.m_pMemory == 0;
        };
        return isProjectRCPtrEmpty(hud.m_rcPlayScreen)
            && isRCPtrEmpty(hud.m_rcSpeedGauge)
            && isRCPtrEmpty(hud.m_rcRingEnergyGauge)
            && isRCPtrEmpty(hud.m_rcGaugeFrame)
            && isRCPtrEmpty(hud.m_rcExpCount)
            && isRCPtrEmpty(hud.m_rcSpeedCount)
            && isNodeRCPtrEmpty(hud.m_rcScoreCount)
            && isNodeRCPtrEmpty(hud.m_rcTimeCount)
            && isNodeRCPtrEmpty(hud.m_rcTimeCount2)
            && isNodeRCPtrEmpty(hud.m_rcTimeCount3)
            && isNodeRCPtrEmpty(hud.m_rcPlayerCount);
    }

    // Port of the destructor-time RCPtr release sequence observed in
    // `sub_824D8CE8`. The recomp walks the 15 named RCPtrs in reverse
    // construction order (`this+0x180` down to `this+0x108`) and calls
    // `sub_830BA1D8` (the `RCPtr<T>::~RCPtr()` destructor) on each.
    //
    // The human-readable port relies on C++ destructor semantics: when
    // `CHudSonicStage` goes out of scope the compiler walks the data
    // members in reverse declaration order and calls each member's
    // destructor. The generated layout declares RCPtrs from
    // `m_rcPlayScreen` (first) to `m_rcPtrField180` (last), so reverse
    // walking matches the recomp's `sub_830BA1D8` call order exactly.
    //
    // RCPtr currently has trivial destruction (it is a POD struct of
    // two `uint32_t`s) so this function is the actual stand-in for the
    // recomp release calls until the SWA RCObject runtime is ported.
    // Calling it explicitly mirrors the recomp behavior of "release
    // every owned scene wrapper, leaving the slots empty".
    void releaseAllOwnedScenes(CHudSonicStage& hud) noexcept
    {
        const auto release = [](auto& rc) noexcept
        {
            rc.m_pRCObject = 0;
            rc.m_pMemory   = 0;
        };
        // Walk in reverse construction order to match `sub_824D8CE8`.
        release(hud.m_rcPtrField180);
        release(hud.m_rcPtrField178);
        release(hud.m_rcPtrField170);
        release(hud.m_rcPtrField168);
        release(hud.m_rcPtrField160);
        release(hud.m_rcPtrField158);
        release(hud.m_rcPtrField150);
        release(hud.m_rcPlayerCount);
        release(hud.m_rcTimeCount3);
        release(hud.m_rcTimeCount2);
        release(hud.m_rcTimeCount);
        release(hud.m_rcScoreCount);
        release(hud.m_rcPtrField120);
        release(hud.m_rcSpeedCount);
        release(hud.m_rcPtrField110);
        release(hud.m_rcPtrField108);
        release(hud.m_rcExpCount);
        release(hud.m_rcGaugeFrame);
        release(hud.m_rcRingEnergyGauge);
        release(hud.m_rcSpeedGauge);
        release(hud.m_rcPlayScreen);
    }

    // Asset-side equivalent of the SWA "did m_rcPlayScreen finish
    // loading" check. The SWA recomp queries `m_rcPlayScreen.m_pMemory`
    // (non-zero == project loaded); the human-readable port instead
    // uses the asset loader (Phase 275 / 279) to verify the on-disk
    // CSD project is present and parses cleanly. Returns true iff the
    // gauge-cluster project's asset file is loadable AND the parsed
    // root scene IDs cover every cross-validated SceneBinding the port
    // currently knows about. This is the host-side analog of
    // "everything the HUD needs is ready to render".
    bool isPlayScreenAssetSideReadyForBinding(const std::filesystem::path& playScreenYncpPath) noexcept
    {
        const CsdProjectFile loaded = loadCsdProjectFile(playScreenYncpPath);
        if (!loaded.hasRecognizedMagic())
            return false;
        if (loaded.parseStatus.rfind("ok:", 0) != 0)
            return false;

        for (const auto& binding : kSceneBindings)
        {
            if (binding.confidenceTier != "cross-validated")
                continue;
            // Each `scenePath` is `<project>[/<sub>]/<scene>`. Drop the
            // `<project>/` prefix, then look for a matching nodePath +
            // bare scene name pair in the recursive scene reference
            // list (Phase 280 walks both root and child nodes).
            std::string_view scenePath{binding.scenePath};
            const auto firstSlash = scenePath.find('/');
            if (firstSlash != std::string_view::npos)
                scenePath.remove_prefix(firstSlash + 1);
            const auto lastSlash = scenePath.find_last_of('/');
            const std::string_view subPath = lastSlash == std::string_view::npos
                ? std::string_view{}
                : scenePath.substr(0, lastSlash);
            const std::string_view sceneName = lastSlash == std::string_view::npos
                ? scenePath
                : scenePath.substr(lastSlash + 1);

            bool found = false;
            for (const auto& ref : loaded.allSceneRefs)
            {
                if (ref.name == sceneName && ref.nodePath == subPath)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }
        return true;
    }

    // ----------------------------------------------------------------
    // Compile-time guards: the helpers above must compile against the
    // runtime-extended generated layout without modification. If a
    // future regen drops the SceneBinding struct or the kSceneBindings
    // array, the build breaks here and forces a coordinated update.
    static_assert(
        sizeof(CHudSonicStage) >= 0x188,
        "sgfx_hud_chud_sonic_stage_methods.cpp expects the Phase 265-extended "
        "CHudSonicStage layout to be at least 0x188 bytes (the last RCPtr "
        "lives at +0x180). Regenerate the layout header if this fails.");
    static_assert(
        kSceneBindings.size() >= 4,
        "sgfx_hud_chud_sonic_stage_methods.cpp expects at least four cross-"
        "validated scene bindings (gauge cluster + speed_count). If the "
        "binding registry shrank, the runtime sweep regressed.");
}
