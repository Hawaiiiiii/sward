#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace sward::ui_runtime
{
struct FrontendScreenMaterialSlot
{
    std::string slotId;
    std::string textureName;
    std::string placeholderFamily;
    std::string sgfxBinding;
    bool swappable = true;
};

struct FrontendScreenTimelineChannel
{
    std::string animationName;
    int sampleFrame = 0;
    int frameCount = 0;
    double durationSeconds = 0.0;
    std::vector<std::string> channelRoles;
};

struct FrontendScreenScenePolicy
{
    std::string scenePath;
    std::string sceneName;
    std::string sgfxSlot;
    std::string activationEvent;
    int renderOrder = 0;
    int drawableCommandCount = 0;
    int structuralCommandCount = 0;
    int sourceFreeStructuralCommandCount = 0;
    FrontendScreenTimelineChannel timeline;
    std::vector<std::string> statePolicy;
};

struct FrontendScreenPolicy
{
    std::string screenId;
    std::string screenName;
    std::string layoutName;
    std::string referenceContract;
    std::string activationEvent;
    std::string transitionBand;
    std::string inputLockTiming;
    std::string renderOrderPolicy;
    std::vector<FrontendScreenMaterialSlot> materialSlots;
    std::vector<FrontendScreenScenePolicy> scenes;
};

struct FrontendScreenTimelineSample
{
    std::string screenId;
    std::string sceneName;
    std::string animationName;
    int frame = 0;
    int frameCount = 0;
    double progress = 0.0;
    bool clamped = false;
};

[[nodiscard]] const std::vector<FrontendScreenPolicy>& frontendScreenPolicies();
[[nodiscard]] const FrontendScreenPolicy* findFrontendScreenPolicy(std::string_view screenIdOrName);
[[nodiscard]] const FrontendScreenScenePolicy* findFrontendScreenScenePolicy(
    const FrontendScreenPolicy& screen,
    std::string_view sceneNameOrPath);
[[nodiscard]] FrontendScreenTimelineSample sampleFrontendScreenTimeline(
    const FrontendScreenPolicy& screen,
    const FrontendScreenScenePolicy& scene,
    int frame);
[[nodiscard]] std::string formatFrontendScreenReferenceCatalog();
[[nodiscard]] std::string formatFrontendScreenReferenceDetail(const FrontendScreenPolicy& screen);
[[nodiscard]] std::string formatFrontendScreenSceneDetail(
    const FrontendScreenPolicy& screen,
    const FrontendScreenScenePolicy& scene);
} // namespace sward::ui_runtime
