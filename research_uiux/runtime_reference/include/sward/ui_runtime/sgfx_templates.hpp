#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime
{
struct SgfxEvidenceSource
{
    std::string lane;
    std::string runtimeTarget;
    std::vector<std::string> requiredEvents;
    std::vector<std::string> proofArtifacts;
    std::vector<std::string> localEvidenceSessions;
};

struct SgfxTimelineBand
{
    std::string id;
    double seconds = 0.0;
    std::string source;
};

struct SgfxLayerRole
{
    std::string runtimeRole;
    std::string sgfxRole;
    bool interactive = false;
};

struct SgfxAssetPolicy
{
    std::string renderIntent;
    std::string placeholderAssetFamily;
    std::string finalAssetFamily;
    std::vector<std::string> swappableSlots;
    std::vector<std::string> constraints;
};

struct SgfxScreenTemplate
{
    std::string id;
    std::string displayName;
    std::string contractFileName;
    std::string primaryRuntimeTarget;
    std::string sourceFamily;
    std::vector<std::string> recoveredSourcePaths;
    std::vector<SgfxTimelineBand> timelineBands;
    std::vector<SgfxLayerRole> layerRoles;
    std::vector<std::string> stateFlow;
    std::vector<std::string> promptPolicy;
    SgfxAssetPolicy assetPolicy;
    SgfxEvidenceSource evidence;
    std::vector<std::string> sgfxAdaptationNotes;
};

[[nodiscard]] const std::vector<SgfxScreenTemplate>& sgfxScreenTemplates();
[[nodiscard]] const SgfxScreenTemplate* findSgfxScreenTemplate(std::string_view id);
[[nodiscard]] std::string formatSgfxTemplateCatalog();
[[nodiscard]] std::string formatSgfxTemplateDetail(const SgfxScreenTemplate& screenTemplate);
} // namespace sward::ui_runtime
