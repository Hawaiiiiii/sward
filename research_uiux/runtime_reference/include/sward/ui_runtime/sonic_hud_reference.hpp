#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime
{
struct SonicHudOwnerReference
{
    std::string ownerType;
    std::string ownerHook;
    std::string projectName;
    std::string readyEvent;
    int sceneCount = 0;
    int runtimeLayerCount = 0;
    int drawableLayerCount = 0;
    std::string evidence;
};

struct SonicHudMaterialSlot
{
    std::string slotId;
    std::string textureName;
    std::string placeholderFamily;
    std::string sgfxBinding;
    bool swappable = true;
};

struct SonicHudTimelineChannel
{
    std::string animationName;
    int sampleFrame = 0;
    int frameCount = 0;
    double durationSeconds = 0.0;
    std::vector<std::string> channelRoles;
};

struct SonicHudScenePolicy
{
    std::string scenePath;
    std::string sceneName;
    std::string sgfxSlot;
    std::string activationEvent;
    int renderOrder = 0;
    int runtimeLayerCount = 0;
    int drawableLayerCount = 0;
    int structuralLayerCount = 0;
    SonicHudTimelineChannel timeline;
    std::vector<SonicHudMaterialSlot> materialSlots;
    std::vector<std::string> statePolicy;
};

struct SonicHudTimelineSample
{
    std::string sceneName;
    std::string animationName;
    int frame = 0;
    int frameCount = 0;
    double progress = 0.0;
    bool clamped = false;
};

[[nodiscard]] const SonicHudOwnerReference& sonicHudOwnerReference();
[[nodiscard]] const std::vector<SonicHudScenePolicy>& sonicHudScenePolicies();
[[nodiscard]] const SonicHudScenePolicy* findSonicHudScenePolicy(std::string_view sceneNameOrPath);
[[nodiscard]] SonicHudTimelineSample sampleSonicHudTimeline(const SonicHudScenePolicy& scene, int frame);
[[nodiscard]] std::string formatSonicHudReferenceCatalog();
[[nodiscard]] std::string formatSonicHudSceneDetail(const SonicHudScenePolicy& scene);
} // namespace sward::ui_runtime
