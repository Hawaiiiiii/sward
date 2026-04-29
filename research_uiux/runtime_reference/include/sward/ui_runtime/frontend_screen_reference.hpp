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

struct FrontendScreenMaterialSemantics
{
    std::string blendModel;
    std::string alphaModel;
    std::string colorModel;
    std::string filteringModel;
    std::string pixelOffsetModel;
    std::string oraclePolicy;
};

struct FrontendScreenMediaCue
{
    std::string cueId;
    std::string cueKind;
    std::string sgfxSlot;
    std::string assetName;
    double startSeconds = 0.0;
    double durationSeconds = 0.0;
    std::string timingSource;
    std::string evidenceStatus;
    bool runtimeVisualProven = false;
    bool audioPending = false;
};

struct FrontendScreenMediaCueCounts
{
    int movie = 0;
    int text = 0;
    int glyph = 0;
    int fade = 0;
    int sfx = 0;
    int visualProven = 0;
    int audioPending = 0;
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
    FrontendScreenMaterialSemantics materialSemantics;
    std::vector<FrontendScreenMaterialSlot> materialSlots;
    std::vector<FrontendScreenMediaCue> mediaCues;
    std::vector<FrontendScreenScenePolicy> scenes;
};

struct FrontendRuntimeAlignment
{
    std::string screenId;
    std::vector<std::string> activeScenes;
    std::string activeMotionName;
    int activeFrame = 0;
    std::string cursorOwner;
    std::string transitionBand;
    std::string inputLockState;
    std::string source;
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
[[nodiscard]] FrontendRuntimeAlignment defaultFrontendRuntimeAlignment(const FrontendScreenPolicy& screen);
[[nodiscard]] FrontendScreenTimelineSample sampleFrontendScreenTimeline(
    const FrontendScreenPolicy& screen,
    const FrontendScreenScenePolicy& scene,
    int frame);
[[nodiscard]] FrontendScreenMediaCueCounts frontendScreenMediaCueCounts(const FrontendScreenPolicy& screen);
[[nodiscard]] std::string formatFrontendScreenReferenceCatalog();
[[nodiscard]] std::string formatFrontendScreenReferenceDetail(const FrontendScreenPolicy& screen);
[[nodiscard]] std::string formatFrontendScreenSceneDetail(
    const FrontendScreenPolicy& screen,
    const FrontendScreenScenePolicy& scene);
[[nodiscard]] std::string formatFrontendScreenMediaTimingCatalog();
[[nodiscard]] std::string formatFrontendScreenMediaCueDetail(
    const FrontendScreenPolicy& screen,
    const FrontendScreenMediaCue& cue);
[[nodiscard]] std::string formatFrontendRuntimeAlignment(const FrontendRuntimeAlignment& alignment);
} // namespace sward::ui_runtime
